// areawnd.h
// Two-letter Module Descriptor: aw
#include"moledefs.h"
#include"dstruct.h"

#define AW_WIND_CLASS_NAME "MOLE Area Hierarchy"

extern DSREF g_awRootRef; /* the root of all evil, er, I mean, of all structures */
extern DSFTL *g_awFTList; /* may as well keep this here too. */
extern DSFTL *g_awFTLObjType; /* and this too */
extern int g_awRootTop; /* the # of the top line in the root window */
extern HWND g_awRootWnd;
extern char g_awMUDName[100];

BOOL awInitAreaWnd(void);
void awShutdownAreaWnd(void);
BOOL awInitAreaWndApplication(void);
BOOL awInitAreaWndSecondApp(void);
LRESULT CALLBACK _export awAreaWndProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
void awResetConnection(void);
int awFindHighlightItem(DSSTRUCT **p_area, DSSTRUCT **p_thing, int *p_type);
void awShowAreaWindow(void);
void awHideAreaWindow(void);
BOOL awIsAreaWindowVisible(void);
COLORREF awGetColor(int p_color);

