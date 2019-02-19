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
 *                     A R E A   S T U F F                       *
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
#include "object.h"
#include "char.h"
#include "player.h"
#include "parse.h"
#include "cmd_area.h"


LWORD AGetArea(THING *thing, BYTE **cmd) {
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
    return AreaOf(virtual);
  } else {
    world = Base(thing)->bInside;
    if (!world || world->tType != TTYPE_WLD) {
      SendThing("I'm afraid you're not inside a WORLD right at the moment\n", thing);
      return -1;
    }
    return Wld(world)->wArea;
  }
  return -1;
}

CMDPROC(CmdArea) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  BYTE   buf[512];
  /* ULWORD minuteSinceReset; */

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);
  if (area == -1)
    return;

  if (thing->tType!=TTYPE_PLR) 
    return;

  /* regular player -- show area name and desc */
  sprintf(buf, "^gName:^G[^c%s^G]\n", areaList[area].aFileName->sText);
  SendThing(buf, thing);
  SendThing("^gDescription:^b\n", thing);
  SendThing(areaList[area].aDesc->sText, thing);
  SendThing("\n", thing);
  return;
}

CMDPROC(CmdAStat) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  BYTE   buf[512];
  ULWORD minuteSinceReset;

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);
  if (area == -1)
    return;

  if (thing->tType!=TTYPE_PLR) 
    return;

  if (Character(thing)->cLevel<LEVEL_GOD) {
    /* regular player -- show area name and desc */
    sprintf(buf, "^gName:^G[^c%s^G]\n", areaList[area].aFileName->sText);
    SendThing(buf, thing);
    SendThing("^gDescription:^b\n", thing);
    SendThing(areaList[area].aDesc->sText, thing);
    SendThing("\n", thing);
    return;
  }

  /* show them the stats on the AREA */
  /* first line, # Name & Type */
  sprintf(buf, "^g#:^G[^c%ld^G] ^gName:^G[^c", area);
  SendThing(buf, thing);
  SendThing(areaList[area].aFileName->sText, thing);
  sprintf(buf,"^G] [^w%5ld^G-^w%5ld^G]\n", areaList[area].aVirtualMin, areaList[area].aVirtualMax);
  SendThing(buf, thing);
  
  SendThing("^gAuthorized Editors:^c ", thing);
  SendThing(areaList[area].aEditor->sText, thing);
  SendThing("\n", thing);

  sprintf(buf,"^gWorlds ^G[^c%5ld^G] ^gMobiles^G[^c%3ld^G] ^gObjects^G[^c%3ld^G] ^gResets^G[^c%3ld^G]\n", areaList[area].aWldIndex.iNum, areaList[area].aMobIndex.iNum, areaList[area].aObjIndex.iNum, areaList[area].aResetNum);
  SendThing(buf, thing);

  /* flags */
  SendThing("^gSystem Flags: ^G[^c", thing);
  SendThing(FlagSprintf(buf, areaList[area].aSystem, wSystemList, ' ', 512), thing);
  SendThing("^G]\n", thing);
  SendThing("^gReset/Area Flags: ^G[^c", thing);
  SendThing(FlagSprintf(buf, areaList[area].aResetFlag, rFlagList, ' ', 512), thing);
  SendThing("^G]\n", thing);

  sprintf(buf,"^gReset Delay^G[^c%3hd^G] minutes\n", areaList[area].aResetDelay);
  SendThing(buf, thing);
  minuteSinceReset = (time(0) - areaList[area].aResetLast - startTime)/60;
  sprintf(buf,"^gLast Reset ^G[^c%3ld^G] minutes ago\n", minuteSinceReset);
  SendThing(buf, thing);

  SendThing("^gDescription:^b\n", thing);
  SendThing(areaList[area].aDesc->sText, thing);
  SendThing("\n", thing);
}


CMDPROC(CmdAList) { /* void CmdProc(THING thing, BYTE* cmd) */
  BYTE   saved;
  LWORD  area;
  BYTE   buf[256];
  BYTE   truncateStr[256];
  BYTE   word[256];

  /* List the rooms in 2 nicely formatted columns*/
  for (area=0; area<areaListMax; area+=2) {
    if (areaList[area].aSystem&(AS_WLDUNSAVED|AS_MOBUNSAVED|AS_OBJUNSAVED|AS_RSTUNSAVED))
      saved='*';
    else 
      saved=' ';
    if (areaList[area].aResetFlag&RF_AREAISCLOSED) {
      sprintf(buf, "^r%5ld-%5ld ^y%c^r%-24s", 
        areaList[area].aVirtualMin, 
        areaList[area].aVirtualMax, 
        saved,
        StrTruncate(truncateStr, StrFirstWord(areaList[area].aFileName->sText,word), 25));
    } else {
      sprintf(buf, "^w%5ld-%5ld ^y%c^c%-24s", 
        areaList[area].aVirtualMin, 
        areaList[area].aVirtualMax, 
        saved,
        StrTruncate(truncateStr, StrFirstWord(areaList[area].aFileName->sText,word), 25));
    }
    if (area+1 < areaListMax) {
      if (areaList[area+1].aSystem&(AS_WLDUNSAVED|AS_MOBUNSAVED|AS_OBJUNSAVED|AS_RSTUNSAVED))
        saved='*';
      else 
        saved=' ';
      if (areaList[area+1].aResetFlag&RF_AREAISCLOSED) {
        sprintf(buf+strlen(buf), "^r%5ld-%5ld ^y%c^r%-24s", 
          areaList[area+1].aVirtualMin, 
          areaList[area+1].aVirtualMax, 
          saved,
          StrTruncate(truncateStr, StrFirstWord(areaList[area+1].aFileName->sText,word), 25));
      } else {
        sprintf(buf+strlen(buf), "^w%5ld-%5ld ^y%c^c%-24s", 
          areaList[area+1].aVirtualMin, 
          areaList[area+1].aVirtualMax, 
          saved,
          StrTruncate(truncateStr, StrFirstWord(areaList[area+1].aFileName->sText,word), 25));
      }
    }
    strcat(buf, "\n");
    SendThing(buf, thing);
  }
  sprintf(buf, "^g%ld ^bareas listed.  ^y* ^bmeans the area has unsaved changes. \n", areaListMax);
  SendThing(buf, thing);
}

CMDPROC(CmdADesc) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  BYTE   strName[512];

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);
  if (area == -1) {
    SendThing("^wYou cant do that here...\n", thing);
    return;
  }

  /* Edit the requisite string */
  SendHint("^;HINT: Descriptions must end with a blank line\n", thing);
  sprintf(strName, "Area %s - Description", areaList[area].aFileName->sText);
  EDITSTR(thing, areaList[area].aDesc, 4096, strName, EP_ENDLF);
  EDITFLAG(thing, &areaList[area].aSystem, AS_RSTUNSAVED);
}

CMDPROC(CmdAFlag) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  BYTE   buf[512];
  FLAG   flag;

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);
  if (area == -1) {
    SendThing("^wYou cant do that here...\n", thing);
    return;
  }

  sprintf(buf, "^g#:^G[^c%ld^G] ^gName:^G[^c", area);
  SendThing(buf, thing);
  SendThing(areaList[area].aFileName->sText, thing);
  SendThing("^G]\n\n", thing);

  SendThing("^gReset/Area Flags Set:\n^c", thing);
  SendThing(FlagSprintf(buf, areaList[area].aResetFlag, rFlagList, ' ', 512), thing);

  cmd = StrOneWord(cmd, buf);
  flag = FlagFind(buf, rFlagList);
  if (!flag) {
    SendThing("\n\n^wPossible Flags Are:^g\n", thing);
    SENDARRAY(rFlagList, 3, thing);
    return;
  }

  BITFLIP(areaList[area].aResetFlag, flag);
  SendThing("\n\n^gFlipping ^r", thing);
  SendThing(FlagSprintf(buf, flag, rFlagList, '\n', 512), thing);
  SendThing("\n\n^gReset/Area Flags Now:\n^c", thing);
  SendThing(FlagSprintf(buf, areaList[area].aResetFlag, rFlagList, ' ', 512), thing);
  SendThing("\n", thing);
  BITSET(areaList[area].aSystem, AS_RSTUNSAVED);
}

CMDPROC(CmdADelay) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  BYTE   buf[512];

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, NULL);
  if (area == -1) {
    SendThing("^wYou cant do that here...\n", thing);
    return;
  }

  sprintf(buf, "^g#:^G[^c%ld^G] ^gName:^G[^c", area);
  SendThing(buf, thing);
  SendThing(areaList[area].aFileName->sText, thing);
  SendThing("^G]\n\n", thing);

  if (!*cmd) {
    sprintf(buf, "^gReset Delay is Currently: %hd\n^c", areaList[area].aResetDelay);
    SendThing(buf, thing);
    return;
  } else {
    sprintf(buf, "^gReset Delay Was: %hd\n^c", areaList[area].aResetDelay);
    SendThing(buf, thing);
  }
  areaList[area].aResetDelay = atoi(cmd);

  sprintf(buf, "^gReset Delay Now: %hd\n^c", areaList[area].aResetDelay);
  SendThing(buf, thing);

  BITSET(areaList[area].aSystem, AS_RSTUNSAVED);
}

CMDPROC(CmdAReset) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  LWORD  areaMin;
  LWORD  areaMax;

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);

  if (StrExact(cmd, "all")) {
    if (!ParseCommandCheck(TYPEFIND("reboot",commandList), BaseControlFind(thing), "")) {
      SendThing("^wI'm afraid that sort of thing is reserved for admins and the like.\n", thing);
      return;
    }
    area    = 0;
    areaMin = 0;
    areaMax = areaListMax;
  } else {
    areaMin = area;
    areaMax = area+1;
  }

  if (area == -1) {
    SendThing("^wYou cant do that here...\n", thing);
    return;
  }

  for (area=areaMin; area<areaMax; area++) {
    ResetArea(area);

    SendThing("^wYou reset ", thing);
    SendThing(areaList[area].aFileName->sText, thing);
    SendThing("\n", thing);
  }
}

CMDPROC(CmdAReboot) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  LWORD  i;
  THING *t;
  THING *next;
  LWORD  areaMin;
  LWORD  areaMax;

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);

  if (StrExact(cmd, "all")) {
    if (!ParseCommandCheck(TYPEFIND("reboot",commandList), BaseControlFind(thing), "")) {
      SendThing("^wI'm afraid that sort of thing is reserved for admins and the like.\n", thing);
      return;
    }
    area    = 0;
    areaMin = 0;
    areaMax = areaListMax;
  } else {
    areaMin = area;
    areaMax = area+1;
  }

  if (area == -1) {
    SendThing("^wYou cant do that here...\n", thing);
    return;
  }

  for (area=areaMin; area<areaMax; area++) {

    /* Clear everything out */
    for (i=0; i<areaList[area].aWldIndex.iNum; i++) {
      for( t=areaList[area].aWldIndex.iThing[i]->tContain; t; t=next ) {
        next = t->tNext;
        if (t->tType==TTYPE_MOB)
          ThingFree(t);
        else if (t->tType==TTYPE_OBJ && Obj(t)->oTemplate->oType != OTYPE_RESETCMD)
          ThingFree(t);
      }
    }

    /* Reset it */
    ResetArea(area); 

    SendThing("^wYou reboot ", thing);
    SendThing(areaList[area].aFileName->sText, thing);
    SendThing("\n", thing);

  }
}

CMDPROC(CmdAProperty) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  BYTE   strName[256];

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);
  if (area == -1)
    return;

  sprintf(strName, "^cArea ^w%s^c/", areaList[area].aFileName->sText);
  if (EditProperty(thing, "APROPERTY", cmd, strName, &areaList[area].aResetThing.tProperty))
    EDITFLAG(thing, &areaList[area].aSystem, AS_RSTUNSAVED);
}

CMDPROC(CmdACompile) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  LWORD        area;
  BYTE         buf[256];
  PROPERTY    *property;
  LWORD        areaMin;
  LWORD        areaMax;

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);

  if (StrExact(cmd, "all")) {
    if (!ParseCommandCheck(TYPEFIND("reboot",commandList), BaseControlFind(thing), "")) {
      SendThing("^wI'm afraid that sort of thing is reserved for admins and the like.\n", thing);
      return;
    }
    area    = 0;
    areaMin = 0;
    areaMax = areaListMax;
  } else {
    areaMin = area;
    areaMax = area+1;
  }

  if (area == -1) {
    SendThing("^wYou cant do that here...\n", thing);
    return;
  }

  for (area=areaMin; area<areaMax; area++) {

    sprintf(buf, "^yCOMPILE: ^c%s\n", areaList[area].aFileName->sText);
    SendThing(buf, thing);
  
    for (property = areaList[area].aResetThing.tProperty; property; property=property->pNext) {
      SendThing("^gProp.: ^c", thing);
      SendThing(property->pKey->sText, thing);
      SendThing("\n", thing);
      if (property->pKey->sText[0]=='@') {
        if (!CodeCompileProperty(property,thing))
          CodeSetFlag(&areaList[area].aResetThing, areaList[area].aResetThing.tProperty);
      }
    }

  }
}


CMDPROC(CmdADecomp) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  LWORD        area;
  BYTE         buf[256];
  PROPERTY    *property;

  area = AGetArea(thing, &cmd);
  if (area == -1)
    return;

  sprintf(buf, "^yDECOMP: ^c%s\n", areaList[area].aFileName->sText);
  SendThing(buf, thing);
  
  for (property = areaList[area].aResetThing.tProperty; property; property=property->pNext) {
    SendThing("^gProp.: ^c", thing);
    SendThing(property->pKey->sText, thing);
    SendThing("\n", thing);
    if (property->pKey->sText[0]=='@') {
      if (!CodeDecompProperty(property,thing))
        CodeClearFlag(&areaList[area].aResetThing, areaList[area].aResetThing.tProperty);
    }
  }
  
}


/* actually this should be CmdRSave */
CMDPROC(CmdASave) { /* void CmdProc(THING thing, BYTE* cmd) */
  LWORD  area;
  BYTE   buf[512];
  SOCK  *sock;
  LWORD  areaMin;
  LWORD  areaMax;

  cmd = StrOneWord(cmd, NULL);
  area = AGetArea(thing, &cmd);

  if (StrExact(cmd, "all")) {
    if (!ParseCommandCheck(TYPEFIND("reboot",commandList), BaseControlFind(thing), "")) {
      SendThing("^wI'm afraid that sort of thing is reserved for admins and the like.\n", thing);
      return;
    }
    area    = 0;
    areaMin = 0;
    areaMax = areaListMax;
  } else {
    areaMin = area;
    areaMax = area+1;
  }

  if (area == -1) {
    SendThing("^wYou cant do that here...\n", thing);
    return;
  }

  for (area=areaMin; area<areaMax; area++) {

    if (BIT(areaList[area].aSystem, AS_RSTSHOWN))
      SendThing("WARNING: Reset editing currently in progress\n", thing);

    if (!BIT(areaList[area].aSystem, AS_RSTUNSAVED)) {
      SendThing("There are no unsaved changes!\n", thing);
    } else {
      BITFLIP(areaList[area].aSystem, AS_RSTUNSAVED);
      ResetWrite(area);
      sock = BaseControlFind(thing);
      sprintf(buf, "%s.rst saved by %s. (%ld resets)\n", 
        areaList[area].aFileName->sText, 
        sock->sHomeThing->tSDesc->sText, 
        areaList[area].aResetNum);
      Log(LOG_AREA, buf);
      sprintf(buf, "%s.rst saved. (^w%ld resets^V)\n", areaList[area].aFileName->sText, areaList[area].aResetNum);
      SendThing(buf, thing);
    }
  }

}

