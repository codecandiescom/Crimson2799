// debug.c
// Two-letter Module Descriptor: db
// This module is completely self contained to support reuseability
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

#include<windows.h>
#include<windowsx.h>
#include<stdio.h>
#include<string.h>
#include"debug.h"

#define DB_DEBUG_LINES 40
#define DB_DEBUG_COLUMNS 200
#define DB_WINDOW_CLASS "DebugWindow"

/* Debug menu commands quick-menu commands */
#define DB_USER_KILLWINDOW  WM_USER + 0x0001
#define DB_USER_KILLPROCESS WM_USER + 0x0002

/* Globals used exclusively in this application */
BOOL g_dbBlock=FALSE;       /* TRUE if can't get our window class */
BOOL g_dbDebugOK=FALSE;     /* TRUE if debug is working properly */
char g_dbInfo[DB_DEBUG_LINES][DB_DEBUG_COLUMNS]; /* debug information */
int g_dbTopMsg;
HWND g_dbhWnd;              /* Debug Window */
HINSTANCE g_dbhInst;        /* owner's instance */
void (FAR *g_dbWrapupProc)(void); /* wrapup procedure to kill program */
FILE *g_dbLog;

#ifdef DB_DEBUG_NODEBUG

/*
#pragma argsused
void dbInitDebugApplication(HINSTANCE p_hInst,void (FAR *p_wrapup)(void)) return;
#pragma argsused
void dbInitDebugSecondApp(HINSTANCE p_hInst,void (FAR *p_wrapup)()) return;
#pragma argsused
BOOL dbInitDebug(HINSTANCE p_hInst) return;
#pragma argsused
void dbShutdownDebug() return;
#pragma argsused
void WINAPI dbPrint(char *p_msg) return;
*/

#else  /* DB_DEBUG_NODEBUG */
void dbInitDebugApplication(HINSTANCE p_hInst,void (FAR *p_wrapup)(void))
  {
  WNDCLASS l_wc2;

  g_dbWrapupProc=p_wrapup;

  g_dbhInst=p_hInst;

  if ((g_dbLog=fopen("debuglog.txt","wt"))==NULL)
    return;

	l_wc2.style = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
	l_wc2.lpfnWndProc = dbDebugProc;
	l_wc2.cbClsExtra = 0;
	l_wc2.cbWndExtra = 0;
	l_wc2.hInstance = g_dbhInst;
	l_wc2.hIcon = NULL;
	l_wc2.hCursor = NULL;
	l_wc2.hbrBackground = GetStockObject(LTGRAY_BRUSH);
	l_wc2.lpszMenuName =NULL;
	l_wc2.lpszClassName = DB_WINDOW_CLASS;

  if (!RegisterClass(&l_wc2))
    g_dbBlock=TRUE;


  return;
  }

#pragma argsused
void dbInitDebugSecondApp(HINSTANCE p_hInst,void (FAR *p_wrapup)())
  {
  g_dbWrapupProc=p_wrapup;
  return;
  }

BOOL dbInitDebug(HINSTANCE p_hInst)
  {
  int i;

  if (g_dbDebugOK)
    return TRUE;

  g_dbhInst=p_hInst;

  if (!g_dbBlock)
    {
    /* Clear all message areas */
    g_dbTopMsg=0;
    for(i=0;i<DB_DEBUG_LINES;i++)
      g_dbInfo[i][0]=0;

    /* open debug window */
	  g_dbhWnd = CreateWindow(
			 DB_WINDOW_CLASS,
			 "Debug",
			 WS_BORDER|WS_CAPTION|WS_CLIPCHILDREN|WS_THICKFRAME|WS_POPUP
        |WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 NULL,
			 NULL,
			 g_dbhInst,
			 NULL
			 );

    if (g_dbhWnd)
      {
      ShowWindow(g_dbhWnd,SW_SHOWNORMAL);
      g_dbDebugOK=TRUE;
      }
    else
      g_dbDebugOK=FALSE;
    }
  return g_dbDebugOK;
  }


void dbShutdownDebug()
  {
  if (g_dbDebugOK)
    {
    /* close debug window */
    DestroyWindow(g_dbhWnd);
    g_dbDebugOK=FALSE;
    }
  return;
  }

void WINAPI dbPrintNum(char *p_msg,int p_len)
  {
  int i;
  if (!g_dbDebugOK)
    return;

  fprintf(g_dbLog,p_msg);
  fprintf(g_dbLog,"\n");

  for (i=0;(i<p_len)&&(p_msg[i])&&(i<(DB_DEBUG_COLUMNS-1));i++)
    {
    if (p_msg[i]>=32)
      g_dbInfo[g_dbTopMsg][i]=p_msg[i];
    else
      g_dbInfo[g_dbTopMsg][i]='_';
    }
  g_dbInfo[g_dbTopMsg][i]=0;
  g_dbTopMsg++;
  if (g_dbTopMsg>=DB_DEBUG_LINES)
    g_dbTopMsg=0;
  InvalidateRgn(g_dbhWnd,NULL,TRUE);
  return;
  }

LRESULT CALLBACK _export dbDebugProc(HWND p_hWnd, UINT p_message,
				WPARAM p_wParam, LPARAM p_lParam)
	{
  RECT l_rect;

  switch (p_message)
    {
 		case WM_PAINT:
      if (IsIconic(p_hWnd))
        break;
      /* Paint window */
			if (GetUpdateRect(p_hWnd,&l_rect,TRUE))
				{
      	PAINTSTRUCT l_ps; // paint structure
	      HDC l_hDC; // device context
        HFONT l_oldFont;
        int i,j;

        l_hDC=BeginPaint(p_hWnd,&l_ps);

        l_oldFont=SelectObject(l_hDC,GetStockObject(SYSTEM_FONT));
        SetBkColor(l_hDC,RGB(196,196,196));
        SetBkMode(l_hDC,OPAQUE);

        i=g_dbTopMsg;      
        j=0;
        GetClientRect(g_dbhWnd,&l_rect);
        do
          {
          l_rect.top=j*18;
          l_rect.bottom=j*18+10;
          DrawText(l_hDC,g_dbInfo[i],strlen(g_dbInfo[i]),&l_rect,
            DT_LEFT|DT_NOCLIP|DT_SINGLELINE);
          i++;
          j++;
          if (i>=DB_DEBUG_LINES)
            i=0;
          }
        while(i!=g_dbTopMsg);

        SelectObject(l_hDC,l_oldFont);
  			EndPaint(p_hWnd,&l_ps);

				return 0L;
				}
			break;
    case WM_SYSCOMMAND:
      switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          dbShutdownDebug();
          return 0L;
				default:
					break;
				}
      break;
    case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case DB_USER_KILLWINDOW:
          dbShutdownDebug();
          return 0L;
        case DB_USER_KILLPROCESS:
          (*g_dbWrapupProc)();
          return 0L;
        default:
          break;
        }
      break;
		case WM_RBUTTONDOWN:
			{
			POINT l_ptCurrent;
			HMENU l_hmenu;
			UINT l_flags;

			/* let's create that handy little pop-up menu */
#ifdef WIN32
			l_ptCurrent.x = LOWORD(p_lParam);
      l_ptCurrent.y = HIWORD(p_lParam);
#else
			l_ptCurrent = MAKEPOINT(p_lParam);
#endif
			l_hmenu = CreatePopupMenu();
			l_flags=MF_ENABLED;
 			AppendMenu(l_hmenu, l_flags, DB_USER_KILLPROCESS,"Kill process");
 			AppendMenu(l_hmenu, l_flags, DB_USER_KILLWINDOW,"Close debug window");
			ClientToScreen(p_hWnd, &l_ptCurrent);
			TrackPopupMenu(l_hmenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, l_ptCurrent.x,
					l_ptCurrent.y, 0, p_hWnd, NULL);
       return 0L;
			}
    default:
      break;
    }
  return (DefWindowProc(p_hWnd, p_message, p_wParam, p_lParam));
  }

#endif /* DB_DEBUG_NODEBUG */
