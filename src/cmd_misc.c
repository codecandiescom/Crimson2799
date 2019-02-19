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

/* Inventory related commands */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "crimson2.h"
#include "macro.h"
#include "queue.h"
#include "log.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "file.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "alias.h"
#include "area.h"
#include "exit.h"
#include "world.h"
#include "send.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "mobile.h"
#include "fight.h"
#include "affect.h"
#include "effect.h"
#include "player.h"
#include "skill.h"
#include "parse.h"
#include "cmd_god.h"
#include "cmd_misc.h"

COLORPREFLIST colorPrefList[] = {
  { "Name:",           '0', 0 },
  { "Description:",    '1', 0 },
  { "LDesc:",          '2', 0 },
  { "Info-text:",      '3', 0 },
  { "Info-filler:",    '4', 0 },
  { "Info-highlight:", '5', 0 },
  { "Speech:",         '6', 0 },
  { "Fight-Hit:",      '7', 0 },
  { "Fight-Damage:",   '8', 0 },
  { "Fight-Others:",   '9', 0 },

  { "Channels:",       ':', 1 },
  { "Hint-Text:",      ';', 1 },
  { "Hint-Highlight:", '<', 1 },
  { "", '=', 0 },
  { "", '>', 0 },
  { "", '?', 0 },
  { "", '@', 0 },
};



CMDPROC(CmdTitle) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE buf[512];
  BYTE *i;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  i = StrFind(cmd, "$$n");
  while (i && i[2]=='N') i=StrFind(i+1, "$$n");
  if (i) {
    /* convert $$n into $n */
    while( *(i+1) ) {
      *i = *(i+1);
      i++;
    }
    *i = '\0'; /* copy null too */
    sprintf(buf,   "%s", cmd);
  } else {
    sprintf(buf,   "$n %s", cmd);
  }

  SendThing("^GOkay Doke, you are now:\n^g", thing);
  STRFREE(Base(thing)->bLDesc);
  Base(thing)->bLDesc = STRCREATE(buf); /* name & title */
  SendAction(Base(thing)->bLDesc->sText, thing, NULL, SEND_SRC);
  SendThing("\n", thing);
}

/* at some point you should be able to who names to skip the titles */
/* also who gods */
CMDPROC(CmdWho) { /* Cmd(THING *thing, BYTE *cmd) */
  SOCK *sock;
  SOCK *s;
  BYTE  buf[256];
  LWORD i = 0;

  sock = BaseControlFind(thing);
  if (!sock) return;
  SendThing("^pWho's on with you:\n", thing);
  SendThing("^P-----------------\n", thing);
  for (s = sockList; s; s=s->sNext) {
    if (s->sHomeThing) { /* block out those halfway thru signon */
      i++;
      if (Character(s->sHomeThing)->cLevel>=LEVEL_IMMORTAL) {
        if (Character(s->sHomeThing)->cLevel>=LEVEL_CODER)
          SendThing("^WIMP ^w", thing);
        else if (Character(s->sHomeThing)->cLevel>=LEVEL_ADMIN)
          SendThing("^WADM ^w", thing);
        else if (Character(s->sHomeThing)->cLevel>=LEVEL_GOD)
          SendThing("^WGOD ^p", thing);
        else 
          SendThing("^WIMM ^r", thing);
      } else
        SendThing("^W    ^g", thing);
      /* Should use ThingShow so they can see flags */
      SendAction(Base(s->sHomeThing)->bLDesc->sText, s->sHomeThing, thing, 
        SEND_DST|SEND_CAPFIRST|SEND_SELF);
      if (Plr(s->sHomeThing)->pIdleTick > playerIdleTick) {
        sprintf(buf, " <IDLE %ldM>", Plr(s->sHomeThing)->pIdleTick);
        SendThing(buf, thing);
      }
      if (BIT(Plr(s->sHomeThing)->pAuto, PA_AFK))
        SendThing(" <AFK>", thing);
      /* Show if they are signing on */
      if (s->sMode != MODE_PLAY) {
        if (Character(sock->sHomeThing)->cLevel >= LEVEL_GOD) {
          SendThing(" <", thing);
          SendThing(sModeList[s->sMode], thing);
          SendThing(">", thing);
        } else
          SendThing(" <SIGNING ON>", thing);
      }
    } else
      SendThing("<New Connect in Progress>", thing);
    SendThing("^V\n", thing);
  }
  sprintf(buf, "^w%ld ^pPeople total\n", i);
  SendThing(buf, thing);
}

CMDPROC(CmdWhere) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE   srcKey[256];
  LWORD  srcNum;
  LWORD  srcOffset;
  THING *found;
  SOCK  *sock;
  THING *homeThing;
  BYTE   buf[256];
  BYTE   truncateStr[256];
  LWORD  i = 0;
  LWORD  area;
 
  sock = BaseControlFind(thing);
  if (!sock) return;
  homeThing = sock->sHomeThing;

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */

  SendThing("^pWhere on earth is everybody:\n", thing);
  SendThing("^P---------------------------\n", thing);

  if (!Base(thing)->bInside || Base(thing)->bInside->tType != TTYPE_WLD)
    return;
  area = Wld(Base(thing)->bInside)->wArea;


  /* Show where all the players are */
  if (!*cmd) {
    for (sock = sockList; sock; sock=sock->sNext) {
      if (sock->sHomeThing 
          && Base(sock->sHomeThing)->bInside
          && Base(sock->sHomeThing)->bInside->tType == TTYPE_WLD
          && ( Wld(Base(sock->sHomeThing)->bInside)->wArea == area 
              ||Character(homeThing)->cLevel >= LEVEL_GOD )) { /* block out those halfway thru signon */
        i++;
        /* show the person */
        if (i%2)
          SendThing("  ^r", thing);
        else
          SendThing("  ^R", thing);
        sprintf(buf, "%-25s - ", StrTruncate(truncateStr, sock->sHomeThing->tSDesc->sText, 25));
        SendThing(buf, thing);
        /* show the location */
        if (i%2)
          SendThing("^c", thing);
        else
          SendThing("^C", thing);
        SendThing(Base(sock->sHomeThing)->bInside->tSDesc->sText, thing);
 
        /* Gods get to see the area */             
        if ( Character(homeThing)->cLevel >= LEVEL_GOD ) { 
          LWORD area;
          BYTE  word[256];
          BYTE  truncateStr[256];
 
          /* Find area name */
          area = Wld( Base(sock->sHomeThing)->bInside )->wArea;
          StrFirstWord(areaList[area].aFileName->sText,word);
          StrTruncate(truncateStr, word, 25);
          
          /* send it */
          SendThing(" (", thing);
          SendThing( truncateStr, thing);
          SendThing(")", thing);

          sprintf(word, " [%ld]", Wld( Base(sock->sHomeThing)->bInside )->wVirtual);
          SendThing(word, thing);
        }

        SendThing("\n", thing);
      }
    }
    sprintf(buf, "^w%ld ^pPeople total\n", i);
    SendThing(buf, thing);
    return;
  }

  /* show matching mobs and plrs in the same area */
  cmd = ParseFind(cmd, srcKey, &srcOffset, &srcNum, NULL, NULL);

  found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_MOB|TF_PLR|TF_MOB_WLD|TF_PLR_WLD, &srcOffset);
  if (!found) {
    SendThing("^wWhere's who?, Never heard of 'em\n", thing);
    return;
  } 

  while (found && srcNum!=0 && i<50) {
    if (found
        && Base(found)->bInside
        && Base(found)->bInside->tType == TTYPE_WLD
        && ( Wld(Base(found)->bInside)->wArea == area 
            ||Character(homeThing)->cLevel >= LEVEL_GOD )) { /* block out those halfway thru signon */
      i++;
      /* show the person */
      if (i%2)
        SendThing("  ^r", thing);
      else
        SendThing("  ^R", thing);
      sprintf(buf, "%-25s - ", StrTruncate(truncateStr, found->tSDesc->sText, 25));
      SendThing(buf, thing);
      /* show the location */
      if (i%2)
        SendThing("^c", thing);
      else
        SendThing("^C", thing);
      SendThing(Base(found)->bInside->tSDesc->sText, thing);
      SendThing("\n", thing);
    } else {
      if (srcNum>=0) srcNum++;
    }
    found = ThingFind(srcKey, -1, Base(thing)->bInside, TF_MOB|TF_PLR|TF_MOB_WLD|TF_PLR_WLD|TF_CONTINUE, &srcOffset);
  }
  
}


CMDPROC(CmdSave) { /* Cmd(THING *thing, BYTE *cmd) */
  SOCK *sock;

  sock = BaseControlFind(thing);
  SendThing("^wSaving your Player Data\n", thing);
  PlayerWrite(sock->sHomeThing, PWRITE_CRASH);
}



/******************* CmdSet - Start ************************/

/* The following is all to do with the Set command, it's
   kind of complicated but it allows nicely formatted output and
   easy addition of arbitrary columns and rows */

#define SV_TITLE    0 /* title no flag */
#define SV_SOCKPREF 1 /* socket preference flag */
#define SV_AUTO     2 /* autoaction preference */
#define SV_SYSTEM   3 /* Player system */

FLAG *SetFlag(SOCK *sock, LWORD flagNum) {
  if (!sock) return NULL;
  switch (flagNum) {
  case SV_SOCKPREF:
    return &(sock->sPref);
  case SV_SYSTEM:
    return &(Plr(sock->sHomeThing)->pSystem);
  case SV_AUTO:
    return &(Plr(sock->sHomeThing)->pAuto);
  default:
    return NULL;
  }
}


  struct SetType {
    BYTE  *sName;
    LWORD  sColumn;
    LWORD  sVariable;
    LWORD  sFlag;
    BYTE  *sCmd; /* priviledge based on this cmd # */
  };

  #define MAX_COLUMN 4

  struct SetType expertSetList[] = {
    { "SOCKET:",    0, SV_TITLE,    0,              NULL    },
    { "Car.Ret.",   0, SV_SOCKPREF, SP_CR,          NULL    },
    { "LineFeed",   0, SV_SOCKPREF, SP_LF,          NULL    },
    { "Continue",   0, SV_SOCKPREF, SP_CONTINUE,    NULL    },
    { "TelnetGA",   0, SV_SOCKPREF, SP_TELNETGA,    NULL    },
    { "Ansi",       0, SV_SOCKPREF, SP_ANSI,        NULL    },
    { "Compact",    0, SV_SOCKPREF, SP_COMPACT,     NULL    },
    { " "       ,   0, SV_TITLE,    0,              NULL    }, /* spacer must be " " */
    { "ScreenSize", 0, SV_TITLE,    SP_COMPACT,     NULL    },
    { " "       ,   0, SV_TITLE,    0,              NULL    }, /* spacer must be " " */
    /* so people see the words */
    { "INFO:",      0, SV_TITLE,    0,              NULL    }, /* spacer must be " " */
    { "^3Description",0, SV_TITLE,  0,              NULL,   }, 
    { "^3Email",    0, SV_TITLE,    0,              NULL,   }, 
    { "^3Plan",     0, SV_TITLE,    0,              NULL,   }, 

    { "CHANNELS:",  1, SV_TITLE,    0,              NULL    },
    { "Private",    1, SV_SOCKPREF, SP_CHANPRIVATE, NULL    },
    { "Group",      1, SV_SOCKPREF, SP_CHANGROUP,   NULL    },
    { "Gossip",     1, SV_SOCKPREF, SP_CHANGOSSIP,  NULL    },
    { "Auction",    1, SV_SOCKPREF, SP_CHANAUCTION, NULL    },
    { "Usage",      1, SV_SOCKPREF, SP_CHANUSAGE,   NULL    },
    { "Tick",       1, SV_SOCKPREF, SP_CHANTICK,    NULL    },
    { "Port",       1, SV_SOCKPREF, SP_CHANPORT,    "stat"  },
    { "Error",      1, SV_SOCKPREF, SP_CHANERROR,   "stat"  },
    { "God",        1, SV_SOCKPREF, SP_CHANGOD,     "stat"  },
    { "Area",       1, SV_SOCKPREF, SP_CHANAREA,    "astat" },
/*    { "Spy",        1, SV_SOCKPREF, SP_CHANSPY,     "stat"  },*/
    { "Board",      1, SV_SOCKPREF, SP_CHANBOARD,   "blist" },

    { "PROMPT:",    2, SV_TITLE,    0,              NULL    },
    { "Hit Pts",    2, SV_SOCKPREF, SP_PROMPTHP,    NULL    },
    { "Move Pts",   2, SV_SOCKPREF, SP_PROMPTMV,    NULL    },
    { "Power Pts",  2, SV_SOCKPREF, SP_PROMPTPP,    NULL    },
    { "Room #'s",   2, SV_SOCKPREF, SP_PROMPTRM,    "wgoto" },
    { "Exits",      2, SV_SOCKPREF, SP_PROMPTEX,    NULL    },
    { " "       ,   2, SV_TITLE   , 0,              NULL    }, /* spacer must be " " */
    { "PREFS:",     2, SV_TITLE,    0,              NULL    },
    { "NoEmail",    2, SV_AUTO,     PA_NOEMAIL,     NULL    },
    { "Hint",       2, SV_AUTO,     PA_HINT,        NULL    },
    { "Expert",     2, SV_AUTO,     PA_EXPERT,      NULL    },
    { "NoHassle",   2, SV_SYSTEM,   PS_NOHASSLE,    "mload" },
    { "AFK",        2, SV_AUTO,     PA_AFK,         NULL    },

    { "AUTO:",      3, SV_TITLE,    0,              NULL    },
    { "AutoLook",   3, SV_AUTO,     PA_AUTOLOOK,    NULL    },
    { "AutoExit",   3, SV_AUTO,     PA_AUTOEXIT,    NULL    },
    { "AutoLoot",   3, SV_AUTO,     PA_AUTOLOOT,    NULL    },
    { "AutoAssist", 3, SV_AUTO,     PA_AUTOASSIST,  NULL    },
    { "AutoAggr",   3, SV_AUTO,     PA_AUTOAGGR,    NULL    },
    { "AutoRescue", 3, SV_AUTO,     PA_AUTORESCUE,  NULL    },
    { "AutoJunk",   3, SV_AUTO,     PA_AUTOJUNK,    NULL    },
    { "AutoFlee",   3, SV_AUTO,     PA_AUTOFLEE,    NULL    },
    { "AutoEat",    3, SV_AUTO,     PA_AUTOEAT,     NULL    },
    { "AutoDrink",  3, SV_AUTO,     PA_AUTODRINK,   NULL    },
    { "Consider",   3, SV_AUTO,     PA_CONSIDER,    NULL    },

    { ""        ,  -1, SV_TITLE,    0,              NULL    }
  };

  struct SetType newbieSetList[] = {
    { "SOCKET:",    0, SV_TITLE,    0,              NULL    },
    { "Ansi",       0, SV_SOCKPREF, SP_ANSI,        NULL    },
    { " "       ,   0, SV_TITLE,    0,              NULL    }, /* spacer must be " " */
    /* so people see the words */
    { "INFO:",      0, SV_TITLE,    0,              NULL    }, /* spacer must be " " */
    { "^3Description",0, SV_TITLE,  0,              NULL,   }, 
    { "^3Email",    0, SV_TITLE,    0,              NULL,   }, 
    { "^3Plan",     0, SV_TITLE,    0,              NULL,   }, 

    { "CHANNELS:",  1, SV_TITLE,    0,              NULL    },
    { "Private",    1, SV_SOCKPREF, SP_CHANPRIVATE, NULL    },
    { "Group",      1, SV_SOCKPREF, SP_CHANGROUP,   NULL    },
    { "Gossip",     1, SV_SOCKPREF, SP_CHANGOSSIP,  NULL    },
    { "Auction",    1, SV_SOCKPREF, SP_CHANAUCTION, NULL    },
    { "Usage",      1, SV_SOCKPREF, SP_CHANUSAGE,   NULL    },

    { "PROMPT:",    2, SV_TITLE,    0,              NULL    },
    { "Hit Pts",    2, SV_SOCKPREF, SP_PROMPTHP,    NULL    },
    { "Move Pts",   2, SV_SOCKPREF, SP_PROMPTMV,    NULL    },
    { "Power Pts",  2, SV_SOCKPREF, SP_PROMPTPP,    NULL    },
    { "Exits",      2, SV_SOCKPREF, SP_PROMPTEX,    NULL    },
    { " "       ,   2, SV_TITLE   , 0,              NULL    }, /* spacer must be " " */
    { "PREFS:",     2, SV_TITLE,    0,              NULL    },
    { "Expert",     2, SV_AUTO,     PA_EXPERT,      NULL    },

    { "AUTO:",      3, SV_TITLE,    0,              NULL    },
    { "AutoLook",   3, SV_AUTO,     PA_AUTOLOOK,    NULL    },
    { "AutoFlee",   3, SV_AUTO,     PA_AUTOFLEE,    NULL    },
    { "AutoEat",    3, SV_AUTO,     PA_AUTOEAT,     NULL    },
    { "AutoDrink",  3, SV_AUTO,     PA_AUTODRINK,   NULL    },

    { ""        ,  -1, SV_TITLE,    0,              NULL    }
  };


/* This proc goes through the thing's setting list and makes sure
 * it has permission to have all it's set's set. */
void SetFlagCheck(THING *thing) {
  LWORD i;
  SOCK *sock;
  FLAG *flag;

  if (!thing)
    return;
  if (thing->tType!=TTYPE_PLR)
    return;

  sock = BaseControlFind(thing);
  for (i=0; expertSetList[i].sName[0]; i++) {
    if (expertSetList[i].sCmd) {
      /* this is a restricted flag - let's check it */
      if (!ParseCommandCheck(TYPEFIND(expertSetList[i].sCmd,commandList),sock, "")) {
	/* uh oh, clear this sucker! if it's set! */
        flag = SetFlag(sock, expertSetList[i].sVariable);
        if (flag) 
          BITCLR(*flag, expertSetList[i].sFlag);
      }
    }
  }
}

CMDPROC(CmdSet) { /* Cmd(THING *thing, BYTE *cmd) */
  struct SetType * setList;
  LWORD columnIndex[MAX_COLUMN];

  BYTE  bufColumn[256];
  BYTE  bufLine[512];
  BYTE  word[256];
  FLAG *flag;
  LWORD i;
  LWORD temp;
  BYTE  notDone = TRUE;
  SOCK *sock;
  BYTE  expert = FALSE;

  /* what are we doing */
  cmd = StrOneWord(cmd, word); /* toss "set" */
  cmd = StrOneWord(cmd, word); /* grab argument (if any) */

  /* need this to change socket prefs */
  sock = BaseControlFind(thing);

  /* Determine is expert is set */
  if (sock && BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) expert = TRUE;

  /* Give them appropriate list of options to change */
  if (expert)
    setList = expertSetList;
  else
    setList = newbieSetList;
    
  /* See if they want to change a flag setting */
  if (*word) {
    for (i=0; setList[i].sName[0]; i++) {
      /* ignore color code to determine match */
      if (StrAbbrev(setList[i].sName, word)
      || (setList[i].sName[0]=='^' && StrAbbrev(setList[i].sName+2, word)) ) {
        if (setList[i].sCmd 
            && !ParseCommandCheck(TYPEFIND(setList[i].sCmd, commandList), sock, "")) {
          SendThing("^cI'm afraid you're not authorized to do that\n", thing);
          return;
        }
        if (setList[i].sVariable!=SV_TITLE) {
          flag = SetFlag(sock, setList[i].sVariable);
          BITFLIP(*flag, setList[i].sFlag);
          sprintf(
            bufLine,
            "^3Setting ^5%s^3 to ^5%-3s^V\n",
            setList[i].sName,
            BIT(*flag, setList[i].sFlag)  ? "On" : "Off"
          );
          SendThing(bufLine, thing);
          Plr(sock->sHomeThing)->pSockPref = sock->sPref;
          return;
        } else if (!strcmp(setList[i].sName, "ScreenSize")) { /* special case for screen size */
          cmd = StrOneWord(cmd, word); /* grab argument (if any) */
          if (!word) {
            SendThing("^cYes but what do you want to set it to?\n", thing);
            return;
          }
          sock->sScreenLines = atoi(word);
          MINSET(sock->sScreenLines, 5);
          temp = sock->sScreenLines;
          sprintf(bufLine, "^3Setting ^5ScreenSize^3 to ^5%ld\n", temp);
          SendThing(bufLine, thing);
          Plr(sock->sHomeThing)->pScreenLines = temp;
          return;
        } else if (!strcmp(setList[i].sName, "^3Description")) {
          EditStr(sock, &sock->sHomeThing->tDesc, 512, "Player Description", EP_ENDLF);
          return;
        } else if (!strcmp(setList[i].sName, "^3Email")) {
          SendThing("^cEnter your email address:\n", thing);
          EditStr(sock, &Plr(sock->sHomeThing)->pEmail, 128, "Email Address", EP_IMMNEW|EP_ONELINE|EP_ENDNOLF);
          return;
        } else if (!strcmp(setList[i].sName, "^3Plan")) {
          EditStr(sock, &Plr(sock->sHomeThing)->pPlan, 512, "Plan Description", EP_ENDLF);
          return;
        }
      }
    }
    SendThing("^cWhat was it you wanted to set now?\n", thing);
    return;
  }

  /* setup to display all the settings */
  notDone = TRUE;
  for (i=0; i<MAX_COLUMN; i++) /* init all the column indices to zero */
    columnIndex[i]=0;

  /* Okay List what all the flags are currently */
  while (notDone) {
    bufLine[0] = '\0';
    notDone = FALSE;

    /* print one from each column */
    for (i = 0; i<MAX_COLUMN; i++) {
      /* find next column entry that they are authorized to see */
      while((setList[columnIndex[i]].sName[0] )
          &&( (setList[columnIndex[i]].sColumn!=i)
            ||( (setList[columnIndex[i]].sCmd)
              &&(!ParseCommandCheck(TYPEFIND(setList[columnIndex[i]].sCmd, commandList), sock, "")) 
            )))
        columnIndex[i]++;

      /* Write out column data */
      flag = SetFlag(sock, setList[columnIndex[i]].sVariable);
      if (!flag) {
        if (!strcmp(setList[columnIndex[i]].sName, "ScreenSize")){  /* special case for screen size */
          temp = sock->sScreenLines;
          sprintf(bufColumn, "^3%-12s^5%-7ld", setList[columnIndex[i]].sName, temp);
        } else  /* Allow for color formats in TITLE's */
          if (setList[columnIndex[i]].sName[0] == '^')
            sprintf(bufColumn, 
                    "^%c%-19s", 
                    setList[columnIndex[i]].sName[1],
                    setList[columnIndex[i]].sName+2);
          else 
            sprintf(bufColumn, "^4%-19s", setList[columnIndex[i]].sName);
      } else {
        sprintf(
          bufColumn,
          "^3%-12s^5%-7s^V",
          setList[columnIndex[i]].sName,
          BIT(*flag, setList[columnIndex[i]].sFlag)  ? "On" : "Off"
        );
      }
      /* See if we have displayed every entry to this column */
      if (setList[columnIndex[i]].sName[0]) {
        columnIndex[i]++;/* still going */
        notDone = TRUE; /* as long as at least one column printed something keep going */
      }
      strcat(bufLine, bufColumn);
    }
    if (notDone) {
      strcat(bufLine, "\n");
      SendThing(bufLine, thing);
    }
  }
  SendHint("^V\n", thing);
  SendHint("^;HINT: See also ^<SETCOLOR^;\n", thing);
}
/******************* CmdSet - End **************************/
CMDPROC(CmdScore) {    /* void CmdProc(THING *thing, BYTE* cmd) */
  BYTE         buf[256];
  BYTE         buf2[256];
  BYTE         expert = FALSE;
  SOCK        *sock;
  LWORD        gain;

  /* Determine is expert is set */
  sock = BaseControlFind(thing);
  if (sock && BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) expert = TRUE;
 
   /* show them the stats */
  SendThing("^3SDesc:^4[^0", thing);
  SendThing(thing->tSDesc->sText, thing);
  SendThing("^4]\n^3LDesc: ^2", thing);
  SendAction(Base(thing)->bLDesc->sText, thing, NULL, SEND_SRC);
  SendThing("\n", thing);
  SendThing("^3Description:\n^1", thing);
  SendThing(thing->tDesc->sText, thing);
  if (thing->tType == TTYPE_PLR) {
    SendThing("^3Class:^4[^5", thing);
    TYPESPRINTF(buf2, Plr(thing)->pClass, classList, sizeof(buf2));
    strcat(buf2, "^4]");
    sprintf(buf, "%-13s ", buf2);
    SendThing(buf, thing);
    SendThing("        ^3Race:^4[^5", thing);
    SendThing(TYPESPRINTF(buf, Plr(thing)->pRace, raceList, sizeof(buf)), thing);
    SendThing("^4]\n", thing);
  }
  sprintf(buf, "^3Level:^4[^5%4hd^4] ", Character(thing)->cLevel);
  SendThing(buf, thing);
  if (expert) {
    sprintf(buf, "              ^3Aura:^4[^5%4hd^4]", Character(thing)->cAura);
    SendThing(buf, thing);
  }
  SendThing("               ^3Sex:^4[^5", thing);
  SendThing(TYPESPRINTF(buf, Character(thing)->cSex, sexList, 512), thing);
  SendThing("^4]\n", thing);
  
  sprintf(buf, "^3Armor:^4[^5%4hd^4] ", Character(thing)->cArmor);
  SendThing(buf, thing);
  if (expert) {
    sprintf(buf, "              ^3HitBonus:^4[^5%4ld^4] ", CharGetHitBonus(thing, Character(thing)->cWeapon));
    SendThing(buf, thing);
    sprintf(buf, "          ^3DamBonus:^4[^5%4ld^4]", CharGetDamBonus(thing, Character(thing)->cWeapon));
    SendThing(buf, thing);
  }
  SendThing("\n", thing);
  sprintf(buf2, "^3Money:^4[^5%ld^4] ", Character(thing)->cMoney);
  sprintf(buf, "%-34s ", buf2);
  SendThing(buf, thing);
  if (thing->tType == TTYPE_PLR) {
    sprintf(buf2, "^3Bank:^4[^5%ld^4] ", Plr(thing)->pBank);
    sprintf(buf, "%-34s ", buf2);
    SendThing(buf, thing);
  }
  sprintf(buf, "^3Exp:^4[^5%ld^4/^5%ld^4]\n", Character(thing)->cExp, PlayerExpNeeded(thing));
  SendThing(buf, thing);
  if (thing->tType == TTYPE_PLR) {
    sprintf(buf2, "^3Fame:^4[^5%hd^4] ", Plr(thing)->pFame);
    sprintf(buf, "%-34s ", buf2);
    SendThing(buf, thing);

    sprintf(buf, "^3Infamy:^4[^5%hd^4]\n", Plr(thing)->pInfamy);
    SendThing(buf, thing);
  }
  if (thing->tType == TTYPE_PLR && expert) {
    gain = raceList[Plr(thing)->pRace].rGainHunger;
    gain = CharGainAdjust(thing, GAIN_HUNGER, gain);
    MINSET(gain, 1);
    SendThing("\n^5Your Abilities are currently:\n^3", thing);    
    sprintf(buf, "^3Str: ^4[^5%4hd^4]                ^3HP:^4[^5%5ld^4/^5%5ld^3]          ^3Hunger:^4[^5%hd/%hd^4] +^5%ld^4/tick\n", 
            Plr(thing)->pStr, 
            Character(thing)->cHitP,
            CharGetHitPMax(thing),
            Plr(thing)->pHunger,
            raceList[Plr(thing)->pRace].rMaxHunger,
            gain
    );
    SendThing(buf, thing);
    gain = raceList[Plr(thing)->pRace].rGainThirst;
    gain = CharGainAdjust(thing, GAIN_THIRST, gain);
    MINSET(gain, 1);
    sprintf(buf, "^3Dex: ^4[^5%4hd^4]                ^3MP:^4[^5%5ld^4/^5%5ld^3]          ^3Thirst:^4[^5%hd/%hd^4] +^5%ld^4/tick\n", 
            Plr(thing)->pDex, 
            Character(thing)->cMoveP, 
            CharGetMovePMax(thing),
            Plr(thing)->pThirst,
            raceList[Plr(thing)->pRace].rMaxThirst,
            gain
    );
    SendThing(buf, thing);
    sprintf(buf, "^3Con: ^4[^5%4hd^4]                ^3PP:^4[^5%5ld^4/^5%5ld^3]          ^3Intox: ^4[^5%hd/%hd^4] -^5%hd^4/tick\n", 
            Plr(thing)->pCon, 
            Character(thing)->cPowerP, 
            CharGetPowerPMax(thing),
            Plr(thing)->pIntox,
            raceList[Plr(thing)->pRace].rMaxIntox,
            raceList[Plr(thing)->pRace].rGainIntox
    );
    SendThing(buf, thing);
    sprintf(buf, "^3Wis: ^4[^5%4hd^4]\n", Plr(thing)->pWis);
    SendThing(buf, thing);
    sprintf(buf, "^3Int: ^4[^5%4hd^4]                ^3Practices:^4[^5%ld^4]\n", 
            Plr(thing)->pInt,
            Plr(thing)->pPractice);
    SendThing(buf, thing);
  } else {
    sprintf(buf, "^3HP:^4[^5%5ld^4/^5%5ld^3]\n", 
            Character(thing)->cHitP,
            CharGetHitPMax(thing));
    SendThing(buf, thing);
    sprintf(buf, "^3MP:^4[^5%5ld^4/^5%5ld^3]\n", 
            Character(thing)->cMoveP, 
            CharGetMovePMax(thing));
    SendThing(buf, thing);
    sprintf(buf, "^3PP:^4[^5%5ld^4/^5%5ld^3]\n", 
            Character(thing)->cPowerP, 
            CharGetPowerPMax(thing));
    SendThing(buf, thing);
  }
  if (expert) {
    SendThing("\nYour Resistances are:\n", thing);
    sprintf(buf, 
            "^3Puncture  ^4[^5%3hd^4]",
            CharGetResist(thing, FD_PUNCTURE));
    SendThing(buf, thing);
    sprintf(buf, 
            "            ^3Slash ^4[^5%3hd^4]",
              CharGetResist(thing, FD_SLASH));
    SendThing(buf, thing);
    sprintf(buf, 
            "               ^3Concussive^4[^5%3hd^4]\n",
              CharGetResist(thing, FD_CONCUSSIVE));
    SendThing(buf, thing);
    sprintf(buf, 
            "^3Heat      ^4[^5%3hd^4]",
              CharGetResist(thing, FD_HEAT));
    SendThing(buf, thing);
    sprintf(buf, 
            "            ^3EMR   ^4[^5%3hd^4]",
              CharGetResist(thing, FD_EMR));
    SendThing(buf, thing);
    sprintf(buf, 
            "               ^3Laser     ^4[^5%3hd^4]\n",
              CharGetResist(thing, FD_LASER));
    SendThing(buf, thing);
    sprintf(buf, 
            "^3Psychic   ^4[^5%3hd^4]",
              CharGetResist(thing, FD_PSYCHIC));
    SendThing(buf, thing);
    sprintf(buf, 
            "            ^3Acid  ^4[^5%3hd^4]",
              CharGetResist(thing, FD_ACID));
    SendThing(buf, thing);
    sprintf(buf, 
            "               ^3Poison    ^4[^5%3hd^4]\n",
              CharGetResist(thing, FD_POISON));
    SendThing(buf, thing);
    SendThing("\n^3Affected by: ^4[^5", thing);
    SendThing(FlagSprintf(buf, Character(thing)->cAffectFlag, affectList, ' ', 512), thing);
    SendThing("^4]\n", thing);

    if (Character(thing)->cAffect) {
      SendHint("^;HINT: You can see what is affecting you by typing ^<AFFECT^;\n", thing);
    }
  }
}


CMDPROC(CmdAlias) { /* Cmd(THING *thing, BYTE *cmd) */
  ALIAS *alias;
  SOCK  *sock;
  BYTE   buf[512];
  BYTE   word[256];
  LWORD  i;
  
  sock = BaseControlFind(thing);
  if (!sock) return;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    SendThing("^5Pattern     Action\n", thing);
    SendThing("^4=======     ======\n", thing);
    for (i=0; i<sock->sAliasIndex.iNum; i++) {
      sprintf(buf, "^3%-10s ^4= ^b%s\n", 
        Alias(sock->sAliasIndex.iThing[i])->aPattern->sText,
        Alias(sock->sAliasIndex.iThing[i])->aAction->sText);
      SendThing(buf, thing);
    }
    SendThing("\n", thing);
    sprintf(buf, "^5%ld Aliases listed.\n", sock->sAliasIndex.iNum);
    SendThing(buf, thing);
    return;
  }

  cmd = StrOneWord(cmd, word);
  alias = AliasFind(&sock->sAliasIndex, word);
  if (*cmd) {
    if (alias) {
      sprintf(buf, "^5Overwriting ^3old Alias %s\n", alias->aPattern->sText);
      ALIASFREE(&sock->sAliasIndex, alias);
      SendThing(buf, thing);
    }
    alias = AliasCreate(&sock->sAliasIndex, word, cmd);
    if (!alias) {
      SendThing("^wI'm sorry you allready have too many aliases to create another\n", thing);
      return;
    }
    sprintf(buf, "^3Creating %s = %s\n", alias->aPattern->sText, cmd);
  } else if (alias) {
    sprintf(buf, "^3Deleting Alias %s\n", alias->aPattern->sText);
    ALIASFREE(&sock->sAliasIndex, alias);
  }
  SendThing(buf, thing);
  AliasWrite(sock);
}

CMDPROC(CmdUnAlias) { /* Cmd(THING *thing, BYTE *cmd) */
  ALIAS *alias;
  SOCK  *sock;
  BYTE   buf[256];
  
  sock = BaseControlFind(thing);
  if (!sock) return;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    CmdAlias(thing, ""); /* get list */
    return;
  }

  cmd = StrOneWord(cmd, buf);
  alias = AliasFind(&sock->sAliasIndex, buf);
  if (alias) {
    sprintf(buf, "^3Deleting Alias %s\n", alias->aPattern->sText);
    ALIASFREE(&sock->sAliasIndex, alias);
    AliasWrite(sock);
  } else
    sprintf(buf, "^gNo such Alias\n");

  SendThing(buf, thing);
}

CMDPROC(CmdTime) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE  buf[256];
  LWORD timeSinceReset;
  LWORD timeUntilReset;
  LWORD area;

  SendThing("^CUptime: ^g    ", thing);
  TimeSprintf(buf, time(0)-startTime);
  SendThing(buf, thing);

  if (Base(thing)->bInside && Base(thing)->bInside->tType==TTYPE_WLD) {
    area = Wld(Base(thing)->bInside)->wArea;
    SendThing("\n^CArea Reset: ^g", thing);
    if (areaList[area].aResetDelay <= 0) {
      SendThing("^rNever!\n", thing);
    } else {
      timeSinceReset = time(0) - areaList[area].aResetLast - startTime;
      timeUntilReset = areaList[area].aResetDelay*60 - timeSinceReset;
      if (timeUntilReset <= 0) {
        SendThing("^wAny Time!\n", thing);
      } else {
        TimeSprintf(buf, timeUntilReset);
        SendThing(buf, thing);
        SendThing("\n", thing);
      }
    }
  }
}

CMDPROC(CmdEnterMsg) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE buf[512];
  BYTE *i;

  if (thing->tType!=TTYPE_PLR) return;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    SendThing("^GYour Entrance message is currently:\n^g", thing);
    SendAction(Plr(thing)->pEnter->sText, thing, NULL, SEND_SRC);
    return;
  }

  i = StrFind(cmd, "$$n");
  while (i && i[2]=='N') i=StrFind(cmd, "$$n");
  if (i) {
    /* convert $$n into $n */
    while( *(i+1) ) {
      *i = *(i+1);
      i++;
    }
    *i = '\0'; /* copy null too */
    sprintf(buf,   "%s\n", cmd);
  } else {
    sprintf(buf,   "$n %s\n", cmd);
  }

  SendThing("^GOkay Doke, your entrance message is now:\n^g", thing);
  STRFREE(Plr(thing)->pEnter);
  Plr(thing)->pEnter = STRCREATE(buf); /* name & title */
  SendAction(Plr(thing)->pEnter->sText, thing, NULL, SEND_SRC);
}

CMDPROC(CmdExitMsg) { /* Cmd(THING *thing, BYTE *cmd) */
  BYTE buf[512];
  BYTE *i;

  if (thing->tType!=TTYPE_PLR) return;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    SendThing("^GYour exit message is currently:\n^g", thing);
    SendAction(Plr(thing)->pExit->sText, thing, NULL, SEND_SRC);
    return;
  }

  i = StrFind(cmd, "$$n");
  while (i && i[2]=='N') i=StrFind(cmd, "$$n");
  if (i) {
    /* convert $$n into $n */
    while( *(i+1) ) {
      *i = *(i+1);
      i++;
    }
    *i = '\0'; /* copy null too */
    sprintf(buf,   "%s\n", cmd);
  } else {
    sprintf(buf,   "$n %s\n", cmd);
  }

  SendThing("^GOkay Doke, your exit message is now:\n^g", thing);
  STRFREE(Plr(thing)->pExit);
  Plr(thing)->pExit = STRCREATE(buf); /* name & title */
  SendAction(Plr(thing)->pExit->sText, thing, NULL, SEND_SRC);
}

CMDPROC(CmdPrompt) { /* Cmd(THING *thing, BYTE *cmd) */
  if (thing->tType!=TTYPE_PLR) return;
  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  if (!*cmd) {
    SendThing("^GRestoring default settable prompt:\n^g", thing);
    STRFREE(Plr(thing)->pPrompt);
    Plr(thing)->pPrompt = STRCREATE("");

    SendThing("\n^GCustom prompt variables are ie (%<letter>):\n^g", thing);
    SendThing("h hitpoints         m move\n", thing);
    SendThing("p power             H max hits\n", thing);
    SendThing("M max move          P max power\n", thing);
    SendThing("t thirst            n hunger\n", thing);
    SendThing("u aura              c credits\n", thing);
    SendThing("i intox             R room#\n", thing);
    SendThing("e exits             a areaname\n", thing);
    SendThing("N name              f fighting\n", thing);
    SendThing("l leadername        w weaponname\n", thing);
    SendThing("b hitbonus          B dambonus\n", thing);
    SendThing("r Armor             E total experience\n", thing);
    SendThing("L XP to Next level\n", thing);
    SendHint("\n^;HINT: A typical prompt would be: ^<^^gH%h/%H M%m/%M P%p/%P ^^G-+^^C>\n", thing);
    return;
  }

  SendThing("^GOkay Doke, your prompt message is now:\n^g", thing);
  STRFREE(Plr(thing)->pPrompt);
  Plr(thing)->pPrompt = STRCREATE(cmd);
  SendAction(Plr(thing)->pPrompt->sText, thing, NULL, SEND_SRC);
  SendThing("\n", thing);
}

CMDPROC(CmdFinger) {
  BYTE   readPlayer = FALSE;
  THING *player;
  BYTE   buf[256];

  cmd = StrOneWord(cmd, NULL); /* lose the command at the start */
  player = PlayerFind(cmd);
  if (!player) {
    player = PlayerRead(cmd, PREAD_SCAN);
    readPlayer = TRUE;
  }
  if (!player) {
    SendThing("^wFinger who exactly?!?\n", thing);
    return;
  }

  SendAction("You finger $N\n", thing, player, SEND_SRC);
  if (readPlayer || Character(thing)->cLevel >= LEVEL_GOD) {
    sprintf(buf, "^gLevel:^G[^c%4hd^G] ", Character(player)->cLevel);
    SendThing(buf, thing);
  } else {
    sprintf(buf, "^G[^c%s^G] ", PlayerGetLevelDesc(player));
    SendThing(buf, thing);
  }
  SendThing("^gClass:^G[", thing);
  SendThing(TYPESPRINTF(buf, Plr(player)->pClass, classList, sizeof(buf)), thing);
  SendThing("^G] ^gRace:^G[", thing);
  SendThing(TYPESPRINTF(buf, Plr(player)->pRace, raceList, sizeof(buf)), thing);
  SendThing("^G]\n", thing);
  if (readPlayer) {
    SendThing("^wOffline        ^g", thing);
    TimeSprintf(buf, time(0)-Plr(player)->pTimeLastOn);
    SendThing(buf, thing);
    SendThing("\n", thing);
  } else {
    sprintf(buf, "^wIdle %ld Ticks\n", Plr(player)->pIdleTick);
    SendThing(buf, thing);
  }
  SendThing("^wTotal Playtime ^g", thing);
  TimeSprintf(buf, Plr(player)->pTimeTotal);
  SendThing(buf, thing);
  SendThing("\n", thing);

  if (Character(thing)->cLevel >= LEVEL_ADMIN) {
    SendThing("^wLast Login:    ^g", thing);
    SendThing(Plr(player)->pLastLogin->sText, thing);
    SendThing("\n", thing);
  }

  /* send their email if they have one */
  SendThing("^CEmail:\n^g", thing);
  if (BIT(Plr(player)->pAuto, PA_NOEMAIL) && Character(thing)->cLevel<LEVEL_ADMIN)
    SendThing("^r<Private>", thing);
  else if (*Plr(player)->pEmail->sText)
    SendThing(Plr(player)->pEmail->sText, thing);
  else
    SendThing("^r<None>", thing);

  /* send their plan if they have one */
  SendThing("\n^CPlan:\n^g", thing);
  if (*Plr(player)->pPlan->sText)
    SendThing(Plr(player)->pPlan->sText, thing);
  else
    SendThing("^r<None>", thing);
  SendThing("\n", thing);
  
  if (readPlayer)
    THINGFREE(player);
}

/* Theoretically quite a simple little command, but then I wanted 3 nicely
  formatted columns so simplicity and readability went to hell... ah well */
CMDPROC(CmdLevels) { /* Cmd(THING *thing, BYTE *cmd) */
  LWORD i;
  LWORD level;
  LWORD realLevel;
  BYTE  buf[256];

  if (thing->tType != TTYPE_PLR) {
    SendThing("^wI'm afraid you cant go up in level...\n", thing);
    return;
  }
  SendThing("^CLevels and the experience points required for them...\n\n", thing);
  realLevel = Character(thing)->cLevel;
  level = Character(thing)->cLevel-1;
  for (i=0; i<10; i++) {
    Character(thing)->cLevel=level;
    if (level < LEVEL_GOD) {
      sprintf(buf, "^w%4ld ^y%13ld       ", level+1, PlayerExpNeeded(thing));
      SendThing(buf, thing);
    } else {
      sprintf(buf, "^w%4ld ^g<GOD>\n", level+1);
      SendThing(buf, thing);
      break;
    }
    level+=10;

    Character(thing)->cLevel=level;
    if (level < LEVEL_GOD) {
      sprintf(buf, "^w%4ld ^y%13ld       ", level+1, PlayerExpNeeded(thing));
      SendThing(buf, thing);
    }
    level+=10;

    Character(thing)->cLevel=level;
    if (level < LEVEL_GOD) {
      sprintf(buf, "^w%4ld ^y%13ld\n", level+1, PlayerExpNeeded(thing));
      SendThing(buf, thing);
    } else 
      SendThing("\n", thing);
    level-=19;
  }
  Character(thing)->cLevel = realLevel;
}

CMDPROC(CmdSkillMax) {
  BYTE  buf[256];
  LWORD level;

  if (thing->tType!=TTYPE_PLR) return;
  cmd = StrOneWord(cmd, NULL);
  cmd = StrOneWord(cmd, buf);
  if (!*buf) {
    SendThing("^gSkillMax shows you the maximum skills you can have for a given level\n", thing);
    SendThing("^gUsage: SkillMax <level>\n", thing);
    return;
  }

  level = Character(thing)->cLevel;
  Character(thing)->cLevel = atol(buf);
  sprintf(buf, "pr %s", cmd);
  SkillShow(thing, buf, 0, 0xFFFFFFFF, 200); /* Thing, Cmd, CanPractice, CantPractice, maxLevel */
  Character(thing)->cLevel = level;
}

CMDPROC(CmdAffects) { /* Cmd(THING *thing, BYTE *cmd) */
  AFFECT *affect;
  BYTE    buf[512];

  SendThing("^pYou are currently affected by:\n", thing);
  SendThing("^P-+- +-+ -+-+-+-+- +-+-+-+- +-\n", thing);
  for (affect = Character(thing)->cAffect; affect; affect=affect->aNext) {
    sprintf(buf, "^3%-20s %hd\n", effectList[affect->aEffect].eName, affect->aDuration);
    SendThing(buf, thing);
  }

  SendThing("\n", thing);
  SendThing("^3AffectFlags:^4[^5", thing);
  SendThing(FlagSprintf(buf, Character(thing)->cAffectFlag, affectList, ' ', 512), thing);
  SendThing("^4]\n", thing);
}

/* 
 * Stop affects, things like Spiritwalk, Invis etc that you might
 * want to wear off prematurely
 */
CMDPROC(CmdStop) {
  /* match the name of the effect here */
  BYTE *stopList[] = {
    "Spiritwalk",
    "Invisibility",
    "ImprovedInvis",
    "\0"
  };

  LWORD   i;
  BYTE    buf[256];
  AFFECT *affect;
  
  cmd = StrOneWord(cmd, NULL);
  cmd = StrOneWord(cmd, buf);
  if (!*buf) {
    SendThing("^5Stoppable Affects are:\n^3", thing);
    SENDARRAY(stopList, 3, thing);
    return;
  }

  i = TYPEFIND(buf, stopList);
  if (i==-1) {
    SendThing("^wI'm afraid that is not a stoppable affect\n", thing);
    return;
  }
  /* Find the affect and set its duration to 0 - get rid of it */
  i = TYPEFIND(buf, effectList);
  if (i==-1) {
    SendThing("^wThe sysadmin has corrupted the stopList, what a loser!\n", thing);
    return;
  }
  affect = AffectFind(thing, i);
  if (!affect) {
    SendThing("^wBut you're not being affected by that right now!\n", thing);
    SendHint("^;HINT: Type ^<AFFECT^; for a list of things you are being affected by\n", thing);
    return;
  }

  AffectRemove(thing, affect);
}

void MiscGetColorName(BYTE *name, BYTE fg, BYTE bg) {
  LWORD i;
  LWORD j;

  bg = toupper(bg);
  for (i=0; *ansiFGColorList[i].cName && ansiFGColorList[i].cSymbol!=fg; i++);
  for (j=0; *ansiBGColorList[j].cName && ansiBGColorList[j].cSymbol!=bg; j++);
  sprintf(name, "%s-On-%s", ansiFGColorList[i].cName, ansiBGColorList[j].cName);
}

BYTE MiscGetColorSymbol(BYTE *name, BYTE *fg, BYTE *bg) {
  LWORD i;
  LWORD j;
  BYTE  buf[256];

  if (strlen(name)==4 && name[0]=='^' && name[1]=='&') {
    /* should validate against colorlist */
    for (i=0; *ansiFGColorList[i].cName && ansiFGColorList[i].cSymbol!=name[2]; i++);
    for (j=0; *ansiBGColorList[j].cName && ansiBGColorList[j].cSymbol!=name[3]; j++);
    if (!*ansiFGColorList[i].cName) return FALSE;
    if (!*ansiFGColorList[j].cName) return FALSE;
    *fg = name[2];
    *bg = name[3];
    return TRUE;
  }
  if (strlen(name)==2 && name[0]=='^') {
    /* should validate against colorlist */
    for (i=0; *ansiFGColorList[i].cName && ansiFGColorList[i].cSymbol!=name[1]; i++);
    if (!*ansiFGColorList[i].cName) return FALSE;
    *fg = name[1];
    *bg = 'A';
    return TRUE;
  }

  for (j=0; *ansiBGColorList[j].cName; j++) {
    for (i=0; *ansiFGColorList[i].cName; i++) {
      MiscGetColorName(buf, ansiFGColorList[i].cSymbol, ansiBGColorList[j].cSymbol);
      if (StrAbbrev(buf, name)) {
        *fg = ansiFGColorList[i].cSymbol;
        *bg = ansiBGColorList[j].cSymbol;
        return TRUE;
      }
    }
  }
  return FALSE;
}

/* Set the color prefs */
CMDPROC(CmdSetColor) {
  LWORD i;
  LWORD j;
  BYTE  buf[256];
  BYTE  word[256];
  LWORD numSent;

  if (thing->tType != TTYPE_PLR) return;

  cmd = StrOneWord(cmd, NULL);
  if (!*cmd) {
    SendThing("^5Color Preferences:\n", thing);
    /* show the list of current color prefs */
    i=0;
    j=0;
    while (*colorPrefList[i].cName || *colorPrefList[j].cName) {
      while (*colorPrefList[i].cName && colorPrefList[i].cColumn != 0) i++;
      while (*colorPrefList[j].cName && colorPrefList[j].cColumn != 1) j++;
      
      if (*colorPrefList[i].cName) {
        MiscGetColorName(word, Plr(thing)->pColorPref[i].cFG, Plr(thing)->pColorPref[i].cBG);
        sprintf(buf, "^3%-18s^%c%-18s^V    ", colorPrefList[i].cName, colorPrefList[i].cSymbol, word);
        i++;
      } else {
        sprintf(buf, "%-40c", ' ');
      }
      SendThing(buf, thing);

      if (*colorPrefList[j].cName) {
        MiscGetColorName(word, Plr(thing)->pColorPref[j].cFG, Plr(thing)->pColorPref[j].cBG);
        sprintf(buf, "^3%-18s^%c%-18s^V\n", colorPrefList[j].cName, colorPrefList[j].cSymbol, word);
        j++;
      } else
        strcpy(buf, "\n");

      SendThing(buf, thing);
    }
    return;
  }

  cmd = StrOneWord(cmd, word);
  i = TYPEFIND(word, colorPrefList);
  if (i == -1) {
    SendThing("^wThat isnt a valid preference name\n", thing);
    return;
  }

  if (!*cmd) {
    /* Show all the possible colors they could choose */
    SendThing("^5Available Colors Are:\n", thing);
    numSent = 0;
    for (j=0; *ansiBGColorList[j].cName; j++) {
      for (i=0; *ansiFGColorList[i].cName; i++) {
        if (ansiBGColorList[j].cFG != ansiFGColorList[i].cFG) {
          MiscGetColorName(word, ansiFGColorList[i].cSymbol, ansiBGColorList[j].cSymbol);
          sprintf(buf, "^&%c%c%-20s", ansiFGColorList[i].cSymbol, ansiBGColorList[j].cSymbol, word);
          /* strcpy(buf, word); */
          numSent++;
          if (numSent%4==0)
            strcat(buf, "\n");
          SendThing(buf, thing);
        }
      }
    }
    return;
  }

  /* okay pick a setting and change its color */
  if (!MiscGetColorSymbol(cmd, &Plr(thing)->pColorPref[i].cFG, &Plr(thing)->pColorPref[i].cBG)) {
    SendThing("^wThat is not a valid color name\n", thing);
    return;
  } else {
    MiscGetColorName(word, Plr(thing)->pColorPref[i].cFG, Plr(thing)->pColorPref[i].cBG);
    sprintf(buf, "^3Okey doke, from on it's %s ^%c%-20s\n", colorPrefList[i].cName, colorPrefList[i].cSymbol, word);
    SendThing(buf, thing);
    return;
  }
}

CMDPROC(CmdQuit) { /* Cmd(THING *thing, BYTE *cmd) */
  SOCK *sock;

  if (strcmp(cmd, "quit") == 0) {
    sock = BaseControlFind(thing);
    if (sock) {
      if (sock->sControlThing != sock->sHomeThing)
        CmdSwitch(thing, "");
      else
        sock->sMode = MODE_MAINMENU;
      SendThing(fileList[FILE_QUIT].fileStr->sText, thing);
      if (thing->tType == TTYPE_MOB && Mob(thing)->mTemplate == spiritTemplate)
        THINGFREE(thing);
    }
  } else {
    SendThing("^GType ^gQUIT ^Gno less, no more to quit\n", thing);
  }
}

