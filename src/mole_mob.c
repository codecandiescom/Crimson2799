/* part of Crimson2 */
/* Written by B. Cameron Lesiuk, 1997 */
/* Written for use with Crimson2 MUD (written/copyright Ryan Haksi 1995).
 * This source is proprietary. Use of this code without permission from 
 * Ryan Haksi or Cam Lesiuk is strictly prohibited. 
 * 
 * (clesiuk@engr.uvic.ca)
 */

/* MOLE related commands */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
  #include <unistd.h>
#endif

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "str.h"
#include "queue.h"
#include "send.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "file.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "mem.h"
#include "socket.h"
#include "site.h"
#include "exit.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "affect.h"
#include "fight.h"
#include "mobile.h"
#include "skill.h"
#include "player.h"
#include "parse.h"
#include "area.h"
#include "code.h"
#include "moledefs.h"
#include "mole.h"
#include "mole_msc.h"

CMDMOLEPROC(CmdMOLEmlrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE    buf[MOLE_PKT_MAX];
  LWORD   i=0;
  WORD    area;
  LWORD   k;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"mlist",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the specified area */
  area=AreaOf(virtual);
  if (area<0) {
    /* didn't find specified area - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  } else { /* we found the area - now send the list */
    MOLEWriteULWORD(sock,virtual,buf,&i);
    for (k=0;k<areaList[area].aMobIndex.iNum;k++) {
      MOLEWriteBuf(sock,MobTemplate(areaList[area].aMobIndex.iThing[k])->mSDesc->sText,buf,&i);
      MOLEWriteULWORD(sock,MobTemplate(areaList[area].aMobIndex.iThing[k])->mVirtual,buf,&i);
    }
    MOLESend(sock,buf,i,MOLE_CMD_MLST);
  }
}

CMDMOLEPROC(CmdMOLEmdrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE         buf[MOLE_PKT_MAX];
  LWORD        i=0;
  MOBTEMPLATE *mobile;
  EXTRA       *extra;
  PROPERTY    *prop;
  ULWORD       k;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"mstat",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the mobile */
  mobile=MobileOf(virtual);
  if (mobile) {
    /* we found it - provide data */
    MOLEWriteULWORD(sock,mobile->mVirtual,buf,&i);
    MOLEWriteBuf(sock,mobile->mKey->sText,buf,&i);
    MOLEWriteBuf(sock,mobile->mSDesc->sText,buf,&i);
    MOLEWriteBuf(sock,mobile->mLDesc->sText,buf,&i);
    MOLEWriteBuf(sock,mobile->mDesc->sText,buf,&i);
    MOLEWriteULWORD(sock,mobile->mAct,buf,&i);
    MOLEWriteULWORD(sock,mobile->mAffect,buf,&i);
    MOLEWriteULWORD(sock,mobile->mAura,buf,&i);
    MOLEWriteULWORD(sock,mobile->mLevel,buf,&i);
    MOLEWriteULWORD(sock,mobile->mHitBonus,buf,&i);
    MOLEWriteULWORD(sock,mobile->mArmor,buf,&i);
    MOLEWriteULWORD(sock,mobile->mHPDiceNum,buf,&i);
    MOLEWriteULWORD(sock,mobile->mHPDiceSize,buf,&i);
    MOLEWriteULWORD(sock,mobile->mHPBonus,buf,&i);
    MOLEWriteULWORD(sock,mobile->mDamDiceNum,buf,&i);
    MOLEWriteULWORD(sock,mobile->mDamDiceSize,buf,&i);
    MOLEWriteULWORD(sock,mobile->mDamBonus,buf,&i);
    MOLEWriteULWORD(sock,mobile->mMoney,buf,&i);
    MOLEWriteULWORD(sock,mobile->mExp,buf,&i);
    MOLEWriteULWORD(sock,mobile->mPos,buf,&i);
    MOLEWriteULWORD(sock,mobile->mType,buf,&i);
    MOLEWriteULWORD(sock,mobile->mSex,buf,&i);
    MOLEWriteULWORD(sock,mobile->mWeight,buf,&i);
    /* send extra list */
    /* first, count the number of extras */
    k=0;
    for (extra=mobile->mExtra;extra;extra=extra->eNext) {
      k++;
    }
    MOLEWriteULWORD(sock,k,buf,&i);
    /* and now send the extras. */
    for (extra=mobile->mExtra;extra;extra=extra->eNext) {
      MOLEWriteBuf(sock,extra->eKey->sText,buf,&i);
      MOLEWriteBuf(sock,extra->eDesc->sText,buf,&i);
    }
    /* send property list */
    /* You'll note we need to worry about 
     * DECOMPILING properties if they're code 
     * properties, prior to transmission! Whoops! */
    k=0;
    for (prop=mobile->mProperty;prop;prop=prop->pNext) {
      k++;
    }
    MOLEWriteULWORD(sock,k,buf,&i);
    for (prop=mobile->mProperty;prop;prop=prop->pNext) {
      /* note this CodeDecompProperty will check 
       * if it's compiled or not, so we don't 
       * have to pre-check before calling it. */
      CodeDecompProperty(prop,NULL);
      mobile->mCompile=1; /* turn on compile on demand */
      MOLEWriteBuf(sock,prop->pKey->sText,buf,&i);
      MOLEWriteBuf(sock,prop->pDesc->sText,buf,&i);
    }
    /* and send the sucker */
    MOLESend(sock,buf,i,MOLE_CMD_MDTL);
  } else {
    /* didn't find specified mobile - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
}

CMDMOLEPROC(CmdMOLEmdtl) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE         buf[MOLE_PKT_MAX];
  LWORD        i=0;
  MOBTEMPLATE *mobile;
  LWORD        rc;      /* return code */
  ULWORD       data;
  STR         *strData;
  STR         *strKey;
  STR         *strDesc;
  LWORD        counter;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"mdesc",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the mobile */
  mobile=MobileOf(virtual);
  if (mobile) {
    /* first, mark area as changed */
    BITSET(areaList[AreaOf(virtual)].aSystem,AS_MOBUNSAVED);
    mobile->mCompile=1; /* turn on compile on demand */
    /* Ok, let's read in our new MOB stats */
    if ((!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(mobile->mKey);
      mobile->mKey=strData;
    }
    if ((!rc)&&(!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(mobile->mSDesc);
      mobile->mSDesc=strData;
    }
    if ((!rc)&&(!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(mobile->mLDesc);
      mobile->mLDesc=strData;
    }
    if ((!rc)&&(!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(mobile->mDesc);
      mobile->mDesc=strData;
    }
    /* note I do the following with all ULWORDs because
     * then I don't actually care what the specific size
     * or sign of the numerical value is... the compiler
     * will sort it all out and do the appropriate conversion. */
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mAct=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mAffect=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mAura=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mLevel=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mHitBonus=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mArmor=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mHPDiceNum=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mHPDiceSize=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mHPBonus=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mDamDiceNum=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mDamDiceSize=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mDamBonus=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mMoney=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mExp=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mPos=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mType=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mSex=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      mobile->mWeight=data;
    /* get extra list */
    counter=0;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      counter=data;                /* counter=#extras */
    /* extra list - first we need to delete our old ones */
    if (!rc)
      while(mobile->mExtra)
        mobile->mExtra=ExtraFree(mobile->mExtra,mobile->mExtra);
    while((counter>0)&&(!rc)) {
      strKey=strDesc=NULL;
      rc=MOLEGetQStr(sock,&strKey);
      if (!rc)
        rc=MOLEGetQStr(sock,&strDesc);
      if (!rc) {
        mobile->mExtra=ExtraAlloc(mobile->mExtra,strKey,strDesc);
      } else {
        if (strKey) StrFree(strKey);
        if (strDesc) StrFree(strDesc);
      }
      counter--;
    }
    /* get property list */
    counter=0;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      counter=data;                /* counter=#properties */
    /* property list - first we need to delete our old ones */
    if (!rc)
      while(mobile->mProperty)
        mobile->mProperty=PropertyFree(mobile->mProperty,mobile->mProperty);
    while((counter>0)&&(!rc)) {
      strKey=strDesc=NULL;
      rc=MOLEGetQStr(sock,&strKey);
      if (!rc)
        rc=MOLEGetQStr(sock,&strDesc);
      if (!rc) {
        mobile->mProperty=PropertyCreate(mobile->mProperty,strKey,strDesc);
      } else {
        if (strKey) StrFree(strKey);
        if (strDesc) StrFree(strDesc);
      }
      counter--;
    }
    MOLESend(sock,buf,i,MOLE_CMD_ACKP);
  } else {
    /* didn't find specified mobile - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
  return;
}
