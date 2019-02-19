// editflag.h
// two-character identifier: ef
// Flag editing tool functions.

#include"dstruct.h"

#define EP_STRLEN 50

typedef struct EFPARAMtag {
  char *pWindowTitle;
  DSFLAG *pFlag;
  int pFType;
} EFPARAM;

BOOL efEditFlag(HWND p_hWnd,DSFLAG *p_flag,int p_fType,char *p_WindowTitle);
BOOL CALLBACK _export efEditFlagProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
