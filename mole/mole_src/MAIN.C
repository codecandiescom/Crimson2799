// main.c identifier: mn
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
#include<ctl3d.h>
#include<commdlg.h>
#include"winsock.h"
#include"molem.h"
#include"molerc.h"
#include"main.h"
#include"debug.h"
#include"about.h"
#include"host.h"
#include"moleprot.h"
#include"terminal.h"
#include"edit.h"
#include"areawnd.h"

LRESULT CALLBACK _export mnMainProc(HWND p_hWnd, UINT p_message,
				WPARAM p_wParam, LPARAM p_lParam)
	{
  static char l_buf[200];
  int l_i;

  switch (p_message)
		{
		case WM_SYSCOMMAND:
      switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
 					aaWrapup(); /* call wrapup procedure */
					return(0L);
        default:
          break;
				}
      break;
		case WM_COMMAND:
			{
			switch (GET_WM_COMMAND_ID(p_wParam,p_lParam)) /* Process menu selection event */
				{
				case CM_MAINFILECONNECT:
          hoConnectHost();
          mnMenuMainUpdate();
          return(0L);
				case CM_MAINFILEDISCONNECT:
          hoDisconnectHost(FALSE);
          mnMenuMainUpdate();
          return(0L);
				case CM_MAINFILEEXIT:
					aaWrapup();
					return(0L);
        case CM_MAINVIEWTERMINAL:
          if (trIsTerminalVisible())
            trHideTerminal();
          else
            trShowTerminal();
          break;
        case CM_MAINVIEWAREALIST:
          if (awIsAreaWindowVisible())
            awHideAreaWindow();
          else
            awShowAreaWindow();
          break;
        case CM_MAINHELPCONTENTS:
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTENTS,0L);
          return 0L;
        case CM_MAINHELPSEARCH:
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_FORCEFILE,0L);
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_COMMAND,(DWORD)"Search()");
          return 0L;
        case CM_MAINHELPHOWTOUSE:
          WinHelp(g_aahWnd,NULL,HELP_HELPONHELP,0L);
          return 0L;
        case CM_MAINHELPABOUT:
          DialogBox(g_aahInst,MAKEINTRESOURCE(DIALOG_ABOUT),g_aahWnd,abDialogAboutProc);
          return(0L);
				default:
					break;
				}
			}
			break;
    case WM_SYSCOLORCHANGE:
      Ctl3dColorChange();
			break;
    case WM_USER_TERMINAL_INPUT:
      do {
        l_i=trReadTerminalInput(l_buf,200);
        if (l_i) {
//        dbPrint("This is about to be sent out:");
//        dbPrintNum(l_buf,l_i);
          mpSendBufferAppend(l_buf,l_i);
          mpMoleSendData();
        }
      } while(l_i);
      return(0L);
    case WM_USER_HOST_SOCKET:
//      dbPrint("Check socket");
      if (g_hoConnectedToHost) /* we only go async AFTER we've got a connection */
        hoSocketHandler(p_wParam, p_lParam);
      return(0L);
    case WM_USER_EDITERROR:
      edEditError(p_wParam,p_lParam);
      return (0L);
    case WM_TIMER:
      /* check up on our socket and our request Q */
//      dbPrint("Timer Tick");
      if (g_hoConnectedToHost)
        mpCheckTimeout();
      return(0L);
		default:
			if (p_message==g_aaCommDlgHelp) // we'll get this from our file dialog boxes boxes.
				{
				WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTENTS,0L);
				return 0L;
				}
			break;
		}
	return (DefWindowProc(p_hWnd, p_message, p_wParam, p_lParam));
	}

/* MENU COMMAND LOCATIONS - if you alter the menu structure, CHANGE THESE
 * to reflect the changes */
#define MN_MENU_MAIN_POS_FILE 0
//#define MN_MENU_MAIN_POS_EDIT 1
#define MN_MENU_MAIN_POS_VIEW 1
#define MN_MENU_MAIN_POS_HELP 2
void mnMenuMainUpdate() {
	HMENU l_menu,l_submenu;

  if (!g_aahWnd)
    return;

  l_menu=GetMenu(g_aahWnd);

  /* File menu */
	l_submenu=GetSubMenu(l_menu,MN_MENU_MAIN_POS_FILE);
  if (g_hoConnectedToHost) {
    EnableMenuItem(l_submenu,CM_MAINFILEDISCONNECT,
      MF_BYCOMMAND|MF_ENABLED);
  } else {
    EnableMenuItem(l_submenu,CM_MAINFILEDISCONNECT,
      MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
  }

  /* view menu */
	l_submenu=GetSubMenu(l_menu,MN_MENU_MAIN_POS_VIEW);
  if (trIsTerminalVisible()) {
    CheckMenuItem(l_submenu,CM_MAINVIEWTERMINAL,
      MF_BYCOMMAND|MF_CHECKED);
  } else {
    CheckMenuItem(l_submenu,CM_MAINVIEWTERMINAL,
      MF_BYCOMMAND|MF_UNCHECKED);
  }
  if (awIsAreaWindowVisible()) {
    CheckMenuItem(l_submenu,CM_MAINVIEWAREALIST,
      MF_BYCOMMAND|MF_CHECKED);
  } else {
    CheckMenuItem(l_submenu,CM_MAINVIEWAREALIST,
      MF_BYCOMMAND|MF_UNCHECKED);
  }


}
