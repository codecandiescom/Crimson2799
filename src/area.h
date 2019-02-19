struct AreaListType {
  /* read from the tbl file */
  STR       *aFileName;      /* whats the file name for this area */
  LWORD      aVirtualMin;
  LWORD      aVirtualMax;    /* the basic virtual range */
  STR       *aEditor;        /* currently authorized editors */

  /* system info */
  FLAG       aStatus;        /* unsaved/scrambled etc nonsavable flags */
  INDEX      aWldIndex;
  INDEX      aMobIndex;
  INDEX      aObjIndex;

  LWORD      aOffset;        /* used to relocated an area at load-time */

  /* these are stored in the rst (reset) file for the area */
  STR       *aDesc;          /* writeup on the area, authors listed here */
  FLAG       aSystem;        /* system area flags, notably AS_SCRAMBLED */
  FLAG       aResetFlag;     /* reset (savable) flags, notably RF_CLOSED */
  WORD       aResetDelay;    /* how long between resets (minutes)*/
  ULWORD     aResetLast;     /* current age of this zone(minutes) */
  RESETLIST *aResetList;     /* command table for reset          */
  LWORD      aResetNum;      /* number of resetlist entries */
  LWORD      aResetByte;     /* memory allocated for resetlist entries */
  LWORD      aResetWait;     /* for slow resets - note implemented yet */
  THING      aResetThing;    /* For attaching areawide scripts to */
};

#define AS_WLDUNSAVED   1<<0
#define AS_MOBUNSAVED   1<<1
#define AS_OBJUNSAVED   1<<2
#define AS_RSTUNSAVED   1<<3
#define AS_RSTSHOWN     1<<4 /* if allready shown, cannot rshow */

extern BYTE *wSystemList[];

extern AREALIST *areaList;
extern LWORD     areaListMax;
extern BYTE      areaWriteBinary;

extern void  AreaInit(void);
extern WORD  AreaOf(WORD virtual);
extern BYTE  AreaHasPlayers(WORD area);
extern LWORD AreaMove(WORD area, LWORD virtual);
extern BYTE  AreaIsEditor(WORD area, THING *thing);
