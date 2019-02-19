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
#include "base.h"
#include "char.h"
#include "parse.h"
#include "social.h"


INDEX     socialIndex;
LWORD     socialReadLog;

/* returns same values as strcmp */
INDEXPROC(SocialCompareProc) { /* BYTE IndexProc(void *index1, void *index2) */
  return STRICMP( Social(index1)->sName->sText, Social(index2)->sName->sText );
}
INDEXFINDPROC(SocialFindProc) { /* BYTE IndexProc(void *key, void *index) */
  if (StrAbbrev( Social(index)->sName->sText, (BYTE*)key ))
    return 0;
  else 
    return STRICMP( key, Social(index)->sName->sText );
}

/* END - INTERNAL FUNCTIONS */

void SocialInit(void) {
  BYTE  buf[256];

  IndexInit(&socialIndex, SOCIAL_INDEX_SIZE, "socialIndex(social.c)", 0);
  sprintf(buf, "Initial socialIndex allocation of %d entries\n", SOCIAL_INDEX_SIZE/sizeof(THING *));
  Log(LOG_BOOT, buf);
  socialReadLog = INILWordRead("crimson2.ini", "socialReadLog", 0);

  SocialRead();

  sprintf(buf,
    "Total Number of Socials: [%3ld]\n",
    socialIndex.iNum
   );
  Log(LOG_BOOT, buf);
}

void SocialRead(void) {
  BYTE           buf[256];
  BYTE          *trim;
  SOCIAL        *social = NULL;
  FILE          *msgFile;

  msgFile = fopen("msg/social.msg", "r");
  if (!msgFile) {
    Log(LOG_ERROR, "Unable to open msg/social.msg file\n");
    return;
  }
  fgets(buf, 256, msgFile);
  while (!feof(msgFile)) {
    if (buf[0]=='$') break;

    switch (buf[0]) {
      case '#':
        MEMALLOC(social, SOCIAL, SOCIAL_ALLOC_SIZE);
        memset(social, 0, sizeof(SOCIAL));
        trim = StrTrim(buf+1);
  social->sName = STRCREATE(trim);
        /* insert them alpabetically into the index */
  IndexInsert(&socialIndex, social, SocialCompareProc);
        if (socialReadLog) {
          sprintf(buf, "Reading Social: %s\n", social->sName->sText);
          Log(LOG_BOOT, buf);
        }
        /* read minimum position - anything not in the list will be interpreted as same */
        fgets(buf, sizeof(buf), msgFile);
        trim = StrTrim(buf);
        social->sPos = TYPEFIND(trim, posList);
        if (social->sPos==-1) {
          if (!StrExact(trim, "SAME")) {
            sprintf(buf, "Social: %s - Illegal Position, assuming SAME\n", social->sName->sText);
            Log(LOG_BOOT, buf);
          }
        }
        break;

      case 'N':
        if (!social) break; /* must have read this first */
  if (social->sNullSrc || social->sNullRoom) {
    sprintf(buf, "Duplicate N entry for social %s\n", social->sName->sText);
    Log(LOG_BOOT, buf);
    break;
  }
  social->sNullSrc  = FileStrRead(msgFile);
        social->sNullRoom = FileStrRead(msgFile);
  break;

      case 'C':
        if (!social) break; /* must have read this first */
  if (social->sCharSrc || social->sCharDst || social->sCharRoom) {
    sprintf(buf, "Duplicate C entry for social %s\n", social->sName->sText);
    Log(LOG_BOOT, buf);
    break;
  }
  social->sCharSrc  = FileStrRead(msgFile);
        social->sCharDst  = FileStrRead(msgFile);
        social->sCharRoom = FileStrRead(msgFile);
  break;

      case 'O':
        if (!social) break; /* must have read this first */
  if (social->sObjSrc || social->sObjRoom) {
    sprintf(buf, "Duplicate O entry for social %s\n", social->sName->sText);
    Log(LOG_BOOT, buf);
    break;
  }
  social->sObjSrc  = FileStrRead(msgFile);
        social->sObjRoom = FileStrRead(msgFile);
        break;

      case ';': /* its a comment */
        break;

      case '\0': /* its nothing */
      case '\r': /* its nothing */
      case '\n': /* its nothing */
        social = NULL;
        break;

      default: /* corrupt crap for example */
        Log(LOG_ERROR, "in [msg/social.msg] not sure what to make of:\n");
        Log(LOG_ERROR, buf);
        break; /* its a comment */
    }
    fgets(buf, 256, msgFile);
  }
}

SOCIAL *SocialFind(BYTE *key) {
  return IndexFind(&socialIndex, key, SocialFindProc);
}

BYTE SocialParse(THING *thing, BYTE *cmd) {
  SOCIAL *social = NULL;
  BYTE    buf[256];
  BYTE    srcKey[256];
  LWORD   srcNum;
  LWORD   srcOffset;
  THING  *found;
  THING  *search;
  LWORD   i;
  LWORD   count;

  cmd = StrOneWord(cmd, buf); 
  if (StrAbbrev("socials", buf)) {
    SendThing("^cAvailable Social Commands:\n", thing);
    SendThing("^C-+-+-+-+- +-+-+- +-+-+-+-\n\n", thing);
    count = 0;
    for (i=0; i<socialIndex.iNum; i++) {
      count++;
      if ( count%2 )
        strcpy(buf, "^g");
      else
        strcpy(buf, "^G");

      if ( count%5 )
        sprintf(buf+2, "%-15s", Social(socialIndex.iThing[i])->sName->sText);
      else 
        sprintf(buf+2, "%s\n", Social(socialIndex.iThing[i])->sName->sText);
      SendThing(buf, thing);
    }
    if ( count%5 )
      SendThing("\n", thing);
    sprintf(buf, "\n^CTotal Socials Available: ^p%ld\n", count);
    SendThing(buf, thing);
    return TRUE;
  }
  social = SocialFind(buf);
  if (!social)
    return FALSE;
  
  if (*cmd) {
    ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
    search = Base(thing)->bInside;
    found = CharThingFind(thing, srcKey, -1, search, TF_PLR|TF_MOB|TF_OBJ, &srcOffset);
  } else
    found = NULL;

  /* check minimum position */
  if ((social->sPos != -1) && (Character(thing)->cPos < social->sPos)) {
    sprintf(buf, "^wYou must at least be ^r%s^w to %s\n", posList[social->sPos].pName, social->sName->sText);
    SendThing(buf, thing);
    return TRUE;
  }

  if (!found) {
    if (!social->sNullSrc || !social->sNullRoom) {
      sprintf(buf, "^w%s must be directed at someone or something\n", social->sName->sText);
      SendThing(buf, thing);
      return TRUE;
    } else {
      SendAction(social->sNullSrc->sText,
     thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("\n",
     thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction(social->sNullRoom->sText,
     thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("\n",
     thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      return TRUE;
    }
  }
  
  else if (found->tType==TTYPE_PLR || found->tType==TTYPE_MOB) {
    if (!social->sCharSrc || !social->sCharDst || !social->sCharRoom) {
      sprintf(buf, "^w%s cant be directed against someone\n", social->sName->sText);
      SendThing(buf, thing);
      return TRUE;
    } else {
      if (social->sPos==-1 && Character(thing)->cPos!=Character(found)->cPos) {
        sprintf(buf, "^wBut they are ^r%s^w and you are %s\n", posList[Character(found)->cPos].pName, posList[Character(thing)->cPos].pName);
        SendThing(buf, thing);
        return TRUE;
      }
      SendAction(social->sCharSrc->sText,
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("\n",
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction(social->sCharDst->sText,
        thing, found, SEND_DST |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("\n",
        thing, found, SEND_DST |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction(social->sCharRoom->sText,
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("\n",
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      return TRUE;
    }
  }
  
  else if (found->tType==TTYPE_OBJ) {
    if (!social->sObjSrc || !social->sObjRoom) {
      sprintf(buf, "^w%s cant be directed against an object\n", social->sName->sText);
      SendThing(buf, thing);
      return TRUE;
    } else {
      SendAction(social->sObjSrc->sText,
     thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("\n",
     thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction(social->sObjRoom->sText,
     thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("\n",
     thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      return TRUE;
    }
  }
  
  return TRUE;
}
