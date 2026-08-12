"""Capability taxonomy for the playerbot PvP rotation matrix.

The matrix asks one question per cell: *given my class and spec, and the archetype of the enemy I am
fighting, do I have any reachable answer to this situation?* Answering it needs a mapping from the
module's action names to the capability each one provides, because the module has no such notion -
an action is just a string.

Names are the module's own action names (lower case, as they appear in NextAction), which in turn
must match Spell.dbc. Where an ability serves two purposes it is listed under both.
"""

# --- capabilities -----------------------------------------------------------------------------

GAP_CLOSE = "gap_close"            # reach an enemy who is at range while I am melee
SNARE = "snare"                    # slow an enemy who is running away or kiting
ESCAPE_CONTROL = "escape_control"  # free myself from a root or snare
CC_BREAK = "cc_break"              # free myself from a stun, fear or polymorph
INTERRUPT = "interrupt"            # stop a cast
HARD_CC = "hard_cc"                # remove an enemy from the fight for several seconds
DEF_PHYSICAL = "def_physical"      # survive physical burst
DEF_MAGIC = "def_magic"            # survive magic burst
SELF_HEAL = "self_heal"            # restore my own health mid fight
FINISHER = "finisher"              # execute-range damage
OFFENSIVE_DISPEL = "off_dispel"    # strip an enemy buff
ANTI_HEALER = "anti_healer"        # mana pressure or healing reduction
RANGED_DAMAGE = "ranged_damage"    # damage an enemy I cannot reach

ALL_CAPABILITIES = [
    GAP_CLOSE, SNARE, ESCAPE_CONTROL, CC_BREAK, INTERRUPT, HARD_CC,
    DEF_PHYSICAL, DEF_MAGIC, SELF_HEAL, FINISHER, OFFENSIVE_DISPEL, ANTI_HEALER, RANGED_DAMAGE,
]

# --- action name -> capabilities --------------------------------------------------------------

ACTION_CAPABILITIES = {
    # warrior
    "charge": [GAP_CLOSE], "intercept": [GAP_CLOSE], "intercept on snare target": [GAP_CLOSE],
    "intercept on enemy healer": [GAP_CLOSE, INTERRUPT],
    "heroic throw": [RANGED_DAMAGE], "throw": [RANGED_DAMAGE], "shoot": [RANGED_DAMAGE],
    "hamstring": [SNARE], "piercing howl": [SNARE],
    "heroic fury": [ESCAPE_CONTROL], "berserker rage": [CC_BREAK],
    "pummel": [INTERRUPT], "shield bash": [INTERRUPT], "spell reflection": [DEF_MAGIC],
    "shield wall": [DEF_PHYSICAL, DEF_MAGIC], "last stand": [DEF_PHYSICAL, DEF_MAGIC],
    "retaliation": [DEF_PHYSICAL], "disarm": [DEF_PHYSICAL],
    "enraged regeneration": [SELF_HEAL], "victory rush": [SELF_HEAL],
    "execute": [FINISHER], "shattering throw": [OFFENSIVE_DISPEL],
    "mortal strike": [ANTI_HEALER], "concussion blow": [HARD_CC], "shockwave": [HARD_CC],

    # death knight
    "death grip": [GAP_CLOSE, INTERRUPT], "chains of ice": [SNARE],
    "mind freeze": [INTERRUPT], "strangulate": [INTERRUPT],
    "anti-magic shell": [DEF_MAGIC], "anti-magic zone": [DEF_MAGIC],
    "icebound fortitude": [DEF_PHYSICAL, DEF_MAGIC, CC_BREAK],
    "lichborne": [CC_BREAK], "death pact": [SELF_HEAL], "death strike": [SELF_HEAL],
    "rune tap": [SELF_HEAL], "hungering cold": [HARD_CC],
    "death coil": [RANGED_DAMAGE], "icy touch": [RANGED_DAMAGE], "howling blast": [RANGED_DAMAGE],

    # paladin
    "judgement of justice": [SNARE], "hand of freedom": [ESCAPE_CONTROL],
    "hammer of justice": [HARD_CC, INTERRUPT], "repentance": [HARD_CC],
    "repentance on snare target": [HARD_CC],
    "avenging wrath": [], "divine shield": [DEF_PHYSICAL, DEF_MAGIC, CC_BREAK],
    "divine protection": [DEF_PHYSICAL, DEF_MAGIC], "hand of protection": [DEF_PHYSICAL],
    "lay on hands": [SELF_HEAL], "flash of light": [SELF_HEAL], "holy light": [SELF_HEAL],
    "hammer of wrath": [FINISHER, RANGED_DAMAGE], "cleanse": [OFFENSIVE_DISPEL],
    "avenger's shield": [RANGED_DAMAGE, INTERRUPT],

    # hunter
    "disengage": [ESCAPE_CONTROL], "wing clip": [SNARE], "concussive shot": [SNARE],
    "frost trap": [SNARE], "freezing trap": [HARD_CC], "wyvern sting": [HARD_CC],
    "scatter shot": [HARD_CC], "silencing shot": [INTERRUPT],
    "deterrence": [DEF_PHYSICAL, DEF_MAGIC], "feign death": [DEF_PHYSICAL, CC_BREAK],
    "kill shot": [FINISHER, RANGED_DAMAGE], "tranquilizing shot": [OFFENSIVE_DISPEL],
    "viper sting": [ANTI_HEALER], "aimed shot": [ANTI_HEALER],
    "steady shot": [RANGED_DAMAGE], "arcane shot": [RANGED_DAMAGE], "auto shot": [RANGED_DAMAGE],

    # rogue
    "sprint": [GAP_CLOSE, ESCAPE_CONTROL], "shadowstep": [GAP_CLOSE],
    "crippling poison": [SNARE], "deadly throw": [SNARE],
    "cloak of shadows": [DEF_MAGIC, CC_BREAK], "evasion": [DEF_PHYSICAL],
    "vanish": [DEF_PHYSICAL, DEF_MAGIC, CC_BREAK], "kick": [INTERRUPT],
    "kidney shot": [HARD_CC], "cheap shot": [HARD_CC], "blind": [HARD_CC], "sap": [HARD_CC],
    "dismantle": [DEF_PHYSICAL], "eviscerate": [FINISHER],
    "mutilate": [ANTI_HEALER], "wound poison": [ANTI_HEALER],
    "throw": [RANGED_DAMAGE],

    # mage
    "blink": [ESCAPE_CONTROL, CC_BREAK], "frost nova": [SNARE, HARD_CC],
    "cone of cold": [SNARE], "chilled": [SNARE], "frostbolt": [SNARE, RANGED_DAMAGE],
    "counterspell": [INTERRUPT], "polymorph": [HARD_CC], "deep freeze": [HARD_CC],
    "ice block": [DEF_PHYSICAL, DEF_MAGIC, CC_BREAK], "mana shield": [DEF_MAGIC],
    "ice barrier": [DEF_PHYSICAL, DEF_MAGIC], "invisibility": [DEF_PHYSICAL],
    "spellsteal": [OFFENSIVE_DISPEL], "remove curse": [],
    "fireball": [RANGED_DAMAGE], "frostfire bolt": [RANGED_DAMAGE], "arcane blast": [RANGED_DAMAGE],

    # warlock
    "curse of exhaustion": [SNARE], "death coil": [HARD_CC, RANGED_DAMAGE],
    "fear": [HARD_CC], "howl of terror": [HARD_CC], "shadowfury": [HARD_CC],
    "spell lock": [INTERRUPT], "devour magic": [OFFENSIVE_DISPEL],
    "curse of tongues": [ANTI_HEALER], "drain mana": [ANTI_HEALER],
    "drain life": [SELF_HEAL], "death pact": [SELF_HEAL], "healthstone": [SELF_HEAL],
    "shadowburn": [FINISHER], "drain soul": [FINISHER],
    "shadow bolt": [RANGED_DAMAGE], "incinerate": [RANGED_DAMAGE], "corruption": [RANGED_DAMAGE],
    "demon charge": [GAP_CLOSE], "metamorphosis": [DEF_PHYSICAL, ESCAPE_CONTROL],

    # priest
    "psychic scream": [HARD_CC], "shackle undead": [HARD_CC], "mind control": [HARD_CC],
    "silence": [INTERRUPT], "dispel magic": [OFFENSIVE_DISPEL], "mass dispel": [OFFENSIVE_DISPEL],
    "mana burn": [ANTI_HEALER],
    "power word: shield": [DEF_PHYSICAL, DEF_MAGIC], "dispersion": [DEF_PHYSICAL, DEF_MAGIC],
    "pain suppression": [DEF_PHYSICAL, DEF_MAGIC], "fade": [DEF_PHYSICAL],
    "flash heal": [SELF_HEAL], "renew": [SELF_HEAL], "desperate prayer": [SELF_HEAL],
    "shadow word: death": [FINISHER], "shadow word: pain": [RANGED_DAMAGE],
    "mind blast": [RANGED_DAMAGE], "mind flay": [SNARE, RANGED_DAMAGE], "smite": [RANGED_DAMAGE],
    "psychic horror": [HARD_CC], "fear ward": [CC_BREAK],

    # druid
    "feral charge": [GAP_CLOSE], "feral charge - cat": [GAP_CLOSE], "dash": [GAP_CLOSE],
    "entangling roots": [SNARE, HARD_CC], "infected wounds": [SNARE],
    "travel form": [ESCAPE_CONTROL], "cat form": [ESCAPE_CONTROL], "bear form": [ESCAPE_CONTROL],
    "bash": [INTERRUPT, HARD_CC], "skull bash": [INTERRUPT],
    "cyclone": [HARD_CC], "hibernate": [HARD_CC], "maim": [HARD_CC], "pounce": [HARD_CC],
    "barkskin": [DEF_PHYSICAL, DEF_MAGIC], "survival instincts": [DEF_PHYSICAL],
    "frenzied regeneration": [SELF_HEAL], "healing touch": [SELF_HEAL], "rejuvenation": [SELF_HEAL],
    "ferocious bite": [FINISHER], "starfire": [RANGED_DAMAGE], "wrath": [RANGED_DAMAGE],
    "moonfire": [RANGED_DAMAGE], "insect swarm": [RANGED_DAMAGE],
    "prowl": [], "ravage": [],

    # shaman
    "earthbind totem": [SNARE], "frost shock": [SNARE], "ghost wolf": [ESCAPE_CONTROL],
    "wind shear": [INTERRUPT], "hex": [HARD_CC],
    "grounding totem": [DEF_MAGIC], "stoneclaw totem": [DEF_PHYSICAL],
    "shamanistic rage": [DEF_PHYSICAL, DEF_MAGIC],
    "tremor totem": [CC_BREAK], "healing wave": [SELF_HEAL], "lesser healing wave": [SELF_HEAL],
    "purge": [OFFENSIVE_DISPEL],
    "lightning bolt": [RANGED_DAMAGE], "chain lightning": [RANGED_DAMAGE],
    "earth shock": [INTERRUPT, RANGED_DAMAGE], "lava burst": [RANGED_DAMAGE],

    # shared / racial
    "every man for himself": [CC_BREAK], "will of the forsaken": [CC_BREAK],
    "pvp trinket": [CC_BREAK], "arcane torrent": [INTERRUPT],
    "healthstone": [SELF_HEAL], "healing potion": [SELF_HEAL],
}

# --- enemy archetypes and what each demands ----------------------------------------------------
#
# The demand list is what a bot must be able to answer to have a fair fight against that archetype.
# It deliberately does not include damage - every spec can damage. It lists the answers whose absence
# turns the matchup into a loss regardless of damage.

ENEMY_ARCHETYPES = {
    "melee_physical": {
        "classes": ["warrior", "rogue", "death knight", "paladin(ret)", "druid(feral)", "shaman(enh)"],
        "melee_bot_needs": [SNARE, DEF_PHYSICAL, CC_BREAK],
        "ranged_bot_needs": [SNARE, ESCAPE_CONTROL, DEF_PHYSICAL, CC_BREAK],
    },
    "caster": {
        "classes": ["mage", "warlock", "priest(shadow)", "druid(balance)", "shaman(ele)"],
        "melee_bot_needs": [GAP_CLOSE, INTERRUPT, DEF_MAGIC, ESCAPE_CONTROL],
        "ranged_bot_needs": [INTERRUPT, DEF_MAGIC],
    },
    "healer": {
        "classes": ["priest(holy/disc)", "paladin(holy)", "druid(resto)", "shaman(resto)"],
        # Healing reduction is a WotLK class privilege - Mortal Strike, Wound Poison, Aimed Shot,
        # Viper Sting, Mana Burn. Demanding it of everyone would report a design fact as a defect, so
        # the requirement is the pressure a healer actually has to answer: stop the cast, and take
        # him out of the fight for long enough to kill his partner.
        "melee_bot_needs": [INTERRUPT, GAP_CLOSE],
        "ranged_bot_needs": [INTERRUPT, HARD_CC],
    },
    "ranged_physical": {
        "classes": ["hunter"],
        "melee_bot_needs": [GAP_CLOSE, SNARE, DEF_PHYSICAL],
        "ranged_bot_needs": [DEF_PHYSICAL, CC_BREAK],
    },
    "stealth": {
        "classes": ["rogue", "druid(feral)"],
        "melee_bot_needs": [CC_BREAK, DEF_PHYSICAL, SELF_HEAL],
        "ranged_bot_needs": [CC_BREAK, DEF_PHYSICAL, SNARE, ESCAPE_CONTROL],
    },
}

# Specs that fight at range; everything else is treated as melee for the demand lists above.
RANGED_SPECS = {
    ("mage", "arcane"), ("mage", "fire"), ("mage", "frost"),
    ("warlock", "affliction"), ("warlock", "demonology"), ("warlock", "destruction"),
    ("priest", "shadow"), ("priest", "holy"), ("priest", "discipline"),
    ("hunter", "beast mastery"), ("hunter", "marksmanship"), ("hunter", "survival"),
    ("druid", "balance"), ("druid", "restoration"),
    ("shaman", "elemental"), ("shaman", "restoration"),
    ("paladin", "holy"),
}
