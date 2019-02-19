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

#include "crimson2.h"
#include "macro.h"
#include "mem.h"
#include "log.h"
#include "str.h"
#include "index.h"
#include "edit.h"
#include "queue.h"
#include "history.h"
#include "socket.h"
#include "send.h"
#include "thing.h"
#include "parse.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "group.h"
#include "mobile.h"
#include "player.h"
#include "skill.h"
#include "fight.h"
#include "affect.h"
#include "cmd_inv.h"
#include "cmd_move.h"
#include "effect.h"

#define EFFECT_MAX_TARGET 16 /* pick a power of two here */


struct EffectListType effectList[] = {
/*  Name              EffectProc                            Associated Constant */
  { "NONE",           NULL,                                 EFFECT_NONE },
  { "UNCODED",        EffectUncoded,                        EFFECT_UNCODED },

/* Body */
  { "CELL-REPAIR",    EffectCellRepair,                     EFFECT_CELL_REPAIR },
  { "REFRESH",        EffectRefresh,                        EFFECT_REFRESH },
  { "ENDURANCE",      EffectEndurance,                      EFFECT_ENDURANCE },
  { "BREATHWATER",    EffectBreathwater,                    EFFECT_BREATHWATER },
  { "STRENGTH",       EffectStrength,                       EFFECT_STRENGTH },
  { "DARKVISION",     EffectDarkvision,                     EFFECT_DARKVISION },
  { "SLOW-POISON",    EffectSlowPoison,                     EFFECT_SLOW_POISON },
  { "CURE-POISON",    EffectCurePoison,                     EFFECT_CURE_POISON },
  { "POISON",         EffectPoison,                         EFFECT_POISON },
  { "HEAL-MINOR",     EffectHealMinor,                      EFFECT_HEAL_MINOR },
  { "REGENERATION",   EffectRegeneration,                   EFFECT_REGENERATION },
  { "HEAL-MAJOR",     EffectHealMajor,                      EFFECT_HEAL_MAJOR },
  { "DEXTERITY",      EffectDexterity,                      EFFECT_DEXTERITY },
  { "CONSTITUTION",   EffectConstitution,                   EFFECT_CONSTITUTION },
  { "HASTE",          EffectHaste,                          EFFECT_HASTE },
  { "QUENCH",         EffectQuench,                         EFFECT_QUENCH },
  { "SUSTENANCE",     EffectSustenance,                     EFFECT_SUSTENANCE },
  { "ACIDTOUCH",      EffectAcidtouch,                      EFFECT_ACIDTOUCH },
  { "POISONTOUCH",    EffectPoisontouch,                    EFFECT_POISONTOUCH },

  /* Telekinetic */                     
  { "CRUSH",          EffectCrush,                          EFFECT_CRUSH },
  { "FORCESTORM",     EffectForcestorm,                     EFFECT_FORCESTORM },
  { "GHOSTFIST",      EffectGhostfist,                      EFFECT_GHOSTFIST },
  { "KINETIC-SHIELD", EffectKineticShield,                  EFFECT_KINETIC_SHIELD },
  { "IMMOBILIZE",     EffectUncoded,                        EFFECT_IMMOBILIZE },
  { "HEARTSTOP",      EffectUncoded,                        EFFECT_HEARTSTOP },
  { "ASPHYXIATE",     EffectUncoded,                        EFFECT_ASPHYXIATE },
  { "INVISIBILITY",   EffectInvisibility,                   EFFECT_INVISIBILITY },
  { "SLOW",           EffectSlow,                           EFFECT_SLOW },
  { "IMPROVEDINVIS",  EffectImprovedInvis,                  EFFECT_IMPROVEDINVIS },
  { "VACUUMWALK",     EffectVacuumwalk,                     EFFECT_VACUUMWALK },
  
  /* Telepathy */                     
  { "PHANTOMEAR",     EffectUncoded,                        EFFECT_PHANTOMEAR },
  { "PHANTOMEYE",     EffectUncoded,                        EFFECT_PHANTOMEYE },
  { "MINDLINK",       EffectUncoded,                        EFFECT_MINDLINK },
  { "DOMINATION",     EffectDomination,                     EFFECT_DOMINATION },
  { "THOUGHTBLADE",   EffectThoughtblade,                   EFFECT_THOUGHTBLADE },
  { "MINDCRUSH",      EffectMindcrush,                      EFFECT_MINDCRUSH },
  { "DEATHDREAM",     EffectDeathdream,                     EFFECT_DEATHDREAM },
  { "MINDSHIELD",     EffectMindshield,                     EFFECT_MINDSHIELD },
  { "SLEEP",          EffectUncoded,                        EFFECT_SLEEP },
  { "BERSERK",        EffectBerserk,                        EFFECT_BERSERK },
  { "MINDCLEAR",      EffectMindclear,                      EFFECT_MINDCLEAR },

  /* Apportation */                     
  { "TELEPORT",       EffectUncoded,                        EFFECT_TELEPORT },
  { "SUMMON",         EffectSummon,                         EFFECT_SUMMON },
  { "SUCCOR",         EffectSuccor,                         EFFECT_SUCCOR },
  { "BANISH",         EffectUncoded,                        EFFECT_BANISH },
  { "DISRUPTDOOR",    EffectUncoded,                        EFFECT_DISRUPTDOOR },
  { "PHASEDOOR",      EffectPhasedoor,                      EFFECT_PHASEDOOR },
  { "PHASEWALK",      EffectPhasewalk,                      EFFECT_PHASEWALK },
  { "PHANTOMPOCKET",  EffectPhantompocket,                  EFFECT_PHANTOMPOCKET },
  { "WATERWALK",      EffectWaterwalk,                      EFFECT_WATERWALK },
  { "TELETRACK",      EffectTeletrack,                      EFFECT_TELETRACK },
  { "RECALL",         EffectRecall,                         EFFECT_RECALL },
  { "MARK",           EffectMark,                           EFFECT_MARK },
  { "TRANSLOCATE",    EffectTranslocate,                    EFFECT_TRANSLOCATE },

  /* Spirit */                     
  { "SPIRITWALK",     EffectSpiritwalk,                     EFFECT_SPIRITWALK },
  { "SENSELIFE",      EffectSenseLife,                      EFFECT_SENSELIFE },
  { "SEEINVISIBLE",   EffectSeeinvisible,                   EFFECT_SEEINVISIBLE },
  { "LUCKSHIELD",     EffectLuckshield,                     EFFECT_LUCKSHIELD },
  { "IDENTIFY",       EffectIdentify,                       EFFECT_IDENTIFY },
  { "STAT",           EffectStat,                           EFFECT_STAT },
  { "LUCKYHITS",      EffectLuckyhits,                      EFFECT_LUCKYHITS },
  { "LUCKYDAMAGE",    EffectLuckydamage,                    EFFECT_LUCKYDAMAGE },

  /* Pyrokinetic */                     
  { "BURNINGFIST",    EffectBurningfist,                    EFFECT_BURNINGFIST },
  { "FLAMESTRIKE",    EffectFlamestrike,                    EFFECT_FLAMESTRIKE },
  { "INCINERATE",     EffectIncinerate,                     EFFECT_INCINERATE },
  { "IGNITE",         EffectUncoded,                        EFFECT_IGNITE },
  { "HEATSHIELD",     EffectHeatshield,                     EFFECT_HEATSHIELD },
  { "FIREBLADE",      EffectFireblade,                      EFFECT_FIREBLADE },
  { "FIRESHIELD",     EffectFireshield,                     EFFECT_FIRESHIELD },
  { "FIREARMOR",      EffectFirearmor,                      EFFECT_FIREARMOR },

  /* Misc. */
  { "REVITALIZE",     EffectRevitalize,                     EFFECT_REVITALIZE },

  { "", NULL, 0 }
};

INDEX tarIndex; /* passed to effect procs */

void EffectInit(void) {
  LWORD i;
  BYTE  buf[256];

  for(i=0;*effectList[i].eName;i++) {
    if (effectList[i].eCheck!=i) {
      sprintf(buf,
        "EffectInit: Mismatched constant for entry %3ld:%s\n",
        i,
        effectList[i].eName);
      Log(LOG_ERROR, buf);
    }
  }
  IndexInit(&tarIndex, EFFECT_MAX_TARGET*4, "tarIndex", 0);
}

/* data varies by effect, but its the value if Effect was called by apply */
/* called by CmdUse, InvEquip, CmdCast , if and only if called by InvEquip
   should cmd=NULL */
/* be carefull of what target flags you pass the entries that directly
   cause an affect without creating an AFFECT structure as you can lose
   track of things easily */
LWORD Effect(WORD effect, FLAG tarFlag, THING *thing, BYTE *cmd, LWORD data) {
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;
  FLAG   searchFlag;

  /* 
   * do direct affects - no AFFECT structure - only for equipped objs,
   * ie, TAR_AFFECT is implicitly TAR_ON_WEAR
   */
  if (tarFlag==TAR_AFFECT && !cmd) {
    AffectThing(AFFECT_CREATE, thing, effect, data);
    return TRUE;
  }

  /* if we are equipping a tar_on_war item target ourself */
  if (!cmd || BIT(tarFlag, TAR_ON_WEAR)) BITSET(tarFlag, TAR_SELF_DEF);

  IndexClear(&tarIndex);

  /* first off, check for defaults */
  if (!cmd || !*cmd) {
    if (!cmd && !BIT(tarFlag, TAR_ON_WEAR)) return FALSE;
    if (BIT(tarFlag, TAR_NODEFAULT)) return FALSE;

    if (BIT(tarFlag, TAR_SELF_DEF))
      IndexAppend(&tarIndex, thing);

    if (BIT(tarFlag, TAR_FIGHT_DEF) && Character(thing)->cFight)
      IndexAppend(&tarIndex, Character(thing)->cFight);

    if (BIT(tarFlag, TAR_GROUP_DEF) 
        /*&& BIT(Character(thing)->cAffectFlag, AF_GROUP)*/
        && Character(thing)->cLead) {
      for (found = Base(thing)->bInside->tContain; found; found=found->tNext) {
        if ((found != thing)
        /*&& Character(found)->cLead == Character(thing)->cLead
        && BIT(Character(found)->cAffectFlag, AF_GROUP))*/
        && GroupIsGroupedMember(found,thing))
          IndexAppend(&tarIndex, found);
      }
    }
    
    if (BIT(tarFlag, TAR_GROUPWLD_DEF)
        /*&& BIT(Character(thing)->cAffectFlag, AF_GROUP)*/
        && Character(thing)->cLead) {
      for (found = GroupGetHighestLeader(thing); found; found=Character(found)->cFollow) {
        if ((found != thing)
          && GroupIsGroupedMember(found,thing))
          IndexAppend(&tarIndex, found);
      }
    }

    if (BIT(tarFlag, TAR_NOTGROUP_DEF)) {
      if (/*!BIT(Character(thing)->cAffectFlag, AF_GROUP) ||*/ !Character(thing)->cLead) {
        BITSET(tarFlag, TAR_NOTSELF_DEF);
      } else {
        for (found = Base(thing)->bInside->tContain; found; found=found->tNext) {
          /*if (found != thing && found !=Character(thing)->cLead)*/
          if (!GroupIsGroupedMember(found,thing))
            IndexAppend(&tarIndex, found);
        }
      }
    }

    if (BIT(tarFlag, TAR_NOTSELF_DEF)) {
        for (found = Base(thing)->bInside->tContain; found; found=found->tNext) {
          if (found != thing)
            IndexAppend(&tarIndex, found);
        }
    }

  } else if (tarFlag) { /* lets see who they want to target */
    cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

    found = NULL;
    searchFlag = 0;
    search = thing;
    if (BIT(tarFlag, TAR_OBJ_INV))  BITSET(searchFlag, TF_OBJINV);
    if (BIT(tarFlag, TAR_PLR_ROOM)) BITSET(searchFlag, TF_PLR);
    if (BIT(tarFlag, TAR_MOB_ROOM)) BITSET(searchFlag, TF_MOB);
    if (searchFlag)
      found = ThingFind(srcKey, -1, search, searchFlag, &srcOffset);

    if (!found) {
      search = Base(thing)->bInside;
      searchFlag = 0;
      if (BIT(tarFlag, TAR_PLR_ROOM)) BITSET(searchFlag, TF_PLR);
      if (BIT(tarFlag, TAR_PLR_WLD))  BITSET(searchFlag, TF_PLR_WLD);
      if (BIT(tarFlag, TAR_MOB_ROOM)) BITSET(searchFlag, TF_MOB);
      if (BIT(tarFlag, TAR_MOB_WLD))  BITSET(searchFlag, TF_MOB_WLD);
      if (BIT(tarFlag, TAR_OBJ_ROOM)) BITSET(searchFlag, TF_OBJ);
      if (BIT(tarFlag, TAR_OBJ_WLD))  BITSET(searchFlag, TF_OBJ_WLD);
      if (searchFlag)
        found = ThingFind(srcKey, -1, search, searchFlag, &srcOffset);
    }
    if (!found) return FALSE;

    while(found && srcNum!=0 && tarIndex.iNum<EFFECT_MAX_TARGET) {
      IndexAppend(&tarIndex, found);

      if (!BIT(tarFlag, TAR_MULTIPLE)) break; /* only allow multiple sels where valid */
      found = ThingFind(srcKey, -1, search, searchFlag, &srcOffset);
      if (srcNum>0) srcNum--;
    }
  }
  if (tarIndex.iNum == 0) return FALSE;

  if (effectList[effect].eProc)
    (*effectList[effect].eProc)(EVENT_EFFECT, thing, cmd, data, &tarIndex, effect, NULL, NULL);
  return TRUE;
}

/* Called by the unequip routine */
/* only TAR_SELF_DEF && TAR_ON_WEAR stuff should apply direct mods to abilities
   and unlimited durations */
LWORD EffectFree(WORD effect, FLAG tarFlag, THING *thing, LWORD data) {
  AFFECT *affect;

  if (tarFlag==TAR_AFFECT) {
    AffectThing(AFFECT_FREE, thing, effect, data);
    return TRUE;
  }

  else if (BIT(tarFlag,TAR_ON_WEAR)) {
    affect = AffectFind(thing, effect);
    if (affect) AffectFree(thing, affect);
    return TRUE;
  }

  return FALSE;
}

/* Effect Procedures follow, in general they should call AffectCreate to 
   apply flags and modifiers. When the duration expires, the affects will
   automaticly be removed by AffectTick. (which is called by the appropriate
   Tick() procedure - MobileTick, PlayerTick etc). The Effect proc is also 
   given a chance to handle things as it is passed an event of UNEFFECT. 
   Note that tarIndex is null in this case!
 */

/* Uncoded as of yet */
EFFECTPROC(EffectUncoded) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    /* Let people know what happened */
    SendAction("^cSadly, the EffectProc for that is still uncoded.\n",  
      thing, NULL, SEND_SRC);
  } break;
  
  }
  return FALSE;
}

/* Body */
/* EFFECT_CELL_REPAIR - more hit points */
EFFECTPROC(EffectCellRepair) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD hitMax;
    LWORD bonus;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Let people know what happened */
      SendAction("^cYou cell repair $N.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n cell repairs you.\n",  
        thing, tarIndex->iThing[i], SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^c$n cell repairs $N.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* heal them */
      hitMax = CharGetHitPMax(tarIndex->iThing[i]);
      bonus = Dice(data/10+1,4) + data/10 + 1;
      Character(tarIndex->iThing[i])->cHitP += bonus;
      MAXSET(Character(tarIndex->iThing[i])->cHitP, hitMax);
    }
  } break;

  }
  return FALSE;
}

/* EFFECT_REFRESH - more move points */
EFFECTPROC(EffectRefresh) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD moveMax;
    LWORD bonus;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Let people know what happened */
      SendAction("^cYou refresh $N.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n refreshes you.\n",  
        thing, tarIndex->iThing[i], SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^c$n refreshes $N.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* heal them */
      moveMax = CharGetMovePMax(tarIndex->iThing[i]);
      bonus = Dice(data/10+1,4) + data/10 + 1;
      Character(tarIndex->iThing[i])->cMoveP += bonus;
      MAXSET(Character(tarIndex->iThing[i])->cMoveP, moveMax);
    }
  } break;

  }
  return FALSE;
}

/* EFFECT_ENDURANCE - set endurance flag - half move pt expenditure*/
/* see in the dark */
EFFECTPROC(EffectEndurance) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_ENDURANCE)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        continue;
      }
      /* Let people know what happened */
      SendAction("^cYou feel like you could run all day without tiring.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_ENDURANCE);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cSuddenly you feel a little winded.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_ENDURANCE);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_ENDURANCE);
    break;

  }
  return FALSE;
}

/* Typical sort of general affect spell */
EFFECTPROC(EffectBreathwater) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD duration = (data>=0) ? data/20+20 : -1;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_BREATHWATER)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        continue;
      }
      /* Let people know what happened */
      SendAction("^cYou start to breath a little easier.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, duration, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_BREATHWATER);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYou suddenly find it a little harder to breath.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_BREATHWATER);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_BREATHWATER);
    break;

  }
  return FALSE;
}

/* more strength for a while */
EFFECTPROC(EffectStrength) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel a little stronger.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(2,10) + Number(0,data/30);
      AffectCreate(tarIndex->iThing[i], AFFECT_STR, bonus, data/10+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendThing("^cYou feel a little weaker.\n", thing);
  } break;

  }
  return FALSE;
}

/* see in the dark */
EFFECTPROC(EffectDarkvision) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_DARKVISION)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendThing("^cThe light suddenly seems a little brighter.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_DARKVISION);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cEverything suddenly gets a little darker.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_DARKVISION);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_DARKVISION);
    break;

  }
  return FALSE;
}

/* higher resistance towards poison for a while */
EFFECTPROC(EffectSlowPoison) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel a little tougher.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(2,10) + Number(0,data/30);
      AffectCreate(tarIndex->iThing[i], AFFECT_RES_POISON, bonus, data/10+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendThing("^cYou feel a little weaker.\n", thing);
  } break;

  }
  return FALSE;
}

/* EFFECT_CURE_POISON    */
/* Remove a single poison effect */
/* less intox */
EFFECTPROC(EffectCurePoison) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    AFFECT *affect;

    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) {
        affect->aValue -= (data/10+1);
        if (affect->aValue < 0) AffectFree(tarIndex->iThing[i], affect);
        /* Let people know what happened */
        SendAction("^cYou help reduce the poisons effect on $n.\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        SendAction("^c$n reduces the effect of your poison.\n",  
          thing, tarIndex->iThing[i], SEND_DST);
      } else {
        SendAction("^cNothing much seems to happen.\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
      }
    }
  } break;

  }
  return FALSE;
}

/* Generic poison them proc */
EFFECTPROC(EffectPoison) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   damDice  = (data/10) + 1;
    LWORD   poison;
    AFFECT *affect;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      damDice = MAXV(1, damDice);
      poison = Dice(damDice, 3);
      if (CharDodgeCheck(tarIndex->iThing[i], -data/2))
        poison >>= 1;
      MINSET(poison, 1);
      affect = AffectFind(tarIndex->iThing[i], EFFECT_POISON);
      if (affect) {
        affect->aValue += poison;
        MINSET(affect->aDuration, data/10+10); 
      } else
        AffectCreate(tarIndex->iThing[i], AFFECT_NONE, poison, data/10+10, EFFECT_POISON);
    }
  } break;

  }
  return FALSE;
}


/* EFFECT_HEAL_MINOR     */
/* better at healing than cell-repair, also cure poison */
EFFECTPROC(EffectHealMinor) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD hitMax;
    LWORD bonus;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Let people know what happened */
      SendAction("^cYou heal $n.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n heals you.\n",  
        thing, tarIndex->iThing[i], SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^c$n heals $N.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* heal them */
      hitMax = CharGetHitPMax(tarIndex->iThing[i]);
      bonus = Dice(data/10+1,6) + data/10 + 10;
      Character(tarIndex->iThing[i])->cHitP += bonus;
      MAXSET(Character(tarIndex->iThing[i])->cHitP, hitMax);
    }
  } break;

  }
  return FALSE;
}


/* Get your points back faster */
EFFECTPROC(EffectRegeneration) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_REGENERATION)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendAction("^cYour metabolism speeds up.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_REGENERATION);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour metabolism slows down to normal.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_REGENERATION);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_REGENERATION);
    break;

  }
  return FALSE;
}

/* EFFECT_HEAL_MAJOR     */
/* same as heal-minor but also heal move pts */
/* better at healing than cell-repair, also cure poison */
EFFECTPROC(EffectHealMajor) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD hitMax;
    LWORD moveMax;
    LWORD bonus;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Let people know what happened */
      SendAction("^cYou heal $N.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n heals you.\n",  
        thing, tarIndex->iThing[i], SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^c$n heals $N.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);

      /* heal them */
      hitMax  = CharGetHitPMax(tarIndex->iThing[i]);
      bonus = Dice(data/10+1,6) + data/10 + 25;
      Character(tarIndex->iThing[i])->cHitP += bonus;
      MAXSET(Character(tarIndex->iThing[i])->cHitP, hitMax);

      /* refresh them too */
      moveMax = CharGetMovePMax(tarIndex->iThing[i]);
      bonus = Dice(data/10+1,6) + data/10 + 25;
      Character(tarIndex->iThing[i])->cMoveP += bonus;
      MAXSET(Character(tarIndex->iThing[i])->cMoveP, moveMax);
    }
  } break;

  }
  return FALSE;
}

/* more strength for a while */
EFFECTPROC(EffectDexterity) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel a little nimbler.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(2,10) + Number(0,data/30);
      AffectCreate(tarIndex->iThing[i], AFFECT_DEX, bonus, data/10+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendThing("^cYou feel a little less nimble.\n", thing);
  } break;

  }
  return FALSE;
}

/* more strength for a while */
EFFECTPROC(EffectConstitution) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel a little tougher.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(2,10) + Number(0,data/30);
      AffectCreate(tarIndex->iThing[i], AFFECT_CON, bonus, data/10+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendThing("^cYou feel a little wimpier.\n", thing);
  } break;

  }
  return FALSE;
}

/* more speed for a while */
EFFECTPROC(EffectHaste) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel a little faster.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(5,10) + Number(0,data/2);
      AffectCreate(tarIndex->iThing[i], AFFECT_SPEED, bonus, data/20+10, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendThing("^cYou feel a little slower.\n", thing);
    break;
  }
  return FALSE;
}

/* EFFECT_QUENCH - less thirst */
EFFECTPROC(EffectQuench) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event)  {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD thirstMin;
    LWORD bonus;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Let people know what happened */
      SendAction("^cYou quench $n's thirst.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n quenches your thirst.\n",  
        thing, tarIndex->iThing[i], SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^c$n quenches $N's thirst.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* heal them */
      if (tarIndex->iThing[i]->tType == TTYPE_PLR) {
        thirstMin = -1*raceList[Plr(tarIndex->iThing[i])->pRace].rMaxThirst;
        bonus = Dice(data/20+1,3) + 5;
        Plr(tarIndex->iThing[i])->pThirst -= bonus;
        MINSET(Plr(tarIndex->iThing[i])->pThirst, thirstMin);
      }
    }
  } break;

  }
  return FALSE;
}

/* less hunger */
EFFECTPROC(EffectSustenance) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD hungerMin;
    LWORD bonus;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Let people know what happened */
      SendAction("^cYou take away $n's hunger.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n takes away your hunger.\n",  
        thing, tarIndex->iThing[i], SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^c$n take away $N's hunger.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* heal them */
      if (tarIndex->iThing[i]->tType == TTYPE_PLR) {
        hungerMin = -1*raceList[Plr(tarIndex->iThing[i])->pRace].rMaxHunger;
        bonus = Dice(data/20+1,3) + 5;
        Plr(tarIndex->iThing[i])->pHunger -= bonus;
        MINSET(Plr(tarIndex->iThing[i])->pHunger, hungerMin);
      }
    }
  } break;

  }
  return FALSE;
}

/* typical sort of attack spell - wimpy like */
EFFECTPROC(EffectAcidtouch) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 2;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_ACID);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 4) + damBonus;
      if (CharDodgeCheck(tarIndex->iThing[i], -data/2))
        damage >>= 1;
      FightDamage(thing, damage, 1, FD_ACID, effectNum, "acidic touch");
    }
  } break;

  }
  return FALSE;
}


/* typical sort of attack spell - wimpy like */
EFFECTPROC(EffectPoisontouch) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   damDice  = (data/10) + 1;
    LWORD   damBonus = 2;
    LWORD   damage;
    LWORD   poison;
    LWORD   resist;
    AFFECT *affect;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_POISON);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 4) + damBonus;
      if (CharDodgeCheck(tarIndex->iThing[i], -data/2)) {
        damage >>= 1;
      } else {
        affect = AffectFind(tarIndex->iThing[i], EFFECT_POISON);
        poison = data/20;
        MINSET(poison, 1);
        if (affect) {
          affect->aValue += poison;
          MINSET(affect->aDuration, data/10);
        } else
          AffectCreate(tarIndex->iThing[i], AFFECT_NONE, poison, data/10, EFFECT_POISON);
      }
      FightDamage(thing, damage, 1, FD_POISON, effectNum, "poisonous touch");
    }
  } break;

  }
  return FALSE;
}


/* typical sort of attack spell - 3rd grade */
EFFECTPROC(EffectCrush) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 30;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_CONCUSSIVE);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 6) + damBonus;
      if (CharDodgeCheck(tarIndex->iThing[i], -data))
        damage >>= 1;
      FightDamage(thing, damage, 1, FD_CONCUSSIVE, effectNum, "invisible vise");
    }
  } break;

  }
  return FALSE;
}


/* typical sort of attack spell - one grade better */
EFFECTPROC(EffectForcestorm) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 20;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_CONCUSSIVE);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 5) + damBonus;
      if (CharDodgeCheck(tarIndex->iThing[i], -data/2))
        damage >>= 1;
      FightDamage(thing, damage, 1, FD_CONCUSSIVE, effectNum, "invisible blow");
    }
  } break;

  }
  return FALSE;
}


/* typical sort of attack spell - wimpy like */
EFFECTPROC(EffectGhostfist) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 2;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_CONCUSSIVE);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 4) + damBonus;
      if (CharDodgeCheck(tarIndex->iThing[i], -data/2+20))
        damage >>= 1;
      FightDamage(thing, damage, 1, FD_CONCUSSIVE, effectNum, "ghostly fist");
    }
  } break;

  }
  return FALSE;
}

/* higher resistance towards kinetic energy type weapons */
EFFECTPROC(EffectKineticShield) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel somewhat sheltered.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(2,10) + Number(0,data/30);
      AffectCreate(tarIndex->iThing[i], AFFECT_RES_PUNCTURE,   bonus, data/10+20, effectNum);
      AffectCreate(tarIndex->iThing[i], AFFECT_RES_SLASH,      bonus, data/10+20, effectNum);
      AffectCreate(tarIndex->iThing[i], AFFECT_RES_CONCUSSIVE, bonus, data/10+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendThing("^cYou stop feeling sheltered.\n", thing);
  } break;

  }
  return FALSE;
}

/* EFFECT_IMMOBILIZE     */
/* paralyze somebody for a while, deadly since they wont be able to defend themselves */

/* EFFECT_HEARTSTOP      */
/* Do massive damage to someone or maybe instakill them */

/* EFFECT_ASPHYXIATE     */
/* do some damage every round for awhile */

/* Invisible */
EFFECTPROC(EffectInvisibility) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_INVIS)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendAction("^cYou abruptly fade into invisibility.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      SendAction("^c$N abruptly fades out of existence.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_INVIS);
      BITCLR(Character(tarIndex->iThing[i])->cAffectFlag, AF_IMPROVED);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYou slowly fade into existence.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n slowly fades into existence.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_INVIS);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_INVIS);
    BITCLR(Character(thing)->cAffectFlag, AF_IMPROVED);
    break;

  }
  return FALSE;
}

/* less speed for a while */
EFFECTPROC(EffectSlow) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel slowed down.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(5,10) + Number(0,data/2);
      AffectCreate(tarIndex->iThing[i], AFFECT_SPEED, -bonus, data/20+10, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendThing("^cYou feel a little faster.\n", thing);
    break;
  }
  return FALSE;
}

/* Improved Invisibility */
EFFECTPROC(EffectImprovedInvis) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_INVIS)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendAction("^cYou abruptly fade into invisibility.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      SendAction("^c$N abruptly fades out of existence.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_INVIS|AF_IMPROVED);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYou slowly fade into existence.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n slowly fades into existence.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_INVIS|AF_IMPROVED);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_INVIS|AF_IMPROVED);
    break;

  }
  return FALSE;
}

/*  EFFECT_VACUUMWALK      */
/* Walk in vacuum without damage */
EFFECTPROC(EffectVacuumwalk) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD duration = (data>=0) ? data/20+20 : -1;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_VACUUMWALK)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendThing("^cYou surround yourself with a pocket of air.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, duration, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_VACUUMWALK);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour pocket of air dissipates.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_VACUUMWALK);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_VACUUMWALK);
    break;
  }
  return FALSE;
}


  /* Telepathy */                     
/*  EFFECT_PHANTOMEAR     */
/* hear what they hear */
/*  EFFECT_PHANTOMEYE     */
/* see what they see */
/*  EFFECT_MINDLINK       */
/* get to see what they type, a god-level snoop link */


/*  EFFECT_DOMINATION     */
/* essentially charm person */
/* Typical sort of general affect spell */
EFFECTPROC(EffectDomination) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_DOMINATED)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        continue;
      }

      if (Character(tarIndex->iThing[i])->cLevel>data/10 
       || CharWillPowerCheck(tarIndex->iThing[i], -data/2)) {
        /* well well, we have attacked them */
        FightStart(thing, tarIndex->iThing[i], 1, NULL);
        continue;
      }
      /* Make 'em follow */
      CharAddFollow(tarIndex->iThing[i], thing);

      /* Let people know what happened */
      SendAction("^cYou are bent to the will of $n.\n",  
        thing, tarIndex->iThing[i], SEND_DST);

      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+5, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_DOMINATED);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour mind is now your own.\n", 
         thing, NULL, SEND_SRC);
    /* Mob should attack leader now */
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_DOMINATED);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_DOMINATED);
    break;

  }
  return FALSE;
}


/* Attack their mind - wimpy */
EFFECTPROC(EffectThoughtblade) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 2;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_PSYCHIC);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 4) + damBonus;
      if (CharWillPowerCheck(tarIndex->iThing[i], -data/2+20))
        damage >>= 1;
      
      FightDamage(thing, damage, 1, FD_PSYCHIC, effectNum, "blade of thought");
    }
  } break;

  }
  return FALSE;
}


/* Attack their mind - a bit better */
EFFECTPROC(EffectMindcrush) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 20;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_PSYCHIC);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 5) + damBonus;
      if (CharWillPowerCheck(tarIndex->iThing[i], -data/2))
        damage >>= 1;
      
      FightDamage(thing, damage, 1, FD_PSYCHIC, effectNum, "brutal mindcrush");
    }
  } break;

  }
  return FALSE;
}


/* Attack their mind - 3rd grade */
EFFECTPROC(EffectDeathdream) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 40;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_PSYCHIC);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 6) + damBonus;
      if (CharWillPowerCheck(tarIndex->iThing[i], -data))
        damage >>= 1;
      
      FightDamage(thing, damage, 1, FD_PSYCHIC, effectNum, "dreams of death");
    }
  } break;

  }
  return FALSE;
}


/* higher resistance towards kinetic energy type weapons */
EFFECTPROC(EffectMindshield) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel more determined.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(2,10) + Number(0,data/30);
      AffectCreate(tarIndex->iThing[i], AFFECT_RES_PSYCHIC,   bonus, data/10+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendThing("^cYou feel less determined.\n", thing);
  } break;

  }
  return FALSE;
}

/*  EFFECT_SLEEP          */
/* put them to sleep maybe, if we fight them, they wake up though */

/* less intox */
EFFECTPROC(EffectMindclear) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD intoxMin;
    LWORD bonus;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Let people know what happened */
      SendAction("^cYou help clear $n's mind.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n helps clear your mind.\n",  
        thing, tarIndex->iThing[i], SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^c$n helps clear $N's mind.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* heal them */
      if (tarIndex->iThing[i]->tType == TTYPE_PLR) {
        intoxMin = -1*raceList[Plr(tarIndex->iThing[i])->pRace].rMaxIntox;
        bonus = Dice(data/20+1,3) + 5;
        Plr(tarIndex->iThing[i])->pIntox -= bonus;
        MINSET(Plr(tarIndex->iThing[i])->pIntox, intoxMin);
      }
    }
  } break;

  }
  return FALSE;
}

/* Makes you do more damage, hard to hit people */
EFFECTPROC(EffectBerserk) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    LWORD i;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendAction("^cYou make $N berserk.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      SendAction("^c$n makes you berserk.\n", 
        thing, tarIndex->iThing[i], SEND_DST |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^c$n makes $N berserk.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
      AffectCreate(tarIndex->iThing[i], AFFECT_DAMROLL, data/25+1, data/30+10, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendAction("^cYou stop seeing the world through a red haze.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n stops being berserk.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  } break;

  }
  return FALSE;
}

/* Apportation */                     
/*  EFFECT_TELEPORT       */
/* go somewhere - keep in same zone - good escape spell */
                    
/*  EFFECT_SUMMON         */
/* bring someone to us */
EFFECTPROC(EffectSummon) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Transport away */
      if (Base(tarIndex->iThing[i])->bInside != Base(thing)->bInside) {
        if (tarIndex->iThing[i]->tType == TTYPE_PLR)
          SendAction(Plr(tarIndex->iThing[i])->pExit->sText, tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n dissolves into a cloud of blue sparkles.\n", tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        FightStop(tarIndex->iThing[i]);
        ThingTo(tarIndex->iThing[i], Base(thing)->bInside);
        if (tarIndex->iThing[i]->tType == TTYPE_PLR)
          SendAction(Plr(tarIndex->iThing[i])->pEnter->sText, tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n appears from a cloud of blue sparkles.\n", tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        CmdLook(thing, "");
      }
    }
  } break;

  }
  return FALSE;
}


/*  EFFECT_SUCCOR         */
/* everyone go visit the group leader */
EFFECTPROC(EffectSuccor) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    if (!Character(thing)->cLead) {
      SendThing("^wBut you have no leader to succor...\n", thing);
      break;
    }
    for (i=0; i<tarIndex->iNum; i++) {
      /* Transport away */
      if (Base(tarIndex->iThing[i])->bInside != Base(Character(thing)->cLead)->bInside) {
        if (tarIndex->iThing[i]->tType == TTYPE_PLR)
          SendAction(Plr(tarIndex->iThing[i])->pExit->sText, tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n dissolves into a cloud of blue sparkles.\n", tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        ThingTo(tarIndex->iThing[i], Base(Character(thing)->cLead)->bInside);
        FightStop(tarIndex->iThing[i]);
        if (tarIndex->iThing[i]->tType == TTYPE_PLR)
          SendAction(Plr(tarIndex->iThing[i])->pEnter->sText, tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n appears from a cloud of blue sparkles.\n", tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        CmdLook(thing, "");
      }
    }
  } break;

  }
  return FALSE;
}


/*  EFFECT_BANISH         */
/* banish some clod somewhere random (nearby) */

/*  EFFECT_DISRUPTDOOR    */
/* get rid of a phantom door prematurely */

/*  EFFECT_PHASEDOOR    */
/* walk through doors */
/* Typical sort of general affect spell */
EFFECTPROC(EffectPhasedoor) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_PHASEDOOR)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendThing("^cYou start to move a little out of phase with the rest of the world.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_PHASEDOOR);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYou are suddenly wrenched fully into the normal universe.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_PHASEDOOR);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_PHASEDOOR);
    break;

  }
  return FALSE;
}


/*  EFFECT_PHASEWALK      */
/* Walk out of phase, ignore terrain costs */
/* Typical sort of general affect spell */
EFFECTPROC(EffectPhasewalk) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_PHASEWALK)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendThing("^cYou start to move a little out of phase with the rest of the world.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_PHASEWALK);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYou are suddenly wrenched fully into the normal universe.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_PHASEWALK);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_PHASEWALK);
    break;

  }
  return FALSE;
}


/*  EFFECT_PHANTOMDOOR      */
/* create a timed portal */

/* EFFECT_PHANTOMPOCKET */
/* Create a pocket to put stuff in */
EFFECTPROC(EffectPhantompocket) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    LWORD  i;
    THING  *item;
    AFFECT *affect;
    LWORD  duration = data/10+10;
    LWORD  containerMax = data/3+20;
    
    for (i=0; i<tarIndex->iNum; i++) {
      /* Check if they have a pocket allready */
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) {
        /* strengthen duration */
        if (duration > affect->aDuration)
          affect->aDuration = duration;
        continue;
      }

      /* Create object */
      item = ObjectCreate(phantomPocketTemplate, tarIndex->iThing[i]);
      OBJECTSETFIELD(item, OF_CONTAINER_MAX, containerMax);
      
      /* If we can equip it */
      if (InvEquip(thing, item, NULL)) {
        /* Let people know what happened */
        SendAction("^cYou create a pocket-sized rip in space-time for $N\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        SendAction("^c$n creates a pocket-sized rip in space-time for you\n", 
          thing, tarIndex->iThing[i], SEND_DST |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("^c$n creates a pocket-sized rip in space-time for $N\n",  
          thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
        AffectCreate(tarIndex->iThing[i], AFFECT_NONE, data, duration, effectNum);
      } else {
        /* If we cant equip it turf the object */
        /* Let caster know their hand was full */
        SendAction("^cYou try to create a phantom pocket for $N but have no place to put it.\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        THINGFREE(item);
      }
      
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour pocket-sized hole in space-time suddenly winks out of existence.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n's pocket-sized hole in space-time suddenly winks out of existence.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    /* then fall thru to free it */
  case EVENT_FREE: {
    THING *i;
    THING *next;
    
    /* search for pocket - turf em as we find em, should only be one */
    for (i = thing->tContain; i; i=next) {
      next = i->tNext;
      if (i->tType==TTYPE_OBJ && Obj(i)->oTemplate == phantomPocketTemplate) {
        while (i->tContain) {
          /* move contents to the room */
          if (Base(thing)->bInside) {
            SendAction("^c$n winks into existence and falls to the floor\n",  
              i->tContain, NULL, SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
            ThingTo(i->tContain, Base(thing)->bInside);
          } else
            ThingFree(i->tContain);
        }
        /* Pocket is history */
        THINGFREE(i);
      }
    }
  } break;


  case EVENT_READ: {
    THING *item;
    LWORD  containerMax = affect->aValue/3+20;

    AffectReadPrimitive(thing, affect, file);
    item = ObjectCreate(phantomPocketTemplate, thing);
    OBJECTSETFIELD(item, OF_CONTAINER_MAX, containerMax);
    InvEquip(thing, item, NULL);
  } break;

  case EVENT_WRITE: {
    AffectWritePrimitive(thing, affect, file);
  } break;

  }
  return TRUE;
}


/*  EFFECT_WATERWALK      */
/* Walk on water */
EFFECTPROC(EffectWaterwalk) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_WATERWALK)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendThing("^cYou start to move a little out of phase with the rest of the world.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_WATERWALK);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYou are suddenly wrenched fully into the normal universe.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_FREE:
  case EVENT_UNAPPLY:
    BITCLR(Character(thing)->cAffectFlag, AF_WATERWALK);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_WATERWALK);
    break;

  }
  return FALSE;
}

/* everyone go to the target */
EFFECTPROC(EffectTeletrack) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   trackRange;

    for (i=0; i<tarIndex->iNum; i++) {
      /* Check if they are in range */
      trackRange = data;
     
      /* be suitably snide if they are in the same room */
      if (Base(thing)->bInside == Base(tarIndex->iThing[i])->bInside) {
        SendThing("^yTheir tracks are all around you!\n", thing);
        return FALSE;
      }
     
      /* Can only track one thing, no while loop */
      if (!ThingTrack(Base(thing)->bInside, tarIndex->iThing[i], trackRange)) {
        SendThing("^yThey're out of range I'm afraid\n", thing);
        return FALSE;
      } 
      
      /* Transport away */
      if (Base(tarIndex->iThing[i])->bInside != Base(thing)->bInside) {
        if (thing->tType == TTYPE_PLR)
          SendAction(Plr(thing)->pExit->sText, thing, NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n dissolves into a cloud of blue sparkles.\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
        ThingTo(thing, Base(tarIndex->iThing[i])->bInside);
        FightStop(thing);
        if (thing->tType == TTYPE_PLR)
          SendAction(Plr(thing)->pEnter->sText, thing, NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n appears from a cloud of blue sparkles.\n", thing, NULL, SEND_ROOM|SEND_VISIBLE);
        CmdLook(thing, "");
      }
      /* use effect flag to build target for us, should do it here ourself instead so that
         I can add MassTeletrack more conveniently but this is easy */
      return FALSE;
    }
  } break;

  }
  return FALSE;
}


/* everyone go to the target */
EFFECTPROC(EffectRecall) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    THING  *to = NULL;

    /* Check where they have marked to recall */
    if (thing->tType == TTYPE_PLR) {
      to = WorldOf(Plr(thing)->pRecallRoom);
    }
    if (!to) to = WorldOf(playerStartRoom);
     
    /* be suitably snide if they are in the same room */
    if (!to) {
      SendThing("^yYou have no place marked in memory to recall to!\n", thing);
      return FALSE;
    }

    for (i=0; i<tarIndex->iNum; i++) {
      /* Transport away */
      if (Base(tarIndex->iThing[i])->bInside != to) {
        if (tarIndex->iThing[i]->tType == TTYPE_PLR)
          SendAction(Plr(tarIndex->iThing[i])->pExit->sText, tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n dissolves into a cloud of blue sparkles.\n", tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        ThingTo(tarIndex->iThing[i], to);
        FightStop(tarIndex->iThing[i]);
        if (tarIndex->iThing[i]->tType == TTYPE_PLR)
          SendAction(Plr(tarIndex->iThing[i])->pEnter->sText, thing, NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n appears from a cloud of blue sparkles.\n", tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        CmdLook(tarIndex->iThing[i], "");
      } else {
        SendThing("^yYou try to recall but you're allready there!\n", tarIndex->iThing[i]);
      }
    }
  } break;

  }
  return FALSE;
}


/* everyone go to the target */
EFFECTPROC(EffectMark) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;

    for (i=0; i<tarIndex->iNum; i++) {
      /* be suitably snide if they are in the same room */
      if (Base(tarIndex->iThing[i])->bInside->tType != TTYPE_WLD) {
        SendThing("^yTheres nothing about this place you can mark in your memory!\n", tarIndex->iThing[i]);
        continue;
      }
     
      /* Mark away */
      if (tarIndex->iThing[i]->tType == TTYPE_PLR) {
        Plr(tarIndex->iThing[i])->pRecallRoom = Wld(Base(tarIndex->iThing[i])->bInside)->wVirtual;
        SendThing("^yYou mentally mark your surroundings so you can recall them later\n", tarIndex->iThing[i]);
      } else {
        SendThing("^yOnly Players can Mark!\n", tarIndex->iThing[i]);
      }
    }
  } break;

  }
  return FALSE;
}


/* swap current position and marked position */
EFFECTPROC(EffectTranslocate) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    THING  *to = NULL;
    LWORD   recallVirtual;

    /* be suitably snide if they are in the same room */
    if (Base(thing)->bInside->tType != TTYPE_WLD) {
      SendThing("^yTheres nothing about this place you can mark in your memory!\n", thing);
      return FALSE;
    }
    recallVirtual = Wld(Base(thing)->bInside)->wVirtual;

    /* Check where they have marked to recall */
    if (thing->tType == TTYPE_PLR) {
      to = WorldOf(Plr(thing)->pRecallRoom);
    }
    if (!to) to = WorldOf(playerStartRoom);
    Plr(thing)->pRecallRoom = recallVirtual;
    
    /* be suitably snide if they are in the same room */
    if (!to) {
      SendThing("^yYou have no place marked in memory to recall to!\n", thing);
      return FALSE;
    }

    for (i=0; i<tarIndex->iNum; i++) {
      /* Transport away */
      if (Base(tarIndex->iThing[i])->bInside != to) {
        if (tarIndex->iThing[i]->tType == TTYPE_PLR)
          SendAction(Plr(tarIndex->iThing[i])->pExit->sText, tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n dissolves into a cloud of blue sparkles.\n", tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        ThingTo(tarIndex->iThing[i], to);
        FightStop(tarIndex->iThing[i]);
        if (tarIndex->iThing[i]->tType == TTYPE_PLR)
          SendAction(Plr(tarIndex->iThing[i])->pEnter->sText, thing, NULL, SEND_ROOM|SEND_VISIBLE);
        else 
          SendAction("$n appears from a cloud of blue sparkles.\n", tarIndex->iThing[i], NULL, SEND_ROOM|SEND_VISIBLE);
        CmdLook(tarIndex->iThing[i], "");
      } else {
        SendThing("^yYou try to translocate but you're allready there!\n", tarIndex->iThing[i]);
      }
    }
  } break;

  }
  return FALSE;
}




  /* Spirit */                     
/*  EFFECT_SPIRITWALK     */
/* walk around without a body */
EFFECTPROC(EffectSpiritwalk) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    THING *spirit;
    SOCK  *sock;
    for (i=0; i<tarIndex->iNum; i++) {
      sock = BaseControlFind(tarIndex->iThing[i]);
      if (!sock) return FALSE;
      /* Let people know what happened */
      spirit = MobileCreate(spiritTemplate, Base(tarIndex->iThing[i])->bInside); 
      BaseControlFree(tarIndex->iThing[i], sock);
      BaseControlAlloc(spirit, sock);
      SendThing("^cYou feel your spirit leave your body.\n", spirit);
      SendHint("^;HINT: type ^<STOP SPIRITWALK^; to return to your body\n", spirit);
      AffectCreate(spirit, AFFECT_NONE, 0, data/10+10, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYou are abrubtly yanked back to your real body.\n", 
         thing, NULL, SEND_SRC);
    THINGFREE(thing);
    break;

  case EVENT_FREE: {
    SOCK *sock;

    /* restore link back to real body */
    sock = BaseControlFind(thing);
    if (sock) {
      BaseControlFree(thing, sock);
      BaseControlAlloc(sock->sHomeThing, sock);
    }
  } break;

  }
  return FALSE;
}


/*  EFFECT_SENSELIFE      */
/* see hidden creatures */
EFFECTPROC(EffectSenseLife) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_SENSELIFE)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendAction("^cYou start to see the world from a loftier plane.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_SENSELIFE);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour vision returns to normal.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_UNAPPLY:
  case EVENT_FREE:
    BITCLR(Character(thing)->cAffectFlag, AF_SENSELIFE);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_SENSELIFE);
    break;

  }
  return FALSE;
}


/* See Invisible things */
EFFECTPROC(EffectSeeinvisible) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    for (i=0; i<tarIndex->iNum; i++) {
      if (BIT(Character(tarIndex->iThing[i])->cAffectFlag, AF_SEEINVIS)) {
        SendThing("^cNothing new seems to happen.\n", tarIndex->iThing[i]);
        return FALSE;
      }
      /* Let people know what happened */
      SendAction("^cYou start to see the world from a higher plane.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      /* AffectCreate */
      AffectCreate(tarIndex->iThing[i], AFFECT_NONE, 0, data/10+20, effectNum);
      BITSET(Character(tarIndex->iThing[i])->cAffectFlag, AF_SEEINVIS);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour vision returns to normal.\n", 
         thing, NULL, SEND_SRC);
  case EVENT_UNAPPLY:
  case EVENT_FREE:
    BITCLR(Character(thing)->cAffectFlag, AF_SEEINVIS);
    break;

  case EVENT_APPLY:
    BITSET(Character(thing)->cAffectFlag, AF_SEEINVIS);
    break;

  }
  return FALSE;
}


/* Lowers actual armor class - GOOD protect spell */
EFFECTPROC(EffectLuckshield) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    LWORD i;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendAction("^cYou create a force-field around $N.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n creates a force-field around you.\n", 
        thing, tarIndex->iThing[i], SEND_DST |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^c$n creates a force-field around $N.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
      AffectCreate(tarIndex->iThing[i], AFFECT_ARMOR, data/10, data/10+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendAction("^cThe forcefield around you abruptly disappears.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^cThe forcefield around $n abruptly disappears.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  } break;

  }
  return FALSE;
}

/* Identify an object */
EFFECTPROC(EffectIdentify) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD         i;
    LWORD         fieldNum;
    LWORD         apply;
    OBJTEMPLATE   *object;
    BYTE          buf[512];
    WORD          word;
    
    for (i=0; i<tarIndex->iNum; i++) {
      if (!tarIndex->iThing[i] 
      || tarIndex->iThing[i]->tType!=TTYPE_OBJ 
      || Number(0,100)>data) {
        SendAction("^cYou fail to identify $N.\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        continue;
      }
    
      SendAction("^cYou identify $N.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
        
      object = Obj(tarIndex->iThing[i])->oTemplate;

      /* first line, # Name & Type */
      SendThing("^gName:^G[^c", thing);
      SendThing(object->oSDesc->sText, thing);
      SendThing("^G] ^gType:^G[^c", thing);
      SendThing(TYPESPRINTF(buf, object->oType, oTypeList, 512), thing);
      SendThing("^G]\n^gKeywords:^G[^c", thing);
      SendThing(object->oKey->sText, thing);
      SendThing("^G]\n^gAct: ^G[^c", thing);
      SendThing(FlagSprintf(buf, Obj(tarIndex->iThing[i])->oAct, oActList, ' ', 512), thing);
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
      for (fieldNum=0;
              fieldNum<OBJECT_MAX_FIELD
           && ObjectGetFieldStr(object->oType, &Obj(tarIndex->iThing[i])->oDetail, fieldNum, buf, 512);
           fieldNum++) {
        SendThing(buf, thing);
      }
    
      for (apply=0; apply<OBJECT_MAX_APPLY; apply++) {
        if (object->oApply[apply].aType) {
          sprintf(buf,"^gApply%ld:^G[^c", apply);
          SendThing(buf, thing);
          SendThing(TYPESPRINTF(buf, Obj(tarIndex->iThing[i])->oApply[apply].aType, applyList, 512), thing);
          word = Obj(tarIndex->iThing[i])->oApply[apply].aValue;
          sprintf(buf, "^G] ^gValue:^G[^c%hd^G]\n", word);
          SendThing(buf, thing);
        }
      }
    }
  } break;

  }
  return FALSE;
}

/* Identify a person */
EFFECTPROC(EffectStat) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD         i;
    BYTE          buf[512];
    BYTE          buf2[256];
    THING        *statThing;
    
    for (i=0; i<tarIndex->iNum; i++) {
      if (!tarIndex->iThing[i] 
      || tarIndex->iThing[i]->tType<TTYPE_CHARACTER 
      || Number(0,100)+Character(tarIndex->iThing[i])->cLevel-20>data) {
        SendAction("^cYou fail to stat $N.\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        continue;
      }
   
      statThing=tarIndex->iThing[i];

      SendAction("^cYou stat $N.\n",  
        thing, statThing, SEND_SRC);

      /* show them the stats */
      SendThing("^gName:^G[^c", thing);
      SendThing(statThing->tSDesc->sText, thing);
      SendThing("^G]\n", thing);
      if (statThing->tType == TTYPE_PLR) {
        SendThing("^gClass:^G[", thing);
        TYPESPRINTF(buf2, Plr(statThing)->pClass, classList, sizeof(buf2));
        strcat(buf2, "^G]");
        sprintf(buf, "%-12s ", buf2);
        SendThing(buf, thing);
        SendThing("         ^gRace:^G[", thing);
        SendThing(TYPESPRINTF(buf, Plr(statThing)->pRace, raceList, sizeof(buf)), thing);
        SendThing("^G]\n", thing);
      }
      sprintf(buf, "^gLevel:^G[^c%4hd^G] ", Character(statThing)->cLevel);
      SendThing(buf, thing);
      sprintf(buf, "              ^gAura:^G[^c%4hd^G]", Character(statThing)->cAura);
      SendThing(buf, thing);
      SendThing("               ^gSex:^G[", thing);
      SendThing(TYPESPRINTF(buf, Character(statThing)->cSex, sexList, sizeof(buf)), thing);
      SendThing("^G]\n", thing);
      
      sprintf(buf, "^gArmor:^G[^c%4hd^G] ", Character(statThing)->cArmor);
      SendThing(buf, thing);
      sprintf(buf, "              ^gHitBonus:^G[^c%4ld^G] ", CharGetHitBonus(statThing, Character(statThing)->cWeapon));
      SendThing(buf, thing);
      sprintf(buf, "          ^gDamBonus:^G[^c%4ld^G]\n", CharGetDamBonus(statThing, Character(statThing)->cWeapon));
      SendThing(buf, thing);
      sprintf(buf, "^gMoney:^G[^c%ld^G] ", Character(statThing)->cMoney);
      SendThing(buf, thing);
      sprintf(buf, "^gExp:^G[^c%ld^G]\n", Character(statThing)->cExp);
      SendThing(buf, thing);
      if (statThing->tType == TTYPE_PLR) {
        SendThing("\n^cAbilities:\n^g", thing);
        sprintf(buf, "^gStr: ^G[^c%4hd^G]                ^gHP:^G[^c%5ld^G/^c%5ld^g]          ^gHunger:^G[^c%hd^G]\n", 
                Plr(statThing)->pStr, 
                Character(statThing)->cHitP,
                CharGetHitPMax(statThing),
                Plr(statThing)->pHunger);
        SendThing(buf, thing);
        sprintf(buf, "^gDex: ^G[^c%4hd^G]                ^gMP:^G[^c%5ld^G/^c%5ld^g]          ^gThirst:^G[^c%hd^G]\n", 
                Plr(statThing)->pDex, 
                Character(statThing)->cMoveP, 
                CharGetMovePMax(statThing),
                Plr(statThing)->pThirst);
        SendThing(buf, thing);
        sprintf(buf, "^gCon: ^G[^c%4hd^G]                ^gPP:^G[^c%5ld^G/^c%5ld^g]          ^gIntox:^G[^c%hd^G]\n", 
                Plr(statThing)->pCon, 
                Character(statThing)->cPowerP, 
                CharGetPowerPMax(statThing),
                Plr(statThing)->pIntox);
        SendThing(buf, thing);
        sprintf(buf, "^gWis: ^G[^c%4hd^G]\n", Plr(statThing)->pWis);
        SendThing(buf, thing);
        sprintf(buf, "^gInt: ^G[^c%4hd^G]                ^gPractices:^G[^c%ld^G]\n", 
                Plr(statThing)->pInt,
                Plr(statThing)->pPractice);
        SendThing(buf, thing);
      } else {
        sprintf(buf, "^gHP:^G[^c%5ld^G^G]  ^gMP:^G[^c%5ld^G]  ^gPP:^G[^c%5ld^G]\n", 
                Character(statThing)->cHitP,
                Character(statThing)->cMoveP, 
                Character(statThing)->cPowerP);
        SendThing(buf, thing);
      }
      SendThing("\nResistances:\n", thing);
      sprintf(buf, 
              "^gPuncture  [^c%3hd^G]",
              CharGetResist(statThing, FD_PUNCTURE));
      SendThing(buf, thing);
      sprintf(buf, 
              "            ^gSlash [^c%3hd^G]",
                CharGetResist(statThing, FD_SLASH));
      SendThing(buf, thing);
      sprintf(buf, 
              "               ^gConcussive[^c%3hd^G]\n",
                CharGetResist(statThing, FD_CONCUSSIVE));
      SendThing(buf, thing);
      sprintf(buf, 
              "^gHeat      [^c%3hd^G]",
                CharGetResist(statThing, FD_HEAT));
      SendThing(buf, thing);
      sprintf(buf, 
              "            ^gEMR   [^c%3hd^G]",
                CharGetResist(statThing, FD_EMR));
      SendThing(buf, thing);
      sprintf(buf, 
              "               ^gLaser     [^c%3hd^G]\n",
                CharGetResist(statThing, FD_LASER));
      SendThing(buf, thing);
      sprintf(buf, 
              "^gPsychic   [^c%3hd^G]",
                CharGetResist(statThing, FD_PSYCHIC));
      SendThing(buf, thing);
      sprintf(buf, 
              "            ^gAcid  [^c%3hd^G]",
                CharGetResist(statThing, FD_ACID));
      SendThing(buf, thing);
      sprintf(buf, 
              "               ^gPoison    [^c%3hd^G]\n",
                CharGetResist(statThing, FD_POISON));
      SendThing(buf, thing);
      SendThing("\n^gAffected by: ^G[^c", thing);
      SendThing(FlagSprintf(buf, Character(statThing)->cAffectFlag, affectList, ' ', 512), thing);
      SendThing("^G]\n", thing);
    }
  } break;

  }
  return FALSE;
}

/* Makes it easier for you to hit */
EFFECTPROC(EffectLuckyhits) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    LWORD i;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendAction("^cYou improve the luck of $N.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      SendAction("^c$n makes you luckier.\n", 
        thing, tarIndex->iThing[i], SEND_DST |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^c$n improves the luck of $N.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
      AffectCreate(tarIndex->iThing[i], AFFECT_HITROLL, data/5, data/30+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendAction("^cYou dont feel quite so lucky anymore.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n stops feeling quite so lucky.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  } break;

  }
  return FALSE;
}

/* Makes it easier for you to hit */
EFFECTPROC(EffectLuckydamage) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    LWORD i;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendAction("^cYou make $N seem a little more ominous somehow.\n",  
        thing, tarIndex->iThing[i], SEND_DST);
      SendAction("^c$n makes you a little more ominous somehow.\n", 
        thing, tarIndex->iThing[i], SEND_DST |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^c$n makes $N seem a little more ominous somehow.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
      AffectCreate(tarIndex->iThing[i], AFFECT_DAMROLL, data/25+1, data/30+10, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendAction("^cYou dont feel quite so ominous anymore.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n stops feeling quite so ominous.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  } break;

  }
  return FALSE;
}

  /* Pyrokinetic */                     
/* typical sort of attack spell - wimpy */
EFFECTPROC(EffectBurningfist) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 2;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_HEAT);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 4) + damBonus;
      if (CharDodgeCheck(tarIndex->iThing[i], -data/2+20))
        damage >>= 1;
      FightDamage(thing, damage, 1, FD_HEAT, effectNum, "burning fist");
    }
  } break;

  }
  return FALSE;
}


/* typical sort of attack spell - one grade better */
EFFECTPROC(EffectFlamestrike) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 20;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_HEAT);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 5) + damBonus;
      if (CharDodgeCheck(tarIndex->iThing[i], -data/2))
        damage >>= 1;
      FightDamage(thing, damage, 1, FD_HEAT, effectNum, "pillar of flame");
    }
  } break;

  }
  return FALSE;
}


/* typical sort of attack spell - powerfull */
EFFECTPROC(EffectIncinerate) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) { 
  case EVENT_EFFECT: {
    LWORD i;
    LWORD damDice  = (data/10) + 1;
    LWORD damBonus = 40;
    LWORD damage;
    LWORD resist;

    for (i=0; i<tarIndex->iNum; i++) {
      FightStart(thing, tarIndex->iThing[i], 1, NULL);
      resist = CharGetResist(tarIndex->iThing[i], FD_HEAT);
      damDice = MAXV(damDice/2+1, damDice - resist/15);
      damage = Dice(damDice, 6) + damBonus;
      if (CharDodgeCheck(tarIndex->iThing[i], -data))
        damage >>= 1;
      FightDamage(thing, damage, 1, FD_HEAT, effectNum, "blast of heat");
    }
  } break;

  }
  return FALSE;
}

/* EFFECT_IGNITE         */
/* This should ignite any explosives on a character 
   the chance of which is affected by RESIST_HEAT */

/* higher resistance towards kinetic energy type weapons */
EFFECTPROC(EffectHeatshield) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD   i;
    LWORD   bonus;
    AFFECT *affect;
    for (i=0; i<tarIndex->iNum; i++) {
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) AffectFree(tarIndex->iThing[i], affect);
      /* Let people know what happened */
      SendThing("^cYou feel a little cooler.\n", tarIndex->iThing[i]);
      /* AffectCreate */
      bonus = Dice(2,10) + Number(0,data/30);
      AffectCreate(tarIndex->iThing[i], AFFECT_RES_HEAT, bonus, data/10+20, effectNum);
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT: {
    /* Let people know what happened */
    SendThing("^cYou feel a little warmer.\n", thing);
  } break;

  }
  return FALSE;
}


/* Create a blade */
EFFECTPROC(EffectFireblade) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    LWORD  i;
    THING  *item;
    AFFECT *affect;
    LWORD  duration = data/10+5;
    LWORD  damDieSize = data/30+6;
    
    for (i=0; i<tarIndex->iNum; i++) {
      /* Check if they have a pocket allready */
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) {
        /* strengthen duration */
        if (duration > affect->aDuration)
          affect->aDuration = duration;
        continue;
      }

      /* Create object */
      item = ObjectCreate(fireBladeTemplate, tarIndex->iThing[i]);
      OBJECTSETFIELD(item, OF_WEAPON_DIENUM,  1);
      OBJECTSETFIELD(item, OF_WEAPON_DIESIZE, damDieSize);
      OBJECTSETFIELD(item, OF_WEAPON_TYPE, TYPEFIND("FIREBLADE", weaponList) );
      
      /* If we can equip it */
      if (InvEquip(thing, item, NULL)) {
        /* Let people know what happened */
        SendAction("^cYou create a blade of fire\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        SendAction("^c$n creates a blade of fire for you\n", 
          thing, tarIndex->iThing[i], SEND_DST |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("^c$n creates a blade of fire for $N\n",  
          thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
        AffectCreate(tarIndex->iThing[i], AFFECT_NONE, data, duration, effectNum);
      } else {
        /* If we cant equip it turf the object */
        /* Let caster know their hand was full */
        SendAction("^cYou try to create a blade of fire for $N but have no place to put it.\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        THINGFREE(item);
      }
      
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour blade of fire suddenly winks out of existence.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n's blade of fire suddenly winks out of existence.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    /* fall thru to free */
  case EVENT_FREE: {
    THING *i;
    THING *next;
    
    /* search for it - turf em as we find em, should only be one */
    for (i = thing->tContain; i; i=next) {
      next = i->tNext;
      if (i->tType==TTYPE_OBJ && Obj(i)->oTemplate == fireBladeTemplate) {
        /* its history */
        THINGFREE(i);
      }
    }
  } break;

  case EVENT_READ: {
    THING *item;
    LWORD  damDieSize = affect->aValue/30+6;

    AffectReadPrimitive(thing, affect, file);
    item = ObjectCreate(fireBladeTemplate, thing);
    OBJECTSETFIELD(item, OF_WEAPON_DIESIZE, damDieSize);
    OBJECTSETFIELD(item, OF_WEAPON_TYPE, TYPEFIND("FIREBLADE", weaponList) );
    InvEquip(thing, item, NULL);
  } break;

  case EVENT_WRITE: {
    AffectWritePrimitive(thing, affect, file);
  } break;

  }
  return TRUE;
}



EFFECTPROC(EffectFireshield) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    LWORD  i;
    AFFECT *affect;
    THING  *item;
    LWORD  duration = data/10+5;
    LWORD  armor = data/30+3;
    LWORD  rheat = data/10;
    
    for (i=0; i<tarIndex->iNum; i++) {
      /* Check if they have a pocket allready */
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) {
        /* strengthen duration */
        if (duration > affect->aDuration)
          affect->aDuration = duration;
        continue;
      }

      /* Create object */
      item = ObjectCreate(fireShieldTemplate, tarIndex->iThing[i]);
      OBJECTSETFIELD(item, OF_ARMOR_ARMOR, armor);
      OBJECTSETFIELD(item, OF_ARMOR_RHEAT, rheat);
      
      /* If we can equip it */
      if (InvEquip(thing, item, NULL)) {
        /* Let people know what happened */
        SendAction("^cYou create a shield of fire\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        SendAction("^c$n creates a shield of fire for you\n", 
          thing, tarIndex->iThing[i], SEND_DST |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("^c$n creates a shield for $N\n",  
          thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
        AffectCreate(tarIndex->iThing[i], AFFECT_NONE, data, duration, effectNum);
      } else {
        /* If we cant equip it turf the object */
        /* Let caster know their hand was full */
        SendAction("^cYou try to create a shield of fire for $N but have no place to put it.\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        THINGFREE(item);
      }
      
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour shield of fire winks out of existence.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n's shield of fire suddenly winks out of existence.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    /* then fall thru to free it */
  case EVENT_FREE: {
    THING *i;
    THING *next;
    
    /* search for it - turf em as we find em, should only be one */
    for (i = thing->tContain; i; i=next) {
      next = i->tNext;
      if (i->tType==TTYPE_OBJ && Obj(i)->oTemplate == fireShieldTemplate) {
        /* Pocket is history */
        THINGFREE(i);
      }
    }
  } break;

  case EVENT_READ: {
    THING *item;
    LWORD  armor = affect->aValue/30+3;
    LWORD  rheat = affect->aValue/10;

    AffectReadPrimitive(thing, affect, file);
    item = ObjectCreate(fireShieldTemplate, thing);
    OBJECTSETFIELD(item, OF_ARMOR_ARMOR, armor);
    OBJECTSETFIELD(item, OF_ARMOR_RHEAT, rheat);
    InvEquip(thing, item, NULL);
  } break;

  case EVENT_WRITE: {
    AffectWritePrimitive(thing, affect, file);
  } break;


  }
  return TRUE;
}



EFFECTPROC(EffectFirearmor) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  case EVENT_EFFECT: {
    LWORD  i;
    THING  *item;
    AFFECT *affect;
    LWORD  duration = data/10+10;
    LWORD  armor = data/25+3;
    LWORD  rheat = data/10;
    
    for (i=0; i<tarIndex->iNum; i++) {
      /* Check if they have a pocket allready */
      affect = AffectFind(tarIndex->iThing[i], effectNum);
      if (affect) {
        /* strengthen duration */
        if (duration > affect->aDuration)
          affect->aDuration = duration;
        continue;
      }

      /* Create object */
      item = ObjectCreate(fireArmorTemplate, tarIndex->iThing[i]);
      OBJECTSETFIELD(item, OF_ARMOR_ARMOR, armor);
      OBJECTSETFIELD(item, OF_ARMOR_RHEAT, rheat);
      
      /* If we can equip it */
      if (InvEquip(thing, item, NULL)) {
        /* Let people know what happened */
        SendAction("^cYou create armor of fire\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        SendAction("^c$n creates armor of fire for you\n", 
          thing, tarIndex->iThing[i], SEND_DST |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("^c$n creates armor of fire for $N\n",  
          thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
        AffectCreate(tarIndex->iThing[i], AFFECT_NONE, data, duration, effectNum);
      } else {
        /* If we cant equip it turf the object */
        /* Let caster know their hand was full */
        SendAction("^cYou try to create armor of fire for $N but have no place to put it.\n",  
          thing, tarIndex->iThing[i], SEND_SRC);
        THINGFREE(item);
      }
      
    }
  } break;

  /* called separately as each thing expires, tarIndex ignored */
  case EVENT_UNEFFECT:
    /* Let people know what happened */
    SendAction("^cYour armor of fire winks out of existence.\n", 
         thing, NULL, SEND_SRC);
    SendAction("^c$n's armor of fire suddenly winks out of existence.\n", 
         thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  case EVENT_FREE: {
    THING *i;
    THING *next;
    
    /* search for it - turf em as we find em, should only be one */
    for (i = thing->tContain; i; i=next) {
      next = i->tNext;
      if (i->tType==TTYPE_OBJ && Obj(i)->oTemplate == fireArmorTemplate) {
        /* its is history */
        THINGFREE(i);
      }
    }
  } break;

  case EVENT_READ: {
    THING *item;
    LWORD  armor = affect->aValue/25+3;
    LWORD  rheat = affect->aValue/10;

    AffectReadPrimitive(thing, affect, file);
    item = ObjectCreate(fireArmorTemplate, thing);
    OBJECTSETFIELD(item, OF_ARMOR_ARMOR, armor);
    OBJECTSETFIELD(item, OF_ARMOR_RHEAT, rheat);
    InvEquip(thing, item, NULL);
  } break;

  case EVENT_WRITE: {
    AffectWritePrimitive(thing, affect, file);
  } break;


  }
  return TRUE;
}




/* EFFECT_REVITALIZE - more power points */
EFFECTPROC(EffectRevitalize) { /* (thing, cmd, data, tarIndex, effectNum, event) */
  switch (event) {

  /* Works on tarIndex */
  case EVENT_EFFECT: {
    LWORD i;
    LWORD powerMax;
    LWORD bonus;
    for (i=0; i<tarIndex->iNum; i++) {
      /* Let people know what happened */
      SendAction("^cYou revitalize $n.\n",  
        thing, tarIndex->iThing[i], SEND_SRC);
      SendAction("^c$n revitalizes you.\n",  
        thing, tarIndex->iThing[i], SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^c$n revitalizes $N.\n",  
        thing, tarIndex->iThing[i], SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      /* heal them */
      powerMax = CharGetPowerPMax(tarIndex->iThing[i]);
      bonus = Dice(data/20+1,4) + data/10 + 1;
      Character(tarIndex->iThing[i])->cPowerP += bonus;
      MAXSET(Character(tarIndex->iThing[i])->cPowerP, powerMax);
    }
  } break;

  }
  return FALSE;
}

