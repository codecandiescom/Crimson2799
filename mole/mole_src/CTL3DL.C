// ctl3d.c
// Two-letter Module Descriptor: d3
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

// This module handles ctl3xxx.dll functionality.
// This module encapsulates the .dll functionality so that we have more
// control over it. Simply compile and link in this file instead of the .DLL
// or associated static library. You can still use the standard <ctl3d.h> include
// file because this file exactly replicates the .DLL functionality based on <ctl3d.h>.
//
// This module is self-contained and doesn't need any Init or Wrapup calls except
// for the normal CTL3D calls.

#include<windows.h>
#include<ctl3d.h>
#include<stdio.h>
#include"debug.h"

/* Defines */
#define	D3_STATE_NOT_LOADED   0 /* NOTE: for STATE vars, >0 means good load, <0 means error/noload */
#define D3_STATE_LOADED       1
#define D3_STATE_ERROR      (-1)
#define D3_STATE_DONT_LOAD  (-2)
#define D3_STATE_INTERNAL   (-3)
//#define D3_STATE_INTERNAL   (-3) /* operation is internal -- no .DLL loaded or required*/

#define D3_PROC_ATOM        "CTL3DL-DefProcAtom"
#define D3_PROC_ATOMLOW     "CTL3DL-DefProcAtom-Low"
#define D3_PROC_ATOMHI      "CTL3DL-DefProcAtom-Hi"

/* Globals */
/* All globals in this module start with "d3" for 3-D Link */
HINSTANCE d3_hInst=NULL;
int d3_State=D3_STATE_NOT_LOADED;
//int d3_State=D3_STATE_DONT_LOAD;

/* Private (local) function prototypes */
int d3InitCtl3d(void);
//LRESULT CALLBACK _export d3InternalSubclassProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
//BOOL d3SubClassWnd(HWND p_hWnd);

/* CTL3D Function handle (variable) declarations */
BOOL WINAPI (*d3_Ctl3dSubclassDlg)(HWND p_a, WORD p_b);
BOOL WINAPI (*d3_Ctl3dSubclassDlgEx)(HWND, DWORD);

WORD WINAPI (*d3_Ctl3dGetVer)(void);
BOOL WINAPI (*d3_Ctl3dEnabled)(void);

HBRUSH WINAPI (*d3_Ctl3dCtlColor)(HDC, LONG);	// ARCHAIC, use Ctl3dCtlColorEx
HBRUSH WINAPI (*d3_Ctl3dCtlColorEx)(UINT wm, WPARAM wParam, LPARAM lParam);

BOOL WINAPI (*d3_Ctl3dColorChange)(void);

BOOL WINAPI (*d3_Ctl3dSubclassCtl)(HWND);
BOOL WINAPI (*d3_Ctl3dSubclassCtlEx)(HWND, int);
BOOL WINAPI (*d3_Ctl3dUnsubclassCtl)(HWND);

LONG WINAPI (*d3_Ctl3dDlgFramePaint)(HWND, UINT, WPARAM, LPARAM);

BOOL WINAPI (*d3_Ctl3dAutoSubclass)(HANDLE);
BOOL WINAPI (*d3_Ctl3dIsAutoSubclass)(VOID);
BOOL WINAPI (*d3_Ctl3dUnAutoSubclass)(VOID);

BOOL WINAPI (*d3_Ctl3dRegister)(HANDLE);
BOOL WINAPI (*d3_Ctl3dUnregister)(HANDLE);

//begin DBCS: far east short cut key support
VOID WINAPI (*d3_Ctl3dWinIniChange)(void);
//end DBCS



/* Actual Procedures */
int d3InitCtl3d()
	{
	UINT l_emf;
	DWORD l_winver;
	char l_buf[100];

	if (d3_State)
		{ /* we've already tried to load, or we're instructed to load/not load */
		if (d3_State>0) return TRUE;
		return FALSE;
		}

//	d3_State=D3_STATE_INTERNAL;
//	return FALSE;

	dbPrint("CTL3D: Init");
	/* Check our version of windows */
	l_winver=GetVersion();
	sprintf(l_buf,"CTL3D: Win %li.%02li,  O/S %04X",l_winver&0x000000FFL,
		(l_winver&0x0000FF00L)>>8,HIWORD(l_winver));
	dbPrint(l_buf);
	if ((l_winver&0x000000FFL)==3)
		{ /* need at least version 3.10 to run! */
		if (((l_winver&0x0000FF00L)>>8)<10)
			{ /* pre-3.10 (probably 3.0) - no load */
			dbPrint("CTL3D: LOAD ABORTED - Pre-3.10 version of windows");
			d3_State=D3_STATE_DONT_LOAD;
			return FALSE;
			}
		else if (((l_winver&0x0000FF00L)>>8)>=95) /* windows 95 */
			{
			dbPrint("CTL3D: LOAD ABORTED - 3.95-3.99 version of windows. Using internal methods");
			d3_State=D3_STATE_INTERNAL;
			return FALSE;
			}
		/* otherwise, we're ok to load libraries! */
		}
	else if ((l_winver&0x000000FFL)>3)
		{ /* Post-win3.11 -> don't load */
		dbPrint("CTL3D: WARNING - Post-3.XX version of windows. Using internal methods ");
		d3_State=D3_STATE_INTERNAL;
		return FALSE;
		}
	else
		{ /* Pre-win3.0 -> don't load */
		dbPrint("CTL3D: LOAD ABORTED - Pre-3.00 version of windows");
		d3_State=D3_STATE_DONT_LOAD;
		return FALSE;
		}

	l_emf=SetErrorMode(SEM_NOOPENFILEERRORBOX);

#ifdef WIN32
	dbPrint("CTL3D: Loading 32-bit library CTL3D32.DLL");
	d3_hInst=LoadLibrary("CTL3D32.DLL");
	if (d3_hInst==0)
		{
		sprintf(l_buf,"CTL3D: Library load failed (error 0x%lX)",(unsigned long)GetLastError());
#else
	dbPrint("CTL3D: Loading 16-bit library CTL3DV2.DLL");
	d3_hInst=LoadLibrary("CTL3DV2.DLL");
	if (d3_hInst<=HINSTANCE_ERROR)
		{
//		switch(d3_hInst)
//			{
//			case 0: // system error
//			case 2: // file not found
//			case 3: // path not found
//			case 5: // sharing/protection error
//			case 8: // lib required separate data segment for each task
//			case 10: // Incorrect Windows version
//			case 11: // Executable file is invalid
//			case 12: // Wrong O/S
//			case 13: // Designed for DOS 4.0
//			case 14: // Unknown executable type/format
//			case 15: // attempt to load real-mode executable (earlier version of Windows)
//			case 16: // Attempt was made to load a second instance of an executable file containing multiple data segments that were not marked read-only
//			case 19: // Attempt to load compressed executable file. Decompress it first.
//			case 20: // DLL file is invalid. DLL file is corrupt.
//			case 21: // Requires 32-bit extension
//			default:
//				break;
//			}
		sprintf(l_buf,"CTL3D: Library load failed (error 0x%lX)",(unsigned long)d3_hInst);
#endif
		dbPrint(l_buf);
		SetErrorMode(l_emf);
		d3_State=D3_STATE_ERROR;
    d3_hInst=NULL;
		return FALSE;
		}

	dbPrint("CTL3D: Fetching procedure handles");
	if (((FARPROC)d3_Ctl3dSubclassDlg=GetProcAddress(d3_hInst,"Ctl3dSubclassDlg"))==NULL)
		d3_State=D3_STATE_ERROR;
	if (((FARPROC)d3_Ctl3dSubclassDlgEx=GetProcAddress(d3_hInst,"Ctl3dSubclassDlgEx"))==NULL)
		d3_State=D3_STATE_ERROR;

	if (((FARPROC)d3_Ctl3dGetVer=GetProcAddress(d3_hInst,"Ctl3dGetVer"))==NULL)
		d3_State=D3_STATE_ERROR;
	if (((FARPROC)d3_Ctl3dEnabled=GetProcAddress(d3_hInst,"Ctl3dEnabled"))==NULL)
		d3_State=D3_STATE_ERROR;

	if (((FARPROC)d3_Ctl3dCtlColor=GetProcAddress(d3_hInst,"Ctl3dCtlColor"))==NULL)
		d3_State=D3_STATE_ERROR;
	if (((FARPROC)d3_Ctl3dCtlColorEx=GetProcAddress(d3_hInst,"Ctl3dCtlColorEx"))==NULL)
		d3_State=D3_STATE_ERROR;

	if (((FARPROC)d3_Ctl3dColorChange=GetProcAddress(d3_hInst,"Ctl3dColorChange"))==NULL)
		d3_State=D3_STATE_ERROR;

	if (((FARPROC)d3_Ctl3dSubclassCtl=GetProcAddress(d3_hInst,"Ctl3dSubclassCtl"))==NULL)
		d3_State=D3_STATE_ERROR;
//	if (((FARPROC)d3_Ctl3dSubclassCtlEx=GetProcAddress(d3_hInst,"Ctl3dSubclassCtlEx"))==NULL)
//		d3_State=D3_STATE_ERROR;
//	if (((FARPROC)d3_Ctl3dUnsubclassCtl=GetProcAddress(d3_hInst,"Ctl3dUnsubclassCtl"))==NULL)
//		d3_State=D3_STATE_ERROR;

	if (((FARPROC)d3_Ctl3dDlgFramePaint=GetProcAddress(d3_hInst,"Ctl3dDlgFramePaint"))==NULL)
		d3_State=D3_STATE_ERROR;

	if (((FARPROC)d3_Ctl3dAutoSubclass=GetProcAddress(d3_hInst,"Ctl3dAutoSubclass"))==NULL)
		d3_State=D3_STATE_ERROR;
//	if (((FARPROC)d3_Ctl3dIsAutoSubclass=GetProcAddress(d3_hInst,"Ctl3dIsAutoSubclass"))==NULL)
//		d3_State=D3_STATE_ERROR;
//	if (((FARPROC)d3_Ctl3dUnAutoSubclass=GetProcAddress(d3_hInst,"Ctl3dUnAutoSubclass"))==NULL)
//		d3_State=D3_STATE_ERROR;

	if (((FARPROC)d3_Ctl3dRegister=GetProcAddress(d3_hInst,"Ctl3dRegister"))==NULL)
		d3_State=D3_STATE_ERROR;
	if (((FARPROC)d3_Ctl3dUnregister=GetProcAddress(d3_hInst,"Ctl3dUnregister"))==NULL)
		d3_State=D3_STATE_ERROR;

//begin DBCS: far east short cut key support
	if (((FARPROC)d3_Ctl3dWinIniChange=GetProcAddress(d3_hInst,"Ctl3dWinIniChange"))==NULL)
		d3_State=D3_STATE_ERROR;
//end DBCS

	if (d3_State<0)
		{
		dbPrint("CTL3D: Error fetching handles. Library released.");
		FreeLibrary(d3_hInst);
    d3_hInst=NULL;
		return FALSE;
		}
		dbPrint("CTL3D: Handles fetched. CTL3D Ready.");
	d3_State=D3_STATE_LOADED;
	return TRUE;
}

BOOL WINAPI Ctl3dSubclassDlg(HWND p_a, WORD p_b)
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dSubclassDlg)(p_a,p_b);
	return FALSE;
	}
BOOL WINAPI Ctl3dSubclassDlgEx(HWND p_a, DWORD p_b)
	{
//	HWND l_hWnd;

	if (d3InitCtl3d())
		return (*d3_Ctl3dSubclassDlgEx)(p_a,p_b);
//	else if (d3_State==D3_STATE_INTERNAL)
//		{
//		/* internal sub-class this dialog box */
//		/* Note: Parameter p_b is ignored. */
//		/* First, subclass parent */
//		if (!d3SubClassWnd(p_a))
//			return FALSE;
//		/* Now subclass all child windows */
//		for(l_hWnd=GetWindow(p_a,GW_CHILD);l_hWnd;l_hWnd=GetWindow(l_hWnd,GW_HWNDNEXT))
//			{
//			d3SubClassWnd(l_hWnd);
//			}
//		return TRUE;
//		}
	return FALSE;
	}

//BOOL d3SubClassWnd(HWND p_hWnd)
//	{
//	FARPROC l_TempProc;
// 	HDC l_hDC;
//
//	l_TempProc=(FARPROC)SetWindowLong(p_hWnd,GWL_WNDPROC,(LONG)d3InternalSubclassProc);
//#ifdef WIN32
//	if (!SetProp(p_hWnd,D3_PROC_ATOM,(HANDLE)l_TempProc))
//		{
//		SetWindowLong(p_hWnd,GWL_WNDPROC,(LONG)l_TempProc);
//		return FALSE;
//		}
//#else
//	if (SetProp(p_hWnd,D3_PROC_ATOMLOW,(HANDLE)LOWORD(l_TempProc)))
//		{
//		if (!SetProp(p_hWnd,D3_PROC_ATOMHI,(HANDLE)HIWORD(l_TempProc)))
//			{
//			RemoveProp(p_hWnd,D3_PROC_ATOMLOW);
//			SetWindowLong(p_hWnd,GWL_WNDPROC,(LONG)l_TempProc);
//			return FALSE;
// 			}
//		}
//	else
//		{
//		SetWindowLong(p_hWnd,GWL_WNDPROC,(LONG)l_TempProc);
//		return FALSE;
//		}
//#endif
///* didn't work
//	l_hDC=GetWindowDC(p_hWnd);
//	SetBkColor(l_hDC,RGB(198,198,198));
//	ReleaseDC(p_hWnd,l_hDC);
//	l_hDC=GetDC(p_hWnd);
//	SetBkColor(l_hDC,RGB(198,198,198));
//	ReleaseDC(p_hWnd,l_hDC); */
//	return TRUE;
//	}

WORD WINAPI Ctl3dGetVer()
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dGetVer)();
	return 0;
	}
BOOL WINAPI Ctl3dEnabled()
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dEnabled)();
	return FALSE;
	}

HBRUSH WINAPI Ctl3dCtlColor(HDC p_a, LONG p_b)	// ARCHAIC, use Ctl3dCtlColorEx
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dCtlColor)(p_a,p_b);
	return NULL;
	}
HBRUSH WINAPI Ctl3dCtlColorEx(UINT p_wm, WPARAM p_wParam, LPARAM p_lParam)
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dCtlColorEx)(p_wm,p_wParam,p_lParam);
	return NULL;
	}

BOOL WINAPI Ctl3dColorChange()
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dColorChange)();
	return FALSE;
	}

BOOL WINAPI Ctl3dSubclassCtl(HWND p_a)
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dSubclassCtl)(p_a);
	return FALSE;
	}
/*BOOL WINAPI Ctl3dSubclassCtlEx(HWND p_a, int p_b)
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dSubclassCtlEx)(p_a,p_b);
	return FALSE;
	}
BOOL WINAPI Ctl3dUnsubclassCtl(HWND p_a)
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dUnsubclassCtl)(p_a);
	return FALSE;
	}
*/

LONG WINAPI Ctl3dDlgFramePaint(HWND p_a, UINT p_b, WPARAM p_c, LPARAM p_d)
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dDlgFramePaint)(p_a,p_b,p_c,p_d);
	return 0L;
	}

BOOL WINAPI Ctl3dAutoSubclass(HANDLE p_a)
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dAutoSubclass)(p_a);
	return FALSE;
	}
/*BOOL WINAPI Ctl3dIsAutoSubclass()
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dIsAutoSubclass)();
	return FALSE;
	}
BOOL WINAPI Ctl3dUnAutoSubclass()
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dUnAutoSubclass)();
	return FALSE;
	}
*/

BOOL WINAPI Ctl3dRegister(HANDLE p_a)
	{
	if (d3InitCtl3d())
		return (*d3_Ctl3dRegister)(p_a);
	return FALSE;
	}
BOOL WINAPI Ctl3dUnregister(HANDLE p_b)
	{
	int rc=0;

	if (d3InitCtl3d())
		{
#ifdef WIN32
		if (d3_hInst)
#else
		if (d3_hInst>HINSTANCE_ERROR)
#endif
			{
			rc=(*d3_Ctl3dUnregister)(p_b);
			FreeLibrary(d3_hInst);
			d3_State=D3_STATE_NOT_LOADED;
			d3_hInst=NULL;
			}
		return(rc);
		}
	return FALSE;
	}

//begin DBCS: far east short cut key support
VOID WINAPI Ctl3dWinIniChange()
	{
	if (d3InitCtl3d())
		(*d3_Ctl3dWinIniChange)();
	return;
	}
//end DBCS

/*
LRESULT CALLBACK _export d3InternalSubclassProc(HWND p_hWnd, UINT p_message,
				WPARAM p_wParam, LPARAM p_lParam)
	{
	FARPROC l_DefProc;
	RECT l_rect;

	switch (p_message)
		{
		case WM_ERASEBKGND:
			GetClientRect(p_hWnd,&l_rect);
			FillRect((HDC)p_wParam,&l_rect,GetStockObject(LTGRAY_BRUSH));
			return 1L;
		case WM_DESTROY:
#ifdef WIN32
			l_DefProc=(FARPROC)GetProp(p_hWnd,D3_PROC_ATOM);
			RemoveProp(p_hWnd,D3_PROC_ATOM);
#else
			l_DefProc=(FARPROC)MAKELONG(GetProp(p_hWnd,D3_PROC_ATOMLOW),GetProp(p_hWnd,D3_PROC_ATOMHI));
			RemoveProp(p_hWnd,D3_PROC_ATOMLOW);
			RemoveProp(p_hWnd,D3_PROC_ATOMHI);
#endif
			SetWindowLong(p_hWnd,GWL_WNDPROC,(LONG)l_DefProc);
			return ((LRESULT)(*l_DefProc)((HWND)p_hWnd,(UINT)p_message,
				(WPARAM)p_wParam,(LPARAM)p_lParam));
		default:
			break;
		}
#ifdef WIN32
	l_DefProc=(FARPROC)GetProp(p_hWnd,D3_PROC_ATOM);
#else
	l_DefProc=(FARPROC)MAKELONG(GetProp(p_hWnd,D3_PROC_ATOMLOW),GetProp(p_hWnd,D3_PROC_ATOMHI));
#endif
	return ((LRESULT)(*l_DefProc)((HWND)p_hWnd,(UINT)p_message,
		(WPARAM)p_wParam,(LPARAM)p_lParam));
	}
*/

/* This function is intended to be used last in a dialog box operation.
 * It handles the WM_CTLCOLOR (Win16) or WM_CTLCOLOR*(Win32) to set
 * the background nicely.
 */
#pragma argsused
BOOL d3DlgMessageCheck(HWND p_hWnd, UINT p_message,
	WPARAM p_wParam, LPARAM p_lParam) {

 	if (d3_State==D3_STATE_LOADED) return FALSE;

#ifdef WIN32
	switch (p_message) {
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORSTATIC:
//			SetBkColor((HDC)p_wParam,GetStockObject(LTGRAY_BRUSH));
			SetBkMode((HDC)p_wParam,TRANSPARENT);
			/* fall through to the following */
		case WM_CTLCOLORDLG:
			return (BOOL)(GetStockObject(LTGRAY_BRUSH));
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORSCROLLBAR:
		default:
			break;
	}
#else /* Win16 */
  dbPrint("CTL3DL: Warning - untested code!");
	if (p_message==WM_CTLCOLOR) {
		switch(HIWORD(p_lParam) {
			case CTLCOLOR_STATIC:
				SetBkMode((HDC)p_wParam,TRANSPARENT);
			case CTLCOLOR_DLG:
				return (BOOL)(GetStockObject(LTGRAY_BRUSH));
			case CTLCOLOR_BTN:
			case CTLCOLOR_EDIT:
			case CTLCOLOR_LISTBOX:
			case CTLCOLOR_MSGBOX:
			case CTLCOLOR_SCROLLBAR:
			default:
        break;
		}
	}
#endif
	return FALSE;
}
