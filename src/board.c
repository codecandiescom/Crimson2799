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
#include "log.h"
#include "mem.h"
#include "str.h"
#include "extra.h"
#include "file.h"
#include "ini.h"
#include "send.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "social.h"
#include "base.h"
#include "object.h"
#include "board.h"

#define BOARD_DEFAULT_TEXT "<Message Currently being Written>\n"

BOARDLIST *boardList = NULL;

BYTE  boardReadLog;
LWORD boardListMax = 0;
LWORD boardListByte = BOARD_LIST_SIZE;
FLAG  boardListFlag;

BYTE *bFlagList[] = {
  "UNSAVED",
  ""
};


/* 
 * Sorts by sequence number's
 */

/* returns same values as strcmp */
INDEXPROC(BoardCompareProc) { /* BYTE IndexProc(INDEX *i, void *index1, void *index2) */
  if (BoardMsg(index1)->bSequence < BoardMsg(index2)->bSequence)
    return -1;
  if (BoardMsg(index1)->bSequence > BoardMsg(index2)->bSequence)
    return 1;
  return 0;
}

/* END - INTERNAL FUNCTIONS */

void BoardInit(void) {
  FILE *boardFile;
  BYTE  buf[256];
  BYTE  tmp[128];
  LWORD virtual;
  LWORD board;
  LWORD boardNum = 0;

  boardReadLog     = INILWordRead("crimson2.ini", "boardReadLog",  0);

  boardFile = fopen("board/board.tbl", "rb");
  if (!boardFile) {
    Log(LOG_BOOT, "Unable to read board.tbl file, killing server\n");
    exit(ERROR_BADFILE);
  }

  /* report on initialization */
  sprintf(buf, "Initial boardList allocation of %ld entries\n", boardListByte/sizeof(BOARDLIST));
  Log(LOG_BOOT, buf);

  /* note allways keep totally null entry at end of board list so I can
     use it as a type array */
  REALLOC("BoardInit(board.c): boardList reallocation\n", boardList, BOARDLIST, boardListMax+2, boardListByte);
  memset(&boardList[boardListMax+1], 0, sizeof(BOARDLIST)); /* init to zeros */
 
  /* okay we opened it up so read it.... */
  while (!feof(boardFile)) {
    fscanf(boardFile, " %s", tmp); /* get file name */
    if (tmp[0] == '$' || feof(boardFile))
      break; /* Dikumud file format EOF character */
    if (tmp[0] != '#' && tmp[0] != '*') { /* ignore comments */
      /* note allways keep totally null entry at end of board list so I can
         use it as a type array */
      strcpy(boardList[boardListMax].bFileName, tmp);
      virtual = -1;
      fscanf(boardFile, " %ld ", &virtual); 
      board = BoardCreate(tmp, virtual);
      fscanf(boardFile, " "); /* strip out spaces */
      boardList[board].bEditor = FileStrRead(boardFile);

      /* load the board file for this board */
      BoardRead(board);
      BoardMsgDeleteCheck(board);
      boardNum+=boardList[board].bIndex.iNum;

      if (boardReadLog) {
        sprintf(buf,
          "Read Board:[%-18s] Msg[%5ld] [%s]\n",
          boardList[board].bFileName,
          boardList[board].bIndex.iNum,
          boardList[board].bEditor->sText
        );
        Log(LOG_BOOT, buf);
      }

    } else { /* read comment */
      fgets(buf, 256, boardFile);
    }
  }
  /* all done close up shop */
  fclose(boardFile);

  sprintf(buf,
    "Board Totals: Boards[%2ld] Messages[%5ld]\n\n",
    boardListMax,
    boardNum
   );
  Log(LOG_BOOT, buf);
}

void BoardTableWrite(void) {
  FILE *boardFile;
  LWORD board;

  boardListFlag = 0; /* mark it as up to date */
  boardFile = fopen("board/board.tbl", "wb");
  if (!boardFile) {
    Log(LOG_BOOT, "Unable to write board.tbl file\n");
    PERROR("BoardTableWrite");
    return;
  }

  /* okay we opened it up so read it.... */
  for(board=0; board<boardListMax; board++) {
    fprintf(boardFile, "%-15s %5ld ", boardList[board].bFileName, boardList[board].bVirtual);
    FileStrWrite(boardFile, boardList[board].bEditor);
  }
  fprintf(boardFile, "$\n");
  
  /* all done close up shop */
  fclose(boardFile);
}

void BoardRead(BYTE board) {
  FILE     *boardFile;
  BYTE      buf[256];
  BYTE      tmp[256];
  BOARDMSG *boardMsg;

  sprintf(buf, "board/%s.brd", boardList[board].bFileName);
  boardFile = fopen(buf, "rb");
  if (!boardFile) {
    sprintf(buf, "Unable to read board/%s, skipping\n", boardList[board].bFileName);
    Log(LOG_BOOT, buf);
    boardList[board].bName     = STRCREATE(boardList[board].bFileName);
    boardList[board].bEditor   = STRCREATE("");
    boardList[board].bMax      = 30;
    boardList[board].bMinLevel = 1;
    sprintf(buf, "boardIndex/%s", boardList[board].bFileName);
    IndexInit( &boardList[board].bIndex, 32*sizeof(BOARDMSG*), buf, 0 );
    return;
  }
  boardList[board].bName     = FileStrRead(boardFile);
  boardList[board].bFlag     = FileFlagRead(boardFile, bFlagList); 
  fscanf(boardFile, " %hd %hd ", &boardList[board].bMax, &boardList[board].bMinLevel);
  sprintf(buf, "boardIndex/%s", boardList[board].bFileName);
  IndexInit( &boardList[board].bIndex, boardList[board].bMax*sizeof(BOARDMSG*), buf, 0 );

  /* okay we opened it up so read it.... */
  while (!feof(boardFile)) {
    fgets(tmp, 256, boardFile); /* get filename */
    if (tmp[0] == '$' || feof(boardFile))
      break; /* Dikumud file format EOF character */
    if (tmp[0] == '#') { /* ignore comments */
      MEMALLOC(boardMsg, BOARDMSG, BOARDMSG_ALLOC_SIZE);
      boardMsg->bAuthor = FileStrRead(boardFile);
      boardMsg->bTitle  = FileStrRead(boardFile);
      boardMsg->bText   = FileStrRead(boardFile);
      fscanf(boardFile, " %lu ", &boardMsg->bCreateTime);
      fscanf(boardFile, " %lu ", &boardMsg->bLastRead);
      fscanf(boardFile, " %hd ", &boardMsg->bReplyTo);

      boardMsg->bSequence = boardList[board].bIndex.iNum;
      if (boardMsg->bReplyTo >= boardMsg->bSequence)
        boardMsg->bReplyTo = BOARD_NOTREPLY;
      if (boardMsg->bReplyTo < 0)
        boardMsg->bReplyTo = BOARD_NOTREPLY;
      IndexInsert(&boardList[board].bIndex, boardMsg, BoardCompareProc);
    } else {
      sprintf(buf, "Illegal character found near line:\n");
      strcat(tmp, "\n");
      Log(LOG_ERROR, buf);
      Log(LOG_ERROR, tmp);
    }
  }
  /* all done close up shop */
  fclose(boardFile);
}

BYTE BoardWrite(BYTE board) {
  LWORD i;
  FILE *boardFile;
  BYTE  buf[256];

  BITCLR(boardList[board].bFlag, B_UNSAVED);
  sprintf(buf, "board/%s.brd", boardList[board].bFileName);
  boardFile = fopen(buf, "wb");
  if (!boardFile) {
    sprintf(buf, "Unable to write file board/%s.brd\n", boardList[board].bFileName);
    Log(LOG_ERROR, buf);
    return FALSE;
  }
  FileStrWrite(boardFile,  boardList[board].bName);
  FileFlagWrite(boardFile, boardList[board].bFlag, bFlagList, '\n');
  fprintf(boardFile, "%hd %hd\n", boardList[board].bMax, boardList[board].bMinLevel);
  for (i=0; i<boardList[board].bIndex.iNum; i++) {
    fprintf(boardFile, "#%hd\n", BoardMsg(boardList[board].bIndex.iThing[i])->bSequence);
    FileStrWrite(boardFile,  BoardMsg(boardList[board].bIndex.iThing[i])->bAuthor);
    FileStrWrite(boardFile,  BoardMsg(boardList[board].bIndex.iThing[i])->bTitle);
    FileStrWrite(boardFile,  BoardMsg(boardList[board].bIndex.iThing[i])->bText);
    fprintf(boardFile, "%lu %lu %hd\n",
      BoardMsg(boardList[board].bIndex.iThing[i])->bCreateTime,   
      BoardMsg(boardList[board].bIndex.iThing[i])->bLastRead,
      BoardMsg(boardList[board].bIndex.iThing[i])->bReplyTo);
  }
  fprintf(boardFile, "$\n");
  fclose(boardFile);
  return TRUE;
}

/* ouch this is gonna be a costly proc to execute - insertion sort it...*/
LWORD BoardCreate(BYTE *fileName, LWORD virtual){
  LWORD board;
  BYTE  buf[256];

  board = 0;
  while ( (board < boardListMax)
       && (virtual > boardList[board].bVirtual)
    ) board++;

  /* note allways keep totally null entry at end of board list so I can
     use it as a type array */
  REALLOC("BoardInit(board.c): boardList reallocation\n", boardList, BOARDLIST, boardListMax+2, boardListByte);
  memset(&boardList[boardListMax+1], 0, sizeof(BOARDLIST)); /* init to zeros */
  boardListMax++;

  /* can use the os memmove instead, thought it was giving me grief but it was something else */
  MemMoveHigher(&boardList[board+1], &boardList[board], sizeof(BOARDLIST)*(boardListMax-board+1));
  memset(&boardList[board], 0, sizeof(BOARDLIST)); /* init to zeros */

  strcpy(boardList[board].bFileName, fileName);
  boardList[board].bEditor = STRCREATE("");
  boardList[board].bName = STRCREATE(fileName);
  boardList[board].bVirtual = virtual;
  boardList[board].bMax = 23; /* 23 msgs by default */
  sprintf(buf, "boardIndex/%s", boardList[board].bFileName);
  IndexInit( &boardList[board].bIndex, boardList[board].bMax*sizeof(BOARDMSG*), buf, 0 );

  return board;
}

LWORD BoardOf(LWORD virtual) {
  LWORD i;

  /* since list is sorted we can actually binary search */
  for (i=0; i<boardListMax; i++) {
    if (virtual == boardList[i].bVirtual)
      return i;
    if (virtual < boardList[i].bVirtual)
      return -1;
  }
  return -1;
}

/* delete one as necessary to keep under max */
void BoardMsgDeleteCheck(BYTE board) {
  LWORD     i;
  LWORD     lru; /* least recently used strategy */
  BOARDMSG *boardMsg;

/* First a bit of editor theory to explain how cancelling a message works:
 * while a STR is being edited in the editor its given a private STR struct
 * with sNum = STR_PRIVATE, that points to the shared sText buffer. Upon
 * exiting the editor the private STR struct is free'd and a global one
 * restored/created. Therefore if the sNum is still STR_PRIVATE then it is
 * still being edited, and we shouldnt touch it!
 */

  /* Check for Cancelled messages and turf them too */
  for (i=0; i<boardList[board].bIndex.iNum; ) {
    boardMsg = BoardMsg(boardList[board].bIndex.iThing[i]);
    if (boardMsg->bText->sNum != STR_PRIVATE 
    && !strcmp(BOARD_DEFAULT_TEXT, boardMsg->bText->sText)) {
      BoardMsgDelete(board, i);
    } else {
      i++; 
    }
  }

  /* Turf excess messages */
  while ( boardList[board].bIndex.iNum > 0
       && boardList[board].bIndex.iNum > boardList[board].bMax) {

    lru = 0;
    for (i=0; i<boardList[board].bIndex.iNum; i++) {
      if (BoardMsg(boardList[board].bIndex.iThing[i])->bCreateTime < BoardMsg(boardList[board].bIndex.iThing[lru])->bCreateTime)
        lru = i;
    }

    BoardMsgDelete(board, lru);
  }
}

BOARDMSG *BoardMsgCreate(BYTE board, THING *thing, BYTE *title, WORD replyTo){
  BOARDMSG *boardMsg;
  BOARDMSG *root;
  BOARDMSG *msg;
  LWORD     i;
  LWORD     insert;

  /* Validate replyTo so it cant crash mud if board file corrupt */
  BOUNDSET(-1, replyTo, boardList[board].bIndex.iNum-1);

  MEMALLOC(boardMsg, BOARDMSG, BOARDMSG_ALLOC_SIZE);
  boardMsg->bAuthor     = StrAlloc(thing->tSDesc);
  boardMsg->bTitle      = STRCREATE(title);
  boardMsg->bText       = STRCREATE(BOARD_DEFAULT_TEXT);
  boardMsg->bCreateTime = time(0);
  boardMsg->bLastRead   = time(0);
  boardMsg->bReplyTo    = replyTo;
  boardMsg->bNotify     = TRUE;

  if (replyTo == BOARD_NOTREPLY) {
    /* thank god no insertion required */
    insert  = boardList[board].bIndex.iNum;
  } else {
    /* find where to insert the reply */
    insert = replyTo+1;
    
    while (insert < boardList[board].bIndex.iNum) {
      root = BoardMsg(boardList[board].bIndex.iThing[insert]);
      while (root->bReplyTo != BOARD_NOTREPLY && root->bReplyTo!=replyTo) {
        root = BoardMsg(boardList[board].bIndex.iThing[root->bReplyTo]);
      }
      if (root->bReplyTo==replyTo)
        insert++;
      else
        break;
    }
  }

  /* insert as required */
  if (insert < boardList[board].bIndex.iNum) {
    /* re-arrange sequence numbers to leave a gap for the new message */
    for (i=0; i<boardList[board].bIndex.iNum; i++) {
      msg = BoardMsg(boardList[board].bIndex.iThing[i]);
      if (msg->bReplyTo >= insert)
        msg->bReplyTo++;
      if (msg->bSequence >= insert)
        msg->bSequence++;
    }
  }
  
  boardMsg->bSequence  = insert;
  IndexInsert(&boardList[board].bIndex, boardMsg, BoardCompareProc);

  /*
   * Make sure you set this when editing is complete 
   * BITSET(boardList[board].bFlag, B_UNSAVED);
   */
  return boardMsg;
}

void BoardMsgDelete(LWORD board, LWORD msgNum){
  LWORD i;
  BOARDMSG *boardMsg;
  
  /* fix up reply to's and sequence numbers */
  for (i=0; i<boardList[board].bIndex.iNum; i++) {
    boardMsg = BoardMsg(boardList[board].bIndex.iThing[i]);
    if (boardMsg->bReplyTo == msgNum) {
      boardMsg->bReplyTo = BoardMsg(boardList[board].bIndex.iThing[msgNum])->bReplyTo;
    } else if (boardMsg->bReplyTo > msgNum) {
      boardMsg->bReplyTo--;
    }
    if (boardMsg->bSequence > msgNum)
      boardMsg->bSequence--;
  } 
  IndexDelete(&boardList[board].bIndex, boardList[board].bIndex.iThing[msgNum], BoardCompareProc);
  BITSET(boardList[board].bFlag, B_UNSAVED);
}

BYTE *BoardMsgGetHeader(BYTE *buf, LWORD board, LWORD msgNum) {
  LWORD      reply;
  BOARDMSG  *boardMsg;
  struct tm *theTm;

  boardMsg = BoardMsg(boardList[board].bIndex.iThing[msgNum]);

  theTm = localtime(&boardMsg->bCreateTime);
  sprintf(buf, "^c%02d/%02d ", theTm->tm_mon+1, theTm->tm_mday);

  reply = boardMsg->bReplyTo;
  while (reply != BOARD_NOTREPLY) {
    strcat(buf, "  ");
    reply = BoardMsg(boardList[board].bIndex.iThing[reply])->bReplyTo;
  }

  sprintf(buf+strlen(buf),
    "^g%s ^Y(^y%s^Y)\n", 
    boardMsg->bTitle->sText,
    boardMsg->bAuthor->sText);

  return buf;
}

void BoardNotify(LWORD board) {
  LWORD i;
  LWORD beep;

  beep = FALSE;
  for (i=0; i<boardList[board].bIndex.iNum; i++) {
    if ( BoardMsg(boardList[board].bIndex.iThing[i])->bText->sNum != STR_PRIVATE
      && BoardMsg(boardList[board].bIndex.iThing[i])->bNotify) {
      BoardMsg(boardList[board].bIndex.iThing[i])->bNotify = FALSE;
      SendChannel("^:BRD: [", NULL, SP_CHANBOARD);
      SendChannel(boardList[board].bName->sText, NULL, SP_CHANBOARD);
      SendChannel("] ", NULL, SP_CHANBOARD);
      SendChannel(BoardMsg(boardList[board].bIndex.iThing[i])->bAuthor->sText, NULL, SP_CHANBOARD);
      SendChannel(" has written a note\n", NULL, SP_CHANBOARD);

      /* search all objectIndex for board objects and emit a beep? */
      beep = TRUE;
    }
  }

  if (beep) {
    for (i=0; i<objectIndex.iNum; i++) {
      /* Check otype */
      if (Obj(objectIndex.iThing[i])->oTemplate->oType != OTYPE_BOARD) continue;
      /* check board vnum */
      if ( OBJECTGETFIELD(objectIndex.iThing[i], OF_BOARD_BVIRTUAL) != boardList[board].bVirtual) continue;
      /* Skip it if its not in a room/player */
      if (!Base(objectIndex.iThing[i])->bInside) continue;
      if (Base(objectIndex.iThing[i])->bInside->tType==TTYPE_WLD) {
        SendAction("^b$n beeps quietly\n",   
          objectIndex.iThing[i], NULL, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      }
      if (Base(objectIndex.iThing[i])->bInside->tType>=TTYPE_CHARACTER) {
        SendAction("^b$N beeps quietly\n",   
          Base(objectIndex.iThing[i])->bInside, objectIndex.iThing[i], SEND_SRC|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      }
    }
  }
}

void BoardIdle(void) {
  LWORD i;

  if (boardListFlag)
    BoardTableWrite();

  for (i=0; i<boardListMax; i++) {
    if (BIT(boardList[i].bFlag, B_UNSAVED)) {
      /* Notification */
      BoardNotify(i);
      BoardMsgDeleteCheck(i);
      BoardWrite(i);
    }
  }
}

void BoardShow(BYTE board, THING *thing) {
  BYTE       buf[256];
  LWORD      i;

  /* 
   * turf any over the limit or cancelled messages
   * Actually, this should never do anything here, 
   * The call in BoardIdle should catch it and do it there
   * or at boot time it will be caught by the call in BoardRead
   */
  BoardMsgDeleteCheck(board);

  sprintf(buf, 
    "^gBoard Title:^G[^b%s^G]\n",
    boardList[board].bName->sText);
  SendThing(buf, thing);
  for (i=0; i<boardList[board].bIndex.iNum; i++) {
    /* sprintf(buf, "^r%2ld. ", i+1); */
    /* realized it would be better if we can observe seq.# screwups */
    sprintf(buf, "^r%2hd. ", BoardMsg(boardList[board].bIndex.iThing[i])->bSequence+1); 
    SendThing(buf, thing);
    BoardMsgGetHeader(buf, board, i);
    SendThing(buf, thing);
  }
  sprintf(buf, "^g%ld ^pMessages shown.\n", boardList[board].bIndex.iNum);
  SendThing(buf, thing);
}

void BoardShowMessage(BYTE board, LWORD msgNum, THING *thing) {
  BOARDMSG  *boardMsg;
  struct tm *theTm;
  BYTE       buf[256];

  /* 
   * turf any over the limit or cancelled messages
   * Actually, this should never do anything here, 
   * The call in BoardIdle should catch it and do it there
   * or at boot time it will be caught by the call in BoardRead
   */
  BoardMsgDeleteCheck(board);

  boardMsg = BoardMsg(boardList[board].bIndex.iThing[msgNum]);
  sprintf(buf, "^gBoard Title:    ^G[^c%s^G]\n", boardList[board].bName->sText); 
  SendThing(buf, thing);
  theTm = localtime(&boardMsg->bCreateTime);
  sprintf(buf, "^gCreated:        ^G[^C%02d^b/^C%02d^G]\n",
    theTm->tm_mon+1,
    theTm->tm_mday);
  SendThing(buf, thing);
  theTm = localtime(&boardMsg->bLastRead);
  sprintf(buf, "^gLast Read:      ^G[^C%02d^b/^C%02d^G]\n", 
    theTm->tm_mon+1,
    theTm->tm_mday);
  SendThing(buf, thing);
  sprintf(buf, "^gMessage Number: ^G[^C%ld^b/^C%ld^G]\n", msgNum+1, boardList[board].bIndex.iNum); 
  SendThing(buf, thing);
  sprintf(buf, "^gAuthor:         ^G[^w%s^G]\n",
    boardMsg->bAuthor->sText);
  SendThing(buf, thing);
  SendThing(   "^gMessage Title:  ^G[^c", thing);
  SendThing(boardMsg->bTitle->sText, thing);
  SendThing("^G]\n", thing);
  SendThing("^G------------------------------------------------------------------------------\n^b", thing);
  SendThing(boardMsg->bText->sText, thing);
  SendThing("\n", thing);

  boardMsg->bLastRead = time(0);
}
