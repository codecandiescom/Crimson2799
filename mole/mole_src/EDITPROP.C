// editprop.h
// two-character identifier: ep
// Property editing tool functions.
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
#include"editprop.h"
#include"help.h"
#include"ctl3dl.h"

/* return codes from window handler */
#define EP_RETURN_CANCEL  (0)
#define EP_RETURN_OK      (1<<0)
#define EP_RETURN_CHANGED (1<<1)

/* edit's the p_ptr property. If p_sType is DS_STYPE_NONE, creates a NEW struct
 * of type p_newType and attaches it to the REFERENCE p_ptr. */
BOOL epEditProperty(HWND p_hWnd,void *p_ptr,int p_sType,int p_newType) {
  EPPARAM l_param;
  int l_rc;

  if ((p_sType!=DS_STYPE_PROPERTY)&&(p_sType!=DS_STYPE_EXTRA)&&(p_sType!=DS_STYPE_NONE))
    return FALSE;
  if (p_sType==DS_STYPE_NONE)
    if ((p_newType!=DS_STYPE_PROPERTY)&&(p_newType!=DS_STYPE_EXTRA))
      return FALSE;

  if (p_sType==DS_STYPE_NONE) {
    l_param.pProperty=dsStructAlloc(p_newType);
  } else { /* extra / property */
    l_param.pProperty=DSStruct(p_ptr);
  }
  if (!l_param.pProperty)
    return FALSE;
  if (DSStruct(l_param.pProperty)->sType==DS_STYPE_EXTRA)
    l_param.pWindowTitle="Edit Extra";
  else
    l_param.pWindowTitle="Edit Property";

  /* edit property */
  l_rc=DialogBoxParam(g_aahInst,MAKEINTRESOURCE(DIALOG_EDITPROP),p_hWnd,
    epEditPropProc,(LPARAM)(&l_param));

  /* cleanup */
  if (p_sType==DS_STYPE_NONE) {
    if (l_rc&EP_RETURN_OK) { /* insert property into our reference */
      dsStructInsert(p_ptr,NULL,l_param.pProperty);
      return TRUE;
    } else { /* free property */
      dsStructFree(l_param.pProperty);
      return FALSE;
    }
  }
  if (l_rc&EP_RETURN_CHANGED)
    return TRUE;
  return FALSE;
}

BOOL CALLBACK _export epEditPropProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
  EPPARAM *l_param;
  BOOL l_changed;

  switch (p_message)
		{
    case WM_INITDIALOG:
      Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCentreDialogBox(p_hWnd);
      l_param=(EPPARAM *)p_lParam;
      if (!l_param)
        EndDialog(p_hWnd,IDCANCEL);
#ifdef WIN32
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(l_param));
#else
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,HIWORD(l_param));
      SetProp(p_hWnd,(LPCSTR)g_edLongLo,LOWORD(l_param));
#endif
      edSetDlgItemData(p_hWnd,IDC_EDITPROPKEY,&(DSSProperty(l_param->pProperty)->pKey),
        EDSGD_STR,EDSGD_STR_KEEPNL);
      edSetDlgItemData(p_hWnd,IDC_EDITPROPDESC,&(DSSProperty(l_param->pProperty)->pDesc),
        EDSGD_STR,EDSGD_STR_KEEPNL);
      SetWindowText(p_hWnd,l_param->pWindowTitle);
      return TRUE;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          EndDialog(p_hWnd,EP_RETURN_CANCEL);
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
          l_param=(EPPARAM *)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
          l_param=(EPPARAM *)MAKELP(GetProp(p_hWnd,(LPCSTR)g_edLongHi),GetProp(p_hWnd,(LPCSTR)g_edLongLo));
#endif
          l_changed=edGetDlgItemData(p_hWnd,IDC_EDITPROPKEY,&(DSSProperty(l_param->pProperty)->pKey),
            EDSGD_STR,EDSGD_STR_KEEPNL);
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITPROPDESC,&(DSSProperty(l_param->pProperty)->pDesc),
            EDSGD_STR,EDSGD_STR_KEEPNL);
          if (l_changed)
            EndDialog(p_hWnd,EP_RETURN_OK|EP_RETURN_CHANGED);
          else
            EndDialog(p_hWnd,EP_RETURN_OK);
          return TRUE;
				case IDHELP:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_EP_DIALOG_EDITPROP_HELP);
          return TRUE;
        case IDCANCEL:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          EndDialog(p_hWnd,EP_RETURN_CANCEL);
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

