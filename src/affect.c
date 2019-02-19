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
#include <time.h>

#include "crimson2.h"
#include "macro.h"
#include "mem.h"
#include "str.h"
#include "log.h"
#include "file.h"
#include "queue.h"
#include "thing.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "player.h"
#include "fight.h"
#include "affect.h"
#include "effect.h"
#include "cmd_inv.h"
#include "skill.h"

/* the reason that we have Effects, and Affects is to prevent recursion, this allows
   easy implementation of items that have an Effect (ie could cast a spell), apply the
   invisibility flag, which is an Affect... get it? 

   put it this way in the old mud it was a real nightmare to make applies that
   cast a spell, since the spell would in turn create an apply which generally
   leads to an infinite loop situation
 */
 

#define AFFECT_ALLOC_SIZE 4096
 

/* list of affect flags */
BYTE *affectList[] = {
  "GROUP",
  "IMPROVED",
  "INVIS",
  "BREATHWATER",
  "DARKVISION",
  "SEEINVIS",
  "REGENERATION",
  "ENDURANCE",
  "HIDDEN",
  "PHASEWALK",
  "WATERWALK",
  "VACUUMWALK",
  "!FIGHTSTART",
  "DOMINATED",
  "SNEAK",
  "SENSELIFE",
  "PHASEDOOR",
  ""
};

void AffectThing(BYTE mode, THING *thing, WORD aNum, LWORD aValue) {
  if (mode==AFFECT_FREE) {
    aValue = -aValue;
  }

  switch (aNum) {
  case AFFECT_NONE:
    break;
  case AFFECT_STR:
    if (thing->tType == TTYPE_PLR)
      Plr(thing)->pStr += aValue;
    break;
  case AFFECT_DEX:
    if (thing->tType == TTYPE_PLR)
      Plr(thing)->pDex += aValue;
    break;
  case AFFECT_INT:
    if (thing->tType == TTYPE_PLR)
      Plr(thing)->pInt += aValue;
    break;
  case AFFECT_WIS:
    if (thing->tType == TTYPE_PLR)
      Plr(thing)->pWis += aValue;
    break;
  case AFFECT_CON:
    if (thing->tType == TTYPE_PLR)
      Plr(thing)->pCon += aValue;
    break;
  case AFFECT_HIT:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cHitPMax += aValue;
    break;
  case AFFECT_MOVE:
    if (thing->tType == TTYPE_PLR)
      Plr(thing)->pMovePMax += aValue;
    break;
  case AFFECT_POWER:
    if (thing->tType == TTYPE_PLR)
      Plr(thing)->pPowerPMax += aValue;
    break;
  case AFFECT_ARMOR:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cArmor += aValue;
    break;
  case AFFECT_HITROLL:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cHitBonus += aValue;
    break;
  case AFFECT_DAMROLL:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cDamBonus += aValue; 
    break;      
  case AFFECT_RES_PUNCTURE:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[0] += aValue; 
    break; 
  case AFFECT_RES_SLASH:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[1] += aValue; 
    break; 
  case AFFECT_RES_CONCUSSIVE:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[2] += aValue; 
    break; 
  case AFFECT_RES_HEAT:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[3] += aValue; 
    break;      
  case AFFECT_RES_EMR:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[4] += aValue; 
    break;
  case AFFECT_RES_LASER:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[5] += aValue; 
    break;
  case AFFECT_RES_PSYCHIC:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[6] += aValue; 
    break;
  case AFFECT_RES_ACID:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[7] += aValue; 
    break;
  case AFFECT_RES_POISON:
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cResist[8] += aValue; 
    break;
  case AFFECT_SPEED:
    /* Make a field in char so that this works against chars */
    if (thing->tType >= TTYPE_CHARACTER)
      Character(thing)->cSpeed += aValue;
    break;
  }
}


/* called by Effect, do not call this directly, Effect should provide error checking */
AFFECT *AffectCreate(THING *thing, WORD aNum, LWORD aValue, WORD aDuration, WORD eType) {
  AFFECT *affect;

  AffectThing(AFFECT_CREATE, thing, aNum, aValue);
  
  if (aDuration != AD_NONE) {
    /* create an affect structure and attach it */
    MEMALLOC(affect, AFFECT, AFFECT_ALLOC_SIZE);
    affect->aNum = aNum;
    affect->aEffect = eType;
    affect->aDuration = aDuration;
    affect->aValue = aValue;
    affect->aNext = Character(thing)->cAffect;
    Character(thing)->cAffect = affect;
  }
  return affect;
}

/* called by Effect, do not call this directly, Effect should provide error checking */
void AffectReplace(THING *thing, WORD aNum, LWORD aValue, WORD aDuration, WORD eType) {
  AFFECT *affect;

  affect = AffectFind(thing, eType);
  if (affect) {
    MINSET(affect->aDuration, aDuration);
    return;
  }

  AffectThing(AFFECT_CREATE, thing, aNum, aValue);
  
  if (aDuration != AD_NONE) {
    /* create an affect structure and attach it */
    MEMALLOC(affect, AFFECT, AFFECT_ALLOC_SIZE);
    affect->aNum = aNum;
    affect->aEffect = eType;
    affect->aDuration = aDuration;
    affect->aValue = aValue;
    affect->aNext = Character(thing)->cAffect;
    Character(thing)->cAffect = affect;
  }
}

/* should never be called unless Unapply was called prior */
void AffectApply(THING *thing, AFFECT *affect) {
  AffectThing(AFFECT_CREATE, thing, affect->aNum, affect->aValue);
  if (effectList[affect->aEffect].eProc)
    (*effectList[affect->aEffect].eProc)(EVENT_APPLY, thing, "", affect->aValue, NULL, affect->aEffect, affect, NULL); 
}

/* Temporarily unapplies an effect, make sure AffectApply is called after */
void AffectUnapply(THING *thing, AFFECT *affect) {
  AffectThing(AFFECT_FREE, thing, affect->aNum, affect->aValue);
  if (effectList[affect->aEffect].eProc)
    (*effectList[affect->aEffect].eProc)(EVENT_UNAPPLY, thing, "", affect->aValue, NULL, affect->aEffect, affect, NULL); 
}

/* Read a single affect from a file, typically a player */
void AffectRead(THING *thing, AFFECT *affect, FILE *file) {
  LWORD block;
  if (effectList[affect->aEffect].eProc)
    block = (*effectList[affect->aEffect].eProc)(EVENT_READ, thing, "", affect->aValue, NULL, affect->aEffect, affect, file); 
  if (!block) {
    AffectReadPrimitive(thing, affect, file);
  }
}

/* Write a single affect to a file, typically a player */
void AffectWrite(THING *thing, AFFECT *affect, FILE *file) {
  LWORD block;

  if (effectList[affect->aEffect].eProc)
    block = (*effectList[affect->aEffect].eProc)(EVENT_WRITE, thing, "", affect->aValue, NULL, affect->aEffect, affect, file); 
  if (!block) {
    AffectWritePrimitive(thing, affect, file);
  }
}

void AffectReadPrimitive(THING *thing, AFFECT *affect, FILE *file) {
  affect->aNum = FileByteRead(file);
  fscanf(file, "%hd %ld\n", &affect->aDuration, &affect->aValue);
  AffectApply(thing, affect);
}

void AffectWritePrimitive(THING *thing, AFFECT *affect, FILE *file) {
  fprintf(file, "%s ", effectList[affect->aEffect].eName);
  FileByteWrite(file, affect->aNum, ' ');
  fprintf(file, "%hd %ld\n", affect->aDuration, affect->aValue);
}

void AffectApplyAll(THING *thing) {
  THING  *equip;
  AFFECT *affect;

  /* reapply all equips via InvApply(thing, equip); */
  for (equip=thing->tContain; equip; equip=equip->tNext)
    if (equip->tType==TTYPE_OBJ && Obj(equip)->oEquip)
      InvApply(thing, equip);

  /* reapply all affects vi AffectApply(thing, affect); */
  for (affect=Character(thing)->cAffect; affect; affect=affect->aNext)
    AffectApply(thing, affect);
}

void AffectUnapplyAll(THING *thing) {
  THING  *equip;
  AFFECT *affect;

  /* unapply all equips via InvApply(thing, equip); */
  for (equip=thing->tContain; equip; equip=equip->tNext)
    if (equip->tType==TTYPE_OBJ && Obj(equip)->oEquip)
      InvUnapply(thing, equip);

  /* unapply all affects vi AffectApply(thing, affect); */
  for (affect=Character(thing)->cAffect; affect; affect=affect->aNext)
    AffectUnapply(thing, affect);
}

/* Turf an effect, dont give any messages */
void AffectFree(THING *thing, AFFECT *affect) {
  AFFECT *i;


  if (!thing || !affect) return;

  AffectThing(AFFECT_FREE, thing, affect->aNum, affect->aValue);
  if (Character(thing)->cAffect == affect) {
    Character(thing)->cAffect = affect->aNext;
  } else {
    for (i=Character(thing)->cAffect; i&&i->aNext!=affect; i=i->aNext);
    if (!i) {
      Log(LOG_ERROR, "AffectFree(affect.c): Pointer not found\n");
    } else {
      i->aNext = affect->aNext;
    }
  }
  if (effectList[affect->aEffect].eProc)
    (*effectList[affect->aEffect].eProc)(EVENT_FREE, thing, "", affect->aValue, NULL, affect->aEffect, NULL, NULL); 
  MEMFREE(affect, AFFECT);
}

AFFECT *AffectFind(THING *thing, LWORD effect) {
  AFFECT *i;

  for (i=Character(thing)->cAffect; i; i=i->aNext) {
    if (i->aEffect == effect) 
      break;
  }
  return i;
}

/* Expire an affect noisily */
void AffectRemove(THING *thing, AFFECT *affect) {
  if (effectList[affect->aEffect].eProc) {
    ThingSetEvent(thing);
    (*effectList[affect->aEffect].eProc)(EVENT_UNEFFECT, 
                                    thing, 
                                    "", 
                                    affect->aValue, 
                                    NULL, 
                                    affect->aEffect,
                                    NULL, 
                                    NULL); 
  }
  /* If the thing was moved to a new room or was free'd (killed) then it 
   * will no longer be an event.
   * Used (among other places) for SpiritWalk which creates a one hit
   * point mob for the player to use as a body. When SpiritWalk
   * is called above with the UNEFFECT flag, the thing pointer is 
   * no longer valid for the code below (ie dangling)
   */
  if (ThingIsEvent(thing)) {
    AffectFree(thing, affect);
    ThingDeleteEvent(thing);
  }
}

/* Called by the other Tick procedures ie MobileTick and PlayerTick */
LWORD AffectTick(THING *thing) {
  AFFECT *i;
  AFFECT *next;
  LWORD   damage;

  for (i=Character(thing)->cAffect; i; i=next) {
    next = i->aNext;
    /* Poisoned people take damage every tick */
    if (i->aEffect == EFFECT_POISON){
      damage = i->aValue;
      damage -= CharGetResist(thing, FD_POISON)/10;
      MINSET(damage, 1);
      /* if its killed by poison whoever its fighting gets the xp */
      if (FightDamagePrimitive(Character(thing)->cFight, thing, damage)) return TRUE;
    }
    if (i->aDuration > 0) {
      i->aDuration -= 1;
      if (i->aDuration ==0) {
         AffectRemove(thing, i);
      }
    }
  }
  return FALSE;
}
