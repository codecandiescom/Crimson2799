// MOLE Client for MS Windows 3.11 (WIN32)
// molem.c identifier: aa
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1997-1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************
//
// Written by B. Cameron Lesiuk
// March, 1997

#include<windows.h>
#include<ctl3d.h>
#include<commdlg.h>
#include<stdio.h>
#include<dir.h>
#include<string.h>
#include"moledefs.h"
#include"molem.h"
#include"molerc.h"
#include"main.h"
#include"debug.h"
#include"host.h"
#include"moleprot.h"
#include"terminal.h"
#include"timer.h"
#include"areawnd.h"
#include"enviromt.h"
#include"edit.h"
#include"clipbrd.h"

/* Define statements unique to this file */
#define AADEFAULT_UNIQUE_ID "MOLE - March29/97"

/* Global definitions */
HINSTANCE  g_aahInst;              /* Handle to main instance data*/
HACCEL     g_aahAccTable;          /* handle to accelerator table */
char       g_aaWorkingDir[AAFILENAME_MAX_LENGTH]; /* NSCap working directory */
HWND       g_aahWnd;               /* Handle to main window */
char       g_aaInitFile[AAFILENAME_MAX_LENGTH]; /* .ini file */
char       g_aaHelpFile[AAFILENAME_MAX_LENGTH]; /* .hlp file */
UINT       g_aaCommDlgHelp;        /* Window message sent by command dialog boxes */

/* Dialog Box Pool - so we can quickly & easily */
typedef struct AADIALOGPOOLtag {
  HWND aWnd;
  HGLOBAL aMemory;
  struct AADIALOGPOOLtag *aNext;
} AADIALOGPOOL;
AADIALOGPOOL *g_aaDialogPool;

/* WinMain
 * Main windows message handling procedure
 */
int WINAPI WinMain(HINSTANCE p_hInstance, HINSTANCE p_hPrevInstance,
									 LPSTR p_lpCmdLine, int p_nCmdShow)
	{
	MSG l_msg;
  AADIALOGPOOL *l_dialog;
  int q;

  g_aahInst=p_hInstance; // Lots of procedures use this - let's globalize it right now!

  /* First order of business: set our message queue size */
  q=110;  /* initial (requested) size */
  while(!SetMessageQueue(q))
    q--;

	if (!p_hPrevInstance)
    {
		if (!aaInitApplication())
			return (FALSE);
    }
  else
    {
    if (!aaInitSecondApp())
      return (FALSE);
    }

	if (!aaInitInstance(p_lpCmdLine, p_nCmdShow))
		return (FALSE);

	while (GetMessage(&l_msg, NULL, 0, 0))
		{
    /* first, check dialog boxes */
    for (l_dialog=g_aaDialogPool;l_dialog;l_dialog=l_dialog->aNext)
      if (IsDialogMessage(l_dialog->aWnd,&l_msg))
        break;

    if (l_msg.message==WM_USER_NEWDATA) {
      if (l_dialog)
        dbPrint("MMM: WM_USER_NEWDATA dispatched to dialog box");
      else
        dbPrint("MMM: WM_USER_NEWDATA dispatched to main window");
    }
    if (!l_dialog) {
  		/* Only translate message if it is not an accelerator message */
	  	if (!TranslateAccelerator(g_aahWnd, g_aahAccTable, &l_msg))
	  		{
        TranslateMessage(&l_msg);
  	    DispatchMessage(&l_msg);
  			}
      }
		}
	return (l_msg.wParam);
	}

/* nsInitApplication
 * Initialization for application data - only needs to be once, not every
 * instance.
 */
BOOL aaInitApplication()
	{
	WNDCLASS l_wc;

	l_wc.style = CS_HREDRAW|CS_VREDRAW;
	l_wc.lpfnWndProc = mnMainProc;
	l_wc.cbClsExtra = 0;
	l_wc.cbWndExtra = 0;
	l_wc.hInstance = g_aahInst;
	l_wc.hIcon = LoadIcon(g_aahInst, MAKEINTRESOURCE(ICON_MOLE));
	l_wc.hCursor = LoadCursor(NULL, IDC_CROSS);
	l_wc.hbrBackground = GetStockObject(LTGRAY_BRUSH); 
	l_wc.lpszMenuName =MAKEINTRESOURCE(MENU_MAIN);
	l_wc.lpszClassName = AA_WIND_CLASS_NAME;

  dbInitDebugApplication(g_aahInst,aaWrapup);
  trInitTerminalApplication(g_aahInst);

	return (RegisterClass(&l_wc)&&awInitAreaWndApplication());
	}

/* nsInitSecondApp
 * Initialization for the second (or more) instance of the application
 */
BOOL aaInitSecondApp()
	{
  dbInitDebugSecondApp(g_aahInst,aaWrapup);
	return (awInitAreaWndSecondApp());
	}

/* aaInitInstance
 * Initialize instance. Virtually all initialization commands should
 * go here.
 */
#pragma argsused
BOOL aaInitInstance(LPSTR p_lpCmdLine, int p_nCmdShow)
	{
	RECT l_rect;

  /* dialog pool */
  g_aaDialogPool=NULL;

	getcwd(g_aaWorkingDir,AAFILENAME_MAX_LENGTH);
	strcpy(g_aaInitFile,"mole.ini"); // use windows directory for .ini file
	strcpy(g_aaHelpFile,g_aaWorkingDir); // setup help file
	strcat(g_aaHelpFile,"\\mole.hlp");

	Ctl3dRegister(g_aahInst);
	g_aaCommDlgHelp=RegisterWindowMessage(HELPMSGSTRING);

	g_aahAccTable = LoadAccelerators(g_aahInst, MAKEINTRESOURCE(ACC_MAIN));
	g_aahWnd = CreateWindow(
			 AA_WIND_CLASS_NAME,
			 "MOLE",
			 WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 NULL,
			 NULL,
			 g_aahInst,
			 NULL
			 );

	GetClientRect(g_aahWnd,&l_rect);

	if (!g_aahWnd)
		{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(NULL,"Could not open main window!?","MOLE: Something bad happened.",MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);
		return (FALSE);
		}

  /* pre-window initialization */
  if (!dbInitDebug(g_aahInst))  // debug window - initially open
    return(FALSE);
  if (!enInitEnvironment(g_aaInitFile))
    return(FALSE);
  if (!mpInitMoleProt())
    return(FALSE);
  if (!cbInitClipboard())
    return(FALSE);
  if (!edInitEdit())
    return(FALSE);
  if (!trInitTerminal())
    return(FALSE);
  if (!tmInitTimer())
    return(FALSE);
  if (!awInitAreaWnd())
    return(FALSE);
  if (!hoInitHost()) /* call this one last - due to the call to WSAStartup() */
    return(FALSE);
	dbPrint("MOLE: Initialization complete");

  SetWindowPlacement(g_aahWnd,enRestoreWindowPlacement("MainWindow",
    0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYCAPTION)+
    GetSystemMetrics(SM_CYMENU)+(2*GetSystemMetrics(SM_CYFRAME)) ));
	ShowWindow(g_aahWnd, SW_SHOWNORMAL);
  SetActiveWindow(g_aahWnd);

  /* post-window initialization */
  hoConnectHost();
  mnMenuMainUpdate();

  return (TRUE);
  }

void aaWrapup()
  {

  /* Do any touchy-feely checks before we thrash the program */
  hoDisconnectHost(FALSE);
  if (g_hoConnectionState==HO_CONNECT_STATE_ONLINE)
    return;

  /* first disable main window - we don't want recursive
   * exists, or people doing other things while we shut down,
   * which could happen if the network delays things a few seconds! */
  EnableWindow(g_aahWnd,FALSE);

  enSaveWindowPlacement("MainWindow",g_aahWnd);

  /* Shut down various modules */
  WinHelp(g_aahWnd,NULL,HELP_QUIT,0L);
  hoShutdownHost();
  awShutdownAreaWnd();
  tmShutdownTimer();
  trShutdownTerminal();
  edShutdownEdit();
  cbShutdownClipboard();
  mpShutdownMoleProt();
  enShutdownEnvironment();
  dbShutdownDebug();

	DestroyWindow(g_aahWnd); /* destroy main window and all child windows */
	PostQuitMessage(0);  /* signal program to end */

  Ctl3dUnregister(g_aahInst);
  }

void aaAddToDialogPool(HWND p_hWnd) {
  HGLOBAL l_GlobalTemp;
  AADIALOGPOOL *l_dialog;

  l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(AADIALOGPOOL));
  l_dialog=(AADIALOGPOOL *)GlobalLock(l_GlobalTemp);
  if ((!l_GlobalTemp)||(!(l_dialog)))
    return;
  l_dialog->aWnd=p_hWnd;
  l_dialog->aMemory=l_GlobalTemp;
  l_dialog->aNext=g_aaDialogPool;
  g_aaDialogPool=l_dialog;
  return;
}

void aaRemoveFromDialogPool(HWND p_hWnd) {
  HGLOBAL l_GlobalTemp;
  AADIALOGPOOL *l_dialog,*l_lastdialog;

  l_lastdialog=NULL;
  for (l_dialog=g_aaDialogPool;l_dialog;l_dialog=l_dialog->aNext) {
    if (l_dialog->aWnd==p_hWnd)
      break;
    l_lastdialog=l_dialog;
  }

  if (l_dialog) {
    /* extract from list */
    if (l_lastdialog)
      l_lastdialog->aNext=l_dialog->aNext;
    else
      g_aaDialogPool=l_dialog->aNext;
    /* free dialog */
    l_GlobalTemp=l_dialog->aMemory;
    GlobalUnlock(l_GlobalTemp);
    GlobalFree(l_GlobalTemp);
  } /* else window not found */
  return;
}

void aaDestroyWindow(HWND p_hWnd) {
  aaRemoveFromDialogPool(p_hWnd);
  DestroyWindow(p_hWnd);
  return;
}
