// pack.c
// two-char identifier: pa
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

// This module contains the functions to help package
// up data items into forms ready to be sent to the moleprot module.
// This module is sorta equivalent to "unpack.c" except data
// is being SENT instead of RECEIVED.

#include<windows.h>
#include<string.h>
#include"dstruct.h"
#include"moleprot.h"
#include"areawnd.h"
#include"pack.h"


void paPackCleanup(HGLOBAL p_Memory) {
  GlobalUnlock(p_Memory);
  GlobalFree(p_Memory);
  return;
}

HGLOBAL paPackWorld(DSSTRUCT *p_thing,long *p_dataLen,char **p_data) {
  long l_size;
  HGLOBAL l_memory;
  char *l_data,*l_p;
  DSSTRUCT *l_tmp;
  int l_extraCount,l_propertyCount,l_exitCount;

  if (!p_thing) return NULL;
  if (p_thing->sType!=DS_STYPE_WORLD) return NULL;

  /* first, let's determine how large of a buffer we need for this item */
  l_size=0L;
  l_size+=4; /* virtual number */
  l_size+=strlen(DSSList(p_thing)->lName.sData)+1;      /* tSDesc */
  l_size+=strlen(DSSWorld(p_thing)->wDesc.sData)+1;    /* tDesc */
  l_size+=4; /* wType */
  l_size+=4; /* wFlag */

  l_size+=4; /* # of exits */
  l_exitCount=0;
  for (l_tmp=DSSWorld(p_thing)->wExit.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_size+=4; /* eDir */
    l_size+=strlen(DSSExit(l_tmp)->eKey.sData)+1;
    l_size+=strlen(DSSExit(l_tmp)->eDesc.sData)+1;
    l_size+=4; /* eKeyObj */
    l_size+=4; /* eFlag */
    l_size+=4; /* eWorld */
    l_exitCount++;
  }
  l_size+=4; /* # of extras */
  l_extraCount=0;
  for (l_tmp=DSSWorld(p_thing)->wExtra.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_size+=strlen(DSSExtra(l_tmp)->eKey.sData)+1;
    l_size+=strlen(DSSExtra(l_tmp)->eDesc.sData)+1;
    l_extraCount++;
  }
  l_size+=4; /* # of properties */
  l_propertyCount=0;
  for (l_tmp=DSSWorld(p_thing)->wProperty.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_size+=strlen(DSSProperty(l_tmp)->pKey.sData)+1;
    l_size+=strlen(DSSProperty(l_tmp)->pDesc.sData)+1;
    l_propertyCount++;
  }

  /* Next, let's allocate the buffer */
  l_memory=GlobalAlloc(GHND|GMEM_NOCOMPACT,l_size);
  l_data=GlobalLock(l_memory);
  if (!l_memory) return NULL;
  if (!l_data) {
    GlobalFree(l_memory);
    return NULL;
  }

  /* ok, load the info into the buffer */
  l_p=l_data;
  l_p+=mpEncodeULWORD(l_p,DSSList(p_thing)->lVNum);
  l_p+=paEncodeString(l_p,DSSList(p_thing)->lName.sData);
  l_p+=paEncodeString(l_p,DSSWorld(p_thing)->wDesc.sData);
  l_p+=mpEncodeULWORD(l_p,DSSWorld(p_thing)->wType);
  l_p+=mpEncodeULWORD(l_p,DSSWorld(p_thing)->wFlag);

  /* encode exits */
  l_p+=mpEncodeULWORD(l_p,l_exitCount);
  for (l_tmp=DSSWorld(p_thing)->wExit.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=mpEncodeULWORD(l_p,DSSExit(l_tmp)->eDir);
    l_p+=paEncodeString(l_p,DSSExit(l_tmp)->eKey.sData);
    l_p+=paEncodeString(l_p,DSSExit(l_tmp)->eDesc.sData);
    l_p+=mpEncodeULWORD(l_p,DSSExit(l_tmp)->eKeyObj);
    l_p+=mpEncodeULWORD(l_p,DSSExit(l_tmp)->eFlag);
    l_p+=mpEncodeULWORD(l_p,DSSExit(l_tmp)->eWorld);
  }
  /* encode extras */
  l_p+=mpEncodeULWORD(l_p,l_extraCount);
  for (l_tmp=DSSWorld(p_thing)->wExtra.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=paEncodeString(l_p,DSSExtra(l_tmp)->eKey.sData);
    l_p+=paEncodeString(l_p,DSSExtra(l_tmp)->eDesc.sData);
  }
  /* encode properties */
  l_p+=mpEncodeULWORD(l_p,l_propertyCount);
  for (l_tmp=DSSWorld(p_thing)->wProperty.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=paEncodeString(l_p,DSSProperty(l_tmp)->pKey.sData);
    l_p+=paEncodeString(l_p,DSSProperty(l_tmp)->pDesc.sData);
  }

  if (p_dataLen)
    *p_dataLen=l_size;
  if (p_data)
    *p_data=l_data;

  /* and we're done! */
  return l_memory;
}

HGLOBAL paPackMobile(DSSTRUCT *p_thing,long *p_dataLen,char **p_data) {
  long l_size;
  HGLOBAL l_memory;
  char *l_data,*l_p;
  DSSTRUCT *l_tmp;
  int l_extraCount,l_propertyCount;

  if (!p_thing) return NULL;
  if (p_thing->sType!=DS_STYPE_MOBILE) return NULL;

  /* first, let's determine how large of a buffer we need for this mobile */
  l_size=0L;
  l_size+=4; /* virtual number */
  l_size+=strlen(DSSMobile(p_thing)->mKey.sData)+1;     /* mKey */
  l_size+=strlen(DSSList(p_thing)->lName.sData)+1;      /* mSDesc */
  l_size+=strlen(DSSMobile(p_thing)->mLDesc.sData)+1;   /* mLDesc */
  l_size+=strlen(DSSMobile(p_thing)->mDesc.sData)+1;    /* mDesc */
  l_size+=4; /* mAct */
  l_size+=4; /* mAffect */
  l_size+=4; /* mAura */
  l_size+=4; /* mLevel */
  l_size+=4; /* mHitBonus */
  l_size+=4; /* mArmor */
  l_size+=4; /* mHPDiceNum */
  l_size+=4; /* mHPDiceSize */
  l_size+=4; /* mHPBonus */
  l_size+=4; /* mDamDiceNum */
  l_size+=4; /* mDamDiceSize */
  l_size+=4; /* mDamBonus */
  l_size+=4; /* mMoney */
  l_size+=4; /* mExp */
  l_size+=4; /* mPos */
  l_size+=4; /* mType */
  l_size+=4; /* mSex */
  l_size+=4; /* mWeight */

  l_size+=4; /* # of extras */
  l_extraCount=0;
  for (l_tmp=DSSMobile(p_thing)->mExtra.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_size+=strlen(DSSExtra(l_tmp)->eKey.sData)+1;
    l_size+=strlen(DSSExtra(l_tmp)->eDesc.sData)+1;
    l_extraCount++;
  }
  l_size+=4; /* # of properties */
  l_propertyCount=0;
  for (l_tmp=DSSMobile(p_thing)->mProperty.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_size+=strlen(DSSProperty(l_tmp)->pKey.sData)+1;
    l_size+=strlen(DSSProperty(l_tmp)->pDesc.sData)+1;
    l_propertyCount++;
  }

  /* Next, let's allocate the buffer */
  l_memory=GlobalAlloc(GHND|GMEM_NOCOMPACT,l_size);
  l_data=GlobalLock(l_memory);
  if (!l_memory) return NULL;
  if (!l_data) {
    GlobalFree(l_memory);
    return NULL;
  }

  /* ok, load the info into the buffer */
  l_p=l_data;

  l_p+=mpEncodeULWORD(l_p,DSSList(p_thing)->lVNum);
  l_p+=paEncodeString(l_p,DSSMobile(p_thing)->mKey.sData);
  l_p+=paEncodeString(l_p,DSSList(p_thing)->lName.sData);
  l_p+=paEncodeString(l_p,DSSMobile(p_thing)->mLDesc.sData);
  l_p+=paEncodeString(l_p,DSSMobile(p_thing)->mDesc.sData);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mAct);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mAffect);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mAura);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mLevel);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mHitBonus);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mArmor);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mHPDiceNum);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mHPDiceSize);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mHPBonus);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mDamDiceNum);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mDamDiceSize);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mDamBonus);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mMoney);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mExp);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mPos);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mType);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mSex);
  l_p+=mpEncodeULWORD(l_p,DSSMobile(p_thing)->mWeight);
  /* encode extras */
  l_p+=mpEncodeULWORD(l_p,l_extraCount);
  for (l_tmp=DSSMobile(p_thing)->mExtra.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=paEncodeString(l_p,DSSExtra(l_tmp)->eKey.sData);
    l_p+=paEncodeString(l_p,DSSExtra(l_tmp)->eDesc.sData);
  }
  /* encode properties */
  l_p+=mpEncodeULWORD(l_p,l_propertyCount);
  for (l_tmp=DSSMobile(p_thing)->mProperty.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=paEncodeString(l_p,DSSProperty(l_tmp)->pKey.sData);
    l_p+=paEncodeString(l_p,DSSProperty(l_tmp)->pDesc.sData);
  }

  if (p_dataLen)
    *p_dataLen=l_size;
  if (p_data)
    *p_data=l_data;

  /* and we're done! */
  return l_memory;
}

HGLOBAL paPackObject(DSSTRUCT *p_thing,long *p_dataLen,char **p_data) {
  long l_size;
  HGLOBAL l_memory;
  char *l_data,*l_p;
  DSSTRUCT *l_tmp;
  int l_extraCount,l_propertyCount,l_applyCount,l_detailCount;
  int l_i;
  DSFTL *l_ftl;

  if (!p_thing) return NULL;
  if (p_thing->sType!=DS_STYPE_OBJECT) return NULL;

  /* first, let's determine how large of a buffer we need for this object */
  l_size=0L;
  l_size+=4; /* virtual number */
  l_size+=strlen(DSSObject(p_thing)->oKey.sData)+1;     /* oKey */
  l_size+=strlen(DSSList(p_thing)->lName.sData)+1;      /* oSDesc */
  l_size+=strlen(DSSObject(p_thing)->oLDesc.sData)+1;   /* oLDesc */
  l_size+=strlen(DSSObject(p_thing)->oDesc.sData)+1;    /* oDesc */
  l_size+=4; /* oType */
  l_size+=4; /* oAct */
  l_size+=4; /* oWear */
  l_size+=4; /* oWeight */
  l_size+=4; /* oValue */
  l_size+=4; /* oRent */

  /* # details */
  l_ftl=dsFTLOf(&g_awFTLObjType,DSSObject(p_thing)->oType);
  l_detailCount=0;
  /* skip past initial (blank) marker FTL */
  if (l_ftl)
    l_ftl=l_ftl->fNext;
  while(l_ftl) {
    l_detailCount++;
    l_ftl=l_ftl->fNext;
    l_size+=4; /* oDetail */
  }

  l_size+=4; /* # of apply values */
  l_applyCount=DS_OBJECT_MAX_APPLY;
  l_size+=DS_OBJECT_MAX_APPLY*4; /* oApplyType  */
  l_size+=DS_OBJECT_MAX_APPLY*4; /* oApplyValue */

  l_size+=4; /* # of extras */
  l_extraCount=0;
  for (l_tmp=DSSObject(p_thing)->oExtra.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_size+=strlen(DSSExtra(l_tmp)->eKey.sData)+1;
    l_size+=strlen(DSSExtra(l_tmp)->eDesc.sData)+1;
    l_extraCount++;
  }
  l_size+=4; /* # of properties */
  l_propertyCount=0;
  for (l_tmp=DSSObject(p_thing)->oProperty.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_size+=strlen(DSSProperty(l_tmp)->pKey.sData)+1;
    l_size+=strlen(DSSProperty(l_tmp)->pDesc.sData)+1;
    l_propertyCount++;
  }

  /* Next, let's allocate the buffer */
  l_memory=GlobalAlloc(GHND|GMEM_NOCOMPACT,l_size);
  l_data=GlobalLock(l_memory);
  if (!l_memory) return NULL;
  if (!l_data) {
    GlobalFree(l_memory);
    return NULL;
  }

  /* ok, load the info into the buffer */
  l_p=l_data;

  l_p+=mpEncodeULWORD(l_p,DSSList(p_thing)->lVNum);
  l_p+=paEncodeString(l_p,DSSObject(p_thing)->oKey.sData);
  l_p+=paEncodeString(l_p,DSSList(p_thing)->lName.sData);
  l_p+=paEncodeString(l_p,DSSObject(p_thing)->oLDesc.sData);
  l_p+=paEncodeString(l_p,DSSObject(p_thing)->oDesc.sData);
  l_p+=mpEncodeULWORD(l_p,DSSObject(p_thing)->oType);
  l_p+=mpEncodeULWORD(l_p,DSSObject(p_thing)->oAct);
  l_p+=mpEncodeULWORD(l_p,DSSObject(p_thing)->oWear);
  l_p+=mpEncodeULWORD(l_p,DSSObject(p_thing)->oWeight);
  l_p+=mpEncodeULWORD(l_p,DSSObject(p_thing)->oValue);
  l_p+=mpEncodeULWORD(l_p,DSSObject(p_thing)->oRent);

  /* details */
  for (l_i=0;l_i<l_detailCount;l_i++) {
    l_p+=mpEncodeULWORD(l_p,(DSSObject(p_thing)->oDetail)[l_i]);
  }

  /* encode apply values */
  l_p+=mpEncodeULWORD(l_p,l_applyCount);
  for(l_i=0;l_i<l_applyCount;l_i++) {
    l_p+=mpEncodeULWORD(l_p,(DSSObject(p_thing)->oApplyType)[l_i]);
    l_p+=mpEncodeULWORD(l_p,(DSSObject(p_thing)->oApplyValue)[l_i]);
  }

  /* encode extras */
  l_p+=mpEncodeULWORD(l_p,l_extraCount);
  for (l_tmp=DSSObject(p_thing)->oExtra.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=paEncodeString(l_p,DSSExtra(l_tmp)->eKey.sData);
    l_p+=paEncodeString(l_p,DSSExtra(l_tmp)->eDesc.sData);
  }
  /* encode properties */
  l_p+=mpEncodeULWORD(l_p,l_propertyCount);
  for (l_tmp=DSSObject(p_thing)->oProperty.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=paEncodeString(l_p,DSSProperty(l_tmp)->pKey.sData);
    l_p+=paEncodeString(l_p,DSSProperty(l_tmp)->pDesc.sData);
  }

  if (p_dataLen)
    *p_dataLen=l_size;
  if (p_data)
    *p_data=l_data;

  /* and we're done! */
  return l_memory;
}

HGLOBAL paPackAreaDetail(DSSTRUCT *p_thing,long *p_dataLen,char **p_data) {
  long l_size;
  HGLOBAL l_memory;
  char *l_data,*l_p;
  DSSTRUCT *l_tmp;
  DSSTRUCT *l_area;
  int l_propertyCount;

  if (!p_thing) return NULL;
  if (p_thing->sType!=DS_STYPE_AREA) return NULL;

  l_area=p_thing;
  p_thing=DSSArea(l_area)->aDetail.rList;
  if (!p_thing) return NULL;

  /* from aResetThing, reset scripts */
  /* first, let's determine how large of a buffer we need for this item */
  l_size=0L;
  l_size+=4; /* virtual number */
//  l_size+=strlen(DSSList(l_area)->lName.sData)+1;      /* lName */
  l_size+=strlen(DSSAreaDetail(p_thing)->aEditor.sData)+1;      /* aEditor */
  l_size+=strlen(DSSAreaDetail(p_thing)->aDesc.sData)+1;      /* aDesc */
  l_size+=4; /* aResetFlag */
  l_size+=4; /* aResetDelay */

  l_size+=4; /* # of properties */
  l_propertyCount=0;
  for (l_tmp=DSSAreaDetail(p_thing)->aProperty.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_size+=strlen(DSSProperty(l_tmp)->pKey.sData)+1;
    l_size+=strlen(DSSProperty(l_tmp)->pDesc.sData)+1;
    l_propertyCount++;
  }

  /* Next, let's allocate the buffer */
  l_memory=GlobalAlloc(GHND|GMEM_NOCOMPACT,l_size);
  l_data=GlobalLock(l_memory);
  if (!l_memory) return NULL;
  if (!l_data) {
    GlobalFree(l_memory);
    return NULL;
  }

  /* ok, load the info into the buffer */
  l_p=l_data;
  l_p+=mpEncodeULWORD(l_p,DSSList(l_area)->lVNum);
//  l_p+=paEncodeString(l_p,DSSList(p_thing)->lName.sData);
  l_p+=paEncodeString(l_p,DSSAreaDetail(p_thing)->aEditor.sData);
  l_p+=paEncodeString(l_p,DSSAreaDetail(p_thing)->aDesc.sData);
  l_p+=mpEncodeULWORD(l_p,DSSAreaDetail(p_thing)->aResetFlag);
  l_p+=mpEncodeULWORD(l_p,DSSAreaDetail(p_thing)->aResetDelay);

  /* encode properties */
  l_p+=mpEncodeULWORD(l_p,l_propertyCount);
  for (l_tmp=DSSAreaDetail(p_thing)->aProperty.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=paEncodeString(l_p,DSSProperty(l_tmp)->pKey.sData);
    l_p+=paEncodeString(l_p,DSSProperty(l_tmp)->pDesc.sData);
  }

  if (p_dataLen)
    *p_dataLen=l_size;
  if (p_data)
    *p_data=l_data;

  /* and we're done! */
  return l_memory;
}

HGLOBAL paPackReset(DSSTRUCT *p_thing,long *p_dataLen,char **p_data) {
  long l_size;
  HGLOBAL l_memory;
  char *l_data,*l_p;
  DSSTRUCT *l_tmp;
  DSSTRUCT *l_area;
  int l_resetCount;

  if (!p_thing) return NULL;
  if (p_thing->sType!=DS_STYPE_AREA) return NULL;

  l_area=p_thing;
  /* note that we **MAY HAVE A NULL RESET LIST** (ie: p_thing may be NULL) */

  /* first, let's determine how large of a buffer we need for this item */
  l_size=0L;
  l_size+=4; /* virtual number */
  l_size+=4; /* # reset items */
  l_resetCount=0;
  for (l_tmp=DSSArea(l_area)->aRST.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_resetCount++;
    l_size+=4; /* Cmd */
    l_size+=4; /* if */
    l_size+=4; /* Arg1 */
    l_size+=4; /* Arg2 */
    l_size+=4; /* Arg3 */
    l_size+=4; /* Arg4 */
  }

  /* Next, let's allocate the buffer */
  l_memory=GlobalAlloc(GHND|GMEM_NOCOMPACT,l_size);
  l_data=GlobalLock(l_memory);
  if (!l_memory) return NULL;
  if (!l_data) {
    GlobalFree(l_memory);
    return NULL;
  }

  /* ok, load the info into the buffer */
  l_p=l_data;
  l_p+=mpEncodeULWORD(l_p,DSSList(l_area)->lVNum);
  l_p+=mpEncodeULWORD(l_p,l_resetCount);
  for (l_tmp=DSSArea(l_area)->aRST.rList;l_tmp;l_tmp=l_tmp->sNext) {
    l_p+=mpEncodeULWORD(l_p,DSSReset(l_tmp)->rCmd);
    l_p+=mpEncodeULWORD(l_p,DSSReset(l_tmp)->rIf);
    l_p+=mpEncodeULWORD(l_p,DSSReset(l_tmp)->rArg1);
    l_p+=mpEncodeULWORD(l_p,DSSReset(l_tmp)->rArg2);
    l_p+=mpEncodeULWORD(l_p,DSSReset(l_tmp)->rArg3);
    l_p+=mpEncodeULWORD(l_p,DSSReset(l_tmp)->rArg4);
  }

  if (p_dataLen)
    *p_dataLen=l_size;
  if (p_data)
    *p_data=l_data;

  /* and we're done! */
  return l_memory;
}

int paEncodeString(char *p_dst,char *p_src) {
  int l_size;
  if (!p_dst) return 0;
  if (!p_src) {
    *p_dst=0;
    return 1;
  }
  l_size=1; /* 1 = terminating 0 */
  while (*p_src) {
    *p_dst=*p_src;
    l_size++;
    p_dst++;
    p_src++;
  }
  *p_dst=0;
  return l_size;
}


