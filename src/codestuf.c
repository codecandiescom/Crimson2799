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
#include "queue.h"
#include "send.h"
#include "extra.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "mobile.h"
#include "player.h"
#include "area.h"
#include "codestuf.h"
#include "compile.h"
#include "interp.h"
#include "function.h"
#include "decomp.h"
#include "exit.h"
#include "skill.h"
#include "fight.h"

/* Globals */
ULWORD cSystemVariable;       /* # of system variables which take up first bit of local variable tables */
ULWORD cSystemVariableStatic; /* # of sys vars which are STATIC. IE: unchangeable */

BYTE *cDataType[] = {
  "NULL",
  "INT",
  "STR",
  "THING",
  "EXTRA",
  "EXIT",
  "UNDEF6", /* Why Cam used 31 as CDT_ETC I cant tell ya */
  "UNDEF7",
  "UNDEF8",
  "UNDEF9",
  "UNDEF10",
  "UNDEF11",
  "UNDEF12",
  "UNDEF13",
  "UNDEF14",
  "UNDEF15",
  "UNDEF16",
  "UNDEF17",
  "UNDEF18",
  "UNDEF19",
  "UNDEF20",
  "UNDEF21",
  "UNDEF22",
  "UNDEF23",
  "UNDEF24",
  "UNDEF25",
  "UNDEF26",
  "UNDEF27",
  "UNDEF28",
  "UNDEF29",
  "UNDEF30",
  "ETC",
  ""
};

CODERESERVEDWORD cResWord[] = { /* table of reserved words */
  { "int" },
  { "str" },
  { "thing" },
  { "local" },
  { "global" },
  { "private" },
  { "for" },
  { "while" },
  { "else" },
  { "stop" },
  { "if" },
  { "switch" },
  { "break" },
  { "case" },
  { "goto" },
  { "char" },
  { "const" },
  { "default" },
  { "double" },
  { "extern" },
  { "float" },
  { "asm" },
  { "long" },
  { "public" },
  { "return" },
  { "short" },
  { "signed" },
  { "sizeof" },
  { "struct" },
  { "typedef" },
  { "union" },
  { "unsigned" },
  { "void" },
  { "extra" },
  { "continue" },
  { "exit" },
  { "prop" },
  { "property" }, 
  { "" }
};

/* Master Global Variable Table */
/* To add a global variable, do the following:
 *   step 1: Add an entry into the following table.
 *           CAREFUL: the first group are STATIC (ie C4 cannot
 *           change their value) and the second group are DYNAMIC
 *           (C4 can change their value!). The groups are each 
 *           terminated by a NULL entry.
 *           The first parameter is the variable name. 
 *           The second parameter is the variable type.
 *               third is the initial integer value, if type CDT_INT.
 *               fourth is the initial pointer value, if type CDT_STR,
 *                   CDT_THING, or CDT_EXTRA.
 *   step 2: If you've added a lot of variables, you may have to 
 *           increment the constant CMAX_VAR in compile.h. This 
 *           constant defines the max # of local variables available,
 *           including those found below! 
 *   step 3: If the initial value is sufficient, you're done! If the 
 *           value of the variable changes at boot-up (most pointer
 *           types will have to be set like this), or at various times
 *           (eg: TIME), then you will have to go into interp.c and
 *           modify InterpInit and Interp to set your new variable 
 *           accordingly. There is existing code there that you can
 *           look at and copy for your own variable.
 *
 * HUGE WARNING!!! GIGANTIC ENORMOUS MEGA WARNING!!!!!!!!!!!!!!!!!!
 * READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS!!!
 *   When the system saves C4 code attached to a THING, it can either 
 *   disassemble the code back into source code, or it can save it as
 *   binary data to save boot time. Here's the glitch: the variables
 *   are stored in the binary format as an integer value offset into 
 *   the array cSysVar below. Do you see the danger?
 *   
 *   Suppose you ran your MUD and saved all your C4 code as binary. 
 *   Suppose also that you had a C4 program which used the system
 *   variable TNULL. When your MUD compiled your C4, the variable TNULL 
 *   would be stored in memory as 0x0001 ("NULL" would be  0x0000). 
 *   So you saved your C4 code as binary
 *   to disk and TNULL got written to disk as "00 01". Now, suppose
 *   you went in and added a NEW system variable inbetween NULL and
 *   TNULL called "NEWVAR". You see, the position of NEWVAR is now
 *   0x0001 according to your table, but your C4 code 
 *   saved in binary format is expecting 0x0001 to be TNULL!!!
 *   So when you load your code, your binary code expects TNULL to be 0x00001
 *   but your compiled MUD expects TNULL to be 0x0002!!!! OH MY! In fact, ALL
 *   your variables are now shifted by 1!!! 
 * 
 *  MORAL OF THE STORY: If you EVER, and I mean EVER, think of changing
 *  the following cSysVar array, be it adding a variable, rearranging,
 *  deleting, or whatever, MAKE SURE YOUR MUD FILES ARE SAVED AS SOURCE
 *  CODE!!!!!!! Let me say this another way: MAKE SURE YOUR MUD C4 CODE IS
 *  SAVED AS SOURCE CODE, NOT BINARY!!! 
 *
 *  In case you missed it, MAKE SURE YOUR MUD SAVES SCRIPTS AS SOURCE
 *  CODE, NOT BINARY FORMAT!!! 
 *
 *  I hope I have made myself clear on this point.
 *  
 * Well, good luck and happy coding!
 */
CODESYSVAR cSysVar[] = {
 /* static (non-C4-changeable) system variables */
  { "NULL",          CDT_STR,   0                ,NULL },
  { "TNULL",         CDT_THING, 0                ,NULL },
  { "ENULL",         CDT_EXTRA, 0                ,NULL },
  { "XNULL",         CDT_EXIT,  0                ,NULL },
  { "TRUE",          CDT_INT,   1                ,NULL },
  { "FALSE",         CDT_INT,   0                ,NULL },
  { "YES",           CDT_INT,   1                ,NULL },
  { "NO",            CDT_INT,   0                ,NULL },
  { "EVENT_THING",   CDT_THING, 0                ,NULL },
  { "CODE_THING",    CDT_THING, 0                ,NULL },
  { "SPARE_THING",   CDT_THING, 0                ,NULL },
  { "EXIT",          CDT_EXIT,  0                ,NULL },
  { "TIME",          CDT_INT,   0                ,NULL },
  { "SEGMENT",       CDT_INT,   0                ,NULL },
  { "COMMAND",       CDT_STR,   0                ,NULL },
  { "CMD_CMD",       CDT_STR,   0                ,NULL },
  { "CMD_SRCKEY",    CDT_STR,   0                ,NULL },
  { "CMD_SRCOFFSET", CDT_INT,   0                ,NULL },
  { "CMD_SRCNUM",    CDT_INT,   0                ,NULL },
  { "CMD_DSTKEY",    CDT_STR,   0                ,NULL },
  { "CMD_DSTOFFSET", CDT_INT,   0                ,NULL },
  { "SEND_ROOM",     CDT_INT,   SEND_ROOM        ,NULL },
  { "SEND_SRC",      CDT_INT,   SEND_SRC         ,NULL },
  { "SEND_DST",      CDT_INT,   SEND_DST         ,NULL },
  { "SEND_VISIBLE",  CDT_INT,   SEND_VISIBLE     ,NULL },
  { "SEND_AUDIBLE",  CDT_INT,   SEND_AUDIBLE     ,NULL },
  { "SEND_CAPFIRST", CDT_INT,   SEND_CAPFIRST    ,NULL },
  { "TTYPE_UNDEF",   CDT_INT,   TTYPE_UNDEF      ,NULL },
  { "TTYPE_WLD",     CDT_INT,   TTYPE_WLD        ,NULL },
  { "TTYPE_BASE",    CDT_INT,   TTYPE_BASE       ,NULL },
  { "TTYPE_OBJ",     CDT_INT,   TTYPE_OBJ        ,NULL },
  { "TTYPE_CHARACTER",CDT_INT,  TTYPE_CHARACTER  ,NULL },
  { "TTYPE_MOB",     CDT_INT,   TTYPE_MOB        ,NULL },
  { "TTYPE_PLR",     CDT_INT,   TTYPE_PLR        ,NULL },
  { "TF_CONTINUE",   CDT_INT,   TF_CONTINUE      ,NULL },
  { "TF_PLR",        CDT_INT,   TF_PLR           ,NULL },
  { "TF_OBJ",        CDT_INT,   TF_OBJ           ,NULL },
  { "TF_MOB",        CDT_INT,   TF_MOB           ,NULL },
  { "TF_PLR_ANYWHERE",CDT_INT,  TF_PLR_ANYWHERE  ,NULL },
  { "TF_OBJ_ANYWHERE",CDT_INT,  TF_OBJ_ANYWHERE  ,NULL },
  { "TF_MOB_ANYWHERE",CDT_INT,  TF_MOB_ANYWHERE  ,NULL },
  { "TF_PLR_WLD",    CDT_INT,   TF_PLR_WLD       ,NULL },
  { "TF_OBJ_WLD",    CDT_INT,   TF_OBJ_WLD       ,NULL },
  { "TF_MOB_WLD",    CDT_INT,   TF_MOB_WLD       ,NULL },
  { "TF_WLD",        CDT_INT,   TF_WLD           ,NULL },
  { "TF_OBJINV",     CDT_INT,   TF_OBJINV        ,NULL },
  { "TF_OBJEQUIP",   CDT_INT,   TF_OBJEQUIP      ,NULL },
  { "TF_ALLMATCH",   CDT_INT,   TF_ALLMATCH      ,NULL },
  { "EDIR_NORTH",    CDT_INT,   EDIR_NORTH       ,NULL },
  { "EDIR_EAST",     CDT_INT,   EDIR_EAST        ,NULL },
  { "EDIR_SOUTH",    CDT_INT,   EDIR_SOUTH       ,NULL },
  { "EDIR_WEST",     CDT_INT,   EDIR_WEST        ,NULL },
  { "EDIR_UP",       CDT_INT,   EDIR_UP          ,NULL },
  { "EDIR_DOWN",     CDT_INT,   EDIR_DOWN        ,NULL },
  { "EDIR_UNDEFINED",CDT_INT,   EDIR_UNDEFINED   ,NULL },
  { "EF_ISDOOR",     CDT_INT,   EF_ISDOOR        ,NULL },
  { "EF_PICKPROOF",  CDT_INT,   EF_PICKPROOF     ,NULL },
  { "EF_LOCKED",     CDT_INT,   EF_LOCKED        ,NULL },
  { "EF_CLOSED",     CDT_INT,   EF_CLOSED        ,NULL },
  { "EF_HIDDEN",     CDT_INT,   EF_HIDDEN        ,NULL },
  { "EF_ELECTRONIC", CDT_INT,   EF_ELECTRONIC    ,NULL },
  { "SF_WEAPON",     CDT_INT,   SF_WEAPON        ,NULL },
  { "SF_MELEE",      CDT_INT,   SF_MELEE         ,NULL },
  { "SF_LASER",      CDT_INT,   SF_LASER         ,NULL },
  { "SF_SLUG",       CDT_INT,   SF_SLUG          ,NULL },
  { "SF_BLASTER",    CDT_INT,   SF_BLASTER       ,NULL },
  { "SF_ION",        CDT_INT,   SF_ION           ,NULL },
  { "SF_SUPPORT",    CDT_INT,   SF_SUPPORT       ,NULL },
  { "SF_PLASMA",     CDT_INT,   SF_PLASMA        ,NULL },
  { "SF_OFFENSE",    CDT_INT,   SF_OFFENSE       ,NULL },
  { "SF_DEFENSE",    CDT_INT,   SF_DEFENSE       ,NULL },
  { "SF_PSYCHIC",    CDT_INT,   SF_PSYCHIC       ,NULL },
  { "SF_BODY",       CDT_INT,   SF_BODY          ,NULL },
  { "SF_TELEKINETIC",CDT_INT,   SF_TELEKINETIC   ,NULL },
  { "SF_APPORTATION",CDT_INT,   SF_APPORTATION   ,NULL },
  { "SF_SPIRIT",     CDT_INT,   SF_SPIRIT        ,NULL },
  { "SF_PYROKINETIC",CDT_INT,   SF_PYROKINETIC   ,NULL },
  { "SF_TELEPATHY",  CDT_INT,   SF_TELEPATHY     ,NULL },
  { "SF_GENERAL",    CDT_INT,   SF_GENERAL       ,NULL },
  { "SF_CLASS",      CDT_INT,   SF_CLASS         ,NULL },
  { "OR_BIO",        CDT_INT,   OR_BIO           ,NULL },
  { "OR_CHIP",       CDT_INT,   OR_CHIP          ,NULL },
  { "OR_RICH",       CDT_INT,   OR_RICH          ,NULL },
  { "FD_PUNCTURE",   CDT_INT,   FD_PUNCTURE      ,NULL },
  { "FD_SLASH",      CDT_INT,   FD_SLASH         ,NULL },
  { "FD_CONCUSSIVE", CDT_INT,   FD_CONCUSSIVE    ,NULL },
  { "FD_HEAT",       CDT_INT,   FD_HEAT          ,NULL },
  { "FD_EMR",        CDT_INT,   FD_EMR           ,NULL },
  { "FD_LASER",      CDT_INT,   FD_LASER         ,NULL },
  { "FD_PSYCHIC",    CDT_INT,   FD_PSYCHIC       ,NULL },
  { "FD_ACID",       CDT_INT,   FD_ACID          ,NULL },
  { "FD_POISON",     CDT_INT,   FD_POISON        ,NULL },
  { "FR_PUNCTURE",   CDT_INT,   FR_PUNCTURE      ,NULL },
  { "FR_SLASH",      CDT_INT,   FR_SLASH         ,NULL },
  { "FR_CONCUSSIVE", CDT_INT,   FR_CONCUSSIVE    ,NULL },
  { "FR_HEAT",       CDT_INT,   FR_HEAT          ,NULL },
  { "FR_EMR",        CDT_INT,   FR_EMR           ,NULL },
  { "FR_LASER",      CDT_INT,   FR_LASER         ,NULL },
  { "FR_PSYCHIC",    CDT_INT,   FR_PSYCHIC       ,NULL },
  { "FR_ACID",       CDT_INT,   FR_ACID          ,NULL },
  { "FR_POISON",     CDT_INT,   FR_POISON        ,NULL },
  { "WT_INDOORS",    CDT_INT,   WT_INDOORS       ,NULL },
  { "WT_CITY",       CDT_INT,   WT_CITY          ,NULL },
  { "WT_FIELD",      CDT_INT,   WT_FIELD         ,NULL },
  { "WT_FOREST",     CDT_INT,   WT_FOREST        ,NULL },
  { "WT_HILLS",      CDT_INT,   WT_HILLS         ,NULL },
  { "WT_MOUNTAIN",   CDT_INT,   WT_MOUNTAIN      ,NULL },
  { "WT_WATERSWIM",  CDT_INT,   WT_WATERSWIM     ,NULL },
  { "WT_WATERNOSWIM",CDT_INT,   WT_WATERNOSWIM   ,NULL },
  { "WT_UNDERWATER", CDT_INT,   WT_UNDERWATER    ,NULL },
  { "WT_VACUUM",     CDT_INT,   WT_VACUUM        ,NULL },
  { "WT_DESERT",     CDT_INT,   WT_DESERT        ,NULL },
  { "WT_ARCTIC",     CDT_INT,   WT_ARCTIC        ,NULL },
  { "WT_ROAD",       CDT_INT,   WT_ROAD          ,NULL },
  { "WT_TRAIL",      CDT_INT,   WT_TRAIL         ,NULL },
  { "WF_DARK",       CDT_INT,   WF_DARK          ,NULL },
  { "WF_DEATHTRAP",  CDT_INT,   WF_DEATHTRAP     ,NULL },
  { "WF_NOMOB",      CDT_INT,   WF_NOMOB         ,NULL },
  { "WF_VACUUM",     CDT_INT,   WF_VACUUM        ,NULL },
  { "WF_NOGOOD",     CDT_INT,   WF_NOGOOD        ,NULL },
  { "WF_NONEUTRAL",  CDT_INT,   WF_NONEUTRAL     ,NULL },
  { "WF_NOEVIL",     CDT_INT,   WF_NOEVIL        ,NULL },
  { "WF_NOPSIONIC",  CDT_INT,   WF_NOPSIONIC     ,NULL },
  { "WF_SMALL",      CDT_INT,   WF_SMALL         ,NULL },
  { "WF_PRIVATE",    CDT_INT,   WF_PRIVATE       ,NULL },
  { "WF_DRAINPOWER", CDT_INT,   WF_DRAINPOWER    ,NULL },
  { "WF_NOTELEPORTOUT",CDT_INT, WF_NOTELEPORTOUT ,NULL },
  { "WF_NOTELEPORTIN",CDT_INT,  WF_NOTELEPORTIN  ,NULL },
  { "RACE_HUMAN"     ,CDT_INT,  0                ,NULL },
  { "RACE_ARTIFICER" ,CDT_INT,  1                ,NULL },
  { "RACE_SILICONOID",CDT_INT,  2                ,NULL },
  { "RACE_SALAMANDER",CDT_INT,  3                ,NULL },
  { NULL,            CDT_NULL,  0                ,NULL },
 /* end of static (non-C4-changeable) system variables */

 /* dynamic (C4-changeable) system variables */
  { "BLOCK_CMD",     CDT_INT,   0                ,NULL },
  { NULL,            CDT_NULL,  0                ,NULL }
 /* end of dynamic (C4-changeable) system variables */
};


void CodeStufInit() {
  ULWORD i;

  /* Initialize global settings */
  for (i=0;cSysVar[i].cText!=NULL;i++); /* find first NULL entry */
  cSystemVariableStatic=i;
  for (i++;cSysVar[i].cText!=NULL;i++); /* find second NULL entry */
  cSystemVariable=i-1; /* -1 is to ignore first NULL entry */
}

