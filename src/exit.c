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

/* exit.c */ #include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "index.h"
#include "extra.h"
#include "thing.h"
#include "exit.h"
#include "world.h"

#define EXIT_BLOCK_SIZE 4096

LWORD exitNum = 0;

BYTE *dirList[] = {
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "out",
  "undefined",
  ""
};

/* the order in which exits will be listed */
LWORD eOrderList[] = {
  EDIR_WEST,
  EDIR_NORTH,
  EDIR_SOUTH,
  EDIR_EAST,
  EDIR_UP,
  EDIR_DOWN,
  EDIR_OUT,
  EDIR_UNDEFINED,
  EDIR_MAX
};

BYTE *eFlagList[] = {
  "ISDOOR",
  "PICKPROOF",
  "LOCKED",
  "CLOSED",
  "HIDDEN",
  "ELECTRONIC",
  "NOPHASE",
  ""
};

BYTE reverseDirList[] = {
  EDIR_SOUTH,
  EDIR_WEST,
  EDIR_NORTH,
  EDIR_EAST,
  EDIR_DOWN,
  EDIR_UP,
  EDIR_UNDEFINED,
  EDIR_UNDEFINED
};

EXIT *ExitAlloc(EXIT *eNext, BYTE eDir, STR *eKey, STR *eDesc, FLAG eFlag, WORD eKeyObj, THING *eWorld) {
  EXIT *exit;

  MEMALLOC(exit, EXIT, EXIT_BLOCK_SIZE);
  exit->eNext = eNext;
  exit->eDir  = eDir;
  exit->eKey  = eKey;
  exit->eDesc = eDesc;
  exit->eFlag = eFlag;
  exit->eKeyObj=eKeyObj;
  exit->eWorld= eWorld;

  exitNum++;
  return exit;
}

EXIT *ExitCreate(EXIT *eNext, BYTE eDir, BYTE *eKey, BYTE *eDesc, FLAG eFlag, WORD eKeyObj, THING *eWorld) {
  STR *sKey;
  STR *sDesc;

  /* Check to see if an exit in this direction already exists */
  if (ExitDir(eNext,eDir))
    return NULL;

  sKey = STRCREATE(eKey);
  sDesc = STRCREATE(eDesc);
  return ExitAlloc(eNext, eDir, sKey, sDesc, eFlag, eKeyObj, eWorld);
}


EXIT *ExitFind(EXIT *eList, BYTE *eKey) {
  EXIT *exit;
  LWORD i;

  if (!eKey) return NULL;
  exit = NULL;
  i = TYPEFIND(eKey, dirList);
  if (i != -1) {
    for (exit = eList; exit && (exit->eDir != i); exit = exit->eNext);
  }
  /* if we havent found a matching dir then search on keyword */
  if (exit==NULL) {
    for (exit = eList; exit && !StrIsKey(eKey, exit->eKey); exit = exit->eNext);
  }
  return exit; /* exit will be either the found exit or Null */
}

EXIT *ExitDir(EXIT *eList, BYTE eDir) {
  EXIT *exit;

  if (eDir == EDIR_UNDEFINED)
    return NULL;
  for (exit = eList; exit && (exit->eDir != eDir); exit = exit->eNext);
  return exit; /* exit will be either the found exit or Null */
}

/* Find the exit that points back to us */
EXIT *ExitReverse(THING *world, EXIT *exit) {
  EXIT *eList;
  EXIT *reverse = NULL;

  if (!exit) return NULL;
  if (!(exit->eWorld)) return NULL;

  /* look for the obvious choice - a return path back along the opposite direction */
  if (exit->eDir!=EDIR_UNDEFINED) { 
    eList = Wld(exit->eWorld)->wExit;
    reverse = ExitDir(eList, reverseDirList[exit->eDir]);
    if (reverse && reverse->eWorld == world)
      return reverse;
  }

  /* look for a return exit with a corner in it */
  for (reverse = eList; reverse && (reverse->eWorld != world); reverse = reverse->eNext);
  return reverse; /* exit will be either the found exit or Null */
}

/* this esoteric piece of code determines if there is a bend in a pair of exits
   ie if you leave north from one room and come in from the west into the destination
   we could reasonably assume that there is a corner in the way

   This is important since weapons cannot fire around corners!
*/
LWORD ExitIsCorner(EXIT *exit, EXIT *reverse) {
  if (exit && reverse) {
    if (exit->eDir == EDIR_UNDEFINED || reverse->eDir == EDIR_UNDEFINED)
      return TRUE;
    if (exit->eDir == reverseDirList[reverse->eDir])
      return TRUE;
  }
  return FALSE;
}

EXIT *ExitFree(EXIT *eList, EXIT *exit) {
  EXIT *i;
  EXIT *eFree;

  if (!exit) return eList;
  exitNum -= 1;

  while (IndexFind(&eventThingIndex, exit, NULL))
    IndexDelete(&eventThingIndex, exit, NULL);

  if (eList == exit) {
    eFree = eList;
    eList = eList->eNext;
    STRFREE(eFree->eKey);
    STRFREE(eFree->eDesc);
    MEMFREE(eFree, EXIT);
    return eList;
  }

  for (i = eList; i && i->eNext != exit; i = i->eNext);
  if (i) { /* found it */
    eFree = i->eNext;
    i->eNext = eFree->eNext;
    STRFREE(eFree->eKey);
    STRFREE(eFree->eDesc);
    MEMFREE(eFree, EXIT);
  } else {
    eFree = exit;
    STRFREE(eFree->eKey);
    STRFREE(eFree->eDesc);
    MEMFREE(eFree, EXIT);
  }
  return eList;
}

BYTE *ExitGetName(EXIT *exit, BYTE *buf) {
  if (exit->eDir != EDIR_UNDEFINED) {
    strcpy(buf, dirList[exit->eDir]);
  } else {
    StrOneWord(exit->eKey->sText, buf);
  }
  return buf;
}
