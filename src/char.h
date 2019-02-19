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

#define CHAR_MAX_RESIST 9
#define CHAR_MAX_FOLLOW 10

/* Sorry about naming this file "char.c" rather than character.c but
  I go back and forth between Unix and Dos and character is 9 letters */

struct CharacterType { /* common data between players and Mobs */
   BASE         cBase;

   WORD         cAura;
   BYTE         cLevel;
   WORD         cHitBonus;
   WORD         cDamBonus;
   WORD         cArmor;
   WORD         cSpeed;
   LWORD        cHitP;
   LWORD        cMoveP;
   LWORD        cPowerP;

   LWORD        cHitPMax;

   LWORD        cMoney;
   LWORD        cExp;
   BYTE         cPos;
   BYTE         cSex;

   FLAG         cAffectFlag; /* affect flags invis etc */
   AFFECT      *cAffect;     /* affect chain */
   THING       *cFollow; /* the next person following */
   THING       *cLead;   /* the overall leader */
   FLAG         cEquip;
   THING       *cWeapon; /* equipped weapon */

   STR         *cRespond; /* who we're responding to - set with first key of last tell */

   THING       *cFight; /* who are we fighting */
   EXIT        *cFightExit; /* which way to the target */
   BYTE         cFightRange; /* which way to the target */
   WORD         cResist[CHAR_MAX_RESIST];   /* damage resistances */
};

typedef struct PosType {
  BYTE *pName;
  BYTE *pDesc;
} POS;

#define SEX_MALE     0
#define SEX_FEMALE   1
#define SEX_SEXLESS  2

#define POS_DEAD     0
#define POS_MORTAL   1
#define POS_INCAP    2
#define POS_STUNNED  3
#define POS_SLEEPING 4
#define POS_RESTING  5
#define POS_SITTING  6
#define POS_FIGHTING 7
#define POS_STANDING 8

extern BYTE  *sexList[];
extern POS    posList[];

extern BYTE  CharAddFollow(THING *thing, THING *group);
extern void  CharRemoveFollow(THING *thing);
extern void  CharKillFollow(THING *thing);
extern void  CharGainExpFollow(THING *thing, LWORD exp);
extern WORD  CharGetResist(THING *thing, FLAG rFlag);
extern LWORD CharGetHitPMax(THING *thing);
extern LWORD CharGetMovePMax(THING *thing);
extern LWORD CharGetPowerPMax(THING *thing);

/* Generally these skills are here because mobs get calculated defaults,
   there shouldnt be a routine here for every skill in the game */
extern LWORD CharGetHide(THING *thing);
extern LWORD CharGetSneak(THING *thing);
extern LWORD CharGetPerception(THING *thing);
extern LWORD CharGetGuard(THING *thing);
extern LWORD CharGetPeek(THING *thing);
extern LWORD CharGetSteal(THING *thing);
extern LWORD CharGetPickpocket(THING *thing);
extern LWORD CharGetAmbush(THING *thing); /* Get adjusted ambush skill */

/* Use this when a specific Get<SkillName> doesnt exist
 * I guess I should do a big switch lookup to the specific skill routine
 * to simplify this, would be slower though... hmmm */
extern LWORD CharGetSkill(THING *thing, LWORD i); 
extern LWORD CharGetHitBonus(THING *thing, THING *weapon);
extern LWORD CharGetDamBonus(THING *thing, THING *weapon);
extern LWORD CharWillPowerCheck(THING *thing, LWORD bonus);
extern LWORD CharDodgeCheck(THING *thing, LWORD bonus);
extern LWORD CharMoveCostAdjust(THING *thing, LWORD cost);
#define GAIN_HITP   0 
#define GAIN_MOVEP  1
#define GAIN_POWERP 2
#define GAIN_HUNGER 3
#define GAIN_THIRST 4
extern LWORD  CharGainAdjust(THING *thing, BYTE gainType, LWORD gain);
extern void   CharShowHealth(THING *show, THING *thing);
extern void   CharShowEquip(THING *show, THING *thing);
extern LWORD  CharCanCarry(THING *thing, THING *object);
extern LWORD  CharGetCarryMax(THING *thing);
extern LWORD  CharTick(THING *thing);
extern LWORD  CharFastTick(THING *thing);
extern THING *CharThingFind(THING *thing, BYTE *key, LWORD virtual, THING *search, FLAG findFlag, LWORD *offset);

#define Character(x) ((CHARACTER*)(x))
