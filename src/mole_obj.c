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

CMDMOLEPROC(CmdMOLEolrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE    buf[MOLE_PKT_MAX];
  LWORD   i=0;
  WORD    area;
  LWORD   k;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"olist",virtual)) {
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
    for (k=0;k<areaList[area].aObjIndex.iNum;k++) {
      MOLEWriteBuf(sock,ObjTemplate(areaList[area].aObjIndex.iThing[k])->oSDesc->sText,buf,&i);
      MOLEWriteULWORD(sock,ObjTemplate(areaList[area].aObjIndex.iThing[k])->oVirtual,buf,&i);
    }
    MOLESend(sock,buf,i,MOLE_CMD_OLST);
  }
}

CMDMOLEPROC(CmdMOLEodrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE         buf[MOLE_PKT_MAX];
  LWORD        i=0;
  OBJTEMPLATE *object;
  EXTRA       *extra;
  PROPERTY    *prop;
  ULWORD       k;
  WORD         field;


  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"ostat",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the object */
  object=ObjectOf(virtual);
  if (object) {
    /* we found it - provide data */
    MOLEWriteULWORD(sock,object->oVirtual,buf,&i);
    MOLEWriteBuf(sock,object->oKey->sText,buf,&i);
    MOLEWriteBuf(sock,object->oSDesc->sText,buf,&i);
    MOLEWriteBuf(sock,object->oLDesc->sText,buf,&i);
    MOLEWriteBuf(sock,object->oDesc->sText,buf,&i);
    MOLEWriteULWORD(sock,object->oType,buf,&i);
    MOLEWriteULWORD(sock,object->oAct,buf,&i);
    MOLEWriteULWORD(sock,object->oWear,buf,&i);
    MOLEWriteULWORD(sock,object->oWeight,buf,&i);
    MOLEWriteULWORD(sock,object->oValue,buf,&i);
    MOLEWriteULWORD(sock,object->oRent,buf,&i);

    /* send details */
    for (field=0;*(oTypeList[object->oType].oField[field]);field++) {
      MOLEWriteULWORD(sock,ObjectGetField(object->oType,
        &(object->oDetail),field),buf,&i);
    }

    /* send apply list */
    MOLEWriteULWORD(sock,OBJECT_MAX_APPLY,buf,&i);
    for (k=0;k<OBJECT_MAX_APPLY;k++) {
      MOLEWriteULWORD(sock,(object->oApply)[k].aType,buf,&i);
      MOLEWriteULWORD(sock,(object->oApply)[k].aValue,buf,&i);
    }

    /* send extra list */
    /* first, count the number of extras */
    k=0;
    for (extra=object->oExtra;extra;extra=extra->eNext) {
      k++;
    }
    MOLEWriteULWORD(sock,k,buf,&i);
    /* and now send the extras. */
    for (extra=object->oExtra;extra;extra=extra->eNext) {
      MOLEWriteBuf(sock,extra->eKey->sText,buf,&i);
      MOLEWriteBuf(sock,extra->eDesc->sText,buf,&i);
    }
    /* send property list */
    /* You'll note we need to worry about 
     * DECOMPILING properties if they're code 
     * properties, prior to transmission! Whoops! */
    k=0;
    for (prop=object->oProperty;prop;prop=prop->pNext) {
      k++;
    }
    MOLEWriteULWORD(sock,k,buf,&i);
    for (prop=object->oProperty;prop;prop=prop->pNext) {
      /* note this CodeDecompProperty will check 
       * if it's compiled or not, so we don't 
       * have to pre-check before calling it. */
      CodeDecompProperty(prop,NULL);
      object->oCompile=1; /* compile on demand enabled */
      MOLEWriteBuf(sock,prop->pKey->sText,buf,&i);
      MOLEWriteBuf(sock,prop->pDesc->sText,buf,&i);
    }
    /* and send the sucker */
    MOLESend(sock,buf,i,MOLE_CMD_ODTL);
  } else {
    /* didn't find specified object - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
}

CMDMOLEPROC(CmdMOLEodtl) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE         buf[MOLE_PKT_MAX];
  LWORD        i=0;
  OBJTEMPLATE *object;
  LWORD        rc;      /* return code */
  ULWORD       data;
  STR         *strData;
  STR         *strKey;
  STR         *strDesc;
  LWORD        counter;
  LWORD        k;
  WORD         field;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"odesc",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the object */
  object=ObjectOf(virtual);
  if (object) {
    /* first, mark area as changed */
    BITSET(areaList[AreaOf(virtual)].aSystem,AS_OBJUNSAVED);
    object->oCompile=1; /* compile on demand enabled */
    /* Ok, let's read in our new MOB stats */
    if ((!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(object->oKey);
      object->oKey=strData;
    }
    if ((!rc)&&(!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(object->oSDesc);
      object->oSDesc=strData;
    }
    if ((!rc)&&(!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(object->oLDesc);
      object->oLDesc=strData;
    }
    if ((!rc)&&(!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(object->oDesc);
      object->oDesc=strData;
    }

    /* note I do the following with all ULWORDs because
     * then I don't actually care what the specific size
     * or sign of the numerical value is... the compiler
     * will sort it all out and do the appropriate conversion. */
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      object->oType=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      object->oAct=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      object->oWear=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      object->oWeight=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      object->oValue=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      object->oRent=data;

    /* get details */
    for (field=0;(*(oTypeList[object->oType].oField[field]))&&(!rc);field++) {
      if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
        ObjectSetField(object->oType, &(object->oDetail),field,data);
    }

    /* get apply list */
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      counter=data;
    for(k=0;(k<counter)&&(!rc);k++) {
      if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
        (object->oApply)[k].aType=data;
      if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
        (object->oApply)[k].aValue=data;
    }

    /* get extra list */
    counter=0;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      counter=data;                /* counter=#extras */
    /* extra list - first we need to delete our old ones */
    if (!rc)
      while(object->oExtra)
        object->oExtra=ExtraFree(object->oExtra,object->oExtra);
    while((counter>0)&&(!rc)) {
      strKey=strDesc=NULL;
      rc=MOLEGetQStr(sock,&strKey);
      if (!rc)
        rc=MOLEGetQStr(sock,&strDesc);
      if (!rc) {
        object->oExtra=ExtraAlloc(object->oExtra,strKey,strDesc);
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
      while(object->oProperty)
        object->oProperty=PropertyFree(object->oProperty,object->oProperty);
    while((counter>0)&&(!rc)) {
      strKey=strDesc=NULL;
      rc=MOLEGetQStr(sock,&strKey);
      if (!rc)
        rc=MOLEGetQStr(sock,&strDesc);
      if (!rc) {
        object->oProperty=PropertyCreate(object->oProperty,strKey,strDesc);
      } else {
        if (strKey) StrFree(strKey);
        if (strDesc) StrFree(strDesc);
      }
      counter--;
    }
    MOLESend(sock,buf,i,MOLE_CMD_ACKP);
  } else {
    /* didn't find specified object - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
  return;
}
