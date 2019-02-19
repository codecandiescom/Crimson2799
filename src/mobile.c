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
#ifndef WIN32
  #include <unistd.h> /* for unlink */
#endif

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "queue.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "code.h"
#include "file.h"
#include "thing.h"
#include "exit.h"
#include "world.h"
#include "send.h"
#include "base.h"
#include "index.h"
#include "object.h"
#include "char.h"
#include "group.h"
#include "fight.h"
#include "player.h"
#include "affect.h"
#include "reset.h"
#include "area.h"
#include "cmd_move.h"
#include "cmd_cbt.h"
#include "mobile.h"

#define MOBILE_INDEX_SIZE 8192

INDEX        mobileIndex;
MOBTEMPLATE *spiritTemplate = NULL;

MTYPE mTypeList[] = {
  { "NONE",             0 },
  { "ROBOT",            MT_ROBOT },
  { "ANIMAL",           MT_ORGANIC|MT_ANIMAL },
  { "PLANT",            MT_ORGANIC|MT_PLANT },
  { "GALCIV-CITIZEN",   MT_ORGANIC|MT_SENTIENT|MT_HASMONEY },
  { "SENTIENT-PLANT",   MT_ORGANIC|MT_PLANT|MT_SENTIENT },
  { "SENTIENT-ROBOT",   MT_ROBOT|MT_SENTIENT },
  { "", 0 }
};

BYTE *mActList[] = {
  "TRACKER",
  "SENTINEL",
  "SCAVENGER",
  "PICKPOCKET",
  "FORGIVING",
  "AGGRESSIVE",
  "STAYAREA",
  "WIMPY",
  "HYPERAGGR",
  "RPUNCTURE",
  "RSLASH",
  "RCONCUSSIVE",
  "RHEAT",
  "REMR",
  "RLASER",
  "RPSYCHIC",
  "RACID",
  "RPOISON",
  "RESCUE",
  ""
};

BYTE     mobileOfLog;    /* log if a virtual # cant be found */
BYTE     mobileReadLog;  /* log each mob structure as its read */
LWORD    mobileNum = 0;  /* number of mob structures in use - just curious really */

void MobileInit(void) {
  BYTE buf[256];

  mobileOfLog   = INILWordRead("crimson2.ini", "mobileOfLog", 0);
  mobileReadLog = INILWordRead("crimson2.ini", "mobileReadLog", 0);
  sprintf(buf, "Reading Mobile logging defaults\n");
  Log(LOG_BOOT, buf);
  sprintf(buf, "MobileTemplate structure size is %d bytes\n", sizeof(MOBTEMPLATE));
  Log(LOG_BOOT, buf);

  IndexInit(&mobileIndex, MOBILE_INDEX_SIZE, "mobileIndex", 0);

  MEMALLOC(spiritTemplate, MOBTEMPLATE, MOBILE_ALLOC_SIZE);
  memset( (void*)spiritTemplate, 0, sizeof(MOBTEMPLATE)); /* init to zeros */
  spiritTemplate->mVirtual = MVIRTUAL_SPIRIT;
  spiritTemplate->mPos = POS_STANDING;
  spiritTemplate->mKey = STRCREATE("dissembodied spirit");
  spiritTemplate->mSDesc = STRCREATE("a spirit");
  spiritTemplate->mLDesc = STRCREATE("a dissembodied spirit floats here");
  spiritTemplate->mDesc = STRCREATE("its a ghost!\n");
  BITSET(spiritTemplate->mAffect, AF_INVIS|AF_IMPROVED|AF_VACUUMWALK|AF_BREATHWATER|AF_DARKVISION|AF_PHASEWALK|AF_WATERWALK|AF_PHASEDOOR);
  BITSET(spiritTemplate->mAct, MACT_SENTINEL);
  spiritTemplate->mProperty = PropertyCreate(spiritTemplate->mProperty, STRCREATE("%CarryMax"), STRCREATE("0"));
}


void MobileRead(WORD area) {
  FILE        *mobileFile;
  BYTE         mobFileBuf[256];
  BYTE         buf[256];
  BYTE         tmp[256];
  STR         *sKey;
  STR         *sDesc;
  MOBTEMPLATE *mobile;
  LWORD last = -1;

  sprintf(mobFileBuf, "area/%s.mob", areaList[area].aFileName->sText);
  mobileFile = fopen(mobFileBuf, "rb");
  if (!mobileFile) {
    sprintf(buf, "Unable to read %s, killing server\n", mobFileBuf);
    Log(LOG_BOOT, buf);
    PERROR("MobileRead");
    exit(ERROR_BADFILE);
  }
  if (areaList[area].aOffset) {
    sprintf(buf, "Relocating Area/Mobs [%s]\n", areaList[area].aFileName->sText);
    Log(LOG_BOOT, buf);
  }

  /* okay we opened it up so read it.... */
  fscanf(mobileFile, " %s ", tmp); /* get virtual number */
  while (!feof(mobileFile)) {
    if (tmp[0] == '$' || feof(mobileFile))
      break; /* Dikumud file format EOF character */
    if (tmp[0] != '#') { /* whoa... whadda we got here */
      sprintf(buf, "MobileRead: Unknown virtual %s, aborting\n", tmp);
      Log(LOG_BOOT, buf);
      break;
    }
    MEMALLOC(mobile, MOBTEMPLATE, MOBILE_ALLOC_SIZE);
    memset( (void*)mobile, 0, sizeof(MOBTEMPLATE)); /* init to zeros */
    mobile->mVirtual = atoi(tmp+1);
    mobile->mVirtual += areaList[area].aOffset;
    if (mobileReadLog) {
      sprintf(buf, "Reading Mobile#%ld\n", mobile->mVirtual);
      Log(LOG_BOOT, buf);
    }
    /* confirm that virtual number is valid */
    if (mobile->mVirtual < last) {
      sprintf(buf, "%s - mob%s < than previous\n", mobFileBuf, tmp);
      Log(LOG_BOOT, buf);
    }
    last = MAXV(last, mobile->mVirtual);
    if (mobile->mVirtual < areaList[area].aVirtualMin) {
      sprintf(buf, "%s - mob%s < than %ld\n", mobFileBuf, tmp, areaList[area].aVirtualMin);
      Log(LOG_BOOT, buf);
      break;
    }
    if (mobile->mVirtual > areaList[area].aVirtualMax) {
      sprintf(buf, "%s - mob%s > than %ld\n", mobFileBuf, tmp, areaList[area].aVirtualMax);
      Log(LOG_BOOT, buf);
      break;
    }

    /* NOTE that Private Strings are not designated as such, until just prior to editing */
    mobileNum++;
    mobile->mKey   = FileStrRead(mobileFile);
    if (fileError) {
      sprintf(buf, "Error reading Key for Mobile#%ld\n", mobile->mVirtual);
      Log(LOG_BOOT, buf);
    }
    mobile->mSDesc = FileStrRead(mobileFile);
    if (fileError) {
      sprintf(buf, "Error reading Desc for Mobile#%ld\n", mobile->mVirtual);
      Log(LOG_BOOT, buf);
    }
    /* in crimson2 no lDesc should have \n on the end if there
     * is one lose it (in vanilla diku mob ldescs have a trailing \n)
     */
    mobile->mLDesc = FileStrRead(mobileFile);
    if (fileError) {
      sprintf(buf, "Error reading LDesc for Mobile#%ld\n", mobile->mVirtual);
      Log(LOG_BOOT, buf);
    }
    if (mobile->mLDesc->sText[mobile->mLDesc->sLen-1] == '\n') {
      strcpy(buf, mobile->mLDesc->sText);
      buf[mobile->mLDesc->sLen-1] = '\0';
      STRFREE(mobile->mLDesc);
      mobile->mLDesc = STRCREATE(buf);
    }
    mobile->mDesc  = FileStrRead(mobileFile);
    if (fileError) {
      sprintf(buf, "Error reading Desc for Mobile#%ld\n", mobile->mVirtual);
      Log(LOG_BOOT, buf);
    }
    mobile->mAct   = FileFlagRead(mobileFile, mActList);    
    mobile->mAffect= FileFlagRead(mobileFile, affectList); 
    fscanf(mobileFile, " %hd ", &mobile->mAura);
    fscanf(mobileFile, "%*s\n"); /* throwaway old S/M param */
    fscanf(mobileFile, " %hd ", &mobile->mLevel);
    fscanf(mobileFile, " %hd ", &mobile->mHitBonus);
    fscanf(mobileFile, " %hd ", &mobile->mArmor);
    fscanf(mobileFile, " %hd ", &mobile->mHPDiceNum);
    fscanf(mobileFile, "d");
    fscanf(mobileFile, "D");
    fscanf(mobileFile, " %hd ", &mobile->mHPDiceSize);
    fscanf(mobileFile, "+%hd ", &mobile->mHPBonus);
    fscanf(mobileFile, " %hd ", &mobile->mDamDiceNum);
    fscanf(mobileFile, "d");
    fscanf(mobileFile, "D");
    fscanf(mobileFile, " %hd ", &mobile->mDamDiceSize);
    fscanf(mobileFile, "+%hd ", &mobile->mDamBonus);
    fscanf(mobileFile, " %ld ", &mobile->mMoney);
    fscanf(mobileFile, " %ld ", &mobile->mExp);
    mobile->mPos  = FILETYPEREAD(mobileFile, posList);
    if (fileError) {
      sprintf(buf, "Error reading Pos for Mobile#%ld\n", mobile->mVirtual);
      Log(LOG_BOOT, buf);
    }
    TYPECHECK(mobile->mPos, posList);
    mobile->mType = FILETYPEREAD(mobileFile, mTypeList);
    if (fileError) {
      sprintf(buf, "Error reading Type for Mobile#%ld\n", mobile->mVirtual);
      Log(LOG_BOOT, buf);
    }
    TYPECHECK(mobile->mType, mTypeList);
    mobile->mSex = FILETYPEREAD(mobileFile, sexList);
    if (fileError) {
      sprintf(buf, "Error reading Sex for Mobile#%ld\n", mobile->mVirtual);
      Log(LOG_BOOT, buf);
    }
    TYPECHECK(mobile->mSex, sexList);

    /* read attachments ie exits, keywords etc */
    while (!feof(mobileFile)) {
      fscanf(mobileFile, " %s ", tmp);
      if (tmp[0] == '#') /* well thats it for this chunk of the world */
        break;

      if (tmp[0] == '$') /* well thats it for this chunk of the world */
        break;

      else if (tmp[0] == 'E') { /* haha an extra */
        sKey  = FileStrRead(mobileFile);
        if (fileError) {
          sprintf(buf, "Error reading ExtraKey for Mobile#%ld\n", mobile->mVirtual);
          Log(LOG_BOOT, buf);
        }
        sDesc = FileStrRead(mobileFile);
        if (fileError) {
          sprintf(buf, "Error reading ExtraDesc for Mobile#%ld\n", mobile->mVirtual);
          Log(LOG_BOOT, buf);
        }
        mobile->mExtra = ExtraAlloc(mobile->mExtra, sKey, sDesc);
      }

      else if (tmp[0] == 'P') { /* a property of some kind */
        sKey = FileStrRead(mobileFile); /* property name */
        if (fileError) {
          sprintf(buf, "Error reading PropertyKey for Mobile#%ld\n", mobile->mVirtual);
          Log(LOG_BOOT, buf);
        }
        sDesc = FileStrRead(mobileFile);/* property value */
        if (fileError) {
          sprintf(buf, "Error reading PropertyDesc for Mobile#%ld\n", mobile->mVirtual);
          Log(LOG_BOOT, buf);
        }
        mobile->mProperty = PropertyCreate(mobile->mProperty, sKey, sDesc);
      }

      else { /* what the hell is this anyways? */
        sprintf(buf, "MobileRead: Unknown character in Mobile Entry #%ld\n", mobile->mVirtual);
        Log(LOG_ERROR, buf);
        do {
          fscanf(mobileFile, " %s ", tmp);
        } while (tmp[0]!='#' && tmp[0]!='$' && !feof(mobileFile));
      }

    }

    /* guess this ones a keeper, update mins and maxes etc... */
    IndexInsert(&areaList[area].aMobIndex, mobile, MobileCompareProc);
    if (indexError) {
      sprintf(buf, "%s - mob%s duplicated\n", mobFileBuf, tmp);
      Log(LOG_BOOT, buf);
    }
  }
  /* all done close up shop */
  fclose(mobileFile);
}

MOBTEMPLATE *MobileOf(LWORD virtual) {
  THING *search;
  BYTE   buf[256];
  LWORD  area;

  area = AreaOf(virtual);
  if (area == -1) return NULL;

  search = IndexFind(&areaList[area].aMobIndex, (void*)virtual, MobileFindProc);
  if (search) {
    return MobTemplate(search);
  } else {
    if (mobileOfLog) {
      sprintf(buf, "MobileOf: virtual %ld doesnt exist\n", virtual);
      Log(LOG_ERROR, buf);
    }
  }
  return NULL;
}


void MobileWrite(WORD area) {
  FILE        *mobileFile;
  BYTE         mobFileBuf[256];
  BYTE         buf[256];
  MOBTEMPLATE *mobile;
  LWORD        i;
  EXTRA       *extra;
  PROPERTY    *property;
  
  /* take a backup case we crash halfways through this */
  sprintf(buf,
    "mv area/mob/%s.mob area/mob/%s.mob.bak",
    areaList[area].aFileName->sText,
    areaList[area].aFileName->sText);
  system(buf);
  
  sprintf(mobFileBuf, "area/%s.mob", areaList[area].aFileName->sText);
  mobileFile = fopen(mobFileBuf, "wb");
  if (!mobileFile) {
    sprintf(buf, "Unable to write %s, killing server\n", mobFileBuf);
    Log(LOG_ERROR, buf);
    PERROR("MobileWrite");
    return;
  }
  
  /* okay we opened it up so write it.... */
  for (i=0; i<areaList[area].aMobIndex.iNum; i++) {
    mobile = MobTemplate(areaList[area].aMobIndex.iThing[i]);
    fprintf(mobileFile, "#%ld\n", mobile->mVirtual); /* get virtual number */
    FileStrWrite(mobileFile, mobile->mKey);
    FileStrWrite(mobileFile, mobile->mSDesc);
    FileStrWrite(mobileFile, mobile->mLDesc);
    FileStrWrite(mobileFile, mobile->mDesc);
    FileFlagWrite(mobileFile, mobile->mAct, mActList, ' ');
    FileFlagWrite(mobileFile, mobile->mAffect, affectList, ' ');
    fprintf(mobileFile, "%hd S\n", mobile->mAura);
    fprintf(mobileFile, "%hd %hd %hd", mobile->mLevel, mobile->mHitBonus, mobile->mArmor);
    fprintf(mobileFile, " %hdd%hd+%hd",   mobile->mHPDiceNum,  mobile->mHPDiceSize,  mobile->mHPBonus);
    fprintf(mobileFile, " %hdd%hd+%hd\n", mobile->mDamDiceNum, mobile->mDamDiceSize, mobile->mDamBonus);
    fprintf(mobileFile, "%ld %ld\n", mobile->mMoney, mobile->mExp);
    FILETYPEWRITE(mobileFile, mobile->mPos,  posList,   ' ');
    FILETYPEWRITE(mobileFile, mobile->mType, mTypeList, ' ');
    FILETYPEWRITE(mobileFile, mobile->mSex,  sexList,   '\n');
    
    /* extra descriptions */
    for (extra=mobile->mExtra; extra; extra=extra->eNext) {
      fprintf(mobileFile, "E\n");
      FileStrWrite(mobileFile, extra->eKey);
      FileStrWrite(mobileFile, extra->eDesc);
    }
    
    /* property's */
    for (property=mobile->mProperty; property; property=property->pNext) {
      fprintf(mobileFile, "P\n");
      FileStrWrite(mobileFile, property->pKey);
      if ( CodeIsCompiled(property) ) {
        if (!areaWriteBinary) {
          /* decompile the property */
          CodeDecompProperty(property, NULL);
          mobile->mCompile=1; /* compile on demand enabled */
          /* Warn if it didnt decompile */
          if (CodeIsCompiled(property)) {
            sprintf(buf, "MobileWrite: Property %s failed to decompile for mobile#%ld!\n",
              property->pKey->sText, 
              mobile->mVirtual);
            Log(LOG_AREA, buf);
          } else {
            FileStrWrite(mobileFile, property->pDesc);
          }
        } else {
          FileBinaryWrite(mobileFile, property->pDesc);
        }
      } else {
        FileStrWrite(mobileFile, property->pDesc);
      }
    }
    
  }
  /* all done close up shop */
  fprintf(mobileFile, "$");
  fclose(mobileFile);

  /* turf backup we didnt crash */
  sprintf(buf,
    "area/%s.mob.bak",
    areaList[area].aFileName->sText);
  unlink(buf);

}


INDEXPROC(MobileCompareProc) { /* BYTE IndexProc(void *index1, void *index2) */
  if ( MobTemplate(index1)->mVirtual == MobTemplate(index2)->mVirtual )
    return 0;
  else if ( MobTemplate(index1)->mVirtual < MobTemplate(index2)->mVirtual )
    return -1;
  else
    return 1;
}

INDEXFINDPROC(MobileFindProc) { /* BYTE IFindProc(void *key, void *index) */
  if ( (LWORD)key == MobTemplate(index)->mVirtual )
    return 0;
  else if ( (LWORD)key < MobTemplate(index)->mVirtual )
    return -1;
  else
    return 1;
}


THING *MobileAlloc(void) {
  MOB *mobile;

  MEMALLOC(mobile, MOB, MOBILE_ALLOC_SIZE);
  memset(mobile, 0, sizeof(MOB));
  IndexAppend(&mobileIndex, Thing(mobile));

  Thing(mobile)->tType = TTYPE_MOB;
  return Thing(mobile);
}

THING *MobileCreate(MOBTEMPLATE *template, THING *within) {
  THING    *mobile;
  THING    *leader = NULL;
  THING    *i;
  PROPERTY *p;

  if (!template) return NULL;
  mobile = MobileAlloc();

  Thing(mobile)->tSDesc          = StrAlloc(template->mSDesc);
  Thing(mobile)->tDesc           = StrAlloc(template->mDesc);
  Thing(mobile)->tExtra          = ExtraCopy(template->mExtra);
  if (template->mCompile) {
    template->mCompile = 0;
    for (p=template->mProperty; p; p=p->pNext) {
      CodeCompileProperty(p, NULL);
    }
  }
  Thing(mobile)->tProperty       = PropertyCopy(template->mProperty);
  Base(mobile)->bKey             = StrAlloc(template->mKey);
  Base(mobile)->bLDesc           = StrAlloc(template->mLDesc);
  Base(mobile)->bWeight          = template->mWeight;
  Character(mobile)->cAura       = template->mAura;
  Character(mobile)->cLevel      = template->mLevel;
  Character(mobile)->cHitBonus   = template->mHitBonus;
  Character(mobile)->cArmor      = template->mArmor;
  Character(mobile)->cHitP       = Dice(template->mHPDiceNum,
                                        template->mHPDiceSize)
                                  +template->mHPBonus;
  Character(mobile)->cHitPMax    = Character(mobile)->cHitP;

  Character(mobile)->cExp        = template->mExp;
  Character(mobile)->cPos        = template->mPos;
  Character(mobile)->cSex        = template->mSex;
  Mob(mobile)->mTemplate         = template;

  /* correct for affect flags */
  Character(mobile)->cAffectFlag = template->mAffect;
  BITCLR(Character(mobile)->cAffectFlag, AF_GROUP);
  if (BIT(template->mAffect, AF_IMPROVED))
    BITSET(Character(mobile)->cAffectFlag, AF_INVIS);

  /* if they carry money then give them cash */
  if (BIT(mTypeList[template->mType].mTFlag, MT_HASMONEY))
    Character(mobile)->cMoney      = template->mMoney;

  template->mOnline++;
  CodeCheckFlag(mobile);

  if (within) {
    /* put mobile in room */
    ThingTo(mobile, within);

    /* try to group with somebody */
    if (BIT(template->mAffect, AF_GROUP)) {
      for (i=within->tContain; i; i=i->tNext) {
        /* not a mob cant be a leader */
        if (i->tType != TTYPE_MOB) continue;
        /* not a mob that groups */
        if (!BIT(Mob(i)->mTemplate->mAffect, AF_GROUP)) continue;
        if (Character(i)->cLead) {
          /* not the leader of its group */
          if (Character(i)->cLead != i) continue;
          /* if following a player */
          if (Character(i)->cLead->tType == TTYPE_PLR) continue;
        }
        leader = i; /* a potential leader */
      }
      /* if we found a leader group with it */
      if (leader && leader != mobile) {
        CharAddFollow(mobile, leader);
        BITSET(Character(mobile)->cAffectFlag, AF_GROUP);
        /*BITSET(Character(leader)->cAffectFlag, AF_GROUP);*/
      }
    }
  }

  /* once struct is fully inited give 'em some pts */
  Character(mobile)->cMoveP      = CharGetMovePMax(mobile);
  Character(mobile)->cPowerP     = CharGetPowerPMax(mobile);

  return mobile;
}

void MobileFree(THING *thing) {
  Mob(thing)->mTemplate->mOnline--;
  IndexDelete(&mobileIndex, thing, NULL);
  MEMFREE(Mob(thing), MOB);
}

LWORD MobilePresent(MOBTEMPLATE *template, THING *within) {
  LWORD  present = 0;
  THING *i;

  for (i = within->tContain; i; i=i->tNext) {
    if ( (i->tType == TTYPE_MOB) && (Mob(i)->mTemplate == template) )
      present++;
  }
  return present;
}

void MobileTick() {
  LWORD  i=0;
  LWORD  gain;
  THING *thing=NULL;

  if (mobileIndex.iNum==0) return;
  while(1) {
    /* Corbin's footnote: you did this really lame Cryogen - 
     * why not just use a for() loop like everyone else? CONFUSING! */
    if (thing == mobileIndex.iThing[i]) i++;
    if (i>=mobileIndex.iNum) break;
    thing = mobileIndex.iThing[i];
    
    /* do a tick */
    if (CharTick(thing)) continue;
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }

    /* gain hit points back */
    gain = PropertyGetLWord(thing, "%GainHitP", 10+Character(thing)->cLevel/3);
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }
    gain = CharGainAdjust(thing, GAIN_HITP, gain);
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }
    Character(thing)->cHitP   += gain;
    MAXSET(Character(thing)->cHitP,   CharGetHitPMax(thing));
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }

    /* gain move points back */
    gain = PropertyGetLWord(thing, "%GainMoveP", 10+Character(thing)->cLevel/3);
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }
    gain = CharGainAdjust(thing, GAIN_MOVEP, gain);
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }
    Character(thing)->cMoveP  += gain;
    MAXSET(Character(thing)->cMoveP,  CharGetMovePMax(thing));
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }

    /* gain mana points back */
    gain = PropertyGetLWord(thing, "%GainPowerP", 10+Character(thing)->cLevel/3);
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }
    gain = CharGainAdjust(thing, GAIN_POWERP, gain);
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }
    Character(thing)->cPowerP += gain;
    MAXSET(Character(thing)->cPowerP, CharGetPowerPMax(thing));
    /* crash test - debug purposes only -- I was trying to track a bug */
      if (thing!=mobileIndex.iThing[i]) {int *c; c=NULL; *c=1; }
 
    /* gain money back */
    if (Character(thing)->cMoney < Mob(thing)->mTemplate->mMoney) {
      gain = PropertyGetLWord(thing, "%GainMoney", 0);
      Character(thing)->cMoney += gain;
      MAXSET(Character(thing)->cMoney, Mob(thing)->mTemplate->mMoney);
    } 
  } 
}

LWORD MobileCheckExit(THING *mobile, EXIT *exit) {
  /* basic stuff */
  if (!mobile) return FALSE;
  if (!exit) return FALSE;

  /* must be in a world */
  if (!Base(mobile)->bInside->tType == TTYPE_WLD) return FALSE;
  /* area cant have MOBFREEZE set */
  if (BIT(areaList[Wld(Base(mobile)->bInside)->wArea].aResetFlag, RF_MOBFREEZE)) return FALSE;
  /* exit must lead somewhere */
  if (!exit->eWorld) return FALSE;
  /* cant be a room with no exits */
  if (!Wld(exit->eWorld)->wExit) return FALSE;
  /* check if we cant leave our area */
  if (BIT(Mob(mobile)->mTemplate->mAct, MACT_STAY_AREA) 
  && Wld(Base(mobile)->bInside)->wArea != Wld(exit->eWorld)->wArea)
    return FALSE;
  /* avoid deathtraps */
  if (BITANY(Wld(exit->eWorld)->wFlag, WF_DEATHTRAP|WF_NOMOB)) return FALSE;

  /* MoveExitParse will block the move if they are fighting,
     dont do it here */
  
  /* Check for SENTINEL is elsewhere, that way SENTINELS, will
     still track down enemies, flee etc */

  return TRUE;
}


/* 
 * NOTE: This is probably the most frequently called function in the mud
 *    keep that in mind when you are twiddling with it.
 */
void MobileIdlePrimitive(THING *mobile) {  
  /* Should probably put an explicit check for cFightExit here and move
     towards it */

  /* Tracker mobs will hunt down their assailants */
  if (  (BIT(Mob(mobile)->mTemplate->mAct, MACT_TRACKER)) 
      &&(Mob(mobile)->mTrack)
      &&(Character(mobile)->cMoveP >= CharGetMovePMax(mobile)>>2)
      &&(  (!BIT(Mob(mobile)->mTemplate->mAct, MACT_WIMPY))/* will flee if chasee enters room */
         ||(Character(mobile)->cHitP >= CharGetHitPMax(mobile)/5) )
      ) {  /* Will chase people who flee */
    EXIT  *exit;
    
    if (Base(mobile)->bInside != Base(Mob(mobile)->mTrack)->bInside) {
      exit = ThingTrack(Base(mobile)->bInside, Mob(mobile)->mTrack, PropertyGetLWord(mobile, "%Track", Mob(mobile)->mTemplate->mLevel+10)*3);
      if (MobileCheckExit(mobile, exit)) {
        MoveExitParse(mobile, exit);
        /* should pick a fight here - but give the player time to react */
        return;
      } else {
        if (!Number(0,30)) Mob(mobile)->mTrack = NULL; /* give up */
      }
    }
  }

  /* Scavenger mobs will take stuff lying on the ground */  
  if(BIT(Mob(mobile)->mTemplate->mAct, MACT_SCAVENGER)) {/* will pick stuff up */
    THING *i, *next;

    for(i=Base(mobile)->bInside->tContain; i; i=next) {
      next=i->tNext;
      if (i->tType==TTYPE_OBJ 
       && !Number(0,10) 
       && Obj(i)->oTemplate->oValue>5
       && CharCanCarry(mobile, i)) {
        /* guess we can take it */
        SendAction("^bYou get ^g$N\n", 
                   mobile, i, SEND_SRC |SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("^b$n gets ^g$N\n",   
                   mobile, i, SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
        ThingTo(i, mobile);
        return;
      }
    }
  }

  /* Wimpy mobs will keep running away if their hit points are low */  
  if(BIT(Mob(mobile)->mTemplate->mAct, MACT_WIMPY)
  && (Character(mobile)->cHitP < CharGetHitPMax(mobile)/5)) {/* will flee if chasee enters room */
    THING *i;
    EXIT  *exit;

    if (Base(mobile)->bInside->tType == TTYPE_WLD) {
      for(i=Base(mobile)->bInside; i && i!=Mob(mobile)->mTrack; i=i->tNext);
      if(i && ThingCanSee(mobile, i)==TCS_SEENORMAL) {  
        /* Effectively a MoveRandom */
        exit = ExitDir(Wld(Base(mobile)->bInside)->wExit, Number(0,6));
        if((exit)
         &&(exit->eWorld)
         &&((!BIT(Mob(mobile)->mTemplate->mAct, MACT_STAY_AREA)
           ||Wld(Base(mobile)->bInside)->wArea == Wld(exit->eWorld)->wArea))) {
          MoveExitParse(mobile, exit);
          return;
        }
      }
    }
  }
  
  /* Will steal items from players */
  if(BIT(Mob(mobile)->mTemplate->mAct, MACT_PICKPOCKET)) {/* steals items */
  }
  
  /* Hiding mobs tend towards lurking more */
  if (BIT(Character(mobile)->cAffectFlag, AF_HIDDEN)) {
    if (!Number(0,2)) return;
  } else if (BIT(Mob(mobile)->mTemplate->mAffect, AF_HIDDEN)) {
    CmdHide(mobile, "");
    /* if (BIT(Character(mobile)->cAffectFlag, AF_HIDDEN)) */ return;
  }

  /* Attack stuff sometimes */
  if (BIT(Mob(mobile)->mTemplate->mAct, MACT_AGGRESSIVE)
    ||BIT(Mob(mobile)->mTemplate->mAct, MACT_HYPERAGGR)) {/* Will initiate fights */
    THING *i;

    /* see if the person we are after is here first off*/
    if (Mob(mobile)->mTrack) {
      for(i=Base(mobile)->bInside->tContain; i && i!=Mob(mobile)->mTrack; i=i->tNext);
      if (i && ThingCanSee(mobile, i)==TCS_SEENORMAL) {
        FightStart(mobile, i, 1, NULL);
        return;
      }
    }
    /* oh well kill someone at random */
    for(i=Base(mobile)->bInside->tContain; i; i=i->tNext) {
      /* Aggr mobs will attack players */
      if( i->tType==TTYPE_PLR 
        && !Number(0,10) 
        && !BIT(Plr(i)->pSystem, PS_NOHASSLE) 
        && ThingCanSee(mobile, i)>TCS_CANTSEE) {  
        /* make sure we dont wack a group member */
        /*if  (!Character(mobile)->cLead
          || Character(mobile)->cLead!=Character(i)->cLead) {*/
        if (!GroupIsGroupedMember(i,mobile)) {
          FightStart(mobile, i, 1, NULL);
          return;
        }
      /* HyperAggr Mobs will also attack other mobs */
      } else if ( i->tType==TTYPE_MOB 
               && BIT(Mob(mobile)->mTemplate->mAct, MACT_HYPERAGGR) 
               && !Number(0,20) 
               && ThingCanSee(mobile, i)>TCS_CANTSEE) {
        /* make sure we dont wack a group member or should we? */
        /*if  (!BIT(Character(mobile)->cAffectFlag, AF_GROUP) 
          || !Character(mobile)->cLead
          || Character(mobile)->cLead!=Character(i)->cLead) {*/
        if (!GroupIsGroupedMember(i,mobile)) {
          FightStart(mobile, i, 1, NULL);
          return;
        }
      }
    }
  }

  /* Move around randomly, 
     Note: hurt mobs should look for nearby regen rooms and go there */  
  if(!BIT(Mob(mobile)->mTemplate->mAct, MACT_SENTINEL) /* sentinels dont move */
    &&(!BIT(areaList[Wld(Base(mobile)->bInside)->wArea].aResetFlag, RF_MOBFREEZE))
    &&(Character(mobile)->cMoveP >= CharGetMovePMax(mobile)>>1)
    &&(!Character(mobile)->cLead || Character(mobile)->cLead==mobile)) {   /* if we are following someone dont leave */
    EXIT *exit;
    LWORD exitNum;
    
    if (Base(mobile)->bInside 
     && Base(mobile)->bInside->tType == TTYPE_WLD 
     && !Number(0,10)) {

      /* count exits */
      for (exitNum=0,exit=Wld(Base(mobile)->bInside)->wExit; exit; exit=exit->eNext) exitNum++;
      /* pick a random direction */
      exitNum = Number(1,exitNum);
      for (exit=Wld(Base(mobile)->bInside)->wExit; exit&&exitNum!=1; exit=exit->eNext,exitNum--);

      if(MobileCheckExit(mobile, exit)) {
        MoveExitParse(mobile, exit);
        return;
      }
    }
  }

/* End of MobileIdlePrimitive() */
}


void MobileIdle(THING *mobile) {
  /* if they are fighting then they are not idling */
  if (Character(mobile)->cFight) return;

  /* let code handler get first crack at it */
  if ((CodeParseIdle(mobile))) return;

  MobileIdlePrimitive(mobile);
}

