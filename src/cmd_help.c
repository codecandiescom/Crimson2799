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
#include "mem.h"
#include "log.h"
#include "queue.h"
#include "str.h"
#include "parse.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "send.h"
#include "base.h"
#include "exit.h"
#include "world.h"
#include "help.h"
#include "cmd_help.h"


/* List Help sections/entries */
CMDPROC(CmdHList) { /*(THING *thing, BYTE *cmd)*/ 
  LWORD section;
  LWORD i;
  LWORD numShown;
  BYTE  buf[256];
  BYTE  truncateStr[256];
  LWORD totalShown;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^p    Help Sections                    Entries Editors\n", thing);
    SendThing("^P    =-=-=-=-=-=-=                    =-=-=-= =-=-=-=\n", thing);
    totalShown=0;
    for (i=0; i<helpListMax; i++) {
      if (BIT(helpList[i].hFlag, H_GODONLY) 
          && !ParseCommandCheck(PARSE_COMMAND_WGOTO,BaseControlFind(thing),"")) {
	  continue;
      }
      totalShown++;
      sprintf(buf, 
              "^r%2ld. ^G[^g%-30s^G] [^g%5hd^G] [^g%-10s^G] ", 
              i,
              helpList[i].hName->sText, 
              helpList[i].hNum, 
              helpList[i].hEditor->sText);
      SendThing(buf, thing);
      if (BIT(helpList[i].hFlag, H_UNSAVED))
        SendThing("^r<UNSAVED> ", thing);
      if (BIT(helpList[i].hFlag, H_GODONLY))
        SendThing("^r<GOD ONLY> ", thing);
      SendThing("\n", thing);
    }
    sprintf(buf, "^pTotal Help Entries: ^g%ld\n", totalShown);
    SendThing(buf, thing);
    SendHint("^;HINT: Try ^wHLIST ^c<section> for a list of entries\n", thing);
    return;
  }

  /* are they looking for a particular section */
  section = -1;
  sscanf(cmd, "%ld", &section); /* see if they entered a number */
  if (section == -1)
    for (i=0; i<helpListMax; i++)
      if (StrAbbrev(helpList[i].hName->sText,cmd)) {
        section = i;
        break;
      }

  /* List a section for them */
  if ((section != -1)&&(section<helpListMax)) {
    if (BITANY(helpList[section].hFlag, H_GODONLY) 
      && !ParseCommandCheck(PARSE_COMMAND_WGOTO,BaseControlFind(thing),"")) {
      SendThing("^wUnfortunately that help topic is reserved for ^rGOD EYES ONLY^w.\n",thing);
      return;
    }
    for (i=0, numShown=0; i<helpIndex.iNum; i++)
      if (Help(helpIndex.iThing[i])->hSection == section) {
        sprintf(buf, "%-26s", StrTruncate(truncateStr, Help(helpIndex.iThing[i])->hKey->sText, 25));
        SendThing(buf, thing);
        if (numShown%3 == 2)
          SendThing("\n", thing);
        numShown++;
      }
      if (numShown%3)
        SendThing("\n", thing);
    return;
  } else {
    SendThing("^rUnknown help section\n", thing);
  }

}

/* Create a new keyword in the given section */
CMDPROC(CmdHCreate) { /*(THING *thing, BYTE *cmd)*/ 
  HELP *help;
  BYTE  word[256];
  BYTE  sectionBuf[256];
  BYTE  strName[256];
  LWORD section;
  LWORD i;

  /* rip apart arguments */
  cmd = StrOneWord(cmd, NULL);
  cmd = StrOneWord(cmd, sectionBuf);
  StrOneWord(cmd, word);

  if (!*cmd || !*sectionBuf || !*word) {
    SendThing("^rTry HCREATE <SECTION> <KEYWORD> [<KEYWORD>...]\n", thing);
    return;
  }

  section = -1;
  sscanf(sectionBuf, "%ld", &section); /* see if they entered a number */
  if (section == -1)
    for (i=0; i<helpListMax; i++)
      if (StrAbbrev(helpList[i].hName->sText,sectionBuf)) {
        section = i;
        break;
      }

  if ((section != -1)&&(section<helpListMax)) {
    help = HelpFind(word);
    if (help) {
      SendThing("^rA help entry matching that keyword allready exists\n", thing);
      return;
    }

    MEMALLOC(help, HELP, HELP_ALLOC_SIZE);
    help->hSection = section;
    help->hKey = STRCREATE(cmd);
    help->hDesc = STRCREATE("");
    BITSET(helpList[help->hSection].hFlag, H_UNSAVED);
    sprintf(strName, "HelpEntry [%s - %s] - Description", helpList[help->hSection].hName->sText, help->hKey->sText);
    EDITSTR(thing, help->hDesc, 4096, strName, EP_ENDLF);
    EDITFLAG(thing, &helpList[help->hSection].hFlag, H_UNSAVED);
    IndexInsert(&helpIndex, help, HelpCompareProc);
    if (indexError) {
      SendThing("^rA help entry matching that keyword allready exists.\n -- this is a bug. Please notify the sysadmin.\n", thing);
      STRFREE(help->hKey);
      STRFREE(help->hDesc);
      MEMFREE(help,HELP);
      return;
    }
    helpList[section].hNum++;
  } else {
    SendThing("^rUnknown help section\n", thing);
  }
}

/* Edit the keyword list of an existing keyword */
CMDPROC(CmdHKey) { /*(THING *thing, BYTE *cmd)*/ 
  HELP *help;
  BYTE  buf[256];
  BYTE  strName[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry HKEY <KEYWORD>\n", thing);
    return;
  }
  help = HelpFind(cmd);
  if (!help) {
    SendThing("^rNo help entry matching that keyword found\n", thing);
    return;
  }

  sprintf(buf, "^bMatch found in Section [^c%-18s^b]\n", helpList[help->hSection].hName->sText);
  SendThing(buf, thing);
  sprintf(buf, "^bKeys: [^c%s]\n", help->hKey->sText);
  SendThing(buf, thing);
  SendThing(help->hDesc->sText, thing);

  BITSET(helpList[help->hSection].hFlag, H_UNSAVED);
  sprintf(strName, 
          "HelpEntry in Section [%s] - Keywords", 
          helpList[help->hSection].hName->sText);
  EDITSTR(thing, help->hKey, 4096, strName, EP_ONELINE|EP_ENDNOLF);
  EDITFLAG(thing, &helpList[help->hSection].hFlag, H_UNSAVED);
  /* it would be nice to check the new keys don't match ones already in existance, 
   * and to also resort the index, but I don't know how to do either easily --corbin */
  /* So for now I leave the resorting for the next reboot, and as for duplicate
   * keys... well, the server will probably puke next reboot if we've done that. */
}

/* Edit the entry for a given keyword */
CMDPROC(CmdHDesc) { /*(THING *thing, BYTE *cmd)*/ 
  HELP *help;
  BYTE  buf[256];
  BYTE  strName[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry HDESC <KEYWORD>\n", thing);
    return;
  }
  help = HelpFind(cmd);
  if (!help) {
    SendThing("^rNo help entry matching that keyword found\n", thing);
    return;
  }

  sprintf(buf, "^bMatch found in Section [^c%-18s^b]\n", helpList[help->hSection].hName->sText);
  SendThing(buf, thing);
  sprintf(buf, "^bKeys: [^c%s]\n", help->hKey->sText);
  SendThing(buf, thing);
  SendThing(help->hDesc->sText, thing);

  BITSET(helpList[help->hSection].hFlag, H_UNSAVED);
  sprintf(strName, "HelpEntry [%s - %s] - Description", helpList[help->hSection].hName->sText, help->hKey->sText);
  EDITSTR(thing, help->hDesc, 4096, strName, EP_ENDLF);
  EDITFLAG(thing, &helpList[help->hSection].hFlag, H_UNSAVED);
}

/* Delete a keyword in the given section */
CMDPROC(CmdHDelete) { /*(THING *thing, BYTE *cmd)*/ 
  HELP *help;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^rTry HDELETE <KEYWORD>\n", thing);
    return;
  }
  help = HelpFind(cmd);
  if (!help) {
    SendThing("^rNo help entry matching that keyword found\n", thing);
    return;
  }
  HELPFREE(help);
}

/* Save the section */
CMDPROC(CmdHSave) { /*(THING *thing, BYTE *cmd)*/ 
  LWORD section;
  LWORD i;
  BYTE  buf[256];

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^cTry ^wHSAVE ^c<section> to save the entries in a help section\n", thing);
    return;
  }

  section = -1;
  sscanf(cmd, "%ld", &section); /* see if they entered a number */
  if (section == -1)
    for (i=0; i<helpListMax; i++)
      if (StrAbbrev(helpList[i].hName->sText,cmd)) {
        section = i;
        break;
      }

  if ((section != -1)&&(section<helpListMax)) {
    if (HelpWrite(section)) {
      sprintf(buf, "^bSaving Help Section [^c%-18s^b]\n", helpList[section].hName->sText);
      SendThing(buf, thing);
    } else {
      sprintf(buf, "^rERROR Unable to save Help Section [^c%-18s^b]\n", helpList[section].hName->sText);
      SendThing(buf, thing);
    }
  } else {
    SendThing("^rUnknown help section\n", thing);
  }
}
