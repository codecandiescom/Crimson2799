// EditRST module
// two-character identifier: er
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
#include"editrst.h"
#include"dialog.h"
#include"editprop.h"
#include"editextr.h"
#include"editflag.h"
#include"areawnd.h"
#include"help.h"
#include"ctl3dl.h"

// Current Reset Commands supported:
// M - mobile to room              M <if> <mobile> <max>   <room>  [<room max>]
// O - object to room              O <if> <object> <max>   <room>  [<room max>]
// G - give object to last mobile  G <if> <object> <max>
// P - put object inside object    P <if> <object> <max>   <object>
// E - equip mobile with an object E <if> <object> <max>   [<position>]
// D - set door state              D <if> <room>   <exit>  <door_flags>
// R - remove object from room     R <if> <room>   <object>
// * - denotes a comment the rest of the line is ignored

/* Constants */
#define EDITRST_RPAGE "RPage"   /* property label */

/* Globals */
char g_erResetCmd[8]= {
 'M', /* Mobile to room */                 /* offset 0 */
 'O', /* Object to room */                 /* offset 1 */
 'R', /* Remove object from room */        /* offset 2 */
 'G', /* Give object to last mobile */     /* offset 3 */
 'P', /* Put object inside object */       /* offset 4 */
 'E', /* Equip MOB with an object */       /* offset 5 */
 'D', /* Set door state */                 /* offset 6 */
 '*', /* comment */                        /* offset 7 */
};
char g_erResetCmdLabel[8][20]= {
 "MOB to Room",
 "OBJ to Room",
 "OBJ from Room",
 "Give OBJ to MOB",
 "Put OBJ in OBJ",
 "Equip MOB with OBJ",
 "Set EXIT state",
 "*comment*"
};


BOOL CALLBACK _export erEditRSTProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam) {
  long l_VNum,l_i,l_j,l_k;
  DSSTRUCT *l_thing,*l_edit;
//  char l_buf[12];
  BOOL l_changed;
//  DSWORD l_tWord,l_tResultWord;
//  DSLWORD l_tResultLWord;
  DSFLAG l_flag;

  switch (p_message)
		{
    case WM_INITDIALOG:
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCascadeDialogBox(p_hWnd);
      for (l_i=0;l_i<IDC_EDITRSTCELLNUM;l_i++) {
        SendDlgItemMessage(p_hWnd,
          IDC_EDITRSTCMD1+l_i*IDC_EDITRSTCELLELEMNTS,
          CB_SETEXTENDEDUI,TRUE,0L);
        SendDlgItemMessage(p_hWnd,
          IDC_EDITRSTARG2B1+l_i*IDC_EDITRSTCELLELEMNTS,
          CB_SETEXTENDEDUI,TRUE,0L);
      }
      /* and then fall through to the following ... */
    case WM_USER_INITDIALOGSUB:
      l_VNum=p_lParam;
#ifdef WIN32
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,(HANDLE)(l_VNum));
#else
      SetProp(p_hWnd,(LPCSTR)g_edLongHi,HIWORD(l_VNum));
      SetProp(p_hWnd,(LPCSTR)g_edLongLo,LOWORD(l_VNum));
#endif
      RemoveProp(p_hWnd,(LPCSTR)g_edFlag);
      for (l_i=IDC_EDITRSTACTIVESTART;l_i<=IDC_EDITRSTACTIVEEND;l_i++) {
        EnableWindow(GetDlgItem(p_hWnd,(int)l_i),FALSE);
      }
      for (l_i=0;l_i<IDC_EDITRSTCELLNUM;l_i++) {
        for (l_j=0;l_j<IDC_EDITRSTCELLELEMNTS;l_j++) {
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTCELLSTART+l_j+l_i*IDC_EDITRSTCELLELEMNTS),SW_HIDE);
        }
        /* let's set up that pesky command combo box */
        /* Note we don't use FTLs for this because these reset cmds shouldn't
         * change in any big way, and if they do, well too bad. */
        /* no easy list exists on the MUD side to support FTLs anyways... we'd
         * have to build one special for the MUD. That would suck, and it would
         * rival OBJ's in complexity, so I figured the benefit wasn't worth the
         * cost. */
        for (l_j=0;l_j<8;l_j++) {
          l_k=SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+l_i*IDC_EDITRSTCELLELEMNTS,
            CB_ADDSTRING,0,(LPARAM)(g_erResetCmdLabel[l_j]));
          if ((l_k!=LB_ERR)&&(l_k!=LB_ERRSPACE)) {
            SendDlgItemMessage(p_hWnd,
              IDC_EDITRSTCMD1+l_i*IDC_EDITRSTCELLELEMNTS,
              CB_SETITEMDATA,(WPARAM)l_j,(LPARAM)g_erResetCmd[l_j]);
          }
        }
      }

      EnableWindow(GetDlgItem(p_hWnd,IDOK),FALSE);
      return TRUE;
    case WM_USER_NEWDATA:
      SetProp(p_hWnd,EDITRST_RPAGE,(HANDLE)0);
#ifdef WIN32
      l_VNum=(long)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
      l_VNum=(long)MAKELONG(GetProp(p_hWnd,(LPCSTR)g_edLongLo),
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
      }
      /* load data from structure into window fields */
      l_thing=edAreaOf(l_VNum);
      if (l_thing) {
        edSetDlgItemData(p_hWnd,IDC_EDITRSTAREA,&(DSSList(l_thing)->lName),EDSGD_STR,EDSGD_STR_STRIPNL);
      }
      for (l_i=IDC_EDITRSTACTIVESTART;l_i<=IDC_EDITRSTACTIVEEND;l_i++) {
        EnableWindow(GetDlgItem(p_hWnd,(int)l_i),TRUE);
      }
      EnableWindow(GetDlgItem(p_hWnd,IDOK),TRUE);
      /* and fall through to the following */





    case WM_USER_CHANGEDDATA:
      /* this message refreshes the screen and displays this page of data */
#ifdef WIN32
      l_VNum=(long)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
      l_VNum=(long)MAKELONG(GetProp(p_hWnd,(LPCSTR)g_edLongLo),
        GetProp(p_hWnd,(LPCSTR)g_edLongHi));
#endif
      l_thing=edResetOf(l_VNum);
      l_i=(long)GetProp(p_hWnd,EDITRST_RPAGE);
      /* we need to find out how many pages we have available */
      for (l_edit=l_thing,l_j=0;(l_edit);l_edit=l_edit->sNext,l_j++);
      l_k=l_j/IDC_EDITRSTCELLNUM;
      if (l_j%IDC_EDITRSTCELLNUM)
        l_k++;  /* partial page */
      if (l_k==0)
        l_k++;
      if (l_i>=l_k) {
        l_i=l_k-1;
        SetProp(p_hWnd,EDITRST_RPAGE,(HANDLE)l_i);
      }
      SetDlgItemInt(p_hWnd,IDC_EDITRSTPAGE,(UINT)(l_i+1),TRUE);
      SetDlgItemInt(p_hWnd,IDC_EDITRSTPAGETOTAL,(UINT)l_k,TRUE);
      for (l_j=0,l_thing=erFindReset(p_hWnd,l_VNum,0);l_j<IDC_EDITRSTCELLNUM;l_j++) {
        edSetDlgItemData(p_hWnd,IDC_EDITRSTCELLSTART+(l_j*IDC_EDITRSTCELLELEMNTS),
          l_thing,EDSGD_RESET,0);
        if (l_thing)
          l_thing=l_thing->sNext;
      }
      /* now check in case we have NO resets */
      if (!edResetOf(l_VNum)) {
        /* show first INSERT button */
        ShowWindow(GetDlgItem(p_hWnd,IDC_EDITRSTINSERT1),SW_SHOWNA);
      }
      return TRUE;
		case WM_SYSCOMMAND:
#ifdef WIN32
      l_VNum=(long)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
      l_VNum=(long)MAKELONG(GetProp(p_hWnd,(LPCSTR)g_edLongLo),
        GetProp(p_hWnd,(LPCSTR)g_edLongHi));
#endif
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          l_thing=edAreaOf(l_VNum);
          if (l_thing) {
            if (DSSArea(l_thing)->aRST.rEditWindow!=p_hWnd)
              aaDestroyWindow(p_hWnd);
            dsRefFree(&(DSSArea(l_thing)->aRST)); /* this'll destroy our window */
          }
          PostMessage(g_awRootWnd,WM_USER_AREALIST_CHNG,0,0L);
					return TRUE;
				default:
					break;
				}
      break;
    case WM_COMMAND:
#ifdef WIN32
      l_VNum=(long)GetProp(p_hWnd,(LPCSTR)g_edLongHi);
#else
      l_VNum=(long)MAKELONG(GetProp(p_hWnd,(LPCSTR)g_edLongLo),
        GetProp(p_hWnd,(LPCSTR)g_edLongHi));
#endif
      switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case IDC_EDITRSTDELETE1:
        case IDC_EDITRSTDELETE2:
        case IDC_EDITRSTDELETE3:
        case IDC_EDITRSTDELETE4:
        case IDC_EDITRSTDELETE5:
        case IDC_EDITRSTDELETE6:
        case IDC_EDITRSTINSERT1:
        case IDC_EDITRSTINSERT2:
        case IDC_EDITRSTINSERT3:
        case IDC_EDITRSTINSERT4:
        case IDC_EDITRSTINSERT5:
        case IDC_EDITRSTINSERT6:
        case IDC_EDITRSTPREVIOUS:
        case IDC_EDITRSTNEXT:
        case IDOK:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_changed=0;
          /* save data & check for changes */
          l_thing=erFindReset(p_hWnd,l_VNum,0);
          for (l_i=0;(l_i<IDC_EDITRSTCELLNUM)&&(l_thing);l_i++) {
            l_changed|=edGetDlgItemData(p_hWnd,
              IDC_EDITRSTCELLSTART+l_i*IDC_EDITRSTCELLELEMNTS,l_thing,EDSGD_RESET,0);
            l_thing=l_thing->sNext;
          }
          if (l_changed)
            SetProp(p_hWnd,(LPCSTR)g_edFlag,
              (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
          switch (GET_WM_COMMAND_ID(p_wParam,p_lParam)) {
            case IDOK:
              /* save back to server if changes have been made */
              if (((unsigned int)(GetProp(p_hWnd,(LPCSTR)g_edFlag)))&EDFLAG_CHANGED)
                edSendItem(EDFI_RESET,edAreaOf(l_VNum),NULL,NULL);
              /* and then exit - note at this point, our IDOK is the same as our IDCANCEL */
              /* note keep in cache if they hit OK! */
              l_thing=edAreaOf(l_VNum);
              if (l_thing) {
                if (DSSArea(l_thing)->aRST.rEditWindow!=p_hWnd)
                  aaDestroyWindow(p_hWnd);
                dsRefFree(&(DSSArea(l_thing)->aRST)); /* this'll destroy our window */
              }
              PostMessage(g_awRootWnd,WM_USER_AREALIST_CHNG,0,0L);
              return TRUE;
            case IDC_EDITRSTNEXT:
            case IDC_EDITRSTPREVIOUS:
              l_i=(long)GetProp(p_hWnd,EDITRST_RPAGE);
              for (l_j=0,l_thing=edResetOf(l_VNum);(l_thing);l_thing=l_thing->sNext,l_j++);
              l_j++;
              l_k=l_j/IDC_EDITRSTCELLNUM;
              if (l_j%IDC_EDITRSTCELLNUM)
                l_k++;  /* partial page */
              l_k--; /* because our stored value starts at 0 */
              /* l_i is our current page, l_k is our max page */
              if (GET_WM_COMMAND_ID(p_wParam,p_lParam)==IDC_EDITRSTNEXT) {
                l_i++;
                if (l_i>l_k)
                  l_i=0;
              } else {
                l_i--;
                if (l_i<0)
                  l_i=l_k;
              }
              SetProp(p_hWnd,EDITRST_RPAGE,(HANDLE)l_i);
              PostMessage(p_hWnd,WM_USER_CHANGEDDATA,0,0L);
              return TRUE;
            case IDC_EDITRSTDELETE1:
            case IDC_EDITRSTDELETE2:
            case IDC_EDITRSTDELETE3:
            case IDC_EDITRSTDELETE4:
            case IDC_EDITRSTDELETE5:
            case IDC_EDITRSTDELETE6:
              l_thing=erFindReset(p_hWnd,l_VNum,GET_WM_COMMAND_ID(p_wParam,p_lParam));
              if (!l_thing)
                break;
              dsStructFree(l_thing);
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              PostMessage(p_hWnd,WM_USER_CHANGEDDATA,0,0L);
              return TRUE;
            case IDC_EDITRSTINSERT1:
            case IDC_EDITRSTINSERT2:
            case IDC_EDITRSTINSERT3:
            case IDC_EDITRSTINSERT4:
            case IDC_EDITRSTINSERT5:
            case IDC_EDITRSTINSERT6:
              l_thing=erFindReset(p_hWnd,l_VNum,GET_WM_COMMAND_ID(p_wParam,p_lParam));
              l_edit=dsStructAlloc(DS_STYPE_RESET);
              if (!l_edit)
                break;
              if (l_thing) {
                dsStructInsert(NULL,l_thing,l_edit);
              } else {
                l_thing=edAreaOf(l_VNum);
                dsStructInsert(&(DSSArea(l_thing)->aRST),NULL,l_edit);
              }
              DSSReset(l_edit)->rCmd='*';
              erDefaultReset(l_edit);
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
              PostMessage(p_hWnd,WM_USER_CHANGEDDATA,0,0L);
              return TRUE;
            default:
              break;
          }
          break;
    		case IDHELP:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_ER_DIALOG_EDITRST_HELP);
          return TRUE;
        case IDCANCEL:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=edAreaOf(l_VNum);
          if (l_thing) {
            if (DSSArea(l_thing)->aRST.rEditWindow!=p_hWnd)
              aaDestroyWindow(p_hWnd);
            dsRefFree(&(DSSArea(l_thing)->aRST)); /* this'll destroy our window */
          }
          PostMessage(g_awRootWnd,WM_USER_AREALIST_CHNG,0,0L);
					return TRUE;
        case IDC_EDITRSTCMD1:
        case IDC_EDITRSTCMD2:
        case IDC_EDITRSTCMD3:
        case IDC_EDITRSTCMD4:
        case IDC_EDITRSTCMD5:
        case IDC_EDITRSTCMD6:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)==CBN_SELCHANGE) {
            /* set up defaults */
            l_thing=erFindReset(p_hWnd,l_VNum,GET_WM_COMMAND_ID(p_wParam,p_lParam));
            if (l_thing) {
              DSSReset(l_thing)->rCmd=g_erResetCmd[SendDlgItemMessage(p_hWnd,
                GET_WM_COMMAND_ID(p_wParam,p_lParam),CB_GETCURSEL,0,0L)];
              DSSReset(l_thing)->rIf=0;
              erDefaultReset(l_thing);
              edSetDlgItemData(p_hWnd,
                GET_WM_COMMAND_ID(p_wParam,p_lParam)-
                  (IDC_EDITRSTCMD1-IDC_EDITRSTCELLSTART),
                l_thing,EDSGD_RESET,0);
              /* mark this reset window as "changed" */
              SetProp(p_hWnd,(LPCSTR)g_edFlag,
                (HANDLE)(((unsigned int)GetProp(p_hWnd,(LPCSTR)g_edFlag))|EDFLAG_CHANGED));
            }
            return TRUE;
          }
          break;
        case IDC_EDITRSTARG3B1:
        case IDC_EDITRSTARG3B2:
        case IDC_EDITRSTARG3B3:
        case IDC_EDITRSTARG3B4:
        case IDC_EDITRSTARG3B5:
        case IDC_EDITRSTARG3B6:
          /* edit door flags */
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=BN_CLICKED)
            break;
          l_thing=erFindReset(p_hWnd,l_VNum,GET_WM_COMMAND_ID(p_wParam,p_lParam));
          if (!l_thing)
            break;
          l_flag=(DSFLAG)(DSSReset(l_thing)->rArg3);
          if (efEditFlag(p_hWnd,&l_flag,MOLE_LIST_EFLAG,"Exit Flags"))
            (DSSReset(l_thing)->rArg3)=l_flag;
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
      RemoveProp(p_hWnd,EDITRST_RPAGE);
      break;
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
}

DSSTRUCT *erFindReset(HWND p_hWnd,long p_VNum,int p_Control) {
  DSSTRUCT *l_thing;
  int l_i,l_page;
  int l_offset;

  l_page=(long)GetProp(p_hWnd,EDITRST_RPAGE);
  if (p_Control>=IDC_EDITRSTCELLSTART)
    l_offset=l_page*IDC_EDITRSTCELLNUM+((p_Control-IDC_EDITRSTCELLSTART)/IDC_EDITRSTCELLELEMNTS);
  else
    l_offset=l_page*IDC_EDITRSTCELLNUM;
  l_thing=edResetOf(p_VNum);
  for (l_i=0;(l_thing)&&(l_i<l_offset);l_thing=l_thing->sNext,l_i++);
  return l_thing;
}

void erDefaultReset(DSSTRUCT *p_thing) {

  if (!p_thing)
    return;
  if (p_thing->sType!=DS_STYPE_RESET)
    return;

  switch(DSSReset(p_thing)->rCmd) {
    case 'M': /* Mobile to room */
      DSSReset(p_thing)->rArg1=0;
      DSSReset(p_thing)->rArg2=1;
      DSSReset(p_thing)->rArg3=0;
      DSSReset(p_thing)->rArg4=1;
      break;
    case 'O': /* Object to room */
      DSSReset(p_thing)->rArg1=0;
      DSSReset(p_thing)->rArg2=99;
      DSSReset(p_thing)->rArg3=0;
      DSSReset(p_thing)->rArg4=1;
      break;
    case 'R': /* Remove object from room */
      DSSReset(p_thing)->rArg1=0;
      DSSReset(p_thing)->rArg2=0;
      break;
    case 'G': /* Give object to last mobile */
      DSSReset(p_thing)->rArg1=0;
      DSSReset(p_thing)->rArg2=99;
      break;
    case 'P': /* Put object inside object */
      DSSReset(p_thing)->rArg1=0;
      DSSReset(p_thing)->rArg2=99;
      DSSReset(p_thing)->rArg3=0;
      break;
    case 'E': /* Equip MOB with an object */
      DSSReset(p_thing)->rArg1=0;
      DSSReset(p_thing)->rArg2=99;
      break;
    case 'D': /* Set door state */
      DSSReset(p_thing)->rArg1=0;
      DSSReset(p_thing)->rArg2=0;
      DSSReset(p_thing)->rArg3=0;
      break;
    case '*': /* comment */
    default:
      break;
  }
}
