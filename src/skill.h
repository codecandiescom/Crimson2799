#define SF_WEAPON      1<<0
#define SF_MELEE       1<<1
#define SF_LASER       1<<2
#define SF_SLUG        1<<3
#define SF_BLASTER     1<<4
#define SF_ION         1<<5
#define SF_SUPPORT     1<<6
#define SF_PLASMA      1<<7
#define SF_OFFENSE     1<<8
#define SF_DEFENSE     1<<9
#define SF_PSYCHIC     1<<10
#define SF_BODY        1<<11
#define SF_TELEKINETIC 1<<12
#define SF_APPORTATION 1<<13
#define SF_SPIRIT      1<<14
#define SF_PYROKINETIC 1<<15
#define SF_TELEPATHY   1<<16
#define SF_GENERAL     1<<17 /* everyone gets */
#define SF_CLASS       1<<18 /* class specific */
#define SF_ALL         1<<19 /* so we can pr *all */
#define SF_GOOD        1<<20 /* so we can pr *all */
#define SF_EVIL        1<<21 /* so we can pr *all */


/*
   Notice that the first entry in a "category" is defined in relation to the category above,
   and that other entries in the same category are defined in terms of the first!
   I know it sounds like Greek right?, except this allows you to easily insert entries
   (without screwing up the players data either cuz its all text based not numeric)
   at the end of category where it logically makes sense rather than at the end of the
   entire list
*/

extern WORD SKILL_MELEE_ACCURACY;
extern WORD SKILL_MELEE_DAMAGE;
extern WORD SKILL_MELEE_SPEED;
#define SKILL_MELEE                SKILL_MELEE_ACCURACY

extern WORD SKILL_PISTOL_ACCURACY;
extern WORD SKILL_PISTOL_DAMAGE;
extern WORD SKILL_PISTOL_SPEED;
#define SKILL_PISTOL               SKILL_PISTOL_ACCURACY

extern WORD SKILL_RIFLE_ACCURACY;
extern WORD SKILL_RIFLE_DAMAGE;
extern WORD SKILL_RIFLE_SPEED;
#define SKILL_RIFLE                SKILL_RIFLE_ACCURACY

extern WORD SKILL_CANNON_ACCURACY;
extern WORD SKILL_CANNON_DAMAGE;
extern WORD SKILL_CANNON_SPEED;
#define SKILL_CANNON               SKILL_CANNON_ACCURACY

extern WORD SKILL_MELEE_KNIFE;
extern WORD SKILL_MELEE_BLADE;
extern WORD SKILL_MELEE_BLUDGEON;
extern WORD SKILL_MELEE_AXE;

extern WORD SKILL_LASER_PISTOL;
extern WORD SKILL_SLUG_PISTOL;
extern WORD SKILL_SLUG_MACHINE;
extern WORD SKILL_BLASTER_PISTOL;

extern WORD SKILL_LASER_RIFLE;
extern WORD SKILL_SLUG_RIFLE;
extern WORD SKILL_BLASTER_RIFLE;
extern WORD SKILL_ION_RIFLE;
extern WORD SKILL_GRENADE_RIFLE;
extern WORD SKILL_PLASMA_RIFLE;

extern WORD SKILL_ION_CANNON;
extern WORD SKILL_BLASTER_CANNON;
extern WORD SKILL_MISSILE_CANNON;
extern WORD SKILL_PLASMA_CANNON;

/* metabolic skills */
extern WORD SKILL_CELL_REPAIR;
extern WORD SKILL_REFRESH;
extern WORD SKILL_ENDURANCE;
extern WORD SKILL_BREATHWATER;
extern WORD SKILL_STRENGTH;
extern WORD SKILL_DARKVISION;
extern WORD SKILL_SLOW_POISON;
extern WORD SKILL_CURE_POISON;
extern WORD SKILL_HEAL_MINOR;
extern WORD SKILL_REGENERATION;
extern WORD SKILL_HEAL_MAJOR;
extern WORD SKILL_DEXTERITY;
extern WORD SKILL_CONSTITUTION;
extern WORD SKILL_HASTE;
extern WORD SKILL_QUENCH;
extern WORD SKILL_SUSTENANCE;
extern WORD SKILL_ACIDTOUCH;
extern WORD SKILL_POISONTOUCH;

/* telekinetic skills */
extern WORD SKILL_CRUSH;
extern WORD SKILL_FORCESTORM;
extern WORD SKILL_GHOSTFIST;
extern WORD SKILL_KINETIC_SHIELD;
extern WORD SKILL_IMMOBILIZE;
extern WORD SKILL_HEARTSTOP;
extern WORD SKILL_ASPHYXIATE;    /* create a limited vacuum */
extern WORD SKILL_INVISIBILITY;
extern WORD SKILL_SLOW;          /* opposite of haste */
extern WORD SKILL_IMPROVEDINVIS; /* doesnt wear off in attack */
extern WORD SKILL_VACUUMWALK;    /* walk in vacuum without damage */
/* extern WORD SKILL_KINETICWEAPON - weapon does concussive damage too */

/* telepathic skills */
extern WORD SKILL_PHANTOMEAR;
extern WORD SKILL_PHANTOMEYE;
extern WORD SKILL_MINDLINK;
extern WORD SKILL_DOMINATION;
extern WORD SKILL_THOUGHTBLADE;
extern WORD SKILL_MINDCRUSH;
extern WORD SKILL_DEATHDREAM;
extern WORD SKILL_MINDSHIELD;
extern WORD SKILL_SLEEP;
extern WORD SKILL_BERSERK;
extern WORD SKILL_MINDCLEAR;   /* remove intoxication */
/* extern WORD SKILL_IRONWILL        SKILL_SLEEP+1  increase willpower for awhile */
/* extern WORD SKILL_FORCE           SKILL_IRONWILL+1 order command */
/* extern WORD SKILL_FEEBLEMIND      SKILL_FORCE+1  harder to work psi powers */
/* extern WORD SKILL_CONFUSE         SKILL_FEEBLEMIND+1  reduce perception/guard */
/* extern WORD SKILL_COUNTERSTRIKE   attacker feels our pain (mind damage) */
/* extern WORD SKILL_MESSAGE         talk to sleeping people even */
/* extern WORD SKILL_SLEEPWATCH      talk/watch while "sleeping" */

/* apportation skills */
extern WORD SKILL_TELEPORT;      /* go somewhere */
extern WORD SKILL_SUMMON;        /* bring someone here */
extern WORD SKILL_SUCCOR;        /* go to someone */
extern WORD SKILL_BANISH;        /* bounce someone a few rooms away */
extern WORD SKILL_MASSTELEPORT;  /* everyone with me to dest */
extern WORD SKILL_MASSSUCCOR;    /* everyone to leader */
extern WORD SKILL_MASSSUMMON;    /* everyone to me */
extern WORD SKILL_DISRUPTDOOR;   /* destroy a teleportal */
extern WORD SKILL_PHASEWALK;     /* walk out of phase, no terrain costs */
extern WORD SKILL_PHANTOMPOCKET; /* can carry stuff in this doesnt weigh anything */
extern WORD SKILL_WATERWALK;     /* dont need a boat on water */
extern WORD SKILL_TELETRACK;     /* go to someone nearby */
extern WORD SKILL_RECALL;        /* go to marked location */
extern WORD SKILL_MARK;          /* mark this location */
extern WORD SKILL_TRANSLOCATE;   /* Swap goto marked location but mark this one */
extern WORD SKILL_PHASEDOOR;     /* Walk through closed door */
/* extern WORD SKILL_PHANTOMDOOR;   -* create a portal you can enter */
/* extern WORD SKILL_BLIT  go a few rooms in a direction */
/* extern WORD SKILL_TELESHIELD more than nosummon prevent people from coming */
/* extern WORD SKILL_FLICKER flicker in and out reduce damage by 25% */

/* Spirit skills */
extern WORD SKILL_SPIRITWALK;    /* set spirit free */
extern WORD SKILL_SENSELIFE;     
extern WORD SKILL_SEEINVISIBLE;  
extern WORD SKILL_LUCKSHIELD;    
extern WORD SKILL_IDENTIFY;      /* identify an object */
extern WORD SKILL_STAT;          /* stat a creature resist, hitroll etc */
extern WORD SKILL_LUCKYHITS;     /* hit bonus */
extern WORD SKILL_LUCKYDAMAGE;   /* damage bonus */
/* extern WORD SKILL_SECURE - make things hard to find/picklock */
/* extern WORD SKILL_PSI_RESIST - disrupt psionics */
/* extern WORD SKILL_PSI_IMMUNE - disrupt psionics completely */
/* extern WORD SKILL_LUCKYDODGE - improve dodge chances */
/* extern WORD SKILL_ESP - identify what someone is concentrating on */
/* extern WORD SKILL_LOCATE_OBJECT - locate some objects */
/* extern WORD SKILL_POWERSTEAL - steal power points */
/* extern WORD SKILL_TIMESTOP - decrement tWait, POWERFULL!! */

/* Pyrokinesis */
extern WORD SKILL_BURNINGFIST;   
extern WORD SKILL_FLAMESTRIKE;   
extern WORD SKILL_INCINERATE;    
extern WORD SKILL_IGNITE;        /* set off grenades */
extern WORD SKILL_HEATSHIELD;    
#define SKILL_FIRE               SKILL_FIRE_ACCURACY
extern WORD SKILL_FIRE_ACCURACY;
extern WORD SKILL_FIRE_DAMAGE;   
extern WORD SKILL_FIRE_SPEED;    
extern WORD SKILL_FIREBLADE;     /* make a burning sword we can wield */
extern WORD SKILL_FIRESHIELD;    /* make a burning shield we can wield */
extern WORD SKILL_FIREARMOR;     /* make burning armor we can wield */
/* extern WORD SKILL_FLAMEWEAPON    weapon is burning does heat damage too */
/* extern WORD SKILL_BURNINGAURA    every round attacker take heat damage */

extern WORD SKILL_FLEE;
extern WORD SKILL_PURSUE;
extern WORD SKILL_RESCUE;
extern WORD SKILL_PICKLOCK;
extern WORD SKILL_HACKING;
extern WORD SKILL_SEARCH;
extern WORD SKILL_CONCEAL;
extern WORD SKILL_HIDE;
extern WORD SKILL_DODGE;
extern WORD SKILL_WILLPOWER;        
extern WORD SKILL_TRACK;            
extern WORD SKILL_SNEAK;            
extern WORD SKILL_DISARM;           
extern WORD SKILL_AMBUSH;           
extern WORD SKILL_SPEED;         
extern WORD SKILL_PERCEPTION;    
extern WORD SKILL_PICKPOCKET;    
extern WORD SKILL_PEEK;          
extern WORD SKILL_STEAL;         
extern WORD SKILL_GUARD;

struct SkillListType {
  BYTE *sName;
  FLAG  sFlag;
  LWORD sLevel[4];
  LWORD sMax[4];
  LWORD sGain[4];
  WORD *sCheck; /* copy the number into this on startup so we can use it for constant */
};

#define SD_GETSKILL    TRUE
#define SD_MODIFYSKILL FALSE

struct SkillDetailType {
  WORD *sNum;
  WORD  sModifier; /* 0 is average, + is good, - is bad */
  BYTE  sGetSkill; /* if true they get the skill otherwise,
                      it only modifies the skill IF they get it */
};

struct RaceListType {
  BYTE                      *rName;
  WORD                       rResist[9];
  FLAG                       rSkillCant;
  FLAG                       rSkillGood;
  FLAG                       rSkillBad;
  WORD                       rMaxHunger;
  WORD                       rMaxThirst;
  WORD                       rMaxIntox;
  WORD                       rGainHunger;
  WORD                       rGainThirst;
  WORD                       rGainIntox;
  WORD                       rMaxHitP;
  WORD                       rMaxMoveP;
  WORD                       rMaxPowerP;
  WORD                       rGainHitP;
  WORD                       rGainMoveP;
  WORD                       rGainPowerP;
  WORD                       rMinStat[5];
  WORD                       rMaxStat[5];
  WORD                       rBonusStat[5];
  WORD                       rNormalize; /* final scores will average to this */
  WORD                       rGainLevel; /* percentage of normal advancement */
  SKILLDETAIL               *rSkill;
  FLAG                       rLocation;
};

struct ClassListType {
  BYTE                      *cName;
  FLAG                       cSkillCant;
  FLAG                       cSkillGood;
  FLAG                       cSkillBad;
  FLAG                       cSkillDefault; 
  WORD                       cMaxHitP;
  WORD                       cMaxMoveP;
  WORD                       cMaxPowerP;
  WORD                       cGainHitP;
  WORD                       cGainMoveP;
  WORD                       cGainPowerP;
  WORD                       cStartHitP;
  WORD                       cStartMoveP;
  WORD                       cStartPowerP;
  WORD                       cGainLevel; /* percentage of normal advancement */
  SKILLDETAIL               *cSkill;
  WORD                       cBonusStat[5];
};

#define SA_STR     0
#define SA_DEX     1
#define SA_CON     2
#define SA_WIS     3
#define SA_INT     4

extern BYTE                *skillFlagList[];
extern struct SkillListType skillList[];
extern WORD                 skillNum;
extern struct RaceListType  raceList[];
extern struct ClassListType classList[];

extern void  SkillInit(void);
extern void  SkillPractice(THING *thing, BYTE *cmd, FLAG canPractice, FLAG cantPractice, LWORD maxLevel);
extern void  SkillShow(THING *thing, BYTE *cmd, FLAG canPractice, FLAG cantPractice, LWORD maxLevel);



