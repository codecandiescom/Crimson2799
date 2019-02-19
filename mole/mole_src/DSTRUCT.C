/* Dynamic Structure header file */
// 2-char identifier: ds
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

#include<windows.h>
#include<string.h>
#include<time.h>
#include"moledefs.h"
#include"molem.h"
#include"debug.h"
#include"host.h"
#include"dstruct.h"

/***** Reference Library *****/
/* dsRefFree - clears any attached structures and sets elements to NULL */
void dsRefFree(DSREF *p_ref) {
  if (!p_ref) return;

  while(p_ref->rList) {
    dsStructFree(p_ref->rList);
  }
  p_ref->rState=DS_NS_NOTAVAIL;
  p_ref->rList=NULL;
  if (p_ref->rEditWindow)
    DestroyWindow(p_ref->rEditWindow);
  p_ref->rEditWindow=NULL;
  return;
}

/* Initializes a reference structure to all NULL */
void dsRefClear(DSREF *p_ref) {
  if (!p_ref) return;

  p_ref->rState=DS_NS_NOTAVAIL;
  p_ref->rLoadTime=0L;
  p_ref->rList=NULL;
  p_ref->rFlag=0L;
  p_ref->rEditWindow=NULL;

  return;
}

/***** STRing Library *****/
/* Free a STR */
void dsStrFree(DSSTR *p_str) {
  HGLOBAL l_GlobalTemp;

  if (!p_str) return;

  l_GlobalTemp=p_str->sMemory;
  GlobalUnlock(l_GlobalTemp);
  GlobalFree(l_GlobalTemp);

  dsStrClear(p_str);

  return;
}

/* initialize a STR */
void dsStrClear(DSSTR *p_str) {
  if (!p_str) return;
  p_str->sData=NULL;
  p_str->sMemory=NULL;
  return;
}

/* Allocate memory for specified size str */
unsigned long dsStrAlloc(DSSTR *p_str,unsigned long p_len) {
  HGLOBAL l_GlobalTemp;

  if ((!p_str)||(p_len < 1)) return 0L;
  l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,p_len);
  if (!l_GlobalTemp)
    return 0L;
  p_str->sData=(char *)GlobalLock(l_GlobalTemp);
  if (!(p_str->sData)) {
    GlobalFree(l_GlobalTemp);
    return 0L;
  }
  p_str->sMemory=l_GlobalTemp;
  return GlobalSize(l_GlobalTemp);
}

/* a dsStrAlloc with a strcpy included */
unsigned long dsStrCreate(DSSTR *p_str,char *p_data) {
  unsigned long l_size;
  if (!p_str)
    return 0L;
  if (!p_data) {
    dsStrClear(p_str);
    return 0L;
  }

  l_size=dsStrAlloc(p_str,strlen(p_data)+1);
  if (l_size) {
    strcpy(p_str->sData,p_data);
  }
  return l_size;
}

/* duplicates a STR - WITHOUT new alloc!!! */
void dsStrCopy(DSSTR *p_src,DSSTR *p_dst) {
  if ((!p_dst)||(!p_src))
    return;

  p_dst->sData=p_src->sData;
  p_dst->sMemory=p_src->sMemory;
}

/* duplicates a STR - WITH new alloc!!! */
unsigned long dsStrReplicate(DSSTR *p_src,DSSTR *p_dst) {
  if ((!p_dst)||(!p_src)||(!p_src->sData))
    return 0L;

  return dsStrCreate(p_dst,p_src->sData);
}

/***** FTL Library *****/

DSFTL *dsFTLAlloc(char *p_name){
  DSFTL *l_ftl;
  HGLOBAL l_GlobalTemp;
  int l_i;

  l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSFTL));
  l_ftl=(DSFTL *)GlobalLock(l_GlobalTemp);
  if ((!l_GlobalTemp)||(!(l_ftl)))
    return NULL;
  l_ftl->fMemory=l_GlobalTemp;
  l_ftl->fNext=NULL;
  l_ftl->fDown=NULL;
  l_ftl->fType=MOLE_LISTTYPE_INVALID;
  l_ftl->fList=MOLE_LIST_INVALID;
  l_i=0;
  if (p_name)
    for (l_i=0;(l_i<DS_FTL_NAMELEN-1)&&(p_name[l_i]);l_i++)
      (l_ftl->fName)[l_i]=p_name[l_i];
  (l_ftl->fName)[l_i]=0;
  return l_ftl;
}

void dsFTLFree(DSFTL **p_FTList){
  DSFTL *l_ftl,*l_nextftl,*l_downftl;
  HGLOBAL l_GlobalTemp;

  if (!p_FTList)
    return;

  l_ftl=*p_FTList;
  while(l_ftl) {
    l_downftl=l_ftl->fDown;
    while(l_ftl) {
      l_nextftl=l_ftl->fNext;
      l_GlobalTemp=l_ftl->fMemory;
      GlobalUnlock(l_GlobalTemp);
      GlobalFree(l_GlobalTemp);
      l_ftl=l_nextftl;
    }
    l_ftl=l_downftl;
  }
  *p_FTList=NULL;
  return;
}

void dsFTLAppendNext(DSFTL **p_FTList,DSFTL *p_newEntry){
  DSFTL *l_ftl;

  if (!p_FTList)
    return;
  l_ftl=*p_FTList;
  if (!l_ftl)
    *p_FTList=p_newEntry;
  else {
    while(l_ftl->fNext)
      l_ftl=l_ftl->fNext;
    l_ftl->fNext=p_newEntry;
  }
  return;
}

void dsFTLAppendDown(DSFTL **p_FTList,DSFTL *p_newEntry) {
  DSFTL *l_ftl;

  if (!p_FTList)
    return;
  l_ftl=*p_FTList;
  if (!l_ftl)
    *p_FTList=p_newEntry;
  else {
    while(l_ftl->fDown)
      l_ftl=l_ftl->fDown;
    l_ftl->fDown=p_newEntry;
  }
  return;
}

DSFTL *dsFTLOf(DSFTL **p_FTList,int p_FTLNum) {
  DSFTL *l_ftl;

  if (!p_FTList)
    return NULL;
  for (l_ftl=*p_FTList;(l_ftl)&&(p_FTLNum);p_FTLNum--,l_ftl=l_ftl->fDown);
  return l_ftl;
}

/***** Structure Library *****/
/* Free a structure and any child structures */
void dsStructFree(DSSTRUCT *p_struct) {
  HGLOBAL l_GlobalTemp;

  if (!p_struct) return;

  /* first, free any child structures, or type-specific attachments */
  if (p_struct->sType>=DS_STYPE_LIST)
    dsStrFree(&(DSSList(p_struct)->lName));

  switch(p_struct->sType) {
    case DS_STYPE_EXIT:
      break;
    case DS_STYPE_PROPERTY: /* Extra & Property structs are identical */
    case DS_STYPE_EXTRA:
      dsStrFree(&(DSSExtra(p_struct)->eKey));
      dsStrFree(&(DSSExtra(p_struct)->eDesc));
      break;
    case DS_STYPE_AREADETAIL:
      dsStrFree(&(DSSAreaDetail(p_struct)->aEditor));
      dsStrFree(&(DSSAreaDetail(p_struct)->aDesc));
      dsRefFree(&(DSSAreaDetail(p_struct)->aProperty));
      break;
    case DS_STYPE_LIST:
      break;
    case DS_STYPE_AREA:
      dsRefFree(&(DSSArea(p_struct)->aOBJ));
      dsRefFree(&(DSSArea(p_struct)->aMOB));
      dsRefFree(&(DSSArea(p_struct)->aWLD));
      dsRefFree(&(DSSArea(p_struct)->aRST));
      dsRefFree(&(DSSArea(p_struct)->aDetail));
      break;
    case DS_STYPE_WORLD:
      dsStrFree(&(DSSWorld(p_struct)->wDesc));
      dsRefFree(&(DSSWorld(p_struct)->wExit));
      dsRefFree(&(DSSWorld(p_struct)->wExtra));
      dsRefFree(&(DSSWorld(p_struct)->wProperty));
      break;
    case DS_STYPE_MOBILE:
      dsStrFree(&(DSSMobile(p_struct)->mKey));
      dsStrFree(&(DSSMobile(p_struct)->mLDesc));
      dsStrFree(&(DSSMobile(p_struct)->mDesc));
      dsRefFree(&(DSSMobile(p_struct)->mExtra));
      dsRefFree(&(DSSMobile(p_struct)->mProperty));
      break;
    case DS_STYPE_OBJECT:
      dsStrFree(&(DSSObject(p_struct)->oKey));
      dsStrFree(&(DSSObject(p_struct)->oLDesc));
      dsStrFree(&(DSSObject(p_struct)->oDesc));
      dsRefFree(&(DSSObject(p_struct)->oExtra));
      dsRefFree(&(DSSObject(p_struct)->oProperty));
      break;
    case DS_STYPE_RESET:
    case DS_STYPE_NONE:
    default:
      break;
  }

  /* now extract this structure from it's list */
  dsStructExtract(p_struct);

  /* close any editing window which may be open */
  if (p_struct->sEditWindow) {
    aaDestroyWindow(p_struct->sEditWindow);
  }

  /* and then free the memory alloc'ed to this structure */
  l_GlobalTemp=p_struct->sMemory;
  GlobalUnlock(l_GlobalTemp);
  GlobalFree(l_GlobalTemp);

  /* and we're done! */
  return;
}

/* allocate a new structure and initialize it. */
DSSTRUCT *dsStructAlloc(int p_type) {
  HGLOBAL l_GlobalTemp;
  DSSTRUCT *l_struct;

  switch (p_type) {
    case DS_STYPE_EXIT:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSEXIT));
      break;
    case DS_STYPE_EXTRA:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSEXTRA));
      break;
    case DS_STYPE_PROPERTY:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSPROPERTY));
      break;
    case DS_STYPE_AREADETAIL:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSAREADETAIL));
      break;
    case DS_STYPE_LIST:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSLIST));
      break;
    case DS_STYPE_AREA:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSAREA));
      break;
    case DS_STYPE_WORLD:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSWORLD));
      break;
    case DS_STYPE_MOBILE:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSMOBILE));
      break;
    case DS_STYPE_OBJECT:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSOBJECT));
      break;
    case DS_STYPE_RESET:
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSRESET));
      break;
    case DS_STYPE_NONE:
    default:
      p_type=DS_STYPE_NONE;
      l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DSSTRUCT));
      break;
  }

  l_struct=DSStruct(GlobalLock(l_GlobalTemp));
  if ((!l_GlobalTemp)||(!l_struct))
    return NULL;

  /* now initialize the structure */
  l_struct->sType=p_type;
  l_struct->sState=DS_NS_NOTAVAIL;
  l_struct->sMemory=l_GlobalTemp;
  l_struct->sLoadTime=0L;
  l_struct->sPrevious=l_struct->sNext=NULL;
  l_struct->sUpRef=NULL;
  l_struct->sEditWindow=NULL;

  /* type-specific initializations */
  if (p_type>=DS_STYPE_LIST) {
    dsStrClear(&(DSSList(l_struct)->lName));
    DSSList(l_struct)->lVNum=DSSList(l_struct)->lVNum2=0L;
  }

  switch (p_type) {
    case DS_STYPE_EXIT:
      break;
    case DS_STYPE_PROPERTY: /* property & extra structures are identical */
    case DS_STYPE_EXTRA:
      dsStrClear(&(DSSExtra(l_struct)->eKey));
      dsStrClear(&(DSSExtra(l_struct)->eDesc));
      break;
    case DS_STYPE_AREADETAIL:
      dsStrClear(&(DSSAreaDetail(l_struct)->aEditor));
      dsStrClear(&(DSSAreaDetail(l_struct)->aDesc));
      dsRefClear(&(DSSAreaDetail(l_struct)->aProperty));
      break;
    case DS_STYPE_LIST:
      break;
    case DS_STYPE_AREA:
      dsRefClear(&(DSSArea(l_struct)->aOBJ));
      dsRefClear(&(DSSArea(l_struct)->aMOB));
      dsRefClear(&(DSSArea(l_struct)->aWLD));
      dsRefClear(&(DSSArea(l_struct)->aRST));
      dsRefClear(&(DSSArea(l_struct)->aDetail));
      DSSArea(l_struct)->aFlag=0L;
      break;
    case DS_STYPE_WORLD:
      dsStrClear(&(DSSWorld(l_struct)->wDesc));
      dsRefClear(&(DSSWorld(l_struct)->wExit));
      dsRefClear(&(DSSWorld(l_struct)->wExtra));
      dsRefClear(&(DSSWorld(l_struct)->wProperty));
      break;
    case DS_STYPE_MOBILE:
      dsStrClear(&(DSSMobile(l_struct)->mKey));
      dsStrClear(&(DSSMobile(l_struct)->mLDesc));
      dsStrClear(&(DSSMobile(l_struct)->mDesc));
      dsRefClear(&(DSSMobile(l_struct)->mExtra));
      dsRefClear(&(DSSMobile(l_struct)->mProperty));
      break;
    case DS_STYPE_OBJECT:
      dsStrClear(&(DSSObject(l_struct)->oKey));
      dsStrClear(&(DSSObject(l_struct)->oLDesc));
      dsStrClear(&(DSSObject(l_struct)->oDesc));
      dsRefClear(&(DSSObject(l_struct)->oExtra));
      dsRefClear(&(DSSObject(l_struct)->oProperty));
      break;
    case DS_STYPE_RESET:
    case DS_STYPE_NONE:
    default:
      break;
  }

  return l_struct;
}

/* extract structure from its list */
void dsStructExtract(DSSTRUCT *p_struct) {
  if (!p_struct) return;

  /* update the sPrevious structure */
  if (p_struct->sPrevious) {
    p_struct->sPrevious->sNext=p_struct->sNext;
  } else if (p_struct->sUpRef) {  /* check if we need to update our sUpRef */
    p_struct->sUpRef->rList=p_struct->sNext;
  } /*else {
    dbPrint ("Structure list has been lost - oops!");
  }   */

  /* update the sNext structure */
  if (p_struct->sNext) {
    p_struct->sNext->sPrevious=p_struct->sPrevious;
  }

  return;
}

/* This inserts p_struct at the start of the list (if p_upref is not NULL) or
 * after p_prev */
/* NOTE that it's always assumed that the REFERENCE points to the START of this
 * structure linked list, not someplace in the middle */
void dsStructInsert(DSREF *p_upref, DSSTRUCT *p_prev, DSSTRUCT *p_struct) {
  if (!p_struct) return;
  if (p_upref) {
    if (p_upref->rList) {
      p_struct->sNext=p_upref->rList;
      (p_upref->rList)->sPrevious=p_struct;
    } else { /* first element in list */
      p_struct->sPrevious=p_struct->sNext=NULL;
    }
    p_upref->rList=p_struct;
    p_struct->sUpRef=p_upref;
  } else if (p_prev) {
    p_struct->sNext=p_prev->sNext;
    p_struct->sPrevious=p_prev;
    p_struct->sUpRef=p_prev->sUpRef;
    if (p_prev->sNext)
      (p_prev->sNext)->sPrevious=p_struct;
    p_prev->sNext=p_struct;
  } else return;
  return;
}

/* This proc will insert a DS_STYPE_LIST or subtype into a list */
/* The list is sorted according to the lVNum virtual number.    */
void dsStructInsertList(DSREF *p_upref, DSSTRUCT *p_struct) {
  DSSTRUCT *l_curstruct,*l_prevstruct;

  if (!p_struct) return;

  if (p_upref->rList) {
    l_prevstruct=NULL;
    l_curstruct=p_upref->rList;
    while(l_curstruct && (DSSList(l_curstruct)->lVNum<DSSList(p_struct)->lVNum)) {
      l_prevstruct=l_curstruct;
      l_curstruct=l_curstruct->sNext;
    }
    if (l_prevstruct) {
      dsStructInsert(NULL,l_prevstruct,p_struct);
    } else {
      /* it belongs at the start */
      dsStructInsert(p_upref,NULL,p_struct);
    }

  } else { /* first element in list */
    dsStructInsert(p_upref,NULL,p_struct);
  }
  return;
}

/* Copy struct - note both p_src and p_dst MUST be already alloc'ed and
 * both must be of the same sType!!!
 * WARNING WARNING: This proc should be used with caution.
 *   This proc does not copy the structure verbatim. It selectively
 *   transfers SOME information, NOT ALL. This is because this
 *   proc isn't intended to be a complete re-hasher, re-sorter, etc.
 *   IT IS INTENDED TO COPY PERTINENT USER-RELATED DATA for Cut/Paste
 *   type operations. USE WITH CAUTION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*   -------======<<<<< USE WITH CAUTION >>>>>=====------------ */
/*   -------======<<<<< USE WITH CAUTION >>>>>=====------------ */
void dsStructCopy(DSSTRUCT *p_src,DSSTRUCT *p_dst) {
  int l_i;

  if ((!p_src)||(!p_dst))
    return;

  if (p_src->sType!=p_dst->sType)
    return;

  if (p_src->sState!=DS_NS_AVAIL)
    return;

  /* Ok, let's copy! */
  /* DSSTRUCT common */
  p_dst->sState=DS_NS_AVAIL; /* note this may cause confusion if a load from the server is pending! */
  p_dst->sLoadTime=time(NULL);
  switch (p_src->sType) {
    case DS_STYPE_NONE:
      break;
    case DS_STYPE_RESET:
      DSSReset(p_dst)->rCmd=DSSReset(p_src)->rCmd;
      DSSReset(p_dst)->rIf=DSSReset(p_src)->rIf;
      DSSReset(p_dst)->rArg1=DSSReset(p_src)->rArg1;
      DSSReset(p_dst)->rArg2=DSSReset(p_src)->rArg2;
      DSSReset(p_dst)->rArg3=DSSReset(p_src)->rArg3;
      DSSReset(p_dst)->rArg4=DSSReset(p_src)->rArg4;
      break;
    case DS_STYPE_EXTRA:
      dsStrFree(&(DSSExtra(p_dst)->eKey));
      dsStrReplicate(&(DSSExtra(p_dst)->eKey),&(DSSExtra(p_src)->eKey));
      dsStrFree(&(DSSExtra(p_dst)->eDesc));
      dsStrReplicate(&(DSSExtra(p_dst)->eDesc),&(DSSExtra(p_src)->eDesc));
      break;
    case DS_STYPE_PROPERTY:
      dsStrFree(&(DSSProperty(p_dst)->pKey));
      dsStrReplicate(&(DSSProperty(p_dst)->pKey),
        &(DSSProperty(p_src)->pKey));
      dsStrFree(&(DSSProperty(p_dst)->pDesc));
      dsStrReplicate(&(DSSProperty(p_dst)->pDesc),
        &(DSSProperty(p_src)->pDesc));
      break;
    case DS_STYPE_EXIT:
      dsStrFree(&(DSSExit(p_dst)->eKey));
      dsStrReplicate(&(DSSExit(p_dst)->eKey),&(DSSExit(p_src)->eKey));
      dsStrFree(&(DSSExit(p_dst)->eDesc));
      dsStrReplicate(&(DSSExit(p_dst)->eDesc),&(DSSExit(p_src)->eDesc));
      DSSExit(p_dst)->eKeyObj=DSSExit(p_src)->eKeyObj;
      DSSExit(p_dst)->eFlag=DSSExit(p_src)->eFlag;
      DSSExit(p_dst)->eWorld=DSSExit(p_src)->eWorld;
      DSSExit(p_dst)->eDir=DSSExit(p_src)->eDir;
      break;
    case DS_STYPE_AREADETAIL:
      dsStrFree(&(DSSAreaDetail(p_dst)->aEditor));
      dsStrReplicate(&(DSSAreaDetail(p_dst)->aEditor),
        &(DSSAreaDetail(p_src)->aEditor));
      dsStrFree(&(DSSAreaDetail(p_dst)->aDesc));
      dsStrReplicate(&(DSSAreaDetail(p_dst)->aDesc),
        &(DSSAreaDetail(p_src)->aDesc));
      break;
    case DS_STYPE_LIST:
    case DS_STYPE_AREA:
    case DS_STYPE_WORLD:
    case DS_STYPE_MOBILE:
    case DS_STYPE_OBJECT:
      dsStrFree(&(DSSList(p_dst)->lName));
      dsStrReplicate(&(DSSList(p_dst)->lName),
        &(DSSList(p_src)->lName));
      /* DON'T TOUCH lVNum or lVNum2!! */
      switch (p_src->sType) {
        case DS_STYPE_AREA:
          DSSArea(p_dst)->aFlag=DSSArea(p_src)->aFlag;
          break;
        case DS_STYPE_WORLD:
          dsStrFree(&(DSSWorld(p_dst)->wDesc));
          dsStrReplicate(&(DSSWorld(p_dst)->wDesc),
            &(DSSWorld(p_src)->wDesc));
          /* don't copy exits, extras, or properties */
          DSSWorld(p_dst)->wFlag=DSSWorld(p_src)->wFlag;
          DSSWorld(p_dst)->wType=DSSWorld(p_dst)->wType;
          break;
        case DS_STYPE_MOBILE:
          dsStrFree(&(DSSMobile(p_dst)->mKey));
          dsStrReplicate(&(DSSMobile(p_dst)->mKey),
            &(DSSMobile(p_src)->mKey));
          dsStrFree(&(DSSMobile(p_dst)->mLDesc));
          dsStrReplicate(&(DSSMobile(p_dst)->mLDesc),
            &(DSSMobile(p_src)->mLDesc));
          dsStrFree(&(DSSMobile(p_dst)->mDesc));
          dsStrReplicate(&(DSSMobile(p_dst)->mDesc),
            &(DSSMobile(p_src)->mDesc));
          /* don't copy extras or properties */
          DSSMobile(p_dst)->mAct=DSSMobile(p_src)->mAct;
          DSSMobile(p_dst)->mAffect=DSSMobile(p_src)->mAffect;
          DSSMobile(p_dst)->mAura=DSSMobile(p_src)->mAura;
          DSSMobile(p_dst)->mLevel=DSSMobile(p_src)->mLevel;
          DSSMobile(p_dst)->mHitBonus=DSSMobile(p_src)->mHitBonus;
          DSSMobile(p_dst)->mArmor=DSSMobile(p_src)->mArmor;
          DSSMobile(p_dst)->mHPDiceNum=DSSMobile(p_src)->mHPDiceNum;
          DSSMobile(p_dst)->mHPDiceSize=DSSMobile(p_src)->mHPDiceSize;
          DSSMobile(p_dst)->mHPBonus=DSSMobile(p_src)->mHPBonus;
          DSSMobile(p_dst)->mDamDiceNum=DSSMobile(p_src)->mDamDiceNum;
          DSSMobile(p_dst)->mDamDiceSize=DSSMobile(p_src)->mDamDiceSize;
          DSSMobile(p_dst)->mDamBonus=DSSMobile(p_src)->mDamBonus;
          DSSMobile(p_dst)->mMoney=DSSMobile(p_src)->mMoney;
          DSSMobile(p_dst)->mExp=DSSMobile(p_src)->mExp;
          DSSMobile(p_dst)->mPos=DSSMobile(p_src)->mPos;
          DSSMobile(p_dst)->mType=DSSMobile(p_src)->mType;
          DSSMobile(p_dst)->mSex=DSSMobile(p_src)->mSex;
          DSSMobile(p_dst)->mWeight=DSSMobile(p_src)->mWeight;
          break;
        case DS_STYPE_OBJECT:
          dsStrFree(&(DSSObject(p_dst)->oKey));
          dsStrReplicate(&(DSSObject(p_dst)->oKey),
            &(DSSObject(p_src)->oKey));
          dsStrFree(&(DSSObject(p_dst)->oLDesc));
          dsStrReplicate(&(DSSObject(p_dst)->oLDesc),
            &(DSSObject(p_src)->oLDesc));
          dsStrFree(&(DSSObject(p_dst)->oDesc));
          dsStrReplicate(&(DSSObject(p_dst)->oDesc),
            &(DSSObject(p_src)->oDesc));
          /* don't copy extras or properties */
          DSSObject(p_dst)->oType=DSSObject(p_src)->oType;
          DSSObject(p_dst)->oAct=DSSObject(p_src)->oAct;
          DSSObject(p_dst)->oWear=DSSObject(p_src)->oWear;
          DSSObject(p_dst)->oWeight=DSSObject(p_src)->oWeight;
          DSSObject(p_dst)->oValue=DSSObject(p_src)->oValue;
          DSSObject(p_dst)->oRent=DSSObject(p_src)->oRent;
          for (l_i=0;l_i<16;l_i++)
            DSSObject(p_dst)->oDetail[l_i]=DSSObject(p_src)->oDetail[l_i];
          for (l_i=0;l_i<DS_OBJECT_MAX_APPLY;l_i++) {
            DSSObject(p_dst)->oApplyType[l_i]=DSSObject(p_src)->oApplyType[l_i];
            DSSObject(p_dst)->oApplyValue[l_i]=DSSObject(p_src)->oApplyValue[l_i];
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  return;
}

/* change List to sub-list type and back again */
/* Note this'll kill any edit window attached to p_old */
DSSTRUCT *dsChangeListType(DSSTRUCT *p_old, int p_newtype) {
  DSSTRUCT *l_new;

  if (!p_old)
    return NULL;
  if ((p_newtype<DS_STYPE_LIST)||(p_old->sType<DS_STYPE_LIST))
    return p_old;
  if (p_newtype==p_old->sType)
    return p_old;

  l_new=dsStructAlloc(p_newtype);
  if (!l_new)
    return p_old;
  /* swap in the new structure, swap out the old */
  l_new->sPrevious=p_old->sPrevious;
  l_new->sNext=p_old->sNext;
  if (p_old->sPrevious)
    p_old->sPrevious->sNext=l_new;
  if (p_old->sNext)
    p_old->sNext->sPrevious=l_new;
  l_new->sUpRef=p_old->sUpRef;
  if ((p_old->sUpRef)&&(p_old->sUpRef->rList==p_old))
    p_old->sUpRef->rList=l_new;
  p_old->sNext=p_old->sPrevious=NULL;
  p_old->sUpRef=NULL;
  /* yes, here we actually "trade" the lName field to the new list */
  DSSList(l_new)->lName.sData=DSSList(p_old)->lName.sData;
  DSSList(l_new)->lName.sMemory=DSSList(p_old)->lName.sMemory;
  DSSList(p_old)->lName.sData=NULL;
  DSSList(p_old)->lName.sMemory=NULL;
  DSSList(l_new)->lVNum=DSSList(p_old)->lVNum;
  DSSList(l_new)->lVNum2=DSSList(p_old)->lVNum2;
  /* don't forget the other "minor items" */
  l_new->sState=p_old->sState;
  l_new->sLoadTime=p_old->sLoadTime;
  l_new->sEditWindow=p_old->sEditWindow;
  p_old->sEditWindow=NULL;
  /* we're done. Release the sucker & return the new one */
  dsStructFree(p_old);
  return l_new;
}


