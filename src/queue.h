/* queue.h */
/* NOTE: This is not really a queue module, it uses a wrap-around text buffer
 to simulate a queue. and its SPECIFICALLY for use for the input/output command
 string buffers, it is NOT a general purpose Q module */

struct ColorListType {
  BYTE *cName;
  BYTE  cSymbol;
  BYTE  cIntensity;
  BYTE  cFG;
};

struct ColorPrefType {
  BYTE  cFG;
  BYTE  cBG;
};

struct QType {
  BYTE *qText;
  LWORD qValid; /* set to the # of full (valid) commands in the queue */
  LWORD qLen; /* total length of string in q */

  LWORD qStart; /* start index of string */
  LWORD qEnd; /* end index of string */
  LWORD qSize; /* mem allocated towards q buffer */
  
  BYTE  qIntensity;
  BYTE  qFG;
  BYTE  qBG;
};

extern COLORLIST ansiFGColorList[];
extern COLORLIST ansiBGColorList[];

#define COLORPREF_MAX 17 /* # of colors you can set prefs for */

extern COLORPREF defaultPref[COLORPREF_MAX];

extern void QInit(void);

/* create of queue of size qsize, NOTE: qSize should allways be a power of two */
extern Q    *QAlloc(LWORD qSize);

/* insert an entry into the front of a queue, sets valid and appends newline to str*/
/* meant to be called to reschedule allready processed commands ie synch spells to combat round */
extern void  QInsert(Q *q, BYTE *str);

#define Q_CHECKSTR     0
#define Q_DONTCHECKSTR 1

/* append an entry onto the end of a queue, sets valid as appropriate */
extern void  QAppend(Q *q, BYTE *str, BYTE mode);

#define Q_COLOR_IGNORE 0 
#define Q_COLOR_ANSI   1
#define Q_COLOR_STRIP  2

/* read an entry from the queue, it will be deleted in the process */
extern void  QRead(Q *q, BYTE *buf, LWORD bMaxLen, LWORD qType, COLORPREF *colorPref);
extern void  QReadByte(Q *q, BYTE *buf, LWORD bMaxLen, LWORD qType, LWORD numByte);

/* scan info from the q, doesnt affect its contents */
extern LWORD QScan(Q *q, LWORD qIndex, BYTE *buf, LWORD bLen);


extern void QFlush(Q *q);
extern Q    *QFree(Q *q);

/* macro to read from a q */
#define      QFREE(q)               q=QFree(q)
