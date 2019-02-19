// EditMOB module
// two-character identifier: em
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
#include"editmob.h"
#include"dialog.h"
#include"editprop.h"
#include"editextr.h"
#include"editflag.h"
#include"help.h"
#include"ctl3dl.h"

BOOL CALLBACK _export emEditMOBProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
  long l_VNum,l_i;
  DSSTRUCT *l_thing,*l_edit;
  char l_buf[12];
  BOOL l_changed;
  DSWORD l_tWord,l_tResultWord;
  DSLWORD l_tResultLWord;

  switch (p_message)
		{
    case WM_INITDIALOG:
      Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCascadeDialogBox(p_hWnd);
      SendDlgItemMessage(p_hWnd,IDC_EDITMOBPOSITION,CB_SETEXTENDEDUI,TRUE,0L);
      SendDlgItemMessage(p_hWnd,IDC_EDITMOBTYPE,CB_SETEXTENDEDUI,TRUE,0L);
      SendDlgItemMessage(p_hWnd,IDC_EDITMOBSEX,CB_SETEXTENDEDUI,TRUE,0L);
      /* and then fall through to the following ... */
    case WM_USER_INITDIALOGSUB:
      l_VNum=p_lParam;
      if (l_VNum>=0) {
        sprintf(l_buf,"%li",l_VNum);
        SetDlgItemText(p_hWnd,IDC_EDITMOBVIRTUAL,l_buf);
      }
#ifdef WIN32
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(l_VNum));
#else
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,HIWORD(l_VNum));
      SetProp(p_hWnd,(LPCSTR)g_edLongLo,LOWORD(l_VNum));
#endif
      RemoveProp(p_hWnd,(LPCSTR)g_edFlag);
      for (l_i=IDC_EDITMOBACTIVESTART;l_i<=IDC_EDITMOBACTIVEEND;l_i++) {
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
        SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(l_VNum));
#else
        SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)HIWORD(l_VNum));
        SetProp(p_hWnd,(LPCSTR)g_edLongLo,(HANDLE)LOWORD(l_VNum));
#endif
        sprintf(l_buf,"%li",l_VNum);
        SetDlgItemText(p_hWnd,IDC_EDITMOBVIRTUAL,l_buf);
      }
      /* load data from structure into window fields */
      l_thing=edMobileOf(l_VNum);
      if (l_thing) {
        edSetDlgItemData(p_hWnd,IDC_EDITMOBNAME,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
        if (l_thing->sType==DS_STYPE_MOBILE) {
          edSetDlgItemData(p_hWnd,IDC_EDITMOBKEY,&(DSSMobile(l_thing)->mKey),EDSGD_STR,EDSGD_STR_STRIPNL);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBLDESC,&(DSSMobile(l_thing)->mLDesc),EDSGD_STR,EDSGD_STR_STRIPNL);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBDESC,&(DSSMobile(l_thing)->mDesc),EDSGD_STR,EDSGD_STR_FORMAT);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBEXTRA,&(DSSMobile(l_thing)->mExtra),EDSGD_EXTRA,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBPROPERTY,&(DSSMobile(l_thing)->mProperty),EDSGD_PROPERTY,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBACTIONFLAGS,&(DSSMobile(l_thing)->mAct),EDSGD_FLAG,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBAFFECTFLAGS,&(DSSMobile(l_thing)->mAffect),EDSGD_FLAG,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBAURA,&(DSSMobile(l_thing)->mAura),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBLEVEL,&(DSSMobile(l_thing)->mLevel),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBHITBONUS,&(DSSMobile(l_thing)->mHitBonus),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBARMOR,&(DSSMobile(l_thing)->mArmor),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBHPDICENUM,&(DSSMobile(l_thing)->mHPDiceNum),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBHPDICESIZE,&(DSSMobile(l_thing)->mHPDiceSize),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBHPBONUS,&(DSSMobile(l_thing)->mHPBonus),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBDAMDICENUM,&(DSSMobile(l_thing)->mDamDiceNum),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBDAMDICESIZE,&(DSSMobile(l_thing)->mDamDiceSize),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBDAMBONUS,&(DSSMobile(l_thing)->mDamBonus),EDSGD_WORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBMONEY,&(DSSMobile(l_thing)->mMoney),EDSGD_LWORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBEXP,&(DSSMobile(l_thing)->mExp),EDSGD_LWORD,0);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBPOSITION,&(DSSMobile(l_thing)->mPos),EDSGD_LIST,MOLE_LIST_POS);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBTYPE,&(DSSMobile(l_thing)->mType),EDSGD_LIST,MOLE_LIST_MTYPE);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBSEX,&(DSSMobile(l_thing)->mSex),EDSGD_LIST,MOLE_LIST_SEX);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBWEIGHT,&(DSSMobile(l_thing)->mWeight),EDSGD_WORD,0);
          for (l_i=IDC_EDITMOBACTIVESTART;l_i<=IDC_EDITMOBACTIVEEND;l_i++) {
            EnableWindow(GetDlgItem(p_hWnd,(int)l_i),TRUE);
          }
          EnableWindow(GetDlgItem(p_hWnd,IDOK),TRUE);
          SetFocus(GetDlgItem(p_hWnd,IDC_EDITMOBNAME));
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
          l_thing=edMobileOf(l_VNum);
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
        case IDC_EDITMOBPREVIOUS:
        case IDC_EDITMOBNEXT:
        case IDOK:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edMobileOf(l_VNum);
          if (l_thing) {
            l_thing->sEditWindow=NULL;
            /* save data & check for changes to MOB */
            l_changed=edGetDlgItemData(p_hWnd,IDC_EDITMOBNAME,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBKEY,&(DSSMobile(l_thing)->mKey),EDSGD_STR,EDSGD_STR_STRIPNL);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBLDESC,&(DSSMobile(l_thing)->mLDesc),EDSGD_STR,EDSGD_STR_STRIPNL);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBDESC,&(DSSMobile(l_thing)->mDesc),EDSGD_STR,EDSGD_STR_FORMAT);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBEXTRA,&(DSSMobile(l_thing)->mExtra),EDSGD_EXTRA,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBPROPERTY,&(DSSMobile(l_thing)->mProperty),EDSGD_PROPERTY,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBACTIONFLAGS,&(DSSMobile(l_thing)->mAct),EDSGD_FLAG,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBAFFECTFLAGS,&(DSSMobile(l_thing)->mAffect),EDSGD_FLAG,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBAURA,&(DSSMobile(l_thing)->mAura),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBLEVEL,&(DSSMobile(l_thing)->mLevel),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBHITBONUS,&(DSSMobile(l_thing)->mHitBonus),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBARMOR,&(DSSMobile(l_thing)->mArmor),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBHPDICENUM,&(DSSMobile(l_thing)->mHPDiceNum),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBHPDICESIZE,&(DSSMobile(l_thing)->mHPDiceSize),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBHPBONUS,&(DSSMobile(l_thing)->mHPBonus),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBDAMDICENUM,&(DSSMobile(l_thing)->mDamDiceNum),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBDAMDICESIZE,&(DSSMobile(l_thing)->mDamDiceSize),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBDAMBONUS,&(DSSMobile(l_thing)->mDamBonus),EDSGD_WORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBMONEY,&(DSSMobile(l_thing)->mMoney),EDSGD_LWORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBEXP,&(DSSMobile(l_thing)->mExp),EDSGD_LWORD,0);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBPOSITION,&(DSSMobile(l_thing)->mPos),EDSGD_LIST,MOLE_LIST_POS);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBTYPE,&(DSSMobile(l_thing)->mType),EDSGD_LIST,MOLE_LIST_MTYPE);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBSEX,&(DSSMobile(l_thing)->mSex),EDSGD_LIST,MOLE_LIST_SEX);
            l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITMOBWEIGHT,&(DSSMobile(l_thing)->mWeight),EDSGD_WORD,0);

            /* save back to server if changes have been made */
            if ((((unsigned int)(GetProp(p_hWnd,(LPCSTR)g_edFlag)))&EDFLAG_CHANGED)||(l_changed))
              edSendItem(EDFI_MOBILE,NULL,l_thing,NULL);

            /* now process button click */
            if (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITMOBPREVIOUS) {
              if (l_thing->sPrevious) {
                l_thing->sEditWindow=NULL;
                edEditItem(EDFI_MOBILE,NULL,l_thing->sPrevious,p_hWnd);
              }
              return TRUE;
            } else if (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITMOBNEXT) {
              if (l_thing->sNext) {
                l_thing->sEditWindow=NULL;
                edEditItem(EDFI_MOBILE,NULL,l_thing->sNext,p_hWnd);
              }
              return TRUE;
            }
          }
          aaDestroyWindow(p_hWnd);
          return TRUE;
				case IDHELP:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_EM_DIALOG_EDITMOB_HELP);
          return TRUE;
        case IDCANCEL:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edMobileOf(l_VNum);
          if (l_thing)
            l_thing->sEditWindow=NULL;
          aaDestroyWindow(p_hWnd);
          return TRUE;
        case IDC_EDITMOBEXTRANEW:
          l_thing=edMobileOf(l_VNum);
          if ((l_thing)&&(l_thing->sType==DS_STYPE_MOBILE)) {
            l_changed=eeEditExtra(p_hWnd,&(DSSMobile(l_thing)->mExtra),
              DS_STYPE_NONE,DS_STYPE_EXTRA);
            if (l_changed) {
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,
                (LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              edSetDlgItemData(p_hWnd,IDC_EDITMOBEXTRA,
                &(DSSMobile(l_thing)->mExtra),EDSGD_EXTRA,0);
            }
            return TRUE;
          }
          break;
        case IDC_EDITMOBEXTRA:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK)
            break;
          /* else fall through to the following */
        case IDC_EDITMOBEXTRAEDIT:
        case IDC_EDITMOBEXTRADELETE:
          if ((GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)&&
            (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK))
            break;
          l_i=SendDlgItemMessage(p_hWnd,IDC_EDITMOBEXTRA,LB_GETCURSEL,0,0L);
          if (l_i!=LB_ERR)
            l_i=SendDlgItemMessage(p_hWnd,IDC_EDITMOBEXTRA,LB_GETITEMDATA,(WPARAM)l_i,0L);
          if (l_i!=LB_ERR) {
            l_thing=edMobileOf(l_VNum);
            if ((l_thing)&&((l_thing->sType)==DS_STYPE_MOBILE)) {
              for (l_edit=(DSSMobile(l_thing)->mExtra.rList);(l_edit)&&(l_i);
                l_edit=l_edit->sNext) {
                l_i--;
              }
              if (l_edit) {
                l_thing=DSStruct(l_edit->sUpRef);
                if ((GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITMOBEXTRAEDIT)||
                    (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITMOBEXTRA)) {
                  /* l_edit now points to our property! */
                  if (eeEditExtra(p_hWnd,l_edit,DS_STYPE_EXTRA,DS_STYPE_EXTRA))
                    SetProp(p_hWnd,(LPCSTR)g_edFlag,
                      (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                } else { /* delete */
                  dsStructFree(l_edit);
                  SetProp(p_hWnd,(LPCSTR)g_edFlag,
                    (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                }
                edSetDlgItemData(p_hWnd,IDC_EDITMOBEXTRA,l_thing,EDSGD_EXTRA,0);
                return TRUE;
              }
            }
          }
          break;
        case IDC_EDITMOBPROPERTYNEW:
          l_thing=edMobileOf(l_VNum);
          if ((l_thing)&&(l_thing->sType==DS_STYPE_MOBILE)) {
            l_changed=epEditProperty(p_hWnd,&(DSSMobile(l_thing)->mProperty),
              DS_STYPE_NONE,DS_STYPE_PROPERTY);
            if (l_changed) {
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              edSetDlgItemData(p_hWnd,IDC_EDITMOBPROPERTY,
                &(DSSMobile(l_thing)->mProperty),EDSGD_PROPERTY,0);
            }
            return TRUE;
          }
          break;
        case IDC_EDITMOBPROPERTY:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK)
            break;
          /* else fall through to the following */
        case IDC_EDITMOBPROPERTYEDIT:
        case IDC_EDITMOBPROPERTYDELETE:
          if ((GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)&&
            (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK))
            break;
          l_i=SendDlgItemMessage(p_hWnd,IDC_EDITMOBPROPERTY,LB_GETCURSEL,0,0L);
          if (l_i!=LB_ERR)
            l_i=SendDlgItemMessage(p_hWnd,IDC_EDITMOBPROPERTY,LB_GETITEMDATA,(WPARAM)l_i,0L);
          if (l_i!=LB_ERR) {
            l_thing=edMobileOf(l_VNum);
            if ((l_thing)&&((l_thing->sType)==DS_STYPE_MOBILE)) {
              for (l_edit=(DSSMobile(l_thing)->mProperty.rList);(l_edit)&&(l_i);
                l_edit=l_edit->sNext) {
                l_i--;
              }
              if (l_edit) {
                l_thing=DSStruct(l_edit->sUpRef);
                if ((GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITMOBPROPERTYEDIT)||
                    (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITMOBPROPERTY)) {
                  /* l_edit now points to our property! */
                  if (epEditProperty(p_hWnd,l_edit,DS_STYPE_PROPERTY,DS_STYPE_PROPERTY))
                    SetProp(p_hWnd,(LPCSTR)g_edFlag,
                      (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                } else { /* delete */
                  SetProp(p_hWnd,(LPCSTR)g_edFlag,
                    (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
                  dsStructFree(l_edit);
                }
                edSetDlgItemData(p_hWnd,IDC_EDITMOBPROPERTY,l_thing,EDSGD_PROPERTY,0);
                return TRUE;
              }
            }
          }
          break;
        case IDC_EDITMOBACTIONFLAGS:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edMobileOf(l_VNum);
          if (!l_thing)
            break;
          if (efEditFlag(p_hWnd,&(DSSMobile(l_thing)->mAct),MOLE_LIST_MACT,"Action Flags"))
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
          return TRUE;
        case IDC_EDITMOBAFFECTFLAGS:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edMobileOf(l_VNum);
          if (!l_thing)
            break;
          if (efEditFlag(p_hWnd,&(DSSMobile(l_thing)->mAffect),MOLE_LIST_AFFECT,"Affect Flags"))
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
          return TRUE;
        case IDC_EDITMOBSTATDEFAULT:
          /* calculate default values based on level */
          edGetDlgItemData(p_hWnd,IDC_EDITMOBLEVEL,&l_tWord,EDSGD_WORD,0);

          if (l_tWord<=20)
            l_tResultLWord=l_tWord*15; /* experience */
          else 
            l_tResultLWord=(l_tWord-20L)*600L;
          edSetDlgItemData(p_hWnd,IDC_EDITMOBEXP,&l_tResultLWord,EDSGD_LWORD,0);
          l_tResultWord=l_tWord*2.5-3; /* armor */
          if (l_tResultWord<0)
            l_tResultWord=0;
          edSetDlgItemData(p_hWnd,IDC_EDITMOBARMOR,&l_tResultWord,EDSGD_WORD,0);
          if (l_tWord<=20)
            l_tResultWord=1; /* HP Dice Num */
          else
            l_tResultWord=2;
          edSetDlgItemData(p_hWnd,IDC_EDITMOBHPDICENUM,&l_tResultWord,EDSGD_WORD,0);
          if (l_tWord<=20)
            l_tResultWord=(DSWORD)(l_tWord*3); /* HP Dice Size */
          else
            l_tResultWord=(DSWORD)((l_tWord-20)*50);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBHPDICESIZE,&l_tResultWord,EDSGD_WORD,0);
          if (l_tWord<=20)
            l_tResultWord=(DSWORD)(l_tWord*10); /* HP Bonus */
          else
            l_tResultWord=(DSWORD)((l_tWord-20)*100+100);
          edSetDlgItemData(p_hWnd,IDC_EDITMOBHPBONUS,&l_tResultWord,EDSGD_WORD,0);
          l_tResultWord=(DSWORD)(l_tWord*3); /* hit bonus */
          edSetDlgItemData(p_hWnd,IDC_EDITMOBHITBONUS,&l_tResultWord,EDSGD_WORD,0);
          l_tResultWord=3;    /* damage dice num */
          edSetDlgItemData(p_hWnd,IDC_EDITMOBDAMDICENUM,&l_tResultWord,EDSGD_WORD,0);
          l_tResultWord=(DSWORD)((5*l_tWord)/12+1); /* Damage Dice Size */
          edSetDlgItemData(p_hWnd,IDC_EDITMOBDAMDICESIZE,&l_tResultWord,EDSGD_WORD,0);
          l_tResultWord=(DSWORD)(l_tWord/3); /* Damage Bonus */
          edSetDlgItemData(p_hWnd,IDC_EDITMOBDAMBONUS,&l_tResultWord,EDSGD_WORD,0);

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

