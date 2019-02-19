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
#ifndef WIN32
  #include <unistd.h> /* for unlink */
#endif

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "code.h"
#include "file.h"
#include "thing.h"
#include "exit.h"
#include "index.h"
#include "world.h"
#include "area.h"

WTYPE  wTypeList[] = {
  { "INDOORS",      1 }, /* easy as it gets */
  { "CITY-OUTSIDE", 2 }, /* pavement, surfaced whatever */
  { "FIELD",        3 }, /* uneven ground */
  { "FOREST",       4 }, /* breaking through undergrowth */
  { "HILLS",        4 }, /* not as bad as mountains */
  { "MOUNTAIN",     5 }, /* ah, mountain climbing */
  { "WATER-SWIM",   4 }, /* hard work no? */
  { "WATER-NOSWIM", 1 }, /* assumed paddling boat */
  { "UNDERWATER",   4 }, /* tough swimming */
  { "VACUUM",       3 }, /* walk with care */
  { "DESERT",       3 }, /* sloggin through the sand */
  { "ARCTIC",       3 }, /* snow/ice */
  { "ROAD",         1 }, /* easy as it gets */
  { "TRAIL",        2 }, /* easy as it gets */
  { "",             0 }
};

BYTE *wFlagList[] = {
  "DARK",             /* cant see in here without light */
  "DEATHTRAP",        /* instakill mortal visators */
  "NOMOB",            /* mobs wont enter this room */
  "NOWEATHER",        /* noweather */
  "NOGOOD",           /* good auras cant enter */
  "NONEUTRAL",        /* neutral auras cant enter */
  "NOEVIL",           /* evil auras cant enter */
  "NOPSIONIC",        /* psionics wont work here */
  "SMALL",            /* only two people at a time */
  "PRIVATE",          /* no eavesdropping in here */
  "DRAINPOWER",       /* slowly drain away power points */
  "VACUUM",           /* explosive decompression in here */
  "NOTELEPORTOUT",    /* block teleports in */
  "NOTELEPORTIN",     /* block teleports out */
  ""
};

BYTE     worldOfLog;    /* log if a virtual # cant be found */
BYTE     worldReadLog;  /* log each world structure as its read */
LWORD    worldNum = 0;  /* number of world structures in use - just curious really */

void WorldInit(void) {
  BYTE buf[256];

  worldOfLog   = INILWordRead("crimson2.ini", "worldOfLog", 0);
  worldReadLog = INILWordRead("crimson2.ini", "worldReadLog", 0);
  sprintf(buf, "Reading World logging defaults\n");
  Log(LOG_BOOT, buf);
  sprintf(buf, "World structure size is %d bytes\n", sizeof(WLD));
  Log(LOG_BOOT, buf);
}


void WorldRead(WORD area) {
  FILE *worldFile;
  BYTE  wld[256];
  BYTE  buf[256];
  BYTE  tmp[256];
  LWORD last = -1;
  STR  *sKey;
  STR  *sDesc;
  FLAG  eFlag;
  WORD  eKeyObj;
  LWORD eWorld;
  BYTE  eDir;
  WLD  *world;

  sprintf(wld, "area/%s.wld", areaList[area].aFileName->sText);
  worldFile = fopen(wld, "rb");
  if (!worldFile) {
    sprintf(buf, "Unable to read %s, killing server\n", wld);
    Log(LOG_BOOT, buf);
    PERROR("WorldRead");
    exit(ERROR_BADFILE);
  }
  areaList[area].aOffset = 0;

  /* okay we opened it up so read it.... */
  while (!feof(worldFile)) {
    fscanf(worldFile, " %s \n", tmp); /* get virtual number */
    if (tmp[0] == '$' || feof(worldFile))
      break; /* Dikumud file format EOF character */
    if (tmp[0] != '#') { /* whoa... whadda we got here */
      sprintf(buf, "  Unknown keyword %s, aborting\n", tmp);
      Log(LOG_BOOT, buf);
      break;
    }
    MEMALLOC(world, WLD, WORLD_ALLOC_SIZE);
    memset( (void*)world, 0, sizeof(WLD)); /* init to zeros */
    world->wVirtual = atol(tmp+1);
    /* Relocation Code  - do we relocate? */
    if (!areaList[area].aWldIndex.iNum && world->wVirtual!=areaList[area].aVirtualMin) {
      sprintf(buf, "Relocating Area/World [%s]\n", areaList[area].aFileName->sText);
      Log(LOG_BOOT, buf);
      areaList[area].aOffset = areaList[area].aVirtualMin - world->wVirtual;
      BITSET(areaList[area].aSystem, AS_WLDUNSAVED|AS_MOBUNSAVED|AS_OBJUNSAVED|AS_RSTUNSAVED);
    }
    world->wVirtual += areaList[area].aOffset;
    if (worldReadLog) {
      sprintf(buf, "Reading World#%ld\n", world->wVirtual);
      Log(LOG_BOOT, buf);
    }
    /* confirm that virtual number is valid - warn for out of order */
    if (world->wVirtual < last) {
      sprintf(buf, "%s - room%s < than previous\n", wld, tmp);
      Log(LOG_BOOT, buf);
    }
    last = MAXV(last, world->wVirtual);
    if (world->wVirtual < areaList[area].aVirtualMin) {
      sprintf(buf, "%s - room%s < than %ld\n", wld, tmp, areaList[area].aVirtualMin);
      Log(LOG_BOOT, buf);
      break;
    }
    if (world->wVirtual > areaList[area].aVirtualMax) {
      sprintf(buf, "%s - room%s > than %ld\n", wld, tmp, areaList[area].aVirtualMax);
      Log(LOG_BOOT, buf);
      break;
    }

    /* NOTE that Private Strings are not designated as such, until just prior to editing */
    worldNum++;
    world->wArea         = area;
    Thing(world)->tType  = TTYPE_WLD;
    Thing(world)->tSDesc = FileStrRead(worldFile);
    if (fileError) {
      sprintf(buf, "Error reading World#%ld\n", world->wVirtual);
      Log(LOG_BOOT, buf);
    }
    Thing(world)->tDesc  = FileStrRead(worldFile);
    if (fileError) {
      sprintf(buf, "Error reading World#%ld\n", world->wVirtual);
      Log(LOG_BOOT, buf);
    }
    fscanf(worldFile, " %*d ");                /* in original diku this was the zone #, but isnt used anymore */
    world->wFlag = FileFlagRead(worldFile, wFlagList); /* flags for this room */
    world->wType = FILETYPEREAD(worldFile, wTypeList); /* type of this room, ie city, river, underwater etc */
    if (fileError) {
      sprintf(buf, "Error reading wType for World#%ld\n", world->wVirtual);
      Log(LOG_BOOT, buf);
    }

    /* read attachments ie exits, keywords etc */
    while (!feof(worldFile)) {
      fscanf(worldFile, " %s \n", tmp);
      if (tmp[0] == 'S') /* well thats it for this chunk of the world */
        break;
      
      else if (tmp[0] == 'D') { /* haha an exit */
        eDir  = atoi(tmp+1);
        sDesc = FileStrRead(worldFile);
        if (fileError) {
          sprintf(buf, "Error reading ExitDesc for World#%ld\n", world->wVirtual);
          Log(LOG_BOOT, buf);
        }
        /* Should have a trailing return */
        if (sDesc->sText[sDesc->sLen-1] != '\n' && sDesc->sLen>0) {
          BYTE hugeBuf[4096];
    
          sprintf(hugeBuf, "%s\n", sDesc->sText);
          STRFREE(sDesc);
          sDesc = STRCREATE(hugeBuf);
        }
        sKey  = FileStrRead(worldFile);
        if (fileError) {
          sprintf(buf, "Error reading ExitKey for World#%ld\n", world->wVirtual);
          Log(LOG_BOOT, buf);
        }
        eFlag = FileFlagRead(worldFile, eFlagList); /* flags for this exit ie closed locked etc */
        fscanf(worldFile, " %hd ", &eKeyObj); /* room this goes to */
        fscanf(worldFile, " %ld ", &eWorld); /* room this goes to */
        eWorld = AreaMove(area, eWorld);
        world->wExit =  /* will find the pointer to the room l8r */
          ExitAlloc(world->wExit, eDir, sKey, sDesc, eFlag, eKeyObj, Thing(eWorld));
      }
      
      else if (tmp[0] == 'E') { /* haha an extra */
        sKey  = FileStrRead(worldFile);
        if (fileError) {
          sprintf(buf, "Error reading ExtraKey for World#%ld\n", world->wVirtual);
          Log(LOG_BOOT, buf);
        }
        sDesc = FileStrRead(worldFile);
        if (fileError) {
          sprintf(buf, "Error reading ExtraDesc for World#%ld\n", world->wVirtual);
          Log(LOG_BOOT, buf);
        }
        Thing(world)->tExtra =
          ExtraAlloc(Thing(world)->tExtra, sKey, sDesc);
      }
      
      else if (tmp[0] == 'P') { /* a property of some kind */
        sKey =  FileStrRead(worldFile); /* property name */
        if (fileError) {
          sprintf(buf, "Error reading PropertyKey for World#%ld\n", world->wVirtual);
          Log(LOG_BOOT, buf);
        }
        sDesc = FileStrRead(worldFile);/* property value */
        if (fileError) {
          sprintf(buf, "Error reading PropertyDesc for World#%ld\n", world->wVirtual);
          Log(LOG_BOOT, buf);
        }
        Thing(world)->tProperty = PropertyCreate(Thing(world)->tProperty, sKey, sDesc);
      }
    }
    
    /* guess this ones a keeper, update mins and maxes etc... */
    IndexInsert(&areaList[area].aWldIndex, Thing(world), WorldCompareProc);
    if (indexError) {
      sprintf(buf, "%s - duplicate insert for room%s\n", wld, tmp);
      Log(LOG_BOOT, buf);
      THINGFREE( Thing(world) );
    }
  }
  /* all done close up shop */
  fclose(worldFile);
}

THING *WorldOf(LWORD virtual) {
  THING *search;
  BYTE   buf[256];
  LWORD  area;
  
  area = AreaOf(virtual);
  if (area == -1) return NULL;
  
  search = IndexFind(&areaList[area].aWldIndex, (void*)virtual, WorldFindProc);
  if (search) {
    return search;
  } else {
    if (worldOfLog) {
      sprintf(buf, "WorldOf: virtual %ld doesnt exist\n", virtual);
      Log(LOG_ERROR, buf);
    }
  }
  return NULL;
}


/* Once we have finished reading everything in, re-index all entries exits */
void WorldReIndex(void) {
  LWORD area;
  LWORD world;
  EXIT *exit;
  
  Log(LOG_BOOT, "Re-Indexing World\n");
  for (area=0; area<areaListMax; area++) {
    for (world=0; world<areaList[area].aWldIndex.iNum; world++) {
      /* re-index all the exits in this room */
      for (exit=Wld(areaList[area].aWldIndex.iThing[world])->wExit; exit; exit=exit->eNext){
        exit->eWorld = WorldOf((LWORD) exit->eWorld);
      }
    }
  }
}


void WorldWrite(WORD area) {
  FILE     *worldFile;
  BYTE      wld[128];
  BYTE      buf[256];
  THING    *world;
  EXTRA    *extra;
  LWORD     wVirtual;
  PROPERTY *property;
  EXIT     *exit;
  LWORD     i;
  
  /* take a backup case we crash halfways through this */
  sprintf(buf,
    "mv area/wld/%s.wld area/wld/%s.wld.bak",
    areaList[area].aFileName->sText,
    areaList[area].aFileName->sText);
  system(buf);
  
  sprintf(wld, "area/%s.wld", areaList[area].aFileName->sText);
  if (areaList[area].aWldIndex.iNum == 0) {
    Log(LOG_AREA, wld);
    LogPrintf(LOG_AREA, ": nothing to save.\n");
    return;
  }
  worldFile = fopen(wld, "wb");
  if (!worldFile) {
    sprintf(buf, "Unable to write %s!\n", wld);
    Log(LOG_ERROR, buf);
    PERROR("WorldWrite");
  }
  
  /* okay we opened it up so write it.... */
  for (i=0; i<areaList[area].aWldIndex.iNum; i++) {
    world = Thing(areaList[area].aWldIndex.iThing[i]);
    fprintf(worldFile, "#%ld\n", Wld(world)->wVirtual); /* get virtual number */
    FileStrWrite(worldFile, world->tSDesc);
    FileStrWrite(worldFile, world->tDesc);
    fprintf(worldFile, "%hd ", area);
    FileFlagWrite(worldFile, Wld(world)->wFlag, wFlagList, ' ');
    FILETYPEWRITE(worldFile, Wld(world)->wType, wTypeList, '\n');
    
    /* read attachments ie exits, keywords etc */
    for (exit=Wld(world)->wExit; exit; exit=exit->eNext) {
      fprintf(worldFile, "D%d\n", (int)exit->eDir);
      FileStrWrite(worldFile, exit->eDesc);
      FileStrWrite(worldFile, exit->eKey);
      if (exit->eWorld) {
        wVirtual=Wld(exit->eWorld)->wVirtual;
      } else {
        wVirtual=-1;
      }
      FileFlagWrite(worldFile, exit->eFlag, eFlagList, ' ');
      fprintf(worldFile, "%ld %ld\n", exit->eKeyObj, wVirtual);
    }
    
    /* extra descriptions */
    for (extra=world->tExtra; extra; extra=extra->eNext) {
      fprintf(worldFile, "E\n");
      FileStrWrite(worldFile, extra->eKey);
      FileStrWrite(worldFile, extra->eDesc);
    }
    
    /* property's */
    for (property=world->tProperty; property; property=property->pNext) {
      fprintf(worldFile, "P\n");
      FileStrWrite(worldFile, property->pKey);
      if ( CodeIsCompiled(property) ) {
        if (!areaWriteBinary) {
          /* Decompile the code b4 saving */
          CodeDecompProperty(property, NULL);
          /* Warn if it didnt decompile */
          if (CodeIsCompiled(property)) {
            sprintf(buf, "WorldWrite: Property %s failed to decompile for world#%ld!\n", 
              property->pKey->sText, 
              Wld(world)->wVirtual);
            Log(LOG_AREA, buf);
          } else {
            CodeClearFlag(world, property);
            FileStrWrite(worldFile, property->pDesc);
            /* if there is a player in the room - recompile the code */
            if (world->tContain) {
              if (!CodeCompileProperty(property, NULL))
                CodeSetFlag(world, property);
              BITCLR(world->tFlag, TF_COMPILE);
            } else {
              BITSET(world->tFlag, TF_COMPILE);
            }
          }
        } else {
          FileBinaryWrite(worldFile, property->pDesc);
        }
      } else {
        FileStrWrite(worldFile, property->pDesc);
      }
    }
    
    fprintf(worldFile, "S\n");
  }
  
  /* all done close up shop */
  fclose(worldFile);
  
  /* turf backup we didnt crash */
  sprintf(buf,
    "area/%s.wld.bak",
    areaList[area].aFileName->sText);
  unlink(buf);
}


INDEXPROC(WorldCompareProc) { /* BYTE IndexProc(void *index1, void *index2) */
  if ( Wld(index1)->wVirtual == Wld(index2)->wVirtual )
    return 0;
  else if ( Wld(index1)->wVirtual < Wld(index2)->wVirtual )
    return -1;
  else
    return 1;
}

INDEXFINDPROC(WorldFindProc) { /* BYTE IFindProc(void *key, void *index) */
  if ( (LWORD)key == Wld(index)->wVirtual )
    return 0;
  else if ( (LWORD)key < Wld(index)->wVirtual )
    return -1;
  else
    return 1;
}





