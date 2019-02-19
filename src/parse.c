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
#include <ctype.h>

#include "crimson2.h"
#include "macro.h"
#include "queue.h"
#include "log.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "code.h"
#include "file.h"
#include "thing.h"
#include "exit.h"
#include "index.h"
#include "social.h"
#include "help.h"
#include "area.h"
#include "world.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "alias.h"
#include "send.h"
#include "base.h"
#include "char.h"
#include "player.h"
#include "moledefs.h"
#include "cmd_move.h"
#include "cmd_talk.h"
#include "cmd_inv.h"
#include "cmd_cbt.h"
#include "cmd_misc.h"
#include "cmd_god.h"
#include "cmd_area.h"
#include "cmd_wld.h"
#include "cmd_mob.h"
#include "cmd_obj.h"
#include "cmd_rst.h"
#include "cmd_help.h"
#include "cmd_code.h"
#include "cmd_brd.h" /* board stuff */
#include "cmd_mole.h"
#include "parse.h"



BYTE *commandFlagList[] = {
  "MOVE",
  "TALK",
  "MISC",
  "GOD",
  "AREAEDIT",
  "INVENTORY",
  "COMBAT",
  "HELPEDIT",
  "AREA",
  "WLD",
  "MOB",
  "OBJ",
  "RST",
  "BRD",
  "VNUMARG",
  "C4CODING",
  ""
};



/* text, proc, min_pos, min_lev, log, cmd flags */
const COMMANDLIST commandList[] = {
/* Movement */
  { "go",         CmdGo,       POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "north",      CmdNorth,    POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "east",       CmdEast,     POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "south",      CmdSouth,    POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "west",       CmdWest,     POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "up",         CmdUp,       POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "down",       CmdDown,     POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "out",        CmdOut,      POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "enter",      CmdGo,       POS_FIGHTING,  1,            LOG_NEVER,  CF_MOVE },
  { "exits",      CmdExit,     POS_RESTING,   1,            LOG_NEVER,  CF_MOVE },
  { "look",       CmdLook,     POS_RESTING,   1,            LOG_NEVER,  CF_MOVE },
  { "examine",    CmdLook,     POS_RESTING,   1,            LOG_NEVER,  CF_MOVE },
  { "scan",       CmdScan,     POS_SITTING,   1,            LOG_NEVER,  CF_MOVE },
  { "open",       CmdOpen,     POS_SITTING,   1,            LOG_NEVER,  CF_MOVE },
  { "close",      CmdClose,    POS_SITTING,   1,            LOG_NEVER,  CF_MOVE },
  { "unlock",     CmdUnlock,   POS_SITTING,   1,            LOG_NEVER,  CF_MOVE },
  { "lock",       CmdLock,     POS_SITTING,   1,            LOG_NEVER,  CF_MOVE },
  { "picklock",   CmdPicklock, POS_SITTING,   1,            LOG_NEVER,  CF_MOVE },
  { "hack",       CmdHack,     POS_SITTING,   1,            LOG_NEVER,  CF_MOVE },
  { "stand",      CmdStand,    POS_SLEEPING,  1,            LOG_NEVER,  CF_MOVE },
  { "sit",        CmdSit,      POS_SLEEPING,  1,            LOG_NEVER,  CF_MOVE },
  { "rest",       CmdRest,     POS_SITTING,   1,            LOG_NEVER,  CF_MOVE },
  { "sleep",      CmdSleep,    POS_RESTING,   1,            LOG_NEVER,  CF_MOVE },
  { "wake",       CmdWake,     POS_SLEEPING,  1,            LOG_NEVER,  CF_MOVE },
  { "track",      CmdTrack,    POS_STANDING,  1,            LOG_NEVER,  CF_MOVE },
  { "sneak",      CmdSneak,    POS_STANDING,  1,            LOG_NEVER,  CF_MOVE },
/* Combat */
  { "hit",        CmdHit,      POS_STANDING,  1,            LOG_NORMAL, CF_CBT  },
  { "kill",       CmdHit,      POS_STANDING,  1,            LOG_NORMAL, CF_CBT  },
  { "shoot",      CmdHit,      POS_STANDING,  1,            LOG_NORMAL, CF_CBT  },
  { "group",      CmdGroup,    POS_SITTING,   1,            LOG_NEVER,  CF_CBT  },
  { "follow",     CmdFollow,   POS_STANDING,  1,            LOG_NEVER,  CF_CBT  },
  { "concentrate",CmdConcentrate,POS_FIGHTING,1,            LOG_NORMAL, CF_CBT  },
  { "practice",   CmdPractice, POS_STANDING,  1,            LOG_NORMAL, CF_CBT  },
  { "consider",   CmdConsider, POS_STANDING,  1,            LOG_NEVER,  CF_CBT  },
  { "flee",       CmdFlee,     POS_FIGHTING,  1,            LOG_NORMAL, CF_CBT  },
  { "rescue",     CmdRescue,   POS_FIGHTING,  1,            LOG_NORMAL, CF_CBT  },
  { "hide",       CmdHide,     POS_STANDING,  1,            LOG_NORMAL, CF_CBT  },
  { "peek",       CmdPeek,     POS_STANDING,  1,            LOG_NEVER,  CF_CBT  },
  { "steal",      CmdSteal,    POS_STANDING,  1,            LOG_NORMAL, CF_CBT  },
  { "pickpocket", CmdPickpocket,POS_STANDING, 1,            LOG_NORMAL, CF_CBT  },
/* Inventory */
  { "drop",       CmdDrop,     POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "get",        CmdGet,      POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "take",       CmdGet,      POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "pickup",     CmdGet,      POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "junk",       CmdJunk,     POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "put",        CmdPut,      POS_SITTING,   1,            LOG_NEVER,  CF_INV  },
  { "give",       CmdGive,     POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "equip",      CmdEquip,    POS_SITTING,   1,            LOG_NEVER,  CF_INV  },
  { "wear",       CmdEquip,    POS_SITTING,   1,            LOG_NEVER,  CF_INV  },
  { "wield",      CmdEquip,    POS_SITTING,   1,            LOG_NEVER,  CF_INV  },
  { "hold",       CmdEquip,    POS_SITTING,   1,            LOG_NEVER,  CF_INV  },
  { "unequip",    CmdUnEquip,  POS_SITTING,   1,            LOG_NEVER,  CF_INV  },
  { "remove",     CmdUnEquip,  POS_SITTING,   1,            LOG_NEVER,  CF_INV  },
  { "inventory",  CmdInventory,POS_DEAD,      1,            LOG_NEVER,  CF_INV  },
  { "use",        CmdUse,      POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "eat",        CmdEat,      POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "drink",      CmdDrink,    POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "fill",       CmdFill,     POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "empty",      CmdEmpty,    POS_SITTING,   1,            LOG_NORMAL, CF_INV  },
  { "read",       CmdRead,     POS_RESTING,   1,            LOG_NORMAL, CF_INV  },
  { "write",      CmdWrite,    POS_RESTING,   1,            LOG_NORMAL, CF_INV  },
  { "reply",      CmdReply,    POS_RESTING,   1,            LOG_NORMAL, CF_INV  },
  { "edit",       CmdEdit,     POS_RESTING,   1,            LOG_NORMAL, CF_INV  },
  { "erase",      CmdErase,    POS_RESTING,   1,            LOG_NORMAL, CF_INV  },
/* Talking */
  { "gossip",     CmdGossip,   POS_SLEEPING,  1,            LOG_NORMAL, CF_TALK },
  { ".",          CmdGossip,   POS_SLEEPING,  1,            LOG_NORMAL, CF_TALK },
  { "auction",    CmdAuction,  POS_SLEEPING,  1,            LOG_NORMAL, CF_TALK },
  { "godtalk",    CmdGodtalk,  POS_SLEEPING,  LEVEL_GOD,    LOG_NORMAL, CF_TALK |CF_AREAEDIT},
  { "emote",      CmdEmote,    POS_SLEEPING,  1,            LOG_NORMAL, CF_TALK },
  { ":",          CmdEmote,    POS_SLEEPING,  1,            LOG_NORMAL, CF_TALK },
  { "say",        CmdSay,      POS_RESTING,   1,            LOG_NORMAL, CF_TALK },
  { "ask",        CmdSay,      POS_RESTING,   1,            LOG_NORMAL, CF_TALK },
  { "tell",       CmdTell,     POS_RESTING,   1,            LOG_NORMAL, CF_TALK },
  { "respond",    CmdRespond,  POS_RESTING,   1,            LOG_NORMAL, CF_TALK },
  { "order",      CmdOrder,    POS_RESTING,   1,            LOG_NORMAL, CF_TALK },
  { "gtell",      CmdGTell,    POS_SLEEPING,  1,            LOG_NORMAL, CF_TALK },
  { ";",          CmdGTell,    POS_RESTING,   1,            LOG_NORMAL, CF_TALK },
  { "rainbow",    CmdRainbow,  POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_TALK },
  { "chhistory",  CmdChHistory,POS_SLEEPING,  1,            LOG_NEVER,  CF_TALK },
  { "beep",       CmdBeep,     POS_SLEEPING,  1,            LOG_NORMAL, CF_TALK },
/* Miscellaneous */
  { "set",        CmdSet,      POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "setcolor",   CmdSetColor, POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "score",      CmdScore,    POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "title",      CmdTitle,    POS_DEAD,      1,            LOG_NORMAL, CF_MISC },
  { "socials",    NULL,        POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "alias",      CmdAlias,    POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "unalias",    CmdUnAlias,  POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "who",        CmdWho,      POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "where",      CmdWhere,    POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "time",       CmdTime,     POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "entermsg",   CmdEnterMsg, POS_SLEEPING,  1,            LOG_NEVER,  CF_MISC },
  { "exitmsg",    CmdExitMsg,  POS_SLEEPING,  1,            LOG_NEVER,  CF_MISC },
  { "prompt",     CmdPrompt,   POS_SLEEPING,  1,            LOG_NEVER,  CF_MISC },
  { "finger",     CmdFinger,   POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "levels",     CmdLevels,   POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "skillmax",   CmdSkillMax, POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "affects",    CmdAffects,  POS_DEAD,      1,            LOG_NEVER,  CF_MISC },
  { "stop",       CmdStop,     POS_RESTING,   1,            LOG_NEVER,  CF_MISC },
  { "area",       CmdArea,     POS_RESTING,   1,            LOG_NEVER,  CF_MISC },
/* God */
  { "heal",       CmdHeal,     POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_GOD|CF_AREAEDIT  },
  { "purge",      CmdPurge,    POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_GOD|CF_AREAEDIT  },
  { "transfer",   CmdTransfer, POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_GOD  },
  { "setskill",   CmdSetSkill, POS_DEAD,      LEVEL_ADMIN,  LOG_ALLWAYS,CF_GOD  },
  { "setstat",    CmdSetStat,  POS_DEAD,      LEVEL_ADMIN,  LOG_ALLWAYS,CF_GOD  },
  { "setprop",    CmdSetProp,  POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_GOD|CF_AREAEDIT  },
  { "goto",       CmdGoto,     POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_GOD  },
  { "vnum",       CmdVNum,     POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_GOD|CF_AREAEDIT  },
  { "stat",       CmdStat,     POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_GOD|CF_AREAEDIT  },
  { "at",         CmdAt,       POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_GOD  },
  { "force",      CmdForce,    POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_GOD  },
  { "cutconn",    CmdCutConn,  POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_GOD  },
  { "delplayer",  CmdDelPlayer,POS_DEAD,      LEVEL_ADMIN,  LOG_ALLWAYS,CF_GOD  },
  { "undelplayer",CmdUnDelPlayer,POS_DEAD,    LEVEL_ADMIN,  LOG_ALLWAYS,CF_GOD  },
  { "snoop",      CmdSnoop,    POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_GOD  },
  { "site",       CmdSite,     POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_GOD  },
  { "system",     CmdSystem,   POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_GOD  },
  { "users",      CmdUsers,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_GOD  },
  { "switch",     CmdSwitch,   POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_GOD  },
  { "resetskill", CmdResetSkill,POS_DEAD,     LEVEL_GOD,    LOG_ALLWAYS,CF_GOD  },
/*
  { "shutdown",   CmdShutdown, POS_DEAD,      LEVEL_ADMIN,  LOG_ALLWAYS,CF_GOD  },
*/
  { "reboot",     CmdReboot,   POS_DEAD,      LEVEL_ADMIN,  LOG_ALLWAYS,CF_GOD  },

/* Coding commands */
  { "compile",    CmdComp,     POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_C4CODING|CF_AREAEDIT  },
  { "disass",     CmdDisass,   POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_C4CODING|CF_AREAEDIT  },
  { "decompile",  CmdDecomp,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_C4CODING|CF_AREAEDIT  },
  { "dump",       CmdDump,     POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_C4CODING|CF_AREAEDIT  },
  { "c4snoop",    CmdC4Snoop,  POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_C4CODING|CF_AREAEDIT },
  { "flist",      CmdFList,    POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_C4CODING|CF_AREAEDIT },

/* Area Editing */
  { "alist",      CmdAList,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_AREA|CF_AREAEDIT },
  { "astat",      CmdAStat,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_AREA|CF_AREAEDIT|CF_VNUMARG},
  { "adesc",      CmdADesc,    POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_AREA|CF_AREAEDIT|CF_VNUMARG},
  { "aflag",      CmdAFlag,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_AREA|CF_VNUMARG},
  { "adelay",     CmdADelay,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_AREA|CF_AREAEDIT },
  { "areset",     CmdAReset,   POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_AREA|CF_AREAEDIT|CF_VNUMARG },
  { "areboot",    CmdAReboot,  POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_AREA|CF_AREAEDIT|CF_VNUMARG },
  { "aproperty",  CmdAProperty,POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_AREA|CF_AREAEDIT },
  { "acompile",   CmdACompile, POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_AREA|CF_AREAEDIT },
  { "adecomp",    CmdADecomp,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_AREA|CF_AREAEDIT },
  { "asave",      CmdASave,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_AREA|CF_AREAEDIT|CF_VNUMARG },

/* World Editing */
  { "wstat",      CmdWStat,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wgoto",      CmdWGoto,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wlist",      CmdWList,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wcreate",    CmdWCreate,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT },
  { "wcopy",      CmdWCopy,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wname",      CmdWName,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wdesc",      CmdWDesc,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wexit",      CmdWExit,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wedesc",     CmdWEDesc,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wekey",      CmdWEKey,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wekeyobj",   CmdWEKeyObj, POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "weflag",     CmdWEFlag,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wextra",     CmdWExtra,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wproperty",  CmdWProperty,POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wset",       CmdWSet,     POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wcompile",   CmdWCompile, POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wdecomp",    CmdWDecomp,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_WLD|CF_AREAEDIT|CF_VNUMARG },
  { "wsave",      CmdWSave,    POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_WLD|CF_AREAEDIT|CF_VNUMARG },

/* Mob Editing */
  { "mstat",      CmdMStat,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mlist",      CmdMList,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mcreate",    CmdMCreate,  POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT },
  { "mname",      CmdMName,    POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mkey",       CmdMKey,     POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mldesc",     CmdMLDesc,   POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mdesc",      CmdMDesc,    POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mextra",     CmdMExtra,   POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mproperty",  CmdMProperty,POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mset",       CmdMSet,     POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mcompile",   CmdMCompile, POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mdecomp",    CmdMDecomp,  POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },
  { "mload",      CmdMLoad,    POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT },
  { "msave",      CmdMSave,    POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_MOB|CF_AREAEDIT|CF_VNUMARG },

/* Obj Editing */
  { "ostat",      CmdOStat,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "olist",      CmdOList,    POS_DEAD,      LEVEL_GOD,    LOG_NEVER,  CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "ocreate",    CmdOCreate,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "oclear",     CmdOClear,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "oname",      CmdOName,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "okey",       CmdOKey,     POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "oldesc",     CmdOLDesc,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "odesc",      CmdODesc,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "oextra",     CmdOExtra,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "oproperty",  CmdOProperty,POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "oset",       CmdOSet,     POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
/* - Called by OSet now... 
  { "osetfield",  CmdOSetField,POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT },
*/
  { "ocompile",   CmdOCompile, POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "odecomp",    CmdODecomp,  POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_OBJ|CF_AREAEDIT|CF_VNUMARG },
  { "oload",      CmdOLoad,    POS_DEAD,      LEVEL_GOD,    LOG_NORMAL, CF_OBJ|CF_AREAEDIT },
  { "osave",      CmdOSave,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_OBJ|CF_AREAEDIT|CF_VNUMARG },

/* Reset Editing */
  { "rshow",      CmdRShow,    POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_RST|CF_AREAEDIT },
  { "rcreate",    CmdRCreate,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_RST|CF_AREAEDIT },
  { "rset",       CmdRSet,     POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_RST|CF_AREAEDIT },
  { "rfirst",     CmdRFirst,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_RST|CF_AREAEDIT },
  { "rupdate",    CmdRUpdate,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_RST|CF_AREAEDIT },
  { "rhide",      CmdRHide,    POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_RST|CF_AREAEDIT },
  { "rsave",      CmdRSave,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_RST|CF_AREAEDIT },

/* HelpFile Editing */
  { "hlist",      CmdHList,    POS_DEAD,      1,            LOG_NEVER,  CF_HELPEDIT },
  { "hcreate",    CmdHCreate,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_HELPEDIT },
  { "hkey",       CmdHKey,     POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_HELPEDIT },
  { "hdesc",      CmdHDesc,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_HELPEDIT },
  { "hdelete",    CmdHDelete,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_HELPEDIT },
  { "hsave",      CmdHSave,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_HELPEDIT },

/* Board Editing */
  { "blist",      CmdBList,    POS_DEAD,      LEVEL_ADMIN,  LOG_NEVER,  CF_BRD },
  { "bcreate",    CmdBCreate,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_BRD },
  { "beditor",    CmdBEditor,  POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_BRD },
  { "bname",      CmdBName,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_BRD },
  { "bedit",      CmdBEdit,    POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_BRD },
  { "bwrite",     CmdBWrite,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_BRD },
  { "breply",     CmdBReply,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_BRD },
  { "berase",     CmdBErase,   POS_DEAD,      LEVEL_ADMIN,  LOG_NORMAL, CF_BRD },

/* MOLE support */
  { MOLE_HDR,     CmdMOLE,     POS_DEAD,      LEVEL_NEWBIE,  LOG_NEVER, CF_MISC },

/* And Finally... */
  { "quit",       CmdQuit,     POS_DEAD,      1,            LOG_NEVER,  CF_MISC     },

  { "",           NULL,        0,             0,            0,          0 }
};

LWORD PARSE_COMMAND_OSTAT;
LWORD PARSE_COMMAND_WGOTO;
LWORD PARSE_COMMAND_WCREATE;
LWORD PARSE_COMMAND_MSTAT;
LWORD PARSE_COMMAND_RCREATE;
LWORD PARSE_COMMAND_WEXIT;

void ParseInit(void) {
  PARSE_COMMAND_OSTAT   = TYPEFIND("ostat", commandList);
  PARSE_COMMAND_WGOTO   = TYPEFIND("wgoto", commandList);
  PARSE_COMMAND_WCREATE = TYPEFIND("wcreate", commandList);
  PARSE_COMMAND_MSTAT   = TYPEFIND("mstat", commandList);
  PARSE_COMMAND_RCREATE = TYPEFIND("rcreate", commandList);
  PARSE_COMMAND_WEXIT   = TYPEFIND("wexit", commandList);
}

/* find a word that means something to the interpreter */
BYTE *ParseGetWord(BYTE *cmd, BYTE *word) {
  LWORD i;
  BYTE *ignoreList[] = {
    "into",
    "in",
    "inside",
    "from"
    "of",
    "to",
    "the",
    "a",
    "an",
    "at",
    "",
  };

  /* find the first non-ignored word */
  do {
    cmd = StrOneWord(cmd, word); /* strip off one word */
    if (!*cmd) return cmd; /* return if there arent any more words */

    /* ignore the word if its ignorable */
    for(i=0; *ignoreList[i] && STRICMP(ignoreList[i], word); i++);
  } while(*ignoreList[i]);
  return cmd;
}

/* sets num in the event of 1, 2 etc
   sets offset in the event of 1st,2nd,3rd,4th etc
   will only change num if num was previously 0
   ditto for offset
   will return TRUE if there was a match of some kind
*/
BYTE ParseOffset(BYTE *word, LWORD *offset, LWORD *num) {
  LWORD i;
  LWORD sLen;

  /* well this cant possibly be right can it */
  if (!*word) return FALSE;

  /* this could be an offset ie as in get the 3rd bread */
  sLen = strlen(word);
  if (sLen >= 3) { /* otherwise cant be a offset specifier */
    if (!STRICMP(&word[sLen-2], "st") /* ie 1st */
      ||!STRICMP(&word[sLen-2], "nd") /* ie 2nd */
      ||!STRICMP(&word[sLen-2], "rd") /* ie 3rd */
      ||!STRICMP(&word[sLen-2], "th") /* ie 4th */
    ) {
      for (i=0; i<sLen-2 && isdigit(word[i]); i++);
      if (i==sLen-2) { /* all numeric digits */
        word[sLen-2] = '\0';
        if (offset && *offset==0) *offset = atol(word);
        return TRUE;
      }
    }
  }

  /* this could be a # ie as in get 3 bread */
  for (i=0; i<sLen && isdigit(word[i]); i++);
  if (i==sLen) { /* all numeric digits */
    if (num && *num==0) *num =  atol(word);
    return TRUE;
  }

  return FALSE;
}

/* sets offset to 0 for no match, or whatever they picked
   it will only change offset if offset was previously 0
   it will only change num if num was previously 0
   in the event of all.keyword, num will be set to TF_ALLMATCH(ie -1)
*/
BYTE *ParseEmbeddedOffset(BYTE *word, LWORD *offset, LWORD *num) {
  LWORD i;
  LWORD sLen;

  sLen = strlen(word);
  if (sLen < 3) {
    return word; /* cant be a offset specifier */
  }
  for (i=0; i<sLen && word[i]!='.'; i++);
  if (word[i] == '.') {
    word[i] = '\0';
    if (!STRICMP(word, "all")) {
      if(num && *num==0) *num = TF_ALLMATCH;
    } else {
      if(offset && *offset==0) *offset = atol(word);
    }
    return &word[i+1];
  }
  return word; /* cant be a offset specifier */
}

/* will give you everything you need to know, takes apart a line
   and looks for get 2.bread, get all.bread, drop the 4th bread etc etc
 */
BYTE *ParseFind(BYTE *cmd, BYTE *srcKey, LWORD *srcOffset, LWORD *srcNum, BYTE *dstKey, LWORD *dstOffset) {
  BYTE  word[256];
  BYTE *str;

  if (!srcKey || !srcOffset || !srcNum) return cmd;

  *srcKey    = '\0';
  *srcNum    = 0;
  *srcOffset = 0;

  /* find the first non-ignored word */
  cmd = ParseGetWord(cmd, word); /* get the first word */

  /* find a offset like 1st 2nd etc */
  while(ParseOffset(word, srcOffset, srcNum)) cmd=ParseGetWord(cmd, word);

  /* look for standard diku kind of offset & key */
  str = ParseEmbeddedOffset(word, srcOffset, srcNum);
  strcpy(srcKey, str);

  /* if something hasnt been chosen by now, fill in defaults */
  if (*srcNum==0) *srcNum = 1;
  if (!STRICMP(srcKey, "all")) {
    *srcNum = TF_ALLMATCH;
    if (!dstKey) {
      cmd = StrOneWord(cmd, NULL);
      if (*cmd) strcpy(srcKey, cmd);
    }
  }
  if (*srcOffset==0) *srcOffset = 1;

  /* Should we try to find dest key and offset? */
  if (!dstOffset || !dstKey) return cmd;

  *dstKey    = '\0';
  *dstOffset = 0;

  /* find a offset like 1st 2nd etc */
  while(ParseOffset(word, dstOffset, NULL)) cmd=ParseGetWord(cmd, word);

  /* look for standard diku kind of offset & key */
  cmd = ParseGetWord(cmd, word); /* get the next word */
  str = ParseEmbeddedOffset(word, dstOffset, NULL);
  if (dstKey) strcpy(dstKey, str);

  /* if something hasnt been chosen by now, fill in defaults */
  if (*dstOffset==0) *dstOffset = 1;

  return cmd;
}

BYTE ParseCommandCheck(LWORD i, SOCK *sock, BYTE *cmd) {
  BYTE buf[256];
  WORD numScan;
  WORD virtual;
  WORD area;

  /* guard against mobs */
  if (!sock) {
    if (commandList[i].cLevel >= LEVEL_GOD)
      return FALSE;
    else
      return TRUE;
  }

  /* level check */
  if (Character(sock->sHomeThing)->cLevel >= commandList[i].cLevel)
    return TRUE;

  /* special flags check, ie areaedit */
  if (BITANY(commandList[i].cFlag, CF_AREAEDIT)) {
    numScan = 0;
    if (BITANY(commandList[i].cFlag, CF_VNUMARG)) {
      cmd = StrOneWord(cmd, NULL); /* ignore command */
      StrOneWord(cmd, buf); /* get first arg */
      numScan = sscanf(buf, " %hd", &virtual);
    }
    if (numScan < 1) {
      if (Base(sock->sHomeThing)->bInside 
       && Base(sock->sHomeThing)->bInside->tType == TTYPE_WLD)
        virtual = Wld(Base(sock->sHomeThing)->bInside)->wVirtual;
      else
        virtual = -1;
    }
    if (virtual < 0) return FALSE;
    area = AreaOf(virtual);
    if (area < 0) return FALSE;
    if (AreaIsEditor(area,sock->sHomeThing)==2) {
      return TRUE;
    }
  }

  /* fallthru to here */
  return FALSE;
}

BYTE ParsePositionCheck(LWORD i, THING *thing) {
  if (Character(thing)->cPos >= commandList[i].cPos)
    return TRUE;

  switch (Character(thing)->cPos) {
  case POS_DEAD:
    SendThing("^wI'm afraid you're quite dead, so that's out of the question\n", thing);
    break;
  case POS_MORTAL:
    SendThing("^wRight at the moment it's all you can do to lie there and bleed\n", thing);
    break;
  case POS_INCAP:
    SendThing("^wTry again later, right now you're completely incapacitated\n", thing);
    break;
  case POS_STUNNED:
    SendThing("^wWhoa, maybe I'll wait for everything to stop spinning around me first\n", thing);
    break;
  case POS_SLEEPING:
    SendThing("^wWhat and ruin a perfectly good nap?\n", thing);
    break;
  case POS_RESTING:
    SendThing("^wBut its just so comfortable lying down right here\n", thing);
    break;
  case POS_SITTING:
    SendThing("^wPerchance it might be easier to do that if you werent sitting down\n", thing);
    break;
  case POS_FIGHTING:
    SendThing("^wConcentrating on your current battle might be a good idea...\n", thing);
    break;
  default:
    SendThing("^wAlas and alack, that command is not within your powers\n", thing);
  }
  return FALSE;
}

LWORD ParseCommand(THING *thing, BYTE *cmd) {
  LWORD     i;
  BYTE     *word;
  BYTE      wordBuf[256];
  BYTE      buf[256];
  SOCK     *sock;
  BASELINK *link;

  word = wordBuf;
  StrOneWord(cmd, word);
  if (!*cmd)
    return FALSE;
  sock = BaseControlFind(thing);

  if (sock) {
    /* Translate 'message to say message for Realmers */
    if (sock && cmd[0] == '\'') {
      QInsert(sock->sIn, "\n");
      QInsert(sock->sIn, cmd+1);
      QInsert(sock->sIn, "say ");
      return TRUE;
    }
    /* Translate 'message to say message for Realmers */
    if (sock && cmd[0] == ':') {
      QInsert(sock->sIn, "\n");
      QInsert(sock->sIn, cmd+1);
      QInsert(sock->sIn, "emote ");
      return TRUE;
    }
    /* Expand aliases */
    if (AliasParse(sock, cmd)) return TRUE;
  }

  /* to prevent aliases from recursing when we do something like
   * alias wake = wake; stand - it embeds ##ALIAS in front
   * that way we'll get past Alias parse above without
   * expanding the internal wake command infinitely
   */
  if (!strncmp(word, ALIAS_COMMAND_STR, strlen(ALIAS_COMMAND_STR))) {
    word = &(word[strlen(ALIAS_COMMAND_STR)]);
  }

  /* Tell us what they did if we are snooping */
  if (strncmp(MOLE_HDR, cmd, strlen(MOLE_HDR))) {
    if (Base(thing)->bInside->tType==TTYPE_WLD) {
      if (!BIT(Wld(Base(thing)->bInside)->wFlag,WF_PRIVATE)) {
        for (link = Base(thing)->bLink; link; link=link->lNext) {
          if (link->lType == BL_TELEPATHY_SND) {
            SEND("^w%", BaseControlFind(link->lDetail.lThing));
            SEND(cmd, BaseControlFind(link->lDetail.lThing));
            SEND("\n", BaseControlFind(link->lDetail.lThing));
          }
        }
      }
    }
  }

  i = TYPEFIND(word, commandList);
  if (i!=-1 && commandList[i].cProc) { /* well, well, well, this *IS* a valid command, whaddya know */

    /* Choke if they are too weeny */
    if (!ParseCommandCheck(i, sock, cmd)){
      if (sock)
        SEND("^wAlas and alack, that command is not within your powers\n", sock);
      return FALSE;
    }

    /* make sure they are not sleeping etc */
    if (!ParsePositionCheck(i, thing)){
      return FALSE;
    }

    if (sock) {
      /* Log Player if its an interesting command and log flag set */
      if( (commandList[i].cLog == LOG_ALLWAYS)
        ||( (BIT(Plr(sock->sHomeThing)->pSystem, PS_LOG))
          &&(commandList[i].cLog == LOG_NORMAL) )
        ) {
        LogStr(sock->sHomeThing->tSDesc->sText, cmd);
        LogStrPrintf(sock->sHomeThing->tSDesc->sText, "\n");
      }
    }

    /* Log area editing to area channel */
    if( (commandList[i].cLog >= LOG_NORMAL)
      &&(BIT(commandList[i].cFlag, CF_AREAEDIT)) ) {
      Log(LOG_AREA, sock->sHomeThing->tSDesc->sText);
      if (Base(thing)->bInside && Base(thing)->bInside->tType == TTYPE_WLD) {
        sprintf(buf, " [%ld]", Wld(Base(thing)->bInside)->wVirtual);
        LogPrintf(LOG_AREA, buf);
      }
      LogPrintf(LOG_AREA, ": ");
      LogPrintf(LOG_AREA, cmd);
      LogPrintf(LOG_AREA, "\n");
    }

    /* Log god commands to god channel */
    if( (commandList[i].cLog >= LOG_NORMAL)
      &&(BIT(commandList[i].cFlag, CF_GOD)) ) {
      Log(LOG_GOD, sock->sHomeThing->tSDesc->sText);
      LogPrintf(LOG_GOD, ": ");
      LogPrintf(LOG_GOD, cmd);
      LogPrintf(LOG_GOD, "\n");
    }

    /* Guess the command is valid */
    (*commandList[i].cProc)(thing, cmd);
    return TRUE;

  } else { /* return that we did zip... */
    /* Socials - yes/no/maybe they did one , ditto for help */
    if (sock && !SocialParse(thing, cmd) && !HelpParse(thing, cmd, NULL)) {
      SEND("I'm sure you meant something by that, the question is what...\n", sock);
    }
    return FALSE;
  }
}


void ParseSock(SOCK *sock, BYTE *cmd) {
  LWORD  i;
  LWORD  j;
  LWORD  count;
  BYTE   buf[256];
  BYTE   newCmd[256];
  BYTE  *next;
  LWORD  headingSent = FALSE;

  struct cmdListType {
    BYTE *cType;
    FLAG  cFlag;
  };

  struct cmdListType cmdList[] = {
    { "*Movement*"         , CF_MOVE     },
    { "*Combat*"           , CF_CBT      },
    { "*Inventory*"        , CF_INV      },
    { "*Communication*"    , CF_TALK     },
    { "*Miscellaneous*"    , CF_MISC     },
    { "*God-Commands*"     , CF_GOD      },
    { "*Help-Editing*"     , CF_HELPEDIT },
    { "*Area-Editing*"     , CF_AREA     },
    { "*World-Editing*"    , CF_WLD      },
    { "*Mobile-Editing*"   , CF_MOB      },
    { "*Object-Editing*"   , CF_OBJ      },
    { "*Reset-Editing*"    , CF_RST      },
    { "*Board-Editing*"    , CF_BRD      },
    { "*C4-Coding*"        , CF_C4CODING },
    { ""                   , 0           }
  };

  /* Any action at all will prevent their Char from Idling */
  Plr(sock->sHomeThing)->pIdleTick = 0;
  if (Plr(sock->sHomeThing)->pIdleRoom) {
    ThingTo(sock->sHomeThing, Plr(sock->sHomeThing)->pIdleRoom);
    Plr(sock->sHomeThing)->pIdleRoom = NULL;
    SendAction("$n rejoins you from the void\n",
      sock->sHomeThing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
  }

  /* history stuff - whew! (capture input commands in text buffer) */
  if (HistoryParse(&sock->sHistory, sock, cmd)) return;

  if (*cmd && !strcmp(cmd, HistoryGetLast(&sock->sHistory))) {
    sock->sLastRepeat++;
  } else {
    sock->sLastRepeat = 0;
  }

  if (strncmp(MOLE_HDR, cmd, strlen(MOLE_HDR))) 
    HistoryAdd(&sock->sHistory, cmd);

  /* get one word from the command */
  next = StrOneWord(cmd, buf);

  /******************************************************
   * command listing - intercept default help
   ******************************************************/
    
  if (StrAbbrev("?",buf) || StrAbbrev("help",buf)) {

    /******************************************************
     * check to see if they want a listing of command categories 
     ******************************************************/
    
    if (!*next) { /* give 'em the list of help topics */
      SEND("^yYou have access to commands belonging to the following categories:\n", sock);

      /* put "? <virtual#>" into newCmd so we can see if areaEditing is authorized */
      if (Base(sock->sHomeThing)->bInside->tType == TTYPE_WLD)
        sprintf(newCmd, "? %ld", Wld(Base(sock->sHomeThing)->bInside)->wVirtual);
      else
        sprintf(newCmd, "? -1");

      /* init counter */
      i=0;
      while (cmdList[i].cType[0]) {
        j=0;
        while (commandList[j].cText[0]) {
          if (BITANY(commandList[j].cFlag, cmdList[i].cFlag)
              && ParseCommandCheck(j,sock,newCmd)) {
            SEND("^G<^g", sock);
            SEND(cmdList[i].cType, sock);
            SEND("^G>\n", sock);
            break;
          }
          j++;
        }
        i++;
      }
      SEND("\n^yTo list commands in a given category, Try ^whelp <category>\n", sock);
      SEND("^ye.g. ^whelp *move\n", sock);
      return;
    }
    
    /******************************************************
     * check to see if they want help on a specific topic 
     ******************************************************/
    
    i = TYPEFIND(next, cmdList);
    if (i>=0) {
      SEND("^pYou can have access to the following ^w", sock);
      SEND(cmdList[i].cType, sock);
      SEND("^p commands\n", sock);

      /* put "? <virtual#>" into newCmd so we can see if areaEditing is authorized */
      if (Base(sock->sHomeThing)->bInside->tType == TTYPE_WLD)
        sprintf(newCmd, "? %ld", Wld(Base(sock->sHomeThing)->bInside)->wVirtual);
      else
        sprintf(newCmd, "? -1");

      /* init counter */
      headingSent = FALSE;
      count=0; j=0;
      while (commandList[j].cText[0]) {
        if (BITANY(commandList[j].cFlag, cmdList[i].cFlag)
            && ParseCommandCheck(j,sock,newCmd)) {
          if (!headingSent) {
            if (i>0)
              SEND("\n", sock);
            SEND("^G<^g", sock);
            SEND(cmdList[i].cType, sock);
            SEND("^G>\n", sock);
            headingSent = TRUE;
          }
          if (count%3 == 2)
            sprintf(buf, "^c%-25s\n", commandList[j].cText);
          else
            sprintf(buf, "^c%-25s", commandList[j].cText);
          SEND(buf, sock);
          count++;
        }
        j++;
      }
      if (headingSent && count%3)
        SEND("\n", sock);
      return;
    }
  }

  /* cant hurt */
  if (!sock->sControlThing) return;

  ParseCommandStub(sock->sControlThing, cmd);
}

LWORD ParseCommandStub(THING *event, BYTE *cmd) { 
  LWORD  processed = FALSE;
  THING *code;
  THING *inside;
  THING *newInside;

  inside = newInside = Base(event)->bInside;
  ThingSetEvent(event);

  /* Let attached code properties have first crack at it */
  if ((CodeParseCommand(event, inside, cmd))) {
    processed = TRUE;
  } else {
    for (code = inside->tContain; code; code=code->tNext) 
      BITSET(code->tFlag, TF_CODETHING);
    while( (code = ThingFindFlag(inside->tContain, TF_CODETHING)) ) {
      BITCLR(code->tFlag, TF_CODETHING);
      if (event && !ThingIsEvent(event)) event = NULL;
      if (event) { /* if they've been turfed, nothing to do */
        if ((CodeParseCommand(event, code, cmd))) {
          processed = TRUE;
          break;
        }
      }
    }
  }
  /* Check Area Event */
  if (!processed && inside->tType == TTYPE_WLD) {
    if (event && !ThingIsEvent(event)) event = NULL;
    if (event) { /* if they've been turfed, nothing to do */
      if (CodeParseCommand(event, &areaList[Wld(inside)->wArea].aResetThing, cmd))
        processed = TRUE;
    }
  }

  /* Commands */
  if (!processed) {
    if (event && !ThingIsEvent(event)) event = NULL;
    if (event) processed = ParseCommand(event, cmd);
  }
    
  /* Let after code properties execute */
  if (event && !ThingIsEvent(event)) event = NULL;
  if (event) CodeParseAfterCommand(event, inside, cmd);

  if (inside) {
    for (code = inside->tContain; code; code=code->tNext) 
      BITSET(code->tFlag, TF_CODETHING);
    while( (code = ThingFindFlag(inside->tContain, TF_CODETHING)) ) {
      BITCLR(code->tFlag, TF_CODETHING);
      if (event && !ThingIsEvent(event)) event = NULL;
      if (event) CodeParseAfterCommand(event, code, cmd);
    }
  }
  /* Check area-wide script */
  if (event && !ThingIsEvent(event)) event = NULL;
  if (event && inside->tType == TTYPE_WLD)
    CodeParseAfterCommand(event, &areaList[Wld(inside)->wArea].aResetThing, cmd);

  /* Clean up */
  ThingDeleteEvent(event);

  return processed;
}


