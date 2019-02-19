/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary. Use in whole or in part without
 * explicity permission by the author is strictly prohibited
 *
 * Current email address(es): cryogen@infoserve.net
 * Phone number: (604) 591-5295
 *
 * C4 Script Language written/copyright Cam Lesiuk 1995
 * Email: clesiuk@engr.uvic.ca
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
  #include <unistd.h> /* for unlink */
#endif

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "send.h"
#include "extra.h"
#include "property.h"
#include "code.h"
#include "file.h"
#include "thing.h"
#include "world.h"
#include "index.h"
#include "base.h"
#include "affect.h"
#include "effect.h"
#include "fight.h"
#include "object.h"
#include "board.h"
#include "area.h"
#include "cmd_inv.h"

#define OBJECT_INDEX_SIZE 8192

INDEX objectIndex;

  /* this allows for up to 16 different attributes to stored on a given object up
     to a total byte storage of 16 bytes

     first entry is the text name of the object type, followed by 3 arrays:
     an array of type specifiers each type specifier consists of 3 characters
     the first is the data-array (B,W or L), element #,  and then (<space> for no list, T for typelist
     or F for flag list)

NOTE: Everything is manipulated as an unsigned long int, so Big/Little endian is
      irrelevant.

ALSO: numbers in the form \XX seems to be in octal!!! so remember
     \00 0
     \01 1
     \02 2
     \03 3 (Max value for L entries)
     \04 4
     \05 5
     \06 6
     \07 7 (Max value for W entries)
     \10 8
     \11 9
     \12 10
     \13 11
     \14 12
     \15 13
     \16 14
     \17 15 (Max value for B entries)

     ie. Possible locations are:
     |B00 B01 B02 B03 |B04 B05 B06 B07 |B10 B11 B12 B13 |B14 B15 B16 B17|
     |W00     W01     |W02     W03     |W04     W05     |W06     W07    |
     |L00             |L01             |L02             |L03            |
*/

#define LNULL   {NULL,0}
#define L(list) {(BYTE**)list,sizeof(*list)}
OTYPELIST oTypeList[] = {
  { "!RESETCMD", {"B\00 ",       "B\01 ",       "B\02 ",       "L\01 ",       "L\02 ",       "L\03 ",       ""    },
                 {"RCMD",        "RIF",         "RARG4",       "RARG1",       "RARG2",       "RARG3",       ""    },
                 {LNULL,         LNULL,         LNULL,          LNULL,         LNULL,         LNULL,        LNULL } },

  { "LIGHT",     {"L\00 ",       "L\01T",       "L\02 ",       ""    },
                 {"INTENSITY",   "AMMO-TYPE",   "AMMO-USE",    ""    },
                 {LNULL,         L(oAmmoList),  LNULL,         LNULL } },

  { "SCANNER",   {"W\00F",       "B\02T",       "B\03 ",       "L\01 ",       "L\02 ",       "L\03 ",       ""    },
                 {"SCAN-FLAG",   "AMMO-TYPE",   "AMMO-USE",    "SCAN-MAX",    "BIO-SCANNED", "CHIP-SCANNED",""    },
                 {L(oSFlagList), L(oAmmoList),  LNULL,         LNULL,         LNULL,         LNULL,         LNULL } },

  { "DEVICE",    {"L\00 ",       "L\01T",       ""   },
                 {"AMMO-USE",    "AMMO-TYPE",   ""   },
                 {LNULL,         L(oAmmoList),  LNULL} },

  { "!STAFF",    {"L\00 ",       "L\01 ",       "L\02 ",       "L\03 ",       ""    },
                 {"UNDEF1",      "UNDEF2",      "UNDEF3",      "UNDEF4",      ""    },
                 {LNULL,         LNULL,         LNULL,         LNULL,         LNULL } },

  { "WEAPON",    {"L\00F",       "L\01F",       "B\13 ",       "B\12 ",       "B\11 ",       "B\17 ",       "B\16T",       "B\15 ",      "B\14T",       ""   },
                 {"WFLAGS",      "DAMFLAG",     "DAMDIENUM",   "FIRERATE",    "DAMDIESIZE",  "AMMO-USE",    "AMMO-TYPE",   "RANGE",      "WEAPONTYPE",  ""   },
                 {L(oWFlagList), L(resistList), LNULL,         LNULL,         LNULL,         LNULL,         L(oAmmoList),  LNULL,        L(weaponList), LNULL} },

  { "BOARD",     {"L\00 ",      ""   },
                 {"BVIRTUAL",   ""   },
                 {LNULL,        LNULL} },

  { "AMMO",      {"L\00T",       "L\01 ",       "W\05 ",       "W\04 ",       "L\03F",       ""     },
                 {"AMMOTYPE",    "AMMOLEFT",    "HITBONUS",    "DAMBONUS",    "DAMTYPE",     ""     },
                 {L(oAmmoList),  LNULL,         LNULL,         LNULL,         L(resistList), LNULL} },

  { "TREASURE",  {""   },
                 {""   },
                 {LNULL} },

  { "ARMOR",     {"L\00 ",       "B\04 ",       "B\05 ",       "B\06 ",       "B\07 ",       "B\10 ",      "B\11 ",      "B\12 ",      "B\13 ",      "B\14 ",      "B\15 ",       "B\16T",       ""   },
                 {"ARMOR",       "R-PUNCTURE",  "R-SLASH",     "R-CONCUSSIVE","R-HEAT",      "R-EMR",      "R-LASER",    "R-PSYCHIC",  "R-ACID",     "R-POISON",   "AMMO-USE",    "AMMO-TYPE",   ""   },
                 {LNULL,         LNULL,         LNULL,         LNULL,         LNULL,         LNULL,        LNULL,        LNULL,        LNULL,        LNULL,        LNULL,         L(oAmmoList),  LNULL} },

  { "DRUG",      {"L\00 ",       ""   },
                 {"INTOX",       ""   },
                 {LNULL,         LNULL} },

  { "WORN",      {""   },
                 {""   },
                 {LNULL} },

  { "OTHER",     {""   },
                 {""   },
                 {LNULL} },

  { "TRASH",     {""   },
                 {""   },
                 {LNULL} },

  { "!TRAP",     {"L\00 ",       "L\01 ",       "L\02 ",       "L\03 ",       ""   },
                 {"UNDEF1",      "UNDEF2",      "UNDEF3",      "UNDEF4",      ""   },
                 {LNULL,         LNULL,         LNULL,         LNULL,         LNULL} },

  { "CONTAINER", {"W\00 ",       "W\01 ",       "L\01F",       "L\02 ",       "L\03 ",       ""   },
                 {"MAX-CONTAIN", "ROT",         "C-FLAGS",     "KEY-NUM",     "SCAN-VALUE",  ""   },
                 {LNULL,         LNULL,         L(oCFlagList), LNULL,         LNULL,         LNULL} },

  { "!NOTE",     {"L\00 ",       "L\01 ",       "L\02 ",       "L\03 ",       ""   },
                 {"UNDEF1",      "UNDEF2",      "UNDEF3",      "UNDEF4",      ""   },
                 {LNULL,         LNULL,         LNULL,         LNULL,         LNULL} },

  { "DRINKCON",  {"L\00 ",       "L\01 ",       "L\02T",       "L\03 ",       ""   },
                 {"MAX-CONTAIN", "NOW-CONTAIN", "LIQUID",      "POISON",      ""   },
                 {LNULL,         LNULL,         L(oLiquidList),LNULL,         LNULL} },

  { "KEY",       {"L\00 ",       ""   },
                 {"KEYNUMBER",   ""   },
                 {LNULL,         LNULL} },

  { "FOOD",      {"L\00 ",       "L\03 ",       ""   },
                 {"HOWFILLING",  "POISON",      ""   },
                 {LNULL,         LNULL,         LNULL} },

  { "MONEY",     {"L\00 ",       ""   },
                 {"AMOUNT",      ""   },
                 {LNULL,         LNULL} },

  { "!PEN",      {"L\00 ",       "L\01 ",       "L\02 ",       "L\03 ",       ""       },
                 {"UNDEF1",      "UNDEF2",      "UNDEF3",      "UNDEF4",      ""       },
                 {LNULL,         LNULL,         LNULL,         LNULL,         LNULL} },

  { "!BOAT",     {"L\00 ",       "L\01 ",       "L\02 ",       "L\03 ",       ""       },
                 {"UNDEF1",      "UNDEF2",      "UNDEF3",      "UNDEF4",      ""       },
                 {LNULL,         LNULL,         LNULL,         LNULL,         LNULL} },

  { "EXIT",      {"L\00 ",       "L\02 ",       ""     },
                 {"WldVirtual",  "ROT"          ""     },
                 {LNULL,         LNULL,         LNULL} },

  { "VEHICLE",   {"L\00 ",       ""     },
                 {"WldVirtual",  ""     },
                 {LNULL,         LNULL} },

  /* Terminating entry */
  { "",      {"","","","","","","",""},
             {"","","","","","","",""},
             {LNULL,   LNULL,   LNULL,   LNULL,   LNULL,   LNULL,   LNULL,   LNULL} }

};

BYTE *oActList[] = {
  "GLOW",           /* glows */
  "HUM",            /* hums */
  "DARK",           /* creates darkness */
  "GOOD",           /* detects as good */
  "EVIL",           /* detects as evil */
  "INVISIBLE",      /* is invis */
  "AURA",           /* detects as psionic aura */
  "NODROP",         /* cant drop */
  "BLESS",          /* blessed/dedicated to some power (ie enspell it once) */
  "ANTI-GOOD",      /* good cant get */
  "ANTI-EVIL",      /* evil cant get */
  "ANTI-NEUTRAL",   /* neutral cant get */
  "HIDDEN",         /* hidden */
  "TRACK-OFFLINE",  /* only a limited number in the game */
  "CARRY2USE",      /* Can only use it when its carried */
  "NODEATHTRAP",    /* item will never be lost in a deathtrap */
  ""
};

BYTE *oSFlagList[] = {
  "SCANBIO",
  "SCANCHIP",
  ""
};

BYTE *oWFlagList[] = {
  "UNDEF0",
  ""
};

/* Name, Desc */
OAMMOTYPE oAmmoList[] = {
  { "NONE",                 "absolutely nothing"    },
  { "33-CALIBRE-CLIP",      "a 33 Calibre Clip"     },
  { "45-CALIBRE-CLIP",      "a 45 Calibre Clip"     },
  { "50-CALIBRE-BELT",      "a 50 Calibre Belt"     },
  { "DEUTERIUM-CARTRIDGE",  "a Deuterium Cartridge" },
  { "MICRO-GALCIV-CELL",    "a MicroCell"           },
  { "MINI-GALCIV-CELL",     "a MiniCell"            },
  { "MEDIUM-GALCIV-CELL",   "a MediumCell"          },
  { "LARGE-GALCIV-CELL",    "a MaxiCell"            },
  { "MICRO-BUILDER-CELL",   "a Builder MicroCell"   },
  { "MINI-BUILDER-CELL",    "a Builder MiniCell"    },
  { "MEDIUM-BUILDER-CELL",  "a Builder MediumCell"  },
  { "LARGE-BUILDER-CELL",   "a Builder MaxiCell"    },
  { "DRUG-CARTRIDGE",       "a Drug Cartridge"      },
  { "3INCH-SHOTSHELL-DRUM", "a 3\" Shotshell Drum"  },
  { "NEEDLE-CLIP",          "a Needle Clip"         },
  { "OXYGEN-CANNISTER",     "an Oxygen Cannister"   },
  { "",                     ""                      }
};


BYTE *oCFlagList[] = {
  "CLOSABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "CORPSE",
  "ELECTRONIC",
  "!PLAYERCORPSE",
  "!PLAYERLOOTED",
  ""
};

/* water is the bench mark against which all others are measured,
 * think of things as percents (humans have thirst/hunger/intox of 100)
 * so 10 drinks of water will take you right from dying of thirst to no thirsty
 * think of intox as (double?) percentage of alchohol, also means to get smashed 
 * will take you quite a few beers....
 */

/*   Name              Desc                        Thirst/Hunger/Intox/Poison/Acid */
OLIQUIDTYPE oLiquidList[] = {
  /* regular edibles */
  { "WATER",              "a clear liquid",              20,  2,    0,   0,   0 },
  { "RECYCLED-WATER",     "an almost clear liquid",      15,  1,    0,   0,   2 },
  { "COFFEE",             "a black liquid",              16,  8,    2,   0,   4 },
  { "ORANGE-JUICE",       "an orange liquid",            18, 12,    0,   0,   7 },
  { "GRAPE-JUICE",        "a purple liquid",             18, 12,    0,   0,   3 },
  { "MANGO-JUICE",        "a light orange liquid",       18, 12,    0,   0,   3 },
  { "APPLE-JUICE",        "a pale yellow liquid",        18, 12,    0,   0,   0 },
  { "DAJORIN-JUICE",      "a flourescent green liquid",  18, 12,    0,   0,   3 },
  { "QUALIL-JUICE",       "a neon blue liquid",          30, 16,    5,   1,   3 },
  { "PLEATH-JUICE",       "a sour liquid",                6, 15,    2,   1,  10 },
  { "GAL-CIV-COLA",       "a fizzy brown liquid",        10,  5,    0,   0,  10 },
  { "MILK",               "a milky white liquid",        18, 15,    0,   0,   0 },
  { "MILK-CHOCOLATE",     "a milky brown liquid",        15, 10,    0,   0,   0 },
  { "SYRUP",              "a dark syrup",                 1,  8,    0,   0,   0 },
  { "MAPLE-SYRUP",        "a thick goey brown syrup",     9, 14,    0,   0,   0 },
  { "BLOOD",              "a blood red liquid",           5, 10,    3,   0,   0 },
  { "Artificer-BLOOD",    "a thin yellow liquid",         8,  2,   10,  20,   0 },
  { "Salamander-BLOOD",   "a thick blue liquid",          3,  1,    5,   0,  12 },
  { "OLIVE-OIL",          "an oily yellow liquid",        0, 17,    0,   0,   0 },
  { "KETCHUP",            "a very thick red liquid",      5, 17,    0,   0,   0 },
  { "MUSTARD",            "a very thick yellow liquid",   2, 16,    0,   0,   0 },
  { "MEAL-REPLACEMENT",   "a very thick grey liquid",     2, 25,    1,   0,   0 },

  /* intox */
  { "BEER",               "a fizzy amber liquid",        12,  10,   8,   0,   0 },
  { "LABATTS-BEER",       "a pale amber liquid",         16,   8,  18,   0,   0 },
  { "WEST-COAST-ALE",     "a fizzy dark amber liquid",   15,  12,   9,   0,   0 },
  { "HUMAN-VODKA",        "a clear liquid",               6,   0,  30,   0,   0 },
  { "HUMAN-WHISKEY",      "a light amber liquid",         8,   0,  20,   0,   0 },
  { "HUMAN-SCOTCH",       "a clear amber liquid",         8,   0,  20,   0,   0 },
  { "HUMAN-RUM",          "an amber liquid",              8,   0,  20,   0,   0 },
  { "ARTIFICER-ALE",      "a light green liquid",        15,   5,  15,   0,   0 },
  { "SALAMANDER-GROG",    "a firey red and black liquid", 5,   0,  35,   0,   0 },
  { "LIQUID-SILICON",     "a heavy cloudy liquid",       10,   1,  20,   0,   0 },
  { "ETHANOL",            "a thin clear liquid",          2,   0,  40,  10,   0 },

  /* poisons and acids */
  { "STAGNANT-WATER",     "a smelly liquid",             10,   4,   3,   5,   2 },
  { "POISON-WATER",       "a clear liquid",              20,   2,   5,  35,   0 },
  { "BLUE-PAINT",         "a thick, blue liquid",         2,   5,   6,  15,   2 },
  { "RED-PAINT",          "a thick, red liquid",          2,   5,   6,  15,   2 },
  { "YELLOW-PAINT",       "a thick, yellow liquid",       2,   5,   6,  15,   2 },
  { "GREEN-PAINT",        "a thick, green liquid",        2,   5,   6,  15,   2 },
  { "GREY-PAINT",         "a thick, grey liquid",         2,   5,   6,  15,   2 },
  { "GASOLINE",           "a smelly, amber liquid",       1,   0,  12,  20,   3 },
  { "MACHINE-OIL",        "a dark, oily liquid",          1,   1,   2,  20,   0 },
  { "VERATHANE",          "a smelly, clear liquid",       0,   0,  20,  25,   5 },
  { "CYANIDE",            "a featureless liquid",         0,   0,   9, 200,   0 },
  { "SULFURIC-ACID",      "a thick, clear liquid",        0,   0,   8,  30, 100 },
  { "HYDROCHLORIC-ACID",  "a thick, clear liquid",        0,   0,   8,  30, 100 },

  { "" }
};

/* when you equip an object (or use it) it applies an EFFECT/AFFECT to something(s)
   each EFFECT can apply multiple AFFECT structures, and must apply at least
   one.

   if you specify TAR_NODEFAULT, then that apply will do nothing unless you
   USE the object in question and then pass it a target (without TAR_NODEFAULT
   the player will get a you have to specify a target type message)

   when you equip an item it will apply any applies that are TAR_ON_WEAR
   and direct AFFECTS (ie TAR_AFFECT's, which are implicitly TAR_SELF & only TAR_SELF)
  */
struct ApplyListType applyList[] = {
  {"NONE",                      0,                   0},
  {"STR",                       AFFECT_STR,          TAR_AFFECT},
  {"DEX",                       AFFECT_DEX,          TAR_AFFECT},
  {"INT",                       AFFECT_INT,          TAR_AFFECT},
  {"WIS",                       AFFECT_WIS,          TAR_AFFECT},
  {"CON",                       AFFECT_CON,          TAR_AFFECT},
  {"HIT",                       AFFECT_HIT,          TAR_AFFECT},
  {"MOVE",                      AFFECT_MOVE,         TAR_AFFECT},
  {"POWER",                     AFFECT_POWER,        TAR_AFFECT},
  {"ARMOR",                     AFFECT_ARMOR,        TAR_AFFECT},
  {"HITROLL",                   AFFECT_HITROLL,      TAR_AFFECT},
  {"DAMROLL",                   AFFECT_DAMROLL,      TAR_AFFECT},
  {"SPEED",                     AFFECT_SPEED,        TAR_AFFECT},

  {"USE-SELF-CELLREPAIR",       EFFECT_CELL_REPAIR,  TAR_SELF_DEF},
  {"USE-SELF-REFRESH",          EFFECT_REFRESH,      TAR_SELF_DEF},
  {"USE-SELF-ENDURANCE",        EFFECT_ENDURANCE,    TAR_SELF_DEF},
  {"USE-SELF-BREATHWATER",      EFFECT_BREATHWATER,  TAR_SELF_DEF},
  {"USE-SELF-STRENGTH",         EFFECT_STRENGTH,     TAR_SELF_DEF},
  {"USE-SELF-DARKVISION",       EFFECT_DARKVISION,   TAR_SELF_DEF},
  {"USE-SELF-SLOW-POISON",      EFFECT_SLOW_POISON,  TAR_SELF_DEF},
  {"USE-SELF-BERSERK",          EFFECT_BERSERK,      TAR_SELF_DEF},
  {"USE-SELF-CURE-POISON",      EFFECT_CURE_POISON,  TAR_SELF_DEF},
  {"USE-SELF-HEAL-MINOR",       EFFECT_HEAL_MINOR,   TAR_SELF_DEF},
  {"USE-SELF-REGENERATION",     EFFECT_REGENERATION, TAR_SELF_DEF},
  {"USE-SELF-HEAL-MAJOR",       EFFECT_HEAL_MAJOR,   TAR_SELF_DEF},
  {"USE-SELF-DEXTERITY",        EFFECT_DEXTERITY,    TAR_SELF_DEF},
  {"USE-SELF-CONSTITUTION",     EFFECT_CONSTITUTION, TAR_SELF_DEF},
  {"USE-SELF-HASTE",            EFFECT_HASTE,        TAR_SELF_DEF},
  {"USE-SELF-REVITALIZE",       EFFECT_REVITALIZE,   TAR_SELF_DEF},
  {"USE-SELF-RECALL",           EFFECT_RECALL,       TAR_SELF_DEF},
  {"USE-SELF-MARK",             EFFECT_MARK,         TAR_SELF_DEF},
  {"USE-SELF-POISON",           EFFECT_POISON,       TAR_SELF_DEF},

  {"WEAR-BREATHWATER",          EFFECT_BREATHWATER,  TAR_ON_WEAR},
  {"WEAR-VACUUMWALK",           EFFECT_VACUUMWALK,   TAR_ON_WEAR},

  {"",0,0}
};

BYTE         objectOfLog;    /* log if a virtual # cant be found */
BYTE         objectReadLog;  /* log each Obj structure as its read */
LWORD        objectNum = 0;  /* number of Obj structures in use - just curious really */
OBJTEMPLATE *corpseTemplate = NULL;
OBJTEMPLATE *resetTemplate = NULL;
OBJTEMPLATE *moneyTemplate = NULL;
OBJTEMPLATE *phantomPocketTemplate = NULL;
OBJTEMPLATE *fireBladeTemplate = NULL;
OBJTEMPLATE *fireShieldTemplate = NULL;
OBJTEMPLATE *fireArmorTemplate = NULL;

void ObjectInit(void) {
  BYTE buf[256];
 
  /* INI reads */
  objectOfLog   = INILWordRead("crimson2.ini", "objectOfLog", 0);
  objectReadLog = INILWordRead("crimson2.ini", "objectReadLog", 0);
  sprintf(buf, "Reading object logging defaults\n");
  Log(LOG_BOOT, buf);
  sprintf(buf, "ObjectTemplate structure size is %d bytes\n", sizeof(OBJTEMPLATE));
  Log(LOG_BOOT, buf);

  IndexInit(&objectIndex, OBJECT_INDEX_SIZE, "objectIndex", 0);

  /* *********************************************** */
  /* System object templates must *NOT* be equipable */
  /* *********************************************** */

  /* create a corpse template for fighting */
  MEMALLOC(corpseTemplate, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
  memset( (void*)corpseTemplate, 0, sizeof(OBJTEMPLATE));
  corpseTemplate->oVirtual = OVIRTUAL_CORPSE;
  corpseTemplate->oKey =   STRCREATE("Its dead Jim...");
  corpseTemplate->oSDesc = STRCREATE("Its dead Jim...");
  corpseTemplate->oLDesc = STRCREATE("Its dead Jim...");
  corpseTemplate->oDesc =  STRCREATE("Its dead Jim...\n");
  corpseTemplate->oType = OTYPE_CONTAINER;
  corpseTemplate->oWeight = 150;
  corpseTemplate->oValue = 0;
  corpseTemplate->oRent = 0;
  ObjectSetField(OTYPE_CONTAINER, &corpseTemplate->oDetail, OF_CONTAINER_MAX, 150);
  ObjectSetField(OTYPE_CONTAINER, &corpseTemplate->oDetail, OF_CONTAINER_KEY, -1);
  ObjectSetField(OTYPE_CONTAINER, &corpseTemplate->oDetail, OF_CONTAINER_ROT, 5);

  /* create a reset command template for fighting */
  MEMALLOC(resetTemplate, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
  memset( (void*)resetTemplate, 0, sizeof(OBJTEMPLATE));
  resetTemplate->oVirtual = OVIRTUAL_RESET;
  resetTemplate->oKey =   STRCREATE("reset command rst cmd");
  resetTemplate->oSDesc = STRCREATE("Reset Command");
  resetTemplate->oLDesc = STRCREATE("Reset Command");
  resetTemplate->oDesc =  STRCREATE("A Reset Command.\n");
  resetTemplate->oType =  OTYPE_RESETCMD;
  resetTemplate->oWeight =0;
  resetTemplate->oValue = 0;
  resetTemplate->oRent = -1;

  /* create a reset command template for fighting */
  MEMALLOC(moneyTemplate, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
  memset( (void*)moneyTemplate, 0, sizeof(OBJTEMPLATE));
  moneyTemplate->oVirtual = OVIRTUAL_MONEY;
  moneyTemplate->oKey =   STRCREATE("money galciv credits");
  moneyTemplate->oSDesc = STRCREATE("pile of GalCiv credits");
  moneyTemplate->oLDesc = STRCREATE("A pile of GalCiv credits has been left here");
  moneyTemplate->oDesc =  STRCREATE("GalCiv credits, the standard medium of currency exchange across the entire\ngalaxy. Too bad there isnt more...\n");
  moneyTemplate->oType =  OTYPE_MONEY;
  moneyTemplate->oWeight =0;
  moneyTemplate->oValue = 0;
  moneyTemplate->oRent = -1;

  /* create a template for phantomPocket's */
  MEMALLOC(phantomPocketTemplate, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
  memset( (void*)phantomPocketTemplate, 0, sizeof(OBJTEMPLATE));
  phantomPocketTemplate->oVirtual = OVIRTUAL_PHANTOMPOCKET;
  phantomPocketTemplate->oKey =   STRCREATE("phantom pocket opening");
  phantomPocketTemplate->oSDesc = STRCREATE("pocket-sized opening in space-time");
  phantomPocketTemplate->oLDesc = STRCREATE("A pocket-sized opening in space-time has been left here");
  phantomPocketTemplate->oDesc =  STRCREATE("Its a pocket-sized hole in space-time that leads to another dimension. It's\ngreat for storing stuff in because then you dont have to carry them.\n");
  phantomPocketTemplate->oType =  OTYPE_CONTAINER;
  phantomPocketTemplate->oWear =  TYPEFIND("HELD", wearList);
  phantomPocketTemplate->oWeight =0;
  phantomPocketTemplate->oValue = 0;
  phantomPocketTemplate->oRent = -1;

  /* create a template for fireBlade's */
  MEMALLOC(fireBladeTemplate, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
  memset( (void*)fireBladeTemplate, 0, sizeof(OBJTEMPLATE));
  fireBladeTemplate->oVirtual = OVIRTUAL_FIREBLADE;
  fireBladeTemplate->oKey =   STRCREATE("slender blade fireblade");
  fireBladeTemplate->oSDesc = STRCREATE("Slender blade of fire");
  fireBladeTemplate->oLDesc = STRCREATE("A slender blade of fire has been left here");
  fireBladeTemplate->oDesc =  STRCREATE("Its a slender blade shaped field of fire, the better to attack people with.\n");
  fireBladeTemplate->oType =  OTYPE_WEAPON;
  fireBladeTemplate->oWear =  TYPEFIND("HELD", wearList);
  fireBladeTemplate->oWeight =0;
  fireBladeTemplate->oValue = 0;
  fireBladeTemplate->oRent = -1;

  /* create a template for fireShields's */
  MEMALLOC(fireShieldTemplate, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
  memset( (void*)fireShieldTemplate, 0, sizeof(OBJTEMPLATE));
  fireShieldTemplate->oVirtual = OVIRTUAL_FIRESHIELD;
  fireShieldTemplate->oKey =   STRCREATE("shield fireshield disc large");
  fireShieldTemplate->oSDesc = STRCREATE("large disc of fire");
  fireShieldTemplate->oLDesc = STRCREATE("A disc of fire has been left here");
  fireShieldTemplate->oDesc =  STRCREATE("Its a large disc shaped field of fire that you can you use to help ward off attacks\n");
  fireShieldTemplate->oType =  OTYPE_ARMOR;
  fireShieldTemplate->oWear =  TYPEFIND("HELD", wearList);
  fireShieldTemplate->oWeight =0;
  fireShieldTemplate->oValue = 0;
  fireShieldTemplate->oRent = -1;

  /* create a template for fireArmor's */
  MEMALLOC(fireArmorTemplate, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
  memset( (void*)fireArmorTemplate, 0, sizeof(OBJTEMPLATE));
  fireArmorTemplate->oVirtual = OVIRTUAL_FIREARMOR;
  fireArmorTemplate->oKey =   STRCREATE("armor firearmor column");
  fireArmorTemplate->oSDesc = STRCREATE("column of fire");
  fireArmorTemplate->oLDesc = STRCREATE("A column of fire is standing here");
  fireArmorTemplate->oDesc =  STRCREATE("Its a large column of fire that surrounds you and much like a\nbig suit of fiery armor, protects you from harm.\n");
  fireArmorTemplate->oType =  OTYPE_ARMOR;
  fireArmorTemplate->oWear =  TYPEFIND("OVERBODY", wearList);
  fireArmorTemplate->oWeight =0;
  fireArmorTemplate->oValue = 0;
  fireArmorTemplate->oRent = -1;
}


void ObjectRead(WORD area) {
  FILE        *objectFile;
  BYTE         objFileBuf[256];
  BYTE         buf[256];
  BYTE         tmp[256];
  STR         *sKey;
  STR         *sDesc;
  OBJTEMPLATE *object;
  LWORD        last = -1;
  LWORD        apply;

  sprintf(objFileBuf, "area/%s.obj", areaList[area].aFileName->sText);
  objectFile = fopen(objFileBuf, "rb");
  if (!objectFile) {
    sprintf(buf, "Unable to read %s, killing server\n", objFileBuf);
    Log(LOG_BOOT, buf);
    PERROR("ObjectRead");
    exit(ERROR_BADFILE);
  }
  if (areaList[area].aOffset) {
    sprintf(buf, "Relocating Area/Objects [%s]\n", areaList[area].aFileName->sText);
    Log(LOG_BOOT, buf);
  }

  /* okay we opened it up so read it.... */
  fscanf(objectFile, " %s \n", tmp); /* get virtual number */
  while (!feof(objectFile)) {
    if (tmp[0] == '$' || feof(objectFile))
      break; /* Dikumud file format EOF character */
    if (tmp[0] != '#') { /* whoa... whadda we got here */
      sprintf(buf, "ObjectRead: Unknown virtual %s, aborting\n", tmp);
      Log(LOG_BOOT, buf);
      break;
    }
    MEMALLOC(object, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
    memset( (void*)object, 0, sizeof(OBJTEMPLATE)); /* init to zeros */
    object->oVirtual = atoi(tmp+1);
    object->oVirtual += areaList[area].aOffset;
    if (objectReadLog) {
      sprintf(buf, "Reading object#%ld\n", object->oVirtual);
      Log(LOG_BOOT, buf);
    }
    /* confirm that virtual number is valid */
    if (object->oVirtual < last) {
      sprintf(buf, "%s - Obj%s < than previous\n", objFileBuf, tmp);
      Log(LOG_BOOT, buf);
    }
    last = MAXV(last, object->oVirtual);
    if (object->oVirtual < areaList[area].aVirtualMin) {
      sprintf(buf, "%s - Obj%s < than %ld\n", objFileBuf, tmp, areaList[area].aVirtualMin);
      Log(LOG_BOOT, buf);
      break;
    }
    if (object->oVirtual > areaList[area].aVirtualMax) {
      sprintf(buf, "%s - Obj%s > than %ld\n", objFileBuf, tmp, areaList[area].aVirtualMax);
      Log(LOG_BOOT, buf);
      break;
    }

    /* NOTE that Private Strings are not designated as such, until just prior to editing */
    objectNum++;
    object->oKey   = FileStrRead(objectFile);
    if (fileError) {
      sprintf(buf, "Error reading Key for object#%ld\n", object->oVirtual);
      Log(LOG_BOOT, buf);
    }
    object->oSDesc = FileStrRead(objectFile);
    if (fileError) {
      sprintf(buf, "Error reading SDesc for object#%ld\n", object->oVirtual);
      Log(LOG_BOOT, buf);
    }
    /* under vanilla diku file format object ldesc's dont have \n's on end 
     * but mob lDescs do, in any case Crimson2 ldescs should not have the \n
     */
    object->oLDesc  = FileStrRead(objectFile);
    if (fileError) {
      sprintf(buf, "Error reading LDesc for object#%ld\n", object->oVirtual);
      Log(LOG_BOOT, buf);
    }
    if (object->oLDesc->sText[object->oLDesc->sLen-1] == '\n') {
      strcpy(buf, object->oLDesc->sText);
      buf[object->oLDesc->sLen-1] = '\0';
      STRFREE(object->oLDesc);
      object->oLDesc = STRCREATE(buf);
    }
    /* in vanilla diku format this is an unused string field, and should be blank */
    object->oDesc  = FileStrRead(objectFile);
    if (fileError) {
      sprintf(buf, "Error reading Desc for object#%ld\n", object->oVirtual);
      Log(LOG_BOOT, buf);
    }
    if (object->oDesc->sText[0] == '\0') {
      STRFREE(object->oDesc);
      object->oDesc = STRCREATE("You see nothing special\n");
    }
    object->oType = FILETYPEREAD(objectFile, oTypeList); /* type of this room, ie city, river, underwater etc */
    object->oAct = FileFlagRead(objectFile, oActList); /* flags for this room */
    object->oWear = FILETYPEREAD(objectFile, wearList); /* type of this room, ie city, river, underwater etc */
    if (fileError) {
      sprintf(buf, "Error reading Wear value for object#%ld\n", object->oVirtual);
      Log(LOG_BOOT, buf);
    }
    fscanf(objectFile, " %ld ", &object->oDetail.lValue[0]);
    fscanf(objectFile, " %ld ", &object->oDetail.lValue[1]);
    fscanf(objectFile, " %ld ", &object->oDetail.lValue[2]);
    fscanf(objectFile, " %ld ", &object->oDetail.lValue[3]);
    fscanf(objectFile, " %hd ", &object->oWeight);
    fscanf(objectFile, " %ld ", &object->oValue);
    fscanf(objectFile, " %ld ", &object->oRent);

    /* read attachments ie keywords etc */
    apply = 0;
    while (!feof(objectFile)) {
      fscanf(objectFile, " %s \n", tmp);
      if (tmp[0] == '#') /* well thats it for this chunk of the world */
        break;

      else if (tmp[0] == 'E') { /* haha an extra */
        sKey  = FileStrRead(objectFile);
        if (fileError) {
          sprintf(buf, "Error reading ExtraKey for object#%ld\n", object->oVirtual);
          Log(LOG_BOOT, buf);
        }
        sDesc = FileStrRead(objectFile);
        if (fileError) {
          sprintf(buf, "Error reading ExtraDesc for object#%ld\n", object->oVirtual);
          Log(LOG_BOOT, buf);
        }
        object->oExtra = ExtraAlloc(object->oExtra, sKey, sDesc);
      }

      else if (tmp[0] == 'P') { /* a property of some kind */
        sKey =  FileStrRead(objectFile); /* property name */
        if (fileError) {
          sprintf(buf, "Error reading PropertyKey for object#%ld\n", object->oVirtual);
          Log(LOG_BOOT, buf);
        }
        sDesc = FileStrRead(objectFile);/* property value */
        if (fileError) {
          sprintf(buf, "Error reading PropertyDesc for object#%ld\n", object->oVirtual);
          Log(LOG_BOOT, buf);
        }
        object->oProperty = PropertyCreate(object->oProperty, sKey, sDesc);
      }

      else if (tmp[0] == 'A') { /* an Apply */
        if (apply < OBJECT_MAX_APPLY) {
          object->oApply[apply].aType  = FILETYPEREAD(objectFile, applyList);
          TYPECHECK(object->oApply[apply].aType, applyList);
          object->oApply[apply].aValue = FileByteRead(objectFile);
          apply++;
        } else {
          sprintf(buf, "Object #%ld - too many apply's\n", object->oVirtual);
          Log(LOG_BOOT, buf);
          fgets(buf, sizeof(buf), objectFile);
        }
      }
    }

    /* guess this ones a keeper, update mins and maxes etc... */
    IndexInsert(&areaList[area].aObjIndex, object, ObjectCompareProc);
    if (indexError) {
      sprintf(buf, "%s - obj%s duplicated\n", objFileBuf, tmp);
      Log(LOG_BOOT, buf);
    }

  }
  /* all done close up shop */
  fclose(objectFile);
}


OBJTEMPLATE *ObjectOf(LWORD virtual) {
  THING *search;
  BYTE   buf[256];
  LWORD  area;

  area = AreaOf(virtual);
  if (area == -1) return NULL;

  search = IndexFind(&areaList[area].aObjIndex, (void*)virtual, ObjectFindProc);
  if (search) {
    return ObjTemplate(search);
  } else {
    if (objectOfLog) {
      sprintf(buf, "ObjectOf: virtual %ld doesnt exist\n", virtual);
      Log(LOG_ERROR, buf);
    }
  }
  return NULL;
}


void ObjectWrite(WORD area) {
  FILE        *objectFile;
  BYTE         objFileBuf[256];
  BYTE         buf[256];
  OBJTEMPLATE *object;
  LWORD        i;
  LWORD        apply;
  EXTRA       *extra;
  PROPERTY    *property;
  
  /* take a backup case we crash halfways through this */
  sprintf(buf,
    "mv area/obj/%s.obj area/obj/%s.obj.bak",
    areaList[area].aFileName->sText,
    areaList[area].aFileName->sText);
  system(buf);
  
  sprintf(objFileBuf, "area/%s.obj", areaList[area].aFileName->sText);
  objectFile = fopen(objFileBuf, "wb");
  if (!objectFile) {
    sprintf(buf, "Unable to write %s, killing server\n", objFileBuf);
    Log(LOG_ERROR, buf);
    PERROR("ObjectWrite");
    return;
  }
  
  /* okay we opened it up so write it.... */
  for (i=0; i<areaList[area].aObjIndex.iNum; i++) {
    object = ObjTemplate(areaList[area].aObjIndex.iThing[i]);
    fprintf(objectFile, "#%ld\n", object->oVirtual); /* get virtual number */
    FileStrWrite(objectFile, object->oKey);
    FileStrWrite(objectFile, object->oSDesc);
    FileStrWrite(objectFile, object->oLDesc);
    FileStrWrite(objectFile, object->oDesc);
    FILETYPEWRITE(objectFile, object->oType, oTypeList, ' ');
    FileFlagWrite(objectFile, object->oAct, oActList, ' ');
    FILETYPEWRITE(objectFile, object->oWear, wearList, '\n');
    
    fprintf(objectFile, "%ld %ld %ld %ld\n", object->oDetail.lValue[0], object->oDetail.lValue[1], object->oDetail.lValue[2], object->oDetail.lValue[3]);
    fprintf(objectFile, "%hd %ld %ld\n", object->oWeight, object->oValue, object->oRent);
    
    /* apply's */
    for (apply=0; apply<OBJECT_MAX_APPLY; apply++) {
      if (object->oApply[apply].aType){
        fprintf(objectFile, "A\n");
        FILETYPEWRITE(objectFile, object->oApply[apply].aType, applyList, ' ');
        FileByteWrite(objectFile, object->oApply[apply].aValue, '\n');
      }
    }
    
    /* extra descriptions */
    for (extra=object->oExtra; extra; extra=extra->eNext) {
      fprintf(objectFile, "E\n");
      FileStrWrite(objectFile, extra->eKey);
      FileStrWrite(objectFile, extra->eDesc);
    }
    
    /* property's */
    for (property=object->oProperty; property; property=property->pNext) {
      fprintf(objectFile, "P\n");
      FileStrWrite(objectFile, property->pKey);
      if ( CodeIsCompiled(property) ) {
        if (!areaWriteBinary) {
          /* decompile the property */
          CodeDecompProperty(property, NULL);
          object->oCompile = 1; /* enable compile on demand */
          /* Warn if it didnt decompile */
          if (CodeIsCompiled(property)) {
            sprintf(buf, "ObjectWrite: Property %s failed to decompile for object#%ld!\n",
                         property->pKey->sText, 
                         object->oVirtual);
            Log(LOG_AREA, buf);
          } else {
            FileStrWrite(objectFile, property->pDesc);
          }
        } else {
          FileBinaryWrite(objectFile, property->pDesc);
        }
      } else {
        FileStrWrite(objectFile, property->pDesc);
      }
    }

  }
  /* all done close up shop */
  fprintf(objectFile, "$");
  fclose(objectFile);

  /* turf backup we didnt crash */
  sprintf(buf,
    "area/%s.obj.bak",
    areaList[area].aFileName->sText);
  unlink(buf);

}


INDEXPROC(ObjectCompareProc) { /* BYTE IndexProc(void *index1, void *index2) */
  if ( ObjTemplate(index1)->oVirtual == ObjTemplate(index2)->oVirtual )
    return 0;
  else if ( ObjTemplate(index1)->oVirtual < ObjTemplate(index2)->oVirtual )
    return -1;
  else
    return 1;
}

INDEXFINDPROC(ObjectFindProc) { /* BYTE IFindProc(void *key, void *index) */
  if ( (LWORD)key == ObjTemplate(index)->oVirtual )
    return 0;
  else if ( (LWORD)key < ObjTemplate(index)->oVirtual )
    return -1;
  else
    return 1;
}


THING *ObjectAlloc(void) {
  OBJ *object;

  MEMALLOC(object, OBJ, OBJECT_ALLOC_SIZE);
  memset(object, 0, sizeof(OBJ));
  IndexAppend(&objectIndex, Thing(object));

  Thing(object)->tType = TTYPE_OBJ;
  return Thing(object);
}

THING *ObjectCreate(OBJTEMPLATE *template, THING *within) {
  THING    *object;
  LWORD     i;
  PROPERTY *p;

  if (!template) return NULL;
  object = ObjectAlloc();

  Thing(object)->tSDesc    = StrAlloc(template->oSDesc);
  Thing(object)->tDesc     = StrAlloc(template->oDesc);
  Thing(object)->tExtra    = ExtraCopy(template->oExtra);
  if (template->oCompile) {
    template->oCompile = 0;
    for (p=template->oProperty; p; p=p->pNext) {
      CodeCompileProperty(p, NULL);
    }
  }
  Thing(object)->tProperty = PropertyCopy(template->oProperty);
  Base(object)->bKey       = StrAlloc(template->oKey);
  Base(object)->bLDesc     = StrAlloc(template->oLDesc);
  Base(object)->bWeight    = template->oWeight;
  Obj(object)->oTemplate   = template;

  /* this data changes so needs to be kept on each object */
  Obj(object)->oAct        = template->oAct;
  for (i=0; i<4; i++)
    Obj(object)->oDetail.lValue[i] = template->oDetail.lValue[i];
  for (i=0; i<OBJECT_MAX_APPLY; i++) {
    Obj(object)->oApply[i].aType  = template->oApply[i].aType;
    Obj(object)->oApply[i].aValue = template->oApply[i].aValue;
  }

#ifdef UGLY_VEHICLE
  /* Special hook for VEHICLES */
  if (template->oType == OTYPE_VEHICLE) {
    WLD   *vehicle = NULL;
    THING *wld = WorldOf( OBJECTGETFIELD(object, OF_VEHICLE_WVIRTUAL) );

    if (wld) {
      MEMALLOC(vehicle, WLD, WORLD_ALLOC_SIZE);
      memset(vehicle, 0, sizeof(WLD));
      Thing(vehicle)->tType     = TTYPE_WLD;
      Thing(vehicle)->tSDesc    = StrAlloc(wld->tSDesc);
      Thing(vehicle)->tDesc     = StrAlloc(wld->tDesc);
      Thing(vehicle)->tExtra    = ExtraCopy(wld->tExtra);
      Wld(vehicle)->wVirtual = WVIRTUAL_VEHICLE;
      Wld(vehicle)->wArea = Wld(wld)->wArea;
      Wld(vehicle)->wFlag = Wld(wld)->wFlag;
      Wld(vehicle)->wType = Wld(wld)->wType;
    }
    Obj(object)->oDetail.lValue[3] = (LWORD)vehicle;
  }
#endif

  template->oOnline++;
  CodeCheckFlag(object);
  if (within) ThingTo(object, within);
  return object;
}

THING *ObjectCreateMoney(LWORD amount, THING *within) {
  THING *money;
  BYTE   buf[256];

  money = ObjectCreate(moneyTemplate, within);
  if (!money) return NULL;

  STRFREE(money->tSDesc);
  STRFREE(Base(money)->bLDesc);
  STRFREE(money->tDesc);

  MINSET(amount, 1);
  if (amount <= 1) {
    money->tSDesc = STRCREATE("A single GalCiv credit");
    Base(money)->bLDesc = STRCREATE("Someone has carelessly discarded a single GalCiv credit here");
    money->tDesc =  STRCREATE("One miserable GalCiv credit, the standard medium of currency exchange across\nthe entire galaxy. Too bad there isnt more...\n");
  } else if (amount <= 2) {
    money->tSDesc = STRCREATE("A couple GalCiv credits");
    Base(money)->bLDesc = STRCREATE("Someone has carelessly discarded a couple GalCiv credits here");
    money->tDesc =  STRCREATE("2 miserable GalCiv credits, the standard medium of currency exchange across\nthe entire galaxy. Too bad there isnt more...\n");
  } else if (amount <= 100) {
    sprintf(buf, "%ld GalCiv credits", amount);
    money->tSDesc = STRCREATE(buf);
    sprintf(buf, "Someone has carelessly discarded %ld GalCiv credits here", amount);
    Base(money)->bLDesc = STRCREATE(buf);
    sprintf(buf, "%ld GalCiv credits, the standard medium of currency exchange across\nthe entire galaxy. Too bad there isnt more...\n", amount);
    money->tDesc =  STRCREATE(buf);
  } else if (amount <= 1000) {
    money->tSDesc = STRCREATE("A stack of GalCiv credits");
    Base(money)->bLDesc = STRCREATE("Someone has carelessly discarded a stack of GalCiv credits here");
    sprintf(buf, "A stack of GalCiv credits, the standard medium of currency exchange across\nthe entire galaxy. Looks like there is about %ld credits, too bad there isnt even more...\n", amount/50*50);
    money->tDesc =  STRCREATE(buf);
  } else if (amount <= 20000) {
    money->tSDesc = STRCREATE("A pile of GalCiv credits");
    Base(money)->bLDesc = STRCREATE("Someone has carelessly discarded a pile of GalCiv credits here");
    sprintf(buf, "A pile of GalCiv credits, the standard medium of currency exchange across\nthe entire galaxy. Looks like there is about %ld credits, not too shabby...\n", amount/500*500);
    money->tDesc =  STRCREATE(buf);
  } else if (amount <= 50000) {
    money->tSDesc = STRCREATE("A huge pile of GalCiv credits");
    Base(money)->bLDesc = STRCREATE("Someone has carelessly discarded a huge pile of GalCiv credits here");
    sprintf(buf, "A huge pile of GalCiv credits, the standard medium of currency exchange across\nthe entire galaxy. Looks like there is about %ld credits, now thats more like it...\n", amount/5000*5000);
    money->tDesc =  STRCREATE(buf);
  } else {
    money->tSDesc = STRCREATE("A mountain of GalCiv credits");
    Base(money)->bLDesc = STRCREATE("Someone has carelessly discarded a mountain of GalCiv credits here");
    sprintf(buf, "A mountain of GalCiv credits, the standard medium of currency exchange across\nthe entire galaxy. Looks like there is about %ld credits, better snarf it quick...\n", amount/10000*10000);
    money->tDesc =  STRCREATE(buf);
  }

  OBJECTSETFIELD(money, OF_MONEY_AMOUNT, amount);

  return money;
}

void ObjectFree(THING *thing) {
  Obj(thing)->oTemplate->oOnline--;
  IndexDelete(&objectIndex, thing, NULL);
  if (Obj(thing)->oTemplate->oType == OTYPE_VEHICLE)
    if (Obj(thing)->oDetail.lValue[3])
      MEMFREE( (WLD*) Obj(thing)->oDetail.lValue[3], WLD );
  MEMFREE(Obj(thing), OBJ);
}


LWORD ObjectPresent(OBJTEMPLATE *template, THING *within, FLAG presentFlag){
  LWORD  present = 0;
  THING *i;

  for (i = within->tContain; i; i=i->tNext) {
    if ( (i->tType == TTYPE_OBJ) && (Obj(i)->oTemplate == template) ) {
      if ( (!Obj(i)->oEquip && BIT(presentFlag, OP_INVENTORY))
         ||(Obj(i)->oEquip && BIT(presentFlag, OP_EQUIPPED)))
        present++;
    }
  }
  return present;
}

BYTE ObjectMaxReached(OBJTEMPLATE *template, LWORD maxAllowed) {
  LWORD current;

  if (maxAllowed<0) return FALSE;
  current = template->oOnline;
  if (BIT(template->oAct, OA_TRACK_OFFLINE)) {
    current += template->oOffline;
  }
  if (current<maxAllowed) return FALSE;
  return TRUE;
}

void ObjectTick() {
  LWORD i=0;
  THING *thing=NULL;
  LWORD rotTimer;

  if (objectIndex.iNum == 0) return;
  while (1) {
    if (thing == objectIndex.iThing[i]) i++;
    if (i>=objectIndex.iNum) break;
    thing = objectIndex.iThing[i];

    /* do a tick */

    /* Check for objects "rotting" */
    if (Obj(thing)->oTemplate->oType == OTYPE_CONTAINER) {
      rotTimer = OBJECTGETFIELD(thing, OF_CONTAINER_ROT);
   
      if (rotTimer>0) {
        rotTimer--;
        if (rotTimer==0) {
          FLAG cFlag;

          SendAction("$n decays into a cloud of dust and blows away\n",
                     thing,
                     NULL,
                     SEND_ROOM|SEND_VISIBLE
          );
          /* keep players items (mobs items rot with them) */
          cFlag  = OBJECTGETFIELD(thing, OF_CONTAINER_CFLAG);
          if (BIT(cFlag, OCF_PLAYERCORPSE)) {
            while (thing->tContain)
              ThingTo(thing->tContain, Base(thing)->bInside);
          }
          THINGFREE(thing); /* Extract object from game & free its memory */
        } else
          OBJECTSETFIELD(thing, OF_CONTAINER_ROT, rotTimer);
      }
  
    /* Check for objects "rotting" */
    } else if (Obj(thing)->oTemplate->oType == OTYPE_EXIT) {
      rotTimer = OBJECTGETFIELD(thing, OF_EXIT_ROT);
   
      if (rotTimer>0) {
        rotTimer--;
        if (rotTimer==0) {
          SendAction("$n shimmers briefly and then winks out of existence\n",
                     thing,
                     NULL,
                     SEND_ROOM|SEND_VISIBLE
          );
        } else
          OBJECTSETFIELD(thing, OF_EXIT_ROT, rotTimer);
      }
    }

  }
}

void ObjectIdle(THING *object) {

  /* let code handler get first crack at it */
  if ((CodeParseIdle(object))) return;

  /* Okay now what will objs do in these situations - precious little so far */
}

/* Get the value of an object field */
LWORD ObjectGetField(BYTE oType, ODETAIL *oDetail, WORD fieldNum) {
  OTYPELIST *typeEntry;
  LWORD      entry;
  LWORD      value;
  BYTE       type;

  typeEntry = &oTypeList[oType];
  entry = typeEntry->oField[fieldNum][1];
  type = typeEntry->oField[fieldNum][0];

  switch(type) {
  case 'L':
    value = oDetail->lValue[entry];
    break;
  case 'W':
    value = oDetail->lValue[entry/2];
    value >>= (16*(entry%2));
    value &= 65535;
    value = (LWORD)((WORD) value);
    /* value = oDetail->wValue[entry]; */
    break;
  case 'B':
    value = oDetail->lValue[entry/4];
    value >>= (8*(entry%4));
    value &= 255;
    value = (LWORD)((SBYTE) value);
    /* value = oDetail->bValue[entry]; */
    break;
  }

  return value;
}

/*
 * Set the value of an object field - low level routine, intended to be
 * called by ObjectSetFieldStr mainly
 */
void ObjectSetField(BYTE oType, ODETAIL *oDetail, WORD fieldNum, LWORD value) {
  OTYPELIST *typeEntry;
  LWORD      entry;
  BYTE       type;
  LWORD      lValue;
  ULWORD     mask;
  WORD       wValue;
  BYTE       bValue;

  typeEntry = &oTypeList[oType];
  entry = typeEntry->oField[fieldNum][1];
  type = typeEntry->oField[fieldNum][0];

  switch(type) {
  case 'L':
    oDetail->lValue[entry] = value;
    break;
  case 'W':
    wValue = value;
    lValue = (wValue << (16*(entry%2)));
    mask = 65535;
    mask <<= (16*(entry%2));

    oDetail->lValue[entry/2] &= (~(mask));
    oDetail->lValue[entry/2] |= lValue;
    /* oDetail->wValue[entry] = value; */
    break;
  case 'B':
    bValue = (BYTE)(SBYTE)value;
    lValue = (bValue << (8*(entry%4)));
    mask = 255;
    mask <<= (8*(entry%4));

    oDetail->lValue[entry/4] &= (~(mask));
    oDetail->lValue[entry/4] |= lValue;
    /* oDetail->bValue[entry] = value; */
    break;
  }
}

/* Flip flags in queston */
void ObjectFlipFieldFlag(BYTE oType, ODETAIL *oDetail, WORD fieldNum, LWORD value) {
  OTYPELIST *typeEntry;
  LWORD      entry;
  BYTE       type;
  BYTE       flag;
  BYTE       bValue;
  UWORD      wValue;

  typeEntry = &oTypeList[oType];
  entry = typeEntry->oField[fieldNum][1];
  type = typeEntry->oField[fieldNum][0];
  flag = typeEntry->oField[fieldNum][2];

  /* will do nothing for non-flag fields */
  if (flag=='F') {
    switch(type) {
    case 'L': {
      BITFLIP(oDetail->lValue[entry],value);
      break;
    }
    case 'W':
      wValue = ObjectGetField(oType, oDetail, fieldNum);
      BITFLIP(wValue,value);
      ObjectSetField(oType, oDetail, fieldNum, wValue);
      break;
    case 'B':
      bValue = ObjectGetField(oType, oDetail, fieldNum);
      BITFLIP(bValue,value);
      ObjectSetField(oType, oDetail, fieldNum, bValue);
      break;
    }
  }
}

/* Look up the value of one the 16 bytes of object field information */
LWORD ObjectGetFieldNumber(BYTE *key, BYTE oType) {
  OTYPELIST *typeEntry;
  LWORD      type;

  /* assumes oType is valid */
  typeEntry = &oTypeList[oType];
  if (!*typeEntry->oTypeStr)
    return -1;

  type = TYPEFIND(key, typeEntry->oFieldStr);
  if (type == -1) {
    if (sscanf(key, " %ld", &type) < 1)
      return -1;
  }
  return type;
}

/* Look up the value of one the 16 bytes of object field information */
LWORD ObjectGetFieldStr(BYTE oType, ODETAIL *oDetail, WORD fieldNum, BYTE *fieldStr, WORD maxLen) {
  OTYPELIST *typeEntry;
  BYTE       buf[128];
  LWORD      value;
  LWORD      i;

  if (!fieldStr) return FALSE;
  if (fieldNum < 0) return FALSE;
  if (fieldNum >= OBJECT_MAX_FIELD) return FALSE;

  /* ensure valid type */
  for (i=0; i<=oType; i++) {
    typeEntry = &oTypeList[i];
    if (!*typeEntry->oTypeStr)
      return FALSE;
  }
  /* ensure valid field */
  for (i=0; i<=fieldNum; i++)
    if (!typeEntry->oField[fieldNum][0])
      return FALSE; /* whoops no field info here */

  value = ObjectGetField(oType, oDetail, fieldNum);

  /* assume buf is big enough to contain name at the very least */
  sprintf(fieldStr, "%2d.^g%-13s^G:", fieldNum, typeEntry->oFieldStr[fieldNum]);

  switch(typeEntry->oField[fieldNum][2]) {
  case 'T':
    TypeSprintf(buf, value, (ULWORD)typeEntry->oArray[fieldNum].oList, typeEntry->oArray[fieldNum].oSize, 128);
    strcat(fieldStr, buf);
    strcat(fieldStr, "\n");
    break;
  case 'F':
    FlagSprintf(buf, value, typeEntry->oArray[fieldNum].oList, ' ', 128);
    strcat(fieldStr, buf);
    strcat(fieldStr, "\n");
    break;
  default:
    sprintf(fieldStr+strlen(fieldStr), "%ld\n", value);
    break;
  }

  return TRUE;
}

/*
 * Set the value of one the 16 bytes of object field information
 * error check the fields etc.
 * Return FALSE if we cannot
 * (this is the high-level one, for cmd_obj...)
 */
LWORD ObjectSetFieldStr(THING *thing, BYTE oType, ODETAIL *oDetail, WORD fieldNum, BYTE *fieldStr) {
  OTYPELIST *typeEntry;
  LWORD      value;
  LWORD      i;

  if (fieldNum < 0) return FALSE;
  if (fieldNum >= OBJECT_MAX_FIELD) return FALSE;

  /* ensure valid type */
  for (i=0; i<=oType; i++) {
    typeEntry = &oTypeList[i];
    if (!*typeEntry->oTypeStr)
      return FALSE;
  }
  /* ensure valid fieldNum */
  for (i=0; i<=fieldNum; i++)
    if (!*typeEntry->oField[fieldNum])
      return FALSE; /* whoops no field info here */

  switch(typeEntry->oField[fieldNum][2]) {
  case 'T':
    if (!fieldStr || !*fieldStr) 
      value = -1;
    else 
      value = TypeFind(fieldStr, (ULWORD)typeEntry->oArray[fieldNum].oList, typeEntry->oArray[fieldNum].oSize);
    if (value == -1) { /* show 'em possibles */
      SendArray((ULWORD)typeEntry->oArray[fieldNum].oList, typeEntry->oArray[fieldNum].oSize, 3, thing);
      return FALSE;
    }
    ObjectSetField(oType, oDetail, fieldNum, value);
    break;
  case 'F':
    if (!fieldStr || !*fieldStr) 
      value = 0;
    else 
      value = FlagFind(fieldStr, typeEntry->oArray[fieldNum].oList);
    if (value == 0) { /* show 'em possibles */
      SENDARRAY(typeEntry->oArray[fieldNum].oList, 3, thing);
      return FALSE;
    }

    ObjectFlipFieldFlag(oType, oDetail, fieldNum, value);
    break;
  case ' ':
    if (!fieldStr || !*fieldStr)  return FALSE;
    if (sscanf(fieldStr, " %ld", &value) < 1)
      return FALSE;
    ObjectSetField(oType, oDetail, fieldNum, value);
    break;
  }

  return TRUE;
}


THING *ObjectGetAmmo(THING *thing, LWORD *aType, LWORD *aUse, LWORD *aLeft) {
  THING *ammo     = NULL;
  LWORD  ammoType = 0;
  LWORD  ammoUse  = 0;
  LWORD  ammoLeft = 0;
  
  if (thing && thing->tType==TTYPE_OBJ) {
    /* determine if we use ammo etc */
    switch (Obj(thing)->oTemplate->oType) {
    case OTYPE_LIGHT:
      ammoType = OBJECTGETFIELD(thing, OF_LIGHT_AMMOTYPE);
      ammoUse  = OBJECTGETFIELD(thing, OF_LIGHT_AMMOUSE);
      break;
    case OTYPE_SCANNER:
      ammoType = OBJECTGETFIELD(thing, OF_SCANNER_AMMOTYPE);
      ammoUse  = OBJECTGETFIELD(thing, OF_SCANNER_AMMOUSE);
      break;
    case OTYPE_DEVICE:
      ammoType = OBJECTGETFIELD(thing, OF_DEVICE_AMMOTYPE);
      ammoUse  = OBJECTGETFIELD(thing, OF_DEVICE_AMMOUSE);
      break;
    case OTYPE_WEAPON:
      ammoType = OBJECTGETFIELD(thing, OF_WEAPON_AMMOTYPE);
      ammoUse  = OBJECTGETFIELD(thing, OF_WEAPON_AMMOUSE);
      break;
    case OTYPE_ARMOR:
      ammoType = OBJECTGETFIELD(thing, OF_ARMOR_AMMOTYPE);
      ammoUse  = OBJECTGETFIELD(thing, OF_ARMOR_AMMOUSE);
      break;
    }
  
    if (thing->tContain 
    && thing->tContain->tType == TTYPE_OBJ
    && Obj(thing->tContain)->oTemplate->oType == OTYPE_AMMO)
      ammo = thing->tContain;
    if (ammo)
      ammoLeft = OBJECTGETFIELD(ammo, OF_AMMO_AMMOLEFT);
  }
  
  if (aType) *aType = ammoType;
  if (aUse)  *aUse  = ammoUse;
  if (aLeft) *aLeft = ammoLeft;
  return ammo;
}

void ObjectUseAmmo(THING *thing) {
  THING *ammo = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;

  if (!thing) return;
  ammo = ObjectGetAmmo(thing, &ammoType, &ammoUse, &ammoLeft);
  if (!ammo) return;
  if (!ammoType) return;
  if (ammoUse<=0) return;

  ammoLeft -= ammoUse;
  OBJECTSETFIELD(ammo, OF_AMMO_AMMOLEFT, ammoLeft);

  /* get rid of ammo if used up */
  if (ammoLeft<ammoUse)
    THINGFREE(ammo);
}

void ObjectShowLight(THING *show, THING *thing) {
  BYTE   buf[256];
  THING *ammo = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;
  ammo = ObjectGetAmmo(show, &ammoType, &ammoUse, &ammoLeft);

  /* Info about ammo */
  if (ammoType) {
    if (ammo) {
      if (ammoUse>0) {
        sprintf(buf, 
                "^gIt has enough power to stay lit for %ld more ticks\n", 
                ammoLeft/ammoUse);
        SendThing(buf, thing);
      } else {
        SendThing("^gIt has enough power for about a zillion more years of light!\n", thing);
      }
    } else {
      SendThing("^rIt needs ", thing);
      SendThing(oAmmoList[ammoType].oADesc, thing);
      SendThing(" to work\n", thing);
    }
  }
}

void ObjectShowScanner(THING *show, THING *thing) {
  BYTE   buf[256];
  FLAG   oFlag       = OBJECTGETFIELD(show, OF_SCANNER_SFLAG);
  LWORD  bioScanned  = OBJECTGETFIELD(show, OF_SCANNER_BIO);
  LWORD  chipScanned = OBJECTGETFIELD(show, OF_SCANNER_CHIP);
  THING *ammo        = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;
  ammo = ObjectGetAmmo(show, &ammoType, &ammoUse, &ammoLeft);
        
  if (BIT(oFlag, OSF_SCANBIO)) {
    sprintf(buf, 
            "^gIt contains ^w%ld^g credits worth of scanned biologicals\n", 
            bioScanned);
    SendThing(buf, thing);
  }
  if (BIT(oFlag, OSF_SCANCHIP)) {
    sprintf(buf, 
            "^gIt contains ^w%ld^g credits worth of scanned chip technology\n", 
            chipScanned);
    SendThing(buf, thing);
  }
  sprintf(buf, 
          "^gIt has %ld units of free memory left\n", 
          OBJECTGETFIELD(show, OF_SCANNER_MAX) - bioScanned - chipScanned);
  SendThing(buf, thing);

  /* Info about ammo */
  if (ammoType) {
    if (ammo) {
      if (ammoUse>0) {
        sprintf(buf, 
                "^gIt has enough power for %ld more scans\n", 
                ammoLeft/ammoUse);
        SendThing(buf, thing);
      } else {
        SendThing("^gIt has enough power for about a zillion more scans!\n", thing);
      }
    } else {
      SendThing("^rIt needs ", thing);
      SendThing(oAmmoList[ammoType].oADesc, thing);
      SendThing(" to work\n", thing);
    }
  }
}

void ObjectShowDevice(THING *show, THING *thing) {
  BYTE   buf[256];
  THING *ammo = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;
  ammo = ObjectGetAmmo(show, &ammoType, &ammoUse, &ammoLeft);

  /* Info about ammo */
  if (ammoType) {
    if (ammo) {
      if (ammoUse>0) {
        sprintf(buf, 
                "^gIt has enough power for for %ld more uses\n", 
                ammoLeft/ammoUse);
        SendThing(buf, thing);
      } else {
        SendThing("^gIt can be used about a zillion more times!\n", thing);
      }
    } else {
      SendThing("^rIt needs ", thing);
      SendThing(oAmmoList[ammoType].oADesc, thing);
      SendThing(" to work\n", thing);
    }
  }
}

void ObjectShowWeapon(THING *show, THING *thing) {
  BYTE   buf[256];
  THING *ammo = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;
  LWORD  weaponType;
  ammo = ObjectGetAmmo(show, &ammoType, &ammoUse, &ammoLeft);

  /* Info about ammo */
  if (ammoType) {
    if (ammo) {
      if (ammoUse>0) {
        sprintf(buf, 
                "^gIt has enough ammo for %ld more shots\n", 
                ammoLeft/ammoUse);
        SendThing(buf, thing);
      } else {
        SendThing("^gIt can be used about a zillion more times!\n", thing);
      }
    } else {
      SendThing("^rIt needs ", thing);
      SendThing(oAmmoList[ammoType].oADesc, thing);
      SendThing(" to work\n", thing);
    }
  }
  /* Skill to use */
  weaponType = OBJECTGETFIELD(show, OF_WEAPON_TYPE);
  SendHint("^;HINT: Practice ^<", thing);
  TYPESPRINTF(buf, weaponType, weaponList, sizeof(buf));
  SendHint(buf, thing);
  SendHint(" ^;to improve with this weapon\n", thing);
}

BYTE ObjectShowContainer(THING *show, THING *thing) {
  FLAG oFlag = OBJECTGETFIELD(show, OF_CONTAINER_CFLAG);
  
  if (BIT(oFlag, OCF_CLOSED))
    SendThing("^gIts Closed...\n", thing);
  if (BIT(oFlag, OCF_LOCKED))
    SendThing("^gIts Locked...\n", thing);
  /* Cant see inside a closed object */
  if (BIT(oFlag, OCF_CLOSED))
    return FALSE;
  if (!show->tContain)
    SendThing("^gIts Empty...\n", thing);
    
  return TRUE;
}

/* Should show "Its about half full of <LDesc>" */
void ObjectShowDrinkcon(THING *show, THING *thing) {
  BYTE  buf[255];
  LWORD dMax     = OBJECTGETFIELD(show, OF_DRINKCON_MAX);
  LWORD dContain = OBJECTGETFIELD(show, OF_DRINKCON_CONTAIN);
  LWORD dLiquid  = OBJECTGETFIELD(show, OF_DRINKCON_LIQUID);
  LWORD ratio;

  /* negative dContain is an infinite source */
  if (dMax <= 0)
    ratio = 0;
  else if (dContain < 0)
    ratio = 100;
  else
    ratio = 100 * dContain / dMax;
  TYPECHECK(dLiquid, oLiquidList); /* ensure no out of range errors */
  
  if (ratio <= 0)
    sprintf(buf, "^wIts empty\n");
  else if (ratio <= 7)
    sprintf(buf, "^wIt contains %s, but its almost empty\n", oLiquidList[dLiquid].oLDesc);
  else if (ratio <= 19)
    sprintf(buf, "^wIts less than a quarter full of %s\n", oLiquidList[dLiquid].oLDesc);
  else if (ratio <= 29)
    sprintf(buf, "^wIts about a quarter full of %s\n", oLiquidList[dLiquid].oLDesc);
  else if (ratio <= 41)
    sprintf(buf, "^wIts about a third full of %s\n", oLiquidList[dLiquid].oLDesc);
  else if (ratio <= 58)
    sprintf(buf, "^wIts about half full of %s\n", oLiquidList[dLiquid].oLDesc);
  else if (ratio <= 71)
    sprintf(buf, "^wIts about two-thirds full of %s\n", oLiquidList[dLiquid].oLDesc);
  else if (ratio <= 81)
    sprintf(buf, "^wIts about three-quarters full of %s\n", oLiquidList[dLiquid].oLDesc);
  else if (ratio <= 93)
    sprintf(buf, "^wIts more than three-quarters full of %s\n", oLiquidList[dLiquid].oLDesc);
  else if (ratio <= 99)
    sprintf(buf, "^wIts almost full of %s\n", oLiquidList[dLiquid].oLDesc);
  else
    sprintf(buf, "^wIts completely full of %s!\n", oLiquidList[dLiquid].oLDesc);

  SendThing(buf, thing);
}

void ObjectShowArmor(THING *show, THING *thing) {
  BYTE   buf[256];
  THING *ammo = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;
  ammo = ObjectGetAmmo(show, &ammoType, &ammoUse, &ammoLeft);

  /* Info about ammo */
  if (ammoType) {
    if (ammo) {
      if (ammoUse>0) {
        sprintf(buf, 
                "^gIt has enough power for for %ld more minutes\n", 
                ammoLeft/ammoUse);
        SendThing(buf, thing);
      } else {
        SendThing("^gIt has enough power left to keep running for eternity!\n", thing);
      }
    } else {
      SendThing("^rIt needs ", thing);
      SendThing(oAmmoList[ammoType].oADesc, thing);
      SendThing(" to work\n", thing);
    }
  }
}

