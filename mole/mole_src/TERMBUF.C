// termbuf.c
// two-letter descriptor: tb
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************
//
// This module provides a terminal buffer functionality

#include<windows.h>
#include<windowsx.h>
#include"debug.h"
#include"termbuf.h"

TBTERMBUF *tbAllocTermBuf(unsigned long p_ID,long p_BufX,long p_BufY,long p_ScrX,long p_ScrY) {
	TBTERMBUF *l_tBuf;
	unsigned long *l_Buf;
	HGLOBAL l_memory,l_bufmemory;
	long x,y;
	unsigned long data;
	int l_i;
//  int i,r,g,b,lvl;

	/* first, alloc some memory for this sucker */
	l_memory=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(TBTERMBUF));
	if (!l_memory)
		return NULL;
	l_tBuf=(TBTERMBUF*)GlobalLock(l_memory);
	if (!l_tBuf) {
		GlobalFree(l_memory);
		return NULL;
	}
	/* I alloc an extra 1K because if there are race-cases in the programming
	 * (which Winfuck is full of) then this may prevent us from spilling out
   * into the netherlands should a pointer go temporarily astray. */
	l_bufmemory=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(long)*p_BufX*(p_BufY+2)+1024);
	if (!l_bufmemory) {
		GlobalLock(l_memory);
		GlobalFree(l_memory);
		return NULL;
	}
	l_Buf=(unsigned long *)GlobalLock(l_bufmemory);
	if (!l_Buf) {
		GlobalLock(l_memory);
		GlobalFree(l_memory);
		GlobalFree(l_bufmemory);
		return NULL;
	}
	l_tBuf->tMemory=l_memory;
	l_tBuf->tBufferMemory=l_bufmemory;
	l_tBuf->tBuffer=l_Buf;
	l_tBuf->tTab=&(l_Buf[p_BufX*p_BufY]); /* tab stops are kept in an "extra" line at the end of the buffer */
	l_tBuf->tID=p_ID;
	l_tBuf->tTopY=0;
	l_tBuf->tBufX=p_BufX;
	l_tBuf->tBufY=p_BufY;
	l_tBuf->tCurX=0; /* set cursor to default at top-left */
	l_tBuf->tCurY=0;
	l_tBuf->tScrX=p_ScrX;
	l_tBuf->tScrY=p_ScrY;
	l_tBuf->tScrollRegionActive=0;
	l_tBuf->tSRX1=0;
	l_tBuf->tSRX2=p_ScrX;
	l_tBuf->tSRY1=0;
	l_tBuf->tSRY2=p_ScrY;
	l_tBuf->tCurrentAttribute=TB_MAKEFCOLOUR(7L)|TB_MAKEBCOLOUR(0L); /* grey on black */
	l_tBuf->tHLStartX=l_tBuf->tHLStartY=0L; /* tHLEnd is ONE MORE than last tHLStart, so this gives us a NULL hilight */
	l_tBuf->tHLEndX=l_tBuf->tHLEndY=0L; /* tHLEnd is ONE MORE than last tHLStart, so this gives us a NULL hilight */
	l_tBuf->tHTab=8;
	l_tBuf->tNotifyFn=NULL;
	l_tBuf->tAutoRightMargin=TRUE;

	/* create default RGB colour map */
	(l_tBuf->tColourBank)[ 0]=RGB(  0,  0,  0);
	(l_tBuf->tColourBank)[ 1]=RGB(128,  0,  0);
	(l_tBuf->tColourBank)[ 2]=RGB(  0,128,  0);
	(l_tBuf->tColourBank)[ 3]=RGB(128,128,  0);
	(l_tBuf->tColourBank)[ 4]=RGB(  0,  0,128);
  (l_tBuf->tColourBank)[ 5]=RGB(128,  0,128);
  (l_tBuf->tColourBank)[ 6]=RGB(  0,128,128);
  (l_tBuf->tColourBank)[ 7]=RGB(192,192,192);
  (l_tBuf->tColourBank)[ 8]=RGB(128,128,128);
  (l_tBuf->tColourBank)[ 9]=RGB(255,  0,  0);
  (l_tBuf->tColourBank)[10]=RGB(  0,255,  0);
  (l_tBuf->tColourBank)[11]=RGB(255,255,  0);
  (l_tBuf->tColourBank)[12]=RGB(  0,  0,255);
	(l_tBuf->tColourBank)[13]=RGB(255,  0,255);
  (l_tBuf->tColourBank)[14]=RGB(  0,255,255);
	(l_tBuf->tColourBank)[15]=RGB(255,255,255);

	for (l_i=0;l_i<(1<<TB_COLOURDEPTH);l_i++) {
		(l_tBuf->tColourPen)[l_i]=NULL;
	}



//  i=0;
//  for (lvl=128;lvl<256;lvl+=127) {
//    for (r=0;r<2;r++) {
//      for (g=0;g<2;g++) {
//        for (b=0;b<2;b++) {
//          (l_tBuf->tColourBank)[i]=RGB(r*lvl,g*lvl,b*lvl);
//          i++;
//        }
//      }
//    }
//  }

	/* clear the buffer of any & all data */
	data=TB_MAKEEFFECT(0)|TB_MAKEBCOLOUR(0)|TB_MAKEFCOLOUR(7)|TB_MAKECHAR(0x20);
	for (y=0;y<p_BufY;y++)
		for (x=0;x<p_BufX;x++)
			*(l_Buf+x+(y*p_BufX))=data;

	/* reset our tabs to the default - every 8 characters */
	for (l_i=0;l_i<p_BufX;l_i++) {
		if (l_i%8)
			(l_tBuf->tTab)[l_i]=TBTAB_NONE;
		else
			(l_tBuf->tTab)[l_i]=TBTAB_LEFTALIGN;
	}

	/* and we're done! */
	return l_tBuf;
}

void tbFreeTermBuf(TBTERMBUF *p_TermBuf) {
	HGLOBAL l_memory;
	int l_i;
	/* delete attached pens */
	if (p_TermBuf) {
		for (l_i=0;l_i<(1<<TB_COLOURDEPTH);l_i++) {
			if ((p_TermBuf->tColourPen)[l_i]) {
				DeleteObject((p_TermBuf->tColourPen)[l_i]);
			}
		}
		l_memory=p_TermBuf->tBufferMemory;
    GlobalUnlock(l_memory);
    GlobalFree(l_memory);
    l_memory=p_TermBuf->tMemory;
    GlobalUnlock(l_memory);
    GlobalFree(l_memory);
  }
}

/* adjusts the width/height of the terminal buffer */
/* If tScrX=tBufX, then this equality is preserved. Otherwise, tScrX is
 * left as-is (unless p_BufX<tScrX, in which case, tScrX is set to p_BufX) */
/**** THIS NEEDS REDOING ****/
//#pragma argsused
//void tbResizeScreen(TBTERMBUF *p_TermBuf, long p_ScrX, long p_ScrY) {
//  if (!p_TermBuf)
//    return;
//  p_TermBuf->tScrX=TB_MIN(p_TermBuf->tBufX,p_ScrX);
//  p_TermBuf->tScrY=TB_MIN(p_TermBuf->tBufY,p_ScrY);
//
//  p_TermBuf->tCurX=TB_MIN(p_TermBuf->tCurX,p_ScrX);
//  p_TermBuf->tCurY=TB_MIN(p_TermBuf->tCurY,p_ScrY);
//  if (p_TermBuf->tNotifyFn) {
//    (*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_RESIZE,p_TermBuf->tScrX,p_TermBuf->tScrY,0L,0L);
//    (*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,0L,0L);
//  }
//}

TBTERMBUF *tbReAllocTermBuf(TBTERMBUF *p_TermBuf,long p_BufX,long p_BufY,long p_ScrX,long p_ScrY) {
	HGLOBAL l_oldmem,l_newmem;
	unsigned long *l_new;
	long l_x,l_y;
	long xmin,xmax,ymin,ymax;
//  char l_buf[200];
	unsigned long l_data;

	if (!p_TermBuf) {
		return NULL;
	}
	if (p_BufX<p_ScrX)
		p_BufX=p_ScrX;
	if (p_BufY<p_ScrY)
		p_BufY=p_ScrY;

//  sprintf(l_buf,"RESIZE ENTER: Buf=(%i,%i) Scr=(%i,%i)    newBuf=(%i,%i) newScr=(%i,%i)",
//    p_TermBuf->tBufX,p_TermBuf->tBufY,
//    p_TermBuf->tScrX,p_TermBuf->tScrY,
//    p_BufX,p_BufY,p_ScrX,p_ScrY);
//  dbPrint(l_buf);

	if ((p_BufX!=p_TermBuf->tBufX)||(p_BufY!=p_TermBuf->tBufY)) {
		l_newmem=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(long)*p_BufX*(p_BufY+2)+1024);
		if (!l_newmem) {
			return p_TermBuf;
		}
		l_new=(unsigned long *)GlobalLock(l_newmem);
		if (!l_new) {
			GlobalFree(l_newmem);
			return p_TermBuf;
		}

		/* first, clear the new buffer */
		l_data=TB_MAKEEFFECT(0)|TB_MAKEBCOLOUR(0)|TB_MAKEFCOLOUR(7)|TB_MAKECHAR(0x20);
		for (l_y=0;l_y<(p_BufY+2);l_y++)
			for (l_x=0;l_x<p_BufX;l_x++)
				*(l_new+l_x+(l_y*p_BufX))=l_data;
		/* clear all tabs */
		for (l_x=0;l_x<p_BufX;l_x++)
			l_new[p_BufX*p_BufY+l_x]=TBTAB_NONE;

		 /* now copy data over from old buffer to new one */
		xmin=0;
		xmax=TB_MIN(p_TermBuf->tBufX,p_BufX);
		ymin=TB_MAX(p_TermBuf->tScrY-p_TermBuf->tBufY,p_ScrY-p_BufY);
		ymax=TB_MIN(p_TermBuf->tScrY,p_ScrY);
		for (l_y=ymin;l_y<ymax;l_y++) {
			for (l_x=xmin;l_x<xmax;l_x++) {
				l_data=TB_GETCHAR(p_TermBuf,l_x,l_y);
//        l_data=TB_MAKEEFFECT(0)|TB_MAKEBCOLOUR(0)|TB_MAKEFCOLOUR(7)|TB_MAKECHAR(0x20);
				if (l_y<0) {
					*(l_new+l_x+((p_BufY+l_y)*p_BufX))=l_data;
				} else {
					*(l_new+l_x+(l_y*p_BufX))=l_data;
				}
			}
		}
		/* reset our tabs to the default - every 8 characters */
		for (l_x=0;l_x<p_BufX;l_x++) {
			if (l_x%8)
				l_new[p_BufX*p_BufY+l_x]=TBTAB_NONE;
//				(l_tBuf->tTab)[l_i]=TBTAB_NONE;
			else
				l_new[p_BufX*p_BufY+l_x]=TBTAB_LEFTALIGN;
//				(l_tBuf->tTab)[l_i]=TBTAB_LEFTALIGN;
		}
		/* copy over tabs from old buffer to new one */
		for (l_x=xmin;l_x<xmax;l_x++) {
			l_new[p_BufX*p_BufY+l_x]=p_TermBuf->tTab[l_x];
		}
		l_oldmem=p_TermBuf->tBufferMemory;
		/* and now let's swap in our new settings */
		p_TermBuf->tBufferMemory=l_newmem;
		p_TermBuf->tBuffer=l_new;
		p_TermBuf->tTab=&(l_new[p_BufX*p_BufY]);
		p_TermBuf->tTopY=0;
		p_TermBuf->tBufX=p_BufX;
		p_TermBuf->tBufY=p_BufY;
		/* ok, let's turf the old memory */
		GlobalUnlock(l_oldmem);
		GlobalFree(l_oldmem);
	}
	/* let's adjust tTopY so that our bottom line stays where it is. */
	p_TermBuf->tTopY+=p_TermBuf->tScrY-p_ScrY;
	if (p_TermBuf->tTopY<0)
		p_TermBuf->tTopY+=p_BufY;
	else if (p_TermBuf->tTopY>=p_BufY)
		p_TermBuf->tTopY-=p_BufY;
	/* and the cursor too; it's top-relative */
	p_TermBuf->tCurY-=p_TermBuf->tScrY-p_ScrY;

	p_TermBuf->tScrX=p_ScrX;
	p_TermBuf->tScrY=p_ScrY;
	if (p_TermBuf->tCurX>=p_ScrX)
		p_TermBuf->tCurX=p_ScrX-1;
	else if (p_TermBuf->tCurX<0)
		p_TermBuf->tCurX=0;
	if (p_TermBuf->tCurY>=p_ScrY)
		p_TermBuf->tCurY=p_ScrY-1;
	else if (p_TermBuf->tCurY<0)
		p_TermBuf->tCurY=0;
	p_TermBuf->tHLStartX=p_TermBuf->tHLStartY=0;
	p_TermBuf->tHLEndX=p_TermBuf->tHLEndY=0;

	if (p_TermBuf->tScrollRegionActive) {
		dbPrint("TB: Scroll Region not supported");
/*		if (p_TermBuf->tSRX1>p_TermBuf->tScrX)
			p_TermBuf->tSCRX1=p_TermBuf->tSCRX);
		if (p_TermBuf->tSRX2>p_TermBuf->tScrX)
			p_TermBuf->tSCRX2=p_TermBuf->tSCRX);
		if (p_TermBuf->tSRY1>p_TermBuf->tScrY)
			p_TermBuf->tSCRY1=p_TermBuf->tSCRY);
		if (p_TermBuf->tSRY2>p_TermBuf->tScrY)
			p_TermBuf->tSCRY2=p_TermBuf->tSCRY); */
	}

//  sprintf(l_buf,"RESIZE EXIT: Buf=(%i,%i) Scr=(%i,%i)    newBuf=(%i,%i) newScr=(%i,%i)",
//    p_TermBuf->tBufX,p_TermBuf->tBufY,
//    p_TermBuf->tScrX,p_TermBuf->tScrY,
//    p_BufX,p_BufY,p_ScrX,p_ScrY);
//  dbPrint(l_buf);


		return p_TermBuf;
}




/* tbPutChar writes the specified ASCII 8-bit character to the
 * terminal as would be expected, and does all attribute-assignment
 * and scrolling as required. */
void tbPutChar(TBTERMBUF *p_TermBuf, unsigned char p_char) {

	if (p_TermBuf) {
		if (p_TermBuf->tCurX>TB_GETXMAX(p_TermBuf)) {
			/* we are off the end of our line. Throw in a wrap-around here first */
			p_TermBuf->tCurX=0;
			/* and do a newline */
			tbLineFeed(p_TermBuf);
		}
		*TB_GETCHARADDR(p_TermBuf,p_TermBuf->tCurX,p_TermBuf->tCurY)=
			p_TermBuf->tCurrentAttribute|TB_MAKECHAR(p_char);
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,p_TermBuf->tCurX,p_TermBuf->tCurY,
				p_TermBuf->tCurX,p_TermBuf->tCurY);
	}
	/* now let's advance the cursor position etc. */
	/* first, check if we're inside the scrolling region */
	if (p_TermBuf->tScrollRegionActive) {
		dbPrint("TB: Scroll Region not supported ");
//	if ((p_TermBuf->tCurX>=p_TermBuf->tSRX1)&&
//			(p_TermBuf->tCurX<=p_TermBuf->tSRX2)&&
//			(p_TermBuf->tCurY>=p_TermBuf->tSRY1)&&
//			(p_TermBuf->tCurY<=p_TermBuf->tSRY2)) {
//		/* we are within the scroll region */
//		if (p_TermBuf->tCurX<p_TermBuf->tSRX2) {
//			/* Cursor moves right one */
//			p_TermBuf->tCurX++;
//	  } else {
//			/* line wrap */
//			p_TermBuf->tCurX=p_TermBuf->tSRX1;
//			/* and do a newline */
//			tbLineFeed(p_TermBuf);
//    }
//		return;
//  }
	}

	/* note what the following does.. it lets a character hitting the end of
	 * a line to actually "poke" through into an invalid area of the screen.
	 * This is to avoid wrapping until we for sure need to with the reception
	 * of the next character */
	p_TermBuf->tCurX++;

	/* fuck this is stupid. I actualy have a linux box where VI *requires* you
	 * to wrap at the end of a line BEFORE the next character. And I have other
	 * boxes which *require* you to *not* wrap until you've actually given a
	 * new character (which must then be wrapped onto next line). FUCK!!!!!!
	 * NO WAY OUT! */
	/* Ok, I've discovered (I think) this is the termcap entry auto_right_margin.
	 * it's a flag in the termcap/terminfo. Let's assume it's ON, but allow it
	 * to be turned OFF (by command line etc.) */
	/* I WOULD JUST LIKE TO SAY THAT programs that *depend* on shit like this
	 * should BE THROWN TO HELL WHERE THEY BELONG. FUCK YOU ALL! You don't know
	 * how to fucking program your way out of a binary fucking tree!!! FUCK!! */
	/* (as you can tell, I really don't think I should have to be doing this) */
	if (p_TermBuf->tAutoRightMargin) {
		if (p_TermBuf->tCurX>TB_GETXMAX(p_TermBuf)) {
			/* we are off the end of our line. Throw in a wrap-around here first */
			p_TermBuf->tCurX=0;
			/* and do a newline */
			tbLineFeed(p_TermBuf);
		}
	}
	if (p_TermBuf->tNotifyFn)
		(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,0L,0L);
}

/* tbPutChar writes the specified ASCII 8-bit character to the
 * terminal as would be expected, and does all attribute-assignment
 * and scrolling as required. */
void tbPutCharReverse(TBTERMBUF *p_TermBuf, unsigned char p_char) {
//  long i;
	if (p_TermBuf) {
		*TB_GETCHARADDR(p_TermBuf,p_TermBuf->tCurX,p_TermBuf->tCurY)=
			p_TermBuf->tCurrentAttribute|TB_MAKECHAR(p_char);
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,p_TermBuf->tCurX,p_TermBuf->tCurY,
				p_TermBuf->tCurX,p_TermBuf->tCurY);
	}
	/* now let's advance the cursor position etc. */
	/* first, check if we're inside the scrolling region */
	if (p_TermBuf->tScrollRegionActive) {
		dbPrint("TB: Scroll Region not supported ");
//	if ((p_TermBuf->tCurX>=p_TermBuf->tSRX1)&&
//			(p_TermBuf->tCurX<=p_TermBuf->tSRX2)&&
//			(p_TermBuf->tCurY>=p_TermBuf->tSRY1)&&
//			(p_TermBuf->tCurY<=p_TermBuf->tSRY2)) {
//		/* we are within the scroll region */
//		if (p_TermBuf->tCurX<p_TermBuf->tSRX2) {
//			/* Cursor moves right one */
//			p_TermBuf->tCurX++;
//	  } else {
//			/* line wrap */
//			p_TermBuf->tCurX=p_TermBuf->tSRX1;
//			/* and do a newline */
//			tbLineFeed(p_TermBuf);
//    }
//		return;
//  }
	}

	if (p_TermBuf->tCurX>0) {
		/* Cursor moves left one */
		p_TermBuf->tCurX--;
	} else {
		/* line wrap */
		p_TermBuf->tCurX=TB_GETXMAX(p_TermBuf);
		/* and do a newline */
		tbLineFeedUp(p_TermBuf);
	}
	if (p_TermBuf->tNotifyFn)
		(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,0L,0L);
}

void tbSetFColour(TBTERMBUF *p_TermBuf, unsigned long p_colour) {
	if (p_TermBuf) {
		p_TermBuf->tCurrentAttribute=TB_SETATTRFCOLOUR(p_TermBuf,p_colour);
	}
}

void tbSetBColour(TBTERMBUF *p_TermBuf, unsigned long p_colour) {
	if (p_TermBuf) {
		p_TermBuf->tCurrentAttribute=TB_SETATTRBCOLOUR(p_TermBuf,p_colour);
	}
}

void tbSetEffect(TBTERMBUF *p_TermBuf, unsigned long p_effect) {
	if (p_TermBuf) {
		p_TermBuf->tCurrentAttribute=TB_SETATTREFFECT(p_TermBuf,p_effect);
	}
}

void tbSetCursor(TBTERMBUF *p_TermBuf, unsigned long p_CurX, unsigned long p_CurY) {
	if (p_TermBuf) {
		p_TermBuf->tCurX=TB_MIN(TB_MAX(p_CurX,0),TB_GETXMAX(p_TermBuf));
		p_TermBuf->tCurY=TB_MIN(TB_MAX(p_CurY,0),TB_GETYMAX(p_TermBuf));
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,1L,0L);
	}
}

void tbClearScreen(TBTERMBUF *p_TermBuf) {
	long l_x,l_y;
	if (p_TermBuf) {
		for (l_y=0;l_y<=TB_GETYMAX(p_TermBuf);l_y++)
			for (l_x=0;l_x<=TB_GETXMAX(p_TermBuf);l_x++)
				*TB_GETCHARADDR(p_TermBuf,l_x,l_y)=p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,0L,0L,
				TB_GETXMAX(p_TermBuf),TB_GETYMAX(p_TermBuf));
		p_TermBuf->tCurX=p_TermBuf->tCurY=0;
	}
}

///* the following may not adhere to VT100 standards */
//void tbCarridgeReturn(TBTERMBUF *p_TermBuf) {
//	if (p_TermBuf) {
//		p_TermBuf->tCurX=0;
//		if (p_TermBuf->tNotifyFn)
//			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,0L,0L);
//	}
//}

void tbLineFeed(TBTERMBUF *p_TermBuf) {
	long l_i;

	if (p_TermBuf) {
		if (p_TermBuf->tScrollRegionActive) {
			dbPrint("TB: Scroll Region not supported ");
//		if ((p_TermBuf->tCurX>=p_TermBuf->tSRX1)&&
//				(p_TermBuf->tCurX<=p_TermBuf->tSRX2)&&
//				(p_TermBuf->tCurY>=p_TermBuf->tSRY1)&&
//				(p_TermBuf->tCurY<=p_TermBuf->tSRY2)) {
//			/* we're inside the scroll region */
//			if (p_TermBuf->tCurY<p_TermBuf->tSRY2) {
//				/* not at bottom yet.. just increment our cursor Y */
//				p_TermBuf->tCurY++;
//			} else {
//				tbScrollUp(p_TermBuf);
//			}
//			if (p_TermBuf->tNotifyFn)
//				(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,0L,0L);
//			}
		}
		if (p_TermBuf->tCurY<TB_GETYMAX(p_TermBuf)) {
			/* not at bottom yet.. just increment our cursor Y */
			p_TermBuf->tCurY++;
			l_i=1L;
		} else {
			tbScrollUp(p_TermBuf);
			l_i=0L;
		}
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,
				p_TermBuf->tCurX,p_TermBuf->tCurY,l_i,0L);
	}
}

void tbLineFeedUp(TBTERMBUF *p_TermBuf) {
	long l_i;
	if (p_TermBuf) {
		if (p_TermBuf->tCurY>0) {
			/* not at top yet.. just increment our cursor Y */
			p_TermBuf->tCurY--;
			l_i=1L;
		} else {
			tbScrollDown(p_TermBuf);
			l_i=0L;
		}
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,
			  p_TermBuf->tCurX,p_TermBuf->tCurY,l_i,0L);
	}
}

/* This does a scroll down operation - all text is moved down 1 line */
/* I could use a tbMoveText for this operation, but this function
 * uses the scrolling region (because it's full-screen) and is thus faster */
void tbScrollDown(TBTERMBUF *p_TermBuf) {
	long l_x,l_y;
	/* this isn't as easy as a ScrollUp because we can't just use
	 * the scrollback buffer; we have to actually move all our data
	 * down 1 line and redraw the screen */
	if (!p_TermBuf)
		return;

	if (p_TermBuf->tScrollRegionActive) {
		dbPrint("TB: Scroll Region not supported ");
/*		if ((p_TermBuf->tCurX>=p_TermBuf->tSRX1)&&
				(p_TermBuf->tCurX<=p_TermBuf->tSRX2)&&
				(p_TermBuf->tCurY>=p_TermBuf->tSRY1)&&
				(p_TermBuf->tCurY<=p_TermBuf->tSRY2)&&
				((p_TermBuf->tSRX1!=0)||
				 (p_TermBuf->tSRX2!=p_TermBuf->tScrX)||
				 (p_TermBuf->tSRY1!=0)||
				 (p_TermBuf->tSRY2!=p_TermBuf->tScrY) )) {

			return;
		}*/
	}

	/* copy all data down one line */
	for (l_y=TB_GETYMAX(p_TermBuf)-1;l_y>=0;l_y--) {
		for (l_x=0;l_x<=TB_GETXMAX(p_TermBuf);l_x++) {
			*TB_GETCHARADDR(p_TermBuf,l_x,(l_y+1))=*TB_GETCHARADDR(p_TermBuf,l_x,l_y);
		}
	}

	/* clear the top line */
	for (l_x=0;l_x<=TB_GETXMAX(p_TermBuf);l_x++) {
		*TB_GETCHARADDR(p_TermBuf,l_x,0)=p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
	}
	/* notify */
	if (p_TermBuf->tNotifyFn)
		(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_SCROLL,-1L,0L,0L,0L);
}

void tbScrollUp(TBTERMBUF *p_TermBuf) {
	long i;

	if (!p_TermBuf)
		return;

	/* Leave cursor Y alone and scroll screen 1 */
	if (p_TermBuf->tScrollRegionActive) {
		dbPrint("TB: Scroll Region not supported ");
//	if ((p_TermBuf->tCurX>=p_TermBuf->tSRX1)&&
//			(p_TermBuf->tCurX<=p_TermBuf->tSRX2)&&
//			(p_TermBuf->tCurY>=p_TermBuf->tSRY1)&&
//			(p_TermBuf->tCurY<=p_TermBuf->tSRY2)&&
//			((p_TermBuf->tSRX1!=0)||
//			 (p_TermBuf->tSRX2!=p_TermBuf->tScrX)||
//			 (p_TermBuf->tSRY1!=0)||
//			 (p_TermBuf->tSRY2!=p_TermBuf->tScrY) )) {
//		/* we are within the scroll region, AND our scroll region is NOT the full screen */
//		tbMoveText(TBTERMBUF *p_TermBuf,p_TermBuf->tSRX1,p_TermBuf->tSRY1+1,
//			p_TermBuf->tSRX2,p_TermBuf->tSRY2,p_TermBuf->tSRX1,p_TermBuf->tSRY1);
//		if (p_TermBuf->tNotifyFn)
//			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,
//				p_TermBuf->tSRX1,p_TermBuf->tSRY1,p_TermBuf->tSRX2,p_TermBuf->tSRY2);
//		return;
//		}
	}
	if (p_TermBuf->tTopY<(p_TermBuf->tBufY-1)) {
		/* not at end of the buffer yet... just move screen down 1 */
		p_TermBuf->tTopY++;
	} else {
		/* at the end of the buffer... wrap screen */
		p_TermBuf->tTopY=0;
	}
	/* Note, since we've moved our screen, we also have to move our hilight
	 * region (if there is one) */
	p_TermBuf->tHLStartY--;
	p_TermBuf->tHLEndY--;
	if (p_TermBuf->tHLStartY<TB_GETYMIN(p_TermBuf)) {
		p_TermBuf->tHLStartY=TB_GETYMIN(p_TermBuf);
		p_TermBuf->tHLStartX=0;
	}
	if (p_TermBuf->tHLEndY<TB_GETYMIN(p_TermBuf)) {
		p_TermBuf->tHLEndY=TB_GETYMIN(p_TermBuf);
		p_TermBuf->tHLEndX=0;
	}

	for (i=TB_GETXMIN(p_TermBuf);i<=TB_GETXMAX(p_TermBuf);i++) {
		*TB_GETCHARADDR(p_TermBuf,i,p_TermBuf->tCurY)=
			p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
	}
	if (p_TermBuf->tNotifyFn)
		(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_SCROLL,1L,0L,0L,0L);
}

void tbScrollLeft(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf)
		tbMoveText(p_TermBuf,1,0,TB_GETXMAX(p_TermBuf),TB_GETYMAX(p_TermBuf),0,0);
}

void tbScrollRight(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf)
		tbMoveText(p_TermBuf,0,0,TB_GETXMAX(p_TermBuf),TB_GETYMAX(p_TermBuf),1,0);
}

/* Clears to End Of Line with current attribute. Note if p_Start<0, the
 * current cursor position to EOL is cleared. If p_Start>=0, the erase
 * starts at p_Start */
void tbClearToEOL(TBTERMBUF *p_TermBuf,long p_Start) {
	long i;
	if (p_TermBuf) {
		if (p_Start<0)
			p_Start=p_TermBuf->tCurX;
		for (i=p_Start;i<=TB_GETXMAX(p_TermBuf);i++) {
			*TB_GETCHARADDR(p_TermBuf,i,p_TermBuf->tCurY)=
				p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
		}
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,
				p_Start,p_TermBuf->tCurY,TB_GETXMAX(p_TermBuf),p_TermBuf->tCurY);
	}
}
void tbClearLineX(TBTERMBUF *p_TermBuf,long p_Start, long p_End) {
	long i;
	if (p_TermBuf) {
		for (i=p_Start;i<=p_End;i++) {
			*TB_GETCHARADDR(p_TermBuf,i,p_TermBuf->tCurY)=
				p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
		}
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,
				p_Start,p_TermBuf->tCurY,p_End,p_TermBuf->tCurY);
	}
}
void tbClearRight(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		tbClearLineX(p_TermBuf,p_TermBuf->tCurX,TB_GETXMAX(p_TermBuf));
	}
}
void tbClearLeft(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		tbClearLineX(p_TermBuf,0,p_TermBuf->tCurX);
	}
}
void tbClearLine(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		tbClearLineX(p_TermBuf,0,TB_GETXMAX(p_TermBuf));
	}
}

/* tbHTab - advance to next horizontal tab position */
/* move to next tab position */
void tbHTab(TBTERMBUF *p_TermBuf) {
	int l_i;

	if (p_TermBuf) {
		l_i=p_TermBuf->tCurX;
		do {
			l_i++;
			if (l_i>TB_GETXMAX(p_TermBuf))
				l_i=0;
			if (p_TermBuf->tTab[l_i])
				break;
		} while(l_i!=p_TermBuf->tCurX);
		if ((l_i<=p_TermBuf->tCurX)&&(p_TermBuf->tTab[l_i])) {
			/* do line down */
			p_TermBuf->tCurX=l_i;
			tbLineFeed(p_TermBuf); /* will perform notify function */
			/* tbLineFeed should reposition the cursor for us */
		} else if (l_i>p_TermBuf->tCurX) {
			p_TermBuf->tCurX=l_i;
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,1L,0L);
		} /* else no tabs set -- do nothing */
	}
}

/* tbHTabReverse - advance to previous horizontal tab position */
void tbHTabReverse(TBTERMBUF *p_TermBuf) {
	int l_i;

	if (p_TermBuf) {
		l_i=p_TermBuf->tCurX;
		do {
			l_i--;
			if (l_i<0)
				l_i=TB_GETXMAX(p_TermBuf);
			if (p_TermBuf->tTab[l_i])
				break;
		}	while(l_i!=p_TermBuf->tCurX);
		if ((l_i>=p_TermBuf->tCurX)&&(p_TermBuf->tTab[l_i])) {
			/* do line up */
			p_TermBuf->tCurX=l_i;
			tbLineFeedUp(p_TermBuf); /* will perform notify function */
			/* tbLineFeed should reposition the cursor for us */
		} else if (l_i<p_TermBuf->tCurX) {
			p_TermBuf->tCurX=l_i;
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,1L,0L);
		} /* else no tabs set -- do nothing */
	}
}

/* tbBackSpace - go back without deleting or moving any characters */
void tbBackSpace(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		if (p_TermBuf->tCurX==0)
			return;
    p_TermBuf->tCurX-=1;
    if (p_TermBuf->tNotifyFn)
      (*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,p_TermBuf->tCurY,1L,0L);
  }
}

/* tbDelete - delete current character, dragging anything to the right over 1 */
void tbDelete(TBTERMBUF *p_TermBuf) {
	int l_i;

	if (p_TermBuf) {
		for (l_i=p_TermBuf->tCurX+1;l_i<=TB_GETXMAX(p_TermBuf);l_i++) {
			*TB_GETCHARADDR(p_TermBuf,l_i-1,p_TermBuf->tCurY)=
				*TB_GETCHARADDR(p_TermBuf,l_i,p_TermBuf->tCurY);
		}
		*TB_GETCHARADDR(p_TermBuf,TB_GETXMAX(p_TermBuf),p_TermBuf->tCurY)=
				p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
		if (p_TermBuf->tNotifyFn) {
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,
				p_TermBuf->tCurX,p_TermBuf->tCurY,
				TB_GETXMAX(p_TermBuf),p_TermBuf->tCurY);
		}
	}
}

void tbCurUp(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		if (p_TermBuf->tCurY>0) {
			p_TermBuf->tCurY--;
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,
				p_TermBuf->tCurY,1L,0L);
		}
	}
}

void tbCurDown(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		if (p_TermBuf->tCurY<TB_GETYMAX(p_TermBuf)) {
			p_TermBuf->tCurY++;
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,
				p_TermBuf->tCurY,1L,0L);
		}
	}
}

void tbCurRight(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		if (p_TermBuf->tCurX<TB_GETXMAX(p_TermBuf)) {
			p_TermBuf->tCurX++;
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,
				p_TermBuf->tCurY,1L,0L);
		}
	}
}

void tbCurLeft(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		if (p_TermBuf->tCurX>0) {
			p_TermBuf->tCurX--;
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_MOVECUR,p_TermBuf->tCurX,
				p_TermBuf->tCurY,1L,0L);
		}
	}
}

/* inserts p_Spaces blanks at the cursor's position, no wraparound */
void tbInsertChar(TBTERMBUF *p_TermBuf,int p_Spaces) {
	long i;
	if (p_TermBuf) {
		if (p_Spaces<0)
			p_Spaces=1;
		if (p_Spaces>TB_GETXMAX(p_TermBuf)-p_TermBuf->tCurX) {
			tbClearRight(p_TermBuf);
		} else {
			for (i=TB_GETXMAX(p_TermBuf)-p_Spaces;i>=p_TermBuf->tCurX;i--) {
				*TB_GETCHARADDR(p_TermBuf,i+p_Spaces,p_TermBuf->tCurY)=
				*TB_GETCHARADDR(p_TermBuf,i,p_TermBuf->tCurY);
			}
			for (i=p_TermBuf->tCurX;i<p_TermBuf->tCurX+p_Spaces;i++)
				*TB_GETCHARADDR(p_TermBuf,i,p_TermBuf->tCurY)=
          p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);

			if (p_TermBuf->tNotifyFn)
				(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,
					p_TermBuf->tCurX,p_TermBuf->tCurY,TB_GETXMAX(p_TermBuf),p_TermBuf->tCurY);
		}
	}
}

/* clears from cursor position to end of display, inclusive */
void tbClearBelow(TBTERMBUF *p_TermBuf) {
	long l_x,l_y;
	if (p_TermBuf) {
		for (l_y=p_TermBuf->tCurY;l_y<=TB_GETYMAX(p_TermBuf);l_y++) {
			if (l_y==p_TermBuf->tCurY)
				l_x=p_TermBuf->tCurX;
			else
				l_x=0;

			for (;l_x<=TB_GETXMAX(p_TermBuf);l_x++)
				*TB_GETCHARADDR(p_TermBuf,l_x,l_y)=p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
		}
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,0L,p_TermBuf->tCurY,
			TB_GETXMAX(p_TermBuf),TB_GETYMAX(p_TermBuf));
	}
}

/* clears from cursor position to start of display, inclusive */
void tbClearAbove(TBTERMBUF *p_TermBuf) {
	long l_x,l_y;
	if (p_TermBuf) {
		for (l_y=0;l_y<=p_TermBuf->tCurY;l_y++) {
			if (l_y==p_TermBuf->tCurY)
				l_x=p_TermBuf->tCurX;
			else
				l_x=TB_GETXMAX(p_TermBuf);
			for (;l_x>=0;l_x--)
				*TB_GETCHARADDR(p_TermBuf,l_x,l_y)=p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
		}
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,0L,0L,
				TB_GETXMAX(p_TermBuf),p_TermBuf->tCurY);
	}
}

/* Inserts p_NumLines lines at cursor position. Lines above bottom margin are lost. */
void tbInsertLine(TBTERMBUF *p_TermBuf,long l_NumLines){
	long l_i;
	if (p_TermBuf) {
		for (l_i=0;l_i<l_NumLines;l_i++) {
			tbMoveText(p_TermBuf,
				0,p_TermBuf->tCurY,                     /* start x,y */
				TB_GETXMAX(p_TermBuf),TB_GETYMAX(p_TermBuf),  /* end x,y */
				0,p_TermBuf->tCurY+1);                  /* destination start x,y */
		}
	}
}

/* Deletes p_NumLines lines at cursor position. Lines added at bottom are blank. */
void tbDeleteLine(TBTERMBUF *p_TermBuf,long l_NumLines){
	long l_i;
	if (p_TermBuf) {
		for (l_i=0;l_i<l_NumLines;l_i++) {
			tbMoveText(p_TermBuf,
				0,p_TermBuf->tCurY+1,                   /* start x,y */
				TB_GETXMAX(p_TermBuf),TB_GETYMAX(p_TermBuf),    /* end x,y */
				0,p_TermBuf->tCurY);                    /* destination start x,y */
		}
	}
}


/* This proc moves text from one place to another on the screen. It blanks the
 * source area with 0x20 (spaces) in the current background color */
/* note this proc DOES NOT pay ANY attention to the current scrolling region */
/* Furthermore, it can handle copying from and to regions outside the current screen */
void tbMoveText(TBTERMBUF *p_TermBuf,long p_startX,long p_startY,
	long p_endX,long p_endY,long p_destX,long p_destY) {
	long l_x,l_y,l_dstx,l_dsty,l_OffsetX,l_OffsetY;
	unsigned long l_Char,*l_pChar;
	long l_startX,l_startY,l_endX,l_endY,l_stepX,l_stepY;

	if (p_TermBuf) {
		/* let's make sure we go from top-left to bottom right */
		if (p_endX<p_startX) {
			l_x=p_startX;
			p_startX=p_endX;
			p_endX=l_x;
		}
		if (p_endY<p_startY) {
			l_y=p_startY;
			p_startY=p_endY;
			p_endX=l_y;
		}

		/* First, let's figure out the order in which we will be copying */
		if (p_startX<p_destX) {
			l_startX=p_endX;
			l_endX=p_startX;
			l_stepX=-1;
		} else {
			l_startX=p_startX;
			l_endX=p_endX;
			l_stepX=1;
		}
		l_OffsetX=p_destX-p_startX;
		if (p_startY<p_destY) {
			l_startY=p_endY;
			l_endY=p_startY;
			l_stepY=-1;
		} else {
			l_startY=p_startY;
			l_endY=p_endY;
			l_stepY=1;
		}
		l_OffsetY=p_destY-p_startY;

		for (l_y=l_startY;l_y!=(l_endY+l_stepY);l_y+=l_stepY) {
			for (l_x=l_startX;l_x!=(l_endX+l_stepX);l_x+=l_stepX) {
				/* first, let's get & clear our source */
				if ((l_y>=0)&&(l_y<=TB_GETYMAX(p_TermBuf))&&
						(l_x>=0)&&(l_x<=TB_GETXMAX(p_TermBuf))) {
					l_pChar=TB_GETCHARADDR(p_TermBuf,l_x,l_y);
					l_Char=*l_pChar;
					*l_pChar=p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
				} else {
					l_Char=p_TermBuf->tCurrentAttribute|TB_MAKECHAR(0x20);
				}
				/* next, let's save this char in it's destination spot */
				l_dstx=l_x+l_OffsetX;
				l_dsty=l_y+l_OffsetY;
				if ((l_dsty>=0)&&(l_dsty<=TB_GETYMAX(p_TermBuf))&&
						(l_dstx>=0)&&(l_dstx<=TB_GETXMAX(p_TermBuf))) {
					*TB_GETCHARADDR(p_TermBuf,l_dstx,l_dsty)=l_Char;
				}
			}
		}
		if (p_TermBuf->tNotifyFn)
			(*(p_TermBuf->tNotifyFn))(p_TermBuf->tID,TBNOTIFY_NEWCHAR,
				p_startX<p_destX?p_startX:p_destX,
				p_startY<p_destY?p_startY:p_destY,
				p_startX<p_destX?p_endX+l_OffsetX:p_endX,
        p_startY<p_destY?p_endY+l_OffsetY:p_endY);
	}
}

void tbCreateUnderscorePen(TBTERMBUF *p_TermBuf,int p_FontHeight) {
	int l_i,l_width;

	if (p_TermBuf) {
		l_width=(p_FontHeight+5)/10;
		if (!l_width)
			l_width=1;
		for (l_i=0;l_i<(1<<TB_COLOURDEPTH);l_i++) {
			if ((p_TermBuf->tColourPen)[l_i]) {
				DeleteObject((p_TermBuf->tColourPen)[l_i]);
			}
			(p_TermBuf->tColourPen)[l_i]=CreatePen(PS_SOLID,l_width,p_TermBuf->tColourBank[l_i]);
		}
	}
}

/* save cursor and attributes */
void tbCursorSave(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		p_TermBuf->tSaveCurrentAttribute=p_TermBuf->tCurrentAttribute;
		p_TermBuf->tSaveCurX=p_TermBuf->tCurX;
		p_TermBuf->tSaveCurY=p_TermBuf->tCurY;
	}
}

/* restore cursor and attributes */
void tbCursorRestore(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		p_TermBuf->tCurrentAttribute=p_TermBuf->tSaveCurrentAttribute;
		p_TermBuf->tCurX=p_TermBuf->tSaveCurX;
		p_TermBuf->tCurY=p_TermBuf->tSaveCurY;
	}
}

/* sets tabstop at specified position, or at current cursor position if p_X<1 */
void tbTabSet(TBTERMBUF *p_TermBuf,int p_Type, int p_X) {
	if (p_TermBuf) {
		if ((p_X>=0)&&(p_X<=(TB_GETXMAX(p_TermBuf)))) {
			p_TermBuf->tTab[p_X]=p_Type;
		} else {
			p_TermBuf->tTab[p_TermBuf->tCurX]=p_Type;
		}
	}
}

void tbTabClearAll(TBTERMBUF *p_TermBuf) {
	int l_i;
	if (p_TermBuf) {
		for (l_i=0;l_i<=TB_GETXMAX(p_TermBuf);l_i++) {
			p_TermBuf->tTab[l_i]=TBTAB_NONE;
		}
	}
}

void tbStartHilight(TBTERMBUF *p_TermBuf,int p_X,int p_Y) {
	if (p_TermBuf) {
		if (p_Y>TB_GETYMAX(p_TermBuf)) {
			p_Y=TB_GETYMAX(p_TermBuf);
			p_X=TB_GETXMAX(p_TermBuf);
		} else if (p_Y<TB_GETYMIN(p_TermBuf)) {
			p_Y=TB_GETYMIN(p_TermBuf);
			p_X=TB_GETXMIN(p_TermBuf);
		} else if (p_X>TB_GETXMAX(p_TermBuf)) {
			p_X=TB_GETXMAX(p_TermBuf);
		} else if (p_X<TB_GETXMIN(p_TermBuf)) {
			p_X=TB_GETXMIN(p_TermBuf);
		}
		p_TermBuf->tHLStartX=p_TermBuf->tHLEndX=p_X;
		p_TermBuf->tHLStartY=p_TermBuf->tHLEndY=p_Y;
	}
}

void tbUpdateHilight(TBTERMBUF *p_TermBuf,int p_X,int p_Y) {
	if (p_TermBuf) {
		if (p_Y>TB_GETYMAX(p_TermBuf)) {
			p_Y=TB_GETYMAX(p_TermBuf);
			p_X=TB_GETXMAX(p_TermBuf)+1;
		} else if (p_Y<TB_GETYMIN(p_TermBuf)) {
			p_Y=TB_GETYMIN(p_TermBuf);
			p_X=TB_GETXMIN(p_TermBuf);
		} else if (p_X>TB_GETXMAX(p_TermBuf)+1) {
			p_X=TB_GETXMAX(p_TermBuf)+1;
		} else if (p_X<TB_GETXMIN(p_TermBuf)) {
			p_X=TB_GETXMIN(p_TermBuf);
		}
		p_TermBuf->tHLEndX=p_X;
		p_TermBuf->tHLEndY=p_Y;
	}
}

void tbClearHilight(TBTERMBUF *p_TermBuf) {
	if (p_TermBuf) {
		p_TermBuf->tHLStartX=p_TermBuf->tHLStartY=0L; /* tHLEnd is ONE MORE than last tHLStart, so this gives us a NULL hilight */
		p_TermBuf->tHLEndX=p_TermBuf->tHLEndY=0L;     /* tHLEnd is ONE MORE than last tHLStart, so this gives us a NULL hilight */
	}
}

BOOL tbIsHilight(TBTERMBUF *p_TermBuf,int p_X,int p_Y) {
	if (p_TermBuf) {
		if (p_TermBuf->tHLStartY<p_TermBuf->tHLEndY) {
			/* we go from start, FORWARD to our end */
			if ((p_Y>p_TermBuf->tHLStartY) &&
					(p_Y<p_TermBuf->tHLEndY))
				return TRUE;
			if (p_Y==p_TermBuf->tHLStartY) {
				/* on starting line */
				if (p_X>=p_TermBuf->tHLStartX)
					return TRUE;
			} else if (p_Y==p_TermBuf->tHLEndY) {
				/* on ending line */
				if (p_X<p_TermBuf->tHLEndX)
					return TRUE;
			}
			return FALSE;
		} else if (p_TermBuf->tHLStartY>p_TermBuf->tHLEndY) {
			/* we go from start, BACKWARD to our end */
			if ((p_Y>p_TermBuf->tHLEndY) &&
					(p_Y<p_TermBuf->tHLStartY))
				return TRUE;
			if (p_Y==p_TermBuf->tHLEndY) {
				/* on starting line */
				if (p_X>=p_TermBuf->tHLEndX)
					return TRUE;
			} else if (p_Y==p_TermBuf->tHLStartY) {
				/* on ending line */
				if (p_X<p_TermBuf->tHLStartX)
					return TRUE;
			}
			return FALSE;
		} else if (p_TermBuf->tHLStartX<p_TermBuf->tHLEndX) {
			/* we go from start, FORWARD to our end */
			/* single line! */
			if ((p_Y==p_TermBuf->tHLStartY) &&
					(p_X >= p_TermBuf->tHLStartX) &&
					(p_X < p_TermBuf->tHLEndX) )
				return TRUE;
			return FALSE;
		} else if (p_TermBuf->tHLStartX>p_TermBuf->tHLEndX) {
			/* we go from start, BACKWARD to our end */
			/* single line! */
			if ((p_Y==p_TermBuf->tHLStartY) &&
					(p_X >= p_TermBuf->tHLEndX) &&
					(p_X < p_TermBuf->tHLStartX) )
				return TRUE;
			return FALSE;
		} else {
			/* no hilight */
			return FALSE;
		}
	}
	return FALSE;
}

/* returns the size, in bytes, the hilighted region would be expected to consume */
int tbHilightSize(TBTERMBUF *p_TermBuf) {
	int l_total;

	if (p_TermBuf) {
		if (p_TermBuf->tHLStartY<p_TermBuf->tHLEndY) {
			/* we go from start, FORWARD to our end */
			/* first, add in our full, margin-to-margin lines */
			l_total=(p_TermBuf->tHLEndY-p_TermBuf->tHLStartY-1)*(TB_GETXMAX(p_TermBuf)+1+2);
			/* next, our start line chunk */
			l_total+=(TB_GETXMAX(p_TermBuf)-p_TermBuf->tHLStartX+1);
			/* finally, our end line chunk */
			l_total+=p_TermBuf->tHLEndX;
			/* and allow for a few extra CRLF's */
			l_total+=6;
		} else if (p_TermBuf->tHLStartY>p_TermBuf->tHLEndY) {
			/* we go from start, BACKWARD to our end */
			/* first, add in our full, margin-to-margin lines */
			l_total=(p_TermBuf->tHLStartY-p_TermBuf->tHLEndY-1)*(TB_GETXMAX(p_TermBuf)+1+2);
			/* next, our start line chunk */
			l_total+=(TB_GETXMAX(p_TermBuf)-p_TermBuf->tHLEndX+1);
			/* finally, our end line chunk */
			l_total+=p_TermBuf->tHLStartX;
			/* and allow for a few extra CRLF's */
			l_total+=6;
		} else if (p_TermBuf->tHLStartX<p_TermBuf->tHLEndX) {
			/* we go from start, FORWARD to our end */
			/* single line! */
			l_total=p_TermBuf->tHLEndX-p_TermBuf->tHLStartX;
			/* and allow for a few extra CRLF's */
			l_total+=6;
		} else if (p_TermBuf->tHLStartX>p_TermBuf->tHLEndX) {
			/* we go from start, BACKWARD to our end */
			/* single line! */
			l_total=p_TermBuf->tHLStartX-p_TermBuf->tHLEndX;
			/* and allow for a few extra CRLF's */
			l_total+=6;
		} else {
			/* no hilight */
			l_total=0; /* a single 0x00 */
		}
	}
	return l_total;
}

/* copies hilighted region into buffer, inserting CRLF at the ends of lines */
void tbCopyHilight(TBTERMBUF *p_TermBuf,unsigned char *p_buf) {
	int l_sx,l_sy,l_ex,l_ey,l_x,l_y,l_char,l_CountEndX;
	unsigned char *l_p;

	if ((p_TermBuf)&&(p_buf)) {
		if (p_TermBuf->tHLStartY<p_TermBuf->tHLEndY) {
			/* we go from start, FORWARD to our end */
			l_sx=p_TermBuf->tHLStartX;
			l_sy=p_TermBuf->tHLStartY;
			l_ex=p_TermBuf->tHLEndX;
			l_ey=p_TermBuf->tHLEndY;
		} else if (p_TermBuf->tHLStartY>p_TermBuf->tHLEndY) {
			/* we go from start, BACKWARD to our end */
			l_sx=p_TermBuf->tHLEndX;
			l_sy=p_TermBuf->tHLEndY;
			l_ex=p_TermBuf->tHLStartX;
			l_ey=p_TermBuf->tHLStartY;
		} else if (p_TermBuf->tHLStartX<p_TermBuf->tHLEndX) {
			/* we go from start, FORWARD to our end */
			l_sx=p_TermBuf->tHLStartX;
			l_sy=p_TermBuf->tHLStartY;
			l_ex=p_TermBuf->tHLEndX;
			l_ey=p_TermBuf->tHLEndY;
		} else if (p_TermBuf->tHLStartX>p_TermBuf->tHLEndX) {
			/* we go from start, BACKWARD to our end */
			l_sx=p_TermBuf->tHLEndX;
			l_sy=p_TermBuf->tHLEndY;
			l_ex=p_TermBuf->tHLStartX;
			l_ey=p_TermBuf->tHLStartY;
		} else {
			/* no hilight */
      p_buf[0]=0;
			return;
		}
		l_p=p_buf;
		for (l_y=l_sy;l_y<=l_ey;l_y++) {

			if (l_y==l_sy)
				l_x=l_sx;
			else
				l_x=0;

			if (l_y==l_ey)
				l_CountEndX=l_ex;
			else
				l_CountEndX=TB_GETXMAX(p_TermBuf)+1;

			for (;l_x<l_CountEndX;l_x++) {
				/* copy this character */
				l_char=TB_GETCHAR(p_TermBuf,l_x,l_y);
				*l_p=(unsigned char)TB_EXTCHAR(l_char);
				l_p++;
			}
			/* As a courtesy, let's remove any trailing spaces */
			l_p--;
			while((l_p>=p_buf)&&(*l_p==' ')) {
				l_p--;
			}
      l_p++;
			/* end of line -- add CRLF */
			if (l_x>TB_GETXMAX(p_TermBuf)) {
				*l_p=0x0D;
				l_p++;
				*l_p=0x0A;
				l_p++;
			}
		}
		/* oh, and terminate the sucker. Forgot this the first time ;) */
		*l_p=0;
	}
	return;
}

