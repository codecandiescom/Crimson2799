/* Help.h */

/* Helps are stored in HELPS in memory, diku servers
   actually load this at run time but since help entries
   are only a couple hundred K, big deal mise will just
   load it all in at boot time, can allways change it l8r
 */
typedef struct HelpListType {
  STR   *hName;
  STR   *hFileName;
  STR   *hEditor;
  WORD   hNum;
  FLAG   hFlag;
} HELPLIST;

typedef struct HelpType {
  STR *hKey;
  STR *hDesc;
  BYTE hSection;
} HELP;

#define HELP_TABLE_SIZE 128
#define HELP_INDEX_SIZE 256
#define HELP_ALLOC_SIZE 512

#define H_UNSAVED 1<<0
#define H_GODONLY 1<<1

extern HELPLIST *helpList;

/* Global List of all help entries for search purposes */
extern INDEX     helpIndex;
extern LWORD     helpListMax;
extern LWORD     helpListByte;

extern INDEXPROC(HelpMsgCompare);
extern INDEXFINDPROC(HelpMsgFind);

extern void  HelpInit(void);
extern void  HelpRead(BYTE section);
extern BYTE  HelpWrite(BYTE section);
extern HELP *HelpFind(BYTE *key);
extern HELP *HelpFree(HELP *help);
extern BYTE  HelpParse(THING *thing, BYTE *cmd, BYTE *defaultCmd);
extern INDEXPROC(HelpCompareProc);

#define Help(x) ((HELP*)(x))
#define HELPFREE(x) x=HelpFree(x);
