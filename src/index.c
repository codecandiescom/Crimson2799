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


#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "queue.h"
#include "str.h"
#include "extra.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "mobile.h"
#include "reset.h"
#include "area.h"

BYTE indexError = FALSE;

/* Setup the Index's fields */
void IndexInit(INDEX *index, LWORD iByte, BYTE* errorStr, FLAG flag) {
  index->iThing = NULL;
  index->iNum   = 0;
  index->iByte  = iByte;
  index->iFlag  = flag;
  sprintf(index->iErrorStr, "%s", errorStr);
}

void IndexFree(INDEX *index) {
  if (index->iThing) {
    MemFree( (MEM*)index->iThing, index->iByte);
    index->iThing = NULL;
  }
  index->iNum   = 0;
}

/* allocate space for another entry */
void IndexAlloc(INDEX *index) {
  BYTE errorStr[256];

  errorStr[0]='\0';
  if (*index->iErrorStr)
    sprintf(errorStr, "WARNING: resizing %s.\n", index->iErrorStr);
  index->iNum++;
  REALLOC(errorStr, index->iThing, THING*, index->iNum, index->iByte);
}

void IndexAppend(INDEX *index, void *thing) {
  IndexAlloc(index);
  index->iThing[index->iNum-1] = thing;
}


void IndexInsert(INDEX *index, void *thing, INDEXPROC(*indexProc) ) {
  LWORD i;
  void *tNext;
  void *tThis;

  indexError = FALSE;
  if ( (index->iNum==0) || (*indexProc)(index, (void*)thing, (void*)index->iThing[index->iNum-1])>0 ) {
    IndexAppend(index, thing);
    return;
  }

  IndexAlloc(index); /* remember that iNum has been incremented */
  /* find where to insert the thing into the index */
  i=0;
  while(   (i<index->iNum-1)
        && ( (*indexProc)(index, (void*)thing, (void*)index->iThing[i]) >=0)
       ) {
    if ( (*indexProc)(index, (void*)thing, (void*)index->iThing[i]) ==0) {
      if (!BIT(index->iFlag, IF_ALLOW_DUPLICATE)) {
        Log(LOG_ERROR, "Duplicate index on IndexInsert:");
        LogPrintf(LOG_ERROR, index->iErrorStr);
        LogPrintf(LOG_ERROR, "\n");
        /* should I return here??? */
        indexError = TRUE;
      }
      break;
    } else
      i++; /* this should actually work outside of else statement too but why risk it */
  }

  /* okay now insert thing into index */
  tNext = Thing(index->iThing[i]);
  index->iThing[i] = thing;
  for(i++;i<index->iNum; i++) {
    tThis = index->iThing[i];
    index->iThing[i] = tNext;
    tNext = tThis;
  }
}

/* delete an index entry, preserve ordering as required */
void IndexDelete(INDEX *index, void *thing, INDEXPROC(*indexProc) ) {
  LWORD i;
  void *tNext;

  for (i=0; i<index->iNum; i++) {
    if (index->iThing[i] == thing) break;
  }
  if (i==index->iNum) {
    Log(LOG_ERROR, "IndexDelete: Thing not found\n");
    return; /* not found */
  }
  if (i != index->iNum -1) {
    index->iThing[i] = index->iThing[index->iNum-1];
    if (indexProc) { /* preserve ordering if there is one */
      for(; i<index->iNum-1 && ( (*indexProc)(index, (void*)index->iThing[i], (void*)index->iThing[i+1])>0 ); i++) {
        tNext = index->iThing[i+1];
        index->iThing[i+1] = index->iThing[i];
        index->iThing[i] = tNext;
      }
    }
  }
  index->iNum--;

}

void *IndexFind(INDEX *index, void *key, INDEXFINDPROC(*iFindProc) ) {
  LWORD   min;
  LWORD   mid;
  LWORD   max;
  LWORD   i;
  void   *search;

  if (!index || !index->iThing || !index->iNum) return NULL;

  if (!iFindProc) {
    for (i=0; i<index->iNum; i++)
      if (index->iThing[i] == key) return key;
    return NULL;
  }

  /* binary search via index/findproc */
  min = 0;
  max = index->iNum-1;
  while (max >= min) {
    mid = (min + max)/2;
    if ( (*iFindProc)(index, (void*)key, (void*)index->iThing[mid])  <0)
      max = mid - 1;
    else if ( (*iFindProc)(index, (void*)key, (void*)index->iThing[mid]) >0)
      min = mid + 1;
    else
      break;
  }
  search = index->iThing[mid];

  /* fall through from both search types to here */
  if ((search) && ((*iFindProc)(index, (void*)key, (void*)search) ==0)) {
    return search;
  }
  return NULL;
}

/* erase all entries without de-allocating any entries */
void IndexClear(INDEX *index) {
  if (!index) return;
  index->iNum = 0;
}




