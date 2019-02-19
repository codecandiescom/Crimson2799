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
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "str.h"
#include "property.h"
#include "queue.h"
#include "send.h"
#include "parse.h"
#include "thing.h"
#include "exit.h"
#include "world.h"
#include "base.h"
#include "char.h"
#include "group.h"
#include "mobile.h"
#include "player.h"
#include "affect.h"
#include "object.h"
#include "fight.h"
#include "skill.h"
#include "effect.h"
#include "cmd_move.h"
#include "cmd_cbt.h"


/* look for something to kill along the lines of <key> <direction> */
/* synonymous with shoot, kill */
CMDPROC(CmdHit) { /*(THING *thing, BYTE *cmd)*/ 
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *target;
  EXIT  *exit;
  BYTE   dir[256];
  LWORD  range = 1; /* 1 is inside the same room,
                       thus a weapon with a range
                       of 0 isnt very usefull */

  cmd = StrOneWord(cmd, NULL);
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  target = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_PLR|TF_MOB, &srcOffset);

  if (!target) {
    SendThing("^wWho was that again?\n", thing);
    return;
  }

  if (target->tType==TTYPE_PLR) {
    if (thing->tType==TTYPE_PLR && Character(thing)->cLevel<6) {
      SendThing("^wYou must be at least 6th level before you may attack other players\n", thing);
      return;
    }
  }

  if (target) {
    FightStart(thing, target, 1, NULL);
    return;
  }

  /* the following is for ranged attacks */
return;

  /* Look to see if they are aiming somewhere */
  cmd = StrOneWord(cmd, dir);
  if (!*dir) return;
  if (!Character(thing)->cWeapon) return;
  if (Character(thing)->cWeapon->tType != TTYPE_OBJ) return;
  if (Obj(Character(thing)->cWeapon)->oTemplate->oType != OTYPE_WEAPON) return;

  exit = ExitFind(Wld(Base(thing)->bInside)->wExit, dir);
  while (exit) {
    range++;
    if (range > OBJECTGETFIELD(Character(thing)->cWeapon, OF_WEAPON_RANGE))
      break;
    target = CharThingFind(thing, srcKey, -1, exit->eWorld, TF_PLR|TF_MOB, &srcOffset);
    if (target) {
      FightStart(thing, target, 1, NULL);
      return;
    }
    exit = ExitDir(Wld(exit->eWorld)->wExit, exit->eDir);
  }
}

/* Add/Remove/View Current Group */
/* It would be cool if this tacked: Re, As, Lo etc combat related
 * flags after group members names
 */
CMDPROC(CmdGroup) { /*(THING *thing, BYTE *cmd)*/ 
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *target;
  BYTE   buf[256];
  BYTE   share[256];
  LWORD  followLevel = 0;
  LWORD  followNumber = 0;
  THING *i;
  THING *tree[CHAR_MAX_FOLLOW];
  LWORD  treePos;
  LWORD  j;
  LWORD  skip;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    if (!Character(thing)->cLead) {
      SendThing("You're not IN a group\n", thing);
      return;
    }
    
    /* We're going to print them a pretty picture of their group. */
     
    /* Collect Statistics on the group */
    GroupGetStat(thing,&followNumber,&followLevel);
    
    /* Show them the group */
    SendThing("^cGroup Member         ^gLevel  XP Share    Hits   Move   Power\n", thing);
    SendThing("^C-+-+-+-+-+-+         ^G-+-+-  +-+-+-+-    -+-+   -+-+   -+-+-\n", thing);
    /* ok, the stuff to follow involves recursion. That's why it's 
     * so complicated. */
    tree[0]=GroupGetHighestLeader(thing);
    tree[1]=tree[0];
    treePos=1;
    skip=0;
    do {
      /* let's print everyone at this level by this leader. */
      for (;  
        tree[treePos];  
        tree[treePos]=Character(tree[treePos])->cFollow) {
        if (Character(tree[treePos])->cLead==tree[treePos-1]) {
          /* print this character */
          for (j=0;j<(treePos-1);j++)    
            SendThing(" ", thing);
          i=tree[treePos];
          if (GroupIsGroupedMember(tree[treePos],thing)) {
            sprintf(share, "%ld%%", Character(i)->cLevel*100/followLevel);
            sprintf(buf,"^c%-15s",i->tSDesc->sText);
            SendThing(buf,thing);
            for (j=0;j<(7-treePos);j++)
              SendThing(" ", thing);
            sprintf(buf, 
                    "^g%-5hd  %-8s    %-5ld  %-5ld  %-5ld ", 
                    Character(i)->cLevel, 
                    share,
                    Character(i)->cHitP,
                    Character(i)->cMoveP,
                    Character(i)->cPowerP);
            if (i->tType == TTYPE_PLR) {
              if (BIT(Plr(i)->pAuto, PA_AUTOLOOT))
                strcat(buf, "Lo");
              if (BIT(Plr(i)->pAuto, PA_AUTOAGGR))
                strcat(buf, "Ag");
              if (BIT(Plr(i)->pAuto, PA_AUTORESCUE))
                strcat(buf, "Re");
              if (BIT(Plr(i)->pAuto, PA_AUTOASSIST))
                strcat(buf, "As");
            } else if (i->tType == TTYPE_MOB) {
              if (BIT(Mob(i)->mTemplate->mAct, MACT_RESCUE))
                strcat(buf, "Re");
              if (BIT(Mob(i)->mTemplate->mAct, MACT_AGGRESSIVE))
                strcat(buf, "Ag");
              if (BIT(Mob(i)->mTemplate->mAct, MACT_SCAVENGER))
                strcat(buf, "Sc");
            }
            strcat(buf, "\n");
          } else {
            sprintf(buf,"^C%-15s",i->tSDesc->sText);
            SendThing(buf,thing);
            for (j=0;j<(7-treePos);j++)
              SendThing(" ", thing);
            sprintf(buf,"^W<NOT GROUPED>\n");
          }
          SendThing(buf,thing);
          /* check if we need to recurse deeper */
          if (GroupIsLeader(tree[treePos])&&(tree[treePos]!=tree[treePos-1])){
            treePos++; 
            tree[treePos]=GroupGetHighestLeader(thing);
            skip=1;
            break;
          }
        }
      }
      if (!skip) {
        treePos--;
        tree[treePos]=Character(tree[treePos])->cFollow;
      }
      skip=0;
    } while(treePos>0);
    
    
 /*     if (i == GroupGetHighestLeader(i)) {
        if (BIT(Character(i)->cAffectFlag, AF_GROUP)) {
          if (BIT(Character(thing)->cAffectFlag, AF_GROUP)) {
            sprintf(share, "%ld%%", Character(i)->cLevel*100/followLevel);
            sprintf(buf, 
                    "^c%-20s ^g%-5hd  %-8s    %-5ld  %-5ld  %-5ld ", 
                    i->tSDesc->sText, 
                    Character(i)->cLevel, 
                    share,
                    Character(i)->cHitP,
                    Character(i)->cMoveP,
                    Character(i)->cPowerP);
            if (i->tType == TTYPE_PLR) {
              if (BIT(Plr(i)->pAuto, PA_AUTOLOOT))
                strcat(buf, "Lo");
              if (BIT(Plr(i)->pAuto, PA_AUTOAGGR))
                strcat(buf, "Ag");
              if (BIT(Plr(i)->pAuto, PA_AUTORESCUE))
                strcat(buf, "Re");
              if (BIT(Plr(i)->pAuto, PA_AUTOASSIST))
                strcat(buf, "As");
            } else if (i->tType == TTYPE_MOB) {
              if (BIT(Mob(i)->mTemplate->mAct, MACT_RESCUE))
                strcat(buf, "Re");
              if (BIT(Mob(i)->mTemplate->mAct, MACT_AGGRESSIVE))
                strcat(buf, "Ag");
              if (BIT(Mob(i)->mTemplate->mAct, MACT_SCAVENGER))
                strcat(buf, "Sc");
            }
            strcat(buf, "\n");
          } else {
            sprintf(buf, 
                    "^c%-20s ^w<GROUPED>\n", 
                    i->tSDesc->sText);
          }
        } else {
          sprintf(buf, 
                  "^c%-20s ^w<NOT GROUPED>\n", 
                  i->tSDesc->sText);
        }
      } else if (Character(i)->cLead==thing) {
        sprintf(buf, 
                "^c%-20s ^y<FOLLOWING YOU>\n", 
                i->tSDesc->sText);
      } else if (Character(i)->cLead==i) {
        sprintf(buf, 
                "^c%-20s ^y<LEADING>\n", 
                i->tSDesc->sText);
      } else if (i==Character(thing)->cLead && Character(i)->cLead) {
        sprintf(buf, 
                "^c%-20s ^p<FOLLOWING> %s, <LEADING YOU>\n", 
                i->tSDesc->sText,
                Character(i)->cLead->tSDesc->sText);
      } else if (Character(i)->cLead) {
        sprintf(buf, 
                "^c%-20s ^p<FOLLOWING> %s\n", 
                i->tSDesc->sText,
                Character(i)->cLead->tSDesc->sText);
      } else
        buf[0]=0;
        
      if (buf[0]) SendThing(buf, thing);
    }*/
    
    sprintf(buf, "\n^wPeople in Group: ^c%ld, ^wTotal Level: ^c%ld\n", followNumber, followLevel);
    SendThing(buf, thing);
    return;
  }

  
  /* add or remove target as appropriate */

  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  target = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_PLR|TF_MOB, &srcOffset);
  if (target) {
    if (Character(target)->cLead != thing) {
      SendThing("They're not following you\n", thing);
    } else if (target==thing) {
      SendThing("You are always grouped within the group you lead\n", thing);
    } else {
      BITFLIP(Character(target)->cAffectFlag, AF_GROUP);
      if (!GroupVerifySize(thing)) {
        BITFLIP(Character(target)->cAffectFlag, AF_GROUP);
	SendAction("You cannout group $N: your group would be too big!\n",
	  thing, target, SEND_SRC|SEND_CAPFIRST);
	SendAction("$n tried to group you but failed: $n's group would be too big!\n",
	  thing, target, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
	SendAction("$n tried to group $N but failed: $n's group would be too big!\n",
	  thing, target, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
	return;
      }
      if (BIT(Character(target)->cAffectFlag, AF_GROUP)) {
        SendAction("You add $N to your group\n", 
       thing, target, SEND_SRC |SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("$n adds you to $s group\n", 
       thing, target, SEND_DST |SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("$n adds $N to $s group\n",   
       thing, target, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
      } else {
        SendAction("You kick $N out of your group\n", 
       thing, target, SEND_SRC |SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("$n kicks you out of $s group\n", 
       thing, target, SEND_DST |SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("$n kicks $N out of $s group\n",   
       thing, target, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
      }
    }
  }
} 


/* Follow / Unfollow somebody */
CMDPROC(CmdFollow) { /*(THING *thing, BYTE *cmd)*/ 
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *target;

  /*
   * Should check for domination here, the dominated cant change who
   * they are following 
   */

  cmd = StrOneWord(cmd, NULL);
  /* Stop following */
  if (!*cmd) {
    if ((!Character(thing)->cLead) || (Character(thing)->cLead==thing)) {
      SendThing("^wYes, but who do you want to follow...\n", thing);
      return;
    }
    SendAction("You stop following $N\n", 
         thing, Character(thing)->cLead, SEND_SRC |SEND_AUDIBLE|SEND_CAPFIRST);
    SendAction("$n stops following you\n", 
         thing, Character(thing)->cLead, SEND_DST |SEND_AUDIBLE|SEND_CAPFIRST);
    SendAction("$n stops following $N\n",   
         thing, Character(thing)->cLead, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    CharRemoveFollow(thing);
    return;
  }


  /* Follow the target */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  target = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_PLR|TF_MOB, &srcOffset);
  if (target) {
    if (target == thing) {
      SendThing("^wYou start to follow yourself\n", thing);
      CharRemoveFollow(thing);
      return;
    }

    if (CharAddFollow(thing, target)) {
      SendAction("You start following $N\n", 
                 thing, Character(thing)->cLead, SEND_SRC |SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("$n starts following you\n", 
                 thing, Character(thing)->cLead, SEND_DST |SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("$n starts following $N\n",   
                 thing, Character(thing)->cLead, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    } else {
      SendThing("I'm afraid they've got too many followers allready\n", thing);
    }
  }
}

/*
 * I could make this so that it pulls the text out of the skill table,
 * but I'd rather have the minor annoyance of double-entering the text
 * than a non-standard text lookup method 
 */
struct PsiListType psiList[] = {
  /* Body */
  { "Cell-Repair",     &SKILL_CELL_REPAIR,   EFFECT_CELL_REPAIR,  50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Refresh",         &SKILL_REFRESH,       EFFECT_REFRESH,      50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Endurance",       &SKILL_ENDURANCE,     EFFECT_ENDURANCE,    50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Breathwater",     &SKILL_BREATHWATER,   EFFECT_BREATHWATER,  50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Strength",        &SKILL_STRENGTH,      EFFECT_STRENGTH,     50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Darkvision",      &SKILL_DARKVISION,    EFFECT_DARKVISION,   50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Slow-Poison",     &SKILL_SLOW_POISON,   EFFECT_SLOW_POISON,  50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Cure-Poison",     &SKILL_CURE_POISON,   EFFECT_CURE_POISON,  50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Heal-Minor",      &SKILL_HEAL_MINOR,    EFFECT_HEAL_MINOR,   50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Regeneration",    &SKILL_REGENERATION,  EFFECT_REGENERATION, 50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Heal-Major",      &SKILL_HEAL_MAJOR,    EFFECT_HEAL_MAJOR,   100,25, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Dexterity",       &SKILL_DEXTERITY,     EFFECT_DEXTERITY,    50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Constitution",    &SKILL_CONSTITUTION,  EFFECT_CONSTITUTION, 50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Haste",           &SKILL_HASTE,         EFFECT_HASTE,        50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Quench",          &SKILL_QUENCH,        EFFECT_QUENCH,       50,  5, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Sustenance",      &SKILL_SUSTENANCE,    EFFECT_SUSTENANCE,   50,  5, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Acidtouch",       &SKILL_ACIDTOUCH,     EFFECT_ACIDTOUCH,    70, 10, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Poisontouch",     &SKILL_POISONTOUCH,   EFFECT_POISONTOUCH,  70, 10, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
   
  /* Telekinetic */                     
  { "Crush",           &SKILL_CRUSH,         EFFECT_CRUSH,        60, 20, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Forcestorm",      &SKILL_FORCESTORM,    EFFECT_FORCESTORM,   60, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Ghostfist",       &SKILL_GHOSTFIST,     EFFECT_GHOSTFIST,    60, 10, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Kinetic-Shield",  &SKILL_KINETIC_SHIELD,EFFECT_KINETIC_SHIELD,50,15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Immobilize",      &SKILL_IMMOBILIZE,    EFFECT_IMMOBILIZE,   50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Heartstop",       &SKILL_HEARTSTOP,     EFFECT_HEARTSTOP,    50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Asphyxiate",      &SKILL_ASPHYXIATE,    EFFECT_ASPHYXIATE,   50, 15, },
  { "Invisibility",    &SKILL_INVISIBILITY,  EFFECT_INVISIBILITY, 50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Slow",            &SKILL_SLOW,          EFFECT_SLOW,         50,  5, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "ImprovedInvis",   &SKILL_IMPROVEDINVIS, EFFECT_IMPROVEDINVIS,75, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Vacuumwalk",      &SKILL_VACUUMWALK,    EFFECT_VACUUMWALK,   50, 15, TAR_SELF_DEF},

  /* Telepathy */                     
  { "PhantomEar",      &SKILL_PHANTOMEAR,    EFFECT_PHANTOMEAR,   50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_NODEFAULT},
  { "PhantomEye",      &SKILL_PHANTOMEYE,    EFFECT_PHANTOMEYE,   50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_NODEFAULT},
  { "Mindlink",        &SKILL_MINDLINK,      EFFECT_MINDLINK,     50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_NODEFAULT},
  { "Domination",      &SKILL_DOMINATION,    EFFECT_DOMINATION,   75, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_NODEFAULT},
  { "Thoughtblade",    &SKILL_THOUGHTBLADE,  EFFECT_THOUGHTBLADE, 50,  5, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Mindcrush",       &SKILL_MINDCRUSH,     EFFECT_MINDCRUSH,    55, 10, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Deathdream",      &SKILL_DEATHDREAM,    EFFECT_DEATHDREAM,   60, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Mindshield",      &SKILL_MINDSHIELD,    EFFECT_MINDSHIELD,   50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Sleep",           &SKILL_SLEEP,         EFFECT_SLEEP,        50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_NODEFAULT},
  { "Berserk",         &SKILL_BERSERK,       EFFECT_BERSERK,      99, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Mindclear",       &SKILL_MINDCLEAR,     EFFECT_MINDCLEAR,    50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},

  /* Apportation */                     
  { "Teleport",        &SKILL_TELEPORT,      EFFECT_TELEPORT,     99, 25, TAR_SELF_DEF },
  { "Summon",          &SKILL_SUMMON,        EFFECT_SUMMON,       99, 25, TAR_PLR_WLD|TAR_MOB_WLD },
  { "Succor",          &SKILL_SUCCOR,        EFFECT_SUCCOR,       99, 25, TAR_SELF_DEF},
  { "Banish",          &SKILL_BANISH,        EFFECT_BANISH,       99, 25, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "MassTeleport",    &SKILL_MASSTELEPORT,  EFFECT_TELEPORT,     99, 25, TAR_GROUP_DEF },
  { "MassSuccor",      &SKILL_MASSSUCCOR,    EFFECT_SUCCOR,       99, 25, TAR_SELF_DEF|TAR_GROUPWLD_DEF },
  { "MassSummon",      &SKILL_MASSSUMMON,    EFFECT_SUMMON,       99, 25, TAR_SELF_DEF|TAR_GROUPWLD_DEF },
  { "DisruptDoor",     &SKILL_DISRUPTDOOR,   EFFECT_DISRUPTDOOR,  50, 15, },
  { "PhaseDoor",       &SKILL_PHASEDOOR,     EFFECT_PHASEDOOR,    50, 15, },
  { "Phasewalk",       &SKILL_PHASEWALK,     EFFECT_PHASEWALK,    50, 15, TAR_SELF_DEF},
  { "Phantompocket",   &SKILL_PHANTOMPOCKET, EFFECT_PHANTOMPOCKET,75, 25, TAR_SELF_DEF},
  { "Waterwalk",       &SKILL_WATERWALK,     EFFECT_WATERWALK,    50, 15, TAR_SELF_DEF},
  { "Teletrack",       &SKILL_TELETRACK,     EFFECT_TELETRACK,    75, 25, TAR_PLR_WLD|TAR_MOB_WLD},
  { "Recall",          &SKILL_RECALL,        EFFECT_RECALL,       75, 25, TAR_SELF_DEF },
  { "Mark",            &SKILL_MARK,          EFFECT_MARK,         75, 25, TAR_SELF_DEF },
  { "Translocate",     &SKILL_TRANSLOCATE,   EFFECT_TRANSLOCATE,  99, 30, TAR_SELF_DEF },

  /* Spirit */                     
  { "Spiritwalk",      &SKILL_SPIRITWALK,    EFFECT_SPIRITWALK,   50, 15, TAR_SELF_DEF },
  { "SenseLife",       &SKILL_SENSELIFE,     EFFECT_SENSELIFE,    50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF },
  { "SeeInvisible",    &SKILL_SEEINVISIBLE,  EFFECT_SEEINVISIBLE, 50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF },
  { "LuckShield",      &SKILL_LUCKSHIELD,    EFFECT_LUCKSHIELD,   99, 25, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF },
  { "Identify",        &SKILL_IDENTIFY,      EFFECT_IDENTIFY,    150, 50, TAR_OBJ_ROOM|TAR_OBJ_INV },
  { "Stat",            &SKILL_STAT,          EFFECT_STAT,        150, 50, TAR_PLR_ROOM|TAR_MOB_ROOM },
  { "Luckyhits",       &SKILL_LUCKYHITS,     EFFECT_LUCKYHITS,    99, 10, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF },
  { "Luckydamage",     &SKILL_LUCKYDAMAGE,   EFFECT_LUCKYDAMAGE,  99, 10, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF },

  /* Pyrokinetic */                     
  { "BurningFist",     &SKILL_BURNINGFIST,   EFFECT_BURNINGFIST,  50, 5,  TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "FlameStrike",     &SKILL_FLAMESTRIKE,   EFFECT_FLAMESTRIKE,  55, 10, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Incinerate",      &SKILL_INCINERATE,    EFFECT_INCINERATE,   60, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Ignite",          &SKILL_IGNITE,        EFFECT_IGNITE,       50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_FIGHT_DEF},
  { "Heatshield",      &SKILL_HEATSHIELD,    EFFECT_HEATSHIELD,   50, 15, TAR_PLR_ROOM|TAR_MOB_ROOM|TAR_SELF_DEF},
  { "Fireblade",       &SKILL_FIREBLADE,     EFFECT_FIREBLADE,    75, 20, TAR_SELF_DEF},
  { "Fireshield",      &SKILL_FIRESHIELD,    EFFECT_FIRESHIELD,   75, 20, TAR_SELF_DEF},
  { "Firearmor",       &SKILL_FIREARMOR,     EFFECT_FIREARMOR,    75, 20, TAR_SELF_DEF},

  { "",                0,                   0,                   0,   0,  0 }
};

/* Cast spells/Concentrate psionics etc */
CMDPROC(CmdConcentrate) { /*(THING *thing, BYTE *cmd)*/ 
  LWORD i;
  LWORD numShown;
  BYTE  buf[256];
  BYTE  line[256];
  LWORD cost;
  LWORD skill;
  
  #define SCALEFACTOR 50

  /* throwaway concentrate */
  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^wYes but the question remains, what should I cast... Your options are:\n", thing);
    numShown = 0;
    for(i=0; *psiList[i].pName; i++) {
      skill = CharGetSkill(thing, *(psiList[i].pSkill) );
      if (skill) {
        if (thing->tType==TTYPE_PLR && Character(thing)->cLevel >= LEVEL_GOD) {
          cost = 0;
        } else {
          cost = psiList[i].pMaxCost*SCALEFACTOR/( skill + SCALEFACTOR );
          MINSET(cost, psiList[i].pMinCost);
          MAXSET(cost, psiList[i].pMaxCost);
        }
        sprintf(line, "%s[%ld]", psiList[i].pName, cost);
        sprintf(buf,"%-19s", line);
        SendThing(buf, thing);
        numShown++;
        if (numShown%4 == 0) 
          SendThing("\n", thing);
      }
    }
    if (numShown%4 != 0)
      SendThing("\n", thing);
    return;
  }
  /* get the next word - ie what they cast */
  cmd = StrOneWord(cmd, buf);

  i = TYPEFIND(buf, psiList);
  if (i == -1) {
    SendThing("^wOh, good one... making this up as we go are we?", thing);
    return;
  }
  
  /* Can they cast it */
  skill = CharGetSkill(thing, *(psiList[i].pSkill) );
  if (skill <= 0) {
    SendThing("^rI'm afraid you're not very good at that sort of thing.\n", thing);
    return;
  }

  /* Calc Cost */  
  if (thing->tType==TTYPE_PLR && Character(thing)->cLevel >= LEVEL_GOD) {
    cost = 0;
  } else {
    cost = psiList[i].pMaxCost*SCALEFACTOR/( skill + SCALEFACTOR );
    MINSET(cost, psiList[i].pMinCost);
    MAXSET(cost, psiList[i].pMaxCost);
  }

  if (Character(thing)->cPowerP < cost) {
    SendThing("^rI'm afraid you dont have enough energy left for that sort of thing.\n", thing);
    return;
  }

  /* produce an effect */
  sprintf(buf, "^cYou concentrate briefly on ^g'%s' ^c(expending ^w%ld ^cpoints)\n", psiList[i].pName, cost);
  SendAction(buf, thing, NULL, SEND_SRC);
  SendAction("^c$n concentrates briefly.\n", thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  thing->tWait++; /* make it so they cant initiate a command for a "round" */
  if (!Effect(psiList[i].pEffect, psiList[i].pTarget, thing, cmd, skill)) {
    SendThing("^wUmm, yeah okay, maybe you should specify a target for that\n", thing);
    return;
  }
  Character(thing)->cPowerP -= cost;
}

CMDPROC(CmdPractice) { /*(THING *thing, BYTE *cmd)*/ 
  SkillShow(thing, cmd, 0xFFFFFFFF, 0, 0); /* Thing, Cmd, CanPractice, CantPractice, maxLevel */
                                           /* Since MaxLevel is 0, it is display only and will
                                            * not allow any actual practicing to take place
                                            */
}

void CbtConsider(THING *thing, THING *target) {
  LWORD  relLevel = 1;

  if (!thing || !target) return;
  if (thing->tType<TTYPE_CHARACTER || target->tType<TTYPE_CHARACTER) return;
  relLevel = Character(target)->cLevel - Character(thing)->cLevel;
/* Tough fight */
  if (relLevel > 10) {
    SendThing("^wIt gives you the willies just thinking about it. Brrrr...\n", thing);
  } else if (relLevel > 5) {
    SendThing("^wWith a great group and a little luck you might be able to pull it off\n", thing);
  } else if (relLevel > 2) {
    SendThing("^wTheres safety in numbers, I wouldnt try this alone\n", thing);
  } else if (relLevel > 0) {
    SendThing("^wA little luck and you're there\n", thing);

/* Equal Fight */
  } else if (relLevel == 0) {
    SendThing("^wHeads you win.....\n", thing);

/* Easy Fight */
  } else if (relLevel < 10) {
    SendThing("^wSure you could do it, but why bother\n", thing);
  } else if (relLevel < 5) {
    SendThing("^wYou could take 'em in your sleep\n", thing);
  } else if (relLevel < 2) {
    SendThing("^wI dont think its going to put up too stiff a fight\n", thing);
  } else if (relLevel < 0) {
    SendThing("^wAs long as it doesnt have something up its sleeves\n", thing);
  }

  /* Warn if their weapon sucks */
}

CMDPROC(CmdConsider) { /*(THING *thing, BYTE *cmd)*/ 
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *target;

  cmd = StrOneWord(cmd, NULL);
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  target = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_PLR|TF_MOB, &srcOffset);
  if (!target) {
    SendThing("^wWho was that again?\n", thing);
    return;
  }
  CbtConsider(thing, target);
}

BYTE Flee(THING *thing) {
  EXIT  *exit;
  LWORD  exitNum;
  LWORD  success = FALSE;
  LWORD  check;

  /* additional movement point cost check here */
  if (Character(thing)->cMoveP < 10)
    return FALSE;
  Character(thing)->cMoveP -= 10;

  check = DiceOpenEnded(1, 100, 100);
  if (thing->tType == TTYPE_PLR) {
    check += Plr(thing)->pSkill[SKILL_FLEE];
  } else if (thing->tType == TTYPE_MOB) {
    check += PropertyGetLWord(thing, "%Flee", Mob(thing)->mTemplate->mLevel*2);
  }
  if (Character(thing)->cFight) {
    if (Character(thing)->cFight->tType == TTYPE_PLR) {
      check -= Plr(Character(thing)->cFight)->pSkill[SKILL_PURSUE];
    } else if (Character(thing)->cFight->tType == TTYPE_MOB) {
      check -= PropertyGetLWord(Character(thing)->cFight, "%Pursue", Mob(Character(thing)->cFight)->mTemplate->mLevel*2);
    }
  }

  /* See if they made it */
  if (check >= 50) { /* 50% chance of fleeing */
    /* count exits */
    for (exitNum=0,exit=Wld(Base(thing)->bInside)->wExit; exit; exit=exit->eNext) exitNum++;
    /* pick a random direction */
    exitNum = Number(1,exitNum);
    for (exit=Wld(Base(thing)->bInside)->wExit; exit&&exitNum!=1; exit=exit->eNext,exitNum--);
    /* see if we can go this way */
    if(exit && exit->eWorld)
      success = MoveExit(thing, exit);
  } 

  if (!Character(thing)->cFight) {
    SendThing("^bAck, Eeek, panic, panic... ^P(^pWhat are we running from again?^P)\n", thing);
  } else if (!success) {
    SendThing("^wAck, Eeek, panic, panic... ^P(^bYOU COULDNT GET AWAY^P)\n", thing);
  } else {
    SendThing("^wRun Away! Run Away!\n", thing);
  }
  
  return success;
}

CMDPROC(CmdFlee) { /*(THING *thing, BYTE *cmd)*/ 
  /*
   Check for code script interception
   */

  if (!Flee(thing))
    thing->tWait++; /* make it so they cant initiate a command for a "round" */
}

BYTE CbtRescue(THING *thing, THING *target) {
  LWORD  check;
  LWORD  cost;

  if (!thing || !target || Base(target)->bInside!=Base(thing)->bInside)
    return FALSE;

  if (!Character(target)->cFight) {
    /* found somebody to rescue */
    SendThing("^wThey're not in need of rescuing right now...\n", thing);
    return FALSE;
  }

  thing->tWait+=3; /* make it so theycant initiate a command for a "round" */
  cost = RESCUE_MOVE_COST;
  cost = CharMoveCostAdjust(thing, cost);
  if (Character(thing)->cMoveP < cost) {
    /* failed the rescue */
    SendAction("^YYou try to rescue $N, ^ybut are too exhausted!\n", 
      thing, target, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^Y$n tries to rescue you, ^ybut $h is too exhausted!!\n", 
      thing, target, SEND_DST |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^Y$n tries to rescue $N, ^ybut is too exhausted!\n",   
      thing, target, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    return FALSE;
  }
  Character(thing)->cMoveP -= cost;

  check = DiceOpenEnded(1, 100, 100);
  if (thing->tType == TTYPE_PLR) {
    check += Plr(thing)->pSkill[SKILL_RESCUE];
  } else if (thing->tType == TTYPE_MOB) {
    check += PropertyGetLWord(thing, "%Rescue", Mob(thing)->mTemplate->mLevel*2);
  }
  if (Character(target)->cFight->tType == TTYPE_PLR) {
    check -= Plr(Character(target)->cFight)->pSkill[SKILL_PURSUE];
  } else if (Character(target)->cFight->tType == TTYPE_MOB) {
    check -= PropertyGetLWord(Character(target)->cFight, "%Pursue", Mob(Character(target)->cFight)->mTemplate->mLevel*2);
  } else
    return FALSE;

  if (check >= 75) {
    /* Made the rescue */
    SendAction("^yYou rescue $N\n", 
      thing, target, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^y$n rescues you\n", 
      thing, target, SEND_DST |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^y$n rescues $N\n",   
      thing, target, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);

    FightStart(thing, Character(target)->cFight, Character(target)->cFightRange, Character(target)->cFightExit);
    Character(Character(target)->cFight)->cFight = thing;
    Character(thing)->cFight = Character(target)->cFight;
    return TRUE;
  } else {
    /* failed the rescue */
    SendAction("^YYou try to rescue $N, ^ybut FAIL!\n", 
      thing, target, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^Y$n tries to rescue you, ^ybut FAILS!\n", 
      thing, target, SEND_DST |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^Y$n tries to rescue $N, ^ybut FAILS!\n",   
      thing, target, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    return FALSE;
  }
}

CMDPROC(CmdRescue) { /*(THING *thing, BYTE *cmd)*/ 
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *target;

  cmd = StrOneWord(cmd, NULL);
  /* Stop following */
  if (!*cmd) {
    SendThing("^wYes, but who do you want to rescue...\n", thing);
    return;
  }

  /* rescue the target */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  target = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_PLR|TF_MOB, &srcOffset);
  if (!target) {
    /* found somebody to rescue */
    SendThing("^wWho did you want to rescue again?...\n", thing);
    return;
  }
  CbtRescue(thing, target);
}

CMDPROC(CmdHide) { /*(THING *thing, BYTE *cmd)*/ 
  LWORD  cost; 
  LWORD  check; 

  if (BIT(Character(thing)->cAffectFlag, AF_HIDDEN)) {
    SendThing("^wBut you're allready hiding!\n", thing);
    return;
  }

  if (thing->tType == TTYPE_PLR && !Plr(thing)->pSkill[SKILL_HIDE]) {
    SendThing("^wI'm afraid you're not much good at this sort of thing\n", thing);
    return;
  }

  thing->tWait++; /* make it so theycant initiate a command for a "round" */
  cost = HIDE_MOVE_COST;
  cost = CharMoveCostAdjust(thing, cost);
  if (Character(thing)->cMoveP < cost) {
    /* failed to hide */
    SendAction("^YYou try to hide, ^ybut are too exhausted!\n", 
      thing, NULL, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^Y$n tries to hide, ^ybut is too exhausted!\n",   
      thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    return;
  }
  Character(thing)->cMoveP -= cost;

  check = DiceOpenEnded(1, 100, 100);
  if (thing->tType == TTYPE_PLR) {
    check += Plr(thing)->pSkill[SKILL_HIDE];
  } else if (thing->tType == TTYPE_MOB) {
    check += PropertyGetLWord(thing, "%Hide", Mob(thing)->mTemplate->mLevel*2);
  }

  if (check >= 75) {
    SendThing("^YYou disappear into the shadows\n", thing);
    /* Should probably tell everyone who makes a PERCEPTION check
      what just happened */
    BITSET(Character(thing)->cAffectFlag, AF_HIDDEN);
  } else {
    SendAction("^YYou try to hide, ^ybut FAIL!\n", 
      thing, NULL, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
    SendAction("^Y$n tries to hide, ^ybut FAILS!\n",   
      thing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
  }
}


/* Peek into someone elses inventory... naughty, naughty... */
CMDPROC(CmdPeek) {
  BYTE      srcKey[256];
  LWORD     srcNum;
  LWORD     srcOffset;
  THING    *found;
  LWORD     cost;
  LWORD     guard;
  LWORD     perception;
  LWORD     peek;
  LWORD     check;
  THING    *i;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry Peek <Person>\n", thing);
    return;
  }
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);
  
  found = CharThingFind(thing, srcKey, -1, Base(thing)->bInside, TF_MOB|TF_PLR, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that around\n", thing);
    return;
  }
  
  thing->tWait+=3; /* make it so they cant initiate a command for a "round" */
  cost = PEEK_MOVE_COST;
  cost = CharMoveCostAdjust(thing, cost);
  if (Character(thing)->cMoveP < cost) {
    /* failed to peek */
    SendAction("^YYou try to peek into $N's inventory, ^ybut are too exhausted!\n", 
      thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
    return;
  }
  Character(thing)->cMoveP -= cost;

  /* get skills involved */
  peek       = CharGetPeek(thing);
  if (peek <= 0) {
    SendThing("^bYou've never bothered to learn how to do that\n", thing);
    return;
  }
  
  perception = CharGetPerception(found);
  guard      = CharGetGuard(found);
  
  /* Guard against it */
  check = DiceOpenEnded(1, 100, 100);
  if (perception + 50 > peek + check) {
    if (guard > peek) {
      SendAction("^YYou try to peek into $N's inventory, but they guard against it\n", 
        thing, found, SEND_SRC |SEND_VISIBLE);
      SendAction("^YYou notice $n trying to peek into your inventory and guard against it\n", 
        thing, found, SEND_DST |SEND_VISIBLE);
      return;
    } else {
      SendAction("^YYou notice $n trying to peek into your inventory\n", 
        thing, found, SEND_DST |SEND_VISIBLE);
      peek -= guard;
    }
    /* If the mob we peeked is unfriendly to thieves attack them here */
    if (found->tType == TTYPE_MOB && perception+50 > peek + check) {
      if (Number(1,100) < 10)
        FightStart(found, thing, 1, NULL);
    }
  }
  SendAction("^YYou try to peek into $N's inventory, and see:\n", 
    thing, found, SEND_SRC |SEND_VISIBLE);
  
  for (i=found->tContain; i;) {
    check = DiceOpenEnded(1, 100, 100);
    check += peek; /* Peek has been adjusted for guard allready */
    if (check >= 75 && i->tType==TTYPE_OBJ && !Obj(i)->oEquip) {
      /* Show them the item */
      i = ThingShow(i, thing);
    } else
      i = i->tNext;
  }
  
}


/* Steal an item from someone's inventory */
CMDPROC(CmdSteal) {     /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   srcKey[256];
  BYTE   dstKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  LWORD  dstOffset;
  THING *found;
  THING *search;
  THING *dest;
  LWORD  guard;
  LWORD  perception;
  LWORD  steal;
  LWORD  check;
  LWORD  cost;
  THING *i;
  LWORD  noticed = 0;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, dstKey, &dstOffset);

  /* search the room for a target */
  if (*dstKey) { 
    dest = CharThingFind(thing, dstKey, -1, Base(thing)->bInside, TF_PLR|TF_MOB, &dstOffset);
  } else
    dest = NULL;
  if (!dest) {
    SendThing("^bThere doesnt appear to be anyone like that to steal from\n", thing);
    return;
  }

  /* search them for an object */
  search = dest;
  found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV, &srcOffset);
  if (!found) {
    SendThing("^bThere doesnt appear to be anything like that on them to steal\n", thing);
    return;
  }

  /* get skills involved */
  steal      = CharGetSteal(thing);
  if (steal <= 0) {
    SendThing("^bYou've never bothered to learn how to do that\n", thing);
    return;
  }
  
  perception = CharGetPerception(dest);
  guard      = CharGetGuard(dest);
  
  while (found && srcNum!=0) {
    thing->tWait+=3; /* make it so they cant initiate a command for a "round" */
    cost = STEAL_MOVE_COST;
    cost = CharMoveCostAdjust(thing, cost);
    if (Character(thing)->cMoveP < cost) {
      SendAction("^YYou try to steal from $N's inventory, ^ybut are too exhausted!\n", 
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      return;
    }
    Character(thing)->cMoveP -= cost;

    if (Base(found)->bWeight + Base(thing)->bConWeight > CharGetCarryMax(thing)) {
      SendAction("^CYou try to steal ^g$N", thing, found, SEND_SRC);
      SendAction("^Cfrom ^g$N^C, but its too heavy!\n", thing, dest, SEND_SRC);
    } else {
      check = DiceOpenEnded(1, 100, 100);
      if (perception + 50 >= steal + check) {
        /* They noticed */
        steal -= guard;
        noticed += 1;
      }
      if (steal + check >= 75) {
        /* Steal succeeds */
        for (i= Base(thing)->bInside->tContain; i; i=i->tNext) {
          perception = CharGetPerception(dest);
          if (i != thing && perception+50 > steal + check) {
            /* Someone noticed acts */ 
            SendAction("^b$N ",         i, thing, SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST);
            SendAction("^bsteals ^g$N ", i, found, SEND_SRC|SEND_VISIBLE);
            SendAction("^bfrom ^g$N\n", i, dest,  SEND_SRC|SEND_VISIBLE);
            noticed += 1;
          }
        }
  
        SendAction("^bYou steal ^g$N ", thing, found, SEND_SRC|SEND_CAPFIRST);
        SendAction("^bfrom ^g$N\n", thing, dest, SEND_SRC);
        ThingTo(found, thing);

        /* Infamy */
        if (DiceOpenEnded(1, 100, 100) < noticed) {
          if (thing->tType==TTYPE_PLR) {
            BYTE buf[256];

            sprintf(buf, "^yYou've been ^wcaught ^ystealing, you gain 1 point of ^wINFAMY^w!!\n");
            SendThing(buf, thing);
            PlayerGainInfamy(thing, 1, buf);
          }
        }

      } else {
        /* Steal fails */
        for (i= Base(thing)->bInside->tContain; i; i=i->tNext) {
          perception = CharGetPerception(i);
          if (i != thing && perception+50 > steal + check) {
            /* Someone noticed acts */ 
            SendAction("^b$N ", i, thing, SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST);
            SendAction("^btries to steal ^g$N ", i, found, SEND_SRC|SEND_VISIBLE);
            SendAction("^bfrom ^g$N^w and FAILS!\n", i, dest,  SEND_SRC|SEND_VISIBLE);
            noticed += 1;
          }
        }
  
        SendAction("^bYou try to steal ^g$N ", thing, found, SEND_SRC|SEND_CAPFIRST);
        SendAction("^bfrom ^g$N ^wand FAIL!\n", thing, dest, SEND_SRC);

        /* Infamy */
        if (DiceOpenEnded(1, 100, 100) < noticed/2) {
          if (thing->tType==TTYPE_PLR) {
            BYTE buf[256];

            sprintf(buf, "^yYou've been ^wcaught ^ystealing, you gain 1 point of ^wINFAMY^w!!\n");
            SendThing(buf, thing);
            PlayerGainInfamy(thing, 1, buf);
          }
        }

        /* If the mob we steal from is unfriendly to thieves attack them here */
        if (dest->tType == TTYPE_MOB && perception+50 > steal + check) {
          FightStart(dest, thing, 1, NULL);
        }
      }
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

/* Steal an item from someone's inventory */
CMDPROC(CmdPickpocket) {     /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE   buf[256];
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  THING *search;
  LWORD  guard;
  LWORD  perception;
  LWORD  pickpocket;
  LWORD  check;
  LWORD  cost;
  LWORD  amount;
  THING *i;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  /* search the room for a target */
  search = Base(thing)->bInside;
  if (*srcKey) { 
    found = CharThingFind(thing, srcKey, -1, search, TF_PLR|TF_MOB, &srcOffset);
  } else
    found = NULL;
  if (!found) {
    SendThing("^bThere doesnt appear to be anyone like that to pickpocket\n", thing);
    return;
  }

  /* get skills involved */
  pickpocket = CharGetPickpocket(thing);
  if (pickpocket <= 0) {
    SendThing("^bYou've never bothered to learn how to do that\n", thing);
    return;
  }
  
  perception = CharGetPerception(found);
  guard      = CharGetGuard(found);
  
  while (found && srcNum!=0) {
    thing->tWait+=3; /* make it so they cant initiate a command for a "round" */
    cost = PICKPOCKET_MOVE_COST;
    cost = CharMoveCostAdjust(thing, cost);
    if (Character(thing)->cMoveP < cost) {
      SendAction("^YYou try to pickpocket $N, ^ybut are too exhausted!\n", 
        thing, found, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
      return;
    }
    Character(thing)->cMoveP -= cost;

    check = DiceOpenEnded(1, 100, 100);
    if (perception + 50 >= pickpocket + check) {
      /* They noticed */
      pickpocket -= guard;
    }
    if (pickpocket + check >= 75) {
      /* Steal succeeds */
      for (i= Base(thing)->bInside->tContain; i; i=i->tNext) {
        perception = CharGetPerception(found);
        if (i != thing && perception+50 > pickpocket + check) {
          /* Someone noticed acts */ 
          SendAction("^b$N ", i, thing, SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST);
          SendAction("^bpicks ^g$N's pocket\n", i, found, SEND_SRC|SEND_VISIBLE);
        }
      }
  
      SendAction("^bYou pick ^g$N's ^bpocket ", thing, found, SEND_SRC|SEND_CAPFIRST);
      /* Take some money */
      if (Character(found)->cMoney == 0) {
        SendAction("but they're broke!\n", thing, found, SEND_SRC);
      } else {
        amount = Number(1, MAXV(1,pickpocket*3));
        MAXSET(amount, Character(found)->cMoney);
        Character(found)->cMoney -= amount;
        Character(thing)->cMoney += amount;
        sprintf(buf, "and get ^w%ld ^bcredits\n", amount);
        SendAction(buf, thing, found, SEND_SRC);
      }
      
    } else {
      /* Pickpocket fails */
      for (i= Base(thing)->bInside->tContain; i; i=i->tNext) {
        perception = CharGetPerception(i);
        if (i != thing && perception+50 > pickpocket + check) {
          /* Someone noticed acts */ 
          SendAction("^b$N ", i, thing, SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST);
          SendAction("^btries to pickpocket ^g$N^w and FAILS!\n", i, found,  SEND_SRC|SEND_VISIBLE);
        }
      }
  
      SendAction("^bYou try to pickpocket ^g$N ^wand FAIL!\n", thing, found, SEND_SRC);

      /* If the mob we pickpocket is unfriendly to thieves attack them here */
      if (found->tType == TTYPE_MOB && perception+50 > pickpocket + check) {
        FightStart(found, thing, 1, NULL);
      }
    }

    /* see if there is more (will not find successive matches unless offset is TF_ALLMATCH) */
    found = CharThingFind(thing, srcKey, -1, search, TF_OBJINV|TF_CONTINUE, &srcOffset);
    if (srcNum>0) srcNum--;
  }
}

