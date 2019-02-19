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
#include <string.h>    /* for memset et al */
#include <ctype.h>    /* for memset et al */
#include <time.h>
#ifndef WIN32
  #include <sys/time.h>   /* for select/timezone etc */
  #include <sys/types.h>  /* for set sockopt etc */
  #include <sys/socket.h>
  #include <netinet/in.h>
#endif

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "ini.h"
#include "mem.h"
#include "queue.h"
#include "str.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h" /* this can point to things */
#include "base.h"   /* has links that point to sockets */
#include "char.h"
#include "mobile.h"
#include "player.h"
#include "file.h"
#include "send.h"
#include "alias.h"

#ifdef WIN32
  /* including winsock.h etc auto includes the windows types */
  /* turn off warning: benign type redefinition */
  #pragma warning( disable : 4142 )
  #include <winsock.h>
  #pragma warning( default : 4142 )
#endif

#define ALIAS_MAX_PARM 9

LWORD aliasMax;

/* returns same values as strcmp */
INDEXPROC(AliasCompareProc) { /* BYTE IndexProc9void *index1, void *index2) */
  return STRICMP( Alias(index1)->aPattern->sText, Alias(index2)->aPattern->sText );
}

INDEXFINDPROC(AliasFindProc) { /* BYTE IndexProc(void *key, void *index) */
  return STRICMP( key, Alias(index)->aPattern->sText );
}

void AliasInit() {
  aliasMax = INILWordRead("crimson2.ini", "aliasMax", 30);
}

ALIAS *AliasAlloc(STR *pattern, BYTE parmNum, STR *action) {
  ALIAS *alias;

  MEMALLOC(alias, ALIAS, ALIAS_BLOCK_SIZE);
  alias->aPattern = pattern;
  alias->aParmNum = parmNum;
  alias->aAction = action;

  return alias;
}

ALIAS *AliasCreate(INDEX *aIndex, BYTE *pattern, BYTE *action) {
  BYTE   buf[256];
  BYTE  *strWalk;
  BYTE   parmCount;
  ALIAS *alias;
  
  if (aIndex->iNum >= aliasMax) return NULL;
  /* take action apart - count highest parm they use */
  strWalk = action;
  do {
    strWalk = StrOneWord(strWalk, buf);
    if (buf[0] == '%' && buf[2] == '\0' && isdigit(buf[1]))
      parmCount = atoi(buf+1);
  } while(*buf);
  
  /* Create the alias */
  alias = AliasAlloc(STRCREATE(pattern), parmCount, STRCREATE(action));
  IndexInsert(aIndex, alias, AliasCompareProc);

  return alias;
}

ALIAS *AliasFree(INDEX *aIndex, ALIAS *alias) {
  IndexDelete(aIndex, alias, AliasCompareProc);
  STRFREE(alias->aPattern);
  STRFREE(alias->aAction);
  MEMFREE(alias, ALIAS);
  return NULL;
}

ALIAS *AliasFind(INDEX *aIndex, BYTE *pattern) {
  return IndexFind(aIndex, pattern, AliasFindProc);
}

void AliasParseLine(BYTE *line, BYTE *action, BYTE parm[ALIAS_MAX_PARM][256]) {
  BYTE  parmNum[2];
  LWORD i;

  parmNum[1] = '\0';
  for (; *action==' '; action++);
  while (*action) {
    if (*action == '%' 
     && isdigit(*(action+1))
     && ((*(action+2)==' ') || (!*(action+2)))
    ) {
      parmNum[0] = *(action+1);
      i = atol(parmNum) - 1;
      BOUNDSET(0, i, ALIAS_MAX_PARM - 1);
      sprintf(line, "%s", parm[i]);
      while (*line) line++; /* find new null */
      action += 2;
    } else {
      *line = *action;
      line++;
      action++;
    }
  }
  *line = '\0';
}

BYTE AliasParse(SOCK *sock, BYTE *cmd) {
  BYTE   buf[512];
  BYTE   action[512];
  BYTE   parm[ALIAS_MAX_PARM][256];
  LWORD  i;
  ALIAS *alias;

  if (!strncmp(cmd, ALIAS_COMMAND_STR, strlen(ALIAS_COMMAND_STR))) 
    return FALSE;

  cmd = StrOneWord(cmd, buf);
  alias = AliasFind(&sock->sAliasIndex, buf);
  if (!alias) return FALSE;
  if (!alias->aAction->sLen) return FALSE;
  /* get the parms we need */
  for (i=0; i<ALIAS_MAX_PARM; i++)
    if (i<alias->aParmNum)
      cmd = StrOneWord(cmd, parm[i]); 
    else
      *parm[i] = '\0';
  i = alias->aAction->sLen;
  strcpy(action, alias->aAction->sText);
  while (i>0) {
    while (i>0 && action[i]!=';') i--;
    if (action[i] == ';') {
      action[i] = '\0';
      AliasParseLine(buf, action+i+1, parm);
    } else {
      AliasParseLine(buf, action+i, parm);
    }
    if (*cmd) {
      strcat(buf, " ");
      strcat(buf, cmd); /* tack whatever extra they typed on */
    }
    strcat(buf, "\n");
    QInsert(sock->sIn, buf);
    QInsert(sock->sIn, ALIAS_COMMAND_STR);
  }
  return TRUE;
}

void AliasRead(SOCK *sock) {
  LWORD i;
  BYTE  aliasFileBuf[128];
  FILE *aliasFile;
  BYTE  buf[256];
  BYTE *action;
  
  if (!sock || !sock->sHomeThing) return;
  IndexClear(&sock->sAliasIndex);

  sprintf(aliasFileBuf, "alias/%s.als", sock->sHomeThing->tSDesc->sText);
  StrToLower(aliasFileBuf);
  
  aliasFile = fopen(aliasFileBuf, "rb");
  if (!aliasFile) {
    /* I decided not to create an empty alias file for each player
     * so this probably isnt an error, it just means they have never
     * made an alias for themselves
     */
    /*
    sprintf(buf, "Unable to read %s\n", aliasFileBuf);
    Log(LOG_ERROR, buf);
    PERROR("AliasRead");
    */
    return;
  }
  
  fgets(buf, sizeof(buf), aliasFile);
  while (! (buf[0] == '$' && buf[1] == '~') ) {
    /* trim crap off end of action/line just read */
    for (i=strlen(buf)-1; i>=0 && (buf[i]=='\n' || buf[i]=='\r' || buf[i]==' '); i--) 
      buf[i] = '\0';
      
    /* look for equals sign separating pattern from action */
    for (i=0; buf[i] && buf[i]!='='; i++);
    if (buf[i] == '=') {
      buf[i] = '\0';
      action = buf + i + 1;
      /* trim crap off end of pattern */
      for (i=strlen(buf)-1; i>=0 && (buf[i]=='\n' || buf[i]=='\r' || buf[i]==' '); i--) 
        buf[i] = '\0';
      AliasCreate(&sock->sAliasIndex, buf, action);
    }
    /* read next line */
    fgets(buf, sizeof(buf), aliasFile);
  }
  
  fclose(aliasFile);
}

void AliasWrite(SOCK *sock) {
  LWORD i;
  BYTE  aliasFileBuf[128];
  FILE *aliasFile;
  BYTE  buf[256];
  
  if (!sock || !sock->sHomeThing) return;

  sprintf(aliasFileBuf, "alias/%s.als", sock->sHomeThing->tSDesc->sText);
  StrToLower(aliasFileBuf);
  
  aliasFile = fopen(aliasFileBuf, "wb");
  if (!aliasFile) {
    sprintf(buf, "Unable to write %s\n", aliasFileBuf);
    Log(LOG_ERROR, buf);
    PERROR("AliasWrite");
    return;
  }
  
  for (i = 0; i<sock->sAliasIndex.iNum; i++) {
    fprintf(aliasFile, 
      "%-10s=%s\n",
      Alias(sock->sAliasIndex.iThing[i])->aPattern->sText,
      Alias(sock->sAliasIndex.iThing[i])->aAction->sText);
  }
  
  fprintf(aliasFile, "$~\n");
  fclose(aliasFile);
}
