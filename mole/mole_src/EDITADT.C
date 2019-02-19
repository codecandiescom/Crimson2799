// EditADt module
// two-character identifier: ea
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
#include"moledefs.h"
#include"molerc.h"
#include"molem.h"
#include"dstruct.h"
#include"areawnd.h"
#include"edit.h"
#include"editadt.h"
#include"dialog.h"
#include"editprop.h"
#include"editextr.h"
#include"editflag.h"
#include"help.h"
#include"ctl3dl.h"

BOOL CALLBACK _export eaEditAreaDetailProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
  long l_VNum,l_i;
  DSSTRUCT *l_thing,*l_edit;
  BOOL l_changed;

  switch (p_message)
		{
    case WM_INITDIALOG:
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
			dlCascadeDialogBox(p_hWnd);
			/* and then fall through to the following ... */
		case WM_USER_INITDIALOGSUB:
			l_VNum=p_lParam;
#ifdef WIN32
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)l_VNum);
#else
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,HIWORD(l_VNum));
      SetProp(p_hWnd,(LPCSTR)g_edLongLo,LOWORD(l_VNum));
#endif
      RemoveProp(p_hWnd,(LPCSTR)g_edFlag);
      for (l_i=IDC_EDITADTACTIVESTART;l_i<=IDC_EDITADTACTIVEEND;l_i++) {
        EnableWindow(GetDlgItem(p_hWnd,(int)l_i),FALSE);
      }
      EnableWindow(GetDlgItem(p_hWnd,IDOK),FALSE);
      return TRUE;
    case WM_USER_NEWDATA:
#ifdef WIN32
      l_VNum=(LONG)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
      l_VNum=(LONG)MAKELONG(GetProp(p_hWnd,(LPCSTR)g_edLongLo),
        GetProp(p_hWnd,(LPCSTR)g_edLongHi));
#endif
      if (l_VNum<0) {
        l_VNum=p_lParam;
#ifdef WIN32
        SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)l_VNum);
#else
        SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)HIWORD(l_VNum));
        SetProp(p_hWnd,(LPCSTR)g_edLongLo,(HANDLE)LOWORD(l_VNum));
#endif
      }
      /* load data from structure into window fields */
      l_thing=edAreaOf(l_VNum);
      if (l_thing) {
        edSetDlgItemData(p_hWnd,IDC_EDITADTNAME,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
      }
      l_thing=edAreaDetailOf(l_VNum);
      if (l_thing) {
        if (l_thing->sType==DS_STYPE_AREADETAIL) {
          edSetDlgItemData(p_hWnd,IDC_EDITADTDESC,&(DSSAreaDetail(l_thing)->aDesc),EDSGD_STR,EDSGD_STR_FORMAT);
          edSetDlgItemData(p_hWnd,IDC_EDITADTEDITORS,&(DSSAreaDetail(l_thing)->aEditor),EDSGD_STR,EDSGD_STR_FORMAT);
          edSetDlgItemData(p_hWnd,IDC_EDITADTRESETDELAY,&(DSSAreaDetail(l_thing)->aResetDelay),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITADTFLAGS,&(DSSAreaDetail(l_thing)->aResetFlag),EDSGD_FLAG,0);
          edSetDlgItemData(p_hWnd,IDC_EDITADTPROPERTY,&(DSSAreaDetail(l_thing)->aProperty),EDSGD_PROPERTY,0);
          for (l_i=IDC_EDITADTACTIVESTART;l_i<=IDC_EDITADTACTIVEEND;l_i++) {
            EnableWindow(GetDlgItem(p_hWnd,(int)l_i),TRUE);
          }
          EnableWindow(GetDlgItem(p_hWnd,IDOK),TRUE);
          SetFocus(GetDlgItem(p_hWnd,IDC_EDITADTNAME));
        }
      }
      return TRUE;
    case WM_SYSCOMMAND:
#ifdef WIN32
      l_VNum=(LONG)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
      l_VNum=(LONG)MAKELONG(GetProp(p_hWnd,(LPCSTR)g_edLongLo),
        GetProp(p_hWnd,(LPCSTR)g_edLongHi));
#endif
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          l_thing=edAreaOf(l_VNum);
          if (l_thing)
            DSSArea(l_thing)->aDetail.rEditWindow=NULL;
          aaDestroyWindow(p_hWnd);
					return TRUE;
				default:
					break;
				}
      break;
    case WM_COMMAND:
#ifdef WIN32
      l_VNum=(LONG)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
      l_VNum=(LONG)MAKELONG(GetProp(p_hWnd,(LPCSTR)g_edLongLo),
        GetProp(p_hWnd,(LPCSTR)g_edLongHi));
#endif
      switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case IDOK:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          /* Currently can't change the name of an area */
//          l_thing=edAreaOf(l_VNum);
//          if (l_thing) {
//            l_changed=edGetDlgItemData(p_hWnd,IDC_EDITADTNAME,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
          l_changed=FALSE;
//          }
          l_thing=edAreaOf(l_VNum);
          if (l_thing)
            DSSArea(l_thing)->aDetail.rEditWindow=NULL;
          l_thing=edAreaDetailOf(l_VNum);
          if (l_thing) {
            /* save data & check for changes to ADT */
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITADTDESC,&(DSSAreaDetail(l_thing)->aDesc),EDSGD_STR,EDSGD_STR_FORMAT);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITADTEDITORS,&(DSSAreaDetail(l_thing)->aEditor),EDSGD_STR,EDSGD_STR_FORMAT);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITADTRESETDELAY,&(DSSAreaDetail(l_thing)->aResetDelay),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITADTFLAGS,&(DSSAreaDetail(l_thing)->aResetFlag),EDSGD_FLAG,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITADTPROPERTY,&(DSSAreaDetail(l_thing)->aProperty),EDSGD_PROPERTY,0);

            /* save back to server if changes have been made */
            if ((((unsigned int)(GetProp(p_hWnd,(LPCSTR)g_edFlag)))&EDFLAG_CHANGED)||(l_changed)) {
              edSendItem(EDFI_AREADETAIL,edAreaOf(l_VNum),NULL,NULL);
              /* exception: also update areawindow; they may have changed the name
                 of this area! */
              PostMessage(g_awRootWnd,WM_USER_AREALIST_CHNG,0,0L);
            }

            /* now process button click */
          }
          aaDestroyWindow(p_hWnd);
          return TRUE;
				case IDHELP:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_EA_DIALOG_EDITADT_HELP);
          return TRUE;
        case IDCANCEL:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edAreaOf(l_VNum);
          if (l_thing)
            DSSArea(l_thing)->aDetail.rEditWindow=NULL;
          aaDestroyWindow(p_hWnd);
          return TRUE;
        case IDC_EDITADTPROPERTYNEW:
          l_thing=edAreaDetailOf(l_VNum);
          if ((l_thing)&&(l_thing->sType==DS_STYPE_AREADETAIL)) {
            l_changed=epEditProperty(p_hWnd,&(DSSAreaDetail(l_thing)->aProperty),
              DS_STYPE_NONE,DS_STYPE_PROPERTY);
            if (l_changed) {
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              edSetDlgItemData(p_hWnd,IDC_EDITADTPROPERTY,
                &(DSSAreaDetail(l_thing)->aProperty),EDSGD_PROPERTY,0);
            }
            return TRUE;
          }
          break;
        case IDC_EDITADTPROPERTY:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK)
            break;
          /* else fall through to the following */
        case IDC_EDITADTPROPERTYEDIT:
        case IDC_EDITADTPROPERTYDELETE:
          if ((GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)&&
            (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK))
            break;
          l_i=SendDlgItemMessage(p_hWnd,IDC_EDITADTPROPERTY,LB_GETCURSEL,0,0L);
          if (l_i!=LB_ERR)
            l_i=SendDlgItemMessage(p_hWnd,IDC_EDITADTPROPERTY,LB_GETITEMDATA,(WPARAM)l_i,0L);
          if (l_i!=LB_ERR) {
            l_thing=edAreaDetailOf(l_VNum);
            if ((l_thing)&&((l_thing->sType)==DS_STYPE_AREADETAIL)) {
              for (l_edit=(DSSAreaDetail(l_thing)->aProperty.rList);(l_edit)&&(l_i);
                l_edit=l_edit->sNext) {
                l_i--;
              }
              if (l_edit) {
                l_thing=DSStruct(l_edit->sUpRef);
                if ((GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITADTPROPERTYEDIT)||
                    (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITADTPROPERTY)) {
                  /* l_edit now points to our property! */
                  if (epEditProperty(p_hWnd,l_edit,DS_STYPE_PROPERTY,DS_STYPE_PROPERTY))
                    SetProp(p_hWnd,(LPCSTR)g_edFlag,
                      (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                } else { /* delete */
                  SetProp(p_hWnd,(LPCSTR)g_edFlag,
                    (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                  dsStructFree(l_edit);
                }
                edSetDlgItemData(p_hWnd,IDC_EDITADTPROPERTY,l_thing,EDSGD_PROPERTY,0);
                return TRUE;
              }
            }
          }
          break;
        case IDC_EDITADTFLAGS:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edAreaDetailOf(l_VNum);
          if (!l_thing)
            break;
          if (efEditFlag(p_hWnd,&(DSSAreaDetail(l_thing)->aResetFlag),MOLE_LIST_RFLAG,"Area Flags"))
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
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
      RemoveProp(p_hWnd,(LPCSTR)g_edFlag);
      break;
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
}

