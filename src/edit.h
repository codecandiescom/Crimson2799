#define EP_ENDLF        1<<0  /* end with a trailing LF */
#define EP_ENDNOLF      1<<1  /* dont end with a trailing LF */
#define EP_ONELINE      1<<2  /* only one line allowed */
#define EP_IMMNEW       1<<3  /* immediately enter new/append mode */
#define EP_ALLOWESCAPE  1<<4  /* Allow $ escape sequences */

struct EditType {                
  STR               *eStr;       /* are we editing something */
  STR              **eStrP;      /* addr of the str field we're editing */
  STR               *eBuf;       /* buffer for changes */
  WORD               eSize;      /* size of edit buffer - static for a given edit */
  BYTE               eName[64];  /* what are we editing */
  FLAG              *eFlag;      /* pointer to flag to set once edit is done */
  FLAG               eBit;       /* bit to set when edit is done */
  FLAG               ePref;      /* editor config prefs */
  LWORD              eInsert;    /* offset we are currently inserting to */
  STR               *eClipBoard; /* persistent storage buffer for copy/paste */
};

/* externally available functions */
extern void   EditSendPrompt(SOCK *sock);
extern void   EditProcess(SOCK *sock, BYTE *arg);
extern void   EditCancel(SOCK *sock);
extern void   EditFree(SOCK *sock);

extern LWORD  EditStr(SOCK *sock, STR **str, LWORD size, BYTE *name, FLAG pref);
extern void   EditFlag(SOCK *sock, FLAG *flag, FLAG bit);
extern LWORD  EditProperty(THING *thing, BYTE *commandName, BYTE *cmd, BYTE *targetName, PROPERTY **pList);
extern LWORD  EditExtra(THING *thing, BYTE *commandName, BYTE *cmd, BYTE *targetName, EXTRA **eList);

typedef struct SetListType {
  BYTE   *sName;
  ULWORD  sOffset;
  ULWORD  sSize;
  ULWORD  sArray;
  LWORD   sArraySize;
  BYTE    sType;
} SETLIST;

#define SET_NUMERIC(base,field)      ( ((ULWORD)&(field)) - ((ULWORD)&(base)) ) , sizeof(field), (ULWORD)NULL, 0,              '\0'
#define SET_FLAG(base, field, array) ( ((ULWORD)&(field)) - ((ULWORD)&(base)) ) , sizeof(field), (ULWORD)array,sizeof(*array), 'F'
#define SET_TYPE(base, field, array) ( ((ULWORD)&(field)) - ((ULWORD)&(base)) ) , sizeof(field), (ULWORD)array,sizeof(*array), 'T'
#define SET_PROPERTYINT(base,field)  ( ((ULWORD)&(field)) - ((ULWORD)&(base)) ) , 0,             (ULWORD)NULL, 0,              'I'
#define SET_PROPERTYSTR(base,field)  ( ((ULWORD)&(field)) - ((ULWORD)&(base)) ) , 0,             (ULWORD)NULL, 0,              'S'

/* Set Call */
extern  LWORD EditSet(THING *thing, BYTE *cmd, void *set, BYTE *setName, SETLIST *setList);

#define EDITSTR(thing, str, size, name, pref) EditStr(BaseControlFind(thing), &str, size, name, pref)
#define EDITFLAG(thing, flag, bit) EditFlag(BaseControlFind(thing), flag, bit)
