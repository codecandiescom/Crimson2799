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

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"

/* enable the following to use individual mallocs for everything - since that way
   you wont be able to step on another blocks space without unix screaming */
#define MEM_MALLOCALLWAYS 

/* enable the following to memset each block to 0 prior to use */
/* #define MEM_CLRBLOCK */

/* enable the following to disable the memSpace monster block allocation */
#define MEM_NOSPACE 

/* this is the largest unit size, not block size! ie it mallocs in really large
   chunks which are broken down into units each smaller than the reuse size */
#define REUSE_MAX_SIZE 512

MEM *freeList[REUSE_MAX_SIZE];

/* this stuff is so that we can prealloc a HUGE chunk of memory (in theory
   just enough to boot the mud) since all the strings etc are seldom changed
 */
BYTE *memSpace     = NULL;
LWORD memSpaceByte = 1048576; /* one meg of space to begin with, resized after boot completion */
LWORD memSpaceUsed = 0; /* amount of preallocated space used up so far...  */
LWORD memUsed = 0;
LWORD memFreeCache = 0;
LWORD memWasted = 0;

void MemInit(void) {
  LWORD i;

  for (i=0; i<REUSE_MAX_SIZE; i++)
    freeList[i] = NULL;
  
  /* prealloc global space (or not)*/
#ifndef MEM_NOSPACE
  MALLOC(memSpace, BYTE, memSpaceByte);
#endif
#ifdef MEM_NOSPACE
  memSpaceByte = 0;
#endif
}


/* if this isnt called dont sweat it.... */
void MemInitDone(void) {
#ifndef MEM_NOSPACE
  while (memSpaceByte/2 > memSpaceUsed) {
    memSpaceByte>>=1;
  }
  memSpace = (BYTE*) realloc(memSpace, memSpaceByte);
  if (!memSpace) {
    Log(LOG_ERROR, "Unable to realloc memSpace!=n");
  }
#endif
}


/* we never free anything just stick it in list till its needed */
BYTE *MemFree(MEM *memFree, LWORD size) {
  memset(memFree, 0, ABSV(size));

#ifndef MEM_MALLOCALLWAYS
  if ((size <= MEM_MALLOC) || (size > REUSE_MAX_SIZE)  || (size < sizeof(MEM))) {
    if ( (BYTE*)memFree >= memSpace && (BYTE*)memFree < memSpace+memSpaceByte) {
      while(size > sizeof(STR)) { /* in memSpace cant free it, so rip it apart */
        MemFree(memFree, sizeof(STR));
        memFree+=sizeof(STR);
        size -= sizeof(STR);
      }
      if (size > sizeof(MEM)) {/* wast not, want not */
        MemFree(memFree, size);
      }
    } else 
#endif
    {
      FREE(memFree, ABSV(size));
    }
#ifndef MEM_MALLOCALLWAYS
  } else {
    memFreeCache += size;
    memFree->mNext = freeList[size-1];
    freeList[size-1] = memFree;
  }
#endif
  return NULL;
}


BYTE *MemAlloc(LWORD size, LWORD blockSize) {
  BYTE *retMemItem;
#ifndef MEM_MALLOCALLWAYS
  BYTE *memBlock;
  MEM  *thisMem;
  LWORD i;
  LWORD numMemBlock;

  /* we dont track these blocks just MALLOC it and return */
  if ((blockSize == MEM_MALLOC) || (size > REUSE_MAX_SIZE)  || (size < sizeof(MEM))) {
#endif
    if (memSpaceByte - memSpaceUsed >= size) {
      retMemItem = memSpace + memSpaceUsed;
      memSpaceUsed += size;
    } else {
      MALLOC(retMemItem, BYTE, size);
#ifdef MEM_CLRBLOCK
      memset(retMemItem, 0, size);
#endif
    }
    return retMemItem;
#ifndef MEM_MALLOCALLWAYS
  }
  
  if (blockSize < size) {
    Log(LOG_ERROR, "MemAlloc(mem.c) BlockSize < size!\n");
    exit(ERROR_NOMEM);
  }
  
  /* well see if we got any lying around I guess */
  if (freeList[size-1]) { /* yah reused an old one */
    retMemItem = (BYTE*)freeList[size-1];
    freeList[size-1] = freeList[size-1]->mNext;
    memFreeCache -= size;

  /* none allready handy, make some */
  } else {
    if (blockSize <= REUSE_MAX_SIZE && freeList[size-1]) {
      memBlock = (BYTE*)freeList[size-1];
      freeList[size-1] = freeList[size-1]->mNext;
    } else {
      if (memSpaceByte - memSpaceUsed >= blockSize) {
        memBlock = memSpace + memSpaceUsed;
        memSpaceUsed += blockSize;
      } else {
        MALLOC(memBlock, BYTE, blockSize); 
#ifdef MEM_CLRBLOCK
        memset(memBlock, 0, blockSize);
#endif
      }
    }
    numMemBlock = blockSize/size;
    retMemItem  = memBlock; /* return first item in the block */
    for (i=1; i<numMemBlock; i++) { /* i=1 because first block is allready taken */
      thisMem = (MEM*) (memBlock + i*size); /* find start of next block */
      thisMem->mNext = freeList[size-1];
      freeList[size-1] = thisMem; /* put it in the available table */
      memFreeCache += size;
    }
      /* if I was really tricky I could use up any remainder too and put it towards 
         a known commonly used and small blocksize ie STR, a future project I think */
    size = blockSize%numMemBlock;
    if (size > sizeof(MEM))/* waste not, want not */
      MemFree( (MEM*)(memBlock + i*size) , size);
    else
      memWasted += size;
  }
#ifdef MEM_CLRBLOCK
  memset(retMemItem, 0, size);
#endif
  return retMemItem;
#endif
}


/* double mem requests every time */
/* note can only access < curMaxSize */
/* str should be something like "WARNING! resizing XXYYZZ\n" */
/* mem should be a pointer pre-inited to NULL if empty */
/* curMaxByte determines the size of the initial malloc too, curMaxSize is recalc'd for any change */
void *MemReallocDouble(BYTE *errorStr, void *memPtr, LWORD memSize, LWORD memNum, LWORD *memByte) { 
  if ( (memPtr == NULL) || (memNum >= (*memByte/memSize) ) ){ 
    if (memPtr) 
      *memByte *=2; 
    if (!memPtr) { 
      if ( !(memPtr = malloc(*memByte)) ) { 
       Log(LOG_ERROR, "realloc/malloc failure: dying a horrible death...\n"); 
       exit(ERROR_NOMEM); 
      }
      memUsed += *memByte;
    } else { 
      if ( !(memPtr = realloc(memPtr, *memByte)) ) { 
        Log(LOG_ERROR, "realloc failure: dying a horrible death...\n"); 
       exit(ERROR_NOMEM); 
      }
      memUsed += *memByte/2;
      /* Turn off the REALLOC warnings for now */
      /* 
      if (errorStr && *errorStr)
        Log(LOG_ERROR, errorStr); 
      */
    } 
  }
  return memPtr;
}


void *MemMoveHigher(void *dst, void *src, LWORD size) {
  LWORD i;
  for (i=size-1; i>=0; i--)
    *((BYTE*)dst+i) = *((BYTE*)src+i);
  return dst;
}



