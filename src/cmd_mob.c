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

/*****************************************************************
 *                                                               *
 *                                                               *
 *                     M O B    S T U F F                        *
 *                                                               *
 *                                                               *
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crimson2.h"
#include "macro.h"
#include "mem.h"
#include "queue.h"
#include "log.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "code.h"
#include "file.h"
#include "thing.h"
#include "reset.h"
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
#include "affect.h"
#include "mobile.h"
#include "player.h"
#include "parse.h"
#include "cmd_wld.h"
#include "cmd_mob.h"


MOBTEMPLATE *MobGetMobile(THING *thing, BYTE **cmd) {
  MOBTEMPLATE *mobile = NULL;
  LWORD        virtual = -1;

  sscanf(*cmd, " %ld", &virtual);
  if (virtual>=0) {
    *cmd = StrOneWord(*cmd, NULL);
    mobile = MobileOf(virtual);
    if (!mobile) {
      SendThing("Sorry, no MOBILE with that virtual number exists\n", thing);
      return NULL;
    }
  } 

  return mobile;
}


CMDPROC(CmdMStat) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         buf[512];
  BYTE         truncateStr[256];
  EXTRA       *extra;
  PROPERTY    *property;

  cmd = StrOneWord(cmd, NULL);
  mobile = MobGetMobile(thing, &cmd);
  if (!mobile)
    return;

  /* show them the stats on the WORLD */
  /* first line, # Name & Type */
  sprintf(buf, "^g#:^G[^w%ld^G] ^gOnline^G[^c%ld^G] ^gName:^G[^c", mobile->mVirtual, mobile->mOnline);
  SendThing(buf, thing);
  SendThing(mobile->mSDesc->sText, thing);
  SendThing("^G]\n^gKeywords:^G[^c", thing);
  SendThing(mobile->mKey->sText, thing);
  SendThing("^G]\n^gLDesc:\n^c", thing);
  SendThing(mobile->mLDesc->sText, thing);
  SendThing("\n^gDescription:^b\n", thing);
  SendThing(mobile->mDesc->sText, thing);

  /* 2nd line - flags */
  SendThing("^gAct: ^G[^c", thing);
  SendThing(FlagSprintf(buf, mobile->mAct, mActList, ' ', 512), thing);
  SendThing("^G]\n", thing);
  SendThing("^gAffect: ^G[^c", thing);
  SendThing(FlagSprintf(buf, mobile->mAffect, affectList, ' ', 512), thing);
  SendThing("^G]\n", thing);

  sprintf(buf, "^gAura:^G[^c%hd^G] ", mobile->mAura);
  SendThing(buf, thing);
  sprintf(buf, "^gLevel:^G[^c%hd^G] ", mobile->mLevel);
  SendThing(buf, thing);
  sprintf(buf, "^gHitBonus:^G[^c%hd^G] ", mobile->mHitBonus);
  SendThing(buf, thing);
  sprintf(buf, "^gArmor:^G[^c%hd^G] ", mobile->mArmor);
  SendThing(buf, thing);
  sprintf(buf, "^gHP:^G[^c%hd^gD^c%hd+%hd^G] ", mobile->mHPDiceNum, mobile->mHPDiceSize, mobile->mHPBonus);
  SendThing(buf, thing);
  sprintf(buf, "^gDamage:^G[^c%hd^gD^c%hd+%hd^G]\n", mobile->mDamDiceNum, mobile->mDamDiceSize, mobile->mDamBonus);
  SendThing(buf, thing);

  sprintf(buf, "^gMoney:^G[^c%ld^G] ", mobile->mMoney);
  SendThing(buf, thing);
  sprintf(buf, "^gExp:^G[^c%ld^G] ", mobile->mExp);
  SendThing(buf, thing);
  SendThing("^gPos:^G[^c", thing);
  SendThing(TYPESPRINTF(buf, mobile->mPos, posList, 512), thing);
  SendThing("^G] ^gType:^G[^c", thing);
  SendThing(TYPESPRINTF(buf, mobile->mType, mTypeList, 512), thing);
  SendThing("^G] ^gSex:^G[^c", thing);
  SendThing(TYPESPRINTF(buf, mobile->mSex, sexList, 512), thing);
  SendThing("^G]\n", thing);

#ifdef BLAH
  /* Show Resists */
  SendThing("\nResistances:\n", thing);
  sprintf(buf, 
          "^gPuncture  [^c%3hd^G]",
          CharGetResist(thing, FD_PUNCTURE));
  SendThing(buf, thing);
  sprintf(buf, 
          "            ^gSlash [^c%3hd^G]",
          CharGetResist(thing, FD_SLASH));
  SendThing(buf, thing);
  sprintf(buf, 
          "               ^gConcussive[^c%3hd^G]\n",
          CharGetResist(thing, FD_CONCUSSIVE));
  SendThing(buf, thing);
  sprintf(buf, 
          "^gHeat      [^c%3hd^G]",
          CharGetResist(thing, FD_HEAT));
  SendThing(buf, thing);
  sprintf(buf, 
          "            ^gEMR   [^c%3hd^G]",
          CharGetResist(thing, FD_EMR));
  SendThing(buf, thing);
  sprintf(buf, 
          "               ^gLaser     [^c%3hd^G]\n",
          CharGetResist(thing, FD_LASER));
  SendThing(buf, thing);
  sprintf(buf, 
          "^gPsychic   [^c%3hd^G]",
          CharGetResist(thing, FD_PSYCHIC));
  SendThing(buf, thing);
  sprintf(buf, 
          "            ^gAcid  [^c%3hd^G]",
          CharGetResist(thing, FD_ACID));
  SendThing(buf, thing);
  sprintf(buf, 
          "               ^gPoison    [^c%3hd^G]\n",
          CharGetResist(thing, FD_POISON));
  SendThing(buf, thing);
#endif

  for (extra = mobile->mExtra; extra; extra=extra->eNext) {
    sprintf(buf,
            "^gExtra: ^c%-31s = ",
            StrTruncate(truncateStr,extra->eKey->sText,  30));
    SendThing(buf, thing);
    StrTruncate(truncateStr,extra->eDesc->sText, 30);
    StrOneLine(truncateStr);
    sprintf(buf, "%s\n", truncateStr);
    SendThing(buf, thing);
  }
  for (property = mobile->mProperty; property; property=property->pNext) {
    sprintf(buf,
            "^gProp.: ^c%-31s = ",
            StrTruncate(truncateStr,property->pKey->sText,  30));
    SendThing(buf, thing);
    StrTruncate(truncateStr,property->pDesc->sText, 30);
    StrOneLine(truncateStr);
    sprintf(buf, "%s\n", truncateStr);
    SendThing(buf, thing);
  }
}

CMDPROC(CmdMList) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         truncateStr[256];
  BYTE         buf[256];
  LWORD        i;
  THING       *world;
  LWORD        area;

  world = WldGetWorld(thing, NULL);
  area = Wld(world)->wArea;

  /* List the rooms */
  for (i=0; i<areaList[area].aMobIndex.iNum; i++) {
    mobile = MobTemplate(areaList[area].aMobIndex.iThing[i]);
    sprintf(buf, "^w%5ld ^c%-19s", mobile->mVirtual, StrTruncate(truncateStr,mobile->mSDesc->sText, 19));
    SendThing(buf, thing);
    if (i%3 == 2) {
      SendThing("\n", thing);
    } else
      SendThing(" ", thing);
  }
  if (i%3)
    SendThing("\n", thing); /* last line allways gets a return */
  sprintf(buf, "^g%ld ^bMobile Templates listed.\n", i);
  SendThing(buf, thing);
}



CMDPROC(CmdMCreate) {
  WLD         *world;
  MOBTEMPLATE *mobile;
  LWORD        area;
  BYTE         buf[256];
  LWORD        i;
  INDEX       *index;

  cmd = StrOneWord(cmd, NULL);
  world = Wld(WldGetWorld(thing, &cmd));
  if (!world)
    return;
  area = world->wArea;
  if (areaList[area].aMobIndex.iNum == (areaList[area].aVirtualMax - areaList[area].aVirtualMin + 1)) {
    SendThing("^gBad news.... This Area is completely full of MobTemplates allready\n", thing);
    return;
  }

  MEMALLOC(mobile, MOBTEMPLATE, MOBILE_ALLOC_SIZE);
  memset( (void*)mobile, 0, sizeof(MOBTEMPLATE)); /* init to zeros */

  mobile->mKey         = STRCREATE("blank mobile");
  mobile->mSDesc       = STRCREATE("Type ^wMNAME^c to give this Mob a better name.");
  mobile->mLDesc       = STRCREATE("Type ^wMLDESC^c to give this Mob a better LDesc.");
  mobile->mDesc        = STRCREATE("Type ^wMDESC^c to give this Mob a better Description.\n");
  mobile->mLevel       = 1;
  mobile->mHPDiceNum   = 1;
  mobile->mHPDiceSize  = 4;
  mobile->mDamDiceNum  = 1;
  mobile->mDamDiceSize = 2;
  mobile->mPos         = POS_STANDING;
  mobile->mAct         = MACT_STAY_AREA;

  /* update total num of MobTemplates in the game */
  mobileNum++;

  /* give the MobTemplate the first available virtual number */
  mobile->mVirtual = areaList[area].aVirtualMin;
  index = &areaList[area].aMobIndex;
  i=0;
  while( (i<=index->iNum-1) && (mobile->mVirtual>=MobTemplate(index->iThing[i])->mVirtual) ){
    if (mobile->mVirtual==MobTemplate(index->iThing[i])->mVirtual)
      mobile->mVirtual++;
    else 
      i++;
  }
  /* insert it into index */
  IndexInsert(index, Thing(mobile), MobileCompareProc);

  BITSET(areaList[Wld(world)->wArea].aSystem, AS_MOBUNSAVED);
  sprintf(buf, "^bYou have just created MobTemplate #^g[%5ld]\n", mobile->mVirtual);
  SendThing(buf, thing);
}




CMDPROC(CmdMName) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         strName[256];

  cmd = StrOneWord(cmd, NULL);
  mobile = MobGetMobile(thing, &cmd);
  if (!mobile)
    return;

  /* Edit the requisite string */
  SendHint("^;HINT: Names should not end on a blank line\n", thing);
  SendHint("^;HINT: The next thing you should do is use ^<MLDESC ^;to add a long description\n", thing);
  sprintf(strName, "Mobile #%ld - Name", mobile->mVirtual);
  EDITSTR(thing, mobile->mSDesc, 256, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &areaList[AreaOf(mobile->mVirtual)].aSystem, AS_MOBUNSAVED);
}



CMDPROC(CmdMKey) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         strName[256];

  cmd = StrOneWord(cmd, NULL);
  mobile = MobGetMobile(thing, &cmd);
  if (!mobile)
    return;

  /* Edit the requisite string */
  SendHint("^;HINT: Keywords should be only a single line\n", thing);
  sprintf(strName, "Mobile #%ld - Keywords", mobile->mVirtual);
  EDITSTR(thing, mobile->mKey, 256, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &areaList[AreaOf(mobile->mVirtual)].aSystem, AS_MOBUNSAVED);
}



CMDPROC(CmdMLDesc) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         strName[256];

  cmd = StrOneWord(cmd, NULL);
  mobile = MobGetMobile(thing, &cmd);
  if (!mobile)
    return;

  /* Edit the requisite string */
  SendHint("^;HINT: Names should not end on a blank line\n", thing);
  SendHint("^;HINT: The next thing you should do is use ^<MDESC ^;to add a description\n", thing);
  sprintf(strName, "Mobile #%ld - LDesc", mobile->mVirtual);
  EDITSTR(thing, mobile->mLDesc, 256, strName, EP_ENDNOLF);
  EDITFLAG(thing, &areaList[AreaOf(mobile->mVirtual)].aSystem, AS_MOBUNSAVED);
}



CMDPROC(CmdMDesc) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         strName[256];

  cmd = StrOneWord(cmd, NULL);
  mobile = MobGetMobile(thing, &cmd);
  if (!mobile)
    return;

  /* Edit the requisite string */
  SendHint("^;HINT: Descriptions must end with a blank line\n", thing);
  sprintf(strName, "Mobile #%ld - Detailed Description", mobile->mVirtual);
  EDITSTR(thing, mobile->mDesc, 4096, strName, EP_ENDLF);
  EDITFLAG(thing, &areaList[AreaOf(mobile->mVirtual)].aSystem, AS_MOBUNSAVED);
}

CMDPROC(CmdMExtra) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         strName[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd)
    EditExtra(thing, "MEXTRA <#>", "", NULL, NULL);
  mobile = MobGetMobile(thing, &cmd);

  if (!mobile)
    return;

  sprintf(strName, "^cMobile ^w#%ld^c/", mobile->mVirtual);
  if (EditExtra(thing, "MEXTRA <#>", cmd, strName, &mobile->mExtra))
    EDITFLAG(thing, &areaList[AreaOf(mobile->mVirtual)].aSystem, AS_MOBUNSAVED);
}

CMDPROC(CmdMProperty) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         strName[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd)
    EditProperty(thing, "MPROPERTY <#>", "", NULL, NULL);
  mobile = MobGetMobile(thing, &cmd);

  if (!mobile)
    return;

  sprintf(strName, "^cMobile ^w#%ld^c/", mobile->mVirtual);
  if (EditProperty(thing, "MPROPERTY <#>", cmd, strName, &mobile->mProperty))
    EDITFLAG(thing, &areaList[AreaOf(mobile->mVirtual)].aSystem, AS_MOBUNSAVED);
}

CMDPROC(CmdMSet) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE         buf[512];
  LWORD        i;
  MOBTEMPLATE *mobile;
  MOBTEMPLATE  mobTemplate;

  SETLIST mobSetList[] = {
    { "AURA",         SET_NUMERIC(     mobTemplate, mobTemplate.mAura )          },
    { "LEVEL",        SET_NUMERIC(     mobTemplate, mobTemplate.mLevel )         },
    { "HITBONUS",     SET_NUMERIC(     mobTemplate, mobTemplate.mHitBonus )      },
    { "ARMOR",        SET_NUMERIC(     mobTemplate, mobTemplate.mArmor )         },
    { "EXPERIENCE",   SET_NUMERIC(     mobTemplate, mobTemplate.mExp )           },
    { "HPDICENUM",    SET_NUMERIC(     mobTemplate, mobTemplate.mHPDiceNum )     },
    { "HPDICESIZE",   SET_NUMERIC(     mobTemplate, mobTemplate.mHPDiceSize )    },
    { "HPBONUS",      SET_NUMERIC(     mobTemplate, mobTemplate.mHPBonus )       },
    { "DAMDICENUM",   SET_NUMERIC(     mobTemplate, mobTemplate.mDamDiceNum )    },
    { "DAMDICESIZE",  SET_NUMERIC(     mobTemplate, mobTemplate.mDamDiceSize )   },
    { "DAMBONUS",     SET_NUMERIC(     mobTemplate, mobTemplate.mDamBonus )      },
    { "MONEY",        SET_NUMERIC(     mobTemplate, mobTemplate.mMoney )         },
    { "EXP",          SET_NUMERIC(     mobTemplate, mobTemplate.mExp )           },
    { "SEX",          SET_TYPE(        mobTemplate, mobTemplate.mSex, sexList )  },
    { "TYPE",         SET_TYPE(        mobTemplate, mobTemplate.mType,mTypeList )},
    { "POSITION",     SET_TYPE(        mobTemplate, mobTemplate.mPos, posList )  },
    { "ACT",          SET_FLAG(        mobTemplate, mobTemplate.mAct, mActList ) },
    { "AFFECT",       SET_FLAG(        mobTemplate, mobTemplate.mAffect, affectList ) },

    /* Skill/Numeric/Properties */
    { "%SPEED",       SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%PERCEPTION",  SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%GUARD",       SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%AMBUSH",      SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RESCUE",      SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%PURSUE",      SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%TRACK",       SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RPUNCTURE",   SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RSLASH",      SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RCONCUSSIVE", SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RHEAT",       SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%REMR",        SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RLASER",      SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RPSYCHIC",    SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RACID",       SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%RPOISON",     SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%WILLPOWER",   SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%DODGE",       SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%PICKPOCKET",  SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%STEAL",       SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%PEEK",        SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },
    { "%CARRYMAX",    SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty ) },

    { "%WEAPONSDESC", SET_PROPERTYSTR( mobTemplate, mobTemplate.mProperty ) },

    { "" }
  };

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) { /* if they didnt type anything, give 'em a list */
    SendThing("^pUsuage:\n^P=-=-=-=\n", thing);
    SendThing("^cMSET <Mob#> <stat> <value>\n", thing);
    EditSet(thing, cmd, NULL, NULL, mobSetList);
    return;
  }

  mobile = MobGetMobile(thing, &cmd);
  if (!mobile) {
    SendThing("^bThere doesnt appear to be a mob with that virtual number\n", thing);
    return;
  }
  if (!*cmd) {
    sprintf(buf, "mstat %ld", mobile->mVirtual);
    CmdMStat(thing, buf);
    return;
  }

  i = EditSet(thing, cmd, mobile, mobile->mSDesc->sText, mobSetList);
  if (i==-1) {
    SendThing("^wOh, good one.... I bet you made that stat up on the spot, didnt you?\n", thing);
    SendThing("^wType MSET with no arguments for a list of changeable stats\n", thing);
    return;
  } else if (i == 1)
    BITSET(areaList[AreaOf(mobile->mVirtual)].aSystem, AS_MOBUNSAVED);
}


CMDPROC(CmdMCompile) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         buf[256];
  PROPERTY    *property;
  LWORD        i;
  LWORD        area;
  BYTE         printLine;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  /* Check for Compile all command */
  if (StrExact(cmd, "all")) {
    if (!Base(thing)->bInside || Base(thing)->bInside->tType != TTYPE_WLD) {
      SendThing("^wYou cant do that here\n", thing);
      return;
    }
    area = Wld(Base(thing)->bInside)->wArea;
    for (i=0; i<areaList[area].aMobIndex.iNum; i++) {
      printLine=0;
      for (property = MobTemplate(areaList[area].aMobIndex.iThing[i])->mProperty; property; property=property->pNext) {
        
        if (property->pKey->sText[0]=='@') {
          if (!printLine) {
            printLine=1;
            sprintf(buf, 
              "^yMCOMPILE: ^c#%ld - ^b%s ", 
              MobTemplate(areaList[area].aMobIndex.iThing[i])->mVirtual, 
              MobTemplate(areaList[area].aMobIndex.iThing[i])->mSDesc->sText);

            SendThing(buf, thing);
          }
          sprintf(buf,"^G(%s) ", property->pKey->sText);
          SendThing(buf, thing);
          CodeCompileProperty(property,thing);
        }
      }
      if (printLine)
        SendThing("\n",thing);
    }
    return;
  }
  
  mobile = MobGetMobile(thing, &cmd);
  if (!mobile) {
    SendThing("^GUSUAGE: ^gMCOMPILE <Mob Template #>\n", thing);
    SendThing("^G or\n", thing);
    SendThing("^GUSUAGE: ^gMCOMPILE all\n", thing);
    return;
  }

  sprintf(buf, "^yCOMPILE: ^c#%ld - ^b%s\n", mobile->mVirtual, mobile->mSDesc->sText);
  SendThing(buf, thing);
  
  for (property = mobile->mProperty; property; property=property->pNext) {
    SendThing("^gProp.: ^c", thing);
    SendThing(property->pKey->sText, thing);
    SendThing("\n", thing);
    if (property->pKey->sText[0]=='@') {
      CodeCompileProperty(property,thing);
    }
  }
  
}


CMDPROC(CmdMDecomp) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  BYTE         buf[256];
  PROPERTY    *property;
  LWORD        i;
  LWORD        area;
  BYTE         printLine;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  /* Check for Compile all command */
  if (StrExact(cmd, "all")) {
    if (!Base(thing)->bInside || Base(thing)->bInside->tType != TTYPE_WLD) {
      SendThing("^wYou cant do that here\n", thing);
      return;
    }
    area = Wld(Base(thing)->bInside)->wArea;
    for (i=0; i<areaList[area].aMobIndex.iNum; i++) {
      printLine=0;
      for (property = MobTemplate(areaList[area].aMobIndex.iThing[i])->mProperty; property; property=property->pNext) {
        
        if (property->pKey->sText[0]=='@') {
          if (!printLine) {
            printLine=1;
            sprintf(buf, 
              "^yMDECOMP: ^c#%ld - ^b%s ", 
              MobTemplate(areaList[area].aMobIndex.iThing[i])->mVirtual, 
              MobTemplate(areaList[area].aMobIndex.iThing[i])->mSDesc->sText);

            SendThing(buf, thing);
          }
          sprintf(buf,"^G(%s) ", property->pKey->sText);
          SendThing(buf, thing);
          CodeDecompProperty(property,thing);
        }
      }
      if (printLine)
        SendThing("\n",thing);
    }
    return;
  }
  
  mobile = MobGetMobile(thing, &cmd);

  if (!mobile) {
    SendThing("^GUSUAGE: ^gMDECOMP <Mob Template #>\n", thing);
    SendThing("^G or\n", thing);
    SendThing("^GUSUAGE: ^gMDECOMP all\n", thing);
    return;
  }

  sprintf(buf, "^yDECOMP: ^c#%ld - ^b%s\n", mobile->mVirtual, mobile->mSDesc->sText);
  SendThing(buf, thing);
  
  for (property = mobile->mProperty; property; property=property->pNext) {
    SendThing("^gProp.: ^c", thing);
    SendThing(property->pKey->sText, thing);
    SendThing("\n", thing);
    if (property->pKey->sText[0]=='@') {
      CodeDecompProperty(property,thing);
    }
  }
  
}


CMDPROC(CmdMLoad) { /* void CmdProc(THING thing, BYTE* cmd) */
  MOBTEMPLATE *mobile;
  THING       *create;

  cmd = StrOneWord(cmd, NULL);
  mobile = MobGetMobile(thing, &cmd);
  if (!mobile)
    return;

  /* load one */
  create = MobileCreate(mobile, Base(thing)->bInside);
  SendAction("^wYou create a $N\n", 
             thing, create, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
  SendAction("^w$n creates a $N\n", 
             thing, create, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
}


CMDPROC(CmdMSave) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   buf[256];
  WORD   area;
  SOCK  *sock;

  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;
  area = Wld(world)->wArea;

  if (!BIT(areaList[Wld(world)->wArea].aSystem, AS_MOBUNSAVED)) {
    SendThing("There are no unsaved changes!\n", thing);
  } else {
    BITFLIP(areaList[area].aSystem, AS_MOBUNSAVED);
    MobileWrite(area);
    sock = BaseControlFind(thing);
    sprintf(buf, "%s.mob saved by %s. (%ld mobiles)\n", areaList[area].aFileName->sText, sock->sHomeThing->tSDesc->sText, areaList[area].aMobIndex.iNum);
    Log(LOG_AREA, buf);
    sprintf(buf, "%s.mob saved. (^w%ld mobiles^V)\n", areaList[area].aFileName->sText, areaList[area].aMobIndex.iNum);
    SendThing(buf, thing);
  }
}

