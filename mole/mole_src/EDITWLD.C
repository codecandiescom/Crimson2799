// EditWLD module
// two-character identifier: ew
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
#include"edit.h"
#include"dialog.h"
#include"areawnd.h"
#include"editprop.h"
#include"editextr.h"
#include"editflag.h"
#include"editwld.h"
#include"help.h"
#include"ctl3dl.h"

BOOL CALLBACK _export ewEditWLDProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
  long l_VNum,l_i,l_j;
  DSSTRUCT *l_thing,*l_edit;
  char l_buf[40];
  BOOL l_changed;
  DSFTL *l_ftl;
  DSLWORD l_goto_world=-1;

  switch (p_message)
		{
		case WM_INITDIALOG:
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCascadeDialogBox(p_hWnd);
      SendDlgItemMessage(p_hWnd,IDC_EDITWLDTYPE,CB_SETEXTENDEDUI,TRUE,0L);
      /* Title the directions */
      for (l_ftl=dsFTLOf(&g_awFTList,MOLE_LIST_DIR),l_i=0;(l_ftl)&&(l_i<IDC_EDITWLDEXITNUMELEMNTS);l_i++,l_ftl=l_ftl->fNext) {
        SetDlgItemText(p_hWnd,(int)(IDC_EDITWLDD0+(l_i*IDC_EDITWLDEXITNUMELEMNTS)),l_ftl->fName);
        sprintf(l_buf,"%s Flags",l_ftl->fName);
        SetDlgItemText(p_hWnd,(int)(IDC_EDITWLDD0+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+5),l_buf);
      }
      /* and then fall through to the following ... */
    case WM_USER_INITDIALOGSUB:
      l_VNum=p_lParam;
      if (l_VNum>=0) {
        sprintf(l_buf,"%li",l_VNum);
        SetDlgItemText(p_hWnd,IDC_EDITWLDVIRTUAL,l_buf);
      }
#ifdef WIN32
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(l_VNum));
#else
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,HIWORD(l_VNum));
      SetProp(p_hWnd,(LPCSTR)g_edLongLo,LOWORD(l_VNum));
#endif
      RemoveProp(p_hWnd,(LPCSTR)g_edFlag);
      for (l_i=IDC_EDITWLDACTIVESTART;l_i<=IDC_EDITWLDACTIVEEND;l_i++) {
        EnableWindow(GetDlgItem(p_hWnd,(int)l_i),FALSE);
      }
      for (l_i=0;l_i<IDC_EDITWLDEXITNUMELEMNTS;l_i++)
        for (l_j=0;l_j<IDC_EDITWLDEXITNUMELEMNTS;l_j++)
          EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITWLDEXITSTART+IDC_EDITWLDEXITNUMELEMNTS*l_i+l_j)),FALSE);

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
        SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(l_VNum));
#else
        SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)HIWORD(l_VNum));
        SetProp(p_hWnd,(LPCSTR)g_edLongLo,(HANDLE)LOWORD(l_VNum));
#endif
        sprintf(l_buf,"%li",l_VNum);
        SetDlgItemText(p_hWnd,IDC_EDITWLDVIRTUAL,l_buf);
      }
      /* load data from structure into window fields */
      l_thing=edWorldOf(l_VNum);
      if (l_thing) {
        edSetDlgItemData(p_hWnd,IDC_EDITWLDNAME,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
        if (l_thing->sType==DS_STYPE_WORLD) {
          edSetDlgItemData(p_hWnd,IDC_EDITWLDDESC,&(DSSWorld(l_thing)->wDesc),EDSGD_STR,EDSGD_STR_FORMAT);
          edSetDlgItemData(p_hWnd,IDC_EDITWLDFLAGS,&(DSSWorld(l_thing)->wFlag),EDSGD_FLAG,0);
          edSetDlgItemData(p_hWnd,IDC_EDITWLDTYPE,&(DSSWorld(l_thing)->wType),EDSGD_LIST,MOLE_LIST_WTYPE);
          edSetDlgItemData(p_hWnd,IDC_EDITWLDEXITSTART,&(DSSWorld(l_thing)->wExit),EDSGD_EXIT,MOLE_LIST_DIR);
          edSetDlgItemData(p_hWnd,IDC_EDITWLDEXTRA,&(DSSWorld(l_thing)->wExtra),EDSGD_EXTRA,0);
          edSetDlgItemData(p_hWnd,IDC_EDITWLDPROPERTY,&(DSSWorld(l_thing)->wProperty),EDSGD_PROPERTY,0);

          for (l_i=IDC_EDITWLDACTIVESTART;l_i<=IDC_EDITWLDACTIVEEND;l_i++) {
            EnableWindow(GetDlgItem(p_hWnd,(int)l_i),TRUE);
          }
          EnableWindow(GetDlgItem(p_hWnd,IDOK),TRUE);
          SetFocus(GetDlgItem(p_hWnd,IDC_EDITWLDNAME));
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
          l_thing=edWorldOf(l_VNum);
          if (l_thing)
            l_thing->sEditWindow=NULL;
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
        case IDC_EDITWLDD0GOTO:
        case IDC_EDITWLDD1GOTO:
        case IDC_EDITWLDD2GOTO:
        case IDC_EDITWLDD3GOTO:
        case IDC_EDITWLDD4GOTO:
        case IDC_EDITWLDD5GOTO:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edWorldOf(l_VNum);
          if (!l_thing)
            break;
          l_i=(GET_WM_COMMAND_ID(p_wParam,p_lParam)-IDC_EDITWLDD0GOTO)/IDC_EDITWLDEXITNUMELEMNTS;
          edGetDlgItemData(p_hWnd,(int)(IDC_EDITWLDEXITSTART+(IDC_EDITWLDEXITNUMELEMNTS*l_i)+1),
            &l_goto_world,EDSGD_LWORD,0);
          if (!edWorldOf(l_goto_world))
            break; /* change this some day */

          /* and flow down into the following */
        case IDC_EDITWLDPREVIOUS:
        case IDC_EDITWLDNEXT:
        case IDOK:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edWorldOf(l_VNum);
          if (l_thing) {
            l_thing->sEditWindow=NULL;
            /* save data & check for changes to WLD */
            l_changed=edGetDlgItemData(p_hWnd,IDC_EDITWLDNAME,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITWLDDESC,&(DSSWorld(l_thing)->wDesc),EDSGD_STR,EDSGD_STR_FORMAT);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITWLDFLAGS,&(DSSWorld(l_thing)->wFlag),EDSGD_FLAG,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITWLDTYPE,&(DSSWorld(l_thing)->wType),EDSGD_LIST,MOLE_LIST_WTYPE);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITWLDEXITSTART,&(DSSWorld(l_thing)->wExit),EDSGD_EXIT,MOLE_LIST_DIR);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITWLDEXTRA,&(DSSWorld(l_thing)->wExtra),EDSGD_EXTRA,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITWLDPROPERTY,&(DSSWorld(l_thing)->wProperty),EDSGD_PROPERTY,0);
            /* save back to server if changes have been made */
            if ((((unsigned int)(GetProp(p_hWnd,(LPCSTR)g_edFlag)))&EDFLAG_CHANGED)||(l_changed))
              edSendItem(EDFI_WORLD,NULL,l_thing,NULL);

            /* now process button click */
            if (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITWLDPREVIOUS) {
              if (l_thing->sPrevious) {
                l_thing->sEditWindow=NULL;
                edEditItem(EDFI_WORLD,NULL,l_thing->sPrevious,p_hWnd);
              }
              return TRUE;
            } else if (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITWLDNEXT) {
              if (l_thing->sNext) {
                l_thing->sEditWindow=NULL;
                edEditItem(EDFI_WORLD,NULL,l_thing->sNext,p_hWnd);
              }
              return TRUE;
            } else if (l_goto_world >=0) {
              if (edWorldOf(l_goto_world)) {
                l_thing->sEditWindow=NULL;
                edEditItem(EDFI_WORLD,NULL,edWorldOf(l_goto_world),p_hWnd);
              }
              return TRUE;
            }
          }
          aaDestroyWindow(p_hWnd);
          return TRUE;
				case IDHELP:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_EW_DIALOG_EDITWLD_HELP);
          return TRUE;
        case IDCANCEL:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edWorldOf(l_VNum);
          if (l_thing)
            l_thing->sEditWindow=NULL;
          aaDestroyWindow(p_hWnd);
          return TRUE;
        case IDC_EDITWLDEXTRANEW:
          l_thing=edWorldOf(l_VNum);
          if ((l_thing)&&(l_thing->sType==DS_STYPE_WORLD)) {
            l_changed=eeEditExtra(p_hWnd,&(DSSWorld(l_thing)->wExtra),
              DS_STYPE_NONE,DS_STYPE_EXTRA);
            if (l_changed) {
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,
                (LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              edSetDlgItemData(p_hWnd,IDC_EDITWLDEXTRA,
                &(DSSWorld(l_thing)->wExtra),EDSGD_EXTRA,0);
            }
            return TRUE;
          }
          break;
        case IDC_EDITWLDEXTRA:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK)
            break;
          /* else fall through to the following */
        case IDC_EDITWLDEXTRAEDIT:
        case IDC_EDITWLDEXTRADELETE:
          if ((GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)&&
            (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK))
            break;
          l_i=SendDlgItemMessage(p_hWnd,IDC_EDITWLDEXTRA,LB_GETCURSEL,0,0L);
          if (l_i!=LB_ERR)
            l_i=SendDlgItemMessage(p_hWnd,IDC_EDITWLDEXTRA,LB_GETITEMDATA,(WPARAM)l_i,0L);
          if (l_i!=LB_ERR) {
            l_thing=edWorldOf(l_VNum);
            if ((l_thing)&&((l_thing->sType)==DS_STYPE_WORLD)) {
              for (l_edit=(DSSWorld(l_thing)->wExtra.rList);(l_edit)&&(l_i);
                l_edit=l_edit->sNext) {
                l_i--;
              }
              if (l_edit) {
                l_thing=DSStruct(l_edit->sUpRef);
                if ((GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITWLDEXTRAEDIT)||
                    (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITWLDEXTRA))  {
                  /* l_edit now points to our property! */
                  if (eeEditExtra(p_hWnd,l_edit,DS_STYPE_EXTRA,DS_STYPE_EXTRA))
                    SetProp(p_hWnd,(LPCSTR)g_edFlag,
                      (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                } else { /* delete */
                  dsStructFree(l_edit);
                  SetProp(p_hWnd,(LPCSTR)g_edFlag,
                    (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                }
                edSetDlgItemData(p_hWnd,IDC_EDITWLDEXTRA,l_thing,EDSGD_EXTRA,0);
                return TRUE;
              }
            }
          }
          break;
        case IDC_EDITWLDPROPERTYNEW:
          l_thing=edWorldOf(l_VNum);
          if ((l_thing)&&(l_thing->sType==DS_STYPE_WORLD)) {
            l_changed=epEditProperty(p_hWnd,&(DSSWorld(l_thing)->wProperty),
              DS_STYPE_NONE,DS_STYPE_PROPERTY);
            if (l_changed) {
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              edSetDlgItemData(p_hWnd,IDC_EDITWLDPROPERTY,
                &(DSSWorld(l_thing)->wProperty),EDSGD_PROPERTY,0);
            }
            return TRUE;
          }
          break;
        case IDC_EDITWLDPROPERTY:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK)
            break;
          /* else fall through to the following */
        case IDC_EDITWLDPROPERTYEDIT:
        case IDC_EDITWLDPROPERTYDELETE:
          if ((GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)&&
            (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK))
            break;
          l_i=SendDlgItemMessage(p_hWnd,IDC_EDITWLDPROPERTY,LB_GETCURSEL,0,0L);
          if (l_i!=LB_ERR)
            l_i=SendDlgItemMessage(p_hWnd,IDC_EDITWLDPROPERTY,LB_GETITEMDATA,(WPARAM)l_i,0L);
          if (l_i!=LB_ERR) {
            l_thing=edWorldOf(l_VNum);
            if ((l_thing)&&((l_thing->sType)==DS_STYPE_WORLD)) {
              for (l_edit=(DSSWorld(l_thing)->wProperty.rList);(l_edit)&&(l_i);
                l_edit=l_edit->sNext) {
                l_i--;
              }
              if (l_edit) {
                l_thing=DSStruct(l_edit->sUpRef);
                if ((GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITWLDPROPERTYEDIT)||
                    (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITWLDPROPERTY)) {
                  /* l_edit now points to our property! */
                  if (epEditProperty(p_hWnd,l_edit,DS_STYPE_PROPERTY,DS_STYPE_PROPERTY))
                    SetProp(p_hWnd,(LPCSTR)g_edFlag,
                      (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                } else { /* delete */
                  SetProp(p_hWnd,(LPCSTR)g_edFlag,
                    (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                  dsStructFree(l_edit);
                }
                edSetDlgItemData(p_hWnd,IDC_EDITWLDPROPERTY,l_thing,EDSGD_PROPERTY,0);
                return TRUE;
              }
            }
          }
          break;
        case IDC_EDITWLDFLAGS:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edWorldOf(l_VNum);
          if (!l_thing)
            break;
          if (efEditFlag(p_hWnd,&(DSSWorld(l_thing)->wFlag),MOLE_LIST_WFLAG,"World Flags"))
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
          return TRUE;
        case IDC_EDITWLDD0:
        case IDC_EDITWLDD1:
        case IDC_EDITWLDD2:
        case IDC_EDITWLDD3:
        case IDC_EDITWLDD4:
        case IDC_EDITWLDD5:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          /* find direction */
          l_i=GET_WM_COMMAND_ID(p_wParam,p_lParam);
          /* enable/disable this string of windows */
          l_changed=(BOOL)IsDlgButtonChecked(p_hWnd,(int)(l_i));
          if (l_changed>=0)
            for (l_j=1;l_j<IDC_EDITWLDEXITNUMELEMNTS;l_j++)
              EnableWindow(GetDlgItem(p_hWnd,(int)(l_i+l_j)),l_changed);
          break;
        case IDC_EDITWLDD0FLAG:
        case IDC_EDITWLDD1FLAG:
        case IDC_EDITWLDD2FLAG:
        case IDC_EDITWLDD3FLAG:
        case IDC_EDITWLDD4FLAG:
        case IDC_EDITWLDD5FLAG:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edWorldOf(l_VNum);
          if (!l_thing)
            break;
          /* find direction */
          l_i=(GET_WM_COMMAND_ID(p_wParam,p_lParam)-IDC_EDITWLDD0FLAG)/(IDC_EDITWLDD1FLAG-IDC_EDITWLDD0FLAG);
          for(l_edit=DSSWorld(l_thing)->wExit.rList;l_edit;l_edit=l_edit->sNext)
            if (DSSExit(l_edit)->eDir==l_i)
              break;
          if (!l_edit) {
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,
              (LPCSTR)g_edFlag))|EDFLAG_CHANGED));
            l_edit=dsStructAlloc(DS_STYPE_EXIT);
            if (!l_edit)
              break;
            DSSExit(l_edit)->eDir=(DSBYTE)(l_i);
            dsStructInsert(&(DSSWorld(l_thing)->wExit), NULL, l_edit);
          }
          /* find FTL for this direction so that we can print our direction */
          for (l_ftl=dsFTLOf(&g_awFTList,MOLE_LIST_DIR),l_j=0;l_ftl&&(l_j<l_i);l_ftl=l_ftl->fNext,l_j++);
          if (l_ftl)
            sprintf(l_buf,"%s Flags",l_ftl->fName);
          else
            sprintf(l_buf,"Exit Flags");
          if (efEditFlag(p_hWnd,&(DSSExit(l_edit)->eFlag),MOLE_LIST_EFLAG,l_buf))
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
          break;
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

