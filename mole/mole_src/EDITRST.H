// EditRST module
// two-character identifier: er

BOOL CALLBACK _export erEditRSTProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
DSSTRUCT *erFindReset(HWND p_hWnd,long p_VNum,int p_Control);
void erDefaultReset(DSSTRUCT *p_thing);


extern char g_erResetCmd[8];


