// molem.h identifier: aa

/* Window class names */
#define AA_WIND_CLASS_NAME "MOLE_Window"

#define AAFILENAME_MAX_LENGTH 200  /* used everywhere as the max length of a file name. Very Important!*/

/* user-defined window messages */
#define WM_USER_OPENHOST_UPDATE   WM_USER+0x0001
#define WM_USER_CONNECT_CONNECT   WM_USER+0x0002
#define WM_USER_HOST_SOCKET       WM_USER+0x0003 /* socket state has changed! */
#define WM_USER_LOGIN_IDENTIFIED  WM_USER+0x0004 /* for use with login dialog box */
#define WM_USER_LOGIN_COMPLETE    WM_USER+0x0005 /* for use with login dialog box */
#define WM_USER_TERMINAL_INPUT    WM_USER+0x0006 /* There's terminal input ready for reading */
#define WM_USER_AREALIST_CHNG     WM_USER+0x0007 /* general-purpose "something has changed - maybe new data, maybe data deleted */
#define WM_USER_AREALIST_SCROLL   WM_USER+0x0008
#define WM_USER_NEWDATA           WM_USER+0x0009 /* general-purpose "there's new data available" */
#define WM_USER_CHANGEDDATA       WM_USER+0x000A /* for when data has "changed" in existing window. */
#define WM_USER_INITDIALOGSUB     WM_USER+0x000B /* repeatable part of processing for WM_INITDIALOG message */
#define WM_USER_EDITERROR         WM_USER+0x000C /* callback for edit error */

void aaWrapup(void);
BOOL aaInitInstance(LPSTR p_lpCmdLine, int p_nCmdShow);
BOOL aaInitSecondApp(void);
BOOL aaInitApplication(void);
int WINAPI WinMain(HINSTANCE p_hInstance, HINSTANCE p_hPrevInstance,LPSTR p_lpCmdLine, int p_nCmdShow);
void aaAddToDialogPool(HWND p_hWnd);
void aaRemoveFromDialogPool(HWND p_hWnd);
void aaDestroyWindow(HWND p_hWnd); /* encapsulates steps required to delete/destroy a dialog edit window */

extern HINSTANCE  g_aahInst;              /* Handle to main instance data*/
extern HACCEL     g_aahAccTable;          /* handle to accelerator table */
extern char       g_aaWorkingDir[AAFILENAME_MAX_LENGTH]; /* NSCap working directory */
extern HWND       g_aahWnd;               /* Handle to main window */
extern char       g_aaInitFile[AAFILENAME_MAX_LENGTH]; /* .ini file */
extern char       g_aaHelpFile[AAFILENAME_MAX_LENGTH]; /* .hlp file */
extern UINT       g_aaCommDlgHelp;        /* Window message sent by command dialog boxes */

