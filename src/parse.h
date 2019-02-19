
#define LOG_NEVER    0
#define LOG_NORMAL   1
#define LOG_ALLWAYS  2

/* Command Flags */
#define CF_MOVE         1<<0    /* movement related commands */
#define CF_TALK         1<<1    /* talk, gossip etc */
#define CF_MISC         1<<2    /* Quit and the like */
#define CF_GOD          1<<3    /* God-only/Admin commands */
#define CF_AREAEDIT     1<<4    /* check editing command auth */
#define CF_INV          1<<5    /* inventory, get drop etc related commands */
#define CF_CBT          1<<6    /* combat related commands, including cast, follow, group */
#define CF_HELPEDIT     1<<7    /* Help Editing commands */
#define CF_AREA         1<<8    /* Area commands */
#define CF_WLD          1<<9    /* WLD commands */
#define CF_MOB          1<<10   /* MOB commands */
#define CF_OBJ          1<<11   /* OBJ commands */
#define CF_RST          1<<12   /* RST commands */
#define CF_BRD          1<<13   /* BRD commands */
#define CF_VNUMARG      1<<14   /* can also be a valid vnum arg for editing */
#define CF_C4CODING     1<<15   /* C4 commands */

struct CommandListType {
  BYTE    *cText;
  CMDPROC(*cProc);
  BYTE     cPos;
  BYTE     cLevel;
  BYTE     cLog;
  FLAG     cFlag;
};

extern BYTE *commandFlagList[];
extern const COMMANDLIST commandList[];

extern LWORD PARSE_COMMAND_OSTAT;
extern LWORD PARSE_COMMAND_WGOTO;
extern LWORD PARSE_COMMAND_WCREATE;
extern LWORD PARSE_COMMAND_MSTAT;
extern LWORD PARSE_COMMAND_RCREATE;
extern LWORD PARSE_COMMAND_WEXIT;

extern void  ParseInit(void);
extern BYTE *ParseFind(BYTE *cmd, BYTE *srcKey, LWORD *srcOffset, LWORD *srcNum, BYTE *dstKey, LWORD *dstOffset);
extern BYTE  ParseCommandCheck(LWORD i, SOCK *sock, BYTE *cmd);
extern void  ParseSock(SOCK *sock, BYTE *buf);
extern LWORD ParseCommand(THING *thing, BYTE *cmd);
extern LWORD ParseCommandStub(THING *event, BYTE *cmd);


