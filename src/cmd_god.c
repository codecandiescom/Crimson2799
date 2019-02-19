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

/* god related commands */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
  #include <unistd.h> /* for unlink */
#endif

#include "crimson2.h"
#include "macro.h"
#include "mem.h"
#include "log.h"
#include "str.h"
#include "queue.h"
#include "send.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "file.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "site.h"
#include "exit.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "affect.h"
#include "fight.h"
#include "mobile.h"
#include "skill.h"
#include "player.h"
#include "parse.h"
#include "help.h"
#include "area.h"
#include "cmd_talk.h"
#include "cmd_inv.h"
#include "cmd_move.h"
#include "cmd_god.h"

CMDPROC(CmdHeal) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  search = Base(thing)->bInside;
  found = ThingFind(srcKey, -1, search, TF_PLR_WLD|TF_PLR, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  while (found && srcNum!=0) {
    SendThing("^bYou heal ^g", thing);
    SendThing(found->tSDesc->sText, thing);
    SendThing("\n", thing);
    SendAction("$n heals you\n", thing, found, SEND_DST|SEND_CAPFIRST);
    SendAction("$n heals $N\n", thing, found, SEND_ROOM|SEND_CAPFIRST);
    Character(found)->cHitP   = CharGetHitPMax(found);
    Character(found)->cMoveP  = CharGetMovePMax(found);
    Character(found)->cPowerP = CharGetPowerPMax(found);

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdPurge) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  search = Base(thing)->bInside;
  found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  while (found && srcNum!=0) {
    SendThing("^bYou blast ^g", thing);
    SendThing(found->tSDesc->sText, thing);
    SendThing("^b right out of existence\n", thing);
    SendAction("$n blasts $N right out of existence\n", thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    ThingExtract(found);
    ThingFree(found);

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = ThingFind(srcKey, -1, search, TF_OBJ|TF_MOB|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdTransfer) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  THING *found;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);
  MAXSET(srcNum, 100); /* after that things could get crowded */


  /* Set to ignore matches in the same room as us */
  if (!STRICMP(srcKey, "all"))
    found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_PLR_WLD, &srcOffset);
  else
    found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_PLR_WLD|TF_OBJ_WLD|TF_MOB_WLD, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  while (found && srcNum!=0) {
    SendThing("^bYou transfer ^g", thing);
    SendThing(found->tSDesc->sText, thing);
    SendThing("\n", thing);
    SendAction("$n idly waves $s hand and summons $N\n", thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    SendAction("$n rips apart space and time and suddenly you are elsewhere\n", thing, found, SEND_DST |SEND_AUDIBLE|SEND_CAPFIRST);
    SendAction("$N rips apart space and time and suddenly $n is gone\n", found, thing, SEND_ROOM |SEND_AUDIBLE|SEND_CAPFIRST);
    ThingTo(found, Base(thing)->bInside);
    CmdLook(found, "");

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    if (!STRICMP(srcKey, "all"))
      found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_PLR_WLD|TF_CONTINUE, &srcOffset);
    else
      found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_PLR_WLD|TF_OBJ_WLD|TF_MOB_WLD|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdSetSkill) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  BYTE   buf[256];
  BYTE   truncateStr[256];
  LWORD  i;
  LWORD  skill;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) { /* if they didnt type anything, give 'em a list */
    SendThing("^pUsuage:\n^P=-=-=-=\n", thing);
    SendThing("^cSETSKILL <Target> <0-255>\n", thing);
    SendThing("\n^gAvailable Skills:\n^G=-=-=-=-= -=-=-=-\n^c", thing);
    for (i=0; *skillList[i].sName; i++) {
      sprintf(buf, "%-19s", StrTruncate(truncateStr,skillList[i].sName, 19));
      SendThing(buf, thing);
      if (i%4 == 3)
        SendThing("\n", thing);
      else
        SendThing(" ", thing);
    }
    if (i%4)
      SendThing("\n", thing); /* last line allways gets a return */
    sprintf(buf, "^g%ld ^bskills listed.\n", i);
    SendThing(buf, thing);
    return;
  }

  cmd = cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  found = ThingFind(srcKey, -1, NULL, TF_PLR_WLD, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  cmd = StrOneWord(cmd, buf); /* get the word in question */
  i = TYPEFIND(buf, skillList);
  if (i == -1) {
    SendThing("^wOh, good one.... I bet you made that up on the spot, didnt you?\n", thing);
    return;
  }
  cmd = StrOneWord(cmd, buf); /* what shall we set it to */
  skill = atoi(buf);
  sprintf(buf,
    "^rYou have changed %s's skill '%s' from %ld to %ld\n",
    found->tSDesc->sText,
    skillList[i].sName,
    (LWORD)Plr(found)->pSkill[i],
    skill);
  Plr(found)->pSkill[i] = skill;
  SendThing(buf, thing);

  sprintf(buf,
    "^r$n has changed your skill at '%s' from %ld to %ld\n",
    skillList[i].sName,
    (LWORD)Plr(found)->pSkill[i],
    skill);
  SendAction(buf, thing, found, SEND_DST |SEND_AUDIBLE|SEND_CAPFIRST);
}

CMDPROC(CmdSetStat) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE      srcKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  THING    *found;
  BYTE      buf[256];
  BYTE      truncateStr[256];
  LWORD     i;
  CHARACTER character;
  PLR       player;
  LWORD    *lword;
  WORD     *word;
  BYTE     *byte;
  LWORD     oldStat;
  LWORD     newStat;
  LWORD     readPlayer = FALSE;

  typedef struct StatList {
    BYTE   *sName;
    ULWORD  sOffset;
    ULWORD  sSize;
    ULWORD  sArray;
    LWORD   sArraySize;
    BYTE    sType;
  } STATLIST;
  
  #define ST_NUMERIC(base,field)      ( ((ULWORD)&(field)) - ((ULWORD)&(base)) ) , sizeof(field), (ULWORD)NULL, 0,              '\0'
  #define ST_FLAG(base, field, array) ( ((ULWORD)&(field)) - ((ULWORD)&(base)) ) , sizeof(field), (ULWORD)array,sizeof(*array), 'F'
  #define ST_TYPE(base, field, array) ( ((ULWORD)&(field)) - ((ULWORD)&(base)) ) , sizeof(field), (ULWORD)array,sizeof(*array), 'T'
  STATLIST statList[] = {
    { "LEVEL",        ST_NUMERIC(character,character.cLevel)                  },
    { "EXPERIENCE",   ST_NUMERIC(character,character.cExp)                    },
    { "AURA",         ST_NUMERIC(character,character.cAura)                   },
    { "MONEY",        ST_NUMERIC(character,character.cMoney)                  },
    { "BANK",         ST_NUMERIC(player,   player.pBank)                      },
    { "SEX",          ST_TYPE(   character,character.cSex, sexList)           },
    { "POSITION",     ST_TYPE(   character,character.cPos, posList)           },
    { "AFFECTFLAG",   ST_TYPE(   character,character.cAffectFlag, affectList) },
    { "HITP",         ST_NUMERIC(character,character.cHitP)                   },
    { "MOVEP",        ST_NUMERIC(character,character.cMoveP)                  },
    { "POWERP",       ST_NUMERIC(character,character.cPowerP)                 },
    { "HITPMAX",      ST_NUMERIC(character,character.cHitPMax)                },
    { "MOVEPMAX",     ST_NUMERIC(player,   player.pMovePMax)                  },
    { "POWERPMAX",    ST_NUMERIC(player,   player.pPowerPMax)                 },
    { "STRENGTH",     ST_NUMERIC(player,   player.pStr)                       },
    { "INTELLIGENCE", ST_NUMERIC(player,   player.pInt)                       },
    { "WISDOM",       ST_NUMERIC(player,   player.pWis)                       },
    { "DEXTERITY",    ST_NUMERIC(player,   player.pDex)                       },
    { "CONSTITUTION", ST_NUMERIC(player,   player.pCon)                       },
    { "PRACTICES",    ST_NUMERIC(player,   player.pPractice)                  },
    { "HUNGER",       ST_NUMERIC(player,   player.pHunger)                    },
    { "THIRST",       ST_NUMERIC(player,   player.pThirst)                    },
    { "INTOXICATION", ST_NUMERIC(player,   player.pIntox)                     },
    { "SYSTEMFLAG",   ST_FLAG(   player,   player.pSystem, pSystemList)       },
    { "AUTOACTION",   ST_FLAG(   player,   player.pAuto, pAutoList)           },
    { "" }
  };

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) { /* if they didnt type anything, give 'em a list */
    SendThing("^pUsuage:\n^P=-=-=-=\n", thing);
    SendThing("^cSETSTAT <Target> <stat> <value>\n", thing);
    SendThing("\n^gAvailable Stats:\n^G=-=-=-=-= -=-=-=-\n^c", thing);
    for (i=0; *statList[i].sName; i++) {
      sprintf(buf, "%-19s", StrTruncate(truncateStr,statList[i].sName, 19));
      SendThing(buf, thing);
      if (i%4 == 3)
        SendThing("\n", thing);
      else
        SendThing(" ", thing);
    }
    if (i%4)
      SendThing("\n", thing); /* last line allways gets a return */
    sprintf(buf, "^g%ld ^bstats listed.\n", i);
    SendThing(buf, thing);
    return;
  }

  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  found = ThingFind(srcKey, -1, NULL, TF_PLR_WLD, &srcOffset);
  if (!found) {
    found = PlayerRead(srcKey, PREAD_NORMAL);
    readPlayer = TRUE;
  }
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  do {
    cmd = StrOneWord(cmd, buf); /* get the word in question */
    i = TYPEFIND(buf, statList);
    if (i == -1) {
      SendThing("^wOh, good one.... I bet you made that stat up on the spot, didnt you?\n", thing);
      goto exit;
    }
    cmd = StrOneWord(cmd, buf); /* what shall we set it to */
    if(statList[i].sType=='\0') {
      newStat = atol(buf);
    } else {
      newStat = TypeFind(buf, statList[i].sArray, statList[i].sArraySize);
      if (newStat == -1) {
        SendThing("^cValid Values are:\n", thing);
        SendArray(statList[i].sArray, statList[i].sArraySize, 3, thing);
        goto exit;
      }
    }
  
    /* change the structure in question */
    switch(statList[i].sSize) {
    case sizeof(BYTE):
      byte = (BYTE*)((ULWORD)found + statList[i].sOffset);
      oldStat = *byte;
      if (statList[i].sType=='F') {
        BITFLIP(*byte, (1<<newStat) );
      } else {
        *byte = newStat;
      }
      newStat = *byte; /* tell them if it overflowed */
      break;
    
    case sizeof(WORD):
      word = (WORD*)((ULWORD)found + statList[i].sOffset);
      oldStat = *word;
      if (statList[i].sType=='F') {
        BITFLIP(*word, (1<<newStat) );
      } else {
        *word = newStat;
      }
      newStat = *word; /* tell them if it overflowed */
      break;
    
    case sizeof(LWORD):
      lword = (LWORD*)((ULWORD)found + statList[i].sOffset);
      oldStat = *lword;
      if (statList[i].sType=='F') {
        BITFLIP(*lword, (1<<newStat) );
      } else {
        *lword = newStat;
      }
      newStat = *lword; /* tell them if it overflowed */
      break;
    
    default:
      SendThing("I dont know how to change that stat sorry... (illegal stat size)\n", thing);
      goto exit;
    }
  
    /* keep the God posted on results */
    if (statList[i].sType=='F') {
      sprintf(buf,
        "^rYou have changed %s's stat '%s'.\n",
        found->tSDesc->sText,
        statList[i].sName);
      SendThing(buf, thing);
      SendThing("^gCurrently Set Flags: ^G[^c", thing);
      SendThing(FlagSprintf(buf, newStat, (BYTE**)statList[i].sArray, ' ', 512), thing);
      SendThing("^G]\n", thing);
    } else {
      sprintf(buf,
        "^rYou have changed %s's stat '%s' from %ld to %ld\n",
        found->tSDesc->sText,
        statList[i].sName,
        oldStat,
        newStat);
      SendThing(buf, thing);
    }

    /* Keep the target posted as to what happened */
    if (statList[i].sType=='F') {
      sprintf(buf,
        "^r$n has changed your stat '%s'.\n",
        statList[i].sName);
      SendAction(buf, thing, found, SEND_DST|SEND_CAPFIRST);
      SendThing("^gCurrently Set Flags: ^G[^c", found);
      SendThing(FlagSprintf(buf, newStat, (BYTE**)statList[i].sArray, ' ', 512), found);
      SendThing("^G]\n", found);
    } else {
      sprintf(buf,
        "^r$n has changed your stat '%s' from %ld to %ld\n",
        statList[i].sName,
        oldStat,
        newStat);
      SendAction(buf, thing, found, SEND_DST|SEND_CAPFIRST);
    }
  } while (0);

  exit: if (readPlayer) {
    PlayerWrite(found, PWRITE_PLAYER);
    THINGFREE(found);
  }
}

/* modify a property on the fly */
CMDPROC(CmdSetProp) {
  BYTE         buf[256];
  BYTE         srcKey[256];
  LWORD        srcNum;
  LWORD        srcOffset;
  THING       *found;
  SOCK        *sock;
  PROPERTY    *property;

  sock = BaseControlFind(thing);
  if (!sock) return;
  
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    SendThing("^bTry ^gSETPROPERTY <thing> <property> <value> ^b- set mob/obj/player\n", thing);
    SendThing("^bor  ^gSETPROPERTY . <property> <value>       ^b- set current room\n", thing);
    return;
  }

  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  if (StrExact(srcKey, "."))
    found = Base(thing)->bInside;
  else
    found = ThingFind(srcKey, -1, Base(thing)->bInside->tContain, TF_MOB|TF_OBJ|TF_PLR_WLD, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  /* No property selected */
  if (!*cmd) {
    SendThing("^bYou have to specifiy a property name you know\n", thing);
    return;
  }
  cmd = StrOneWord(cmd, buf); /* lose the command at the start */

  property = PropertyFind(thing->tProperty, buf);
  if (*cmd) {
    if (property) {
      STRFREE(property->pDesc);
      property->pDesc = STRCREATE(cmd);
    } else {
      thing->tProperty = PropertyCreate(thing->tProperty, STRCREATE(buf), STRCREATE(cmd));
      property = thing->tProperty;
    }

    SendThing("Property Set!\n", thing);
    SendThing("^cKey: ^g", thing);
    SendThing(property->pKey->sText, thing);
    SendThing("\n^b", thing);
    SendThing(property->pDesc->sText, thing);
    SendThing("\n", thing);
  } else {
    if (property) {
      thing->tProperty = PropertyFree(thing->tProperty, property);
      SendThing("Property Deleted!\n", thing);
    } else {
      SendThing("No such property exists!\n", thing);
    }
  }
}


CMDPROC(CmdGoto) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, NULL, TF_WLD|TF_PLR_WLD|TF_OBJ_WLD|TF_MOB_WLD, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  SendThing("^bYou goto ^g", thing);
  SendThing(found->tSDesc->sText, thing);
  SendThing("\n", thing);
  if (thing->tType==TTYPE_PLR)
    SendAction(Plr(thing)->pExit->sText, thing, NULL, SEND_ROOM|SEND_VISIBLE);
  else
    SendAction("$n dissolves into a cloud of blue sparkles.\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  if (found->tType == TTYPE_WLD)
    ThingTo(thing, found);
  else 
    ThingTo(thing, Base(found)->bInside);
  FightStop(thing);
  if (thing->tType==TTYPE_PLR)
    SendAction(Plr(thing)->pEnter->sText, thing, NULL, SEND_ROOM|SEND_VISIBLE);
  else
    SendAction("$n appears from a cloud of blue sparkles.\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  CmdLook(thing, "");
}

CMDPROC(CmdVNum) { /* (BYTE *cmd, THING *thing) */
  LWORD area;
  LWORD i;
  BYTE  buf[256];
  
  cmd = StrOneWord(cmd, buf); /* buf = cmd */
  cmd = StrOneWord(cmd, buf); /* buf = mobile|object */
  if (!StrAbbrev("mobile",buf) && !StrAbbrev("object",buf) && !StrAbbrev("world",buf)) {
    SendThing("^cUSUAGE: vnum <\"object\" | \"mobile\" | \"world\"> <key>\n", thing);
    return;
  }
  if (!*cmd) {
    SendThing("^cUSUAGE: vnum <\"object\" | \"mobile\" | \"world\"> <key>\n", thing);
    return;
  }
  
  /* show matching mobs */
  if (StrAbbrev("mobile",buf)) {
    for (area=0; area<areaListMax; area++) {
      for (i=0; i<areaList[area].aMobIndex.iNum; i++) {
        if (StrIsKey(cmd, MobTemplate(areaList[area].aMobIndex.iThing[i])->mKey)) {
          sprintf(buf, 
                  "^c%5ld - ^g%s ^w[%ld]\n", 
                  MobTemplate(areaList[area].aMobIndex.iThing[i])->mVirtual,
                  MobTemplate(areaList[area].aMobIndex.iThing[i])->mSDesc->sText,
                  MobTemplate(areaList[area].aMobIndex.iThing[i])->mOnline);
          SendThing(buf, thing);
        }
      }
    }
  
  /* show matching objects */
  } else if (StrAbbrev("object",buf)) {
    for (area=0; area<areaListMax; area++) {
      for (i=0; i<areaList[area].aObjIndex.iNum; i++) {
        if (StrIsKey(cmd, ObjTemplate(areaList[area].aObjIndex.iThing[i])->oKey)) {
          sprintf(buf, 
                  "^c%5ld - ^g%s ^w[%ld+%ld]\n", 
                  ObjTemplate(areaList[area].aObjIndex.iThing[i])->oVirtual,
                  ObjTemplate(areaList[area].aObjIndex.iThing[i])->oSDesc->sText,
                  ObjTemplate(areaList[area].aObjIndex.iThing[i])->oOnline,
                  ObjTemplate(areaList[area].aObjIndex.iThing[i])->oOffline);
          SendThing(buf, thing);
        }
      }
    }
  
  /* show matching objects */
  } else if (StrAbbrev("world",buf)) {
    for (area=0; area<areaListMax; area++) {
      for (i=0; i<areaList[area].aWldIndex.iNum; i++) {
        if (StrIsKey(cmd, Thing(areaList[area].aWldIndex.iThing[i])->tSDesc)) {
          sprintf(buf, 
                  "^c%5ld - ^g%s\n", 
                  Wld(areaList[area].aWldIndex.iThing[i])->wVirtual,
                  Thing(areaList[area].aWldIndex.iThing[i])->tSDesc->sText);
          SendThing(buf, thing);
        }
      }
    }
  }
  
}

CMDPROC(CmdStat) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE         srcKey[256];
  LWORD        srcNum;
  LWORD        srcOffset;
  THING       *found;
  BYTE         buf[256];
  BYTE         eName[256];
  BYTE         truncateStr[256];
  LWORD        i;
  EXTRA       *extra;
  PROPERTY    *property;
  EXIT        *exit;
  OBJTEMPLATE *object;
  BASELINK    *link;
  LWORD        readPlayer = FALSE;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (*cmd) {
    cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
    found = ThingFind(srcKey, -1, NULL, TF_PLR_ANYWHERE|TF_OBJ_ANYWHERE|TF_MOB_ANYWHERE, &srcOffset);
    if (!found) {
      found = PlayerRead(srcKey, PREAD_SCAN);
      readPlayer = TRUE;
    }
    if (!found) {
      SendThing("^bThere doesnt appear to be anything like that around\n", thing);
      return;
    }
  } else
    found = Base(thing)->bInside;

  /* show them the stats */
  SendThing("^gSDesc:^G[^c", thing);
  SendThing(found->tSDesc->sText, thing);
  SendThing("^G]\n", thing);
  if (found->tType >= TTYPE_BASE) {
    SendThing("^gLDesc: ^c", thing);
    SendThing(Base(found)->bLDesc->sText, thing);
  }
  SendThing("\n^gDescription:^b\n", thing);
  SendThing(found->tDesc->sText, thing);

  switch (found->tType) {

  case TTYPE_WLD:
    sprintf(buf, "^g#:^G[%ld] ^gType:^G[^c", Wld(found)->wVirtual);
    SendThing(buf, thing);
    SendThing(TYPESPRINTF(buf, Wld(found)->wType, wTypeList, 512), thing);
    SendThing("^G]\n", thing);
    /* 2nd line - flags */
    SendThing("^gFlags: ^G[^c", thing);
    SendThing(FlagSprintf(buf, Wld(found)->wFlag, wFlagList, ' ', 512), thing);
    SendThing("^G]\n", thing);
    SendThing("^gExits:\n", thing);
    for (exit = Wld(found)->wExit; exit; exit=exit->eNext) {
      ExitGetName(exit, eName);
      sprintf(buf, "^G[^c%-5s^G] ^gto room", eName);
      SendThing(buf, thing);
      if (!exit->eWorld) {
        SendThing("^G[^c NULL^G]", thing);
      } else {
        sprintf(buf, "^G[^w%5ld^G]", Wld(exit->eWorld)->wVirtual);
        SendThing(buf, thing);
      }
      SendThing("^g, eFlags:^G[^b", thing);
      SendThing(FlagSprintf(buf, exit->eFlag, eFlagList, ' ', 512), thing);
      SendThing("^G]\n", thing);
      if (exit->eKeyObj!=-1 || exit->eKey->sText[0]) {
        sprintf(buf, "^gKey:^G[^b%5ld^G] ^gKeywords:^G[^b", exit->eKeyObj);
        SendThing(buf, thing);
        SendThing(exit->eKey->sText, thing);
        SendThing("^G]\n", thing);
      }
    }
    break;

  case TTYPE_OBJ:
    object = Obj(found)->oTemplate;
    sprintf(buf, "^g#:^G[%ld] ^gOnline^G[^c%ld^G] ^gOffline^G[^c%ld^G] ^gType:^G[^c", object->oVirtual, object->oOnline, object->oOffline);
    SendThing(buf, thing);
    SendThing(TYPESPRINTF(buf, object->oType, oTypeList, 512), thing);
    SendThing("^G]\n^gKeywords:^G[^c", thing);
    SendThing(Base(found)->bKey->sText, thing);
    SendThing("^G]\n^gEquipped As: ^G[^c", thing);
    SendThing(FlagSprintf(buf, Obj(found)->oEquip, equipList, ' ', 512), thing);
    SendThing("^G]\n^gAct: ^G[^c", thing);
    SendThing(FlagSprintf(buf, Obj(found)->oAct, oActList, ' ', 512), thing);
    SendThing("^G]\n^gWear: ^G[^c", thing);
    SendThing(TYPESPRINTF(buf, object->oWear, wearList, 512), thing);
    SendThing("^G]\n", thing);
    sprintf(buf, "^gWeight:^G[^c%hd^G] ", object->oWeight);
    SendThing(buf, thing);
    sprintf(buf, "^gValue:^G[^c%ld^G] ", object->oValue);
    SendThing(buf, thing);
    sprintf(buf, "^gRent:^G[^c%ld^G]\n", object->oRent);
    SendThing(buf, thing);
    SendThing("^gType-Specific Information\n", thing);
    for (i=0;  
         i<OBJECT_MAX_FIELD 
         && ObjectGetFieldStr(object->oType, &Obj(found)->oDetail, i, buf, 512); 
         i++) {
      SendThing(buf, thing);
    }
    for (i=0; i<OBJECT_MAX_APPLY; i++) {
      if (!Obj(found)->oApply[i].aType) break;
      SendThing("^gApply:^G[^c", thing);
      SendThing(TYPESPRINTF(buf, Obj(found)->oApply[i].aType, applyList, 512), thing);
      sprintf(buf, "^G] ^gValue:^G[^c%hd^G]\n", (WORD)Obj(found)->oApply[i].aValue);
      SendThing(buf, thing);
    }
    break;

  case TTYPE_MOB:
    sprintf(buf, "^g#:^G[%ld] ^gOnline^G[^c%ld^G] ^gAct:^G[^c", Mob(found)->mTemplate->mVirtual, Mob(found)->mTemplate->mOnline);
    SendThing(buf, thing);
    SendThing(FlagSprintf(buf, Mob(found)->mTemplate->mAct, mActList, ' ', 512), thing);
    SendThing("^G]\n", thing);
    SendThing("^gAffect: ^G[^c", thing);
    SendThing(FlagSprintf(buf, Character(found)->cAffectFlag, affectList, ' ', 512), thing);
    SendThing("^G]\n", thing);
    sprintf(buf, "^gAura:^G[^c%hd^G] ", Character(found)->cAura);
    SendThing(buf, thing);
    sprintf(buf, "^gLevel:^G[^c%hd^G] ", Character(found)->cLevel);
    SendThing(buf, thing);
    sprintf(buf, "^gHit Bonus:^G[^c%ld^G] ", CharGetHitBonus(found, Character(found)->cWeapon));
    SendThing(buf, thing);
    sprintf(buf, "^gDam Bonus:^G[^c%ld^G] ", CharGetDamBonus(found, Character(found)->cWeapon));
    SendThing(buf, thing);
    sprintf(buf, "^gArmor:^G[^c%hd^G]\n", Character(found)->cArmor);
    SendThing(buf, thing);
    sprintf(buf, "^gHP:^G[^c%ld^G/^c%ld^g] ", Character(found)->cHitP, CharGetHitPMax(found));
    SendThing(buf, thing);
    sprintf(buf, "^gMV:^G[^c%ld^G/^c%ld^g] ", Character(found)->cMoveP, CharGetMovePMax(found));
    SendThing(buf, thing);
    sprintf(buf, "^gPP:^G[^c%ld^G/^c%ld^g]\n", Character(found)->cPowerP, CharGetPowerPMax(found));
    SendThing(buf, thing);
    sprintf(buf, "^gBare-Handed Damage:^G[^c%hd^gD^c%hd+%hd^G]\n", 
      Mob(found)->mTemplate->mDamDiceNum, 
      Mob(found)->mTemplate->mDamDiceSize, 
      Mob(found)->mTemplate->mDamBonus);
    SendThing(buf, thing);
    sprintf(buf, "^gMoney:^G[^c%ld^G] ", Character(found)->cMoney);
    SendThing(buf, thing);
    sprintf(buf, "^gExp:^G[^c%ld^G] ", Character(found)->cExp);
    SendThing(buf, thing);
    SendThing("^gPos:^G[^c", thing);
    SendThing(TYPESPRINTF(buf, Character(found)->cPos, posList, 512), thing);
    SendThing("^G] ^gType:^G[^c", thing);
    SendThing(TYPESPRINTF(buf, Mob(found)->mTemplate->mType, mTypeList, 512), thing);
    SendThing("^G] ^gSex:^G[^c", thing);
    SendThing(TYPESPRINTF(buf, Character(found)->cSex, sexList, 512), thing);
    SendThing("^G]\n", thing);
    break;
    
  case TTYPE_PLR:
    SendThing("^gAffect: ^G[^c", thing);
    SendThing(FlagSprintf(buf, Character(found)->cAffectFlag, affectList, ' ', 512), thing);
    SendThing("^G]\n", thing);
    SendThing("^gHas Equip:  ^G[^c", thing);
    SendThing(FlagSprintf(buf, Character(found)->cEquip, equipList, ' ', 512), thing);
    SendThing("^G]\n", thing);
    SendThing("^gClass:^G[", thing);
    SendThing(TYPESPRINTF(buf, Plr(found)->pClass, classList, sizeof(buf)), thing);
    SendThing("^G] ^gRace:^G[", thing);
    SendThing(TYPESPRINTF(buf, Plr(found)->pRace, raceList, sizeof(buf)), thing);
    SendThing("^G]\n", thing);
    sprintf(buf, "^gAura:^G[^c%hd^G] ", Character(found)->cAura);
    SendThing(buf, thing);
    sprintf(buf, "^gLevel:^G[^c%hd^G] ", Character(found)->cLevel);
    SendThing(buf, thing);
    sprintf(buf, "^gHit Bonus:^G[^c%ld^G] ", CharGetHitBonus(found, Character(found)->cWeapon));
    SendThing(buf, thing);
    sprintf(buf, "^gDam Bonus:^G[^c%ld^G] ", CharGetDamBonus(found, Character(found)->cWeapon));
    SendThing(buf, thing);
    sprintf(buf, "^gArmor:^G[^c%hd^G]\n", Character(found)->cArmor);
    SendThing(buf, thing);
    sprintf(buf, "^gHP:^G[^c%ld^G/^c%ld^g] ", Character(found)->cHitP, CharGetHitPMax(found));
    SendThing(buf, thing);
    sprintf(buf, "^gMV:^G[^c%ld^G/^c%ld^g] ", Character(found)->cMoveP, CharGetMovePMax(found));
    SendThing(buf, thing);
    sprintf(buf, "^gPP:^G[^c%ld^G/^c%ld^g]\n", Character(found)->cPowerP, CharGetPowerPMax(found));
    SendThing(buf, thing);
    sprintf(buf, 
            "^gStr:^G[^c%hd^G] ^gWis:^G[^c%hd^G] ^gInt:^G[^c%hd^G] ^gDex:^G[^c%hd^G] ^gCon:^G[^c%hd^G]\n",
            Plr(found)->pStr,
            Plr(found)->pWis,
            Plr(found)->pInt,
            Plr(found)->pDex,
            Plr(found)->pCon);
    SendThing(buf, thing);
    sprintf(buf, 
            "^gResist:^GPU[^c%hd^G] ^GSL[^c%hd^G] ^GCO[^c%hd^G] ^GHT[^c%hd^G] ^GEMR[^c%hd^G] ^GLAS[^c%hd^G] ^GPSY[^c%hd^G] ^GACID[^c%hd^G] ^GPOI[^c%hd^G]\n",
            CharGetResist(found, FD_PUNCTURE),
            CharGetResist(found, FD_SLASH),
            CharGetResist(found, FD_CONCUSSIVE),
            CharGetResist(found, FD_HEAT),
            CharGetResist(found, FD_EMR),
            CharGetResist(found, FD_LASER),
            CharGetResist(found, FD_PSYCHIC),
            CharGetResist(found, FD_ACID),
            CharGetResist(found, FD_POISON));
    SendThing(buf, thing);
    sprintf(buf, "^gMoney:^G[^c%ld^G] ", Character(found)->cMoney);
    SendThing(buf, thing);
    sprintf(buf, "^gExp:^G[^c%ld^G/^c%ld^G] ", Character(found)->cExp, PlayerExpNeeded(found));
    SendThing(buf, thing);
    SendThing("^gPos:^G[^c", thing);
    SendThing(TYPESPRINTF(buf, Character(found)->cPos, posList, 512), thing);
    SendThing("^G] ^gSex:^G[^c", thing);
    SendThing(TYPESPRINTF(buf, Character(found)->cSex, sexList, 512), thing);
    SendThing("^G]\n", thing);
    /* Hunger Thirst Intox */
    sprintf(buf, "^gBank:^G[^c%ld^G] ", Plr(found)->pBank);
    SendThing(buf, thing);
    sprintf(buf, "^gHunger:^G[^c%hd^G] ", Plr(found)->pHunger);
    SendThing(buf, thing);
    sprintf(buf, "^gThirst:^G[^c%hd^G] ", Plr(found)->pThirst);
    SendThing(buf, thing);
    sprintf(buf, "^gIntox:^G[^c%hd^G]\n", Plr(found)->pIntox);
    SendThing(buf, thing);
    break;

  default:
    break;
  }
  for (extra = found->tExtra; extra; extra=extra->eNext) {
    sprintf(buf,
            "^gExtra: ^c%-31s = ",
            StrTruncate(truncateStr,extra->eKey->sText,  30));
    SendThing(buf, thing);
    StrTruncate(truncateStr,extra->eDesc->sText, 30);
    StrOneLine(truncateStr);
    sprintf(buf, "%s\n", truncateStr);
    SendThing(buf, thing);
  }
  for (property = found->tProperty; property; property=property->pNext) {
    sprintf(buf,
            "^gProp.: ^c%-31s = ",
            StrTruncate(truncateStr,property->pKey->sText,  30));
    SendThing(buf, thing);
    StrTruncate(truncateStr,property->pDesc->sText, 30);
    StrOneLine(truncateStr);
    sprintf(buf, "%s\n", truncateStr);
    SendThing(buf, thing);
  }
  if (found->tType >= TTYPE_BASE) {
    for (link = Base(found)->bLink; link; link=link->lNext) {
      if (link->lType==BL_CONTROL) {
        sprintf(buf, 
                "^gLink: [%s] %s@%s\n",
                linkTypeList[link->lType],
                link->lDetail.lSock->sHomeThing->tSDesc->sText,
                link->lDetail.lSock->sSiteName);
      } else {
        sprintf(buf, 
                "^gLink: [%s] -> %s\n",
                linkTypeList[link->lType],
                link->lDetail.lThing->tSDesc->sText);
      }
      SendThing(buf, thing);
    }
  }

  if (readPlayer) {
    THINGFREE(found);
  }
}

CMDPROC(CmdAt) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *from;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, NULL, TF_PLR_WLD|TF_OBJ_WLD|TF_MOB_WLD, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  from = Base(thing)->bInside;
  ThingTo(thing, Base(found)->bInside);
  ParseCommand(thing, cmd);
  ThingTo(thing, from);
}

CMDPROC(CmdForce) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  SOCK  *sock;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, NULL, TF_PLR_WLD|TF_MOB_WLD, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  /* tell the god what s/he did */
  SendAction("^rYou force $N to ^w'", thing, found, SEND_SRC|SEND_CAPFIRST);
  SendAction(cmd,  thing, found, SEND_SRC);
  SendAction("'\n", thing, found, SEND_SRC);
  /* warn the victim */
  SendAction("^r$n forces you to ^w'", thing, found, SEND_DST|SEND_CAPFIRST);
  SendAction(cmd,  thing, found, SEND_DST);
  SendAction("'\n", thing, found, SEND_DST);

  /* make the victim act */
  sock = BaseControlFind(found);
  if (!sock) {
    ParseCommand(found, cmd);
  } else {
    QInsert(sock->sIn, "\n");
    QInsert(sock->sIn, cmd);
  }
}

/* Force logout a character */
CMDPROC(CmdCutConn) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  SOCK  *sock;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, NULL, TF_PLR_WLD, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  sock = BaseControlFind(found);
  if (!sock) {
    SendAction("^r$N doesnt seem to have a connection to cut\n", thing, found, SEND_SRC|SEND_CAPFIRST);
  } else {
    /* tell the god what s/he did */
    SendAction("^rYou cut $N's connection\n", thing, found, SEND_SRC|SEND_CAPFIRST);
    /* warn the victim */
    SendAction("^r$n cuts your connection\n", thing, found, SEND_DST|SEND_CAPFIRST);
    /* save them out */
    /* Update where they start */
    if (found->tType == TTYPE_PLR && Base(found)->bInside && Base(found)->bInside->tType==TTYPE_WLD)
      Plr(found)->pStartRoom = Wld(Base(found)->bInside)->wVirtual;
    else
      Plr(found)->pStartRoom = -1;
    PlayerWrite(found, PWRITE_PLAYER);
    sock->sMode = MODE_KILLSOCKET; /* they're not long for this world.... */
  }
}

/* Delete a player */
CMDPROC(CmdDelPlayer) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  THING *player;
  SOCK  *sock = NULL;
  BYTE   buf[256];
  BYTE   readPlayer = FALSE;

  cmd = StrOneWord(cmd, buf); /* lose the command at the start */
  if (STRICMP(buf, "delplayer")) {
    SendThing("^wCarefull with that! ^g(type DELPLAYER in full if you mean it)\n", thing);
    return;
  }

  player = PlayerFind(cmd);
  if (!player) {
    player = PlayerRead(cmd, PREAD_SCAN);
    if (!player) {
      SendThing("^wThey don't seem to exist, so you cant delete 'em.\n", thing);
      return;
    }
    readPlayer = TRUE;
  }

  sock = BaseControlFind(thing);
  if (Character(player)->cLevel >= Character(sock->sHomeThing)->cLevel) {
    SendThing("^wThey are too godlike for you to delete 'em.\n", thing);
    return;
  }
  
  if (readPlayer) {
    THINGFREE(player);
    /* read their objects in too so that we update # in game */
    player = PlayerRead(cmd, PREAD_NORMAL);
  }

  /* tell the god what s/he did */
  SendAction("^rYou ^wDELETE ^r$N\n", thing, player, SEND_SRC|SEND_CAPFIRST);
  /* tell others */
  SendAction("^r$n ^wDELETES ^r$N - they're gone....\n", thing, player, SEND_ROOM|SEND_CAPFIRST);
  /* tell victim */
  SendAction("^r$n ^wDELETES ^ryou - you're outta here buddy\n", thing, player, SEND_DST|SEND_CAPFIRST);

  /* get rid of the playerFile - but keep a backup in case we change our mind */
  PlayerDelete(player);
}


/* Undelete a player, if the .plr.del file exists */
CMDPROC(CmdUnDelPlayer) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   bufPlrDel[256];
  BYTE   bufCrash[256];
  BYTE   bufPlr[256];
  BYTE   bufCmd[256];
  BYTE   buf[256];
  BYTE   playerName[256];
  FILE  *filePlr;
  FILE  *fileCrash;

  cmd = StrOneWord(cmd, buf); /* lose the command at the start */
  if (STRICMP(buf, "undelplayer")) {
    SendThing("^wCarefull with that! ^g(type UNDELPLAYER in full if you mean it)\n", thing);
    return;
  }
  cmd=StrOneWord(cmd,playerName);
  StrToLower(playerName);
  if (!playerName[0]) {
    SendThing("^wCarefull with that! ^g(type UNDELPLAYER <player name> )\n", thing);
    return;
  }
#ifdef WIN32
  sprintf(bufCrash,"crash\\%s.plr",playerName);
  sprintf(bufPlr,"player\\%s.plr",playerName);
  sprintf(bufPlrDel,"player\\%s.plr.del",playerName);
  sprintf(bufCmd,"move %s %s",bufPlrDel,bufPlr);
#else
  sprintf(bufCrash,"crash/%s.plr",playerName);
  sprintf(bufPlr,"player/%s.plr",playerName);
  sprintf(bufPlrDel,"player/%s.plr.del",playerName);
  sprintf(bufCmd,"mv %s %s",bufPlrDel,bufPlr);
#endif
  /* check first if a player file exists */
  fileCrash=fopen(bufCrash,"rb");
  filePlr=fopen(bufPlr,"rb");
  if (filePlr||fileCrash) {
    if (fileCrash)
      fclose(fileCrash);
    if (filePlr)
      fclose(filePlr);
    /* tell the god what s/he did */
    sprintf(buf,"^rYou ^wUNDELETE ^r%s but the player is not deleted!!",
      playerName);
    SendAction(buf, thing, NULL, SEND_SRC|SEND_CAPFIRST);
    /* tell others */
    sprintf(buf,"^r$n ^wUNDELETES ^r%s - but they are not deleted!",
      playerName);
    SendAction(buf, thing, NULL, SEND_ROOM|SEND_CAPFIRST);
    return;
  }
  sprintf(buf,"^rYou ^wUNDELETE ^r%s.",
    playerName);
  SendAction(buf, thing, NULL, SEND_SRC|SEND_CAPFIRST);
  /* tell others */
  sprintf(buf,"^r$n ^wUNDELETES ^r%s.",
    playerName);
  SendAction(buf, thing, NULL, SEND_ROOM|SEND_CAPFIRST);
  system(bufCmd);
  return;
}

/* Watch what somebody is up to */
CMDPROC(CmdSnoop) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE      srcKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  THING    *found;
  BASELINK *baseLink;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, NULL, TF_PLR_WLD|TF_MOB, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  if (found == thing) {
    /* stop all snoops */
    SendAction("^rYou just snoop yourself\n", thing, found, SEND_SRC|SEND_CAPFIRST);
    baseLink = BaseLinkFind(Base(thing)->bLink, BL_TELEPATHY_RCV, NULL);
    while (baseLink) {
      SendAction("^rYou stop spying on $N\n", thing, baseLink->lDetail.lThing, SEND_SRC|SEND_CAPFIRST);
      BaseLinkFree(thing, baseLink);
      baseLink = BaseLinkFind(Base(thing)->bLink, BL_TELEPATHY_RCV, NULL);
    }
    return;
  }

  /* Check for an allready existing link, if found remove it */
  baseLink = BaseLinkFind(Base(thing)->bLink, BL_TELEPATHY_RCV, found);
  if (baseLink) {
    SendAction("^rYou stop spying on $N\n", thing, found, SEND_SRC|SEND_CAPFIRST);
    BaseLinkFree(thing, baseLink);
  } else {
    /* Otherwise, make a new link */
    SendAction("^rYou start spying on $N\n", thing, found, SEND_SRC|SEND_CAPFIRST);
    BaseLinkCreate(found, thing, BL_TELEPATHY_SND);
  }
}

CMDPROC(CmdSite) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  SITE *site;
  SOCK *sock;
  BYTE  buf[256];
  BYTE  siteTime[256];
  LWORD i;

  sock = BaseControlFind(thing);
  if (!sock) return;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    sprintf(buf, "^w%-25s ^p%-7s ^P%-25s ^b%s\n",
      "Site",
      "Status",
      "Modified on Date",
      "By Player"
    );
    SendThing(buf, thing);
    sprintf(buf, "^V%-25s ^P%-7s ^R%-25s ^B%s\n",
      "====",
      "======",
      "================",
      "========="
    );
    SendThing(buf, thing);
    for(i=0; i<siteIndex.iNum; i++) {
      site = Site(siteIndex.iThing[i]);
      strcpy(siteTime, ctime(&site->sDate));
      siteTime[strlen(siteTime)-1] = '\0';
      sprintf(buf, "^w%-25s ^p%-7s ^P%-25s ^b%s\n",
        site->sSite,
        sTypeList[site->sType],
        siteTime,
        site->sPlayer->sText
      );
      SendThing(buf, thing);
    }
    SendThing("\n", thing);
    sprintf(buf, "^gTotal Number of Special Sites: ^G[^c%ld^G]\n", siteIndex.iNum);
    SendThing(buf, thing);
    return;
  }

  cmd = StrOneWord(cmd, buf); /* lose the command at the start */
  i = TYPEFIND(cmd, sTypeList);
  if (!*cmd || i == -1) {
    SendThing("Never heard of that status type, try:\n", thing);
    SENDARRAY(sTypeList, 4, thing);
    SendThing("Hint: a status of OFFSITE will delete the entry\n", thing);
    return;
  }

  site = SiteFindExact(buf);
  if (site) { 
    if (StrExact(site->sPlayer->sText, "<SYSTEM>")) {
      SendThing("That is a system defined entry it cannot be changed online\n", thing);
      return;
    }
    if (i == SITE_OFFSITE) {
      sprintf(buf, "^wRemoving Site ^G[^g%s^G]^w", site->sSite);
      SendThing(buf, thing);
      SITEFREE(site);
      SiteWrite();
    } else {
      site->sType = i;
      site->sDate = time(0);
      STRFREE(site->sPlayer);
      site->sPlayer = StrAlloc(sock->sHomeThing->tSDesc);
      sprintf(buf, "^wChanging Site ^G[^g%s^G]^w to ^G[^g%s^G]\n", site->sSite, sTypeList[site->sType]);
      SendThing(buf, thing);
      SiteWrite();
    }
    return;
  }

  if (i == SITE_OFFSITE) {
    SendThing("A Site is OFFSITE by default, so no need to add it...\n", thing);
    return;
  }
  
  site = SiteCreate(sock->sHomeThing, buf, i);
  sprintf(buf, "^wAdding Site ^G[^g%s^G]^w as ^G[^g%s^G]\n", site->sSite, sTypeList[site->sType]);
  SendThing(buf, thing);
  SiteWrite();
}

CMDPROC(CmdSystem) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE  buf[256];
  LWORD objNum = 0;
  LWORD mobNum = 0;
  LWORD wldNum = 0;
  LWORD i;

  for (i=0;i<areaListMax;i++) {
    objNum += areaList[i].aObjIndex.iNum;
    mobNum += areaList[i].aMobIndex.iNum;
    wldNum += areaList[i].aWldIndex.iNum;
  }

  SendThing("^pSystem Stats:\n", thing);
  SendThing("^P-=-=-=-=-=-=\n", thing);
  sprintf(buf, "^bWld structs Loaded:   ^p%ld (%ldK)\n", wldNum, wldNum*sizeof(WLD)/1024);
  SendThing(buf, thing);
  sprintf(buf, "^bMobTemplates Loaded:  ^p%ld (%ldK)\n", mobNum, mobNum*sizeof(MOBTEMPLATE)/1024);
  SendThing(buf, thing);
  sprintf(buf, "^bMobiles Loaded:       ^p%ld (%ldK)\n", mobileIndex.iNum, mobileIndex.iNum*sizeof(MOB)/1024);
  SendThing(buf, thing);
  sprintf(buf, "^bObjTemplates Loaded:  ^p%ld (%ldK)\n", objNum, objNum*sizeof(OBJTEMPLATE)/1024);
  SendThing(buf, thing);
  sprintf(buf, "^bObjects Loaded:       ^p%ld (%ldK)\n", objectIndex.iNum, objectIndex.iNum*sizeof(OBJ)/1024);
  SendThing(buf, thing);
  sprintf(buf, "^bPlayers Loaded:       ^p%ld (%ldK)\n", playerIndex.iNum, playerIndex.iNum*sizeof(PLR)/1024);
  SendThing(buf, thing);
  sprintf(buf, "^bHelp Entries Loaded:  ^p%ld (%ldK)\n", helpIndex.iNum, helpIndex.iNum*sizeof(HELP)/1024);
  SendThing(buf, thing);
  sprintf(buf, "^bExit structs Loaded:  ^p%ld (%ldK)\n", exitNum, exitNum*sizeof(EXIT)/1024);
  SendThing(buf, thing);
  sprintf(buf, "^b# Fighting:           ^p%ld\n", fightIndex.iNum);
  SendThing(buf, thing);
  sprintf(buf, "^bStr MemUsage:         ^p%ldK\n", strMemTotal/1024);
  SendThing(buf, thing);
  sprintf(buf, "^bMalloc MemUsage:      ^p%ldK\n", memUsed/1024);
  SendThing(buf, thing);
  sprintf(buf, "^beventThingIndex:      ^p%ld - should be 1\n", eventThingIndex.iNum);
  SendThing(buf, thing);

/* Should use getrusage here to find out memory consumption */
}

CMDPROC(CmdUsers) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE buf[256];
  BYTE buf2[256];
  SOCK *sock;

  sprintf(buf, 
          "^p%-25s %-4s  %-16s %s\n",
          "User Name",
          "Idle",
          "IP Address",
          "Site");
  SendThing(buf, thing);
  sprintf(buf, 
          "^P%-25s %-4s  %-16s %s\n",
          "-=-=-=-=-",
          "-=-=",
          "-=-=-=-=-=",
          "-=-=");
  SendThing(buf, thing);


  for (sock = sockList; sock; sock=sock->sNext) {
    if (sock->sHomeThing) {
      sprintf(buf2,
              "%s (%hd %c%c%c%c)",
              sock->sHomeThing->tSDesc->sText,
              Character(sock->sHomeThing)->cLevel,
              raceList[Plr(sock->sHomeThing)->pRace].rName[0],
              raceList[Plr(sock->sHomeThing)->pRace].rName[1],
              classList[Plr(sock->sHomeThing)->pClass].cName[0],
              classList[Plr(sock->sHomeThing)->pClass].cName[1]);
      if (sock->sMode != MODE_PLAY) {
        sprintf(buf, 
                "^g%-25s ^b%4ld  %-16s %s <%s>\n",
                buf2,
                Plr(sock->sHomeThing)->pIdleTick,
                sock->sSiteIP,
                sock->sSiteName,
                sModeList[sock->sMode]);
      } else {
        sprintf(buf, 
                "^g%-25s ^b%4ld  %-16s %s\n",
                buf2,
                Plr(sock->sHomeThing)->pIdleTick,
                sock->sSiteIP,
                sock->sSiteName);
      }
    } else {
      sprintf(buf, 
              "^g%-25s ^b%4d  %-16s %s\n",
              "<???>",
              0,
              sock->sSiteIP,
              sock->sSiteName);
    }
    SendThing(buf, thing);
  }
}

CMDPROC(CmdSwitch) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE         srcKey[256];
  LWORD        srcNum;
  LWORD        srcOffset;
  THING       *found;
  SOCK        *sock;

  sock = BaseControlFind(thing);
  if (!sock) return;
  
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (*cmd) {
    cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
    found = ThingFind(srcKey, -1, NULL, TF_MOB_ANYWHERE, &srcOffset);
    if (!found) {
      SendThing("^bThere doesnt appear to be anything like that around\n", thing);
      return;
    }
  } else {
    /* switch back home */
    if (sock->sHomeThing != sock->sControlThing) {
      if (BaseControlFind(sock->sHomeThing)) {
        SendThing("^wYour body is allready occupied\n", thing);
        return;
      }  
      SendThing("^yYou return to yourself\n", thing);
      BaseControlFree(thing, sock); /* turf old control link */
      BaseControlAlloc(sock->sHomeThing, sock); /* control homething */
      return;
    } else {
      SendThing("^wSwitch fine, but switch control to WHOM?!?\n", thing);
      return;
    }
  }

  /* Move on to new body */
  if (BaseControlFind(found)) {
    SendThing("^wThat body is allready occupied\n", thing);
    return;
  }  
  SendThing("^yYou take control of a new body\n", thing);
  BaseControlFree(thing, sock); /* turf old control link */
  BaseControlAlloc(found, sock); /* control homething */
}

CMDPROC(CmdResetSkill) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;
  LWORD  i;
  LWORD  level;
  LWORD  exp;
  LWORD  hp;
  LWORD  mp;
  LWORD  pp;
  LWORD  readPlayer = FALSE;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  search = Base(thing)->bInside;
  found = ThingFind(srcKey, -1, search, TF_PLR_WLD|TF_PLR, &srcOffset);
  if (!found) {
    found = PlayerRead(srcKey, PREAD_NORMAL);
    readPlayer = TRUE;
  }
  if (!found) {
    SendThing("^bThere doesnt appear to be anyone like that around\n", thing);
    return;
  }

  /* tell the god what s/he did */
  SendAction("^rYou ^wResetskill ^r$N\n", thing, found, SEND_SRC|SEND_CAPFIRST);
  SendAction("^r$n ^wResetskill's ^ryou\n", thing, found, SEND_DST|SEND_CAPFIRST);

  /* okay zero out somebodies skills */
  for(i=0; i<skillNum; i++)
    Plr(found)->pSkill[i] = 0;

  /* store some info about them so we can restore it l8r */
  hp = Character(found)->cHitPMax;
  mp = Plr(found)->pMovePMax;
  pp = Plr(found)->pPowerPMax;
  exp = Character(found)->cExp;
  level = Character(found)->cLevel;

  /* init their stuff back to start */
  Character(found)->cLevel = 1;
  Character(found)->cHitPMax = 1;
  Plr(found)->pMovePMax = 1;
  Plr(found)->pPowerPMax = 1;
  Plr(found)->pPractice = 9;
  Plr(found)->pGainPractice = 0;

  while (Character(found)->cLevel<level) {
    PlayerGainLevel(found, FALSE);
    Character(found)->cExp = exp;
  }

  /* reset movep, hitpt, powerp back */  
  Character(found)->cHitPMax = hp;
  Plr(found)->pMovePMax = mp;
  Plr(found)->pPowerPMax = pp;

  if (readPlayer) {
    PlayerWrite(found, PWRITE_PLAYER);
    THINGFREE(found);
  }
}

CMDPROC(CmdShutdown) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  if (STRICMP(cmd, "shutdown")) {
    SendThing("^wAgh no dont even think about it (type SHUTDOWN if you mean it)\n", thing);
    return;
  }

  SendThing("^wYou meanie, now you've done it...\n", thing);
  crimsonRun = FALSE;
  unlink(REBOOT_FLAG_FILE); /* dont autoreboot */
}

CMDPROC(CmdReboot) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  if (STRICMP(cmd, "reboot")) {
    SendThing("^wAgh no dont even think about it (type REBOOT if you mean it)\n", thing);
    return;
  }

  SendThing("^wYou meanie, now you've done it...\n", thing);
  crimsonRun = FALSE;
}
