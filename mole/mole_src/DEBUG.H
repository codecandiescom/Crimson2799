/* Uncomment this if you want a "finished product" - no debug capabilities, just shell */
#define DB_DEBUG_NODEBUG


#ifdef DB_DEBUG_NODEBUG
#define dbInitDebugApplication(x,y)
#define dbInitDebugSecondApp(x,y)
#define dbInitDebug(x)               (1)
#define dbShutdownDebug()
#define dbPrint(x)
#define dbPrintNum(x,y) 
#else /* DB_DEBUG_NODEBUG */
void dbInitDebugApplication(HINSTANCE p_hInst,void (FAR *p_wrapup)());
void dbInitDebugSecondApp(HINSTANCE p_hInst,void (FAR *p_wrapup)());
BOOL dbInitDebug(HINSTANCE p_hInst);
void dbShutdownDebug(void);
extern void _export WINAPI dbPrintNum(char *p_msg,int p_len);
#define dbPrint(x) dbPrintNum(x,0x7fff)
extern LRESULT CALLBACK dbDebugProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
#endif /* DB_DEBUG_NODEBUG */
