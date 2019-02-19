/* Inventory related commands */

#define MAX_EQUIP 3 /* max # of locations 1 item can take up */
struct WearListType {
  BYTE  *wName;
  FLAG   wEquipNot; /* wont equip if char has this location ie pants/tail */
  FLAG   wEquipList[MAX_EQUIP];
};

/* internal equipment position flags */
#define EQU_HEAD         1<<0  /* hat, headband */
#define EQU_NECK_1       1<<1  /* necklace, cloak */
#define EQU_NECK_2       1<<2
#define EQU_TORSO        1<<3  /* a shirt */
#define EQU_BODY         1<<4  /* trenchcoat, Vacc-Suit */
#define EQU_ARM_R        1<<5  /* sleeves, bracer */
#define EQU_ARM_L        1<<6
#define EQU_WRIST_R      1<<7  /* bracelet, watch */
#define EQU_WRIST_L      1<<8
#define EQU_HAND_R       1<<9  /* glove */
#define EQU_HAND_L       1<<10 /* glove */
#define EQU_HELD_R       1<<11 /* gun, almost anything really */
#define EQU_HELD_L       1<<12
#define EQU_FINGER_R     1<<13 /* a ring */
#define EQU_FINGER_L     1<<14
#define EQU_WAIST        1<<15 /* belt */
#define EQU_LEGS         1<<16 /* pants */
#define EQU_FEET         1<<17 /* shoes, boots */
#define EQU_TAIL         1<<18 /* sheathed around tail ie big glove */
#define EQU_TAILGRIP     1<<19 /* prehensile tail */
#define EQU_MAX          1<<19 /* equal to the last thing in the list */

extern struct WearListType  wearList[];
extern BYTE                *equipList[];

extern void InvApply(THING *thing, THING *equip);
extern BYTE InvEquip(THING *thing, THING *equip, BYTE *message);
#define IUE_NONBLOCKABLE   0
#define IUE_BLOCKABLE      1 
extern void InvUnapply(THING *thing, THING *equip);
extern BYTE InvUnEquip(THING *thing, THING *equip, BYTE blockable);
extern void InvEat(THING *thing, THING *found);
extern void InvDrink(THING *thing, THING *found);

extern CMDPROC(CmdGet);
extern CMDPROC(CmdDrop);
extern CMDPROC(CmdJunk);
extern CMDPROC(CmdPut);
extern CMDPROC(CmdGive);
extern CMDPROC(CmdEquip);
extern CMDPROC(CmdUnEquip);
extern CMDPROC(CmdInventory);
extern CMDPROC(CmdEat);
extern CMDPROC(CmdEmpty);
extern CMDPROC(CmdFill);
extern CMDPROC(CmdPour);
extern CMDPROC(CmdDrink);
extern CMDPROC(CmdRead);
extern CMDPROC(CmdEdit);
extern CMDPROC(CmdWrite);
extern CMDPROC(CmdReply);
extern CMDPROC(CmdErase);
extern CMDPROC(CmdUse);
