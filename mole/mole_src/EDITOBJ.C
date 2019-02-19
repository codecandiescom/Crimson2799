// EditOBJ module
// two-character identifier: eo
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
#include"editprop.h"
#include"editextr.h"
#include"editflag.h"
#include"editobj.h"
#include"areawnd.h"
#include"help.h"
#include"ctl3dl.h"

BOOL CALLBACK _export eoEditOBJProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
  long l_VNum,l_i,l_j;
  DSSTRUCT *l_thing,*l_edit;
  char l_buf[40];
  BOOL l_changed;
  DSFTL *l_ftl;
  DSBYTE l_byte;

  switch (p_message)
		{
    case WM_INITDIALOG:
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCascadeDialogBox(p_hWnd);
      SendDlgItemMessage(p_hWnd,IDC_EDITOBJTYPE,CB_SETEXTENDEDUI,TRUE,0L);
      SendDlgItemMessage(p_hWnd,IDC_EDITOBJWEAR,CB_SETEXTENDEDUI,TRUE,0L);
      SendDlgItemMessage(p_hWnd,IDC_EDITOBJAPPLYTYPE0,CB_SETEXTENDEDUI,TRUE,0L);
      SendDlgItemMessage(p_hWnd,IDC_EDITOBJAPPLYTYPE1,CB_SETEXTENDEDUI,TRUE,0L);
      SendDlgItemMessage(p_hWnd,IDC_EDITOBJAPPLYTYPE2,CB_SETEXTENDEDUI,TRUE,0L);
      SendDlgItemMessage(p_hWnd,IDC_EDITOBJAPPLYTYPE3,CB_SETEXTENDEDUI,TRUE,0L);
      for (l_i=0;l_i<16;l_i++) {
        SendDlgItemMessage(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4)),CB_SETEXTENDEDUI,TRUE,0L);
      }
      /* and then fall through to the following ... */
    case WM_USER_INITDIALOGSUB:
      l_VNum=p_lParam;
      if (l_VNum>=0) {
        sprintf(l_buf,"%li",l_VNum);
        SetDlgItemText(p_hWnd,IDC_EDITOBJVIRTUAL,l_buf);
      }
#ifdef WIN32
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(l_VNum));
#else
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,HIWORD(l_VNum));
      SetProp(p_hWnd,(LPCSTR)g_edLongLo,LOWORD(l_VNum));
#endif
      RemoveProp(p_hWnd,(LPCSTR)g_edFlag);
      for (l_i=IDC_EDITOBJACTIVESTART;l_i<=IDC_EDITOBJACTIVEEND;l_i++) {
        EnableWindow(GetDlgItem(p_hWnd,(int)l_i),FALSE);
      }
      /* hide all our detail fields */
      for (l_i=0;l_i<16;l_i++) {
        for (l_j=0;l_j<4;l_j++)
          ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDL0+(l_i*4)+l_j)),SW_HIDE);
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
        SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(l_VNum));
#else
        SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)HIWORD(l_VNum));
        SetProp(p_hWnd,(LPCSTR)g_edLongLo,(HANDLE)LOWORD(l_VNum));
#endif
        sprintf(l_buf,"%li",l_VNum);
        SetDlgItemText(p_hWnd,IDC_EDITOBJVIRTUAL,l_buf);
      }
      /* load data from structure into window fields */
      l_thing=edObjectOf(l_VNum);
      if (l_thing) {
        edSetDlgItemData(p_hWnd,IDC_EDITOBJNAME,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
        if (l_thing->sType==DS_STYPE_OBJECT) {
          edSetDlgItemData(p_hWnd,IDC_EDITOBJKEY,&(DSSObject(l_thing)->oKey),EDSGD_STR,EDSGD_STR_STRIPNL);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJLDESC,&(DSSObject(l_thing)->oLDesc),EDSGD_STR,EDSGD_STR_STRIPNL);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJDESC,&(DSSObject(l_thing)->oDesc),EDSGD_STR,EDSGD_STR_FORMAT);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJTYPE,&(DSSObject(l_thing)->oType),EDSGD_LIST,MOLE_LIST_OTYPE);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJACTIONFLAGS,&(DSSObject(l_thing)->oAct),EDSGD_FLAG,0);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJWEAR,&(DSSObject(l_thing)->oWear),EDSGD_LIST,MOLE_LIST_WEAR);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJWEIGHT,&(DSSObject(l_thing)->oWeight),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJVALUE,&(DSSObject(l_thing)->oValue),EDSGD_LWORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJRENT,&(DSSObject(l_thing)->oRent),EDSGD_LWORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJEXTRA,&(DSSObject(l_thing)->oExtra),EDSGD_EXTRA,0);
          edSetDlgItemData(p_hWnd,IDC_EDITOBJPROPERTY,&(DSSObject(l_thing)->oProperty),EDSGD_PROPERTY,0);

          /* details */
          /* we set details later with a call to WM_USER_CHANGEDDATA */

          /* apply */
          for (l_i=0;l_i<DS_OBJECT_MAX_APPLY;l_i++) {
            edSetDlgItemData(p_hWnd,(int)(IDC_EDITOBJAPPLYTYPE0+(l_i*2)),
              &(DSSObject(l_thing)->oApplyType[(int)l_i]),EDSGD_LIST,MOLE_LIST_APPLY);
            edSetDlgItemData(p_hWnd,(int)(IDC_EDITOBJAPPLYTYPE0+(l_i*2)+1),
              &(DSSObject(l_thing)->oApplyValue[(int)l_i]),EDSGD_SBYTE,0);
          }

          for (l_i=IDC_EDITOBJACTIVESTART;l_i<=IDC_EDITOBJACTIVEEND;l_i++) {
            EnableWindow(GetDlgItem(p_hWnd,(int)l_i),TRUE);
          }
          EnableWindow(GetDlgItem(p_hWnd,IDOK),TRUE);
          SetFocus(GetDlgItem(p_hWnd,IDC_EDITOBJNAME));
        }
      }
      SendMessage(p_hWnd,WM_USER_CHANGEDDATA,0,0L);
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
          l_thing=edObjectOf(l_VNum);
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
        case IDC_EDITOBJPREVIOUS:
        case IDC_EDITOBJNEXT:
        case IDOK:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edObjectOf(l_VNum);
          if (l_thing) {
            l_thing->sEditWindow=NULL;
            /* save data & check for changes to OBJ */
            l_changed=edGetDlgItemData(p_hWnd,IDC_EDITOBJNAME,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJKEY,&(DSSObject(l_thing)->oKey),EDSGD_STR,EDSGD_STR_STRIPNL);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJLDESC,&(DSSObject(l_thing)->oLDesc),EDSGD_STR,EDSGD_STR_STRIPNL);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJDESC,&(DSSObject(l_thing)->oDesc),EDSGD_STR,EDSGD_STR_FORMAT);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJTYPE,&(DSSObject(l_thing)->oType),EDSGD_LIST,MOLE_LIST_OTYPE);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJACTIONFLAGS,&(DSSObject(l_thing)->oAct),EDSGD_FLAG,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJWEAR,&(DSSObject(l_thing)->oWear),EDSGD_LIST,MOLE_LIST_WEAR);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJWEIGHT,&(DSSObject(l_thing)->oWeight),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJVALUE,&(DSSObject(l_thing)->oValue),EDSGD_LWORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJRENT,&(DSSObject(l_thing)->oRent),EDSGD_LWORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJEXTRA,&(DSSObject(l_thing)->oExtra),EDSGD_EXTRA,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITOBJPROPERTY,&(DSSObject(l_thing)->oProperty),EDSGD_PROPERTY,0);

            /* details */
            l_ftl=dsFTLOf(&g_awFTLObjType,DSSObject(l_thing)->oType);
            /* skip past initial (blank) marker FTL */
            if (l_ftl)
              l_ftl=l_ftl->fNext;
            for (l_i=0;(l_i<16)&&(l_ftl);l_i++,l_ftl=l_ftl->fNext) {
              switch (l_ftl->fType) {
                case MOLE_LISTTYPE_TYPE:
                  /* type combo box */
                  /* get the value */
                  l_byte=(DSBYTE)(DSSObject(l_thing)->oDetail[(int)l_i]);
                  l_changed|=edGetDlgItemData(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4)),
                    &l_byte,EDSGD_LIST,l_ftl->fList);
                  DSSObject(l_thing)->oDetail[(int)l_i]=l_byte;
                  break;
                case MOLE_LISTTYPE_FLAG:
                  /* flag button */
                  /* get the value */
                  l_changed|=edGetDlgItemData(p_hWnd,(int)(IDC_EDITOBJDF0+(l_i*4)),
                    &(DSSObject(l_thing)->oDetail[(int)l_i]),EDSGD_FLAG,
                    l_ftl->fList);
                  break;
                case MOLE_LISTTYPE_INVALID:
                  /* edit window */
                  l_changed|=edGetDlgItemData(p_hWnd,(int)(IDC_EDITOBJDV0+(l_i*4)),
                    &(DSSObject(l_thing)->oDetail[(int)l_i]),EDSGD_LWORD,0);
                  break;
              }
            }

            /* apply */
            for (l_i=0;l_i<DS_OBJECT_MAX_APPLY;l_i++) {
              l_changed|=edGetDlgItemData(p_hWnd,(int)(IDC_EDITOBJAPPLYTYPE0+(l_i*2)),
                &(DSSObject(l_thing)->oApplyType[(int)l_i]),EDSGD_LIST,MOLE_LIST_APPLY);
              l_changed|=edGetDlgItemData(p_hWnd,(int)(IDC_EDITOBJAPPLYTYPE0+(l_i*2)+1),
                &(DSSObject(l_thing)->oApplyValue[(int)l_i]),EDSGD_SBYTE,0);
            }
            /* save back to server if changes have been made */
            if ((((unsigned int)(GetProp(p_hWnd,(LPCSTR)g_edFlag)))&EDFLAG_CHANGED)||(l_changed))
              edSendItem(EDFI_OBJECT,NULL,l_thing,NULL);

            /* now process button click */
            if (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITOBJPREVIOUS) {
              if (l_thing->sPrevious) {
                l_thing->sEditWindow=NULL;
                edEditItem(EDFI_OBJECT,NULL,l_thing->sPrevious,p_hWnd);
              }
              return TRUE;
            } else if (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITOBJNEXT) {
              if (l_thing->sNext) {
                l_thing->sEditWindow=NULL;
                edEditItem(EDFI_OBJECT,NULL,l_thing->sNext,p_hWnd);
              }
              return TRUE;
            }
          }
          aaDestroyWindow(p_hWnd);
          return TRUE;
				case IDHELP:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_EO_DIALOG_EDITOBJ_HELP);
          return TRUE;
        case IDCANCEL:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edObjectOf(l_VNum);
          if (l_thing)
            l_thing->sEditWindow=NULL;
          aaDestroyWindow(p_hWnd);
          return TRUE;
        case IDC_EDITOBJEXTRANEW:
          l_thing=edObjectOf(l_VNum);
          if ((l_thing)&&(l_thing->sType==DS_STYPE_OBJECT)) {
            l_changed=eeEditExtra(p_hWnd,&(DSSObject(l_thing)->oExtra),
              DS_STYPE_NONE,DS_STYPE_EXTRA);
            if (l_changed) {
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,
                (LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              edSetDlgItemData(p_hWnd,IDC_EDITOBJEXTRA,
                &(DSSObject(l_thing)->oExtra),EDSGD_EXTRA,0);
            }
            return TRUE;
          }
          break;
        case IDC_EDITOBJEXTRA:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK)
            break;
          /* else fall through to the following */
        case IDC_EDITOBJEXTRAEDIT:
        case IDC_EDITOBJEXTRADELETE:
          if ((GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)&&
            (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK))
            break;
          l_i=SendDlgItemMessage(p_hWnd,IDC_EDITOBJEXTRA,LB_GETCURSEL,0,0L);
          if (l_i!=LB_ERR)
            l_i=SendDlgItemMessage(p_hWnd,IDC_EDITOBJEXTRA,LB_GETITEMDATA,(WPARAM)l_i,0L);
          if (l_i!=LB_ERR) {
            l_thing=edObjectOf(l_VNum);
            if ((l_thing)&&((l_thing->sType)==DS_STYPE_OBJECT)) {
              for (l_edit=(DSSObject(l_thing)->oExtra.rList);(l_edit)&&(l_i);
                l_edit=l_edit->sNext) {
                l_i--;
              }
              if (l_edit) {
                l_thing=DSStruct(l_edit->sUpRef);
                if ((GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITOBJEXTRAEDIT)||
                    (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITOBJEXTRA))  {
                  /* l_edit now points to our property! */
                  if (eeEditExtra(p_hWnd,l_edit,DS_STYPE_EXTRA,DS_STYPE_EXTRA))
                    SetProp(p_hWnd,(LPCSTR)g_edFlag,
                      (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                } else { /* delete */
                  dsStructFree(l_edit);
                  SetProp(p_hWnd,(LPCSTR)g_edFlag,
                    (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                }
                edSetDlgItemData(p_hWnd,IDC_EDITOBJEXTRA,l_thing,EDSGD_EXTRA,0);
                return TRUE;
              }
            }
          }
          break;
        case IDC_EDITOBJPROPERTYNEW:
          l_thing=edObjectOf(l_VNum);
          if ((l_thing)&&(l_thing->sType==DS_STYPE_OBJECT)) {
            l_changed=epEditProperty(p_hWnd,&(DSSObject(l_thing)->oProperty),
              DS_STYPE_NONE,DS_STYPE_PROPERTY);
            if (l_changed) {
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              edSetDlgItemData(p_hWnd,IDC_EDITOBJPROPERTY,
                &(DSSObject(l_thing)->oProperty),EDSGD_PROPERTY,0);
            }
            return TRUE;
          }
          break;
        case IDC_EDITOBJPROPERTY:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK)
            break;
          /* else fall through to the following */
        case IDC_EDITOBJPROPERTYEDIT:
        case IDC_EDITOBJPROPERTYDELETE:
          if ((GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)&&
            (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK))
            break;
          l_i=SendDlgItemMessage(p_hWnd,IDC_EDITOBJPROPERTY,LB_GETCURSEL,0,0L);
          if (l_i!=LB_ERR)
            l_i=SendDlgItemMessage(p_hWnd,IDC_EDITOBJPROPERTY,LB_GETITEMDATA,(WPARAM)l_i,0L);
          if (l_i!=LB_ERR) {
            l_thing=edObjectOf(l_VNum);
            if ((l_thing)&&((l_thing->sType)==DS_STYPE_OBJECT)) {
              for (l_edit=(DSSObject(l_thing)->oProperty.rList);(l_edit)&&(l_i);
                l_edit=l_edit->sNext) {
                l_i--;
              }
              if (l_edit) {
                l_thing=DSStruct(l_edit->sUpRef);
                if ((GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITOBJPROPERTYEDIT)||
                    (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITOBJPROPERTY)) {
                  /* l_edit now points to our property! */
                  if (epEditProperty(p_hWnd,l_edit,DS_STYPE_PROPERTY,DS_STYPE_PROPERTY))
                    SetProp(p_hWnd,(LPCSTR)g_edFlag,
                      (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                } else { /* delete */
                  SetProp(p_hWnd,(LPCSTR)g_edFlag,
                    (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                  dsStructFree(l_edit);
                }
                edSetDlgItemData(p_hWnd,IDC_EDITOBJPROPERTY,l_thing,EDSGD_PROPERTY,0);
                return TRUE;
              }
            }
          }
          break;
        case IDC_EDITOBJACTIONFLAGS:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edObjectOf(l_VNum);
          if (!l_thing)
            break;
          if (efEditFlag(p_hWnd,&(DSSObject(l_thing)->oAct),MOLE_LIST_OACT,"Object Action Flags"))
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
          return TRUE;
        case IDC_EDITOBJDF0:
        case IDC_EDITOBJDF1:
        case IDC_EDITOBJDF2:
        case IDC_EDITOBJDF3:
        case IDC_EDITOBJDF4:
        case IDC_EDITOBJDF5:
        case IDC_EDITOBJDF6:
        case IDC_EDITOBJDF7:
        case IDC_EDITOBJDF8:
        case IDC_EDITOBJDF9:
        case IDC_EDITOBJDF10:
        case IDC_EDITOBJDF11:
        case IDC_EDITOBJDF12:
        case IDC_EDITOBJDF13:
        case IDC_EDITOBJDF14:
        case IDC_EDITOBJDF15:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)==BN_CLICKED) {
            /* get edit item number */
            l_i=(GET_WM_COMMAND_ID(p_wParam,p_lParam)-IDC_EDITOBJDF0)/4;

            l_thing=edObjectOf(l_VNum);
            if (!l_thing)
              break;

            edGetDlgItemData(p_hWnd,IDC_EDITOBJTYPE,&l_byte,EDSGD_LIST,MOLE_LIST_OTYPE);
            l_ftl=dsFTLOf(&g_awFTLObjType,l_byte);
            /* skip past initial (blank) marker FTL */
            if (l_ftl)
              l_ftl=l_ftl->fNext;
            for (l_j=0;(l_j<l_i)&&(l_ftl);l_j++,l_ftl=l_ftl->fNext);

            if (!l_ftl)
              break;
            if (l_ftl->fType!=MOLE_LISTTYPE_FLAG)
              break;
            if (efEditFlag(p_hWnd,(DSFLAG *)(&(DSSObject(l_thing)->oDetail[(int)l_i])),
              l_ftl->fList,l_ftl->fName))
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
            return TRUE;
          }
          break;
        case IDC_EDITOBJDV0:
        case IDC_EDITOBJDV1:
        case IDC_EDITOBJDV2:
        case IDC_EDITOBJDV3:
        case IDC_EDITOBJDV4:
        case IDC_EDITOBJDV5:
        case IDC_EDITOBJDV6:
        case IDC_EDITOBJDV7:
        case IDC_EDITOBJDV8:
        case IDC_EDITOBJDV9:
        case IDC_EDITOBJDV10:
        case IDC_EDITOBJDV11:
        case IDC_EDITOBJDV12:
        case IDC_EDITOBJDV13:
        case IDC_EDITOBJDV14:
        case IDC_EDITOBJDV15:
          break;
        case IDC_EDITOBJDT0:
        case IDC_EDITOBJDT1:
        case IDC_EDITOBJDT2:
        case IDC_EDITOBJDT3:
        case IDC_EDITOBJDT4:
        case IDC_EDITOBJDT5:
        case IDC_EDITOBJDT6:
        case IDC_EDITOBJDT7:
        case IDC_EDITOBJDT8:
        case IDC_EDITOBJDT9:
        case IDC_EDITOBJDT10:
        case IDC_EDITOBJDT11:
        case IDC_EDITOBJDT12:
        case IDC_EDITOBJDT13:
        case IDC_EDITOBJDT14:
        case IDC_EDITOBJDT15:
          #define EO_TYPE_WINDOW_GROWTH 120
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)==CBN_SETFOCUS) {
            RECT l_rect;
            POINT l_point;
            HWND l_hWnd;

            /* get window */
            l_i=(GET_WM_COMMAND_ID(p_wParam,p_lParam)-IDC_EDITOBJDT0)/4;
            l_hWnd=GET_WM_COMMAND_HWND(p_wParam,p_lParam);

            /* Hide label so we don't overlap */
            ShowWindow(GetDlgItem(p_hWnd,
              (int)(IDC_EDITOBJDL0+(l_i*4))),
              SW_HIDE);

            /* enlarge window */
            GetWindowRect(l_hWnd,&l_rect); /* get cx and cy values */
            l_point.x=l_point.y=0;
            MapWindowPoints(l_hWnd,p_hWnd,&l_point,1); /* get x and y values */
            if ((l_i%3)==1) /* left hand column */
              SetWindowPos(l_hWnd,HWND_TOP,
                l_point.x-0,l_point.y,
                l_rect.right-l_rect.left+EO_TYPE_WINDOW_GROWTH,
                l_rect.bottom-l_rect.top+EO_TYPE_WINDOW_GROWTH,
                SWP_DRAWFRAME|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOREDRAW);
            else if ((l_i%3)==2) /* middle column */
              SetWindowPos(l_hWnd,HWND_TOP,
                l_point.x-(EO_TYPE_WINDOW_GROWTH/2),l_point.y,
                l_rect.right-l_rect.left+EO_TYPE_WINDOW_GROWTH,
                l_rect.bottom-l_rect.top+EO_TYPE_WINDOW_GROWTH,
                SWP_DRAWFRAME|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOREDRAW);
            else /* right hand column */
              SetWindowPos(l_hWnd,HWND_TOP,
                l_point.x-EO_TYPE_WINDOW_GROWTH,l_point.y,
                l_rect.right-l_rect.left+EO_TYPE_WINDOW_GROWTH,
                l_rect.bottom-l_rect.top+EO_TYPE_WINDOW_GROWTH,
                SWP_DRAWFRAME|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOREDRAW);
            InvalidateRect(l_hWnd,NULL,TRUE);
            return TRUE;
          } else if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)==CBN_KILLFOCUS) {
            RECT l_rect;
            POINT l_point;
            HWND l_hWnd;

            /* get window */
            l_i=(GET_WM_COMMAND_ID(p_wParam,p_lParam)-IDC_EDITOBJDT0)/4;
            l_hWnd=GET_WM_COMMAND_HWND(p_wParam,p_lParam);

            /* Hide label so we don't overlap */
            ShowWindow(GetDlgItem(p_hWnd,
              (int)(IDC_EDITOBJDL0+(l_i*4))),
              SW_SHOWNA);

            /* enlarge window */
            GetWindowRect(l_hWnd,&l_rect); /* get cx and cy values */
            l_point.x=l_point.y=0;
            MapWindowPoints(l_hWnd,p_hWnd,&l_point,1); /* get x and y values */
            if ((l_i%3)==1) /* left hand column */
              SetWindowPos(l_hWnd,NULL,
                l_point.x-0,l_point.y,
                l_rect.right-l_rect.left-EO_TYPE_WINDOW_GROWTH,
                l_rect.bottom-l_rect.top,
                SWP_DRAWFRAME|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);
            else if ((l_i%3)==2) /* middle column */
              SetWindowPos(l_hWnd,NULL,
                l_point.x+(EO_TYPE_WINDOW_GROWTH/2),l_point.y,
                l_rect.right-l_rect.left-EO_TYPE_WINDOW_GROWTH,
                l_rect.bottom-l_rect.top,
                SWP_DRAWFRAME|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);
            else /* right hand column */
              SetWindowPos(l_hWnd,NULL,
                l_point.x+EO_TYPE_WINDOW_GROWTH,l_point.y,
                l_rect.right-l_rect.left-EO_TYPE_WINDOW_GROWTH,
                l_rect.bottom-l_rect.top,
                SWP_DRAWFRAME|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);
            return TRUE;
          }
          break;
        case IDC_EDITOBJTYPE:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)==CBN_SELCHANGE) {
            /* user has changed the object type - update our details */
            l_thing=edObjectOf(l_VNum);
            if (!l_thing)
              break;

            /* clear out any detail data */
            for (l_i=0;l_i<16;l_i++)
              (DSSObject(l_thing)->oDetail)[(int)l_i]=0;

            /* mark object as changed */
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,
              (LPCSTR)g_edFlag))|EDFLAG_CHANGED));

            /* update the display of our details section */
            SendMessage(p_hWnd,WM_USER_CHANGEDDATA,0,0L);
            return TRUE;
          }
          break;
        default:
          break;
        }
      break;
    case WM_USER_CHANGEDDATA:
      /* This call updates the Details section of the screen */
      /* first thing we need to do is look at what type of object this is. Note we
       * get this info from the screen itself because it's the "edited" informations */
#ifdef WIN32
      l_VNum=(LONG)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
      l_VNum=(LONG)MAKELONG(GetProp(p_hWnd,(LPCSTR)g_edLongLo),
        GetProp(p_hWnd,(LPCSTR)g_edLongHi));
#endif
      if (l_VNum>=0) {
        l_thing=edObjectOf(l_VNum);
        if ((l_thing) && (l_thing->sType==DS_STYPE_OBJECT)) {
          edGetDlgItemData(p_hWnd,IDC_EDITOBJTYPE,&l_byte,EDSGD_LIST,MOLE_LIST_OTYPE);
          l_ftl=dsFTLOf(&g_awFTLObjType,l_byte);
          if (l_ftl)
            l_ftl=l_ftl->fNext;
          /* cycle through windows setting their properties appropriately */
          for (l_i=0;l_i<16;l_i++) {
            if (l_ftl) {
              /* this control is live */
              /* let's set up the name of this detail */
              SetDlgItemText(p_hWnd,(int)(IDC_EDITOBJDL0+(l_i*4)+0),l_ftl->fName);
              /* and show the name */
              EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDL0+(l_i*4))),TRUE);
              ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDL0+(l_i*4))),SW_SHOWNA);
              /* now, what kind of control do we need? Look at l_ftl to tell us */
              switch (l_ftl->fType) {
                case MOLE_LISTTYPE_TYPE:
                  /* type combo box */
                  /* set the value */
                  l_byte=(DSBYTE)(DSSObject(l_thing)->oDetail[(int)l_i]);
                  edSetDlgItemData(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4)),
                    &l_byte,EDSGD_LIST,l_ftl->fList);
                  /* and now show it */
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDV0+(l_i*4))),FALSE);
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4))),TRUE);
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDF0+(l_i*4))),FALSE);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDV0+(l_i*4))),SW_HIDE);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4))),SW_SHOWNA);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDF0+(l_i*4))),SW_HIDE);
                  break;
                case MOLE_LISTTYPE_FLAG:
                  /* flag button */
                  /* set the value */
                  edSetDlgItemData(p_hWnd,(int)(IDC_EDITOBJDF0+(l_i*4)),
                    &(DSSObject(l_thing)->oDetail[(int)l_i]),EDSGD_FLAG,l_ftl->fList);
                  /* and now show it */
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDV0+(l_i*4))),FALSE);
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4))),FALSE);
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDF0+(l_i*4))),TRUE);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDV0+(l_i*4))),SW_HIDE);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4))),SW_HIDE);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDF0+(l_i*4))),SW_SHOWNA);
                  break;
                case MOLE_LISTTYPE_INVALID:
                  /* edit window */
                  edSetDlgItemData(p_hWnd,(int)(IDC_EDITOBJDV0+(l_i*4)),
                    &(DSSObject(l_thing)->oDetail[(int)l_i]),EDSGD_LWORD,0);
                  /* and now show it */
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDV0+(l_i*4))),TRUE);
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4))),FALSE);
                  EnableWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDF0+(l_i*4))),FALSE);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDV0+(l_i*4))),SW_SHOWNA);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDT0+(l_i*4))),SW_HIDE);
                  ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDF0+(l_i*4))),SW_HIDE);
                  break;
                default:
                  /* we don't know how to handle this - hide it */
                  for (l_j=1;l_j<4;l_j++) /* leave name present so they can see it */
                    ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDL0+(l_i*4)+l_j)),SW_HIDE);
                  break;
              }
              l_ftl=l_ftl->fNext;
            } else {
              /* this control is dead */
            for (l_j=0;l_j<4;l_j++)
              ShowWindow(GetDlgItem(p_hWnd,(int)(IDC_EDITOBJDL0+(l_i*4)+l_j)),SW_HIDE);
            }
          } /* for */
        }
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

