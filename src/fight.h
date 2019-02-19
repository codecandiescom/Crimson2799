#define FIGHT_MOVECOST 1

#define FIGHT_CORPSE_KEY "body remnants remains corpse %s"

/* Fight/Damage flags */
#define FD_PUNCTURE   1<<0
#define FD_SLASH      1<<1
#define FD_CONCUSSIVE 1<<2
#define FD_HEAT       1<<3
#define FD_EMR        1<<4
#define FD_LASER      1<<5
#define FD_PSYCHIC    1<<6
#define FD_ACID       1<<7
#define FD_POISON     1<<8

#define FR_PUNCTURE   0
#define FR_SLASH      1
#define FR_CONCUSSIVE 2
#define FR_HEAT       3
#define FR_EMR        4
#define FR_LASER      5
#define FR_PSYCHIC    6
#define FR_ACID       7
#define FR_POISON     8

#define FR_NONE       9

struct WeaponListType {
  BYTE *wName;
  BYTE  wStrBonus;
  FLAG  wDamage;
  WORD *wSkill;
  WORD *wFamily;
};

#define FM_DEFAULT    0    /* when there is nothing else */
#define FM_WEAPON  1024
#define FM_EFFECT  2048
#define FM_UNARMED 4096    /* so we can have claw, bite etc */
/* what gets stored in the msg Indexes */
typedef struct FightMsgType {
  WORD fmType;
  STR *fmSrc;
  STR *fmDst;
  STR *fmRoom;
} FIGHTMSG;
#define FightMsg(x) ((FIGHTMSG*)(x))

extern struct WeaponListType weaponList[];
extern BYTE  *resistList[];

extern INDEX fightIndex;

extern void  FightInit(void);
extern void  FightIdle(void);
extern void  FightStart(THING *thing, THING *target, BYTE range, EXIT *exit);
extern void  FightStop(THING *thing);
extern void  FightMove(THING *thing, EXIT *exit);
extern void  FightAttack(THING *thing);
extern void  FightReload(THING *thing);
extern LWORD FightDamagePrimitive(THING *thing, THING *target, LWORD damage);
extern LWORD FightDamage(THING *thing, LWORD damage, WORD hitNum, FLAG damFlag, WORD damType, BYTE *weaponName);
extern void  FightKill(THING *thing, THING *target);
