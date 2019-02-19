// areawnd.c
// Two-letter Module Descriptor: aw
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

/* This module implements the area list window which displays,
 * as a hierarchy, the structure tree. */

#include<windows.h>
#include<windowsx.h>
#include<time.h>
#include<stdio.h>
#include<string.h>
#include"molem.h"
#include"molerc.h"
#include"dstruct.h"
#include"areawnd.h"
#include"enviromt.h"
#include"debug.h"
#include"edit.h"
#include"main.h"
#include"clipbrd.h"

#ifdef WIN32
#define MoveTo(x,y,z) MoveToEx(x,y,z,NULL)
#endif

/* ICON references */
#define AW_ICON_PLUS   0
#define AW_ICON_MINUS  1

#define AW_MUDNAME_NOTCONN "<not connected>"
#define AW_PROFILE_STR     "AreaWindow"

/* Globals */
DSREF g_awRootRef; /* the root of all evil, er, I mean, of all structures */
DSFTL *g_awFTList; /* may as well keep this here too. */
DSFTL *g_awFTLObjType; /* and this too */
int g_awRootTop; /* the # of the top line in the root window */
HWND g_awRootWnd;
int g_awTextY; /* height of a single row in Area window */
int g_awTextX; /* width of a "column" in Area Window - a column is a | section */
int g_awTextHalfY; /* height of a single row in Area window */
int g_awTextHalfX; /* width of a "column" in Area Window - a column is a | section */
int g_awRowsReqd; /* # rows required for current display options */
RECT g_awRootRect; /* rectangle for the client area of the area window */
int g_awRSWidth; /* width of scrollbar */
int g_awPageScroll; /* # lines for a page-sized scroll */
HFONT g_awFont;

//HICON g_awGlobeIcon[3],g_awMOBsIcon[3],g_awOBJIcon[3];
//HICON g_awMOBIcon,g_awMapIcon[3],g_awResetIcon,g_awArrowIcon[3];
HICON g_awRootIcon[2],g_awAreaIcon[2],g_awWLDListIcon[2];
HICON g_awMOBListIcon[2],g_awOBJListIcon[2];
HICON g_awAreaDetailIcon,g_awResetIcon,g_awWLDIcon,g_awMOBIcon,g_awOBJIcon;
int g_awHighlightLine;
char g_awMUDName[100];
char *g_awDetailString,*g_awMOBString,*g_awOBJString;
char *g_awWLDString,*g_awRSTString;
int g_awAvailable,g_awPending,g_awUnavailable;

BOOL awInitAreaWnd() {
  HDC l_hDC;
  TEXTMETRIC l_tm;

  g_awAvailable=COLOR_WINDOWTEXT;
  g_awUnavailable=COLOR_GRAYTEXT;
  g_awPending=-1;
//  g_awAvailable=-2;
//  g_awPending=-3;
//  g_awUnavailable=-1;

  dsRefClear(&g_awRootRef);
  g_awFTList=NULL;
  g_awFTLObjType=NULL;
  g_awHighlightLine=0;
  g_awRootRef.rState=DS_NS_NOTAVAIL;
  g_awRootRef.rLoadTime=time(NULL);
  g_awRootRef.rFlag=DS_FLAG_WINOPEN;
  g_awDetailString="Area settings";
  g_awMOBString="Mobiles";
  g_awOBJString="Objects";
  g_awWLDString="World";
  g_awRSTString="Reset";
  strcpy(g_awMUDName,AW_MUDNAME_NOTCONN);

  g_awRowsReqd=1;
  g_awRSWidth=GetSystemMetrics(SM_CXVSCROLL);
	g_awRootWnd = CreateWindow(
			 AW_WIND_CLASS_NAME,
			 "Area Hierarchy",
			 WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_VSCROLL,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 g_aahWnd,
			 NULL,
			 g_aahInst,
			 NULL
			 );
  if (!g_awRootWnd)
    return FALSE;


  g_awRootTop=0;
  g_awFont=GetStockObject(SYSTEM_FONT);
  l_hDC=GetDC(g_awRootWnd);
  if (l_hDC) {
    SelectObject(l_hDC,g_awFont);
    GetTextMetrics(l_hDC,&l_tm);
    ReleaseDC(g_awRootWnd,l_hDC);
    g_awTextX=l_tm.tmAveCharWidth;
    g_awTextY=l_tm.tmHeight+l_tm.tmExternalLeading;
  } else {
    g_awTextX=18;
    g_awTextY=16;
  }
  if (g_awTextY<16)
    g_awTextY=16;
  if (g_awTextX<18)
    g_awTextX=18;
  g_awTextHalfX=g_awTextX>>1;
  g_awTextHalfY=g_awTextY>>1;

  g_awRootIcon[AW_ICON_PLUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_GLOBEP));
  g_awRootIcon[AW_ICON_MINUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_GLOBEM));
  g_awAreaIcon[AW_ICON_PLUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_ARROWGREENRIGHT));
  g_awAreaIcon[AW_ICON_MINUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_ARROWGREENDOWN));
  g_awWLDListIcon[AW_ICON_PLUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_ARROWBLUERIGHT));
  g_awWLDListIcon[AW_ICON_MINUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_ARROWBLUEDOWN));
  g_awMOBListIcon[AW_ICON_PLUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_ARROWYELLOWRIGHT));
  g_awMOBListIcon[AW_ICON_MINUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_ARROWYELLOWDOWN));
  g_awOBJListIcon[AW_ICON_PLUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_ARROWREDRIGHT));
  g_awOBJListIcon[AW_ICON_MINUS]=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_ARROWREDDOWN));

  g_awAreaDetailIcon=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_MAP));
  g_awResetIcon=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_RESET));
  g_awWLDIcon=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_GLOBE));
  g_awMOBIcon=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_MOB));
  g_awOBJIcon=LoadIcon(g_aahInst,MAKEINTRESOURCE(ICON_OBJ));

  SetWindowPlacement(g_awRootWnd,enRestoreWindowPlacement(AW_PROFILE_STR,
    GetSystemMetrics(SM_CXSCREEN)*3/4,GetSystemMetrics(SM_CYSCREEN)/8,
    GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)*2/3));
  if (enRestoreYesNo(AW_PROFILE_STR,"ShowWindow",TRUE))
    ShowWindow(g_awRootWnd,SW_SHOWNORMAL);
  else
    ShowWindow(g_awRootWnd,SW_HIDE);
  GetClientRect(g_awRootWnd,&g_awRootRect);
  PostMessage(g_awRootWnd,WM_USER_AREALIST_CHNG,0,0L);
  return TRUE;
  }

void awShutdownAreaWnd() {
  if (g_awRootWnd) {
    enSaveWindowPlacement(AW_PROFILE_STR,g_awRootWnd);
    enSaveYesNo(AW_PROFILE_STR,"ShowWindow",awIsAreaWindowVisible());
    DestroyWindow(g_awRootWnd);
  }
  dsRefFree(&g_awRootRef);
  dsFTLFree(&g_awFTList);
  dsFTLFree(&g_awFTLObjType);

  DestroyIcon(g_awRootIcon[AW_ICON_PLUS]);
  DestroyIcon(g_awRootIcon[AW_ICON_MINUS]);
  DestroyIcon(g_awAreaIcon[AW_ICON_PLUS]);
  DestroyIcon(g_awAreaIcon[AW_ICON_MINUS]);
  DestroyIcon(g_awWLDListIcon[AW_ICON_PLUS]);
  DestroyIcon(g_awWLDListIcon[AW_ICON_MINUS]);
  DestroyIcon(g_awMOBListIcon[AW_ICON_PLUS]);
  DestroyIcon(g_awMOBListIcon[AW_ICON_MINUS]);
  DestroyIcon(g_awOBJListIcon[AW_ICON_PLUS]);
  DestroyIcon(g_awOBJListIcon[AW_ICON_MINUS]);
  DestroyIcon(g_awAreaDetailIcon);
  DestroyIcon(g_awResetIcon);
  DestroyIcon(g_awWLDIcon);
  DestroyIcon(g_awMOBIcon);
  DestroyIcon(g_awOBJIcon);

  return;
}

/* connection has been killed - reset all our variables */
void awResetConnection() {
  /* kill our local struct tree */
  dsRefFree(&g_awRootRef);
  /* reset our MUD name */
  strcpy(g_awMUDName,AW_MUDNAME_NOTCONN);
  /* turf our FTLists */
  dsFTLFree(&g_awFTList);
  dsFTLFree(&g_awFTLObjType);

  /* refresh window */
  PostMessage(g_awRootWnd,WM_USER_AREALIST_CHNG,0,0L);
  return;
}

BOOL awInitAreaWndApplication() {
	WNDCLASS l_wc;

	l_wc.style = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
	l_wc.lpfnWndProc = awAreaWndProc;
	l_wc.cbClsExtra = 0;
	l_wc.cbWndExtra = 0;
	l_wc.hInstance = g_aahInst;
	l_wc.hIcon = LoadIcon(g_aahInst, MAKEINTRESOURCE(ICON_MOLE));
	l_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	l_wc.hbrBackground =(HBRUSH)(COLOR_WINDOW+1);
	l_wc.lpszMenuName = NULL;
	l_wc.lpszClassName = AW_WIND_CLASS_NAME;

	return (RegisterClass(&l_wc));
}

BOOL awInitAreaWndSecondApp() {
  return TRUE;
}

/* this proc recursively goes through the structure and counts how many
 * things are open, and returns the number of lines required for each thing */

/* Macro used only in this proc -- text type macro */
#define AW_HILIGHT_TEXTOUT(hDC,x,y,buf,len) \
          if (l_i==g_awHighlightLine) {  \
            SetBkColor(hDC,awGetColor(COLOR_HIGHLIGHT)); \
            TextOut(hDC,x,y,buf,len); \
            SetBkColor(hDC,awGetColor(COLOR_WINDOW)); \
          } else { \
            TextOut(hDC,x,y,buf,len); \
          }

LRESULT CALLBACK _export awAreaWndProc(HWND p_hWnd, UINT p_message,
				WPARAM p_wParam, LPARAM p_lParam)
	{
  static RECT l_rect;
 	static PAINTSTRUCT l_ps; // paint structure
  static HDC l_hDC; // device context
  static HFONT l_oldFont;
  static int l_i,l_j,l_x,l_y;
  static int l_start,l_end; /* start & end lines */
  static DSSTRUCT *l_area,*l_thing;
  static char l_buf[200];
	static POINT l_ptCurrent; /* for quick-menu */
	static HMENU l_hmenu;     /* for quick-menu */
	static UINT l_flags;      /* for quick-menu */
	static UINT l_flagsInactive; /* for quick-menu */
	static UINT l_flagsSeparator; /* for quick-menu */

  switch (p_message)
    {
    case WM_USER_NEWDATA:
    case WM_USER_AREALIST_CHNG:
      /* structure has received an update - we have to redraw and recalculate everything */

      /* our rectangle should be OK */

      /* let's recalculate our total number of lines */
      g_awRowsReqd=1; /* 1 - always show g_awRootRef */
      if (g_awRootRef.rFlag&DS_FLAG_WINOPEN) {
        for(l_area=g_awRootRef.rList;l_area;l_area=l_area->sNext) {
          g_awRowsReqd++; /* for area name */
          if (DSSArea(l_area)->aFlag&DS_FLAG_WINOPEN) {
            g_awRowsReqd+=5; /* for detail,MOB,OBJ,WLD,RST */
            if (((DSSArea(l_area)->aWLD.rFlag)&DS_FLAG_WINOPEN)&&
                (DSSArea(l_area)->aWLD.rState==DS_NS_AVAIL))
              for (l_thing=DSSArea(l_area)->aWLD.rList;l_thing;l_thing=l_thing->sNext)
                g_awRowsReqd++;
            if (((DSSArea(l_area)->aMOB.rFlag)&DS_FLAG_WINOPEN)&&
                (DSSArea(l_area)->aMOB.rState==DS_NS_AVAIL))
              for (l_thing=DSSArea(l_area)->aMOB.rList;l_thing;l_thing=l_thing->sNext)
                g_awRowsReqd++;
            if (((DSSArea(l_area)->aOBJ.rFlag)&DS_FLAG_WINOPEN)&&
                (DSSArea(l_area)->aOBJ.rState==DS_NS_AVAIL))
              for (l_thing=DSSArea(l_area)->aOBJ.rList;l_thing;l_thing=l_thing->sNext)
                g_awRowsReqd++;
          }
        }
      }

      /* check on our scroll bar */
      PostMessage(p_hWnd,WM_USER_AREALIST_SCROLL,0,0L);

//      RedrawWindow(p_hWnd,NULL,NULL,RDW_ERASE);
      InvalidateRect(p_hWnd,NULL,TRUE);
//      UpdateWindow(p_hWnd);

      return 0L;
    case WM_USER_AREALIST_SCROLL:
      /* check up on, and adjust if necessary, our scroll bar */
      if (g_awRootTop>g_awRowsReqd)
        g_awRootTop=0;

      /* check to see if we can move up */
      l_i=(g_awRootRect.bottom/g_awTextY);
      if (l_i>(g_awRowsReqd-g_awRootTop))
        g_awRootTop=g_awRowsReqd-l_i;
      if (g_awRootTop<0)
        g_awRootTop=0;

      if (g_awRootTop || (g_awRowsReqd>(g_awRootRect.bottom/g_awTextY))) {
        /* set up our scroll bar */
        SetScrollRange(g_awRootWnd,SB_VERT,0,g_awRowsReqd-l_i,FALSE);
        SetScrollPos(g_awRootWnd,SB_VERT,g_awRootTop,TRUE);
      } else {
        SetScrollRange(g_awRootWnd,SB_VERT,0,0,TRUE);
      }

      g_awPageScroll=g_awRootRect.bottom/g_awTextY-2;
      if (g_awPageScroll<2)
        g_awPageScroll=2;

      return 0L;
//    case WM_MOVE:
//    case WM_SIZE:
    case WM_WINDOWPOSCHANGED:
      GetClientRect(g_awRootWnd,&g_awRootRect);
      PostMessage(p_hWnd,WM_USER_AREALIST_SCROLL,0,0L);
      return 0L;
    case WM_VSCROLL:
      l_j=g_awRootTop;
      switch(GET_WM_VSCROLL_CODE(p_wParam,p_lParam)) {
        case SB_BOTTOM:
          g_awRootTop=g_awRowsReqd-1;
          break;
        case SB_LINEDOWN:
          if (g_awRootTop<(g_awRowsReqd-1))
            g_awRootTop++;
          break;
        case SB_LINEUP:
          if (g_awRootTop>0)
            g_awRootTop--;
          break;
        case SB_PAGEDOWN:
          if (g_awRootTop<(g_awRowsReqd-1-g_awPageScroll))
            g_awRootTop+=g_awPageScroll;
          else
            g_awRootTop=g_awRowsReqd-1;
          break;
        case SB_PAGEUP:
          if (g_awRootTop>=g_awPageScroll)
            g_awRootTop-=g_awPageScroll;
          else
            g_awRootTop=0;
          break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
          g_awRootTop=GET_WM_HSCROLL_POS(p_wParam,p_lParam);
          ScrollWindow(p_hWnd,0,(l_j-g_awRootTop)*g_awTextY,NULL,NULL);
          return 0L;
        case SB_TOP:
          g_awRootTop=0;
          break;
        case SB_ENDSCROLL:
          PostMessage(p_hWnd,WM_USER_AREALIST_SCROLL,0,0L);
          break;
      }

      /* check to see if we can move up */
      l_i=(g_awRootRect.bottom/g_awTextY);
      if (l_i>(g_awRowsReqd-g_awRootTop))
        g_awRootTop=g_awRowsReqd-l_i;
      if (g_awRootTop<0)
        g_awRootTop=0;

      if (l_j!=g_awRootTop) {
        ScrollWindow(p_hWnd,0,(l_j-g_awRootTop)*g_awTextY,NULL,NULL);
        PostMessage(p_hWnd,WM_USER_AREALIST_SCROLL,0,0L);
      }
      return 0L;
    case WM_SYSCOMMAND:
      switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          awHideAreaWindow();
          return 0L;
				default:
					break;
				}
      break;
    case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case CM_QUICKCOPY:
        case CM_QUICKPASTE:
        case CM_QUICKEDIT:
          awFindHighlightItem(&l_area,&l_thing,&l_j);
          if (p_wParam==CM_QUICKEDIT) {
            if ((l_j>=EDFI_EDITABLE)&&(l_j<=EDFI_ENDEDITABLE))
              edEditItem(l_j,l_area,l_thing,NULL);
          } else if (p_wParam==CM_QUICKCOPY) {
            cbCopyStruct(l_thing);
          } else if (p_wParam==CM_QUICKPASTE) {
            cbPasteStruct(l_thing);
          }
          return 0L;
        case CM_QUICKEDITAREA:
        case CM_QUICKEDITRESET:
        case CM_QUICKNEW:
          break;
        case CM_QUICKRELOAD:
          awFindHighlightItem(&l_area,&l_thing,&l_j);
          if (l_j==EDFI_NULL) { /* reload area list */
            edFetchItem(EDFI_AREALIST,NULL,NULL,NULL);
          } else if (l_j==EDFI_AREALIST) { /* reload area's lists */
            edFetchItem(EDFI_WLDLIST,l_area,NULL,NULL);
            edFetchItem(EDFI_MOBLIST,l_area,NULL,NULL);
            edFetchItem(EDFI_OBJLIST,l_area,NULL,NULL);
          } else if ((l_j==EDFI_WLDLIST)||(l_j==EDFI_WORLD)) { /* reload world list */
            edFetchItem(EDFI_WLDLIST,l_area,NULL,NULL);
          } else if ((l_j==EDFI_MOBLIST)||(l_j==EDFI_MOBILE)) { /* reload mobile list */
            edFetchItem(EDFI_MOBLIST,l_area,NULL,NULL);
          } else if ((l_j==EDFI_OBJLIST)||(l_j==EDFI_OBJECT)) { /* reload object list */
            edFetchItem(EDFI_OBJLIST,l_area,NULL,NULL);
          } /* else ignore */
          return 0L;
        default:
          break;
        }
      break;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN: /* change focus and possibly open up a structure */
      /* First, find line */
      l_x=LOWORD(p_lParam)/g_awTextX; /* column */
      l_y=g_awRootTop+(HIWORD(p_lParam)/g_awTextY); /* Row */

      if (l_y>=g_awRowsReqd) /* clicked outside our region - abort */
        return 0L;
      /* first, move highlight to this position now */
      if (g_awHighlightLine!=l_y) {
        l_rect.left=g_awRootRect.left;
        l_rect.right=g_awRootRect.right;
        if ((g_awHighlightLine>=g_awRootTop)&&
            (g_awHighlightLine<(g_awRootRect.bottom/g_awTextY+1+g_awRootTop))) {
          l_rect.top=(g_awHighlightLine-g_awRootTop)*g_awTextY;
          l_rect.bottom=l_rect.top+g_awTextY;
          InvalidateRect(p_hWnd,&l_rect,TRUE);
        }
        g_awHighlightLine=l_y;
        l_rect.top=(g_awHighlightLine-g_awRootTop)*g_awTextY;
        l_rect.bottom=l_rect.top+g_awTextY;
        InvalidateRect(p_hWnd,&l_rect,TRUE);
      } /* redraw highlight bar if req'd */

      /* next, find the object we're pointing to */
      awFindHighlightItem(&l_area,&l_thing,&l_j);
      /* l_j: EEDFI_* describes item pointed to. */

      /* OK, now process the sucker */
      /* First, filter out clicks to the icons - to perform open/close */
      /* l_start is the "start" of our X-coord for the icon */
      if (l_j==EDFI_NULL) l_start=0;               /* root */
      else if (l_j==EDFI_AREALIST) l_start=1;  /* area */
      else if (l_j<EDFI_WORLD) l_start=2; /* area detail,rst,etc.*/
      else l_start=3;
      if (l_x<l_start)
        return 0L; /* to the left of the icon - ignore */
      else if (l_x<(l_start+1)) {
        /* we've got an icon hit! Let's take another look at what we hit. */
        if (p_message==WM_LBUTTONDOWN) { /* only left button allowed to trigger icon */
          if (l_j==EDFI_NULL) {
            g_awRootRef.rFlag^=DS_FLAG_WINOPEN;
            if ((g_awRootRef.rFlag|DS_FLAG_WINOPEN)&&(g_awRootRef.rState==DS_NS_NOTAVAIL))
              edFetchItem(EDFI_AREALIST,NULL,NULL,NULL);
          } else if (l_j==EDFI_AREALIST) {
            DSSArea(l_area)->aFlag^=DS_FLAG_WINOPEN;
          } else if (l_j==EDFI_WLDLIST) {
            DSSArea(l_area)->aWLD.rFlag^=DS_FLAG_WINOPEN;
            if ((DSSArea(l_area)->aWLD.rFlag|DS_FLAG_WINOPEN)&&
              (DSSArea(l_area)->aWLD.rState==DS_NS_NOTAVAIL))
              edFetchItem(EDFI_WLDLIST,l_area,NULL,NULL);
          } else if (l_j==EDFI_MOBLIST) {
            DSSArea(l_area)->aMOB.rFlag^=DS_FLAG_WINOPEN;
            if ((DSSArea(l_area)->aMOB.rFlag|DS_FLAG_WINOPEN)&&
              (DSSArea(l_area)->aMOB.rState==DS_NS_NOTAVAIL))
              edFetchItem(EDFI_MOBLIST,l_area,NULL,NULL);
          } else if (l_j==EDFI_OBJLIST) {
            DSSArea(l_area)->aOBJ.rFlag^=DS_FLAG_WINOPEN;
            if ((DSSArea(l_area)->aOBJ.rFlag|DS_FLAG_WINOPEN)&&
              (DSSArea(l_area)->aOBJ.rState==DS_NS_NOTAVAIL))
              edFetchItem(EDFI_OBJLIST,l_area,NULL,NULL);
          } else return 0L; /* other icon's don't matter */
          PostMessage(p_hWnd,WM_USER_AREALIST_CHNG,0,0L);
        }
        return 0L;
      }
      /* l_j: EEDFI_* describes item pointed to. */
      if (p_message==WM_LBUTTONDBLCLK) { /* action time */
        if (l_j==EDFI_NULL) {
          g_awRootRef.rFlag^=DS_FLAG_WINOPEN;
          if ((g_awRootRef.rFlag|DS_FLAG_WINOPEN)&&(g_awRootRef.rState==DS_NS_NOTAVAIL))
            edFetchItem(EDFI_AREALIST,NULL,NULL,NULL);
        } else if (l_j==EDFI_AREALIST) {
          DSSArea(l_area)->aFlag^=DS_FLAG_WINOPEN;
        } else if (l_j==EDFI_WLDLIST) {
          DSSArea(l_area)->aWLD.rFlag^=DS_FLAG_WINOPEN;
          if ((DSSArea(l_area)->aWLD.rFlag|DS_FLAG_WINOPEN)&&
            (DSSArea(l_area)->aWLD.rState==DS_NS_NOTAVAIL))
            edFetchItem(EDFI_WLDLIST,l_area,NULL,NULL);
        } else if (l_j==EDFI_MOBLIST) {
          DSSArea(l_area)->aMOB.rFlag^=DS_FLAG_WINOPEN;
          if ((DSSArea(l_area)->aMOB.rFlag|DS_FLAG_WINOPEN)&&
            (DSSArea(l_area)->aMOB.rState==DS_NS_NOTAVAIL))
            edFetchItem(EDFI_MOBLIST,l_area,NULL,NULL);
        } else if (l_j==EDFI_OBJLIST) {
          DSSArea(l_area)->aOBJ.rFlag^=DS_FLAG_WINOPEN;
          if ((DSSArea(l_area)->aOBJ.rFlag|DS_FLAG_WINOPEN)&&
            (DSSArea(l_area)->aOBJ.rState==DS_NS_NOTAVAIL))
            edFetchItem(EDFI_OBJLIST,l_area,NULL,NULL);
        } else if ((l_j>=EDFI_EDITABLE)&&(l_j<=EDFI_ENDEDITABLE)) {
          /* catch-all for EDFI_AREADETAIL, RESET, WORLD, MOBILE, OBJECT */
          edEditItem(l_j,l_area,l_thing,NULL);
          return 0L;
        } else { /* who knows. */
          return 0L;
        }
        PostMessage(p_hWnd,WM_USER_AREALIST_CHNG,0,0L);
        return 0L;
      } else if (p_message==WM_RBUTTONDOWN) { /* quick-menu time! */
        /* let's create that handy little pop-up menu */
#ifdef WIN32
        l_ptCurrent.x = LOWORD(p_lParam);
        l_ptCurrent.y = HIWORD(p_lParam);
#else
        l_ptCurrent = MAKEPOINT(p_lParam);
#endif
        l_hmenu = CreatePopupMenu();
        l_flags=MF_ENABLED;
        l_flagsInactive=MF_GRAYED;
        l_flagsSeparator=MF_SEPARATOR;
      /* l_j: EEDFI_* describes item pointed to. */
        if (l_j==EDFI_NULL) {
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKNEW,"&New Area");
          l_flags=MF_SEPARATOR;
          AppendMenu(l_hmenu, l_flags, 0,NULL);
          l_flags=MF_ENABLED;
          AppendMenu(l_hmenu, l_flags, CM_QUICKRELOAD,"&Reload Area list");
        } else if (l_j==EDFI_AREALIST) {
          /* I took the following two out because they kind of broke the
           * standard menu structure. Well, the real reason is because if I
           * allow editing from here, I should allow copy/paste, and this
           * menu would start to get carried away! */
//          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKEDITAREA,"Edit &Area settings");
//          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKEDITRESET,"Edit &Reset");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKNEW,"&New Area");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flags, CM_QUICKRELOAD,"&Reload Area's lists");
        } else if (l_j==EDFI_AREADETAIL) {
          AppendMenu(l_hmenu, l_flags, CM_QUICKEDIT,"Edit &Area settings");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKCOPY,"&Copy Area settings");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKPASTE,"&Paste Area settings");
        } else if (l_j==EDFI_RESET) {
          AppendMenu(l_hmenu, l_flags, CM_QUICKEDIT,"Edit &Reset");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKCOPY,"&Copy Reset");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKPASTE,"&Paste Reset");
        } else if (l_j==EDFI_WLDLIST) {
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKNEW,"New &World");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flags, CM_QUICKRELOAD,"&Reload World list");
        } else if (l_j==EDFI_MOBLIST) {
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKNEW,"New &Mobile");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flags, CM_QUICKRELOAD,"&Reload Mobile list");
        } else if (l_j==EDFI_OBJLIST) {
          AppendMenu(l_hmenu, l_flags, CM_QUICKNEW,"New &Object");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flags, CM_QUICKRELOAD,"&Reload Object list");
        } else if (l_j==EDFI_WORLD) {
          AppendMenu(l_hmenu, l_flags, CM_QUICKEDIT,"&Edit World");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKNEW,"&New World");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKCOPY,"&Copy World");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKPASTE,"&Paste World");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flags, CM_QUICKRELOAD,"&Reload World list");
        } else if (l_j==EDFI_MOBILE) {
          AppendMenu(l_hmenu, l_flags, CM_QUICKEDIT,"&Edit Mobile");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKNEW,"&New Mobile");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKCOPY,"&Copy Mobile");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKPASTE,"&Paste Mobile");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flags, CM_QUICKRELOAD,"&Reload Mobile list");
        } else if (l_j==EDFI_OBJECT) {
          AppendMenu(l_hmenu, l_flags, CM_QUICKEDIT,"&Edit Object");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKNEW,"&New Object");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKCOPY,"&Copy Object");
          AppendMenu(l_hmenu, l_flagsInactive, CM_QUICKPASTE,"&Paste Object");
          AppendMenu(l_hmenu, l_flagsSeparator, 0,NULL);
          AppendMenu(l_hmenu, l_flags, CM_QUICKRELOAD,"&Reload Object list");
        } else {
          return 0L;
        }
        ClientToScreen(p_hWnd, &l_ptCurrent);
        TrackPopupMenu(l_hmenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, l_ptCurrent.x,
          l_ptCurrent.y, 0, p_hWnd, NULL);
        DestroyMenu(l_hmenu);
       return 0L;
      } /* else we don't care - just move the highlight */
      return 0L;
    case WM_KEYDOWN:
      l_rect.left=0;
      l_rect.right=10000;
      if (p_wParam==VK_UP) {
        if (g_awHighlightLine>0) {
          if ((g_awHighlightLine>=g_awRootTop)&&
              (g_awHighlightLine<(g_awRootRect.bottom/g_awTextY+1+g_awRootTop))) {
            l_rect.top=(g_awHighlightLine-g_awRootTop)*g_awTextY;
            l_rect.bottom=l_rect.top+g_awTextY;
            InvalidateRect(p_hWnd,&l_rect,TRUE);
          }
          g_awHighlightLine--;
          if ((g_awHighlightLine>=g_awRootTop)&&
              (g_awHighlightLine<(g_awRootRect.bottom/g_awTextY+1+g_awRootTop))) {
            l_rect.top=(g_awHighlightLine-g_awRootTop)*g_awTextY;
            l_rect.bottom=l_rect.top+g_awTextY;
            InvalidateRect(p_hWnd,&l_rect,TRUE);
          }
        }
      } else if (p_wParam==VK_DOWN) {
        if (g_awHighlightLine<(g_awRowsReqd-1)) {
          if ((g_awHighlightLine>=g_awRootTop)&&
              (g_awHighlightLine<(g_awRootRect.bottom/g_awTextY+1+g_awRootTop))) {
            l_rect.top=(g_awHighlightLine-g_awRootTop)*g_awTextY;
            l_rect.bottom=l_rect.top+g_awTextY;
            InvalidateRect(p_hWnd,&l_rect,TRUE);
          }
          g_awHighlightLine++;
          if ((g_awHighlightLine>=g_awRootTop)&&
              (g_awHighlightLine<(g_awRootRect.bottom/g_awTextY+1+g_awRootTop))) {
            l_rect.top=(g_awHighlightLine-g_awRootTop)*g_awTextY;
            l_rect.bottom=l_rect.top+g_awTextY;
            InvalidateRect(p_hWnd,&l_rect,TRUE);
          }
        }
      } else if (p_wParam==VK_RETURN) {
        SendMessage(p_hWnd,WM_LBUTTONDBLCLK,(WPARAM)0,
          (LPARAM)MAKELONG(1000,(g_awHighlightLine-g_awRootTop)*g_awTextY+1));
      } else if (p_wParam==VK_HOME) {
        InvalidateRect(p_hWnd,NULL,TRUE);
        g_awHighlightLine=0;
      } else if (p_wParam==VK_END) {
        InvalidateRect(p_hWnd,NULL,TRUE);
        g_awHighlightLine=g_awRowsReqd-1;
      } else if (p_wParam==VK_PRIOR) {
        if ((g_awRootTop<g_awHighlightLine)&&
          (g_awHighlightLine<g_awRootRect.bottom/g_awTextY)){
          if (g_awRootTop>=g_awPageScroll)
            g_awRootTop-=g_awPageScroll;
          else
            g_awRootTop=0;
        }
        InvalidateRect(p_hWnd,NULL,TRUE);
        if (g_awHighlightLine>=g_awPageScroll)
          g_awHighlightLine-=g_awPageScroll;
        else
          g_awHighlightLine=0;
      } else if (p_wParam==VK_NEXT) {
        if ((g_awRootTop<g_awHighlightLine)&&
          (g_awHighlightLine<g_awRootRect.bottom/g_awTextY)){
          if (g_awRootTop<(g_awRowsReqd-1-g_awPageScroll))
            g_awRootTop+=g_awPageScroll;
          else
            g_awRootTop=g_awRowsReqd-1;
        }
        InvalidateRect(p_hWnd,NULL,TRUE);
        if (g_awHighlightLine<(g_awRowsReqd-1-g_awPageScroll))
          g_awHighlightLine+=g_awPageScroll;
        else
          g_awHighlightLine=g_awRowsReqd-1;
      } else {
        return 0L;
      }
      if (g_awHighlightLine<g_awRootTop) {
        g_awRootTop=g_awHighlightLine;
        InvalidateRect(p_hWnd,NULL,TRUE);
      } else if (g_awHighlightLine>(g_awRootTop+g_awRootRect.bottom/g_awTextY-1)) {
        g_awRootTop=g_awHighlightLine-g_awRootRect.bottom/g_awTextY+1;
        InvalidateRect(p_hWnd,NULL,TRUE);
      }
      PostMessage(p_hWnd,WM_USER_AREALIST_SCROLL,0,0L);
      return 0L;
 		case WM_PAINT:
      if (IsIconic(p_hWnd))
        break;
      /* Paint window */
			if (GetUpdateRect(p_hWnd,&l_rect,TRUE)) {
        /* first, determine our line numbers to update */
        l_y=(l_rect.top/g_awTextY);
        l_start=g_awRootTop+l_y;
        l_end=g_awRootTop+(l_rect.bottom/g_awTextY);
        if (l_end >= g_awRowsReqd)
          l_end=g_awRowsReqd-1;
        l_y*=g_awTextY; /* we do this to round up to the nearest row boundary */
//        l_x=l_start*g_awTextX;
        /* we redraw the whole line each time */

        /* get a device context */
        l_hDC=BeginPaint(p_hWnd,&l_ps);

        l_oldFont=SelectObject(l_hDC,g_awFont);
        SetBkColor(l_hDC,awGetColor(COLOR_WINDOW));

        /** BIG UGLY **/
        l_i=0;
        if (g_awRootRef.rFlag&DS_FLAG_WINOPEN) {
          if (l_i>=l_start) { /* root is open - display tree */
            DrawIcon(l_hDC,1,l_y,g_awRootIcon[AW_ICON_MINUS]);
            if (g_awRootRef.rState==DS_NS_PENDING) {
              SetTextColor(l_hDC,awGetColor(g_awUnavailable));
              AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX,l_y,
                g_awMUDName,strlen(g_awMUDName))
              SetTextColor(l_hDC,awGetColor(g_awAvailable));
            } else if (g_awRootRef.rState!=DS_NS_AVAIL) {
              SetTextColor(l_hDC,awGetColor(g_awUnavailable));
              AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX,l_y,
                g_awMUDName,strlen(g_awMUDName))
              SetTextColor(l_hDC,awGetColor(g_awAvailable));
            } else {
              AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX,l_y,
                g_awMUDName,strlen(g_awMUDName))
            }
            l_y+=g_awTextY;
          }
          l_i++;
          for(l_area=g_awRootRef.rList;l_area;l_area=l_area->sNext) {
            if (l_i>l_end)
              break;
            /* first, print this area's node */
            if (l_i>=l_start) {
              if (l_area->sNext) {
                MoveTo(l_hDC,g_awTextHalfX,l_y);
                LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                MoveTo(l_hDC,g_awTextHalfX,l_y+g_awTextHalfY);
                LineTo(l_hDC,g_awTextX,l_y+g_awTextHalfY);
              } else { /* last area - square off line here */
                MoveTo(l_hDC,g_awTextHalfX,l_y);
                LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextHalfY);
                LineTo(l_hDC,g_awTextX,l_y+g_awTextHalfY);
              }
            }
            if (DSSArea(l_area)->aFlag&DS_FLAG_WINOPEN) {
              if (l_i>=l_start) {
                DrawIcon(l_hDC,g_awTextX+1,l_y,g_awAreaIcon[AW_ICON_MINUS]);
                sprintf(l_buf,"%s (%li-%li)",DSSList(l_area)->lName.sData,
                  DSSList(l_area)->lVNum,DSSList(l_area)->lVNum2);
                AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*2,l_y,l_buf,strlen(l_buf))
                l_y+=g_awTextY;
              }
              l_i++;
              if (l_i>l_end)
                break;
              /* Print aDetail reference - should only ever contain 1 struct */
              if (l_i>=l_start) {
                /* standard line display */
                if (l_area->sNext) {
                  /* area line continuation */
                  MoveTo(l_hDC,g_awTextHalfX,l_y);
                  LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                }
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y);
                LineTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextY);
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextHalfY);
                LineTo(l_hDC,g_awTextX*2,l_y+g_awTextHalfY);

                DrawIcon(l_hDC,g_awTextX*2+1,l_y,g_awAreaDetailIcon);
                if (DSSArea(l_area)->aDetail.rState==DS_NS_PENDING) {
                  SetTextColor(l_hDC,awGetColor(g_awPending));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awDetailString,strlen(g_awDetailString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else if (DSSArea(l_area)->aDetail.rState!=DS_NS_AVAIL) {
                  SetTextColor(l_hDC,awGetColor(g_awUnavailable));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awDetailString,strlen(g_awDetailString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else {
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awDetailString,strlen(g_awDetailString))
                }
                l_y+=g_awTextY;
              }
              l_i++;
              if (l_i>l_end)
                break;

              /* Print reset - only get's a single node */
              if (l_i>=l_start) {
                /* standard line display */
                if (l_area->sNext) {
                  /* area line continuation */
                  MoveTo(l_hDC,g_awTextHalfX,l_y);
                  LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                }
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y);
                LineTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextY);
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextHalfY);
                LineTo(l_hDC,g_awTextX*2,l_y+g_awTextHalfY);

                DrawIcon(l_hDC,g_awTextX*2+1,l_y,g_awResetIcon);
                if (DSSArea(l_area)->aRST.rState==DS_NS_PENDING) {
                  SetTextColor(l_hDC,awGetColor(g_awPending));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awRSTString,strlen(g_awRSTString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else if (DSSArea(l_area)->aRST.rState!=DS_NS_AVAIL) {
                  SetTextColor(l_hDC,awGetColor(g_awUnavailable));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awRSTString,strlen(g_awRSTString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else {
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awRSTString,strlen(g_awRSTString))
                }
                l_y+=g_awTextY;
              }
              l_i++;
              if (l_i>l_end)
                break;

              /* now do other reference lists */

              /* WLD Header */
              if (l_i>=l_start) {
                /* standard line display */
                if (l_area->sNext) {
                  /* area line continuation */
                  MoveTo(l_hDC,g_awTextHalfX,l_y);
                  LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                }
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y);
                LineTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextY);
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextHalfY);
                LineTo(l_hDC,g_awTextX*2,l_y+g_awTextHalfY);

                if (DSSArea(l_area)->aWLD.rFlag&DS_FLAG_WINOPEN)
                  DrawIcon(l_hDC,g_awTextX*2+1,l_y,g_awWLDListIcon[AW_ICON_MINUS]);
                else
                  DrawIcon(l_hDC,g_awTextX*2+1,l_y,g_awWLDListIcon[AW_ICON_PLUS]);
                if (DSSArea(l_area)->aWLD.rState==DS_NS_PENDING) {
                  SetTextColor(l_hDC,awGetColor(g_awPending));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awWLDString,strlen(g_awWLDString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else if (DSSArea(l_area)->aWLD.rState!=DS_NS_AVAIL) {
                  SetTextColor(l_hDC,awGetColor(g_awUnavailable));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awWLDString,strlen(g_awWLDString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else {
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awWLDString,strlen(g_awWLDString))
                }
                l_y+=g_awTextY;
              }
              l_i++;
              if (l_i>l_end)
                break;
              /* WLD list */
              if (DSSArea(l_area)->aWLD.rFlag&DS_FLAG_WINOPEN) {
                for (l_thing=DSSArea(l_area)->aWLD.rList;l_thing;l_thing=l_thing->sNext) {
                  if (l_i>=l_start) {
                    if (l_area->sNext) {
                      /* area line continuation */
                      MoveTo(l_hDC,g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                    }
                    /* area-contents (WLD,OBJ,MOB, etc) continuation */
                    MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y);
                    LineTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextY);

                    /* WLD line */
                    if (l_thing->sNext) { /* there's more */
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextY);
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextHalfY);
                      LineTo(l_hDC,g_awTextX*3,l_y+g_awTextHalfY);
                    } else { /* last in list */
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextHalfY);
                      LineTo(l_hDC,g_awTextX*3,l_y+g_awTextHalfY);
                    }

                    DrawIcon(l_hDC,g_awTextX*3+1,l_y,g_awWLDIcon);
                    sprintf(l_buf,"%s <%li>",DSSList(l_thing)->lName.sData,
                      DSSList(l_thing)->lVNum);
                    if (l_thing->sState==DS_NS_PENDING) {
                      SetTextColor(l_hDC,awGetColor(g_awPending));
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf))
                      SetTextColor(l_hDC,awGetColor(g_awAvailable));
                    } else if (l_thing->sState!=DS_NS_AVAIL) {
                      SetTextColor(l_hDC,awGetColor(g_awUnavailable));
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf))
                      SetTextColor(l_hDC,awGetColor(g_awAvailable));
                    } else {
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf))
                    }
                    l_y+=g_awTextY;
                  }
                  l_i++;
                  if (l_i>l_end)
                    break;
                }
              }

              /* MOB Header */
              if (l_i>=l_start) {
                /* standard line display */
                if (l_area->sNext) {
                  /* area line continuation */
                  MoveTo(l_hDC,g_awTextHalfX,l_y);
                  LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                }
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y);
                LineTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextY);
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextHalfY);
                LineTo(l_hDC,g_awTextX*2,l_y+g_awTextHalfY);

                if (DSSArea(l_area)->aMOB.rFlag&DS_FLAG_WINOPEN)
                  DrawIcon(l_hDC,g_awTextX*2+1,l_y,g_awMOBListIcon[AW_ICON_MINUS]);
                else
                  DrawIcon(l_hDC,g_awTextX*2+1,l_y,g_awMOBListIcon[AW_ICON_PLUS]);
                if (DSSArea(l_area)->aMOB.rState==DS_NS_PENDING) {
                  SetTextColor(l_hDC,awGetColor(g_awPending));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awMOBString,strlen(g_awMOBString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else if (DSSArea(l_area)->aMOB.rState!=DS_NS_AVAIL) {
                  SetTextColor(l_hDC,awGetColor(g_awUnavailable));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awMOBString,strlen(g_awMOBString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else {
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awMOBString,strlen(g_awMOBString))
                }
                l_y+=g_awTextY;
              }
              l_i++;
              if (l_i>l_end)
                break;
              /* MOB list */
              if (DSSArea(l_area)->aMOB.rFlag&DS_FLAG_WINOPEN) {
                for (l_thing=DSSArea(l_area)->aMOB.rList;l_thing;l_thing=l_thing->sNext) {
                  if (l_i>=l_start) {
                    if (l_area->sNext) {
                      /* area line continuation */
                      MoveTo(l_hDC,g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                    }
                    /* area-contents (WLD,OBJ,MOB, etc) continuation */
                    MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y);
                    LineTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextY);

                    /* MOB line */
                    if (l_thing->sNext) { /* there's more */
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextY);
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextHalfY);
                      LineTo(l_hDC,g_awTextX*3,l_y+g_awTextHalfY);
                    } else { /* last in list */
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextHalfY);
                      LineTo(l_hDC,g_awTextX*3,l_y+g_awTextHalfY);
                    }

                    DrawIcon(l_hDC,g_awTextX*3+1,l_y,g_awMOBIcon);
                    sprintf(l_buf,"%s <%li>",DSSList(l_thing)->lName.sData,
                      DSSList(l_thing)->lVNum);
                    if (l_thing->sState==DS_NS_PENDING) {
                      SetTextColor(l_hDC,awGetColor(g_awPending));
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf));
                      SetTextColor(l_hDC,awGetColor(g_awAvailable));
                    } else if (l_thing->sState!=DS_NS_AVAIL) {
                      SetTextColor(l_hDC,awGetColor(g_awUnavailable));
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf));
                      SetTextColor(l_hDC,awGetColor(g_awAvailable));
                    } else {
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf));
                    }
                    l_y+=g_awTextY;
                  }
                  l_i++;
                  if (l_i>l_end)
                    break;
                }
              }

              /* OBJ Header */
              if (l_i>=l_start) {
                /* standard line display */
                if (l_area->sNext) {
                  /* area line continuation */
                  MoveTo(l_hDC,g_awTextHalfX,l_y);
                  LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                }
                MoveTo(l_hDC,g_awTextX+g_awTextHalfX,l_y);
                LineTo(l_hDC,g_awTextX+g_awTextHalfX,l_y+g_awTextHalfY);
                LineTo(l_hDC,g_awTextX*2,l_y+g_awTextHalfY);

                if (DSSArea(l_area)->aOBJ.rFlag&DS_FLAG_WINOPEN)
                  DrawIcon(l_hDC,g_awTextX*2+1,l_y,g_awOBJListIcon[AW_ICON_MINUS]);
                else
                  DrawIcon(l_hDC,g_awTextX*2+1,l_y,g_awOBJListIcon[AW_ICON_PLUS]);
                if (DSSArea(l_area)->aOBJ.rState==DS_NS_PENDING) {
                  SetTextColor(l_hDC,awGetColor(g_awPending));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awOBJString,strlen(g_awOBJString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else if (DSSArea(l_area)->aOBJ.rState!=DS_NS_AVAIL) {
                  SetTextColor(l_hDC,awGetColor(g_awUnavailable));
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awOBJString,strlen(g_awOBJString))
                  SetTextColor(l_hDC,awGetColor(g_awAvailable));
                } else {
                  AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*3,l_y,
                    g_awOBJString,strlen(g_awOBJString))
                }
                l_y+=g_awTextY;
              }
              l_i++;
              if (l_i>l_end)
                break;
              /* OBJ list */
              if (DSSArea(l_area)->aOBJ.rFlag&DS_FLAG_WINOPEN) {
                for (l_thing=DSSArea(l_area)->aOBJ.rList;l_thing;l_thing=l_thing->sNext) {
                  if (l_i>=l_start) {
                    if (l_area->sNext) {
                      /* area line continuation */
                      MoveTo(l_hDC,g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextHalfX,l_y+g_awTextY);
                    }

                    /* OBJ line */
                    if (l_thing->sNext) { /* there's more */
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextY);
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextHalfY);
                      LineTo(l_hDC,g_awTextX*3,l_y+g_awTextHalfY);
                    } else { /* last in list */
                      MoveTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y);
                      LineTo(l_hDC,g_awTextX*2+g_awTextHalfX,l_y+g_awTextHalfY);
                      LineTo(l_hDC,g_awTextX*3,l_y+g_awTextHalfY);
                    }

                    DrawIcon(l_hDC,g_awTextX*3+1,l_y,g_awOBJIcon);
                    sprintf(l_buf,"%s <%li>",DSSList(l_thing)->lName.sData,
                      DSSList(l_thing)->lVNum);
                    if (l_thing->sState==DS_NS_PENDING) {
                      SetTextColor(l_hDC,awGetColor(g_awPending));
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf));
                      SetTextColor(l_hDC,awGetColor(g_awAvailable));
                    } else if (l_thing->sState!=DS_NS_AVAIL) {
                      SetTextColor(l_hDC,awGetColor(g_awUnavailable));
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf));
                      SetTextColor(l_hDC,awGetColor(g_awAvailable));
                    } else {
                      AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*4,l_y,l_buf,strlen(l_buf));
                    }
                    l_y+=g_awTextY;
                  }
                  l_i++;
                  if (l_i>l_end)
                    break;
                }
              }
            } else { /* if... area open */
              if (l_i>=l_start) {
                DrawIcon(l_hDC,g_awTextX+1,l_y,g_awAreaIcon[AW_ICON_PLUS]);
                sprintf(l_buf,"%s (%li-%li)",DSSList(l_area)->lName.sData,
                  DSSList(l_area)->lVNum,DSSList(l_area)->lVNum2);
                AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX*2,l_y,l_buf,strlen(l_buf))
                l_y+=g_awTextY;
              }
              l_i++;
            }
          } /* for... area list */
        } else { /* root is not open */
          if (l_i>=l_start) {
            DrawIcon(l_hDC,1,l_y,g_awRootIcon[AW_ICON_PLUS]);
            if (g_awRootRef.rState==DS_NS_PENDING) {
              SetTextColor(l_hDC,awGetColor(g_awPending));
              AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX,l_y,
                g_awMUDName,strlen(g_awMUDName))
              SetTextColor(l_hDC,awGetColor(g_awAvailable));
            } else if (g_awRootRef.rState!=DS_NS_AVAIL) {
              SetTextColor(l_hDC,awGetColor(g_awUnavailable));
              AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX,l_y,
                g_awMUDName,strlen(g_awMUDName))
              SetTextColor(l_hDC,awGetColor(g_awAvailable));
            } else {
              AW_HILIGHT_TEXTOUT(l_hDC,g_awTextX,l_y,
                g_awMUDName,strlen(g_awMUDName))
            }
            l_y+=g_awTextY;
          }
          l_i++;
        }
        /** End of BIG UGLY **/
        SelectObject(l_hDC,l_oldFont);
  			EndPaint(p_hWnd,&l_ps);
				return 0L;
      }
			break;
    default:
      break;
    }
  return (DefWindowProc(p_hWnd, p_message, p_wParam, p_lParam));
  }


/* This proc finds and identifies the item pointed to by the highlighted line.
 * NOTE that this proc uses and returns EDFI_* constants in p_type!!! */
int awFindHighlightItem(DSSTRUCT **p_area, DSSTRUCT **p_thing, int *p_type) {
  int l_i,l_j;
  DSSTRUCT *l_area, *l_thing;
  BOOL l_FoundIt;

  /* find the object we're pointing to */
  l_i=0;
  l_j=EDFI_NULL; /* Use EDFI_* constants */
  l_thing=l_area=NULL;
  l_FoundIt=FALSE;
  if (l_i<g_awHighlightLine) {
    if (g_awRootRef.rFlag&DS_FLAG_WINOPEN) {
      for(l_area=g_awRootRef.rList;l_area;l_area=l_area->sNext) {
        l_i++; /* for area name */
        if (l_i==g_awHighlightLine) {
          l_j=EDFI_AREALIST;
          break;
        }
        if (DSSArea(l_area)->aFlag&DS_FLAG_WINOPEN) {
          l_i++; /* for area details */
          if (l_i==g_awHighlightLine) {
            l_j=EDFI_AREADETAIL;
            l_thing=DSSArea(l_area)->aDetail.rList;
            break;
          }
          l_i++; /* for reset */
          if (l_i==g_awHighlightLine) {
            l_j=EDFI_RESET;
            l_thing=DSSArea(l_area)->aRST.rList;
            break;
          }
          l_i++; /* for WLD */
          if (l_i==g_awHighlightLine) {
            l_j=EDFI_WLDLIST;
            break;
          }
          if (((DSSArea(l_area)->aWLD.rFlag)&DS_FLAG_WINOPEN)&&
              (DSSArea(l_area)->aWLD.rState==DS_NS_AVAIL))
            for (l_thing=DSSArea(l_area)->aWLD.rList;l_thing;l_thing=l_thing->sNext) {
              l_i++; /* for l_thing */
              if (l_i==g_awHighlightLine) {
                l_FoundIt=TRUE;
                l_j=EDFI_WORLD;
                break;
              }
            }
          if (l_i==g_awHighlightLine) /* check for thing match */
            break;
          l_i++; /* for MOB */
          if (l_i==g_awHighlightLine) {
            l_j=EDFI_MOBLIST;
            break;
          }
          if (((DSSArea(l_area)->aMOB.rFlag)&DS_FLAG_WINOPEN)&&
            (DSSArea(l_area)->aMOB.rState==DS_NS_AVAIL))
            for (l_thing=DSSArea(l_area)->aMOB.rList;l_thing;l_thing=l_thing->sNext) {
              l_i++; /* for l_thing */
              if (l_i==g_awHighlightLine) {
                l_FoundIt=TRUE;
                l_j=EDFI_MOBILE;
                break;
              }
            }
          if (l_i==g_awHighlightLine) /* check for thing match */
            break;
          l_i++; /* for OBJ */
          if (l_i==g_awHighlightLine) {
            l_j=EDFI_OBJLIST;
            break;
          }
          if (((DSSArea(l_area)->aOBJ.rFlag)&DS_FLAG_WINOPEN)&&
              (DSSArea(l_area)->aOBJ.rState==DS_NS_AVAIL))
            for (l_thing=DSSArea(l_area)->aOBJ.rList;l_thing;l_thing=l_thing->sNext) {
              l_i++; /* for l_thing */
              if (l_i==g_awHighlightLine) {
                l_FoundIt=TRUE;
                l_j=EDFI_OBJECT;
                break;
              }
            }
        } /* area's open */
      if (l_FoundIt)
        break;
      } /* loop through areas */
    } /* root's open */
  } /* if root is highlighted */

  /* return our findings */
  if (p_area)
    *p_area=l_area;
  if (p_thing)
    *p_thing=l_thing;
  if (p_type)
    *p_type=l_j;
  return l_j;
} /* awFindHighlightItem() */

void awShowAreaWindow() {
  if (g_awRootWnd)
    ShowWindow(g_awRootWnd,SW_SHOWNA);
  mnMenuMainUpdate();
  return;
}

void awHideAreaWindow() {
  if (g_awRootWnd)
    ShowWindow(g_awRootWnd,SW_HIDE);
  mnMenuMainUpdate();
  return;
}

BOOL awIsAreaWindowVisible() {
  if (g_awRootWnd)
    return IsWindowVisible(g_awRootWnd);
  return FALSE;
}

COLORREF awGetColor(int p_color) {
  if (p_color<0) {
    return RGB(0,0,255);
  }
  return GetSysColor(p_color);
}
