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
#include "help.h"
#include "queue.h"
#include "base.h"
#include "char.h"
#include "player.h"
#include "parse.h"
HELPLIST *helpList = NULL;
INDEX     helpIndex;

BYTE  helpReadLog;
LWORD helpNum;
LWORD helpListMax = 0;
LWORD helpListByte = HELP_TABLE_SIZE;

/* BEGIN - INTERNAL FUNCTIONS */

/* Help entries are inserted into their index in alphabetical order,
   just for neatness's sake, I cant binary search because there can be
   multiple keywords for each entry
 */

/* returns same values as strcmp */
INDEXPROC(HelpCompareProc) { /* BYTE IndexProc(void *index1, void *index2) */
  return STRICMP( Help(index1)->hKey->sText, Help(index2)->hKey->sText );
}

/* END - INTERNAL FUNCTIONS */

BYTE *helpFlagList[]= {
  "UNSAVED",
  "GODONLY",
  ""
};

void HelpInit(void) {
  FILE *helpFile;
  BYTE  buf[256];
  BYTE  tmp[128];

  IndexInit(&helpIndex, HELP_INDEX_SIZE, "helpIndex(help.c)", 0);
  helpReadLog     = INILWordRead("crimson2.ini", "helpReadLog",  0);

  helpFile = fopen("help/help.tbl", "rb");
  if (!helpFile) {
    Log(LOG_BOOT, "Unable to read help.tbl file, killing server\n");
    exit(ERROR_BADFILE);
  }

  /* report on initialization */
  sprintf(buf, "Initial helpList allocation of %ld entries\n", helpListByte/sizeof(HELPLIST));
  Log(LOG_BOOT, buf);
  sprintf(buf, "Initial helpIndex allocation of %d entries\n", HELP_INDEX_SIZE/sizeof(HELP*));
  Log(LOG_BOOT, buf);

  /* okay we opened it up so read it.... */
  while (!feof(helpFile)) {
    fscanf(helpFile, " %s", tmp); /* get file name */
    if (tmp[0] == '$' || feof(helpFile))
      break; /* Dikumud file format EOF character */
    if (tmp[0] != '#' && tmp[0] != '*') { /* ignore comments */
      REALLOC("HelpInit(help.c): helpList reallocation\n", helpList, HELPLIST, helpListMax+1, helpListByte);
      memset( (void*)&helpList[helpListMax], 0, sizeof(HELPLIST)); /* init to zeros */
      helpList[helpListMax].hFileName = STRCREATE(tmp);
      /*fscanf(helpFile, " "); */ /* strip out spaces */
      /*helpList[helpListMax].hEditor = FileStrRead(helpFile);*/
      fscanf(helpFile," %s",tmp); /* get editor */
      helpList[helpListMax].hEditor = STRCREATE(tmp);
      helpList[helpListMax].hNum = 0;
      helpList[helpListMax].hFlag = FileFlagRead(helpFile,helpFlagList);
      fscanf(helpFile, " %s", tmp); /* get stupid ~ at the end */

      /* load the help file for this section */
      HelpRead(helpListMax);

      if (helpReadLog) {
        sprintf(buf,
          "Read Help Section:[%-18s] Hlp[%5hd] [%s]\n",
          helpList[helpListMax].hFileName->sText,
          helpList[helpListMax].hNum,
          helpList[helpListMax].hEditor->sText
        );
        Log(LOG_BOOT, buf);
      }

      helpListMax++;
    } else { /* read comment */
      fgets(buf, 256, helpFile);
    }
  }
  /* all done close up shop */
  fclose(helpFile);

  sprintf(buf,
    "Help Totals: Sections[%2ld] Entries[%5ld]\n\n",
    helpListMax,
    helpNum
   );
  Log(LOG_BOOT, buf);
}


void HelpRead(BYTE section) {
  FILE  *helpFile;
  BYTE   buf[256];
  BYTE   tmp[256];
  HELP  *help;

  sprintf(buf, "help/%s", helpList[section].hFileName->sText);
  helpFile = fopen(buf, "rb");
  if (!helpFile) {
    sprintf(buf, "Unable to read help/%s, skipping\n", helpList[section].hFileName->sText);
    Log(LOG_BOOT, buf);
    helpList[section].hName = StrAlloc(helpList[section].hFileName);
    return;
  }
  helpList[section].hName = FileStrRead(helpFile);

  /* okay we opened it up so read it.... */
  while (!feof(helpFile)) {
    fgets(tmp, 256, helpFile); /* get filename */
    if (tmp[0] == '$' || feof(helpFile))
      break; /* Dikumud file format EOF character */
    if (tmp[0] == '#') { /* ignore comments */
      MEMALLOC(help, HELP, HELP_ALLOC_SIZE);
      help->hSection = section;
      help->hKey = FileStrRead(helpFile);
      help->hDesc = FileStrRead(helpFile);

      /* load the help file for this section */
      helpList[help->hSection].hNum++;
      IndexInsert(&helpIndex, help, HelpCompareProc);
      if (indexError) {
        sprintf(buf, "For file:%s offset:%ld\n", helpList[section].hFileName->sText, ftell(helpFile));
        Log(LOG_ERROR, buf);
        Log(LOG_ERROR, "Duplicate Index for help entry:\n");
        Log(LOG_ERROR, help->hKey->sText);
        LogPrintf(LOG_ERROR, "\n");
      }

      helpNum++;
    } else {
      LWORD helpPos;
      LWORD i;

      sprintf(buf, "Illegal character found near line:\n");
      Log(LOG_ERROR, buf);
      Log(LOG_ERROR, tmp);
      helpPos = ftell(helpFile);
      for (i=0; i<5; i++) {
        if (feof(helpFile)) break;
        tmp[0]=0;
        fgets(tmp, 256, helpFile); /* get filename */
        LogPrintf(LOG_ERROR, tmp);
      }
      LogPrintf(LOG_ERROR, "\n");
      fseek(helpFile, helpPos, SEEK_SET);
    }
  }
  /* all done close up shop */
  fclose(helpFile);
}

BYTE HelpWrite(BYTE section) {
  LWORD i;
  FILE *helpFile;
  BYTE  buf[256];

  sprintf(buf, "help/%s", helpList[section].hFileName->sText);
  helpFile = fopen(buf, "wb");
  if (!helpFile) {
    Log(LOG_ERROR, "Unable to write help section file\n");
    return FALSE;
  }
  BITCLR(helpList[section].hFlag, H_UNSAVED);
  /* first write the title of our help file */
  FileStrWrite(helpFile,helpList[section].hName);
  for (i=0; i<helpIndex.iNum; i++) {
    if (Help(helpIndex.iThing[i])->hSection == section) {
      fwrite("#\n",1,2,helpFile);
      FileStrWrite(helpFile, Help(helpIndex.iThing[i])->hKey);
      FileStrWrite(helpFile, Help(helpIndex.iThing[i])->hDesc);
    }
  }
  fwrite("$\n",1,2,helpFile);
  fclose(helpFile);
  return TRUE;
}

HELP *HelpFind(BYTE *key) {
  LWORD i;

  /* Linear search because there can be multiple keywords per entry */
  for (i=0; i<helpIndex.iNum; i++)
    if (StrIsExactKey(key, Help(helpIndex.iThing[i])->hKey))
      return Help(helpIndex.iThing[i]);

  /* Linear search because there can be multiple keywords per entry */
  for (i=0; i<helpIndex.iNum; i++)
    if (StrIsKey(key, Help(helpIndex.iThing[i])->hKey))
      return Help(helpIndex.iThing[i]);

  return NULL;
}

HELP *HelpFree(HELP *help) {
  helpList[help->hSection].hNum--;
  BITSET(helpList[help->hSection].hFlag, H_UNSAVED);
  IndexDelete(&helpIndex, help, HelpCompareProc);
  MEMFREE(help, HELP);
  return NULL;
}

BYTE HelpParse(THING *thing, BYTE *cmd, BYTE *defaultCmd) {
  HELP  *help = NULL;
  BYTE   buf[256];

  cmd = StrOneWord(cmd, buf);
  if (!StrAbbrev("?", buf) && !StrAbbrev("help", buf))
    return FALSE;
  
  if (*cmd) {
    help = HelpFind(cmd);

    /* check to see if it's a god-only help section */
    if (help && BITANY(helpList[help->hSection].hFlag,H_GODONLY)) {
      if (thing->tType==TTYPE_PLR) {
        if (!ParseCommandCheck(PARSE_COMMAND_WGOTO,BaseControlFind(thing),"")) {
          SendThing("^wNo help available on that topic\n", thing);
	  /*SendThing("^wUnfortunately that help topic is reserved for ^rGOD EYES ONLY^w.\n",thing);*/
	  return TRUE;
	}
      }
    }

    /* show socials if they type help socials */
    if (!help) {
      StrOneWord(cmd, buf);
      if (StrAbbrev("socials", buf)) {
        SocialParse(thing, "socials");
        return TRUE;
      }
    }
  } else {
    /* show default help topic */
    if (defaultCmd && *defaultCmd)
      help = HelpFind(defaultCmd);
  }
  if (!help) {
    SendThing("^wNo help available on that topic\n", thing);
    return TRUE;
  }

  sprintf(buf, "^bMatch found in Section ^B[^w%-18s^B]\n", helpList[help->hSection].hName->sText);
  SendThing(buf, thing);
  SendThing("^bKeys: [^c", thing);
  SendThing(help->hKey->sText, thing);
  SendThing("^b]\n^C", thing);
  SendThing(help->hDesc->sText, thing);
  return TRUE;
}
