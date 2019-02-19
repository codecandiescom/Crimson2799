BOOL dlInitDialog(void);
void dlShutdownDialog(void);
void dlCentreDialogBox(HWND p_hWnd);
void dlCascadeDialogBox(HWND p_hWnd);
BOOL CALLBACK _export dl3DSubClassProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
void dlAddToDialogPool(HWND p_hWnd);
BOOL dlIsDialogMessage(MSG* p_msg);
void dlRemoveFromDialogPool(HWND p_hWnd);
void dlDestroyWindow(HWND p_hWnd);

