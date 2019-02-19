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
#include <ctype.h>
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
#include "mobile.h"
#include "player.h"
#include "parse.h"
#include "cmd_area.h"
#include "cmd_rst.h"

void RSetDesc(THING *reset, BYTE rCmd, WORD rIf, LWORD rArg1, LWORD rArg2, LWORD rArg3, LWORD rArg4) {
  BYTE         buf[256];
  BYTE         key[512];
  BYTE         name[256];
  MOBTEMPLATE *mobile;
  OBJTEMPLATE *object;
  
  strcpy(key, "rst reset");
  strcpy(buf, "");
  strcpy(name, "");
  switch (rCmd) {
  case 'M':
    strcat(key, " mob ");
    mobile = MobileOf(rArg1);
    if (mobile)
      sprintf(name, "%s", mobile->mSDesc->sText);
    else
      sprintf(name, "<???>");
    strcat(key, name);
    sprintf(buf, 
      "^wMOB: ^CIf^b[^p%hd^b] ^CMob^b[^p%5ld^b] ^CGamemax[^p%3ld^b] ^CRoommax^b[^p%3ld^b] ^r%s",
      rIf,
      rArg1,
      rArg2,
      rArg4,
      name);
    break;
    
  case 'O':
    strcat(key, " obj");
    object = ObjectOf(rArg1);
    if (object)
      sprintf(name, "%s", object->oSDesc->sText);
    else
      sprintf(name, "<???>");
    strcat(key, name);
    sprintf(buf,
      "^wOBJ: ^CIf^b[^p%hd^b] ^CObj^b[^p%5ld^b] ^CGamemax^b[^p%3ld^b] ^CRoommax^b[^p%3ld^b] ^r%s",
      rIf,
      rArg1,
      rArg2,
      rArg4,
      name);
    break;
    
  case 'G':
    strcat(key, " giv");
    object = ObjectOf(rArg1);
    if (object)
      sprintf(name, "%s", object->oSDesc->sText);
    else
      sprintf(name, "<???>");
    strcat(key, name);
    sprintf(buf,
      "^wGIV: ^CIf^b[^p%hd^b] ^CObj^b[^p%5ld^b] ^CGamemax^b[^p%3ld^b] ^r%s",
      rIf,
      rArg1,
      rArg2,
      name);
    break;
    
  case 'P':
    strcat(key, " put");
    object = ObjectOf(rArg1);
    if (object)
      sprintf(name, "%s", object->oSDesc->sText);
    else
      sprintf(name, "<???>");
    strcat(key, name);
    sprintf(buf,
      "^wPUT: ^CIf^b[^p%hd^b] ^CObj^b[^p%5ld^b] ^CGamemax^b[^p%3ld^b] ^CDestObj^b[^p%5ld^b] ^r%s",
      rIf,
      rArg1,
      rArg2,
      rArg4,
      name);
    break;
    
  case 'E':
    strcat(key, " equ");
    object = ObjectOf(rArg1);
    if (object)
      sprintf(name, "%s", object->oSDesc->sText);
    else
      sprintf(name, "<???>");
    strcat(key, name);
    sprintf(buf,
      "^wEQU: ^CIf^b[^p%hd^b] ^CObj^b[^p%5ld^b] ^CGamemax^b[^p%3ld^b] ^r%s",
      rIf,
      rArg1,
      rArg2,
      name);
    break;
    
  case 'D':
    strcat(key, " dor");
    sprintf(buf,
      "^wDOR: ^CIf^b[^p%hd^b] ^CExit^b[^p%s^b] ^CFlag^b[^p%s^b]",
      rIf,
      dirList[rArg2],
      FlagSprintf(name, rArg3, eFlagList, '|', sizeof(name)));
    strcat(key, name);
    break;
    
  case 'R':
    strcat(key, " rem");
    object = ObjectOf(rArg2);
    if (object)
      sprintf(name, "%s", object->oSDesc->sText);
    else
      sprintf(name, "<???>");
    strcat(key, name);
    sprintf(buf,
      "^wREM: ^CIf^b[^C%hd^b] ^CObj^b[^p%5ld^b] ^r%s",
      rIf,
      rArg2,
      name);
    break;
    
  default:
    strcat(key, " ???");
    if (isprint(rCmd)) 
      sprintf(buf,
        "^w[%c]: ^CIf^b[^p%hd^b] ^CArg1^b[^p%5ld^b] ^CArb2^b[^p%5ld^b] ^CArg3^b[^p%5ld^b] ^CArg4^b[^p%5ld^b] ^r<???>",
        rCmd,
        rIf,
        rArg1,
        rArg2,
        rArg3,
        rArg4);
    else
      sprintf(buf,
        "^w[\\%hd]: ^CIf^b[^p%hd^b] ^CArg1^b[^p%5ld^b] ^CArb2^b[^p%5ld^b] ^CArg3^b[^p%5ld^b] ^CArg4^b[^p%5ld^b] ^r<???>",
        (WORD)rCmd,
        rIf,
        rArg1,
        rArg2,
        rArg3,
        rArg4);
  }

  STRFREE(reset->tSDesc);
  reset->tSDesc = STRCREATE(buf);
  STRFREE(Base(reset)->bLDesc);
  Base(reset)->bLDesc = STRCREATE(buf);
  STRFREE(Base(reset)->bKey);
  Base(reset)->bKey = STRCREATE(key);
}

CMDPROC(CmdRShow) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING     *reset;
  THING     *world = NULL;
  RESETLIST *resetList = NULL;
  LWORD      area;
  LWORD      i;

  area = AGetArea(thing, &cmd);
  if (area == -1)
    return;
  if (BIT(areaList[area].aSystem, AS_RSTSHOWN)) {
    SendThing("Reset Commands are allready being shown!\n", thing);
    return;
  }

  resetList = areaList[area].aResetList;
  for (i=0; i<areaList[area].aResetNum; i++) {
    /* strip out comments (for now?) */
    if (resetList[i].rCmd=='#' || resetList[i].rCmd=='*')
      continue;

    /* Manufacture a "ResetCmd" */
    reset = ObjectCreate(resetTemplate, NULL);
    OBJECTSETFIELD(reset, OF_RESET_CMD, resetList[i].rCmd);
    OBJECTSETFIELD(reset, OF_RESET_IF,  resetList[i].rIf);
    OBJECTSETFIELD(reset, OF_RESET_ARG2,resetList[i].rArg2.rNum);
    OBJECTSETFIELD(reset, OF_RESET_ARG4,resetList[i].rArg4.rNum);
    switch (resetList[i].rCmd) {
    case 'M':
      OBJECTSETFIELD(reset, OF_RESET_ARG1,resetList[i].rArg1.rMob->mVirtual);
      OBJECTSETFIELD(reset, OF_RESET_ARG3,Wld(resetList[i].rArg3.rWld)->wVirtual);
      world = resetList[i].rArg3.rWld;
      break;

    case 'O':
      OBJECTSETFIELD(reset, OF_RESET_ARG1,resetList[i].rArg1.rObj->oVirtual);
      OBJECTSETFIELD(reset, OF_RESET_ARG3,Wld(resetList[i].rArg3.rWld)->wVirtual);
      world = resetList[i].rArg3.rWld;
      break;

    case 'G':
      OBJECTSETFIELD(reset, OF_RESET_ARG1,resetList[i].rArg1.rObj->oVirtual);
      break;

    case 'P':
      OBJECTSETFIELD(reset, OF_RESET_ARG1,resetList[i].rArg1.rObj->oVirtual);
      OBJECTSETFIELD(reset, OF_RESET_ARG3,resetList[i].rArg3.rNum);
      break;

    case 'E':
      OBJECTSETFIELD(reset, OF_RESET_ARG1,resetList[i].rArg1.rObj->oVirtual);
      break;

    case 'D':
      OBJECTSETFIELD(reset, OF_RESET_ARG1,Wld(resetList[i].rArg1.rWld)->wVirtual);
      OBJECTSETFIELD(reset, OF_RESET_ARG3,resetList[i].rArg3.rNum);
      world = resetList[i].rArg1.rWld;
      break;

    case 'R':
      OBJECTSETFIELD(reset, OF_RESET_ARG1,Wld(resetList[i].rArg1.rWld)->wVirtual);
      world = resetList[i].rArg1.rWld;
      break;

    default:
      OBJECTSETFIELD(reset, OF_RESET_ARG1,resetList[i].rArg1.rNum);
      OBJECTSETFIELD(reset, OF_RESET_ARG3,resetList[i].rArg3.rNum);
    }
    /* it in the room */
    ThingTo(reset, world);
    /* give it a specific description */
    RSetDesc(reset, 
       OBJECTGETFIELD(reset, OF_RESET_CMD),
       OBJECTGETFIELD(reset, OF_RESET_IF),
       OBJECTGETFIELD(reset, OF_RESET_ARG1),
       OBJECTGETFIELD(reset, OF_RESET_ARG2),
       OBJECTGETFIELD(reset, OF_RESET_ARG3),
       OBJECTGETFIELD(reset, OF_RESET_ARG4));
  }
  BITSET(areaList[area].aSystem, AS_RSTSHOWN);
}

CMDPROC(CmdRCreate) { /* void CmdProc(THING *thing, BYTE* cmd) */
  THING       *reset;
  LWORD        area;
  LWORD        i;
  BYTE         buf[256];
  WORD         rCmd;
  WORD         rIf = -1;
  WORD         rArg1;
  WORD         rArg2;
  WORD         rArg3;
  WORD         rArg4;
  MOBTEMPLATE *mobile;
  OBJTEMPLATE *object;
  EXIT        *exit;

  BYTE *validCmd[] = {
    "MOBILE",
    "OBJECT",
    "GIVE",
    "PUT",
    "EQUIP",
    "DOR",
    "DOORSET",
    "REMOVE",
    ""
  };
  
  area = AGetArea(thing, &cmd);
  if (area == -1)
    return;
  if (!BIT(areaList[area].aSystem, AS_RSTSHOWN)) {
    /* Make sure cmds are allready being shown */
    CmdRShow(thing, "");
  }

  cmd = StrOneWord(cmd, NULL); /* lose rcreate */
  if (!*cmd) {
    SendThing("Try ^gRCREATE <CMD> [IF||NOIF] ARG1 ARG2 ARG3\n\n", thing);
    SendThing("^we.g. ^gRCREATE MOBILE  <MOB#> <GAMEMAX> <ROOMMAX>\n", thing);
    SendThing("^we.g. ^gRCREATE OBJECT  <OBJ#> <GAMEMAX> <ROOMMAX>\n", thing);
    SendThing("^we.g. ^gRCREATE GIVE    <OBJ#> <GAMEMAX>\n", thing);
    SendThing("^we.g. ^gRCREATE PUT     <OBJ#> <GAMEMAX> <DESTOBJ#>\n", thing);
    SendThing("^we.g. ^gRCREATE EQUIP   <OBJ#> <GAMEMAX>\n", thing);
    SendThing("^we.g. ^gRCREATE DOORSET <DIR>  <EXITFLAG1>|<EXITFLAG2>...\n", thing);
    SendThing("^we.g. ^gRCREATE REMOVE  <OBJ#>\n", thing);
    SendThing("\n", thing);
    SendThing("^we.g. ^gRCREATE MOBILE 100 5 2\n", thing);
    SendThing("^we.g. ^gRCREATE IF M 100 5 2\n", thing);
    SendThing("^we.g. ^gRCREATE EQUIP IF 100 50\n", thing);
    SendThing("^we.g. ^gRCREATE E NOIF 100 50\n", thing);
    return;
  }

  cmd = StrOneWord(cmd, buf); /* determine cmd type */
  i = TYPEFIND(buf, validCmd);
  if (i== -1) {
    SendThing("Invalid reset command!\n", thing);
    return;
  } else
    rCmd = toupper(buf[0]);

  cmd = StrOneWord(cmd, buf); /* determine if setting */
  if (StrAbbrev("if", buf)) {
    rIf = 1;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
  }
  if (StrAbbrev("noif", buf)) {
    rIf = 0;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
  }

  /* Manufacture a "ResetCmd" */
  switch (rCmd) {
  case 'M':
    if (rIf==-1) rIf = 0;
    mobile = MobileOf(atoi(buf));
    if (!mobile) {
      SendThing("Unknown Mobile virtual number\n", thing);
      return;
    }
    rArg1 = mobile->mVirtual;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
    rArg2 = atoi(buf);
    if (rArg2==0) rArg2=99;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
    rArg4 = atoi(buf);
    if (rArg4==0) rArg4=1;
    break;
    
  case 'O':
    if (rIf==-1) rIf = 0;
    object = ObjectOf(atoi(buf));
    if (!object) {
      SendThing("Unknown Object virtual number\n", thing);
      return;
    }
    rArg1 = object->oVirtual;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
    rArg2 = atoi(buf);
    if (rArg2==0) rArg2=99;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
    rArg4 = atoi(buf);
    if (rArg4==0) rArg4=1;
    break;
    
  case 'G':
  case 'E':
    if (rIf==-1) rIf = 1;
    object = ObjectOf(atoi(buf));
    if (!object) {
      SendThing("Unknown Object virtual number\n", thing);
      return;
    }
    rArg1 = object->oVirtual;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
    rArg2 = atoi(buf);
    if (rArg2==0) rArg2=99;
    break;
    
  case 'P':
    if (rIf==-1) rIf = 1;
    object = ObjectOf(atoi(buf));
    if (!object) {
      SendThing("Unknown Object virtual number\n", thing);
      return;
    }
    rArg1 = object->oVirtual;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
    rArg2 = atoi(buf);
    if (rArg2==0) rArg2=99;
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
    object = ObjectOf(atoi(buf));
    if (!object) {
      SendThing("Unknown Dest. Object virtual number\n", thing);
      return;
    }
    rArg4 = object->oVirtual;
    break;
    
   case 'D':
    if (rIf==-1) rIf = 0;
    /* determine exit and its direction */
    exit = ExitFind(Wld(Base(thing)->bInside)->wExit, buf);
    if (!exit) {
      SendThing("Exit not found\n", thing);
      return;
    }
    rArg2 = exit->eDir;
    /* Determine flags selected */
    cmd = StrOneWord(cmd, buf); /* get arg setting into buf */
    if (!*buf) {
      rArg3 = exit->eFlag;
    } else {
      if (IsNumber(buf))
        rArg3 = atoi(buf);
      else
        rArg3 = FlagSscanf(buf, eFlagList);
    }
    break;
    
  case 'R':
    if (rIf==-1) rIf = 0;
    object = ObjectOf(atoi(buf));
    if (!object) {
      SendThing("Unknown Object virtual number\n", thing);
      return;
    }
    rArg2 = object->oVirtual;
    break;
  }

  /* Make the object */
  SendThing("^wReset command created\n", thing);
  reset = ObjectCreate(resetTemplate, NULL);
  OBJECTSETFIELD(reset, OF_RESET_CMD, rCmd);
  OBJECTSETFIELD(reset, OF_RESET_IF,  rIf);
  OBJECTSETFIELD(reset, OF_RESET_ARG1,rArg1);
  OBJECTSETFIELD(reset, OF_RESET_ARG2,rArg2);
  OBJECTSETFIELD(reset, OF_RESET_ARG3,rArg3);
  OBJECTSETFIELD(reset, OF_RESET_ARG4,rArg4);
  ThingTo(reset, Base(thing)->bInside);
  RSetDesc(reset, rCmd, rIf, rArg1, rArg2, rArg3, rArg4);
  BITSET(areaList[area].aSystem, AS_RSTSHOWN);
}

/* big long and ugly..... and you wonder why I put it off */
CMDPROC(CmdRSet) { /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE         buf[256];
  BYTE         srcKey[256];
  LWORD        srcNum;
  LWORD        srcOffset;
  THING       *found;
  WORD         rCmd;
  WORD         area;
  LWORD        i;
  MOBTEMPLATE *mobile;
  OBJTEMPLATE *object;
  EXIT        *exit;
  BYTE       **list;

  BYTE *mobList[] = {
    "IF",
    "MOBILE",
    "GAMEMAX",
    "ROOMMAX",
    ""
  };
  BYTE *objList[] = {
    "IF",
    "OBJECT",
    "GAMEMAX",
    "ROOMMAX",
    ""
  };
  BYTE *givList[] = {
    "IF",
    "OBJECT",
    "GAMEMAX",
    ""
  };
  BYTE *putList[] = {
    "IF",
    "OBJECT",
    "GAMEMAX",
    "DESTOBJ",
    ""
  };
  BYTE *dorList[] = {
    "IF",
    "DIRECTION",
    "EXITFLAG",
    ""
  };
  BYTE *remList[] = {
    "IF",
    "REMOVEOBJECT",
    ""
  };


  area = AGetArea(thing, &cmd);
  if (area == -1)
    return;
  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^wUse RSET <COMMAND> <SETTING> <VALUE>\n", thing);
    SendThing("\nTry RSET <COMMAND> for a list of valid settings\n", thing);
    return;
  }

  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
  if (!found) {
    SendThing("^rThere doesnt appear to be anything like that around\n", thing);
    return;
  }
  if (Obj(found)->oTemplate->oType != OTYPE_RESETCMD) {
    SendThing("^rDont know how to break it to you but that isnt a reset command\n", thing);
    return;
  }

  /* found is the reset command - do what we will to it */
  rCmd = OBJECTGETFIELD(found, OF_RESET_CMD);
  switch(rCmd) {
  case 'M': list = mobList; break;
  case 'O': list = objList; break;
  case 'G':
  case 'E': list = givList; break;
  case 'P': list = putList; break;
  case 'D': list = dorList; break;
  case 'R': list = remList; break;
  default:
    SendThing("Unrecognised Reset Command, try another\n", thing);
    return;
  }
  
  cmd = StrOneWord(cmd, buf);
  if (!*buf) {
    SendThing("^CValid settings:\n", thing);
    SENDARRAY(list, 4, thing);
    return;
  }
  i = TYPEFIND(buf, list);
  if (i==-1) {
    SendThing("^wThat is not a valid setting, Try ^rRSET <command> for a list\n", thing);
    return;
  }

  /* checkout the setting */

  /* To take advantage of the large degree of overlap between 
     settings for each rCmd, error checking is done against the
     string, not its number in the list
     */
  cmd = StrOneWord(cmd, buf);
  if (StrExact(list[i], "IF")) {
    if (atoi(buf)>0)
      OBJECTSETFIELD(found, OF_RESET_IF, 1);
    else
      OBJECTSETFIELD(found, OF_RESET_IF, 0);
  }
  else if (StrExact(list[i], "MOBILE")) {
    mobile = MobileOf(atoi(buf));
    if (!mobile) {
      SendThing("Unknown Mobile virtual number\n", thing);
      return;
    }
    OBJECTSETFIELD(found, OF_RESET_ARG1, mobile->mVirtual);
  }
  else if (StrExact(list[i], "OBJECT")) {
    object = ObjectOf(atoi(buf));
    if (!object) {
      SendThing("Unknown Object virtual number\n", thing);
      return;
    }
    OBJECTSETFIELD(found, OF_RESET_ARG1, object->oVirtual);
  }
  else if (StrExact(list[i], "GAMEMAX")) {
    OBJECTSETFIELD(found, OF_RESET_ARG2, atoi(buf));
  }
  else if (StrExact(list[i], "ROOMMAX")) {
    OBJECTSETFIELD(found, OF_RESET_ARG4, atoi(buf));
  }
  else if (StrExact(list[i], "DESTOBJECT")) {
    object = ObjectOf(atoi(buf));
    if (!object) {
      SendThing("Unknown Object virtual number\n", thing);
      return;
    }
    OBJECTSETFIELD(found, OF_RESET_ARG3, object->oVirtual);
  }
  else if (StrExact(list[i], "DIRECTION")) {
    /* determine exit and its direction */
    exit = ExitFind(Wld(Base(thing)->bInside)->wExit, buf);
    if (!exit) {
      SendThing("Exit not found\n", thing);
      return;
    }
    OBJECTSETFIELD(found, OF_RESET_ARG2, exit->eDir);
    OBJECTSETFIELD(found, OF_RESET_ARG3, exit->eFlag);
  }
  else if (StrExact(list[i], "EXITFLAG")) {
    if (IsNumber(buf))
      OBJECTSETFIELD(found, OF_RESET_ARG3, atoi(buf));
    else
      OBJECTSETFIELD(found, OF_RESET_ARG3, FlagSscanf(buf, eFlagList));
  }
  else if (StrExact(list[i], "REMOVEOBJECT")) {
    object = ObjectOf(atoi(buf));
    if (!object) {
      SendThing("Unknown Object virtual number\n", thing);
      return;
    }
    OBJECTSETFIELD(found, OF_RESET_ARG2, object->oVirtual);
  }

  /* all done correct its descriptions */
  RSetDesc(found, 
     OBJECTGETFIELD(found, OF_RESET_CMD),
     OBJECTGETFIELD(found, OF_RESET_IF),
     OBJECTGETFIELD(found, OF_RESET_ARG1),
     OBJECTGETFIELD(found, OF_RESET_ARG2),
     OBJECTGETFIELD(found, OF_RESET_ARG3),
     OBJECTGETFIELD(found, OF_RESET_ARG4));
  SendAction("^CNow $n\n", 
       found, thing, SEND_DST |SEND_VISIBLE);
  BITSET(areaList[area].aSystem, AS_RSTSHOWN);
}

CMDPROC(CmdRFirst) { /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;

  cmd = StrOneWord(cmd, NULL);
  ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
  if (!found) {
    SendThing("^rThere doesnt appear to be anything like that around\n", thing);
    return;
  }
  if (Obj(found)->oTemplate->oType != OTYPE_RESETCMD) {
    SendThing("^rDont know how to break it to you but that isnt a reset command\n", thing);
    return;
  }

  ThingFrom(found);
  found->tNext = Base(thing)->bInside->tContain;
  Base(thing)->bInside->tContain = found;
  Base(found)->bInside = Base(thing)->bInside;
  SendAction("^bYou move $n to the front\n", 
       found, thing, SEND_DST |SEND_VISIBLE);
  SendAction("^b$N moves $n to the front\n", 
       found, thing, SEND_ROOM|SEND_VISIBLE);
}

CMDPROC(CmdRUpdate) { /* void CmdProc(THING *thing, BYTE* cmd) */
  THING     *reset;
  RESETLIST *resetList = NULL;
  LWORD      area;
  LWORD      i;
  WORD       rCmd;
  LWORD      iWld;
  BYTE       buf[256];

  area = AGetArea(thing, &cmd);
  if (area == -1)
    return;
  if (!BIT(areaList[area].aSystem, AS_RSTSHOWN)) {
    SendThing("Reset Commands arent even being shown!\n", thing);
    return;
  }

  resetList = areaList[area].aResetList;
  i=0;
  for (iWld=0; iWld<areaList[area].aWldIndex.iNum; iWld++) {
    for (reset=areaList[area].aWldIndex.iThing[iWld]->tContain; 
         reset && reset->tType==TTYPE_OBJ && Obj(reset)->oTemplate->oType==OTYPE_RESETCMD;
         reset=reset->tNext) {
      /* strip out comments (for now?) */
      rCmd = OBJECTGETFIELD(reset, OF_RESET_CMD);

      if (rCmd=='#' || rCmd=='*')
        continue;
      
      /* ensure adequate space */
      REALLOC("CmdRUpdate(cmd_rst.c): resetList reallocation\n", resetList, RESETLIST, i+1, areaList[area].aResetByte);

      resetList[i].rCmd       = OBJECTGETFIELD(reset, OF_RESET_CMD);
      resetList[i].rIf        = OBJECTGETFIELD(reset, OF_RESET_IF);
      resetList[i].rArg2.rNum = OBJECTGETFIELD(reset, OF_RESET_ARG2);
      resetList[i].rArg4.rNum = OBJECTGETFIELD(reset, OF_RESET_ARG4);
      switch (resetList[i].rCmd) {
      case 'M':
        resetList[i].rArg1.rMob = MobileOf(OBJECTGETFIELD(reset, OF_RESET_ARG1));
        resetList[i].rArg3.rWld = areaList[area].aWldIndex.iThing[iWld];
        break;
  
      case 'O':
        resetList[i].rArg1.rObj = ObjectOf(OBJECTGETFIELD(reset, OF_RESET_ARG1));
        resetList[i].rArg3.rWld = areaList[area].aWldIndex.iThing[iWld];
        break;
  
      case 'G':
        resetList[i].rArg1.rObj = ObjectOf(OBJECTGETFIELD(reset, OF_RESET_ARG1));
        break;
  
      case 'P':
        resetList[i].rArg1.rObj = ObjectOf(OBJECTGETFIELD(reset, OF_RESET_ARG1));
        resetList[i].rArg3.rNum = OBJECTGETFIELD(reset, OF_RESET_ARG3);
        break;
  
      case 'E':
        resetList[i].rArg1.rObj = ObjectOf(OBJECTGETFIELD(reset, OF_RESET_ARG1));
        break;
  
      case 'D':
        resetList[i].rArg1.rWld = areaList[area].aWldIndex.iThing[iWld];
        resetList[i].rArg3.rNum = OBJECTGETFIELD(reset, OF_RESET_ARG3);
        break;
  
      case 'R':
        resetList[i].rArg1.rWld = areaList[area].aWldIndex.iThing[iWld];
        break;
  
      default:
        resetList[i].rArg1.rNum = OBJECTGETFIELD(reset, OF_RESET_ARG1);
        resetList[i].rArg3.rNum = OBJECTGETFIELD(reset, OF_RESET_ARG3);
      }
      i++;
    }
  }
  areaList[area].aResetList = resetList; 
  areaList[area].aResetNum = i;
  sprintf(buf, "^gReset table updated. ^w(%ld reset commands)\n", i);
  SendThing(buf, thing);
  BITSET(areaList[area].aSystem, AS_RSTUNSAVED);
}

CMDPROC(CmdRHide) { /* void CmdProc(THING *thing, BYTE* cmd) */
  LWORD  area;
  LWORD  i;
  THING *t;
  THING *next;

  area = AGetArea(thing, &cmd);
  if (area == -1) {
    SendThing("^wYou cant do that here...\n", thing);
    return;
  }

  SendThing("^gWARNING: any changes made since the last ^wRUPDATE ^gwere just lost!\n", thing);

  if (!BIT(areaList[area].aSystem, AS_RSTSHOWN)) {
    SendThing("Reset Commands are allready hidden!\n", thing);
    return;
  }

  /* Clear everything out */
  for (i=0; i<areaList[area].aWldIndex.iNum; i++) {
    for(t=areaList[area].aWldIndex.iThing[i]->tContain; t; t=next ) {
      next = t->tNext;
      if (t->tType==TTYPE_OBJ && Obj(t)->oTemplate->oType==OTYPE_RESETCMD)
        ThingFree(t);
    }
  }

  BITCLR(areaList[area].aSystem, AS_RSTSHOWN);
  SendThing("Reset Commands hidden.\n", thing);
}

CMDPROC(CmdRSave) { /* void CmdProc(THING *thing, BYTE* cmd) */
  CmdASave(thing, cmd);
}




