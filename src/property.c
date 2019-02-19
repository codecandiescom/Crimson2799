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
#include "extra.h"
#include "code.h"
#include "property.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "alias.h"
#include "base.h"
#include "effect.h"
#include "object.h"
#include "char.h"
#include "mobile.h"

#define PROPERTY_BLOCK_SIZE 4096

PROPERTY *PropertyAlloc(PROPERTY *pNext, STR *pKey, STR *pDesc) {
  PROPERTY *property;

  MEMALLOC(property, PROPERTY, PROPERTY_BLOCK_SIZE);
  property->pKey = pKey;
  property->pNext = pNext;
  property->pDesc = pDesc;

  return property;
}

/* Insertion sort the list */
PROPERTY *PropertyCreate(PROPERTY *pHead, STR *pKey, STR *pDesc) {
  PROPERTY *prop;

  /* check to make sure it isnt first */
  if (!pHead || STRICMP(pKey->sText, pHead->pKey->sText)<0)
    return PropertyAlloc(pHead, pKey, pDesc);

  /* find insertion point */
  for (prop = pHead; 
       prop->pNext && (STRICMP(pKey->sText, prop->pNext->pKey->sText)>0); 
       prop=prop->pNext);
  
  prop->pNext = PropertyAlloc(prop->pNext, pKey, pDesc);

  return pHead;
}

/* Dont use for binary strings - just plain jane null terminated ones */
PROPERTY *PropertySet(PROPERTY *pHead, BYTE *pKey, BYTE *pDesc) {
  PROPERTY *i;

  i = PropertyFind(pHead, pKey);
  if (i) {
    STRFREE(i->pDesc);
    i->pDesc = STRCREATE(pDesc);
  } else 
    pHead = PropertyCreate(pHead, STRCREATE(pKey), STRCREATE(pDesc));

  return pHead;
}

/*
 * recursive so we don't reverse the order of the property chain 
 * Properties starting with % will not be copied by this routine
 * to allow templates to have global properties across all things
 * created from them 
 */
PROPERTY *PropertyCopy(PROPERTY *pChain) {
  PROPERTY *newChain = NULL;

  if (pChain) {
    if (pChain->pKey->sText[0] != '%') {
      newChain = PropertyAlloc(NULL, StrAlloc(pChain->pKey), StrAlloc(pChain->pDesc));
      newChain->pNext = PropertyCopy(pChain->pNext);
      return newChain;
    } else
      return PropertyCopy(pChain->pNext);
  } 
  return NULL;
}

/* Case insensitive exact match - linear sorted search */
PROPERTY *PropertyFind(PROPERTY *pList, BYTE *pKey) {
  PROPERTY *i;
  LWORD     check;

  if (!pKey) return NULL;
  for (i = pList; i; i = i->pNext) {
    check = STRICMP(pKey, i->pKey->sText);
    if (check == 0) return i;
    else if (check < 0) return NULL; 
  }
  return NULL; /* fall thru to here */
}

PROPERTY *PropertyDelete(PROPERTY *pHead, BYTE *pKey) {
  PROPERTY *i;

  i = PropertyFind(pHead, pKey);
  if (i) pHead = PropertyFree(pHead, i);
  return pHead;
}

PROPERTY *PropertyFree(PROPERTY *pList, PROPERTY *property) {
  PROPERTY *i;
  PROPERTY *pFree;

  if (!pList || !property) return NULL;
  if (pList == property) {
    pFree = pList;
    pList = pFree->pNext;
    STRFREE(pFree->pDesc);
    STRFREE(pFree->pKey);
    MEMFREE(pFree, PROPERTY);
    return pList;
  }

  for (i = pList; i && i->pNext != property; i = i->pNext);
  if (i) { /* found it */
    pFree = i->pNext;
    i->pNext = pFree->pNext;
    STRFREE(pFree->pKey);
    STRFREE(pFree->pDesc);
    MEMFREE(pFree, PROPERTY);
  } else {
    Log(LOG_ERROR, "PropertyFree: PROPERTY pointer not found in pList\n");
  }
  return pList;
}

LWORD PropertyGetLWord(THING *thing, BYTE *key, LWORD defaultVal) {
  PROPERTY *i;

  i = PropertyFind(thing->tProperty, key);
  if (!i && key[0]=='%') {
    if (thing->tType==TTYPE_OBJ)
      i = PropertyFind(Obj(thing)->oTemplate->oProperty, key);
    else if (thing->tType==TTYPE_MOB)
      i = PropertyFind(Mob(thing)->mTemplate->mProperty, key);
  }
  
  if (i) return atol(i->pDesc->sText);
  return defaultVal;
}

void PropertySetLWord(THING *thing, BYTE *key, LWORD setVal) {
  PROPERTY *i;
  BYTE      buf[256];

  i = PropertyFind(thing->tProperty, key);
  
  sprintf(buf, "%ld", setVal);
  if (i) {
    STRFREE(i->pDesc);
    i->pDesc = STRCREATE(buf);
  } else 
    thing->tProperty = PropertyCreate(thing->tProperty, STRCREATE(key), STRCREATE(buf));
}

