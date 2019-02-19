
#define BOARD_NAME_MAX   20
#define BOARD_NOTREPLY   -1

typedef struct BoardListType {
  BYTE   bFileName[BOARD_NAME_MAX];
  LWORD  bVirtual;
  STR   *bName;
  STR   *bEditor;
  FLAG   bFlag; /* add class specific etc flags l8r */
  WORD   bMax; /* max # of messages on this board */
  WORD   bMinLevel; /* Min level to read */
  INDEX  bIndex;
} BOARDLIST;

typedef struct BoardMsgType {
  STR     *bAuthor;
  STR     *bTitle;
  STR     *bText;
  time_t   bCreateTime;
  time_t   bLastRead;
  WORD     bReplyTo; /* -1 for not a reply */
  WORD     bSequence; /* sequence number */
  BYTE     bNotify;
} BOARDMSG;

#define BOARD_LIST_SIZE      1024
#define BOARDMSG_ALLOC_SIZE  256

#define B_UNSAVED            1<<0


/* list of all the boards */
extern BOARDLIST *boardList;
extern LWORD      boardListMax;
extern FLAG       boardListFlag;
extern BYTE      *bFlagList[];

extern INDEXPROC(BoardMsgCompare);

extern void      BoardInit(void);
extern void      BoardTableWrite(void);
extern void      BoardRead(BYTE board);
extern BYTE      BoardWrite(BYTE board);
extern LWORD     BoardCreate(BYTE *fileName, LWORD virtual);
extern LWORD     BoardOf(LWORD virtual);
extern BOARDMSG *BoardMsgCreate(BYTE board, THING *thing, BYTE *title, WORD replyTo);
extern void      BoardMsgDelete(LWORD board, LWORD msgNum);
extern void      BoardMsgDeleteCheck(BYTE board);
extern BYTE     *BoardMsgGetHeader(BYTE *buf, LWORD board, LWORD msgNum);
extern void      BoardIdle(void);
extern void      BoardShow(BYTE board, THING *thing);
extern void      BoardShowMessage(BYTE board, LWORD msgNum, THING *thing);

#define BoardMsg(x) ((BOARDMSG*)(x))
#define BOARDFREE(x) x=BoardFree(x)
