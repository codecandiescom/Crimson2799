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
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "crimson2.h"
#include "macro.h"
#include "mem.h"
#include "log.h"
#include "queue.h"
#include "str.h"
#include "parse.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "send.h"
#include "base.h"
#include "exit.h"
#include "world.h"
#include "char.h"
#include "player.h"
#include "board.h"
#include "cmd_brd.h"


/* God / Editing Command */
CMDPROC(CmdBList) { /*(THING *thing, BYTE *cmd)*/ 
  LWORD      board;
  LWORD      i;
  BYTE       buf[256];
  SOCK      *sock;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^p   # Boards                           Entries Editors\n", thing);
    SendThing("^P     =-=-=-=-=-=-=                    =-=-=-= =-=-=-=\n", thing);
    for (i=0; i<boardListMax; i++) {
      sprintf(buf, 
              "^r%4ld ^G[^g%-30s^G] [^g%2ld/%2hd^G] [^g%s^G]", 
              boardList[i].bVirtual,
              boardList[i].bName->sText, 
              boardList[i].bIndex.iNum, 
              boardList[i].bMax, 
              boardList[i].bEditor->sText);
      SendThing(buf, thing);
      SendThing("\n", thing);
    }
    sprintf(buf, "^pBoards: ^g%ld\n", boardListMax);
    SendThing(buf, thing);
    SendHint("^;HINT: Try ^wBLIST ^c<virtual> for a list of messages\n", thing);
    return;
  }

  cmd = StrOneWord(cmd, buf);
  board = BoardOf(atol(buf));
  if (board == -1) {
    SendThing("^wThere is no board with that virtual number\n", thing);
    return;
  }

  sock = BaseControlFind(thing);
  if (!sock || Character(sock->sHomeThing)->cLevel < boardList[board].bMinLevel) {
    SendThing("^wYou are too low in level to read that board\n", thing);
    return;
  }

  /* List of message titles */
  if (!*cmd) {
    BoardShow(board, thing);
    SendHint("^;HINT: Try ^wBLIST ^c<virtual> <msg#> to read a message\n", thing);
    return;
  }
  
  i = -1;
  sscanf(cmd, " %ld", &i);
  i -= 1;
  if (i<0 || i>= boardList[board].bIndex.iNum) {
    SendThing("^wThere is no message with that number\n", thing);
    return;
  }

  BoardShowMessage(board, i, thing);
}

/* Create a new keyword in the given section */
CMDPROC(CmdBCreate) { /*(THING *thing, BYTE *cmd)*/ 
  BYTE  fileName[256];
  LWORD virtual;
  LWORD board;

  /* rip apart arguments */
  cmd = StrOneWord(cmd, NULL);
  cmd = StrOneWord(cmd, fileName);
  virtual = -1;
  sscanf(cmd, "%ld", &virtual); /* see if they entered a number */

  if (!*cmd || !*fileName || virtual<0) {
    SendThing("^rTry BCREATE <FileName> <Board Virtual#>\n", thing);
    return;
  }
  if (!StrIsAlNum(fileName)) {
    SendThing("^rThat was not a valid FileName!\n", thing);
  }

  board = BoardCreate(fileName, virtual);
  if (board == -1) {
    SendThing("^wOh oh this aint good, critical error of some kind...\n", thing);
    return;
  }
  
  BoardTableWrite();
  BoardWrite(board);
}

/* Edit the entry for a given keyword */
CMDPROC(CmdBName) { /*(THING *thing, BYTE *cmd)*/ 
  LWORD board;
  BYTE  buf[256];
  BYTE  strName[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry BName <Board Virtual>\n", thing);
    return;
  }
  board = BoardOf(atol(cmd));
  if (board == -1) {
    SendThing("^rNo board with that virtual found\n", thing);
    return;
  }

  sprintf(buf, "^bBoard [^c%-18s^b]\n", boardList[board].bFileName);
  SendThing(buf, thing);
  sprintf(buf, "^bVirtual: [^c%ld]\n", boardList[board].bVirtual);
  SendThing(buf, thing);

  sprintf(strName, "Board [%s #%ld] - Name", boardList[board].bFileName, boardList[board].bVirtual);
  EDITSTR(thing, boardList[board].bName, 64, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &boardListFlag, B_UNSAVED);
}


/* Edit the entry for a given keyword */
CMDPROC(CmdBEditor) { /*(THING *thing, BYTE *cmd)*/ 
  LWORD board;
  BYTE  buf[256];
  BYTE  strName[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry BEditor <Board Virtual>\n", thing);
    return;
  }
  board = BoardOf(atol(cmd));
  if (board == -1) {
    SendThing("^rNo board with that virtual found\n", thing);
    return;
  }

  sprintf(buf, "^bBoard [^c%-18s^b]\n", boardList[board].bFileName);
  SendThing(buf, thing);
  sprintf(buf, "^bVirtual: [^c%ld]\n", boardList[board].bVirtual);
  SendThing(buf, thing);

  sprintf(strName, "Board [%s #%ld] - Editor", boardList[board].bFileName, boardList[board].bVirtual);
  EDITSTR(thing, boardList[board].bEditor, 64, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &boardListFlag, B_UNSAVED);
}

CMDPROC(CmdBEdit) {
  LWORD     board;
  BYTE      buf[256];
  BYTE      strName[256];
  BOARDMSG *boardMsg;
  LWORD     i;
  
  if (thing->tType!=TTYPE_PLR) {
    SendThing("^rMobs cant edit board messages\n", thing);
    return;
  }

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry BEdit <Board Virtual> <msg #>\n", thing);
    return;
  }
  board = BoardOf(atol(cmd));
  if (board == -1) {
    SendThing("^rNo board with that virtual found\n", thing);
    return;
  }

  sprintf(buf, "^bBoard [^c%-18s^b]\n", boardList[board].bFileName);
  SendThing(buf, thing);
  sprintf(buf, "^bVirtual: [^c%ld]\n", boardList[board].bVirtual);
  SendThing(buf, thing);

  /* check virtual # */
  cmd = StrOneWord(cmd, NULL);
  i = -1;
  sscanf(cmd, " %ld", &i);
  i -= 1;
  if (i<0 || i>= boardList[board].bIndex.iNum) {
    SendThing("^wThere is no message with that number\n", thing);
    return;
  }
  boardMsg = BoardMsg(boardList[board].bIndex.iThing[i]);

  if (  (Character(thing)->cLevel<LEVEL_CODER) 
      ||(!StrExact(thing->tSDesc->sText, boardMsg->bAuthor->sText)) ) {
    SendThing("^rYou can only edit your own messages\n", thing);
    return;
  }

  sprintf(strName, "Board [%s %ld]- Edit", boardList[board].bFileName, boardList[board].bVirtual);
  EDITSTR(thing, boardMsg->bText, 2048, strName, EP_ENDLF|EP_IMMNEW);
  EDITFLAG(thing, &boardList[board].bFlag, B_UNSAVED);
}


CMDPROC(CmdBWrite) {
  LWORD     board;
  BYTE      buf[256];
  BYTE      strName[256];
  BOARDMSG *boardMsg;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry BWrite <Board Virtual> <msg title>\n", thing);
    return;
  }
  board = BoardOf(atol(cmd));
  if (board == -1) {
    SendThing("^rNo board with that virtual found\n", thing);
    return;
  }

  sprintf(buf, "^bBoard [^c%-18s^b]\n", boardList[board].bFileName);
  SendThing(buf, thing);
  sprintf(buf, "^bVirtual: [^c%ld]\n", boardList[board].bVirtual);
  SendThing(buf, thing);

  /* chuck virtual # */
  cmd = StrOneWord(cmd, NULL);
  if (!*cmd)
    boardMsg = BoardMsgCreate(board, thing, "^Y<No Title>", BOARD_NOTREPLY);
  else
    boardMsg = BoardMsgCreate(board, thing, cmd, BOARD_NOTREPLY);

  if (!boardMsg) {
    SendThing("^rNot good, critical error - aborting message\n", thing);
    return;
  }
  sprintf(strName, "Board [%s %ld]- Write", boardList[board].bFileName, boardList[board].bVirtual);
  EDITSTR(thing, boardMsg->bText, 2048, strName, EP_ENDLF|EP_IMMNEW);
  EDITFLAG(thing, &boardList[board].bFlag, B_UNSAVED);
}

CMDPROC(CmdBReply) {
  LWORD     board;
  BYTE      buf[256];
  BYTE      titleBuf[50];
  BYTE      strName[256];
  BOARDMSG *boardMsg;
  LWORD     i;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry BWrite <Board Virtual> <msg #> [<msg title>]\n", thing);
    return;
  }
  board = BoardOf(atol(cmd));
  if (board == -1) {
    SendThing("^rNo board with that virtual found\n", thing);
    return;
  }

  sprintf(buf, "^bBoard [^c%-18s^b]\n", boardList[board].bFileName);
  SendThing(buf, thing);
  sprintf(buf, "^bVirtual: [^c%ld]\n", boardList[board].bVirtual);
  SendThing(buf, thing);

  /* check msg # */
  cmd = StrOneWord(cmd, NULL);
  i = -1;
  sscanf(cmd, " %ld", &i);
  i -= 1;
  if (i<0 || i>= boardList[board].bIndex.iNum) {
    SendThing("^wThere is no message with that number\n", thing);
    return;
  }
  boardMsg = BoardMsg(boardList[board].bIndex.iThing[i]);

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    strcpy(titleBuf, "Re:");
    strncpy(titleBuf+3, boardMsg->bTitle->sText, sizeof(titleBuf)-3);
    titleBuf[sizeof(titleBuf)-1] = 0;
    boardMsg = BoardMsgCreate(board, thing, titleBuf, i);
  } else
    boardMsg = BoardMsgCreate(board, thing, cmd, i);

  if (!boardMsg) {
    SendThing("^rNot good, critical error aborting message\n", thing);
    return;
  }
  sprintf(strName, "Board [%s %ld]- Reply", boardList[board].bFileName, boardList[board].bVirtual);
  EDITSTR(thing, boardMsg->bText, 2048, strName, EP_ENDLF|EP_IMMNEW);
  EDITFLAG(thing, &boardList[board].bFlag, B_UNSAVED);
}

CMDPROC(CmdBErase) {
  LWORD     board;
  BYTE      buf[256];
  BOARDMSG *boardMsg;
  LWORD     i;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry BErase <Board Virtual> <msg #>\n", thing);
    return;
  }
  board = BoardOf(atol(cmd));
  if (board == -1) {
    SendThing("^rNo board with that virtual found\n", thing);
    return;
  }

  sprintf(buf, "^bBoard [^c%-18s^b]\n", boardList[board].bFileName);
  SendThing(buf, thing);
  sprintf(buf, "^bVirtual: [^c%ld]\n", boardList[board].bVirtual);
  SendThing(buf, thing);

  /* check msg # */
  cmd = StrOneWord(cmd, NULL);
  i = -1;
  sscanf(cmd, " %ld", &i);
  i -= 1;
  if (i<0 || i>= boardList[board].bIndex.iNum) {
    SendThing("^wThere is no message with that number\n", thing);
    return;
  }
  boardMsg = BoardMsg(boardList[board].bIndex.iThing[i]);

  SendThing("^wDeleting:\n", thing);
  sprintf(buf, "^gMessage Number: ^G[^b%ld/%ld^G]\n", i+1, boardList[board].bIndex.iNum); 
  SendThing(buf, thing);
  sprintf(buf, "^gAuthor:         ^G[^b%s^G]\n", boardMsg->bAuthor->sText);
  SendThing(buf, thing);
  SendThing(   "^gMessage Title:  ^G[^g", thing);
  SendThing(boardMsg->bTitle->sText, thing);
  SendThing("^G]\n", thing);

  BoardMsgDelete(board, i);
}
















