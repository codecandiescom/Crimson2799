
#ifndef _ANSITERM_H_
#define _ANSITERM_H_

#define AT_WIND_CLASS_NAME "AnsiTerm:BCL 98/05/02"

/* window extra data offsets */
#define ATWND_ATTERMSEG 0  /* pointer to the window's ATTERM structure */
#define ATWND_ATTERMOFF 4  /* pointer to the window's ATTERM structure */

/* Emulation Modes */
#define AT_EMU_NONE  0
#define AT_EMU_VT100 1

/* constants */
#define TBSTR_TITLE_LEN  200     /* max # chars in title of window */
#define TBSTR_ENVSECTION  200    /* max size of environment string */
#define TBMACRO_MAXLEN 100
#define AT_ESC_PARAM_MAX 50
#define AT_STATEVAL_DEFAULT -1

/* effect bits */
#define AT_FONTEFFECT_BOLD      (1<<0)
#define AT_FONTEFFECT_UNDERLINE (1<<1)
#define AT_FONTEFFECT_BLINK     (1<<2)
#define AT_FONTEFFECT_REVERSE   (1<<3)

/* aNotify messages */
#define TBNOTIFY_MENUCLOSE   0x0001 /* user has choosen "close" from the menu
																		 * or system menu */
#define TBNOTIFY_USERINPUT   0x0002 /* user input is available for transmission */
#define TBNOTIFY_OPTIONS     0x0003 /* user options have changed                */
#define TBNOTIFY_OPTIONSHELP 0x0004 /* options Help requested                   */
#define TBNOTIFY_FONT        0x0005 /* user has selected a new display font     */
#define TBNOTIFY_FONTHELP    0x0006 /* font picker help requested               */
#define TBNOTIFY_MACROS      0x0007 /* user changed function key macros         */
#define TBNOTIFY_MACROSHELP  0x0008 /* function key macros help requested       */
#define ATNOTIFY_WINDOWSIZE  0x0009 /* Terminal window size has changed         */

typedef struct ATTERMtag {
	HINSTANCE aInst;         /* instance to which this term belongs */
	HGLOBAL aMemory;         /* our memory handle */
	HGLOBAL aOutBufMemory;   /* output buffer memory */
	unsigned char *aOutBuf;            /* output buffer */
	unsigned long aOutBufSize;        /* output buffer size */
	unsigned char *aOutBufSockTail;    /* outgoing socket position (tail) */
//  char *aOutBufLETail;    /* local echo position */
	unsigned char *aOutBufHead;        /* head position */
	HWND *aWnd;               /* our window */
	TBTERMBUF *aTermBuf;      /* our terminal buffer */
	long aShowX,aShowY;   /* the screen coordinate of the top-left shown cell */
	long aSizeX,aSizeY;   /* size of visible screen - updated by atAdjustTerm */
	long aScroll;         /* # lines to scroll up (neg for down) next WM_PAINT */
	HFONT aFont;          /* DO NOT CHANGE DIRECTLY - CALL atChangeFont!!! */
  HFONT aFontUnderline; /* DO NOT CHANGE DIRECTLY - CALL atChangeFont!!! */
	LOGFONT aLogFont;    /* DO NOT CHANGE DIRECTLY - CALL atChangeFont!!! */
	BOOL aLogFontValid;  /* TRUE if aLogFont is valid */
	long aFontX,aFontY;   /* assumed font cell width,height. DO NOT CHANGE DIRECTLY - CALL atChangeFont!!! */
	int* aState;          /* terminal state for the parsing state machine - see ATSTATE_* below */
//	long aState1,aState2; /* general-purpose sub-state variable (use varries) */
	long aStateVal[AT_ESC_PARAM_MAX];   /* parsed state variable array (if req'd) */
	long aStateValPos;    /* for use with above */
	char aEnvSection[TBSTR_ENVSECTION];/* environment (window position) section */
	char aTitle[TBSTR_TITLE_LEN]; /* Window title */
	void (*aNotify)(struct ATTERMtag *p_Term, int p_msg);    /* function called to notify of out-bound data is available */
	LRESULT CALLBACK (*aAuxMsgHandler)(struct ATTERMtag *p_Term,HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam); /* function hook to operate unhandled window messages */
	long aLE_BSCount;     /* used to allow only x backspaces during local echo */
	/* aSeparateInputLine settings */
	RECT aScrollRect;     /* aSeparateInputLine: rect maintained of scroll region in the terminal -- NOTE this is only used if aSeparateInputLine is specified */
	long aInputWndDY;     /* aSeparateInputLine: Height of input window */
	long aInputBorderDY;  /* aSeparateInputLine: height of border between input window & main terminal */
	int  aInputFColour;   /* input line foreground colour */
	int  aInputBColour;   /* input line background colour */
	HGLOBAL aInputBufMemory;  /* input memory handle */
	unsigned char *aInputBuf; /* input line buffer */
	unsigned long aInputBufSize;
	unsigned char *aInputCurLine; /* start of current line text */
	unsigned char *aInputCurLineEnd; /* ending max character for current line */
	unsigned char *aInputCurPos;  /* Cursor position -- insert mode is always on */
	unsigned char *aInputDispCurPos; /* start of DISPLAYED position */
	unsigned char *aInputScrollBackPos; /* scroll back through previous cmds */
	long aInputScrollback; /* when they push up/down arrow, cycle through command history */
				/* The input line buffer looks like this: (0 = 0x00 character)
				 *
				 * d y0cmd a0cmd b0cmd c0current command00000000000000000cmd w0cmd x0cm
				 * ^   ^                 ^  ^       ^                    ^
				 *     aInputScrollBack  |  |       aInputCurPos         aInputCurLineEnd
				 *                  Pos  |  aInputDispCurPos
				 *                       aInputCurLine
	/* Mouse hilite settings */
	BOOL aMouseHiliteCapture; /* 0 normally, 1 if we've captured the mouse input */
	int aCaptureLastX,aCaptureLastY; /* the last location of the mouse, relative to client screen */
	/* terminal emulation options - this includes NVR settings */
	unsigned long aEmulation; /* AT_EMU_* setting */
	BOOL aSeparateInputLine;  /* if TRUE, input line is separate at bottom of screen */
	BOOL aLocalEcho;     /* TRUE for local echo */
	BOOL aBell;          /* audible bell */
	unsigned char aBackSpaceChar; /* backspace key character */
	unsigned char aDeleteChar;    /* delete character */

	/* Other terminal settings */
	unsigned char aFnKeyMacro[48][TBMACRO_MAXLEN];  /* plain, shift, control, shift-control */

	/* active emulation options - this is the "volatile" selectable settings and emulation settings */
	BOOL aVLocalEcho;
	BOOL aLED[4];                   /* TRUE=on        FALSE=off      */
	/* modes                             SET            RESET          */
	BOOL aLFwithNLMode;             /* TRUE=nl        FALSE=lf       */
	BOOL aCursorKeyAppMode;         /* TRUE=app       FALSE=cursor   */
	BOOL aANSIMode;                 /* TRUE=ANSI      FALSE=VT52     */
	BOOL aColumnMode;               /* TRUE=132       FALSE=80       */
	BOOL aScrollingMode;            /* TRUE=Smooth    FALSE=Jump     */
	BOOL aScreenMode;               /* TRUE=Reverse   FALSE=Normal   */
	BOOL aOriginMode;               /* TRUE=Relative  FALSE=Absolute */
	BOOL aWraparoundMode;           /* TRUE=on        FALSE=off      */
	BOOL aAutoRepeatMode;           /* TRUE=on        FALSE=off      */
	BOOL aInterlaceMode;            /* TRUE=on        FALSE=off      */
	BOOL aGraphicProcMode;          /* TRUE=on        FALSE=off      */  /* Graphic Processor */
	BOOL aKeypadMode;               /* TRUE=App       FALSE=Numeric  */
	char aCharacterSet[4];
	int aCurCharacterSet;
	int aSCSType; /* temporary holding place for state machine -- NOT a terminal setting!!! */

	/* ECMA-48 Codes and Modes */
	int aSIMD; /* Select Implicit Movement Direction */
	int aSLH;  /* Set Line Home - home position (starts at 1) */
	int aSLL;  /* Set Line Limit */
} ATTERM;

BOOL atInitAnsiTermApplication(HINSTANCE p_hInst);
BOOL atInitAnsiTerm(void);
void atShutdownAnsiTerm(void);
ATTERM *atAllocTerm(HINSTANCE p_hInst, HWND p_hParent,long p_BufX,long p_BufY,long p_ScrX,long p_ScrY, char *p_EnvSection, char *p_Title);
void atFreeTerm(ATTERM *p_Term);
void atAdjustTerm(ATTERM *p_Term,long p_Scrollback);
void atUpdateTermTitle(ATTERM *p_Term,char *p_NewTitle);
void atAdjustScrollbar(ATTERM *p_Term);
void atShowTerm(ATTERM *p_Term);
void atHideTerm(ATTERM *p_Term);
BOOL atIsTermVisible(ATTERM *p_Term);
int atSendTerm(void* p_TermA, unsigned char *p_Buf, int p_BufLen);
void atClearTerm(ATTERM *p_Term);
void atNotifyFn(unsigned long p_ID, unsigned int p_Operation, long p_P0,long p_P1, long p_P2, long p_P3);
LRESULT CALLBACK _export atAnsiTermProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
void atChangeFont(ATTERM *p_Term, HFONT p_font, LOGFONT *p_logfont);
void atUpdateCaret(ATTERM *p_Term);
void atScrollTermTo(ATTERM *p_Term,BOOL p_Rel,long p_Val);
long atReadTermInput(ATTERM *p_Term, unsigned char *p_Buf, unsigned long p_BufLen);
long atUnReadTermInput(ATTERM *p_Term, unsigned char *p_Buf, unsigned long p_BufLen);
long atWriteTermInput(ATTERM *p_Term, unsigned char *p_Buf, unsigned long p_BufLen);
void atSendTermInput(ATTERM *p_Term, unsigned char *p_Buf, unsigned long p_BufLen);
BOOL CALLBACK _export atAnsiTermOptionsProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
BOOL CALLBACK _export atATFnKeyProc(HWND p_hWnd, UINT p_message,WPARAM p_wParam, LPARAM p_lParam);
void atSaveTermSettings(ATTERM *p_Term, int p_Settings, char *p_Section);
void atLoadTermSettings(ATTERM *p_Term, int p_Settings, char *p_Section);
void atResetTerm(ATTERM *p_Term);
void atResetSeparateInputLine(ATTERM *p_Term);

#endif /* _ANSITERM_H_ */


