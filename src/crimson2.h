#ifdef WIN32
  /* turn off warning: integral size mismatch */
  /* turn off warning: conversion from long to short */
  /* turn off warning: signed/unsigned mismatch */
  /* turn off warning: int to short signed/unsigned mismatch */
  #pragma warning( disable : 4761 4244 4018)

  /*
   * Note to compile under win32 you must link the following libs:
   * wsock32.lib, winmm.lib
   */
#endif

/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary, if you are using it without my express
 * permission you are violating the copyright act and can potentially be
 * sued.
 * That said, if you would like to use it, I'm not an ogre, gimme a call
 * and maybe we can work something out.
 *
 * Current email address(es): cryogen@unix.infoserve.net
 *                            rhaksi@freenet.vancouver.bc.ca
 * Phone number: (604) 591-5295
 */

/* Further note: you can probably talk me into letting you use a version of my
 * server, and even giving you the odd bug fix and enhancement however:
 * all areas developed for (and by) this mud server are owned by:
 * Ryan Haksi.
 * what does this mean for you? if you develop an area for my server I want a copy
 * which I may or may not use as I see fit (you of course can continue using it
 * *grin* but cant make money off it, or indeed make money of anything else
 * related to *MY* mud server - unless of course you get my permission... )
 */


/* naming conventions - our motto, nothing is plural... */

/* structs - ExampleStructName
 *      all structs should be typedef'd as an allcaps name (see type below)
 *      none of the fields of a struct (or anything else) should be plural
 *      fields of a struct should start with the first letter of the type
 *      ie in the example above fields would be eType, eName etc
 * functions - ExampleFunctionName
 * constants - EXAMPLE_CONSTANT_NAME
 * type - EXAMPLETYPE
 *
 * Note: nothing should use chars, ints etc as they are system dependant
 *  use the types defined in this module as they are NON-system dependant
 *
 * standard type functions for each module include:
 * Alloc function (typically declare space for a structure)
 * Create function (typically calls alloc and then inits structure)
 * Free function (do anything and everything to remove object from game cleanly)
 */


/* dir structure */
/*
 * Crimson2 (executable)
 * src\*.c
 * src\*.h
 * player\<name>.plr - player data files
 * doc\<various>.doc
 * log\port.log - port connects, disconnects, errors
 * log\error.log - stuff that shouldnt happen
 * log\boot.log - bootup warnings, erased prior to every startup
 * log\usuage.log - level gains, deaths, failed rent
 * log\command.log - things like purge, delete, ban etc
 * log\<name>.log - if a player is so logged watch everthing he does
 * msg\ciao.msg - after we give 'em the boot
 * msg\motd.msg - after a successful login
 * msg\greeting.msg -sent right after connect before login
 * area\area.tbl
 * area\wld\<area>.wld
 * area\mob\<area>.mob
 * area\rst\<area>.rst
 * area\obj\<area>.obj
 * help\help.tbl - list of help sections and editing authorization
 * help\<section>.hlp - help section
 */

/* Version Info - v0.8 */
#define CRIMSON_MAJOR_VERSION 0
#define CRIMSON_MINOR_VERSION 8

/* Return codes from the mud server itself, used for exit statements */
#define ERROR_NOERROR 0 /* normal shutdown */
#define ERROR_SOCKET  1 /* socket error (couldnt init) */
#define ERROR_LOG     2
#define ERROR_NOMEM   3 /* no memory left */
#define ERROR_CORRUPT 4 /* corrupt memory ie seg fault or something */
#define ERROR_BADFILE 5 /* corrupt datafiles - dont autoreboot*/

/* global type general constants */
#define TRUE  1
#define FALSE 0

/* global type def'ns */
typedef unsigned char             BYTE;    /* 8bit unsigned */
typedef signed char               SBYTE;   /* 8bit signed */
typedef signed short int          WORD;    /* 16bit signed */
typedef unsigned short int        UWORD;   /* 16bit unsigned */
typedef signed long int           LWORD;   /* 32bit signed */
typedef unsigned long int         ULWORD;  /* 32bit unsigned */
typedef unsigned long int         FLAG;    /* 32bit unsigned */

/* crimson specific types (to allow forward references) */
typedef struct ColorListType      COLORLIST;
typedef struct ColorPrefType      COLORPREF;
typedef struct QType              Q;
typedef struct ColorPrefListType  COLORPREFLIST;
typedef struct HistoryType        HISTORY;
typedef struct SockType           SOCK;
typedef struct EditType           EDIT;
typedef struct StrType            STR;
typedef struct ExtraType          EXTRA;
typedef struct PropertyType       PROPERTY;
typedef struct ThingType          THING;
typedef struct ExitType           EXIT;
typedef struct WldType            WLD;
typedef struct BaseLinkType       BASELINK;
typedef struct BaseType           BASE;
typedef struct AffectType         AFFECT;
typedef struct ApplyType          APPLY;
typedef struct OTypeListType      OTYPELIST;
typedef struct ObjTemplateType    OBJTEMPLATE;
typedef struct ObjType            OBJ;
typedef struct ODetailType        ODETAIL;
typedef struct CharacterType      CHARACTER;
typedef struct PlrType            PLR;
typedef struct SkillDetailType    SKILLDETAIL;
typedef struct AliasType          ALIAS;
typedef struct MobTemplateType    MOBTEMPLATE;
typedef struct MobType            MOB;
typedef struct ResetListType      RESETLIST;
typedef struct AreaListType       AREALIST;
typedef struct IndexType          INDEX;
typedef struct CommandListType    COMMANDLIST;
typedef struct CmdMOLEListType    CMDMOLELIST;

/* function def'n macros */
#define INDEXPROC(proc) WORD (proc)(INDEX *i, void *index1, void *index2)
#define INDEXFINDPROC(proc) WORD (proc)(INDEX *i, void *key, void *index)
#define CMDPROC(proc) void (proc)(THING *thing, BYTE *cmd)
#define EFFECTPROC(proc) LWORD (proc)(BYTE event, THING *thing, BYTE *cmd, LWORD data, INDEX *tarIndex, WORD effectNum, AFFECT *affect, FILE *file)
#define CMDMOLEPROC(proc) void (proc)(SOCK *sock, ULWORD pktID, LWORD virtual)


/* various ultra low-level procedures */
extern LWORD Dice(LWORD dNum, LWORD dSize);
extern LWORD DiceOpenEnded(LWORD dNum, LWORD dSize, LWORD oSize);
extern LWORD Number(LWORD minV, LWORD maxV);
extern BYTE  IsNumber(BYTE *str);
extern void  TimeSprintf(BYTE *str, ULWORD time);

extern LWORD TypeFind(BYTE *tKey, ULWORD tList, LWORD tListSize);
extern LWORD TypeFindExact(BYTE *tKey, ULWORD tList, LWORD tListSize);
extern LWORD TypeCheck(LWORD tNum, ULWORD tList, LWORD tListSize);
extern BYTE *TypeSprintf(BYTE *str, LWORD tNum, ULWORD tList, LWORD tListSize, LWORD sMax);
extern LWORD TypeSscanf(BYTE *str, ULWORD tList, LWORD tListSize);
#define      TYPEFIND(tKey,tList) TypeFind(tKey, (ULWORD)tList, sizeof(*tList))
#define      TYPESSCANF(str,tList) TypeSscanf(str, (ULWORD)tList, sizeof(*tList))
#define      TYPECHECK(tNum,tList) tNum = TypeCheck(tNum, (ULWORD)tList, sizeof(*tList))
#define      TYPESPRINTF(str, tNum, tList,sMax) TypeSprintf(str, tNum, (ULWORD)tList, sizeof(*tList), sMax)

extern FLAG  FlagFind(BYTE *fText, BYTE *fList[]);
extern FLAG  FlagFindExact(BYTE *fText, BYTE *fList[]);
extern FLAG  FlagSscanf(BYTE *fText, BYTE *fList[]);
extern BYTE *FlagSprintf(BYTE *str, FLAG flag, BYTE *fList[], BYTE separator, LWORD sMax);
extern LWORD Flag2Type(FLAG flag);
extern LWORD FlagSetNum(FLAG flag);

extern LWORD SafeDivide(LWORD divisee, LWORD divisor, LWORD errorResult);
