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
#include "area.h"
#include "world.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "send.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "affect.h"
#include "fight.h"
#include "mobile.h"
#include "player.h"
#include "skill.h"
#include "parse.h"
#include "code.h"
#include "cmd_move.h"

/* Move a single thing through an exit, code the effects of
   deathtraps and move point loss here  */
BYTE MoveExitPrimitive(THING *thing, EXIT *exit) {
  BYTE   buf[256];
  THING *i;
  THING *next;
  THING *from;
  THING *to;
  LWORD  cost;
  SOCK  *sock;
  LWORD  damage; 
  BYTE   eName[256];

  if (!exit || !exit->eWorld) {
    SendThing("^wThere doesnt appear to be anything in that direction.\n", thing);
    return FALSE;
  }
  
  sock = BaseControlFind(thing);
  if (BIT(exit->eFlag, EF_CLOSED) && !ParseCommandCheck(PARSE_COMMAND_WEXIT, sock, "")) {
    if (BIT(exit->eFlag, EF_NOPHASE) || !BIT(Character(thing)->cAffectFlag, AF_PHASEDOOR)) {
      SendThing("^wThat way is closed to you.\n", thing);
      return FALSE;
    }
  }
  
  /* see if we have enough move points left */
  if (Character(thing)->cLevel<LEVEL_GOD && Character(thing)->cLevel>=LEVEL_MOVETIRING) {
    /* determine terrain cost */
    if (BIT(Character(thing)->cAffectFlag, AF_PHASEWALK))
      cost = 1;
    else
      cost = (wTypeList[Wld(Base(thing)->bInside)->wType].wTMoveCost + wTypeList[Wld(exit->eWorld)->wType].wTMoveCost)/2;
    if (thing->tType == TTYPE_PLR) {
      /* Adjust Move costs for encumbrance  */
      cost = CharMoveCostAdjust(thing, cost);
    }

    if (BIT(Character(thing)->cAffectFlag, AF_ENDURANCE))
      cost /= 2;

    /* Check if we can do it */
    if (cost > Character(thing)->cMoveP) {
      SendThing("^wYou're too exhausted to move!\n", thing);
      SendHint("^;HINT: Try ^<SLEEP^;'ing for a little while!\n", thing);
      return FALSE;
    } else {
      Character(thing)->cMoveP -= cost;
    }
  }
  
  if (Character(thing)->cFight)
    FightMove(thing, exit);
 
  if (BIT(Character(thing)->cAffectFlag, AF_HIDDEN)) {
    BITCLR(Character(thing)->cAffectFlag, AF_HIDDEN);
    SendAction("^YYou step out of the shadows\n", 
      thing, NULL, SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^Y$n steps out of the shadows\n", 
      thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  }
 
  /* Send the move messages and move the thing to the next room */
  ExitGetName(exit, eName);
  from = Base(thing)->bInside;
  to = exit->eWorld;

  /* Sneak or normal move? */
  if (!BIT(Character(thing)->cAffectFlag, AF_SNEAK)) {
    if (exit->eDir != EDIR_UNDEFINED) {
      sprintf(buf, "^CYou leave %s.\n", eName);
      SendThing(buf, thing);
      sprintf(buf, "^c$n leaves %s.\n", eName);
      SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    } else {
      sprintf(buf, "^CYou leave via the %s.\n", eName);
      SendThing(buf, thing);
      sprintf(buf, "^c$n leaves via the %s.\n", eName);
      SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    }

    ThingTo(thing, exit->eWorld);                       
    sprintf(buf, "^c$n saunters in.\n");
    SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    CmdLook(thing, "");
  
  } else {
    THING *room;
    LWORD perception;
    LWORD check;
    LWORD sneak;

    if (exit->eDir != EDIR_UNDEFINED) {
      sprintf(buf, "^CYou sneak %s.\n", eName);
      SendThing(buf, thing);
    } else {
      sprintf(buf, "^CYou sneak out via the %s.\n", eName);
      SendThing(buf, thing);
    }
    sneak = CharGetSneak(thing);
    for (room = from->tContain; room; room = room->tNext) {
      if (room->tType < TTYPE_CHARACTER) continue;
      perception = CharGetPerception(room);
      check = DiceOpenEnded(1, 100, 100);

      if (perception + 50 > sneak + check) {
        if (exit->eDir != EDIR_UNDEFINED) {
          sprintf(buf, "^c$n sneaks %s.\n", eName);
          SendAction(buf, thing, room, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
        } else {
          sprintf(buf, "^c$n sneaks out via the %s.\n", eName);
          SendAction(buf, thing, room, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
        }
      }
    }

    ThingTo(thing, to);                       
    for (room = to->tContain; room; room = room->tNext) {
      if (room->tType < TTYPE_CHARACTER) continue;
      perception = CharGetPerception(room);
      check = DiceOpenEnded(1, 100, 100);

      if (perception + 50 > sneak + check) {
        if (exit->eDir != EDIR_UNDEFINED) {
          sprintf(buf, "^c$n sneaks %s.\n", eName);
          SendAction(buf, thing, room, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
        } else {
          sprintf(buf, "^c$n sneaks out via the %s.\n", eName);
          SendAction(buf, thing, room, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
        }
      }
    }
    CmdLook(thing, "");
  
  }

  /* Check for VACUUM here */
  if (Base(thing)->bInside->tType == TTYPE_WLD) {
    if ( (BIT(Wld(Base(thing)->bInside)->wFlag, WF_VACUUM)
        ||Wld(Base(thing)->bInside)->wType==WT_VACUUM)
    && !BIT(Character(thing)->cAffectFlag, AF_VACUUMWALK)){
      damage = Number(1, CharGetHitPMax(thing)/3);
      MINSET(damage, 1);
      /* if its killed by poison whoever its fighting gets the xp */
      SendThing("^rYou're suffering explosive decompression!\n", thing);
      if (FightDamagePrimitive(Character(thing)->cFight, thing, damage)) return FALSE;
    }
  }
  
  /* Check for UNDERWATER here */
  if (Base(thing)->bInside->tType == TTYPE_WLD) {
    if ( Wld(Base(thing)->bInside)->wType==WT_UNDERWATER
     && !BIT(Character(thing)->cAffectFlag, AF_BREATHWATER) ){
      damage = Number(1, CharGetHitPMax(thing)/5);
      MINSET(damage, 1);
      /* if its killed by poison whoever its fighting gets the xp */
      SendThing("^rYou're drowning!\n", thing);
      if (FightDamagePrimitive(Character(thing)->cFight, thing, damage)) return FALSE;
    }
  }
  
  /* Be a bummer if we're dead now.... */
  if (BIT(Wld(to)->wFlag, WF_DEATHTRAP)) {
    if (thing->tType==TTYPE_PLR 
    && (Character(thing)->cLevel>=LEVEL_GOD
      ||(AreaIsEditor(Wld(to)->wArea,thing)==2))) {
    /* They're a god type, dont kill 'em */ 
      SendThing("^wOHNO, a deathtrap... <Whew> lucky you're immune\n", thing);
    } else {
      /* They're toast - log it*/
      if (thing->tType == TTYPE_PLR) {
        Log(LOG_USAGE, thing->tSDesc->sText);
        LogPrintf(LOG_USAGE, " was just killed by a deathtrap! (");
        LogPrintf(LOG_USAGE, to->tSDesc->sText);
        LogPrintf(LOG_USAGE, ")\n");
      }
      /* Salvage some of their stuff */
      for (i=thing->tContain; i; i=next) {
        next = i->tNext;
        if ( (Number(1,100) > 50)
          || (i->tType==TTYPE_OBJ && BIT(Obj(i)->oAct, OA_NODEATHTRAP)) ) {
          ThingTo(i, from);
          SendAction("^g$n is thrown clear of the deathtrap!",
            i, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
          SendAction("^g$n is thrown clear of the deathtrap!",
            thing, NULL, SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST);
        }
      }
      
      /* tell them they're dead */
      SendThing(fileList[FILE_DEATH].fileStr->sText, thing);
      
      /* This should be a subroutine I think, PlayerKill() or somesuch */
      /* This should now work for switching */
      if (sock) {
        LWORD bank;

        if ((thing->tType==TTYPE_MOB)&&(Mob(thing)->mTemplate==spiritTemplate))
          BaseControlFree(thing,sock);
        sprintf(buf, "%s", thing->tSDesc->sText);
        if (sock->sHomeThing==thing) {
          bank = Plr(thing)->pBank;
          ThingFree(thing); /* will stop fighting automaticly */
          sock->sHomeThing = Thing(PlayerRead(buf, PREAD_CLONE));
          ThingTo(sock->sHomeThing, WorldOf(playerStartRoom));
          Plr(sock->sHomeThing)->pBank = bank;
        } else {
          ThingFree(thing); /* will stop fighting automaticly */
        }
        BaseControlAlloc(sock->sHomeThing, sock); /* make new pointer */
        CmdLook(sock->sHomeThing, "");
      } else {
        /* they're dead and not connected sign them off */
        ThingFree(thing); /* will stop fighting automaticly */
      }
    }
    return FALSE;
  }
  
  
  /* Scan for hostiles - autoattack other aggressives */
  if (thing->tType==TTYPE_PLR && BIT(Plr(thing)->pAuto, PA_AUTOAGGR)) {
    for (i=Base(thing)->bInside; i; i=i->tNext) {
      if (i->tType==TTYPE_MOB 
    && i!=thing 
    && BITANY(Mob(i)->mTemplate->mAct, MACT_AGGRESSIVE|MACT_HYPERAGGR))
        FightStart(thing, i, 1, NULL);
    }
  }
  
  thing->tWait++; /* make it so they cant initiate a command for a "round" */
  
  return TRUE;
}

/* Needed another layer for script event blocking */
BYTE MoveExitEnterStub(THING *thing, EXIT *exit) {
  THING  *i;
  BYTE    retValue = FALSE;
  BYTE    block = FALSE;
  THING  *from;
  THING  *to;
  
  if (!thing || !exit) return FALSE;

  /* Worlds are static, no worries about storing these pointers */
  from = Base(thing)->bInside;
  to = exit->eWorld;

  ThingSetEvent(thing);
  ThingSetEvent(exit);

  /**************
   * @ENTRY event 
   **************/
  /* check the room for a script */
  block = CodeParseEntry(thing, exit->eWorld, exit);
  /* check other things within the room for scripts */
  for (i=to->tContain; i && !block; i=i->tNext) 
    BITSET(i->tFlag, TF_CODETHING);
  while ( (i = ThingFindFlag(to->tContain, TF_CODETHING)) ) {
    BITCLR(i->tFlag, TF_CODETHING);
    if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
      block = TRUE; /* Nothing left to do */
    if (!block) block = CodeParseEntry(thing, i, exit);
  }
  /* see if area wide script intercepts */
  if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
    block = TRUE; /* Nothing left to do */
  if (!block)
    block = CodeParseEntry(thing, &areaList[Wld(to)->wArea].aResetThing, exit);

  /***********************
   * Perform the Movement 
   ***********************/
  if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
    block = TRUE; /* Nothing left to do */
  if (!block) retValue = MoveExitPrimitive(thing, exit);


  /**************
   * @AFTERENTRY event 
   **************/
  if (retValue && !block) {
    /* Check the room they are in */
    if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
      block = TRUE; /* Nothing left to do */
    if (!block) CodeParseAfterEntry(thing, to, exit);
    
    /* check for people in the room */
    for (i=to->tContain; i && !block; i=i->tNext) 
      BITSET(i->tFlag, TF_CODETHING);
    while ( (i = ThingFindFlag(to->tContain, TF_CODETHING)) ) {
      BITCLR(i->tFlag, TF_CODETHING);
      if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
        block = TRUE; /* Nothing left to do */
      if (!block) CodeParseAfterEntry(thing, i, exit);
    }
  
    /* see if area wide script intercepts */
    if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
      block = TRUE; /* Nothing left to do */
    if (!block)
      CodeParseAfterEntry(thing, &areaList[Wld(to)->wArea].aResetThing, exit);
  } 

  if (ThingIsEvent(thing)) {
    if (block) retValue = (Base(thing)->bInside == to);
    ThingDeleteEvent(thing);
  }
  ThingDeleteEvent(exit);
  return retValue;
}


/* ultimately calls MoveExitPrimitive 
   Entry point to move a single person, notably
   used by the flee command */ 
BYTE MoveExit(THING *thing, EXIT *exit) {
  THING  *i;
  BYTE    retValue = FALSE;
  BYTE    block = FALSE;
  THING  *from;
  THING  *to;
  
  if (!thing || !exit) return FALSE;
  if (thing->tType < TTYPE_BASE) return FALSE;
  if (!Base(thing)->bInside) return FALSE;
  if (Base(thing)->bInside->tType!=TTYPE_WLD) return FALSE;
  if (!exit->eWorld) return FALSE;

  /* Worlds are static, no worries about storing these pointers */
  from = Base(thing)->bInside;
  to = exit->eWorld;

  /* Compile on demand */
  if (to->tType == TTYPE_WLD) {
    PROPERTY *p;
    if (BIT(to->tFlag, TF_COMPILE)) {
      BITCLR(to->tFlag, TF_COMPILE);
      for (p=to->tProperty; p; p=p->pNext) {
        if (!CodeCompileProperty(p, NULL))
          CodeSetFlag(to, p);
      }
    }
  }

  ThingSetEvent(thing);
  ThingSetEvent(exit);
    
  /**************
   * @EXIT event 
   **************/
  /* check the room for a script */
  block = CodeParseExit(thing, Base(thing)->bInside, exit);
  /* check other things within the room for scripts */
  for (i=from->tContain; i && !block; i=i->tNext) 
    BITSET(i->tFlag, TF_CODETHING);
  while ( (i = ThingFindFlag(from->tContain, TF_CODETHING)) ) {
    BITCLR(i->tFlag, TF_CODETHING);
    if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
      block = TRUE; /* Nothing left to do */
    if (!block)
      block = CodeParseExit(thing, i, exit);
  }
  /* see if area wide script intercepts */
  if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
    block = TRUE; /* Nothing left to do */
  if (!block)
    block = CodeParseExit(thing, &areaList[Wld(from)->wArea].aResetThing, exit);


  /********************************
   *  Check out the Entry events 
   ********************************/
  if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
    block = TRUE; /* Nothing left to do */
  if (!block) retValue = MoveExitEnterStub(thing, exit);

  
  /**************
   * @AFTEREXIT event 
   **************/
  if (retValue && !block) {
    /* Check the room they are in */
    if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
      block = TRUE; /* Nothing left to do */
    if (!block) CodeParseAfterExit(thing, from, exit);
    
    /* check for people in the room */
    for (i=from->tContain; i&&!block; i=i->tNext) 
      BITSET(i->tFlag, TF_CODETHING);
    while ( (i = ThingFindFlag(from->tContain, TF_CODETHING)) ) {
      BITCLR(i->tFlag, TF_CODETHING);
      if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
        block = TRUE; /* Nothing left to do */
      if (!block) CodeParseAfterExit(thing, i, exit);
    }
  
    /* see if area wide script intercepts */
    if (!block && (!ThingIsEvent(thing) || !ThingIsEvent(exit)))
      block = TRUE; /* Nothing left to do */
    if (!block)
      CodeParseAfterExit(thing, &areaList[Wld(from)->wArea].aResetThing, exit);
  } 

  if (ThingIsEvent(thing)) {
    if (block) retValue = (Base(thing)->bInside == to);
    ThingDeleteEvent(thing);
  }
  ThingDeleteEvent(exit);
  return retValue;
}

/* ****************************************************
 * If you want to move somebody through an exit, this is what
 * you should be calling.

 * - stops if somebody cant move through the exit
 * - Entry point for moving entire groups, blocks
 * if they are busy fighting someone
 * - scripts are checked in ROOM, OBJECTS IN ROOM, 
 * then AREAWIDE order
 * - parms are verified just before each check and 
 * if thing or exit has ceased to exist the whole 
 * movement process is aborted
 
 * The moving process....
 * MoveExitParse {       (<-- Call this function to move something)
 *   Check @EXIT
 *   MoveExitEnterStub {
 *     Check @ENTRY
 *     MovePrimitive     ( does the move, uses mv pts up etc )
 *     Check @AFTERENTRY
 *   }
 *   Check @AFTEREXIT
 * }
 
 */
void MoveExitParse(THING *thing, EXIT *exit) {
  THING  *i;
  THING  *wasIn;
  BYTE    possibleSuccess;
  BYTE    eName[256];
  
  if (!thing || !exit) return;
  if (thing->tType < TTYPE_BASE) return;
  if (!Base(thing)->bInside) return;
  if (!exit->eWorld) return;
  
  if (Character(thing)->cPos==POS_FIGHTING) {
    if (!Character(thing)->cFightExit) {
      SendThing("^cYou're right in the middle of a fight here! If you want to leave ^wFLEE!\n", thing);
      return;
    } else {
      if (exit != Character(thing)->cFightExit) {
        ExitGetName(exit, eName);
        SendThing("^cYou're in the middle of a fight here! Either go ^p", thing);
        SendThing(eName, thing);
        SendThing(" or ^wFLEE!\n", thing);
        return;
      }
    }
  }
  
  /* Move everybody else in the group if they can move */
  wasIn = Base(thing)->bInside;
  possibleSuccess = MoveExit(thing, exit);
  i = Character(thing)->cFollow;
  while (possibleSuccess && i) {
    if (Base(i)->bInside == wasIn) {
      /* Block out fighting chars from moving */
      if (Character(i)->cPos<POS_STANDING) {
        if (Character(i)->cPos==POS_FIGHTING
         && Character(i)->cFightExit == exit) {
          possibleSuccess = MoveExit(i, exit);
        } else {
          possibleSuccess = FALSE;
        }
      } else {
        possibleSuccess = MoveExit(i, exit);
      }
    }
    i = Character(i)->cFollow;
  }
}

void MoveDir(THING *thing, BYTE dir) {
  EXIT *exit;
  
  if (Base(thing)->bInside->tType != TTYPE_WLD) {
    SendThing("There doesnt appear to be anything in that direction.\n", thing);
    return;
  }
  exit = ExitDir(Wld(Base(thing)->bInside)->wExit, dir);
  if (!exit) {
    SendThing("There doesnt appear to be anything in that direction.\n", thing);
    return;
  }
  MoveExitParse(thing, exit);
}

CMDPROC(CmdGo) { /* void CmdProc(THING *thing, BYTE *cmd) */
  EXIT *exit;
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  
  cmd = StrOneWord(cmd, NULL); /* drop command */
  
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  if (Base(thing)->bInside->tType != TTYPE_WLD) {
    SendThing("There doesnt appear to be anything in that direction.\n", thing);
    return;
  }
  exit = ExitFind(Wld(Base(thing)->bInside)->wExit, srcKey);
  if (exit && srcOffset>0) {
    exit = NULL;
    srcOffset--;
  }
  if (!exit) {
    /* Look for an exit object */
    found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
    if (found) {
      if (Obj(found)->oTemplate->oType == OTYPE_EXIT
        ||Obj(found)->oTemplate->oType == OTYPE_VEHICLE
      ) {
        LWORD  wVirtual;
        THING *wld;
        wVirtual = OBJECTGETFIELD(found, OF_EXIT_WVIRTUAL);
        wld = WorldOf(wVirtual);
        if (!wld) {
          SendThing("That doesnt seem to go anywhere.\n", thing);
          return;
        }
        /* make an exit, move thru it than turf it */
        exit = ExitAlloc(NULL, EDIR_UNDEFINED, StrAlloc(Base(found)->bKey), StrAlloc(found->tDesc), 0, -1, wld);        
        MoveExitParse(thing, exit);
        ExitFree(NULL, exit);

#ifdef UGLY_VEHICLE
      } else if (Obj(found)->oTemplate->oType == OTYPE_VEHICLE) {
        THING *wld;
        wld = (THING*) Obj(found)->oDetail.lValue[3];
        if (!wld) {
          SendThing("That doesnt seem to go anywhere.\n", thing);
          return;
        }
        /* make an exit, move thru it than turf it */
        exit = ExitAlloc(NULL, EDIR_UNDEFINED, StrAlloc(Base(found)->bKey), StrAlloc(found->tDesc), 0, -1, wld);        
        MoveExitParse(thing, exit);
        ExitFree(NULL, exit);
#endif

      } else {
        SendThing("How exactly would that work now?.\n", thing);
      }
    } else {
      SendThing("There doesnt appear to be anything in that direction.\n", thing);
    }
    return;
  }
  MoveExitParse(thing, exit);
}

CMDPROC(CmdNorth) { /* void CmdProc(THING thing, BYTE* cmd) */
  MoveDir(thing, EDIR_NORTH);
}

CMDPROC(CmdEast) { /* void CmdProc(THING thing, BYTE* cmd) */
  MoveDir(thing, EDIR_EAST);
}

CMDPROC(CmdSouth) { /* void CmdProc(THING thing, BYTE* cmd) */
  MoveDir(thing, EDIR_SOUTH);
}

CMDPROC(CmdWest) { /* void CmdProc(THING thing, BYTE* cmd) */
  MoveDir(thing, EDIR_WEST);
}

CMDPROC(CmdUp) { /* void CmdProc(THING thing, BYTE* cmd) */
  MoveDir(thing, EDIR_UP);
}

CMDPROC(CmdDown) { /* void CmdProc(THING thing, BYTE* cmd) */
  MoveDir(thing, EDIR_DOWN);
}

CMDPROC(CmdOut) { /* void CmdProc(THING thing, BYTE* cmd) */
  MoveDir(thing, EDIR_OUT);
}

CMDPROC(CmdSneak) { /* void CmdProc(THING thing, BYTE* cmd) */
  BYTE *next;
  LWORD sneak;

  if (CharGetSneak(thing) <= 0) {
    SendThing("^wI'm afraid you're not too good at that sort of thing\n", thing);
    return;
  }

  next = StrOneWord(cmd, NULL);
  if (!*next) {
    /* toggle sneak */
    if (BIT(Character(thing)->cAffectFlag, AF_SNEAK)) {
      SendThing("^wYou resume walking around normally\n", thing);
    } else {
      SendThing("^wYou start sneaking around\n", thing);
    }
    BITFLIP(Character(thing)->cAffectFlag, AF_SNEAK);
    return;
  }

  sneak = FALSE;
  if (BIT(Character(thing)->cAffectFlag, AF_SNEAK))
    sneak = TRUE;
  BITSET(Character(thing)->cAffectFlag, AF_SNEAK);
  CmdGo(thing, cmd);
  if (!sneak) BITCLR(Character(thing)->cAffectFlag, AF_SNEAK);
}

CMDPROC(CmdLook) { /* CmdLook(thing,cmd) */
  SOCK  *sock;
  BYTE   buf[256];
  BYTE   srcKey[256];
  BYTE   eName[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *search;
  THING *show;
  EXTRA *extra;
  EXIT  *exit;
  BYTE   showDesc = TRUE;
  BYTE   canSee;
  LWORD  eOrderNum;

  sock = BaseControlFind(thing);
  if (!sock) return;
  search = Base(thing)->bInside;
  cmd = StrOneWord(cmd, buf); /* lose the command at the start */
  if (!*cmd) {
    canSee = ThingCanSee(thing, NULL);
    /* Show the room */
    SendThing("^0",thing);
    SendThing(search->tSDesc->sText, thing);
    if (*buf == '\0' && !BIT(Plr(sock->sHomeThing)->pAuto, PA_AUTOLOOK)) {
      SendThing("\n",thing);
    } else if (canSee==TCS_SEENORMAL) {
      SendThing("\n^1",thing);
      SendThing(search->tDesc->sText, thing);
    } else
      SendThing("\n",thing);

    /* See if its too dark */
    if (canSee == TCS_CANTSEE)
      SendThing("^yIts pitch black\n", thing);
    else if (canSee == TCS_SEEPARTIAL)
      SendThing("^yIts so dark you can only dimly make things out\n", thing);

    /* show the Exits */
    if (BIT(Plr(sock->sHomeThing)->pAuto, PA_AUTOEXIT)) {
      sprintf(buf, "^cVisible Exits: [");
      /* ok, now we could just go through the exit list, but it 
       * would be nice to have a SORTED list. */
      for (eOrderNum=0;eOrderList[eOrderNum]<EDIR_MAX;eOrderNum++) {
        for (exit = Wld(search)->wExit; exit; exit=exit->eNext) {
          if (exit->eWorld && !BIT(exit->eFlag, EF_HIDDEN)&&
	      (exit->eDir==eOrderList[eOrderNum])) {
            ExitGetName(exit, eName);
            if (BIT(exit->eFlag, EF_CLOSED))
              sprintf(buf+strlen(buf), "^r%s ", eName);
            else
              sprintf(buf+strlen(buf), "^b%s ", eName);
          }
        }
      }
      if (buf[strlen(buf)-1] == ' ')
        buf[strlen(buf)-1] = '\0';
      strcat(buf, "^c]\n");
      SendThing(buf, thing);
    }
    
    /* show the things in the room */
    show = search->tContain;
    while(show) {
      if (show != thing) {
        show = ThingShow(show, thing);
      } else
        show = show->tNext;
    }
    return;
  }
  
  StrOneWord(cmd, buf); /* see if they tried to look in or inside something */
  if (!STRICMP(buf, "in") || !STRICMP(buf, "inside")) {
    cmd = StrOneWord(cmd, NULL);
    showDesc = FALSE;
  }
  
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  show = CharThingFind(thing, srcKey, -1, thing, TF_PLR|TF_OBJ|TF_MOB, &srcOffset);
  if (!show)
    show = CharThingFind(thing, srcKey, -1, search, TF_PLR|TF_OBJ|TF_MOB, &srcOffset);
  if (show) {
    ThingShowDetail(show, thing, showDesc);
    return;
  }
  /* Look extras */
  extra = ExtraFindWithin(search, srcKey, &srcOffset);
  if (extra && srcOffset<=0) {
    SendThing("^b", thing);
    SendThing(extra->eDesc->sText, thing);
    return;
  }
  
  /* Look Exits */
  do {
    exit = ExitFind(Wld(search)->wExit, srcKey);
    if (exit) srcOffset--;
  } while (exit && srcOffset>0);
  if (exit && srcOffset<=0) {
    SendThing("^b", thing);
    if (*exit->eDesc->sText)
      SendThing(exit->eDesc->sText, thing);
    else
      SendThing("You see nothing special\n", thing);
    return;
  }
  
  /* well guess theres nothing else to look for */
  SendThing("^bThere doesnt appear to be anything like that around\n", thing);
}

/* scan adjacent rooms */
BYTE  *scanAdjective[] = {
  "",
  "Far to the ",
  "Way Far "
};

CMDPROC(CmdScan) { /* CmdProc(thing,cmd) */
  BYTE   buf[256];
  BYTE   word[256];
  BYTE   eName[256];
  EXIT  *exit;
  THING *scan;
  LWORD  i;
  
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    /* show the Exits */
    SendThing("^wScanning all around..\n", thing);
    for (exit = Wld(Base(thing)->bInside)->wExit; exit; exit=exit->eNext) {
      if (exit->eWorld && !BIT(exit->eFlag, EF_HIDDEN)) {
        ExitGetName(exit, eName);
        sprintf(buf, "^b[%s]\n", eName);
        SendThing(buf, thing);
        for (scan = exit->eWorld->tContain; scan; ) {
          scan = ThingShow(scan, thing);
        }
        SendThing("\n", thing);
      }
    }
    return;
  }
  
  /* Scan in a particular direction here */
  cmd = StrOneWord(cmd, word); /* Get the keyword */
  sprintf(buf, "^wScanning %s...\n", word);
  SendThing(buf, thing);
  scan = Base(thing)->bInside;
  for (i=0; i<3; i++) { /* scan this many rooms away */
    exit = ExitFind(Wld(scan)->wExit, word);
    if (!exit) return;
    ExitGetName(exit, eName);
    sprintf(buf, "^b[%s%s]\n", scanAdjective[i], eName);
    SendThing(buf, thing);
    for (scan = exit->eWorld->tContain; scan; ) {
      scan = ThingShow(scan, thing);
    }
    SendThing("\n", thing);
    scan = exit->eWorld;
  }
}

CMDPROC(CmdExit) { /* CmdProc(thing,cmd) */
  EXIT  *exit;
  SOCK  *sock;
  BYTE   buf[256];
  BYTE   eName[256];

  sock = BaseControlFind(thing);

  /* Show the obvious exits */
  SendThing("^bAmong the obvious exits, include such diverse elements as:\n",thing);

  /* show the Exits */
  for (exit = Wld(Base(thing)->bInside)->wExit; exit; exit=exit->eNext) {
    if (!BIT(exit->eFlag, EF_HIDDEN)) {
      ExitGetName(exit, eName);
      sprintf(buf, "^g%-7s- ^c", eName);
      SendThing(buf, thing);
      if (exit->eDesc->sLen>1)
        SendThing(exit->eDesc->sText, thing);
      else if (exit->eWorld) {
        SendThing(exit->eWorld->tSDesc->sText, thing);
        SendThing("\n", thing);
      }
    }
  }

}

CMDPROC(CmdOpen) { /* CmdProc(thing,cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *search;
  THING *open;
  EXIT  *exit;
  FLAG   oFlag;
  BYTE   word[256];
  BYTE   buf[256];

  cmd = StrOneWord(cmd, NULL); /* lose command at start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  /* search ourself first */
  search = thing;
  open = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!open) { /* also search the room */
    search = Base(thing)->bInside;
    open = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  }
  
  if (open) {
    if (Obj(open)->oTemplate->oType == OTYPE_CONTAINER) {
      oFlag = OBJECTGETFIELD(open, OF_CONTAINER_CFLAG);
      if (!BIT(oFlag, OCF_CLOSED)) {
        SendThing("^wBoy, how difficult that's going to be, since its allready open....\n", thing);
        return;
      }
      if (!BIT(oFlag, OCF_LOCKED)) {
        BITCLR(oFlag, OCF_CLOSED);
        ObjectSetField(OTYPE_CONTAINER, &Obj(open)->oDetail, OF_CONTAINER_CFLAG, oFlag);
        SendThing("^gYou open ", thing);
        SendThing(open->tSDesc->sText, thing);
        SendThing("\n", thing);
        SendAction("$n opens $N\n", thing, open, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      } else {
        SendThing("^gYou cant open that, its LOCKED!\n", thing);
        return;
      }
    } else {
      SendThing("^wAnd how on earth are you supposed to do that, exactly?\n", thing);
      return;
    }
  }
  
  /* search for Exits */
  do {
    exit = ExitFind(Wld(search)->wExit, srcKey);
    if (exit) srcOffset--;
  } while (exit && srcOffset>0);
  if (exit && srcOffset<=0) {
    if (!BIT(exit->eFlag, EF_ISDOOR)) {
      SendThing("^wYou cant open ^rTHAT\n", thing);
      return;
    }
    if (!BIT(exit->eFlag, EF_CLOSED)) {
      SendThing("^wBoy, how difficult that's going to be, since its allready open....\n", thing);
      return;
    }
    if (BIT(exit->eFlag, EF_LOCKED)) {
      SendThing("^wIt's locked.\n", thing);
      return;
    }
    if (*exit->eKey->sText)
      StrOneWord(exit->eKey->sText, word);
    else
      strcpy(word, "door");
    sprintf(buf, "^gYou open the %s\n", word);
    SendThing(buf, thing);
    sprintf(buf, "^g$n opens the %s\n", word);
    SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  
    BITCLR(exit->eFlag, EF_CLOSED);
    exit = ExitReverse(search, exit);
    if (exit)
      BITCLR(exit->eFlag, EF_CLOSED);
    return;
  }
  
  /* well guess theres nothing else to open */
  SendThing("^bThere doesnt appear to be anything like that around\n", thing);
}

CMDPROC(CmdClose) { /* CmdProc(thing,cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *search;
  THING *close;
  EXIT  *exit;
  FLAG   oFlag;
  BYTE   word[256];
  BYTE   buf[256];

  cmd = StrOneWord(cmd, NULL); /* lose command at start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  /* search ourself first */
  search = thing;
  close = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!close) { /* also search the room */
    search = Base(thing)->bInside;
    close = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  }
  
  if (close) {
    if (Obj(close)->oTemplate->oType == OTYPE_CONTAINER) {
      oFlag = OBJECTGETFIELD(close, OF_CONTAINER_CFLAG);
      if (!BIT(oFlag, OCF_CLOSABLE)) {
        SendThing("^wYou cant close ^rTHAT\n", thing);
        return;
      }
      if (BIT(oFlag, OCF_CLOSED)) {
        SendThing("^wBoy, how difficult that's going to be, since its allready closed...\n", thing);
        return;
      } else {
        BITSET(oFlag, OCF_CLOSED);
        ObjectSetField(OTYPE_CONTAINER, &Obj(close)->oDetail, OF_CONTAINER_CFLAG, oFlag);
        SendThing("^gYou close ", thing);
        SendThing(close->tSDesc->sText, thing);
        SendThing("\n", thing);
        SendAction("$n closes $N\n", thing, close, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      }
    } else {
      SendThing("^wAnd how on earth are you supposed to do that, exactly?\n", thing);
      return;
    }
  }

  /* search for Exits */
  do {
    exit = ExitFind(Wld(search)->wExit, srcKey);
    if (exit) srcOffset--;
  } while (exit && srcOffset>0);
  if (exit && srcOffset<=0) {
    if (!BIT(exit->eFlag, EF_ISDOOR)) {
      SendThing("^wYou cant close ^rTHAT\n", thing);
      return;
    }
    if (BIT(exit->eFlag, EF_CLOSED)) {
      SendThing("^wBoy, how difficult that's going to be, since its allready closed....\n", thing);
      return;
    } else {
      if (*exit->eKey->sText)
        StrOneWord(exit->eKey->sText, word);
      else
        strcpy(word, "door");
      sprintf(buf, "^gYou close the %s\n", word);
      SendThing(buf, thing);
      sprintf(buf, "^g$n closes the %s\n", word);
      SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);

      BITSET(exit->eFlag, EF_CLOSED);
      exit = ExitReverse(search, exit);
      if (exit)
        BITSET(exit->eFlag, EF_CLOSED);
      return;
    }
  }

  /* well guess theres nothing else to close */
  SendThing("^bThere doesnt appear to be anything like that around\n", thing);
}

CMDPROC(CmdUnlock) { /* CmdProc(thing,cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *search;
  THING *unlock;
  THING *key;
  LWORD  unlockNumber;
  EXIT  *exit;
  FLAG   oFlag;
  BYTE   word[256];
  BYTE   buf[256];
  
  cmd = StrOneWord(cmd, NULL); /* lose command at start */
  search = Base(thing)->bInside;
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  unlock = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  if (unlock) {
    if (Obj(unlock)->oTemplate->oType == OTYPE_CONTAINER) {
      oFlag = OBJECTGETFIELD(unlock, OF_CONTAINER_CFLAG);
      if (!BIT(oFlag, OCF_LOCKED)) {
        SendThing("^wBoy, how difficult that's going to be, since its allready unlocked....\n", thing);
        return;
      }
      
      /* find if we have the key */
      unlockNumber = OBJECTGETFIELD(unlock, OF_CONTAINER_KEY);
      for(key = thing->tContain; key; key = key->tNext) {
        if (key->tType == TTYPE_OBJ && Obj(key)->oTemplate->oType == OTYPE_KEY) {
          if (Obj(key)->oTemplate->oVirtual == unlockNumber)
            break;
          if (OBJECTGETFIELD(key, OF_KEY_NUMBER) == unlockNumber)
            break;
        }
      }
      
      if (key) {
        BITCLR(oFlag, OCF_LOCKED);
        ObjectSetField(OTYPE_CONTAINER, &Obj(unlock)->oDetail, OF_CONTAINER_CFLAG, oFlag);
        SendThing("^gYou unlock ", thing);
        SendThing(unlock->tSDesc->sText, thing);
        SendThing("\n", thing);
        SendAction("$n unlocks $N\n", thing, unlock, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      } else {
        SendThing("^wI'm afraid you dont have anything that you can use to unlock it\n", thing);
        return;
      }
    } else {
      SendThing("^wAnd how on earth are you supposed to do that, exactly?\n", thing);
      return;
    }
  }
  
  /* search for Exits */
  do {
    exit = ExitFind(Wld(search)->wExit, srcKey);
    if (exit) srcOffset--;
  } while (exit && srcOffset>0);
  if (exit && srcOffset<=0) {
    if (!BIT(exit->eFlag, EF_ISDOOR)) {
      SendThing("^wYou cant unlock ^rTHAT!\n", thing);
      return;
    }
    
    if (!BIT(exit->eFlag, EF_LOCKED)) {
      SendThing("^wBoy, how difficult that's going to be, since its allready unlocked....\n", thing);
      return;
    }
    
    /* Do we have the key */
    unlockNumber = exit->eKeyObj;
    for(key = thing->tContain; key; key = key->tNext) {
      if (key->tType == TTYPE_OBJ && Obj(key)->oTemplate->oType == OTYPE_KEY) {
        if (Obj(key)->oTemplate->oVirtual == unlockNumber)
          break;
        if (OBJECTGETFIELD(key, OF_KEY_NUMBER) == unlockNumber)
          break;
      }
    }
    
    if (key) {
      if (*exit->eKey->sText)
        StrOneWord(exit->eKey->sText, word);
      else
        strcpy(word, "door");
      sprintf(buf, "^gYou unlock %s\n", word);
      SendThing(buf, thing);
      sprintf(buf, "^g$n unlocks %s\n", word);
      SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      
      BITCLR(exit->eFlag, EF_LOCKED);
      exit = ExitReverse(search, exit);
      if (exit)
        BITCLR(exit->eFlag, EF_LOCKED);
      return;
    } else {
      SendThing("^wI'm afraid you dont have anything that you can use to unlock it\n", thing);
      return;
    }
  }
  
  /* well guess theres nothing else to unlock */
  SendThing("^bThere doesnt appear to be anything like that around\n", thing);
}


CMDPROC(CmdLock) { /* CmdProc(thing,cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *search;
  THING *lock;
  THING *key;
  LWORD  lockNumber;
  EXIT  *exit;
  FLAG   oFlag;
  BYTE   word[256];
  BYTE   buf[256];
  
  cmd = StrOneWord(cmd, NULL); /* lose command at start */
  search = Base(thing)->bInside;
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  lock = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  if (lock) {
    if (Obj(lock)->oTemplate->oType == OTYPE_CONTAINER) {
      oFlag = OBJECTGETFIELD(lock, OF_CONTAINER_CFLAG);
      if (BIT(oFlag, OCF_LOCKED)) {
        SendThing("^wBoy, how difficult that's going to be, since its allready locked....\n", thing);
        return;
      }
      if (!BIT(oFlag, OCF_CLOSED)) {
        SendThing("^wIt has to be closed before you can lock it\n", thing);
        return;
      }
      
      /* find if we have the key */
      lockNumber = OBJECTGETFIELD(lock, OF_CONTAINER_KEY);
      for(key = thing->tContain; key; key = key->tNext) {
        if (key->tType == TTYPE_OBJ && Obj(key)->oTemplate->oType == OTYPE_KEY) {
          if (Obj(key)->oTemplate->oVirtual == lockNumber)
            break;
          if (OBJECTGETFIELD(key, OF_KEY_NUMBER) == lockNumber)
            break;
        }
      }
      
      if (key) {
        BITSET(oFlag, OCF_LOCKED);
        ObjectSetField(OTYPE_CONTAINER, &Obj(lock)->oDetail, OF_CONTAINER_CFLAG, oFlag);
        SendThing("^gYou lock ", thing);
        SendThing(lock->tSDesc->sText, thing);
        SendThing("\n", thing);
        SendAction("$n locks $N\n", thing, lock, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      } else {
        SendThing("^wI'm afraid you dont have anything that you can use to lock it\n", thing);
        return;
      }
    } else {
      SendThing("^wAnd how on earth are you supposed to do that, exactly?\n", thing);
      return;
    }
  }
  
  /* search for Exits */
  do {
    exit = ExitFind(Wld(search)->wExit, srcKey);
    if (exit) srcOffset--;
  } while (exit && srcOffset>0);
  if (exit && srcOffset<=0) {
    if (!BIT(exit->eFlag, EF_ISDOOR)) {
      SendThing("^wYou cant lock ^rTHAT!\n", thing);
      return;
    }
    if (BIT(exit->eFlag, EF_LOCKED)) {
      SendThing("^wBoy, how difficult that's going to be, since its allready locked....\n", thing);
      return;
    }
    if (!BIT(exit->eFlag, EF_CLOSED)) {
      SendThing("^wIt has to be closed before it can be locked\n", thing);
      return;
    }

    /* Do we have the key */
    lockNumber = exit->eKeyObj;
    for(key = thing->tContain; key; key = key->tNext) {
      if (key->tType == TTYPE_OBJ && Obj(key)->oTemplate->oType == OTYPE_KEY) {
        if (Obj(key)->oTemplate->oVirtual == lockNumber)
          break;
        if (OBJECTGETFIELD(key, OF_KEY_NUMBER) == lockNumber)
          break;
      }
    }
    
    if (key) {
      if (*exit->eKey->sText)
        StrOneWord(exit->eKey->sText, word);
      else
        strcpy(word, "door");
      sprintf(buf, "^gYou lock %s\n", word);
      SendThing(buf, thing);
      sprintf(buf, "^g$n locks %s\n", word);
      SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);

      BITSET(exit->eFlag, EF_LOCKED);
      exit = ExitReverse(search, exit);
      if (exit)
        BITSET(exit->eFlag, EF_LOCKED);
      return;
    } else {
      SendThing("^wI'm afraid you dont have anything that you can use to lock it\n", thing);
      return;
    }
  }
  
  /* well guess theres nothing else to lock */
  SendThing("^bThere doesnt appear to be anything like that around\n", thing);
}


CMDPROC(CmdPicklock) { /* CmdProc(thing,cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *search;
  THING *unlock;
  BYTE   success;
  LWORD  unlockNumber;
  EXIT  *exit;
  FLAG   oFlag;
  BYTE   word[256];
  BYTE   buf[256];

  thing->tWait++;
  cmd = StrOneWord(cmd, NULL); /* lose command at start */
  search = Base(thing)->bInside;
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  unlock = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  if (unlock) {
    if (Obj(unlock)->oTemplate->oType == OTYPE_CONTAINER) {
      oFlag = OBJECTGETFIELD(unlock, OF_CONTAINER_CFLAG);
      if (!BIT(oFlag, OCF_LOCKED)) {
        SendThing("^wBoy, how difficult that's going to be, since its allready unlocked....\n", thing);
        return;
      }
      
      if (BIT(oFlag, OCF_ELECTRONIC)) {
        SendThing("^wYou cant pick an electronic lock\n", thing);
        return;
      }
      
      /* find if we picked the lock */
      success = FALSE;
      if (thing->tType == TTYPE_PLR && Plr(thing)->pSkill[SKILL_PICKLOCK]>0) {
        unlockNumber = DiceOpenEnded(1, 100, 100);
        unlockNumber += PropertyGetLWord(unlock, "%security", 0);
        if (unlockNumber <= Plr(thing)->pSkill[SKILL_PICKLOCK])
          success = TRUE;
      }
      
      if (success) {
        BITCLR(oFlag, OCF_LOCKED);
        ObjectSetField(OTYPE_CONTAINER, &Obj(unlock)->oDetail, OF_CONTAINER_CFLAG, oFlag);
        SendThing("^gYou pick the lock of the ", thing);
        SendThing(unlock->tSDesc->sText, thing);
        SendThing("\n", thing);
        SendAction("$n picks the lock on the $N\n", thing, unlock, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      } else {
        SendThing("^wI'm afraid you fail to pick the lock\n", thing);
        return;
      }
    } else {
      SendThing("^wPicklock what exactly?\n", thing);
      return;
    }
  }
  
  /* search for Exits */
  do {
    exit = ExitFind(Wld(search)->wExit, srcKey);
    if (exit) srcOffset--;
  } while (exit && srcOffset>0);
  if (exit && srcOffset<=0) {
    if (!BIT(exit->eFlag, EF_ISDOOR)) {
      SendThing("^wYou cant unlock ^rTHAT!\n", thing);
      return;
    }
    
    if (!BIT(exit->eFlag, EF_LOCKED)) {
      SendThing("^wBoy, how difficult that's going to be, since its allready unlocked....\n", thing);
      return;
    }
    
    if (BIT(exit->eFlag, EF_ELECTRONIC)) {
      SendThing("^wYou cant pick an electronic lock\n", thing);
      return;
    }
    
    /* find if we picked the lock */
    success = FALSE;
    if (thing->tType == TTYPE_PLR && Plr(thing)->pSkill[SKILL_PICKLOCK]>0) {
      unlockNumber = DiceOpenEnded(1, 100, 100);
      unlockNumber += PropertyGetLWord(search, "%security", 0);
      if (unlockNumber <= Plr(thing)->pSkill[SKILL_PICKLOCK])
        success = TRUE;
    }
    
    
    if (success) {
      if (*exit->eKey->sText)
        StrOneWord(exit->eKey->sText, word);
      else
        strcpy(word, "door");
      sprintf(buf, "^gYou pick the lock of the %s\n", word);
      SendThing(buf, thing);
      sprintf(buf, "^g$n picks the lock of the %s\n", word);
      SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      
      BITCLR(exit->eFlag, EF_LOCKED);
      exit = ExitReverse(search, exit);
      if (exit)
        BITCLR(exit->eFlag, EF_LOCKED);
      return;
    } else {
      SendThing("^wI'm afraid you fail to pick the lock\n", thing);
      return;
    }
  }
  
  /* well guess theres nothing else to unlock */
  SendThing("^bThere doesnt appear to be anything like that around\n", thing);
}

CMDPROC(CmdHack) { /* CmdProc(thing,cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *search;
  THING *unlock;
  BYTE   success;
  LWORD  unlockNumber;
  EXIT  *exit;
  FLAG   oFlag;
  BYTE   word[256];
  BYTE   buf[256];

  thing->tWait++;
  cmd = StrOneWord(cmd, NULL); /* lose command at start */
  search = Base(thing)->bInside;
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  unlock = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  if (unlock) {
    if (Obj(unlock)->oTemplate->oType == OTYPE_CONTAINER) {
      oFlag = OBJECTGETFIELD(unlock, OF_CONTAINER_CFLAG);
      if (!BIT(oFlag, OCF_LOCKED)) {
        SendThing("^wBoy, how difficult that's going to be, since its allready unlocked....\n", thing);
        return;
      }
      
      if (!BIT(oFlag, OCF_ELECTRONIC)) {
        SendThing("^wYou cant hack a mechanical lock\n", thing);
        return;
      }
      
      /* find if we picked the lock */
      success = FALSE;
      if (thing->tType == TTYPE_PLR && Plr(thing)->pSkill[SKILL_HACKING]>0) {
        unlockNumber = DiceOpenEnded(1, 100, 100);
        unlockNumber += PropertyGetLWord(unlock, "%security", 0);
        if (unlockNumber <= Plr(thing)->pSkill[SKILL_HACKING])
          success = TRUE;
      }
      
      if (success) {
        BITCLR(oFlag, OCF_LOCKED);
        ObjectSetField(OTYPE_CONTAINER, &Obj(unlock)->oDetail, OF_CONTAINER_CFLAG, oFlag);
        SendThing("^gYou hack the lock of the ", thing);
        SendThing(unlock->tSDesc->sText, thing);
        SendThing("\n", thing);
        SendAction("$n hacks the lock on the $N\n", thing, unlock, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      } else {
        SendThing("^wI'm afraid you fail to hack the lock\n", thing);
        return;
      }
    } else {
      SendThing("^wHack what exactly?\n", thing);
      return;
    }
  }
  
  /* search for Exits */
  do {
    exit = ExitFind(Wld(search)->wExit, srcKey);
    if (exit) srcOffset--;
  } while (exit && srcOffset>0);
  if (exit && srcOffset<=0) {
    if (!BIT(exit->eFlag, EF_ISDOOR)) {
      SendThing("^wYou cant unlock ^rTHAT!\n", thing);
      return;
    }
    
    if (!BIT(exit->eFlag, EF_LOCKED)) {
      SendThing("^wBoy, how difficult that's going to be, since its allready unlocked....\n", thing);
      return;
    }
    
    if (!BIT(exit->eFlag, EF_ELECTRONIC)) {
      SendThing("^wYou cant hack a mechanical lock\n", thing);
      return;
    }
    
    /* find if we picked the lock */
    success = FALSE;
    if (thing->tType == TTYPE_PLR && Plr(thing)->pSkill[SKILL_PICKLOCK]>0) {
      unlockNumber = DiceOpenEnded(1, 100, 100);
      unlockNumber += PropertyGetLWord(search, "%security", 0);
      if (unlockNumber <= Plr(thing)->pSkill[SKILL_PICKLOCK])
        success = TRUE;
    }
    
    
    if (success) {
      if (*exit->eKey->sText)
        StrOneWord(exit->eKey->sText, word);
      else
        strcpy(word, "door");
      sprintf(buf, "^gYou hack the lock of the %s\n", word);
      SendThing(buf, thing);
      sprintf(buf, "^g$n hacks the lock of the %s\n", word);
      SendAction(buf, thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      
      BITCLR(exit->eFlag, EF_LOCKED);
      exit = ExitReverse(search, exit);
      if (exit)
        BITCLR(exit->eFlag, EF_LOCKED);
      return;
    } else {
      SendThing("^wI'm afraid you fail to hack the lock\n", thing);
      return;
    }
  }
  
  /* well guess theres nothing else to unlock */
  SendThing("^bThere doesnt appear to be anything like that around\n", thing);
}

CMDPROC(CmdStand) { /* CmdProc(thing,cmd) */
  /* stand up first */
  if (Character(thing)->cPos == POS_SLEEPING) {
    CmdWake(thing, cmd);
  }
  if (Character(thing)->cPos < POS_RESTING) {
    SendThing("^wNice Try...\n", thing);
    return;
  }
  if (Character(thing)->cPos == POS_STANDING) {
    SendThing("^gBut you are allready!\n", thing);
    return;
  }
  
  SendAction("^YYou stand up\n", thing, NULL, SEND_SRC |SEND_VISIBLE);
  SendAction("^Y$n stands up\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  Character(thing)->cPos = POS_STANDING; 
}

CMDPROC(CmdSit) { /* CmdProc(thing,cmd) */
  if (Character(thing)->cPos < POS_SLEEPING) {
    SendThing("^wNice Try...\n", thing);
    return;
  }
  if (Character(thing)->cPos == POS_SITTING) {
    SendThing("^gBut you are allready!\n", thing);
    return;
  }
  if (Character(thing)->cPos == POS_FIGHTING) {
    SendThing("^wYou're in the middle of a fight here!\n", thing);
    return;
  }

  /* wake up first */
  if (Character(thing)->cPos == POS_SLEEPING) {
    CmdWake(thing, cmd);
    /* if it didnt work for some reason return */
    if (Character(thing)->cPos == POS_SLEEPING) return;
  }
  if (Character(thing)->cPos < POS_SITTING) {
    SendAction("^YYou sit up\n", thing, NULL, SEND_SRC |SEND_VISIBLE);
    SendAction("^Y$n sits up\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  } else {
    SendAction("^YYou sit down\n", thing, NULL, SEND_SRC |SEND_VISIBLE);
    SendAction("^Y$n sits down\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  }
  Character(thing)->cPos = POS_SITTING; 
}

CMDPROC(CmdRest) { /* CmdProc(thing,cmd) */
  if (Character(thing)->cPos < POS_RESTING) {
    SendThing("^wNice Try...\n", thing);
    return;
  }
  if (Character(thing)->cPos == POS_RESTING) {
    SendThing("^gBut you are allready!\n", thing);
    return;
  }
  if (Character(thing)->cPos == POS_FIGHTING) {
    SendThing("^wYou're in the middle of a fight here!\n", thing);
    return;
  }
  
  SendAction("^YYou lie down, and rest awhile\n", thing, NULL, SEND_SRC |SEND_VISIBLE);
  SendAction("^Y$n lies down and starts resting\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  Character(thing)->cPos = POS_RESTING; 
}

CMDPROC(CmdSleep) { /* CmdProc(thing,cmd) */
  if (Character(thing)->cPos < POS_RESTING) {
    SendThing("^wNice Try...\n", thing);
    return;
  }
  if (Character(thing)->cPos == POS_SLEEPING) {
    SendThing("^gBut you are allready!\n", thing);
    return;
  }
  if (Character(thing)->cPos == POS_FIGHTING) {
    SendThing("^wYou're in the middle of a fight here!\n", thing);
    return;
  }

  /* lie down first */
  if (Character(thing)->cPos > POS_RESTING) {
    CmdRest(thing, cmd);
    /* if they couldnt return */
    if (Character(thing)->cPos > POS_RESTING) return;
  } 
  SendAction("^YYou go to sleep\n", thing, NULL, SEND_SRC |SEND_VISIBLE);
  SendAction("^Y$n goes to sleep\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
  Character(thing)->cPos = POS_SLEEPING; 
}

CMDPROC(CmdWake) { /* CmdProc(thing,cmd) */
  if (Character(thing)->cPos < POS_SLEEPING) {
    SendThing("^wNice Try...\n", thing);
    return;
  }
  if (Character(thing)->cPos > POS_SLEEPING) {
    SendThing("^wYou are allready awake!\n", thing);
    return;
  }
  
  Character(thing)->cPos = POS_RESTING; 
  SendAction("^YYou wake up\n", thing, NULL, SEND_SRC |SEND_VISIBLE);
  SendAction("^Y$n wakes up\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
}

CMDPROC(CmdTrack) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE   srcKey[256];
  BYTE   eName[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  EXIT  *exit;
  LWORD  trackRange;
 
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */

  if (!Base(thing)->bInside || Base(thing)->bInside->tType != TTYPE_WLD)
    return;

  /* Show where all the players are */
  if (!*cmd) {
    SendThing("^wWho was it you wanted to track again?\n", thing);
    return;
  }

  /* show matching mobs and plrs in the same area */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_MOB|TF_PLR|TF_MOB_WLD|TF_PLR_WLD, &srcOffset);
  if (!found) {
    SendThing("^wTrack who?, ^gNever heard of 'em\n", thing);
    return;
  } 
  
  trackRange = 1;
  if (thing->tType == TTYPE_PLR)
    trackRange = Plr(thing)->pSkill[SKILL_TRACK];
  else if (thing->tType == TTYPE_MOB)
    trackRange = PropertyGetLWord(thing, "%Track", Mob(thing)->mTemplate->mLevel+10)*3;
 
  /* be suitably snide if they are in the same room */
  if (Base(thing)->bInside == Base(found)->bInside) {
    SendThing("^yTheir tracks are all around you!\n", thing);
    return;
  }
 
  /* Can only track one thing, no while loop */
  exit = ThingTrack(Base(thing)->bInside, found, trackRange);
  if (exit) {
    SendThing("^ySimon says: Go ", thing);
    ExitGetName(exit, eName);
    SendThing(eName, thing);
    SendThing("\n", thing);
  } else {
    SendThing("^yYou study the ground but you cant find any tracks\n", thing);
  } 
}


