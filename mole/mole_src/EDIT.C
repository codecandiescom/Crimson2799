// Edit Manager
// two character identifier: ed
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

// This module manages the interface between the DSTRUCT/MOLE protocol,
// and the editing features of the various windows. Functions in here
// are notified when structure data is available, and in turn the
// appropriate windows are then notified.
// Functions in here are also called to fetch structure data, windows
// to notify upon receipt of said structures are recorded.

#include<windows.h>
#include<stdio.h>
#include<string.h>
#include"molerc.h"
#include"moledefs.h"
#include"molem.h"
#include"moleprot.h"
#include"dstruct.h"
#include"areawnd.h"
#include"edit.h"
#include"infobox.h"
#include"debug.h"
#include"host.h"
#include"editwld.h"
#include"editmob.h"
#include"editobj.h"
#include"editadt.h"
#include"editrst.h"
#include"pack.h"
#include"help.h"

/* Constants */
#define ED_ATOMSTR_HI "EditLong Hi"
#define ED_ATOMSTR_LO "EditLong Lo"
#define ED_ATOMSTR_FLAG "EditFlag"
#define ED_ATOMSTR_DATAHI "Extra Data Hi"
#define ED_ATOMSTR_DATALO "Extra Data Lo"

/* Globals */
ATOM g_edLongHi,g_edLongLo,g_edFlag;
EDREQ *g_edReqList; /* list of all pending requests & the windows to notify upon receipt */
long g_edItemTimeout;

BOOL edInitEdit() {
  g_edReqList=NULL;
//  g_edItemTimeout=0L; /* timeout in seconds */
  g_edItemTimeout=60L; /* timeout in seconds */
  g_edLongHi=GlobalAddAtom(ED_ATOMSTR_HI);
  g_edLongLo=GlobalAddAtom(ED_ATOMSTR_LO);
  g_edFlag=GlobalAddAtom(ED_ATOMSTR_FLAG);
  return TRUE;
}

void edShutdownEdit() {
  while(g_edReqList)
    edReqFree(g_edReqList);
  GlobalDeleteAtom(g_edLongHi);
  GlobalDeleteAtom(g_edLongLo);
  GlobalDeleteAtom(g_edFlag);
  return;
}

void edResetConnection() {
  while(g_edReqList)
    edReqFree(g_edReqList);
  return;
}

EDREQ *edReqAlloc() {
  HGLOBAL l_GlobalTemp;
  EDREQ *l_req;

  l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(EDREQ));
  l_req=(EDREQ *)GlobalLock(l_GlobalTemp);
  if ((!l_GlobalTemp)||(!(l_req)))
    return NULL;
  l_req->rNext=g_edReqList;
  l_req->rPrevious=NULL;
  if (g_edReqList)
    g_edReqList->rPrevious=l_req;
  g_edReqList=l_req;
  l_req->rMemory=l_GlobalTemp;
  l_req->rSend=FALSE; /* by default */
  return l_req;
}

void edReqFree(EDREQ *p_req) {
  HGLOBAL l_GlobalTemp;

  if (!p_req) return;

  if (p_req->rPrevious) {
    p_req->rPrevious->rNext=p_req->rNext;
  } else {
    g_edReqList=p_req->rNext;
  }

  if (p_req->rNext)
    p_req->rNext->rPrevious=p_req->rPrevious;

  l_GlobalTemp=p_req->rMemory;
  GlobalUnlock(l_GlobalTemp);
  GlobalFree(l_GlobalTemp);
  return;
}

/* This proc is the general-purpose fetch-all function. */
/* Note that this proc also makes sure that, if two sources fetch the same
 * data, that only one request is sent, but that both sources are still
 * notified. */
/* note: if p_area is not required, it MUST BE NULL. */
/* note: if p_thing is not required, it MUST BE 0L. */
/* If p_hWnd is specified, this window will be sent a WM_USER_NEWDATA message
 * when the data comes in. */
void edFetchItem(int p_itemType,DSSTRUCT *p_area,DSSTRUCT *p_thing,HWND p_hWnd) {
  EDREQ *l_req,*l_newreq;
  DSSTRUCT *l_thing;
  HWND l_wnd;

  if (g_hoConnectionState!=HO_CONNECT_STATE_ONLINE)
    return;

  /* we always generate a new request */
  l_newreq=edReqAlloc();
  l_newreq->rItemType=p_itemType;
  l_newreq->rWnd=p_hWnd;
  if (p_area)
    l_newreq->rArea=DSSList(p_area)->lVNum;
  else
    l_newreq->rArea=-1L;
  if (p_thing)
    l_newreq->rVNum=DSSList(p_thing)->lVNum;
  else
    l_newreq->rVNum=-1L;
  l_newreq->rSend=FALSE; /* we're REQUESTING, not SENDING */

  /* first make sure item hasn't already been requested, and if so, add a new
   * requests on to include the new window (if an edit has been requested */
  for (l_req=g_edReqList;l_req;l_req=l_req->rNext) {
    if (l_req!=l_newreq)
      if (l_req->rItemType==l_newreq->rItemType)
        if (l_req->rVNum==l_newreq->rVNum)
          if (l_req->rSend==l_newreq->rSend)
            if (l_newreq->rArea==l_req->rArea) {
              dbPrint("Repeat Request found");
              break; /* we found a match */
            }
  }
  if (l_req) {
    /* and the most important step is.... */
    l_newreq->rID=l_req->rID;
    return;
  }

  /* nope, this is new data! Generate a MOLE request! */
  switch (p_itemType) {
    case EDFI_AREALIST: /* needs no other params */
      if (g_awRootRef.rState==DS_NS_AVAIL) { /* warn about re-load */
        if (ibInfoBox(g_aahWnd,"You are about to re-load the Area List. This will cause any un-saved\nediting changes to be lost. Do you wish to continue?",
          "Warning!",IB_YESNO|IB_RUNHELP,NULL,HP_IB_AREALIST_RELOAD_WARN)==IB_NO) {
          edReqFree(l_newreq);
          return;
        }
      }
      /* first, get rid of any existing list */
      dsRefFree(&g_awRootRef);
      /* next, mark our list as "pending" and refresh any displays */
      g_awRootRef.rState=DS_NS_PENDING;
      edGlobalNotification();

      /* clear any pending requests from g_edReqList */
      /* (because everything has been invalidated, any incoming data
       * essentially must be ignored */
      while (l_newreq->rNext)
        edReqFree(l_newreq->rNext);
      while (l_newreq->rPrevious)
        edReqFree(l_newreq->rPrevious);

      /* and now go fetch our new list */
      l_newreq->rID=mpReqSubmitEx(MOLE_CMD_ALRQ,NULL,0);

      return;
    case EDFI_WLDLIST:  /* needs p_areaName */
      if (!p_area) break;

      l_newreq->rVNum=DSSList(p_area)->lVNum;
      if (DSSArea(p_area)->aWLD.rState==DS_NS_AVAIL) { /* warn about re-load */
        /* check to see if anything is being edited */
        for (l_thing=DSSArea(p_area)->aWLD.rList;l_thing;l_thing=l_thing->sNext) {
          if (l_thing->sEditWindow)
            break;
        }
        if (l_thing) {
          if (ibInfoBox(g_aahWnd,"You are about to re-load a World List.\nUn-saved World editing changes will be lost (for this area only).\n Do you wish to continue?",
            "Warning!",IB_YESNO|IB_RUNHELP,NULL,HP_IB_WLDLIST_RELOAD_WARN)==IB_NO) {
            edReqFree(l_newreq);
            return;
          }
        }
      }
      /* first, get rid of any existing list */
      dsRefFree(&(DSSArea(p_area)->aWLD));
      /* next, mark our list as "pending" and refresh any displays */
      DSSArea(p_area)->aWLD.rState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go fetch our new list */
      l_newreq->rID=mpReqSubmitNum(MOLE_CMD_WLRQ,l_newreq->rArea);
      return;
    case EDFI_MOBLIST:  /* needs p_areaName */
      if (!p_area) break;

      l_newreq->rVNum=DSSList(p_area)->lVNum;
      if (DSSArea(p_area)->aMOB.rState==DS_NS_AVAIL) { /* warn about re-load */
        /* check to see if anything is being edited */
        for (l_thing=DSSArea(p_area)->aMOB.rList;l_thing;l_thing=l_thing->sNext) {
          if (l_thing->sEditWindow)
            break;
        }
        if (l_thing) {
          if (ibInfoBox(g_aahWnd,"You are about to re-load a Mobile List.\nUn-saved Mobile editing changes will be lost (for this area only).\n Do you wish to continue?",
            "Warning!",IB_YESNO|IB_RUNHELP,NULL,HP_IB_MOBLIST_RELOAD_WARN)==IB_NO) {
            edReqFree(l_newreq);
            return;
          }
        }
      }
      /* first, get rid of any existing list */
      dsRefFree(&(DSSArea(p_area)->aMOB));
      /* next, mark our list as "pending" and refresh any displays */
      DSSArea(p_area)->aMOB.rState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go fetch our new list */
      l_newreq->rID=mpReqSubmitNum(MOLE_CMD_MLRQ,l_newreq->rArea);
      return;
    case EDFI_OBJLIST:  /* needs p_areaName */
      if (!p_area) break;

      l_newreq->rVNum=DSSList(p_area)->lVNum;
      if (DSSArea(p_area)->aOBJ.rState==DS_NS_AVAIL) { /* warn about re-load */
        /* check to see if anything is being edited */
        for (l_thing=DSSArea(p_area)->aOBJ.rList;l_thing;l_thing=l_thing->sNext) {
          if (l_thing->sEditWindow)
            break;
        }
        if (l_thing) {
          if (ibInfoBox(g_aahWnd,"You are about to re-load an Object List.\nUn-saved Object editing changes will be lost (for this area only).\n Do you wish to continue?",
            "Warning!",IB_YESNO|IB_RUNHELP,NULL,HP_IB_OBJLIST_RELOAD_WARN)==IB_NO) {
            edReqFree(l_newreq);
            return;
          }
        }
      }
      /* first, get rid of any existing list */
      dsRefFree(&(DSSArea(p_area)->aOBJ));
      /* next, mark our list as "pending" and refresh any displays */
      DSSArea(p_area)->aOBJ.rState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go fetch our new list */
      l_newreq->rID=mpReqSubmitNum(MOLE_CMD_OLRQ,l_newreq->rArea);
      return;
    case EDFI_AREADETAIL:  /* needs p_areaName */
      if (!p_area) break;

      l_newreq->rVNum=DSSList(p_area)->lVNum;
//      if (DSSArea(p_area)->aDetail.rState==DS_NS_AVAIL) { /* warn about re-load */
//        /* check to see if anything is being edited */
//        /* note there should only ever be 1 areadetail structure attached to
//         * this reference. */
//        if (DSSArea(p_area)->aDetail.rEditWindow) {
//          if (ibInfoBox(g_aahWnd,"You are about to re-load Area Details.\nUn-saved Area Detail editing changes will be lost (for this area only).\n Do you wish to continue?",
//            "Warning!",IB_YESNO,NULL,0L)==IB_NO) {
//            edReqFree(l_newreq);
//            return;
//          }
//        }
//      }
      /* first, get rid of any existing list */
      /* oops - dsRefFree will kill our window! */
      l_wnd=DSSArea(p_area)->aDetail.rEditWindow;
      DSSArea(p_area)->aDetail.rEditWindow=NULL;
      dsRefFree(&(DSSArea(p_area)->aDetail));
      DSSArea(p_area)->aDetail.rEditWindow=l_wnd;
      /* next, mark our list as "pending" and refresh any displays */
      DSSArea(p_area)->aDetail.rState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go fetch our new list */
      l_newreq->rID=mpReqSubmitNum(MOLE_CMD_ADRQ,l_newreq->rArea);
      return;
    case EDFI_RESET:   /* needs p_areaName */
      if (!p_area) break;

      l_newreq->rVNum=DSSList(p_area)->lVNum;
//      if (DSSArea(p_area)->aRST.rState==DS_NS_AVAIL) { /* warn about re-load */
//        /* check to see if anything is being edited */
//        /* note the reset list is "all or nothing"... so the FIRST item should
//         * or should not contain the edit window pointer... but let's check them
//         * all just in case */
//        if (DSSArea(p_area)->aRST.rEditWindow) {
//          if (ibInfoBox(g_aahWnd,"You are about to re-load Reset information.\nUn-saved Reset editing changes will be lost (for this area only).\n Do you wish to continue?",
//            "Warning!",IB_YESNO,NULL,0L)==IB_NO) {
//            edReqFree(l_newreq);
//            return;
//          }
//        }
//      }
      /* first, get rid of any existing list */
      l_wnd=DSSArea(p_area)->aRST.rEditWindow;
      DSSArea(p_area)->aRST.rEditWindow=NULL;
      dsRefFree(&(DSSArea(p_area)->aRST));
      DSSArea(p_area)->aRST.rEditWindow=l_wnd;
      /* next, mark our list as "pending" and refresh any displays */
      DSSArea(p_area)->aRST.rState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go fetch our new list */
      l_newreq->rID=mpReqSubmitNum(MOLE_CMD_RLRQ,l_newreq->rArea);
      return;
    case EDFI_WORLD:   /* needs p_virtualNum */
    case EDFI_MOBILE:  /* needs p_virtualNum */
    case EDFI_OBJECT:  /* needs p_virtualNum */
      if (!p_thing) break;

      /* first, get rid of any existing data - replace existing structure
       * with a list if it's not already one.  */
      if (p_thing->sType!=DS_STYPE_LIST) {
        p_thing=dsChangeListType(p_thing,DS_STYPE_LIST);
      }
      /* next, mark our list as "pending" and refresh any displays */
      p_thing->sState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go fetch our new mobile data */
      if (p_itemType==EDFI_WORLD)
        l_newreq->rID=mpReqSubmitNum(MOLE_CMD_WDRQ,DSSList(p_thing)->lVNum);
      else if (p_itemType==EDFI_MOBILE)
        l_newreq->rID=mpReqSubmitNum(MOLE_CMD_MDRQ,DSSList(p_thing)->lVNum);
      else /* EDFI_OBJECT */
        l_newreq->rID=mpReqSubmitNum(MOLE_CMD_ODRQ,DSSList(p_thing)->lVNum);
      return;
    default:
      break;
  }
  /* error */
  edReqFree(l_newreq);
  return;
}

void edPacketHasArrived(unsigned long p_pktID,unsigned long p_Cmd,unsigned long p_returnCode) {
  EDREQ *l_req,*l_nextreq;
  DSSTRUCT *l_thing;

  /* search through request list and notify any windows waiting for this structure */
  for (l_req=g_edReqList;l_req;l_req=l_nextreq) {
    l_nextreq=l_req->rNext;
    if (l_req->rID==p_pktID) {
      /* we handle ACKP and NACK messages here. I know, this seems a little
       * bit out of the layer; but nothing is decoded so this doesn't belong
       * in unpack.c. Besides, unpack.c doesn't get the packet ID, nor does it
       * have access to the request lists at this level. Therefore, this
       * "exception processing" is done here. */
      if (p_Cmd==MOLE_CMD_ACKP) {
        switch (l_req->rItemType) {
          case EDFI_AREADETAIL:
            /* mark area detail as being updated & available */
            l_thing=edAreaOf(l_req->rVNum);
            if (l_thing) {
              DSSArea(l_thing)->aDetail.rState=DS_NS_AVAIL;
              l_thing=DSSArea(l_thing)->aDetail.rList;
            }
            if (l_thing) {
              if (l_thing->sType==DS_STYPE_AREADETAIL)
                l_thing->sState=DS_NS_AVAIL;
            }
            break;
          case EDFI_RESET:
            /* mark world as being updated & available */
            l_thing=edAreaOf(l_req->rVNum);
            if (l_thing) {
              DSSArea(l_thing)->aRST.rState=DS_NS_AVAIL;
            }
            if (l_thing) {
              for (l_thing=DSSArea(l_thing)->aRST.rList;l_thing;l_thing=l_thing->sNext) {
                if (l_thing->sType==DS_STYPE_RESET)
                  l_thing->sState=DS_NS_AVAIL;
              }
            }
            break;
          case EDFI_WORLD:
            /* mark world as being updated & available */
            l_thing=edWorldOf(l_req->rVNum);
            if (l_thing) {
              if (l_thing->sType==DS_STYPE_WORLD)
                l_thing->sState=DS_NS_AVAIL;
              else if (l_thing->sType==DS_STYPE_LIST)
                l_thing->sState=DS_NS_NOTAVAIL;
            }
            break;
          case EDFI_MOBILE:
            /* mark mobile as being updated & available */
            l_thing=edMobileOf(l_req->rVNum);
            if (l_thing) {
              if (l_thing->sType==DS_STYPE_MOBILE)
                l_thing->sState=DS_NS_AVAIL;
              else if (l_thing->sType==DS_STYPE_LIST)
                l_thing->sState=DS_NS_NOTAVAIL;
            }
            break;
          case EDFI_OBJECT:
            /* mark object as being updated & available */
            l_thing=edObjectOf(l_req->rVNum);
            if (l_thing) {
              if (l_thing->sType==DS_STYPE_OBJECT)
                l_thing->sState=DS_NS_AVAIL;
              else if (l_thing->sType==DS_STYPE_LIST)
                l_thing->sState=DS_NS_NOTAVAIL;
            }
            break;
          default:
            /* duno what happened here. */
            break;
        }
      } else if (p_Cmd==MOLE_CMD_NACK) {
        /* sent for a variety of reasons */
 //       l_thing=NULL;
        switch (l_req->rItemType) {
          case EDFI_AREALIST:
            /* only expect l_req->rSend to be FALSE */
            dsRefFree(&g_awRootRef);
            DSBITCLEAR(g_awRootRef.rFlag,DS_FLAG_WINOPEN);
            break;
          case EDFI_WLDLIST:
          case EDFI_MOBLIST:
          case EDFI_OBJLIST:
            /* only expect l_req->rSend to be FALSE (receiving) */
            l_thing=edAreaOf(l_req->rArea);
            if (l_thing) {
              if (l_req->rItemType==EDFI_WLDLIST) {
                dsRefFree(&(DSSArea(l_thing)->aWLD));
                DSBITCLEAR(DSSArea(l_thing)->aWLD.rFlag,DS_FLAG_WINOPEN);
              } else if (l_req->rItemType==EDFI_MOBLIST) {
                dsRefFree(&(DSSArea(l_thing)->aMOB));
                DSBITCLEAR(DSSArea(l_thing)->aMOB.rFlag,DS_FLAG_WINOPEN);
              } else { /* EDFI_OBJLIST */
                dsRefFree(&(DSSArea(l_thing)->aOBJ));
                DSBITCLEAR(DSSArea(l_thing)->aOBJ.rFlag,DS_FLAG_WINOPEN);
              }
            }
            break;
          case EDFI_AREADETAIL:
          case EDFI_RESET:
            l_thing=edAreaOf(l_req->rArea);
            if (l_thing) {
              if (l_thing->sType==DS_STYPE_AREA) {
                if (l_req->rSend==TRUE) {
                  /* trying to SEND information... so let's not TURF user's changes! */
                  if (l_req->rItemType==EDFI_AREADETAIL)
                    DSSArea(l_thing)->aDetail.rState=DS_NS_AVAIL;
                  else /* EDFI_RESET */
                    DSSArea(l_thing)->aRST.rState=DS_NS_AVAIL;
                } else {
                  /* Trying to RECEIVE information */
                  if (l_req->rItemType==EDFI_AREADETAIL)
                    dsRefFree(&(DSSArea(l_thing)->aDetail));
                  else /* EDFI_RESET */
                    dsRefFree(&(DSSArea(l_thing)->aRST));
                }
              }
            }
            break;
          case EDFI_WORLD:
          case EDFI_MOBILE:
          case EDFI_OBJECT:
            if (l_req->rItemType==EDFI_WORLD)
              l_thing=edWorldOf(l_req->rVNum);
            else if (l_req->rItemType==EDFI_MOBILE)
              l_thing=edMobileOf(l_req->rVNum);
            else /* EDFI_OBJECT */
              l_thing=edObjectOf(l_req->rVNum);

            if (l_thing) {
              if (l_req->rSend) {
                l_thing->sState=DS_NS_AVAIL;
              } else {
                l_thing->sState=DS_NS_NOTAVAIL;
                if (l_thing->sEditWindow) {
                  aaDestroyWindow(l_thing->sEditWindow);
                  l_thing->sEditWindow=NULL;
                }
              }
            }
            break;
          default:
            dbPrint("Unexpected or undecoded NACK!");
            /* duno what happened here. */
            break;
        }
        PostMessage(g_aahWnd,WM_USER_EDITERROR,
          (WPARAM)(((l_req->rItemType&0xFF)<<8)|((p_returnCode&0x7F))|
            (l_req->rSend?0x80:0x00)),
          (LPARAM)l_req->rVNum);
      }
      if (IsWindow(l_req->rWnd))
        SendMessage(l_req->rWnd,WM_USER_NEWDATA,0,0L);
//      dbPrint("Removing request");
      edReqFree(l_req);
    }
  }

  /* lastly, execute any default notifications */
  edGlobalNotification();

  return;
}

/* this proc is simply used to report errors to the user when sending/fetching
 * data. The problem is that the ibInfoBox can't be called from within
 * another thread - subsequent socket calls cause a recursive calling
 * of the procs and everything messes up. Therefore, the proc just
 * "tags" the message box to be created by sending a WM_USER_EDITERROR
 * message to g_aahWnd with the appropriate params. Then, later, outside
 * the thread, this function is called and alerts the user to the
 * error condition. */
void edEditError(WPARAM p_wParam,LPARAM p_lParam) {
  char l_buf[200];
  long l_itemType,l_returnCode,l_VNum;
  int l_send;

  l_itemType=(p_wParam&0xFF00)>>8;
  l_returnCode=(p_wParam&0x007F);
  l_send=(p_wParam&0x0080);
  l_VNum=p_lParam;

  l_buf[0]=0;
  if (l_itemType==EDFI_WORLD)
    sprintf(l_buf,"Request to %s World #%li denied: ",l_send?"change":"fetch",l_VNum);
  else if (l_itemType==EDFI_MOBILE)
    sprintf(l_buf,"Request to %s Mobile #%li denied: ",l_send?"change":"fetch",l_VNum);
  else if (l_itemType==EDFI_OBJECT)
    sprintf(l_buf,"Request to %s Object #%li denied: ",l_send?"change":"fetch",l_VNum);
  else if (l_itemType==EDFI_RESET)
    sprintf(l_buf,"Request to %s Reset #%li denied: ",l_send?"change":"fetch",l_VNum);
  else if (l_itemType==EDFI_AREADETAIL)
    sprintf(l_buf,"Request to %s Area Detail #%li denied: ",l_send?"change":"fetch",l_VNum);
  else
    sprintf(l_buf,"Request to fetch MOB/OJB/WLD/Area List denied: ");
  if (l_returnCode==MOLE_NACK_NOTIMPLEMENTED)
    strcat(l_buf,"Command Not Implemented");
  else if (l_returnCode==MOLE_NACK_RESEND)
    strcat(l_buf,"Garbled Packet. Please retry command.");
  else if (l_returnCode==MOLE_NACK_AUTHORIZATION)
    strcat(l_buf,"You don't have the authorization.");
  else if (l_returnCode==MOLE_NACK_NODATA)
    strcat(l_buf,"Area/wld/obj/mob doesn't exist.");
  else if (l_returnCode==MOLE_NACK_PROTECTION)
    strcat(l_buf,"you don't have write/modify privledges.");
  else if (l_returnCode==MOLE_NACK_POSITION)
    strcat(l_buf,"You are not in the correct position for that action!");
  ibInfoBox(g_aahWnd,l_buf,"Access Denied",IB_OK|IB_RUNHELP,NULL,HP_IB_MOLEEDITERROR_HELP);
  return;
}


/* This proc issues notification to "global" windows - windows which
 * ALWAYS need to be informed of changes to the structure tree. */
void edGlobalNotification() {
  PostMessage(g_awRootWnd,WM_USER_AREALIST_CHNG,0,0L);
  return;
}

/* This function is called for editable objects (area detail, mob, obj, etc).
 * This function opens an edit window and optionally fetches the data
 * (if it's unavailable) */
void edEditItem(int p_itemType,DSSTRUCT *p_area,DSSTRUCT *p_thing,HWND p_hWnd) {
  HWND l_hWnd;

  if (g_hoConnectionState!=HO_CONNECT_STATE_ONLINE)
    return;

#ifndef WIN32
  if (GetFreeSystemResources(GFSR_SYSTEMRESOURCES)<15) {
    ibInfoBox(g_aahWnd,"You are running short on system resources.\nTry closing some windows, eh?",
      "Whoah!",IB_OK|IB_RUNHELP,NULL,HP_IB_LOW_SYS_RES_WARN);
    /* Why do I do the following?? I don't understand. */
    switch (p_itemType) {
      case EDFI_MOBILE:
      case EDFI_WORLD:
      case EDFI_OBJECT:
        p_thing->sState=DS_NS_NOTAVAIL;
        break;
      case EDFI_AREADETAIL:
        DSSArea(p_area)->aDetail.rState=DS_NS_NOTAVAIL;
        break;
      case EDFI_RESET:
        DSSArea(p_area)->aRST.rState=DS_NS_NOTAVAIL;
        break;
    }
    edGlobalNotification();
    return;
  }
#endif

  switch (p_itemType) {
    case EDFI_AREADETAIL: /* uses p_area and p_thing */
      /* These are handled differently because we may not have a p_thing...
       * and without a p_thing, we don't have anyplace to keep a window
       * pointer!! So we store it in the reference. */
      if (p_area)
        l_hWnd=DSSArea(p_area)->aDetail.rEditWindow;
      else
        l_hWnd=NULL;

      if (l_hWnd) {
        if (p_hWnd) { /* Delete "prospective" window in favor of existing window */
          aaDestroyWindow(p_hWnd);
        }
      } else {
        /* Item is NOT being edited. First, make sure we have a window */

        if (!p_hWnd) {
          /* we don't have a window - so allocate one. */
          l_hWnd=CreateDialogParam(g_aahInst,MAKEINTRESOURCE(DIALOG_EDITADT),g_aahWnd,
            eaEditAreaDetailProc,(LPARAM)(DSSList(p_area)->lVNum));
          aaAddToDialogPool(l_hWnd);
          /* a WM_USER_INITDIALOGSUB is implied (ie happens) when the dialog is created. */
        } else {
          l_hWnd=p_hWnd;
          SendMessage(p_hWnd,WM_USER_INITDIALOGSUB,0,(LPARAM)(DSSList(p_area)->lVNum));
        }

        DSSArea(p_area)->aDetail.rEditWindow=l_hWnd;
        /* now check if we should reload */
        if ((!p_thing)||
            ((time(NULL)-DSSArea(p_area)->aDetail.rLoadTime)>g_edItemTimeout)) {
          /* reload data */
          edFetchItem(p_itemType,p_area,p_thing,l_hWnd); /* window will be notified from elsewhere */
          /* WM_USER_NEWDATA occurs when the item is finished being fetched. */
        } else {
          /* info is up-to-date, but we need to notify our window of that fact. */
          SendMessage(l_hWnd,WM_USER_NEWDATA,0,0L);
        }
      }
      /* now just highlight window & wait for data */
      SetFocus(l_hWnd);
      break;
    case EDFI_RESET:      /* uses p_area and p_thing */
      /* These are handled differently because we may not have a p_thing...
       * and without a p_thing, we don't have anyplace to keep a window
       * pointer!! */
      if (p_area)
        l_hWnd=DSSArea(p_area)->aRST.rEditWindow;
      else
        l_hWnd=NULL;

      if (l_hWnd) {
        if (p_hWnd) { /* Delete "prospective" window in favor of existing window */
          aaDestroyWindow(p_hWnd);
        }
      } else {
        /* Item is NOT being edited. First, make sure we have a window */

        if (!p_hWnd) {
          /* we don't have a window - so allocate one. */
          l_hWnd=CreateDialogParam(g_aahInst,MAKEINTRESOURCE(DIALOG_EDITRST),g_aahWnd,
            erEditRSTProc,(LPARAM)(DSSList(p_area)->lVNum));
          aaAddToDialogPool(l_hWnd);
          /* a WM_USER_INITDIALOGSUB is implied (ie happens) when the dialog is created. */
        } else {
          l_hWnd=p_hWnd;
          SendMessage(p_hWnd,WM_USER_INITDIALOGSUB,0,(LPARAM)(DSSList(p_area)->lVNum));
        }

        DSSArea(p_area)->aRST.rEditWindow=l_hWnd;
        /* now check if we should reload */
        if ((!p_thing)||
            ((time(NULL)-DSSArea(p_area)->aRST.rLoadTime)>g_edItemTimeout)) {
          /* reload data */
          edFetchItem(p_itemType,p_area,p_thing,l_hWnd); /* window will be notified from elsewhere */
          /* WM_USER_NEWDATA occurs when the item is finished being fetched. */
        } else {
          /* info is up-to-date, but we need to notify our window of that fact. */
          SendMessage(l_hWnd,WM_USER_NEWDATA,0,0L);
        }
      }
      /* now just highlight window & wait for data */
      SetFocus(l_hWnd);
      break;
    case EDFI_WORLD:
    case EDFI_MOBILE:
    case EDFI_OBJECT:
      /* fetch data, if it's not available */
      /* check for existing window - open one up if not */
      if (!(p_thing->sEditWindow)) {
        if (p_hWnd) {
          l_hWnd=p_hWnd;
          p_thing->sEditWindow=l_hWnd;
            SendMessage(l_hWnd,WM_USER_INITDIALOGSUB,0,(LPARAM)(DSSList(p_thing)->lVNum));
        } else {
          /* create window */
          if (p_itemType==EDFI_WORLD) {
            l_hWnd=CreateDialogParam(g_aahInst,MAKEINTRESOURCE(DIALOG_EDITWLD),g_aahWnd,
              ewEditWLDProc,(LPARAM)(DSSList(p_thing)->lVNum));
          } else if (p_itemType==EDFI_MOBILE) {
            l_hWnd=CreateDialogParam(g_aahInst,MAKEINTRESOURCE(DIALOG_EDITMOB),g_aahWnd,
              emEditMOBProc,(LPARAM)(DSSList(p_thing)->lVNum));
          } else {
            l_hWnd=CreateDialogParam(g_aahInst,MAKEINTRESOURCE(DIALOG_EDITOBJ),g_aahWnd,
              eoEditOBJProc,(LPARAM)(DSSList(p_thing)->lVNum));
          }
          aaAddToDialogPool(l_hWnd);
          p_thing->sEditWindow=l_hWnd;
        }
        if ( ((time(NULL)-p_thing->sLoadTime)>g_edItemTimeout)||
             ((p_thing->sType!=DS_STYPE_WORLD)&&(p_itemType==EDFI_WORLD))||
             ((p_thing->sType!=DS_STYPE_MOBILE)&&(p_itemType==EDFI_MOBILE))||
             ((p_thing->sType!=DS_STYPE_OBJECT)&&(p_itemType==EDFI_OBJECT))
           ) { /* reload item */
          edFetchItem(p_itemType,p_area,p_thing,p_hWnd); /* window will be notified from elsewhere */
        /* note that past this point, p_thing MAY BE INVALID!! */
        }
        SendMessage(l_hWnd,WM_USER_NEWDATA,0,0L);
      } else {
        /* there is already a window - so kill the "prospective" window. */
        if (p_hWnd) {
          aaDestroyWindow(p_hWnd);
        }
        l_hWnd=p_thing->sEditWindow;
      }
      SetFocus(l_hWnd);
      break;
    default:
      break;
  } /* switch */
  return;
}

DSSTRUCT *edWorldOf(long p_virtual) {
  DSSTRUCT *l_area,*l_thing;

  if (p_virtual<0)
    return NULL;

  if (g_awRootRef.rState==DS_NS_AVAIL) {
    for (l_area=g_awRootRef.rList;l_area;l_area=l_area->sNext) {
      if ((p_virtual>=DSSList(l_area)->lVNum)&&(p_virtual<=DSSList(l_area)->lVNum2)) {
        if (DSSArea(l_area)->aWLD.rState==DS_NS_AVAIL) {
          for (l_thing=DSSArea(l_area)->aWLD.rList;l_thing;l_thing=l_thing->sNext) {
            if (DSSList(l_thing)->lVNum==p_virtual)
              return l_thing;
          }
        } else { /* area list not available */
          break;
        }
      }
    }
  } /* else rootref not available */
  return NULL;
}

DSSTRUCT *edMobileOf(long p_virtual) {
  DSSTRUCT *l_area,*l_thing;

  if (p_virtual<0)
    return NULL;

  if (g_awRootRef.rState==DS_NS_AVAIL) {
    for (l_area=g_awRootRef.rList;l_area;l_area=l_area->sNext) {
      if ((p_virtual>=DSSList(l_area)->lVNum)&&(p_virtual<=DSSList(l_area)->lVNum2)) {
        if (DSSArea(l_area)->aMOB.rState==DS_NS_AVAIL) {
          for (l_thing=DSSArea(l_area)->aMOB.rList;l_thing;l_thing=l_thing->sNext) {
            if (DSSList(l_thing)->lVNum==p_virtual)
              return l_thing;
          }
        } else { /* area list not available */
          break;
        }
      }
    }
  } /* else rootref not available */
  return NULL;
}

DSSTRUCT *edObjectOf(long p_virtual) {
  DSSTRUCT *l_area,*l_thing;

  if (p_virtual<0)
    return NULL;

  if (g_awRootRef.rState==DS_NS_AVAIL) {
    for (l_area=g_awRootRef.rList;l_area;l_area=l_area->sNext) {
      if ((p_virtual>=DSSList(l_area)->lVNum)&&(p_virtual<=DSSList(l_area)->lVNum2)) {
        if (DSSArea(l_area)->aOBJ.rState==DS_NS_AVAIL) {
          for (l_thing=DSSArea(l_area)->aOBJ.rList;l_thing;l_thing=l_thing->sNext) {
            if (DSSList(l_thing)->lVNum==p_virtual)
              return l_thing;
          }
        } else { /* area list not available */
          break;
        }
      }
    }
  } /* else rootref not available */
  return NULL;
}

/* This proc returns the area encompassing the provided virtual number */
DSSTRUCT *edAreaOf(long p_virtual) {
  DSSTRUCT *l_area;

  if (p_virtual<0)
    return NULL;

  if (g_awRootRef.rState==DS_NS_AVAIL) {
    for (l_area=g_awRootRef.rList;l_area;l_area=l_area->sNext) {
      if ((p_virtual>=DSSList(l_area)->lVNum)&&(p_virtual<=DSSList(l_area)->lVNum2)) {
        return l_area;
      }
    }
  } /* else rootref not available */
  return NULL;
}

DSSTRUCT *edAreaDetailOf(long p_virtual) {
  DSSTRUCT *l_area;

  l_area=edAreaOf(p_virtual);
  if (l_area) {
    if (DSSArea(l_area)->aDetail.rState==DS_NS_AVAIL)
      l_area=DSSArea(l_area)->aDetail.rList;
    else
      l_area=NULL;
  }
  return l_area;
}

DSSTRUCT *edResetOf(long p_virtual) {
  DSSTRUCT *l_area;

  l_area=edAreaOf(p_virtual);
  if (l_area) {
    l_area=DSSArea(l_area)->aRST.rList;
  }
  return l_area;
}

/* requried parameters:
 * note: for all, p_hWnd is a dialog box (appropriate), and p_control is the
 * number for the dialog control. The TYPE of the dialog control is mentioned below.
 *
 * p_dataType      Params
 * ----------      ------
 * EDSGD_BYTE      p_control=text/edit p_data=DSBYTE* p_dataParam=<ignored>
 * EDSGD_SBYTE     p_control=text/edit p_data=DSSBYTE* p_dataParam=<ignored>
 * EDSGD_WORD      p_control=text/edit p_data=DSWORD* p_dataParam=<ignored>
 * EDSGD_UWORD     p_control=text/edit p_data=DSUWORD* p_dataParam=<ignored>
 * EDSGD_LWORD     p_control=text/edit p_data=DSLWORD* p_dataParam=<ignored>
 * EDSGD_ULWORD    p_control=text/edit p_data=DSULWORD* p_dataParam=<ignored>
 * EDSGD_FLAG      // meaningless - ignored
 * EDSGD_LIST      p_control=text/edit p_data=DSBYTE* p_dataParam=list_type  ** NOTE: TYPE OF LIST IS DSBYTE!! **
 * EDSGD_STR       p_control=text/edit p_data=DSSTR* p_dataParam=<OR of EDSGD_STR_* flags>
 * EDSGD_EXIT      p_control=first in an ordered cluster of controls   p_data=DSEXIT* p_dataParam=<ignored>
 * EDSGD_EXTRA     p_control=list box  p_data=DSREF* p_dataParam=<ignored>
 * EDSGD_PROPERTY  p_control=list box  p_data=DSREF* p_dataParam=<ignored>
 * EDSGD_RESET     p_control=first item in cell  p_data=DSSRESET* p_dataParam=<ignored>
 */
void edSetDlgItemData(HWND p_hWnd,int p_control,void *p_data,int p_dataType,int p_dataParam) {
  char l_buf[40];
  DSSTRUCT *l_thing;
  long l_i,l_j;
  DSFTL *l_ftl;
  DSBYTE l_byte;
  DSFLAG l_flag;

  if (p_dataType==EDSGD_BYTE)
    sprintf(l_buf,"%u",(unsigned int)(*((DSBYTE*)(p_data))));
  else if (p_dataType==EDSGD_SBYTE)
    sprintf(l_buf,"%i",(int)(*((DSSBYTE*)(p_data))));
  else if (p_dataType==EDSGD_WORD)
    sprintf(l_buf,"%i",(int)(*((DSWORD*)(p_data))));
  else if (p_dataType==EDSGD_UWORD)
    sprintf(l_buf,"%u",(unsigned int)(*((DSUWORD*)(p_data))));
  else if (p_dataType==EDSGD_LWORD)
    sprintf(l_buf,"%li",(long)(*((DSLWORD*)(p_data))));
  else if (p_dataType==EDSGD_ULWORD)
    sprintf(l_buf,"%lu",(unsigned long)(*((DSULWORD*)(p_data))));
  else if (p_dataType==EDSGD_FLAG) {
    return;
  } else if (p_dataType==EDSGD_LIST) {
    /* first, clear out any existing lines */
    l_i=1;
    while (l_i&&(l_i!=LB_ERR)) {
      l_i=SendDlgItemMessage(p_hWnd,p_control,CB_DELETESTRING,0,0L);
    }
    /* now fill'er up with our new stuff */
    l_i=0; /* we use l_i to hold our list value */
    for (l_ftl=dsFTLOf(&g_awFTList,p_dataParam);l_ftl;l_ftl=l_ftl->fNext) {
      l_j=SendDlgItemMessage(p_hWnd,p_control,CB_ADDSTRING,0,(LPARAM)(l_ftl->fName));
      if ((l_j!=LB_ERR)&&(l_j!=LB_ERRSPACE)) {
        SendDlgItemMessage(p_hWnd,p_control,CB_SETITEMDATA,(WPARAM)l_j,(LPARAM)l_i);
      }
      if (l_i==*((DSBYTE*)(p_data))) {
        SendDlgItemMessage(p_hWnd,p_control,CB_SETCURSEL,(WPARAM)l_j,0L);
      }
      l_i++;
    }
    return;
  } else if (p_dataType==EDSGD_STR) {
    if (p_dataParam&(EDSGD_STR_STRIPNL|EDSGD_STR_FORMAT)) {
      edStripCRLF(DSStr(p_data));
    }
    SetDlgItemText(p_hWnd,p_control,DSStr(p_data)->sData);
    return;
  } else if (p_dataType==EDSGD_EXIT) {
    /* setup ordered cluster of controls for the six exit directions */
    for (l_ftl=dsFTLOf(&g_awFTList,MOLE_LIST_DIR),l_i=0;(l_ftl)&&(l_i<IDC_EDITWLDEXITNUMELEMNTS);
      l_i++,l_ftl=l_ftl->fNext) {
      /* note hard coded # of exits - WHOOPS! */

      /* find this direction in our exit list */
      for (l_thing=DSRef(p_data)->rList;l_thing;l_thing=l_thing->sNext)
        if (DSSExit(l_thing)->eDir==l_i)
          break;
      EnableWindow(GetDlgItem(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS))),TRUE);
      if (l_thing) {
        CheckDlgButton(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)),1);
        for (l_j=1;l_j<IDC_EDITWLDEXITNUMELEMNTS;l_j++) {
          EnableWindow(GetDlgItem(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+l_j)),TRUE);
        }
        /* text */
        sprintf(l_buf,"%li",DSSExit(l_thing)->eWorld);
        SetDlgItemText(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+1),l_buf);
        sprintf(l_buf,"%li",DSSExit(l_thing)->eKeyObj);
        SetDlgItemText(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+2),l_buf);
        SetDlgItemText(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+3),DSSExit(l_thing)->eKey.sData);
        edStripCRLF(&(DSSExit(l_thing)->eDesc));
        SetDlgItemText(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+4),DSSExit(l_thing)->eDesc.sData);
      } else { /* clear this set of controls */
        CheckDlgButton(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)),0);
        for (l_j=1;l_j<IDC_EDITWLDEXITNUMELEMNTS;l_j++) {
          EnableWindow(GetDlgItem(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+l_j)),FALSE);
          if (l_j<5)
            SetDlgItemText(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+l_j),NULL);
        }
      }
    }
    return;
  } else if (p_dataType==EDSGD_EXTRA) {
    /* p_control had better be a list box! */
    /* first, empty out any existing entries in the list box */
    l_i=1;
    while (l_i&&(l_i!=LB_ERR)) {
      l_i=SendDlgItemMessage(p_hWnd,p_control,LB_DELETESTRING,0,0L);
    }
    /* now fill'er up with our new stuff */
    l_i=0; /* we use l_i to count the offset into our list - to help with finding it later */
    for (l_thing=DSRef(p_data)->rList;l_thing;l_thing=l_thing->sNext) {
      if (l_thing->sType==DS_STYPE_EXTRA) {
        if (strlen(DSSExtra(l_thing)->eKey.sData))
          l_j=SendDlgItemMessage(p_hWnd,p_control,LB_ADDSTRING,0,
            (LPARAM)(DSSExtra(l_thing)->eKey.sData));
        else
          l_j=SendDlgItemMessage(p_hWnd,p_control,LB_ADDSTRING,0,
            (LPARAM)("<no name>"));
        if ((l_j!=LB_ERR)&&(l_j!=LB_ERRSPACE)) {
          SendDlgItemMessage(p_hWnd,p_control,LB_SETITEMDATA,(WPARAM)l_j,(LPARAM)l_i);
        }
      }
      l_i++;
    }
    return;
  } else if (p_dataType==EDSGD_PROPERTY) {
    /* first, empty out any existing entries in the list box */
    l_i=1;
    while (l_i&&(l_i!=LB_ERR)) {
      l_i=SendDlgItemMessage(p_hWnd,p_control,LB_DELETESTRING,0,0L);
    }
    /* now fill'er up with our new stuff */
    l_i=0; /* we use l_i to count the offset into our list - to help with finding it later */
    for (l_thing=DSRef(p_data)->rList;l_thing;l_thing=l_thing->sNext) {
      if (l_thing->sType==DS_STYPE_PROPERTY) {
        if (strlen(DSSProperty(l_thing)->pKey.sData))
          l_j=SendDlgItemMessage(p_hWnd,p_control,LB_ADDSTRING,0,
            (LPARAM)(DSSProperty(l_thing)->pKey.sData));
        else
          l_j=SendDlgItemMessage(p_hWnd,p_control,LB_ADDSTRING,0,
            (LPARAM)("<no name>"));
        if ((l_j!=LB_ERR)&&(l_j!=LB_ERRSPACE)) {
          SendDlgItemMessage(p_hWnd,p_control,LB_SETITEMDATA,(WPARAM)l_j,(LPARAM)l_i);
        }
      }
      l_i++;
    }
    return;
  } else if (p_dataType==EDSGD_RESET) {
    if (p_data) {
      /* display this data */
      p_control-=IDC_EDITRSTCELLSTART; /* this adjusts our p_control for our offset */
      ShowWindow(GetDlgItem(p_hWnd,
        IDC_EDITRSTDELETE1+p_control),SW_SHOWNA);
      ShowWindow(GetDlgItem(p_hWnd,
        IDC_EDITRSTINSERT1+p_control),SW_SHOWNA);
      ShowWindow(GetDlgItem(p_hWnd,
        IDC_EDITRSTCMD1+p_control),SW_SHOWNA);
      ShowWindow(GetDlgItem(p_hWnd,
        IDC_EDITRSTIFNO1+p_control),SW_SHOWNA);
      ShowWindow(GetDlgItem(p_hWnd,
        IDC_EDITRSTIFYES1+p_control),SW_SHOWNA);
      ShowWindow(GetDlgItem(p_hWnd,
        IDC_EDITRSTCONDITIONAL1+p_control),SW_SHOWNA);
      ShowWindow(GetDlgItem(p_hWnd,
        IDC_EDITRSTDIVIDER1+p_control),SW_SHOWNA);
      if (DSSReset(p_data)->rIf)
        CheckRadioButton(p_hWnd,IDC_EDITRSTIFNO1+p_control,
          IDC_EDITRSTIFYES1+p_control,IDC_EDITRSTIFYES1+p_control);
      else
        CheckRadioButton(p_hWnd,IDC_EDITRSTIFNO1+p_control,
          IDC_EDITRSTIFYES1+p_control,IDC_EDITRSTIFNO1+p_control);
      /* let's show/hide our windows and set our values appropriately */
      switch (DSSReset(p_data)->rCmd) {
        case 'O': /* Object to room */                 /* offset 1 */
          SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+p_control,
            CB_SETCURSEL,(WPARAM)1,(LPARAM)0L);
          /* arg 1 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG11+p_control),SW_SHOWNA);
          /* arg 2 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2B1+p_control),SW_HIDE);
          /* arg 3 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG3A1+p_control,
            &(DSSReset(p_data)->rArg3),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3B1+p_control),SW_HIDE);
          /* arg 4 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG41+p_control,
            &(DSSReset(p_data)->rArg4),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG41+p_control),SW_SHOWNA);

          /* lastly, set up our field labels */
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL11+p_control,
            "Object");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL11+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL12+p_control,
            "Game Max");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL12+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL13+p_control,
            "Room");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL13+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL14+p_control,
            "Room Max");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL14+p_control),SW_SHOWNA);

          break;
        case 'M': /* Mobile to room */                 /* offset 0 */
          SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+p_control,
            CB_SETCURSEL,(WPARAM)0,(LPARAM)0L);
          /* arg 1 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG11+p_control),SW_SHOWNA);
          /* arg 2 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2B1+p_control),SW_HIDE);
          /* arg 3 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG3A1+p_control,
            &(DSSReset(p_data)->rArg3),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3B1+p_control),SW_HIDE);
          /* arg 4 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG41+p_control,
            &(DSSReset(p_data)->rArg4),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG41+p_control),SW_SHOWNA);

          /* lastly, set up our field labels */
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL11+p_control,
            "Mobile");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL11+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL12+p_control,
            "Game Max");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL12+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL13+p_control,
            "Room");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL13+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL14+p_control,
            "Room Max");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL14+p_control),SW_SHOWNA);
          break;
        case 'G': /* Give object to last mobile */     /* offset 3 */
          SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+p_control,
            CB_SETCURSEL,(WPARAM)3,(LPARAM)0L);
          /* arg 1 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG11+p_control),SW_SHOWNA);
          /* arg 2 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2B1+p_control),SW_HIDE);
          /* arg 3 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3A1+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3B1+p_control),SW_HIDE);
          /* arg 4 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG41+p_control),SW_HIDE);

          /* lastly, set up our field labels */
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL11+p_control,
            "Object");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL11+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL12+p_control,
            "Game Max");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL12+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL13+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL14+p_control),SW_HIDE);
          break;
        case 'P': /* Put object inside object */       /* offset 4 */
          SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+p_control,
            CB_SETCURSEL,(WPARAM)4,(LPARAM)0L);
          /* arg 1 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG11+p_control),SW_SHOWNA);
          /* arg 2 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2B1+p_control),SW_HIDE);
          /* arg 3 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG3A1+p_control,
            &(DSSReset(p_data)->rArg3),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3B1+p_control),SW_HIDE);
          /* arg 4 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG41+p_control),SW_HIDE);

          /* lastly, set up our field labels */
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL11+p_control,
            "Object");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL11+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL12+p_control,
            "Game Max");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL12+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL13+p_control,
            "Into Object");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL13+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL14+p_control),SW_HIDE);
          break;
        case 'E': /* Equip MOB with an object */       /* offset 5 */
          SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+p_control,
            CB_SETCURSEL,(WPARAM)5,(LPARAM)0L);
          /* arg 1 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG11+p_control),SW_SHOWNA);
          /* arg 2 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2B1+p_control),SW_HIDE);
          /* arg 3 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3A1+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3B1+p_control),SW_HIDE);
          /* arg 4 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG41+p_control),SW_HIDE);

          /* lastly, set up our field labels */
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL11+p_control,
            "Object");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL11+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL12+p_control,
            "Game Max");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL12+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL13+p_control,
            "Position");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL13+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL14+p_control),SW_HIDE);
          break;
        case 'D': /* Set door state */                 /* offset 6 */
          SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+p_control,
            CB_SETCURSEL,(WPARAM)6,(LPARAM)0L);
          /* arg 1 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG11+p_control),SW_SHOWNA);
          /* arg 2 */
          l_byte=(DSBYTE)(DSSReset(p_data)->rArg2);
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG2B1+p_control,
            &l_byte,EDSGD_LIST,MOLE_LIST_DIR);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2A1+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2B1+p_control),SW_SHOWNA);
          /* arg 3 */
          l_flag=(DSFLAG)(DSSReset(p_data)->rArg3);
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG2B1+p_control,
            &l_flag,EDSGD_FLAG,MOLE_LIST_EFLAG);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3A1+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3B1+p_control),SW_SHOWNA);
          /* arg 4 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG41+p_control),SW_HIDE);

          /* lastly, set up our field labels */
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL11+p_control,
            "Room");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL11+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL12+p_control,
            "Exit");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL12+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL13+p_control,
            "Door Flags");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL13+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL14+p_control),SW_HIDE);
          break;
        case 'R': /* Remove object from room */        /* offset 2 */
          SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+p_control,
            CB_SETCURSEL,(WPARAM)2,(LPARAM)0L);
          /* arg 1 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG11+p_control),SW_SHOWNA);
          /* arg 2 */
          edSetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2A1+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2B1+p_control),SW_HIDE);
          /* arg 3 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3A1+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3B1+p_control),SW_HIDE);
          /* arg 4 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG41+p_control),SW_HIDE);

          /* lastly, set up our field labels */
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL11+p_control,
            "Room");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL11+p_control),SW_SHOWNA);
          SetDlgItemText(p_hWnd,
            IDC_EDITRSTARGL12+p_control,
            "Object");
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL12+p_control),SW_SHOWNA);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL13+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL14+p_control),SW_HIDE);
          break;
        case '*': /* comment */                        /* offset 7 */
        default:
          SendDlgItemMessage(p_hWnd,
            IDC_EDITRSTCMD1+p_control,
            CB_SETCURSEL,(WPARAM)7,(LPARAM)0L);
          /* arg 1 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG11+p_control),SW_HIDE);
          /* arg 2 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2A1+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG2B1+p_control),SW_HIDE);
          /* arg 3 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3A1+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG3B1+p_control),SW_HIDE);
          /* arg 4 */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARG41+p_control),SW_HIDE);

          /* lastly, set up our field labels */
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL11+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL12+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL13+p_control),SW_HIDE);
          ShowWindow(GetDlgItem(p_hWnd,
            IDC_EDITRSTARGL14+p_control),SW_HIDE);
          break;
        }
    } else {
      /* just hide this cell */
      for (l_i=0;l_i<IDC_EDITRSTCELLELEMNTS;l_i++) {
        ShowWindow(GetDlgItem(p_hWnd,p_control+l_i),SW_HIDE);
      }
    }
    return;
  } else
    sprintf(l_buf,"<ERROR>");
  SetDlgItemText(p_hWnd,p_control,l_buf);
  return;
}

#pragma argsused
int edGetDlgItemData(HWND p_hWnd,int p_control,void *p_data,int p_dataType,int p_dataParam) {
  long      l_i,l_len;
  char      l_buf[12];
  char      *l_p;
  DSSTR     l_str;
  DSBYTE    l_tByte;
  DSSBYTE   l_tSByte;
  DSWORD    l_tWord;
  DSUWORD   l_tUWord;
  DSLWORD   l_tLWord;
  DSULWORD  l_tULWord;
  BOOL      l_changed;
  DSSTRUCT *l_struct;
  DSFLAG    l_tFlag;

  if (p_dataType==EDSGD_BYTE) {
    l_tByte=(*((DSBYTE*)(p_data)));
    (*((DSBYTE*)(p_data)))=(DSBYTE)GetDlgItemInt(p_hWnd,p_control,NULL,FALSE);
    return (l_tByte!=(*((DSBYTE*)(p_data))));
  } else if (p_dataType==EDSGD_SBYTE) {
    l_tSByte=(*((DSSBYTE*)(p_data)));
    (*((DSSBYTE*)(p_data)))=(DSSBYTE)GetDlgItemInt(p_hWnd,p_control,NULL,TRUE);
    return (l_tSByte!=(*((DSSBYTE*)(p_data))));
  } else if (p_dataType==EDSGD_WORD) {
    l_tWord=(*((DSWORD*)(p_data)));
    (*((DSWORD*)(p_data)))=(DSWORD)GetDlgItemInt(p_hWnd,p_control,NULL,TRUE);
    return (l_tWord!=(*((DSWORD*)(p_data))));
  } else if (p_dataType==EDSGD_UWORD) {
    l_tUWord=(*((DSUWORD*)(p_data)));
    (*((DSUWORD*)(p_data)))=(DSUWORD)GetDlgItemInt(p_hWnd,p_control,NULL,FALSE);
    return (l_tUWord!=(*((DSUWORD*)(p_data))));
  } else if (p_dataType==EDSGD_LWORD) {
    l_tLWord=(*((DSLWORD*)(p_data)));
    GetDlgItemText(p_hWnd,p_control,l_buf,12);
    sscanf(l_buf,"%li",p_data);
    return (l_tLWord!=(*((DSLWORD*)(p_data))));
  } else if (p_dataType==EDSGD_ULWORD) {
    l_tULWord=(*((DSULWORD*)(p_data)));
    GetDlgItemText(p_hWnd,p_control,l_buf,12);
    sscanf(l_buf,"%lu",p_data);
    return (l_tULWord!=(*((DSULWORD*)(p_data))));
  } else if (p_dataType==EDSGD_FLAG) {
    return FALSE; /* ignored */
  } else if (p_dataType==EDSGD_LIST) {
    l_tByte=(*((DSBYTE*)(p_data)));
    l_i=SendDlgItemMessage(p_hWnd,p_control,CB_GETCURSEL,0,0L);
    (*((DSBYTE*)(p_data)))=(DSBYTE)SendDlgItemMessage(p_hWnd,p_control,
      CB_GETITEMDATA,(WPARAM)(l_i),0L);
    return (l_tByte!=(*((DSBYTE*)(p_data))));
  } else if (p_dataType==EDSGD_STR) {
    /* Ok, we've got a problem here. The editbox from which we must read
     * text may contain 2 bytes of data, or 2000 bytes. We don't know, and
     * we don't have any way of finding out, either. Sure, we could use
     * EM_GETHANDLE, but there's no guarentee that we're using a MLE, so
     * we still have to be able to support single line windows. So here's
     * our icky solution: allocate a buffer and get the text. If the
     * text we get goes to the end of the buffer, assume there's more;
     * double our buffer & try again & again until our buffer is only
     * partially full. */
    l_i=(1<<5); /* starting size */
    dsStrClear(&l_str);
    l_len=l_i;
    while (((l_len+5)>GlobalSize(l_str.sMemory))&&(l_i<32767)){ /* the '5' is rather arbitrary, and the 32767 is so that we don't get carried away. */
      dsStrFree(&l_str);
      l_i<<=1;
      dsStrAlloc(&l_str,l_i);
      l_len=GetDlgItemText(p_hWnd,p_control,l_str.sData,(int)GlobalSize(l_str.sMemory));
    }
    /* at this point we SHOULD have some data in l_str which we can copy over. */
    (l_str.sData)[(unsigned int)GlobalSize(l_str.sMemory)-1]=0; /* Juuuuust in case */
    /* check for changed flag */
    if (((DSSTR*)(p_data))->sData)
      l_i=strcmp(((DSSTR*)(p_data))->sData,l_str.sData);
    else
      l_i=-1;
    /* free any existing data */
    dsStrFree((DSSTR*)(p_data));
    dsStrAlloc(((DSSTR*)(p_data)),l_len+2);
    strcpy(((DSSTR*)(p_data))->sData,l_str.sData);
    dsStrFree(&l_str);
    if (p_dataParam&EDSGD_STR_FORMAT) {
      edFormatWithWrap(p_data); /* will strip CRLF and reformat as required */
    } else if (p_dataParam&EDSGD_STR_STRIPNL) {
      edStripCRLF(p_data);
    }
    if (l_i)
      return TRUE; /* changes were made */
    return FALSE;
  } else if (p_dataType==EDSGD_EXIT) {
    /* p_control must point to the first of an ordered cluster of controls */
    /* go through directions */
    l_changed=FALSE;
    for (l_i=0;l_i<IDC_EDITWLDEXITNUMELEMNTS;l_i++) {
      /* let's see if we can find this exit */
      for(l_struct=DSRef(p_data)->rList;l_struct;l_struct=l_struct->sNext)
        if (DSSExit(l_struct)->eDir==l_i)
          break;
      if (IsDlgButtonChecked(p_hWnd,(int)(p_control+(l_i*IDC_EDITWLDEXITNUMELEMNTS)+0))) {
        if (!l_struct) {
          /* exit doesn't exist, but it should. Create it */
          l_struct=dsStructAlloc(DS_STYPE_EXIT);
          DSSExit(l_struct)->eDir=(DSBYTE)l_i;
          dsStructInsert(p_data, NULL, l_struct);
          l_changed|=TRUE;
        }
        l_changed|=edGetDlgItemData(p_hWnd,(int)(p_control+(IDC_EDITWLDEXITNUMELEMNTS*l_i)+1),
          &(DSSExit(l_struct)->eWorld),EDSGD_LWORD,0);
        GetDlgItemText(p_hWnd,(int)(p_control+(IDC_EDITWLDEXITNUMELEMNTS*l_i)+2),l_buf,12);
        l_buf[11]=0; /* just in case */
        for (l_p=l_buf;(*l_p)&&((*l_p<'0')||(*l_p>'9'));l_p++);
        if (!(*l_p)) {
          if (DSSExit(l_struct)->eKeyObj!=-1)
            l_changed|=TRUE;
          DSSExit(l_struct)->eKeyObj=-1;
        } else
          l_changed|=edGetDlgItemData(p_hWnd,(int)(p_control+(IDC_EDITWLDEXITNUMELEMNTS*l_i)+2),
            &(DSSExit(l_struct)->eKeyObj),EDSGD_LWORD,0);
        l_changed|=edGetDlgItemData(p_hWnd,(int)(p_control+(IDC_EDITWLDEXITNUMELEMNTS*l_i)+3),
          &(DSSExit(l_struct)->eKey),EDSGD_STR,0);
        l_changed|=edGetDlgItemData(p_hWnd,(int)(p_control+(IDC_EDITWLDEXITNUMELEMNTS*l_i)+4),
          &(DSSExit(l_struct)->eDesc),EDSGD_STR,0);
        edAppendCRLF(&(DSSExit(l_struct)->eDesc));
      } else {
        if (l_struct) {
          /* exit exists, but it shouldn't. Delete it */
          dsStructFree(l_struct);
          l_changed|=TRUE;
        }
      }
    }
    return l_changed;
  } else if (p_dataType==EDSGD_EXTRA) {
    return FALSE; /* ignored */
  } else if (p_dataType==EDSGD_PROPERTY) {
    return FALSE; /* ignored */
  } else if (p_dataType==EDSGD_RESET) {
    if (p_data) {
      p_control-=IDC_EDITRSTCELLSTART; /* this adjusts our p_control for our offset */
      l_changed=FALSE;
      /* check command */
      l_i=g_erResetCmd[SendDlgItemMessage(p_hWnd,
        p_control+IDC_EDITRSTCMD1,CB_GETCURSEL,0,0L)];
      if (DSSReset(p_data)->rCmd!=l_i) {
        DSSReset(p_data)->rCmd=(DSBYTE)l_i;
        l_changed|=TRUE;
      }
      l_i=IsDlgButtonChecked(p_hWnd,p_control+IDC_EDITRSTIFYES1);
      if (DSSReset(p_data)->rIf!=l_i) {
        DSSReset(p_data)->rIf=(DSWORD)l_i;
        l_changed|=TRUE;
      }
      /* let's show/hide our windows and set our values appropriately */
      switch (DSSReset(p_data)->rCmd) {
        case 'O': /* Object to room */                 /* offset 1 */
          /* arg 1 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          /* arg 2 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          /* arg 3 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG3A1+p_control,
            &(DSSReset(p_data)->rArg3),EDSGD_LWORD,0);
          /* arg 4 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG41+p_control,
            &(DSSReset(p_data)->rArg4),EDSGD_LWORD,0);
          break;
        case 'M': /* Mobile to room */                 /* offset 0 */
          /* arg 1 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          /* arg 2 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          /* arg 3 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG3A1+p_control,
            &(DSSReset(p_data)->rArg3),EDSGD_LWORD,0);
          /* arg 4 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG41+p_control,
            &(DSSReset(p_data)->rArg4),EDSGD_LWORD,0);
          break;
        case 'G': /* Give object to last mobile */     /* offset 3 */
          /* arg 1 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          /* arg 2 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          /* arg 3 */
          /* arg 4 */
          break;
        case 'P': /* Put object inside object */       /* offset 4 */
          /* arg 1 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          /* arg 2 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          /* arg 3 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG3A1+p_control,
            &(DSSReset(p_data)->rArg3),EDSGD_LWORD,0);
          /* arg 4 */
          break;
        case 'E': /* Equip MOB with an object */       /* offset 5 */
          /* arg 1 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          /* arg 2 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          /* arg 3 */
          /* arg 4 */
          break;
        case 'D': /* Set door state */                 /* offset 6 */
          /* arg 1 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          /* arg 2 */
          l_tByte=(DSBYTE)(DSSReset(p_data)->rArg2);
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG2B1+p_control,
            &l_tByte,EDSGD_LIST,MOLE_LIST_DIR);
          DSSReset(p_data)->rArg2=l_tByte;
          /* arg 3 */
          l_tFlag=(DSFLAG)(DSSReset(p_data)->rArg3);
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG2B1+p_control,
            &l_tFlag,EDSGD_FLAG,MOLE_LIST_EFLAG);
          DSSReset(p_data)->rArg3=l_tFlag;
          /* arg 4 */
          break;
        case 'R': /* Remove object from room */        /* offset 2 */
          /* arg 1 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG11+p_control,
            &(DSSReset(p_data)->rArg1),EDSGD_LWORD,0);
          /* arg 2 */
          l_changed|=edGetDlgItemData(p_hWnd,IDC_EDITRSTARG2A1+p_control,
            &(DSSReset(p_data)->rArg2),EDSGD_LWORD,0);
          /* arg 3 */
          /* arg 4 */
          break;
        case '*': /* comment */                        /* offset 7 */
        default:
          /* arg 1 */
          /* arg 2 */
          /* arg 3 */
          /* arg 4 */
          break;
        }
      return l_changed;
    } else
      return FALSE;
  } /* else */
  return FALSE;
}

void edSendItem(int p_itemType,DSSTRUCT *p_area,DSSTRUCT *p_thing,HWND p_hWnd) {
  EDREQ *l_req,*l_newreq;
  char *l_data;
  long l_dataLen;
  HGLOBAL l_dataMemory;

  if (g_hoConnectionState!=HO_CONNECT_STATE_ONLINE)
    return;
  /* we always generate a new request */
  l_newreq=edReqAlloc();
  l_newreq->rItemType=p_itemType;
  l_newreq->rWnd=p_hWnd;
  if (p_area)
    l_newreq->rArea=DSSList(p_area)->lVNum;
//    strcpy(l_newreq->rArea,DSSList(p_area)->lName.sData);
  else
//    l_newreq->rArea[0]=0;
    l_newreq->rArea=-1L;
  if (p_thing)
    l_newreq->rVNum=DSSList(p_thing)->lVNum;
  else
    l_newreq->rVNum=-1L;

  l_newreq->rSend=TRUE; /* we're SENDING, not REQUESTING */

  /* first make sure item hasn't already been sent, and if so, add a new
   * requests on to include the new window (if an edit has been requested */
  for (l_req=g_edReqList;l_req;l_req=l_req->rNext) {
    if (l_req!=l_newreq)
      if (l_req->rItemType==l_newreq->rItemType)
        if (l_req->rVNum==l_newreq->rVNum)
          if (l_req->rSend==l_newreq->rSend)
            if (l_newreq->rArea==l_req->rArea) {
              dbPrint("Repeat Request found");
              break; /* we found a match */
            }
  }
  if (l_req) {
    /* and the most important step is.... */
    l_newreq->rID=l_req->rID;
    return;
  }

  /* nope, this is new data! Generate a MOLE request! */
  switch (p_itemType) {
    case EDFI_AREALIST: /* needs no other params */
      break;
    case EDFI_WLDLIST:  /* needs p_areaName */
      break;
    case EDFI_MOBLIST:  /* needs p_areaName */
      break;
    case EDFI_OBJLIST:  /* needs p_areaName */
      break;
    case EDFI_AREADETAIL:  /* needs p_area */
      if (!p_area) break;
      if (p_area->sType!=DS_STYPE_AREA) break;
      p_thing=DSSArea(p_area)->aDetail.rList;
      l_newreq->rVNum=DSSList(p_area)->lVNum;
      if (!p_thing) break;
      if (p_thing->sType!=DS_STYPE_AREADETAIL) break;

      /* first, package up our data into a nice big buffer */
      l_dataMemory=paPackAreaDetail(p_area,&l_dataLen,&l_data);
      if ((!l_data)||(l_dataLen<0)) break;

      /* next, mark our item as "pending" and refresh any displays */
      p_thing->sState=DSSArea(p_area)->aDetail.rState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go send our data */
      l_newreq->rID=mpReqSubmitEx(MOLE_CMD_ADTL,l_data,l_dataLen);
      paPackCleanup(l_dataMemory);
      return;
    case EDFI_RESET:   /* needs p_area */
      if (!p_area) break;
      if (p_area->sType!=DS_STYPE_AREA) break;
      l_newreq->rVNum=DSSList(p_area)->lVNum;

      /* first, package up our data into a nice big buffer */
      l_dataMemory=paPackReset(p_area,&l_dataLen,&l_data);
      if ((!l_data)||(l_dataLen<0)) break;

      /* next, mark our item as "pending" and refresh any displays */
      DSSArea(p_area)->aRST.rState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go send our data */
      l_newreq->rID=mpReqSubmitEx(MOLE_CMD_RLST,l_data,l_dataLen);
      paPackCleanup(l_dataMemory);
      return;
    case EDFI_WORLD:   /* needs p_virtualNum */
      if (!p_thing) break;
      if (p_thing->sType!=DS_STYPE_WORLD) break;

      /* first, package up our data into a nice big buffer */
      l_dataMemory=paPackWorld(p_thing,&l_dataLen,&l_data);
      if ((!l_data)||(l_dataLen<0)) break;

      /* next, mark our item as "pending" and refresh any displays */
      p_thing->sState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go send our data */
      l_newreq->rID=mpReqSubmitEx(MOLE_CMD_WDTL,l_data,l_dataLen);
      paPackCleanup(l_dataMemory);
      return;
    case EDFI_MOBILE:  /* needs p_virtualNum */
      if (!p_thing) break;
      if (p_thing->sType!=DS_STYPE_MOBILE) break;

      /* first, package up our data into a nice big buffer */
      l_dataMemory=paPackMobile(p_thing,&l_dataLen,&l_data);
      if ((!l_data)||(l_dataLen<0)) break;

      /* next, get rid of any existing mobile data - replace existing structure
       * with a list.  */
      /* We do this because our mobile data may be changed by the server
       * depending on what kinds of rules it needs to enforce for consistency
       * etc. So if we need to edit the sucker again, re-fetch it. */
      /* I decided against it - let it stay as long as a user wishes it
       * to be cached. If this is a problem, uncomment the following. */
//      if (p_thing->sType!=DS_STYPE_LIST) {
//        p_thing=dsChangeListType(p_thing,DS_STYPE_LIST);
//      }
      /* next, mark our item as "pending" and refresh any displays */
      p_thing->sState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go send our mobile data */
      l_newreq->rID=mpReqSubmitEx(MOLE_CMD_MDTL,l_data,l_dataLen);
      paPackCleanup(l_dataMemory);
      return;
    case EDFI_OBJECT:  /* needs p_virtualNum */
      if (!p_thing) break;
      if (p_thing->sType!=DS_STYPE_OBJECT) break;

      /* first, package up our data into a nice big buffer */
      l_dataMemory=paPackObject(p_thing,&l_dataLen,&l_data);
      if ((!l_data)||(l_dataLen<0)) break;

      /* next, mark our item as "pending" and refresh any displays */
      p_thing->sState=DS_NS_PENDING;
      edGlobalNotification();

      /* and now go send our data */
      l_newreq->rID=mpReqSubmitEx(MOLE_CMD_ODTL,l_data,l_dataLen);
      paPackCleanup(l_dataMemory);
      return;
    default:
      break;
  }
  /* error */
  edReqFree(l_newreq);
  return;
}

/* The following will remove CR/LF from the string */
/* Note we don't worry about resizing the STR because
 * with this proc we could only ever DECREASE the size,
 * never INCREASE it. */
/* Note this proc will substitute in a space if a CR/LF
 * between two words is being removed. */
void edStripCRLF(DSSTR *p_str) {
  char *l_new,*l_old;

  if (!p_str)
    return;
  if (!(p_str->sData))
    return;

  l_new=l_old=(p_str->sData)-1;
  do {
    l_old++;
    if ((*l_old==0x0A)||(*l_old==0x0D)) {
      if (l_new > p_str->sData ) {
        if (*l_new==' ') {
          /* it's OK; there's a space already. We can safely delete this CR/LF */
        } else {
          /* There's no space previous to this - add one */
          l_new++;
          *l_new=' ';
        }
      } else {
        /* we're at the start of the string anyways - no space is required */
      }
    } else {
      l_new++;
      *l_new=*l_old;
    }
  } while(*l_old);
}

#define ED_FWW_LINELENGTH (79-1) /* length minus 1 (cause we start at 0, remember!)

/* The following will reformat the string to 80 columns, with wrap */
void edFormatWithWrap(DSSTR *p_str) {
  char *l_new,*l_old,*l_mark;
  int l_size;
  int l_linepos,l_linepossave;
  DSSTR l_str;

  if (!p_str)
    return;
  if (!(p_str->sData))
    return;

  /* first, take out any existing CR/LFs */
  edStripCRLF(p_str);

  /* Figure out how big of a buffer we need. */
  /* To do this, we run through the wrap logic once, then a second
   * time later to actually create the wrapped buffer */
  l_old=l_mark=p_str->sData;
  l_size=l_linepos=l_linepossave=0;
  while (*l_old) {
    /* first, find the end of the current word */
    while ((*l_old)&&(*l_old!=' ')&&(l_linepos<(ED_FWW_LINELENGTH-1))) {
      l_old++;
      l_linepos++;
    }
    if (l_linepos>=(ED_FWW_LINELENGTH-1)) {
      /* we either have to wrap, or split this word */
      if (l_linepossave==0) {
        /* no, this is the first word - we have to split the word RIGHT HERE */
        l_size+=l_linepos; /* for word */
        l_size+=2; /* for CRLF */
        l_linepos=0;
        l_mark=l_old;
        l_linepossave=l_linepos;
      } else {
        /* this is NOT the first word - we have to wrap. */
        /* put a CRLF right at l_linepossave. */
        /* Note this will leave the previous space - oh well. */
        l_old=l_mark;
        l_size+=l_linepossave;
        l_size+=2; /* for CRLF */
        l_linepos=0;
        l_linepossave=l_linepos;
      }
    } else if (*l_old) {
      /* This word fits. Update pointers & start next word. */
      l_old++;
      l_linepos++;
      l_mark=l_old;
      l_linepossave=l_linepos;
    }
    if (!(*l_old)) { /* in the event we have a space as our last character */
      l_size+=l_linepos;
    }
  }
  l_size+=2; /* trailing CRLF */
  l_size++; /* for trailing 0x00 */

  /* allocate new buffer for this sucker */
  if (!dsStrAlloc(&l_str,l_size))
    return;

  /* allocate new buffer for this sucker */
  if (!dsStrAlloc(&l_str,l_size))
    return;

  /* now copy over with formatting */
  l_old=l_mark=p_str->sData;
  l_new=l_str.sData;
  l_linepos=l_linepossave=0;
  while (*l_old) {
    /* first, find the end of the current word */
    while ((*l_old)&&(*l_old!=' ')&&(l_linepos<(ED_FWW_LINELENGTH-1))) {
      l_old++;
      l_linepos++;
    }
    if (l_linepos>=(ED_FWW_LINELENGTH-1)) {
      /* we either have to wrap, or split this word */
      if (l_linepossave==0) {
        /* no, this is the first word - we have to split the word RIGHT HERE */
        for (;l_mark<l_old;l_mark++,l_new++)
          *l_new=*l_mark;
        *l_new=0x0D;
        l_new++;
        *l_new=0x0A;
        l_new++;
        l_linepos=0;
        l_mark=l_old;
        l_linepossave=l_linepos;
      } else {
        /* this is NOT the first word - we have to wrap. */
        /* put a CRLF right at l_linepossave. */
        /* Note this will leave the previous space - oh well. */
        *l_new=0x0D;
        l_new++;
        *l_new=0x0A;
        l_new++;
        l_old=l_mark;
        l_linepos=0;
        l_linepossave=l_linepos;
      }
    } else if (*l_old) {
      /* This word fits. Update pointers & start next word. */
      l_old++;
      for (;l_mark<l_old;l_mark++,l_new++)
        *l_new=*l_mark;
      l_linepos++;
      l_linepossave=l_linepos;
    }
    if (!(*l_old)) { /* in the event we have a space as our last character */
      for (;l_mark<=l_old;l_mark++,l_new++)
        *l_new=*l_mark;
//      l_size+=l_linepos;
    }
  }
 // l_size+=2; /* trailing CRLF */
 // l_size++; /* for trailing 0x00 */

  /* and lastly, turf old str and replace with new one */
  dsStrFree(p_str);
  /* copy over l_str (new string) to p_str */
  dsStrCopy(&l_str,p_str);
  /* make sure we've got a CRLF on the end of the sucker */
  edAppendCRLF(p_str);
  /* and we're done! */
}

/* The following will append a CR/LF if it's not already there */
void edAppendCRLF(DSSTR *p_str) {
  char *l_new,*l_old;
  int l_size;
  DSSTR l_str;

  if (!p_str)
    return;
  if (!(p_str->sData))
    return;

  /* first, strip any existing CR/LF currently present */
  l_old=p_str->sData;
  while (*l_old)
    l_old++;
  if (l_old>p_str->sData)
    l_old--;
  while (((*l_old==0x0A)||(*l_old==0x0D))&&(l_old>p_str->sData)) {
    *l_old=0;
    l_old--;
  }
  l_size=(int)(l_old-p_str->sData)+4; /* +1 because l_old is 1 less than the end.
                                       * +2 for the CR/LF we're about to add. *
                                       * +1 for the terminator */
  if (l_size>GlobalSize(p_str->sMemory)) {
    /* alloc new (bigger) string & copy data over */
    if (dsStrAlloc(&l_str,l_size)) {
      l_new=l_str.sData-1;
      l_old=p_str->sData-1;
      do {
        l_old++;
        l_new++;
        *l_new=*l_old;
      } while (*l_old);
      /* discard old string */
      dsStrFree(p_str);
      /* copy over l_str (new string) to p_str */
      dsStrCopy(&l_str,p_str);
      l_old=l_new-1;
    } else {
      return;
    }
  }
  /* append CR/LF */
  l_old++;
  *l_old=0x0D;
  l_old++;
  *l_old=0x0A;
  l_old++;
  *l_old=0x00; /* terminate */
}
