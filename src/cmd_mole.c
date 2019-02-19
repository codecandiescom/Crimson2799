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
#include "moledefs.h"
#include "mole.h"
#include "cmd_mole.h"
#include "mole_msc.h"
#include "mole_are.h"
#include "mole_wld.h"
#include "mole_mob.h"
#include "mole_obj.h"
#include "mole_rst.h"

const CMDMOLELIST cmdMOLEList[] = {
  { MOLE_CMD_MORE, NULL,           POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_IDRQ, CmdMOLEidrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_IDEN, NULL,           POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_SYSR, CmdMOLEsysr,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_SYSD, NULL,           POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_ALRQ, CmdMOLEalrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_RLRQ, CmdMOLErlrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_WLRQ, CmdMOLEwlrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_MLRQ, CmdMOLEmlrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_OLRQ, CmdMOLEolrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_ADRQ, CmdMOLEadrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_WDRQ, CmdMOLEwdrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_MDRQ, CmdMOLEmdrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_ODRQ, CmdMOLEodrq,    POS_DEAD,  1,           LOG_NEVER,   CF_MISC }, 
  { MOLE_CMD_RLST, CmdMOLErlst,    POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { MOLE_CMD_ADTL, CmdMOLEadtl,    POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { MOLE_CMD_WDTL, CmdMOLEwdtl,    POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { MOLE_CMD_MDTL, CmdMOLEmdtl,    POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { MOLE_CMD_ODTL, CmdMOLEodtl,    POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { MOLE_CMD_ACPY, NULL,           POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { MOLE_CMD_WCPY, NULL,           POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { MOLE_CMD_MCPY, NULL,           POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { MOLE_CMD_OCPY, NULL,           POS_DEAD,  1,           LOG_NORMAL,  CF_MISC }, 
  { 0L,            NULL,           0,         0,           0,         0 }
}; 

CMDPROC(CmdMOLE) { /* void CmdProc(THING *thing, BYTE* cmd) */
  SOCK   *sock;
  UWORD   offset;
  ULWORD  pktCmd;
  LWORD   i;
  BYTE    buf[MOLE_PKT_MAX];
  ULWORD  pktID;
  LWORD   virtual;
  ULWORD  error;

  if ((!thing)||(!cmd))
    return;
  sock=BaseControlFind(thing);
  if (!sock) 
    return;

  /* Check to ensure this is a valid MOLE packet, and if not, 
   * respond with an error message. */
  if(MOLEParse(sock,cmd,&pktCmd)<0) {
    ParseCommand(thing,"YOUDUMBASSWHATTHEHELLDOESTHATMEAN");
    return;
  }
  /* don't process MORE commands any further */
  if (pktCmd==MOLE_CMD_MORE) {
    return;
  }

  /* strip off packet number */
  if (MOLEGetQULWORD(sock,&pktID)<0) {
    ParseCommand(thing,"YOUDUMBASSWHATTHEHELLDOESTHATMEAN");
    MOLEFlushQ(sock);
    return;
  } else {
    /* and our virtual number / extra data */
    if (MOLEGetQLWORD(sock,&virtual)<0)
      virtual=-1;
  }

  error=MOLE_NACK_NOTIMPLEMENTED;
  /* Find command in cmdMOLEList (note this can't use TYPEFIND 
   * because CmdMoleList is indexed off a ULWORD, not a BYTE*). */
  for (offset=0;
    (cmdMOLEList[offset].mCmd)&&(cmdMOLEList[offset].mCmd!=pktCmd);
    offset++);

  /* if valid command found, process the sucker */
  if ((cmdMOLEList[offset].mCmd)&&(cmdMOLEList[offset].mProc)) {
    /* As Ryan says, "Choke if they are too weeny" */
    /* NOTE NOTE NOTE: Permission responsibilities have changed to 
     * the individual procedures and cross referenced through the 
     * parse.c command security system. The following are left in for 
     * future expansion or customization. */
    if (Character(sock->sHomeThing)->cLevel >= cmdMOLEList[offset].mLevel) {
      /* check position */
      if (CmdMOLEPositionCheck(offset,thing)) {
        if (CmdMOLEFlagCheck(offset,sock,virtual)) {
          /* call command */
          if (cmdMOLEList[offset].mProc) 
            (*cmdMOLEList[offset].mProc)(sock,pktID,virtual);
          MOLEFlushQ(sock);
          return;
        } else {
          error=MOLE_NACK_PROTECTION;
        }
      } else {
        error=MOLE_NACK_POSITION;
      }
    } else {
      error=MOLE_NACK_AUTHORIZATION;
    }
  } 

  /* return an error message if something went wrong */
  i=0;
  if (pktID>=MOLE_PKID_START){
    MOLEWriteULWORD(sock,pktID,buf,&i);
  }
  MOLEWriteULWORD(sock,error,buf,&i);
  if (error==MOLE_NACK_POSITION) { /* send current & required positions */
    MOLEWriteULWORD(sock,Character(thing)->cPos,buf,&i);
    MOLEWriteULWORD(sock,cmdMOLEList[offset].mPos,buf,&i);
  }
  MOLESend(sock,buf,i,MOLE_CMD_NACK);
  MOLEFlushQ(sock);
  return;
}

BYTE CmdMOLEPositionCheck(LWORD i, THING *thing) {
  if (Character(thing)->cPos >= cmdMOLEList[i].mPos)
    return TRUE;
  return FALSE;
}

/* this proc does any flag checking & logging necessary */
BYTE CmdMOLEFlagCheck(LWORD i,SOCK *sock,LWORD virtual) {
  WORD area;
  BYTE returnCode;

  returnCode=TRUE;
  /* special flags check */
  if (BITANY(cmdMOLEList[i].mFlag,CF_AREAEDIT)) {
    area=AreaOf(virtual);
    if (area<0) return TRUE;
    if (AreaIsEditor(area,sock->sHomeThing)==2)
      returnCode=TRUE;
    else if (Character(sock->sHomeThing)->cLevel>=LEVEL_CODER)
      returnCode=TRUE;
    else
      returnCode=FALSE;
  }

  if (returnCode==TRUE) {
    /* check for logging */
    if( (cmdMOLEList[i].mLog==LOG_ALLWAYS)
       ||( (BIT(Plr(sock->sHomeThing)->pSystem, PS_LOG))
         &&(cmdMOLEList[i].mLog==LOG_NORMAL) )
      ) {
      LogStr(sock->sHomeThing->tSDesc->sText,CmdMOLELog(i,virtual));
    }
    /* log area editing to area channel */
    if( (cmdMOLEList[i].mLog>=LOG_NORMAL)
      &&(BIT(cmdMOLEList[i].mFlag,CF_AREAEDIT)) ) {
      Log(LOG_AREA,sock->sHomeThing->tSDesc->sText);
      LogPrintf(LOG_AREA,CmdMOLELog(i,virtual));
    }
    if( (cmdMOLEList[i].mLog>=LOG_NORMAL)
      &&(BIT(cmdMOLEList[i].mFlag,CF_GOD)) ) {
      Log(LOG_GOD,sock->sHomeThing->tSDesc->sText);
      LogPrintf(LOG_GOD,CmdMOLELog(i,virtual));
    }
  }

  return returnCode;
}

/* this proc returns a string containing an appropriate log blurb. */
/* NOTE: the returned string is SHARED - it's reused each time
 * this proc is called, so it's only good until the next call
 * to this function! */
BYTE *CmdMOLELog(LWORD i,LWORD virtual) {
  static BYTE buf[100];

  sprintf(buf," xxxx %li\n",virtual);
  buf[1]=(cmdMOLEList[i].mCmd>>24)&0xFF;
  buf[2]=(cmdMOLEList[i].mCmd>>16)&0xFF;
  buf[3]=(cmdMOLEList[i].mCmd>>8)&0xFF;
  buf[4]=(cmdMOLEList[i].mCmd)&0xFF;
  return buf;
}
