// clipbrd.h
// Two-letter Module Descriptor: cb
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

// This module manages the copy/paste functionality between structure
// elements etc.
//
// This module probably would fit in the edit.c module, but that
// module is way to big already. Thus I spawned off a new module
// for just this stuff.

// NOTE NOTE
// This module's functionality is slated to be moved to the server.
// It would work much better that way.
//
// NOTE NOTE NOTE
// It HAS been moved. This module just saves a "copy" command
// for any given THING, then when a "paste" is issued, it
// issues a copy command at the server, copying the "copy" to
// the "paste" thing. Note that separate "copy" things may exist
// for MOBs, OBJs, etc. To avoid confusion, we arbitrarily
// allow only one "copy" thing.

#include<windows.h>
#include"moledefs.h"
#include"molem.h"
#include"molerc.h"
#include"dstruct.h"
#include"infobox.h"
#include"edit.h"
#include"debug.h"
#include"clipbrd.h"

/* Global variables for clipboard */
//DSSTRUCT *g_cbClipboardStruct;

BOOL cbInitClipboard() {
//  g_cbClipboardStruct=NULL;
  return TRUE;
}

void cbShutdownClipboard() {
  return;
}

// cbCopyStruct will copy the p_Struct to the clipboard for a later paste function
// Note that this function currently only supports DSSMOBILE,DSSOBJECT & DSSWORLD
#pragma argsused
BOOL cbCopyStruct(DSSTRUCT *p_Struct) {
/*  if (!p_Struct)
    return FALSE;

  switch(p_Struct->sType) {
    case DS_STYPE_MOBILE:
    case DS_STYPE_OBJECT:
    case DS_STYPE_WORLD:
      dsStructFree(g_cbClipboardStruct);
      g_cbClipboardStruct=dsStructAlloc(p_Struct->sType);
      dsStructCopy(p_Struct,g_cbClipboardStruct);
      break;
    case DS_STYPE_RESET:
    case DS_STYPE_AREADETAIL:
    default:
      ibInfoBox(g_aahWnd,"Copying this item of data is not supported yet.",
        "Darn it all!",IB_OK,NULL,0L);
      return FALSE;
  }  */
  return TRUE;
}

// cbCopyStruct will copy the p_Struct to the clipboard for a later paste function
// Note that this function currently only supports DSSMOBILE,DSSOBJECT & DSSWORLD
#pragma argsused
BOOL cbPasteStruct(DSSTRUCT *p_Struct) {
//  if (!p_Struct)
//    return FALSE;
//  if (!g_cbClipboardStruct) {
//    ibInfoBox(g_aahWnd,"The clipboard is empty. Try copying something.",
//      "Darn it all!",IB_OK,NULL,0L);
//    return FALSE;
//  }
//
//  if (p_Struct->sType!=g_cbClipboardStruct->sType) {
//    ibInfoBox(g_aahWnd,"The item on the clipboard is not the same as your destination item!",
//      "Darn it all!",IB_OK,NULL,0L);
//    return FALSE;
//  }
//
//  switch(p_Struct->sType) {
//    case DS_STYPE_MOBILE:
//    case DS_STYPE_OBJECT:
//    case DS_STYPE_WORLD:
//      /* Copy the struct over */
//      dsStructCopy(g_cbClipboardStruct,p_Struct);
//      /* now update any open edit windows */
//      if (p_Struct->sEditWindow)
//        SendMessage(p_Struct->sEditWindow,WM_USER_NEWDATA,0,0L);
//      /* and refresh everything else */
//      edGlobalNotification();
//      /* and we're done! */
//      break;
//    case DS_STYPE_RESET:
//    case DS_STYPE_AREADETAIL:
//    default:
//      ibInfoBox(g_aahWnd,"Copying this item of data is not supported yet.",
//        "Darn it all!",IB_OK,NULL,0L);
//      return FALSE;
//  }
  return TRUE;
}
