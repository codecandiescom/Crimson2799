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


#define OBJECT_ALLOC_SIZE 8192

/* Object Apply information */
#define OBJECT_MAX_APPLY      4

#define APPLY_NONE            0
#define APPLY_STR             1
#define APPLY_DEX             2
#define APPLY_INT             3
#define APPLY_WIS             4
#define APPLY_CON             5
#define APPLY_HIT             6
#define APPLY_MOVE            7
#define APPLY_AC              8
#define APPLY_HITROLL         9
#define APPLY_DAMROLL         10
#define APPLY_SPEED           11

struct ApplyType {
  BYTE                 aType;
  SBYTE                aValue;
};

struct ApplyListType {
  BYTE                *aName;
  WORD                 aEffect;
  FLAG                 aTarget;
};

/* *********************** */
/*                         */
/* Special Objects         */
/*                         */
/* *********************** */
#define OVIRTUAL_CORPSE           -2
#define OVIRTUAL_RESET            -3
#define OVIRTUAL_MONEY            -4
#define OVIRTUAL_PHANTOMPOCKET    -5
#define OVIRTUAL_FIREBLADE        -6
#define OVIRTUAL_FIRESHIELD       -7
#define OVIRTUAL_FIREARMOR        -8


/* *********************** */
/*                         */
/* Object Types/Field Data */
/*                         */
/* *********************** */

#define OTYPE_RESETCMD       0
#define OF_RESET_CMD         0
#define OF_RESET_IF          1
#define OF_RESET_ARG4        2
#define OF_RESET_ARG1        3
#define OF_RESET_ARG2        4
#define OF_RESET_ARG3        5

#define OTYPE_LIGHT          1
#define OF_LIGHT_INTENSITY   0
#define OF_LIGHT_AMMOTYPE    1
#define OF_LIGHT_AMMOUSE     2

#define OTYPE_SCANNER        2
#define OF_SCANNER_SFLAG     0
#define OSF_SCANBIO          1<<0
#define OSF_SCANCHIP         1<<1
extern BYTE                 *oSFlagList[]; /* container flags */
#define OF_SCANNER_AMMOTYPE  1
#define OF_SCANNER_AMMOUSE   2
#define OF_SCANNER_MAX       3
#define OF_SCANNER_BIO       4
#define OF_SCANNER_CHIP      5

#define OTYPE_DEVICE         3
#define OF_DEVICE_AMMOUSE    0
#define OF_DEVICE_AMMOTYPE   1

#define OTYPE_WEAPON         5
#define OF_WEAPON_WFLAG      0
#define OWF_UNDEF0           1<<0
extern BYTE                 *oWFlagList[]; 
/* see FD flags in fight.h */
/* uses resistList */
#define OF_WEAPON_DAMFLAG    1
#define OF_WEAPON_DIENUM     2
#define OF_WEAPON_FIRERATE   3
#define OF_WEAPON_DIESIZE    4
#define OF_WEAPON_AMMOUSE    5
#define OF_WEAPON_AMMOTYPE   6
typedef struct OAmmoType {
  BYTE *oAName;
  BYTE *oADesc;
} OAMMOTYPE;
extern OAMMOTYPE             oAmmoList[]; 
#define OF_WEAPON_RANGE      7
#define OF_WEAPON_TYPE       8

#define OTYPE_BOARD          6
#define OF_BOARD_BVIRTUAL    0

#define OTYPE_AMMO           7
/* uses oAmmoList */
#define OF_AMMO_AMMOTYPE     0
#define OF_AMMO_AMMOLEFT     1
#define OF_AMMO_HITBONUS     2
#define OF_AMMO_DAMBONUS     3
/* see FD flags in fight.h */
/* uses resistList */
#define OF_AMMO_DAMFLAG      4

#define OTYPE_ARMOR          9
#define OF_ARMOR_ARMOR       0
#define OF_ARMOR_RPUNCTURE   1
#define OF_ARMOR_RSLASH      2
#define OF_ARMOR_RCONCUSSIVE 3
#define OF_ARMOR_RHEAT       4
#define OF_ARMOR_REMR        5
#define OF_ARMOR_RLASER      6
#define OF_ARMOR_RPSYCHIC    7
#define OF_ARMOR_RACID       8
#define OF_ARMOR_RPOISON     9
#define OF_ARMOR_AMMOUSE     10
#define OF_ARMOR_AMMOTYPE    11

#define OTYPE_DRUG           10 
#define OF_DRUG_INTOX        0

#define OTYPE_CONTAINER      15
#define OF_CONTAINER_MAX     0
#define OF_CONTAINER_ROT     1
#define OF_CONTAINER_CFLAG   2
#define OCF_CLOSABLE         1<<0
#define OCF_PICKPROOF        1<<1
#define OCF_CLOSED           1<<2
#define OCF_LOCKED           1<<3
#define OCF_CORPSE           1<<4
#define OCF_ELECTRONIC       1<<5
#define OCF_PLAYERCORPSE     1<<6
#define OCF_PLAYERLOOTED     1<<7
extern BYTE                 *oCFlagList[]; /* container flags */
#define OF_CONTAINER_KEY     3
#define OF_CONTAINER_SVALUE  4

#define OTYPE_DRINKCON       17
#define OF_DRINKCON_MAX      0
#define OF_DRINKCON_CONTAIN  1
#define OF_DRINKCON_LIQUID   2
typedef struct OLiquidType {
  BYTE *oLName;
  BYTE *oLDesc;
  WORD  oLThirst;
  WORD  oLHunger;
  WORD  oLIntox;
  WORD  oLPoison;
  WORD  oLAcid;
} OLIQUIDTYPE;
extern OLIQUIDTYPE           oLiquidList[]; /* liquid types */
#define OF_DRINKCON_POISON   3 /* I'll attach this to liquid type, mostly for diku compat. */

#define OTYPE_KEY            18
#define OF_KEY_NUMBER        0

#define OTYPE_FOOD           19
#define OF_FOOD_HOWFILLING   0
#define OF_FOOD_POISON       1

#define OTYPE_MONEY          20
#define OF_MONEY_AMOUNT      0

#define OTYPE_EXIT           23
#define OF_EXIT_WVIRTUAL     0
#define OF_EXIT_ROT          1

#define OTYPE_VEHICLE        24
#define OF_VEHICLE_WVIRTUAL  0

/* Object type/field information */
#define OBJECT_MAX_FIELD 17
struct OTypeArrayType {
  BYTE                 **oList;
  WORD                   oSize;
};
struct OTypeListType {
  BYTE                  *oTypeStr;
  BYTE                  *oField[OBJECT_MAX_FIELD];
  BYTE                  *oFieldStr[OBJECT_MAX_FIELD];
  struct OTypeArrayType  oArray[OBJECT_MAX_FIELD];
};


struct ODetailType {
  /*
  SBYTE                bValue[16]; // exist logically only
  WORD                 wValue[8];  // exist logically only
  */
  LWORD                lValue[4];
};

struct ObjTemplateType {
  LWORD                oVirtual;

  STR                 *oKey;
  STR                 *oSDesc;
  STR                 *oLDesc;
  STR                 *oDesc;

  BYTE                 oCompile; /* if set auto compile on load */
  BYTE                 oType;
  FLAG                 oAct;
  WORD                 oWear;

  ODETAIL              oDetail;

  WORD                 oWeight;
  LWORD                oValue;
  LWORD                oRent;
  APPLY                oApply[OBJECT_MAX_APPLY];
  EXTRA               *oExtra;
  PROPERTY            *oProperty;

  LWORD                oOnline;
  LWORD                oOffline;
};


struct ObjType {
  BASE                 oBase;

  FLAG                 oEquip;
  OBJTEMPLATE         *oTemplate;

  FLAG                 oAct;
  ODETAIL              oDetail;
  APPLY                oApply[OBJECT_MAX_APPLY];
};

#define OA_GLOW            1<<0 /* brightens room */
#define OA_HUM             1<<1 /* makes noise, cant hide while equipped */
#define OA_DARK            1<<2 /* darkens room */
#define OA_GOOD            1<<3 /* what do these two do? */
#define OA_EVIL            1<<4
#define OA_INVISIBLE       1<<5
#define OA_AURA            1<<6 /* has a psionic aura */
#define OA_NODROP          1<<7
#define OA_BLESS           1<<8
#define OA_ANTI_GOOD       1<<9 /* the appropriate cant carry stuff */
#define OA_ANTI_EVIL       1<<10
#define OA_ANTI_NEUTRAL    1<<11
#define OA_HIDDEN          1<<12
#define OA_TRACK_OFFLINE   1<<13
#define OA_CARRY2USE       1<<14
#define OA_NODEATHTRAP     1<<15


extern LWORD                 objectNum;
extern INDEX                 objectIndex;
extern OBJTEMPLATE          *corpseTemplate;
extern OBJTEMPLATE          *resetTemplate;
extern OBJTEMPLATE          *moneyTemplate;
extern OBJTEMPLATE          *phantomPocketTemplate;
extern OBJTEMPLATE          *fireBladeTemplate;
extern OBJTEMPLATE          *fireShieldTemplate;
extern OBJTEMPLATE          *fireArmorTemplate;
extern OTYPELIST             oTypeList[];
extern BYTE                 *oActList[];
extern struct ApplyListType  applyList[];

extern void         ObjectInit(void);
extern void         ObjectRead(WORD area);
extern OBJTEMPLATE *ObjectOf(LWORD virtual);
extern void         ObjectWrite(WORD area);
extern              INDEXPROC(ObjectCompareProc);
extern              INDEXFINDPROC(ObjectFindProc);
extern THING       *ObjectAlloc(void);
extern THING       *ObjectCreate(OBJTEMPLATE *template, THING *within);
extern THING       *ObjectCreateMoney(LWORD amount, THING *within);
extern void         ObjectFree(THING *thing);

#define OP_INVENTORY (1<<0)
#define OP_EQUIPPED  (1<<1)
#define OP_ALL       (OP_INVENTORY|OP_EQUIPPED)

extern LWORD        ObjectPresent(OBJTEMPLATE *template, THING *within, FLAG presentFlag);
extern BYTE         ObjectMaxReached(OBJTEMPLATE *template, LWORD maxAllowed);
extern void         ObjectTick();
extern void         ObjectIdle(THING *object);
extern LWORD        ObjectGetFieldNumber(BYTE *key, BYTE oType);
extern LWORD        ObjectGetField(BYTE oType, ODETAIL *oDetail, WORD fieldNum);
extern void         ObjectSetField(BYTE oType, ODETAIL *oDetail, WORD fieldNum, LWORD value);
extern LWORD        ObjectGetFieldStr(BYTE oType, ODETAIL *oDetail, WORD fieldNum, BYTE *fieldStr, WORD maxLen);
extern LWORD        ObjectSetFieldStr(THING *thing, BYTE oType, ODETAIL *oDetail, WORD fieldNum, BYTE *fieldStr);
extern THING       *ObjectGetAmmo(THING *thing, LWORD *aType, LWORD *aUse, LWORD *aLeft);
extern void         ObjectUseAmmo(THING *thing);

extern void         ObjectShowLight(THING *show, THING *thing);
extern void         ObjectShowScanner(THING *show, THING *thing);
extern void         ObjectShowDevice(THING *show, THING *thing);
extern void         ObjectShowWeapon(THING *show, THING *thing);
extern BYTE         ObjectShowContainer(THING *show, THING *thing);
extern void         ObjectShowDrinkcon(THING *show, THING *thing);
extern void         ObjectShowArmor(THING *show, THING *thing);


#define             OBJECTGETFIELD(thing, fieldNum) ObjectGetField(Obj(thing)->oTemplate->oType, &Obj(thing)->oDetail, fieldNum)
#define             OBJECTSETFIELD(thing, fieldNum, fieldValue) ObjectSetField(Obj(thing)->oTemplate->oType, &Obj(thing)->oDetail, fieldNum, fieldValue)


#define Obj(x) ((OBJ*)(x))
#define ObjTemplate(x) ((OBJTEMPLATE*)(x))
