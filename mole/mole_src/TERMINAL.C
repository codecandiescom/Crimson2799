// terminal.c
// two-letter descriptor: tr
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************
//
// This module handles the text terminal for the MUD connection.
#include<windows.h>
#include<windowsx.h>
#include<ctl3d.h>
#include"molem.h"
#include"molerc.h"
#ifdef WIN32
#include"termbuf.h"
#include"ansiterm.h"
#include"main.h"
#endif
#include"terminal.h"
#include"moleprot.h"
#include"host.h"
#include"debug.h"
#include"enviromt.h"
#include"help.h"
#include"about.h"
#include"ctl3dl.h"

/* because otherwise this is a pain in the butt. */
LRESULT CALLBACK _export trAnsiTermAuxProc(ATTERM *p_Term,HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);


#define TR_PROFILE_STR "TerminalWindow"

#ifdef WIN32
ATTERM *g_trTerm; /* handle to our terminal */

BOOL trInitTerminalApplication(HINSTANCE p_hInst) {
  return atInitAnsiTermApplication(p_hInst);
}

BOOL trInitTerminal() {
  if (atInitAnsiTerm()) {
    g_trTerm=atAllocTerm(g_aahInst,g_aahWnd,80L,600L,80L,25L,"TerminalMOLE","MOLE Terminal");
    ((void*)(g_trTerm->aNotify))=((void*)(trNotifyFn));
		((void*)(g_trTerm->aAuxMsgHandler))=((void*)(trAnsiTermAuxProc));
		if (g_trTerm) {
      /* setup any default changes etc. */
      atLoadTermSettings(g_trTerm,TBNOTIFY_OPTIONS,NULL);
      atLoadTermSettings(g_trTerm,TBNOTIFY_FONT,NULL);
			atLoadTermSettings(g_trTerm,TBNOTIFY_MACROS,NULL);
      atAdjustTerm(g_trTerm,-1); /* shows/hides scrollbars, edit, etc. */

      /* now show it! */
      if (enRestoreYesNo(TR_PROFILE_STR,"ShowWindow",TRUE))
        atShowTerm(g_trTerm);
      else
        atHideTerm(g_trTerm);
			return TRUE;
		}
    atShutdownAnsiTerm();
  }
  return FALSE;
}

void trShutdownTerminal() {
  if (g_trTerm) {
    enSaveYesNo(TR_PROFILE_STR,"ShowWindow",atIsTermVisible(g_trTerm));
    atFreeTerm(g_trTerm);
    g_trTerm=NULL;
	}
  return;
}

void trClearTerminal() {
  if (g_trTerm) {
    atClearTerm(g_trTerm);
  }
  return;
}

void trSendTerminal(char *p_buf,unsigned long p_buflen) {
  atSendTerm(g_trTerm,((unsigned char*)(p_buf)),p_buflen);
	return;
}

void trShowTerminal() {
  atShowTerm(g_trTerm);
  mnMenuMainUpdate();
  return;
}

void trHideTerminal() {
  atHideTerm(g_trTerm);
  mnMenuMainUpdate();
	return;
}

BOOL trIsTerminalVisible() {
	return atIsTermVisible(g_trTerm);
}

int trReadTerminalInput(char *p_buf,int p_buflen) {
	return atReadTermInput(g_trTerm,(unsigned char*)(p_buf),(unsigned long)p_buflen);
}

void trNotifyFn(void *p_Term, int p_msg) {
	ATTERM *l_Term=(ATTERM*)p_Term;
	switch (p_msg) {
		case TBNOTIFY_USERINPUT:
			PostMessage(g_aahWnd,WM_USER_TERMINAL_INPUT,0,0L);
			break;
		case TBNOTIFY_OPTIONS:
		case TBNOTIFY_FONT:
		case TBNOTIFY_MACROS:
			atSaveTermSettings(l_Term,p_msg,NULL);
			break;
		case TBNOTIFY_OPTIONSHELP:
			WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_AT_DIALOG_ATOPT_HELP);
			break;
		case TBNOTIFY_FONTHELP:
			WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_COMMDLG_FONT_HELP);
			break;
		case TBNOTIFY_MACROSHELP:
			WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_AT_DIALOG_ATFNKEY_HELP);
			break;
		default:
			dbPrint("WARNING: unhandled ansiterm Notify message");
			break;
	}
	return;
}

LRESULT CALLBACK _export trAnsiTermAuxProc(ATTERM *p_Term,HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
	switch (p_message) {
		case WM_SYSCOMMAND:
			switch(p_wParam) { /* Process Control Box / Max-Min function */
				case SC_CLOSE:
//					aaWrapup(); /* call wrapup procedure */
					trHideTerminal();
					return(0L);
				default:
					break;
			}
			break;
		case WM_COMMAND:
			{
			switch (GET_WM_COMMAND_ID(p_wParam,p_lParam)) /* Process menu selection event */
				{
				case TBNOTIFY_MENUCLOSE: /* blah blah blah FIX THIS PLEASE!!!!!! */
				case CM_ANSICLOSE:
					trHideTerminal();
					break;
				case CM_MAINFILEEXIT:
					aaWrapup();
					return(0L);
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
		default:
			break;
		}
	return (DefWindowProc(p_hWnd, p_message, p_wParam, p_lParam));
}

#else /* WIN32 */

/* definitions */
#define TR_ROWS 20 /* number of rows on terminal */

char g_trBuf[85];
int g_trBufPos;
HWND g_trhWnd;

#pragma argsused
BOOL trInitTerminalApplication(HINSTANCE p_hInst) {
	return TRUE;
}

BOOL trInitTerminal() {
  g_trBufPos=0;
  g_trhWnd=CreateDialog(g_aahInst,MAKEINTRESOURCE(DIALOG_TERMINAL),
    g_aahWnd,trDialogTerminalProc);
  aaAddToDialogPool(g_trhWnd);
  if (!g_trhWnd) {
    return FALSE;
  }
  return TRUE;
}

void trShutdownTerminal() {
  if (g_trhWnd) {
    enSaveWindowPlacement(TR_PROFILE_STR,g_trhWnd);
    aaDestroyWindow(g_trhWnd);
    g_trhWnd=NULL;
  }
  return;
}

void trClearTerminal() {
  int l_i;
  if (g_trhWnd) {
    g_trBufPos=0;
    for (l_i=0;l_i<TR_ROWS;l_i++)
      SendDlgItemMessage(g_trhWnd,IDC_TERMINALSCREEN,LB_DELETESTRING,0,0L);
  }
  return;
}

void trSendTerminal(char *p_buf,unsigned long p_buflen) {
  int l_i,l_curlines;
  BOOL l_newline;
  HWND l_hWnd;
  char l_CRLFcheck;

  if (!p_buf) return;

//  dbPrint("Here at the terminal, we got this:");
//  dbPrintNum(p_buf,p_buflen);

  l_hWnd=GetDlgItem(g_trhWnd,IDC_TERMINALSCREEN);
  l_curlines=(int)SendMessage(l_hWnd,LB_GETCOUNT,0,0L);
  l_CRLFcheck=0x00;
  for (l_i=0;l_i<p_buflen;l_i++) {
    l_newline=FALSE;
    if ((p_buf[l_i]==0x0D)||(p_buf[l_i]==0x0A)) {
      if ((l_CRLFcheck) &&(p_buf[l_i]!=l_CRLFcheck)) {
        /* this is a CRLF combo - ignore second dohickey */
        l_CRLFcheck=0x00; /* clear CRLF check flag */
      } else {
        l_newline=TRUE;
        l_CRLFcheck=p_buf[l_i];
      }
    } else {
      l_CRLFcheck=0x00;
      if ((p_buf[l_i]>=32)&&(p_buf[l_i]<127)) {
        g_trBuf[g_trBufPos]=p_buf[l_i];
        g_trBufPos++;
        if (g_trBufPos>=80)
          l_newline=TRUE;
      } else {
        /* ignore character */
      }
    }
    if (l_newline) {
//      dbPrint("Newline to the terminal!");
      g_trBuf[g_trBufPos]=0; /* terminate string */
//      dbPrint(g_trBuf);
      /* delete last line - for replacement */
      if (l_curlines>0)
        SendMessage(l_hWnd,LB_DELETESTRING,l_curlines-1,0L);
      /* append this completed line */
      SendMessage(l_hWnd,LB_ADDSTRING,0,(LPARAM)(LPCSTR)g_trBuf);
      g_trBuf[0]=0;
      g_trBufPos=0;
      /* and append a new (empty) line */
      SendMessage(l_hWnd,LB_ADDSTRING,0,(LPARAM)(LPCSTR)g_trBuf);
      /* delete first line - for scroll */
      if (l_curlines>=TR_ROWS)
        SendMessage(l_hWnd,LB_DELETESTRING,0,0L);
      l_curlines=(int)SendMessage(l_hWnd,LB_GETCOUNT,0,0L);
    }
  }
  /* and now update our last line */
  /* delete last line - for replacement */
  g_trBuf[g_trBufPos]=0; /* terminate string */
  if (l_curlines>0)
    SendMessage(l_hWnd,LB_DELETESTRING,l_curlines-1,0L);
  /* append this completed line */
  SendMessage(l_hWnd,LB_ADDSTRING,0,(LPARAM)(LPCSTR)g_trBuf);
  return;
}

void trShowTerminal() {
  if (g_trhWnd) {
    ShowWindow(g_trhWnd,SW_SHOWMAXIMIZED);
  }
  return;
}

void trHideTerminal() {
  if (g_trhWnd) {
    ShowWindow(g_trhWnd,SW_HIDE);
  }
  return;
}

BOOL trIsTerminalVisible() {
  return IsWindowVisible(g_trhWnd);
}

int trReadTerminalInput(char *p_buf,int p_buflen) {
  int l_i;

//  dbPrint("Fetching terminal input:");
  if (g_trhWnd && p_buf) {
    l_i=GetDlgItemText(g_trhWnd,IDC_TERMINALCOMMAND,p_buf,p_buflen-1);
    /* first, append a CR to the end */
//    p_buf[l_i]='\r';
//    l_i++;
//    dbPrintNum(p_buf,l_i);
    /* now clear and re-enable windows and return buffer */
    SetDlgItemText(g_trhWnd,IDC_TERMINALCOMMAND,"");    /* blank out line */
//    EnableWindow(GetDlgItem(g_trhWnd,IDC_TERMINALSEND),TRUE);
    EnableWindow(GetDlgItem(g_trhWnd,IDC_TERMINALCOMMAND),TRUE);
    SetFocus(GetDlgItem(g_trhWnd,IDC_TERMINALCOMMAND));
  } else {
    l_i=0;
  }
  return l_i;
}

#pragma argsused
BOOL CALLBACK _export trDialogTerminalProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam)
	{
  switch (p_message)
		{
    case WM_INITDIALOG:
      {
      RECT l_rect;
      WINDOWPLACEMENT *l_placement;

      Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      GetWindowRect(p_hWnd,&l_rect);
      l_placement=enRestoreWindowPlacement(TR_PROFILE_STR,l_rect.left,l_rect.top,0,0);
      /* keep size the same -- it's a dialog box */
      l_placement->rcNormalPosition.bottom=l_placement->rcNormalPosition.top+
        (l_rect.bottom-l_rect.top);
      l_placement->rcNormalPosition.right=l_placement->rcNormalPosition.left+
        (l_rect.right-l_rect.left);
      SetWindowPlacement(p_hWnd,l_placement);
      }
      return TRUE;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          trHideTerminal();
					return TRUE;
				default:
					break;
				}
      break;
    case WM_COMMAND:

      switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case IDC_TERMINALCOMMAND:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=EN_VSCROLL)
            break;
//        case IDC_TERMINALSEND:
          if (g_hoConnectedToHost) {
            /* Note that this could interrupt another process. We don't want
             * to interrupt anything so let's send a message to our main
             * window and let it read us and send to the socket. We'll
             * disable the window until it's been successfully read */
//            EnableWindow(GetDlgItem(p_wParam,IDC_TERMINALSEND),FALSE);
            EnableWindow(GetDlgItem(p_wParam,IDC_TERMINALCOMMAND),FALSE);
            PostMessage(g_aahWnd,WM_USER_TERMINAL_INPUT,0,0L);
          }
          return TRUE;
        case IDC_TERMINALSCREEN:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)==LBN_SELCHANGE) {
            SendDlgItemMessage(p_hWnd,IDC_TERMINALSCREEN,LB_SETCURSEL,-1,0);
            SetFocus(GetDlgItem(g_trhWnd,IDC_TERMINALCOMMAND));
          }
          return TRUE;
        default:
          break;
        }
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
  }

#endif /* WIN32 */