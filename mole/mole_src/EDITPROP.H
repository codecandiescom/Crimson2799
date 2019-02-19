// editprop.h
// two-character identifier: ep
// Property editing tool functions.

#include"dstruct.h"

#define EP_STRLEN 50

typedef struct EPPARAMtag {
  char *pWindowTitle;
  DSSTRUCT *pProperty;
} EPPARAM;

BOOL epEditProperty(HWND p_hWnd,void *p_ptr,int p_sType,int p_newType);
BOOL CALLBACK _export epEditPropProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);

