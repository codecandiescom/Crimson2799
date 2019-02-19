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

CMDMOLEPROC(CmdMOLEwlrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE    buf[MOLE_PKT_MAX];
  LWORD   i=0;
  WORD    area;
  LWORD   k;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"wlist",virtual)) {
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
    for (k=0;k<areaList[area].aWldIndex.iNum;k++) {
      MOLEWriteBuf(sock,Thing(areaList[area].aWldIndex.iThing[k])->tSDesc->sText,buf,&i);
      MOLEWriteULWORD(sock,Wld(areaList[area].aWldIndex.iThing[k])->wVirtual,buf,&i);
    }
    MOLESend(sock,buf,i,MOLE_CMD_WLST);
  }
}

CMDMOLEPROC(CmdMOLEwdrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE         buf[MOLE_PKT_MAX];
  LWORD        i=0;
  THING       *world;
  EXTRA       *extra;
  PROPERTY    *prop;
  ULWORD       k;
  EXIT        *exit;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"wstat",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the world */
  world=WorldOf(virtual);
  if (world) {
    /* we found it - provide data */
    MOLEWriteULWORD(sock,Wld(world)->wVirtual,buf,&i);
    MOLEWriteBuf(sock,world->tSDesc->sText,buf,&i);
    MOLEWriteBuf(sock,world->tDesc->sText,buf,&i);
    MOLEWriteULWORD(sock,Wld(world)->wType,buf,&i);
    MOLEWriteULWORD(sock,Wld(world)->wFlag,buf,&i);
    /* EXIT list */
    k=0;
    for (exit=Wld(world)->wExit;exit;exit=exit->eNext) {
      k++;
    }  
    MOLEWriteULWORD(sock,k,buf,&i);
    /* now actually send the exit list */
    for (exit=Wld(world)->wExit;exit;exit=exit->eNext) {
      MOLEWriteULWORD(sock,exit->eDir,buf,&i);
      MOLEWriteBuf(sock,exit->eKey->sText,buf,&i);
      MOLEWriteBuf(sock,exit->eDesc->sText,buf,&i);
      MOLEWriteULWORD(sock,exit->eKeyObj,buf,&i);
      MOLEWriteULWORD(sock,exit->eFlag,buf,&i);
      if(exit->eWorld)
        MOLEWriteULWORD(sock,Wld(exit->eWorld)->wVirtual,buf,&i);
      else
        MOLEWriteULWORD(sock,playerStartRoom,buf,&i);
    }
    /* send extra list */
    /* first, count the number of extras */
    k=0;
    for (extra=world->tExtra;extra;extra=extra->eNext) {
      k++;
    }
    MOLEWriteULWORD(sock,k,buf,&i);
    /* and now send the extras. */
    for (extra=world->tExtra;extra;extra=extra->eNext) {
      MOLEWriteBuf(sock,extra->eKey->sText,buf,&i);
      MOLEWriteBuf(sock,extra->eDesc->sText,buf,&i);
    }
    /* send property list */
    /* You'll note we need to worry about 
     * DECOMPILING properties if they're code 
     * properties, prior to transmission! Whoops! */
    k=0;
    for (prop=world->tProperty;prop;prop=prop->pNext) {
      k++;
    }
    MOLEWriteULWORD(sock,k,buf,&i);
    for (prop=world->tProperty;prop;prop=prop->pNext) {
      /* note this CodeDecompProperty will check 
       * if it's compiled or not, so we don't 
       * have to pre-check before calling it. */
      CodeDecompProperty(prop,NULL);
      /* Enable compile on demand */
      BITSET(world->tFlag, TF_COMPILE);
      MOLEWriteBuf(sock,prop->pKey->sText,buf,&i);
      MOLEWriteBuf(sock,prop->pDesc->sText,buf,&i);
    }
    /* and send the sucker */
    MOLESend(sock,buf,i,MOLE_CMD_WDTL);
  } else {
    /* didn't find specified world - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
}

CMDMOLEPROC(CmdMOLEwdtl) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE         buf[MOLE_PKT_MAX];
  LWORD        i=0;
  THING       *world;
  LWORD        rc;      /* return code */
  ULWORD       data;
  STR         *strData;
  STR         *strKey;
  STR         *strDesc;
  LWORD        counter;
  BYTE         dir;
  LWORD        keyObj;
  FLAG         flag;
  THING       *eWorld;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  if (!MOLECommandCheck(sock,"wdesc",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the world */
  world=WorldOf(virtual);
  if (world) {
    /* first, mark area as changed */
    BITSET(areaList[AreaOf(virtual)].aSystem,AS_WLDUNSAVED);
    /* Enable compile on demand */
    BITSET(world->tFlag, TF_COMPILE);
    /* Ok, let's read in our new WLD stats */
    if ((!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(world->tSDesc);
      world->tSDesc=strData;
    }
    if ((!rc)&&(!(rc=MOLEGetQStr(sock,&strData)))) {
      StrFree(world->tDesc);
      world->tDesc=strData;
    }
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      Wld(world)->wType=data;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      Wld(world)->wFlag=data;
    /* get extra list */
    counter=0;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      counter=data;                /* counter=#exits */
    /* exit list - first we need to delete our old ones */
    if (!rc)
      while(Wld(world)->wExit)
        Wld(world)->wExit=ExitFree(Wld(world)->wExit,Wld(world)->wExit);
    while((counter>0)&&(!rc)) {
      strKey=strDesc=NULL;
      if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
        dir=data;
      if (!rc) 
        rc=MOLEGetQStr(sock,&strKey);
      if (!rc)
        rc=MOLEGetQStr(sock,&strDesc);
      if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
        keyObj=data;
      if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
        flag=data;
      if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
        eWorld=WorldOf(data);
      if (!rc) {
        Wld(world)->wExit=ExitAlloc(Wld(world)->wExit,dir,strKey,strDesc,
          flag,keyObj,eWorld);
      } else {
        if (strKey) StrFree(strKey);
        if (strDesc) StrFree(strDesc);
      }
      counter--;
    }
    /* get extra list */
    counter=0;
    if ((!rc)&&(!(rc=MOLEGetQULWORD(sock,&data)))) 
      counter=data;                /* counter=#extras */
    /* extra list - first we need to delete our old ones */
    if (!rc)
      while(world->tExtra)
        world->tExtra=ExtraFree(world->tExtra,world->tExtra);
    while((counter>0)&&(!rc)) {
      strKey=strDesc=NULL;
      rc=MOLEGetQStr(sock,&strKey);
      if (!rc)
        rc=MOLEGetQStr(sock,&strDesc);
      if (!rc) {
        world->tExtra=ExtraAlloc(world->tExtra,strKey,strDesc);
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
      while(world->tProperty)
        world->tProperty=PropertyFree(world->tProperty,world->tProperty);
    while((counter>0)&&(!rc)) {
      strKey=strDesc=NULL;
      rc=MOLEGetQStr(sock,&strKey);
      if (!rc)
        rc=MOLEGetQStr(sock,&strDesc);
      if (!rc) {
        world->tProperty=PropertyCreate(world->tProperty,strKey,strDesc);
      } else {
        if (strKey) StrFree(strKey);
        if (strDesc) StrFree(strDesc);
      }
      counter--;
    }
    MOLESend(sock,buf,i,MOLE_CMD_ACKP);
  } else {
    /* didn't find specified world - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
  return;
}
