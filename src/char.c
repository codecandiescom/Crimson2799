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
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#ifndef WIN32
  #include <unistd.h> /* for unlink */
  #include <dirent.h>
#endif

#include "crimson2.h"
#include "macro.h"
#include "queue.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "file.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "parse.h"
#include "send.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "group.h"
#include "fight.h"
#include "affect.h"
#include "mobile.h"
#include "player.h"
#include "skill.h"
#include "cmd_inv.h"

BYTE *sexList[] = {
  "MALE",
  "FEMALE",
  "SEXLESS",
  ""
};

POS posList[] = {
  { "DEAD",             "^g$n is here, lying dead on the floor"                              },
  { "MORTALLY WOUNDED", "^g$n is lying ^wmortally wounded ^ghere, soon to die unless aided!" },
  { "INCAPACITATED",    "^g$n is lying on the ground ^rincapacitated!"                       },
  { "STUNNED",          "^g$n is standing here, ^cstunned ^gand unable to move!"             },
  { "SLEEPING",         "^g$n is lying on the ground here sleeping"                          },
  { "RESTING",          "^g$n is resting on the ground here"                                 },
  { "SITTING",          "^g$n is sitting down here"                                          },
  { "FIGHTING",         "^g$n is here fighting someone"                                      },
  { "STANDING",         "^g$n is standing here"                                              },
  { "", "" }
};

/* 
 * group can be any thing in the existing group 
 * returns TRUE/FALSE (there are limitations on
 * group size - or there will be eventually)
 * This will actually add thing, AND any followers that 
 * thing may have into the follow chain of group 
 * belongs to.
 *
 * Incidentally, two things are needed to make somebody
 * a group member.
 * 1) they must be following somebody
 * 2) they must have the AF_GROUP flag set (this prevents
 *   unwanted people from joining your group because only
 *   the group leader can set the AF_GROUP flag)
 */
BYTE CharAddFollow(THING *thing, THING *group) {
  return GroupAddFollow(thing,group);
}

/* Remove thing from any groups they may be in */
void CharRemoveFollow(THING *thing) {
  GroupRemoveFollow(thing);
  return;
}

/* Remove thing from any groups they may be in */
void CharKillFollow(THING *thing) {
  GroupKillFollow(thing);
  return; 
}

/* Share experience with other group members present */
extern void CharGainExpFollow(THING *thing, LWORD exp) {
  GroupGainExp(thing,exp);
  return;
}

/* For multiple flags this should find the worst/lowest resistance */
WORD CharGetResist(THING *thing, FLAG rFlag) {
  LWORD resist;
  LWORD defaultResist;
  LWORD i;
  WORD  retValue;
  BYTE  found = FALSE;
  BYTE  buf[256];

  if (!thing) return 0;
  if (thing->tType < TTYPE_CHARACTER)
    return 0;

  for (i=0; (1<<i) <= rFlag; i++) {
    if (!*resistList[i]) break;
    if (BIT(rFlag, (1<<i) )) {
      resist = Character(thing)->cResist[i];
      if (thing->tType == TTYPE_PLR) {
        resist += raceList[Plr(thing)->pRace].rResist[i];
      } else if (thing->tType == TTYPE_MOB) {
        /* Check property overrides */
        sprintf(buf, "%%R%s", resistList[i]);
        defaultResist = Character(thing)->cLevel/3;
        if ( BIT(Mob(thing)->mTemplate->mAct, ((1<<i)<<9)) )
          defaultResist = (defaultResist+10)*2;
        resist += PropertyGetLWord(thing, buf, defaultResist);
      }
      if (found) {
        retValue = MINV(retValue, resist);
      } else {
        retValue = resist;
        found = TRUE;
      }
    }
  }

  if (found)
    return retValue;
  else
    return 0;
}

LWORD CharGetHitPMax(THING *thing) {
  if (!thing) return 0;
  switch(thing->tType) {
  case TTYPE_MOB:
    return Character(thing)->cHitPMax;
  case TTYPE_PLR:
    return (Character(thing)->cHitPMax+Plr(thing)->pCon/20*Character(thing)->cLevel);
  }
  return 0;
}

LWORD CharGetMovePMax(THING *thing) {
  if (!thing) return 0;
  switch(thing->tType) {
  case TTYPE_MOB:
    return (200+Character(thing)->cLevel*10);
  case TTYPE_PLR:
    return (Plr(thing)->pMovePMax+Plr(thing)->pDex/20*Character(thing)->cLevel);
  }
  return 0;
}

LWORD CharGetPowerPMax(THING *thing) {
  if (!thing) return 0;
  switch(thing->tType) {
  case TTYPE_MOB:
    return (100+Character(thing)->cLevel*10);
  case TTYPE_PLR:
    return ( Plr(thing)->pPowerPMax
           +(Plr(thing)->pInt+Plr(thing)->pWis)/40*Character(thing)->cLevel);
  }
  return 0;
}
 
LWORD CharGetHide(THING *thing) {
  if (!thing) return 0;
  if (thing->tType == TTYPE_PLR)
    return Plr(thing)->pSkill[SKILL_HIDE];
  else if (thing->tType == TTYPE_MOB)
    return PropertyGetLWord(thing, "%Hide", Mob(thing)->mTemplate->mLevel*2);
  return 0;
}

LWORD CharGetSneak(THING *thing) {
  if (!thing) return 0;
  if (thing->tType == TTYPE_PLR)
    return Plr(thing)->pSkill[SKILL_SNEAK];
  else if (thing->tType == TTYPE_MOB)
    return PropertyGetLWord(thing, "%Sneak", Mob(thing)->mTemplate->mLevel*2);
  return 0;
}

LWORD CharGetPerception(THING *thing) {
  if (!thing) return 0;
  if (thing->tType == TTYPE_PLR)
    if (Character(thing)->cLevel >= LEVEL_GOD)
      return 1000;
    else
      return Plr(thing)->pSkill[SKILL_PERCEPTION];
  else if (thing->tType == TTYPE_MOB)
    return PropertyGetLWord(thing, "%Perception", Mob(thing)->mTemplate->mLevel*2);
  return 0;
}

LWORD CharGetGuard(THING *thing) {
  if (!thing) return 0;
  if (thing->tType == TTYPE_PLR)
    if (Character(thing)->cLevel >= LEVEL_GOD)
      return 1000;
    else
      return Plr(thing)->pSkill[SKILL_GUARD];
  else if (thing->tType == TTYPE_MOB)
    return PropertyGetLWord(thing, "%Guard", Mob(thing)->mTemplate->mLevel*2);
  return 0;
}

LWORD CharGetPeek(THING *thing) {
  if (!thing) return 0;
  if (thing->tType == TTYPE_PLR)
    return Plr(thing)->pSkill[SKILL_PEEK];
  else if (thing->tType == TTYPE_MOB)
    return PropertyGetLWord(thing, "%Peek", Mob(thing)->mTemplate->mLevel*2);
  return 0;
}

/* Mob prop and Player skill */
LWORD CharGetSteal(THING *thing) {
  if (!thing) return 0;
  if (thing->tType == TTYPE_PLR)
    return Plr(thing)->pSkill[SKILL_STEAL];
  else if (thing->tType == TTYPE_MOB)
    return PropertyGetLWord(thing, "%Steal", Mob(thing)->mTemplate->mLevel*2);
  return 0;
}

/* Mob prop and Player skill */
LWORD CharGetPickpocket(THING *thing) {
  if (!thing) return 0;
  if (thing->tType == TTYPE_PLR)
    return Plr(thing)->pSkill[SKILL_PICKPOCKET];
  else if (thing->tType == TTYPE_MOB)
    return PropertyGetLWord(thing, "%Pickpocket", Mob(thing)->mTemplate->mLevel*2);
  return 0;
}

LWORD CharGetAmbush(THING *thing) {
  LWORD  hide;
  LWORD  perception;
  LWORD  ambush;
  LWORD  guard;
  THING *target = NULL;

  /* If they are hidden add in ambush skill */
  target = Character(thing)->cFight;
  ambush = 0;
  perception = 0;
  if (BIT(Character(thing)->cAffectFlag, AF_HIDDEN)) {
    /* Get Hide skill of thing */
    hide = CharGetHide(thing);

    /* Get perception skill of target */
    if (target) {
      perception = CharGetPerception(target);
    }

    /* Get Ambush skill of thing */
    if (thing->tType == TTYPE_PLR)
      ambush = Plr(thing)->pSkill[SKILL_AMBUSH];
    else
      ambush = PropertyGetLWord(thing, "%Ambush", Mob(thing)->mTemplate->mLevel*2);

    /* Get Guard skill of target */
    if (target && perception > hide) {
      guard = CharGetGuard(thing);
      ambush -= guard;
      MINSET(ambush, 0);
    }
  }

  return ambush;
}

LWORD CharGetSkill(THING *thing, LWORD i) {
  LWORD skill = 0;
  BYTE  buf[256];

  if (i<0) return 0;
  if (i>= skillNum) return 0;
  
  if (thing->tType == TTYPE_PLR) {
    skill = Plr(thing)->pSkill[i];
    if (Character(thing)->cLevel >= LEVEL_GOD) /* Gods are good what can I say */
      return 300;
    if (!skill) return 0;

    /* Take into account effects of Aura Good/Bad */
    if (BIT(skillList[i].sFlag, SF_GOOD)) {
       if (Character(thing)->cAura>400) {
         skill += Number(0, 1+(Character(thing)->cAura-400)/20 );
       } else if (Character(thing)->cAura<400) {
         skill -= Number(0, 1+(-1*Character(thing)->cAura-400)/20 );
         Character(thing)->cAura+=1;
       }
    }
  
    /* Evil people are better at evil psi powers */
    if (BIT(skillList[i].sFlag, SF_EVIL)) {
       if (Character(thing)->cAura>400) {
         skill -= Number(0, 1+(Character(thing)->cAura-400)/20 );
         Character(thing)->cAura-=1;
       } else if (Character(thing)->cAura<400) {
         skill += Number(0, 1+(-1*Character(thing)->cAura-400)/20 );
       }
    }

  } else if (thing->tType == TTYPE_MOB) {
    sprintf(buf, "%%%s", skillList[i].sName);
    skill = PropertyGetLWord(thing, buf, 0);

  } else {
    skill = 0;
  }

  /* make sure we arent negative at the skill */
  MINSET(skill, 0);
  return skill;
}


LWORD CharGetHitBonus(THING *thing, THING *weapon) {
  LWORD  result;
  LWORD  wType;
  THING *ammo;
  LWORD  ammoType = 0;
  LWORD  ammoUse = 0;
  LWORD  ammoLeft = 0;

  if (!thing) return 0;

  /* modify by base hit bonus */
  result = Character(thing)->cHitBonus;

  /* modify by ambush */
  result += CharGetAmbush(thing)/2;

  ammo = ObjectGetAmmo(weapon, &ammoType, &ammoUse, &ammoLeft);
  
  if (weapon 
    && weapon->tType == TTYPE_OBJ 
    && Obj(weapon)->oTemplate->oType == OTYPE_WEAPON) {
    /* adjust for the ammo we are using if any */
    if ( ammo && ammoLeft>=ammoUse)
      result += OBJECTGETFIELD(ammo, OF_AMMO_HITBONUS);

    /* adjust for skill with weapon */
    if (thing->tType==TTYPE_PLR) {
      if (!ammoType || ammoLeft>=ammoUse) {
        wType = OBJECTGETFIELD(weapon, OF_WEAPON_TYPE); 
        if (wType>=0) result += Plr(thing)->pSkill[*weaponList[wType].wSkill]/2;
        if (wType>=0) result += Plr(thing)->pSkill[*weaponList[wType].wFamily];
      } else {
        /* Out of ammo use like a bludgeon */
        result += Plr(thing)->pSkill[SKILL_MELEE_BLUDGEON]/2;
        result += Plr(thing)->pSkill[SKILL_MELEE];
      }
    }
  }

  /* Check if they are intoxicated */
  if (thing->tType == TTYPE_PLR && Plr(thing)->pIntox>0) {
    LWORD percent;

    percent = Plr(thing)->pIntox * 100 / raceList[Plr(thing)->pRace].rMaxIntox;
    percent = 100 - percent;
    /* 100% intox gets your hitroll down to -50% */
    result += 50;
    result = result * percent / 100;
    result -= 50;
  }

  return result;
}

LWORD CharGetDamBonus(THING *thing, THING *weapon) {
  LWORD  damage;
  LWORD  wType;
  THING *ammo;
  LWORD  ammoType = 0;
  LWORD  ammoUse = 0;
  LWORD  ammoLeft = 0;

  /* base bonus */
  damage = Character(thing)->cDamBonus;

  /* modify by ambush */
  damage += CharGetAmbush(thing)/10;

  /* Mobs allways get their Dam Bonus */
  if (thing->tType==TTYPE_MOB)
    damage += Mob(thing)->mTemplate->mDamBonus;

  ammo = ObjectGetAmmo(weapon, &ammoType, &ammoUse, &ammoLeft);

  if (thing->tType==TTYPE_PLR) {
    /* figure if a character gets to add StrBonus */
    if ((weapon==NULL) 
    || (weapon->tType!=TTYPE_OBJ)
    || (Obj(weapon)->oTemplate->oType != OTYPE_WEAPON)
    || (ammoType && ammoLeft<ammoUse)
    || (weaponList[OBJECTGETFIELD(weapon, OF_WEAPON_TYPE)].wStrBonus))
      damage += ( Plr(thing)->pStr/30 );

    /* Calc DamBonus for damage skill as soon as there are such skills */
    if (weapon 
    && weapon->tType==TTYPE_OBJ 
    && Obj(weapon)->oTemplate->oType==OTYPE_WEAPON) {
      if (!ammoType || (ammoLeft>=ammoUse)) {
        if ( ammo )
          damage += OBJECTGETFIELD(ammo, OF_AMMO_DAMBONUS);
        wType = OBJECTGETFIELD(weapon, OF_WEAPON_TYPE);
        if (wType>=0) damage += Plr(thing)->pSkill[*weaponList[wType].wSkill]/20;
        if (wType>=0) damage += Plr(thing)->pSkill[*weaponList[wType].wFamily+1]/10;
      } else {
        /* Out of ammo use like a bludgeon */
        damage += Plr(thing)->pSkill[SKILL_MELEE_BLUDGEON]/20;
        damage += Plr(thing)->pSkill[SKILL_MELEE+1]/10;
      }
    }
  }
  
  return damage;
}

LWORD CharWillPowerCheck(THING *thing, LWORD bonus) {
  LWORD check;

  check = DiceOpenEnded(1, 100, 100) + bonus;
  if (thing->tType == TTYPE_PLR) {
    check += Plr(thing)->pSkill[SKILL_WILLPOWER];
  } else if (thing->tType == TTYPE_MOB) {
    check += PropertyGetLWord(thing, "%WillPower", Mob(thing)->mTemplate->mLevel*3);
  }
  
  return (check>65);
}

LWORD CharDodgeCheck(THING *thing, LWORD bonus) {
  LWORD check;

  check = DiceOpenEnded(1, 100, 100) + bonus;
  if (thing->tType == TTYPE_PLR) {
    check += Plr(thing)->pSkill[SKILL_DODGE];
    check +=( Plr(thing)->pDex - 75 );
  } else if (thing->tType == TTYPE_MOB) {
    check += PropertyGetLWord(thing, "%Dodge", Mob(thing)->mTemplate->mLevel*3);
  }
  
  return (check>65);
}

LWORD CharMoveCostAdjust(THING *thing, LWORD cost) {
  /* encumbrance adjust */
  if (Base(thing)->bConWeight > CharGetCarryMax(thing)/4) {
    if (Base(thing)->bConWeight <= CharGetCarryMax(thing)/2)
      cost += cost>>1;
    else if (Base(thing)->bConWeight <= 75*CharGetCarryMax(thing)/100)
      cost *= 2;
    else 
      cost = cost * 25 / 10;
  }

  /* Adjust for the fact that they are sneaking */
  if (BIT(Character(thing)->cAffectFlag, AF_SNEAK))
    cost *= 2;

  return cost;
}

LWORD CharGainAdjust(THING *thing, BYTE gainType, LWORD gain) {
  LWORD percent;

  /* Adjust for Regeneration affect */
  if (BIT(Character(thing)->cAffectFlag, AF_REGENERATION)) gain += gain/2;

  /* Adjust for Position */
  switch(Character(thing)->cPos) {
  case POS_DEAD:
    gain = 0;
    break;

  case POS_MORTAL:
    if (gainType == GAIN_HITP) 
      gain = -1;
    else if (gainType==GAIN_POWERP || gainType==GAIN_MOVEP)
      gain = -gain; /* Lose mana and move when mortally hurt */
    break;

  case POS_INCAP:
    break;
  case POS_STUNNED:
    break;

  case POS_SLEEPING:
    if (gainType<GAIN_HUNGER)
      gain += gain>>1;
    else
      gain -= gain>>1;
  case POS_RESTING:
    if (gainType<GAIN_HUNGER)
      gain += gain>>2;
    else
      gain -= gain>>2;
  case POS_SITTING:
    if (gainType<GAIN_HUNGER)
      gain += gain>>2;
    else
      gain -= gain>>2;
    break;

  case POS_FIGHTING:
    gain >>= 1;
    break;
  }

  /* Adjust for Hunger/Thirst */
  if (gain > 0 
   && thing->tType==TTYPE_PLR 
   && (gainType<GAIN_HUNGER)
  ) {
    percent = 100;
    if (Plr(thing)->pHunger>0) {
      percent -= (Plr(thing)->pHunger*100 / raceList[Plr(thing)->pRace].rMaxHunger / 2);
    }
    /* Gain Thirst */
    if (Plr(thing)->pThirst>0) {
      percent -= (Plr(thing)->pThirst*100 / raceList[Plr(thing)->pRace].rMaxThirst / 2);
    }
    gain = gain * percent / 100;
    MINSET(gain, 1);
  }
  return gain;
}

void CharShowHealth(THING *show, THING *thing) {
  LWORD health;

  if (show->tType < TTYPE_CHARACTER)
    return;

  health = Character(show)->cHitP * 100 / MAXV(1, CharGetHitPMax(show));
  if (health>=100) 
    SendAction("^g$n is in perfect health\n", 
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>90) 
    SendAction("^g$n is in excellent condition\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>80) 
    SendAction("^gAside from a few scratches, $n is fine shape\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>70) 
    SendAction("^C$n has a few nicks and cuts but is otherwise ok\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>60) 
    SendAction("^C$n is looking a little bit beat up\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>50) 
    SendAction("^C$n looks like $e has seen better days\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>40) 
    SendAction("^p$n is in bad shape\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>30) 
    SendAction("^p$n is bleeding severely from countless wounds\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>20) 
    SendAction("^p$n is trying not to bleed all over the floor\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else if (health>10) 
    SendAction("^r$n looks like $e just got run over by a truck\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
  else 
    SendAction("^r$n looks ready to leave this world for the next\n",
      show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST|SEND_SELF);
}

void CharShowEquip(THING *show, THING *thing) {
  LWORD  i;
  LWORD  j;
  THING *equip;
  BYTE   buf[256];
  FLAG   bitShown = 0;

  if (thing->tType < TTYPE_CHARACTER) return;
  
  /* <SIGH> Fanatic wants it sorted, here comes the performance hit */
  for (j=0; 1<<j <= EQU_MAX; j++) {
    for (equip=show->tContain; equip; equip=equip->tNext) {
      if (equip->tType==TTYPE_OBJ 
      && BIT(Obj(equip)->oEquip, 1<<j) 
      && !BITANY(Obj(equip)->oEquip, bitShown)) {
        BITSET(bitShown, Obj(equip)->oEquip);
        /* Show the first equip flag */
        for(i=0; !BIT(Obj(equip)->oEquip, 1<<i) && *equipList[i]; i++);
        if (!*equipList[i]) {
          sprintf(buf, "^C%-20s ", "???");
        } else {
          sprintf(buf, "^C%-20s ", equipList[i]);
        }
        SendAction(buf, show, thing, SEND_DST|SEND_VISIBLE|SEND_SELF);
     
        /* Show the item */
        SendAction("^g",show, thing, SEND_DST|SEND_VISIBLE|SEND_SELF);
        SendAction(equip->tSDesc->sText, show, thing, SEND_DST|SEND_VISIBLE|SEND_SELF);
        SendAction("\n",show, thing, SEND_DST|SEND_VISIBLE|SEND_SELF);
      
        /* show the rest of the flags */
        i++;
        while (1<<i < Obj(equip)->oEquip) {
          if (!*equipList[i]) break;
          if (BIT(Obj(equip)->oEquip, 1<<i)) {
            SendAction("^C", show, thing, SEND_DST|SEND_VISIBLE|SEND_SELF);
            sprintf(buf, "^C%-20s ", equipList[i]);
            SendAction(buf, show, thing, SEND_DST|SEND_VISIBLE|SEND_SELF);
            SendAction("^Y...\n", show, thing, SEND_DST|SEND_VISIBLE|SEND_SELF);
          }
          i++;
        }
      }
    }
  }
}

LWORD CharCanCarry(THING *thing, THING *object) {
  if (object->tType != TTYPE_OBJ)
    return FALSE;
  if (Obj(object)->oTemplate->oWeight<0) 
    return FALSE;
  if (Base(object)->bWeight+Base(thing)->bConWeight > CharGetCarryMax(thing)) 
    return FALSE;

  return TRUE;
}

LWORD CharGetCarryMax(THING *thing) {
  if (thing->tType == TTYPE_PLR) {
    return Plr(thing)->pStr*2;
  } else if (thing->tType == TTYPE_MOB) {
    return PropertyGetLWord(thing, "%CarryMax", 150);
  }
  return 0;
}

/* return TRUE to black further processing ie they died */
LWORD CharTick(THING *thing) {
  THING *i;
  THING *next;
  THING *ammo     = NULL;
  LWORD  ammoType = 0;
  LWORD  ammoUse  = 0;
  LWORD  ammoLeft = 0;

  if (AffectTick(thing)) return TRUE;

  /* Use ammo for equipped objects that require it */
  for (i=thing->tContain; i; i=next) {
    next = i->tNext;
    if (i->tType==TTYPE_OBJ && Obj(i)->oEquip && Obj(i)->oTemplate->oType!=OTYPE_WEAPON) {
      ammo = ObjectGetAmmo(i, &ammoType, &ammoUse, &ammoLeft);
      if (!ammoType || ammoUse<0) continue;
      if (ammoUse > ammoLeft) {
        SendAction("^wI'm afraid ^g$N just ran outta steam\n", thing, i, SEND_SRC|SEND_CAPFIRST);
        InvUnEquip(thing, i, IUE_NONBLOCKABLE);
      }
      ObjectUseAmmo(i);
    }
  }
  
  return FALSE;
}

LWORD CharFastTick(THING *thing) {
  LWORD damage;

  if (thing->tType==TTYPE_PLR && BIT(Plr(thing)->pSystem, PS_NOHASSLE))
    return FALSE;

  /* Check for VACUUM here */
  if (Base(thing)->bInside && Base(thing)->bInside->tType == TTYPE_WLD) {
    if ( (BIT(Wld(Base(thing)->bInside)->wFlag, WF_VACUUM)
        ||Wld(Base(thing)->bInside)->wType==WT_VACUUM)
    && !BIT(Character(thing)->cAffectFlag, AF_VACUUMWALK)  ){
      damage = Number(1, CharGetHitPMax(thing)/3);
      MINSET(damage, 1);
      /* if its killed by decompression whoever its fighting gets the xp */
      SendThing("^rYou're suffering explosive decompression!\n", thing);
      if (!ParseCommandCheck(PARSE_COMMAND_WCREATE, BaseControlFind(thing), ""))
        if (FightDamagePrimitive(Character(thing)->cFight, thing, damage)) return TRUE;
    }
  }
  
  /* Check for UNDERWATER here */
  if (Base(thing)->bInside && Base(thing)->bInside->tType == TTYPE_WLD) {
    if ((Wld(Base(thing)->bInside)->wType==WT_UNDERWATER)
    && !BIT(Character(thing)->cAffectFlag, AF_BREATHWATER)){
      damage = Number(1, CharGetHitPMax(thing)/5);
      MINSET(damage, 1);
      /* if its killed by poison whoever its fighting gets the xp */
      if (!ParseCommandCheck(PARSE_COMMAND_WCREATE,  BaseControlFind(thing), ""))
        if (FightDamagePrimitive(Character(thing)->cFight, thing, damage)) return TRUE;
    }
  }
  return FALSE;
  
}

THING *CharThingFind(THING *thing, BYTE *key, LWORD virtual, THING *search, FLAG findFlag, LWORD *offset) {
  THING *found;

  found = ThingFind(key, virtual, search, findFlag, offset);
  if (!thing) return found;
  while (found && ThingCanSee(thing, found)==TCS_CANTSEE) {
    found = ThingFind(key, virtual, search, findFlag|TF_CONTINUE, offset);
  }
  return found;
}
