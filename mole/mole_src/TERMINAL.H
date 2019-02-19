#ifdef WIN32
// terminal.h
// two-letter descriptor: tr
//
// This module handles the text terminal for the MUD connection.
BOOL trInitTerminalApplication(HINSTANCE p_hInst);
BOOL trInitTerminal(void);
void trShutdownTerminal(void);
void trClearTerminal(void);
void trSendTerminal(char *p_buf,unsigned long p_buflen);
void trShowTerminal(void);
void trHideTerminal(void);
BOOL trIsTerminalVisible(void);
int trReadTerminalInput(char *p_buf,int p_buflen);
void trNotifyFn(void *p_Term, int p_msg);

#else /* WIN32 */

BOOL trInitTerminalApplication(HINSTANCE p_hInst);
BOOL trInitTerminal(void);
void trShutdownTerminal(void);
void trClearTerminal(void);
void trSendTerminal(char *p_buf,unsigned long p_buflen);
void trShowTerminal(void);
void trHideTerminal(void);
BOOL trIsTerminalVisible(void);
BOOL CALLBACK _export trDialogTerminalProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
int trReadTerminalInput(char *p_buf,int p_buflen);

#endif /* WIN32 */
