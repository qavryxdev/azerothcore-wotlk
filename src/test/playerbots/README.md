# Playerbot PvP rotation harness

Two checks, both runnable without a compiler and both parallel:

    python rotation_matrix.py --jobs $(nproc)
    python ../../../modules/mod-playerbots/tools/check_action_registry.py --jobs $(nproc)

`rotation_matrix.py` answers one question per cell of *my class/spec x enemy archetype*: is there
any reachable answer to that situation? An ability counts as reachable only when a NextAction names
it, it is registered in a creators map, and the strategy holding it is attached to that spec by
AiFactory - including through strategy inheritance and the battleground-only additions.

`capability_map.py` holds the two pieces of judgement: which ability provides which capability, and
what each enemy archetype demands. Both are opinions about WotLK, not facts derived from the code -
edit them when you disagree, and re-read the gaps afterwards.

## Reading the output

An UNRESOLVED spec means the harness could not work out which strategies that spec gets. Its empty
cells say nothing about the bots. Fix the harness first; never treat UNRESOLVED as a finding.

Every gap that looked implausible while this was being written turned out to be a harness bug -
headers not scanned, inheritance not followed, the battleground block attributed to the wrong class.
Check a surprising gap against the source before believing it.

## What this is not

It is not a behavioural simulation. Producing a real rotation decision needs a live PlayerbotAI,
Player and world, and fourteen InitTriggers bodies dereference the AI pointer, so strategies cannot
even be constructed with a null one. This harness proves an answer exists, not that the bot picks it
at the right moment.
