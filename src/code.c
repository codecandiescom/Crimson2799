/*
 * Written by B. Cameron Lesiuk, 1995
 * Written for use with Crimson2 MUD (written/copyright Ryan Haksi 1995).
 * This source is proprietary. Use of this code without permission from 
 * Ryan Haksi or Cam Lesiuk is strictly prohibited. 
 * 
 * (clesiuk@engr.uvic.ca)
 */  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
  #include <unistd.h> /* for unlink */
  #include <sys/time.h>
#endif

#include "crimson2.h"
#include "macro.h"
#include "timing.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "queue.h"
#include "send.h"
#include "property.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "mobile.h"
#include "player.h"
#include "area.h"
#include "reset.h"
#include "code.h"
#include "codestuf.h"
#include "compile.h"
#include "interp.h"
#include "decomp.h"
#include "function.h"
#include "exit.h"


#ifdef WIN32
  /* including winsock.h etc auto includes the windows types */
  /* turn off warning: benign type redefinition */
  #pragma warning( disable : 4142 )
  #include <winsock.h>
  #pragma warning( default : 4142 )
#endif

/* Globals */
ULWORD codeSet;

/* compiles pDesc of property passed to it and swaps in compiled code for
 * uncompiled code. If there's an error, no swap in performed.
 * errorthing is where errors are sent.
 */
WORD CodeDecompProperty(PROPERTY *property,THING *errorthing) {
  STR *cc; /* compiled code */
  WORD rc;
  BYTE buf[512];

  /* Check code is compiled code first */
  if (!CodeIsCompiled(property))
    return DECOMP_OK;

  rc=Decomp(property->pDesc,&cc,errorthing);
  if (!rc) {
    sprintf(buf, "%s", property->pKey->sText);
    StrToUpper(buf);
    STRFREE(property->pKey);
    property->pKey = STRCREATE(buf);
    STRFREE(property->pDesc); /* free source code STR */
    property->pDesc=cc;
  }
  return(rc);
}

WORD CodeCompileProperty(PROPERTY *property,THING *errorthing) {
  STR *cc; /* compiled code */
  WORD rc;
  BYTE buf[512];

  if (property->pKey->sText[0]!='@')
    return COMPILE_SYNTAX_ERROR;

  /* Check if code is already compiled */
  if (CodeIsCompiled(property))
    return COMPILE_OK;

  rc=Compile(property->pDesc,&cc,errorthing);
  if (!rc) {
    sprintf(buf, "%s", property->pKey->sText);
    StrToLower(buf);
    STRFREE(property->pKey);
    property->pKey = STRCREATE(buf);
    STRFREE(property->pDesc); /* free source code STR */
    property->pDesc=cc;
  }
  return(rc);
}

/* Performs all required initialization sequences.
 */
void CodeInit() {
  PROPERTY       *p; /* property */
  LWORD           i,j;
  BYTE            buf[256];
  BYTE            word[256];
  WORD            rc;
  WORD            totw,totm,toto,tote,totr;
  struct timeval  tvStartCompile;
  struct timeval  tvCompileTime;

  codeSet = 0;
  if (INILWordRead("c4.ini", "codeBootComp", 1))
    codeSet |= CODE_BOOTCOMPILE;
  if (INILWordRead("c4.ini", "codeSaveComment", 0))
    codeSet |= CODE_SAVECOMMENT;
  if (INILWordRead("c4.ini", "codeDebugComp", 0))
    codeSet |= CODE_DEBUGCOMPILE;
  if (INILWordRead("c4.ini", "codeDebugInterp", 0))
    codeSet |= CODE_DEBUGINTERP;
  if (INILWordRead("c4.ini", "codeDebugDecomp", 0))
    codeSet |= CODE_DEBUGDECOMPILE;

  CodeStufInit();  /* Initialize C4 common (global) data */
  CompileInit();   /* Initialize compiler engine */
    /* Initialize interpreter engine */
  InterpInit(INILWordRead("c4.ini", "codeMaxInstr",INTERP_MAX_INSTR));
  DecompInit();    /* Initialize decompiler engine */

  totw=totm=toto=tote=totr=0;
  TvGetTime(&tvStartCompile);

  /* Compile code for Mobiles, World, and Objects */
  if (codeSet & CODE_BOOTCOMPILE) {
    for (i=0;i<areaListMax;i++) {
      sprintf(buf,"Compiling Area: [%-18s]\n", StrFirstWord(areaList[i].aFileName->sText,word));
      Log(LOG_BOOT,buf);
      for(j=0;j<areaList[i].aWldIndex.iNum;j++) {
        for (p=areaList[i].aWldIndex.iThing[j]->tProperty;p;p=p->pNext) {
          if (p->pKey->sText[0]=='@') {
            totw++;
            if ((rc=CodeCompileProperty(p,NULL))) {
              CodeClearFlag(areaList[i].aWldIndex.iThing[i], p);
              tote++;
              sprintf(buf,"  COMPILE ERROR %i: area [%-18s]   room #[%5ld] [%-10s]\n",
                rc,areaList[i].aFileName->sText,
                Wld(areaList[i].aWldIndex.iThing[j])->wVirtual,
                p->pKey->sText);
              Log(LOG_ERROR,buf);
            } else CodeSetFlag(areaList[i].aWldIndex.iThing[j], p);
          }
        }
      }
      for(j=0;j<areaList[i].aMobIndex.iNum;j++) {
        for (p=MobTemplate(areaList[i].aMobIndex.iThing[j])->mProperty;p;p=p->pNext) {
          if (p->pKey->sText[0]=='@') {
            totm++;
            if ((rc=CodeCompileProperty(p,NULL))) {
              tote++;
              sprintf(buf,"  COMPILE ERROR %i: area [%-18s] mobile #[%5ld] [%-10s]\n",
                rc,areaList[i].aFileName->sText,
                MobTemplate(areaList[i].aMobIndex.iThing[j])->mVirtual,
                p->pKey->sText);
              Log(LOG_ERROR,buf);
            }
          }
        }
      }
      for(j=0;j<areaList[i].aObjIndex.iNum;j++) {
        for (p=ObjTemplate(areaList[i].aObjIndex.iThing[j])->oProperty;p;p=p->pNext) {
          if (p->pKey->sText[0]=='@') {
            toto++;
            if ((rc=CodeCompileProperty(p,NULL))) {
              tote++;
              sprintf(buf,"  COMPILE ERROR %i: area [%-18s] object #[%5ld] [%-10s]\n",
                rc,areaList[i].aFileName->sText,
                ObjTemplate(areaList[i].aObjIndex.iThing[j])->oVirtual,
                p->pKey->sText);
              Log(LOG_ERROR,buf);
            }
          }
        }
      }
      for (p=areaList[i].aResetThing.tProperty;p;p=p->pNext) {
        if (p->pKey->sText[0]=='@') {
          totr++;
          if ((rc=CodeCompileProperty(p,NULL))) {
            CodeClearFlag(&areaList[i].aResetThing, p);
            tote++;
            sprintf(buf,"  COMPILE ERROR %i: area [%-18s] Reset Script [%-10s]\n",
              rc,areaList[i].aFileName->sText,
              p->pKey->sText);
            Log(LOG_ERROR,buf);
          } else CodeSetFlag(&areaList[i].aResetThing, p);
        }
      }
    }
    sprintf(buf,"Compile totals: Wld[%5i] Mob[%5i] Obj[%5i] Rst[%5i] Err[%5i]\n",totw,totm,toto,totr,tote);
    Log(LOG_BOOT,buf);

    /* calculate total compile time */
    TvGetTime(&tvCompileTime);
    TVSUB(tvCompileTime, tvStartCompile);
    sprintf(buf, "Total time spent compiling: %ld.%ld second(s)\n", (LWORD)tvCompileTime.tv_sec, (LWORD)tvCompileTime.tv_usec);
    Log(LOG_BOOT,buf);
  }
}

/* Set appropriate thing flags for attached code in property */
void CodeSetFlag(THING *thing, PROPERTY *property) {
      if (StrExact(property->pKey->sText, "@IDLE"))               BITSET(thing->tFlag, TF_IDLECODE);
      else if (StrExact(property->pKey->sText, "@FIGHTING"))      BITSET(thing->tFlag, TF_FIGHTINGCODE);
      else if (StrExact(property->pKey->sText, "@COMMAND"))       BITSET(thing->tFlag, TF_COMMANDCODE);
      else if (StrExact(property->pKey->sText, "@ENTRY"))         BITSET(thing->tFlag, TF_ENTRYCODE);
      else if (StrExact(property->pKey->sText, "@DEATH"))         BITSET(thing->tFlag, TF_DEATHCODE);
      else if (StrExact(property->pKey->sText, "@EXIT"))          BITSET(thing->tFlag, TF_EXITCODE);
      else if (StrExact(property->pKey->sText, "@FLEE"))          BITSET(thing->tFlag, TF_FLEECODE);
      else if (StrExact(property->pKey->sText, "@RESET"))         BITSET(thing->tFlag, TF_RESETCODE);
      else if (StrExact(property->pKey->sText, "@DAMAGE"))        BITSET(thing->tFlag, TF_DAMAGECODE);
      else if (StrExact(property->pKey->sText, "@USE"))           BITSET(thing->tFlag, TF_USECODE);
      else if (StrExact(property->pKey->sText, "@AFTERFIGHTING")) BITSET(thing->tFlag, TF_AFTERFIGHTINGCODE);
      else if (StrExact(property->pKey->sText, "@AFTERCOMMAND"))  BITSET(thing->tFlag, TF_AFTERCOMMANDCODE);
      else if (StrExact(property->pKey->sText, "@AFTERENTRY"))    BITSET(thing->tFlag, TF_AFTERENTRYCODE);
      else if (StrExact(property->pKey->sText, "@AFTERDEATH"))    BITSET(thing->tFlag, TF_AFTERDEATHCODE);
      else if (StrExact(property->pKey->sText, "@AFTERFLEE"))     BITSET(thing->tFlag, TF_AFTERFLEECODE);
      else if (StrExact(property->pKey->sText, "@AFTERRESET"))    BITSET(thing->tFlag, TF_AFTERRESETCODE);
      else if (StrExact(property->pKey->sText, "@AFTEREXIT"))     BITSET(thing->tFlag, TF_AFTEREXITCODE);
      else if (StrExact(property->pKey->sText, "@AFTERDAMAGE"))   BITSET(thing->tFlag, TF_AFTERDAMAGECODE);
      else if (StrExact(property->pKey->sText, "@AFTERUSE"))      BITSET(thing->tFlag, TF_AFTERUSECODE);
      else if (StrExact(property->pKey->sText, "@AFTERATTACK"))   BITSET(thing->tFlag, TF_AFTERATTACKCODE);
}
/* Clear appropriate thing flags for attached code in property  */
void CodeClearFlag(THING *thing, PROPERTY *property) {
      if (StrExact(property->pKey->sText, "@IDLE"))               BITCLR(thing->tFlag, TF_IDLECODE);
      else if (StrExact(property->pKey->sText, "@FIGHTING"))      BITCLR(thing->tFlag, TF_FIGHTINGCODE);
      else if (StrExact(property->pKey->sText, "@COMMAND"))       BITCLR(thing->tFlag, TF_COMMANDCODE);
      else if (StrExact(property->pKey->sText, "@ENTRY"))         BITCLR(thing->tFlag, TF_ENTRYCODE);
      else if (StrExact(property->pKey->sText, "@DEATH"))         BITCLR(thing->tFlag, TF_DEATHCODE);
      else if (StrExact(property->pKey->sText, "@EXIT"))          BITCLR(thing->tFlag, TF_EXITCODE);
      else if (StrExact(property->pKey->sText, "@FLEE"))          BITCLR(thing->tFlag, TF_FLEECODE);
      else if (StrExact(property->pKey->sText, "@RESET"))         BITCLR(thing->tFlag, TF_RESETCODE);
      else if (StrExact(property->pKey->sText, "@DAMAGE"))        BITCLR(thing->tFlag, TF_DAMAGECODE);
      else if (StrExact(property->pKey->sText, "@USE"))           BITCLR(thing->tFlag, TF_USECODE);
      else if (StrExact(property->pKey->sText, "@AFTERFIGHTING")) BITCLR(thing->tFlag, TF_AFTERFIGHTINGCODE);
      else if (StrExact(property->pKey->sText, "@AFTERCOMMAND"))  BITCLR(thing->tFlag, TF_AFTERCOMMANDCODE);
      else if (StrExact(property->pKey->sText, "@AFTERENTRY"))    BITCLR(thing->tFlag, TF_AFTERENTRYCODE);
      else if (StrExact(property->pKey->sText, "@AFTERDEATH"))    BITCLR(thing->tFlag, TF_AFTERDEATHCODE);
      else if (StrExact(property->pKey->sText, "@AFTEREXIT"))     BITCLR(thing->tFlag, TF_AFTEREXITCODE);
      else if (StrExact(property->pKey->sText, "@AFTERFLEE"))     BITCLR(thing->tFlag, TF_AFTERFLEECODE);
      else if (StrExact(property->pKey->sText, "@AFTERRESET"))    BITCLR(thing->tFlag, TF_AFTERRESETCODE);
      else if (StrExact(property->pKey->sText, "@AFTERDAMAGE"))   BITCLR(thing->tFlag, TF_AFTERDAMAGECODE);
      else if (StrExact(property->pKey->sText, "@AFTERUSE"))      BITCLR(thing->tFlag, TF_AFTERUSECODE);
      else if (StrExact(property->pKey->sText, "@AFTERATTACK"))   BITCLR(thing->tFlag, TF_AFTERATTACKCODE);
}


/* Set appropriate thing flags for attached codes in property list (if any) */
void CodeCheckFlag(THING *thing) {
  PROPERTY *i;

  for (i=thing->tProperty; i; i=i->pNext) {
    if ((StrExact(i->pDesc->sText,CODE_PROG_HEADER))&&(i->pDesc->sLen>CODE_PROG_HEADER_LENGTH)) {
      CodeSetFlag(thing, i);
    }
  }
}

PROPERTY *CodeFind(PROPERTY *property, BYTE *eventStr) {
  PROPERTY *i;

  for (i=property; i; i=i->pNext) {
    if (StrExact(i->pKey->sText, eventStr)) return i;
  }
  return NULL;
}


/* On the various CodeParse functions */
/*
   eventThing is the thing the event happened in the presence of,
   codeThing is the thing with the code attached to its property list

   this routine should pass back TRUE if it does not want default processing
   to take place, otherwise FALSE
 */

/* called during periodically, note: not *EVERY* tick the mud server
   will probably wait state everything over 4 ticks or something so
   that there is not a HUGE processing load all the time */
BYTE CodeParseIdle(THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_IDLECODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@IDLE");
  if (code) {
    if (codeThing->tIdleWait>0) {
      codeThing->tIdleWait--;
    } else if (codeThing->tIdleWait==0){
      InterpSnoop("\nC4: ***** @IDLE ",codeThing);
      if (codeThing->tSDesc)
        InterpSnoop(codeThing->tSDesc->sText,codeThing);
      InterpSnoop("\n",codeThing);
      if (Interp(code->pDesc,NULL,codeThing,NULL,NULL,&block,NULL)) {
        BITCLR(codeThing->tFlag, TF_IDLECODE); /* if error, don't repeat run */
        sprintf(buf,"C4 Error in %s (@IDLE); code deactivated\n",codeThing->tSDesc->sText);
        Log(LOG_ERROR,buf);
      } 
      return block;
    }
  } 
  return FALSE;
}



/* called during every "round" of fighting */
BYTE CodeParseFighting(THING *eventThing, THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_FIGHTINGCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@FIGHTING");
  if (code) {
    InterpSnoop("\nC4: ***** @FIGHTING ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_FIGHTINGCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@FIGHTING); code deactivated\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}
BYTE CodeParseAfterFighting(THING *eventThing, THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_AFTERFIGHTINGCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERFIGHTING");
  if (code) {
    InterpSnoop("\nC4: ***** @AFTERFIGHTING ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_AFTERFIGHTINGCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERFIGHTING); code deactivated\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}



/* a socket initiates a command */
BYTE CodeParseCommand(THING *eventThing, THING *codeThing, BYTE *cmd) {
  PROPERTY *code;
  STR *cmdstr;
  LWORD block;
  BYTE buf[100];

  if (!codeThing || !eventThing) return FALSE;
  if (!BIT(codeThing->tFlag, TF_COMMANDCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@COMMAND");
  if (code) {
    cmdstr=STRCREATE(cmd);
    InterpSnoop("\nC4: ***** @COMMAND ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,cmdstr,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_COMMANDCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@COMMAND); code deactivated\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    STRFREE(cmdstr);
    return block;
  } else
    return FALSE;
}
BYTE CodeParseAfterCommand(THING *eventThing, THING *codeThing, BYTE *cmd) {
  PROPERTY *code;
  STR *cmdstr;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_AFTERCOMMANDCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERCOMMAND");
  if (code) {
    cmdstr=STRCREATE(cmd);
    InterpSnoop("\nC4: ***** @AFTERCOMMAND ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,cmdstr,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_AFTERCOMMANDCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERCOMMAND); code deactivated\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    STRFREE(cmdstr);
    return block;
  } else
    return FALSE;
}

/* something tries to enter the room - do we really want this to be blockable?
   right now it is, in any case you can find where they came from by looking
   at what room they are currently in - and you can find where they are going by
   looking at what room codeThing is in....
*/
BYTE CodeParseEntry(THING *eventThing, THING *codeThing, EXIT *exit) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_ENTRYCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@ENTRY");
  if (code) {
    InterpSnoop("\nC4: ***** @ENTRY ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,exit)) {
      BITCLR(codeThing->tFlag, TF_ENTRYCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@ENTRY); code deactivated\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}
BYTE CodeParseAfterEntry(THING *eventThing, THING *codeThing, EXIT *exit) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!codeThing || !BIT(codeThing->tFlag, TF_AFTERENTRYCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERENTRY");
  if (code) {
    InterpSnoop("\nC4: ***** @AFTERENTRY ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,exit)) {
      BITCLR(codeThing->tFlag, TF_AFTERENTRYCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERENTRY); code deactivated\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}



/* something tries to Exit the room */
BYTE CodeParseExit(THING *eventThing, THING *codeThing, EXIT *exit) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];
  THING *exitThing;

  if (!BIT(codeThing->tFlag, TF_EXITCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@EXIT");
  if (exit) {
    exitThing=Thing(exit->eWorld);
  } else {
    exitThing=NULL;
  }
  if (code) {
    InterpSnoop("\nC4: ***** @EXIT ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,exitThing,NULL,&block,exit)) {
      BITCLR(codeThing->tFlag, TF_EXITCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@EXIT); code deactivated\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}
BYTE CodeParseAfterExit(THING *eventThing, THING *codeThing, EXIT *exit) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];
  THING *exitThing;

  if (!BIT(codeThing->tFlag, TF_AFTEREXITCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTEREXIT");
  if (exit) {
    exitThing=Thing(exit->eWorld);
  } else {
    exitThing=NULL;
  }
  if (code) {
    InterpSnoop("\nC4: ***** @AFTEREXIT ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,exitThing,NULL,&block,exit)) {
      BITCLR(codeThing->tFlag, TF_AFTEREXITCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTEREXIT); code deactivated\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}

/* something dies in the room */
BYTE CodeParseDeath(THING *eventThing, THING *codeThing, THING *deathThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_DEATHCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@DEATH");
  if (code) {
    InterpSnoop("\nC4: ***** @DEATH ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,deathThing,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_DEATHCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@DEATH); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}
BYTE CodeParseAfterDeath(THING *eventThing, THING *codeThing, THING *deathThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_AFTERDEATHCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERDEATH");
  if (code) {
    InterpSnoop("\nC4: ***** @AFTERDEATH ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,deathThing,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_AFTERDEATHCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERDEATH); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}

/* eventThing just tried to run (yella) */
BYTE CodeParseFlee(THING *eventThing, THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_FLEECODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@FLEE");
  if (code) {
    InterpSnoop("\nC4: ***** @FLEE ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_FLEECODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@FLEE); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}
BYTE CodeParseAfterFlee(THING *eventThing, THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_AFTERFLEECODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERFLEE");
  if (code) {
    InterpSnoop("\nC4: ***** @AFTERFLEE ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_AFTERFLEECODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERFLEE); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}

/* eventThing points to areas aResetThing mostly so you can
  check/set properties for the area */
BYTE CodeParseReset(THING *eventThing, THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_RESETCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@RESET");
  if (code) {
    InterpSnoop("\nC4: ***** @RESET ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_RESETCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@RESET); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}
BYTE CodeParseAfterReset(THING *eventThing, THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_AFTERRESETCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERRESET");
  if (code) {
    InterpSnoop("\nC4: ***** @AFTERRESET ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_AFTERRESETCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERRESET); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}

/* something takes damage, is this redundant with fighting? */
BYTE CodeParseDamage(THING *eventThing, THING *codeThing, THING *damageThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_DAMAGECODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@DAMAGE");
  if (code) {
    InterpSnoop("\nC4: ***** @DAMAGE ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,damageThing,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_DAMAGECODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@DAMAGE); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}
BYTE CodeParseAfterDamage(THING *eventThing, THING *codeThing, THING *damageThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_AFTERDAMAGECODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERDAMAGE");
  if (code) {
    InterpSnoop("\nC4: ***** @AFTERDAMAGE ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,damageThing,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_AFTERDAMAGECODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERDAMAGE); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}

/* Try to use something */
BYTE CodeParseUse(THING *eventThing, THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_USECODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@USE");
  if (code) {
    InterpSnoop("\nC4: ***** @USE ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_USECODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@USE); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}
BYTE CodeParseAfterUse(THING *eventThing, THING *codeThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_AFTERUSECODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERUSE");
  if (code) {
    InterpSnoop("\nC4: ***** @AFTERUSE ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,NULL,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_AFTERUSECODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERUSE); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}

BYTE CodeParseAfterAttack(THING *eventThing, THING *codeThing, THING *targetThing) {
  PROPERTY *code;
  LWORD block;
  BYTE buf[100];

  if (!BIT(codeThing->tFlag, TF_AFTERATTACKCODE))
    return FALSE;
  code = CodeFind(codeThing->tProperty, "@AFTERATTACK");
  if (code) {
    InterpSnoop("\nC4: ***** @AFTERATTACK ",codeThing);
    if (codeThing->tSDesc)
      InterpSnoop(codeThing->tSDesc->sText,codeThing);
    InterpSnoop("\n",codeThing);
    if (Interp(code->pDesc,eventThing,codeThing,targetThing,NULL,&block,NULL)) {
      BITCLR(codeThing->tFlag, TF_AFTERATTACKCODE); /* if error, don't repeat run */
      sprintf(buf,"C4 Error in %s (@AFTERATTACK); code deactivated.\n",codeThing->tSDesc->sText);
      Log(LOG_ERROR,buf);
    }
    return block;
  } else return FALSE;
}

BYTE CodeIsCompiled(PROPERTY *property) {
  if (property) 
    if ((property->pKey)&&(property->pDesc))
      if ((property->pKey->sText)&&(property->pDesc->sText))
        if ((!strcmp(property->pDesc->sText,CODE_PROG_HEADER))&&
            (property->pDesc->sLen>CODE_PROG_HEADER_LENGTH)  &&
            (property->pKey->sText[0]=='@'))
          return TRUE;
  /* otherwise fall through to here */
  return FALSE;
}

/* Free's a compile str. This needs a special function because */
/* The compiled str has embeded pointers to other str's; if we */
/* just did a StrFree(str) on a compiled code, the embedded    */
/* pointers would be lost and some str's would be left floating */
/* around in memory, never to be freed. They would still be    */
/* hashed, so it may never matter, but it's bad nonetheless.   */
/* NOTE: This proc only free's the embedded str's, not the     */
/* passed parameter str itself!!!                              */
/* Note: pass it off to DECOMP module - it has the tools to    */
/* handle this job.                                            */
STR *CodeStrFree(STR *str) {
  DecompStrFree(str);
  return str;
}
