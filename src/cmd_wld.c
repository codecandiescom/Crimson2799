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

/*****************************************************************
 *                                                               *
 *                                                               *
 *                     W L D    S T U F F                        *
 *                                                               *
 *                                                               *
 *****************************************************************/

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
#include "property.h"
#include "code.h"
#include "file.h"
#include "thing.h"
#include "reset.h"
#include "exit.h"
#include "index.h"
#include "world.h"
#include "area.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "send.h"
#include "base.h"
#include "char.h"
#include "fight.h"
#include "player.h"
#include "parse.h"
#include "cmd_move.h"
#include "cmd_wld.h"


THING *WldGetWorld(THING *thing, BYTE **cmd) {
  THING *world;
  LWORD  numScan;
  LWORD  virtual = -1;

  if (cmd) {
    numScan = sscanf(*cmd, " %ld", &virtual);
  } else {
    numScan = 0;
  }
  if (numScan>0) {
    *cmd = StrOneWord(*cmd, NULL);
    world = WorldOf(virtual);
    if (!world || world->tType != TTYPE_WLD) {
      SendThing("Sorry, no WORLD with that virtual number exists\n", thing);
      return NULL;
    }
  } else {
    world = Base(thing)->bInside;
    if (!world || world->tType != TTYPE_WLD) {
      SendThing("I'm afraid you're not inside a WORLD right at the moment\n", thing);
      return NULL;
    }
  }
    /* Check for authorization here */
  return world;
}


CMDPROC(CmdWStat) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING    *world;
  BYTE      buf[512];
  BYTE      truncateStr[512];
  EXTRA    *extra;
  PROPERTY *property;
  EXIT     *exit;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  /* show them the stats on the WORLD */
  /* first line, # Name & Type */
  sprintf(buf, "^g#:^G[^w%ld^G] ^gName:^G[^c", Wld(world)->wVirtual);
  SendThing(buf, thing);
  SendThing(world->tSDesc->sText, thing);
  SendThing("^G] ^gType:^G[^c", thing);
  SendThing(TYPESPRINTF(buf, Wld(world)->wType, wTypeList, 512), thing);
  SendThing("^G]\n", thing);

  /* 2nd line - flags */
  SendThing("^gFlags: ^c", thing);
  SendThing(FlagSprintf(buf, Wld(world)->wFlag, wFlagList, ' ', 512), thing);
  SendThing("^G]\n", thing);

  SendThing("^gDescription:^b\n", thing);
  SendThing(world->tDesc->sText, thing);
  for (extra = world->tExtra; extra; extra=extra->eNext) {
    sprintf(buf,
            "^gExtra: ^c%-31s = ",
            StrTruncate(truncateStr,extra->eKey->sText,  30));
    SendThing(buf, thing);
    StrTruncate(truncateStr,extra->eDesc->sText, 30);
    StrOneLine(truncateStr);
    sprintf(buf, "%s\n", truncateStr);
    SendThing(buf, thing);
  }
  for (property = world->tProperty; property; property=property->pNext) {
    sprintf(buf,
            "^gProp.: ^c%-31s = ",
            StrTruncate(truncateStr,property->pKey->sText,  30));
    SendThing(buf, thing);
    StrTruncate(truncateStr,property->pDesc->sText, 30);
    StrOneLine(truncateStr);
    sprintf(buf, "%s\n", truncateStr);
    SendThing(buf, thing);
  }

  SendThing("^gExits:\n", thing);
  for (exit = Wld(world)->wExit; exit; exit=exit->eNext) {
    sprintf(buf, "^G[^c%-5s^G] ^gto room", dirList[exit->eDir]);
    SendThing(buf, thing);
    if (!exit->eWorld) {
      SendThing("^G[^c NULL^G]", thing);
    } else {
      sprintf(buf, "^G[^w%5ld^G]", Wld(exit->eWorld)->wVirtual);
      SendThing(buf, thing);
    }
    SendThing("^g, flags:^G[^b", thing);
    SendThing(FlagSprintf(buf, exit->eFlag, eFlagList, ' ', 512), thing);
    SendThing("^G]\n", thing);
    if (exit->eKeyObj!=-1 || exit->eKey->sText[0]) {
      sprintf(buf, "^gKey:^G[^b%5ld^G] ^gKeywords:^G[^b", exit->eKeyObj);
      SendThing(buf, thing);
      SendThing(exit->eKey->sText, thing);
      SendThing("^G]\n", thing);
    }
  }
}


CMDPROC(CmdWList) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   truncateStr[256];
  BYTE   buf[256];
  LWORD  i;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  /* List the rooms */
  for (i=0; i<areaList[Wld(world)->wArea].aWldIndex.iNum; i++) {
    world = areaList[Wld(world)->wArea].aWldIndex.iThing[i];
    sprintf(buf, "^w%5ld ^c%-19s", Wld(world)->wVirtual, StrTruncate(truncateStr,world->tSDesc->sText, 19));
    SendThing(buf, thing);
    if (i%3 == 2)
      SendThing("\n", thing);
    else
      SendThing(" ", thing);
  }
  if (i%3) /* one i++ past i%3 == 2 */
    SendThing("\n", thing); /* last line allways gets a return */
  sprintf(buf, "^g%ld ^brooms listed.\n", i);
  SendThing(buf, thing);
}


CMDPROC(CmdWCreate) {
  WLD   *world;
  LWORD  area;
  BYTE   buf[256];
  LWORD  i;
  INDEX  *index;

  cmd = StrOneWord(cmd, NULL);
  world = Wld(WldGetWorld(thing, &cmd));
  if (!world)
    return;
  area = world->wArea;
  if (areaList[area].aWldIndex.iNum == (areaList[area].aVirtualMax - areaList[area].aVirtualMin + 1)) {
    SendThing("^gBad news.... This Area is completely full\n", thing);
    return;
  }

  MEMALLOC(world, WLD, WORLD_ALLOC_SIZE);
  /* Init the data here */
  memset( (void*)world, 0, sizeof(WLD)); /* init to zeros */
  world->wArea         = area;
  Thing(world)->tType  = TTYPE_WLD;
  Thing(world)->tSDesc = STRCREATE("Type ^wWNAME^c to give this room a better title.");
  Thing(world)->tDesc  = STRCREATE("  You are standing in a completely featureless room. It awaits only a creators\nhand to stroke the keyboard spelling out the magic word ^wWDESC^b to breath life\nupon it.\n");
  world->wFlag=0; /* flags for this room */
  world->wType=0; /* type of this room, ie city, river, underwater etc */

  /* update total num of worlds in the game */
  worldNum++;

  /* give the room the first available virtual number */
  world->wVirtual = areaList[area].aVirtualMin;
  index = &areaList[area].aWldIndex;
  i=0;
  while( (i<=index->iNum-1) && (world->wVirtual>=Wld(index->iThing[i])->wVirtual) ){
    if (world->wVirtual==Wld(index->iThing[i])->wVirtual)
      world->wVirtual++;
    else 
      i++;
  }
  /* insert it into index */
  IndexInsert(index, Thing(world), WorldCompareProc);

  /* Goto the new room */
  BITSET(areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
  sprintf(buf, "^bYou have been placed inside the newly created location #^g[%5ld]\n", Wld(world)->wVirtual);
  SendThing(buf, thing);
  ThingTo(thing, Thing(world));
  CmdLook(thing, "");
}


CMDPROC(CmdWCopy) {
  WLD   *world;
  THING *copyFrom;
  LWORD  area;

  cmd = StrOneWord(cmd, NULL);
  world = Wld(WldGetWorld(thing, &cmd));
  if (!world)
    return;
  area = world->wArea;

  copyFrom = WorldOf(atol(cmd));
  if (!*cmd || !copyFrom) {
    SendThing("^gTry WCOPY <dest#> <source#>\n", thing);
    return;
  }
  if (copyFrom == Thing(world)) {
    SendThing("^gYou cant copy a room overtop of itself\n", thing);
    return;
  }

  STRFREE(Thing(world)->tSDesc);
  STRFREE(Thing(world)->tDesc);
  Thing(world)->tSDesc = StrAlloc(Thing(copyFrom)->tSDesc);
  Thing(world)->tDesc  = StrAlloc(Thing(copyFrom)->tDesc);
  world->wFlag=Wld(copyFrom)->wFlag; /* flags for this room */
  world->wType=Wld(copyFrom)->wType; /* type of this room, ie city, river, underwater etc */
  while (Thing(world)->tExtra) 
    Thing(world)->tExtra = ExtraFree(Thing(world)->tExtra, Thing(world)->tExtra);
  Thing(world)->tExtra    = ExtraCopy(copyFrom->tExtra);
  Thing(world)->tProperty = PropertyCopy(copyFrom->tProperty);

  SendThing("^wName, Description, WFlags, Extra's, Properties and WType copied\n", thing);
  CmdLook(thing, "");
}


CMDPROC(CmdWName) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   strName[256];

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  /* Check editor status */

  /* Edit the requisite string */
  SendHint("^;HINT: Names should not end on a blank line\n", thing);
  SendHint("^;HINT: The next thing you should do is use ^vWDESC^; to add a description\n", thing);
  sprintf(strName, "World #%ld - Name", Wld(world)->wVirtual);
  EDITSTR(thing, world->tSDesc, 256, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}



CMDPROC(CmdWDesc) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   strName[256];

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  /* Check editor status */

  /* Edit the requisite string */
  SendHint("^;HINT: Descriptions must end with a blank line\n", thing);
  sprintf(strName, "World #%ld - Detailed Description", Wld(world)->wVirtual);
  EDITSTR(thing, world->tDesc, 4096, strName, EP_ENDLF);
  EDITFLAG(thing, &areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}

CMDPROC(CmdWExit) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   strName[256];
  BYTE   argDir[256];
  BYTE   argVirtual[256];
  EXIT  *exit;
  LWORD  exitDir;
  BYTE   exitKey[256];
  THING *exitWorld;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  cmd = StrOneWord(cmd, argDir);
  if (!*argDir) { /* basic help */
    SendThing("^GUSUAGE: ^gWEXIT <DIR> [<WORLD#> | DELETE]\n", thing);
    SendThing("^CE.G.    ^cwexit north 3001\n", thing);
    SendThing("^CE.G.    ^cwexit north 3001 oneway\n", thing);
    SendThing("^CE.G.    ^cwexit north delete\n", thing);
    return;
  }
  exitDir = TYPEFIND(argDir, dirList);
  if (exitDir == -1) {
    SendThing("^wUnknown Direction specified.\n", thing);   
    return;
  } else {
    sprintf(argDir, "%s", dirList[exitDir]);
    strcpy(exitKey,"");;
  }
  
  /* delete existing exit if any */
  exit = ExitDir(Wld(world)->wExit, exitDir);
  if (exit) {
    Wld(world)->wExit = ExitFree(Wld(world)->wExit, exit);
  }

  cmd = StrOneWord(cmd, argVirtual);
  if (StrAbbrev("delete", argVirtual)) {
    SendThing("Exit deleted.\n", thing);
    BITSET(areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
    return;
  } else {
    /* find the exit destination */
    exitWorld = WorldOf(atoi(argVirtual));
    if (!exitWorld || exitWorld->tType != TTYPE_WLD) {
      SendThing("Destination Virtual number does not exist.\n", thing);
      return;
    }
  }

  /* Create the Exit */
  BITSET(areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
  Wld(world)->wExit = ExitCreate(Wld(world)->wExit, exitDir, exitKey, "", 0, -1, exitWorld);

  /* Edit the exit description */
  SendHint("^;HINT: Dont forget to make an exit from the other room back to here.\n", thing);
  SendHint("^;HINT: Descriptions must end with a blank line\n", thing);
  sprintf(strName, "^cWorld ^w#%ld^c/^w%s^c - Exit Description", Wld(world)->wVirtual, argDir);
  EDITSTR(thing, Wld(world)->wExit->eDesc, 2048, strName, EP_ENDNOLF);
  EDITFLAG(thing, &areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}

CMDPROC(CmdWEDesc) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   strName[256];
  BYTE   argDir[256];
  LWORD  exitDir;
  EXIT  *exit;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  cmd = StrOneWord(cmd, argDir);
  if (!*argDir) {
    SendThing("^GUsuage: ^gWEDESC <DIR>\n", thing);
    return;
  }
  exitDir = TYPEFIND(argDir, dirList);
  if (exitDir == -1) {
    SendThing("^wUnknown Direction specified.\n", thing);   
    return;
  }
  exit = ExitDir(Wld(world)->wExit, exitDir);
  if (!exit) {
    SendThing("^wNo such exit exists.\n", thing);   
    return;
  } 

  /* Edit the requisite string */
  SendHint("^;HINT: Descriptions must end with a blank line\n", thing);
  sprintf(strName, "^cWorld ^w#%ld^c/^w%s^c - Exit Description", Wld(world)->wVirtual, argDir);
  EDITSTR(thing, exit->eDesc, 2048, strName, EP_ENDLF);
  EDITFLAG(thing, &areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}
CMDPROC(CmdWEKey) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   strName[256];
  BYTE   argDir[256];
  LWORD  exitDir;
  EXIT  *exit;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  cmd = StrOneWord(cmd, argDir);
  if (!*argDir) {
    SendThing("^GUsuage: ^gWEKEY <DIR>\n", thing);
    return;
  }
  exitDir = TYPEFIND(argDir, dirList);
  if (exitDir == -1) {
    SendThing("^wUnknown Direction specified.\n", thing);   
    return;
  }
  exit = ExitDir(Wld(world)->wExit, exitDir);
  if (!exit) {
    SendThing("^wNo such exit exists.\n", thing);   
    return;
  } 

  /* Edit the requisite string */
  SendHint("^;HINT: Keyword list should be only one line\n", thing);
  sprintf(strName, "^cWorld ^w#%ld^c/^w%s^c - Exit Keywords", Wld(world)->wVirtual, argDir);
  EDITSTR(thing, exit->eKey, 256, strName, EP_ENDNOLF);
  EDITFLAG(thing, &areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}

CMDPROC(CmdWEFlag) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   buf[512];
  BYTE   argDir[256];
  BYTE   argFlag[256];
  LWORD  exitDir;
  FLAG   flag;
  EXIT  *exit;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  cmd = StrOneWord(cmd, argDir);
  if (!*argDir) {
    SendThing("^GUsuage: ^gWEFLAG <DIR> <EXITFLAG1>|<EXITFLAG2>|etc\n", thing);
    SendThing("\n^cValid Values are:\n^g", thing);
    SENDARRAY(eFlagList, 3, thing);
    return;
  }
  exitDir = TYPEFIND(argDir, dirList);
  if (exitDir == -1) {
    SendThing("^wUnknown Direction specified.\n", thing);   
    return;
  }
  exit = ExitDir(Wld(world)->wExit, exitDir);
  if (!exit) {
    SendThing("^wNo such exit exists.\n", thing);   
    return;
  } 

  cmd = StrOneWord(cmd, argFlag); /* get arg setting into buf */
  if (!*argFlag) {
    SendThing("^wPerhaps you might consider specifying which flags you wish to change\n", thing);
    SendThing("\n^cValid Values are:\n^g", thing);
    SENDARRAY(eFlagList, 3, thing);
    return;
  } else {
    if (IsNumber(argFlag))
      flag = atol(argFlag);
    else
      flag = FlagSscanf(argFlag, eFlagList);
  }

  SendThing("^CPreviously Set Flags: ^b[^p", thing);
  SendThing(FlagSprintf(buf, exit->eFlag, eFlagList, ' ', 512), thing);
  SendThing("^b]\n", thing);
  
  SendThing("\n^bFlipping: ^G[^w", thing);
  SendThing(FlagSprintf(buf, flag, eFlagList, ' ', 512), thing);
  SendThing("^G]\n", thing);
  BITFLIP(exit->eFlag, flag);

  SendThing("\n^gCurrently Set Flags: ^G[^c", thing);
  SendThing(FlagSprintf(buf, exit->eFlag, eFlagList, ' ', 512), thing);
  SendThing("^G]\n", thing);

    /* Edit the requisite string */
  BITSET(areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}

CMDPROC(CmdWEKeyObj) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   buf[256];
  BYTE   argDir[256];
  BYTE   argNum[256];
  LWORD  exitDir;
  EXIT  *exit;
  LWORD  oldValue;
  LWORD  newValue;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  cmd = StrOneWord(cmd, argDir);
  if (!*argDir) {
    SendThing("^GUsuage: ^gWEKEYOBJ <DIR> <KEYNUMBER>\n", thing);
    return;
  }
  exitDir = TYPEFIND(argDir, dirList);
  if (exitDir == -1) {
    SendThing("^wUnknown Direction specified.\n", thing);   
    return;
  }
  exit = ExitDir(Wld(world)->wExit, exitDir);
  if (!exit) {
    SendThing("^wNo such exit exists.\n", thing);   
    return;
  } 
  oldValue = exit->eKeyObj;

  cmd = StrOneWord(cmd, argNum); /* get arg setting into buf */
  if (!*argNum) {
    SendThing("^wPerhaps you might consider specifying which key number you want it to be\n", thing);
    return;
  } else {
    if (IsNumber(argNum)) {
      newValue = atol(argNum);
    } else {
      SendThing("^wThats WEKEYOBJ <DIR> <KEYNUMBER> as in a number... pick one..\n", thing);
      return;
    }
  }
  
  /* Edit the requisite string */
  sprintf(buf, "Exit KeyObj was: %ld, now %ld\n", oldValue, newValue);
  exit->eKeyObj = newValue;
}

CMDPROC(CmdWExtra) { /* void CmdProc(THING thing, BYTE* cmd) */
  BYTE      strName[256];
  THING    *world;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  sprintf(strName, "^cWorld ^w#%ld^c/", Wld(world)->wVirtual);
  if (EditExtra(thing, "WEXTRA [<#>]", cmd, strName, &world->tExtra))
    EDITFLAG(thing, &areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}

CMDPROC(CmdWProperty) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING    *world;
  BYTE      strName[256];

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  sprintf(strName, "^cWorld ^w#%ld^c/", Wld(world)->wVirtual);
  if (EditProperty(thing, "WPROPERTY [<#>]", cmd, strName, &world->tProperty))
    EDITFLAG(thing, &areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}

CMDPROC(CmdWSet) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE         buf[512];
  LWORD        i;
  WLD          wld;
  THING       *world;

  SETLIST setList[] = {
    { "FLAG",         SET_FLAG(    wld, wld.wFlag, wFlagList ) },
    { "TYPE",         SET_TYPE(    wld, wld.wType, wTypeList ) },
    { "%LIGHT",       SET_PROPERTYINT( wld, wld.wThing.tProperty ) },
    { "" }
  };

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) { /* if they didnt type anything, give 'em a list */
    SendThing("^pUsuage:\n^P=-=-=-=\n", thing);
    SendThing("^cWSET [<World#>] <stat> <value>\n", thing);
    EditSet(thing, cmd, NULL, NULL, setList);
    return;
  }

  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  if (!*cmd) {
    sprintf(buf, "wstat %ld", Wld(world)->wVirtual);
    CmdWStat(thing, buf);
    return;
  }

  i = EditSet(thing, cmd, world, world->tSDesc->sText, setList);
  if (i==-1) {
    SendThing("^wOh, good one.... I bet you made that stat up on the spot, didnt you?\n", thing);
    SendThing("^wType WSET with no arguments for a list of changeable stats\n", thing);
    return;
  } else if (i == 1)
    BITSET(areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED);
}


CMDPROC(CmdWCompile) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  THING    *world;
  BYTE      buf[256];
  PROPERTY *property;
  LWORD     i;
  LWORD     area;
  BYTE      printLine;

  cmd = StrOneWord(cmd, NULL);
  /* Check for Compile all command */
  if (StrExact(cmd, "all")) {
    if (!Base(thing)->bInside || Base(thing)->bInside->tType != TTYPE_WLD) {
      SendThing("^wYou cant do that here\n", thing);
      return;
    }
    area = Wld(Base(thing)->bInside)->wArea;
    for (i=0; i<areaList[area].aWldIndex.iNum; i++) {
      printLine=0;
      for (property = Thing(areaList[area].aWldIndex.iThing[i])->tProperty; property; property=property->pNext) {
        if (property->pKey->sText[0]=='@') {
          if (!printLine) {
            printLine=1;
            sprintf(buf, 
               "^yWCOMPILE: ^c#%ld - ^b%s ", 
               Wld(areaList[area].aWldIndex.iThing[i])->wVirtual, 
               Thing(areaList[area].aWldIndex.iThing[i])->tSDesc->sText);

            SendThing(buf, thing);
          }
          sprintf(buf,"^G(%s) ", property->pKey->sText);
          SendThing(buf, thing);
          if (!CodeCompileProperty(property,thing))
            CodeSetFlag(areaList[area].aWldIndex.iThing[i], property);
        }
      }
      if (printLine)
        SendThing("\n",thing);
    }
    return;
  }

  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  sprintf(buf, "^yCOMPILE: ^c#%ld - ^b%s\n", Wld(world)->wVirtual, world->tSDesc->sText);
  SendThing(buf, thing);
  
  for (property = world->tProperty; property; property=property->pNext) {
    SendThing("^gProp.: ^c", thing);
    SendThing(property->pKey->sText, thing);
    SendThing("\n", thing);
    if (property->pKey->sText[0]=='@') {
      if (!CodeCompileProperty(property,thing))
        CodeSetFlag(world, property);
    }
  }
  
}

CMDPROC(CmdWDecomp) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  THING    *world;
  BYTE      buf[256];
  PROPERTY *property;
  LWORD     i;
  LWORD     area;
  BYTE      printLine;

  cmd = StrOneWord(cmd, NULL);
  /* Check for Compile all command */
  if (StrExact(cmd, "all")) {
    if (!Base(thing)->bInside || Base(thing)->bInside->tType != TTYPE_WLD) {
      SendThing("^wYou cant do that here\n", thing);
      return;
    }
    area = Wld(Base(thing)->bInside)->wArea;
    for (i=0; i<areaList[area].aWldIndex.iNum; i++) {
      printLine=0;
      for (property = Thing(areaList[area].aWldIndex.iThing[i])->tProperty; property; property=property->pNext) {
        if (property->pKey->sText[0]=='@') {
          if (!printLine) {
            printLine=1;
            sprintf(buf, 
              "^yWDECOMP: ^c#%ld - ^b%s ", 
              Wld(areaList[area].aWldIndex.iThing[i])->wVirtual, 
              Thing(areaList[area].aWldIndex.iThing[i])->tSDesc->sText);

            SendThing(buf, thing);
          } 
          sprintf(buf,"^G(%s) ", property->pKey->sText);
          SendThing(buf,thing);
          if (!CodeDecompProperty(property, 
            areaList[area].aWldIndex.iThing[i]))
            CodeClearFlag(areaList[area].aWldIndex.iThing[i], property);
        }
      }
      if (printLine)
        SendThing("\n",thing);
    }
    return;
  }

  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  sprintf(buf, "^yDECOMP: ^c#%ld - ^b%s\n", Wld(world)->wVirtual, world->tSDesc->sText);
  SendThing(buf, thing);
  
  for (property = world->tProperty; property; property=property->pNext) {
    SendThing("^gProp.: ^c", thing);
    SendThing(property->pKey->sText, thing);
    SendThing("\n", thing);
    if (property->pKey->sText[0]=='@') {
      if (!CodeDecompProperty(property, thing))
        CodeClearFlag(world, property);
    }
  }
  
}

CMDPROC(CmdWGoto) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;

  cmd = StrOneWord(cmd, NULL);
  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;

  /* Transport away */
  if (thing->tType==TTYPE_PLR)
    SendAction(Plr(thing)->pExit->sText, thing, NULL, SEND_ROOM|SEND_VISIBLE);
  else
    SendAction("$n dissolves into a cloud of blue sparkles.\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  ThingTo(thing, world);
  FightStop(thing);
  if (thing->tType==TTYPE_PLR)
    SendAction(Plr(thing)->pEnter->sText, thing, NULL, SEND_ROOM|SEND_VISIBLE);
  else
    SendAction("$n appears from a cloud of blue sparkles.\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  CmdLook(thing, "");
}


CMDPROC(CmdWSave) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   buf[256];
  WORD   area;
  SOCK  *sock;

  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;
  area = Wld(world)->wArea;

  if (!BIT(areaList[Wld(world)->wArea].aSystem, AS_WLDUNSAVED)) {
    SendThing("There are no unsaved changes!\n", thing);
  } else {
    BITFLIP(areaList[area].aSystem, AS_WLDUNSAVED);
    WorldWrite(area);
    sock = BaseControlFind(thing);
    sprintf(buf, "%s.wld saved by %s. (%ld rooms)\n", areaList[area].aFileName->sText, sock->sHomeThing->tSDesc->sText, areaList[area].aWldIndex.iNum);
    Log(LOG_AREA, buf);
    sprintf(buf, "%s.wld saved. (^w%ld rooms^V)\n", areaList[area].aFileName->sText, areaList[area].aWldIndex.iNum);
    SendThing(buf, thing);
  }
}
