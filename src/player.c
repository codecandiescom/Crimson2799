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
#include <sys/types.h>
#include <fcntl.h>
#ifndef WIN32 
  #include <unistd.h>
  #include <dirent.h>
#else
  #include <io.h>
  #include <direct.h>
#endif
#include <ctype.h>

#include "crimson2.h"
#include "macro.h"
#include "queue.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "send.h"
#include "extra.h"
#include "property.h"
#include "code.h"
#include "file.h"
#include "thing.h"
#include "index.h"
#include "world.h"
#include "area.h"
#include "reset.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "affect.h"
#include "effect.h"
#include "skill.h"
#include "player.h"
#include "parse.h"
#include "cmd_inv.h"
#include "cmd_misc.h"

#ifdef WIN32
  /* including winsock.h etc auto includes the windows types */
  /* turn off warning: benign type redefinition */
  #pragma warning( disable : 4142 )
  #include <winsock.h>
  #pragma warning( default : 4142 )
#endif

#define PLAYER_ALLOC_SIZE 4096
#define PLAYER_INDEX_SIZE 1024

INDEX playerIndex;

BYTE     playerInitLog;
LWORD    playerStartRoom;
LWORD    playerIdleRoom;
LWORD    playerIdleTick;
LWORD    playerDropTick;
LWORD    playerAFKDropTick;

BYTE    *pSystemList[] = {
  "LOG",
  "KILLER",
  "HELPER",
  "CRASHSAVED",
  "NOHASSLE",
  ""
};

BYTE    *pAutoList[] = {
  "A-LOOK",
  "A-EXIT",
  "A-LOOT",
  "A-ASSIST",
  "A-AGGRESSIVE",
  "A-JUNK",
  "A-FLEE",
  "A-EAT",
  "A-DRINK",
  "NOEMAIL",
  "HINT",
  "EXPERT",
  "CONSIDER",
  "A-RESCUE",
  "AFK",
  ""
};

BYTE *pSpeechColor[] = {
  "rA",
  "gA",
  "pA",
  "cA",
  "CA",
  "wA",
  "WA",
  "WR",
  "gR",
  "GR",
  "yR", 
  "pR", 
  "cR", 
  "CR", 
  "WR", 
  "AG", 
  "RG", 
  "yG", 
  "BG", 
  "PG", 
  "cG", 
  "AY", 
  "rY", 
  "RY", 
  "gY", 
  "BY", 
  "PY", 
  "cY", 
  "wY", 
  "rB", 
  "gB", 
  "yB", 
  "pB", 
  "cB", 
  "wB", 
  "WB", 
  "aP", 
  "gP", 
  "yP", 
  "YP", 
  "cP", 
  "CP", 
  "WP", 
  "AC", 
  "rC", 
  "RC", 
  "gC", 
  "yC", 
  "bC", 
  "BC", 
  "PC", 
  "wC", 
  "rW", 
  "AW", 
  "RW", 
  "bW", 
  "BW", 
  "pW", 
  "PW", 
  "GW",
  ""
};

LWORD pSpeechColorNum = 0;

void PlayerInit(void) {
  BYTE           buf[256];
  #ifndef WIN32
    DIR                *playerDir;
    struct dirent      *playerEnt;
  #else
    struct _finddata_t  playerFind;
    LWORD               findError;
  #endif
  LWORD          sLen;
  BYTE           playerName[256];
  BYTE           playerFile[256];
  LWORD          playerNum;
  LWORD          playerDelete = FALSE;
  LWORD          deleteThreshold;
  LWORD          deleteNum;
  THING         *player;

  IndexInit(&playerIndex, PLAYER_INDEX_SIZE, "playerIndex", 0);

  playerInitLog   = INILWordRead("crimson2.ini", "playerInitLog", 0);
  playerStartRoom = INILWordRead("crimson2.ini", "pStartRoom",    3001);
  playerIdleRoom  = INILWordRead("crimson2.ini", "pIdleRoom",     0);
  playerIdleTick  = INILWordRead("crimson2.ini", "pIdleTick",     12);
  playerDropTick  = INILWordRead("crimson2.ini", "pDropTick",     48);
  playerAFKDropTick=INILWordRead("crimson2.ini", "pAFKTick",      360);

  if (PLAYER_MAX_SKILL < skillNum) {
    sprintf(buf, "Increase PLAYER_MAX_SKILL to at least %hd\n", skillNum);
    Log(LOG_ERROR, buf);
    EXIT(ERROR_BADFILE);
  }
  
  for (pSpeechColorNum=0; *pSpeechColor[pSpeechColorNum]; pSpeechColorNum++);


  /* Scan Crash directory - Count objects as online */
  playerNum = 0;
  Log(LOG_BOOT, "Scanning crash/ for active files\n");
#ifndef WIN32
  playerDir = opendir("crash");
  if (!playerDir) {
    Log(LOG_ERROR, "Directory crash/ doesnt exist... create it!\n");
    EXIT(ERROR_BADFILE);
  }
  while( (playerEnt=readdir(playerDir)) ) {
    sLen = strlen(playerEnt->d_name);
    strcpy(playerName, playerEnt->d_name);
#else
  findError = _findfirst( "crash\\*.plr", &playerFind );
  while( findError != -1 ) {
    strcpy(playerName, playerFind.name);
    sLen = strlen(playerName);
#endif
    if (sLen > 4 && !strcmp(playerName+sLen-4, ".plr")) {
      playerName[sLen-4] = '\0';
      /* see if the crash file is stale, if it is, move it! */
      player = PlayerRead(playerName, PREAD_SCAN);
      /* its stale when its >48 hours old */
      if (time(0) - Plr(player)->pTimeLastOn > 48*3600) {
        #ifdef WIN32
          sprintf(buf, "move crash\\%s.plr player\\%s.plr", playerName, playerName);
        #else
          sprintf(buf, "mv crash/%s.plr player/%s.plr", playerName, playerName);
        #endif
        system(buf);
      } else {
#ifndef WIN32
        PlayerCountObjects("crash", playerEnt->d_name, PCO_ONLINE);
#else
        PlayerCountObjects("crash", playerFind.name, PCO_ONLINE);
#endif
        if (playerInitLog) {
          if (playerNum%5 == 4) {
            sprintf(buf, " %-14s\n", playerName);
          } else {
            sprintf(buf, " %-14s", playerName);
          }
          if (playerNum%5==0)
            Log(LOG_BOOT, buf);
          else
            LogPrintf(LOG_BOOT, buf);
        }
      }
      THINGFREE(player);
      playerNum++;
    }
#ifdef WIN32
    if ( _findnext( findError, &playerFind ) !=0) {
      _findclose(findError);
      break;
    }
#endif
  }  
  if (playerNum>0 && playerNum%5!=0)
    LogPrintf(LOG_BOOT, "\n");
#ifndef WIN32
  closedir(playerDir);
#endif
  sprintf(buf, "%ld crashed players found.\n\n", playerNum);
  Log(LOG_BOOT, buf);



  /* Scan Players directory - Count offline objects */
  playerNum = 0;
  deleteNum = 0;
  Log(LOG_BOOT, "Scanning players/ for active files\n");
#ifndef WIN32
  playerDir = opendir("player");
  if (!playerDir) {
    Log(LOG_ERROR, "Directory player/ doesnt exist... create it!\n");
    EXIT(ERROR_BADFILE);
  }
  while( (playerEnt=readdir(playerDir)) ) {
    sLen = strlen(playerEnt->d_name);
    strcpy(playerName, playerEnt->d_name);
    strcpy(playerFile, playerEnt->d_name);
#else
  findError = _findfirst( "player\\*.plr", &playerFind );
  while( findError != -1 ) {
    strcpy(playerName, playerFind.name);
    strcpy(playerFile, playerFind.name);
    sLen = strlen(playerName);
#endif

    if (sLen > 4 && !strcmp(playerName+sLen-4, ".plr")) {
      playerName[sLen-4] = '\0';

      /* Check for auto-delete its stale when its > ?? days */
      player = PlayerRead(playerName, PREAD_SCAN);
      if (Character(player)->cExp <= 0 && Character(player)->cLevel <= 1)
        deleteThreshold = 10 * 24 * 3600;
      else if (Character(player)->cLevel <= 3)
        deleteThreshold = 30 * 24 * 3600;
      else if (Character(player)->cLevel <= 10)
        deleteThreshold = 60 * 24 * 3600;
      else if (Character(player)->cLevel <= 20)
        deleteThreshold = 120 * 24 * 3600;
      else if (Character(player)->cLevel <= 30)
        deleteThreshold = 150 * 24 * 3600;
      else
        deleteThreshold = 365 * 24 * 3600;
      playerDelete = FALSE;
      if (Plr(player)->pTimeLastOn && (time(NULL) - Plr(player)->pTimeLastOn) > deleteThreshold) {
        playerDelete = TRUE;
        deleteNum += 1;
      }
      THINGFREE(player);

      if (playerDelete) {
        /* They're history */
        #ifdef WIN32
          sprintf(buf, "move player\\%s.plr player\\%s.plr.del", playerName, playerName);
        #else
  	/* THERE IS AS BUG deleting player files at random, so I took this out.  -- CAM TAG TAG */
        /*  sprintf(buf, "mv player/%s.plr player/%s.plr.del", playerName, playerName);*/
        #endif
        /*system(buf);*/
      } else {
        /* Still with us, collect some stats on them */
        PlayerCountObjects("player", playerFile, PCO_OFFLINE);

        if (playerInitLog) {
          if (playerNum%5 == 4) {
            sprintf(buf, " %-14s\n", playerName);
          } else {
            sprintf(buf, " %-14s", playerName);
          }
          if (playerNum%5==0)
            Log(LOG_BOOT, buf);
          else
            LogPrintf(LOG_BOOT, buf);
        }
        playerNum++;
      }
    }
#ifdef WIN32
    if ( _findnext( findError, &playerFind ) !=0) {
      _findclose(findError);
      break;
    }
#endif
  }  
  if (playerNum>0 && playerNum%5!=0)
    LogPrintf(LOG_BOOT, "\n");
#ifndef WIN32
  closedir(playerDir);
#endif
  sprintf(buf, "%ld offline players found.\n", playerNum);
  Log(LOG_BOOT, buf);
  sprintf(buf, "%ld old players turfed.\n\n", deleteNum);
  Log(LOG_BOOT, buf);
}

/* Count all the Objects in a player file - call before boot-time reset */
void PlayerCountObjects(BYTE *directory, BYTE *playerFileName, LWORD online) {
  FILE        *playerFile;
  BYTE         playerFileBuf[128];
  BYTE         buf[256];
  OBJTEMPLATE *objTemplate;
  LWORD        oVirtual;


  sprintf(playerFileBuf, "%s/%s", directory, playerFileName);
  playerFile = fopen(playerFileBuf, "rb");
  if (!playerFile) {
    Log(LOG_ERROR, "PlayerCountObjects: Unable to open: ");
    LogPrintf(LOG_ERROR, playerFileBuf);
    LogPrintf(LOG_ERROR, "\n");
    return;
  }

  if (PlayerReadHeader(playerFile, "[Objects]")){
    while(!feof(playerFile)) {
      fscanf(playerFile, " ");
      buf[0]=0;
      fgets(buf, sizeof(buf), playerFile);
      if (*buf=='[') break;
      switch(buf[0]) {
      case '#':
        sscanf(buf, "# %ld", &oVirtual);
        if (oVirtual != -1)
          objTemplate = ObjectOf(oVirtual);
        else
          objTemplate = NULL;
        if (objTemplate) {
          if (online) {
            objTemplate->oOnline++;
          } else {
            objTemplate->oOffline++;
          }
        }
        break;
      }
    }
  }
  fclose(playerFile);
}


THING *PlayerFind(BYTE *playerName) {
  LWORD  i;

  for (i=0; i<playerIndex.iNum; i++) {
    if (!STRICMP(Thing(playerIndex.iThing[i])->tSDesc->sText, playerName))
      return playerIndex.iThing[i]; /* allready playing */
  }
  return NULL;
}

/* reformat a player name to correct case, guard against caps lock morons */
/* returns true/false name string is valid */
BYTE PlayerName(BYTE *playerName) {
  BYTE   allUpper = TRUE;
  LWORD  i;

  if (!playerName) return PNAME_INVALID; /* null */
  if (strlen(playerName)<3) return PNAME_INVALID; /* too short */
  if (strlen(playerName)>15) return PNAME_INVALID; /* too long */

  if (islower(playerName[0]))
    playerName[0] = toupper(playerName[0]);

  /* ensure no punctuation in name */
  for (i=0; playerName[i] != '\0'; i++) {
    if (islower(playerName[i]))
      allUpper = FALSE;
    if (!isalpha(playerName[i]))
      return PNAME_INVALID;
  }

  /* HAHA upper case bozo's are in for a shock */
  if (allUpper) /* lowercase everything but first char */
    for (i=1; playerName[i] != '\0'; i++)
      playerName[i] = tolower(playerName[i]);

  return PNAME_VALID;
}


THING *PlayerAlloc(BYTE *playerName) {
  PLR   *player;
  LWORD  i;

  MEMALLOC(player, PLR, PLAYER_ALLOC_SIZE);
  memset(player, 0, sizeof(PLR));
  IndexAppend(&playerIndex, Thing(player));

  /* Complete set of defaults */
  Thing(player)->tType       = TTYPE_PLR;
  Thing(player)->tSDesc      = STRCREATE(playerName); /* name */
  Thing(player)->tDesc       = STRCREATE("You see nothing special.\n"); /* paragraph description */

  Base(player)->bKey         = STRCREATE(playerName); /* keywords that you can use to access player */
  Base(player)->bLDesc       = STRCREATE("$n the Titleless"); /* name & title */
  Base(player)->bWeight      = 150;

  Character(player)->cLevel  = 1;
  Character(player)->cPos    = POS_STANDING;

  Plr(player)->pPassword     = STRCREATE(""); /* default password */
  Plr(player)->pEnter        = STRCREATE("$n appears from a cloud of blue sparkles\n"); /* default password */
  Plr(player)->pExit         = STRCREATE("$n dissolves into a cloud of blue sparkles\n"); /* default password */
  Plr(player)->pPrompt       = STRCREATE(""); /* default password */
  Plr(player)->pLastLogin    = STRCREATE(""); /* default password */
  Plr(player)->pEmail        = STRCREATE(""); /* default password */
  Plr(player)->pPlan         = STRCREATE(""); /* default password */
  Plr(player)->pHunger       = -1;
  Plr(player)->pThirst       = -1;
  Plr(player)->pAuto         = PA_AUTOLOOK | PA_AUTOEXIT | PA_AUTOEAT | PA_AUTOFLEE
                               | PA_NOEMAIL | PA_AUTODRINK | PA_HINT | PA_CONSIDER;
  Plr(player)->pSockPref     = sockDefFlag;
  Plr(player)->pScreenLines  = 23;

  Plr(player)->pStartRoom    = -1;
  Plr(player)->pRecallRoom   = -1;
  Plr(player)->pDeathRoom    = 100;

  memcpy(Plr(player)->pColorPref, defaultPref, sizeof(COLORPREF)*COLORPREF_MAX);
  /* assign a random speech color */
  i = Number(0, pSpeechColorNum-1);
  Plr(player)->pColorPref[6].cFG = *pSpeechColor[i];
  Plr(player)->pColorPref[6].cBG = *(pSpeechColor[i]+1);

  return Thing(player);
}

BYTE PlayerCrashExists(THING *thing) {
  FILE *file;
  BYTE  playerFileBuf[128];
  BYTE  playerName[128];

  if (!thing) return FALSE;

  /* obtain filename version of players name */
  strcpy(playerName, thing->tSDesc->sText);
  StrToLower(playerName);

  /* plr file */
  sprintf(playerFileBuf, "crash/%s.plr", playerName);
  file = fopen(playerFileBuf, "rb");
  if (file) {
    fclose(file);
    return TRUE;
  } else
    return FALSE;
}

BYTE PlayerCloneExists(THING *thing) {
  FILE *file;
  BYTE  playerFileBuf[128];
  BYTE  playerName[128];

  if (!thing) return FALSE;

  /* obtain filename version of players name */
  strcpy(playerName, thing->tSDesc->sText);
  StrToLower(playerName);

  /* plr file */
  sprintf(playerFileBuf, "clone/%s.plr", playerName);
  file = fopen(playerFileBuf, "rb");
  if (file) {
    fclose(file);
    return TRUE;
  } else
    return FALSE;
}

/* Since the online totals of objects are adjusted when a crash player file
   is read in, in the event that the password check fails (after the file
   has allready been read) the online totals to be re-adjusted.
 */
void PlayerAddOnline(THING *thing) {
  if (!thing) return;
  if (thing->tType == TTYPE_OBJ) {
    Obj(thing)->oTemplate->oOnline++;
  }
  PlayerAddOnline(thing->tContain);
  PlayerAddOnline(thing->tNext);
}

/* the ftell/fseek business prevents a missing setting from preventing reads
   of later ones due to zipping down to the end of the file */
BYTE PlayerReadHeader(FILE *playerFile, BYTE *header) {
  BYTE  buf[512];
  LWORD playerPos;

  playerPos = ftell(playerFile);
  while (!feof(playerFile)) {
    fscanf(playerFile, " %s", buf);
    if (!STRICMP(header, buf)) return TRUE;
    fgets(buf, 511, playerFile); /* must be some other line */
  }
  if (feof(playerFile))
    fseek(playerFile, playerPos, SEEK_SET);
  return FALSE;
}


/* Misplaced {'s and }'s will really screw this thing up, the return value
 * is used so that during recursion it can tell itself not to overrun the
 * end of file or the end of the group!
 */
BYTE PlayerReadObject(THING *player, FILE *playerFile, THING *thing, BYTE mode) {
  THING       *object;
  OBJTEMPLATE *objTemplate;
  LWORD        oVirtual;
  LWORD        apply;
  BYTE         aType[256];
  LWORD        aValue;
  BYTE         buf[256];
  LWORD        i;
  LWORD        typeCheck = -1;

  fgets(buf, sizeof(buf), playerFile);
  for (i=0;buf[i]==' ';i++);
  object = NULL;
  while(!feof(playerFile)) {
  
    if (buf[i]=='{') /* objects within objects */
      if (!PlayerReadObject(player, playerFile, object, mode))
        return FALSE;
      
    if (buf[i]=='}') return TRUE;  /* end of nested objects level */
    if (buf[i]=='[') return FALSE; /* End of section */
    switch(buf[i]) {
    case '#':
      apply=0;
      sscanf(buf+i, "# %ld", &oVirtual);
      if (oVirtual != -1)
        objTemplate = ObjectOf(oVirtual);
      else
        objTemplate = NULL;
      if (objTemplate) {
        object = ObjectCreate(objTemplate, thing);
        typeCheck = -1;
        /* trailing comment of object name is ignored */
        sscanf(buf+i, 
              "# %*d %ld %ld %ld %ld %ld %ld %ld",
               &Obj(object)->oDetail.lValue[0],
               &Obj(object)->oDetail.lValue[1],
               &Obj(object)->oDetail.lValue[2],
               &Obj(object)->oDetail.lValue[3],
               &Obj(object)->oEquip,
               &Obj(object)->oAct,
               &typeCheck);
        if (Obj(object)->oEquip && objTemplate->oType == OTYPE_WEAPON)
          Character(player)->cWeapon = object;
          
        BITSET(Character(player)->cEquip, Obj(object)->oEquip);
          
        /* Now that the object is really loaded remove extra online entry */
        if (mode == PREAD_CRASH)
          objTemplate->oOnline--; 
        /* In the game now, no longer offline */
        else if (mode == PREAD_PLAYER)
          objTemplate->oOffline--;
      } else {
        object = NULL;
      }
      break;
        
    case 'A':
      if (object) {
        sscanf(buf+i, "A %s %ld", aType, &aValue);
        Obj(object)->oApply[apply].aType = TYPESSCANF(aType, applyList);
        TYPECHECK(Obj(object)->oApply[apply].aType, applyList);
        Obj(object)->oApply[apply].aValue = aValue;
        apply++;
      }
      break;
    }
    
    fgets(buf, sizeof(buf), playerFile);
    for (i=0;buf[i]==' ';i++);

    /* if we've finished reading this object see if its been obsoleted */
    switch(buf[i]) {
    case '{':
    case '}':
    case '[':
    case '#':
      if (object && typeCheck!=-1 && typeCheck!=objTemplate->oType) {
        SendThing("^wRemoving out of play object\n", thing);
        THINGFREE(object);
      }
    }
    
  }
  
  /* End of file */
  return FALSE;
}

/* will return NULL if player does not exist, it should be noted
  that due to internal structure lines greater than 511 characters
  are not supported.
  BTW: in case you are wondering why the settings are all named,
   experience has shown that the player file structure is frequently
   changed and that requests to change level, abilities etc are *VERY*
   common.
  */
THING *PlayerRead(BYTE *playerName, BYTE mode) {
  THING       *player;
  FILE        *playerFile;
  BYTE         playerFileBuf[128];
  BYTE         pathBuf[64] = "crash";
  BYTE         buf[256];
  BYTE        *scan;
  LWORD        i;
  STR         *sKey;
  STR         *sDesc;
  AFFECT      *affect;
  

  /* check to see if a crash recovery file exists first */
  switch (mode) {
  case PREAD_NORMAL:
  case PREAD_SCAN:
    sprintf(playerFileBuf, "crash/%s.plr", playerName);
    StrPath(playerFileBuf);
    StrToLower(playerFileBuf);
    playerFile = fopen(playerFileBuf, "rb");
    if (!playerFile) {
      /* check to see if a normal player save file exists */
      strcpy(pathBuf, "player");
      sprintf(playerFileBuf, "player/%s.plr", playerName);
      StrPath(playerFileBuf);
      StrToLower(playerFileBuf);
      playerFile = fopen(playerFileBuf, "rb");
      if (mode == PREAD_NORMAL)
        mode = PREAD_PLAYER;
    } else 
      if (mode == PREAD_NORMAL)
        mode = PREAD_CRASH;
    break;

  case PREAD_CLONE:
    sprintf(playerFileBuf, "clone/%s.plr", playerName);
    StrPath(playerFileBuf);
    StrToLower(playerFileBuf);
    playerFile = fopen(playerFileBuf, "rb");
    break;
  }

  if (playerFile) {
    /* the basics */
    player = PlayerAlloc(playerName);
    FILESTRREAD(playerFile, Thing(player)->tSDesc);
    if (fileError) {
      sprintf(buf, "Error reading SDesc for player %s\n", playerName);
      Log(LOG_ERROR, buf);
    }
    FILESTRREAD(playerFile, Plr(player)->pPassword);
    if (fileError) {
      sprintf(buf, "Error reading Password for player %s\n", playerName);
      Log(LOG_ERROR, buf);
    }
    FILESTRREAD(playerFile, Base(player)->bLDesc);
    if (fileError) {
      sprintf(buf, "Error reading LDesc for player %s\n", playerName);
      Log(LOG_ERROR, buf);
    }
    FILESTRREAD(playerFile, Thing(player)->tDesc);
    if (fileError) {
      sprintf(buf, "Error reading Desc for player %s\n", playerName);
      Log(LOG_ERROR, buf);
    }
    if (PlayerReadHeader(playerFile, "Messages:")) {
      fgets(buf, sizeof(buf), playerFile); 
      FILESTRREAD(playerFile, Plr(player)->pEnter);
      if (fileError) {
        sprintf(buf, "Error reading EnterMsg for player %s\n", playerName);
        Log(LOG_ERROR, buf);
      }
      FILESTRREAD(playerFile, Plr(player)->pExit);
      if (fileError) {
        sprintf(buf, "Error reading ExitMsg for player %s\n", playerName);
        Log(LOG_ERROR, buf);
      }
      FILESTRREAD(playerFile, Plr(player)->pEmail);
      if (fileError) {
        sprintf(buf, "Error reading Email for player %s\n", playerName);
        Log(LOG_ERROR, buf);
      }
      FILESTRREAD(playerFile, Plr(player)->pPlan);
      if (fileError) {
        sprintf(buf, "Error reading Plan for player %s\n", playerName);
        Log(LOG_ERROR, buf);
      }
    }

    if (PlayerReadHeader(playerFile, "Prompt:")) {
      fscanf(playerFile, " ");
      FILESTRREAD(playerFile, Plr(player)->pPrompt);
      if (fileError) {
        sprintf(buf, "Error reading Prompt for player %s\n", playerName);
        Log(LOG_ERROR, buf);
      }
    }
    if (PlayerReadHeader(playerFile, "LastLogin:")) {
      fscanf(playerFile, " ");
      FILESTRREAD(playerFile, Plr(player)->pLastLogin);
      if (fileError) {
        sprintf(buf, "Error reading LastLogin for player %s\n", playerName);
        Log(LOG_ERROR, buf);
      }
    }
    if (PlayerReadHeader(playerFile, "Level:"))
      Character(player)->cLevel = FileByteRead(playerFile);
    if (PlayerReadHeader(playerFile, "Race:"))
      Plr(player)->pRace = FILETYPEREAD(playerFile, raceList);
    if (PlayerReadHeader(playerFile, "Class:"))
      Plr(player)->pClass = FILETYPEREAD(playerFile, classList);
    if (PlayerReadHeader(playerFile, "Position:"))
      Character(player)->cPos = FILETYPEREAD(playerFile, posList);
    if (Character(player)->cPos >= POS_STUNNED)
      Character(player)->cPos = POS_STANDING;
    if (PlayerReadHeader(playerFile, "Sex:"))
      Character(player)->cSex = FILETYPEREAD(playerFile, sexList);

    if (PlayerReadHeader(playerFile, "AutoAction:"))
      Plr(player)->pAuto = FileFlagRead(playerFile, pAutoList);
    if (PlayerReadHeader(playerFile, "System:"))
      Plr(player)->pSystem = FileFlagRead(playerFile, pSystemList);
    if (PlayerReadHeader(playerFile, "SockPref:"))
      Plr(player)->pSockPref = FileFlagRead(playerFile, sfPrefList);
    if (PlayerReadHeader(playerFile, "ScreenLines:"))
      Plr(player)->pScreenLines = FileByteRead(playerFile);
    if (PlayerReadHeader(playerFile, "ColorPref:")) {
      fscanf(playerFile, " ");
      fgets(buf, sizeof(buf), playerFile);
      scan = buf;
      for (i=0; i<COLORPREF_MAX; i++) {
        if (!*scan) break;
        Plr(player)->pColorPref[i].cFG = scan[0];
        Plr(player)->pColorPref[i].cBG = scan[1];
        scan = StrOneWord(scan, NULL);
      }
    }
/*
    if (PlayerReadHeader(playerFile, "Equip:"))
      fscanf(playerFile, " %ld", &Character(player)->cEquip);
*/
    if (PlayerReadHeader(playerFile, "Practice:"))
      fscanf(playerFile, " %ld", &Plr(player)->pPractice);
    if (PlayerReadHeader(playerFile, "HitP:"))
      fscanf(playerFile, " %ld", &Character(player)->cHitP);
    if (PlayerReadHeader(playerFile, "MoveP:"))
      fscanf(playerFile, " %ld", &Character(player)->cMoveP);
    if (PlayerReadHeader(playerFile, "PowerP:"))
      fscanf(playerFile, " %ld", &Character(player)->cPowerP);

    if (PlayerReadHeader(playerFile, "GainPractice:"))
      fscanf(playerFile, " %hd", &Plr(player)->pGainPractice);
    if (PlayerReadHeader(playerFile, "GainHit:"))
      fscanf(playerFile, " %hd", &Plr(player)->pGainHit);
    if (PlayerReadHeader(playerFile, "GainMove:"))
      fscanf(playerFile, " %hd", &Plr(player)->pGainMove);
    if (PlayerReadHeader(playerFile, "GainPower:"))
      fscanf(playerFile, " %hd", &Plr(player)->pGainPower);

    if (PlayerReadHeader(playerFile, "Fame:"))
      fscanf(playerFile, " %hd", &Plr(player)->pFame);
    if (PlayerReadHeader(playerFile, "Infamy:"))
      fscanf(playerFile, " %hd", &Plr(player)->pInfamy);

    if (PlayerReadHeader(playerFile, "HitPMax:"))
      fscanf(playerFile, " %ld", &Character(player)->cHitPMax);
    if (PlayerReadHeader(playerFile, "MovePMax:"))
      fscanf(playerFile, " %ld", &Plr(player)->pMovePMax);
    if (PlayerReadHeader(playerFile, "PowerPMax:"))
      fscanf(playerFile, " %ld", &Plr(player)->pPowerPMax);

    if (PlayerReadHeader(playerFile, "Hunger:"))
      fscanf(playerFile, " %hd", &Plr(player)->pHunger);
    if (PlayerReadHeader(playerFile, "Thirst:"))
      fscanf(playerFile, " %hd", &Plr(player)->pThirst);
    if (PlayerReadHeader(playerFile, "Intox:"))
      fscanf(playerFile, " %hd", &Plr(player)->pIntox);

    if (PlayerReadHeader(playerFile, "Str:"))
      fscanf(playerFile, " %hd", &Plr(player)->pStr);
    /* temporary fix */
    /*MAXSET(Plr(player)->pStr, raceList[Plr(player)->pRace].rMaxStat[SA_STR]);*/
    if (PlayerReadHeader(playerFile, "Dex:"))
      fscanf(playerFile, " %hd", &Plr(player)->pDex);
    if (PlayerReadHeader(playerFile, "Con:"))
      fscanf(playerFile, " %hd", &Plr(player)->pCon);
    if (PlayerReadHeader(playerFile, "Wis:"))
      fscanf(playerFile, " %hd", &Plr(player)->pWis);
    if (PlayerReadHeader(playerFile, "Int:"))
      fscanf(playerFile, " %hd", &Plr(player)->pInt);
/*
    if (PlayerReadHeader(playerFile, "HitBonus:"))
      fscanf(playerFile, " %hd", &Character(player)->cHitBonus);
    if (PlayerReadHeader(playerFile, "DamBonus:"))
      fscanf(playerFile, " %hd", &Character(player)->cDamBonus);
   
    if (PlayerReadHeader(playerFile, "Armor:"))
      fscanf(playerFile, " %hd", &Character(player)->cArmor);
    for (i=0; i<CHAR_MAX_RESIST; i++) {
      sprintf(buf, "Resist%ld:", i);
      if (PlayerReadHeader(playerFile, buf))
        fscanf(playerFile, " %hd", &Character(player)->cResist[i]);
    }
*/
    if (mode!=PREAD_CLONE && PlayerReadHeader(playerFile, "Money:"))
      fscanf(playerFile, " %ld", &Character(player)->cMoney);
    if (mode!=PREAD_CLONE && PlayerReadHeader(playerFile, "Bank:"))
      fscanf(playerFile, " %ld", &Plr(player)->pBank);
    if (PlayerReadHeader(playerFile, "Exp:"))
      fscanf(playerFile, " %ld", &Character(player)->cExp);

    if (PlayerReadHeader(playerFile, "TimeTotal:"))
      fscanf(playerFile, " %lu", &Plr(player)->pTimeTotal);
    if (PlayerReadHeader(playerFile, "TimeLastOn:"))
      fscanf(playerFile, " %lu", &Plr(player)->pTimeLastOn);
    if (PlayerReadHeader(playerFile, "Attempts:"))
      fscanf(playerFile, " %hd", &Plr(player)->pAttempts);
    if (PlayerReadHeader(playerFile, "StartRoom:"))
      fscanf(playerFile, " %ld", &Plr(player)->pStartRoom);
    if (PlayerReadHeader(playerFile, "RecallRoom:"))
      fscanf(playerFile, " %ld", &Plr(player)->pRecallRoom);
    if (PlayerReadHeader(playerFile, "DeathRoom:"))
      fscanf(playerFile, " %ld", &Plr(player)->pDeathRoom);

    /* Read in the Skills */
    if (PlayerReadHeader(playerFile, "[Skills]")){
      while(fscanf(playerFile, " %s", buf)==1) {
        if (buf[0]=='[') break;
        i = TYPEFIND(buf, skillList);
        if (i!=-1) {
          fscanf(playerFile, " %hd ", &Plr(player)->pSkill[i]);
        }
      }
    }

    /* Read all the Objects */
    if (PlayerReadHeader(playerFile, "[Objects]") && mode != PREAD_SCAN){
      fscanf(playerFile, " ");
      PlayerReadObject(player, playerFile, Thing(player), mode);
    }
    /* apply the affects of equipment, saved in virgin state */
    AffectApplyAll(player);

    /* Read all the Affects - these are applied as they load */
    if (PlayerReadHeader(playerFile, "[Affects]") && mode != PREAD_SCAN){
      while(fscanf(playerFile, " %s", buf)==1) {
        if (buf[0]=='[') break;
        i = TYPEFIND(buf, effectList);
        if (i!=-1) {
          affect = AffectCreate(player, AFFECT_NONE, 0, 0, i);
          AffectRead(player, affect, playerFile);
        } else
          fgets(buf, sizeof(buf), playerFile);
      }
    }

    /* Read all the Properties */
    if (PlayerReadHeader(playerFile, "[Properties]")){
      fscanf(playerFile, " ");
      fgets(buf, sizeof(buf), playerFile);
      while(!feof(playerFile)) {
        if (buf[0]!='P') break;
        sKey =  FileStrRead(playerFile);
        sDesc = FileStrRead(playerFile);
        Thing(player)->tProperty = PropertyCreate(Thing(player)->tProperty, sKey, sDesc);
        /* if this is code - compile it */
        if ( sKey->sText[0]=='@' && !CodeIsCompiled(Thing(player)->tProperty) )
          if (!CodeCompileProperty(Thing(player)->tProperty, NULL))
            CodeSetFlag(Thing(player), Thing(player)->tProperty);
        fgets(buf, sizeof(buf), playerFile);
      }
    }

    /* pack it in */
    fclose(playerFile);

  } else
    player = NULL;

  return Thing(player);
}

void PlayerWriteObject(FILE *playerFile, THING *object, BYTE indent, BYTE mode) {
  LWORD  apply;

  if (!object) return;
  if (object->tType != TTYPE_OBJ || Obj(object)->oTemplate->oVirtual<0) {
    /* cant save the system objects - save any contents though */
    PlayerWriteObject(playerFile, object->tContain, indent, mode);
    PlayerWriteObject(playerFile, object->tNext, indent, mode);
    return;
  }

  {
    LWORD i; /* this way this isnt tying up stack frame space during recursion */

    for(i=0;i<indent;i++) fprintf(playerFile, "  ");
    fprintf(playerFile, 
            "#%-5ld %ld %ld %ld %ld %ld %ld ",
            Obj(object)->oTemplate->oVirtual,
            Obj(object)->oDetail.lValue[0],
            Obj(object)->oDetail.lValue[1],
            Obj(object)->oDetail.lValue[2],
            Obj(object)->oDetail.lValue[3],
            Obj(object)->oEquip,
            Obj(object)->oAct);
    FileByteWrite(playerFile, Obj(object)->oTemplate->oType, ' ');
    fprintf(playerFile, "%s\n", object->tSDesc->sText);
    if (mode == PWRITE_PLAYER)
      Obj(object)->oTemplate->oOffline++;
    for (apply=0; apply<OBJECT_MAX_APPLY; apply++) {
      if (Obj(object)->oApply[apply].aType) {
        for(i=0;i<indent;i++) fprintf(playerFile, "  ");
        fprintf(playerFile, "A ");
        FILETYPEWRITE(playerFile, Obj(object)->oApply[apply].aType, applyList, ' ');
        FileByteWrite(playerFile, Obj(object)->oApply[apply].aValue, '\n');
      }
    }
  }
  
  if (object->tContain) {
    {
      LWORD i; /* this way this isnt tying up stack frame space during recursion */
      for(i=0;i<indent;i++) fprintf(playerFile, "  ");
    }
    fprintf(playerFile, "  {\n");
    PlayerWriteObject(playerFile, object->tContain, indent+1, mode);
    {
      LWORD i; /* this way this isnt tying up stack frame space during recursion */
      for(i=0;i<indent;i++) fprintf(playerFile, "  ");
    }
    fprintf(playerFile, "  }\n");
  }
  
  PlayerWriteObject(playerFile, object->tNext, indent, mode);
}

void PlayerUpdateTime(THING *player) {
  ULWORD    timeNow;

  /* Update timeOnline etc */
  timeNow = time(0);
  Plr(player)->pTimeTotal += timeNow-Plr(player)->pTimeLastOn;
  Plr(player)->pTimeLastOn = timeNow;
}

/* path should be either "crash" or "player", nothing else */
void PlayerWrite(THING *player, BYTE mode) {
  FILE     *playerFile;
  BYTE      playerFileBuf[128];
  BYTE      buf[256];
  LWORD     i;
  PROPERTY *property;
  AFFECT   *affect;

  if (!player) return;

  AffectUnapplyAll(player);

  switch(mode) {
  case PWRITE_PLAYER:
    sprintf(playerFileBuf, "crash/%s.plr", Thing(player)->tSDesc->sText);
    StrToLower(playerFileBuf);
    unlink(playerFileBuf);
    sprintf(playerFileBuf, "player/%s.plr", Thing(player)->tSDesc->sText);
    StrToLower(playerFileBuf);
    break;

  case PWRITE_CRASH:
    sprintf(playerFileBuf, "player/%s.plr", Thing(player)->tSDesc->sText);
    StrToLower(playerFileBuf);
    unlink(playerFileBuf);
    sprintf(playerFileBuf, "crash/%s.plr", Thing(player)->tSDesc->sText);
    StrToLower(playerFileBuf);
    break;

  case PWRITE_CLONE:
    sprintf(playerFileBuf, "clone/%s.plr", Thing(player)->tSDesc->sText);
    StrToLower(playerFileBuf);
    break;
  }

  playerFile = fopen(playerFileBuf, "wb");
  if (!playerFile) {
    sprintf(buf, "Unable to write %s\n", playerFileBuf);
    Log(LOG_ERROR, buf);
    PERROR("PlayerWrite");
    return;
  }
  FileStrWrite(playerFile, Thing(player)->tSDesc);
  FileStrWrite(playerFile, Plr(player)->pPassword);
  FileStrWrite(playerFile, Base(player)->bLDesc);
  FileStrWrite(playerFile, Thing(player)->tDesc);
  fprintf(playerFile, "Messages: \n");
  FileStrWrite(playerFile, Plr(player)->pEnter);
  FileStrWrite(playerFile, Plr(player)->pExit);
  FileStrWrite(playerFile, Plr(player)->pEmail);
  FileStrWrite(playerFile, Plr(player)->pPlan);

  fprintf(playerFile, "%-20s", "Prompt:");
  FileStrWrite(playerFile, Plr(player)->pPrompt);

  fprintf(playerFile, "%-20s", "LastLogin:");
  FileStrWrite(playerFile, Plr(player)->pLastLogin);

  fprintf(playerFile, "%-20s", "Level:");
  FileByteWrite(playerFile, Character(player)->cLevel, '\n');

  fprintf(playerFile, "%-20s", "Race:");
  FILETYPEWRITE(playerFile, Plr(player)->pRace, raceList, '\n');

  fprintf(playerFile, "%-20s", "Class:");
  FILETYPEWRITE(playerFile, Plr(player)->pClass, classList, '\n');

  fprintf(playerFile, "%-20s", "Position:");
  FILETYPEWRITE(playerFile, Character(player)->cPos, posList, '\n');
  fprintf(playerFile, "%-20s", "Sex:");
  FILETYPEWRITE(playerFile, Character(player)->cSex, sexList, '\n');

  fprintf(playerFile, "%-20s", "AutoAction:");
  FileFlagWrite(playerFile, Plr(player)->pAuto, pAutoList, '\n');

  fprintf(playerFile, "%-20s", "System:");
  FileFlagWrite(playerFile, Plr(player)->pSystem, pSystemList, '\n');

  fprintf(playerFile, "%-20s", "SockPref:");
  {
    SOCK *sock;
    sock = BaseControlFind(player);
    if (sock && sock->sMode==MODE_PLAY) 
      Plr(player)->pSockPref = sock->sPref;
  }
  FileFlagWrite(playerFile, Plr(player)->pSockPref, sfPrefList, '\n');

  fprintf(playerFile, "%-20s", "ScreenLines:");
  FileByteWrite(playerFile, Plr(player)->pScreenLines, '\n');

  fprintf(playerFile, "%-20s", "ColorPref:");
  for (i=0; i<COLORPREF_MAX; i++) {
    if (!*colorPrefList[i].cName) break;
    fprintf(playerFile, "%c%c ", Plr(player)->pColorPref[i].cFG, Plr(player)->pColorPref[i].cBG);
  }
  fprintf(playerFile, "\n");
/*
  fprintf(playerFile, "%-20s%ld\n", "Equip:",      Character(player)->cEquip);
*/
  fprintf(playerFile, "%-20s%ld\n", "Practice:",   Plr(player)->pPractice);
  fprintf(playerFile, "%-20s%ld\n", "HitP:",       Character(player)->cHitP);
  fprintf(playerFile, "%-20s%ld\n", "MoveP:",      Character(player)->cMoveP);
  fprintf(playerFile, "%-20s%ld\n", "PowerP:",     Character(player)->cPowerP);
  fprintf(playerFile, "%-20s%hd\n", "GainPractice:",Plr(player)->pGainPractice);
  fprintf(playerFile, "%-20s%hd\n", "GainHit:",    Plr(player)->pGainHit);
  fprintf(playerFile, "%-20s%hd\n", "GainMove:",   Plr(player)->pGainMove);
  fprintf(playerFile, "%-20s%hd\n", "GainPower:",  Plr(player)->pGainPower);
  fprintf(playerFile, "%-20s%hd\n", "Fame:",       Plr(player)->pFame);
  fprintf(playerFile, "%-20s%hd\n", "Infamy:",     Plr(player)->pInfamy);
  fprintf(playerFile, "%-20s%ld\n", "HitPMax:",    Character(player)->cHitPMax);
  fprintf(playerFile, "%-20s%ld\n", "MovePMax:",   Plr(player)->pMovePMax);
  fprintf(playerFile, "%-20s%ld\n", "PowerPMax:",  Plr(player)->pPowerPMax);
  fprintf(playerFile, "%-20s%hd\n", "Hunger:",     Plr(player)->pHunger);
  fprintf(playerFile, "%-20s%hd\n", "Thirst:",     Plr(player)->pThirst);
  fprintf(playerFile, "%-20s%hd\n", "Intox:",      Plr(player)->pIntox);
  fprintf(playerFile, "%-20s%hd\n", "Str:",        Plr(player)->pStr);
  fprintf(playerFile, "%-20s%hd\n", "Dex:",        Plr(player)->pDex);
  fprintf(playerFile, "%-20s%hd\n", "Con:",        Plr(player)->pCon);
  fprintf(playerFile, "%-20s%hd\n", "Wis:",        Plr(player)->pWis);
  fprintf(playerFile, "%-20s%hd\n", "Int:",        Plr(player)->pInt);
/*
  fprintf(playerFile, "%-20s%hd\n", "HitBonus:",   Character(player)->cHitBonus);
  fprintf(playerFile, "%-20s%hd\n", "DamBonus:",   Character(player)->cDamBonus);
  fprintf(playerFile, "%-20s%hd\n", "Armor:",      Character(player)->cArmor);
  for (i=0; i<CHAR_MAX_RESIST; i++) {
    sprintf(buf, "Resist%ld:", i);
    fprintf(playerFile, "%-20s%hd\n", buf,           Character(player)->cResist[i]);
  }
*/
  if (mode != PWRITE_CLONE) {
    fprintf(playerFile, "%-20s%ld\n", "Money:", Character(player)->cMoney);
    fprintf(playerFile, "%-20s%ld\n", "Bank:",  Plr(player)->pBank);
  } else {
    fprintf(playerFile, "%-20s%d\n", "Money:", 0);
    fprintf(playerFile, "%-20s%d\n", "Bank:",  0);
  }
  fprintf(playerFile, "%-20s%ld\n", "Exp:",        Character(player)->cExp);

  fprintf(playerFile, "%-20s%lu\n", "TimeTotal:",  Plr(player)->pTimeTotal);
  fprintf(playerFile, "%-20s%lu\n", "TimeLastOn:", Plr(player)->pTimeLastOn);
  fprintf(playerFile, "%-20s%hd\n", "Attempts:",   Plr(player)->pAttempts);
  fprintf(playerFile, "%-20s%ld\n", "StartRoom:",  Plr(player)->pStartRoom);
  fprintf(playerFile, "%-20s%ld\n", "RecallRoom:", Plr(player)->pRecallRoom);
  fprintf(playerFile, "%-20s%ld\n", "DeathRoom:",  Plr(player)->pDeathRoom);

  /* Write out all the Skills */
  fprintf(playerFile, "\n[Skills]\n");
  for(i=0; i<skillNum; i++) {
    if (Plr(player)->pSkill[i] != 0) {
      fprintf(playerFile, "%-20s%hd\n", skillList[i].sName, Plr(player)->pSkill[i]);
    }
  }
  fprintf(playerFile, "[End of Skills]\n");

  if (mode != PWRITE_CLONE) {
    /* Write out all the Objects */
    fprintf(playerFile, "\n[Objects]\n");
    PlayerWriteObject(playerFile, Thing(player)->tContain, 0, mode);
    fprintf(playerFile, "[End of Objects]\n");

    /* Write out affects */
    fprintf(playerFile, "\n[Affects]\n");
    for(affect = Character(player)->cAffect; affect; affect=affect->aNext)  
      AffectWrite(player, affect, playerFile);
    fprintf(playerFile, "[End of Affects]\n");
  }
  
  /* Properties */
  fprintf(playerFile, "\n[Properties]\n");
  for (property=player->tProperty; property; property=property->pNext) {
    fprintf(playerFile, "P\n");
    FileStrWrite(playerFile, property->pKey);
    if ( CodeIsCompiled(property) ){
      CodeDecompProperty(property, NULL);
      /* warn if it didnt decompile */
      if (CodeIsCompiled(property)) {
        sprintf(buf, "PlayerWrite: Property %s failed to decompile for player %s\n",
          property->pKey->sText,
          player->tSDesc->sText);
        Log(LOG_ERROR, buf);
        fprintf(playerFile, "/* <Decompile Error> */\n~\n");
      } else {
        FileStrWrite(playerFile, property->pDesc);
      }
    } else
      FileStrWrite(playerFile, property->pDesc);
  }
  fprintf(playerFile, "[End of Properties]\n");

  AffectApplyAll(player);

  /* all done close the file */
  fclose(playerFile);
}



/* the following routine is called by ThingFree, dont call it directly */
void PlayerFree(THING *thing) {

  STRFREE(Plr(thing)->pPassword);
  STRFREE(Plr(thing)->pEnter);
  STRFREE(Plr(thing)->pExit);
  STRFREE(Plr(thing)->pPrompt);
  STRFREE(Plr(thing)->pLastLogin);
  STRFREE(Plr(thing)->pEmail);
  STRFREE(Plr(thing)->pPlan);
  /* turf the index entry for this player */
  IndexDelete(&playerIndex, thing, NULL);
  /* give up the memory */
  MEMFREE(Plr(thing), PLR);
}

/* Eat the first thing we find whatever it is, 
  poison not withstanding */
LWORD PlayerAutoEat(THING *thing, THING *search) {
  if (!thing) return FALSE;
  if (Character(thing)->cPos < POS_SITTING) return FALSE;
  if (!search) return FALSE;
  if (search->tType != TTYPE_OBJ) {
    return PlayerAutoEat(thing, search->tNext);
  }
  if (Obj(search)->oTemplate->oType == OTYPE_CONTAINER) {
    if (!PlayerAutoEat(thing, search->tContain))
      return PlayerAutoEat(thing, search->tNext);
    return TRUE;
  }
  if (Obj(search)->oTemplate->oType != OTYPE_FOOD
    ||!InvUnEquip(thing, search, IUE_BLOCKABLE) ) {
    return PlayerAutoEat(thing, search->tNext);
  }
  InvEat(thing, search);
  return TRUE;
}

/* drink from the first drinkcon we find, whatever its
   contents */
LWORD PlayerAutoDrink(THING *thing, THING *search) {
  if (!thing) return FALSE;
  if (Character(thing)->cPos < POS_SITTING) return FALSE;
  if (!search) return FALSE;
  if (search->tType != TTYPE_OBJ) {
    return PlayerAutoDrink(thing, search->tNext);
  }
  if (Obj(search)->oTemplate->oType == OTYPE_CONTAINER) {
    if (!PlayerAutoDrink(thing, search->tContain))
      return PlayerAutoDrink(thing, search->tNext);
    return TRUE;
  }
  if (Obj(search)->oTemplate->oType != OTYPE_DRINKCON
    ||OBJECTGETFIELD(search, OF_DRINKCON_CONTAIN)==0 ) {
    return PlayerAutoDrink(thing, search->tNext);
  }
  InvDrink(thing, search);
  return TRUE;
}

void PlayerIdle() {
  BYTE   crashSave = 2; /* save this many players per beat */
  LWORD  i;
  THING  *thing;
  SOCK   *sock;

  if (playerIndex.iNum==0) return;
  for (i=0; i<playerIndex.iNum && crashSave>0; i++) {
    thing = playerIndex.iThing[i];
    /* Crash save 'em as necessary */
    sock = BaseControlFind(thing);
    if ( (!sock || sock->sMode == MODE_PLAY)
     && !BIT(Plr(thing)->pSystem, PS_CRASHSAVED)) {
      crashSave--;
      if (Base(playerIndex.iThing[i])->bInside 
       && Base(playerIndex.iThing[i])->bInside->tType == TTYPE_WLD)
        Plr(playerIndex.iThing[i])->pStartRoom = Wld(Base(playerIndex.iThing[i])->bInside)->wVirtual;
      PlayerWrite(thing, PWRITE_CRASH);
      BITSET(Plr(playerIndex.iThing[i])->pSystem, PS_CRASHSAVED);
      printf("AS_: Autosaving %s\n", thing->tSDesc->sText);
    }
  }
  
  /* if everyone has been crash saved, start over */
  if (crashSave>0) {
    for(i=0;i<playerIndex.iNum;i++)
      BITCLR(Plr(playerIndex.iThing[i])->pSystem, PS_CRASHSAVED);
  }
}

void PlayerTick() {
  LWORD  i=0;
  THING *thing=NULL;
  THING *obj=NULL;
  LWORD  gain;
  SOCK  *sock;
  BYTE   buf[256];

  if (playerIndex.iNum==0) return;
  while(1) {
    /* I know this while thing is a bit of a strange way to do it
       however at some point in the future we need to be able to 
       set thing to null and then kill/drop link to the thing in
       question and still continue through the loop */
    if (thing == playerIndex.iThing[i]) i++;
    if (i>=playerIndex.iNum) break;
    thing = playerIndex.iThing[i];

    /* Update the the amount of time they've played */
    PlayerUpdateTime(thing);

    /* Check for Affect timeouts ie spells expiring */
    if (CharTick(thing)) continue;

    /* Check for equipped items for stuff */
    for (obj = Base(thing)->bInside; obj; obj=obj->tNext) {
      if (obj->tType == TTYPE_OBJ && Obj(obj)->oEquip) {
        /* Check for Good items */
        if (BIT(Obj(obj)->oAct, OA_GOOD)) {
          if (Character(thing)->cAura < 400)
            Character(thing)->cAura +=1;
        }
        /* Check for evil items */
        if (BIT(Obj(obj)->oAct, OA_EVIL)) {
          if (Character(thing)->cAura > -400)
            Character(thing)->cAura -=1;
        }
      }
    }

    sock = BaseControlFind(thing);
    if (sock) {
      PlayerUpdateTime(thing);
    }

    if (Base(thing)->bInside) {
      if (!BIT(Plr(thing)->pAuto, PA_AFK)) {
        /* gain hit points */
        gain = classList[Plr(thing)->pClass].cGainHitP;
        gain = gain*raceList[Plr(thing)->pRace].rGainHitP/100;
        gain += Character(thing)->cLevel/20;
        gain += Plr(thing)->pCon/20;
        gain = CharGainAdjust(thing, GAIN_HITP, gain);
        Character(thing)->cHitP += gain;
        MAXSET(Character(thing)->cHitP, CharGetHitPMax(thing));
        if (sock && BIT(sock->sPref, SP_CHANTICK)) {
          sprintf(buf, "^pYou gain: ^w%ld ^ghp", gain);
          SendThing(buf, thing);
        }

        /* gain move points */
        gain = classList[Plr(thing)->pClass].cGainMoveP;
        gain = gain*raceList[Plr(thing)->pRace].rGainMoveP/100;
        gain += Character(thing)->cLevel/20;
        gain += Plr(thing)->pDex/20;
        gain = CharGainAdjust(thing, GAIN_MOVEP, gain);
        Character(thing)->cMoveP += gain;
        MAXSET(Character(thing)->cMoveP, CharGetMovePMax(thing));
        if (sock && BIT(sock->sPref, SP_CHANTICK)) {
          sprintf(buf, ", ^w%ld ^gmv", gain);
          SendThing(buf, thing);
        }
      
        /* gain power points */
        gain = classList[Plr(thing)->pClass].cGainPowerP;
        gain = gain*raceList[Plr(thing)->pRace].rGainPowerP/100;
        gain += Character(thing)->cLevel/20;
        gain += Plr(thing)->pInt/20;
        gain = CharGainAdjust(thing, GAIN_POWERP, gain);
        Character(thing)->cPowerP += gain;
        MAXSET(Character(thing)->cPowerP, CharGetPowerPMax(thing));
        if (sock && BIT(sock->sPref, SP_CHANTICK)) {
          sprintf(buf, ", and ^w%ld ^gpp\n", gain);
          SendThing(buf, thing);
        }
      } else {
        if (sock && BIT(sock->sPref, SP_CHANTICK)) {
          SendThing("^yWhile AFK you do not heal/rest or get hungry/thirsty!\n", thing);
        }
      }
      
      if (   Character(thing)->cPowerP == CharGetPowerPMax(thing)
          && Character(thing)->cMoveP == CharGetMovePMax(thing)
          && Character(thing)->cHitP == CharGetHitPMax(thing)
          && Character(thing)->cPos >= POS_SLEEPING
          )
        if ( Character(thing)->cPos == POS_SLEEPING && !BIT(Plr(thing)->pAuto, PA_EXPERT))
          SendHint("^;HINT: You're fully rested! You should ^<WAKE^; up now and carry on!\n", thing);
        else if ( Character(thing)->cPos <= POS_SITTING  && !BIT(Plr(thing)->pAuto, PA_EXPERT))
          SendHint("^;HINT: You're fully rested! You should ^<STAND^; up now and carry on!\n", thing);

      if (!ParseCommandCheck(PARSE_COMMAND_WCREATE, sock, "") 
       && Character(thing)->cLevel >= LEVEL_HUNGER
       && !BIT(Plr(thing)->pAuto, PA_AFK)) {
        /* Gain Hunger */
        /* if (Plr(thing)->pHunger>=(-1*raceList[Plr(thing)->pRace].rMaxHunger)){ */
        gain = raceList[Plr(thing)->pRace].rGainHunger;
        gain = CharGainAdjust(thing, GAIN_HUNGER, gain);
        MINSET(gain, 1);
        Plr(thing)->pHunger += gain;
        MAXSET(Plr(thing)->pHunger, raceList[Plr(thing)->pRace].rMaxHunger);
        if (Plr(thing)->pHunger>0) {
          SendThing("You are hungry!\n", thing);
          if (BIT(Plr(thing)->pAuto, PA_AUTOEAT)) {
            /* search your inventory for something to eat */
            PlayerAutoEat(thing, thing->tContain);
          }
        }
      
        /* Gain Thirst */
        /* if (Plr(thing)->pThirst>=(-1*raceList[Plr(thing)->pRace].rMaxThirst)){ */
        gain = raceList[Plr(thing)->pRace].rGainThirst;
        gain = CharGainAdjust(thing, GAIN_THIRST, gain);
        MINSET(gain, 1);
        Plr(thing)->pThirst += gain;
        MAXSET(Plr(thing)->pThirst, raceList[Plr(thing)->pRace].rMaxThirst);
        if (Plr(thing)->pThirst>0) {
          SendThing("You are thirsty!\n", thing);
          if (BIT(Plr(thing)->pAuto, PA_AUTODRINK)) {
            /* search your inventory for something to drink */
            PlayerAutoDrink(thing, thing->tContain);
          }
        }
      } 
      
      /* Lose Intox */
      /* if (Plr(thing)->pIntox>=(-1*raceList[Plr(thing)->pRace].rMaxIntox)){ */
      Plr(thing)->pIntox -= raceList[Plr(thing)->pRace].rGainIntox;
      MAXSET(Plr(thing)->pIntox, raceList[Plr(thing)->pRace].rMaxIntox);
      MINSET(Plr(thing)->pIntox, -raceList[Plr(thing)->pRace].rMaxIntox);
      if (Plr(thing)->pIntox>0) {
        SendThing("You are intoxicated!\n", thing);
      }

      if (Character(thing)->cLevel >= LEVEL_GOD ) {
        /* gods dont get hungry etc */
        Plr(thing)->pHunger = -1;
        Plr(thing)->pThirst = -1;
        Plr(thing)->pIntox  = -1;
      }
      
      /* Check to see if they have been idle too long */
      Plr(thing)->pIdleTick++;
      if ((!Character(thing)->cFight)&&(Character(thing)->cLevel<LEVEL_GOD)){
        if (Plr(thing)->pIdleTick > playerIdleTick && !Plr(thing)->pIdleRoom) {
          /* Suck them into the void */
          SendThing("^wYou have been idle and are pulled into nothingness\n", thing);
          SendAction("$n disappears into the void\n",
            thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
          Plr(thing)->pIdleRoom = Base(thing)->bInside;
          ThingTo(thing, WorldOf(playerIdleRoom));
        } else if ( (Plr(thing)->pIdleTick > playerDropTick
                    &&!BIT(Plr(thing)->pAuto, PA_AFK)
                    &&sock) 
                  ||(Plr(thing)->pIdleTick > playerAFKDropTick)) {
          if (sock)
            sock->sMode = MODE_KILLSOCKET;
            
          /* 
           * rent out char here (if they have the money) 
           */
          
          /* Update where they start */
          if (thing->tType == TTYPE_PLR && Base(thing)->bInside && Base(thing)->bInside->tType==TTYPE_WLD)
            Plr(thing)->pStartRoom = Wld(Base(thing)->bInside)->wVirtual;
          else
            Plr(thing)->pStartRoom = -1;
          PlayerWrite(thing, PWRITE_PLAYER);
        }
      }
    }
  }
}

/* Turns out that normalize is way too generous */
#define NORMALIZE 0
void PlayerRollAbilities(THING *thing, LWORD reroll) {
  LWORD min;
  LWORD max;
  LWORD bonus;
  LWORD str = -1;
  LWORD dex = -1;
  LWORD con = -1;
  LWORD wis = -1;
  LWORD intel = -1; /* int is of course reserved.... */
  LWORD newStr;
  LWORD newDex;
  LWORD newCon;
  LWORD newWis;
  LWORD newIntel;
  LWORD i;
#if (NORMALIZE)
  float normal;
  float actual;
#endif

  /* Roll Abilities */
  for (i=0; i<reroll; i++) {
    min =   raceList[Plr(thing)->pRace].rMinStat[SA_STR];
    max =   raceList[Plr(thing)->pRace].rMaxStat[SA_STR];
    bonus = raceList[Plr(thing)->pRace].rBonusStat[SA_STR];
    bonus += classList[Plr(thing)->pClass].cBonusStat[SA_STR];
    newStr = Number(min, max) + bonus;
    min =   raceList[Plr(thing)->pRace].rMinStat[SA_DEX];
    max =   raceList[Plr(thing)->pRace].rMaxStat[SA_DEX];
    bonus = raceList[Plr(thing)->pRace].rBonusStat[SA_DEX];
    bonus += classList[Plr(thing)->pClass].cBonusStat[SA_DEX];
    newDex = Number(min, max) + bonus;
    min =   raceList[Plr(thing)->pRace].rMinStat[SA_CON];
    max =   raceList[Plr(thing)->pRace].rMaxStat[SA_CON];
    bonus = raceList[Plr(thing)->pRace].rBonusStat[SA_CON];
    bonus += classList[Plr(thing)->pClass].cBonusStat[SA_CON];
    newCon = Number(min, max) + bonus;
    min =   raceList[Plr(thing)->pRace].rMinStat[SA_WIS];
    max =   raceList[Plr(thing)->pRace].rMaxStat[SA_WIS];
    bonus = raceList[Plr(thing)->pRace].rBonusStat[SA_WIS];
    bonus += classList[Plr(thing)->pClass].cBonusStat[SA_WIS];
    newWis = Number(min, max) + bonus;
    min =   raceList[Plr(thing)->pRace].rMinStat[SA_INT];
    max =   raceList[Plr(thing)->pRace].rMaxStat[SA_INT];
    bonus = raceList[Plr(thing)->pRace].rBonusStat[SA_INT];
    bonus += classList[Plr(thing)->pClass].cBonusStat[SA_INT];
    newIntel = Number(min, max) + bonus;

    /* Keep best average */
    if (newStr+newDex+newCon+newWis+newIntel > str+dex+con+wis+intel) {
      str = newStr;
      dex = newDex;
      con = newCon;
      wis = newWis;
      intel = newIntel;
    }
  }

#if (NORMALIZE)
  /* Normalize */
  normal = raceList[Plr(thing)->pRace].rNormalize;
  if (normal > 0) { /* -1 or 0 in race table disables normalization */
    actual = (float) 
            (str
           + dex
           + con
           + wis
           + intel);
    actual = actual / 5.0;

    /* okay changed my mind - ensure actual average is at least the normal one */
    if (actual < normal) {
      str   = (LWORD) ( (float)str   * normal / actual );
      dex   = (LWORD) ( (float)dex   * normal / actual );
      con   = (LWORD) ( (float)con   * normal / actual );
      wis   = (LWORD) ( (float)wis   * normal / actual );
      intel = (LWORD) ( (float)intel * normal / actual );
    }
  }
#endif

  /* watch the maximums */
  MAXSET(str, raceList[Plr(thing)->pRace].rMaxStat[SA_STR]);
  MAXSET(dex, raceList[Plr(thing)->pRace].rMaxStat[SA_DEX]);
  MAXSET(con, raceList[Plr(thing)->pRace].rMaxStat[SA_CON]);
  MAXSET(wis, raceList[Plr(thing)->pRace].rMaxStat[SA_WIS]);
  MAXSET(intel, raceList[Plr(thing)->pRace].rMaxStat[SA_INT]);
 
  /* watch the minimums */ 
  MINSET(str, raceList[Plr(thing)->pRace].rMinStat[SA_STR]);
  MINSET(dex, raceList[Plr(thing)->pRace].rMinStat[SA_DEX]);
  MINSET(con, raceList[Plr(thing)->pRace].rMinStat[SA_CON]);
  MINSET(wis, raceList[Plr(thing)->pRace].rMinStat[SA_WIS]);
  MINSET(intel, raceList[Plr(thing)->pRace].rMinStat[SA_INT]);

  Plr(thing)->pStr = str;
  Plr(thing)->pDex = dex;
  Plr(thing)->pCon = con;
  Plr(thing)->pWis = wis;
  Plr(thing)->pInt = intel;

  /* Set starting hit points etc if they are 1st level */
  if (Character(thing)->cLevel == 1) {
    /* Set Practices */
    Plr(thing)->pPractice = Plr(thing)->pWis/30+5;
    if (Plr(thing)->pInt<60) Plr(thing)->pPractice+=1; /* Dumber people get more pracs */
    if (Plr(thing)->pInt<30) Plr(thing)->pPractice+=2; /* Just starting out - they'll need 'em */
    /* Set Hit Points */
    max = classList[Plr(thing)->pClass].cStartHitP;
    max = max*raceList[Plr(thing)->pRace].rMaxHitP/100;
    Character(thing)->cHitPMax = max;
    Character(thing)->cHitP = CharGetHitPMax(thing);
    /* gain move points */
    max = classList[Plr(thing)->pClass].cStartMoveP;
    max = max*raceList[Plr(thing)->pRace].rMaxMoveP/100;
    Plr(thing)->pMovePMax = max;
    Character(thing)->cMoveP = CharGetMovePMax(thing);
    /* gain power points */
    max = classList[Plr(thing)->pClass].cStartPowerP;
    max = max*raceList[Plr(thing)->pRace].rMaxPowerP/100;
    Plr(thing)->pPowerPMax = max;
    Character(thing)->cPowerP = CharGetPowerPMax(thing);
  }
}



/* How much do we need for next level */
LWORD PlayerExpNeeded(THING *thing) {
  LWORD needed;

  needed = (1+Character(thing)->cLevel);
  needed *= 50;
  needed *= Character(thing)->cLevel;
  needed *= Character(thing)->cLevel;

  /* create plateau right before Hero level */
  if (Character(thing)->cLevel >= LEVEL_LEGEND-1) {
    needed += 20000000;
    needed += (Character(thing)->cLevel-( LEVEL_LEGEND-2 ))*2000000;
  }
  needed = needed * classList[Plr(thing)->pClass].cGainLevel / 100;
  needed = needed * raceList[Plr(thing)->pRace].rGainLevel / 100;
  return MAXV(needed,0);
}

LWORD PlayerExpUntilLevel(THING *thing) {
  LWORD expUntilLevel;

  if (thing->tType!=TTYPE_PLR) return -1;
  expUntilLevel = PlayerExpNeeded(thing)-Character(thing)->cExp;
  MINSET(expUntilLevel, 0);
  return expUntilLevel;
}

void PlayerGainLevel(THING *thing, LWORD message) {
  BYTE  buf[256];
  LWORD gain;
  SOCK *sock;
  BYTE  expert = FALSE;

  sock = BaseControlFind(thing);
  if (sock && BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) expert = TRUE;
  Character(thing)->cLevel += 1;
  MAXSET(Character(thing)->cExp, PlayerExpNeeded(thing)-1);
  if (message) SendThing("^wGRATZ! You just gained a level!\n", thing);

  /* gain practices */
  if (!Plr(thing)->pGainPractice) {
    gain = Plr(thing)->pWis/20;
    if (Character(thing)->cLevel > LEVEL_GREEN)
      gain += Number(1, 10);
    else
      gain += Number(1, 8);
  } else {
    gain = Plr(thing)->pGainPractice;
  }
  Plr(thing)->pGainPractice = 0;
  Plr(thing)->pPractice += gain;
  sprintf(buf, "^wYou gain %ld practices (You now have %ld available)\n", gain, Plr(thing)->pPractice);
  SendThing(buf, thing);

  /* gain hit points */
  if (!Plr(thing)->pGainHit) {
    gain = classList[Plr(thing)->pClass].cMaxHitP;
    if (Character(thing)->cLevel > LEVEL_GREEN)
      gain = Number(1, gain);
  } else {
    gain = Plr(thing)->pGainHit;
  }
  Plr(thing)->pGainHit = 0;
  gain = gain*raceList[Plr(thing)->pRace].rMaxHitP/100;
  Character(thing)->cHitPMax += gain;
  if (message) {
    sprintf(buf, "^wYou gain %ld hit points (Your maximum is now %ld)\n", gain, CharGetHitPMax(thing));
    SendThing(buf, thing);
  }

  /* gain move points */
  if (!Plr(thing)->pGainMove) {
    gain = classList[Plr(thing)->pClass].cMaxMoveP;
    if (Character(thing)->cLevel > LEVEL_GREEN)
      gain = Number(1, gain);
  } else {
    gain = Plr(thing)->pGainMove;
  }
  Plr(thing)->pGainMove = 0;
  gain = gain*raceList[Plr(thing)->pRace].rMaxMoveP/100;
  Plr(thing)->pMovePMax += gain;
  if (message) {
    if (expert || Character(thing)->cLevel >= 5) {
      sprintf(buf, "^wYou gain %ld move points (Your maximum is now %ld)\n", gain, CharGetMovePMax(thing));
      SendThing(buf, thing);
    }
  }

  /* gain power points */
  if (!Plr(thing)->pGainPower) {
    gain = classList[Plr(thing)->pClass].cMaxPowerP;
    if (Character(thing)->cLevel > LEVEL_GREEN)
      gain = Number(1, gain);
  } else {
    gain = Plr(thing)->pGainPower;
  }
  Plr(thing)->pGainPower = 0;
  gain = gain*raceList[Plr(thing)->pRace].rMaxPowerP/100;
  Plr(thing)->pPowerPMax += gain;
  if (message) {
    if (expert || Character(thing)->cLevel >= 10) {
      sprintf(buf, "^wYou gain %ld power points (Your maximum is now %ld)\n", gain, CharGetPowerPMax(thing));
      SendThing(buf, thing);
    }
  }
  
  if (Character(thing)->cLevel == LEVEL_MOVETIRING) {
    if (message) SendThing("NEW: Moving will now tire you, and you will have to rest periodically\n", thing);
  } else if (Character(thing)->cLevel == LEVEL_HUNGER) {
    if (message) SendThing("NEW: You will now need food and drink periodically\n", thing);
    Plr(thing)->pThirst = -1*raceList[Plr(thing)->pRace].rMaxThirst;
    Plr(thing)->pHunger = -1*raceList[Plr(thing)->pRace].rMaxHunger;
    
  } else if (Character(thing)->cLevel == LEVEL_FIGHTTIRING) {
    if (message) SendThing("NEW: Fighting will now tire you, and you will have to rest periodically\n", thing);
  }

  if (!Character(thing)->cLevel < 5) {
    if (message) SendHint("^;HINT: You have practices! Locate Duah and improve your skills\n", thing);
  }
  if (message) SendHint("^;HINT: Save your progress! Visit Club Med and have a brain dump done!\n", thing);
}


void PlayerGainExp(THING *thing, LWORD exp) {
  BYTE   buf[256];
  THING *world;
  LWORD  expUntilLevel;
  LWORD  gain;

  if (thing->tType != TTYPE_PLR) return;

  /* guard against no gain zones etc here */
  world = Base(thing)->bInside;
  if (world->tType==TTYPE_WLD 
   && BIT(areaList[Wld(world)->wArea].aResetFlag, RF_NOGAIN)) {
    sprintf(buf, "^yYou ^rWOULD ^yhave gained %ld experience points! (This zone isnt open yet)\n", exp);
    SendThing(buf, thing);
    return;
  }
  
  /* check if they're a god */
  if (Character(thing)->cLevel>=LEVEL_GOD-1) {
    sprintf(buf, "^yYou ^rWOULD ^yhave gained %ld experience points! (except that you're a god)\n", exp);
    SendThing(buf, thing);
    return;
  }
  
  /* Check if they're a area editor */
  if (ParseCommandCheck(PARSE_COMMAND_WGOTO, BaseControlFind(thing), "")) {
    sprintf(buf, "^yYou ^rWOULD ^yhave gained %ld experience points! (except that you're the area creator)\n", exp);
    SendThing(buf, thing);
    return;
  }

  expUntilLevel = PlayerExpNeeded(thing)-Character(thing)->cExp-exp;
  if (expUntilLevel>0) {
    sprintf(buf, "^yYou just gained %ld experience points! (%ld until next level)\n", exp, expUntilLevel);
    SendThing(buf, thing);
  } else {
    sprintf(buf, "^yYou just gained %ld experience points! (^wLEVEL!!!^y)\n", exp);
    SendThing(buf, thing);
  }

  /* Predetermine Practice roll to prevent kill/relevel "bug" */
  if (!Plr(thing)->pGainPractice) {
    gain = Plr(thing)->pWis/20;
    if (Character(thing)->cLevel > LEVEL_GREEN)
      gain += Number(1, 10);
    else
      gain += Number(1, 8);
    Plr(thing)->pGainPractice = gain;
  }

  /* Predetermine hit roll to prevent kill/relevel "bug" */
  if (!Plr(thing)->pGainHit && !Number(0,5)) {
    gain = classList[Plr(thing)->pClass].cMaxHitP;
    if (Character(thing)->cLevel > LEVEL_GREEN)
      gain = Number(1, gain);
    Plr(thing)->pGainHit= gain;
  }

  /* Predetermine move roll to prevent kill/relevel "bug" */
  if (!Plr(thing)->pGainMove && !Number(0,5)) {
    gain = classList[Plr(thing)->pClass].cMaxMoveP;
    if (Character(thing)->cLevel > LEVEL_GREEN)
      gain = Number(1, gain);
    Plr(thing)->pGainMove= gain;
  }

  /* Predetermine power roll to prevent kill/relevel "bug" */
  if (!Plr(thing)->pGainPower && !Number(0,5)) {
    gain = classList[Plr(thing)->pClass].cMaxPowerP;
    if (Character(thing)->cLevel > LEVEL_GREEN)
      gain = Number(1, gain);
    Plr(thing)->pGainPower = gain;
  }

  Character(thing)->cExp += exp;
  if (Character(thing)->cExp > PlayerExpNeeded(thing)) {
    PlayerGainLevel(thing, TRUE);
  }
}

void PlayerGainFame(THING *player, LWORD fame) {
  BYTE buf[256];

  if (!fame) return;
  if (!player || player->tType!=TTYPE_PLR) return;
  if (fame>0)
    sprintf(buf, "^yYou gain %ld points of fame!\n", fame);
  else
    sprintf(buf, "^rYou lose %ld points of fame!\n", fame);
  Plr(player)->pFame+=fame;
  SendThing(buf, player);
}

void PlayerGainInfamy(THING *player, LWORD infamy, BYTE *message) {
  BYTE  buf[256];
  LWORD infamous;

  if (!infamy) return;
  if (!player || player->tType!=TTYPE_PLR) return;
  infamous = PlayerIsInfamous(player);
  if (!message) {
    if (infamy>0)
      sprintf(buf, "^rYou gain %ld points of infamy!\n", infamy);
    else
      sprintf(buf, "^yYou lose %ld points of infamy!\n", infamy);
    SendThing(buf, player);
  } else {
    SendThing(message, player);
  }
  Plr(player)->pInfamy+=infamy;

  if (!infamous && PlayerIsInfamous(player))  
    SendThing("^wUh Oh, start running now - you just became infamous\n", player);
}

LWORD PlayerIsInfamous(THING *player) {
  if (player->tType!=TTYPE_PLR) return FALSE;
  if (Plr(player)->pInfamy > Character(player)->cLevel + Plr(player)->pFame)
    return Plr(player)->pInfamy - ( Character(player)->cLevel + Plr(player)->pFame );
  return FALSE;
}

void PlayerDelete(THING *player) {
  SOCK *sock;
  BYTE  buf[256];
  BYTE  playerName[256];

  /* get rid of the playerFile - but keep a backup in case we change our mind */
  PlayerWrite(player, PWRITE_CRASH);

  strcpy(playerName, player->tSDesc->sText);
  StrToLower(playerName);
#ifdef WIN32
  sprintf(buf, "move crash\\%s.plr player\\%s.plr.del", playerName, playerName);
#else
  sprintf(buf, "mv crash/%s.plr player/%s.plr.del", playerName, playerName);
#endif
  system(buf);
  sprintf(buf, "clone/%s.plr", playerName);
  unlink(buf);
  sprintf(buf, "player/%s.plr", playerName);
  unlink(buf);
  sprintf(buf, "alias/%s.als", playerName);
  unlink(buf);

  sock = BaseControlFind(player);
  if (sock) {
    BaseControlFree(sock->sControlThing, sock);
    sock->sMode = MODE_KILLSOCKET; /* they're not long for this world.... */
  }
  /* turf the playerThing */
  
  THINGFREE(player);
}


BYTE *PlayerGetLevelDesc(THING *thing) {
  if (!thing) return "NONE";
  if (Character(thing)->cLevel < LEVEL_GREEN)
    return "NEWBIE";
  if (Character(thing)->cLevel < LEVEL_NOVICE)
    return "GREEN";
  if (Character(thing)->cLevel < LEVEL_EXPERIENCED)
    return "NOVICE";
  if (Character(thing)->cLevel < LEVEL_ELITE)
    return "EXPERIENCED";
  if (Character(thing)->cLevel < LEVEL_ULTRAELITE)
    return "ELITE";
  if (Character(thing)->cLevel < LEVEL_LEGEND)
    return "ULTRAELITE";
  if (Character(thing)->cLevel < LEVEL_GOD)
    return "LEGEND";
  if (Character(thing)->cLevel < LEVEL_ADMIN)
    return "GOD";
  if (Character(thing)->cLevel < LEVEL_CODER)
    return "ADMIN";
  else
    return "CODER";
}
