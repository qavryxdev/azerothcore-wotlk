#!/usr/bin/env python3
"""Rotation coverage matrix for playerbot PvP: my class/spec x enemy archetype x situation.

For every bot spec this works out which abilities are actually *reachable* - referenced by a
NextAction inside a strategy that AiFactory really attaches to that spec, and registered in the
class's creators map - maps them onto capabilities, and then reports every matchup cell where the
spec has no answer at all.

Why this and not a behavioural simulation: producing a real rotation decision needs a live
PlayerbotAI, Player and world. Fourteen InitTriggers bodies dereference the AI pointer, so they
cannot even be constructed with a null one. This harness therefore answers the weaker but still
decisive question - *is there any reachable answer at all* - which is what every finding of the form
"class X has no snare" reduces to.

IMPORTANT when reading the output: a spec with zero reachable abilities is far more likely to mean
this harness could not resolve its strategies than that the spec is empty. Such specs are reported
separately as UNRESOLVED, never as gaps.

Usage:
    python rotation_matrix.py --module <path to mod-playerbots> [--jobs N] [--format text|json]
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from capability_map import (  # noqa: E402
    ACTION_CAPABILITIES,
    ALL_CAPABILITIES,
    ENEMY_ARCHETYPES,
    RANGED_SPECS,
)

RE_NEXT_ACTION = re.compile(r'NextAction\s*\(\s*"([^"]+)"')
RE_CREATOR = re.compile(r'creators\s*\[\s*"([^"]+)"\s*\]\s*=')
RE_CLASS = re.compile(r'\b(?:class|struct)\s+([A-Za-z_]\w*)')
# class Derived : public Base - a spec strategy inherits its base's triggers by calling
# Base::InitTriggers(triggers) as its first statement, so the base's actions are its own too.
# Without following this, GenericHunterStrategy's deterrence never reaches "bm"/"mm"/"surv".
RE_INHERIT = re.compile(r'\bclass\s+([A-Za-z_]\w*)\s*:\s*public\s+([A-Za-z_]\w*)')
# Strategy::getName() returns "x"  -> maps a C++ strategy class to the name AiFactory uses
RE_GET_NAME = re.compile(r'std::string\s+const\s+(\w+)::getName\s*\(\s*\)[^{]*\{\s*return\s+"([^"]+)"')
RE_GET_NAME_INLINE = re.compile(r'std::string\s+const\s+getName\s*\(\s*\)\s*override\s*\{\s*return\s+"([^"]+)"')
# addStrategiesNoInit("a", "b", nullptr) / addStrategy("a", ...)
RE_ADD_STRATEGIES = re.compile(r'addStrategiesNoInit\s*\(([^;]*?)\)\s*;', re.S)
RE_ADD_STRATEGY = re.compile(r'addStrategy\s*\(\s*"([^"]+)"')

CLASS_DIRS = {
    "warrior": "Warrior", "paladin": "Paladin", "hunter": "Hunter", "rogue": "Rogue",
    "priest": "Priest", "death knight": "Dk", "shaman": "Shaman", "mage": "Mage",
    "warlock": "Warlock", "druid": "Druid",
}

SPECS = {
    "warrior": ["arms", "fury", "protection"],
    "paladin": ["holy", "protection", "retribution"],
    "hunter": ["beast mastery", "marksmanship", "survival"],
    "rogue": ["assassination", "combat", "subtlety"],
    "priest": ["discipline", "holy", "shadow"],
    "death knight": ["blood", "frost", "unholy"],
    "shaman": ["elemental", "enhancement", "restoration"],
    "mage": ["arcane", "fire", "frost"],
    "warlock": ["affliction", "demonology", "destruction"],
    "druid": ["balance", "feral", "restoration"],
}


def scan_strategy_file(path: str) -> dict:
    """Collect, per C++ strategy class in the file, the action names it references."""
    try:
        text = open(path, "r", encoding="utf-8", errors="replace").read()
    except OSError:
        return {}

    marks = [(m.start(), m.group(1)) for m in RE_CLASS.finditer(text)]

    def enclosing(offset: int) -> str:
        name = ""
        for start, cls in marks:
            if start > offset:
                break
            name = cls
        return name

    # C++ class -> strategy name used by AiFactory
    names: dict[str, str] = {}
    for m in RE_GET_NAME.finditer(text):
        names[m.group(1)] = m.group(2)
    for m in RE_GET_NAME_INLINE.finditer(text):
        names[enclosing(m.start())] = m.group(1)

    actions: dict[str, set] = defaultdict(set)
    for m in RE_NEXT_ACTION.finditer(text):
        # A NextAction inside a method body belongs to the class that defines the method. For
        # <Class>::InitTriggers the enclosing-class scan yields the class name directly.
        owner = ""
        prefix = text[: m.start()]
        method = None
        for mm in re.finditer(r'void\s+(\w+)::(InitTriggers|getDefaultActions)', prefix):
            method = mm.group(1)
        for mm in re.finditer(r'std::vector<NextAction>\s+(\w+)::getDefaultActions', prefix):
            method = mm.group(1)
        owner = method or enclosing(m.start())
        if owner:
            actions[owner].add(m.group(1))

    bases = {m.group(1): m.group(2) for m in RE_INHERIT.finditer(text)}
    return {"path": path, "names": names, "bases": bases,
            "actions": {k: sorted(v) for k, v in actions.items()}}


def parse_ai_factory(module: str) -> dict:
    """class/spec -> strategy names attached by AiFactory (combat engine)."""
    path = os.path.join(module, "src", "Bot", "Factory", "AiFactory.cpp")
    text = open(path, "r", encoding="utf-8", errors="replace").read()

    body = text
    start = body.find("void AiFactory::AddDefaultCombatStrategies")
    end = body.find("Engine* AiFactory::createCombatEngine", start)
    combat = body[start:end] if start >= 0 else ""

    # Strategies added unconditionally to everyone.
    common = set()
    for m in RE_ADD_STRATEGIES.finditer(combat):
        # only take the ones outside the class switch, i.e. before "switch (player->getClass())"
        pass

    per_class: dict[str, set] = defaultdict(set)
    switch_start = combat.find("switch (player->getClass())")
    prologue = combat[:switch_start] if switch_start > 0 else combat
    switch_body = combat[switch_start:] if switch_start > 0 else ""

    # The battleground block sits after the class switch and adds PvP-only strategies, some of them
    # class conditional. Left inside switch_body it was attributed to whichever case happened to be
    # last, so the druid and rogue "cc" strategy - which is where entangling roots, cyclone and sap
    # live - never reached the specs that actually get it.
    bg_start = switch_body.find("if (player->InBattleground()")
    battleground = switch_body[bg_start:] if bg_start > 0 else ""
    if bg_start > 0:
        switch_body = switch_body[:bg_start]

    bg_common: set = set()
    bg_per_class: dict[str, set] = defaultdict(set)
    if battleground:
        class_guard = re.compile(r'getClass\(\)\s*==\s*CLASS_([A-Z_]+)')
        for m in list(RE_ADD_STRATEGIES.finditer(battleground)) + list(RE_ADD_STRATEGY.finditer(battleground)):
            names = re.findall(r'"([^"]+)"', m.group(0))
            # Look back a little for a class guard on the same statement or the enclosing if.
            window = battleground[max(0, m.start() - 220):m.start()]
            guarded = [g.lower().replace("_", " ") for g in class_guard.findall(window)]
            if guarded:
                for cls in guarded:
                    bg_per_class[cls].update(names)
            else:
                bg_common.update(names)
    common |= bg_common
    for cls, names in bg_per_class.items():
        per_class[cls].update(names)

    for m in RE_ADD_STRATEGIES.finditer(prologue):
        for name in re.findall(r'"([^"]+)"', m.group(1)):
            common.add(name)
    for m in RE_ADD_STRATEGY.finditer(prologue):
        common.add(m.group(1))

    # Split the switch into per-class blocks, then attribute each add call to the spec branch that
    # guards it. Without this every spec of a class collects the union of all three branches, which
    # makes the whole matrix meaningless - a holy paladin would appear to have retribution's kit.
    per_spec: dict[tuple, set] = defaultdict(set)
    case_re = re.compile(r'case\s+CLASS_([A-Z_]+):')
    tab_re = re.compile(r'tab\s*==\s*[A-Z_]*TAB_([A-Z_]+)')
    cases = [(m.start(), m.group(1)) for m in case_re.finditer(switch_body)]

    for i, (pos, cls) in enumerate(cases):
        stop = cases[i + 1][0] if i + 1 < len(cases) else len(switch_body)
        block = switch_body[pos:stop]
        key = cls.lower().replace("_", " ")

        # Brace range of every "if (tab == ...)" / "else if (tab == ...)" in this block.
        guards = []
        for gm in re.finditer(r'\b(?:else\s+)?if\s*\(([^)]*tab\s*==[^)]*)\)', block):
            tabs = [t.lower().replace("_", " ") for t in tab_re.findall(gm.group(1))]
            if not tabs:
                continue
            rest = block[gm.end():]
            stripped = rest.lstrip()
            offset = gm.end() + (len(rest) - len(stripped))
            if stripped.startswith("{"):
                depth = 0
                j = offset
                while j < len(block):
                    if block[j] == "{":
                        depth += 1
                    elif block[j] == "}":
                        depth -= 1
                        if depth == 0:
                            break
                    j += 1
                guards.append((offset, j, tabs))
            else:
                end_stmt = block.find(";", offset)
                guards.append((offset, end_stmt if end_stmt > 0 else offset, tabs))

        def specs_for(offset: int):
            for start, stop_, tabs in guards:
                if start <= offset <= stop_:
                    return tabs
            return None

        calls = []
        for m in RE_ADD_STRATEGIES.finditer(block):
            calls.append((m.start(), re.findall(r'"([^"]+)"', m.group(1))))
        for m in RE_ADD_STRATEGY.finditer(block):
            calls.append((m.start(), [m.group(1)]))

        for offset, names in calls:
            guarded = specs_for(offset)
            for name in names:
                if guarded is None:
                    per_class[key].add(name)
                else:
                    for tab in guarded:
                        per_spec[(key, tab)].add(name)

    return {
        "common": sorted(common),
        "per_class": {k: sorted(v) for k, v in per_class.items()},
        "per_spec": {f"{k[0]}|{k[1]}": sorted(v) for k, v in per_spec.items()},
    }


def collect(module: str, jobs: int) -> dict:
    # Headers matter too: most strategies declare getName() inline in the .h, and scanning only .cpp
    # left every C++ class unmapped to its AiFactory name.
    files = []
    for base, _dirs, names in os.walk(os.path.join(module, "src")):
        for name in names:
            if name.endswith((".cpp", ".h")):
                files.append(os.path.join(base, name))

    with ProcessPoolExecutor(max_workers=jobs) as pool:
        scanned = [r for r in pool.map(scan_strategy_file, files, chunksize=16) if r]

    cxx_to_name: dict[str, str] = {}
    bases: dict[str, str] = {}
    cxx_actions: dict[str, set] = defaultdict(set)
    for entry in scanned:
        cxx_to_name.update(entry["names"])
        bases.update(entry.get("bases", {}))
    for entry in scanned:
        for cxx, acts in entry["actions"].items():
            cxx_actions[cxx].update(acts)

    def with_inherited(cxx: str, seen=None) -> set:
        seen = seen or set()
        if cxx in seen:
            return set()
        seen.add(cxx)
        out = set(cxx_actions.get(cxx, ()))
        base = bases.get(cxx)
        if base:
            out |= with_inherited(base, seen)
        return out

    strategy_actions: dict[str, set] = defaultdict(set)
    for cxx, name in cxx_to_name.items():
        strategy_actions[name].update(with_inherited(cxx))

    registered: set = set()
    for base, _dirs, names in os.walk(os.path.join(module, "src")):
        for name in names:
            if not name.endswith((".cpp", ".h")):
                continue
            text = open(os.path.join(base, name), "r", encoding="utf-8", errors="replace").read()
            marks = [(m.start(), m.group(1)) for m in RE_CLASS.finditer(text)]

            def enclosing(offset: int) -> str:
                out = ""
                for start, cls in marks:
                    if start > offset:
                        break
                    out = cls
                return out

            for m in RE_CREATOR.finditer(text):
                owner = enclosing(m.start()).lower()
                if "trigger" in owner or "strategy" in owner or "value" in owner:
                    continue
                registered.add(m.group(1))

    return {
        "strategy_actions": {k: sorted(v) for k, v in strategy_actions.items()},
        "registered": sorted(registered),
    }


def build_matrix(module: str, jobs: int) -> dict:
    data = collect(module, jobs)
    factory = parse_ai_factory(module)
    strategy_actions = data["strategy_actions"]
    registered = set(data["registered"])

    rows = []
    for klass, specs in SPECS.items():
        attached = set(factory["common"]) | set(factory["per_class"].get(klass, []))
        for spec in specs:
            # AiFactory guards spec strategies with "tab == <CLASS>_TAB_<SPEC>"; anything outside a
            # guard applies to every spec of the class.
            tab_key = spec.replace(" ", " ")
            guarded = set()
            for key, names in factory.get("per_spec", {}).items():
                cls_name, _, tab = key.partition("|")
                if cls_name != klass:
                    continue
                if tab == tab_key or tab.replace(" ", "") == tab_key.replace(" ", ""):
                    guarded.update(names)
            spec_strategies = attached | guarded

            reachable = set()
            unresolved = True
            for strategy in spec_strategies:
                acts = strategy_actions.get(strategy)
                if acts is None:
                    continue
                unresolved = False
                for act in acts:
                    if act in registered:
                        reachable.add(act)

            caps = set()
            for act in reachable:
                caps.update(ACTION_CAPABILITIES.get(act, []))

            rows.append({
                "class": klass,
                "spec": spec,
                "strategies": sorted(spec_strategies),
                "resolved": not unresolved,
                "reachable_actions": sorted(reachable),
                "capabilities": sorted(caps),
            })

    gaps = []
    unresolved = []
    for row in rows:
        if not row["resolved"] or not row["reachable_actions"]:
            unresolved.append(row)
            continue

        is_ranged = (row["class"], row["spec"]) in RANGED_SPECS
        for archetype, spec in ENEMY_ARCHETYPES.items():
            needs = spec["ranged_bot_needs"] if is_ranged else spec["melee_bot_needs"]
            missing = [c for c in needs if c not in row["capabilities"]]
            if missing:
                gaps.append({
                    "class": row["class"], "spec": row["spec"],
                    "enemy": archetype, "missing": missing,
                })

    return {"rows": rows, "gaps": gaps, "unresolved": unresolved,
            "factory": factory, "strategy_count": len(strategy_actions)}


def main() -> int:
    parser = argparse.ArgumentParser()
    here = os.path.dirname(os.path.abspath(__file__))
    default_module = os.path.abspath(os.path.join(here, "..", "..", "..", "modules", "mod-playerbots"))
    parser.add_argument("--module", default=default_module)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--format", choices=["text", "json"], default="text")
    parser.add_argument("--fail-on-gap", action="store_true")
    args = parser.parse_args()

    result = build_matrix(args.module, args.jobs)

    if args.format == "json":
        json.dump(result, sys.stdout, indent=1)
        return 0

    print(f"strategies resolved: {result['strategy_count']}")
    print()

    if result["unresolved"]:
        print("UNRESOLVED - the harness could not resolve these specs, so their empty cells mean")
        print("nothing about the bots. Fix the harness before reading anything into them.")
        for row in result["unresolved"]:
            print(f"  {row['class']:14s} {row['spec']:14s} strategies={row['strategies']}")
        print()

    print("capability coverage per spec")
    for row in result["rows"]:
        if not row["resolved"]:
            continue
        have = row["capabilities"]
        missing = [c for c in ALL_CAPABILITIES if c not in have]
        print(f"  {row['class']:14s} {row['spec']:14s} {len(row['reachable_actions']):3d} actions"
              f" | missing: {', '.join(missing) if missing else '-'}")

    print()
    print("matchup gaps (spec has no reachable answer for that archetype)")
    by_spec = defaultdict(list)
    for gap in result["gaps"]:
        by_spec[(gap["class"], gap["spec"])].append(gap)
    for (klass, spec), items in sorted(by_spec.items()):
        for gap in items:
            print(f"  {klass:14s} {spec:14s} vs {gap['enemy']:16s} missing {', '.join(gap['missing'])}")

    print()
    print(f"total gaps: {len(result['gaps'])} | unresolved specs: {len(result['unresolved'])}")
    return 1 if (args.fail_on_gap and result["gaps"]) else 0


if __name__ == "__main__":
    sys.exit(main())
