/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary, if you are using it without my express
 * permission you are violating the copyright act and can potentially be
 * sued.
 * That said, if you would like to use it, Im not an ogre, gimme a call
 * and maybe we can work something out.
 *
 * Current email address: cryogen@unix.infoserve.net
 * Phone number: (604) 591-9746
 */

struct MobTemplateType {
  LWORD                mVirtual;

  STR                 *mKey;
  STR                 *mSDesc;
  STR                 *mLDesc;
  STR                 *mDesc;

  BYTE                 mCompile; /* if set auto compile on load attempt */
  FLAG                 mAct;
  FLAG                 mAffect;
  WORD                 mAura;
  WORD                 mLevel;
  WORD                 mHitBonus;
  WORD                 mArmor;
  WORD                 mHPDiceNum;
  WORD                 mHPDiceSize;
  WORD                 mHPBonus;
  WORD                 mDamDiceNum; /* unarmed damage */
  WORD                 mDamDiceSize;
  WORD                 mDamBonus;
  LWORD                mMoney;
  LWORD                mExp;
  BYTE                 mPos; /* Position default */
  BYTE                 mType;/* Type information ie monster race */
  BYTE                 mSex;

  WORD                 mWeight;

  EXTRA               *mExtra;
  PROPERTY            *mProperty;

  LWORD                mOnline;
};

struct MobType {
  CHARACTER            mCharacter;

  MOBTEMPLATE         *mTemplate; /* Template used */
  THING               *mTrack;    /* what are we chasing (if anything) */
};

extern LWORD        mobileNum;

extern INDEX        mobileIndex;
extern MOBTEMPLATE *spiritTemplate;

/* Special mobs */
#define MVIRTUAL_SPIRIT  -2

#define MOBILE_ALLOC_SIZE 8192

#define MACT_TRACKER     1<<0   /* Will chase people who flee */
#define MACT_SENTINEL    1<<1   /* wont move */
#define MACT_SCAVENGER   1<<2   /* will pick stuff up */
#define MACT_PICKPOCKET  1<<3   /* steals items */
#define MACT_NICE_THIEF  1<<4   /* wont attack a char that steals from it */
#define MACT_AGGRESSIVE  1<<5   /* Will initiate fights with players */
#define MACT_STAY_AREA   1<<6   /* Wont leave its area */
#define MACT_WIMPY       1<<7   /* Will flee if fights go badly */
#define MACT_HYPERAGGR   1<<8   /* will attack other mobs too */
#define MACT_RPUNCTURE   1<<9   /* Dont change these without changing CharResist */
#define MACT_RSLASH      1<<10  /* Dont change these without changing CharResist */
#define MACT_RCONCUSSIVE 1<<11  /* Dont change these without changing CharResist */
#define MACT_RHEAT       1<<12  /* Dont change these without changing CharResist */
#define MACT_REMR        1<<13  /* Dont change these without changing CharResist */
#define MACT_RLASER      1<<14  /* Dont change these without changing CharResist */
#define MACT_RPSYCHIC    1<<15  /* Dont change these without changing CharResist */
#define MACT_RACID       1<<16  /* Dont change these without changing CharResist */
#define MACT_RPOISON     1<<17  /* Dont change these without changing CharResist */
#define MACT_RESCUE      1<<18  /* Autorescue group members */

typedef struct mType {
  BYTE *mTName;
  FLAG  mTFlag;
} MTYPE;

#define MT_ROBOT     1<<0
#define MT_ORGANIC   1<<1
#define MT_PLANT     1<<2
#define MT_ANIMAL    1<<3
#define MT_SENTIENT  1<<4 
#define MT_HASMONEY  1<<5

extern MTYPE  mTypeList[];
extern BYTE  *mActList[];

extern void         MobileInit(void);
extern void         MobileRead(WORD area);
extern MOBTEMPLATE *MobileOf(LWORD virtual);
extern void         MobileWrite(WORD area);
extern              INDEXPROC(MobileCompareProc);
extern              INDEXFINDPROC(MobileFindProc);
extern THING       *MobileAlloc(void);
extern THING       *MobileCreate(MOBTEMPLATE *template, THING *within);
extern void         MobileFree(THING *thing);
extern LWORD        MobilePresent(MOBTEMPLATE *template, THING *within);
extern void         MobileTick();
extern void         MobileIdlePrimitive(THING *mobile);
extern void         MobileIdle(THING *mobile);

#define Mob(x) ((MOB*)(x))
#define MobTemplate(x) ((MOBTEMPLATE*)(x))
