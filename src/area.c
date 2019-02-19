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
#include "mem.h"
#include "queue.h"
#include "log.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "file.h"
#include "thing.h"
#include "exit.h"
#include "index.h"
#include "world.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "mobile.h"
#include "player.h"
#include "reset.h"
#include "area.h"


AREALIST *areaList     = NULL;
LWORD     areaListMax  = 0;       /* size in units */
LWORD     areaListByte = 1<<13;    /* size in bytes */

BYTE      areaOfLog;
BYTE      areaReadLog;
BYTE      areaTotalLog;
BYTE      areaWriteBinary;

BYTE *wSystemList[] = {
  "WLDUNSAVED",
  "MOBUNSAVED",
  "OBJUNSAVED",
  "RSTUNSAVED",
  "RSTSHOWN",
  ""
};

void AreaInit(void) {
  FILE *areaFile;
  BYTE buf[256];
  BYTE word[256];
  BYTE tmp[128];

  areaOfLog       = INILWordRead("crimson2.ini", "areaOfLog",       0);
  areaReadLog     = INILWordRead("crimson2.ini", "areaReadLog",     0);
  areaTotalLog    = INILWordRead("crimson2.ini", "areaTotalLog",    0);
  areaWriteBinary = INILWordRead("crimson2.ini", "areaWriteBinary", 0);
  if (areaWriteBinary) {
    Log(LOG_BOOT, "WARNING: BINARY FILE WRITING ENABLED\n");
  }
  areaFile = fopen("area/area.tbl", "rb");
  if (!areaFile) {
    Log(LOG_BOOT, "Unable to read area.tbl file, killing server\n");
    exit(ERROR_BADFILE);
  }

  /* report on initialization */
  sprintf(buf, "Initial areaList allocation of %ld entries\n", areaListByte/sizeof(AREALIST));
  Log(LOG_BOOT, buf);
  WorldInit();
  MobileInit();
  ObjectInit();
  ResetInit();

  /* okay we opened it up so read it.... */
  while (!feof(areaFile)) {
    fscanf(areaFile, " %s", tmp); /* get filename */
    if (tmp[0] == '$' || feof(areaFile))
      break; /* Dikumud file format EOF character */
    if (tmp[0] != '#' && tmp[0] != '*') { /* ignore comments */
      REALLOC("AreaInit(area.c): areaList reallocation\n", areaList, AREALIST, areaListMax+1, areaListByte);
      memset( (void*)&areaList[areaListMax], 0, sizeof(AREALIST)); /* init to zeros */
      areaList[areaListMax].aFileName = STRCREATE(tmp);
      fscanf(areaFile, " %ld", &(areaList[areaListMax].aVirtualMin) ); /* get Min */
      fscanf(areaFile, " %ld ", &(areaList[areaListMax].aVirtualMax) ); /* get Max */
      areaList[areaListMax].aEditor = FileStrRead(areaFile);
      areaList[areaListMax].aDesc   = STRCREATE("No description.");

      /* init the index values (used for binary searches) */
      sprintf(buf, "[%s] WldIndex",areaList[areaListMax].aFileName->sText); 
      IndexInit(&areaList[areaListMax].aWldIndex, 128, buf, 0);

      sprintf(buf, "[%s] MobIndex",areaList[areaListMax].aFileName->sText); 
      IndexInit(&areaList[areaListMax].aMobIndex, 128, buf, 0);

      sprintf(buf, "[%s] ObjIndex",areaList[areaListMax].aFileName->sText); 
      IndexInit(&areaList[areaListMax].aObjIndex, 128, buf, 0);

      areaList[areaListMax].aResetList       = NULL;
      areaList[areaListMax].aResetNum        = 0;
      areaList[areaListMax].aResetByte       = 128;
      areaList[areaListMax].aResetFlag       = 0;
      areaList[areaListMax].aResetDelay      = 0;
      areaList[areaListMax].aResetLast       = 0;

      if (areaReadLog) {
        sprintf(buf,
          "Reading Area:[%-18s] [%5ld-%5ld] [%s]\n",
          StrFirstWord(areaList[areaListMax].aFileName->sText,word),
          areaList[areaListMax].aVirtualMin,
          areaList[areaListMax].aVirtualMax,
          areaList[areaListMax].aEditor->sText
        );
        Log(LOG_BOOT, buf);
      }

      /* load the world file for this area */
      WorldRead(areaListMax);
      /* load the mobile templates for this area */
      MobileRead(areaListMax);
      /* load the object templates for this area */
      ObjectRead(areaListMax);
      /* load the reset files for this area */
      ResetRead(areaListMax);
      
      areaList[areaListMax].aResetThing.tSDesc = StrAlloc(areaList[areaListMax].aFileName);
      areaList[areaListMax].aResetThing.tDesc  = StrAlloc(areaList[areaListMax].aDesc);

      if (areaTotalLog) {
        sprintf(buf,
          "SubTotal for:[%-18s] Wld[%5ld] Mob[%5ld] Obj[%5ld] Rst[%5ld]\n",
          areaList[areaListMax].aFileName->sText,
          areaList[areaListMax].aWldIndex.iNum,
          areaList[areaListMax].aMobIndex.iNum,
          areaList[areaListMax].aObjIndex.iNum,
          areaList[areaListMax].aResetNum
        );
        Log(LOG_BOOT, buf);
      }

      areaListMax++;
    }
  }
  /* all done close up shop */
  fclose(areaFile);

  /* convert virtual references to actual pointers */
  WorldReIndex();
  ResetReIndex();

  sprintf(buf,
    "Totals: Area[%ld] Wld[%5ld] Mob[%5ld] Obj[%5ld] Rst[%5ld]\n\n",
    areaListMax,
    worldNum,
    mobileNum,
    objectNum,
    resetNum
   );
  Log(LOG_BOOT, buf);
}



WORD AreaOf(WORD virtual) {
  WORD min;
  WORD mid;
  WORD max;
  BYTE buf[256];

  min = 0;
  max = areaListMax-1;

  if (virtual == -1) /* NOWHERE */
    return -1;

  /* if number is way out choke right off the bat */
  if ( (virtual < areaList[min].aVirtualMin)
     ||(virtual > areaList[max].aVirtualMax)) {
    if (areaOfLog) {
      sprintf(buf, "AreaOf(1): area containing virtual#%d does not exist\n", virtual);
      Log(LOG_ERROR, buf);
    }
    return(-1);
  }

  while (max >= min) {
    mid = (min+max) /2;
    if (virtual < areaList[mid].aVirtualMin)
      max = mid-1;
    else if (virtual > areaList[mid].aVirtualMax)
      min = mid+1;
    else { /* this must be it */
      break;
    }
  }

  /* if number fell in a crack between zones detect it here */
  if ( (virtual < areaList[mid].aVirtualMin)
     ||(virtual > areaList[mid].aVirtualMax)) {
    if (areaOfLog) {
      sprintf(buf, "AreaOf(2): area containing virtual#%hd does not exist\n", virtual);
      Log(LOG_ERROR, buf);
    }
    return(-1);
  }
  return (mid);
}


BYTE AreaHasPlayers(WORD area) {
  LWORD i;
  THING *inside;
  
  for (i=0; i<playerIndex.iNum; i++) {
    inside = Base(playerIndex.iThing[i])->bInside;
    if (   (inside                    ) 
        && (inside->tType == TTYPE_WLD) 
        && (Wld(inside)->wArea==area  )  )
      return TRUE;
  }
  return FALSE;
}

/* Move the virtual if it falls in the old range */
LWORD AreaMove(WORD area, LWORD virtual) {
  if (   areaList[area].aOffset
      && virtual >= areaList[area].aVirtualMin-areaList[area].aOffset
      && virtual <= areaList[area].aVirtualMax-areaList[area].aOffset
  )
    virtual += areaList[area].aOffset;
  return virtual;
}

/* AreaIsEditor - this function will check if the specified
 * 'thing' is a valid editor. This function will return 1 if
 * 'thing' is an editor, but not for 'area'; 2 if 'thing' 
 * is a valid editor for 'area'; 0 if 'thing' is not a
 * valid editor of ANY area.  NOTE: This function does NOT
 * take into account player level. If you wish players of certain
 * levels to have extra privs, you have to check that elsewhere. */
BYTE AreaIsEditor(WORD area, THING *thing) {
  WORD i;
  WORD returnVal;

  returnVal=0;
  if (!thing) 
    return 0;
  if (thing->tType!=TTYPE_PLR)
    return 0;
  for (i=0;i<areaListMax;i++) {
    if (StrIsExactKey(thing->tSDesc->sText, areaList[i].aEditor)) {
      if (area==i) {
        returnVal=2;
        break;
      } else {
        returnVal=1;
      }
    } 
  }
  return returnVal;
}
