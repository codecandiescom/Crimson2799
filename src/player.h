/* Crimson2 Mud Server 
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary, if you are using it without my express
 * permission you are violating the copyright act and can potentially be
 * sued.
 * That said, if you would like to use it, I'm not an ogre, gimme a call
 * and maybe we can work something out.
 *
 * Current email address: cryogen@unix.infoserve.net
 * Phone number: (604) 591-9746
 */

#define PLAYER_MAX_SKILL 256

struct PlrType {
  CHARACTER pCharacter;

  BYTE      pRace;
  BYTE      pClass;
  LWORD     pPractice;
  LWORD     pBank;
  LWORD     pMovePMax;
  LWORD     pPowerPMax;
  WORD      pHunger;
  WORD      pThirst;
  WORD      pIntox;
  WORD      pStr;   /* because of maxes allways use ApplyTotal to these */
  WORD      pDex;
  WORD      pCon;
  WORD      pWis;
  WORD      pInt;

  WORD      pGainHit;   /* determined randomly at some point */
  WORD      pGainMove;
  WORD      pGainPower;
  WORD      pGainPractice;

  WORD      pFame;
  WORD      pInfamy;

  FLAG      pSystem;   /* player system flags, pkiller, thief etc */ 
  STR      *pPassword;
  STR      *pEnter;
  STR      *pExit;
  STR      *pPrompt;
  STR      *pEmail;
  STR      *pPlan;
  STR      *pLastLogin;
  FLAG      pAuto;     /* player autoactions */
  FLAG      pSockPref; /* player socket/comm/channel preferences */
  BYTE      pScreenLines;
  ULWORD    pTimeTotal;
  ULWORD    pTimeLastOn;
  ULWORD    pIdleTick;
  THING    *pIdleRoom;
  LWORD     pStartRoom; /* Room we will reappear in at signon */
  LWORD     pRecallRoom; /* Room we will recall to */
  LWORD     pDeathRoom; /* Room we will go to after we die*/
  WORD      pAttempts;

  WORD      pSkill[PLAYER_MAX_SKILL];

  COLORPREF pColorPref[COLORPREF_MAX];
};

/* Player system flags */
#define PS_LOG        1<<0
#define PS_KILLER     1<<1
#define PS_HELPER     1<<2
#define PS_CRASHSAVED 1<<3
#define PS_NOHASSLE   1<<4

/* Autoaction/pref flags */
#define PA_AUTOLOOK   1<<0
#define PA_AUTOEXIT   1<<1
#define PA_AUTOLOOT   1<<2
#define PA_AUTOASSIST 1<<3
#define PA_AUTOAGGR   1<<4
#define PA_AUTOJUNK   1<<5
#define PA_AUTOFLEE   1<<6
#define PA_AUTOEAT    1<<7
#define PA_AUTODRINK  1<<8
#define PA_NOEMAIL    1<<9
#define PA_HINT       1<<10
#define PA_EXPERT     1<<11
#define PA_CONSIDER   1<<12
#define PA_AUTORESCUE 1<<13
#define PA_AFK        1<<14

/* Special levels */
#define LEVEL_NEWBIE       1 /* player levels of note */
#define LEVEL_GREEN        4 
#define LEVEL_NOVICE      15 
#define LEVEL_EXPERIENCED 25 
#define LEVEL_VETERAN     40
#define LEVEL_ELITE       60
#define LEVEL_ULTRAELITE  80
#define LEVEL_LEGEND     101 /* wow they're really something now */
#define LEVEL_IMMORTAL   200 /* administrative levels */
#define LEVEL_GOD        201 /* administrative levels */
#define LEVEL_ADMIN      249
#define LEVEL_CODER      250
#define LEVEL_NOBODY     300

#define LEVEL_MOVETIRING   4
#define LEVEL_HUNGER       5
#define LEVEL_FIGHTTIRING  6

extern LWORD playerIdleTick;
extern INDEX playerIndex;
extern LWORD playerStartRoom;
extern BYTE *pSystemList[];
extern BYTE *pAutoList[];

extern void   PlayerInit(void);
extern THING *PlayerAlloc(BYTE *playerName);

#define PCO_OFFLINE   0
#define PCO_ONLINE    1
extern void   PlayerCountObjects(BYTE *directory, BYTE *playerFileName, LWORD online);

#define PNAME_INVALID 0 
#define PNAME_VALID   1
#define PNAME_IN_USE  2
extern BYTE   PlayerName(BYTE *playerName);

extern THING *PlayerFind(BYTE *playerName);
extern BYTE   PlayerCrashExists(THING *thing); /* remove crash recovery files for this player */
extern BYTE   PlayerCloneExists(THING *thing); /* remove crash recovery files for this player */
extern void   PlayerAddOnline(THING *thing);
extern BYTE   PlayerReadHeader(FILE *playerFile, BYTE *header);

#define PREAD_NORMAL  0 /* first check crash, then player */
#define PREAD_CLONE   1 
#define PREAD_CRASH   2
#define PREAD_PLAYER  3
#define PREAD_SCAN    4 /* Dont read object data */
extern THING *PlayerRead(BYTE *playerName, BYTE mode); /* read the player file and associated data */

extern void   PlayerUpdateTime(THING *player);
#define PWRITE_PLAYER 0
#define PWRITE_CRASH  1
#define PWRITE_CLONE  2
extern void   PlayerWrite(THING *player, BYTE mode); /* write the player file and associated data */
extern void   PlayerFree(THING *thing);
extern void   PlayerIdle();
extern void   PlayerTick();
extern void   PlayerRollAbilities(THING *thing, LWORD reroll);
extern LWORD  PlayerExpNeeded(THING *thing);
extern LWORD  PlayerExpUntilLevel(THING *thing);
extern void   PlayerGainLevel(THING *thing, LWORD message);
extern void   PlayerGainExp(THING *thing, LWORD exp);
extern void   PlayerGainFame(THING *thing, LWORD fame);
extern void   PlayerGainInfamy(THING *thing, LWORD infamy, BYTE *message);
extern LWORD  PlayerIsInfamous(THING *player);
extern void   PlayerDelete(THING *player);
extern BYTE  *PlayerGetLevelDesc(THING *player);

#define Plr(x) ((PLR*)(x))

