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

/* str.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "extra.h"
#include "thing.h"
#include "code.h"

#define EXTRA_BLOCK_SIZE 4096

EXTRA *ExtraAlloc(EXTRA *eNext, STR *eKey, STR *eDesc) {
  EXTRA *extra;

  MEMALLOC(extra, EXTRA, EXTRA_BLOCK_SIZE);
  extra->eKey = eKey;
  extra->eNext = eNext;
  extra->eDesc = eDesc;

  return extra;
}

EXTRA *ExtraCreate(EXTRA *eNext, BYTE *eKey, BYTE *eDesc) {
  STR *sKey;
  STR *sDesc;

  sKey = STRCREATE(eKey);
  sDesc = STRCREATE(eDesc);
  return ExtraAlloc(eNext, sKey, sDesc);
}

/* recursive so we don't reverse the order of the extra chain */
/* will compile code prior to duplicating it */
EXTRA *ExtraCopy(EXTRA *eChain) {
  EXTRA *newChain = NULL;

  if (eChain) {
    newChain = ExtraAlloc(NULL, StrAlloc(eChain->eKey), StrAlloc(eChain->eDesc));
    newChain->eNext = ExtraCopy(eChain->eNext);
    return newChain;
  } 
  return NULL;
}

EXTRA *ExtraFind(EXTRA *eList, BYTE *eKey) {
  EXTRA *i;

  if (!eKey) return NULL;
  for (i = eList; i && !StrIsKey(eKey, i->eKey); i = i->eNext);
  return i; /* i will be either the found key or Null */
}

EXTRA *ExtraFindWithin(THING *thing, BYTE *eKey, LWORD *offset) {
  EXTRA *i;

  i = thing->tExtra;
  do {
    i=ExtraFind(i, eKey);
    if (i) *offset-=1;
  } while(i && *offset>0);
  if (i) return i;

  for (thing = thing->tContain; thing; thing=thing->tNext) {
    i = thing->tExtra;
    do {
      i=ExtraFind(i, eKey);
      if (i) *offset-=1;
    } while(i && *offset>0);
    if (i) return i;
  }
  return NULL;
}

EXTRA *ExtraFree(EXTRA *eList, EXTRA *extra) {
  EXTRA *i;
  EXTRA *eFree;

  if (!eList || !extra) return NULL;
  if (eList == extra) {
    eFree = eList;
    eList = eFree->eNext;
    STRFREE(eFree->eDesc);
    STRFREE(eFree->eKey);
    MEMFREE(eFree, EXTRA);
    return eList;
  }

  for (i = eList; i && i->eNext != extra; i = i->eNext);
  if (i) { /* found it */
    eFree = i->eNext;
    i->eNext = eFree->eNext;
    STRFREE(eFree->eKey);
    STRFREE(eFree->eDesc);
    MEMFREE(eFree, EXTRA);
  } else {
    Log(LOG_ERROR, "ExtraFree: EXTRA pointer not found in eList\n");
  }
  return eList;
}

