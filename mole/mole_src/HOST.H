#include"winsock.h"

#define HO_STRLEN 50

#define HO_INI_SECT "HOSTS"

#define HO_CONNECT_STATE_OFFLINE 1
#define HO_CONNECT_STATE_LOGIN   2
#define HO_CONNECT_STATE_ONLINE  3
#define HO_CONNECT_STATE_LOGOFF  4

#define HO_INVALID_PACKET_ID 0

typedef struct HOHOSTtag {
  char hDescription[HO_STRLEN]; /* host description */
  char hIP[HO_STRLEN]; /* host name */
  long hPort;   /* host port */
  char hLoginID[HO_STRLEN]; /* login username / ID */
  char hPassword[HO_STRLEN]; /* login password */
  struct HOHOSTtag *hNext; /* next host */
  HGLOBAL hMemory; /* handle for global memory block */
  int hID; /* host unique ID - arbitrary but lets us uniquely identify hosts */
} HOHOST;

BOOL hoInitHost(void);
void hoShutdownHost(void);
void hoConnectHost(void);
void hoDisconnectHost(BOOL p_force);
void hoEncryptPassword(char *p_password,char *p_encrypted);
void hoDecryptPassword(char *p_password,char *p_decrypted);
int hoGetHostID(void);
BOOL CALLBACK _export hoDialogNewHostProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
BOOL CALLBACK _export hoDialogOpenHostProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
BOOL CALLBACK _export hoDialogConnectProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
void hoSocketHandler(WPARAM p_wParam, LPARAM p_lParam);
void hoSocketKilled(void);
BOOL hoSocketFriendlyDisconnect(void);
BOOL CALLBACK _export hoDialogLoginProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);

extern char g_hoHostName[50]; /* server DNS name (if available) */
extern unsigned long g_hoHostIP; /* server IP address */
extern long g_hoHostPort; /* server port */
extern unsigned long g_hoTime; /* time connection to host was made */
extern SOCKET g_hoSock; /* socket for connection to host */
extern BOOL g_hoWSAStartup; /* startup successfully called */
extern BOOL g_hoConnectedToHost; /* set to TRUE if connected to host */
extern int g_hoConnectionState; /* connection is a state machine - offline,login,online,logoff */
extern HWND g_hoLoginWnd; /* handle of login dialog box - if it's open */

