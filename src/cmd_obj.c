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
 *                     O B J    S T U F F                        *
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
#include "object.h"
#include "char.h"
#include "player.h"
#include "parse.h"
#include "cmd_wld.h"
#include "cmd_inv.h"
#include "cmd_obj.h"


OBJTEMPLATE *ObjGetObject(THING *thing, BYTE **cmd) {
  OBJTEMPLATE *object  = NULL;
  LWORD        virtual = -1;

  sscanf(*cmd, " %ld", &virtual);
  if (virtual>=0) {
    *cmd = StrOneWord(*cmd, NULL);
    object = ObjectOf(virtual);
    if (!object) {
      SendThing("Sorry, no OBJECTTEMPLATE with that virtual number exists\n", thing);
      return NULL;
    }
  }

  /* Check for authorization here */
  return object;
}


CMDPROC(CmdOStat) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE         buf[512];
  BYTE         truncateStr[512];
  EXTRA       *extra;
  PROPERTY    *property;
  LWORD        i;
  WORD         word;

  cmd = StrOneWord(cmd, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;

  /* first line, # Name & Type */
  sprintf(buf, "^g#:^G[^w%ld^G] ^gName:^G[^c", object->oVirtual);
  SendThing(buf, thing);
  SendThing(object->oSDesc->sText, thing);
  SendThing("^G] ^gType:^G[^c", thing);
  SendThing(TYPESPRINTF(buf, object->oType, oTypeList, 512), thing);
  SendThing("^G]\n^gKeywords:^G[^c", thing);
  SendThing(object->oKey->sText, thing);
  SendThing("^G]\n^gLDesc:\n^c", thing);
  SendThing(object->oLDesc->sText, thing);
  SendThing("\n^gDescription:^b\n", thing);
  SendThing(object->oDesc->sText, thing);

  SendThing("^gAct: ^G[^c", thing);
  SendThing(FlagSprintf(buf, object->oAct, oActList, ' ', 512), thing);
  SendThing("^G]\n^gWear: ^G[^c", thing);
  SendThing(TYPESPRINTF(buf, object->oWear, wearList, 512), thing);
  SendThing("^G]\n", thing);

  sprintf(buf, "^gOnline:^G[^c%ld^G] ", object->oOnline);
  SendThing(buf, thing);
  sprintf(buf, "^gOffline:^G[^c%ld^G] ", object->oOffline);
  SendThing(buf, thing);
  sprintf(buf, "^gWeight:^G[^c%hd^G] ", object->oWeight);
  SendThing(buf, thing);
  sprintf(buf, "^gValue:^G[^c%ld^G] ", object->oValue);
  SendThing(buf, thing);
  sprintf(buf, "^gRent:^G[^c%ld^G]\n", object->oRent);
  SendThing(buf, thing);

  SendThing("^gType-Specific Information\n", thing);
  for (i=0;
          i<OBJECT_MAX_FIELD
       && ObjectGetFieldStr(object->oType, &object->oDetail, i, buf, 512);
       i++) {
    SendThing(buf, thing);
  }

  for (i=0; i<OBJECT_MAX_APPLY; i++) {
    if (object->oApply[i].aType) {
      sprintf(buf,"^gApply%ld:^G[^c", i);
      SendThing(buf, thing);
      SendThing(TYPESPRINTF(buf, object->oApply[i].aType, applyList, 512), thing);
      word = object->oApply[i].aValue;
      sprintf(buf, "^G] ^gValue:^G[^c%hd^G]\n", word);
      SendThing(buf, thing);
    }
  }
  for (extra = object->oExtra; extra; extra=extra->eNext) {
    sprintf(buf,
            "^gExtra: ^c%-31s = ",
            StrTruncate(truncateStr,extra->eKey->sText,  30));
    SendThing(buf, thing);
    StrTruncate(truncateStr,extra->eDesc->sText, 30);
    StrOneLine(truncateStr);
    sprintf(buf, "%s\n", truncateStr);
    SendThing(buf, thing);
  }
  for (property = object->oProperty; property; property=property->pNext) {
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


CMDPROC(CmdOList) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE         truncateStr[256];
  BYTE         buf[256];
  LWORD        i;
  THING       *world;
  LWORD        area;

  world = WldGetWorld(thing, NULL);
  area = Wld(world)->wArea;

  /* List the rooms */
  for (i=0; i<areaList[area].aObjIndex.iNum; i++) {
    object = ObjTemplate(areaList[area].aObjIndex.iThing[i]);
    sprintf(buf, "^w%5ld ^c%-19s", object->oVirtual, StrTruncate(truncateStr,object->oSDesc->sText, 19));
    SendThing(buf, thing);
    if (i%3 == 2) {
      SendThing("\n", thing);
    } else
      SendThing(" ", thing);
  }
  if (i%3) /* one i++ after i%3==2 */
    SendThing("\n", thing); /* last line allways gets a return */
  sprintf(buf, "^g%ld ^bObject Templates listed.\n", i);
  SendThing(buf, thing);
}


CMDPROC(CmdOCreate) {
  WLD         *world;
  OBJTEMPLATE *object;
  LWORD        area;
  BYTE         buf[256];
  LWORD        i;
  INDEX       *index;

  cmd = StrOneWord(cmd, NULL);
  world = Wld(WldGetWorld(thing, &cmd));
  if (!world)
    return;
  area = world->wArea;
  if (areaList[area].aObjIndex.iNum == (areaList[area].aVirtualMax - areaList[area].aVirtualMin + 1)) {
    SendThing("^gBad news.... This Area is completely full of ObjTemplates allready\n", thing);
    return;
  }

  MEMALLOC(object, OBJTEMPLATE, OBJECT_ALLOC_SIZE);
  memset( (void*)object, 0, sizeof(OBJTEMPLATE)); /* init to zeros */

  object->oKey         = STRCREATE("blank object");
  object->oSDesc       = STRCREATE("Type ^wONAME^c to give this Obj a better name.");
  object->oLDesc       = STRCREATE("Type ^wOLDESC^c to give this Obj a better LDesc.");
  object->oDesc        = STRCREATE("Type ^wODESC^c to give this Obj a better Description.\n");
  object->oType        = OTYPE_ARMOR;

  /* update total num of MobTemplates in the game */
  objectNum++;

  /* give the MobTemplate the first available virtual number */
  object->oVirtual = areaList[area].aVirtualMin;
  index = &areaList[area].aObjIndex;
  i=0;
  while( (i<=index->iNum-1) && (object->oVirtual>=ObjTemplate(index->iThing[i])->oVirtual) ){
    if (object->oVirtual==ObjTemplate(index->iThing[i])->oVirtual)
      object->oVirtual++;
    else 
      i++;
  }
  /* insert it into index */
  IndexInsert(index, Thing(object), ObjectCompareProc);

  /* Goto the new room */
  BITSET(areaList[Wld(world)->wArea].aSystem, AS_OBJUNSAVED);
  sprintf(buf, "^bYou have just created ObjTemplate #^g[%5ld]\n", object->oVirtual);
  SendThing(buf, thing);
}


CMDPROC(CmdOClear) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE         buf[512];

  cmd = StrOneWord(cmd, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;

  /* first line, # Name & Type */
  sprintf(buf, "^gClearing field information for object#:^G[^w%ld^G] ^gName:^G[^c", object->oVirtual);
  SendThing(buf, thing);
  SendThing(object->oSDesc->sText, thing);
  SendThing("^G]\n", thing);
  object->oDetail.lValue[0]=0;
  object->oDetail.lValue[1]=0;
  object->oDetail.lValue[2]=0;
  object->oDetail.lValue[3]=0;
}

CMDPROC(CmdOName) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE   strName[256];

  cmd = StrOneWord(cmd, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;

  /* Edit the requisite string */
  SendHint("^;HINT: Names should not end on a blank line\n", thing);
  SendHint("^;HINT: The next thing you should do is use ^<OLDESC ^;to add a description\n", thing);
  sprintf(strName, "Object #%ld - Name", object->oVirtual);
  EDITSTR(thing, object->oSDesc, 256, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &areaList[AreaOf(object->oVirtual)].aSystem, AS_OBJUNSAVED);
}



CMDPROC(CmdOKey) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE   strName[256];

  cmd = StrOneWord(cmd, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;

  /* Edit the requisite string */
  SendThing("^;HINT: Keywords should take only a single line\n", thing);
  sprintf(strName, "Object #%ld - Keywords", object->oVirtual);
  EDITSTR(thing, object->oKey, 256, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &areaList[AreaOf(object->oVirtual)].aSystem, AS_OBJUNSAVED);
}



CMDPROC(CmdOLDesc) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE   strName[256];

  cmd = StrOneWord(cmd, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;

  /* Edit the requisite string */
  SendThing("^;HINT: The next thing you should do is use ^<ODESC ^;to add a description\n", thing);
  sprintf(strName, "Object #%ld - LDesc", object->oVirtual);
  EDITSTR(thing, object->oLDesc, 256, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &areaList[AreaOf(object->oVirtual)].aSystem, AS_OBJUNSAVED);
}



CMDPROC(CmdODesc) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE   strName[256];

  cmd = StrOneWord(cmd, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;

  /* Edit the requisite string */
  SendThing("^;HINT: Descriptions must end with a blank line\n", thing);
  sprintf(strName, "Object #%ld - Detailed Description", object->oVirtual);
  EDITSTR(thing, object->oDesc, 4096, strName, EP_ENDLF);
  EDITFLAG(thing, &areaList[AreaOf(object->oVirtual)].aSystem, AS_OBJUNSAVED);
}

CMDPROC(CmdOExtra) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE         strName[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd)
    EditExtra(thing, "OEXTRA <#>", "", NULL, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;

  sprintf(strName, "^cObject ^w#%ld^c/", object->oVirtual);
  if (EditExtra(thing, "OEXTRA <#>", cmd, strName, &object->oExtra))
    EDITFLAG(thing, &areaList[AreaOf(object->oVirtual)].aSystem, AS_OBJUNSAVED);
}

CMDPROC(CmdOProperty) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE         strName[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd)
    EditProperty(thing, "OPROPERTY <#>", "", NULL, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;

  sprintf(strName, "^cObject ^w#%ld^c/", object->oVirtual);
  if (EditProperty(thing, "OPROPERTY <#>", cmd, strName, &object->oProperty))
    EDITFLAG(thing, &areaList[AreaOf(object->oVirtual)].aSystem, AS_OBJUNSAVED);
}

CMDPROC(CmdOSet) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE         buf[512];
  BYTE         newCmd[512];
  LWORD        i;
  OBJTEMPLATE  objTemplate;
  OBJTEMPLATE *object;

  SETLIST setList[] = {
    { "TYPE",         SET_TYPE(    objTemplate, objTemplate.oType, oTypeList )  },
    { "ACT",          SET_FLAG(    objTemplate, objTemplate.oAct,  oActList )   },
    { "WEAR",         SET_TYPE(    objTemplate, objTemplate.oWear, wearList )   },
    { "VALUE",        SET_NUMERIC( objTemplate, objTemplate.oValue )            },
    { "RENT",         SET_NUMERIC( objTemplate, objTemplate.oRent )             },
    { "WEIGHT",       SET_NUMERIC( objTemplate, objTemplate.oWeight )           },

    { "APPLY0TYPE",   SET_TYPE( objTemplate, objTemplate.oApply[0].aType, applyList ) },
    { "APPLY1TYPE",   SET_TYPE( objTemplate, objTemplate.oApply[1].aType, applyList ) },
    { "APPLY2TYPE",   SET_TYPE( objTemplate, objTemplate.oApply[2].aType, applyList ) },
    { "APPLY3TYPE",   SET_TYPE( objTemplate, objTemplate.oApply[3].aType, applyList ) },

    { "APPLY0VALUE",  SET_NUMERIC( objTemplate, objTemplate.oApply[0].aValue )  },
    { "APPLY1VALUE",  SET_NUMERIC( objTemplate, objTemplate.oApply[1].aValue )  },
    { "APPLY2VALUE",  SET_NUMERIC( objTemplate, objTemplate.oApply[2].aValue )  },
    { "APPLY3VALUE",  SET_NUMERIC( objTemplate, objTemplate.oApply[3].aValue )  },

    { "%MINSTR",      SET_PROPERTYINT( objTemplate, objTemplate.oProperty ) },
    { "%MINDEX",      SET_PROPERTYINT( objTemplate, objTemplate.oProperty ) },
    { "%MINCON",      SET_PROPERTYINT( objTemplate, objTemplate.oProperty ) },
    { "%MINWIS",      SET_PROPERTYINT( objTemplate, objTemplate.oProperty ) },
    { "%MININT",      SET_PROPERTYINT( objTemplate, objTemplate.oProperty ) },
    { "%MINLEVEL",    SET_PROPERTYINT( objTemplate, objTemplate.oProperty ) },
    { "%SECURITY",    SET_PROPERTYINT( objTemplate, objTemplate.oProperty ) },

    { "" }
  };

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) { /* if they didnt type anything, give 'em a list */
    SendThing("^pUsuage:\n^P=-=-=-=\n", thing);
    SendThing("^cOSET <Obj#> <stat> <value>\n", thing);
    EditSet(thing, cmd, NULL, NULL, setList);
    return;
  }

  object = ObjGetObject(thing, &cmd);
  if (!object) {
    SendThing("^bThere doesnt appear to be an Object with that virtual number\n", thing);
    return;
  }

  if (!*cmd) {
    SendThing("^wType ^rOSET ^wwith no arguments for a list of changeable stats\n\n", thing);
    sprintf(buf, "ostat %ld", object->oVirtual);
    CmdOStat(thing, buf);
    return;
  }

  i = EditSet(thing, cmd, object, object->oSDesc->sText, setList);
  if (i == -1) {
    sprintf(newCmd, "osetfield %ld %s", object->oVirtual, cmd);
    CmdOSetField(thing, newCmd);
  } else if (i == 1)
    BITSET(areaList[AreaOf(object->oVirtual)].aSystem, AS_OBJUNSAVED);
}


CMDPROC(CmdOSetField) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  BYTE         buf[512];
  LWORD        i;
  WORD         fieldNum;

  cmd = StrOneWord(cmd, NULL);
  object = ObjGetObject(thing, &cmd);
  if (object) {
    cmd = StrOneWord(cmd, buf);
    fieldNum = ObjectGetFieldNumber(buf, object->oType);
  }

  if (!object || fieldNum==-1) { /* basic help */
    if (object) {
      sprintf(buf, "ostat %ld", object->oVirtual);
      CmdOStat(thing, buf);
      SendThing("\n^gType-Specific Information\n", thing);
      for (i=0;
              i<OBJECT_MAX_FIELD
           && ObjectGetFieldStr(object->oType, &object->oDetail, i, buf, 512);
           i++) 
      {
        SendThing(buf, thing);
      }
    } else {
      SendThing("^GUSAGE: ^gOSETFIELD <OBJECT#> <FIELD> [<VALUE>]\n", thing);
      SendThing("^CE.G.    ^cosetfield 3000 cflag\n", thing);
      SendThing("^CE.G.    ^cosetfield 3000 cflag locked\n", thing);
      SendThing("^CE.G.    ^cosetfield 3000 dambonus 4\n", thing);
      SendThing("^CE.G.    ^cosetfield 3000 1 3\n", thing);
    }
    return;
  }

  if (ObjectSetFieldStr(thing, object->oType, &object->oDetail, fieldNum, cmd)) {

    /* let 'em change something - then show them results */
    SendThing("^gType-Specific Information\n", thing);
    for (i=0;
            i<OBJECT_MAX_FIELD
         && ObjectGetFieldStr(object->oType, &object->oDetail, i, buf, 512);
         i++) 
    {
      SendThing(buf, thing);
    }
    BITSET(areaList[AreaOf(object->oVirtual)].aSystem, AS_OBJUNSAVED);
  }
}


CMDPROC(CmdOCompile) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  OBJTEMPLATE *object;
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
    for (i=0; i<areaList[area].aObjIndex.iNum; i++) {
      printLine=0;
      for (property = ObjTemplate(areaList[area].aObjIndex.iThing[i])->oProperty; property; property=property->pNext) {
        if (property->pKey->sText[0]=='@') {
          if (!printLine) {
            printLine=1;
            sprintf(buf, 
              "^yOCOMPILE: ^c#%ld - ^b%s ", 
              ObjTemplate(areaList[area].aObjIndex.iThing[i])->oVirtual, 
              ObjTemplate(areaList[area].aObjIndex.iThing[i])->oSDesc->sText);

            SendThing(buf, thing);
          }
          sprintf(buf,"^G(%s) ",property->pKey->sText);
          SendThing(buf,thing);
          CodeCompileProperty(property,thing);
        }
      }
      if (printLine)
        SendThing("\n",thing);
    }
    return;
  }

  object = ObjGetObject(thing, &cmd);
  if (!object) {
    SendThing("^GUSUAGE: ^gOCOMPILE <Obj Template #>\n", thing);
    SendThing("^G  or\n", thing);
    SendThing("^GUSUAGE: ^gOCOMPILE all\n", thing);
    return;
  }

  sprintf(buf, "^yCOMPILE: ^c#%ld - ^b%s\n", object->oVirtual, object->oSDesc->sText);
  SendThing(buf, thing);
  
  for (property = object->oProperty; property; property=property->pNext) {
    SendThing("^gProp.: ^c", thing);
    SendThing(property->pKey->sText, thing);
    SendThing("\n", thing);
    if (property->pKey->sText[0]=='@') {
      CodeCompileProperty(property,thing);
    }
  }
  
}


CMDPROC(CmdODecomp) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  OBJTEMPLATE *object;
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
    for (i=0; i<areaList[area].aObjIndex.iNum; i++) {
      printLine=0;
      for (property = ObjTemplate(areaList[area].aObjIndex.iThing[i])->oProperty; property; property=property->pNext) {
        if (property->pKey->sText[0]=='@') {
          if (!printLine) {
            printLine=1;
            sprintf(buf, 
              "^yODECOMP: ^c#%ld - ^b%s ", 
              ObjTemplate(areaList[area].aObjIndex.iThing[i])->oVirtual, 
              ObjTemplate(areaList[area].aObjIndex.iThing[i])->oSDesc->sText);

            SendThing(buf, thing);
          }
          sprintf(buf,"^G(%s) ",property->pKey->sText);
          SendThing(buf,thing);
          CodeDecompProperty(property,thing);
        }
      }
      if (printLine)
        SendThing("\n",thing);
    }
    return;
  }

  object = ObjGetObject(thing, &cmd);
  if (!object) {
    SendThing("^GUSUAGE: ^gODECOMP <Obj Template #>\n", thing);
    SendThing("^G  or\n", thing);
    SendThing("^GUSUAGE: ^gODECOMP all\n", thing);
    return;
  }

  sprintf(buf, "^yODECOMP: ^c#%ld - ^b%s\n", object->oVirtual, object->oSDesc->sText);
  SendThing(buf, thing);
  
  for (property = object->oProperty; property; property=property->pNext) {
    SendThing("^gProp.: ^c", thing);
    SendThing(property->pKey->sText, thing);
    SendThing("\n", thing);
    if (property->pKey->sText[0]=='@') {
      CodeDecompProperty(property, thing);
    }
  }
  
}


CMDPROC(CmdOLoad) { /* void CmdProc(THING thing, BYTE* cmd) */
  OBJTEMPLATE *object;
  THING       *create;

  cmd = StrOneWord(cmd, NULL);
  object = ObjGetObject(thing, &cmd);
  if (!object)
    return;
  /* Make what they wish for... */
  create = ObjectCreate(object, Base(thing)->bInside);
  SendAction("^wYou create $N\n", 
             thing, create, SEND_SRC |SEND_VISIBLE|SEND_CAPFIRST);
  SendAction("^w$n creates $N\n", 
             thing, create, SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
}

CMDPROC(CmdOSave) { /* void CmdProc(THING thing, BYTE* cmd) */
  THING *world;
  BYTE   buf[256];
  WORD   area;
  SOCK  *sock;

  world = WldGetWorld(thing, &cmd);
  if (!world)
    return;
  area = Wld(world)->wArea;

  /* check editor status */

  if (!BIT(areaList[Wld(world)->wArea].aSystem, AS_OBJUNSAVED)) {
    SendThing("There are no unsaved changes!\n", thing);
  } else {
    BITFLIP(areaList[area].aSystem, AS_OBJUNSAVED);
    ObjectWrite(area);
    sock = BaseControlFind(thing);
    sprintf(buf, "%s.obj saved by %s. (%ld objects)\n", areaList[area].aFileName->sText, sock->sHomeThing->tSDesc->sText, areaList[area].aObjIndex.iNum);
    Log(LOG_AREA, buf);
    sprintf(buf, "%s.obj saved. (^w%ld objects^V)\n", areaList[area].aFileName->sText, areaList[area].aObjIndex.iNum);
    SendThing(buf, thing);
  }
}

