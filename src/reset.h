/*
commands:
M - mobile to room              M <if> <mobile> <max>   <room>  [<room max>]
O - object to room              O <if> <object> <max>   <room>  [<room max>]
G - give object to last mobile  G <if> <object> <max>   
P - put object inside object    P <if> <object> <max>   <object>
E - equip mobile with an object E <if> <object> <max>   [<posi>] 
D - set door state              D <if> <room>   <exit>  <door_flags>
R - remove object from room     R <if> <room>   <object>

coming soon:
L - set lever state
*/

struct ResetListType {
  BYTE  rCmd;
  WORD  rIf;

  union rArg1Type {
    LWORD        rNum;
    MOBTEMPLATE *rMob;
    OBJTEMPLATE *rObj;
    STR         *rStr;
    THING       *rWld;
  } rArg1;

  union rArg2Type {
    LWORD        rNum;
  } rArg2;

  union rArg3Type {
    LWORD        rNum;
    THING       *rWld;
  } rArg3;

  union rArg4Type {
    LWORD        rNum;
  } rArg4;

};

#define RF_RESET_WHEN_EMPTY    1<<0 /* gets periodic resets */
#define RF_RESET_IMMEDIATELY   1<<1 /* reset even if players are inside */
#define RF_NOENTER             1<<2 /* cant enter here */
#define RF_NOTAKE              1<<3 /* cant take items from here */
#define RF_NOGAIN              1<<4 /* exp gain */
#define RF_NOMONEY             1<<5 /* money gain */
#define RF_NOTELEPORT_IN       1<<6 /* cant teleport in */
#define RF_NOTELEPORT_OUT      1<<7 /* cant teleport out */
#define RF_MOBFREEZE           1<<8 /* stops mobs from wandering */
#define RF_AREAISCLOSED        1<<9 /* area is closed - admin flag only */

extern BYTE *rFlagList[];
extern LWORD resetNum;  /* number of world structures in use - just curious really */

extern void ResetInit(void);
extern void ResetRead(WORD area);
extern void ResetWrite(WORD area);
extern void ResetReIndex(void);
extern void ResetArea(LWORD area);
extern void ResetAreaPrimitive(LWORD area);
extern void ResetAll(void);
extern void ResetIdle(void);
