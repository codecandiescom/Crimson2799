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
#ifndef WIN32
  #include <arpa/telnet.h>
#else
  #define IAC         '\0'
  #define GA          '\0'
  #define WILL        '\0'
  #define WONT        '\0'
  #define TELOPT_ECHO '\0'
#endif

#include "crimson2.h"
#include "macro.h"
#include "queue.h"
#include "str.h"
#include "extra.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "exit.h"
#include "parse.h"
#include "area.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "group.h"
#include "affect.h"
#include "mobile.h"
#include "player.h"
#include "fight.h"
#include "skill.h"
#include "file.h"
#include "send.h"

#define SEND_HISTORY_MAX 100

BYTE TELNET_GA[]       = {IAC, GA, '\0'};
BYTE TELNET_ECHO_ON[]  = {IAC, WILL, TELOPT_ECHO, '\0'};
BYTE TELNET_ECHO_OFF[] = {IAC, WONT, TELOPT_ECHO, '\0'};

BYTE *sendFlagList[] = {
  "SEND_ROOM",
  "SEND_SRC",
  "SEND_DST",
  "SEND_VISIBLE",
  "SEND_AUDIBLE",
  "SEND_CAPFIRST",
  "SEND_GROUP",
  ""
};

/* started out as a macro */
void SendCheckPrompt(SOCK *sock) {
  if (BIT(sock->sFlag, SF_ATPROMPT)) { 
    SEND("\n", sock); 
    BITCLR(sock->sFlag, SF_ATPROMPT); 
  }
}

/* if we are sitting at a prompt prefix a EOLN sequence */
#define SENDCHECKPROMPT(sock) SendCheckPrompt(sock)

void SendEchoOn(SOCK *sock) {
  QAppend(sock->sOut, TELNET_ECHO_OFF, Q_DONTCHECKSTR);
}

void SendEchoOff(SOCK *sock) {
  QAppend(sock->sOut, TELNET_ECHO_ON, Q_DONTCHECKSTR);
}

/* this routine is only intended to clean up menu text in the editor
   when color codes are being shown, so it handles only up to 512 char
   strings, any longer will result in a crash! (since the edit menu is
   a fixed length this shouldnt be a problem)

   the idea is that color codes can be stripped from menu text but shown
   for the text being edited....
 */
void SendSmartColor(BYTE *str, SOCK *sock) {
  BYTE buf[512];

  LWORD src = 0;
  LWORD dst = 0;

  if ( !BIT(sock->sFlag, SF_XLATE) ) { /* if color codes are being shown */
    while (str[src] != '\0') {
      if (str[src] != '^') { /* copy the character, its not a color code */
        buf[dst] = str[src];
        src++;
        dst++;
      } else {
        src+=2; /* skip over color code */
      }
    }
    buf[dst] = '\0';
    SEND(buf, sock);
  } else {
    SEND(str, sock);
  }
}

void SendDefaultPrompt(SOCK *sock) {
  BYTE  buf[256];
  BYTE  eName[256];
  LWORD bLen;

  /* show exits etc on prompt if flags are so set */
  /* right now this is a little inefficient what with the
     repeated strlen's etc */
  buf[0] = '\0';
  /* hit points */
  if (BIT(sock->sPref, SP_PROMPTHP)) {
    sprintf(buf, "^bHP^p%ld", Character(sock->sControlThing)->cHitP);
  }
  /* move points */
  if (BIT(sock->sPref, SP_PROMPTMV)) {
    bLen = strlen(buf);
    if (bLen) { strcat(buf, " "); bLen++; }
    sprintf(buf+bLen, "^bMV^p%ld", Character(sock->sControlThing)->cMoveP);
  }
  /* Power points */
  if (BIT(sock->sPref, SP_PROMPTPP)) {
    bLen = strlen(buf);
    if (bLen) { strcat(buf, " "); bLen++; }
    sprintf(buf+bLen, "^bPP^p%ld", Character(sock->sControlThing)->cPowerP);
  }
  /* room #'s */
  if (BIT(sock->sPref, SP_PROMPTRM)) {
    bLen = strlen(buf);
    if (bLen) { strcat(buf, " "); bLen++; }
    if (Base(sock->sControlThing)->bInside->tType == TTYPE_WLD)
      sprintf(buf+bLen, "^b#^p%ld", Wld(Base(sock->sControlThing)->bInside)->wVirtual);
    else 
      sprintf(buf+bLen, "^b#^p<?>");
  }
  /* exits */
  if (BIT(sock->sPref, SP_PROMPTEX)) {
    EXIT *exit;
    WORD eOrder;

    bLen = strlen(buf);
    if (bLen) { strcat(buf, " "); bLen++; }
    sprintf(buf+bLen, "^b[^p"); bLen+=5;
    if (Base(sock->sControlThing)->bInside->tType == TTYPE_WLD) {
      for (eOrder=0;eOrderList[eOrder]<EDIR_MAX;eOrder++) {
        for (exit = Wld(Base(sock->sControlThing)->bInside)->wExit; exit; exit=exit->eNext) {
          if (exit->eWorld && !BIT(exit->eFlag, EF_HIDDEN)&&
	    (exit->eDir==eOrderList[eOrder])) {
            ExitGetName(exit, eName);
            if (BIT(exit->eFlag, EF_CLOSED)) {
              buf[bLen]=*eName; bLen++;
            } else {
              buf[bLen]=toupper(*eName); bLen++;
            }
            buf[bLen]=' '; bLen++;
          }
        }
      }
    } else {
      sprintf(buf+bLen, "<?>"); bLen+=3;
    }
    if (buf[bLen-1]==' ') bLen--;
    sprintf(buf+bLen, "^b]"); bLen+=3;
  }

  /* show prompt if prompt is on */
  bLen = strlen(buf);
  if (bLen) { strcat(buf, " "); bLen++; }
  sprintf(buf+bLen, "^b-+^w>^V");
  SEND(buf, sock);
}

void SendCustomPrompt(SOCK *sock) {
  BYTE    buf[512];
  BYTE   *src;
  LWORD   bLen;
  LWORD   srcOS;
  THING  *thing;

  bLen = 0;
  buf[0] = 0;
  thing = sock->sControlThing;
  src = Plr(sock->sHomeThing)->pPrompt->sText;
  srcOS = 0;

  while (src[srcOS]) {
    if (src[srcOS]!='%') {
      buf[bLen] = src[srcOS];
      srcOS++;
      bLen++;
      continue;
    }
    srcOS++;
    switch(src[srcOS]) {
    case '%':
      srcOS++;
      buf[bLen]='%';
      bLen++;
      break;

    case 'h':
      srcOS++;
      sprintf(buf+bLen, "%ld", Character(thing)->cHitP);
      bLen = strlen(buf);
      break;

    case 'm':
      srcOS++;
      sprintf(buf+bLen, "%ld", Character(thing)->cMoveP);
      bLen = strlen(buf);
      break;

    case 'p':
      srcOS++;
      sprintf(buf+bLen, "%ld", Character(thing)->cPowerP);
      bLen = strlen(buf);
      break;

    case 'H':
      srcOS++;
      sprintf(buf+bLen, "%ld", CharGetHitPMax(thing));
      bLen = strlen(buf);
      break;

    case 'M':
      srcOS++;
      sprintf(buf+bLen, "%ld", CharGetMovePMax(thing));
      bLen = strlen(buf);
      break;

    case 'P':
      srcOS++;
      sprintf(buf+bLen, "%ld", CharGetPowerPMax(thing));
      bLen = strlen(buf);
      break;

    case 't':
      srcOS++;
      if (thing->tType==TTYPE_PLR)
        sprintf(buf+bLen, "%hd", Plr(thing)->pThirst);
      else
        sprintf(buf+bLen, "-");
      bLen = strlen(buf);
      break;

    case 'n':
      srcOS++;
      if (thing->tType==TTYPE_PLR)
        sprintf(buf+bLen, "%hd", Plr(thing)->pHunger);
      else
        sprintf(buf+bLen, "-");
      bLen = strlen(buf);
      break;

    case 'u':
      srcOS++;
      sprintf(buf+bLen, "%hd", Character(thing)->cAura);
      bLen = strlen(buf);
      break;

    case 'c':
      srcOS++;
      sprintf(buf+bLen, "%ld", Character(thing)->cMoney);
      bLen = strlen(buf);
      break;

    case 'i':
      srcOS++;
      if (thing->tType==TTYPE_PLR)
        sprintf(buf+bLen, "%hd", Plr(thing)->pIntox);
      else
        sprintf(buf+bLen, "-");
      bLen = strlen(buf);
      break;

    case 'e': {
      EXIT *exit;
      BYTE  eName[256];
      srcOS++;

      if (Base(thing)->bInside->tType == TTYPE_WLD) {
        for (exit = Wld(Base(thing)->bInside)->wExit; exit; exit=exit->eNext) {
          if (exit->eWorld && !BIT(exit->eFlag, EF_HIDDEN)) {
            ExitGetName(exit, eName);
            if (BIT(exit->eFlag, EF_CLOSED)) {
              buf[bLen]=*eName; bLen++;
            } else {
              buf[bLen]=toupper(*eName); bLen++;
            }
            buf[bLen]=' '; bLen++;
          }
        }
      } else {
        sprintf(buf+bLen, "<?>"); bLen+=3;
      }
      if (buf[bLen-1]==' ') bLen--;
    } break;

    case 'b':
      srcOS++;
      sprintf(buf+bLen, "%ld", CharGetHitBonus(thing, Character(thing)->cWeapon));
      bLen = strlen(buf);
      break;

    case 'B':
      srcOS++;
      sprintf(buf+bLen, "%ld", CharGetDamBonus(thing, Character(thing)->cWeapon));
      bLen = strlen(buf);
      break;

    case 'N':
      srcOS++;
      sprintf(buf+bLen, "%s", thing->tSDesc->sText);
      bLen = strlen(buf);
      break;

    case 'w':
      srcOS++;
      if (Character(thing)->cWeapon)
        sprintf(buf+bLen, "%s", Character(thing)->cWeapon->tSDesc->sText);
      else
        sprintf(buf+bLen, "-");
      bLen = strlen(buf);
      break;

    case 'l':
      srcOS++;
      if (Character(thing)->cLead)
        sprintf(buf+bLen, "%s", Character(thing)->cLead->tSDesc->sText);
      else
        sprintf(buf+bLen, "-");
      bLen = strlen(buf);
      break;

    case 'f':
      srcOS++;
      if (Character(thing)->cFight)
        sprintf(buf+bLen, "%s", Character(thing)->cFight->tSDesc->sText);
      else
        sprintf(buf+bLen, "-");
      bLen = strlen(buf);
      break;

    case 'r':
      srcOS++;
      sprintf(buf+bLen, "%hd", Character(thing)->cArmor);
      bLen = strlen(buf);
      break;

    case 'E':
      srcOS++;
      sprintf(buf+bLen, "%ld", Character(thing)->cExp);
      bLen = strlen(buf);
      break;

    case 'a':
      srcOS++;
      if (Base(thing)->bInside->tType==TTYPE_WLD)
        sprintf(buf+bLen, "%s", areaList[Wld(Base(thing)->bInside)->wArea].aFileName->sText);
      else
        sprintf(buf+bLen, "-");
      break;

    case 'R':
      srcOS++;
      if (Base(thing)->bInside->tType==TTYPE_WLD) {
        if (!ParseCommandCheck(TYPEFIND("wdesc", commandList), sock, "")) {
          sprintf(buf+bLen, "-");
        } else {
          sprintf(buf+bLen, "%ld", Wld(Base(thing)->bInside)->wVirtual);
        }
      } else {
        sprintf(buf+bLen, "-");
      }
      break;

/*
 * r Room #  - make sure they are a god before you show it
 */

    case 'L':
      srcOS++;
      sprintf(buf+bLen, "%ld", PlayerExpUntilLevel(thing));
      bLen = strlen(buf);
      break;

    default:
      srcOS++;
      sprintf(buf+bLen, "<???>");
      bLen = strlen(buf);
      break;
    }
  }
  buf[bLen] = '\0';
  SEND(buf, sock);
}

void SendPrompt(SOCK *sock) {
  if (sock->sEdit.eStr) {
    EditSendPrompt(sock); /* to keep all edit stuff together */
  } else if (sock->sSubMode != SUBM_NONE) {
    SEND("^g>^V", sock);
  } else {
    switch(sock->sMode) {
    case MODE_PLAY:
      if (!BIT(sock->sPref, SP_COMPACT))
        SEND("^V\n", sock);

      if (Plr(sock->sHomeThing)->pPrompt->sLen>0) {
        SendCustomPrompt(sock);
        SEND("^V", sock);
      } else {
        SendDefaultPrompt(sock);
      }
      break;
    case MODE_MAINMENU:
      SEND("\n^G[^gMain Menu^G] ^c-+>^V", sock);
      break;
    case MODE_LOGIN:
    case MODE_PASSWORD:
    case MODE_CONFIRMNAME:
      SEND("^g>^V", sock);
      break;
    default:
      SEND("^B[^w? <topic>^b for help^B] ^G-+^g>^V", sock);
      break;
    }
  }
  /* append telnet GA string if flag is so set */
  if (BIT(sock->sPref, SP_TELNETGA))
    SEND(TELNET_GA, sock);
  sock->sCurrentLines=0;
  BITSET(sock->sFlag, SF_ATPROMPT);
}

/* going to move this inside help system, as default soon... */
void SendMainMenu(SOCK *sock) {
  BYTE buf[256];

  SEND("\n", sock);
  SEND("^cChoose an option:\n", sock);
  SEND("^gP^GLAY    ^bStart playing CrimsonMUD II\n", sock);
  if (BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) {
    SEND("^gD^GESC    ^bEdit Character description\n", sock);
    SEND("^gEM^GAIL   ^bEdit Email address\n", sock);
    SEND("^gPL^GAN    ^bEdit Plan message\n", sock);
    SEND("^gN^GEWS    ^bRead News - last updated ", sock);
    SEND(ctime(&fileList[FILE_NEWS].fileDate), sock);
    SEND("^gC^GREDITS ^bRead who did what\n", sock);
  }
  SEND("^gPA^GSS    ^bChange Password\n", sock);
  if (BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) {
    /*SEND("^gL^GOGON   ^bLogon as another Character\n", sock);*/
    SEND("^gS^GTATS   ^bShow Class/Race stats for this character\n", sock);
  }
  SEND("^gQ^GUIT    ^bLeave the Game\n", sock);
  SEND("\n", sock);
  if (BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) {
    SEND("^cThe Following Operations have dire consequences:\n", sock);
    SEND("^gDELETE  ^bthis character\n", sock);
    /*SEND("^gRENAME  ^bthis character (costs you half your level)\n", sock);*/
    if (Character(sock->sHomeThing)->cLevel >= LEVEL_CODER) {
      /* SEND("^gDOWN    ^bturf the server\n", sock); */
      SEND("^gREBOOT  ^breboot the server\n", sock);
    }
    SEND("\n^cUptime: ^g", sock);
    TimeSprintf(buf, time(0)-startTime);
    SEND(buf, sock);
    SEND(".\n", sock);
  }
}

void SendChooseAnsi(SOCK *sock) {
  SEND("\n", sock);
  SEND("Does your terminal support ANSI graphics?\n", sock);
  SEND("YES or NO\n", sock);
  SEND("\nThe Following Operations have dire consequences:\n", sock);
  SEND("CANCEL  Dont create this character\n", sock);
  SEND("\n", sock);
}

void SendChooseExpert(SOCK *sock) {
  SEND("\n", sock);
  SEND("^bIf you answer ^gYES^b to this question you might find that\n", sock);
  SEND("^bthe amount of decisions to make can be quite overwhelming!\n", sock);
  SEND("^bFor fast easy character creation please type ^gNO!\n", sock);
  SEND("\n", sock);
  SEND("^cAre you an expert mud player?\n", sock);
  SEND("^gYES ^bor ^gNO\n", sock);
  SEND("\n^cThe Following Operations have dire consequences:\n", sock);
  SEND("^gCANCEL  ^bDont create this character\n", sock);
  SEND("\n", sock);
}

void SendChooseSex(SOCK *sock) {
  if (!BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) {
    SEND("\n", sock);
    SEND("^bIn this sort of game you create a character that you pretend\n", sock);
    SEND("^bto be. Everyone else in the game sees only what you tell them\n", sock);
    SEND("^babout your character. If you make your character a hulking female\n", sock);
    SEND("^bSiliconoid Soldier over 7 feet tall, then as far as everyone else\n", sock);
    SEND("^bplaying the game is concerned that is who you are!\n", sock);
    SEND("\n", sock);
  }
  SEND("\n", sock);
  SEND("^cChoose a sex for your character:\n^g", sock);
  SENDARRAY(sexList, 1, sock->sHomeThing);
  SEND("\n^cThe Following Operations have dire consequences:\n", sock);
  SEND("^gCANCEL  ^bDont create this character\n", sock);
  SEND("\n", sock);
}

void SendChooseRace(SOCK *sock) {
  SEND("\n", sock);
  SEND("^cChoose a Race for your character:\n^g", sock);
  SENDARRAY(raceList, 1, sock->sHomeThing);
  SEND("\n^cThe Following Operations have dire consequences:\n", sock);
  SEND("^gBACK    ^bBack up a step, I changed my mind about something\n", sock);
  SEND("^gCANCEL  ^bDont create this character\n", sock);
  if (!BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) {
    SEND("\n", sock);
    SEND("^;HINT: The SILICONOID is probably the easiest race to get started with!\n", sock);
  }
  SEND("\n", sock);
}

void SendChooseClass(SOCK *sock) {
  SEND("\n", sock);
  SEND("^cChoose a Class for your character:\n^g", sock);
  SENDARRAY(classList, 1, sock->sHomeThing);
  SEND("\n^cThe Following Operations have dire consequences:\n", sock);
  SEND("^gBACK    ^bBack up a step, I changed my mind about something\n", sock);
  SEND("^gCANCEL  ^bDont create this character\n", sock);
  if (!BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) {
    SEND("\n", sock);
    SEND("^;HINT: The SOLDIER is probably the easiest class to get started with!\n", sock);
  }
  SEND("\n", sock);
}

void SendShowCombo(SOCK *sock) {
  THING       *player;
  BYTE         buf[256];
  LWORD        i;

  player = sock->sHomeThing;
  SEND("\n", sock);
  /* Class Stuff */
  SEND("^c", sock);
  SEND(classList[Plr(player)->pClass].cName, sock);
  SEND("^C skill category modifiers are:\n^w", sock);
  FlagSprintf(buf, classList[Plr(player)->pClass].cSkillDefault, skillFlagList, ' ', sizeof(buf));
  SEND(buf, sock);
  SEND("\n", sock);
  SEND("^bGood: ^p", sock);
  FlagSprintf(buf, classList[Plr(player)->pClass].cSkillGood, skillFlagList, ' ', sizeof(buf));
  SEND(buf, sock);
  SEND("\n", sock);
  SEND("^bLame: ^p", sock);
  FlagSprintf(buf, classList[Plr(player)->pClass].cSkillBad, skillFlagList, ' ', sizeof(buf));
  SEND(buf, sock);
  SEND("\n", sock);
  SEND("^bCant: ^p", sock);
  FlagSprintf(buf, classList[Plr(player)->pClass].cSkillCant, skillFlagList, ' ', sizeof(buf));
  SEND(buf, sock);
  SEND("\n", sock);
  SEND("\n", sock);

  /* Racial Stuff */
  SEND("^c", sock);
  SEND(raceList[Plr(player)->pRace].rName, sock);
  SEND("^C skill category modifiers are:\n", sock);
  SEND("^bGood: ^p", sock);
  FlagSprintf(buf, raceList[Plr(player)->pRace].rSkillGood, skillFlagList, ' ', sizeof(buf));
  SEND(buf, sock);
  SEND("\n", sock);
  SEND("^bLame: ^p", sock);
  FlagSprintf(buf, raceList[Plr(player)->pRace].rSkillBad, skillFlagList, ' ', sizeof(buf));
  SEND(buf, sock);
  SEND("\n", sock);
  SEND("^bCant: ^p", sock);
  FlagSprintf(buf, raceList[Plr(player)->pRace].rSkillCant, skillFlagList, ' ', sizeof(buf));
  SEND(buf, sock);
  SEND("\n", sock);
  SEND("\n", sock);

  /* Show skill modifiers here */
  SEND("^bRacial Specific Skill modifiers are:\n^p", sock);
  #define SDETAIL raceList[Plr(player)->pRace].rSkill
  for (i=0;  SDETAIL[i].sNum; i++) {
    if (SDETAIL[i].sGetSkill == SD_GETSKILL)
      sprintf(buf, 
              "%-15s %+hd ", 
              skillList[ *(SDETAIL[i].sNum) ].sName, 
              (WORD)SDETAIL[i].sModifier);
    else
      sprintf(buf, 
              "%-15s(%+hd)", 
              skillList[ *(SDETAIL[i].sNum) ].sName, 
              (WORD)SDETAIL[i].sModifier);
    if ((i+1)%3)
      strcat(buf, "      ");
    else
      strcat(buf, "\n");
    SEND(buf, sock);
  }

  #undef SDETAIL
  if (i%3)
    SEND("\n", sock);
  /* Show skill modifiers here */
  SEND("\n^bClass Specific Skill modifiers are:\n^p", sock);
  #define SDETAIL classList[Plr(player)->pClass].cSkill
  for (i=0;  SDETAIL[i].sNum; i++) {
    if (SDETAIL[i].sGetSkill == SD_GETSKILL)
      sprintf(buf, 
              "%-15s %+hd ", 
              skillList[ *(SDETAIL[i].sNum) ].sName, 
              (WORD)SDETAIL[i].sModifier);
    else
      sprintf(buf, 
              "%-15s(%+hd)", 
              skillList[ *(SDETAIL[i].sNum) ].sName, 
              (WORD)SDETAIL[i].sModifier);
    if ((i+1)%3)
      strcat(buf, "      ");
    else
      strcat(buf, "\n");
    SEND(buf, sock);
  }
  #undef SDETAIL
  if (i%3)
    SEND("\n", sock);

  SEND("\n", sock);
  if (sock->sMode == MODE_SHOWCOMBO) {
    SEND("\n^cThe Following Operations have dire consequences:\n", sock);
    SEND("^gBACK    ^bBack up a step, I changed my mind about something\n", sock);
    SEND("^gCANCEL  ^bDont create this character\n", sock);
    SEND("^gACCEPT  ^bLooks good, I wanna keep 'em\n", sock);
    SEND("\n", sock);
  }
}

void SendRollAbilities(SOCK *sock) {
  THING *player;
  BYTE   buf[256];

  player = sock->sHomeThing;
  SEND("\n", sock);
  SEND("^cYour Abilities are currently:\n^g", sock);
  sprintf(buf, "^gStr: ^G[^c%3hd^G] ^gCarry: ^c%3ld^glbs        ^gHP:^G[^c%3ld^G]\n", 
               Plr(player)->pStr, 
               CharGetCarryMax(player),
               CharGetHitPMax(player));
  SEND(buf, sock);
  sprintf(buf, "^gDex: ^G[^c%3hd^G] ^g+^c%d ^gMP/level          ^gMP:^G[^c%3ld^G]\n", 
               Plr(player)->pDex, 
               Plr(player)->pDex/20, 
               CharGetMovePMax(player));
  SEND(buf, sock);
  sprintf(buf, "^gCon: ^G[^c%3hd^G] ^g+^c%d ^gHP/level          ^gPP:^G[^c%3ld^G]\n", 
               Plr(player)->pCon, 
               Plr(player)->pCon/20, 
               CharGetPowerPMax(player));
  SEND(buf, sock);
  sprintf(buf, "^gWis: ^G[^c%3hd^G] ^g+^c%3.1f ^gPP/level\n", 
               Plr(player)->pWis,
               ((float)(Plr(player)->pWis/20))/2);
  SEND(buf, sock);
  sprintf(buf, "^gInt: ^G[^c%3hd^G] ^g+^c%3.1f ^gPP/level\n", 
               Plr(player)->pInt,
               ((float)(Plr(player)->pInt/20))/2);
  SEND(buf, sock);
  SEND("\nYour Resistances are:\n", sock);
  sprintf(buf, 
          "^gPuncture  [^c%3hd^G]",
          CharGetResist(player, FD_PUNCTURE));
  SEND(buf, sock);
  sprintf(buf, 
          "  ^gSlash     [^c%3hd^G]",
            CharGetResist(player, FD_SLASH));
  SEND(buf, sock);
  sprintf(buf, 
          "  ^gConcussive[^c%3hd^G]\n",
            CharGetResist(player, FD_CONCUSSIVE));
  SEND(buf, sock);
  sprintf(buf, 
          "^gHeat      [^c%3hd^G]",
            CharGetResist(player, FD_HEAT));
  SEND(buf, sock);
  sprintf(buf, 
          "  ^gEMR       [^c%3hd^G]",
            CharGetResist(player, FD_EMR));
  SEND(buf, sock);
  sprintf(buf, 
          "  ^gLaser     [^c%3hd^G]\n",
            CharGetResist(player, FD_LASER));
  SEND(buf, sock);
  sprintf(buf, 
          "^gPsychic   [^c%3hd^G]",
            CharGetResist(player, FD_PSYCHIC));
  SEND(buf, sock);
  sprintf(buf, 
          "  ^gAcid      [^c%3hd^G]",
            CharGetResist(player, FD_ACID));
  SEND(buf, sock);
  sprintf(buf, 
          "  ^gPoison    [^c%3hd^G]\n",
            CharGetResist(player, FD_POISON));
  SEND(buf, sock);
  SEND("\n^cYou may:\n", sock);
  SEND("^gA^GCCEPT  ^bGreat abilities, I like 'em\n", sock);
  SEND("^gR^GEROLL  ^bEww, Ick, cant we do better than that?\n", sock);
  SEND("\n^cThe Following Operations have dire consequences:\n", sock);
  SEND("^gBACK    ^bBack up a step, I changed my mind about something\n", sock);
  SEND("^gCANCEL  ^bDont create this character\n", sock);
  SEND("\n", sock);
}

/* Telepathy links and Control links are handled here,
   Hear and See links are handled in SendActionStr */
BYTE SendThing(BYTE *str, THING *thing) {
  BASELINK *i;
  SOCK     *sock;
  BYTE      sentTo = 0;
  THING    *room;

  if (!thing)
    return 0;
  if (thing->tType < TTYPE_BASE) return 0;
  room=Base(thing)->bInside;
  for (i=Base(thing)->bLink; i; i=i->lNext) {
    if (i->lType == BL_CONTROL) {
      SENDCHECKPROMPT(i->lDetail.lSock);
      SEND(str, i->lDetail.lSock);
      sentTo++;
    } else if (i->lType == BL_TELEPATHY_SND) {
      if (room->tType==TTYPE_WLD) {
	if (BIT(Wld(room)->wFlag,WF_PRIVATE)) {
	  continue;
	}
      } /* else no private! <grin> */
      sock = BaseControlFind(i->lDetail.lThing);
      SENDCHECKPROMPT(sock);
      SEND("^w%", sock);
      SEND(str, sock);
      sentTo++;
    }
  }
  return sentTo;
}

/* Add to their personal history */
void SendHistory(BYTE *str, THING *thing) {
  SOCK     *sock;
  BYTE      buf[512];

  sock = BaseControlFind(thing);
  if (sock) {
    if (sock->sPersonal->qValid >= SEND_HISTORY_MAX)
      QRead(sock->sPersonal, buf, sizeof(buf), Q_COLOR_IGNORE, NULL);
    QAppend(sock->sPersonal, str, Q_DONTCHECKSTR);
  }
}

BYTE SendThingCapFirst(BYTE *str, THING *thing) {
  BYTE  buf[2];
  BYTE  sentTo = 0;

  buf[0] = toupper(*str);
  buf[1] = '\0';

  sentTo = SendThing(buf, thing);
  if (sentTo) /* if theres anybody out there... send the rest too */
    SendThing(str+1, thing);
 
  return sentTo;
}

/* if the channel flag is set, send them the text */
/* Note that snooping will not show channel messages */
/* will not work for clantalk however since that depends on their clan as well as the flag */
void SendChannel(BYTE *str, THING *thing, FLAG chanFlag) {
  SOCK *i;

  for (i = sockList; i; i=i->sNext) {
    if (i->sMode>MODE_PASSWORD 
    && (BIT(i->sPref, chanFlag)) 
    && (!thing || i->sControlThing != thing)) {
      SENDCHECKPROMPT(i);
      SEND(str, i);
    }
  }
}

void SendHint(BYTE *str, THING *thing) {
  SOCK *sock;
  
  sock = BaseControlFind(thing);
  if (!sock) return;
  if (BIT(Plr(sock->sHomeThing)->pAuto, PA_HINT))
    SendThing(str, thing);
}

void SendAll(BYTE *str) {
  SOCK *i;

  for (i = sockList; i; i=i->sNext) {
    SENDCHECKPROMPT(i);
    SEND(str, i);
  }
}

BYTE *him       = "him";
BYTE *her       = "her";
BYTE *it        = "it";
BYTE *he        = "he";
BYTE *she       = "she";
BYTE *yourself  = "yourself";
BYTE *himself   = "himself";
BYTE *herself   = "herself";
BYTE *itself    = "itself";
BYTE *his       = "his";
BYTE *its       = "its";
BYTE *an        = "an";
BYTE *a         = "a";
BYTE *someone   = "someone";
BYTE *something = "something";

BYTE *HimHerIt(THING *thing) {
  switch(thing->tType) {
  case TTYPE_PLR:
  case TTYPE_MOB:
    if (Character(thing)->cSex == SEX_MALE) return him;
    if (Character(thing)->cSex == SEX_FEMALE) return her;
  }
  return it;
}

BYTE *HeSheIt(THING *thing) {
  switch(thing->tType) {
  case TTYPE_PLR:
  case TTYPE_MOB:
    if (Character(thing)->cSex == SEX_MALE) return he;
    if (Character(thing)->cSex == SEX_FEMALE) return she;
  }
  return it;
}

BYTE *HimselfHerselfItself(THING *thing) {
  switch(thing->tType) {
  case TTYPE_PLR:
  case TTYPE_MOB:
    if (Character(thing)->cSex == SEX_MALE) return himself;
    if (Character(thing)->cSex == SEX_FEMALE) return herself;
  }
  return itself;
}

BYTE *HisHerIts(THING *thing) {
  switch(thing->tType) {
  case TTYPE_PLR:
  case TTYPE_MOB:
    if (Character(thing)->cSex == SEX_MALE) return his;
    if (Character(thing)->cSex == SEX_FEMALE) return her;
  }
  return its;
}

BYTE *AnA(THING *thing) {
  switch(*thing->tSDesc->sText) {
  case 'a':
  case 'e':
  case 'i':
  case 'o':
  case 'u':
  case 'A':
  case 'E':
  case 'I':
  case 'O':
  case 'U':
    return an;
  default: /* fall thru to here */
    return a;
  }
}

BYTE *SomeoneSomething(THING *thing) {
  switch(thing->tType) {
  case TTYPE_PLR:
  case TTYPE_MOB:
    return someone;
  }
  return something;
}


/* Get the string substitution for a reserved escape sequence */
BYTE *SendThingStr(BYTE reservedChar, THING *srcThing, THING *dstThing, FLAG actionFlag) {
  if (reservedChar=='$') return "$";

  /* Substitute srcThing escape sequences */
  if (srcThing) {
    switch (reservedChar) {
    case 'n': /* name ie sdesc */
      if (BIT(actionFlag, SEND_PARTIALSRC))
        return SomeoneSomething(srcThing);
      else
        return srcThing->tSDesc->sText;
      break;
    case 'l': /* ldesc */
      if (srcThing->tType!=TTYPE_WLD)
        return Base(srcThing)->bLDesc->sText;
      else
        return strNull;
      break;
    case 'd': 
      return srcThing->tDesc->sText;
      break;
    case 's': 
      if (BIT(actionFlag, SEND_PARTIALSRC))
        return its;
      else
        return HisHerIts(srcThing);
      break;
    case 'e': 
      if (BIT(actionFlag, SEND_PARTIALSRC))
        return it;
      else
        return HeSheIt(srcThing);
      break;
    case 'm':
      if (BIT(actionFlag, SEND_PARTIALSRC))
        return its;
      else
        return HimHerIt(srcThing);
      break;
    case 'a': 
      return AnA(srcThing);
      break;
    }
  }

  /* Substitute dstThing escape sequeces */
  if (dstThing) {
    switch (reservedChar) {
    case 'N':
      /* dstThing==srcThing is a special case, where you are performing
         an action on yourself, so substitute himself, etc for $N
      */
      if (dstThing==srcThing)
        if (BIT(actionFlag, SEND_SRC))
          return yourself;
        else
          if (BIT(actionFlag, SEND_PARTIALDST))
            return itself;
          else
            return HimselfHerselfItself(dstThing);
      else
        if (BIT(actionFlag, SEND_PARTIALDST))
          return SomeoneSomething(dstThing);
        else
          return dstThing->tSDesc->sText;
      break;
    case 'L':
      if (dstThing->tType!=TTYPE_WLD)
        return Base(dstThing)->bLDesc->sText;
      else
        return strNull;
      break;
    case 'D':
      return dstThing->tDesc->sText;
      break;
    case 'S':
      if (BIT(actionFlag, SEND_PARTIALDST))
        return its;
      else
        return HisHerIts(dstThing);
      break;
    case 'E':
      if (BIT(actionFlag, SEND_PARTIALDST))
        return it;
      else
        return HeSheIt(dstThing);
      break;
    case 'M':
      if (BIT(actionFlag, SEND_PARTIALDST))
        return it;
      else
        return HimHerIt(dstThing);
      break;
    case 'A':
      return AnA(dstThing);
      break;
    }
  }

  return strNull; /* global null str pointer */
}

/* return whether we sent anything at all - this routine does the checking
   for whether the recipient can hear/see the message, it also checks for
   audio/visual links (control/telepathic links are check by SendThing) */
BYTE SendActionStr(BYTE *str, BYTE escapeCode, THING *srcThing, THING *dstThing, FLAG actionFlag, THING *sendThing) {
  BYTE        partialSrc;
  BYTE        send;
  BYTE        hear;
  BYTE        see;
  BASELINK   *baseLink;

  partialSrc = 0;
  send       = 1;
  hear       = 0;
  see        = 0;
  if (BIT(actionFlag, SEND_AUDIBLE|SEND_VISIBLE)) {
    see = 1;
    hear = 1;
    switch(ThingCanSee(sendThing, srcThing)) {
    case TCS_CANTSEE:    
      if(!ThingCanHear(sendThing, srcThing))
        send=0;
      else
        partialSrc=1;
      break;
    case TCS_SEEPARTIAL:  
      partialSrc=1; 
      break;
    }
  } else if (BIT(actionFlag, SEND_VISIBLE)) {
    see = 1;
    switch(ThingCanSee(sendThing, srcThing)) {
    case TCS_CANTSEE:    send=0;       break;
    case TCS_SEEPARTIAL: partialSrc=1; break;
    }
  } else if (BIT(actionFlag, SEND_AUDIBLE)) {
    hear = 1;
    if (!ThingCanHear(sendThing, srcThing)) send=0;
  }


  if (send) {
    if (*str) {
      SendThing(str, sendThing);
      if (BIT(actionFlag, SEND_HISTORY)) 
        SendHistory(str, sendThing);

      /* send to all the eavesdroppers */
      if (sendThing->tType>=TTYPE_BASE && Base(sendThing)->bLink) {
        if (see) {
          baseLink = BaseLinkFind(Base(sendThing)->bLink, BL_HEAR_SND, NULL);
          while (baseLink) {
            SendThing("^c%", baseLink->lDetail.lThing);
            SendThing(str, baseLink->lDetail.lThing);
            baseLink = BaseLinkFind(baseLink->lNext, BL_HEAR_SND, NULL);
          }
        }
      }
      /* send to all the peeping toms */
      if (sendThing->tType>=TTYPE_BASE && Base(sendThing)->bLink) {
        if (see) {
          baseLink = BaseLinkFind(Base(sendThing)->bLink, BL_SEE_SND, NULL);
          while (baseLink) {
            SendThing("^y%", baseLink->lDetail.lThing);
            SendThing(str, baseLink->lDetail.lThing);
            baseLink = BaseLinkFind(baseLink->lNext, BL_SEE_SND, NULL);
          }
        }
      }
    }    
    
    /* Send the xlated escape code if there is one */
    if (escapeCode) {
      if (partialSrc) BITSET(actionFlag, SEND_PARTIALSRC);
      if (escapeCode == toupper(escapeCode)) {
        /* only do this if its going to a dest */
        if (ThingCanSee(sendThing, dstThing) != TCS_SEENORMAL) 
          BITSET(actionFlag, SEND_PARTIALDST);
      }

      if (BIT(actionFlag, SEND_CAPFIRST)) {
        SendThingCapFirst(SendThingStr(escapeCode, srcThing, dstThing, actionFlag), sendThing);
        if (BIT(actionFlag, SEND_HISTORY))
          SendHistory(SendThingStr(escapeCode, srcThing, dstThing, actionFlag), sendThing);

        /* send to all the eavesdroppers */
        if (sendThing->tType>=TTYPE_BASE && Base(sendThing)->bLink) {
          if (see) {
            baseLink = BaseLinkFind(Base(sendThing)->bLink, BL_HEAR_SND, NULL);
            while (baseLink) {
              SendThing("^c%", baseLink->lDetail.lThing);
              SendThingCapFirst(SendThingStr(escapeCode, srcThing, dstThing, actionFlag), baseLink->lDetail.lThing);
              baseLink = BaseLinkFind(baseLink->lNext, BL_HEAR_SND, NULL);
            }
          }
        }
        /* send to all the peeping toms */
        if (sendThing->tType>=TTYPE_BASE && Base(sendThing)->bLink) {
          if (see) {
            baseLink = BaseLinkFind(Base(sendThing)->bLink, BL_SEE_SND, NULL);
            while (baseLink) {
              SendThing("^y%", baseLink->lDetail.lThing);
              SendThingCapFirst(SendThingStr(escapeCode, srcThing, dstThing, actionFlag), baseLink->lDetail.lThing);
              baseLink = BaseLinkFind(baseLink->lNext, BL_SEE_SND, NULL);
            }
          }
        }        

      } else {
        SendThing(SendThingStr(escapeCode, srcThing, dstThing, actionFlag), sendThing);
        if (BIT(actionFlag, SEND_HISTORY))
          SendHistory(SendThingStr(escapeCode, srcThing, dstThing, actionFlag), sendThing);

        /* send to all the eavesdroppers */
        if (sendThing->tType>=TTYPE_BASE && Base(sendThing)->bLink) {
          if (see) {
            baseLink = BaseLinkFind(Base(sendThing)->bLink, BL_HEAR_SND, NULL);
            while (baseLink) {
              SendThing("^c%", baseLink->lDetail.lThing);
              SendThing(SendThingStr(escapeCode, srcThing, dstThing, actionFlag), baseLink->lDetail.lThing);
              baseLink = BaseLinkFind(baseLink->lNext, BL_HEAR_SND, NULL);
            }
          }
        }
        /* send to all the peeping toms */
        if (sendThing->tType>=TTYPE_BASE && Base(sendThing)->bLink) {
          if (see) {
            baseLink = BaseLinkFind(Base(sendThing)->bLink, BL_SEE_SND, NULL);
            while (baseLink) {
              SendThing("^y%", baseLink->lDetail.lThing);
              SendThing(SendThingStr(escapeCode, srcThing, dstThing, actionFlag), baseLink->lDetail.lThing);
              baseLink = BaseLinkFind(baseLink->lNext, BL_SEE_SND, NULL);
            }
          }
        }
        
      }
    }
  }

  return send;
}
  
void SendAction(BYTE *str, THING *srcThing, THING *dstThing, FLAG actionFlag) {
/* color is processed in Q routines, not here so it will work with SendAll etc */
  LWORD i      = 0;
  LWORD count;
  LWORD sendCount = 0;
  BYTE  strEnd = FALSE;
  THING *room;
  THING *group;
  BYTE  buf[512];
  BYTE  escapeCode = 0;

  if (!str) return;
  /* determine if we need to capitalize the first escape sequence */
  for (i=0;str[i]&&str[i]!='$';i++)
    if (str[i]=='^' && str[i+1]) 
      i++; /* Only +1 since the ++ in the for loop will incr. again */
    else 
      BITCLR(actionFlag, SEND_CAPFIRST);

  i=0;
  while(str[i]!='\0') {
    count = 0;
    for(;count < 511 && str[i]!='\0' && str[i]!='$';i++) {
      buf[count] = str[i];
      count++;
    }
    buf[count] = '\0';
    if (str[i] == '\0') {
      strEnd = TRUE;
    } else if (str[i] == '$') {
      i+=2; /* skip over $<c> sequence */
    }
    /* at this point buf contains a copy of everything up to but not including
      the first $ escape sequence,
      
      if we have copied the whole of str then strEnd=TRUE, otherwise 
      str[i-2]=='$' and str[i-1] is equal to the escape code (n,N,s,S etc)
      
      str[i] is the first character after the escape code (possibly even \0)
     */
    /* escapeCode is either 0 or the escape code */
    if (!strEnd && str[i-2]=='$')
      escapeCode = str[i-1];
    else
      escapeCode = 0;

    /* fire off the string */
    if (BIT(actionFlag, SEND_SRC)) {
      /* send all the text up to the escape sequence or end of string */
      /* if there is an escape sequence, xlate it first then send the result */
      sendCount += SendActionStr(buf, escapeCode, srcThing, dstThing, actionFlag, srcThing);
    }
    /* if you are doing an action to yourself (srcThing==dstThing) then
       dont send the SEND_DST message, you got the message allready with SEND_SRC
    */
    if (BIT(actionFlag, SEND_DST) 
    && ( (BIT(actionFlag, SEND_SELF)) || (srcThing!=dstThing) )) {
      sendCount += SendActionStr(buf, escapeCode, srcThing, dstThing, actionFlag, dstThing);
    }
    /* Send the message to the room */
    if (BIT(actionFlag, SEND_ROOM)) {
      /* if the first pointer is a world structure - by Cams request */
      if (srcThing && srcThing->tType == TTYPE_WLD)
        room = srcThing->tContain;
      else if (srcThing && srcThing->tType>=TTYPE_BASE && Base(srcThing)->bInside)
        room = Base(srcThing)->bInside->tContain;
      else
        room = NULL;
      for (; room; room = room->tNext) {
        if (room!=srcThing && room!=dstThing) {
          sendCount += SendActionStr(buf, escapeCode, srcThing, dstThing, actionFlag, room);
        }
      }
    }
    /* Send it to any groups you might be in.... */
    if (BIT(actionFlag, SEND_GROUP) 
        && srcThing->tType >= TTYPE_CHARACTER
        && Character(srcThing)->cLead) {
      for (group = GroupGetHighestLeader(srcThing); group; group = Character(group)->cFollow) {
        if (GroupIsGroupedMember(group,srcThing) 
            && group!=srcThing 
            && group!=dstThing) {
          sendCount += SendActionStr(buf, escapeCode, srcThing, dstThing, actionFlag, group);
        }
      }
    }

    /* loop again or break out of the while loop */
    if (strEnd || sendCount==0) {
      /* break out of the loop if either:
          Nobody can see these messages
       or This is the end of the string silly!
      */
      break;
    } else {
      /* set str to point to the text immediately following the 
         the $ escape sequence
       */
      str += i;
      i = 0;
    }

    /* after first pass, no more capitalization */
    BITCLR(actionFlag, SEND_CAPFIRST);

  }
}

/* can make more efficient by replacing all SendThing's */
void SendArray(ULWORD tList, LWORD tListSize, LWORD column, THING *thing) {
  BYTE   buf[256];
  BYTE   truncateStr[256];
  LWORD  maxLen;
  LWORD  j;
  ULWORD i = 0;

  maxLen = 80/column;

  while(**((BYTE**)(tList+i*tListSize))!='\0') {
    sprintf(buf, "%s", StrTruncate(truncateStr, (*((BYTE**)(tList+i*tListSize))), maxLen-1));
    i++;
    if (i%column==0) {
      strcat(buf,"\n");
    } else {
      for(j = strlen(buf); j<maxLen; j++)
        buf[j]=' '; /* pad out with spaces */
      buf[j]='\0'; /* reterminate */
    }
    SendThing(buf, thing);
  }

  if (i%column!=0)
    SendThing("\n", thing);
}

