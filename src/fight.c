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
#include "log.h"
#include "mem.h"
#include "str.h"
#include "property.h"
#include "file.h"
#include "queue.h"
#include "send.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "code.h"
#include "exit.h"
#include "world.h"
#include "base.h"
#include "char.h"
#include "group.h"
#include "mobile.h"
#include "player.h"
#include "area.h"
#include "skill.h"
#include "object.h"
#include "affect.h"
#include "effect.h"
#include "cmd_move.h"
#include "cmd_inv.h"
#include "cmd_cbt.h"
#include "fight.h"
#include "group.h"

#define FIGHT_INDEX_SIZE     2048
#define FIGHT_MSG_INDEX_SIZE 1024
#define FIGHT_MSG_SIZE        256 /* block alloc size */

/* How combat is going to work
   chance to hit is determined by:
   skill + familyskill + HitBonus with any given weapon, where skill with ONE weapon type is
   reasonably easy to learn but a general skill with all weapons in that family
   should be (MUCH) harder to learn.
*/

/* should damage flags be determined by Ammo?, at the very least they can be overridden! */
/* List is more or less sorted according to damage potential, and in increasing difficulty
   to learn ie Plasma Cannons should rock, but practicing the skill up should be a bitch
  */
struct WeaponListType weaponList[] = {

  /* Name,         strbonus   damflags               skill                  fam. skill */

  /* Melee Weapons */
  {"MELEE-KNIFE",         1,  FD_PUNCTURE,           &SKILL_MELEE_KNIFE,     &SKILL_MELEE},
  {"MELEE-BLADE",         1,  FD_SLASH,              &SKILL_MELEE_BLADE,     &SKILL_MELEE},
  {"MELEE-BLUDGEON",      1,  FD_CONCUSSIVE,         &SKILL_MELEE_BLUDGEON,  &SKILL_MELEE},
  {"MELEE-AXE",           1,  FD_SLASH,              &SKILL_MELEE_AXE,       &SKILL_MELEE},

  /* Laser Weapons */
  {"LASER-PISTOL",        0,  FD_LASER,              &SKILL_LASER_PISTOL,    &SKILL_PISTOL},
  {"LASER-RIFLE",         0,  FD_LASER,              &SKILL_LASER_RIFLE,     &SKILL_RIFLE},

  /* Slugthrower Weapons */
  {"SLUG-PISTOL",         0,  FD_PUNCTURE,           &SKILL_SLUG_PISTOL,     &SKILL_PISTOL},
  {"SLUG-RIFLE",          0,  FD_PUNCTURE,           &SKILL_SLUG_RIFLE,      &SKILL_RIFLE},
  {"SLUG-MACHINEPISTOL",  0,  FD_PUNCTURE,           &SKILL_SLUG_MACHINE,    &SKILL_PISTOL},

  /* Blaster Weapons */
  {"BLASTER-PISTOL",      0,  FD_HEAT,               &SKILL_BLASTER_PISTOL,  &SKILL_PISTOL},
  {"BLASTER-RIFLE",       0,  FD_HEAT,               &SKILL_BLASTER_RIFLE,   &SKILL_RIFLE},
  {"BLASTER-CANNON",      0,  FD_HEAT,               &SKILL_BLASTER_CANNON,  &SKILL_CANNON},

  /* Ion Weapons */
  {"ION-RIFLE",           0,  FD_EMR,                &SKILL_ION_RIFLE,       &SKILL_RIFLE},
  {"ION-CANNON",          0,  FD_EMR,                &SKILL_ION_CANNON,      &SKILL_CANNON},

  /* Support Weapons */
  {"GRENADE-LAUNCHER",    0,  FD_CONCUSSIVE,         &SKILL_GRENADE_RIFLE,   &SKILL_RIFLE},
  {"ROCKET-LAUNCHER",     0,  FD_CONCUSSIVE,         &SKILL_MISSILE_CANNON,  &SKILL_CANNON},

  /* Plasma Weapons */
  {"PLASMA-RIFLE",        0,  FD_HEAT|FD_CONCUSSIVE, &SKILL_PLASMA_RIFLE,    &SKILL_RIFLE},
  {"PLASMA-CANNON",       0,  FD_HEAT|FD_CONCUSSIVE, &SKILL_PLASMA_CANNON,   &SKILL_CANNON},

  {"FIREBLADE",           0,  FD_HEAT,               &SKILL_FIREBLADE,       &SKILL_FIRE},
  /* End of the List */
  {"",                    0,  0,                     0,                     0}
};



/* Heat & Concussive would be the most common I would think... */
BYTE *resistList[] = {
  "PUNCTURE",
  "SLASH",
  "CONCUSSIVE",
  "HEAT",
  "EMR", /* ie electrical */
  "LASER",
  "PSYCHIC", /* ie mind attack */
  "ACID",
  "POISON",
  "NONE",
  ""
};

INDEX fightIndex;
INDEX missMsgIndex;
INDEX singleMsgIndex;
INDEX multipleMsgIndex;
INDEX deathMsgIndex;

/* returns same values as strcmp */
INDEXPROC(FightMsgCompareProc) { /* BYTE IndexProc(void *index1, void *index2) */
  if ( FightMsg(index1)->fmType == FightMsg(index2)->fmType )
    return 0;
  else if ( FightMsg(index1)->fmType < FightMsg(index2)->fmType )
    return -1;
  else
    return 1;
}

/* returns same values as strcmp */
INDEXFINDPROC(FightMsgFindProc) { /* BYTE IFindProc(void *key, void *index) */
  if ( (LWORD)key == FightMsg(index)->fmType )
    return 0;
  else if ( (LWORD)key < FightMsg(index)->fmType )
    return -1;
  else
    return 1;
}


void FightMessageAdd(FIGHTMSG *msg, BYTE *typeBuf, BYTE indexType) {
  LWORD i=0;
  BYTE  word[256];
  LWORD found;
  
  while (typeBuf[i]) {
    sscanf(typeBuf+i, " %s", word);
    found = TYPEFIND(word, weaponList);
    if (found == -1) {
      found = TYPEFIND(word, effectList);
      if (found != -1)
        found += FM_EFFECT;
    } else
      found += FM_WEAPON;
    if (found != -1) {
      msg->fmType = found;
      switch (indexType) {
      case '0':
        IndexInsert(&missMsgIndex, (THING*)msg, FightMsgCompareProc);
        break;
      case 'S':
        IndexInsert(&singleMsgIndex, (THING*)msg, FightMsgCompareProc);
        break;
      case 'M':
        IndexInsert(&multipleMsgIndex, (THING*)msg, FightMsgCompareProc);
        break;
      case 'D':
        IndexInsert(&deathMsgIndex, (THING*)msg, FightMsgCompareProc);
        break;
      }
    } else {
      Log(LOG_ERROR, "FightMsgAdd: Unknown Message Type");
      LogPrintf(LOG_ERROR, word);
      LogPrintf(LOG_ERROR, "\n");
    }
    for(i++; typeBuf[i]&&typeBuf[i]!=' '; i++);
  }
}

void FightMessageRead(void) {
  BYTE            buf[256];
  BYTE            typeBuf[256] = "";
  FILE           *msgFile;
  FIGHTMSG       *msg;

  msgFile = fopen("msg/damage.msg", "rb");
  if (!msgFile) {
    Log(LOG_ERROR, "Unable to open msg/damage.msg file\n");
    return;
  }
  fgets(buf, 256, msgFile);
  while (!feof(msgFile)) {
    switch (buf[0]) {
      case '#':
        strcpy(typeBuf, buf+1);
        break;

      case '0':
      case 'S':
      case 'M':
      case 'D':
        if (!*typeBuf) break; /* must have read this first */
        MEMALLOC(msg, FIGHTMSG, FIGHT_MSG_SIZE);
        memset(msg, 0, sizeof(FIGHTMSG));
        msg->fmSrc  = FileStrRead(msgFile);
        msg->fmDst  = FileStrRead(msgFile);
        msg->fmRoom = FileStrRead(msgFile);
        FightMessageAdd(msg, typeBuf, buf[0]);
        break;

      case ';': /* its a comment */
      case '\0': /* its nothing */
      case '\r': /* its nothing */
      case '\n': /* its nothing */
        break;

      default: /* corrupt crap for example */
        Log(LOG_ERROR, "in [msg/damage.msg] not sure what to make of:\n");
        Log(LOG_ERROR, buf);
        break; /* its a comment */
    }
    fgets(buf, 256, msgFile);
  }
}

void FightInit(void) {
  IndexInit(&fightIndex,       FIGHT_INDEX_SIZE,     "fightIndex",       0                 );
  IndexInit(&missMsgIndex,     FIGHT_MSG_INDEX_SIZE, "missMsgIndex",     IF_ALLOW_DUPLICATE);
  IndexInit(&singleMsgIndex,   FIGHT_MSG_INDEX_SIZE, "singleMsgIndex",   IF_ALLOW_DUPLICATE);
  IndexInit(&multipleMsgIndex, FIGHT_MSG_INDEX_SIZE, "multipleMsgIndex", IF_ALLOW_DUPLICATE);
  IndexInit(&deathMsgIndex,    FIGHT_MSG_INDEX_SIZE, "deathMsgIndex",    IF_ALLOW_DUPLICATE);

  FightMessageRead();
}

/* Just moved through this exit take appropriate action */
void FightMove(THING *thing, EXIT *exit) {
  FightStop(thing);
}

/* see if we hit who we are fighting */
LWORD FightCheckHit(THING *thing, WORD *hitNum) {
  LWORD  hitAttempts = 0; /* Minset to 1 later */
  THING *weapon;
  LWORD  i;
  LWORD  roll;
  LWORD  result;
  THING *ammo     = NULL;
  LWORD  ammoType = 0;
  LWORD  ammoUse  = 0;
  LWORD  ammoLeft = 0;
  THING *target = Character(thing)->cFight;
  LWORD  ambush;
  LWORD  hidden = FALSE;

  /* determine # of hit attempts */
  *hitNum = 0;
  weapon = Character(thing)->cWeapon;
  if (!weapon || weapon->tType!=TTYPE_OBJ || Obj(weapon)->oTemplate->oType!=OTYPE_WEAPON)
    weapon = NULL; /* not a valid weapon */
  if (weapon) {
    ammo = ObjectGetAmmo(weapon, &ammoType, &ammoUse, &ammoLeft);
    if (ammoUse <= ammoLeft || thing->tType==TTYPE_MOB)
      hitAttempts = OBJECTGETFIELD(weapon, OF_WEAPON_FIRERATE);
  }
  MINSET(hitAttempts, 1);

  /* multiple hit attempts skill */
  roll = 100+Character(thing)->cSpeed;
  if (thing->tType == TTYPE_PLR) {
    if (weapon) {
      if (!ammoType || ammoUse <= ammoLeft) {
        i = OBJECTGETFIELD(weapon, OF_WEAPON_TYPE);
        TYPECHECK(i, weaponList);
        roll += Plr(thing)->pSkill[*weaponList[i].wSkill]/3;
        roll += Plr(thing)->pSkill[*weaponList[i].wFamily+2];
      } else {
        /* out of ammo use like a club */
        roll += Plr(thing)->pSkill[SKILL_MELEE_BLUDGEON+2]/3;
        roll += Plr(thing)->pSkill[SKILL_MELEE+2];
      }
    }
    roll += Plr(thing)->pSkill[SKILL_SPEED];
  } else if (thing->tType == TTYPE_MOB) {
    roll += PropertyGetLWord(thing, "%Speed", Mob(thing)->mTemplate->mLevel*3);
  }

  /* If they are hidden add in ambush skill to speed */
  if BIT(Character(thing)->cAffectFlag, AF_HIDDEN) hidden = TRUE;
  ambush = CharGetAmbush(thing);
  roll += ambush;
  /* check for the effects of haste and slow */
  if (AffectFind(thing,EFFECT_HASTE)) {
    roll*=2;
  }
  if (AffectFind(thing,EFFECT_SLOW)) {
    roll/=2;
  }

  MINSET(roll, 1);
  result = DiceOpenEnded(1, roll, 100);
  hitAttempts += result/100;
  if (result%100>50) hitAttempts++;
  if (weapon && ammoType && ammoUse && ammo)
    MAXSET(hitAttempts, ammoLeft/ammoUse);

  /* see how many times we hit the target */
  for (i=0; i<hitAttempts; i++) {
    result = DiceOpenEnded(1,100,100);
    
    /* adjust for armor  */
    result -= Character( target )->cArmor;

    /* adjust for dex if target is player */
    if (target->tType == TTYPE_PLR)
      result -= Plr( target )->pDex/10;

    /* adjust for hit bonus */
    result += CharGetHitBonus(thing, weapon);

    if (result>50) {
      *hitNum += 1;
    } else { /* deduct ammo if we missed, otherwise wait until after damage */
      ObjectUseAmmo(weapon);
    }

    /* Only the first attack gets the ambush bonuses */
    BITCLR(Character(thing)->cAffectFlag, AF_HIDDEN);
  }

  /* Set hidden bit again so that damage message mentions attacking
   * from the shadows */
  if (hidden) BITSET(Character(thing)->cAffectFlag, AF_HIDDEN);

  /* return results */
  if (*hitNum>0)
    return TRUE;
  else
    return FALSE;
}


void FightReload(THING *thing) {
  THING *ammo = NULL;
  THING *newAmmo = NULL;
  THING *weapon;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;

  weapon = Character(thing)->cWeapon;
  ammo = ObjectGetAmmo(weapon, &ammoType, &ammoUse, &ammoLeft);
  if (!ammoType) return;

  if (!ammo || ammoUse > ammoLeft) {
    /* Look for replacement ammo */
    for (newAmmo = thing->tContain; newAmmo; newAmmo=newAmmo->tNext) {
      if (newAmmo->tType != TTYPE_OBJ) continue;
      if (Obj(newAmmo)->oTemplate->oType != OTYPE_AMMO) continue;
      if (OBJECTGETFIELD(newAmmo, OF_AMMO_AMMOTYPE) == ammoType) break;
    }
    if (!newAmmo) return;

    if (ammo) THINGFREE(ammo);
    ThingTo(newAmmo, weapon);
    /* let 'em know what happened */
    SendAction("^G$n reloads $N\n", 
      thing, weapon, SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
    SendAction("^wYou reload $N\n", 
      thing, weapon, SEND_SRC |SEND_VISIBLE|SEND_AUDIBLE|SEND_CAPFIRST);
  }
}

LWORD FightWeaponDamage(THING *thing, FLAG *damFlag, WORD *damType) {
  LWORD  damage;
  LWORD  wtype;
  LWORD  dieNum;
  LWORD  dieSize;
  THING *ammo = NULL;
  LWORD  ammoType;
  LWORD  ammoUse;
  LWORD  ammoLeft;

  switch(thing->tType) {
  case TTYPE_PLR:
    damage = CharGetDamBonus(thing, Character(thing)->cWeapon);
    if (Character(thing)->cWeapon)
      return  (damage + FightWeaponDamage(Character(thing)->cWeapon, damFlag, damType));
    *damFlag = FD_CONCUSSIVE;
    *damType = FM_DEFAULT;
    damage += ( Dice(1,4) );
    break;
    
  case TTYPE_MOB:
    damage = CharGetDamBonus(thing, Character(thing)->cWeapon);
    if (Character(thing)->cWeapon)
      return (damage + FightWeaponDamage(Character(thing)->cWeapon, damFlag, damType));
    *damFlag = FD_CONCUSSIVE;
    *damType = FM_DEFAULT; 
    damage += Dice(Mob(thing)->mTemplate->mDamDiceNum, Mob(thing)->mTemplate->mDamDiceSize);
    break;
    
  case TTYPE_OBJ:
    if (Obj(thing)->oTemplate->oType == OTYPE_WEAPON) {
      wtype    = OBJECTGETFIELD(thing, OF_WEAPON_TYPE);
      dieNum   = OBJECTGETFIELD(thing, OF_WEAPON_DIENUM);
      dieSize  = OBJECTGETFIELD(thing, OF_WEAPON_DIESIZE);
      ammo = ObjectGetAmmo(thing, &ammoType, &ammoUse, &ammoLeft);
      if (!ammoType || ammoUse <= ammoLeft) {
        *damFlag = weaponList[wtype].wDamage;
        *damType = FM_WEAPON+wtype;
        damage = Dice(dieNum, dieSize);
        if (ammoType) {
          ObjectUseAmmo(thing);
        }
      } else {
        /* mobs do weapon damage even without ammo */
        if (Base(thing)->bInside->tType == TTYPE_MOB) {
          *damFlag = weaponList[wtype].wDamage;
          *damType = FM_WEAPON+wtype;
          damage = Dice(dieNum, dieSize);
        } else {
          *damFlag = FD_CONCUSSIVE;
          *damType = FM_DEFAULT; 
          damage = Dice(1, Obj(thing)->oTemplate->oWeight/25+1);
        }
      }
    } else {
      *damFlag = FD_CONCUSSIVE;
      *damType = FM_DEFAULT; 
      damage = Dice(1, Obj(thing)->oTemplate->oWeight/25+1);
    }
    break;
    
  default:
    *damFlag = FD_CONCUSSIVE;
    *damType = FM_DEFAULT; 
    damage = 1;
    break;
  }
  
  return damage;
}


/* for the grammer impaired, a gerund verb form is anything that ends with "ing" */
#define FSS_NORMAL 0
#define FSS_GERUND 1
void FightSprintfSeverity(BYTE *str, THING *victim, LWORD damage, WORD field) {
  typedef struct severityStrType {
    BYTE *sName;
    BYTE *sGerund;
    WORD  sPercent;
  } SEVERITYSTR;

  SEVERITYSTR severityStr[] = {
    {"tickle",        "tickling",           2},
    {"lightly wound", "lightly wounding",   5},
    {"hurt",          "hurting",           10},
    {"badly hurt",    "badly hurting",     20},
    {"mangle",        "mangling",          40},
    {"obliterate",    "obliterating",      99},
    {"kill",          "killing",           -1}
  };

  LWORD percent;
  LWORD i;

  percent = (damage * 100) / ( MAXV(Character(victim)->cHitP,1) );
  for(i=0; percent > severityStr[i].sPercent && severityStr[i].sPercent != -1; i++);
  if (field == FSS_NORMAL)
    strcpy(str, severityStr[i].sName);
  else
    strcpy(str, severityStr[i].sGerund);
}

BYTE *FightSprintf(BYTE *str, THING *victim, BYTE *messageStr, LWORD damage, WORD hitNum, BYTE *weaponName) {
  LWORD i;
  LWORD j=0;
  LWORD sLen;

  sLen = strlen(messageStr);
  for (i=0; i<=sLen; i++) {
    if (messageStr[i]=='#') {
      i++; /* get past # sign */
      switch (messageStr[i]) {
      case 'h': /* # of hits */
        sprintf(str+j, "%hd", hitNum);
        break;
      case 'd': /* relative wound severity */
        FightSprintfSeverity(str+j, victim, damage, FSS_NORMAL);
        break;
      case 'g': /* wound severity (gerund form) */
        FightSprintfSeverity(str+j, victim, damage, FSS_GERUND);
        break;
      case 'p': /* points of damage */
        sprintf(str+j, "%ld", damage);
        break;
      case 'n': /* weapon name */
        sprintf(str+j, "%s", weaponName);
        break;
      }
      j+=strlen(str+j); /* correct for added length */
    } else {
      str[j] = messageStr[i];
      j++;
    }
  }

  return str;
}

/* Thing is optional, its for crediting experience in case of a kill */
LWORD FightDamagePrimitive(THING *thing, THING *target, LWORD damage) {
  THING *code;

  if (!target) return TRUE;
  /* Make sure we cant hurt Gods */
  if (target->tType==TTYPE_PLR) {
    if (Character(target)->cLevel >= LEVEL_GOD)
      damage = 0;
    if ((Base(target)->bInside) &&
        (AreaIsEditor(AreaOf(Wld(Base(target)->bInside)->wVirtual),target)==2)) {
      damage = 0;
    }
  }

  /* do damage */
  Character(target)->cHitP -= damage;

  /* check for death */
  if (Character(target)->cHitP < 0) {
    if (thing) {
      /* see if special handler intercepts */
      code = Base(target)->bInside;
      if ((CodeParseDeath(thing, code, target)))
        return TRUE;
      for (code=code->tContain; code; code=code->tNext)
        if ((CodeParseDeath(thing, code, target))) 
          return TRUE;
      if (Base(thing)->bInside->tType == TTYPE_WLD)
        if (CodeParseDeath(thing, &areaList[Wld(Base(thing)->bInside)->wArea].aResetThing, target))
          return TRUE;
    }

    /* they're history */
    FightKill(thing, target); /* thing will kill target, target is dangling after this */
    target = NULL; /* so we dont forget and use by accident */
    return TRUE;
  } 
  return FALSE;
}

/* do some damage, guess we hit 'em - return TRUE if they croak */
LWORD FightDamage(THING *thing, LWORD damage, WORD hitNum, FLAG damFlag, WORD damType, BYTE *weaponName) {
  /* default miss messages */
  BYTE  defaultMissSrc[] =  "^7You miss $N with #n\n";
  BYTE  defaultMissDst[] =  "^8$n misses you with #n\n";
  BYTE  defaultMissRoom[] = "^9$n misses $N with #n\n";
  /* default single hit messages */
  BYTE  defaultSingleSrc[] =  "^7You #d $N with #n (#p pts)\n";
  BYTE  defaultSingleDst[] =  "^8$n #ds you with #n (#p pts)\n";
  BYTE  defaultSingleRoom[] = "^9$n #ds $N with #n (#p pts)\n";
  /* default multiple hit messages */
  BYTE  defaultMultipleSrc[] =  "^7You #d $N #h times with #n (#p pts)\n";
  BYTE  defaultMultipleDst[] =  "^8$n #ds you #h times with #n (#p pts)\n";
  BYTE  defaultMultipleRoom[] = "^9$n #ds $N #h times with #n (#p pts)\n";
  /* default death message */
  BYTE  defaultDeathSrc[] =  "^7You brutally slay $N with #n (#p pts)\n";
  BYTE  defaultDeathDst[] =  "^8$n brutally slays you with #n (#p pts)\n";
  BYTE  defaultDeathRoom[] = "^9$n brutally slay $N with #n (#p pts)\n";

  BYTE  buf[256];
  BYTE  *srcMessageStr;
  BYTE  *dstMessageStr;
  BYTE  *roomMessageStr;
  THING *target;

  target = Character(thing)->cFight;
  if (!target) return FALSE;

  /* compensate for damage flags, resist checks here */
  damage -= CharGetResist(target, damFlag)/10*hitNum;
  if (hitNum>0) MINSET(damage, 1);

  /* check if we are ambushing */
  if (BIT(Character(thing)->cAffectFlag, AF_HIDDEN)) {
    SendAction("^YYou step out of the shadows\n", 
      thing, NULL, SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST);
    SendAction("^Y$n steps out of the shadows\n",
      thing, NULL, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    /* No longer hidden */
    BITCLR(Character(thing)->cAffectFlag, AF_HIDDEN);
    thing->tWait+=3; /* commit to 3 rounds */
  }

  /* Determine which message to send */
  if (hitNum==0 && damage==0) {
    srcMessageStr  = defaultMissSrc;
    dstMessageStr  = defaultMissDst;
    roomMessageStr = defaultMissRoom;
  } else if (damage >= Character(target)->cHitP) {
    srcMessageStr  = defaultDeathSrc;
    dstMessageStr  = defaultDeathDst;
    roomMessageStr = defaultDeathRoom;
  } else if (hitNum > 1) {
    srcMessageStr  = defaultMultipleSrc;
    dstMessageStr  = defaultMultipleDst;
    roomMessageStr = defaultMultipleRoom;
  } else {
    srcMessageStr  = defaultSingleSrc;
    dstMessageStr  = defaultSingleDst;
    roomMessageStr = defaultSingleRoom;
  }
  
  /* Send the message */
  FightSprintf(buf, target, srcMessageStr, damage, hitNum, weaponName); /* fill in the blanks */
  SendAction(buf, thing, target, SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST);

  FightSprintf(buf, target, dstMessageStr, damage, hitNum, weaponName); /* fill in the blanks */
  SendAction(buf, thing, target, SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);

  FightSprintf(buf, target, roomMessageStr, damage, hitNum, weaponName); /* fill in the blanks */
  SendAction(buf, thing, target, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);

  /* start fighting back as necessary */

/* 
 * Whoops the exit logic here is a little screwy 
 */
  if (!Character(target)->cFight) FightStart(target, thing, Character(thing)->cFightRange, Character(thing)->cFightExit);
/*
 * Fix exit stuff here for ranged combat
 */

  return FightDamagePrimitive(thing, target, damage);
}

/* Try to hit current target with active weapon */
void FightAttack(THING *thing) {
  WORD      hitNum;
  WORD      damType=0;
  FLAG      damFlag=0;
  LWORD     damage=0;
  LWORD     i;
  LWORD     cost;
  BYTE      weaponName[256];
  AFFECT   *affect;

  /* normal invisibility will be expired by this */
  affect = AffectFind(thing, EFFECT_INVISIBILITY);
  if (affect) AffectRemove(thing, affect);

  if (thing->tType==TTYPE_PLR 
   && Character(thing)->cLevel<LEVEL_GOD 
   && Character(thing)->cLevel>=LEVEL_FIGHTTIRING) {
    cost = FIGHT_MOVECOST;
    cost = CharMoveCostAdjust(thing, cost);

    if (cost > Character(thing)->cMoveP) {
      SendThing("^wYOU'RE TOO EXHAUSTED TO FIGHT!\n", thing);
      Character(thing)->cMoveP += 1; /* be nice - recover a little bit */
      return;
    } else {
      Character(thing)->cMoveP -= cost;
    }
    if (cost*5 > Character(thing)->cMoveP)
      SendHint("^;HINT: You're getting tired - ^<FLEE^; NOW!\n", thing);
  }

  /*
   * For multiple weapons simply pass a weapon parm to
   * FightCheckHit and FightWeaponDamage
   *
   */
  if(FightCheckHit(thing, &hitNum)) {
    damage=0;
    for (i=0; i<hitNum; i++)
      damage += FightWeaponDamage(thing, &damFlag, &damType);
  }
  /* this is lame, do something better later */
  if (Character(thing)->cWeapon) {
    strcpy(weaponName, Character(thing)->cWeapon->tSDesc->sText);
  } else {
    PROPERTY *property;
    property = PropertyFind(thing->tProperty, "%WEAPONSDESC");
    if (!property && thing->tType==TTYPE_MOB)
      property = PropertyFind(Mob(thing)->mTemplate->mProperty, "%WEAPONSDESC");
    if (!property)
      strcpy(weaponName, "a flurry of punches");
    else
      strcpy(weaponName, property->pDesc->sText);
  }

  FightDamage(thing, damage, hitNum, damFlag, 0, weaponName);
  FightReload(thing);

  /* After Attack Event goes here */
  if (Character(thing)->cFight) {
    LWORD block;

    ThingSetEvent(thing);
    block = CodeParseAfterAttack(thing, thing, Character(thing)->cFight);
    if (block && ThingIsEvent(thing)) FightStop(thing);
    ThingDeleteEvent(thing);
  }
}

/* kill target */
void FightKill(THING *thing, THING *target) {
  THING *corpse;
  THING *money;
  THING *code;
  THING *dest;
  BYTE   buf[256];
  SOCK  *sock;
  LWORD  gain;
  FLAG   cFlag = OCF_CORPSE;
  LWORD  relLevel; /* relative level */
  LWORD  bank;
  LWORD  infamy;
  LWORD  fightStart=FALSE;

  /* FightStop removes the FIGHTSTART bit */
  if ( thing && BIT(Character(thing)->cAffectFlag, AF_FIGHTSTART) )
    fightStart = TRUE;

  /* Stop everybody from attacking this pointer */
  FightStop(target);
  CharKillFollow(target);

  /* Manufacture a "corpse" */
  if (target->tType!=TTYPE_MOB || Mob(target)->mTemplate!=spiritTemplate) {
    corpse = ObjectCreate(corpseTemplate, Base(target)->bInside);
    sprintf(buf, FIGHT_CORPSE_KEY, Base(target)->bKey->sText);
    STRFREE(Base(corpse)->bKey);
    Base(corpse)->bKey = STRCREATE(buf);
    sprintf(buf, "The remnants of %s", target->tSDesc->sText);
    STRFREE(corpse->tSDesc);
    corpse->tSDesc = STRCREATE(buf);
    sprintf(buf, "The remnants of %s are lying here", target->tSDesc->sText);
    STRFREE(Base(corpse)->bLDesc);
    Base(corpse)->bLDesc = STRCREATE(buf);
    OBJECTSETFIELD(corpse, OF_CONTAINER_ROT, 4);
    if (target->tType == TTYPE_MOB
      &&BIT(mTypeList[Mob(target)->mTemplate->mType].mTFlag, MT_ROBOT))
      cFlag |= OCF_ELECTRONIC;
    OBJECTSETFIELD(corpse, OF_CONTAINER_CFLAG, cFlag);

    /* expire affects in case they are carrying psi-items */
    while(Character(target)->cAffect) 
      AffectFree(target, Character(target)->cAffect);
    /* Move all the stuff into the corpse */
    while (target->tContain) ThingTo(target->tContain, corpse);
  
    /* if they carry money then create a money object inside the corpse */
    if (Character(target)->cMoney>0)
      money = ObjectCreateMoney(Character(target)->cMoney, corpse);
    
    /* If their corpse is worth money set its value */
    if (target->tType==TTYPE_MOB 
    && !BIT(mTypeList[Mob(target)->mTemplate->mType].mTFlag, MT_HASMONEY)) {
      OBJECTSETFIELD(corpse, OF_CONTAINER_SVALUE, Mob(target)->mTemplate->mMoney);
    }
  }

  /* See if we are killing a player */
  if (target->tType == TTYPE_PLR) {
    cFlag |= OCF_PLAYERCORPSE;
    OBJECTSETFIELD(corpse, OF_CONTAINER_CFLAG, cFlag);
    OBJECTSETFIELD(corpse, OF_CONTAINER_ROT, 10);
    Log(LOG_USAGE, target->tSDesc->sText);
    if (thing) {
      LogPrintf(LOG_USAGE, " just kicked the bucket... (killed by ");
      LogPrintf(LOG_USAGE, thing->tSDesc->sText);
      LogPrintf(LOG_USAGE, ")\n");
    } else {
      LogPrintf(LOG_USAGE, " just kicked the bucket...\n");
    }
    SendThing(fileList[FILE_DEATH].fileStr->sText, target);
    sprintf(buf, "%s", target->tSDesc->sText);

    /* Set infamy */
    if (thing && thing->tType==TTYPE_PLR && fightStart && !PlayerIsInfamous(target)) {
      LWORD gain = 2;
      BYTE  buf[256];

      if (Character(thing)->cLevel - Character(target)->cLevel > 5)
        gain = 3;
      sprintf(buf, "^yFor murdering another player, you gain %ld points of ^wINFAMY^w!!\n", gain);
      PlayerGainInfamy(thing, gain, buf);
    }

    /* execute afterdeath code scripts (if any) */
    code = Base(target)->bInside;
    CodeParseAfterDeath(thing, code, target);
    for (code=code->tContain; code; code=code->tNext)
      CodeParseAfterDeath(thing, code, target); 
    if (thing && Base(thing)->bInside->tType == TTYPE_WLD)
      CodeParseAfterDeath(thing, &areaList[Wld(Base(thing)->bInside)->wArea].aResetThing, target);

    /* Find players socket */
    sock = BaseControlFind(target);
    /* hmm, guess we'll search the socket list for an out of body player */
    if (!sock) {
      sock = sockList;
      while (sock && sock->sControlThing!=target) sock = sock->sNext;
      if (sock) {
        /* return to their real body */
        BaseControlFree(target, sock);
      }
    }
    /* Okay now make 'em dead */
    if (sock) {
      bank = Plr(target)->pBank;
      infamy = Plr(target)->pInfamy;
      sock->sHomeThing = Thing( PlayerRead(buf, PREAD_CLONE) );
      /* some things dont reset */
      Plr(sock->sHomeThing)->pBank     = Plr(target)->pBank;
      Plr(sock->sHomeThing)->pInfamy   = Plr(target)->pInfamy;
      Plr(sock->sHomeThing)->pAuto     = Plr(target)->pAuto;
      Plr(sock->sHomeThing)->pSockPref = Plr(target)->pSockPref;
      Plr(sock->sHomeThing)->pSystem   = Plr(target)->pSystem;
      THINGFREE(target); /* will stop fighting automaticly */
      BaseControlAlloc(sock->sHomeThing, sock); /* make new return pointer */
      target = sock->sHomeThing;
      dest = NULL;
      if (Plr(target)->pDeathRoom>0)
        dest = WorldOf(Plr(target)->pDeathRoom);
      if (!dest) dest = WorldOf(playerStartRoom);
      ThingTo(sock->sHomeThing, dest);
    } else {
      /* they're dead and not connected sign them off */ 
      /* have to read in clone copy stats, then update playerfile */
      THINGFREE(target); /* not good because they arent saved as having lost all */
    }
  } else {
    /* If we are an out of body player return to home body */
    sock = BaseControlFind(target);
    if (sock) {
      BaseControlFree(target, sock);
      BaseControlAlloc(sock->sHomeThing, sock);
    }
  
    /* If they are a mob and we're a player, gain exp/aura */
    if (target->tType==TTYPE_MOB && thing) {
      /* Gain experience */
      gain = Mob(target)->mTemplate->mExp;
      relLevel = Character(GroupGetHighestLevel(thing))->cLevel - Character(target)->cLevel;
      BOUNDSET(-8, relLevel, 8);
      gain -= ( (gain>>3) * relLevel );
      MINSET(gain, 1);
      if (Character(thing)->cLevel>=LEVEL_GOD-1)
        gain = 0;
      CharGainExpFollow(thing, gain);
      
      if (thing->tType == TTYPE_PLR) {
        /* Affect Aura */
        gain = abs(Character(target)->cAura);
        gain = gain / MAXV(50, abs(Character(thing)->cAura));
        if (Character(target)->cAura > 0) {
          Character(thing)->cAura -= gain;
        } else {
          Character(thing)->cAura += gain;
        }
        Character(thing)->cAura = BOUNDV(-1000, Character(thing)->cAura, 1000);
      }
    }
    /* execute afterdeath code scripts (if any) */
    code = Base(target)->bInside;
    CodeParseAfterDeath(thing, code, target);
    for (code=code->tContain; code; code=code->tNext)
      CodeParseAfterDeath(thing, code, target); 
    if (thing && Base(thing)->bInside->tType == TTYPE_WLD)
      CodeParseAfterDeath(thing, &areaList[Wld(Base(thing)->bInside)->wArea].aResetThing, target);

    /* Remove the target from play */
    ThingFree(target); /* will stop fighting automaticly */
  }

  /* Look around */
  if (sock) CmdLook(sock->sHomeThing, "");

  /* autoloot corpse if we are set to do so */
  if (thing && thing->tType==TTYPE_PLR && BIT(Plr(thing)->pAuto, PA_AUTOLOOT))
    CmdGet(thing, "get all remnants");
}

/* Nothing should ever delete items from this fightIndex except for us */
void FightIdle(void) {
  LWORD  block;
  LWORD  i;
  THING *thing;

  /* Check for fighting */
  for (i=0; i<fightIndex.iNum; i++) {
    /* attack opponent */
    if (fightIndex.iThing[i]) {
      /* check for an attached script */
      block=0;
      if (BIT(fightIndex.iThing[i]->tFlag,TF_FIGHTINGCODE))
        block=CodeParseFighting(Character(fightIndex.iThing[i])->cFight,fightIndex.iThing[i]);
      if (!block)
        FightAttack(fightIndex.iThing[i]);
    }
  }

  /* Check for fleeing */
  for (i=0; i<fightIndex.iNum; i++) {
    thing = fightIndex.iThing[i];
    if (thing) {
      if ( (thing->tType==TTYPE_PLR&&BIT(Plr(thing)->pAuto, PA_AUTOFLEE))
         ||(thing->tType==TTYPE_MOB&&BIT(Mob(thing)->mTemplate->mAct, MACT_WIMPY))) {
        if (Character(thing)->cHitP < CharGetHitPMax(thing)/20) {
          CmdFlee(thing, "");
        }
      }
    }
  }
  
  /* turf NULL entries from fightIndex 
   * Note the funny loop which occurs because indexdelete will swap in a fresh
   * entry into the same numeric position as the deleted entry when it is called
   * against an unsorted index
   */
  i=0;
  thing = (void*)0xffffffff;
  if (fightIndex.iNum==0) return;
  while(1) {
    if (thing == fightIndex.iThing[i]) i++;
    if (i>=fightIndex.iNum) break;
    thing = fightIndex.iThing[i];
    if (!thing)
      IndexDelete(&fightIndex, NULL, NULL);
  }

}

/* can the target hit us back */
LWORD FightCanHitBack(THING *thing, THING *target, EXIT *exit, EXIT **reverse) {
  LWORD weaponRange = 0;
  LWORD range = 1; /* 1 is inside the same room,
                      thus a weapon with a range
                      of 0 isnt very usefull */

  if (Base(thing)->bInside == Base(target)->bInside)
    return range;
  if (!exit)
    return 0;

  if (Character(target)->cWeapon)
    OBJECTGETFIELD(Character(target)->cWeapon, OF_WEAPON_RANGE);
  else if (target->tType==TTYPE_MOB)
    weaponRange = PropertyGetLWord(target, "%FightRange", 1);

  while (exit) {
    range++;
    if (range > weaponRange)
      break;
    *reverse = ExitReverse(Base(thing)->bInside, exit);
    if (!*reverse)
      return range;
    if (exit->eWorld == Base(target)->bInside)
      return range;
    exit = ExitDir(Wld(exit->eWorld)->wExit, exit->eDir);
  }
  return 0;
}

void FightStart(THING *thing, THING *target, BYTE range, EXIT *exit) {
  EXIT  *reverse;
  THING *i;
  BYTE   rescueBlock;

  if (!thing || !target)
    return;

  if (thing->tType<TTYPE_CHARACTER || target->tType<TTYPE_CHARACTER)
    return;

  /* no point in attacking ourselves */
  if (thing == target) {
    return;
  }

  /* stop following the thing we just attacked */
  if (Character(thing)->cLead == target)
    CharRemoveFollow(thing);

  /* If we are allready fighting dont re-add to index */
  if (!Character(thing)->cFight)
    IndexAppend(&fightIndex, thing);

  BITSET(Character(thing)->cAffectFlag, AF_FIGHTSTART);
  Character(thing)->cFight = target;
  Character(thing)->cFightExit = exit;
  Character(thing)->cFightRange = range;
  Character(thing)->cPos = POS_FIGHTING;

  /* Have victim attack back */
  if (!Character(target)->cFight 
      && FightCanHitBack(thing, target, exit, &reverse)) {
    Character(target)->cFight = thing;
    Character(target)->cFightExit = reverse;
    Character(target)->cFightRange = range;
    Character(target)->cPos = POS_FIGHTING;
    IndexAppend(&fightIndex, target);
    
    /* if the target has friends that will autoassist, have them join in */
    /*if (BIT(Character(target)->cAffectFlag, AF_GROUP)) {*/
    if (Character(target)->cLead) {
      for (i = GroupGetHighestLeader(target); i; i = Character(i)->cFollow) {
        /* if we are not allready fighting someone else & are nearby, possibly join in */
        if (!Character(i)->cFight && Base(i)->bInside==Base(target)->bInside) {
          /* Players AutoAssist as preferred */
          if (i->tType==TTYPE_PLR && BIT(Plr(i)->pAuto, PA_AUTOASSIST)) {
            FightStart(i, thing, range, reverse);

          /* Mobs allways assist */
          /* } else if (i->tType == TTYPE_MOB && BIT(Character(i)->cAffectFlag, AF_GROUP)) {*/
          } else if (i->tType == TTYPE_MOB && GroupIsGroupedMember(i,target)) {
            FightStart(i, thing, range, reverse);
          }
        }
      }
    }
   
  }

  /* if we have friends that will autoassist, have them join in */
  /*if (BIT(Character(thing)->cAffectFlag, AF_GROUP)) {*/
  if (Character(thing)->cLead) {
    for (i = GroupGetHighestLeader(thing); i; i = Character(i)->cFollow) {
      /* if we are not allready fighting someone else & are nearby, possibly join in */
      if (!Character(i)->cFight && Base(i)->bInside==Base(thing)->bInside) {
        /* Players AutoAssist as preferred */
        if (i->tType==TTYPE_PLR && BIT(Plr(i)->pAuto, PA_AUTOASSIST)) {
          FightStart(i, target, range, reverse);

        /* Mobs allways assist */
        } else if (i->tType == TTYPE_MOB && GroupIsGroupedMember(i,thing)) {
          FightStart(i, target, range, reverse);
        }
      }
    }
  }

  /* If your targets friends are really nice they will Autorescue */
  rescueBlock = FALSE;
  if ((target->tType==TTYPE_PLR && !BIT(Plr(target)->pAuto, PA_AUTORESCUE))
    ||(target->tType==TTYPE_MOB && !BIT(Mob(target)->mTemplate->mAct, MACT_RESCUE)))
  {
    /*if (BIT(Character(target)->cAffectFlag, AF_GROUP)) {*/
    if (Character(target)->cLead) {
      for (i = GroupGetHighestLeader(target); i; i = Character(i)->cFollow) {
        if (!rescueBlock && Base(i)->bInside==Base(target)->bInside) {
          /* Autorescue as preferred */
          if (i->tType==TTYPE_PLR && BIT(Plr(i)->pAuto, PA_AUTORESCUE)) {
            rescueBlock = CbtRescue(i, target);
          } else if (i->tType==TTYPE_MOB && BIT(Mob(i)->mTemplate->mAct, MACT_RESCUE) && GroupIsGroupedMember(i,target)) {
            rescueBlock = CbtRescue(i, target);
          }
        }
      }
    }
  }

}

/* 
 * Cant delete here just change "deleted" pointers to NULL
 * let FightIdle delete it so that we dont accidentally 
 * delete when its parsing through the loop
 */
void FightStop(THING *thing) {
  LWORD  i;

  if (!thing) return;
  for (i=0; i<fightIndex.iNum; i++) {
    if ( (fightIndex.iThing[i] == thing)
       ||(fightIndex.iThing[i] && Character(fightIndex.iThing[i])->cFight == thing)
    ) {
      Character(fightIndex.iThing[i])->cFight = NULL;
      Character(fightIndex.iThing[i])->cFightExit = NULL;
      Character(fightIndex.iThing[i])->cFightRange = 0;
      Character(fightIndex.iThing[i])->cPos = POS_STANDING;
      BITCLR(Character(fightIndex.iThing[i])->cAffectFlag, AF_FIGHTSTART);
      fightIndex.iThing[i] = NULL;
    }
  }
}

