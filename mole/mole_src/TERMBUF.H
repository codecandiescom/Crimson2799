// termbuf.h
// two-letter descriptor: tb
//
// This module provides a terminal buffer functionality

#ifndef _TERMBUF_H_
#define _TERMBUF_H_

/* general purpose macros */
#define TB_MAX(x,y) (x>y?x:y)
#define TB_MIN(x,y) (x<y?x:y)


/* character access definitions - see note below for more detail */
#define TB_CHARMASK       0x0FFL
#define TB_COLOURDEPTH    4     /* # colours = 1<<TB_COLOURDEPTH */
#define TB_FCOLOURMASK    (((1L<<TB_COLOURDEPTH)-1L)<<8)
#define TB_BCOLOURMASK    (((1L<<TB_COLOURDEPTH)-1L)<<(TB_COLOURDEPTH+8))
#define TB_EFFECTMASK     (0xFFFFFFFFL-TB_CHARMASK-TB_FCOLOURMASK-TB_BCOLOURMASK)
/* character access macros - see note below for more detail */
#define TB_MAKECHAR(x)    ((unsigned long)(x))
#define TB_MAKEFCOLOUR(x) (((unsigned long)(x))<<8)                    /* Foregnd colour */
#define TB_MAKEBCOLOUR(x) (((unsigned long)(x))<<(TB_COLOURDEPTH+8))   /* Backgnd colour */
#define TB_MAKEEFFECT(x)  (((unsigned long)(x))<<(TB_COLOURDEPTH*2+8)) /* effect bits (underline, bold, etc) */
#define TB_EXTCHAR(x)     (((unsigned long)(x)) & TB_CHARMASK)         /* EXTract CHARacter */
#define TB_EXTFCOLOUR(x)  ((((unsigned long)(x)) & TB_FCOLOURMASK)>>8)
#define TB_EXTBCOLOUR(x)  ((((unsigned long)(x)) & TB_BCOLOURMASK)>>(TB_COLOURDEPTH+8))
#define TB_EXTEFFECT(x)   ((((unsigned long)(x)) & TB_EFFECTMASK)>>(TB_COLOURDEPTH*2+8))

/* function Notify messages */
#define TBNOTIFY_SCROLL   0x0001  /* scroll occurred; P0=#lines scrolled */
#define TBNOTIFY_NEWCHAR  0x0002  /* new character; P0=startX, P1=startY, P2=endX, P3=endY - buffer coordinates */
#define TBNOTIFY_MOVECUR  0x0003  /* new cursor pos; P0=X, P1=Y */
#define TBNOTIFY_RESIZE   0x0004  /* screen has been resized: P0=newX, P1=newY */

#define TBTAB_NONE        0x00    /* no tab stop present         */
#define TBTAB_LEFTALIGN   0x01    /*             |hello there    */
#define TBTAB_RIGHTALIGN  0x02    /*  hello there|               */
#define TBTAB_CENTREALIGN 0x03    /*        hello|there          */

typedef struct TBTERMBUFtag {
	HGLOBAL tMemory;          /* memory handle for THIS STRUCTURE */
	HGLOBAL tBufferMemory;    /* memory handle for tBuffer */
	unsigned long *tBuffer;   /* our screen data (terminal buffer) */
	unsigned long *tTab;      /* tab stops - an "extra" line alloc'ed at the end of the tBuffer */
	long tTopY;        /* pointer to the "top" row - see discussion below */
	long tBufX,tBufY;  /* buffer dimentions */
	long tCurX,tCurY;  /* cursor position, relative to top left of tScr (active screen) */
	long tScrX,tScrY;  /* "active screen" dimentions */
  int  tScrollRegionActive; /* 0 if no scrolling region defined (ie: whole screen), otherwise non-zero */
	long tSRX1,tSRX2,tSRY1,tSRY2; /* scrolling region -- invalid unless tScrollRegionActive!=0 */
  long tCurrentAttribute; /* current colour/attribute word */
  long tHLStartX,tHLStartY; /* hilight start point */
  long tHLEndX,tHLEndY; /* hilight end point */
  COLORREF tColourBank[1<<TB_COLOURDEPTH];  /* colour cross reference */
  HPEN tColourPen[1<<TB_COLOURDEPTH];
	(*tNotifyFn)(unsigned long p_ID, unsigned int p_Operation, unsigned long p_P0, unsigned long p_P1, unsigned long p_P2, unsigned long p_P3); /* function called to notify of scrolling etc. */
  long tHTab; /* Horizontal Tab size */
  unsigned long tID;  /* for application's use - application-defined ID tag */
                      /* note this can be used in tNotifyFn to determine the ID of the terminal */
	long tSaveCurrentAttribute;
	long tSaveCurX,tSaveCurY;
  BOOL tAutoRightMargin;
} TBTERMBUF;

/* NOTE NOTE NOTE
 * Ok, let's review how this sucker works.
 *
 * First of all, there's the buffer. This is a wack of memory which includes
 * the current screen as well as the scrollback buffer. The visible or "active"
 * screen is merely a subset of the buffer. The active screen is the "VT100"
 * terminal; if you "clear the screen", you will set all the bytes in the
 * active screen to zero. This is to facilitate ansi codes which assume
 * a given screen size of X,Y (usually 80x25).
 *
 * The buffer starts at tBuffer and has tBufY rows of tBufX characters each.
 *
 * The location of the "active screen" is defined by tTopY. This is a Y offset
 * in "buffer coordinates" (ie: relative to tBuffer; tTopY=0 corresponds to the
 * row starting at tBuffer[0], tTopY=1 corresponds to the row starting at
 * tBuffer[tBufX), etc.) tTopY is the row number of the **TOP** of the screen.
 * At the same time, tScrX and tScrY are the X and Y size of the screen relative to
 * tTopY, assuming that tTopY points to the top left corner of the active screen.
 * confused yet? Ok, let's add this all up in a neat little diagram:
 *
 *   NOTE: "->" designates "points to"                      tBufX
 *                                                             |
 *               ______________________________________________V
 *     tBuffer->|                                              |
 *              |                                              |
 *              |      scrollback data                         |
 *              |                                              |
 *              |                                              |
 *              |                                              |
 *              |    tSRX1         tScrX                       |
 *              |        |  tSRX2      |                       |
 *              |________V____V________V                       |
 *       tTopY->|                      |                       |
 *              | active               |                       |
 *              | screen               |                       |
 *     tSRY1--->|        +----+        |                       |
 *              |        |    |<----------- Scrolling Region   |
 *     tSRY2--->|        +----+        |                       |
 *       tScrY->|______________________|                       |
 *              |                                              |
 *              |                                              |
 *              |                                              |
 *              |                                              |
 *              |                                              |
 *              |                                              |
 *       tBufY->|______________________________________________|
 *
 * So tBufX and tBufY define the absolute limits of our buffer, tBuffer.
 * tTopY defines the top of the screen, and can take any value from
 * 0 to tBufY. Note as tTopY gets close to the bottom, the screen wraps back
 * to the start, and the screen will actually be "split" - appearing partially
 * at the start, and partially at the end of the buffer.
 *
 * tScrX and tScrY are relative values. tScrY is the number of rows for the
 * "active" screen. If tScrY is set to 10, for example, then the active
 * screen spans from row (tTopY) to (tTopY+9). It's a similar deal for
 * tScrX, except tScrX will usually be equal to tBufX.
 * Note that if the cursor reaches the X position of tScrX, it will wrap
 * back around to column 0.
 *
 * tSR?? define the bounds of the current scrolling region. These are relative
 * values like tScrX and tScrY.
 *
 * tCurX and tCurY are "screen coordinate" values for the current cursor position.
 * tCurX=0 and tCurY=0 corresponds to the top left of the active screen.
 *
 * To help you with all this stuff, you'll find below a bunch of macros to
 * help coordinate your terminal access. For the most part, positive Y values
 * in the below macros will access the current screen, and negative Y values
 * will access the scrollback buffer. There are also macros to give you
 * the screen and buffer limits. Note that Y=0 corresponds to the top row
 * of the active screen, or tTopY.
 *
 * Now just a quick note on characters. Each character is composed of a long
 * unsigned integer. The LSB (8 bits) is the ANSI character. The rest are left
 * as general attribute bytes and can be customized for your particular needs.
 * To facilitate this, you'll find above Foreground, background, and effect
 * masks and generators. You can define how many bits you assign to your
 * colour bank and everything will adjust accordingly. The colour bank in
 * the TBTERMBUF structure is for your convenience; take it out if you don't
 * want to use it. It's intended to map the value found in tBuffer to the actual
 * character colour when printed to the screen. It's contained in the sctructure
 * so that it may be personalized for each terminal independently.
 *
 * And how about a note on hilighting. The values for tHLStartX etc follow
 * the same positive/negative coordinate system as the macros below. IE:
 * positive Y values correspond to rows within the active screen; negative
 * Y values correspond to rows within the scrollback buffer. Y=0 corresponds
 * to the top row of the active screen, or tTopY. With hilighting, it is assumed
 * that the END HIGHLIGHT POSITION corresponds to the LAST HILIGHT ELEMENT **PLUS ONE**.
 * Therefore, to set the hilight to null, set the start and end coords to
 * the same value.
 *
 * Well, I hope you have it, kuz I'm tired of typing. Good luck.
 */

/* TB_GETCHAR returns the unsigned long element at (x,y). This macro can
 * access both screen (y>=0) and scrollback (y<0) regions. Note it is assumed
 * y is between TB_GETYMIN() and TB_GETYMAX(). */
#define TB_GETCHAR(buf,x,y) \
	(*( ((buf->tTopY+y)>=buf->tBufY)? \
			(buf->tBuffer+x+(buf->tTopY+y-buf->tBufY)*buf->tBufX): \
			((buf->tTopY+y)<0)?(buf->tBuffer+x+(buf->tTopY+y+buf->tBufY)*buf->tBufX):\
			(buf->tBuffer+x+(buf->tTopY+y)*buf->tBufX) ))

/* TB_GET?MIN and TB_GET?MAX return absolute min/max values that you can then
 * use as x/y values. */
#define TB_GETYMIN(buf) (buf->tScrY-buf->tBufY)
#define TB_GETYMAX(buf) (buf->tScrY-1)
#define TB_GETXMIN(buf) (0)
#define TB_GETXMAX(buf) (buf->tBufX-1)

/* TB_IS_SCR_SPLIT() returns 1 if the current screen is split at the
 * wrap-around point in the circular buffer. Otherwise, it returns 0. */
#define TB_IS_SCR_SPLIT(buf) \
	((buf->tTopY+buf->tScrY)>=buf->tBufY)

/* TB_GETCHARADDR returns the address (pointer) of the element at (x,y). The
 * same restrictions apply as for TB_GETCHAR above. */
#define TB_GETCHARADDR(buf,x,y) \
	(((buf->tTopY+y)>=buf->tBufY)? \
		(buf->tBuffer+x+(buf->tTopY+y-buf->tBufY)*buf->tBufX): \
		((buf->tTopY+y)<0)?(buf->tBuffer+x+(buf->tTopY+y+buf->tBufY)*buf->tBufX):\
		(buf->tBuffer+x+(buf->tTopY+y)*buf->tBufX))

/* TB_SETATTR* set the current attribute colours and effect */
#define TB_SETATTRFCOLOUR(buf,x)  \
	(((buf->tCurrentAttribute|TB_FCOLOURMASK)^TB_FCOLOURMASK)|TB_MAKEFCOLOUR(x))
#define TB_SETATTRBCOLOUR(buf,x)  \
	(((buf->tCurrentAttribute|TB_BCOLOURMASK)^TB_BCOLOURMASK)|TB_MAKEBCOLOUR(x))
#define TB_SETATTREFFECT(buf,x)  \
	(((buf->tCurrentAttribute|TB_EFFECTMASK)^TB_EFFECTMASK)|TB_MAKEEFFECT(x))

/* TB_SETNOTIFYFN sets the notify (callback) function */
#define TB_SETNOTIFYFN(buf,fn) (buf->tNotifyFn=fn)

/* TB_SETHTAB sets the horizontal tab */
#define TB_SETHTAB(buf,x) (buf->tHTab=x)

/* Function pool */
TBTERMBUF *tbAllocTermBuf(unsigned long p_ID, long p_BufX,long p_BufY,long p_ScrX,long p_ScrY);
void tbFreeTermBuf(TBTERMBUF *p_TermBuf);
TBTERMBUF *tbReAllocTermBuf(TBTERMBUF *p_TermBuf,long p_BufX,long p_BufY,long p_ScrX,long p_ScrY);
void tbPutChar(TBTERMBUF *p_TermBuf, unsigned char p_char);
void tbPutCharReverse(TBTERMBUF *p_TermBuf, unsigned char p_char);
void tbSetFColour(TBTERMBUF *p_TermBuf, unsigned long p_colour);
void tbSetBColour(TBTERMBUF *p_TermBuf, unsigned long p_colour);
void tbSetEffect(TBTERMBUF *p_TermBuf, unsigned long p_effect);
void tbSetCursor(TBTERMBUF *p_TermBuf, unsigned long p_CurX, unsigned long p_CurY);
void tbClearScreen(TBTERMBUF *p_TermBuf);
//void tbCarridgeReturn(TBTERMBUF *p_TermBuf);
void tbLineFeed(TBTERMBUF *p_TermBuf);
void tbHTab(TBTERMBUF *p_TermBuf);
void tbBackSpace(TBTERMBUF *p_TermBuf);
void tbDelete(TBTERMBUF *p_TermBuf);
void tbCurUp(TBTERMBUF *p_TermBuf);
void tbCurDown(TBTERMBUF *p_TermBuf);
void tbClearLineX(TBTERMBUF *p_TermBuf,long p_Start, long p_End);
void tbCurRight(TBTERMBUF *p_TermBuf);
void tbCurLeft(TBTERMBUF *p_TermBuf);
void tbClearLine(TBTERMBUF *p_TermBuf);
void tbClearRight(TBTERMBUF *p_TermBuf);
void tbClearLeft(TBTERMBUF *p_TermBuf);
void tbScrollDown(TBTERMBUF *p_TermBuf);
void tbScrollUp(TBTERMBUF *p_TermBuf);
void tbInsertChar(TBTERMBUF *p_TermBuf,int p_Spaces);
void tbClearBelow(TBTERMBUF *p_TermBuf);
void tbClearAbove(TBTERMBUF *p_TermBuf);
void tbInsertLine(TBTERMBUF *p_TermBuf,long l_NumLines);
void tbDeleteLine(TBTERMBUF *p_TermBuf,long l_NumLines);
void tbMoveText(TBTERMBUF *p_TermBuf,long p_startX,long p_startY,long p_endX,long p_endY,long p_destX,long p_destY);
void tbCreateUnderscorePen(TBTERMBUF *p_TermBuf,int p_FontHeight);
void tbCursorSave(TBTERMBUF *p_TermBuf);
void tbCursorRestore(TBTERMBUF *p_TermBuf);
void tbTabSet(TBTERMBUF *p_TermBuf,int p_Type, int p_X);
void tbTabClearAll(TBTERMBUF *p_TermBuf);
void tbScrollRight(TBTERMBUF *p_TermBuf);
void tbScrollLeft(TBTERMBUF *p_TermBuf);
void tbHTabReverse(TBTERMBUF *p_TermBuf);
void tbLineFeedUp(TBTERMBUF *p_TermBuf);
int tbVerifyCoordinates(TBTERMBUF *p_TermBuf,int p_X,int p_Y);
void tbStartHilight(TBTERMBUF *p_TermBuf,int p_X,int p_Y);
void tbUpdateHilight(TBTERMBUF *p_TermBuf,int p_X,int p_Y);
void tbClearHilight(TBTERMBUF *p_TermBuf);
BOOL tbIsHilight(TBTERMBUF *p_TermBuf,int p_X,int p_Y);
int tbHilightSize(TBTERMBUF *p_TermBuf);
void tbCopyHilight(TBTERMBUF *p_TermBuf,unsigned char *p_buf);


#endif /* _TERMBUF_H_ */
