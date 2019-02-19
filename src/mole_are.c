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

CMDMOLEPROC(CmdMOLEalrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE    buf[MOLE_PKT_MAX];
  BYTE    truncateStr[256];
  BYTE    word[256];
  LWORD   i=0;
  LWORD   j;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */  
  if (!MOLECommandCheck(sock,"alist",-1)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  for (j=0;j<areaListMax;j++) {
    MOLEWriteBuf(sock,StrTruncate(truncateStr,
      StrFirstWord(areaList[j].aFileName->sText,word),25),buf,&i);
    MOLEWriteULWORD(sock,areaList[j].aVirtualMin,buf,&i);
    MOLEWriteULWORD(sock,areaList[j].aVirtualMax,buf,&i);
  } 
  MOLESend(sock,buf,i,MOLE_CMD_ALST);
}

CMDMOLEPROC(CmdMOLEadrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE      buf[MOLE_PKT_MAX];
  LWORD     i=0;
  WORD      area;
  ULWORD    k;
  PROPERTY *prop;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);
  /* Check security */  
  if (!MOLECommandCheck(sock,"astat",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the area */
  area=AreaOf(virtual);
  if (area>=0) {
    /* we found it - provide data */
    MOLEWriteULWORD(sock,virtual,buf,&i);
    MOLEWriteBuf(sock,areaList[area].aEditor->sText,buf,&i);
    MOLEWriteBuf(sock,areaList[area].aDesc->sText,buf,&i);
    MOLEWriteULWORD(sock,areaList[area].aResetFlag,buf,&i);
    MOLEWriteULWORD(sock,areaList[area].aResetDelay,buf,&i);
    /* send property list */
    /* You'll note we need to worry about 
     * DECOMPILING properties if they're code
     * properties, prior to transmission! Whoops! */
    k=0;
    for (prop=areaList[area].aResetThing.tProperty;prop;prop=prop->pNext) {
      k++;
    }
    MOLEWriteULWORD(sock,k,buf,&i);
    for (prop=areaList[area].aResetThing.tProperty;prop;prop=prop->pNext) {
      /* note this CodeDecompProperty will check
       * if it's compiled or not, so we don't have
       * to pre-check before calling it. */
      CodeDecompProperty(prop,NULL);
      MOLEWriteBuf(sock,prop->pKey->sText,buf,&i);
      MOLEWriteBuf(sock,prop->pDesc->sText,buf,&i);
    }
    /* and send the sucker */
    MOLESend(sock,buf,i,MOLE_CMD_ADTL);
  } else {
    /* didn't find specified area - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
}


CMDMOLEPROC(CmdMOLEadtl) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE         buf[MOLE_PKT_MAX];
  LWORD        i=0;
  WORD         area;
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
  if (!MOLECommandCheck(sock,"adesc",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the area */
  area=AreaOf(virtual);
  if (area>=0) {

    /* first, mark area as changed */
    BITSET(areaList[area].aSystem,AS_RSTUNSAVED);

    /* OK, let's read in our new Area stats */
    /* The first one is a bit tricky; it's the name of the area! */
    /* ignore it for now */
    /*rc=MOLEGetQStr(sock,&strData);
    StrFree(strData);*/

    /* Check security */  
    if ((!(rc=MOLEGetQStr(sock,&strData)))) {
      if (MOLECommandCheck(sock,"aflag",virtual)) {
     /* if ((sock->sHomeThing)&&(sock->sHomeThing->tType==TTYPE_PLR)&& 
        ((Character(sock->sHomeThing)->cLevel)>=LEVEL_ADMIN)) {*/
        StrFree(areaList[area].aEditor);
        areaList[area].aEditor=strData;
      } else {
        StrFree(strData);
      }
    }
    if ((!rc)&&(!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(areaList[area].aDesc);
      areaList[area].aDesc=strData;
    }
    /* note I do the following with all ULWORDs because
     * then I don't actually care what the specific size
     * or sign of the numerical value is... the compiler
     * will sort it all out and do the appropriate conversion. */
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) {
      if (MOLECommandCheck(sock,"aflag",virtual)) {
        areaList[area].aResetFlag=data;
      }
    }
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      areaList[area].aResetDelay=data;

    /* get property list */
    counter=0;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      counter=data;                /* counter=#properties */
    /* property list - first we need to delete our old ones */
    if (!rc)
      while(areaList[area].aResetThing.tProperty)
        areaList[area].aResetThing.tProperty=
          PropertyFree(areaList[area].aResetThing.tProperty,
          areaList[area].aResetThing.tProperty);
    while((counter>0)&&(!rc)) {
      strKey=strDesc=NULL;
      rc=MOLEGetQStr(sock,&strKey);
      if (!rc)
        rc=MOLEGetQStr(sock,&strDesc);
      if (!rc) {
        areaList[area].aResetThing.tProperty=
          PropertyCreate(areaList[area].aResetThing.tProperty,strKey,strDesc);
      } else {
        if (strKey) StrFree(strKey);
        if (strDesc) StrFree(strDesc);
      }
      counter--;
    }
    MOLESend(sock,buf,i,MOLE_CMD_ACKP);
  } else {
    /* didn't find specified area - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
  return;
}

