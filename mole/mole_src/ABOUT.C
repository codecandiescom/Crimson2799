// MOLE Client for MS Windows 3.11 (WIN32)
// about.c identifier: ab
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

// This module just handles the about box

#include<windows.h>
#include<windowsx.h>
#include<ctl3d.h>
#include<stdio.h>
#include"moledefs.h"
#include"molem.h"
#include"molerc.h"
#include"about.h"
#include"dialog.h"
#include"help.h"
#include"debug.h"
#include"host.h"
#include"moleprot.h"
#include"ctl3dl.h"

#pragma argsused
BOOL CALLBACK _export abDialogAboutProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam)
	{
  switch (p_message)
		{
    case WM_INITDIALOG:
      Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      {
        char l_buf[100];
        if (g_hoConnectionState==HO_CONNECT_STATE_ONLINE) {
          sprintf(l_buf,"%s\n%i",g_hoHostName,g_hoHostPort);
          SetDlgItemText(p_hWnd,IDC_ABOUTCONNECTED,l_buf);
          sprintf(l_buf,"MOLE Version %i.%i",(int)((g_mpHostVersion&0x0000FF00L)>>8),
          (int)(g_mpHostVersion&0x000000FFL));
          SetDlgItemText(p_hWnd,IDC_ABOUTSERVER,l_buf);
        }
        sprintf(l_buf,"MOLE Version %i.%i",MOLE_MAJOR_VERSION,MOLE_MINOR_VERSION);
        SetDlgItemText(p_hWnd,IDC_ABOUTCLIENT,l_buf);
#ifdef WIN32
        sprintf(l_buf,"(supposedly) unlimited");
#else
        sprintf(l_buf,"%i%% free",GetFreeSystemResources(GFSR_SYSTEMRESOURCES));
#endif
        SetDlgItemText(p_hWnd,IDC_ABOUTSYSRES,l_buf);
        sprintf(l_buf,"%li bytes free",GetFreeSpace(0));
        SetDlgItemText(p_hWnd,IDC_ABOUTSYSMEM,l_buf);
      }
      dlCentreDialogBox(p_hWnd);
      return TRUE;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          EndDialog(p_hWnd,0);
					return TRUE;
				default:
					break;
				}
      break;
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case IDOK:
          EndDialog(p_hWnd,1);
          return TRUE;
				case IDHELP:
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_DL_DIALOG_ABOUT_HELP);
          return TRUE;
        default:
          break;
        }
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
  }
