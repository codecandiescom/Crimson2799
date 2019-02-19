// Unpack functions
// two character descriptor: un
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

// These functions are mainly an extension of moleprot.c, but I didn't
// want to put them in that already stuffed-full module, so here they are.

#include<windows.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include"moledefs.h"
#include"molem.h"
#include"dstruct.h"
#include"moleprot.h"
#include"areawnd.h"
#include"unpack.h"
#include"debug.h"
#include"edit.h"
#include"editobj.h"

/* This is to process an IDEN packet - at the start of things */
void unNewIden(char *p_data,unsigned long p_datalen) {
  int l_i;
  char l_buf[DS_FTL_NAMELEN];
  DSWORD l_type,l_numType,l_field,l_numField;
  DSWORD l_list;
  DSWORD l_format,l_listnum;
  DSFTL *l_ftl,*l_tmpftl;

  /* Get our MUD name for the Area Window*/
  unGetString(&p_data,&p_datalen,g_awMUDName,50);

  /* now read all our lists */
  unGetInteger(&p_data,&p_datalen,&l_list,UN_GET_WORD);
  for (l_i=0;(l_i<l_list)&&(p_datalen);l_i++) {
    l_ftl=NULL;
    while((p_datalen)&&(*p_data)) {
      unGetString(&p_data,&p_datalen,l_buf,DS_FTL_NAMELEN);
      if (l_ftl)
        dsFTLAppendNext(&l_ftl,dsFTLAlloc(l_buf));
      else
        dsFTLAppendDown(&g_awFTList,l_ftl=dsFTLAlloc(l_buf));
    }
    /* increment past our null string */
    if (p_datalen) {
      p_data++;
      p_datalen--;
    }
  }
  /* read in object types */
  /* Note we may have an object type with NO details. Therefore, for this to
   * work, we have to create an initial "marker" FTL for the start of each list */
  unGetInteger(&p_data,&p_datalen,&l_numType,UN_GET_WORD);
  for (l_type=0;(l_type<l_numType)&&(p_datalen);l_type++) {
    dsFTLAppendDown(&g_awFTLObjType,l_ftl=dsFTLAlloc(NULL));;
    unGetInteger(&p_data,&p_datalen,&l_numField,UN_GET_WORD);
    for (l_field=0;(l_field<l_numField)&&(p_datalen);l_field++) {
      unGetString(&p_data,&p_datalen,l_buf,DS_FTL_NAMELEN);
      dsFTLAppendNext(&l_ftl,l_tmpftl=dsFTLAlloc(l_buf));
      unGetInteger(&p_data,&p_datalen,&l_format,UN_GET_WORD);
      l_tmpftl->fType=l_format;
      switch (l_format) {
        case MOLE_LISTTYPE_TYPE:
        case MOLE_LISTTYPE_FLAG:
          unGetInteger(&p_data,&p_datalen,&l_listnum,UN_GET_WORD);
          l_tmpftl->fList=l_listnum;
          break;
        default:
          l_tmpftl->fList=MOLE_LISTTYPE_INVALID;
          break;
      }
    }
  }

  return;
}

/* this proc builds a new area list */
void unNewAreaList(char *p_data,unsigned long p_datalen) {
  DSSTRUCT *l_struct;
  int l_i;

  /* first, free any existing list. */
  dsRefFree(&g_awRootRef);

  /* oh, but don't forget, we may as well mark our area listing as DS_NS_AVAIL */
  g_awRootRef.rState=DS_NS_AVAIL;
  g_awRootRef.rLoadTime=time(NULL);

  /* next, let's build a new one! */
  while (p_datalen>0) {
    l_struct=dsStructAlloc(DS_STYPE_AREA);
    if (!l_struct) return;
    /* save the area name */
    l_i=strlen(p_data)+1;
    dsStrAlloc(&(DSSList(l_struct)->lName),l_i);
    strcpy(DSSList(l_struct)->lName.sData,p_data);
    p_data+=l_i;
    p_datalen-=l_i;
    if (p_datalen >= 4 ) {
      DSSList(l_struct)->lVNum=mpDecodeULWORD(p_data);
    } else {
      DSSList(l_struct)->lVNum=0L;
      DSSList(l_struct)->lVNum2=0L;
      dsStructInsertList(&g_awRootRef,l_struct);
      return;
    }
    p_data+=4;
    p_datalen-=4;
    if (p_datalen >= 4 ) {
      DSSList(l_struct)->lVNum2=mpDecodeULWORD(p_data);
    } else {
      DSSList(l_struct)->lVNum2=0L;
      dsStructInsertList(&g_awRootRef,l_struct);
      return;
    }
    p_data+=4;
    p_datalen-=4;
    dsStructInsertList(&g_awRootRef,l_struct);
  } /* while */
  return;
}

void unNewWorldList(char *p_data,unsigned long p_datalen) {
  DSSTRUCT *l_thing,*l_area;
  DSLWORD l_areaVNum;
//  int l_i;

  if (g_awRootRef.rState!=DS_NS_AVAIL)
    return; /* we can't add anything if we don't have an area list! */

  /* find area */

  unGetInteger(&p_data,&p_datalen,&l_areaVNum,UN_GET_LWORD);
  l_area=edAreaOf(l_areaVNum);
  if (!l_area)
    return;

  /* free any existing list. */
  dsRefFree(&(DSSArea(l_area)->aWLD));

  /* oh, but don't forget, we hafta mark our area listing as DS_NS_AVAIL */
  DSSArea(l_area)->aWLD.rState=DS_NS_AVAIL;
  DSSArea(l_area)->aWLD.rLoadTime=time(NULL);

  /* next, let's build our list */
  while (p_datalen>0) {
    l_thing=dsStructAlloc(DS_STYPE_LIST);
    if (!l_thing) return;
    /* save the area name */
    unGetStrAlloc(&p_data,&p_datalen,&(DSSList(l_thing)->lName));
//    l_i=strlen(p_data)+1;
//    dsStrAlloc(&(DSSList(l_thing)->lName),l_i);
//    strcpy(DSSList(l_thing)->lName.sData,p_data);
//    p_data+=l_i;
//    p_datalen-=l_i;
    unGetInteger(&p_data,&p_datalen,&(DSSList(l_thing)->lVNum),UN_GET_LWORD);
//    if (p_datalen >= 4 ) {
//      DSSList(l_thing)->lVNum=mpDecodeULWORD(p_data);
//    } else {
//      DSSList(l_thing)->lVNum=0L;
//      dsStructInsertList(&(DSSArea(l_area)->aWLD),l_thing);
//      return;
//    }
//    p_data+=4;
//    p_datalen-=4;
    dsStructInsertList(&(DSSArea(l_area)->aWLD),l_thing);
  } /* while */
  return;
}

void unNewMobileList(char *p_data,unsigned long p_datalen) {
  DSSTRUCT *l_thing,*l_area;
  DSLWORD l_areaVNum;
  int l_i;

  if (g_awRootRef.rState!=DS_NS_AVAIL)
    return; /* we can't add anything if we don't have an area list! */

  /* find area */
  unGetInteger(&p_data,&p_datalen,&l_areaVNum,UN_GET_LWORD);
  l_area=edAreaOf(l_areaVNum);
  if (!l_area)
    return;

  /* free any existing list. */
  dsRefFree(&(DSSArea(l_area)->aMOB));

  /* oh, but don't forget, we hafta mark our area listing as DS_NS_AVAIL */
  DSSArea(l_area)->aMOB.rState=DS_NS_AVAIL;
  DSSArea(l_area)->aMOB.rLoadTime=time(NULL);

  /* next, let's build our list */
  while (p_datalen>0) {
    l_thing=dsStructAlloc(DS_STYPE_LIST);
    if (!l_thing) return;
    /* save the area name */
    l_i=strlen(p_data)+1;
    dsStrAlloc(&(DSSList(l_thing)->lName),l_i);
    strcpy(DSSList(l_thing)->lName.sData,p_data);
    p_data+=l_i;
    p_datalen-=l_i;
    if (p_datalen >= 4 ) {
      DSSList(l_thing)->lVNum=mpDecodeULWORD(p_data);
    } else {
      DSSList(l_thing)->lVNum=0L;
      dsStructInsertList(&(DSSArea(l_area)->aMOB),l_thing);
      return;
    }
    p_data+=4;
    p_datalen-=4;
    dsStructInsertList(&(DSSArea(l_area)->aMOB),l_thing);
  } /* while */
  return;
}

void unNewObjectList(char *p_data,unsigned long p_datalen) {
  DSSTRUCT *l_thing,*l_area;
  DSLWORD l_areaVNum;
  int l_i;

  if (g_awRootRef.rState!=DS_NS_AVAIL)
    return; /* we can't add anything if we don't have an area list! */

  /* find area */
  unGetInteger(&p_data,&p_datalen,&l_areaVNum,UN_GET_LWORD);
  l_area=edAreaOf(l_areaVNum);
  if (!l_area)
    return;

  /* free any existing list. */
  dsRefFree(&(DSSArea(l_area)->aOBJ));

  /* oh, but don't forget, we hafta mark our area listing as DS_NS_AVAIL */
  DSSArea(l_area)->aOBJ.rState=DS_NS_AVAIL;
  DSSArea(l_area)->aOBJ.rLoadTime=time(NULL);

  /* next, let's build our list */
  while (p_datalen>0) {
    l_thing=dsStructAlloc(DS_STYPE_LIST);
    if (!l_thing) return;
    /* save the area name */
    l_i=strlen(p_data)+1;
    dsStrAlloc(&(DSSList(l_thing)->lName),l_i);
    strcpy(DSSList(l_thing)->lName.sData,p_data);
    p_data+=l_i;
    p_datalen-=l_i;
    if (p_datalen >= 4 ) {
      DSSList(l_thing)->lVNum=mpDecodeULWORD(p_data);
    } else {
      DSSList(l_thing)->lVNum=0L;
      dsStructInsertList(&(DSSArea(l_area)->aOBJ),l_thing);
      return;
    }
    p_data+=4;
    p_datalen-=4;
    dsStructInsertList(&(DSSArea(l_area)->aOBJ),l_thing);
  } /* while */
  return;
}

void unNewWorld(char *p_data,unsigned long p_datalen) {
  long l_VNum;
  DSLWORD l_i;
  DSSTRUCT *l_thing,*l_new,*l_area;
//  DSSWORLD *l_world;

  /* Extract virtual number */
  unGetInteger(&p_data,&p_datalen,&l_VNum,UN_GET_LWORD);

  /* find thing */
  l_thing=edWorldOf(l_VNum);
  if (!l_thing) {
    /* create thing */
//    (DSSTRUCT*)l_world=l_thing=dsStructAlloc(DS_STYPE_WORLD);
    l_thing=dsStructAlloc(DS_STYPE_WORLD);
    if (l_thing) {
      DSSList(l_thing)->lVNum=l_VNum;
      /* find the appropriate area */
      l_area=edAreaOf(l_VNum);
      if (l_area) {
        dsStructInsertList(&(DSSArea(l_area)->aWLD),l_thing);
      } else {
        dsStructFree(l_thing);
      }
    }
  } else {
    /* now create proper structure for thing */
//    (DSSTRUCT*)l_world=l_thing=dsChangeListType(l_thing,DS_STYPE_WORLD);
    l_thing=dsChangeListType(l_thing,DS_STYPE_WORLD);
  }
  if (!l_thing)
    return;

  /* mise well mark as being DS_STATE_AVAIL */
  l_thing->sState=DS_NS_AVAIL;
  l_thing->sLoadTime=time(NULL);

  /* and finally, read data into new structure from packet */
  unGetStrAlloc(&p_data,&p_datalen,&(DSSList(l_thing)->lName));
  unGetStrAlloc(&p_data,&p_datalen,&(DSSWorld(l_thing)->wDesc));
  unGetInteger(&p_data,&p_datalen,&(DSSWorld(l_thing)->wType),UN_GET_FLAG);
  unGetInteger(&p_data,&p_datalen,&(DSSWorld(l_thing)->wFlag),UN_GET_FLAG);

  /* input exits */
  unGetInteger(&p_data,&p_datalen,&l_i,UN_GET_LWORD);
  while(l_i) {
    /* create EXIT */
    l_new=dsStructAlloc(DS_STYPE_EXIT);
    if (l_new) {
      unGetInteger(&p_data,&p_datalen,&(DSSExit(l_new)->eDir),UN_GET_BYTE);
      unGetStrAlloc(&p_data,&p_datalen,&(DSSExit(l_new)->eKey));
      unGetStrAlloc(&p_data,&p_datalen,&(DSSExit(l_new)->eDesc));
      unGetInteger(&p_data,&p_datalen,&(DSSExit(l_new)->eKeyObj),UN_GET_LWORD);
      unGetInteger(&p_data,&p_datalen,&(DSSExit(l_new)->eFlag),UN_GET_FLAG);
      unGetInteger(&p_data,&p_datalen,&(DSSExit(l_new)->eWorld),UN_GET_LWORD);
      /* and insert into extra list - append it to the start of the list
       * (order is irrelevent to us) */
      dsStructInsert(&(DSSWorld(l_thing)->wExit), NULL, l_new);
    }
    l_i--;
  }

  /* now input extras */
  /* first we need to read in how many extras there are. */
  unGetInteger(&p_data,&p_datalen,&l_i,UN_GET_LWORD);
  while(l_i) {
    /* create EXTRA */
    l_new=dsStructAlloc(DS_STYPE_EXTRA);
    if (l_new) {
      unGetStrAlloc(&p_data,&p_datalen,&(DSSExtra(l_new)->eKey));
      unGetStrAlloc(&p_data,&p_datalen,&(DSSExtra(l_new)->eDesc));
      /* and insert into extra list - append it to the start of the list
       * (order is irrelevent to us) */
      dsStructInsert(&(DSSWorld(l_thing)->wExtra), NULL, l_new);
    }
    l_i--;
  }

  /* and now input properties */
  unGetInteger(&p_data,&p_datalen,&l_i,UN_GET_LWORD);
  while((l_i)&&(p_datalen>0)) {
    /* create PROPERTY */
    l_new=dsStructAlloc(DS_STYPE_PROPERTY);
    if (l_new) {
      unGetStrAlloc(&p_data,&p_datalen,&(DSSProperty(l_new)->pKey));
      unGetStrAlloc(&p_data,&p_datalen,&(DSSProperty(l_new)->pDesc));
      /* and insert into property list - append it to the start of the list
       * (order is irrelevent to us) */
      dsStructInsert(&(DSSWorld(l_thing)->wProperty), NULL, l_new);
    }
  }

  /* oooop, don't forget to notify any edit window we may have! */
  if (IsWindow(l_thing->sEditWindow))
    PostMessage(l_thing->sEditWindow,WM_USER_NEWDATA,0,0L);
  return;
}

void unNewMobile(char *p_data,unsigned long p_datalen) {
  long l_VNum;
  DSLWORD l_i;
  DSSTRUCT *l_thing,*l_new,*l_area;
//  DSSMOBILE *l_mob;

  /* Extract virtual number */
  unGetInteger(&p_data,&p_datalen,&l_VNum,UN_GET_ULWORD);

  /* find thing */
  l_thing=edMobileOf(l_VNum);
  if (!l_thing) {
    /* create thing */
//    (DSSTRUCT*)l_mob=l_thing=dsStructAlloc(DS_STYPE_MOBILE);
    l_thing=dsStructAlloc(DS_STYPE_MOBILE);
    if (l_thing) {
      DSSList(l_thing)->lVNum=l_VNum;
      /* find the appropriate area */
      l_area=edAreaOf(l_VNum);
      if (l_area) {
        dsStructInsertList(&(DSSArea(l_area)->aMOB),l_thing);
      } else {
        dsStructFree(l_thing);
      }
    }
  } else {
    /* now create proper structure for thing */
//    (DSSTRUCT*)l_mob=l_thing=dsChangeListType(l_thing,DS_STYPE_MOBILE);
    l_thing=dsChangeListType(l_thing,DS_STYPE_MOBILE);
  }
  if (!l_thing)
    return;

  /* mise well mark as being DS_STATE_AVAIL */
  l_thing->sState=DS_NS_AVAIL;
  l_thing->sLoadTime=time(NULL);

  /* and finally, read data into new structure from packet */
  unGetStrAlloc(&p_data,&p_datalen,&(DSSMobile(l_thing)->mKey));
  unGetStrAlloc(&p_data,&p_datalen,&(DSSList(l_thing)->lName));
  unGetStrAlloc(&p_data,&p_datalen,&(DSSMobile(l_thing)->mLDesc));
  unGetStrAlloc(&p_data,&p_datalen,&(DSSMobile(l_thing)->mDesc));
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mAct),UN_GET_FLAG);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mAffect),UN_GET_FLAG);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mAura),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mLevel),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mHitBonus),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mArmor),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mHPDiceNum),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mHPDiceSize),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mHPBonus),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mDamDiceNum),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mDamDiceSize),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mDamBonus),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mMoney),UN_GET_LWORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mExp),UN_GET_LWORD);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mPos),UN_GET_SBYTE);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mType),UN_GET_SBYTE);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mSex),UN_GET_SBYTE);
  unGetInteger(&p_data,&p_datalen,&(DSSMobile(l_thing)->mWeight),UN_GET_WORD);

  /* now input extras */
  /* first we need to read in how many extras there are. */
  unGetInteger(&p_data,&p_datalen,&l_i,UN_GET_LWORD);
  while(l_i) {
    /* create EXTRA */
    l_new=dsStructAlloc(DS_STYPE_EXTRA);
    if (l_new) {
      unGetStrAlloc(&p_data,&p_datalen,&(DSSExtra(l_new)->eKey));
      unGetStrAlloc(&p_data,&p_datalen,&(DSSExtra(l_new)->eDesc));
      /* and insert into extra list - append it to the start of the list
       * (order is irrelevent to us) */
      dsStructInsert(&(DSSMobile(l_thing)->mExtra), NULL, l_new);
    }
    l_i--;
  }

  /* and now input properties */
  unGetInteger(&p_data,&p_datalen,&l_i,UN_GET_LWORD);
  while((l_i)&&(p_datalen>0)) {
    /* create PROPERTY */
    l_new=dsStructAlloc(DS_STYPE_PROPERTY);
    if (l_new) {
      unGetStrAlloc(&p_data,&p_datalen,&(DSSProperty(l_new)->pKey));
      unGetStrAlloc(&p_data,&p_datalen,&(DSSProperty(l_new)->pDesc));
      /* and insert into property list - append it to the start of the list
       * (order is irrelevent to us) */
      dsStructInsert(&(DSSMobile(l_thing)->mProperty), NULL, l_new);
    }
  }

  /* oooop, don't forget to notify any edit window we may have! */
  if (IsWindow(l_thing->sEditWindow))
    PostMessage(l_thing->sEditWindow,WM_USER_NEWDATA,0,0L);
  return;
}

void unNewObject(char *p_data,unsigned long p_datalen) {
  long l_VNum;
  DSLWORD l_i,l_j;
  DSSTRUCT *l_thing,*l_new,*l_area;
  DSBYTE l_junk;
  DSFTL *l_ftl;
//  DSSOBJECT *l_object;

  /* Extract virtual number */
  unGetInteger(&p_data,&p_datalen,&l_VNum,UN_GET_ULWORD);

  /* find thing */
  l_thing=edObjectOf(l_VNum);
  if (!l_thing) {
    /* create thing */
//    (DSSTRUCT*)l_object=l_thing=dsStructAlloc(DS_STYPE_OBJECT);
    l_thing=dsStructAlloc(DS_STYPE_OBJECT);
    if (l_thing) {
      DSSList(l_thing)->lVNum=l_VNum;
      /* find the appropriate area */
      l_area=edAreaOf(l_VNum);
      if (l_area) {
        dsStructInsertList(&(DSSArea(l_area)->aOBJ),l_thing);
      } else {
        dsStructFree(l_thing);
      }
    }
  } else {
    /* now create proper structure for thing */
//    (DSSTRUCT*)l_object=l_thing=dsChangeListType(l_thing,DS_STYPE_OBJECT);
    l_thing=dsChangeListType(l_thing,DS_STYPE_OBJECT);
  }
  if (!l_thing)
    return;

  /* mise well mark as being DS_STATE_AVAIL */
  l_thing->sState=DS_NS_AVAIL;
  l_thing->sLoadTime=time(NULL);

  /* and finally, read data into new structure from packet */
  unGetStrAlloc(&p_data,&p_datalen,&(DSSObject(l_thing)->oKey));
  unGetStrAlloc(&p_data,&p_datalen,&(DSSList(l_thing)->lName));
  unGetStrAlloc(&p_data,&p_datalen,&(DSSObject(l_thing)->oLDesc));
  unGetStrAlloc(&p_data,&p_datalen,&(DSSObject(l_thing)->oDesc));
  unGetInteger(&p_data,&p_datalen,&(DSSObject(l_thing)->oType),UN_GET_BYTE);
  unGetInteger(&p_data,&p_datalen,&(DSSObject(l_thing)->oAct),UN_GET_FLAG);
  unGetInteger(&p_data,&p_datalen,&(DSSObject(l_thing)->oWear),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSObject(l_thing)->oWeight),UN_GET_WORD);
  unGetInteger(&p_data,&p_datalen,&(DSSObject(l_thing)->oValue),UN_GET_LWORD);
  unGetInteger(&p_data,&p_datalen,&(DSSObject(l_thing)->oRent),UN_GET_LWORD);

  /* details */
  l_ftl=dsFTLOf(&g_awFTLObjType,DSSObject(l_thing)->oType);
  l_i=0;
  /* skip past initial (blank) FTL */
  if (l_ftl)
    l_ftl=l_ftl->fNext;
  while(l_ftl) {
    unGetInteger(&p_data,&p_datalen,&(DSSObject(l_thing)->oDetail[(int)l_i]),UN_GET_LWORD);
    l_i++;
    l_ftl=l_ftl->fNext;
  }

  /* apply list */
  unGetInteger(&p_data,&p_datalen,&(l_i),UN_GET_WORD);
  for (l_j=0;(l_j<l_i);l_j++) {
    if (l_j<DS_OBJECT_MAX_APPLY) {
      unGetInteger(&p_data,&p_datalen,&((DSSObject(l_thing)->oApplyType)[(unsigned int)l_j]),UN_GET_BYTE);
      unGetInteger(&p_data,&p_datalen,&((DSSObject(l_thing)->oApplyValue)[(unsigned int)l_j]),UN_GET_SBYTE);
    } else {
      unGetInteger(&p_data,&p_datalen,&(l_junk),UN_GET_BYTE);
      unGetInteger(&p_data,&p_datalen,&(l_junk),UN_GET_SBYTE);
    }
  }

  /* now input extras */
  /* first we need to read in how many extras there are. */
  unGetInteger(&p_data,&p_datalen,&l_i,UN_GET_LWORD);
  while(l_i) {
    /* create EXTRA */
    l_new=dsStructAlloc(DS_STYPE_EXTRA);
    if (l_new) {
      unGetStrAlloc(&p_data,&p_datalen,&(DSSExtra(l_new)->eKey));
      unGetStrAlloc(&p_data,&p_datalen,&(DSSExtra(l_new)->eDesc));
      /* and insert into extra list - append it to the start of the list
       * (order is irrelevent to us) */
      dsStructInsert(&(DSSObject(l_thing)->oExtra), NULL, l_new);
    }
    l_i--;
  }

  /* and now input properties */
  unGetInteger(&p_data,&p_datalen,&l_i,UN_GET_LWORD);
  while((l_i)&&(p_datalen>0)) {
    /* create PROPERTY */
    l_new=dsStructAlloc(DS_STYPE_PROPERTY);
    if (l_new) {
      unGetStrAlloc(&p_data,&p_datalen,&(DSSProperty(l_new)->pKey));
      unGetStrAlloc(&p_data,&p_datalen,&(DSSProperty(l_new)->pDesc));
      /* and insert into property list - append it to the start of the list
       * (order is irrelevent to us) */
      dsStructInsert(&(DSSObject(l_thing)->oProperty), NULL, l_new);
    }
  }

  /* oooop, don't forget to notify any edit window we may have! */
  if (IsWindow(l_thing->sEditWindow))
    PostMessage(l_thing->sEditWindow,WM_USER_NEWDATA,0,0L);
  return;
}

void unNewAreaDetail(char *p_data,unsigned long p_datalen) {
  long l_VNum;
  DSSTRUCT *l_thing,*l_new,*l_area;
  DSLWORD l_i;

  /* Extract virtual number */
  unGetInteger(&p_data,&p_datalen,&l_VNum,UN_GET_ULWORD);

  /* find thing */
  l_area=edAreaOf(l_VNum);
  if (!l_area)
    return;

  /* re-use any existing area detail struct */
  l_thing=DSSArea(l_area)->aDetail.rList;

  if (!l_thing) {
    /* create a new detail struct */
    l_thing=dsStructAlloc(DS_STYPE_AREADETAIL);
    dsStructInsert(&(DSSArea(l_area)->aDetail), NULL, l_thing);
  }
  if (!l_thing)
    return;

  /* mise well mark as being DS_STATE_AVAIL */
  DSSArea(l_area)->aDetail.rState=DS_NS_AVAIL;
  l_thing->sState=DS_NS_AVAIL;
  DSSArea(l_area)->aDetail.rLoadTime=l_thing->sLoadTime=time(NULL);

  /* And finally, read data into structure */
  unGetStrAlloc(&p_data,&p_datalen,&(DSSAreaDetail(l_thing)->aEditor));
  unGetStrAlloc(&p_data,&p_datalen,&(DSSAreaDetail(l_thing)->aDesc));
  unGetInteger(&p_data,&p_datalen,&(DSSAreaDetail(l_thing)->aResetFlag),UN_GET_FLAG);
  unGetInteger(&p_data,&p_datalen,&(DSSAreaDetail(l_thing)->aResetDelay),UN_GET_WORD);

  /* and now input properties */
  unGetInteger(&p_data,&p_datalen,&l_i,UN_GET_LWORD);
  while((l_i)&&(p_datalen>0)) {
    /* create PROPERTY */
    l_new=dsStructAlloc(DS_STYPE_PROPERTY);
    if (l_new) {
      unGetStrAlloc(&p_data,&p_datalen,&(DSSProperty(l_new)->pKey));
      unGetStrAlloc(&p_data,&p_datalen,&(DSSProperty(l_new)->pDesc));
      /* and insert into property list - append it to the start of the list
       * (order is irrelevent to us) */
      dsStructInsert(&(DSSAreaDetail(l_thing)->aProperty), NULL, l_new);
    }
  }

  /* oooop, don't forget to notify any edit window we may have! */
  if (IsWindow(l_thing->sEditWindow))
    PostMessage(l_thing->sEditWindow,WM_USER_NEWDATA,0,0L);
  return;
}

void unNewResetList(char *p_data,unsigned long p_datalen) {
  DSSTRUCT *l_thing,*l_area,*l_lastThing;
  DSLWORD l_areaVNum;
  DSLWORD l_resetEntries;
  int l_i;
  HWND l_wnd;

  if (g_awRootRef.rState!=DS_NS_AVAIL)
    return; /* we can't add anything if we don't have an area list! */

  /* find area */
  unGetInteger(&p_data,&p_datalen,&l_areaVNum,UN_GET_LWORD);
  l_area=edAreaOf(l_areaVNum);
  if (!l_area)
    return;

  /* free any existing list. */
  /* but don't clobber the edit window */
  l_wnd=DSSArea(l_area)->aRST.rEditWindow;
  DSSArea(l_area)->aRST.rEditWindow=NULL;
  dsRefFree(&(DSSArea(l_area)->aRST));
  DSSArea(l_area)->aRST.rEditWindow=l_wnd;

  /* oh, but don't forget, we hafta mark our area listing as DS_NS_AVAIL */
  DSSArea(l_area)->aRST.rState=DS_NS_AVAIL;
  DSSArea(l_area)->aRST.rLoadTime=time(NULL);

  /* let's get our reset entries count */
  unGetInteger(&p_data,&p_datalen,&l_resetEntries,UN_GET_LWORD);

  /* next, let's build our list */
  l_lastThing=NULL;
  for (l_i=0;(l_i<l_resetEntries)&&(p_datalen>0);l_i++) {
    l_thing=dsStructAlloc(DS_STYPE_RESET);
    if (!l_thing) return;
    unGetInteger(&p_data,&p_datalen,&(DSSReset(l_thing)->rCmd),UN_GET_BYTE);
    unGetInteger(&p_data,&p_datalen,&(DSSReset(l_thing)->rIf),UN_GET_WORD);
    unGetInteger(&p_data,&p_datalen,&(DSSReset(l_thing)->rArg1),UN_GET_LWORD);
    unGetInteger(&p_data,&p_datalen,&(DSSReset(l_thing)->rArg2),UN_GET_LWORD);
    unGetInteger(&p_data,&p_datalen,&(DSSReset(l_thing)->rArg3),UN_GET_LWORD);
    unGetInteger(&p_data,&p_datalen,&(DSSReset(l_thing)->rArg4),UN_GET_LWORD);
    /* note we do the following trick to preserve the order of our reset entries. */
    if (l_lastThing)
      dsStructInsert(NULL, l_lastThing, l_thing);
    else
      dsStructInsert(&(DSSArea(l_area)->aRST), NULL, l_thing);
    l_lastThing=l_thing;
  }
  /* oooop, don't forget to notify any edit window we may have! */
  if (IsWindow(l_thing->sEditWindow))
    PostMessage(l_thing->sEditWindow,WM_USER_NEWDATA,0,0L);
  return;
}

/* creates STR & copies from buffer into str */
int unGetStrAlloc(char **p_data,unsigned long *p_datalen,DSSTR *p_str) {
  unsigned long l_size,l_i;
  if ((!p_data)||(!*p_data)||(!p_str))
    return UN_GET_BADINPUT;

  /* first, make sure the EOS is within our data packet, count \r's, etc. */
  for (l_size=l_i=0;l_i<(*p_datalen);l_i++) {
    if (!((*p_data)[(unsigned int)l_i]))
      break;
    if ((*p_data)[(unsigned int)l_i]=='\n')
      l_size++;
    l_size++;
  }
  if ((*p_data)[(unsigned int)l_i]) {
    return UN_GET_NODATA;
  }
  l_size++; /* to include trailing 0x00 */

  if (dsStrAlloc(p_str,l_size)<l_size) {
    dsStrFree(p_str);
    return UN_GET_SYSTEM;
  }

  return unGetString(p_data,p_datalen,p_str->sData,GlobalSize(p_str->sMemory));
}


int unGetString(char **p_data,unsigned long *p_datalen,char *p_dest,unsigned long p_destlen) {

  if (!p_destlen)
    return UN_GET_BADINPUT;
  p_destlen--; /* because we need to be able to append a 0x00 */
  if ((!p_data)||(!(*p_data))||(!p_datalen)||(!p_dest))
    return UN_GET_BADINPUT;
  while((**p_data)&&(p_destlen)&&(*p_datalen)) {
    if ((**p_data)=='\n') { /* convert \n to \n\r */
      *p_dest='\r';
      p_dest++;
      p_destlen--;
    }
    *p_dest=**p_data;
    p_dest++;
    p_destlen--;
    (*p_data)++;
    (*p_datalen)--;
  }
  *p_dest=0;
  if (**p_data) { /* error */
    if (!(*p_datalen))
      return UN_GET_NODATA;
    else {
      /* advance to the end of the string anyways */
      while((**p_data)&&(*p_datalen)) {
        (*p_data)++;
        (*p_datalen)--;
      }
      return UN_GET_STRLEN;
    }
  }
  (*p_data)++;
  (*p_datalen)--;
  return UN_GET_OK;
}


int unGetInteger(char **p_data,unsigned long *p_datalen,void *p_dest,int p_destType) {
  if ((!p_data)||(!(*p_data))||(!p_datalen)||(!p_dest))
    return UN_GET_BADINPUT;

  if ((*p_datalen) >= 4 ) {
    if (p_destType==UN_GET_BYTE)
      *((DSBYTE *)p_dest)=(DSBYTE)mpDecodeULWORD(*p_data);
    else if (p_destType==UN_GET_SBYTE)
      *((DSSBYTE *)p_dest)=(DSSBYTE)mpDecodeULWORD(*p_data);
    else if (p_destType==UN_GET_WORD)
      *((DSWORD *)p_dest)=(DSWORD)mpDecodeULWORD(*p_data);
    else if (p_destType==UN_GET_UWORD)
      *((DSUWORD *)p_dest)=(DSUWORD)mpDecodeULWORD(*p_data);
    else if (p_destType==UN_GET_LWORD)
      *((DSLWORD *)p_dest)=(DSLWORD)mpDecodeULWORD(*p_data);
    else if (p_destType==UN_GET_ULWORD)
      *((DSULWORD *)p_dest)=(DSULWORD)mpDecodeULWORD(*p_data);
    else if (p_destType==UN_GET_FLAG)
      *((DSFLAG *)p_dest)=(DSFLAG)mpDecodeULWORD(*p_data);
    (*p_data)+=4;
    (*p_datalen)-=4;
  } else {
    return UN_GET_NODATA;
  }
  return UN_GET_OK;
}


