// ansiterm.c
// two-letter descriptor: at
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************
//
// This module provides an ANSI colour terminal

/* ECMA-48 Implementation Notes
 * - Modes: EMCA-48 depreciates the use of modes. Consequently, the use of
 *   modes are not implemented. All modes are assumed to have their default value.
 * - It is unspecified what should happen when a LF is given at the bottom of
 *   the screen (or I'm missing something). Anyways, I coded it so that it scrolls
 *   the screen; I think everyone and their dog expects this anyways.
 * - FF is technically unimplemented; it does the same thing as an LF.
 * - Only align-left tabs are implemented. Affected operations: HT TAC TALE TATE TCC
 *   (and probably a few others)
 * - SIMD is implemented, but I'm not sure if I did it completely or correctly.
 *   Setting SIMD to 1 affects *ONLY* BS, CR, NEL, and the printing of characters.
 *   With SIMD=1, characters will be printed in a right-to-left progression, complete
 *   with wrap up and line-feeds when the cursor gets to the top of the screen.
 *   I am unsure if this is what ECMA-48 intended. Furthermore, I am unsure what
 *   other operations, if any, should be affected by SIMD.
 * - I'm not sure if CNL is supposed to generate linefeeds. Currently, it does.
 *   Same goes for CPL.
 * - I didn't have a *clue* what to do with F1-F12, INS, DEL, Home, End, PgUp,
 *   PgDn, or the cursor keys. So I guessed:
 *      INS  = ESC [ 2 ~         F1 = ESC [ [ A        F11= ESC [ 23 ~
 *      DEL  = DEL (0x7F)        F2 = ESC [ [ B        F12= ESC [ 24 ~
 *      Home = ESC [ H           F3 = ESC [ [ C
 *      End  = ESC [ 4 ~         F4 = ESC [ [ D
 *      PgUp = ESC [ 5 ~         F5 = ESC [ [ E
 *      PgDn = ESC [ 6 ~         F6 = ESC [ 17 ~
 *      Up   = ESC [ A           F7 = ESC [ 18 ~
 *      Down = ESC [ B           F8 = ESC [ 19 ~
 *      Right= ESC [ C           F9 = ESC [ 20 ~
 *      Left = ESC [ D           F10= ESC [ 21 ~
 *
 */


#include<windows.h>
#include<commdlg.h>
#include<windowsx.h>
#include<ctl3d.h>
#include"ctl3dl.h"
#include<stdio.h>
#include"molerc.h"
#include"termbuf.h"
#include"ansiterm.h"
#include"ansitbl.h"
#include"ascii_co.h"
#include"enviromt.h"
#include"mem.h"
#include"dialog.h"
#include"debug.h"

#define AT_ATOMSTR_HI "AnsiTermLong Hi"
#define AT_ATOMSTR_LO "AnsiTermLong Lo"
/* Globals */
UINT g_atCommDlgHelp;
ATOM g_atLongHi,g_atLongLo;

/* Macros (i hate them, I know, but...) for manipulating the input buffer */
#define ATIB_PREVCHAR(x,y)  ((y==x->aInputBuf)?(x->aInputBuf+x->aInputBufSize-1):(y-1))   /* x==ATTERM, y=cur pointer */
#define ATIB_NEXTCHAR(x,y)  ((y>=(x->aInputBuf+x->aInputBufSize-1))?(x->aInputBuf):(y+1)) /* x==ATTERM, y=cur pointer */
#define ATIB_ADDCHAR(x,y,z) ((y+z>=(x->aInputBuf+x->aInputBufSize-1))?(y+z-x->aInputBufSize):(y+z)) /* x==ATTERM, y=cur pointer, z=offset */
#define ATIB_SUBCHAR(x,y,z) ((y-z<x->aInputBuf)?(y-z+x->aInputBufSize):(y-z)) /* x==ATTERM, y=cur pointer, z=offset */

/* defines */
#define ATIB_FCOLOUR  7
#define ATIB_BCOLOUR  0

#define ATIB_MINX 20 /* minimum X window size (in characters) */
#define ATIB_MINY 1  /* minimum Y window size (in characters) */

/* The TermBuf doesn't work with a scrollback of 0, so this helps it along some */
#define ATIB_TERMBUF_EXTRALINES 5    /* this is not necessary if termbuf handled scrollback=0 */


BOOL atInitAnsiTermApplication(HINSTANCE p_hInst) {
	WNDCLASS l_wc;

	l_wc.style = CS_HREDRAW|CS_VREDRAW;
	l_wc.lpfnWndProc = atAnsiTermProc;
	l_wc.cbClsExtra = 0;
	l_wc.cbWndExtra = 4;
	l_wc.hInstance = p_hInst;
	l_wc.hIcon = LoadIcon(p_hInst, MAKEINTRESOURCE(ICON_ANSITERM));
	l_wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
	l_wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	l_wc.lpszMenuName =MAKEINTRESOURCE(MENU_ANSITERM);
	l_wc.lpszClassName = AT_WIND_CLASS_NAME;

	return (RegisterClass(&l_wc));
}

BOOL atInitAnsiTerm() {
	g_atLongHi=GlobalAddAtom(AT_ATOMSTR_HI);
	g_atLongLo=GlobalAddAtom(AT_ATOMSTR_LO);
	g_atCommDlgHelp=RegisterWindowMessage(HELPMSGSTRING);
	return TRUE;
}

void atShutdownAnsiTerm() {
	GlobalDeleteAtom(g_atLongHi);
	GlobalDeleteAtom(g_atLongLo);
	return;
}

ATTERM *atAllocTerm(HINSTANCE p_hInst, HWND p_hParent,long p_BufX,
	long p_BufY,long p_ScrX,long p_ScrY, char *p_EnvSection, char *p_Title) {
	ATTERM *l_Term;
	HGLOBAL l_memory;
	long l_i;

	/* first, let's try to alloc our structure */
	l_memory=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(ATTERM));
	if (l_memory) {
		l_Term=(ATTERM*)GlobalLock(l_memory);
		if (l_Term) {
			/* set up our values and defaults */
			l_Term->aMemory=l_memory;
			l_Term->aInst=p_hInst;
			l_Term->aEmulation=AT_EMU_VT100;
			l_Term->aSeparateInputLine=FALSE;
			l_Term->aShowX=l_Term->aShowY=0;
			l_Term->aState=g_atGroundTable;
			strncpy(l_Term->aEnvSection,p_EnvSection,TBSTR_ENVSECTION);
			l_Term->aEnvSection[TBSTR_ENVSECTION-1]=0; /* enforce terminator */
			l_Term->aFont=NULL;
			l_Term->aFontUnderline=NULL;
			l_Term->aLogFontValid=FALSE;
			atChangeFont(l_Term,NULL,NULL);
			l_Term->aInputBorderDY=3;
			l_Term->aInputFColour=ATIB_FCOLOUR;
			l_Term->aInputBColour=ATIB_BCOLOUR;
			strncpy(l_Term->aTitle,p_Title,TBSTR_TITLE_LEN);
			l_Term->aTitle[TBSTR_TITLE_LEN-1]=0; /* enforce terminator */
			l_Term->aLocalEcho=FALSE;
			l_Term->aLFwithNLMode=TRUE;
			l_Term->aBackSpaceChar=ASCII_BS;
			l_Term->aDeleteChar=ASCII_DEL;
			l_Term->aNotify=NULL;
			l_Term->aAuxMsgHandler=NULL;
			l_Term->aLE_BSCount=0L;
			l_Term->aBell=TRUE;
			l_Term->aScroll=0L;
			l_Term->aCharacterSet[0]='B';  /* ASCII_G		*/
			l_Term->aCharacterSet[1]='B';
			l_Term->aCharacterSet[2]='B';  /* DEC supplemental.	*/
			l_Term->aCharacterSet[3]='B';
			l_Term->aCurCharacterSet=0;

			/* ECMA-48 Codes and default Modes */
			l_Term->aSIMD=0;
			l_Term->aSLH=0;

			/* clear function key macros */
			for (l_i=0;l_i<12;l_i++)
				l_Term->aFnKeyMacro[l_i][0]=0;

			/* now we need a termbuf */
			l_Term->aTermBuf=tbAllocTermBuf((unsigned long)(l_Term),p_BufX,
				p_BufY+ATIB_TERMBUF_EXTRALINES,p_ScrX,p_ScrY);
			if (l_Term->aTermBuf) {
				/* and set up our termbuf */
				((void*)(l_Term->aTermBuf->tNotifyFn))=((void*)(atNotifyFn));

				/* now we need an output buffer */
				l_Term->aOutBufMemory=GlobalAlloc(GHND|GMEM_NOCOMPACT,8192);
				if (l_Term->aOutBufMemory) {
					l_Term->aOutBuf=(unsigned char*)GlobalLock(l_Term->aOutBufMemory);
					if (l_Term->aOutBuf) {
						l_Term->aOutBufSize=(unsigned long)GlobalSize(l_Term->aOutBufMemory);
						l_Term->aOutBufSockTail=l_Term->aOutBufHead=l_Term->aOutBuf;

						/* and we need a separate input line buffer */
						l_Term->aInputBufMemory=GlobalAlloc(GHND|GMEM_NOCOMPACT,8192);
						if (l_Term->aInputBufMemory) {
							l_Term->aInputBuf=GlobalLock(l_Term->aInputBufMemory);
							if (l_Term->aInputBuf) {
								l_Term->aInputBufSize=(unsigned long)GlobalSize(l_Term->aInputBufMemory);
								atResetSeparateInputLine(l_Term);

								/* now we need a window */
								l_Term->aWnd = CreateWindow(
									 AT_WIND_CLASS_NAME,
									 p_Title,
									 WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_VSCROLL,
									 CW_USEDEFAULT,
                   CW_USEDEFAULT,
									 CW_USEDEFAULT,
									 CW_USEDEFAULT,
									 p_hParent,
									 NULL,
									 p_hInst,
									 NULL
									 );
                if (l_Term->aWnd) {
                  /* set up our extra window memory */
#ifdef WIN32
                  SetWindowLong(l_Term->aWnd,ATWND_ATTERMSEG,(LONG)l_Term);
#else
									SetWindowLong(l_Term->aWnd,ATWND_ATTERMSEG,(LONG)(SELECTOROF(l_Term)));
									SetWindowLong(l_Term->aWnd,ATWND_ATTERMOFF,(LONG)(OFFSETOF(l_Term)));
#endif
                  SetWindowPlacement(l_Term->aWnd,enRestoreWindowPlacement(l_Term->aEnvSection,
                    GetSystemMetrics(SM_CXSCREEN)*1/10,GetSystemMetrics(SM_CYSCREEN)*1/10,
										GetSystemMetrics(SM_CXSCREEN)*7/10,GetSystemMetrics(SM_CYSCREEN)*6/10));
									atAdjustTerm(l_Term,-1); /* shows/hides scrollbars, edit, etc. */
									tbClearScreen(l_Term->aTermBuf);
									l_Term->aSLL=TB_GETYMAX(l_Term->aTermBuf);
									/* we're done and happy! */
									atResetTerm(l_Term);
									return l_Term;
								}
								GlobalUnlock(l_Term->aInputBufMemory);
							}
							GlobalFree(l_Term->aInputBufMemory);
						}
						GlobalUnlock(l_Term->aOutBufMemory);
					}
					GlobalFree(l_Term->aOutBufMemory);
				}
				tbFreeTermBuf(l_Term->aTermBuf);
			}
			GlobalUnlock(l_memory);
		}
		GlobalFree(l_memory);
	}
	return NULL;
}

void atFreeTerm(ATTERM *p_Term) {
	HGLOBAL l_memory;
	if (p_Term) {
		/* first, close the window */
		DestroyWindow(p_Term->aWnd);
		/* next, turf our termbuf */
		tbFreeTermBuf(p_Term->aTermBuf);
		/* lastly, get rid of our structures */
    l_memory=p_Term->aInputBufMemory;
		GlobalUnlock(l_memory);
		GlobalFree(l_memory);
    l_memory=p_Term->aOutBufMemory;
		GlobalUnlock(l_memory);
		GlobalFree(l_memory);
		l_memory=p_Term->aMemory;
		GlobalUnlock(l_memory);
		GlobalFree(l_memory);
		/* and we're done! */
	}
}

void atChangeFont(ATTERM *p_Term, HFONT p_font, LOGFONT *p_logfont) {
	HDC l_hDC;
	TEXTMETRIC l_tm;
	RECT l_rect;
	HFONT l_newfont;

	if (p_Term) {
		if (p_Term->aFont) {
			DeleteObject(p_Term->aFont);
		}
		if (p_Term->aFontUnderline) {
			DeleteObject(p_Term->aFontUnderline);
		}
		if (p_logfont) {
			l_newfont=CreateFontIndirect(p_logfont);
			if (l_newfont) {
				p_Term->aFont=l_newfont;
				meMemCpy(&(p_Term->aLogFont),p_logfont,sizeof(LOGFONT));
				p_Term->aLogFontValid=TRUE;
				if (p_logfont->lfUnderline) {
					p_Term->aFontUnderline=NULL;
				} else {
					p_logfont->lfUnderline=TRUE;
					l_newfont=CreateFontIndirect(p_logfont);
					p_Term->aFontUnderline=l_newfont;
				}
			} else {
				p_Term->aFont=GetStockObject(SYSTEM_FIXED_FONT);
				p_Term->aLogFontValid=FALSE;
				p_Term->aFontUnderline=NULL;
			}
		} else if (p_font) {
			/* This method is undesired because it leaves us with no aLogFont to save
			 * the font to disk for retrieval next time */
			/* IE: if you set the terminal with only p_font specified, you can't
			 * save the font settings. End of story. */
			/* Also, we then have to way to get underlined fonts, etc. */
			p_Term->aFont=p_font;
			p_Term->aLogFontValid=FALSE;
			p_Term->aFontUnderline=NULL;
		} else {
			p_Term->aFont=GetStockObject(SYSTEM_FIXED_FONT);
			p_Term->aLogFontValid=FALSE;
			p_Term->aFontUnderline=NULL;
		}
		if (p_Term->aFont) {
			l_hDC=GetDC(p_Term->aWnd);
			if (l_hDC) {
				SelectObject(l_hDC,p_Term->aFont);
				GetTextMetrics(l_hDC,&l_tm);
				ReleaseDC(p_Term->aWnd,l_hDC);
				p_Term->aFontX=l_tm.tmAveCharWidth;
				p_Term->aFontY=l_tm.tmHeight; //+l_tm.tmExternalLeading;
			}
		} else {
			p_Term->aFontX=p_Term->aFontY=1;
		}
		/* change the font in our input window too, and redefine its height */
		p_Term->aInputWndDY=p_Term->aFontY;
		/* now give the window a nudge to activate our resizing acgorithms etc. */
		GetWindowRect(p_Term->aWnd,&l_rect);
		MoveWindow(p_Term->aWnd,l_rect.left,l_rect.top,
			l_rect.right-l_rect.left+1,l_rect.bottom-l_rect.top+1,TRUE);
		InvalidateRect(p_Term->aWnd,NULL,TRUE);
		if (!p_Term->aFontX)
			p_Term->aFontX=2;
		if (!p_Term->aFontY)
			p_Term->aFontY=2;
		/* resize caret */
		DestroyCaret();
		CreateCaret(p_Term->aWnd, NULL, (p_Term->aFontX)>>1, p_Term->aFontY);
		atUpdateCaret(p_Term);
		/* resize underline pen */
		tbCreateUnderscorePen(p_Term->aTermBuf,p_Term->aFontY);
	}
}

/* This procedure changes all the internal ansiterm states to reflect the
 * current window dimentions */
void atAdjustTerm(ATTERM *p_Term,long p_Scrollback){
	RECT l_rect;
	int l_ScrX,l_ScrY;
	long l_BufY;

	if (p_Term) {
		GetClientRect(p_Term->aWnd,&l_rect);
		/* ok! This is how much space we have, minus our scroll bars. */
		if (p_Term->aSeparateInputLine) {
			l_ScrX=(l_rect.right-l_rect.left)/p_Term->aFontX;
			l_ScrY=(l_rect.bottom-l_rect.top-p_Term->aInputWndDY-p_Term->aInputBorderDY)/
				p_Term->aFontY;
		} else {
			l_ScrX=(l_rect.right-l_rect.left)/p_Term->aFontX;
			l_ScrY=(l_rect.bottom-l_rect.top)/p_Term->aFontY;
		}
		if (l_ScrX<ATIB_MINX)
			l_ScrX=ATIB_MINX;
		if (l_ScrY<ATIB_MINY)
			l_ScrY=ATIB_MINY;
		if (p_Scrollback>=0)
			l_BufY=l_ScrY+p_Scrollback;
		else
			l_BufY=l_ScrY+(p_Term->aTermBuf->tBufY-ATIB_TERMBUF_EXTRALINES-p_Term->aTermBuf->tScrY);
		p_Term->aTermBuf=tbReAllocTermBuf(p_Term->aTermBuf,l_ScrX,
			l_BufY+ATIB_TERMBUF_EXTRALINES,l_ScrX,l_ScrY);
		p_Term->aSizeX=p_Term->aTermBuf->tScrX;
		p_Term->aSizeY=p_Term->aTermBuf->tScrY;

		/* our scrolling? let's just reset it. */
		p_Term->aShowX=p_Term->aShowY=0;

		/* reposition scrollbars accordingly. */
		atAdjustScrollbar(p_Term);

		/* Redraw the entire box */
		InvalidateRect(p_Term->aWnd,NULL,TRUE);

		atUpdateTermTitle(p_Term,NULL);

		/* and notify our owner of any changes */
		if (p_Term->aNotify)
			(*(p_Term->aNotify))(p_Term,ATNOTIFY_WINDOWSIZE);
	}
}

void atUpdateTermTitle(ATTERM *p_Term,char *p_NewTitle) {
	char l_buf[TBSTR_TITLE_LEN+50];

	if (p_Term) {
		if (p_NewTitle)
			strcpy(p_Term->aTitle,p_NewTitle);

		/* set our title up appropriately */
		sprintf(l_buf,"%s (%ix%i)",p_Term->aTitle,p_Term->aSizeX,p_Term->aSizeY);
		SetWindowText(p_Term->aWnd,l_buf);
	}
}

void atAdjustScrollbar(ATTERM *p_Term) {
	/* this proc just repositions the scrollbars to where they
	 * should be according to the present terminal settings */
	if (p_Term) {
//    SetScrollRange(p_Term->aWnd,SB_HORZ,TB_GETXMIN(p_Term->aTermBuf),
//      TB_GETXMAX(p_Term->aTermBuf)-p_Term->aSizeX, FALSE);
//    SetScrollPos(p_Term->aWnd,SB_HORZ,p_Term->aSizeX,TRUE);
//    SetScrollRange(p_Term->aWnd,SB_VERT,TB_GETYMIN(p_Term->aTermBuf),
//      TB_GETYMAX(p_Term->aTermBuf)-p_Term->aSizeY,FALSE);
		SetScrollRange(p_Term->aWnd,SB_VERT,
			p_Term->aSizeY-p_Term->aTermBuf->tBufY+ATIB_TERMBUF_EXTRALINES,0,FALSE);
		SetScrollPos(p_Term->aWnd,SB_VERT,p_Term->aShowY,TRUE);
	}
}

void atShowTerm(ATTERM *p_Term) {
	if (p_Term)
		ShowWindow(p_Term->aWnd,SW_SHOWNORMAL);
}

void atHideTerm(ATTERM *p_Term) {
	if (p_Term)
		ShowWindow(p_Term->aWnd,SW_HIDE);
}

BOOL atIsTermVisible(ATTERM *p_Term) {
	if (p_Term)
		return IsWindowVisible(p_Term->aWnd);
	return FALSE;
}

int atSendTerm(void *p_TermA, unsigned char *p_Buf, int p_BufLen) {
	int l_byte,l_i,l_j;
	int l_result;
	long l_x,l_y;
#ifndef DB_DEBUG_NODEBUG
	char l_tBuf[80];
#endif
	unsigned long l_p1,l_p2,l_p3;
//	long l_bit;
	ATTERM *p_Term=(ATTERM*)p_TermA;

	if (!p_Term)
		return 0;

//	for (l_byte=0;l_byte<p_BufLen;l_byte++) {
//		sprintf(l_tBuf,"AT Rx: 0x%02X",p_Buf[l_byte]);
//		dbPrint(l_tBuf);
//	}
//

	if (p_Term->aEmulation==AT_EMU_NONE) {
		for (l_byte=0;l_byte<p_BufLen;l_byte++) {
			if ((p_Buf[l_byte]<0x20)||(p_Buf[l_byte]>=0x80)) {
				switch (p_Buf[l_byte]) {
					case 0x0A: // linefeed
						tbLineFeed(p_Term->aTermBuf);
						break;
//					case 0x0D: // carridge return
//						tbCarridgeReturn(p_Term->aTermBuf);
//						break;
					default:
						break;
				}
			} else {
				tbPutChar(p_Term->aTermBuf,p_Buf[l_byte]);
			}
		}
	} else if (p_Term->aEmulation==AT_EMU_VT100) {
		for (l_byte=0;l_byte<p_BufLen;l_byte++) {
			if (p_Buf[l_byte]==0x03) {
				dbPrint("Ctrl-C encountered!");
				continue;
			}
			l_result=(p_Term->aState)[p_Buf[l_byte]];
#ifndef DB_DEBUG_NODEBUG
//			if (l_result!=ATSTATE_PRINT) {
				sprintf(l_tBuf,"AT: (%02X) %s -> %s %s",(unsigned int)(p_Buf[l_byte]),
					p_Term->aState==g_atGroundTable?"Ground Table":
					p_Term->aState==g_atCSITable?"g_atCSITable":
					p_Term->aState==g_atEIGTable?"g_atEIGTable":
					p_Term->aState==g_atESCTable?"g_atESCTable":
					p_Term->aState==g_atIESTable?"g_atIESTable":
					p_Term->aState==g_atIGNTable?"g_atIGNTable":
					p_Term->aState==g_atSCRTable?"g_atSCRTable":
					p_Term->aState==g_atSCSTable?"g_atSCSTable":
					p_Term->aState==g_atCSI_SIBTable?"g_atCSI_SIBTable": "??WHAT??",
					g_atStateTable[l_result],
					((p_Term->aState!=g_atGroundTable)&&(l_result==ATSTATE_GROUND_STATE))?
						"(ignored)":"");
				dbPrint(l_tBuf);
//			}
#endif
			switch(l_result) {
				case ATSTATE_GROUND_STATE:
					/* exit ignore mode */
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_IGNORE_STATE:
					/* IES: ignore anything else */
					p_Term->aState=g_atIGNTable;
					break;
				case ATSTATE_IGNORE_ESC:
					/* IGN: escape */
					p_Term->aState=g_atIESTable;
					break;
				case ATSTATE_IGNORE:
					/* ignore character */
					break;
				case ATSTATE_PRINT:
					/* print character */
					/* we should be using l_Term->aCharacterSet[l_Term->aCurCharacterSet] */
					if (p_Term->aSIMD)
						tbPutCharReverse(p_Term->aTermBuf,p_Buf[l_byte]);
					else
						tbPutChar(p_Term->aTermBuf,p_Buf[l_byte]);
					break;
				case ATSTATE_BELL:
					if (p_Term->aBell) {
						MessageBeep(MB_OK);
					}
					break;
				case ATSTATE_BS:
					if (p_Term->aSIMD)
						tbCurRight(p_Term->aTermBuf);
					else
						tbCurLeft(p_Term->aTermBuf);
					atUpdateCaret(p_Term);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_DEL:
					/* this functionality has been removed from the standard. However,
					 * it's inclusion may be usefull in some situations. For any
					 * qualified shell, it should handle the echo and thus the
					 * delete key properly. However, for things like MUDs that
					 * use the delete key, and which DON'T echo, or for terminals
					 * that (falsely) expect backspace to ERASE the previous character
					 * (ie: not just move the cursor), this can be handy. Basically,
					 * it's a kludge. Furthermore, it doesn't really work properly;
					 * you can delete back past the start of an input prompt. If you are
					 * using a system where this is important, I suggest you use the
           * separate input line operation. 
					 */
					tbDelete(p_Term->aTermBuf);
          break;
				case ATSTATE_DCH:
					/* delete character */
					l_i=p_Term->aStateVal[0];
					if (l_i<1)
						l_i=1;
					for (;l_i;l_i--) {
						tbDelete(p_Term->aTermBuf);
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CR:
					if (p_Term->aSIMD)
						tbSetCursor(p_Term->aTermBuf,p_Term->aSLL,p_Term->aTermBuf->tCurY);
					else
						tbSetCursor(p_Term->aTermBuf,p_Term->aSLH,p_Term->aTermBuf->tCurY);
					atUpdateCaret(p_Term);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_NEL:
					if (p_Term->aSIMD) {
						tbSetCursor(p_Term->aTermBuf,p_Term->aSLL,p_Term->aTermBuf->tCurY);
						tbLineFeedUp(p_Term->aTermBuf); /* will update our cursor for us */
					}	else {
						tbSetCursor(p_Term->aTermBuf,p_Term->aSLH,p_Term->aTermBuf->tCurY);
						tbLineFeed(p_Term->aTermBuf); /* will update our cursor for us */
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_ESC:
					p_Term->aState=g_atESCTable;
					break;
				case ATSTATE_VT: /**/
					/* line feed, form feed, vertical tab */
					tbLineFeed(p_Term->aTermBuf);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_HT:
					/* horizontal tab */
					tbHTab(p_Term->aTermBuf);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_HTS:
					/* horizontal tab set */
					tbTabSet(p_Term->aTermBuf,TBTAB_LEFTALIGN,-1);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_TBC:
					/* horizontal tab clear */
					switch(p_Term->aStateVal[0]) {
						default:
						case 0: /* Character tab at current cursor position is cleared */
							tbTabSet(p_Term->aTermBuf,TBTAB_NONE,-1);
							break;
						case 1: /* Line tabulation at current line is cleared -- unimplemented */
						case 2: /* all character tabs in active line are cleared */
						case 3: /* all character tabs are cleared */
						case 4: /* all line tabs are cleared */
						case 5: /* all tabs are cleared (both character and line) */
							tbTabClearAll(p_Term->aTermBuf);
							break;
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_TSR:
					/* Tabulation Stop Remove */
					if (p_Term->aStateVal[0]!=AT_STATEVAL_DEFAULT) {
						tbTabSet(p_Term->aTermBuf,TBTAB_NONE,p_Term->aStateVal[0]-1);
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CBT:
					/* Cursor Backward Tabulation */
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					for(;l_j;l_j--)
						tbHTabReverse(p_Term->aTermBuf);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CHA:
					/* Cursor Character Absolute */
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					tbSetCursor(p_Term->aTermBuf,l_j-1,p_Term->aTermBuf->tCurY);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_VPA:
					/* Line Position Absolute */
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					tbSetCursor(p_Term->aTermBuf,p_Term->aTermBuf->tCurX,l_j-1);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CHT:
					/* Cursor Forward Tabulation */
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					for(;l_j;l_j--)
						tbHTab(p_Term->aTermBuf);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CNL:
					/* Cursor Next Line */
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					for(;l_j;l_j--)
						tbLineFeed(p_Term->aTermBuf); /* will update our cursor for us */
					tbSetCursor(p_Term->aTermBuf,0,p_Term->aTermBuf->tCurY);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CPL:
					/* Cursor Previous Line */
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					for(;l_j;l_j--)
						tbLineFeedUp(p_Term->aTermBuf); /* will update our cursor for us */
					tbSetCursor(p_Term->aTermBuf,0,p_Term->aTermBuf->tCurY);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CTC:
					/* Cursor Tabulation Control */
					/* horizontal tab clear */
					if (p_Term->aStateVal[0]==AT_STATEVAL_DEFAULT)
						p_Term->aStateVal[0]=0;
					for (l_i=0;p_Term->aStateVal[l_i]!=AT_STATEVAL_DEFAULT;l_i++) {
						switch(p_Term->aStateVal[l_i]) {
							default:
							case 0: /* Character tab at current cursor position is set */
								tbTabSet(p_Term->aTermBuf,TBTAB_LEFTALIGN,-1);
								break;
							case 1: /* Line tabulation at current line is set -- unimplemented */
								break;
							case 2: /* Character tab at current position is cleared */
								tbTabSet(p_Term->aTermBuf,TBTAB_NONE,-1);
								break;
							case 3: /* current line tabs is cleared */
								break;
							case 4: /* all character tabs in current line are cleared */
							case 5: /* all character tabs are cleared */
								tbTabClearAll(p_Term->aTermBuf);
								break;
							case 6: /* all line tabulation stops are cleared */
								break;
						}
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_SD:
					/* Scroll data down n lines */
					if (p_Term->aStateVal[0]==AT_STATEVAL_DEFAULT)
						p_Term->aStateVal[0]=1;
					for (l_i=0;l_i<p_Term->aStateVal[0];l_i++) {
						tbScrollDown(p_Term->aTermBuf);
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_SU:
					/* Scroll data up n lines */
					if (p_Term->aStateVal[0]==AT_STATEVAL_DEFAULT)
						p_Term->aStateVal[0]=1;
					for (l_i=0;l_i<p_Term->aStateVal[0];l_i++) {
						tbScrollUp(p_Term->aTermBuf);
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_SR:
					if (p_Term->aStateVal[0]==AT_STATEVAL_DEFAULT)
						p_Term->aStateVal[0]=1;
					for (l_i=0;l_i<p_Term->aStateVal[0];l_i++) {
						tbScrollRight(p_Term->aTermBuf);
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_SL:
					if (p_Term->aStateVal[0]==AT_STATEVAL_DEFAULT)
						p_Term->aStateVal[0]=1;
					for (l_i=0;l_i<p_Term->aStateVal[0];l_i++) {
						tbScrollLeft(p_Term->aTermBuf);
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_SI:
					dbPrint("AT: Unhandled _SI");
					break;
				case ATSTATE_SO:
					dbPrint("AT: Unhandled _SO");
					break;
				case ATSTATE_SCR_STATE:
					p_Term->aState=g_atSCRTable;
					break;
				case ATSTATE_SCS0_STATE: /**/
					p_Term->aSCSType=0;
					p_Term->aState=g_atSCSTable;
					break;
				case ATSTATE_SCS1_STATE: /**/
					p_Term->aSCSType=1;
					p_Term->aState=g_atSCSTable;
					break;
				case ATSTATE_SCS2_STATE: /**/
					p_Term->aSCSType=2;
					p_Term->aState=g_atSCSTable;
					break;
				case ATSTATE_SCS3_STATE: /**/
					p_Term->aSCSType=3;
					p_Term->aState=g_atSCSTable;
					break;
				case ATSTATE_ESC_IGNORE:
					/* unknown ESC code */
					p_Term->aState=g_atEIGTable;
					break;
				case ATSTATE_ESC_DIGIT:
					l_j=p_Term->aStateVal[p_Term->aStateValPos];
					if (l_j==AT_STATEVAL_DEFAULT) {
						p_Term->aStateVal[p_Term->aStateValPos]=(p_Buf[l_byte]-'0');
					} else {
						p_Term->aStateVal[p_Term->aStateValPos]=l_j*10+(p_Buf[l_byte]-'0');
					}
					break;
				case ATSTATE_ESC_SEMI:
					p_Term->aStateValPos++;
					break;
				case ATSTATE_ICH:
					/* insert characters */
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					tbInsertChar(p_Term->aTermBuf,l_j);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_RI:
					if (p_Term->aTermBuf->tCurY) {
						tbCurUp(p_Term->aTermBuf);
					} else {
						tbScrollDown(p_Term->aTermBuf);
					}
					atUpdateCaret(p_Term);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CUU:
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					for (;l_j;l_j--) {
						tbCurUp(p_Term->aTermBuf);
					}
					atUpdateCaret(p_Term);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CUD:
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					for (;l_j;l_j--) {
						tbCurDown(p_Term->aTermBuf);
					}
					atUpdateCaret(p_Term);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CUF:
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					for (;l_j;l_j--) {
						tbCurRight(p_Term->aTermBuf);
					}
					atUpdateCaret(p_Term);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CUB:
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					for (;l_j;l_j--) {
						tbCurLeft(p_Term->aTermBuf);
					}
					atUpdateCaret(p_Term);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CUP:
					/* cursor position */
					l_y=p_Term->aStateVal[0];
					l_x=p_Term->aStateVal[1];
					if (l_y<1)
						l_y=1;
					if (l_x<1)
						l_x=1;
					tbSetCursor(p_Term->aTermBuf,l_x-1,l_y-1);
					atUpdateCaret(p_Term);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_ECH:
					/* Erase Character */
					l_i=p_Term->aStateVal[0];
					if (l_i<1)
						l_i=1;
					l_i--;
					tbClearLineX(p_Term->aTermBuf,p_Term->aTermBuf->tCurX,p_Term->aTermBuf->tCurX+l_i);
					p_Term->aState=g_atGroundTable;
          break;
				case ATSTATE_ED:
					/* Erase in Page */
					switch(p_Term->aStateVal[0]) {
						default:
						case 0:
							/* clear below */
							tbClearBelow(p_Term->aTermBuf);
							break;
						case 1:
							/* clear above */
							tbClearAbove(p_Term->aTermBuf);
							break;
						case 2:
							/* clear screen */
							tbClearScreen(p_Term->aTermBuf);
							break;
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_EL:
					switch(p_Term->aStateVal[0]) {
						default:
						case 0:
							/* clear to right */
							tbClearRight(p_Term->aTermBuf);
							break;
						case 1:
							/* clear to left */
							tbClearLeft(p_Term->aTermBuf);
							break;
						case 2:
							/* clear line */
							tbClearLine(p_Term->aTermBuf);
							break;
					}
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_SGR:
					if (p_Term->aStateVal[0]==AT_STATEVAL_DEFAULT) {
						/* no parameters - set defaults */
						l_p1=0x07;
						l_p2=0x00;
						l_p3=0x00;
					} else {
						l_p1=TB_EXTFCOLOUR(p_Term->aTermBuf->tCurrentAttribute);
						l_p2=TB_EXTBCOLOUR(p_Term->aTermBuf->tCurrentAttribute);
						l_p3=TB_EXTEFFECT(p_Term->aTermBuf->tCurrentAttribute);
					}
					for (l_j=0;p_Term->aStateVal[l_j]!=AT_STATEVAL_DEFAULT;l_j++) {
						switch(p_Term->aStateVal[l_j]) {
							case 0:	/* default */
								/* reset - lgray on black - all attributes off */
								l_p1=0x07;
								l_p2=0x00;
								l_p3=0x00;
								break;
							case 1: /* bold on */
								l_p3|=AT_FONTEFFECT_BOLD;
								break;
//							case 2:  /* faint on */
//							case 3:  /* italicized on */
							case 4:  /* singly underline on */
								/* Underscores on */
								l_p3|=AT_FONTEFFECT_UNDERLINE;
								break;
							case 5:  /* slowly blinking on (less than 150 per minute) */
							case 6:  /* rapidly blinking on (more than 150 per minute) */
								/* Blink on */
//								l_p2=l_p2|0x08;  /* this should be taken out sometime */
								l_p3|=AT_FONTEFFECT_BLINK;
								break;
							case 7:  /* negative image (reverse video) on */
								/* reverse video on */
								l_p3|=AT_FONTEFFECT_REVERSE;
								/* the following color-adjustment stuff should be taken out sometime */
//								if ((l_p1==0x0F)||(l_p1==0x08))
//									l_p1=0x00;
//								else
//								l_p1^=0x07;
//								l_p2^=0x07;
								break;
//							case 8:  /* concealed characters on */
//							case 9:  /* crossed-out (strike-out) */
							case 10: /* Primary (default) font */
							case 11: /* first alternate font */
							case 12: /* second alternate font */
							case 13: /* third alternate font */
							case 14: /* fourth alternate font */
							case 15: /* fifth alternate font */
							case 16: /* sixth alternate font */
							case 17: /* seventh alternate font */
							case 18: /* eighth alternate font */
							case 19: /* ninth alternate font */
							case 20: /* Fraktur (Gothic) */
								break;
//							case 21: /* double underline on */
							case 21: /* **>> This breaks ECMA-48 (bold off ?!?!?!) */ 
							case 22: /* normal color/intensity (neither bold nor faint) */
								/* Bold off */
//								l_p1=l_p1|0x08;
//								l_p1=l_p1^0x08;
								l_p3|=AT_FONTEFFECT_BOLD;
								l_p3^=AT_FONTEFFECT_BOLD;
								break;
//							case 23: /* not italicized, not fraktur */
							case 24: /* not underlined (not singly nor double) */
								/* Underscores off */
								l_p3|=AT_FONTEFFECT_UNDERLINE;
								l_p3^=AT_FONTEFFECT_UNDERLINE;
								break;
							case 25: /* steady (blink off) */
								/* Blink off */
//								l_p2=l_p2|0x08;
//								l_p2=l_p2^0x08;
								l_p3|=AT_FONTEFFECT_BLINK;
								l_p3^=AT_FONTEFFECT_BLINK;
								break;
//							case 26: /* (reserved for proportional spacing) */
							case 27: /* positive image (negative off) */
								/* reverse video off */
								l_p3|=AT_FONTEFFECT_REVERSE;
								l_p3^=AT_FONTEFFECT_REVERSE;
//								if ((l_p1==0x0F)||(l_p1==0x08))
//									l_p1=0x00;
//								else
//									l_p1^=0x07;
//								l_p2^=0x07;
								break;
//							case 28: /* revealed characters (conceal off) */
//							case 29: /* not crossed out (crossed-out off) */
							case 30: /* black display (char) */
							case 31: /* red display (char) */
							case 32: /* green display (char) */
							case 33: /* yellow display (char) */
							case 34: /* blue display (char) */
							case 35: /* magenta display (char) */
							case 36: /* cyan display (char) */
							case 37: /* white display (char) */
								/* Foreground colour */
								l_p1=(((l_p1|0x07)^0x07)|(p_Term->aStateVal[l_j]-30));
								break;
//							case 38: /* (reserved) */
							case 39: /* default display (char) colour */
								/* turn off foreground color */
								l_p1=0x07;
								break;
							case 40: /* black background */
							case 41: /* red background */
							case 42: /* green background */
							case 43: /* yellow background */
							case 44: /* blue background */
							case 45: /* magenta background */
							case 46: /* cyan background */
							case 47: /* white background */
								/* Background colour */
								l_p2=(((l_p2|0x07)^0x07)|(p_Term->aStateVal[l_j]-40));
								break;
//							case 48: /* (reserved) */
							case 49: /* default background colour */
								/* turn off background color */
								l_p2=0x00;
								break;
//							case 50: /* (reserved - cancels effect of 26) */
//							case 51: /* framed */
//							case 52: /* encircled */
//							case 53: /* overline */
//							case 54: /* not framed, not encircled */
//							case 55: /* not overlined */
//							case 56: /* (reserved) */
//							case 57: /* (reserved) */
//							case 58: /* (reserved) */
//							case 59: /* (reserved) */
//							case 60: /* ideogram underline or right side line */
//							case 61: /* ideogram double underline or double right side line */
//							case 62: /* ideogram overline or left side line */
//							case 63: /* ideogram double overline or double left side line */
//							case 64: /* ideogram stress marking */
//							case 65: /* cancels effect from 60-64 */
							default:
#ifndef DB_DEBUG_NODEBUG
								sprintf(l_tBuf,"VT100: ESC [ %i m sequence ignored",p_Term->aStateVal[l_j]);
								dbPrint(l_tBuf);
#endif
								break;
						}
					}
					tbSetFColour(p_Term->aTermBuf,l_p1);
					tbSetBColour(p_Term->aTermBuf,l_p2);
					tbSetEffect(p_Term->aTermBuf,l_p3);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CSI_STATE:
					/* reset parameters */
					for (l_i=0;l_i<AT_ESC_PARAM_MAX;l_i++)
						p_Term->aStateVal[l_i]=AT_STATEVAL_DEFAULT;
					p_Term->aStateValPos=0;
					p_Term->aState=g_atCSITable;
					break;
				case ATSTATE_IL:
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					tbInsertLine(p_Term->aTermBuf,l_j);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_DL:
					l_j=p_Term->aStateVal[0];
					if (l_j<1)
						l_j=1;
					tbDeleteLine(p_Term->aTermBuf,l_j);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_DECSC:
					tbCursorSave(p_Term->aTermBuf);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_DECRC:
					tbCursorRestore(p_Term->aTermBuf);
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_DECSET:
				case ATSTATE_DECRST:
					/* process DEC private modes set, reset */
					if (l_result==ATSTATE_DECSET) {
//						l_bit=0x00;  /* set bit */
					} else {
//						l_bit=~0x00; /* reset (clr) bits */
					}
					l_i=0;
					while(p_Term->aStateVal[l_i]!=AT_STATEVAL_DEFAULT) {
						switch (p_Term->aStateVal[l_i]) {
							case 1: /* DECCKM */
							case 2: /* ANSI/VT52 mode */
							case 3: /* DECCOLM */
							case 4: /* DECSCLM (slow scroll) */
							case 5: /* DECSCNM */
							case 6: /* DECOM */
							case 7: /* DECAWM */
							case 8: /* DECARM */
							case 9: /* MIT bogus sequence */
							case 38: /* DECTEK */
							case 40: /* 132 column mode */
							case 41: /* curses hack */
							case 44: /* margin bell */
							case 45: /* reverse wraparound */
							case 46: /* logging */
							case 47: /* alternate buffer */
							case 1000: /* xterm bogus sequence */
							case 1001: /* xterm sequence w/ hilite tracking */
#ifndef DB_DEBUG_NODEBUG
								if (l_result==ATSTATE_DECSET)
									sprintf(l_tBuf,"AT: unhandled DECSET: %i",p_Term->aStateVal[l_i]);
								else
									sprintf(l_tBuf,"AT: unhandled DECRST: %i",p_Term->aStateVal[l_i]);
								dbPrint(l_tBuf);
#endif
								break;
						}
						l_i++;
					}
					break;
				case ATSTATE_SIMD:
					if (p_Term->aStateVal[0]==1)
						p_Term->aSIMD=1;
					else
						p_Term->aSIMD=0; /* default */
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_CSI_SIB: /* CSI Single Intermediate Byte mode */
					p_Term->aState=g_atCSI_SIBTable;
					break;
				case ATSTATE_SLH:
					if (p_Term->aStateVal[0]<1)
						p_Term->aSLH=0;
					else
						p_Term->aSLH=p_Term->aStateVal[0]-1;
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_SLL:
					if (p_Term->aStateVal[0]<1)
						p_Term->aSLL=0;
					else
						p_Term->aSLL=p_Term->aStateVal[0]-1;
					p_Term->aState=g_atGroundTable;
					break;
				case ATSTATE_DA1:
				case ATSTATE_TRACK_MOUSE:
				case ATSTATE_SET:
				case ATSTATE_RST:
				case ATSTATE_CPR:
				case ATSTATE_DECSTBM:
				case ATSTATE_DECREQTPARM:
				case ATSTATE_DECALN:
				case ATSTATE_GSETS:
				case ATSTATE_DECKPAM:
				case ATSTATE_DECKPNM:
				case ATSTATE_IND:
				case ATSTATE_SS2:
				case ATSTATE_SS3:
				case ATSTATE_OSC:
				case ATSTATE_RIS:
				case ATSTATE_LS2:
				case ATSTATE_LS3:
				case ATSTATE_LS3R:
				case ATSTATE_LS2R:
				case ATSTATE_LS1R:
				case ATSTATE_XTERM_SAVE:
				case ATSTATE_XTERM_RESTORE:
				case ATSTATE_XTERM_TITLE:
				case ATSTATE_DECID:
#ifndef DB_DEBUG_NODEBUG
					dbPrint("Unimplemented ANSITerm state");
					sprintf(l_tBuf,"AT: %s -> %s",
						p_Term->aState==g_atGroundTable?"Ground Table":
						p_Term->aState==g_atCSITable?"g_atCSITable":
						p_Term->aState==g_atEIGTable?"g_atEIGTable":
						p_Term->aState==g_atESCTable?"g_atESCTable":
						p_Term->aState==g_atIESTable?"g_atIESTable":
						p_Term->aState==g_atIGNTable?"g_atIGNTable":
						p_Term->aState==g_atSCRTable?"g_atSCRTable":
						p_Term->aState==g_atSCSTable?"g_atSCSTable":
						p_Term->aState==g_atCSI_SIBTable?"g_atCSI_SIBTable": "??WHAT??",
						g_atStateTable[l_result]);
					dbPrint(l_tBuf);
#endif
					p_Term->aState=g_atGroundTable;
					break;
				default:
#ifndef DB_DEBUG_NODEBUG
					dbPrint("Invalid ANSITerm state");
					sprintf(l_tBuf,"AT: %s -> %s",
						p_Term->aState==g_atGroundTable?"Ground Table":
						p_Term->aState==g_atCSITable?"g_atCSITable":
						p_Term->aState==g_atEIGTable?"g_atEIGTable":
						p_Term->aState==g_atESCTable?"g_atESCTable":
						p_Term->aState==g_atIESTable?"g_atIESTable":
						p_Term->aState==g_atIGNTable?"g_atIGNTable":
						p_Term->aState==g_atSCRTable?"g_atSCRTable":
						p_Term->aState==g_atSCSTable?"g_atSCSTable": "??WHAT??",
						g_atStateTable[l_result]);
					dbPrint(l_tBuf);
#endif
					p_Term->aState=g_atGroundTable;
					break;
			} /* master switch */
		} /* for() -- loop through buffer */
	} /* VT100 emulation */
	return(p_BufLen);
}

void atClearTerm(ATTERM *p_Term) {
	if (p_Term)
		tbClearScreen(p_Term->aTermBuf);
}

#pragma argsused
void atNotifyFn(unsigned long p_ID, unsigned int p_Operation, long p_P0,
	long p_P1, long p_P2, long p_P3) {
	ATTERM *l_Term;
	long l_sy,l_dy;
	RECT l_rect;

	if (!p_ID)
		return;
	l_Term=(ATTERM*)(p_ID);
	switch (p_Operation) {
		case TBNOTIFY_SCROLL:
//      if (l_Term->aShowY==0)
			l_Term->aScroll+=-p_P0*l_Term->aFontY;
			PostMessage(l_Term->aWnd,WM_PAINT,0,0L); /* note there may not be an update region! He he he */
//      else  /* NOTE THIS SHOULD CHANGE, IF UNCOMMENTED, TO */
//            /* CHECK FOR aSeparateInputLine */   <<<CAUSE AN ERROR>>>
//        ScrollWindow(l_Term->aWnd,0,-p_P0*l_Term->aFontY,NULL,NULL);
//      UpdateWindow(l_Term->aWnd);
			break;
		case TBNOTIFY_NEWCHAR:
			l_sy=TB_MAX(p_P1,l_Term->aShowY);
			l_dy=TB_MIN(p_P3,l_Term->aShowY+l_Term->aSizeY);
			if (l_dy>=l_sy) {
				l_rect.top=(l_sy-l_Term->aShowY)*l_Term->aFontY;
				l_rect.bottom=(l_sy-l_Term->aShowY+(l_dy-l_sy)+1)*l_Term->aFontY;
				if (l_sy==l_dy) {
					l_rect.left=(p_P0-l_Term->aShowX)*l_Term->aFontX;
					l_rect.right=(p_P0-l_Term->aShowX+(p_P2-p_P0)+1)*l_Term->aFontX;
				} else {
					l_rect.left=0;
					l_rect.right=l_Term->aSizeX*l_Term->aFontX;
				}
				InvalidateRect(l_Term->aWnd,&l_rect,FALSE);
			}
			break;
		case TBNOTIFY_MOVECUR:
			if (p_P2)
				atUpdateCaret(l_Term);
			break;
		case TBNOTIFY_RESIZE:
//      atAdjustTerm(l_Term,-1);
		default:
			dbPrint("ANSITERM: Unhandled NotifyFn operation");
			break;
	}
//  atUpdateCaret(l_Term);
	return;
}

/* This proc updates the caret position & visibility information based
 * on the current state of the terminal */
void atUpdateCaret(ATTERM *p_Term) {
	if (p_Term) {
		if (GetFocus()==p_Term->aWnd) {
			if (p_Term->aSeparateInputLine) {
				/* TAG TAG TAG THIS has to move the caret over in the X direction
				 * according to whatever text is currently displayed */
				SetCaretPos(p_Term->aFontX*(p_Term->aInputCurPos-p_Term->aInputDispCurPos),
					p_Term->aTermBuf->tScrY*p_Term->aFontY+p_Term->aInputBorderDY);
				ShowCaret(p_Term->aWnd);
			} else if (p_Term->aTermBuf) {
				if ((p_Term->aTermBuf->tCurX>=p_Term->aShowX)&&
						(p_Term->aTermBuf->tCurX<p_Term->aShowX+p_Term->aSizeX) &&
						(p_Term->aTermBuf->tCurY>=p_Term->aShowY)&&
						(p_Term->aTermBuf->tCurY<p_Term->aShowY+p_Term->aSizeY)) {
					SetCaretPos((p_Term->aTermBuf->tCurX-p_Term->aShowX)*p_Term->aFontX,
						(p_Term->aTermBuf->tCurY-p_Term->aShowY)*p_Term->aFontY);
					ShowCaret(p_Term->aWnd);
				} else {
					HideCaret(p_Term->aWnd);
				}
			}
		}
	}
}

/* Does screen scrolling & invalidation etc to cause the screen to
 * be repainted at a new specified location */
void atScrollTermTo(ATTERM *p_Term,BOOL p_Rel,long p_Val) {
	long l_newShowY;
//	char l_buf[100];

//	sprintf(l_buf,"SCROLL: %s %i",p_Rel?"Relative":"Absolute",p_Val);
//	dbPrint(l_buf);

	/* first, let's check our input */
	if (!p_Term)
		return;

	if (p_Rel) {
		/* relative offset */
		l_newShowY=p_Term->aShowY+p_Val;
	} else {
		l_newShowY=p_Val;
	}

	if (l_newShowY>0)
		l_newShowY=0;
	if (l_newShowY<(p_Term->aTermBuf->tScrY-p_Term->aTermBuf->tBufY+ATIB_TERMBUF_EXTRALINES))
		l_newShowY=(p_Term->aTermBuf->tScrY-p_Term->aTermBuf->tBufY+ATIB_TERMBUF_EXTRALINES);

	/* ok, we know where we are scrolling to. Now, let's check if we can
	 * scroll part of our window. If we can, do so. */
	if (p_Term->aShowY==l_newShowY) {
		return;
	} else if (p_Term->aShowY<l_newShowY) {
		/* scroll down */
		if ((l_newShowY-p_Term->aShowY)<p_Term->aSizeY) {
//      p_Term->aScroll+=-(l_newShowY-p_Term->aShowY)*p_Term->aFontY;
			if (p_Term->aSeparateInputLine)
				ScrollWindow(p_Term->aWnd,0,-(l_newShowY-p_Term->aShowY)*p_Term->aFontY,
					&(p_Term->aScrollRect),&(p_Term->aScrollRect));
			else
				ScrollWindow(p_Term->aWnd,0,-(l_newShowY-p_Term->aShowY)*p_Term->aFontY,NULL,NULL);
		} else {
			if (p_Term->aSeparateInputLine)
				InvalidateRect(p_Term->aWnd,&(p_Term->aScrollRect),FALSE);
			else
				InvalidateRect(p_Term->aWnd,NULL,FALSE);
		}
	} else {
		/* scroll up */
		if ((p_Term->aShowY-l_newShowY)<p_Term->aSizeY) {
//      p_Term->aScroll+=(p_Term->aShowY-l_newShowY)*p_Term->aFontY;
			if (p_Term->aSeparateInputLine)
				ScrollWindow(p_Term->aWnd,0,(p_Term->aShowY-l_newShowY)*p_Term->aFontY,
					&(p_Term->aScrollRect),&(p_Term->aScrollRect));
			else
				ScrollWindow(p_Term->aWnd,0,(p_Term->aShowY-l_newShowY)*p_Term->aFontY,NULL,NULL);
		} else {
			if (p_Term->aSeparateInputLine)
				InvalidateRect(p_Term->aWnd,&(p_Term->aScrollRect),FALSE);
			else
				InvalidateRect(p_Term->aWnd,NULL,FALSE);
		}
	}
	p_Term->aShowY=l_newShowY;
	return;
}

#define ATANSITERMPROCBUFSIZE 1024
#pragma argsused
LRESULT CALLBACK _export atAnsiTermProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
	static RECT l_rect;
	static PAINTSTRUCT l_ps; // paint structure
	static HDC l_hDC; // device context
	static HFONT l_oldFont;
	static HPEN l_oldPen;
	static int l_i,l_j,l_x,l_y,l_count;
	static int l_startX,l_endX; /* start & end lines */
	static int l_startY,l_endY; /* start & end lines */
//  static HBRUSH l_hBrush;
//  static DSSTRUCT *l_area,*l_thing;
	static unsigned char l_buf[ATANSITERMPROCBUFSIZE];
//  static POINT l_ptCurrent; /* for quick-menu */
//  static HMENU l_hmenu;     /* for quick-menu */
//  static UINT l_flags;      /* for quick-menu */
//  static UINT l_flagsInactive; /* for quick-menu */
//  static UINT l_flagsSeparator; /* for quick-menu */
	static ATTERM *l_Term;
	static long l_attrib;
	static unsigned long l_rawChar,*l_rawCharP;
	static unsigned char *l_p,*l_q;
	static HMENU l_menu;
	static POINT l_point;

#ifdef WIN32
	l_Term=(ATTERM *)GetWindowLong(p_hWnd,ATWND_ATTERMSEG);
#else
	l_Term=(ATTERM *)MAKELP(GetWindowLong(p_hWnd,ATWND_ATTERMSEG),
		GetWindowLong(p_hWnd,ATWND_ATTERMOFF));
#endif

	if (p_message==g_atCommDlgHelp) {
		(*(l_Term->aNotify))(l_Term,TBNOTIFY_FONTHELP);
		return 0L;
	}

	switch (p_message)
		{
		case WM_VSCROLL:
			DestroyCaret();
			switch(GET_WM_VSCROLL_CODE(p_wParam,p_lParam)) {
				case SB_BOTTOM:
					atScrollTermTo(l_Term,0,0);
					break;
				case SB_LINEDOWN:
					atScrollTermTo(l_Term,1,1);
					break;
				case SB_LINEUP:
					atScrollTermTo(l_Term,1,-1);
					break;
				case SB_PAGEDOWN:
					atScrollTermTo(l_Term,1,l_Term->aTermBuf->tScrY-1);
					break;
				case SB_PAGEUP:
					atScrollTermTo(l_Term,1,-(l_Term->aTermBuf->tScrY-1));
					break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					atScrollTermTo(l_Term,0,(long)((short)(GET_WM_HSCROLL_POS(p_wParam,p_lParam))));
					UpdateWindow(p_hWnd);
					break;
				case SB_TOP:
					atScrollTermTo(l_Term,0,0);
					break;
				case SB_ENDSCROLL:
//          PostMessage(p_hWnd,WM_USER_AREALIST_SCROLL,0,0L);
//          break;
					SendMessage(p_hWnd,WM_SETFOCUS,0,0L);
					break;
			}
			SetScrollPos(p_hWnd,SB_VERT,l_Term->aShowY,TRUE);
			return 0L;
		case WM_COMMAND:
			switch(GET_WM_COMMAND_ID(p_wParam,p_lParam)) {
				case CM_ANSIEDITCOPY:
					l_i=tbHilightSize(l_Term->aTermBuf);
					if (!l_i)
            break; /* nothing to copy */
					if (OpenClipboard(l_Term->aWnd)) {
						if (EmptyClipboard()) {
							HGLOBAL l_hGlobal;
							/* first thing we need to do is calculate how large of a buffer
							 * we need */
							/* next, alloc our buffer */
							l_hGlobal=GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,l_i+10); /* + 10 just to be sure we don't overrun */
							if (l_hGlobal) {
								l_p=(unsigned char*)GlobalLock(l_hGlobal);
								if (l_p) {
									/* now copy our data into the buffer */
									tbCopyHilight(l_Term->aTermBuf,l_p);
									GlobalUnlock(l_hGlobal);
									SetClipboardData(CF_TEXT,(HANDLE)l_hGlobal);
								} else {
									GlobalFree(l_hGlobal);
								}
							}
						}
						CloseClipboard();
					}
					break;
				case CM_ANSIEDITPASTE:
					if (IsClipboardFormatAvailable(CF_TEXT)) {
						if (OpenClipboard(l_Term->aWnd)) {
							HGLOBAL l_hGlobal;
							l_hGlobal=GetClipboardData(CF_TEXT);
							if (l_hGlobal) {
								unsigned char *l_pp;
								l_pp=(unsigned char*)GlobalLock(l_hGlobal);
								if (l_pp) {
									/* insert text */
									atSendTermInput(l_Term,l_pp,strlen((char*)l_pp));
									/* phew, that wasn't so hard. */
									GlobalUnlock(l_hGlobal);
								}
							}
							CloseClipboard();
						}
					}
					break;
				case CM_ANSIOPTIONSTERMSET:
					if (DialogBoxParam(l_Term->aInst,MAKEINTRESOURCE(DIALOG_ATOPT),
						l_Term->aWnd,atAnsiTermOptionsProc,(LPARAM)l_Term)==IDOK)
						(*(l_Term->aNotify))(l_Term,TBNOTIFY_OPTIONS);
					return 0L;
				case CM_ANSIOPTIONSFNKEYS:
					{
					int l_i;
					unsigned char l_Macro[48][TBMACRO_MAXLEN];

					for (l_i=0;l_i<48;l_i++)
						strcpy((char*)l_Macro[l_i],(char*)l_Term->aFnKeyMacro[l_i]);
					if (DialogBoxParam(l_Term->aInst,MAKEINTRESOURCE(DIALOG_ATFNKEY),
						l_Term->aWnd,atATFnKeyProc,(LPARAM)l_Term)==IDOK) {
						(*(l_Term->aNotify))(l_Term,TBNOTIFY_MACROS);
					} else {
						/* restore macros */
						for (l_i=0;l_i<48;l_i++)
							strcpy((char*)l_Term->aFnKeyMacro[l_i],(char*)l_Macro[l_i]);
					}
					}
					return 0L;
				case CM_ANSIOPTIONSFONT:
					{
					CHOOSEFONT l_font;
					LOGFONT l_logfont;

					l_font.lStructSize=sizeof(CHOOSEFONT);
					l_font.hwndOwner=l_Term->aWnd;
					l_font.hDC=NULL;
					l_font.lpLogFont=&l_logfont;
					if (l_Term->aLogFontValid) {
						meMemCpy(&l_logfont,&(l_Term->aLogFont),sizeof(LOGFONT));
						l_font.Flags=CF_ANSIONLY|CF_FIXEDPITCHONLY|CF_INITTOLOGFONTSTRUCT|
							CF_SCREENFONTS|CF_SHOWHELP|CF_ENABLEHOOK;
					} else {
						l_font.Flags=CF_ANSIONLY|CF_FIXEDPITCHONLY|
							CF_SCREENFONTS|CF_SHOWHELP|CF_ENABLEHOOK;
					}
// N/A          l_font.rgbColours
// N/A          l_font.lCustData=0;
					((void*)(l_font.lpfnHook))=((void*)(dl3DSubClassProc));
					l_font.lpTemplateName=NULL;
					l_font.hInstance=NULL;
					l_font.lpszStyle=NULL;
					l_font.nFontType=0;
					l_font.nSizeMin=0;
					l_font.nSizeMax=0;
					if (ChooseFont(&l_font)) {
						/* we have a selection! */
						atChangeFont(l_Term,NULL,&l_logfont);
						(*(l_Term->aNotify))(l_Term,TBNOTIFY_FONT);
					}
					}
				default:
					break;
			}
			break;
		case WM_LBUTTONDBLCLK:
			break;
		case WM_RBUTTONDOWN:
			/* bring up our pop-up menu */
			l_menu=CreatePopupMenu();
			if (l_menu) {        // CM_ANSIEDITCOPY CM_ANSIEDITPASTE
				AppendMenu(l_menu,MF_ENABLED|MF_STRING,CM_ANSIEDITCOPY,"&Copy\tCtrl+Ins");
				if (IsClipboardFormatAvailable(CF_TEXT))
					AppendMenu(l_menu,MF_ENABLED|MF_STRING,CM_ANSIEDITPASTE,"&Paste\tShift+Ins");
				else
					AppendMenu(l_menu,MF_DISABLED|MF_GRAYED,CM_ANSIEDITPASTE,"&Paste\tShift+Ins");
				if (GetCursorPos(&l_point)) {
					TrackPopupMenu(l_menu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON,
					l_point.x,l_point.y,0,p_hWnd,NULL);
				}
				DestroyMenu(l_menu);
			}
			break;
		case WM_LBUTTONDOWN:
			/* start hilighting */
			if (!GetCursorPos(&l_point))
				break;
			ScreenToClient(l_Term->aWnd,&l_point);
			/* let's see if we're in our input line */
			if (l_Term->aSeparateInputLine) {
				if (l_point.y>l_Term->aScrollRect.bottom) {
					break;
				}
			}
			/* first thing we have to do is erase any pre-existing hilight area */
			tbClearHilight(l_Term->aTermBuf);
			InvalidateRect(l_Term->aWnd,NULL,FALSE);
			/* capture our mouse */
			SetCapture(p_hWnd);
			l_Term->aMouseHiliteCapture=TRUE;
			l_Term->aCaptureLastX=l_point.x;
			l_Term->aCaptureLastY=l_point.y;
			tbStartHilight(l_Term->aTermBuf,
				(l_point.x/l_Term->aFontX)+l_Term->aShowX,
				(l_point.y/l_Term->aFontY)+l_Term->aShowY);
			/* generate a timer to check on scrolling */
			SetTimer(l_Term->aWnd,1,100,NULL);
			dbPrint("WM_LBUTTONDOWN");
			break;
		case WM_LBUTTONUP:
			if (l_Term->aMouseHiliteCapture) {
				/* release our mouse */
				l_Term->aMouseHiliteCapture=FALSE;
				ReleaseCapture();
				KillTimer(l_Term->aWnd,1);
			}
			dbPrint("WM_LBUTTONUP");
			break;
		case WM_TIMER:
			if (l_Term->aMouseHiliteCapture) {
				l_i=0;
				if (l_Term->aCaptureLastY<0) {
					l_i=l_Term->aCaptureLastY;
					if (l_i<-25) {
						l_i+=15;
						l_i/=5;
					}	else {
						l_i=-1;
					}
					l_rect.left  =0;
					l_rect.right =l_Term->aFontX*l_Term->aSizeX;
					l_rect.top   =0;
					l_rect.bottom=((-l_i)+1)*l_Term->aFontY;
				} else if (l_Term->aCaptureLastY>l_Term->aFontY*l_Term->aSizeY) {
					l_i=(l_Term->aCaptureLastY-l_Term->aFontY*l_Term->aSizeY);
					if (l_i>25) {
						l_i-=15;
						l_i/=5;
					}	else {
						l_i=1;
					}
					l_rect.left  =0;
					l_rect.right =l_Term->aFontX*l_Term->aSizeX;
					l_rect.top   =l_Term->aFontY*((l_Term->aSizeY-l_i-1));
					l_rect.bottom=l_Term->aFontY*l_Term->aSizeY;
				}
				if (l_i) {
					atScrollTermTo(l_Term,1,l_i);
					l_j=((l_Term->aCaptureLastY/l_Term->aFontY)+l_Term->aShowY);
					if (l_j<l_Term->aSizeY-l_Term->aTermBuf->tBufY+ATIB_TERMBUF_EXTRALINES)
						l_j=l_Term->aSizeY-l_Term->aTermBuf->tBufY+ATIB_TERMBUF_EXTRALINES;
					tbUpdateHilight(l_Term->aTermBuf,
						(l_Term->aCaptureLastX/l_Term->aFontX)+l_Term->aShowX,
						l_j);
					InvalidateRect(l_Term->aWnd,&l_rect,FALSE);
					SetScrollPos(p_hWnd,SB_VERT,l_Term->aShowY,TRUE);
					atUpdateCaret(l_Term);
				}
			} else {
				KillTimer(l_Term->aWnd,p_wParam);
			}
			break;
		case WM_MOUSEMOVE:
			if (l_Term->aMouseHiliteCapture) {
				if (GetCursorPos(&l_point)) {
					/* capture our mouse */
					ScreenToClient(l_Term->aWnd,&l_point);
					l_i=(l_point.x/l_Term->aFontX)+l_Term->aShowX;
					l_j=(l_point.y/l_Term->aFontY)+l_Term->aShowY;
					if (l_j<l_Term->aSizeY-l_Term->aTermBuf->tBufY+ATIB_TERMBUF_EXTRALINES) { /* this is not necessary if termbuf handled scrollback=0 */
						l_j=l_Term->aSizeY-l_Term->aTermBuf->tBufY+ATIB_TERMBUF_EXTRALINES;     /* this is not necessary if termbuf handled scrollback=0 */
						l_i=0;                                                                  /* this is not necessary if termbuf handled scrollback=0 */
					}                                                                         /* this is not necessary if termbuf handled scrollback=0 */
					tbUpdateHilight(l_Term->aTermBuf,l_i,l_j);
					if (l_Term->aCaptureLastY>l_point.y) {
						l_rect.top=(l_point.y/l_Term->aFontY)*l_Term->aFontY;
						l_rect.bottom=(l_Term->aCaptureLastY/l_Term->aFontY+1)*l_Term->aFontY;
					} else {
						l_rect.top=(l_Term->aCaptureLastY/l_Term->aFontY)*l_Term->aFontY;
						l_rect.bottom=(l_point.y/l_Term->aFontY+1)*l_Term->aFontY;
					}
					l_rect.left=0;
					l_rect.right=l_Term->aSizeX*l_Term->aFontX;
					InvalidateRect(p_hWnd,&l_rect,FALSE);
					l_Term->aCaptureLastX=l_point.x;
					l_Term->aCaptureLastY=l_point.y;
				}
			}
			break;
		case WM_KEYDOWN:
			/* let's see if this is a function key macro */
			/* VIRTUAL KEY VALUES:
			 * ------------------
			 * PgUp   : 0x21
			 * PgDn   : 0x22
			 * End    : 0x23
			 * Home   : 0x24
			 * Ins    : 0x2D
			 * Del    : 0x2E
			 * F1-F12 : 0x70-0x7B
			 * CsrLft : 0x25
			 * CsrUp  : 0x26
			 * CsrRgt : 0x27
			 * CsrDn  : 0x28
			 * ------------------
			 */
			l_i=LOWORD(p_wParam);
			if ((l_i>=0x70)&&(l_i<=0x7B)) {
				l_i-=0x70;
				if (GetKeyState(VK_SHIFT)&0x00008000)
					l_i+=12;
				if (GetKeyState(VK_CONTROL)&0x00008000)
					l_i+=24;
				if (l_Term->aFnKeyMacro[l_i][0]) {
					/* we have a macro! */
					/* First, let's process this sucker into a proper string */
					l_j=0;
					for (l_j=0;(l_j<TBMACRO_MAXLEN)&&(l_Term->aFnKeyMacro[l_i][l_j]);l_j++) {
						if (l_Term->aFnKeyMacro[l_i][l_j]=='\\') {
							l_j++;
							if ((l_j<TBMACRO_MAXLEN)&&(l_Term->aFnKeyMacro[l_i][l_j])) {
								if ((l_Term->aFnKeyMacro[l_i][l_j]=='a')||
										(l_Term->aFnKeyMacro[l_i][l_j]=='A')) {
									l_buf[0]=0x07;
									atSendTermInput(l_Term,l_buf,1);
								} else if ((l_Term->aFnKeyMacro[l_i][l_j]=='b')||
										(l_Term->aFnKeyMacro[l_i][l_j]=='B')) {
									l_buf[0]=0x08;
									atSendTermInput(l_Term,l_buf,1);
								} else if ((l_Term->aFnKeyMacro[l_i][l_j]=='t')||
										(l_Term->aFnKeyMacro[l_i][l_j]=='T')) {
									l_buf[0]=0x09;
									atSendTermInput(l_Term,l_buf,1);
								} else if ((l_Term->aFnKeyMacro[l_i][l_j]=='n')||
										(l_Term->aFnKeyMacro[l_i][l_j]=='N')) {
									l_buf[0]=0x0D; /* should be 0x0A, but people will want it to be 0x0D */
									atSendTermInput(l_Term,l_buf,1);
								} else if ((l_Term->aFnKeyMacro[l_i][l_j]=='r')||
										(l_Term->aFnKeyMacro[l_i][l_j]=='R')) {
									l_buf[0]=0x0D;
									atSendTermInput(l_Term,l_buf,1);
								} else if (l_Term->aFnKeyMacro[l_i][l_j]=='\\'){
									l_buf[0]='\\';
									atSendTermInput(l_Term,l_buf,1);
								} /* else ignore it */
							} else {
								break;
							}
						} else {
							atSendTermInput(l_Term,&(l_Term->aFnKeyMacro[l_i][l_j]),1);
						}
					}
					return 0L;
				} else {
					if (l_Term->aSeparateInputLine) {
						return 0L;
					}
					/* send function key sequence */
					if ((l_i>=0)&&(l_i<=4)) {
						l_buf[0]=(unsigned char)ASCII_ESC;
						l_buf[1]=(unsigned char)'[';
						l_buf[2]=(unsigned char)'[';
						l_buf[3]=(unsigned char)('A'+l_i);
						atSendTermInput(l_Term,l_buf,4);
					} else if ((l_i>=5)&&(l_i<=11)) {
						if (l_i>=10) l_i++;
						sprintf((char*)l_buf," [%02i~",l_i+12);
						l_buf[0]=(unsigned char)ASCII_ESC;
						atSendTermInput(l_Term,l_buf,5);
					} /* else don't know what to do with it */
				}
			} else if (l_i==0x21) { /* Page UP */
				if (l_Term->aSeparateInputLine) {
					atScrollTermTo(l_Term,1,-(l_Term->aTermBuf->tScrY-1));
					SetScrollPos(p_hWnd,SB_VERT,l_Term->aShowY,TRUE);
				} else if (GetKeyState(VK_SHIFT)&0x00008000) {
					atScrollTermTo(l_Term,1,-(l_Term->aTermBuf->tScrY-1));
					SetScrollPos(p_hWnd,SB_VERT,l_Term->aShowY,TRUE);
          atUpdateCaret(l_Term);
				} else {
					/* send PGUP */
					l_buf[0]=(unsigned char)ASCII_ESC;
					l_buf[1]=(unsigned char)'[';
					l_buf[2]=(unsigned char)'5';
					l_buf[3]=(unsigned char)'~';
					atSendTermInput(l_Term,l_buf,4);
				}
			} else if (l_i==0x22) { /* Page DOWN */
				if (l_Term->aSeparateInputLine) {
					atScrollTermTo(l_Term,1,l_Term->aTermBuf->tScrY-1);
					SetScrollPos(p_hWnd,SB_VERT,l_Term->aShowY,TRUE);
				} else if (GetKeyState(VK_SHIFT)&0x00008000) {
					atScrollTermTo(l_Term,1,l_Term->aTermBuf->tScrY-1);
					SetScrollPos(p_hWnd,SB_VERT,l_Term->aShowY,TRUE);
					atUpdateCaret(l_Term);
				} else {
					/* send PGDN */
					l_buf[0]=(unsigned char)ASCII_ESC;
					l_buf[1]=(unsigned char)'[';
					l_buf[2]=(unsigned char)'6';
					l_buf[3]=(unsigned char)'~';
					atSendTermInput(l_Term,l_buf,4);
				}
			} else if (l_i==0x23) { /* END */
				if (l_Term->aSeparateInputLine) {
					if (*l_Term->aInputCurPos) {
						while(*(l_Term->aInputCurPos)) {
							/* increment the sucker */
							l_Term->aInputCurPos=ATIB_NEXTCHAR(l_Term,l_Term->aInputCurPos);
							if (l_Term->aInputCurPos==ATIB_ADDCHAR(l_Term,
								l_Term->aInputDispCurPos,l_Term->aTermBuf->tScrX)) {
								/* also scroll input line forward one */
								l_Term->aInputDispCurPos=ATIB_NEXTCHAR(l_Term,l_Term->aInputDispCurPos);
							}
						}
						GetClientRect(l_Term->aWnd,&l_rect);
						l_rect.top=l_rect.bottom-l_Term->aInputWndDY;
						InvalidateRect(l_Term->aWnd,&l_rect,FALSE);
						atUpdateCaret(l_Term);
					}
					return 0L;
				}
				/* send END */
				l_buf[0]=(unsigned char)ASCII_ESC;
				l_buf[1]=(unsigned char)'[';
				l_buf[2]=(unsigned char)'4';
				l_buf[3]=(unsigned char)'~';
				atSendTermInput(l_Term,l_buf,4);
			} else if (l_i==0x24) { /* HOME */
				if (l_Term->aSeparateInputLine) {
					if (l_Term->aInputCurPos!=l_Term->aInputCurLine) {
						l_Term->aInputCurPos=l_Term->aInputCurLine;
						l_Term->aInputDispCurPos=l_Term->aInputCurLine;
						GetClientRect(l_Term->aWnd,&l_rect);
						l_rect.top=l_rect.bottom-l_Term->aInputWndDY;
						InvalidateRect(l_Term->aWnd,&l_rect,FALSE);
						atUpdateCaret(l_Term);
					}
					return 0L;
				}
				/* send Home */
				l_buf[0]=(unsigned char)ASCII_ESC;
				l_buf[1]=(unsigned char)'[';
				l_buf[2]=(unsigned char)'H';
				atSendTermInput(l_Term,l_buf,3);
//				/* send HOME */
//				l_buf[0]=(unsigned char)ASCII_ESC;
//				l_buf[1]=(unsigned char)'[';
//				l_buf[2]=(unsigned char)'1';
//				l_buf[2]=(unsigned char)'~';
//				atSendTermInput(l_Term,l_buf,4);
			} else if (l_i==0x25) { /* Cursor LEFT */
				if (l_Term->aSeparateInputLine) {
					if (l_Term->aInputCurPos!=l_Term->aInputCurLine) {
						if (l_Term->aInputCurPos==l_Term->aInputDispCurPos) {
							/* also scroll input line back one */
							l_Term->aInputDispCurPos=ATIB_PREVCHAR(l_Term,l_Term->aInputDispCurPos);
							GetClientRect(l_Term->aWnd,&l_rect);
							l_rect.top=l_rect.bottom-l_Term->aInputWndDY;
							InvalidateRect(l_Term->aWnd,&l_rect,FALSE);
						}
						l_Term->aInputCurPos=ATIB_PREVCHAR(l_Term,l_Term->aInputCurPos);
						atUpdateCaret(l_Term);
					}
					return 0L;
				}
				/* send ASCII Crsr Left */
				l_buf[0]=(unsigned char)ASCII_ESC;
				l_buf[1]=(unsigned char)'[';
				l_buf[2]=(unsigned char)'D';
				atSendTermInput(l_Term,l_buf,3);
			} else if (l_i==0x27) { /* Cursor RIGHT */
				if (l_Term->aSeparateInputLine) {
					if (*(l_Term->aInputCurPos)) {
						l_Term->aInputCurPos=ATIB_NEXTCHAR(l_Term,l_Term->aInputCurPos);
						if (l_Term->aInputCurPos==ATIB_ADDCHAR(l_Term,
							l_Term->aInputDispCurPos,l_Term->aTermBuf->tScrX)) {
							/* also scroll input line forward one */
							l_Term->aInputDispCurPos=ATIB_NEXTCHAR(l_Term,l_Term->aInputDispCurPos);
							GetClientRect(l_Term->aWnd,&l_rect);
							l_rect.top=l_rect.bottom-l_Term->aInputWndDY;
							InvalidateRect(l_Term->aWnd,&l_rect,FALSE);
						}
						atUpdateCaret(l_Term);
					}
					return 0L;
				}
				/* send ASCII Crsr Right */
				l_buf[0]=(unsigned char)ASCII_ESC;
				l_buf[1]=(unsigned char)'[';
				l_buf[2]=(unsigned char)'C';
				atSendTermInput(l_Term,l_buf,3);
			} else if (l_i==0x26) { /* Cursor UP */
				/* send ASCII Crsr Up */
				if (l_Term->aSeparateInputLine) {
					/* advance one command back and copy this to our current buffer */
					if (l_Term->aInputScrollBackPos!=l_Term->aInputCurLineEnd) {
						/* first, find our previous command in the history */
						l_p=ATIB_PREVCHAR(l_Term,l_Term->aInputScrollBackPos);
						l_p=ATIB_PREVCHAR(l_Term,l_p);
						/* find the start of this command & count characters */
						l_i=0;
						while(*l_p) {
							l_p=ATIB_PREVCHAR(l_Term,l_p);
							l_i++;
						}
						/* we are one past the start of our command... */
						l_p=ATIB_NEXTCHAR(l_Term,l_p);
						/* we don't need to adjust l_i kuz we started 1 lower than we should have */
						if (l_p!=l_Term->aInputCurLine) {
							/* this is a valid command. Let's count the size of our current
							 * input line and ensure there's enuf space to copy this command over */
							l_q=l_Term->aInputCurLine;
							while((l_i) && (l_q!=l_p)) {
								l_q=ATIB_NEXTCHAR(l_Term,l_q);
								l_i--;
							}
							if (l_q!=l_p) {
								/* there's enough space -- let's copy this sucker over and update
								 * our scroll back position */
								l_Term->aInputScrollBackPos=l_p;
								/* clear current input line */
								for (l_q=l_Term->aInputCurLine;
										 l_q!=l_Term->aInputCurLineEnd;
										 l_q=ATIB_NEXTCHAR(l_Term,l_q))
									*l_q=0x00;
								l_Term->aInputCurPos=l_Term->aInputDispCurPos=l_Term->aInputCurLine;
								/* copy history cmd into our current input buffer */
								for (l_q=l_p;*l_q;l_q=ATIB_NEXTCHAR(l_Term,l_q))
									atSendTermInput(l_Term,l_q,1);
								/* now refresh line */
								GetClientRect(l_Term->aWnd,&l_rect);
								l_rect.top=l_rect.bottom-l_Term->aInputWndDY;
								InvalidateRect(l_Term->aWnd,&l_rect,FALSE);
							} /* else our buffer isn't big enuf for previous cmd -- ignore */
						}
					} /* else there is no previous cmd */
				} else {
					l_buf[0]=(unsigned char)ASCII_ESC;
					l_buf[1]=(unsigned char)'[';
					l_buf[2]=(unsigned char)'A';
					atSendTermInput(l_Term,l_buf,3);
				}
			} else if (l_i==0x28) { /* Cursor DOWN */
				/* send ASCII Crsr Down */
				if (l_Term->aSeparateInputLine) {
					/* advance one command forward and copy this to our current buffer */
					if (l_Term->aInputScrollBackPos!=l_Term->aInputCurPos) {
						/* first, find our next command in the history */
						l_p=l_Term->aInputScrollBackPos;
						while (*l_p) {
							l_p=ATIB_NEXTCHAR(l_Term,l_p);
						}
						l_p=ATIB_NEXTCHAR(l_Term,l_p); /* get past ending 0x00 */
						l_Term->aInputScrollBackPos=l_p;
						if (l_Term->aInputScrollBackPos!=l_Term->aInputCurPos) {
							/* clear current input line */
							for (l_q=l_Term->aInputCurLine;
								 l_q!=l_Term->aInputCurLineEnd;
								 l_q=ATIB_NEXTCHAR(l_Term,l_q))
								*l_q=0x00;
							l_Term->aInputCurPos=l_Term->aInputDispCurPos=l_Term->aInputCurLine;
							/* copy history cmd into our current input buffer */
							for (l_q=l_p;*l_q;l_q=ATIB_NEXTCHAR(l_Term,l_q))
								atSendTermInput(l_Term,l_q,1);
							/* now refresh line */
							GetClientRect(l_Term->aWnd,&l_rect);
							l_rect.top=l_rect.bottom-l_Term->aInputWndDY;
							InvalidateRect(l_Term->aWnd,&l_rect,FALSE);
						}
					}
				} else {
					l_buf[0]=(unsigned char)ASCII_ESC;
					l_buf[1]=(unsigned char)'[';
					l_buf[2]=(unsigned char)'B';
					atSendTermInput(l_Term,l_buf,3);
				}
			} else if (l_i==0x2D) { /* INS */
				if (GetKeyState(VK_SHIFT)&0x00008000) {
					/* paste */
					FORWARD_WM_COMMAND(p_hWnd,CM_ANSIEDITPASTE,NULL,0,PostMessage);
				}	else if (GetKeyState(VK_CONTROL)&0x00008000) {
					/* copy */
					FORWARD_WM_COMMAND(p_hWnd,CM_ANSIEDITCOPY,NULL,0,PostMessage);
				} else {
				/* send INS */
				l_buf[0]=(unsigned char)ASCII_ESC;
				l_buf[1]=(unsigned char)'[';
				l_buf[2]=(unsigned char)'2';
				l_buf[3]=(unsigned char)'~';
				atSendTermInput(l_Term,l_buf,4);
				}
			} else if (l_i==0x2E) { /* DEL */
				/* pass on DEL keystroke */
				l_buf[0]=(unsigned char)ASCII_DEL;
				atSendTermInput(l_Term,l_buf,1);
				return 0L;
//				/* send DEL */
//				l_buf[0]=(unsigned char)ASCII_ESC;
//				l_buf[1]=(unsigned char)'[';
//				l_buf[2]=(unsigned char)'3';
//				l_buf[2]=(unsigned char)'~';
//				atSendTermInput(l_Term,l_buf,4);
			}
			break;
//      sprintf(l_buf,"WM_KEYDOWN: %04X repeat %i, scan code %02X, %s, %s %s %s",
//        p_wParam,p_lParam&(0x0000FFFF),(p_lParam&0x00FF0000)>>16,
//        (p_lParam&0x01000000)?"extended":"not extended",
//        (p_lParam&(1<<29))?" ALT ":"",l_i&0x8000?"SHIFT":"",l_j&0x8000?"CONTROL":"");
//      dbPrint(l_buf);

		case WM_CHAR:
			l_buf[0]=(unsigned char)p_wParam;
			atSendTermInput(l_Term,l_buf,1);
			/* return moves scroll back to normal position */
			if (((unsigned char)p_wParam)==ASCII_CR)
				atScrollTermTo(l_Term,0,0);
//      if (!(p_lParam&0x01000000)) {
//        /* regular key stroke */
//        l_buf[0]=(unsigned char)p_wParam;
//        atSendTermInput(l_Term,l_buf,1);
//      }
			return 0L;
//      sprintf((char*)l_buf,"WM_CHAR: %04X repeat %i, scan code %02X, %s, %s ",
//        p_wParam,p_lParam&(0x0000FFFF),(p_lParam&0x00FF0000)>>16,
//        (p_lParam&0x01000000)?"extended":"not extended",
//        (p_lParam&(1<<29))?" ALT ":"");
//      dbPrint((char*)l_buf);
		case WM_PAINT:
//			if (IsIconic(p_hWnd))
//				break;
			/* first, check for scrolling */
			if (l_Term->aScroll) {
				if (l_Term->aSeparateInputLine)
					ScrollWindow(l_Term->aWnd,0,l_Term->aScroll,
						&(l_Term->aScrollRect),&(l_Term->aScrollRect));
				else
					ScrollWindow(l_Term->aWnd,0,l_Term->aScroll,NULL,NULL);
				l_Term->aScroll=0;
			}
			/* Paint window */
			if (GetUpdateRect(p_hWnd,&l_rect,TRUE)) { /* get new update region */
				l_hDC=BeginPaint(p_hWnd,&l_ps);
				l_oldFont=SelectObject(l_hDC,l_Term->aFont);
				/* first, determine our line numbers to update */
				if (l_Term->aSeparateInputLine) {
					if (l_rect.top < l_Term->aScrollRect.bottom) {
						l_startY=(l_rect.top/l_Term->aFontY)+l_Term->aShowY;
					} else {
						l_startY=-1;
					}
					if (l_rect.bottom < l_Term->aScrollRect.bottom) {
						l_endY=((l_rect.bottom+l_Term->aFontY-1)/l_Term->aFontY)+l_Term->aShowY;
					} else {
						l_endY=((l_Term->aScrollRect.bottom-1)/l_Term->aFontY)+l_Term->aShowY;

						/* paint input line */
						l_startX=(l_rect.left/l_Term->aFontX);
						l_endX=((l_rect.right+l_Term->aFontX-1)/l_Term->aFontX);

						SetBkColor(l_hDC,l_Term->aTermBuf->tColourBank[l_Term->aInputBColour]);
						SetTextColor(l_hDC,l_Term->aTermBuf->tColourBank[l_Term->aInputFColour]);

						l_p=l_Term->aInputDispCurPos;
						for (l_x=0;l_x<l_startX;l_x++) {
							if (*l_p)
								l_p++;
						}
						l_count=0;
						for (l_x=l_startX;l_x<l_endX;l_x++) {
							if (*l_p) {
								l_buf[l_count]=*l_p;
								l_p=ATIB_NEXTCHAR(l_Term,l_p);
							} else {
								l_buf[l_count]=' ';
							}
							l_count++;
						}
						TextOut(l_hDC,l_startX*l_Term->aFontX,
							(l_Term->aScrollRect.bottom+l_Term->aInputBorderDY),
							(LPCSTR)l_buf,l_count);

						/* and don't forget to paint the separation line accross the screen */
						l_oldPen=SelectObject(l_hDC,GetStockObject(WHITE_PEN));
						MoveToEx(l_hDC,l_rect.left,l_Term->aScrollRect.bottom+1,NULL);
						LineTo(l_hDC,l_rect.right,l_Term->aScrollRect.bottom+1);
						SelectObject(l_hDC,l_oldPen);
					}
				} else {
					l_startY=(l_rect.top/l_Term->aFontY)+l_Term->aShowY;
					l_endY=((l_rect.bottom-1)/l_Term->aFontY)+l_Term->aShowY;
				}
				l_startX=(l_rect.left/l_Term->aFontX)+l_Term->aShowX;
				l_endX=((l_rect.right-1)/l_Term->aFontX)+l_Term->aShowX;
				if (l_startX>TB_GETXMAX(l_Term->aTermBuf))
					l_startX=TB_GETXMAX(l_Term->aTermBuf);
				if (l_endX>TB_GETXMAX(l_Term->aTermBuf))
					l_endX=TB_GETXMAX(l_Term->aTermBuf);
				if (l_startY>TB_GETYMAX(l_Term->aTermBuf))
					l_startY=TB_GETYMAX(l_Term->aTermBuf);
				if (l_endY>TB_GETYMAX(l_Term->aTermBuf))
					l_endY=TB_GETYMAX(l_Term->aTermBuf);
				/* paint rest of screen, if required */
				/* first, let's make sure we're using the correct colour -- we do this by
				 * ensuring we have the WRONG attrib */
				l_rawCharP=TB_GETCHARADDR(l_Term->aTermBuf,l_startX,l_startY);
				if (IsBadReadPtr(l_rawCharP,1)) {
#ifndef DB_DEBUG_NODEBUG
					sprintf((char*)l_buf,"SNAG!!! =========================== x=%i y=%i bx=%i by=%i",
						l_startX,l_startY,l_Term->aTermBuf->tScrX,l_Term->aTermBuf->tScrY);
					dbPrint((char*)l_buf);
#endif
				} else {
					l_rawChar=*l_rawCharP;
					l_attrib=l_rawChar&(TB_EFFECTMASK|TB_BCOLOURMASK|TB_FCOLOURMASK);
					l_attrib^=0xFFFFFFFFL;
					l_count=0; /* l_count counts how many characters we have in our buffer so far */
					for (l_y=l_startY;l_y<=l_endY;l_y++) {
						for (l_x=l_startX;l_x<=l_endX;l_x++) {
							/* print this character */
							l_rawCharP=TB_GETCHARADDR(l_Term->aTermBuf,l_x,l_y);
							if (!IsBadReadPtr(l_rawCharP,1)) {
								l_rawChar=*l_rawCharP;
							} else {
								dbPrint("SNAG TOO!!!!! ////////////////////////////////////");
								l_rawChar=TB_MAKEEFFECT(0)|TB_MAKEBCOLOUR(0)|TB_MAKEFCOLOUR(7)|TB_MAKECHAR(0x20);
							}
							if (tbIsHilight(l_Term->aTermBuf,l_x,l_y)) {
								l_rawChar^=(TB_BCOLOURMASK|TB_FCOLOURMASK);
							}
							if ((l_attrib!=(l_rawChar&(TB_EFFECTMASK|TB_BCOLOURMASK|TB_FCOLOURMASK)))||
								(l_count>=(ATANSITERMPROCBUFSIZE-1))) {
								if (l_count) {
									if (l_Term->aFontUnderline) {
										/* check for underline */
										if (l_attrib&TB_MAKEEFFECT(AT_FONTEFFECT_UNDERLINE)) {
											SelectObject(l_hDC,l_Term->aFontUnderline);
											TextOut(l_hDC,(l_x-l_count-l_Term->aShowX)*l_Term->aFontX,
												(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,l_count);
											SelectObject(l_hDC,l_Term->aFont);
										} else {
											TextOut(l_hDC,(l_x-l_count-l_Term->aShowX)*l_Term->aFontX,
											(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,l_count);
										}
									} else {
										TextOut(l_hDC,(l_x-l_count-l_Term->aShowX)*l_Term->aFontX,
											(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,l_count);
										/* check for underline */
										if (l_attrib&TB_MAKEEFFECT(AT_FONTEFFECT_UNDERLINE)) {
											l_oldPen=SelectObject(l_hDC,
												l_Term->aTermBuf->tColourPen[l_i]);
											MoveToEx(l_hDC,(l_x-l_count-l_Term->aShowX)*l_Term->aFontX,
												(l_y-l_Term->aShowY+1)*l_Term->aFontY-1,NULL);
											LineTo(l_hDC,(l_x-l_Term->aShowX+1)*l_Term->aFontX,
												(l_y-l_Term->aShowY+1)*l_Term->aFontY-1);
											l_oldPen=SelectObject(l_hDC,l_oldPen);
										}
									}
									l_count=0;
								}


//								if ((l_attrib&TB_BCOLOURMASK)!=(l_rawChar&TB_BCOLOURMASK)) {
//									SetBkColor(l_hDC,l_Term->aTermBuf->tColourBank[TB_EXTBCOLOUR(l_rawChar)]);
//								}
//								if ((l_attrib&TB_FCOLOURMASK)!=(l_rawChar&TB_FCOLOURMASK)) {
//									SetTextColor(l_hDC,l_Term->aTermBuf->tColourBank[TB_EXTFCOLOUR(l_rawChar)]);
//								}
//								if ((l_attrib&TB_EFFECTMASK)!=(l_rawChar&TB_EFFECTMASK)) {
								l_i=TB_EXTFCOLOUR(l_rawChar);
								l_j=TB_EXTBCOLOUR(l_rawChar);
								if (TB_EXTEFFECT(l_rawChar)&AT_FONTEFFECT_BOLD) {
									l_i|=0x08;
								}
//									if (TB_EXTEFFECT(l_rawChar)&AT_FONTEFFECT_UNDERLINE) {
//									} else {
//									}
								if (TB_EXTEFFECT(l_rawChar)&AT_FONTEFFECT_BLINK) {
									l_j|=0x08;
								}
								if (TB_EXTEFFECT(l_rawChar)&AT_FONTEFFECT_REVERSE) {
									/* this is a bit complicated */
									if (l_i==0x08)
										l_i=0x00;
									else
										l_i^=0x07;
									if (l_j==0x08)
										l_j=0x00;
									else
										l_j^=0x07;
								}
								SetTextColor(l_hDC,l_Term->aTermBuf->tColourBank[l_i]);
								SetBkColor(l_hDC,l_Term->aTermBuf->tColourBank[l_j]);
//								}
								l_attrib=(l_rawChar&(TB_EFFECTMASK|TB_BCOLOURMASK|TB_FCOLOURMASK));
							}
							l_buf[l_count]=(char)TB_EXTCHAR(l_rawChar);
							l_count++;
							/* Note: The following commented-out code used to draw the
							 * screen one character at a time. This was deathly slow.
							 * Therefore, I copied this to two places: above, and below. */
//							if (l_Term->aFontUnderline) {
//								/* check for underline */
//								if (l_attrib&TB_MAKEEFFECT(AT_FONTEFFECT_UNDERLINE)) {
//									SelectObject(l_hDC,l_Term->aFontUnderline);
//									TextOut(l_hDC,(l_x-l_Term->aShowX)*l_Term->aFontX,
//										(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,1);
// 									SelectObject(l_hDC,l_Term->aFont);
//								} else {
//									TextOut(l_hDC,(l_x-l_Term->aShowX)*l_Term->aFontX,
// 										(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,1);
//								}
//							} else {
//								TextOut(l_hDC,(l_x-l_Term->aShowX)*l_Term->aFontX,
//									(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,1);
//								/* check for underline */
//								/* actually, I should swap in a new font... but one is,
// 								 * for whatever reason, unavailable. So I fake it. */
//								if (l_attrib&TB_MAKEEFFECT(AT_FONTEFFECT_UNDERLINE)) {
//									l_oldPen=SelectObject(l_hDC,
// 										l_Term->aTermBuf->tColourPen[l_i]);
//									MoveToEx(l_hDC,(l_x-l_Term->aShowX)*l_Term->aFontX,
//										(l_y-l_Term->aShowY+1)*l_Term->aFontY-1,NULL);
//									LineTo(l_hDC,(l_x-l_Term->aShowX+1)*l_Term->aFontX,
//										(l_y-l_Term->aShowY+1)*l_Term->aFontY-1);
//									l_oldPen=SelectObject(l_hDC,l_oldPen);
//								}
//							}
						}
						if (l_count) {
							if (l_Term->aFontUnderline) {
								/* check for underline */
								if (l_attrib&TB_MAKEEFFECT(AT_FONTEFFECT_UNDERLINE)) {
									SelectObject(l_hDC,l_Term->aFontUnderline);
									TextOut(l_hDC,(l_x-l_count-l_Term->aShowX)*l_Term->aFontX,
										(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,l_count);
									SelectObject(l_hDC,l_Term->aFont);
								} else {
									TextOut(l_hDC,(l_x-l_count-l_Term->aShowX)*l_Term->aFontX,
										(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,l_count);
								}
							} else {
								TextOut(l_hDC,(l_x-l_count-l_Term->aShowX)*l_Term->aFontX,
									(l_y-l_Term->aShowY)*l_Term->aFontY,(LPCSTR)l_buf,l_count);
								/* check for underline */
								if (l_attrib&TB_MAKEEFFECT(AT_FONTEFFECT_UNDERLINE)) {
									l_oldPen=SelectObject(l_hDC,
										l_Term->aTermBuf->tColourPen[l_i]);
									MoveToEx(l_hDC,(l_x-l_count-l_Term->aShowX)*l_Term->aFontX,
										(l_y-l_Term->aShowY+1)*l_Term->aFontY-1,NULL);
									LineTo(l_hDC,(l_x-l_Term->aShowX+1)*l_Term->aFontX,
										(l_y-l_Term->aShowY+1)*l_Term->aFontY-1);
									l_oldPen=SelectObject(l_hDC,l_oldPen);
								}
							}
							l_count=0;
						}
					}
				}
				SelectObject(l_hDC,l_oldFont);
				EndPaint(p_hWnd,&l_ps);
			}
			atUpdateCaret(l_Term);
			return 0L;
		case WM_SETFOCUS:
			CreateCaret(p_hWnd, NULL, (l_Term->aFontX)>>1, l_Term->aFontY);
			atUpdateCaret(l_Term);
			return 0L;
		case WM_KILLFOCUS:
			DestroyCaret();
			return 0L;
		case WM_DESTROY:
			enSaveWindowPlacement(l_Term->aEnvSection,l_Term->aWnd);
			return 0L;
		case WM_WINDOWPOSCHANGING:
			/* let's adjust our cx and cy so that our window is perfectly
			 * aligned with our row/column borders and we don't have any
			 * columns/rows half-displayed. */
			if (((WINDOWPOS*)(p_lParam))->flags&SWP_NOSIZE)
				break;
			if (IsIconic(p_hWnd))
				break;

			l_i=(((WINDOWPOS*)(p_lParam))->cx)-2*GetSystemMetrics(SM_CXFRAME)-
				GetSystemMetrics(SM_CXVSCROLL)+GetSystemMetrics(SM_CXBORDER);
			l_j=(((WINDOWPOS*)(p_lParam))->cy)-2*GetSystemMetrics(SM_CYFRAME)-
				GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYMENU);

			/* make our adjustment for our input line */
  		if (l_Term->aSeparateInputLine) {
        l_j-=(l_Term->aInputWndDY+l_Term->aInputBorderDY);
        if (l_j<0)
          l_j=0;
      }

			/* calculate our closest dimentions */
			l_x=(l_i+(l_Term->aFontX/2))/l_Term->aFontX;
			l_y=(l_j+(l_Term->aFontY/2))/l_Term->aFontY;
			if (l_x<ATIB_MINX)
				l_x=ATIB_MINX;
			if (l_y<ATIB_MINY)
        l_y=ATIB_MINY;

			/* while here, update our recorded scroll region */
			l_Term->aScrollRect.left=0;
			l_Term->aScrollRect.top=0;
			l_Term->aScrollRect.right=l_x*l_Term->aFontX;
			l_Term->aScrollRect.bottom=l_y*l_Term->aFontY;

			/* now get column and row "remainders */
			l_i-=(l_x*l_Term->aFontX);
			l_j-=(l_y*l_Term->aFontY);

			/* and make changes to the WINDOWPOS struct */
			((WINDOWPOS*)(p_lParam))->cx-=l_i;
			((WINDOWPOS*)(p_lParam))->cy-=l_j;

			/* and then let the window handler continue it's job */
			break;
		case WM_SIZE:
			if (IsIconic(p_hWnd))
				break;
			atAdjustTerm(l_Term,-1);
			atUpdateCaret(l_Term);
			break;
		default:
			break;
		}
	if (l_Term)
		if (l_Term->aAuxMsgHandler)
			return (*(l_Term->aAuxMsgHandler))(l_Term,p_hWnd,p_message,p_wParam,p_lParam);
	return (DefWindowProc(p_hWnd, p_message, p_wParam, p_lParam));
	}

/* This proc allows a higher module to fetch data from the user input buffer,
 * presumably to send it to a socket */
long atReadTermInput(ATTERM *p_Term, unsigned char *p_Buf, unsigned long p_BufLen) {
	unsigned long l_i;

	if (!p_Term)
		return 0L;
	l_i=0;
	while ((p_Term->aOutBufSockTail!=p_Term->aOutBufHead)&&(l_i<p_BufLen)) {
		p_Buf[l_i]=*(p_Term->aOutBufSockTail);
		l_i++;
		p_Term->aOutBufSockTail++;
		if ((p_Term->aOutBufSockTail-p_Term->aOutBuf)>=p_Term->aOutBufSize)
			p_Term->aOutBufSockTail=p_Term->aOutBuf;
	}
	return l_i;
}

long atUnReadTermInput(ATTERM *p_Term, unsigned char *p_Buf, unsigned long p_BufLen) {
	unsigned long l_i;

	if (!p_Term)
		return 0L;
	l_i=0;
	while ((p_Term->aOutBufSockTail!=p_Term->aOutBufHead)&&(l_i<p_BufLen)) {
		p_Term->aOutBufSockTail--;
		if (p_Term->aOutBufSockTail<p_Term->aOutBuf)
			p_Term->aOutBufSockTail+=p_Term->aOutBufSize;
		*(p_Term->aOutBufSockTail)=p_Buf[l_i];
		l_i++;
	}
	return l_i;
}

/* This proc inserts a string into the output buffer for the terminal. */
long atWriteTermInput(ATTERM *p_Term, unsigned char *p_Buf, unsigned long p_BufLen) {
	unsigned long l_i;

	l_i=0;
	if (p_Term) {
		while (l_i<p_BufLen) {
			*(p_Term->aOutBufHead)=p_Buf[l_i];
			l_i++;
			(p_Term->aOutBufHead)++;
			if ((p_Term->aOutBufHead-p_Term->aOutBuf)>=p_Term->aOutBufSize)
				p_Term->aOutBufHead=p_Term->aOutBuf;
			if (p_Term->aOutBufHead==p_Term->aOutBufSockTail) {
				p_Term->aOutBufHead--;
				if (p_Term->aOutBufHead<p_Term->aOutBuf)
					p_Term->aOutBufHead=p_Term->aOutBuf+p_Term->aOutBufSize-1;
				l_i--;
				break;
			}
		}
	}
	return l_i;
}

/* This proc processes a string as though the user typed it. */
void atSendTermInput(ATTERM *p_Term, unsigned char *p_Buf, unsigned long p_BufLen) {
	int l_i;
	unsigned char l_outStr[256];
  unsigned char *l_p,*l_q;
  RECT l_rect;

	if (p_Term) {
    if (p_Term->aSeparateInputLine) {
      for (l_i=0;l_i<p_BufLen;l_i++) {
        if (p_Buf[l_i]==p_Term->aDeleteChar) {
          /* delete next character after cursor (if there is one) */
          l_p=p_Term->aInputCurPos;
          while(*l_p) {
            l_q=ATIB_NEXTCHAR(p_Term,l_p);
            *l_p=*l_q;
            l_p=l_q;
					}
          /* now refresh line -- from cursor on */
          GetClientRect(p_Term->aWnd,&l_rect);
          l_rect.top=l_rect.bottom-p_Term->aInputWndDY;
          l_rect.left=p_Term->aFontX*(p_Term->aInputCurPos-p_Term->aInputDispCurPos);
					InvalidateRect(p_Term->aWnd,&l_rect,FALSE);
				} else if (p_Buf[l_i]==p_Term->aBackSpaceChar) {
					/* delete previous character from cursor (if there is one) */
					if (p_Term->aInputCurPos!=p_Term->aInputCurLine) {
						l_p=ATIB_PREVCHAR(p_Term,p_Term->aInputCurPos);
						p_Term->aInputCurPos=l_p;
						while(*l_p) {
							l_q=ATIB_NEXTCHAR(p_Term,l_p);
							*l_p=*l_q;
							l_p=l_q;
						}
						/* now refresh line -- from cursor on */
						GetClientRect(p_Term->aWnd,&l_rect);
						l_rect.top=l_rect.bottom-p_Term->aInputWndDY;
						l_rect.left=p_Term->aFontX*(p_Term->aInputCurPos-p_Term->aInputDispCurPos);
						InvalidateRect(p_Term->aWnd,&l_rect,FALSE);
					}
				} else if (p_Buf[l_i]==ASCII_CR) {
					l_q=l_outStr;
					for (l_p=p_Term->aInputCurLine;(*l_p);l_p=ATIB_NEXTCHAR(p_Term,l_p)) {
						*l_q=*l_p;
						l_q++;
						if (l_q-l_outStr>250) {
							/* send this chunk and then continue */
							*l_q=0;
							atWriteTermInput(p_Term,(unsigned char*)l_outStr,l_q-l_outStr);
							if (p_Term->aVLocalEcho) {
								atSendTerm(p_Term,l_outStr,l_q-l_outStr);
							}
							l_q=l_outStr;
						}
          }
          if (l_q!=l_outStr) {
            atWriteTermInput(p_Term,(unsigned char*)l_outStr,l_q-l_outStr);
            if (p_Term->aVLocalEcho) {
              atSendTerm(p_Term,l_outStr,l_q-l_outStr);
            }
          }
          /* now send return */
          l_q=l_outStr;
          *l_q=ASCII_CR;
          l_q++;
          if (p_Term->aLFwithNLMode) {
            *l_q=ASCII_LF;
            l_q++;
          }
          atWriteTermInput(p_Term,(unsigned char*)l_outStr,l_q-l_outStr);
          if (p_Term->aVLocalEcho) {
            atSendTerm(p_Term,l_outStr,l_q-l_outStr);
          }
          (*(p_Term->aNotify))(p_Term,TBNOTIFY_USERINPUT);
					/* now advance our input line */
					for (l_p=p_Term->aInputCurLine;*l_p;l_p=ATIB_NEXTCHAR(p_Term,l_p));
					/* note: l_p should be pointing to our 0x00 terminator for this line */
					if (l_p!=p_Term->aInputCurLine) /* check for null line - don't want'em in our history */
	          l_p=ATIB_NEXTCHAR(p_Term,l_p);
          /* l_p now points to the first character of our new line */
          *l_p=0x00;
          p_Term->aInputCurLine=l_p;
          p_Term->aInputCurPos=l_p;
          p_Term->aInputDispCurPos=l_p;
          while(!(*l_p)) {
            l_p=ATIB_NEXTCHAR(p_Term,l_p);
          }
					p_Term->aInputCurLineEnd=l_p;
					p_Term->aInputScrollBackPos=p_Term->aInputCurLine;
          /* and finally, redraw input line */
          GetClientRect(p_Term->aWnd,&l_rect);
          l_rect.top=l_rect.bottom-p_Term->aInputWndDY;
					InvalidateRect(p_Term->aWnd,&l_rect,FALSE);
          atUpdateCaret(p_Term);
				} else {
          /* insert character into line */
          l_p=p_Term->aInputCurPos;
          if (*l_p==0x00) {
            *(p_Term->aInputCurPos)=p_Buf[l_i];
            /* invalidate that region of the screen */
            GetClientRect(p_Term->aWnd,&l_rect);
            l_rect.top=l_rect.bottom-p_Term->aInputWndDY;
            l_rect.left=p_Term->aFontX*(p_Term->aInputCurPos-p_Term->aInputDispCurPos);
            l_rect.right=l_rect.left+p_Term->aFontX;
            InvalidateRect(p_Term->aWnd,&l_rect,FALSE);
            atUpdateCaret(p_Term);
						l_p=ATIB_NEXTCHAR(p_Term,p_Term->aInputCurPos);
            p_Term->aInputCurPos=l_p;
          } else {
            /* we have to first move the existing line over one, from the cursor on */
            l_p=ATIB_PREVCHAR(p_Term,p_Term->aInputCurLineEnd);
            while(l_p!=p_Term->aInputCurPos) {
              l_q=ATIB_PREVCHAR(p_Term,l_p);
              *l_p=*l_q;
              l_p=l_q;
            }
            *(p_Term->aInputCurPos)=p_Buf[l_i];
            /* invalidate that region of the screen - from that char to end of line */
            GetClientRect(p_Term->aWnd,&l_rect);
            l_rect.top=l_rect.bottom-p_Term->aInputWndDY;
            l_rect.left=p_Term->aFontX*(p_Term->aInputCurPos-p_Term->aInputDispCurPos);
            InvalidateRect(p_Term->aWnd,&l_rect,FALSE);
            atUpdateCaret(p_Term);
            l_p=ATIB_NEXTCHAR(p_Term,p_Term->aInputCurPos);
            p_Term->aInputCurPos=l_p;
            /* we now need to find the end of our line */
            while((*l_p)&&(l_p!=p_Term->aInputCurLineEnd))
              l_p=ATIB_NEXTCHAR(p_Term,l_p);
          }
          /* check end of line -- l_p should point to end of our character line for this to work */
          if (l_p==p_Term->aInputCurLineEnd) {
						/* uh oh, we have to delete a history command */
            do {
              *l_p=0x00;
              l_p=ATIB_NEXTCHAR(p_Term,l_p);
            } while ((*l_p)&&(l_p!=p_Term->aInputCurLine));
            if (l_p==p_Term->aInputCurLine) {
              dbPrint("ACK EEEEK OOOOOOOO input line overrun!!!\n");
              l_p=ATIB_PREVCHAR(p_Term,l_p);
            }
            p_Term->aInputCurLineEnd=l_p;
          }
          /* check if we have to reposition our line */
          if (p_Term->aInputCurPos==ATIB_ADDCHAR(p_Term,
              p_Term->aInputDispCurPos,p_Term->aTermBuf->tScrX)) {
            p_Term->aInputDispCurPos=ATIB_NEXTCHAR(p_Term,p_Term->aInputDispCurPos);
            GetClientRect(p_Term->aWnd,&l_rect);
						l_rect.top=l_rect.bottom-p_Term->aInputWndDY;
						InvalidateRect(p_Term->aWnd,&l_rect,FALSE);
            atUpdateCaret(p_Term);
          }
        }
      }
    } else {
      /* send data immediately */
      for (l_i=0;l_i<p_BufLen;l_i++) {
				/* do keyboard translation NOW */
				if (p_Buf[l_i]==0x7F)
					*l_outStr=p_Term->aDeleteChar;
        else if (p_Buf[l_i]==0x08)
          *l_outStr=p_Term->aBackSpaceChar;
        else
          *l_outStr=p_Buf[l_i];

        /* insert into our output buffer */
        atWriteTermInput(p_Term,(unsigned char*)l_outStr,1L);
        /* check echo to the screen */
        if (p_Term->aVLocalEcho) {
          if ((*l_outStr>=0x20)&&(*l_outStr<0x7F)) {
            atSendTerm(p_Term,l_outStr, 1);
            (p_Term->aLE_BSCount)++;
          } else {
            if ((*l_outStr==ASCII_CR)&&(p_Term->aLFwithNLMode)) {
              l_outStr[1]=ASCII_LF;
              atSendTerm(p_Term,(unsigned char *)l_outStr, 2);
              p_Term->aLE_BSCount=0;
            } else if (*l_outStr==ASCII_BS) {
              if (p_Term->aLE_BSCount) {
                atSendTerm(p_Term,(unsigned char *)l_outStr, 1);
                p_Term->aLE_BSCount--;
              } /* else don't echo */
						} else {
              atSendTerm(p_Term,(unsigned char *)l_outStr, 1);
              p_Term->aLE_BSCount++;
						}
          }
        }
      }
      (*(p_Term->aNotify))(p_Term,TBNOTIFY_USERINPUT);
    }
	}
}

/* clears and resets the input line */
void atResetSeparateInputLine(ATTERM *p_Term) {
  unsigned long l_i;
  if (p_Term) {
		/* let's reset this sucker */
		p_Term->aInputCurLine=p_Term->aInputBuf;
		p_Term->aInputCurLineEnd=p_Term->aInputBuf;
		p_Term->aInputCurPos=p_Term->aInputBuf;
		p_Term->aInputDispCurPos=p_Term->aInputBuf;
		p_Term->aInputScrollBackPos=p_Term->aInputCurLine;
    for(l_i=0;l_i<p_Term->aInputBufSize;l_i++)
      (p_Term->aInputBuf)[l_i]=0x00;
  }
}

BOOL CALLBACK _export atAnsiTermOptionsProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
	ATTERM *l_Term;
	BOOL l_bool;
	RECT l_rect;
	long l_i;

	switch (p_message)
		{
		case WM_INITDIALOG:
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
			dlCentreDialogBox(p_hWnd);
			l_Term=(ATTERM *)p_lParam;
			if (!l_Term)
				EndDialog(p_hWnd,IDCANCEL);
#ifdef WIN32
			SetProp(p_hWnd,(LPCSTR)g_atLongHi,(HANDLE)(p_lParam));
#else
			SetProp(p_hWnd,(LPCSTR)g_atLongHi,HIWORD(p_lParam));
			SetProp(p_hWnd,(LPCSTR)g_atLongLo,LOWORD(p_lParam));
#endif
			/* Ok, let's setup our window to look nice & pretty for the user */
			/* Emulation */
			switch (l_Term->aEmulation) {
				case AT_EMU_VT100:
					CheckRadioButton(p_hWnd,IDC_ATOPTEMUNONE,IDC_ATOPTEMUVT100,IDC_ATOPTEMUVT100);
					break;
				case AT_EMU_NONE:
				default:
					CheckRadioButton(p_hWnd,IDC_ATOPTEMUNONE,IDC_ATOPTEMUVT100,IDC_ATOPTEMUNONE);
					break;
			}
			/*  Separate Input Line */
			if (l_Term->aSeparateInputLine)
				CheckDlgButton(p_hWnd,IDC_ATOPTINPUTLINE,TRUE);
			else
				CheckDlgButton(p_hWnd,IDC_ATOPTINPUTLINE,FALSE);
			/* local echo */
			if (l_Term->aLocalEcho)
				CheckDlgButton(p_hWnd,IDC_ATOPTLOCALECHO,TRUE);
			else
				CheckDlgButton(p_hWnd,IDC_ATOPTLOCALECHO,FALSE);
			/* Bell */
			if (l_Term->aBell)
				CheckDlgButton(p_hWnd,IDC_ATOPTBELL,TRUE);
			else
				CheckDlgButton(p_hWnd,IDC_ATOPTBELL,FALSE);
			/* Backspace Character */
			if (l_Term->aBackSpaceChar==ASCII_BS)
				CheckRadioButton(p_hWnd,IDC_ATOPTBSBS,IDC_ATOPTBSDEL,IDC_ATOPTBSBS);
			else
				CheckRadioButton(p_hWnd,IDC_ATOPTBSBS,IDC_ATOPTBSDEL,IDC_ATOPTBSDEL);
			/* Delete Character */
			if (l_Term->aDeleteChar==ASCII_BS)
				CheckRadioButton(p_hWnd,IDC_ATOPTDELBS,IDC_ATOPTDELDEL,IDC_ATOPTDELBS);
			else
				CheckRadioButton(p_hWnd,IDC_ATOPTDELBS,IDC_ATOPTDELDEL,IDC_ATOPTDELDEL);
			/* Scrollback Buffer */
			SetDlgItemInt(p_hWnd,IDC_ATOPTSCROLLBUF,
				l_Term->aTermBuf->tBufY-l_Term->aTermBuf->tScrY-ATIB_TERMBUF_EXTRALINES,FALSE);
			return TRUE;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
					EndDialog(p_hWnd,IDCANCEL);
					return TRUE;
				default:
					break;
				}
			break;
		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
				{
				case IDOK:
					if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
						break;
#ifdef WIN32
					l_Term=(ATTERM *)GetProp(p_hWnd,(LPCSTR)g_atLongHi);
#else
					l_Term=(ATTERM *)MAKELP(GetProp(p_hWnd,(LPCSTR)g_atLongHi),
						GetProp(p_hWnd,(LPCSTR)g_atLongLo));
#endif
					/* make our adjustments */
					/* Emulation */
					if (IsDlgButtonChecked(p_hWnd,IDC_ATOPTEMUVT100)) {
						l_Term->aEmulation=AT_EMU_VT100;
					} else {
						l_Term->aEmulation=AT_EMU_NONE;
					}
					/*  Separate Input Line */
					l_bool=IsDlgButtonChecked(p_hWnd,IDC_ATOPTINPUTLINE);
					if (l_bool!=l_Term->aSeparateInputLine) {
						l_Term->aSeparateInputLine=l_bool;
						/* we have to reset our terminal here, or something */
						/* Let's do that by "bumping" our window -- it'll cause a recalculation
						 * of the window size, the scroll window, etc etc etc */
						GetWindowRect(l_Term->aWnd,&l_rect);
						MoveWindow(l_Term->aWnd,l_rect.left,l_rect.top,
							l_rect.right-l_rect.left+1,l_rect.bottom-l_rect.top+1,TRUE);
						/* and if we have a separate input line, make sure it's reset */
						if (l_bool)
							atResetSeparateInputLine(l_Term);
					}
					/* local echo */
					l_Term->aLocalEcho=IsDlgButtonChecked(p_hWnd,IDC_ATOPTLOCALECHO);
					l_Term->aVLocalEcho=l_Term->aLocalEcho;
					/* Bell */
					l_Term->aBell=IsDlgButtonChecked(p_hWnd,IDC_ATOPTBELL);
					/* Backspace Character */
					if (IsDlgButtonChecked(p_hWnd,IDC_ATOPTBSBS)) {
						l_Term->aBackSpaceChar=ASCII_BS;
					} else {
						l_Term->aBackSpaceChar=ASCII_DEL;
					}
					/* Delete Character */
					if (IsDlgButtonChecked(p_hWnd,IDC_ATOPTDELBS)) {
						l_Term->aDeleteChar=ASCII_BS;
					} else {
						l_Term->aDeleteChar=ASCII_DEL;
					}
					/* Scrollback buffer */
					l_i=GetDlgItemInt(p_hWnd,IDC_ATOPTSCROLLBUF,&l_bool,FALSE);
					if (l_bool) {
						atAdjustTerm(l_Term,l_i);
					}
					EndDialog(p_hWnd,IDOK);
					return TRUE;
				case IDHELP:
					if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
						break;
#ifdef WIN32
					l_Term=(ATTERM *)GetProp(p_hWnd,(LPCSTR)g_atLongHi);
#else
					l_Term=(ATTERM *)MAKELP(GetProp(p_hWnd,(LPCSTR)g_atLongHi),
						GetProp(p_hWnd,(LPCSTR)g_atLongLo));
#endif
					(*(l_Term->aNotify))(l_Term,TBNOTIFY_OPTIONSHELP);
					return TRUE;
				case IDCANCEL:
					if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
						break;
					EndDialog(p_hWnd,IDCANCEL);
					return TRUE;
				default:
					break;
				}
			break;
		case WM_DESTROY:
			RemoveProp(p_hWnd,(LPCSTR)g_atLongHi);
#ifndef WIN32
			RemoveProp(p_hWnd,(LPCSTR)g_atLongLo);
#endif
			break;
		default:
			break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
}


BOOL CALLBACK _export atATFnKeyProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
	int l_i,l_j,l_k;
	ATTERM *l_Term;

	switch (p_message)
		{
		case WM_INITDIALOG:
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
			dlCentreDialogBox(p_hWnd);
			l_Term=(ATTERM *)p_lParam;
			if (!l_Term)
				EndDialog(p_hWnd,IDCANCEL);
#ifdef WIN32
			SetProp(p_hWnd,(LPCSTR)g_atLongHi,(HANDLE)(p_lParam));
#else
			SetProp(p_hWnd,(LPCSTR)g_atLongHi,HIWORD(p_lParam));
			SetProp(p_hWnd,(LPCSTR)g_atLongLo,LOWORD(p_lParam));
#endif
			/* Ok, let's setup our window to look nice & pretty for the user */
			CheckRadioButton(p_hWnd,IDC_ATFNKEYBANKNONE,IDC_ATFNKEYBANKSHCT,IDC_ATFNKEYBANKNONE);
			for (l_i=0;l_i<12;l_i++) {
				SetDlgItemText(p_hWnd,IDC_ATFNKEYFN1+l_i,(char*)l_Term->aFnKeyMacro[l_i]);
			}
			SetProp(p_hWnd,"FnPage",(HANDLE)0);
			return TRUE;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
					EndDialog(p_hWnd,IDCANCEL);
					return TRUE;
				default:
					break;
				}
			break;
		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
				{
				case IDOK:
				case IDC_ATFNKEYBANKNONE:
				case IDC_ATFNKEYBANKSHIFT:
				case IDC_ATFNKEYBANKCTRL:
				case IDC_ATFNKEYBANKSHCT:
					if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
						break;
#ifdef WIN32
					l_Term=(ATTERM *)GetProp(p_hWnd,(LPCSTR)g_atLongHi);
#else
					l_Term=(ATTERM *)MAKELP(GetProp(p_hWnd,(LPCSTR)g_atLongHi),
						GetProp(p_hWnd,(LPCSTR)g_atLongLo));
#endif
					l_j=((int)(GetProp(p_hWnd,"FnPage")))*12;
					dbPrint("Macro page change");
					if(IsDlgButtonChecked(p_hWnd,IDC_ATFNKEYBANKNONE)) {
						dbPrint("None");
						l_k=0;
						SetProp(p_hWnd,"FnPage",(HANDLE)0);
					} else if(IsDlgButtonChecked(p_hWnd,IDC_ATFNKEYBANKSHIFT)) {
						dbPrint("Shift");
						l_k=12;
						SetProp(p_hWnd,"FnPage",(HANDLE)1);
					} else if(IsDlgButtonChecked(p_hWnd,IDC_ATFNKEYBANKCTRL)) {
						dbPrint("Ctrl");
						l_k=24l;
						SetProp(p_hWnd,"FnPage",(HANDLE)2);
					} else {
						dbPrint("Shift+Ctrl");
						l_k=36;
						SetProp(p_hWnd,"FnPage",(HANDLE)3);
					}
					for (l_i=0;l_i<12;l_i++) {
						/* first save our old setting */
						GetDlgItemText(p_hWnd,IDC_ATFNKEYFN1+l_i,
							(char*)l_Term->aFnKeyMacro[l_i+l_j],TBMACRO_MAXLEN);
						/* force a terminating 0x00 - I don't trust microshit */
						l_Term->aFnKeyMacro[l_i+l_j][TBMACRO_MAXLEN-1]=0x00;
						/* now set our new setting */
						SetDlgItemText(p_hWnd,IDC_ATFNKEYFN1+l_i,
							(char*)l_Term->aFnKeyMacro[l_i+l_k]);
					}
					if (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDOK) {
						/* update our options */
						l_Term->aBell=l_Term->aBell;
						EndDialog(p_hWnd,IDOK);
					}
					return TRUE;
				case IDHELP:
					if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
						break;
#ifdef WIN32
					l_Term=(ATTERM *)GetProp(p_hWnd,(LPCSTR)g_atLongHi);
#else
					l_Term=(ATTERM *)MAKELP(GetProp(p_hWnd,(LPCSTR)g_atLongHi),
						GetProp(p_hWnd,(LPCSTR)g_atLongLo));
#endif
					(*(l_Term->aNotify))(l_Term,TBNOTIFY_MACROSHELP);
					return TRUE;
				case IDCANCEL:
					if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
						break;
					EndDialog(p_hWnd,IDCANCEL);
					return TRUE;
				default:
					break;
				}
			break;
		case WM_DESTROY:
			RemoveProp(p_hWnd,"FnPage");
			RemoveProp(p_hWnd,(LPCSTR)g_atLongHi);
#ifndef WIN32
			RemoveProp(p_hWnd,(LPCSTR)g_atLongLo);
#endif
			break;
		default:
			break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
}

void atSaveTermSettings(ATTERM *p_Term, int p_Settings, char *p_Section) {
	char l_Buf[20];
	char *l_Sect;
	int l_i;

	if (!p_Term)
		return;

	if (p_Section)
		l_Sect=p_Section;
	else
		l_Sect=p_Term->aEnvSection;

	switch (p_Settings) {
		case TBNOTIFY_OPTIONS:
			enSaveLong(l_Sect,"Emulation",p_Term->aEmulation);
			enSaveInt(l_Sect,"SeparateInputLine",(unsigned int)p_Term->aSeparateInputLine);
			enSaveInt(l_Sect,"InputLineFColour",(unsigned int)p_Term->aInputFColour);
			enSaveInt(l_Sect,"InputLineBColour",(unsigned int)p_Term->aInputBColour);
			enSaveInt(l_Sect,"LocalEcho",(unsigned int)p_Term->aLocalEcho);
			enSaveInt(l_Sect,"Bell",(unsigned int)p_Term->aBell);
			enSaveInt(l_Sect,"BackSpaceChar",(unsigned int)p_Term->aBackSpaceChar);
			enSaveInt(l_Sect,"DeleteChar",(unsigned int)p_Term->aDeleteChar);
			enSaveInt(l_Sect,"Scrollback",
				(unsigned long)(p_Term->aTermBuf->tBufY-p_Term->aTermBuf->tScrY-ATIB_TERMBUF_EXTRALINES));
			break;
		case TBNOTIFY_FONT:
			if (p_Term->aLogFontValid) {
				enSaveInt(l_Sect,"LogFontValid",1);
				enSaveLogFont(l_Sect,&(p_Term->aLogFont));
			} else {
				enSaveInt(l_Sect,"LogFontValid",0);
			}
			break;
		case TBNOTIFY_MACROS:
			for (l_i=0;l_i<48;l_i++) {
				sprintf(l_Buf,"Macro%i",l_i+1);
				enSaveString(l_Sect,l_Buf,p_Term->aFnKeyMacro[l_i]);
			}
			break;
		default:
			break;
	}
}

void atLoadTermSettings(ATTERM *p_Term, int p_Settings, char *p_Section) {
	char *l_Sect;
	LOGFONT l_LogFont;
	int l_i;
	char l_Buf[20];
	unsigned long l_u;

	if (!p_Term)
		return;

	if (p_Section)
		l_Sect=p_Section;
	else
		l_Sect=p_Term->aEnvSection;

	switch (p_Settings) {
		case TBNOTIFY_OPTIONS:
			p_Term->aEmulation=enRestoreLong(l_Sect,"Emulation",p_Term->aEmulation);
			p_Term->aSeparateInputLine=(BOOL)enRestoreInt(l_Sect,"SeparateInputLine",0);
			p_Term->aInputFColour=(unsigned int)enRestoreInt(l_Sect,"InputLineFColour",ATIB_FCOLOUR);
			p_Term->aInputBColour=(unsigned int)enRestoreInt(l_Sect,"InputLineBColour",ATIB_BCOLOUR);
			p_Term->aLocalEcho=(BOOL)enRestoreInt(l_Sect,"LocalEcho",1);
      p_Term->aVLocalEcho=p_Term->aLocalEcho;
			p_Term->aBell=(BOOL)enRestoreInt(l_Sect,"Bell",1);
			p_Term->aBackSpaceChar=(unsigned char)enRestoreInt(l_Sect,"BackSpaceChar",ASCII_BS);
			p_Term->aDeleteChar=(unsigned char)enRestoreInt(l_Sect,"DeleteChar",ASCII_DEL);
			l_u=(unsigned int)enRestoreInt(l_Sect,"Scrollback",50);
			atAdjustTerm(p_Term,l_u);
			enSaveInt(l_Sect,"Scrollback",
				(unsigned long)(p_Term->aTermBuf->tBufY-p_Term->aTermBuf->tScrY-ATIB_TERMBUF_EXTRALINES));
			break;
		case TBNOTIFY_FONT:
			if (enRestoreInt(l_Sect,"LogFontValid",0)) {
				enRestoreLogFont(l_Sect,&(l_LogFont));
				atChangeFont(p_Term,NULL, &l_LogFont);
			} else {
				atChangeFont(p_Term,GetStockObject(SYSTEM_FIXED_FONT),NULL);
			}
			break;
		case TBNOTIFY_MACROS:
			for (l_i=0;l_i<48;l_i++) {
				sprintf(l_Buf,"Macro%i",l_i+1);
				enRestoreString(l_Sect,l_Buf,"",p_Term->aFnKeyMacro[l_i],TBMACRO_MAXLEN);
			}
			break;
		default:
			break;
	}
}

void atResetTerm(ATTERM *p_Term) {
	int l_i;
	p_Term->aVLocalEcho=FALSE;
	for (l_i=0;l_i<4;l_i++)
		p_Term->aLED[l_i]=FALSE;
	p_Term->aLFwithNLMode=TRUE;
	p_Term->aCursorKeyAppMode=FALSE;
	p_Term->aANSIMode=TRUE;

	p_Term->aColumnMode=TRUE;
	p_Term->aScrollingMode=TRUE;
	p_Term->aScreenMode=TRUE;
	p_Term->aWraparoundMode=TRUE;
	p_Term->aAutoRepeatMode=TRUE;
	p_Term->aInterlaceMode=TRUE;
	p_Term->aGraphicProcMode=TRUE;
	p_Term->aKeypadMode=TRUE;

}


