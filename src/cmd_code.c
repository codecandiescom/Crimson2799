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

/* code related commands */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "crimson2.h"
#include "macro.h"
#include "queue.h"
#include "log.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "file.h"
#include "thing.h"
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
#include "skill.h"
#include "player.h"
#include "parse.h"
#include "cmd_inv.h"
#include "cmd_move.h"
#include "cmd_god.h"
#include "cmd_code.h"
#include "code.h"
#include "compile.h"
#include "interp.h"
#include "decomp.h"
#include "function.h"
#include "codestuf.h"

CMDPROC(CmdComp) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE      srcKey[256];
  BYTE      dstKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  LWORD     dstOffset;
  THING    *found;
  THING    *search;
  PROPERTY *property;
  BYTE buf[256];

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  search = Base(thing)->bInside;
  found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB, &srcOffset);
  if (!found) {
    if (srcKey[0]=='\'') found=search;
    else {
      SendThing("^bThere doesnt appear to be anything like that around\n", thing);
      return;
    }
  }

  while (found && srcNum!=0) {
    /* show them the stats on the WORLD */
    /* first line, # Name & Type */
    sprintf(buf, "^yCOMPILE: ^b%s\n",found->tSDesc->sText);
    SendThing(buf, thing);

    for (property = found->tProperty; property; property=property->pNext) {
      SendThing("^gProp.: ^c", thing);
      SendThing(property->pKey->sText, thing);
      SendThing("\n", thing);
      if (property->pKey->sText[0]=='@') {
        if (!CodeCompileProperty(property,thing)) {
          CodeSetFlag(found, property);
        }
      }
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdDisass) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE      srcKey[256];
  BYTE      dstKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  LWORD     dstOffset;
  THING    *found;
  THING    *search;
  PROPERTY *property;
  BYTE      buf[256];

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  search = Base(thing)->bInside;
  found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB, &srcOffset);
  if (!found) {
    if (srcKey[0]=='\'') found=search;
    else {
      SendThing("^bThere doesnt appear to be anything like that around\n", thing);
      return;
    }
  }

  while (found && srcNum!=0) {
    /* show them the stats on the WORLD */
    /* first line, # Name & Type */
    sprintf(buf, "^yDISASSEMBLE: ^b%s\n",found->tSDesc->sText);
    SendThing(buf, thing);

    for (property = found->tProperty; property; property=property->pNext) {
      SendThing("^gProp.: ^c", thing);
      SendThing(property->pKey->sText, thing);
      SendThing("\n", thing);
      if (property->pKey->sText[0]=='@') {
        DecompDisassemble(property->pDesc, thing);
      }
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdDecomp) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE      srcKey[256];
  BYTE      dstKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  LWORD     dstOffset;
  THING    *found;
  THING    *search;
  PROPERTY *property;
  BYTE      buf[256];

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  search = Base(thing)->bInside;
  found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB, &srcOffset);
  if (!found) {
    if (srcKey[0]=='\'') found=search;
    else {
      SendThing("^bThere doesnt appear to be anything like that around\n", thing);
      return;
    }
  }

  while (found && srcNum!=0) {
    /* show them the stats on the WORLD */
    /* first line, # Name & Type */
    sprintf(buf, "^yDECOMP: ^b%s\n",found->tSDesc->sText);
    SendThing(buf, thing);

    for (property = found->tProperty; property; property=property->pNext) {
      SendThing("^gProp.: ^c", thing);
      SendThing(property->pKey->sText, thing);
      SendThing("\n", thing);
      if (property->pKey->sText[0]=='@') {
        if (!CodeDecompProperty(property,thing)) {
          CodeClearFlag(found, property);
        }
      }
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdDump) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE      srcKey[256];
  BYTE      dstKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  LWORD     dstOffset;
  THING    *found;
  THING    *search;
  PROPERTY *property;
  BYTE      buf[256];

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  search = Base(thing)->bInside;
  found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB, &srcOffset);
  if (!found) {
    if (srcKey[0]=='\'') found=search;
    else {
      SendThing("^bThere doesnt appear to be anything like that around\n", thing);
      return;
    }
  }

  while (found && srcNum!=0) {
    /* show them the stats on the WORLD */
    /* first line, # Name & Type */
    sprintf(buf, "^yDUMP: ^b%s\n",found->tSDesc->sText);
    SendThing(buf, thing);

    for (property = found->tProperty; property; property=property->pNext) {
      SendThing("^gProp.: ^c", thing);
      SendThing(property->pKey->sText, thing);
      SendThing("\n", thing);
      if (property->pKey->sText[0]=='@') {
        InterpDump(property->pDesc,thing);
      }
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdC4Snoop) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE      srcKey[256];
  BYTE      dstKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  LWORD     dstOffset;
  THING    *found;
  THING    *search;
  BASELINK *baseLink;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  search = Base(thing)->bInside;
  found = ThingFind(srcKey, -1, search, TF_PLR|TF_OBJ|TF_MOB, &srcOffset);
  if (!found) {
/*    if (srcKey[0]=='\'') {
      found=search; 
    } else if (srcKey[0]=='\"') {
      found=&(areaList[Wld(search)->wArea].aResetThing); 
    } else {*/
      SendThing("^bThere doesn't appear to be anything like that around\n",thing);
      return;
  /*  }*/
  }

  if (found == thing) {
    SendAction("^bStopping all C4 snoops:\n", thing, NULL, SEND_SRC);
    baseLink= BaseLinkFind(Base(thing)->bLink, BL_C4_RCV, NULL);
    while(baseLink) {
      SendAction("^bStopping C4 snoop on $N\n", thing, baseLink->lDetail.lThing, SEND_SRC|SEND_CAPFIRST);
      BaseLinkFree(thing, baseLink);
      baseLink= BaseLinkFind(Base(thing)->bLink, BL_C4_RCV, NULL);
    }
    return;
  }

  /* Check for an already existing link */
  baseLink = BaseLinkFind(Base(thing)->bLink, BL_C4_RCV, found);
  if (baseLink) {
    SendAction("^bYou stop C4 snoop on $N\n", thing, found, SEND_SRC|SEND_CAPFIRST);
    BaseLinkFree(thing, baseLink);
  } else {
    /* otherwise make new link */
    SendAction("^bYou start C4 snoop on $N\n", thing, found, SEND_SRC|SEND_CAPFIRST);
    BaseLinkCreate(found, thing, BL_C4_SND);
  }
} 

/* List matching commands from the Function module */
CMDPROC(CmdFList) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  LWORD i;
  LWORD parm;
  BYTE  buf[256];

  SendThing("^wMatching C4 functions:\n", thing);
  cmd = StrOneWord(cmd, NULL);
  for (i=0; fTable[i].fText; i++) {
    if (!*cmd || StrFind(fTable[i].fText, cmd)) {
      sprintf(buf, "^3%-6s ^4%-18s ^5( ", cDataType[fTable[i].fDataType], fTable[i].fText);
      SendThing(buf, thing);

      for (parm=0; parm<FMAX_FUNCTION_PARAMETER && fTable[i].fParamType[parm]; parm++) {
        if (parm>0)
          SendThing("^5, ", thing);
        sprintf(buf, "^3%-6s", cDataType[fTable[i].fParamType[parm]]);
        SendThing(buf, thing);
      }

      SendThing("^5)\n", thing);
    }
  }
  
}
