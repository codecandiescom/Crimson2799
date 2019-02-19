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

/* Inventory related commands */

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
#include "index.h"
#include "area.h"
#include "reset.h"
#include "code.h"
#include "exit.h"
#include "world.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "send.h"
#include "base.h"
#include "affect.h"
#include "effect.h"
#include "object.h"
#include "board.h"
#include "char.h"
#include "fight.h"
#include "player.h"
#include "skill.h"
#include "parse.h"
#include "cmd_move.h"
#include "cmd_inv.h"


/* Each type of object can be worn in up to 3 different locations
   alter the table to taste */

struct WearListType wearList[] = {
  { "NOWEAR",       0, {   0,0,0 } },

  { "HEAD",         0, {   EQU_HEAD,
                           0,
                           0 } },

  { "NECK",         0, {   EQU_NECK_1,
                           EQU_NECK_2,
                           0 } },

  { "LARGE-NECK",   0, {   EQU_NECK_1|EQU_NECK_2,
                           0,
                           0 } },

  { "SHIRT",        0, {   EQU_TORSO,
                           0,
                           0 } },

  { "UPPER-BODY",   0, {   EQU_TORSO|EQU_ARM_R|EQU_ARM_L,
                           0,
                           0 } },

  { "ARM",          0, {   EQU_ARM_R,
                           EQU_ARM_L,
                           0 } },

  { "BOTH-ARMS",    0, {   EQU_ARM_R|EQU_ARM_L,
                           0,
                           0 } },

  { "WRIST",        0, {   EQU_WRIST_R,
                           EQU_WRIST_L,
                           0 } },

  { "BOTH-WRISTS",  0, {   EQU_WRIST_R|EQU_WRIST_L,
                           0,
                           0 } },

  { "HELD",         0, {   EQU_HELD_R,
                           EQU_HELD_L,
                           EQU_TAILGRIP } },

  { "TWO-HANDED",   0, {   EQU_HELD_R|EQU_HELD_L,
                           0,
                           0 } },

  { "FINGER",       0, {   EQU_FINGER_R,
                           EQU_FINGER_L,
                           0 } },

  { "WAIST",        0, {   EQU_WAIST,
                           0,
                           0 } },

  { "LEGS",         0, {   EQU_LEGS,
                           0,
                           0 } },

  { "FEET",         0, {   EQU_FEET,
                           0,
                           0 } },

  { "OVERBODY",     0, {   EQU_BODY,
                           0,
                           0 } },

  { "EITHER-HAND",  0, {   EQU_HAND_R,
                           EQU_HAND_L,
                           0 } },

  { "BOTH-HANDS",   0, {   EQU_HAND_R|EQU_HAND_L,
                           0,
                           0 } },

  { "R-HAND",       0, {   EQU_HAND_R,
                           0,
                           0 } },

  { "L-HAND",       0, {   EQU_HAND_L,
                           0,
                           0 } },

  { "MECH-ARM",     0, {   EQU_ARM_R|EQU_HELD_R,
                           EQU_ARM_L|EQU_HELD_L,
                           0 } },
  
  { "TAIL",         0, {   EQU_TAIL,
                           0,
                           0 } },

  { "",0,{ 0,0,0 } }
};


/* internal equipment position flags */
BYTE *equipList[] = {
  "HEAD",
  "NECK",
  "NECK",
  "TORSO",
  "OVER-BODY",
  "RIGHT-ARM",
  "LEFT-ARM",
  "RIGHT-WRIST",
  "LEFT-WRIST",
  "RIGHT-HAND",
  "LEFT-HAND",
  "HELD-IN-RIGHT-HAND",
  "HELD-IN-LEFT-HAND",
  "FINGER-ON-RIGHT-HAND",
  "FINGER-ON-LEFT-HAND",
  "WAIST",
  "LEGS",
  "FEET",
  "TAIL",
  "GRIPPED-IN-TAIL",
  ""
};


CMDPROC(CmdGet) {     /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  BYTE   buf[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  THING *found;
  THING *search;
  FLAG   flag;
  THING *world;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  if (*dstKey) { /* search a target */
    search = CharThingFind(thing, dstKey, -1, thing, TF_OBJ, &dstOffset);
    if (!search)
      search = CharThingFind(thing, dstKey, -1, Base(thing)->bInside, TF_OBJ, &dstOffset);
    if (!search) {
      SendThing("^bThere doesnt appear to be anything like that around to look in\n", thing);
      return;
    }
  } else  /* search the room */
    search = Base(thing)->bInside;

  found = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  if (!found)
    SendThing("^bThere doesnt appear to be anything to get\n", thing);

  while (found && srcNum!=0) {
    if (found->tType==TTYPE_OBJ && (Base(found)->bWeight<0)) {
      /* cant grab an item with negative weight or NOTAKE item */
      SendAction("^bYou ^ccant^b take ^g$N\n", 
        thing, found, SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST);
    } else if (!CharCanCarry(thing, found)) {
      /* check if too heavy */
      SendAction("^b$N is too heavy for you to lift\n", 
        thing, found, SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST);
    } else {
      /* Check whether we are looting a corpse */
      if (*dstKey 
       && search->tType==TTYPE_OBJ 
       && Obj(search)->oTemplate->oType==OTYPE_CONTAINER)
      {
        flag=OBJECTGETFIELD(search, OF_CONTAINER_CFLAG);
        if ( BIT(flag, OCF_CLOSED) ) {
          SendAction("^bYou try to take something from ^g$N ^b, then realize its closed\n", 
            thing, search, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
          SendAction("^b$n tries to take something from ^g$N ^b, then realizes that its closed\n",   
            thing, search, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
          return;
        } else if ( BIT(flag, OCF_PLAYERCORPSE) ) {
          /* check whether we are looting our OWN corpse */
          sprintf(buf, FIGHT_CORPSE_KEY, Base(thing)->bKey->sText);
          if (STRICMP(Base(search)->bKey->sText, buf)) {
            if (BIT(flag, OCF_PLAYERLOOTED)) {
              SendAction("^wYou try to loot ^g$N ^wbut someone beat you to it\n", 
                thing, search, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
              SendAction("^b$n tries to loot ^g$N ^wbut someone beat $m to it\n",   
                thing, search, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
              return;
            } else {
              BITSET(flag, OCF_PLAYERLOOTED);
              OBJECTSETFIELD(search, OF_CONTAINER_CFLAG, flag);
              srcNum = 0; /* only one object */
            }
          }
        }
      }

      /* guess we can take it */
      SendAction("^bYou get ^g$N", 
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^b$n gets ^g$N",   
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);

      /* complete the action */
      if (*dstKey) {
        SendAction(" ^bfrom $S ^g$N", thing, search, SEND_SRC|SEND_ROOM|SEND_VISIBLE);
      } 
      SendAction("\n", thing, search, SEND_SRC|SEND_ROOM|SEND_VISIBLE);

      if (found->tType==TTYPE_OBJ 
      && Obj(found)->oTemplate->oValue<5
      && Obj(found)->oTemplate->oValue>=0
      && thing->tType==TTYPE_PLR
      && BIT(Plr(thing)->pAuto, PA_AUTOJUNK)) {
        /* autojunk it its worthless */
        SendAction("^CYou autojunk ^g$N\n", 
          thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^C$n autojunks ^g$N\n",   
          thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        ThingFree(found);
      } else {
        /* move the object into the chars inventory */
        if (found->tType==TTYPE_OBJ&&Obj(found)->oTemplate->oType==OTYPE_MONEY) {
          /* guard against no gain zones etc here */
          world = Base(thing)->bInside;
          if (world->tType==TTYPE_WLD 
           && BIT(areaList[Wld(world)->wArea].aResetFlag, RF_NOMONEY)) {
            sprintf(buf, "^yYou ^rWOULD ^yhave gained %ld credits! (This zone isnt open yet)\n", OBJECTGETFIELD(found, OF_MONEY_AMOUNT));
            SendThing(buf, thing);
          } else {
            sprintf(buf, "^yYou get %ld credits!\n", OBJECTGETFIELD(found, OF_MONEY_AMOUNT));
            SendThing(buf, thing);
            Character(thing)->cMoney += OBJECTGETFIELD(found, OF_MONEY_AMOUNT);
          }
          THINGFREE(found);
        } else {
          ThingTo(found, thing);
        }
      }
    }
    
    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJ|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdDrop) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  THING *found;
  THING *search;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!found)
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);

  while (found && srcNum!=0) {
    if (InvUnEquip(thing, found, IUE_BLOCKABLE)) {
      SendAction("^bYou drop ^g$N\n", 
     thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^b$n drops ^g$N\n",   
     thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      ThingTo(found, Base(thing)->bInside);
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}


CMDPROC(CmdJunk) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!found)
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);

  while (found && srcNum!=0) {
    if (InvUnEquip(thing, found, IUE_BLOCKABLE)) {
      SendAction("^bYou junk ^g$N\n", 
     thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^b$n junks ^g$N\n",   
     thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      ThingFree(found);
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}


CMDPROC(CmdPut) {     /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  THING *found = NULL;
  THING *search = NULL;
  THING *dest = NULL;
  THING *ammo = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;
  
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);
  
  if (*dstKey) { /* search a target */
    dest = CharThingFind(thing, dstKey, -1, thing, TF_OBJ, &dstOffset);
    if (!dest) /* search the room */
      dest = CharThingFind(thing, dstKey, -1, Base(thing)->bInside, TF_OBJ, &dstOffset);
  } else
    dest = NULL;
  if (!dest)
    SendThing("^bThere doesnt appear to be anything like that to put things into\n", thing);
  
  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!found) { /* also search ourself if no 2nd object specified */
    search = Base(thing)->bInside;
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  }
  if (!found)
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
  
  while (found && srcNum!=0) {
    if (dest) {
      ammo = ObjectGetAmmo(dest, &ammoType, &ammoUse, &ammoLeft);
      if (  (Obj(dest)->oTemplate->oType == OTYPE_CONTAINER)
          ||(Obj(found)->oTemplate->oType == OTYPE_AMMO
             && ammoType
             && OBJECTGETFIELD(found, OF_AMMO_AMMOTYPE) == ammoType)
      ) {
        if (  (Obj(dest)->oTemplate->oType == OTYPE_CONTAINER
               && Base(dest)->bConWeight + Base(found)->bWeight > OBJECTGETFIELD(dest, OF_CONTAINER_MAX))
            ||(ammoType 
               && ammo)
        ) {
          SendThing("^bYou cant fit that in there!\n", thing);
          return;
        } else if (Obj(dest)->oTemplate->oType == OTYPE_CONTAINER
                   && BIT(OBJECTGETFIELD(dest, OF_CONTAINER_CFLAG), OCF_CLOSED)
        ) {
          SendThing("^bBut its closed!\n", thing);
          return;
        } else if (dest == found) {
          SendThing("^bYou cant put something inside itself!\n", thing);
          return;
        } else {
          SendAction("^bYou put ^g$N", 
               thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
          SendAction("^b$n puts ^g$N",   
               thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
          SendAction(" ^binto ^g$N\n", 
               thing, dest, SEND_SRC|SEND_ROOM|SEND_VISIBLE);
          ThingTo(found, dest);
        }
      } else {
        SendThing("^bYou cant put that in there!\n", thing);
        return;
      }
    }
    
    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (!found) { /* also search room if no 2nd object specified */
      search = Base(thing)->bInside;
      found = CharThingFind(thing, srcKey, -1, search, TF_OBJ|TF_CONTINUE, &srcOffset);
    }
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdGive) {     /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   buf[256];
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  THING *found;
  THING *search;
  THING *dest;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  /* search the room for a target */
  if (*dstKey) { 
    dest = CharThingFind(thing, dstKey, -1, Base(thing)->bInside, TF_PLR|TF_MOB, &dstOffset);
  } else
    dest = NULL;
  if (!dest) {
    SendThing("^bThere doesnt appear to be anyone like that to give things to\n", thing);
    return;
  }

  /* search ourself for an object */
  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  /* intercept here to give credits */
  if ( (!found && StrAbbrev("credits", srcKey)) 
    || (StrExact("credits", srcKey)) )
  {
    if (srcNum < 0) {
      SendThing("^wAnd how does that work again!?!\n", thing);
      return;
    } else if (srcNum > Character(thing)->cMoney) {
      SendThing("^wYou dont have that much money!\n", thing);
      return;
    } else {
      /* transfer the money */
      Character(thing)->cMoney -= srcNum;
      Character(dest)->cMoney += srcNum;
      /* send the messages */
      sprintf(buf, "^CYou give $N %ld credits\n", srcNum);
      SendAction(buf, thing, dest, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      sprintf(buf, "^b$n gives you %ld credits\n", srcNum);
      SendAction(buf, thing, dest, SEND_DST |SEND_VISIBLE|SEND_CAPFIRST);
      sprintf(buf, "^C$n gives $N some money\n");
      SendAction(buf, thing, dest, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      return;
    }
  }

  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around to give\n", thing);
    return;
  }

  while (found && srcNum!=0) {
    if (Base(found)->bWeight + Base(dest)->bConWeight > CharGetCarryMax(dest)) {
      SendAction("^CYou try to give ^g$N", 
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^C$n tries to give ^g$N",   
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction(" ^Cto ^g$N^C, but its too heavy!\n", 
        thing, dest, SEND_SRC|SEND_ROOM|SEND_VISIBLE);
      SendAction(" ^bto ^gyou^C, but its too heavy!\n", 
        thing, dest, SEND_DST|SEND_VISIBLE);
    } else {
      SendAction("^bYou give ^g$N", 
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^b$n gives ^g$N",   
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction(" ^bto ^g$N\n", 
        thing, dest, SEND_SRC|SEND_ROOM|SEND_VISIBLE);
      SendAction(" ^bto ^gyou\n", 
        thing, dest, SEND_DST|SEND_VISIBLE);
      ThingTo(found, dest);
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

/* Add direct applies of an equipped item */
void InvApply(THING *thing, THING *equip) {
  LWORD apply;
  LWORD aType;
  LWORD aValue;

  switch (Obj(equip)->oTemplate->oType) {
  case OTYPE_ARMOR:
    Effect(AFFECT_ARMOR,          TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_ARMOR));
    Effect(AFFECT_RES_PUNCTURE,   TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_RPUNCTURE));
    Effect(AFFECT_RES_SLASH,      TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_RSLASH));
    Effect(AFFECT_RES_CONCUSSIVE, TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_RCONCUSSIVE));
    Effect(AFFECT_RES_HEAT,       TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_RHEAT));
    Effect(AFFECT_RES_EMR,        TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_REMR));
    Effect(AFFECT_RES_LASER,      TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_RLASER));
    Effect(AFFECT_RES_PSYCHIC,    TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_RPSYCHIC));
    Effect(AFFECT_RES_ACID,       TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_RACID));
    Effect(AFFECT_RES_POISON,     TAR_AFFECT, thing, NULL, OBJECTGETFIELD(equip, OF_ARMOR_RPOISON));
    break;
  }
  /* apply direct effects */
  for (apply=0; apply < OBJECT_MAX_APPLY; apply++) {
    aType  = Obj(equip)->oApply[apply].aType;
    if (applyList[aType].aTarget==TAR_AFFECT) {
      aValue = Obj(equip)->oApply[apply].aValue;
      Effect(applyList[aType].aEffect, applyList[aType].aTarget, thing, NULL, aValue);
    }
  }
}

/* return FALSE if we cant equip this */
BYTE InvEquip(THING *thing, THING *equip, BYTE *message) {
  LWORD i;
  LWORD wearType;
  LWORD apply;
  LWORD aType;
  LWORD aValue;
  THING *ammo;
  LWORD ammoType;
  LWORD ammoUse;
  LWORD ammoLeft;

  if (message) message[0]='\0';
  /* MinStr etc */
  if (thing->tType == TTYPE_PLR) {
    if (Plr(thing)->pStr < PropertyGetLWord(thing, "%MINSTR", 0)) {
      if (message) sprintf(message, "You are not strong enough\n");
      return FALSE;
    } 
    if (Plr(thing)->pDex < PropertyGetLWord(thing, "%MINDEX", 0)) {
      if (message) sprintf(message, "You are not dexterous enough\n");
      return FALSE;
    } 
    if (Plr(thing)->pCon < PropertyGetLWord(thing, "%MINCON", 0)) {
      if (message) sprintf(message, "You are not tough enough\n");
      return FALSE;
    } 
    if (Plr(thing)->pWis < PropertyGetLWord(thing, "%MINWIS", 0)) {
      if (message) sprintf(message, "You are not wise enough\n");
      return FALSE;
    } 
    if (Plr(thing)->pInt < PropertyGetLWord(thing, "%MININT", 0)) {
      if (message) sprintf(message, "You are not smart enough\n");
      return FALSE;
    }
  }

  if (Character(thing)->cLevel < PropertyGetLWord(thing, "%MINLEVEL", 0)) {
    if (message) sprintf(message, "You are not high enough in level\n");
    return FALSE;
  }

  /* Aura check */
  if (Character(thing)->cAura>400) {
    if (BIT(Obj(equip)->oAct, OA_ANTI_GOOD)) {
      if (message) sprintf(message, "You are too good\n");
      return FALSE;
    }
  } else if (Character(thing)->cAura<-400) {
    if (BIT(Obj(equip)->oAct, OA_ANTI_EVIL)) {
      if (message) sprintf(message, "You are too evil\n");
      return FALSE;
    }
  } else if (BIT(Obj(equip)->oAct, OA_ANTI_NEUTRAL)) {
    if (message) sprintf(message, "You are too neutral\n");
    return FALSE;
  }

  /* ensure they have ammo for it */
  if (thing->tType==TTYPE_PLR && Obj(equip)->oTemplate->oType!=OTYPE_WEAPON) {
    ammo = ObjectGetAmmo(equip, &ammoType, &ammoUse, &ammoLeft);
    if (!ammo && ammoType) {
      if (message) sprintf(message, "It is out of ammo\n");
      return FALSE;
    }
  }

  wearType = Obj(equip)->oTemplate->oWear;

  /* for loop searches for an available location to equip the item in */
  for (i=0; i<MAX_EQUIP; i++) {
    if (!wearList[wearType].wEquipList[i]) continue;
    if ( ( (thing->tType == TTYPE_PLR)
         &&(!BITANY(Character(thing)->cEquip,           wearList[wearType].wEquipList[i]))
         &&( BIT(raceList[Plr(thing)->pRace].rLocation, wearList[wearType].wEquipList[i]))
         &&(!wearList[wearType].wEquipNot || !BITANY(raceList[Plr(thing)->pRace].rLocation, wearList[wearType].wEquipNot))
         )
       ||( (thing->tType == TTYPE_MOB)
         &&(!BITANY(Character(thing)->cEquip,           wearList[wearType].wEquipList[i]))
         )
       )
    {
      BITSET(Obj(equip)->oEquip,       wearList[wearType].wEquipList[i]);
      BITSET(Character(thing)->cEquip, wearList[wearType].wEquipList[i]);

      switch (Obj(equip)->oTemplate->oType) {
      case OTYPE_WEAPON:
        Character(thing)->cWeapon = equip;
        break;
      case OTYPE_LIGHT:
        if (Base(thing)->bInside->tType==TTYPE_WLD)
          Wld(Base(thing)->bInside)->wLight += OBJECTGETFIELD(equip, OF_LIGHT_INTENSITY);
        break;
      }
      
      InvApply(thing, equip);

      /* add non-direct applies */
      for (apply=0; apply < OBJECT_MAX_APPLY; apply++) {
        aType  = Obj(equip)->oApply[apply].aType;
        if (applyList[aType].aTarget!=TAR_AFFECT) {
          aValue = Obj(equip)->oApply[apply].aValue;
          Effect(applyList[aType].aEffect, applyList[aType].aTarget, thing, NULL, aValue);
        }
      }
      return TRUE;
    }
  }
  if (message) sprintf(message, "You have no place available to use that\n");
  return FALSE;
}

/* Temporarily remove applies of an equipped item, must reapply in same routine */
void InvUnapply(THING *thing, THING *equip) {
  LWORD apply;
  LWORD aType;
  LWORD aValue;

  switch (Obj(equip)->oTemplate->oType) {
  case OTYPE_ARMOR:
    EffectFree(AFFECT_ARMOR,          TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_ARMOR));
    EffectFree(AFFECT_RES_PUNCTURE,   TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_RPUNCTURE));
    EffectFree(AFFECT_RES_SLASH,      TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_RSLASH));
    EffectFree(AFFECT_RES_CONCUSSIVE, TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_RCONCUSSIVE));
    EffectFree(AFFECT_RES_HEAT,       TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_RHEAT));
    EffectFree(AFFECT_RES_EMR,        TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_REMR));
    EffectFree(AFFECT_RES_LASER,      TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_RLASER));
    EffectFree(AFFECT_RES_PSYCHIC,    TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_RPSYCHIC));
    EffectFree(AFFECT_RES_ACID,       TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_RACID));
    EffectFree(AFFECT_RES_POISON,     TAR_AFFECT, thing, OBJECTGETFIELD(equip, OF_ARMOR_RPOISON));
    break;
  }
  /* remove direct effects */
  for (apply=0; apply < OBJECT_MAX_APPLY; apply++) {
    aType  = Obj(equip)->oApply[apply].aType;
    if (applyList[aType].aTarget==TAR_AFFECT) {
      aValue = Obj(equip)->oApply[apply].aValue;
      EffectFree(applyList[aType].aEffect, applyList[aType].aTarget, thing, aValue);
    }
  }
}

/* return FALSE if we cant unequip this */
BYTE InvUnEquip(THING *thing, THING *equip, BYTE blockable) {
  LWORD apply;
  LWORD aType;
  LWORD aValue;

  /* check params for validity */
  if (!thing || !equip) return TRUE;
  if (thing->tType < TTYPE_CHARACTER) return TRUE;

  /* Check to see if its allready unequipped */
  if (equip->tType!=TTYPE_OBJ || Obj(equip)->oEquip==0)
    return TRUE;

  /* If we can, unequip it */
  if (blockable) {
    if (BIT(Obj(equip)->oAct, OA_NODROP))
      return FALSE;
  }
  BITCLR(Character(thing)->cEquip, Obj(equip)->oEquip);

  Obj(equip)->oEquip = 0;
  switch (Obj(equip)->oTemplate->oType) {
  case OTYPE_WEAPON:
    if (Character(thing)->cWeapon == equip)
      Character(thing)->cWeapon = NULL;
    break;
  case OTYPE_LIGHT:
    if (Base(thing)->bInside && Base(thing)->bInside->tType==TTYPE_WLD)
      Wld(Base(thing)->bInside)->wLight -= OBJECTGETFIELD(equip, OF_LIGHT_INTENSITY);
    break;
  }

  InvUnapply(thing, equip);

  /* remove non-direct apply's */
  for (apply=0; apply < OBJECT_MAX_APPLY; apply++) {
    aType  = Obj(equip)->oApply[apply].aType;
    if (applyList[aType].aTarget!=TAR_AFFECT) {
      aValue = Obj(equip)->oApply[apply].aValue;
      EffectFree(applyList[aType].aEffect, applyList[aType].aTarget, thing, aValue);
    }
  }
  return TRUE;
}

void InvEquipShow(THING *thing) { 
  BYTE  buf[256];
  LWORD shotLeft;
  THING *ammo;
  LWORD ammoType;
  LWORD ammoUse;
  LWORD ammoLeft;
  LWORD fireRate;

  SendThing("^pYou are currently Equipped with:\n", thing);
  SendThing("^P-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n", thing);

  CharShowEquip(thing, thing); /* show them their own equip list */
  
  /* Show Weapon */
  if (Character(thing)->cWeapon) {
    SendThing("\n^GYour Currently Selected Weapon is:\n^g", thing);
    SendThing(Character(thing)->cWeapon->tSDesc->sText, thing);
    ammo = ObjectGetAmmo(Character(thing)->cWeapon, &ammoType, &ammoUse, &ammoLeft);
    if (ammoUse>0) {
      if (!ammo)  {
        SendThing(" ^r<UNLOADED>", thing);
      } else if (ammoLeft<0) {
        SendThing(" ^r<UNLIMITED AMMO>", thing);
      } else {
        fireRate = OBJECTGETFIELD(Character(thing)->cWeapon, OF_WEAPON_FIRERATE);
        shotLeft = ammoLeft/ammoUse;
        if (!shotLeft) {
          SendThing(" ^w<OUT OF AMMO>", thing);
        } else if ( fireRate*3 > shotLeft) {
          sprintf(buf, " ^r<Only %ld shots left!>", shotLeft);
          SendThing(buf, thing);
        } else {
          sprintf(buf, " ^y<%ld shots left>", shotLeft);
          SendThing(buf, thing);
        }
      }
    } else
      SendThing(" ^r<NO AMMO NEEDED>", thing);
    SendThing("\n", thing);
  }
}

/* Eat primitive, called by CmdEat as well as PlayerTick */
void InvEat(THING *thing, THING *found) {
  LWORD poison;

  SendAction("^bYou eat ^g$N\n", 
       thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
  SendAction("^b$n eats ^g$N\n",   
       thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  if (thing->tType == TTYPE_PLR) {
    Plr(thing)->pHunger -= OBJECTGETFIELD(found, OF_FOOD_HOWFILLING);
    /* ADD: Check OF_FOOD_POISON here too */
    poison = OBJECTGETFIELD(found, OF_FOOD_POISON);
    if (poison > 0)
      Effect(EFFECT_POISON, TAR_SELF_DEF, thing, "", poison);  
    
    /* check min hunger against race */
    BOUNDSET((-1*raceList[Plr(thing)->pRace].rMaxThirst), Plr(thing)->pThirst, raceList[Plr(thing)->pRace].rMaxThirst);
    BOUNDSET((-1*raceList[Plr(thing)->pRace].rMaxHunger), Plr(thing)->pHunger, raceList[Plr(thing)->pRace].rMaxHunger);
    BOUNDSET((-1*raceList[Plr(thing)->pRace].rMaxIntox) , Plr(thing)->pIntox,  raceList[Plr(thing)->pRace].rMaxIntox );
  }
  THINGFREE(found);
}

/* Eat primitive, called by CmdEat as well as PlayerTick */
void InvDrink(THING *thing, THING *found) {
  LWORD liquid;
  LWORD liquidLeft;
  LWORD poison;

  if (Base(found)->bInside != thing) {
    SendAction("^bYou take a drink from $A ^g$N\n", 
         thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^b$n takes a drink from $A ^g$N\n",   
         thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  } else {
    SendAction("^bYou take a drink from your ^g$N\n", 
         thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^b$n takes a drink from $s ^g$N\n",   
         thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  }
  if (thing->tType == TTYPE_PLR) {
    liquid = OBJECTGETFIELD(found, OF_DRINKCON_LIQUID);
    TYPECHECK(liquid, oLiquidList);
    /* thirst */
    if (Plr(thing)->pThirst>=-1*raceList[Plr(thing)->pRace].rMaxThirst) {
      Plr(thing)->pThirst -= oLiquidList[liquid].oLThirst;
      BOUNDSET((-1*raceList[Plr(thing)->pRace].rMaxThirst), Plr(thing)->pThirst, raceList[Plr(thing)->pRace].rMaxThirst);
    }
    /* Hunger */
    if (Plr(thing)->pHunger>=-1*raceList[Plr(thing)->pRace].rMaxHunger) {
      Plr(thing)->pHunger += oLiquidList[liquid].oLHunger;
      BOUNDSET((-1*raceList[Plr(thing)->pRace].rMaxHunger), Plr(thing)->pHunger, raceList[Plr(thing)->pRace].rMaxHunger);
    }
    /* Intox */
    if (Plr(thing)->pIntox>=-1*raceList[Plr(thing)->pRace].rMaxIntox) {
      Plr(thing)->pIntox  += oLiquidList[liquid].oLIntox;
      BOUNDSET((-1*raceList[Plr(thing)->pRace].rMaxIntox) , Plr(thing)->pIntox,  raceList[Plr(thing)->pRace].rMaxIntox );
    }
    if (oLiquidList[liquid].oLPoison > 0)
      Effect(EFFECT_POISON, TAR_SELF_DEF, thing, "", oLiquidList[liquid].oLPoison);  
  }

  /* ADD: Check OF_DRINKCON_POISON here too */
  poison = OBJECTGETFIELD(found, OF_DRINKCON_POISON);
  if (poison > 0)
    Effect(EFFECT_POISON, TAR_SELF_DEF, thing, "", poison);  

  liquidLeft = OBJECTGETFIELD(found, OF_DRINKCON_CONTAIN);
  if (liquidLeft>0) {
    liquidLeft--;
    MINSET(liquidLeft, 0);
  }
  OBJECTSETFIELD(found, OF_DRINKCON_CONTAIN, liquidLeft);
}

CMDPROC(CmdEquip) {   /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  BYTE   message[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  THING *found;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) { /* give the player the list of things they have equipped */
    InvEquipShow(thing);
    return;
  }
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);
  
  found = CharThingFind(thing, srcKey, -1, thing, TF_OBJINV, &srcOffset);
  if (!found) {
    SendThing("^bEquip what?!?\n", thing);
    return;
  }
  
  while(found && srcNum!=0) {
    if (Obj(found)->oEquip==0) {
      if (InvEquip(thing, found, message)) {
        SendAction("^bYou equip yourself with ^g$N\n", 
          thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n equips $mself with ^g$N\n",   
          thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      } else {
        /* was unable to equip the item */
        if (STRICMP(srcKey, "all")) {
          SendThing("^bYou dont seem to be able to equip ^g", thing);
          SendThing(found->tSDesc->sText, thing);
          SendThing("\n", thing);
          SendThing(message, thing);
        }
      }
    }
    
    found = CharThingFind(thing, srcKey, -1, thing, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}


CMDPROC(CmdUnEquip) { /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING  *found;
  AFFECT *affect;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) { /* give the player the list of things they have equipped */
    InvEquipShow(thing);
    return;
  }
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = CharThingFind(thing, srcKey, -1, thing, TF_OBJEQUIP, &srcOffset);
  if (!found) {
    SendThing("^bUnEquip what?!?\n", thing);
    return;
  }

  while(found && srcNum!=0) {
    if (Obj(found)->oEquip!=0) {
      /* Expire the psi-power object */
      if (Obj(found)->oTemplate == phantomPocketTemplate) {
        affect = AffectFind(thing, EFFECT_PHANTOMPOCKET);
        if (affect) AffectRemove(thing, affect);

      /* Expire the psi-power object */
      } else if (Obj(found)->oTemplate == fireBladeTemplate) {
        affect = AffectFind(thing, EFFECT_FIREBLADE);
        if (affect) AffectRemove(thing, affect);

      /* Expire the psi-power object */
      } else if (Obj(found)->oTemplate == fireShieldTemplate) {
        affect = AffectFind(thing, EFFECT_FIRESHIELD);
        if (affect) AffectRemove(thing, affect);

      /* Expire the psi-power object */
      } else if (Obj(found)->oTemplate == fireArmorTemplate) {
        affect = AffectFind(thing, EFFECT_FIREARMOR);
        if (affect) AffectRemove(thing, affect);

      /* Try to unequip it */
      } else if (InvUnEquip(thing, found, IUE_BLOCKABLE)) {
        SendAction("^bYou unequip ^g$N\n", 
          thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n unequips ^g$N\n",   
          thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);

      /* was unable to unequip the item */
      } else {
        SendThing("^bYou dont seem to be able to unequip ^g", thing);
        SendThing(found->tSDesc->sText, thing);
        SendThing("\n", thing);
      }
    }

    found = CharThingFind(thing, srcKey, -1, thing, TF_OBJEQUIP|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdInventory) { /* void CmdProc(THING *thing, BYTE* cmd) */
  THING *show;
  BYTE  buf[256];
  SOCK *sock;
  BYTE  expert = FALSE;
  
  sock = BaseControlFind(thing);
  if (sock && BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) expert = TRUE;

  if (Base(thing)->bConWeight <= CharGetCarryMax(thing)/4)
    sprintf(buf, 
            "^pYou are currently carrying: ^w%hd/%ld pounds ^y(Move cost = Normal)\n", 
            Base(thing)->bConWeight, 
            CharGetCarryMax(thing));
  else if (Base(thing)->bConWeight <= CharGetCarryMax(thing)/2)
    sprintf(buf, 
            "^pYou are currently carrying: ^w%hd/%ld pounds ^y(Move cost = 1.5X)\n", 
            Base(thing)->bConWeight, 
            CharGetCarryMax(thing));
  else if (Base(thing)->bConWeight <= 75*CharGetCarryMax(thing)/100)
    sprintf(buf, 
            "^pYou are currently carrying: ^w%hd/%ld pounds ^y(Move cost = 2X)\n", 
            Base(thing)->bConWeight, 
            CharGetCarryMax(thing));
  else 
    sprintf(buf, 
            "^pYou are currently carrying: ^w%hd/%ld pounds ^y(Move cost = 2.5X)\n", 
            Base(thing)->bConWeight, 
            CharGetCarryMax(thing));
  SendThing(buf, thing);
  SendThing("^P-=-=-=-=-=-=-=-=-=-=-=-=-=-\n", thing);
  show=thing->tContain; 
  while (show) {
    if (show->tType!=TTYPE_OBJ || Obj(show)->oEquip==0) {
      show = ThingShow(show, thing);
    } else
      show = show->tNext;
  }
  /* Hint for total newbies */
  if (Character(thing)->cEquip && Character(thing)->cLevel<=5 && !expert)
    SendHint("^;HINT: To see what you are equipped with, type ^<EQUIP\n", thing);

}

CMDPROC(CmdEat) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;
  
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  
  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!found)
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
  
  while (found && srcNum!=0) {
    if (InvUnEquip(thing, found, IUE_BLOCKABLE)) {
      if (Obj(found)->oTemplate->oType == OTYPE_FOOD) {
        InvEat(thing, found);
      } else {
        SendAction("^bYou try to eat the ^g$N ^bthen realize it isnt food\n", 
          thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n tries to eat $A ^g$N ^bthen realizes it isnt food\n",
          thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      }
    } else {
      SendAction("^bYou dont seem to be able to unequip ^g$N ^blong enough to eat it!\n", 
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^b$n  doesnt seem to be able to unequip ^g$N ^blong enough to eat it!\n",   
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    }
    
    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}


CMDPROC(CmdDrink) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;
  LWORD  liquidLeft;
  
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  
  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!found)
    found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
  if (!found)
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
  
  while (found && srcNum!=0) {
    if (Obj(found)->oTemplate->oType == OTYPE_DRINKCON) {
      liquidLeft = OBJECTGETFIELD(found, OF_DRINKCON_CONTAIN);
      if (liquidLeft!=0) {
        InvDrink(thing, found);
      } else {
        SendAction("^bYou try to drink from ^g$N ^bthen realize its empty\n", 
          thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n tries to drink from ^g$N ^bthen realizes its empty\n",   
          thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      }
    } else {
      SendAction("^bYou try to drink from ^g$N ^bthen realizes it isnt possible\n", 
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^b$n tries to eat $A ^g$N ^bthen realizes it isnt possible\n",   
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    }
    
    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}


CMDPROC(CmdFill) {     /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  THING *found = NULL;
  THING *search = NULL;
  THING *dest = NULL;
  LWORD  maxFound;
  LWORD  containFound;
  LWORD  liquidFound;
  LWORD  poisonFound;
  LWORD  containDest;
  LWORD  liquidDest;
  LWORD  poisonDest;
  LWORD  transfer;
  
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);
  
  if (*dstKey) { /* search a target */
    dest = CharThingFind(thing, dstKey, -1, thing, TF_OBJINV, &dstOffset);
    if (!dest) /* search the room */
      dest = CharThingFind(thing, dstKey, -1, Base(thing)->bInside, TF_OBJ, &dstOffset);
  } else
    dest = NULL;
  if (!dest) {
    SendThing("^bTry something along the lines of ^wfill canteen fountain\n", thing);
    return;
  } 

  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!found)
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
  
  while (found && srcNum!=0) {
    if (  (Obj(found)->oTemplate->oType == OTYPE_DRINKCON)
  &&(Obj(dest )->oTemplate->oType == OTYPE_DRINKCON)
  ) {
      /* Read data from found */
      maxFound     = OBJECTGETFIELD(found, OF_DRINKCON_MAX);
      containFound = OBJECTGETFIELD(found, OF_DRINKCON_CONTAIN);
      liquidFound  = OBJECTGETFIELD(found, OF_DRINKCON_LIQUID);
      poisonFound  = OBJECTGETFIELD(found, OF_DRINKCON_POISON);
      /* read data from dest */
      containDest  = OBJECTGETFIELD(dest,  OF_DRINKCON_CONTAIN);
      liquidDest   = OBJECTGETFIELD(dest,  OF_DRINKCON_LIQUID);
      poisonDest   = OBJECTGETFIELD(dest,  OF_DRINKCON_POISON);
      
      if (containFound!=0 && liquidFound != liquidDest) {
        /* source allready has a different liquid in it */
        SendAction("^bYou start to fill your ^g$N^b, then realize that you allready have something in it\n", 
          thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n starts to fill $s ^g$N^b, then realizes that $h allready has something in it\n",   
          thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      } else if (containFound<0 || containFound>=maxFound) {
        /* trying to fill to an infinite source or full drinkcon */
        SendAction("^bYou start to fill your ^g$N^b, then realize its allready full\n", 
          thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n starts to fill $s ^g$N^b, then realizes that its allready full\n",   
          thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      } else if (containDest == 0) {
        /* trying to fill from a empty drinkcon */
        SendAction("^bYou start to fill your ^g$N", 
          thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n starts to fill $s ^g$N",   
          thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b, but the ^g$N^b is empty\n", 
          thing, dest, SEND_SRC|SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE);
      } else if (dest==found) {
        /* trying to fill a drinkcon from itself */
        SendAction("^bYou fill your ^g$N ^bfrom itself (real productive).", 
          thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction(
          "^b$n fills ^g$N ^bfrom itself (bright as a bubble, that one).",   
          thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      } else {
        /* perform the fill */
        transfer = maxFound - containFound;
        if (containDest>=0) {
          MAXSET(transfer, containDest);
          containDest -= transfer;
          OBJECTSETFIELD(dest, OF_DRINKCON_CONTAIN, containDest);
        }
        containFound+=transfer;
        OBJECTSETFIELD(found, OF_DRINKCON_CONTAIN, containFound);
        /* Poison accordingly */
        if (poisonDest > poisonFound) {
          poisonFound = poisonDest;
          OBJECTSETFIELD(found, OF_DRINKCON_POISON, poisonFound);
        }
  
        SendAction("^bYou fill your ^g$N", 
          thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n fills $s ^g$N",   
          thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction(" ^bfrom the ^g$N\n", 
          thing, dest, SEND_SRC|SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE);
      }    
    } else {
      SendAction("^bYou cant fill your $N",
        thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^bfrom the $N!\n",
        thing, dest, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
    }
    
    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}


CMDPROC(CmdEmpty) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  LWORD  contain;
  THING *found;
  THING *search;
  
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);
  
  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!found)
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
  
  while (found && srcNum!=0) {
    if (Obj(found)->oTemplate->oType == OTYPE_DRINKCON) {
      contain = OBJECTGETFIELD(found, OF_DRINKCON_CONTAIN);
      if (contain>0) {
        /* full container */
        SendAction("^bYou empty your ^g$N ^bon the floor\n", 
             thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n empties $s ^g$N ^bon the floor\n",   
             thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        OBJECTSETFIELD(found, OF_DRINKCON_CONTAIN, 0);
        OBJECTSETFIELD(found, OF_DRINKCON_POISON,  0);
      } else if (contain==0) {
        /* empty container */
        SendAction("^bYou hold your ^g$N ^bupside down and shake, but nothing comes out\n", 
             thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n holds $s ^g$N ^bupside down and shakes it, but nothing comes out\n",   
             thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      } else {
        /* infinite liquid source */
        SendAction("^bYou hold your ^g$N ^bupside down and spray liquid all over the floor\n", 
             thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^b$n holds $s ^g$N ^bupside down and sprays liquid all over the floor\n",   
             thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      }
    } else {
      SendAction("^bYou empty your ^g$N\n", 
                thing, found, SEND_SRC |SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^b$n empties $s ^g$N\n",   
                thing, found, SEND_ROOM|SEND_AUDIBLE|SEND_VISIBLE|SEND_CAPFIRST);
    }
    while(found->tContain)
      ThingTo(found->tContain, Base(found)->bInside);
    
    
    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

CMDPROC(CmdRead) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE      srcKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  THING    *found;
  LWORD     i;
  LWORD     board;
  BYTE     *origCmd;

  origCmd = cmd;
  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry Read <Object>\n", thing);
    return;
  }
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  
  found = CharThingFind(thing, srcKey, -1, thing, TF_OBJ, &srcOffset);
  if (!found)
    found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
  if (!found) {
    /* SendThing("^bThere doesnt appear to be anything like that around\n", thing); */
    CmdLook(thing, origCmd);
    return;
  }
  if (Obj(found)->oTemplate->oType!=OTYPE_BOARD) {
    /* SendThing("^bThat sort of thing cant have anything written on it.\n", thing); */
    CmdLook(thing, origCmd);
    return;
  }
  board = BoardOf(OBJECTGETFIELD(found, OF_BOARD_BVIRTUAL));
  if (board == -1) {
    SendThing("^wTell whoever created this thing that the virtual # is invalid\n", thing);
    return;
  }

  /* List of message titles */
  if (!*cmd) {
    BoardShow(board, thing);
    SendHint("^;HINT: Try ^<Read ^;<object> <msg#> to read a message\n", thing);
    return;
  }
  
  i = -1;
  sscanf(cmd, " %ld", &i);
  i -= 1;
  if (i<0 || i>= boardList[board].bIndex.iNum) {
    SendThing("^wThere is no message with that number\n", thing);
    return;
  }

  BoardShowMessage(board, i, thing);
}

CMDPROC(CmdEdit) {
  LWORD     board;
  BYTE      strName[256];
  BOARDMSG *boardMsg;
  LWORD     i;
  BYTE      srcKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  THING    *found;

  if (thing->tType!=TTYPE_PLR) {
    SendThing("^rMobs cant edit messages\n", thing);
    return;
  }

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry Edit <Object> <msg #>\n", thing);
    return;
  }
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = CharThingFind(thing, srcKey, -1, thing, TF_OBJ, &srcOffset);
  if (!found)
    found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }
  if (Obj(found)->oTemplate->oType!=OTYPE_BOARD) {
    SendThing("^bThat sort of thing can have anything written on it.\n", thing);
    return;
  }
  board = BoardOf(OBJECTGETFIELD(found, OF_BOARD_BVIRTUAL));
  if (board == -1) {
    SendThing("^wTell whoever created this thing that the virtual # is invalid\n", thing);
    return;
  }

  /* check virtual # */
  i = -1;
  sscanf(cmd, " %ld", &i);
  i -= 1;
  if (i<0 || i>= boardList[board].bIndex.iNum) {
    SendThing("^wThere is no message with that number\n", thing);
    return;
  }
  boardMsg = BoardMsg(boardList[board].bIndex.iThing[i]);

  if (  (Character(thing)->cLevel<LEVEL_CODER) 
      ||(!StrExact(thing->tSDesc->sText, boardMsg->bAuthor->sText)) ) {
    SendThing("^rYou can only edit your own messages\n", thing);
    return;
  }

  sprintf(strName, "Board [%s]- Edit", boardList[board].bFileName);
  EDITSTR(thing, boardMsg->bText, 2048, strName, EP_ENDLF|EP_IMMNEW);
  EDITFLAG(thing, &boardList[board].bFlag, B_UNSAVED);
}


CMDPROC(CmdWrite) {
  LWORD     board;
  BYTE      strName[256];
  BOARDMSG *boardMsg;
  BYTE      srcKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  THING    *found;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry Write <Object> <Message Title>\n", thing);
    return;
  }
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = CharThingFind(thing, srcKey, -1, thing, TF_OBJ, &srcOffset);
  if (!found)
    found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }
  if (Obj(found)->oTemplate->oType!=OTYPE_BOARD) {
    SendThing("^bThat sort of thing can have anything written on it.\n", thing);
    return;
  }
  board = BoardOf(OBJECTGETFIELD(found, OF_BOARD_BVIRTUAL));
  if (board == -1) {
    SendThing("^wTell whoever created this thing that the virtual # is invalid\n", thing);
    return;
  }

  /* chuck virtual # */
  if (!*cmd)
    boardMsg = BoardMsgCreate(board, thing, "^Y<No Title>", BOARD_NOTREPLY);
  else
    boardMsg = BoardMsgCreate(board, thing, cmd, BOARD_NOTREPLY);

  if (!boardMsg) {
    SendThing("^rNot good, critical error aborting message\n", thing);
    return;
  }
  sprintf(strName, "Board [%s] - Write", boardList[board].bFileName);
  EDITSTR(thing, boardMsg->bText, 2048, strName, EP_ENDLF|EP_IMMNEW);
  EDITFLAG(thing, &boardList[board].bFlag, B_UNSAVED);
}

CMDPROC(CmdReply) {
  LWORD     board;
  BYTE      titleBuf[50];
  BYTE      strName[256];
  BOARDMSG *boardMsg;
  LWORD     i;
  BYTE      srcKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  THING    *found;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry Reply <Object> <msg #> [<msg title>]\n", thing);
    return;
  }
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = CharThingFind(thing, srcKey, -1, thing, TF_OBJ, &srcOffset);
  if (!found)
    found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }
  if (Obj(found)->oTemplate->oType!=OTYPE_BOARD) {
    SendThing("^bThat sort of thing cant have anything written on it.\n", thing);
    return;
  }
  board = BoardOf(OBJECTGETFIELD(found, OF_BOARD_BVIRTUAL));
  if (board == -1) {
    SendThing("^wTell whoever created this thing that the virtual # is invalid\n", thing);
    return;
  }

  /* check msg # */
  i = -1;
  sscanf(cmd, " %ld", &i);
  i -= 1;
  if (i<0 || i>= boardList[board].bIndex.iNum) {
    SendThing("^wThere is no message with that number\n", thing);
    return;
  }
  boardMsg = BoardMsg(boardList[board].bIndex.iThing[i]);

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    strcpy(titleBuf, "Re:");
    strncpy(titleBuf+3, boardMsg->bTitle->sText, sizeof(titleBuf)-3);
    titleBuf[sizeof(titleBuf)-1] = 0;
    boardMsg = BoardMsgCreate(board, thing, titleBuf, i);
  } else
    boardMsg = BoardMsgCreate(board, thing, cmd, i);

  if (!boardMsg) {
    SendThing("^rNot good, critical error aborting message\n", thing);
    return;
  }
  sprintf(strName, "Board [%s] - Reply", boardList[board].bFileName);
  EDITSTR(thing, boardMsg->bText, 2048, strName, EP_ENDLF|EP_IMMNEW);
  EDITFLAG(thing, &boardList[board].bFlag, B_UNSAVED);
}

CMDPROC(CmdErase) {
  LWORD     board;
  BYTE      buf[256];
  BOARDMSG *boardMsg;
  LWORD     i;
  BYTE      srcKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  THING    *found;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry Erase <object> <msg #>\n", thing);
    return;
  }
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = CharThingFind(thing, srcKey, -1, thing, TF_OBJ, &srcOffset);
  if (!found)
    found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_OBJ, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }
  if (Obj(found)->oTemplate->oType!=OTYPE_BOARD) {
    SendThing("^bThat sort of thing can have anything written on it.\n", thing);
    return;
  }
  board = BoardOf(OBJECTGETFIELD(found, OF_BOARD_BVIRTUAL));
  if (board == -1) {
    SendThing("^wTell whoever created this thing that the virtual # is invalid\n", thing);
    return;
  }

  /* check msg # */
  i = -1;
  sscanf(cmd, " %ld", &i);
  i -= 1;
  if (i<0 || i>= boardList[board].bIndex.iNum) {
    SendThing("^wThere is no message with that number\n", thing);
    return;
  }
  boardMsg = BoardMsg(boardList[board].bIndex.iThing[i]);

  SendThing("^wErasing:\n", thing);
  sprintf(buf, "^gMessage Number: ^G[^b%ld/%ld^G]\n", i+1, boardList[board].bIndex.iNum); 
  SendThing(buf, thing);
  sprintf(buf, "^gAuthor:         ^G[^b%s^G]\n", boardMsg->bAuthor->sText);
  SendThing(buf, thing);
  SendThing(   "^gMessage Title:  ^G[^g", thing);
  SendThing(boardMsg->bTitle->sText, thing);
  SendThing("^G]\n", thing);

  BoardMsgDelete(board, i);
}

int InvScan(THING *thing, THING *scanner, BYTE *cmd) {
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;
  FLAG   cFlag;
  LWORD  cValue;
  FLAG   sFlag;
  LWORD  sBio;
  LWORD  sMax;
  LWORD  sChip;
  LWORD  sLeft;
  BYTE   buf[256];
  THING *world;

  if (!*cmd) {
    SendThing("^wPerhaps you should specify something to scan\n", thing);
    return TRUE;
  }

  /* Scan, yes fine but What? */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  search = Base(thing)->bInside;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return TRUE;
  }

  while (found && srcNum!=0) {
    /* Scan Found */
    while(1) {
      if (found->tType != TTYPE_OBJ
        ||Obj(found)->oTemplate->oType != OTYPE_CONTAINER) {
        SendThing("^pYou detect nothing of any particular interest\n", thing);
        break;
      } 
      cFlag  = OBJECTGETFIELD(found, OF_CONTAINER_CFLAG);
      cValue = OBJECTGETFIELD(found, OF_CONTAINER_SVALUE);
      if (!BIT(cFlag, OCF_CORPSE)) {
        SendThing("^CYou detect nothing of any particular interest\n", thing);
        break;
      }
      /* Check out their scanner */
      sFlag = OBJECTGETFIELD(scanner, OF_SCANNER_SFLAG);
      sMax  = OBJECTGETFIELD(scanner, OF_SCANNER_MAX);
      sBio  = OBJECTGETFIELD(scanner, OF_SCANNER_BIO);
      sChip = OBJECTGETFIELD(scanner, OF_SCANNER_CHIP);
      sLeft = sMax - sBio - sChip;

      /* Check if they can do that */
      if (BIT(cFlag, OCF_ELECTRONIC)) {
        if (!BIT(sFlag, OSF_SCANCHIP)) {
          SendThing("^wI'm afraid your scanner isnt capable of scanning that\n", thing);
          break;
        }
      } else {
        if (!BIT(sFlag, OSF_SCANBIO)) {
          SendThing("^wI'm afraid your scanner isnt capable of scanning that\n", thing);
          break;
        }
      }

      /* Check if they have room */
      if (sLeft < cValue) {
        SendThing("^wI'm afraid your scanner doesnt have enough memory free to scan that\n", thing);
        break;
      }

      SendAction("^wYou scan ^g$N\n",
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^C$n scans ^g$N\n",
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);

      /* guard against no gain zones etc here */
      world = Base(thing)->bInside;

      OBJECTSETFIELD(found, OF_CONTAINER_SVALUE, 0);
      if (BIT(cFlag, OCF_ELECTRONIC)) {
        sChip += cValue;
        if (world->tType==TTYPE_WLD 
         && BIT(areaList[Wld(world)->wArea].aResetFlag, RF_NOMONEY)) {
          sprintf(buf, "^YYou ^rwould ^Yhave obtained ^y%ld ^Ycredits worth of chip technology\n", cValue);
        } else {
          OBJECTSETFIELD(scanner, OF_SCANNER_CHIP, sChip);
          sprintf(buf, "^YYou obtain ^y%ld ^Ycredits worth of chip technology\n", cValue);
        }
      } else {
        sBio += cValue;
        if (world->tType==TTYPE_WLD 
         && BIT(areaList[Wld(world)->wArea].aResetFlag, RF_NOMONEY)) {
          sprintf(buf, "^YYou ^rwould ^Yhave obtained ^y%ld ^Ycredits worth of biological data\n", cValue);
        } else {
          OBJECTSETFIELD(scanner, OF_SCANNER_BIO, sBio);
          sprintf(buf, "^YYou obtain ^y%ld ^Ycredits worth of biological data\n", cValue);
        }
      }
      SendThing(buf, thing);

      if (!cValue) {
        SendThing("^wArgh! Nothing left worth scanning, someone beat you to it\n", thing);
      }
      break;
    }
    
    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJ|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
  return TRUE;
}

/* for now we'll just make it so ya gotta hold it ta use it... */
CMDPROC(CmdUse) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;
  LWORD  apply;
  LWORD  aType;
  LWORD  aValue;
  THING *ammo = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;
  LWORD  success;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  search = thing;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  if (!found) {
    search = Base(thing)->bInside;
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJ, &srcOffset);
  }
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }

  while (found && srcNum!=0) {
    ammo = ObjectGetAmmo(found, &ammoType, &ammoUse, &ammoLeft);

    /* Block if this is carry2use and we're not carrying it */
    if (BIT(Obj(found)->oAct, OA_CARRY2USE) && search != thing) {
      SendAction("^wI'm afraid you must be carrying that to use it.\n",
        thing, found, SEND_SRC|SEND_CAPFIRST);
    }

    /* Block if we are out of ammo */
    else if (ammoType && !ammo) {
      SendAction("^wI'm afraid ^g$N is unloaded\n",
        thing, found, SEND_SRC|SEND_CAPFIRST);
    }

    else if (ammoUse > ammoLeft) {
      SendAction("^wI'm afraid ^g$N just ran outta steam\n",
        thing, found, SEND_SRC|SEND_CAPFIRST);
    }

    /* Give code Parser a crack at this */
    else if (!CodeParseUse(thing, found)) {

      /* Send they use the item message */
      SendAction("^bYou use ^g$N\n", 
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^b$n uses ^g$N\n",   
        thing, found, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);

      success = FALSE;
      /* Perform the objects actions */
      for (apply=0; apply < OBJECT_MAX_APPLY; apply++) {
        aType  = Obj(found)->oApply[apply].aType;
        aValue = Obj(found)->oApply[apply].aValue;
        success |= Effect(applyList[aType].aEffect, applyList[aType].aTarget, thing, cmd, aValue);
      }
      /* Special actions due to the object type */
      switch (Obj(found)->oTemplate->oType) {
      case OTYPE_SCANNER:
        /* Find the target etc, should I do this with an effect? */
        success |= InvScan(thing, found, cmd);
        break;
      case OTYPE_DRUG:
        if (thing->tType == TTYPE_PLR) {
          Plr(thing)->pIntox += OBJECTGETFIELD(found, OF_DRUG_INTOX);
          MAXSET(Plr(thing)->pIntox, raceList[Plr(thing)->pRace].rMaxIntox);
          success = TRUE;
        }
        break;
      }
      /* after the fact script code */
      success |= CodeParseAfterUse(thing, found);

      /* affect align if good or evil */
      if (success && thing->tType==TTYPE_PLR) {
        if (BIT(Obj(found)->oAct, OA_EVIL)) {
          if (Character(thing)->cAura > 400) {
            Character(thing)->cAura -=1;
          }
        }
        if (BIT(Obj(found)->oAct, OA_GOOD)) {
          if (Character(thing)->cAura < -400) {
            Character(thing)->cAura +=1;
          }
        }
      }

      /* tweak ammo / destroy object */
      if (!success) {
        SendAction("^wBut nothing much seems to happen\n", 
          thing, found, SEND_SRC|SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      } else if (!ammo) {
      /* Destroy object or decrement ammo */
        if (ammoUse>=0) THINGFREE(found);
      } else {
        ObjectUseAmmo(found);
      }
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJ|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}







