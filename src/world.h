/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary, if you are using it without my express
 * permission you are violating the copyright act and can potentially be
 * sued.
 * That said, if you would like to use it, I'm not an ogre, gimme a call
 * and maybe we can work something out.
 *
 * Current email addresses: cryogen@unix.infoserve.net
                            rhaksi@freenet.vancouver.bc.ca
 * Phone number: (604) 591-9746
 */

struct WldType {
   THING      wThing;

   LWORD      wVirtual;
   WORD       wArea;
   FLAG       wFlag;
   WORD       wType;
   WORD       wLight;  /* this is the *EXTRA* light added by internal objs */
   EXIT      *wExit;
};

#define WT_INDOORS        0
#define WT_CITY           1
#define WT_FIELD          2
#define WT_FOREST         3
#define WT_HILLS          4
#define WT_MOUNTAIN       5
#define WT_WATERSWIM      6
#define WT_WATERNOSWIM    7
#define WT_UNDERWATER     8
#define WT_VACUUM         9
#define WT_DESERT         10
#define WT_ARCTIC         11
#define WT_ROAD           12
#define WT_TRAIL          13

#define WF_DARK           1<<0
#define WF_DEATHTRAP      1<<1
#define WF_NOMOB          1<<2
#define WF_NOWEATHER      1<<3
#define WF_NOGOOD         1<<4
#define WF_NONEUTRAL      1<<5
#define WF_NOEVIL         1<<6
#define WF_NOPSIONIC      1<<7
#define WF_SMALL          1<<8 /* only 3 people max */
#define WF_PRIVATE        1<<9 /* no snooping, cant teleport in etc */
#define WF_DRAINPOWER     1<<10
#define WF_VACUUM         1<<11
#define WF_NOTELEPORTOUT  1<<12
#define WF_NOTELEPORTIN   1<<13

#define WVIRTUAL_VEHICLE  -2

typedef struct WType {
  BYTE *wTName;
  WORD  wTMoveCost;
} WTYPE;
extern WTYPE  wTypeList[];
extern BYTE  *wFlagList[];

extern LWORD worldNum;
extern BYTE  worldReadLog; /* blow-by-blow */
extern BYTE  worldOfLog; /* blow-by-blow */

#define Wld(x) ((WLD*)(x))

extern void   WorldInit(void);
extern void   WorldRead(WORD area);
extern THING *WorldOf(LWORD virtual);
extern void   WorldReIndex(void);
extern void   WorldWrite(WORD area);
extern        INDEXPROC(WorldCompareProc);
extern        INDEXFINDPROC(WorldFindProc);

#define WORLD_ALLOC_SIZE 8192
