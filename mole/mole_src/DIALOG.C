// MOLE Client for MS Windows 3.11 (WIN32)
// dialog.c identifier: dl
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
#include"dialog.h"

/** structures used only in this module **/
typedef struct DLDIALOGPOOLtag {
	HGLOBAL dMemory;
	HWND dWnd;
	struct DLDIALOGPOOLtag *dNext;
} DLDIALOGPOOL;

/* globals used only in this module */
DLDIALOGPOOL *g_dlDialogPool;


/* Init dialog */
BOOL dlInitDialog(void) {
	g_dlDialogPool=NULL;
	return TRUE;
}

void dlShutdownDialog(void) {
	while(g_dlDialogPool)
		dlDestroyWindow(g_dlDialogPool->dWnd);
	return;
}

/* The following proc takes a dialog box window handle and
 * will do the appropriate math to centre the dialog box
 * on the screen, both horizontally and vertically. */
void dlCentreDialogBox(HWND p_hWnd)
	{
	POINT l_point;
	RECT l_rect;

	/* let's position ourselves in the middle of the screen */
	GetWindowRect(GetDesktopWindow(),&l_rect); // find middle of screen
	l_point.x=(l_rect.right-l_rect.left)/2;
	l_point.y=(l_rect.bottom-l_rect.top)/2; // l_point is now the scrn middle
	GetWindowRect(p_hWnd,&l_rect); // get dimentions of this dialog box
	l_point.x-=(l_rect.right-l_rect.left)/2; // get top-left of centred dialog box
	l_point.y-=(l_rect.bottom-l_rect.top)/2;
	/* and now do the great re-positioning deed (without resize) */
	SetWindowPos(p_hWnd,HWND_TOP,l_point.x,l_point.y,0,0,SWP_NOACTIVATE|SWP_NOSIZE);
	}

void dlCascadeDialogBox(HWND p_hWnd)
	{
	static POINT l_startpoint={20,20};
	POINT l_point;
	RECT l_rect;
	BOOL l_wrap;

	GetWindowRect(p_hWnd,&l_rect); // get dimentions of this dialog box
	l_point.x=l_startpoint.x;
	l_point.y=l_startpoint.y;
	l_wrap=FALSE;
	if ((l_rect.bottom-l_rect.top+l_point.y)>GetSystemMetrics(SM_CYSCREEN))
		{
		l_point.y=0;
		l_wrap=TRUE;
		}
	if ((l_rect.right-l_rect.left+l_point.x)>GetSystemMetrics(SM_CXSCREEN))
		{
		l_point.x=0;
		l_wrap=TRUE;
		}

	/* and now do the great re-positioning deed (without resize) */
	if (l_wrap)
		{
		l_point.x=l_startpoint.x=0;
		l_point.y=l_startpoint.y=0;
		}
	l_startpoint.x+=GetSystemMetrics(SM_CYCAPTION);
	l_startpoint.y+=GetSystemMetrics(SM_CYCAPTION);
	SetWindowPos(p_hWnd,HWND_TOP,l_point.x,l_point.y,0,0,SWP_NOACTIVATE|SWP_NOSIZE);
	}

#pragma argsused
BOOL CALLBACK _export dl3DSubClassProc(HWND p_hWnd, UINT p_message,
				WPARAM p_wParam, LPARAM p_lParam) {
	if (p_message==WM_INITDIALOG) {
		Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
		return TRUE;
	}
	return FALSE;
}

void dlAddToDialogPool(HWND p_hWnd) {
	HGLOBAL l_GlobalTemp;
	DLDIALOGPOOL *l_dialog;

	l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(DLDIALOGPOOL));
	l_dialog=(DLDIALOGPOOL *)GlobalLock(l_GlobalTemp);
	if ((!l_GlobalTemp)||(!(l_dialog)))
		return;
	l_dialog->dWnd=p_hWnd;
	l_dialog->dMemory=l_GlobalTemp;
	l_dialog->dNext=g_dlDialogPool;
	g_dlDialogPool=l_dialog;
	return;
}

BOOL dlIsDialogMessage(MSG* p_msg) {
	DLDIALOGPOOL *l_dialog;

	for (l_dialog=g_dlDialogPool;l_dialog;l_dialog=l_dialog->dNext)
		if (IsDialogMessage(l_dialog->dWnd,p_msg))
			return TRUE;
	return FALSE;
}

void dlRemoveFromDialogPool(HWND p_hWnd) {
	HGLOBAL l_GlobalTemp;
	DLDIALOGPOOL *l_dialog,*l_lastdialog;

	l_lastdialog=NULL;
	for (l_dialog=g_dlDialogPool;l_dialog;l_dialog=l_dialog->dNext) {
		if (l_dialog->dWnd==p_hWnd)
			break;
		l_lastdialog=l_dialog;
	}

	if (l_dialog) {
		/* extract from list */
		if (l_lastdialog)
			l_lastdialog->dNext=l_dialog->dNext;
		else
			g_dlDialogPool=l_dialog->dNext;
		/* free dialog */
		l_GlobalTemp=l_dialog->dMemory;
		GlobalUnlock(l_GlobalTemp);
		GlobalFree(l_GlobalTemp);
	} /* else window not found */
	return;
}

void dlDestroyWindow(HWND p_hWnd) {
	dlRemoveFromDialogPool(p_hWnd);
	DestroyWindow(p_hWnd);
	return;
}
