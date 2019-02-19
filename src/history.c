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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "crimson2.h"
#include "str.h"
#include "index.h"
#include "queue.h"
#include "edit.h"
#include "alias.h"
#include "history.h"
#include "socket.h"
#include "send.h"

/* History buf format is:
  command\0command\0command\0
  |                        |
 hStart                   hEnd
*/

void HistoryClear(HISTORY *history) {
  history->hBuf[0] = '\0';
  history->hStart = 0;
  history->hEnd = 0;
  history->hFirstCommand = 1;
}

void HistoryIncr(LWORD *index) {
  *index += 1;
  if (*index >= HISTORY_SIZE) *index = 0;
}

void HistoryDecr(LWORD *index) {
  *index -= 1;
  if (*index >= HISTORY_SIZE) *index = 0;
}

void HistoryAdvance(HISTORY *history, LWORD *index) {
  if (*index == history->hEnd) return;

  /* find end of this command */
  while (history->hBuf[*index] != '\0')
    HistoryIncr(index);

  /* advance to next command */
  if (*index == history->hEnd) return;
  HistoryIncr(index);
}

void HistoryAdd(HISTORY *history, BYTE *cmd) {
  if (history->hEnd != history->hStart)
    HistoryIncr(&history->hEnd);
  while (*cmd) {
    history->hBuf[history->hEnd] = *cmd;
    cmd++;
    HistoryIncr(&history->hEnd);
    /* If end overran start - advance to next cmd */
    if (history->hEnd == history->hStart) {
      HistoryAdvance(history, &history->hStart);
      history->hFirstCommand += 1;
      if (history->hFirstCommand > 999) history->hFirstCommand = 1;
    }
  }
  /* Copy \0 too */
  history->hBuf[history->hEnd] = '\0';
}

/* return last command in history */
BYTE *HistoryGetLast(HISTORY *history) {
  LWORD last = history->hEnd;

  /* Is the history empty */
  if (last == history->hStart)
    return (&history->hBuf[history->hStart]);

  /* find end of previous command */
  do {
    HistoryDecr(&last);
    if (last == history->hStart)
      return (&history->hBuf[history->hStart]);
  } while (history->hBuf[last] != '\0');

  /* advance from prev command to start of last cmd */
  HistoryIncr(&last);

  return (&history->hBuf[last]);
}

void HistoryList(HISTORY *history, SOCK *sock) {
  LWORD count = history->hFirstCommand;
  LWORD command = history->hStart;
  BYTE  buf[16];
  
  while (command != history->hEnd) {
    sprintf(buf, "%3ld ", count);
    SEND(buf, sock);
    SEND(&history->hBuf[command], sock);
    SEND("\n", sock);
    count++;
    HistoryAdvance(history, &command);
  }
}

void HistorySend(HISTORY *history, THING *thing) {
  LWORD count = history->hFirstCommand;
  LWORD command = history->hStart;
  
  while (command != history->hEnd) {
    SendThing(&history->hBuf[command], thing);
    count++;
    HistoryAdvance(history, &command);
  }
}

/* Overwrites cmd buf to execute historical command */
LWORD HistoryParse(HISTORY *history, SOCK *sock, BYTE *cmd) {
  LWORD i;
  LWORD command = history->hStart;
  BYTE  buf[256];

  StrOneWord(cmd, buf);
  if (StrAbbrev("history", buf)) {
    HistoryList(history, sock);
    HistoryAdd(history, cmd);
    return TRUE;
  }

  if (cmd[0] == '!') {
    if (cmd[1]=='\0' || cmd[1]=='!') {
      strcpy(cmd, HistoryGetLast(history));
    } else if (isdigit(cmd[1])) {
      i = atol(cmd+1);
      i -= history->hFirstCommand;
      while (i && command!=history->hEnd) {
        HistoryAdvance(history, &command);
        i--;
      }
      strcpy(cmd, &history->hBuf[command]);
    } else {
      /* Search for match in history */
      strcpy(buf, cmd+1);
      while (command!=history->hEnd) {
        if (StrAbbrev(&history->hBuf[command], buf)) {
          strcpy(cmd, &history->hBuf[command]);
        }
        HistoryAdvance(history, &command);
      }
    }
  }
  return FALSE;
}
