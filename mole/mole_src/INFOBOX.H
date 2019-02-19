/* Window Styles */
#define IB_YES          (1L<<0)
#define IB_NO           (1L<<1)
#define IB_OK           (1L<<2)
#define IB_CANCEL       (1L<<3)
#define IB_ABORT        (1L<<4)
#define IB_RETRY        (1L<<5)
#define IB_IGNORE       (1L<<6)
#define IB_HELP         (1L<<7)
#define IB_HELPDEF      (1L<<8) // help keyword defined (useless without IB_HELP)
#define IB_RUNHELP      IB_HELP|IB_HELPDEF // This doesn't close the box when help is pressed -
                                           // instead it calls up the specified HELP
#define IB_OKCANCEL     IB_OK|IB_CANCEL
#define IB_YESNO        IB_YES|IB_NO
#define IB_YESNOCANCEL  IB_YES|IB_NO|IB_CANCEL
#define IB_RETRYCANCEL  IB_RETRY|IB_CANCEL
#define IB_ABORTRETRYIGNORE IB_ABORT|IB_RETRY|IB_IGNORE
#define IB_SYSTEMMODAL  0L // undefined
#define IB_APPLMODAL    0L
#define IB_TASKMODAL    0L
#define IB_DEFBUTTON1   0L
#define IB_DEFBUTTON2   0L
#define IB_DEFBUTTON3   0L

#define IB_ICONASTERISK    0L
#define IB_ICONEXCLAMATION 0L
#define IB_ICONHAND        0L
#define IB_ICONINFORMATION 0L
#define IB_ICONQUESTION    0L
#define IB_ICONSTOP        0L

/* Info Box return values */
#define IB_RYES    IDYES
#define IB_RNO     IDNO
#define IB_ROK     IDOK
#define IB_RCANCEL IDCANCEL
#define IB_RABORT  IDABORT
#define IB_RRETRY  IDRETRY
#define IB_RIGNORE IDIGNORE
#define IB_RHELP   IDHELP

int ibInfoBox(HWND p_hWnd,LPCSTR p_text,LPCSTR p_title,unsigned long p_style,LPCSTR p_icon,DWORD p_help);
BOOL CALLBACK _export ibInfoBoxProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);

