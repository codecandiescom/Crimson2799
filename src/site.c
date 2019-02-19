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
#include <sys/types.h>  /* for set sockopt etc */
#ifndef WIN32
  #include <sys/time.h>   /* for select/timezone etc */
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
#ifdef WIN32
  /* including winsock.h etc auto includes the windows types */
  /* turn off warning: benign type redefinition */
  #pragma warning( disable : 4142 )
  #include <winsock.h>
  #pragma warning( default : 4142 )
  #include "winsockext.h"
#endif

#include "site.h"

INDEX    siteIndex;
LWORD    siteReadLog;

BYTE *sTypeList[] = {
  "OFFSITE",
  "LOCAL",
  "BANNED",
  "BANNEW",
  ""
};

/* returns same values as strcmp */
INDEXPROC(SiteCompareProc) { /* BYTE IndexProc9void *index1, void *index2) */
  return STRICMP( Site(index1)->sSite, Site(index2)->sSite );
}

INDEXFINDPROC(SiteFindProc) { /* BYTE IndexProc(void *key, void *index) */
  if (!STRNICMP( Site(index)->sSite, key, strlen(Site(index)->sSite) ))
    return 0;

  /* keep looking */
  return -1;
}

void SiteInit(void) {
  BYTE buf[256];

  IndexInit(&siteIndex, SITE_INDEX_SIZE, "siteIndex(site.c)", 0);
  sprintf(buf, "Initial siteIndex allocation of %d entries\n", SITE_INDEX_SIZE/sizeof(SITE*));
  Log(LOG_BOOT, buf);
  siteReadLog = INILWordRead("crimson2.ini", "siteReadLog", 1);

  SiteRead();

  sprintf(buf,
    "Total Number of Special Sites: [%3ld]\n",
    siteIndex.iNum
  );
  Log(LOG_BOOT, buf);
  if (siteReadLog) {
    Log(LOG_BOOT, "\n");
  }
}

SITE *SiteAlloc(BYTE *siteName, BYTE type, LWORD date, STR *player) {
  SITE *site;

  MEMALLOC(site, SITE, SITE_BLOCK_SIZE);
  site->sType = type;
  strcpy(site->sSite, siteName);
  site->sDate = date;
  site->sPlayer = player;

  return site;
}

SITE *SiteCreate(THING *thing, BYTE *siteName, BYTE type) {
  SITE *site;

  site = SiteAlloc(siteName, type, time(0), StrAlloc(thing->tSDesc));
  IndexInsert(&siteIndex, site, SiteCompareProc);

  return site;
}

SITE *SiteFree(SITE *site) {
  IndexDelete(&siteIndex, site, SiteCompareProc);
  STRFREE(site->sPlayer);
  MEMFREE(site, SITE);
  return NULL;
}

/* find an exact match */
SITE *SiteFindExact(BYTE *key) {
  return IndexFind(&siteIndex, key, SiteFindProc);
}

/* find a match is sSite is the ending for key since we want to be able to
  specify .baddomain.com say, and find a match for all the machines in 
  domain, ie machine1.badddomain.com, machine2.baddomain.com etc etc
*/
SITE *SiteFind(BYTE *key) {
  LWORD  kLen;
  LWORD  sLen;
  LWORD  offset;
  LWORD  i;

  for (i=0; i<siteIndex.iNum; i++) {
    if (Site(siteIndex.iThing[i])->sSite[0] == '.') {
      sLen = strlen(Site(siteIndex.iThing[i])->sSite);
      kLen = strlen(key);
      offset = kLen - sLen;
      if (offset>=0) {
        if (!STRICMP( key+offset, Site(siteIndex.iThing[i])->sSite ))
          return Site(siteIndex.iThing[i]);
      }
    } else {
      if (!STRICMP( key, Site(siteIndex.iThing[i])->sSite ))
        return Site(siteIndex.iThing[i]);
    }
  }
  return NULL;
}

BYTE SiteGetType(SOCK *sock) {
  SITE *site;
  
  /* lookup based on address first */
  site = SiteFind(sock->sSiteIP);
  if (!site) {
    /* now try site name */
    site = SiteFind(sock->sSiteName);
  }
  if (!site)
    return SITE_OFFSITE; /* not found */

  return site->sType;
}

void SiteRead(void) {
  FILE   *siteFile;
  BYTE    siteType;
  STR    *sitePlayer;
  LWORD   siteDate;
  BYTE    buf[256];
  BYTE    siteTime[256];
  SITE   *site;

  IndexClear(&siteIndex);
  siteFile = fopen(SITE_FILE, "r");
  if (!siteFile) {
    site = SiteAlloc("127.0.0.1", SITE_LOCAL, time(0), STRCREATE("<SYSTEM>"));
    IndexInsert(&siteIndex, Thing( site ), SiteCompareProc);
    Log(LOG_BOOT, "Creating an empty site table\n");
    SiteWrite();
    return;
  }

  while (!feof(siteFile)) {
    fscanf(siteFile, " %s", buf);
    if (buf[0] == '#') {
      fgets(buf, sizeof(buf), siteFile);
      continue;
    } else if (buf[0] == '$') {
      break;
    } else {
      /* read entry data */
      siteType = FILETYPEREAD(siteFile, sTypeList);
      TYPECHECK(siteType, sTypeList);
      fscanf(siteFile, " %ld ", &siteDate);
      sitePlayer = FileStrRead(siteFile);

      /* insert entry */
      site = SiteAlloc(buf, siteType, siteDate, sitePlayer);
      IndexInsert(&siteIndex, Thing( site ), SiteCompareProc);

      if (siteReadLog) {
        strcpy(siteTime, ctime(&site->sDate));
        siteTime[strlen(siteTime)-1] = '\0';
        sprintf(buf, 
          "%-25s %-7s %-25s %s\n",
          site->sSite,
          sTypeList[site->sType],
          siteTime,
          site->sPlayer->sText
        );
        Log(LOG_BOOT, buf);
      }
    }
  }

  fclose(siteFile);
}

void SiteWrite(void) {
  FILE *siteFile;
  LWORD i;
  SITE *site;

  siteFile = fopen(SITE_FILE, "wb");
  if (!siteFile) {
    Log(LOG_ERROR, "SiteWrite failed\n");
    PERROR("SiteWrite");
    return;
  }

  for (i=0; i<siteIndex.iNum; i++) {
    site = Site(siteIndex.iThing[i]);
    fprintf(siteFile, 
      "%-25s %-7s %-15ld %s~\n",
      site->sSite,
      sTypeList[site->sType],
      site->sDate,
      site->sPlayer->sText
    );
  }
  fprintf(siteFile, "$\n");

  fclose(siteFile);
}

