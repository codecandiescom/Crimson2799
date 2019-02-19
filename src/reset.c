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
#include <ctype.h>
#ifndef WIN32
  #include <unistd.h>
#endif

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "file.h"
#include "extra.h"
#include "property.h"
#include "code.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "exit.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "mobile.h"
#include "area.h"
#include "reset.h"
#include "cmd_inv.h"

BYTE *rFlagList[] = {
  "RESET-WHEN-EMPTY",
  "RESET-IMMEDIATELY",
  "AREA-NO-ENTER",
  "AREA-NO-TAKE",
  "AREA-NO-EXPERIENCE",
  "AREA-NO-MONEY",
  "AREA-NO-TELEPORT-IN",
  "AREA-NO-TELEPORT-OUT",
  "AREA-MOBFREEZE",
  "AREA-IS-CLOSED",
  ""
};

BYTE     resetErrorLog; /* log if a reset is illegal (bad ref etc.) */
BYTE     resetReadLog;  /* log each world structure as its read */
LWORD    resetNum = 0;  /* number of world structures in use - just curious really */

void ResetInit(void) {
  BYTE buf[256];

  resetErrorLog = INILWordRead("crimson2.ini", "resetErrorLog", 0);
  resetReadLog  = INILWordRead("crimson2.ini", "resetReadLog", 0);
  sprintf(buf, "Reading Reset logging defaults\n");
  Log(LOG_BOOT, buf);
  sprintf(buf, "Reset structure size is %d bytes\n", sizeof(RESETLIST));
  Log(LOG_BOOT, buf);
}


void ResetRead(WORD area) {
  FILE        *resetFile;
  BYTE         resetFileBuf[256];
  BYTE         buf[256];
  BYTE         tmp[256];
  RESETLIST   *resetList = NULL;
  LWORD        resetI = -1;
  LWORD        i;
  STR         *sKey;
  STR         *sDesc;
  
  sprintf(resetFileBuf, "area/%s.rst", areaList[area].aFileName->sText);
  resetFile = fopen(resetFileBuf, "rb");
  if (!resetFile) { /* hardly a fatal error */
    sprintf(buf, "Unable to read %s\n", resetFileBuf);
    Log(LOG_BOOT, buf);
    return;
  }
  if (areaList[area].aOffset) {
    sprintf(buf, "Relocating Area/Resets [%s]\n", areaList[area].aFileName->sText);
    Log(LOG_BOOT, buf);
  }
  
  /* Read Properties */
  while (!feof(resetFile)) {
    fgets(buf, sizeof(buf), resetFile);
    
    /* in original diku this was the zone #, but now just indicates last property */
    if (buf[0] == '#')
      break;

    else if (buf[0] == 'P') { /* property */
      sKey = FileStrRead(resetFile); 
      sDesc= FileStrRead(resetFile);
      areaList[area].aResetThing.tProperty =
        PropertyCreate(areaList[area].aResetThing.tProperty, sKey, sDesc);
    }
  }
  
  FILESTRREAD(resetFile, areaList[area].aDesc);
  fscanf(resetFile, " %*d %hd ", &areaList[area].aResetDelay); 

  /* althought there is no performance reason for staggereing the resets
   * since the code only will execute 1 reset per second, its annoying if
   * you're watching the area channel to have them all go off 1/second
   */
  areaList[area].aResetLast = -1*Number(0,areaList[area].aResetDelay*60-1);

  areaList[area].aResetFlag = FileFlagRead(resetFile, rFlagList);  
  fscanf(resetFile, "\n");  /* make sure we read the return */
 
  /* okay we opened it up so read it.... */
  while (!feof(resetFile)) {
    /* read in a line */
    fgets(tmp, 255, resetFile);
    if (resetReadLog)
      Log(LOG_BOOT, tmp);
    if (tmp[0] == '$' || tmp[0] == 'S' || feof(resetFile))
      break; /* Dikumud file format EOF character */
    tmp[255] = '\0';
    if (resetReadLog) {
      sprintf(buf, "%s.rst:", areaList[area].aFileName->sText);
      Log(LOG_BOOT, buf);
      LogPrintf(LOG_BOOT, tmp);
    }
    /* kill off trailing \n's and \r's */
    for(i=strlen(tmp)-1; i>=0&&(tmp[i]=='\n'||tmp[i]=='\r'); i++)
      tmp[i]='\0';
    
    /* alloc some space for this */
    resetI++;
    REALLOC("ResetRead(reset.c): resetList reallocation\n", resetList, RESETLIST, resetI+1, areaList[area].aResetByte);
    memset( (void*)&resetList[resetI], 0, sizeof(RESETLIST)); /* init to zeros */

    /* Make reset commands case insensitive */
    tmp[0] = toupper(tmp[0]);

    switch (tmp[0]) {
    case '*': /* comment */
      resetList[resetI].rCmd = '*';
      resetList[resetI].rArg1.rStr = STRCREATE(tmp);
      break;
      
    case 'O':
      resetList[resetI].rArg4.rNum = 1; /* default room max */
      sscanf(tmp, "%c %hd %ld %ld %ld %ld", 
       &resetList[resetI].rCmd, 
       &resetList[resetI].rIf, 
       &resetList[resetI].rArg1.rNum, 
       &resetList[resetI].rArg2.rNum, 
       &resetList[resetI].rArg3.rNum, 
       &resetList[resetI].rArg4.rNum);
      break;
      
    case 'M':
      resetList[resetI].rArg4.rNum = 99; /* default room max */
    case 'G':
    case 'P':
    case 'E':
    case 'D':
    case 'R':
      sscanf(tmp, "%c %hd %ld %ld %ld %ld", 
       &resetList[resetI].rCmd, 
       &resetList[resetI].rIf, 
       &resetList[resetI].rArg1.rNum, 
       &resetList[resetI].rArg2.rNum, 
       &resetList[resetI].rArg3.rNum, 
       &resetList[resetI].rArg4.rNum);
      break;
      
    default:
      sprintf(buf, 
        "ResetRead(reset.c) %s, line#%ld theres a problem with:\n", 
        areaList[area].aFileName->sText,
        resetI + StrNumLine(areaList[area].aDesc) + 2);
      Log(LOG_BOOT, buf);
      Log(LOG_BOOT, tmp);
      resetList[resetI].rCmd = '#';
      break;
    } /* switch */

    /* Relocate Virtuals as necessary */
    if (areaList[area].aOffset) {
      switch(resetList[resetI].rCmd) {
      case 'M':
      case 'O':
      case 'P':
        resetList[resetI].rArg3.rNum = AreaMove(area, resetList[resetI].rArg3.rNum);
        /* Fall thru */
      case 'G':
      case 'E':
      case 'D':
      case 'R':
        resetList[resetI].rArg1.rNum = AreaMove(area, resetList[resetI].rArg1.rNum);
        break;
      }
    }
  } /* while */
  /* all done close up shop */
  fclose(resetFile);
  areaList[area].aResetList = resetList;
  areaList[area].aResetNum  = resetI;
  resetNum += resetI;
}


void ResetReIndex(void) {
  BYTE         buf[256];
  RESETLIST   *resetList;
  LWORD        area;
  LWORD        resetI;
  
  Log(LOG_BOOT, "Re-Indexing Reset Entries\n");
  for (area = 0; area<areaListMax; area++) {
    resetList = areaList[area].aResetList;
    for (resetI = 0; resetI<areaList[area].aResetNum; resetI++) {
      switch (resetList[resetI].rCmd) {
      case '*': /* comment */
  break;
  
      case 'M':
  resetList[resetI].rArg1.rMob = MobileOf(resetList[resetI].rArg1.rNum);
  if (!resetList[resetI].rArg1.rMob) {
    if (resetErrorLog) {
      sprintf(buf, 
        "ResetReIndex(reset.c) %s,line#%ld Illegal Mob#\n", 
        areaList[area].aFileName->sText,
        resetI + StrNumLine(areaList[area].aDesc) + 2);
      Log(LOG_BOOT, buf);
    }
    resetList[resetI].rCmd = '#';
    break;
  } 
  resetList[resetI].rArg3.rWld = WorldOf(resetList[resetI].rArg3.rNum);
  if (!resetList[resetI].rArg3.rWld) {
    if (resetErrorLog) {
      sprintf(buf, 
        "ResetReIndex(reset.c) %s, line#%ld Illegal World #\n", 
        areaList[area].aFileName->sText,
        resetI + StrNumLine(areaList[area].aDesc) + 2);
      Log(LOG_BOOT, buf);
    }
    resetList[resetI].rCmd = '#';
  }
  break;
  
      case 'O':
  resetList[resetI].rArg1.rObj = ObjectOf(resetList[resetI].rArg1.rNum);
  if (!resetList[resetI].rArg1.rObj) {
    if (resetErrorLog) {
      sprintf(buf, 
        "ResetReIndex(reset.c) %s, line#%ld Illegal Object #\n", 
        areaList[area].aFileName->sText,
        resetI + StrNumLine(areaList[area].aDesc) + 2);
      Log(LOG_BOOT, buf);
    }
    resetList[resetI].rCmd = '#';
    break;
  } 
  resetList[resetI].rArg3.rWld = WorldOf(resetList[resetI].rArg3.rNum);
  if (!resetList[resetI].rArg3.rWld) {
    if (resetErrorLog) {
      sprintf(buf, 
        "ResetReIndex(reset.c) %s, line#%ld Illegal World #\n", 
        areaList[area].aFileName->sText,
        resetI + StrNumLine(areaList[area].aDesc) + 2);
      Log(LOG_BOOT, buf);
    }
    resetList[resetI].rCmd = '#';
  }
  break;
  
      case 'G':
      case 'E':
  resetList[resetI].rArg1.rObj = ObjectOf(resetList[resetI].rArg1.rNum);
  if (!resetList[resetI].rArg1.rObj) {
    if (resetErrorLog) {
      sprintf(buf, 
        "ResetReIndex(reset.c) %s, line#%ld Illegal Object#\n", 
        areaList[area].aFileName->sText,
        resetI + StrNumLine(areaList[area].aDesc) + 2);
      Log(LOG_BOOT, buf);
    }
    resetList[resetI].rCmd = '#';
  } 
  break;
  
      case 'P':
  resetList[resetI].rArg1.rObj = ObjectOf(resetList[resetI].rArg1.rNum);
  if (!resetList[resetI].rArg1.rObj) {
    if (resetErrorLog) {
      sprintf(buf, 
        "ResetReIndex(reset.c) %s, line#%ld Illegal object #\n", 
        areaList[area].aFileName->sText,
        resetI + StrNumLine(areaList[area].aDesc) + 2);
      Log(LOG_BOOT, buf);
    }
    resetList[resetI].rCmd = '#';
  } 
  break;
  
      case 'D':
      case 'R':
  resetList[resetI].rArg1.rWld = WorldOf(resetList[resetI].rArg1.rNum);
  if (!resetList[resetI].rArg1.rWld) {
    if (resetErrorLog) {
      sprintf(buf, 
        "ResetReIndex(reset.c) %s, line#%ld Illegal World #\n", 
        areaList[area].aFileName->sText,
        resetI + StrNumLine(areaList[area].aDesc) + 2);
      Log(LOG_BOOT, buf);
    }
    resetList[resetI].rCmd = '#';
  } 
  break;
  
      default:
  sprintf(buf, 
    "ResetReIndex(reset.c) %s, line#%ld Illegal command (memory corruption?!?)\n", 
    areaList[area].aFileName->sText,
    resetI + StrNumLine(areaList[area].aDesc) + 2);
  Log(LOG_BOOT, buf);
  resetList[resetI].rCmd = '#';
  break;
      } /* switch */
    } 
  }
}

void ResetWrite(WORD area) {
  FILE        *resetFile;
  BYTE         resetFileBuf[256];
  BYTE         buf[256];
  BYTE         line[256];
  BYTE         comment[256];
  OBJTEMPLATE *obj;
  RESETLIST   *resetList = NULL;
  LWORD        resetI = -1;
  PROPERTY    *property;
  
  /* take a backup case we crash halfways through this */
  sprintf(buf,
    "mv area/rst/%s.rst area/rst/%s.rst.bak",
    areaList[area].aFileName->sText,
    areaList[area].aFileName->sText);
  system(buf);

  sprintf(resetFileBuf, "area/%s.rst", areaList[area].aFileName->sText);
  resetFile = fopen(resetFileBuf, "wb");
  if (!resetFile) { /* hardly a fatal error */
    sprintf(buf, "Unable to write %s\n", resetFileBuf);
    Log(LOG_ERROR, buf);
    PERROR("ResetWrite");
    return;
  }
  
  /* write out property list here */
      /* property's */
  for (property=areaList[area].aResetThing.tProperty; property; property=property->pNext) {
    fprintf(resetFile, "P\n");
    FileStrWrite(resetFile, property->pKey);
    if ( CodeIsCompiled(property) ) {
      if (!areaWriteBinary) {
        /* Decompile the code b4 saving */
        CodeDecompProperty(property, NULL);
        /* Warn if it didnt decompile */
        if (CodeIsCompiled(property)) {
          sprintf(buf, "ResetWrite: Property %s failed to decompile for area#%hd!\n", 
            property->pKey->sText, 
            area);
          Log(LOG_AREA, buf);
        } else {
          CodeClearFlag(&areaList[area].aResetThing, property);
          FileStrWrite(resetFile, property->pDesc);
          /* recompile the code */
          if (!CodeCompileProperty(property, NULL))
            CodeSetFlag(&areaList[area].aResetThing, property);
        }
      } else {
        FileBinaryWrite(resetFile, property->pDesc);
      }
    } else {
      FileStrWrite(resetFile, property->pDesc);
    }
  }
  
  fprintf(resetFile, "#%d\n", 0);  /* in original diku this was the zone #, but now indicates end of prop list */
  FileStrWrite(resetFile, areaList[area].aDesc);
  fprintf(resetFile, "%ld %hd ", areaList[area].aResetNum, areaList[area].aResetDelay); 
  FileFlagWrite(resetFile, areaList[area].aResetFlag, rFlagList, '\n');  
 
  /* okay we opened it up so write it.... */
  resetList = areaList[area].aResetList;
  for(resetI=0; resetI<areaList[area].aResetNum; resetI++) {
    switch (resetList[resetI].rCmd) {
    case '#': /* disabled command */
      break;

    case '*': /* comment */
      fprintf(resetFile, "%s\n",
        resetList[resetI].rArg1.rStr->sText);
      continue;
      
    case 'O':
      sprintf(line, "%c %hd %ld %ld %ld %ld", 
       resetList[resetI].rCmd, 
       resetList[resetI].rIf, 
       resetList[resetI].rArg1.rObj->oVirtual, 
       resetList[resetI].rArg2.rNum, 
       Wld(resetList[resetI].rArg3.rWld)->wVirtual, 
       resetList[resetI].rArg4.rNum);
      sprintf(comment, "%s", resetList[resetI].rArg1.rObj->oSDesc->sText);
      break;
      
    case 'M':
      sprintf(line, "%c %hd %ld %ld %ld %ld", 
       resetList[resetI].rCmd, 
       resetList[resetI].rIf, 
       resetList[resetI].rArg1.rMob->mVirtual, 
       resetList[resetI].rArg2.rNum, 
       Wld(resetList[resetI].rArg3.rWld)->wVirtual, 
       resetList[resetI].rArg4.rNum);
      sprintf(comment, "%s", resetList[resetI].rArg1.rMob->mSDesc->sText);
      break;

    case 'G':
    case 'E':
      sprintf(line, "%c %hd %ld %ld", 
       resetList[resetI].rCmd, 
       resetList[resetI].rIf, 
       resetList[resetI].rArg1.rObj->oVirtual, 
       resetList[resetI].rArg2.rNum); 
      sprintf(comment, "%s", resetList[resetI].rArg1.rObj->oSDesc->sText);
      break;

    case 'P':
      sprintf(line, "%c %hd %ld %ld %ld", 
       resetList[resetI].rCmd, 
       resetList[resetI].rIf, 
       resetList[resetI].rArg1.rObj->oVirtual, 
       resetList[resetI].rArg2.rNum, 
       resetList[resetI].rArg3.rNum); 
      sprintf(comment, "%s", resetList[resetI].rArg1.rObj->oSDesc->sText);
      break;

    case 'D':
      sprintf(line, "%c %hd %ld %ld %ld", 
       resetList[resetI].rCmd, 
       resetList[resetI].rIf, 
       Wld(resetList[resetI].rArg1.rWld)->wVirtual, 
       resetList[resetI].rArg2.rNum, 
       resetList[resetI].rArg3.rNum); 
      sprintf(comment, "%s-%s", resetList[resetI].rArg1.rWld->tSDesc->sText, dirList[resetList[resetI].rArg2.rNum]);
      break;

    case 'R':
      sprintf(line, "%c %hd %ld %ld", 
        resetList[resetI].rCmd, 
        resetList[resetI].rIf, 
        Wld(resetList[resetI].rArg1.rWld)->wVirtual, 
        resetList[resetI].rArg2.rNum); 
      obj = ObjectOf(resetList[resetI].rArg2.rNum); 
      if (obj) 
        sprintf(comment, "%s", obj->oSDesc->sText);
      else 
        sprintf(comment, "%s", "<Unknown object>");
      break;

    default:
      Log(LOG_ERROR, "ResetWrite(reset.c):Illegal reset command type\n");
      break;
    } /* switch */

    if (resetList[resetI].rIf)
      fprintf(resetFile, "%-30s   %s\n", line, comment);
    else
      fprintf(resetFile, "%-30s %s\n", line, comment);
  } /* for */
  /* all done close up shop */
  fprintf(resetFile, "*\nS\n*\n$\n");
  fclose(resetFile);
}

void ResetArea(LWORD area) {
  if (!CodeParseReset(&areaList[area].aResetThing, &areaList[area].aResetThing))
    ResetAreaPrimitive(area);
}

void ResetAreaPrimitive(LWORD area) {
  THING     *mobile;
  THING     *object;
  LWORD      i;
  LWORD      last;
  RESETLIST *resetList;
  BYTE       buf[256];

  mobile=object=NULL;
  last = TRUE;
  for(i=0; i<areaList[area].aResetNum; i++) {
    resetList = areaList[area].aResetList;
    switch (resetList[i].rCmd) {
      case 'M':
        if ((resetList[i].rArg1.rMob->mOnline < resetList[i].rArg2.rNum)
          &&(!resetList[i].rIf || last)
          &&((resetList[i].rArg4.rNum<=0) || (MobilePresent(resetList[i].rArg1.rMob, resetList[i].rArg3.rWld) < resetList[i].rArg4.rNum))
        ) {
          mobile = MobileCreate(resetList[i].rArg1.rMob, resetList[i].rArg3.rWld);
          last = TRUE;
        } else
          last = FALSE;
        break;

      case 'O':
        if (!ObjectMaxReached(resetList[i].rArg1.rObj, resetList[i].rArg2.rNum)
          &&(!resetList[i].rIf || last)
          &&((resetList[i].rArg4.rNum<=0) || (ObjectPresent(resetList[i].rArg1.rObj, resetList[i].rArg3.rWld, OP_ALL) < resetList[i].rArg4.rNum))
        ) {
          object = ObjectCreate(resetList[i].rArg1.rObj, resetList[i].rArg3.rWld);
          last = TRUE;
        } else
          last = FALSE;
        break;

      case 'G':
        if ((mobile)
          &&(!resetList[i].rIf || last)
          &&!ObjectMaxReached(resetList[i].rArg1.rObj, resetList[i].rArg2.rNum)
        ) {
          ObjectCreate(resetList[i].rArg1.rObj, mobile);
          last = TRUE;
        } else
          last = FALSE;
        break;

      case 'P':
        if ((object)
          &&(!resetList[i].rIf || last)
          &&!ObjectMaxReached(resetList[i].rArg1.rObj, resetList[i].rArg2.rNum)
        ) {
          ObjectCreate(resetList[i].rArg1.rObj, object);
          last = TRUE;
        } else
          last = FALSE;
        break;

      case 'E':
        if ((mobile)
          &&(!resetList[i].rIf || last)
          &&!ObjectMaxReached(resetList[i].rArg1.rObj, resetList[i].rArg2.rNum)
        ) {
          object = ObjectCreate(resetList[i].rArg1.rObj, mobile);
          InvEquip(mobile, object, NULL);
          last = TRUE;
        } else
          last = FALSE;
        break;

      case 'D':
        if ((resetList[i].rArg1.rWld)
          &&(!resetList[i].rIf || last)
        ) {
          EXIT *exit;
          exit = ExitDir(Wld(resetList[i].rArg1.rWld)->wExit, resetList[i].rArg2.rNum);
          if (exit) {
            exit->eFlag = 0;
            BITSET(exit->eFlag, resetList[i].rArg3.rNum);
            exit = ExitReverse(resetList[i].rArg1.rWld, exit);
            if (exit) {
              exit->eFlag = 0;
              BITSET(exit->eFlag, resetList[i].rArg3.rNum);
            }
          }
        }
        break;

      case 'R':
        if ((resetList[i].rArg1.rWld)
          &&(!resetList[i].rIf || last)
        ) {
          THING *thing;
          thing = ThingFind(NULL, resetList[i].rArg2.rNum, resetList[i].rArg1.rWld, TF_OBJ, NULL);
          while (thing) {
            THINGFREE(thing);
            thing = ThingFind(NULL, resetList[i].rArg2.rNum, resetList[i].rArg1.rWld, TF_OBJ, NULL);
          }
        }
        break;

      case '#': /* disabled command */
      case '*': /* comment */
        break;

      default:
        /* do nothing */
        sprintf(buf, 
          "ResetReIndex(reset.c) %s, line#%ld Illegal command (memory corruption?!?)\n", 
          areaList[area].aFileName->sText,
          i + StrNumLine(areaList[area].aDesc) + 2);
        Log(LOG_ERROR, buf);
        resetList[i].rCmd = '#';
        break;
    }
  }
  CodeParseAfterReset(&areaList[area].aResetThing, &areaList[area].aResetThing);
  for (i=0; i<areaList[area].aWldIndex.iNum; i++) {
    CodeParseAfterReset(&areaList[area].aResetThing, areaList[area].aWldIndex.iThing[i]);
  }
}

void ResetAll(void) {
  LWORD i;

  Log(LOG_BOOT, "Resetting all Areas\n");
  for(i=0; i<areaListMax; i++)
    ResetArea(i);
}

/* Call frequently - will stagger resets so they dont all happen at once */
void ResetIdle(void) {
  LWORD  area;
  ULWORD minuteSinceReset;
  BYTE   buf[256];
  BYTE   word[256];

  for (area=0; area<areaListMax; area++) {
    if (BITANY(areaList[area].aResetFlag, RF_RESET_WHEN_EMPTY|RF_RESET_IMMEDIATELY)
        && (areaList[area].aResetDelay>0)
        && (BIT(areaList[area].aResetFlag, RF_RESET_IMMEDIATELY) || (!AreaHasPlayers(area)))) {
      minuteSinceReset = (time(0)-areaList[area].aResetLast-startTime)/60;
      if (minuteSinceReset > areaList[area].aResetDelay) {
        ResetArea(area);
        areaList[area].aResetLast=time(0) - startTime;
        sprintf(buf, "Resetting [%s]\n", StrFirstWord(areaList[area].aFileName->sText,word));
        Log(LOG_AREA, buf);
        return;
      }
    }
  }
}

