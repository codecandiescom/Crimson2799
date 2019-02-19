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
#include "reset.h"
#include "cmd_inv.h"
#include "moledefs.h"
#include "mole.h"
#include "mole_msc.h"

/* Identification */
CMDMOLEPROC(CmdMOLEidrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE    buf[MOLE_PKT_MAX];
  LWORD   i=0;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);
  MOLEWriteULWORD(sock,
    (CRIMSON_MAJOR_VERSION<<24)|(CRIMSON_MINOR_VERSION<<16)|
    (MOLE_MAJOR_VERSION<<8)|(MOLE_MINOR_VERSION),buf,&i);
  MOLESend(sock,buf,i,MOLE_CMD_IDEN);
}

/* System Information */
CMDMOLEPROC(CmdMOLEsysr) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE    buf[MOLE_PKT_MAX];
  LWORD   i=0;
  WORD    type;
  WORD    field;
  WORD    numType;
  WORD    numField;

  if (pktID<MOLE_PKID_START)
    return;

  /* we send the flag and type lists */
  /* Note that the list order MUST correspond to the
   * order defined by the MOLE_LIST_* in mole.h!!! */
  MOLEWriteULWORD(sock,pktID,buf,&i);
  MOLEWriteBuf(sock,MOLE_SERVER_NAME,buf,&i);
  MOLEWriteULWORD(sock,MOLE_LIST_NUMLISTS,buf,&i);
  MOLEWRITELIST(sock,mTypeList,buf,&i);
  MOLEWRITELIST(sock,sexList,buf,&i);
  MOLEWRITELIST(sock,posList,buf,&i);
  MOLEWRITELIST(sock,mActList,buf,&i);
  MOLEWRITELIST(sock,affectList,buf,&i);
  MOLEWRITELIST(sock,wTypeList,buf,&i);
  MOLEWRITELIST(sock,wFlagList,buf,&i);
  MOLEWRITELIST(sock,oActList,buf,&i);
  MOLEWRITELIST(sock,oSFlagList,buf,&i);
  MOLEWRITELIST(sock,oWFlagList,buf,&i);
  MOLEWRITELIST(sock,oAmmoList,buf,&i);
  MOLEWRITELIST(sock,oCFlagList,buf,&i);
  MOLEWRITELIST(sock,oLiquidList,buf,&i);
  MOLEWRITELIST(sock,applyList,buf,&i);
  MOLEWRITELIST(sock,rFlagList,buf,&i);
  MOLEWRITELIST(sock,wearList,buf,&i);
  MOLEWRITELIST(sock,dirList,buf,&i);
  MOLEWRITELIST(sock,eFlagList,buf,&i);
  MOLEWRITELIST(sock,oTypeList,buf,&i);
  MOLEWRITELIST(sock,resistList,buf,&i);
  MOLEWRITELIST(sock,weaponList,buf,&i);
  /* and lastly, and most trickily, we must send
   * the oListType array */
  /* first, count the types */
  for (numType=0;*(oTypeList[numType].oTypeStr);numType++);
  /* write this to our buffer */
  MOLEWriteULWORD(sock,numType,buf,&i);
  /* now cycle through our types and transmit each one */
  for (type=0;type<numType;type++) {
    /* first, count the fields */
    for (numField=0;*(oTypeList[type].oField[numField]);numField++);
    /* write this to our buffer */
    MOLEWriteULWORD(sock,numField,buf,&i);
    /* now cycle through our fields and transmit each one */
    for (field=0;field<numField;field++) {
      /* transmit name of field */
      MOLEWriteBuf(sock,(oTypeList[type]).oFieldStr[field],buf,&i);
      /* transmit field format (int, list, flag) */
      if ((oTypeList[type]).oField[field][2]=='T') {
        /* type lists  */
        MOLEWriteULWORD(sock,MOLE_LISTTYPE_TYPE,buf,&i);
        /* transmit type list number */
        if ((oTypeList[type]).oArray[field].oList==(BYTE**)(oAmmoList)) {
          MOLEWriteULWORD(sock,MOLE_LIST_OAMMO,buf,&i);
        } else if ((oTypeList[type]).oArray[field].oList==(BYTE**)(weaponList)) {
          MOLEWriteULWORD(sock,MOLE_LIST_WEAPON,buf,&i);
        } else if ((oTypeList[type]).oArray[field].oList==(BYTE**)(oLiquidList)) {
          MOLEWriteULWORD(sock,MOLE_LIST_OLIQUID,buf,&i);
        } else {
          /* default MOLE_LIST_INVALID (ie: unimplemented/invalid) */
          MOLEWriteULWORD(sock,MOLE_LIST_INVALID,buf,&i);
        }
      } else if ((oTypeList[type]).oField[field][2]=='F') {
        /* Flag lists  */
        MOLEWriteULWORD(sock,MOLE_LISTTYPE_FLAG,buf,&i);
        /* transmit flag list number */
        if ((oTypeList[type]).oArray[field].oList==(BYTE**)(oSFlagList)) {
          MOLEWriteULWORD(sock,MOLE_LIST_OSFLAG,buf,&i);
        } else if ((oTypeList[type]).oArray[field].oList==(BYTE**)(oWFlagList)) {
          MOLEWriteULWORD(sock,MOLE_LIST_OWFLAG,buf,&i);
        } else if ((oTypeList[type]).oArray[field].oList==(BYTE**)(resistList)) {
          MOLEWriteULWORD(sock,MOLE_LIST_RESIST,buf,&i);
        } else if ((oTypeList[type]).oArray[field].oList==(BYTE**)(oCFlagList)) {
          MOLEWriteULWORD(sock,MOLE_LIST_OCFLAG,buf,&i);
        } else {
          /* default MOLE_LIST_INVALID (ie: unimplemented/invalid) */
          MOLEWriteULWORD(sock,MOLE_LIST_INVALID,buf,&i);
        }
      } else {
        /* default (int)  */
        MOLEWriteULWORD(sock,MOLE_LISTTYPE_INVALID,buf,&i);
      }
    }
  }
  MOLESend(sock,buf,i,MOLE_CMD_SYSD);
}
