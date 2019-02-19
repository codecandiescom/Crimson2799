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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crimson2.h"
#include "macro.h"
#include "queue.h"
#include "log.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "file.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "send.h"
#include "base.h"
#include "affect.h"
#include "char.h"
#include "player.h"
#include "parse.h"
#include "cmd_talk.h"

#define CHANNEL_HISTORY_MAX 100

Q *gossipQ;
Q *auctionQ;
Q *godtalkQ;

void TalkInit(void) {
  gossipQ = QAlloc(512);
  auctionQ = QAlloc(512);
  godtalkQ = QAlloc(512);
}

void TalkChannel(THING *thing, BYTE *cmd, BYTE *chanName, FLAG chanFlag, Q *q) {
  SOCK *sock;
  BYTE *buf;
  BYTE *oldCmd;
  BYTE  bufChannel[128];
  BYTE  bufPlayer[128];
  BYTE  buf2[512];
  LWORD srcOS;
  LWORD dstOS;

  sock = BaseControlFind(thing);
  if (sock && !BIT(sock->sPref, chanFlag)) {
    sprintf(buf2, "^3Turning ^5%s ^3Channel On\n", chanName);
    SendThing(buf2, thing);
    BITSET(sock->sPref, chanFlag);
    Plr(sock->sHomeThing)->pSockPref = sock->sPref;
  }

  oldCmd = cmd;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    SendThing("^wTypically you would try to say something with this command....\n", thing);
    return;
  }
  sprintf(bufChannel, "^:%s: [%s] \"", chanName, thing->tSDesc->sText);
  sprintf(bufPlayer, "^:You %s: \"", chanName);


#ifdef FOR_REFERENCE_ONLY
  sprintf(buf, "^:%s: [%s] \"%s\"\n", chanName, thing->tSDesc->sText, cmd);
  StrRestoreEscape(buf);
  SendChannel(buf, thing, chanFlag);
  if (q) {
    if (q->qValid >= CHANNEL_HISTORY_MAX)
        QRead(q, buf2, sizeof(buf2), Q_COLOR_IGNORE, NULL);
    QAppend(q, buf, Q_DONTCHECKSTR);
  }

  sprintf(buf, "^:You %s: \"%s\"\n", chanName, cmd);
  StrRestoreEscape(buf);
  SendThing(buf, thing);
#endif

  /* Prettily format the text for 79 column display */
  do {
    buf = bufPlayer;
    srcOS = 0;
    dstOS = strlen(buf);
    while (cmd[srcOS]) {
      buf[dstOS] = cmd[srcOS];
      dstOS++;
      srcOS++;
      /* break up the speech into multiple lines */
      if (dstOS >= 78) {
        while(dstOS > 65) {
          if (buf[dstOS]==' ') {
            srcOS++; /* skip space */
            buf[dstOS]='\"'; dstOS++;
            buf[dstOS]='\n'; dstOS++;
            break;
          }
          dstOS-=1;
          srcOS-=1;
        }
        /* couldnt find a space - hyphenate */
        if (dstOS==65) {
          dstOS+=11;
          srcOS+=11;
          buf[dstOS] = '-'; dstOS++;
          buf[dstOS] = '\n'; dstOS++;
        }
        buf[dstOS] = '\0';

        /* Send this line */
        StrRestoreEscape(buf);
        SendThing(buf, thing);

        strcpy(buf, "     \"");
        dstOS=strlen(buf);
      }
    }
    buf[dstOS] = '\0';
    if (!sock || sockOverflow->qLen==0) {
      strcat(buf, "\"\n");
      /* Send this line */
      StrRestoreEscape(buf);
      SendThing(buf, thing);
    }

    buf = bufChannel;
    srcOS = 0;
    dstOS = strlen(buf);
    while (cmd[srcOS]) {
      buf[dstOS] = cmd[srcOS];
      dstOS++;
      srcOS++;
      /* break up the speech into multiple lines */
      if (dstOS >= 78) {
        while(dstOS > 65) {
          if (buf[dstOS]==' ') {
            srcOS++; /* skip space */
            buf[dstOS]='\"'; dstOS++;
            buf[dstOS]='\n'; dstOS++;
            break;
          }
          dstOS-=1;
          srcOS-=1;
        }
        /* couldnt find a space - hyphenate */
        if (dstOS==65) {
          dstOS+=11;
          srcOS+=11;
          buf[dstOS] = '-'; dstOS++;
          buf[dstOS] = '\n'; dstOS++;
        }
        buf[dstOS] = '\0';

        /* Send this line */
        StrRestoreEscape(buf);
        SendChannel(buf, thing, chanFlag);
        if (q) {
          while (q->qValid >= CHANNEL_HISTORY_MAX)
              QRead(q, buf2, sizeof(buf2), Q_COLOR_IGNORE, NULL);
          QAppend(q, buf, Q_DONTCHECKSTR);
        }

        strcpy(buf, "     \"");
        dstOS=strlen(buf);
      }
    }

    buf[dstOS] = '\0';
    if (!sock || sockOverflow->qLen==0) {
      strcat(buf, "\"\n");
      /* Send this line */
      StrRestoreEscape(buf);
      SendChannel(buf, thing, chanFlag);
      if (q) {
        while (q->qValid >= CHANNEL_HISTORY_MAX)
            QRead(q, buf2, sizeof(buf2), Q_COLOR_IGNORE, NULL);
        QAppend(q, buf, Q_DONTCHECKSTR);
      }
      break;
    } else {
      cmd = oldCmd;
      QRead(sockOverflow, cmd, LINE_MAX_LEN, Q_COLOR_IGNORE, NULL);
      thing->tWait++; /* delay them a beat for every 256 chars */
    }
  } while (1);

}

CMDPROC(CmdGossip) { /* Cmd(THING *thing, BYTE *cmd) */
  TalkChannel(thing, cmd, "gossip", SP_CHANGOSSIP, gossipQ);
}

CMDPROC(CmdAuction) { /* Cmd(THING *thing, BYTE *cmd) */
  TalkChannel(thing, cmd, "auction", SP_CHANAUCTION, auctionQ);
}

CMDPROC(CmdGodtalk) { /* Cmd(THING *thing, BYTE *cmd) */
  TalkChannel(thing, cmd, "godtalk", SP_CHANGOD, godtalkQ);
}

CMDPROC(CmdEmote) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE  bufRoom[512];
  BYTE  bufPlayer[512];
  BYTE *i;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  i = StrFind(cmd, "$$n");
  while (i && i[2]=='N') i=StrFind(cmd, "$$n");
  if (i) {
    /* convert $$n into $n */
    while( *(i+1) ) {
      *i = *(i+1);
      i++;
    }
    *i = '\0'; /* copy null too */

    sprintf(bufRoom,   "^c%s\n", cmd);
    sprintf(bufPlayer, "^CYou emote: \"%s\"\n", cmd);
  } else {
    sprintf(bufRoom,   "^c$n %s\n", cmd);
    sprintf(bufPlayer, "^CYou emote: \"$n %s\"\n", cmd);
  }
  SendAction(bufPlayer, thing, NULL, SEND_SRC |SEND_AUDIBLE|SEND_CAPFIRST);
  SendAction(bufRoom,   thing, NULL, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
}

CMDPROC(CmdSay) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE *buf;
  BYTE *oldCmd;
  BYTE  bufRoom[128];
  BYTE  bufPlayer[128];
  BYTE  firstLine = TRUE;
  WORD  sLen;
  LWORD srcOS;
  LWORD dstOS;
  LWORD dstLenOS;
  SOCK *sock;

  sock = BaseControlFind(thing);
  oldCmd = cmd;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  sLen = strlen(cmd);

  if (sock && sockOverflow->qLen>0) {
    if (thing->tType == TTYPE_PLR)
      sprintf(bufRoom,   "^&%c%c$n drones on and on \"", 
        Plr(thing)->pColorPref[6].cFG, Plr(thing)->pColorPref[6].cBG);
    else
      sprintf(bufRoom,   "^p$n drones on and on \"");
    sprintf(bufPlayer, "^6You drone on and on \"");
  } else {
    switch (cmd[sLen-1]) {
    case '.':
      if (sLen>1 && cmd[sLen-2]=='.') {
        if (thing->tType == TTYPE_PLR)
          sprintf(bufRoom,   "^&%c%c$n trails off \"", 
            Plr(thing)->pColorPref[6].cFG, Plr(thing)->pColorPref[6].cBG);
        else
          sprintf(bufRoom,   "^p$n trails off \"");
        sprintf(bufPlayer, "^6You trail off \"");
      } else {
        if (thing->tType == TTYPE_PLR)
          sprintf(bufRoom,   "^&%c%c$n states \"", 
            Plr(thing)->pColorPref[6].cFG, Plr(thing)->pColorPref[6].cBG);
        else
          sprintf(bufRoom,   "^p$n states \"");
        sprintf(bufPlayer, "^6You state \"");
      }
      break;
    case '!':
      if (thing->tType == TTYPE_PLR)
        sprintf(bufRoom,   "^&%c%c$n exclaims \"", 
          Plr(thing)->pColorPref[6].cFG, Plr(thing)->pColorPref[6].cBG);
      else
        sprintf(bufRoom,   "^p$n exclaims \"");
      sprintf(bufPlayer, "^6You exclaim \"");
      break;
    case '?':
      if (thing->tType == TTYPE_PLR)
        sprintf(bufRoom,   "^&%c%c$n asks \"", 
          Plr(thing)->pColorPref[6].cFG, Plr(thing)->pColorPref[6].cBG);
      else
        sprintf(bufRoom,   "^p$n asks \"");
      sprintf(bufPlayer, "^6You ask \"");
      break;
    default:
      if (thing->tType == TTYPE_PLR)
        sprintf(bufRoom,   "^&%c%c$n says \"", 
          Plr(thing)->pColorPref[6].cFG, Plr(thing)->pColorPref[6].cBG);
      else
        sprintf(bufRoom,   "^p$n says \"");
      sprintf(bufPlayer, "^6You say \"");
      break;
    }
  }

  /* Prettily format the text for 79 column display */
  do {
    buf = bufPlayer;
    srcOS = 0;
    dstOS = strlen(buf);
    while (cmd[srcOS]) {
      buf[dstOS] = cmd[srcOS];
      dstOS++;
      srcOS++;
      /* break up the speech into multiple lines */
      if (dstOS >= 78) {
        while(dstOS > 65) {
          if (buf[dstOS]==' ') {
            srcOS++; /* skip space */
            buf[dstOS]='\"'; dstOS++;
            buf[dstOS]='\n'; dstOS++;
            break;
          }
          dstOS-=1;
          srcOS-=1;
        }
        /* couldnt find a space - hyphenate */
        if (dstOS==65) {
          dstOS+=11;
          srcOS+=11;
          buf[dstOS] = '-'; dstOS++;
          buf[dstOS] = '\n'; dstOS++;
        }
        buf[dstOS] = '\0';
        SendAction(buf, thing, NULL, SEND_SRC |SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
        strcpy(buf, "     \"");
        dstOS=strlen(buf);
      }
    }
    buf[dstOS] = '\0';
    if (!sock || sockOverflow->qLen==0) {
      strcat(buf, "\"\n");
      SendAction(buf, thing, NULL, SEND_SRC |SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
    }

    buf = bufRoom;
    srcOS = 0;
    dstOS = strlen(buf);
    if (firstLine) {
      dstLenOS = thing->tSDesc->sLen;
      firstLine = FALSE;
    } else
      dstLenOS = 0;
    while (cmd[srcOS]) {
      buf[dstOS] = cmd[srcOS];
      dstOS++;
      srcOS++;
      /* break up the speech into multiple lines */
      if (dstOS+dstLenOS >= 78) {
        while(dstOS+dstLenOS > 65) {
          if (buf[dstOS]==' ') {
            srcOS++; /* skip space */
            buf[dstOS]='\"'; dstOS++;
            buf[dstOS]='\n'; dstOS++;
            break;
          }
          dstOS-=1;
          srcOS-=1;
        }
        /* couldnt find a space - hyphenate */
        if (dstOS+dstLenOS==65) {
          dstOS+=11;
          srcOS+=11;
          buf[dstOS] = '-'; dstOS++;
          buf[dstOS] = '\n'; dstOS++;
        }
        buf[dstOS] = '\0';
        SendAction(buf, thing, NULL, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
        strcpy(buf, "     \"");
        dstOS=strlen(buf);
        dstLenOS = 0;
        firstLine = FALSE;
      }
    }

    buf[dstOS] = '\0';
    if (!sock || sockOverflow->qLen==0) {
      strcat(buf, "\"\n");
      SendAction(buf, thing, NULL, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
      break;
    } else {
      cmd = oldCmd;
      QRead(sockOverflow, cmd, LINE_MAX_LEN, Q_COLOR_IGNORE, NULL);
      thing->tWait++; /* delay them a beat for every 256 chars */
    }
  } while (1);
}

CMDPROC(CmdTell) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE   bufTarget[512];
  BYTE   bufPlayer[512];
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  SOCK  *sock;
  SOCK  *thingSock;
  BYTE  *buf;
  BYTE  *oldCmd;
  BYTE   targetFirstLine = TRUE;
  BYTE   playerFirstLine = TRUE;
  LWORD  srcOS;
  LWORD  dstOS;
  LWORD  dstLenOS;

  oldCmd = cmd;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_MOB|TF_PLR|TF_MOB_WLD|TF_PLR_WLD, &srcOffset);
  if (!found) {
    SendThing("^wTell who, what?\n", thing);
    return;
  } 

  thingSock = BaseControlFind(thing);
  /* turn private channel on */
  if (thingSock && !BIT(thingSock->sPref, SP_CHANPRIVATE)) {
    SendThing("^wTurning ^cPrivate ^wChannel On\n", thing);
    BITSET(thingSock->sPref, SP_CHANPRIVATE);
    Plr(thingSock->sHomeThing)->pSockPref = thingSock->sPref;
  }

  sock = BaseControlFind(found);
  if (!sock || !BIT(sock->sPref, SP_CHANPRIVATE)) {
    sprintf(bufPlayer, "^CYou try to tell $N something but $E ignores you\n");
    SendAction(bufPlayer, thing, found, SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST);
    return;
  } else if (Character(sock->sHomeThing)->cPos <= POS_SLEEPING) {
    sprintf(bufPlayer, "^CYou try to tell $N something but $E is indisposed\n");
    SendAction(bufPlayer, thing, found, SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST);
    return;
  }

#ifdef FOR_REFERENCE_ONLY
  sprintf(bufTarget, "^b$n tells you \"%s\"\n", cmd);
  SendAction(bufTarget, thing, found, SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
  sprintf(bufPlayer, "^CYou tell $N \"%s\"\n", cmd);
  SendAction(bufPlayer, thing, found, SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
#endif

  sprintf(bufTarget, "^b$n tells you \"");
  sprintf(bufPlayer, "^CYou tell $N \"");

  /* Prettily format the text for 79 column display */
  do {
    buf = bufPlayer;
    srcOS = 0;
    dstOS = strlen(buf);
    if (playerFirstLine) {
      dstLenOS = found->tSDesc->sLen;
      playerFirstLine = FALSE;
    } else
      dstLenOS = 0;
    while (cmd[srcOS]) {
      buf[dstOS] = cmd[srcOS];
      dstOS++;
      srcOS++;
      /* break up the speech into multiple lines */
      if (dstOS+dstLenOS >= 78) {
        while(dstOS+dstLenOS > 65) {
          if (buf[dstOS]==' ') {
            srcOS++; /* skip space */
            buf[dstOS]='\"'; dstOS++;
            buf[dstOS]='\n'; dstOS++;
            break;
          }
          dstOS-=1;
          srcOS-=1;
        }
        /* couldnt find a space - hyphenate */
        if (dstOS+dstLenOS==65) {
          dstOS+=11;
          srcOS+=11;
          buf[dstOS] = '-'; dstOS++;
          buf[dstOS] = '\n'; dstOS++;
        }
        buf[dstOS] = '\0';
        SendAction(buf, thing, found, SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
        strcpy(buf, "     \"");
        dstOS=strlen(buf);
        playerFirstLine = FALSE;
      }
    }
    buf[dstOS] = '\0';
    if (!sock || sockOverflow->qLen==0) {
      strcat(buf, "\"\n");
      SendAction(buf, thing, found, SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
    }

    buf = bufTarget;
    srcOS = 0;
    dstOS = strlen(buf);
    if (targetFirstLine) {
      dstLenOS = thing->tSDesc->sLen;
      targetFirstLine = FALSE;
    } else
      dstLenOS = 0;
    while (cmd[srcOS]) {
      buf[dstOS] = cmd[srcOS];
      dstOS++;
      srcOS++;
      /* break up the speech into multiple lines */
      if (dstOS+dstLenOS >= 78) {
        while(dstOS+dstLenOS > 65) {
          if (buf[dstOS]==' ') {
            srcOS++; /* skip space */
            buf[dstOS]='\"'; dstOS++;
            buf[dstOS]='\n'; dstOS++;
            break;
          }
          dstOS-=1;
          srcOS-=1;
        }
        /* couldnt find a space - hyphenate */
        if (dstOS+dstLenOS==65) {
          dstOS+=11;
          srcOS+=11;
          buf[dstOS] = '-'; dstOS++;
          buf[dstOS] = '\n'; dstOS++;
        }
        buf[dstOS] = '\0';
        SendAction(buf, thing, found, SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
        strcpy(buf, "     \"");
        dstOS=strlen(buf);
        dstLenOS = 0;
        targetFirstLine = FALSE;
      }
    }

    buf[dstOS] = '\0';
    if (!sock || sockOverflow->qLen==0) {
      strcat(buf, "\"\n");
      SendAction(buf, thing, found, SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
      break;
    } else {
      cmd = oldCmd;
      QRead(sockOverflow, cmd, LINE_MAX_LEN, Q_COLOR_IGNORE, NULL);
      thing->tWait++; /* delay them a beat for every 256 chars */
    }
  } while (1);


  /* Status messages etc */
  if (BIT(Plr(sock->sHomeThing)->pAuto, PA_AFK)) {
    sprintf(bufPlayer, "^CHowever, $N is probably away from keyboard right now\n");
    SendAction(bufPlayer, thing, found, SEND_SRC|SEND_CAPFIRST);
  }
  if (sock->sEdit.eStr) {
    sprintf(bufPlayer, "^CHowever, $N is editing and might take awhile to respond\n");
    SendAction(bufPlayer, thing, found, SEND_SRC|SEND_CAPFIRST);
  }
  if (found->tType==TTYPE_PLR && Plr(found)->pIdleTick > playerIdleTick) {
    sprintf(bufPlayer, "^CHowever, $N has now been idle for %ld minutes\n", Plr(found)->pIdleTick);
    SendAction(bufPlayer, thing, found, SEND_SRC|SEND_CAPFIRST);
  }
  /* Update reply field */
  StrOneWord(Base(found)->bKey->sText, bufTarget);
  if (Character(found)->cRespond) STRFREE(Character(found)->cRespond);
  Character(found)->cRespond = StrAlloc(Base(thing)->bKey);
}

CMDPROC(CmdRespond) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE    newCmd[512];

  if (!Character(thing)->cRespond) {
    SendThing("^wBefore you can respond to someone, someone has to talk to you first\n", thing);
    return;
  }
  
  /* auto-tell the person who last tell'd to us */
  cmd = StrOneWord(cmd, NULL); /* lose respond at front */
  sprintf(newCmd, "tell %s %s", Character(thing)->cRespond->sText, cmd);
  CmdTell(thing, newCmd);
}

CMDPROC(CmdOrder) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE   bufTarget[512];
  BYTE   bufPlayer[512];
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_MOB|TF_PLR, &srcOffset);
  if (!found) {
    SendThing("^wOrder who, what?\n", thing);
    return;
  } 

  thing->tWait++;

  sprintf(bufTarget, "^b$n orders you to \"%s\"\n", cmd);
  sprintf(bufPlayer, "^CYou order $N to \"%s\"\n", cmd);
  SendAction(bufPlayer, thing, found, SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
  SendAction(bufTarget, thing, found, SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);

  if (Character(found)->cLead == thing 
   && BIT(Character(found)->cAffectFlag, AF_DOMINATED))
      ParseCommandStub(found, cmd);
}

CMDPROC(CmdGTell) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE   bufTarget[512];
  BYTE   bufPlayer[512];
  SOCK  *sock;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  sock = BaseControlFind(thing);
  /* turn group channel on */
  if (!BIT(sock->sPref, SP_CHANGROUP)) {
    SendThing("^wTurning ^cGroup ^wChannel On\n", thing);
    BITSET(sock->sPref, SP_CHANGROUP);
    Plr(sock->sHomeThing)->pSockPref = sock->sPref;
  }
  sprintf(bufTarget, "^b$n gtells you \"%s\"\n", cmd);
  sprintf(bufPlayer, "^CYou gtell \"%s\"\n", cmd);
  SendAction(bufPlayer, thing, NULL, SEND_SRC  |SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
  SendAction(bufTarget, thing, NULL, SEND_GROUP|SEND_AUDIBLE|SEND_CAPFIRST|SEND_HISTORY);
}

/* fun proc, rainbow-izes spoken text */
CMDPROC(CmdRainbow) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  #define COLOR_CODES "cCbwrRpPyYgG"

  BYTE    newCmd[256*3];
  LWORD   cmdLen;
  LWORD   colorCodeMax;
  LWORD   i;

  cmd = StrOneWord(cmd, NULL); /* lose rainbow at front */
  sprintf(newCmd, "say ");
  cmdLen = strlen(newCmd);
  colorCodeMax = strlen(COLOR_CODES)-1;
  for (i=0; cmd[i]; i++) {
    newCmd[cmdLen  ] = '^';
    newCmd[cmdLen+1] = COLOR_CODES[Number(0, colorCodeMax)];
    newCmd[cmdLen+2] = cmd[i];
    cmdLen += 3;
  }
  newCmd[cmdLen]='\0';
  CmdSay(thing, newCmd);
}

struct ChType {
  BYTE *chName;
  BYTE *chCheck;
};

struct ChType chList[] = {
  { "Personal", NULL      },
  { "Gossip",   NULL      },
  { "Auction",  NULL      },
  { "Godtalk",  "godtalk" },
  { "",         NULL      }
};

CMDPROC(CmdChHistory) {
  SOCK    *sock = NULL;
  LWORD    i;
  BYTE     buf[512];
  Q       *q;
  LWORD    maxLines;
  LWORD    skipLines;
  LWORD    search;
  LWORD    show;

  sock = BaseControlFind(thing);
  if (!sock) return;

  cmd = StrOneWord(cmd, NULL);
  
  if (!*cmd) {
    SendThing("^wYou can see the channel history for the following:\n^g", thing);
    for (i=0; *chList[i].chName; i++) {
      if (!chList[i].chCheck || ParseCommandCheck(TYPEFIND(chList[i].chCheck, commandList), sock, "")) {
        SendThing("  ", thing);
        SendThing(chList[i].chName, thing);
        SendThing("\n", thing);
      }
    }
    SendHint("^;HINT: You can also specify the # of lines you want to see\n", thing);
    SendHint("^;HINT: ie ^<chhistory personal 20 ^;to see only the last 20 lines\n", thing);
    SendHint("^;HINT: You can also search the history\n", thing);
    SendHint("^;HINT: ie ^<chhistory personal cryogen ^;to see what cryogen had to say\n", thing);
    return;
  }

  cmd = StrOneWord(cmd, buf);
  i = TYPEFIND(buf, chList);
  if (i == -1) {
    SendThing("^wWhich channel would that be now?\n", thing);
    return;
  }

  if (chList[i].chCheck && !ParseCommandCheck(TYPEFIND(chList[i].chCheck, commandList), sock, "")) {
    SendThing("^wWhich channel would that be now?\n", thing);
    return;
  }
  
  /* Set q to point to the channel */
  switch(i) {
  case 0:
    q = sock->sPersonal;
    break;
  case 1:
    q = gossipQ;
    break;
  case 2:
    q = auctionQ;
    break;
  case 3:
    q = godtalkQ;
    break;
  }

  /* Find how many lines they are interested in */
  skipLines = 0;
  maxLines = CHANNEL_HISTORY_MAX;
  if (*cmd) {
    if (StrIsNumber(cmd)) 
      maxLines = atol(cmd);
    else
      search = TRUE;
  }
  MAXSET(maxLines, q->qValid);
  skipLines = q->qValid - maxLines;

  /* Send the channel title */
  sprintf(buf, 
          "^w%s Channel History: (Showing %ld/%ld)\n",
          chList[i].chName,
          maxLines,
          q->qValid);
  SendThing(buf, thing);

  /* send the Qbuffer */
  i = q->qStart;
  show = TRUE;
  do {
    buf[0] = 0;
    i =  QScan(q, i, buf, sizeof(buf));
    if (skipLines>0) {
      skipLines -= 1;
      continue;
    }
    /* search for messages matching pattern */
    if (search && buf[0]!=' ') {
      if (StrFind(buf, cmd)) 
        show = TRUE;
      else
        show = FALSE;
    }
    if (*buf && show) {
      SendThing(buf,  thing);
      SendThing("\n", thing);
    }
  } while(*buf);
  SendThing("^V\n", thing);
}


CMDPROC(CmdBeep) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE   bufTarget[512];
  BYTE   bufPlayer[512];
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  SOCK  *sock;
  SOCK  *thingSock;
  BYTE   c = 7;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_MOB|TF_PLR|TF_MOB_WLD|TF_PLR_WLD, &srcOffset);
  if (!found) {
    SendThing("^wSend an audible Beep to who?\n", thing);
    return;
  } 

  thingSock = BaseControlFind(thing);
  sock = BaseControlFind(found);

  if (!sock || !BIT(sock->sPref, SP_CHANPRIVATE)) {
    sprintf(bufPlayer, "^CYou try to beep $N but $E ignores you\n");
    SendAction(bufPlayer, thing, found, SEND_SRC|SEND_CAPFIRST);
  } else if (sock->sEdit.eStr) {
    sprintf(bufPlayer, "^CYou try to beep $N but $E is editing\n");
    SendAction(bufPlayer, thing, found, SEND_SRC|SEND_CAPFIRST);
  } else {
    sprintf(bufTarget, "^b$n sends you an audible beep!%c\n", c);
    sprintf(bufPlayer, "^CYou send $N an audible beep!%c\n",c);
    SendAction(bufPlayer, thing, found, SEND_SRC|SEND_CAPFIRST|SEND_HISTORY);
    SendAction(bufTarget, thing, found, SEND_DST|SEND_CAPFIRST|SEND_HISTORY);
    if (BIT(Plr(sock->sHomeThing)->pAuto, PA_AFK)) {
      sprintf(bufPlayer, "^CHowever, $N is probably away from keyboard right now\n");
      SendAction(bufPlayer, thing, found, SEND_SRC|SEND_CAPFIRST);
    }
    if (found->tType==TTYPE_PLR && Plr(found)->pIdleTick > playerIdleTick) {
      sprintf(bufPlayer, "^CHowever, $N has now been idle for %ld minutes\n", Plr(found)->pIdleTick);
      SendAction(bufPlayer, thing, found, SEND_SRC|SEND_CAPFIRST);
    }

  }
}

