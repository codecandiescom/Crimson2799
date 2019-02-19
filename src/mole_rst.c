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
#include "reset.h"
#include "moledefs.h"
#include "mole.h"
#include "mole_msc.h"

CMDMOLEPROC(CmdMOLErlrq) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE       buf[MOLE_PKT_MAX];
  LWORD      i=0;
  WORD       area;
  LWORD      k;

  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  /* we can't use regular "rshow" kuz it doesn't take a vnum arg */
  if (!MOLECommandCheck(sock,"wlist",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the specified area */
  area=AreaOf(virtual);
  if (area>=0) {
    MOLEWriteULWORD(sock,virtual,buf,&i);
    MOLEWriteULWORD(sock,areaList[area].aResetNum,buf,&i);
    for (k=0;k<areaList[area].aResetNum;k++) {
      MOLEWriteULWORD(sock,areaList[area].aResetList[k].rCmd,buf,&i);
      MOLEWriteULWORD(sock,areaList[area].aResetList[k].rIf,buf,&i);
      switch ((areaList[area].aResetList)[k].rCmd) {
        case 'O': /* Object to room */
        case 'o': /* Object to room */
          MOLEWriteULWORD(sock,
            (areaList[area].aResetList)[k].rArg1.rObj->oVirtual,
            buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg2.rNum,buf,&i);
          MOLEWriteULWORD(sock,Wld((areaList[area].aResetList)[k].rArg3.rWld)->wVirtual,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg4.rNum,buf,&i);
          break;
        case 'M': /* Mobile to room */
        case 'm': /* Mobile to room */
          MOLEWriteULWORD(sock,
            (areaList[area].aResetList)[k].rArg1.rMob->mVirtual,
            buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg2.rNum,buf,&i);
          MOLEWriteULWORD(sock,Wld((areaList[area].aResetList)[k].rArg3.rWld)->wVirtual,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg4.rNum,buf,&i);
          break;
        case 'G': /* give object to last mobile */
        case 'g': /* give object to last mobile */
          MOLEWriteULWORD(sock,
            (areaList[area].aResetList)[k].rArg1.rObj->oVirtual,
            buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg2.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg3.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg4.rNum,buf,&i);
          break;
        case 'P': /* put object inside object */
        case 'p': /* put object inside object */
          MOLEWriteULWORD(sock,
            (areaList[area].aResetList)[k].rArg1.rObj->oVirtual,
            buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg2.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg3.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg4.rNum,buf,&i);
          break;
        case 'E': /* equip mobile with an object */
        case 'e': /* equip mobile with an object */
          MOLEWriteULWORD(sock,
            (areaList[area].aResetList)[k].rArg1.rObj->oVirtual,
            buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg2.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg3.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg4.rNum,buf,&i);
          break;
        case 'D': /* set door state */
        case 'd': /* set door state */
          MOLEWriteULWORD(sock,
            Wld((areaList[area].aResetList)[k].rArg1.rWld)->wVirtual,
            buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg2.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg3.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg4.rNum,buf,&i);
          break;
        case 'R': /* remove object from room */
        case 'r': /* remove object from room */
          MOLEWriteULWORD(sock,
            Wld((areaList[area].aResetList)[k].rArg1.rWld)->wVirtual,
            buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg2.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg3.rNum,buf,&i);
          MOLEWriteULWORD(sock,(areaList[area].aResetList)[k].rArg4.rNum,buf,&i);
          break;
        case '*': /* comment */
        default:
          MOLEWriteULWORD(sock,0L,buf,&i);
          MOLEWriteULWORD(sock,0L,buf,&i);
          MOLEWriteULWORD(sock,0L,buf,&i);
          MOLEWriteULWORD(sock,0L,buf,&i);
          break;
      }
    }
    MOLESend(sock,buf,i,MOLE_CMD_RLST);
  } else { 
    /* didn't find specified area - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
}

CMDMOLEPROC(CmdMOLErlst) { /* CmdMOLEProc( SOCK *sock, ULWORD *pktID, LWORD virtual) */
  BYTE         buf[MOLE_PKT_MAX];
  LWORD        i=0;
  RESETLIST    reset;
  LWORD        rc;      /* return code */
  ULWORD       data;
  ULWORD       dataCmd;
  ULWORD       dataIf;
  ULWORD       dataArg[4];
  LWORD        counter;
  WORD         resetGood;
  WORD         area;


  if (pktID<MOLE_PKID_START)
    return;

  MOLEWriteULWORD(sock,pktID,buf,&i);

  /* Check security */
  /* we can't use regular "rupdate" kuz it doesn't take a vnum arg */
  if (!MOLECommandCheck(sock,"wdesc",virtual)) {
    MOLEWriteULWORD(sock,MOLE_NACK_AUTHORIZATION,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
    MOLEFlushQ(sock);
    return;
  }

  /* first, we need to find the specified area */
  area=AreaOf(virtual);
  if (area>=0) {
    /* first, mark area as changed */
    BITSET(areaList[area].aSystem,AS_RSTUNSAVED);
    /* delete all previous reset commands */
    areaList[area].aResetNum=0;
    /* Ok, let's read in our new RST stats */
    if ((!(rc=MOLEGetQULWORD(sock,&data)))) 
      counter=data;                /* counter=# reset entries */
    while((counter>0)&&(!rc)) {
      if (!rc) rc=MOLEGetQULWORD(sock,&dataCmd);
      if (!rc) rc=MOLEGetQULWORD(sock,&dataIf);
      if (!rc) rc=MOLEGetQULWORD(sock,&dataArg[0]);
      if (!rc) rc=MOLEGetQULWORD(sock,&dataArg[1]);
      if (!rc) rc=MOLEGetQULWORD(sock,&dataArg[2]);
      if (!rc) rc=MOLEGetQULWORD(sock,&dataArg[3]);

      /* now let's look at this sucker and see if we can make *
       * a valid reset from it. If we can't, we just turf the *
       * whole thing.                                         */
      reset.rCmd=dataCmd;
      reset.rIf=dataIf;
      resetGood=0;
      switch (dataCmd) {
        case 'M':
          if (!(reset.rArg1.rMob=MobileOf(dataArg[0])))
            break;
          reset.rArg2.rNum=dataArg[1];
          if (!(reset.rArg3.rWld=WorldOf(dataArg[2])))
            break;
          reset.rArg4.rNum=dataArg[3];
          resetGood=1;
          break;
        case 'O':
          if (!(reset.rArg1.rObj=ObjectOf(dataArg[0])))
            break;
          reset.rArg2.rNum=dataArg[1];
          if (!(reset.rArg3.rWld=WorldOf(dataArg[2])))
            break;
          reset.rArg4.rNum=dataArg[3];
          resetGood=1;
          break;
        case 'G':
          if (!(reset.rArg1.rObj=ObjectOf(dataArg[0])))
            break;
          reset.rArg2.rNum=dataArg[1];
          reset.rArg3.rNum=0; /* zero out extra params */
          reset.rArg4.rNum=0; /* zero out extra params */
          resetGood=1;
          break;
        case 'P':
          if (!(reset.rArg1.rObj=ObjectOf(dataArg[0])))
            break;
          reset.rArg2.rNum=dataArg[1];
          reset.rArg3.rNum=dataArg[2];
          reset.rArg4.rNum=0; /* zero out extra params */
          resetGood=1;
          break;
        case 'E':
          if (!(reset.rArg1.rObj=ObjectOf(dataArg[0])))
            break;
          reset.rArg2.rNum=dataArg[1];
          reset.rArg3.rNum=0; /* zero out extra params */
          reset.rArg4.rNum=0; /* zero out extra params */
          resetGood=1;
          break;
        case 'D':
          if (!(reset.rArg1.rWld=WorldOf(dataArg[0])))
            break;
          reset.rArg2.rNum=dataArg[1];
          reset.rArg3.rNum=dataArg[2];
          reset.rArg4.rNum=0; /* zero out extra params */
          resetGood=1;
          break;
        case 'R':
          if (!(reset.rArg1.rWld=WorldOf(dataArg[0])))
            break;
          reset.rArg2.rNum=dataArg[1];
          reset.rArg3.rNum=0; /* zero out extra params */
          reset.rArg4.rNum=0; /* zero out extra params */
          resetGood=1;
          break;
        case '*':
        default:
          /* who knows... unsupported. Just ignore */
          reset.rArg1.rNum=0; /* zero out extra params */
          reset.rArg2.rNum=0; /* zero out extra params */
          reset.rArg3.rNum=0; /* zero out extra params */
          reset.rArg4.rNum=0; /* zero out extra params */
          break;
      }
      if (resetGood) {
        /* let's create this reset! */
        REALLOC("MOLEReset(mole_rst.c): resetList reallocation\n", areaList[area].aResetList,RESETLIST,areaList[area].aResetNum+1, areaList[area].aResetByte);
        memcpy((void*)&(areaList[area].aResetList[areaList[area].aResetNum]),(void*)&reset,sizeof(RESETLIST));
        areaList[area].aResetNum++;
      }
      counter--;
    }

    MOLESend(sock,buf,i,MOLE_CMD_ACKP);
  } else {
    /* didn't find specified area - return nack */
    MOLEWriteULWORD(sock,MOLE_NACK_NODATA,buf,&i);
    MOLESend(sock,buf,i,MOLE_CMD_NACK);
  }
}
