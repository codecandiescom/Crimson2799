// Edit Manager
// two character identifier: ed

#include"dstruct.h"

/* length of string constants in this module */
#define ED_STRLEN 256

typedef struct EDREQtag {
  int rItemType; /* requested item type */
  unsigned long rID; /* MoleProt Request ID */
  HWND rWnd; /* Window to notify when data is received */
  long rArea; /* virtual number of area */
  long rVNum; /* virtual number */
  BOOL rSend; /* FALSE if requesting data, TRUE if providing data */
  struct EDREQtag *rNext,*rPrevious;
  HGLOBAL rMemory; /* memory pointer for this struct */
} EDREQ;

/* FetchItem constants */
#define EDFI_NULL       0
#define EDFI_AREALIST   1
#define EDFI_WLDLIST    2
#define EDFI_MOBLIST    3
#define EDFI_OBJLIST    4
#define EDFI_EDITABLE   5 /* edit-able objects must come after this point */
#define EDFI_AREADETAIL 5
#define EDFI_RESET      6
#define EDFI_WORLD      7
#define EDFI_MOBILE     8
#define EDFI_OBJECT     9
#define EDFI_ENDEDITABLE 9 /* last editable item */

/* Set/GetDlgItemData data types */
#define EDSGD_BYTE       1
#define EDSGD_SBYTE      2
#define EDSGD_WORD       3
#define EDSGD_UWORD      4
#define EDSGD_LWORD      5
#define EDSGD_ULWORD     6
#define EDSGD_FLAG       7 /* meaningless - ignored */
#define EDSGD_LIST       8 /* for selection list box (eg: male,female,sexless listbox) */
#define EDSGD_STR        9 /* for text windows - names, descriptions, etc. */
#define EDSGD_EXIT      10 /* for Set function only - updates list box */
#define EDSGD_EXTRA     11 /* for Set function only - updates list box */
#define EDSGD_PROPERTY  12 /* for Set function only - updates list box */
#define EDSGD_RESET     13 
/* Set/GetDlgItemData data parms (some-not all) */
#define EDSGD_STR_KEEPNL     (0)
#define EDSGD_STR_STRIPNL    (1<<0)
#define EDSGD_STR_KEEPFORMAT (0)
#define EDSGD_STR_FORMAT     (2<<0)  /* overrides EDSGD_STR_STRIPNL */

/* Edit flags (for use with edit windows & the g_edFlag atom) */
#define EDFLAG_CHANGED (1<<0)

extern long g_edItemTimeout,g_edListTimeout;
extern ATOM g_edLongHi,g_edLongLo,g_edFlag;

BOOL edInitEdit(void);
void edShutdownEdit(void);
EDREQ *edReqAlloc(void);
void edReqFree(EDREQ *p_req);
void edFetchItem(int p_itemType,DSSTRUCT *p_area,DSSTRUCT *p_thing,HWND p_hWnd);
void edSendItem(int p_itemType,DSSTRUCT *p_area,DSSTRUCT *p_thing,HWND p_hWnd);
void edPacketHasArrived(unsigned long p_pktID,unsigned long p_Cmd,unsigned long p_returnCode);
void edGlobalNotification(void);
void edResetConnection(void);
void edEditItem(int p_itemType,DSSTRUCT *p_area,DSSTRUCT *p_thing,HWND p_hWnd);
DSSTRUCT *edWorldOf(long p_virtual);
DSSTRUCT *edMobileOf(long p_virtual);
DSSTRUCT *edObjectOf(long p_virtual);
DSSTRUCT *edAreaOf(long p_virtual);
DSSTRUCT *edAreaDetailOf(long p_virtual);
DSSTRUCT *edResetOf(long p_virtual);
void edSetDlgItemData(HWND p_hWnd,int p_control,void *p_data,int p_dataType,int p_listType);
BOOL edGetDlgItemData(HWND p_hWnd,int p_control,void *p_data,int p_dataType,int p_dataParam);
void edEditError(WPARAM p_wParam,LPARAM p_lParam);
void edStripCRLF(DSSTR *p_str);
void edFormatWithWrap(DSSTR *p_str);
void edAppendCRLF(DSSTR *p_str);

