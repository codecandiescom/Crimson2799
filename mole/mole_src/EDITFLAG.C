// editflag.h
// two-character identifier: ef
// Flag editing tool functions.
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

#include<windows.h>
#include<windowsx.h>
#include<ctl3d.h>
#include"molem.h"
#include"molerc.h"
#include"dstruct.h"
#include"edit.h"
#include"dialog.h"
#include"areawnd.h"
#include"editflag.h"
#include"help.h"
#include"ctl3dl.h"

/* edit's the p_ptr property. If p_sType is DS_STYPE_NONE, creates a NEW struct
 * of type p_newType and attaches it to the REFERENCE p_ptr. */
BOOL efEditFlag(HWND p_hWnd,DSFLAG *p_flag,int p_fType,char *p_WindowTitle) {
  EFPARAM l_param;
  int l_rc;
  DSFLAG l_flag;

  if ((!p_flag)||(p_fType<0)||(p_fType>MOLE_LIST_NUMLISTS))
    return FALSE;

  l_param.pWindowTitle=p_WindowTitle;
  l_param.pFlag=p_flag;
  l_param.pFType=p_fType;

  /* saved (old) value - for changed check */
  l_flag=*p_flag;

  /* edit flag */
  l_rc=DialogBoxParam(g_aahInst,MAKEINTRESOURCE(DIALOG_EDITFLAG),p_hWnd,
    efEditFlagProc,(LPARAM)(&l_param));

  if (l_rc==IDOK) {
    if (l_flag==*(l_param.pFlag))
      return FALSE;
    else
      return TRUE;
  }
  return FALSE;
}

BOOL CALLBACK _export efEditFlagProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
  EFPARAM *l_param;
  int l_i;
  DSFTL *l_ftl;

  switch (p_message)
		{
		case WM_INITDIALOG:
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCentreDialogBox(p_hWnd);
      l_param=(EFPARAM *)p_lParam;
      if (!l_param)
        EndDialog(p_hWnd,IDCANCEL);
#ifdef WIN32
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(p_lParam));
#else
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,HIWORD(p_lParam));
      SetProp(p_hWnd,(LPCSTR)g_edLongLo,LOWORD(p_lParam));
#endif
      SetWindowText(p_hWnd,l_param->pWindowTitle);
      /* Ok, let's setup our window to look nice & pretty for the user */
      l_ftl=dsFTLOf(&g_awFTList,l_param->pFType);
      for (l_i=0;l_i<32;l_i++) {
        if (l_ftl) {
          SetDlgItemText(p_hWnd,IDC_EDITFLAG_FLAG1+l_i,l_ftl->fName);
          if ((*(l_param->pFlag))&(1L<<l_i))
            CheckDlgButton(p_hWnd,IDC_EDITFLAG_FLAG1+l_i,TRUE);
          else
            CheckDlgButton(p_hWnd,IDC_EDITFLAG_FLAG1+l_i,FALSE);
          l_ftl=l_ftl->fNext;
        } else {
          EnableWindow(GetDlgItem(p_hWnd,IDC_EDITFLAG_FLAG1+l_i),FALSE);
          ShowWindow(GetDlgItem(p_hWnd,IDC_EDITFLAG_FLAG1+l_i),SW_HIDE);
        }
      }
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
          l_param=(EFPARAM *)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
          l_param=(EFPARAM *)MAKELP(GetProp(p_hWnd,(LPCSTR)g_edLongHi),
            GetProp(p_hWnd,(LPCSTR)g_edLongLo));
#endif
          l_ftl=dsFTLOf(&g_awFTList,l_param->pFType);
          for (l_i=0;l_i<32;l_i++) {
            if (l_ftl) {
              if (IsDlgButtonChecked(p_hWnd,IDC_EDITFLAG_FLAG1+l_i)) {
                (*(l_param->pFlag))|=(1L<<l_i);
              } else {
                (*(l_param->pFlag))|=(1L<<l_i); /* OR */
                (*(l_param->pFlag))^=(1L<<l_i); /* XOR */
              }
              l_ftl=l_ftl->fNext;
            } else {
              break;
            }
          }
          EndDialog(p_hWnd,IDOK);
          return TRUE;
				case IDHELP:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_EF_DIALOG_EDITFLAG_HELP);
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
      RemoveProp(p_hWnd,(LPCSTR)g_edLongHi);
#ifndef WIN32
      RemoveProp(p_hWnd,(LPCSTR)g_edLongLo);
#endif
      break;
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
}


