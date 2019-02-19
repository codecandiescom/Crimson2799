// enviromt.h
// Two-letter descriptor: en

BOOL enInitEnvironment(char *p_inifile);
void enShutdownEnvironment(void);
WINDOWPLACEMENT *enRestoreWindowPlacement(char *p_section,int p_defLeft,int p_defTop,int p_defRight,int p_defBottom);
void enSaveWindowPlacement(char *p_section,HWND p_hWnd);
void enSaveLogFont(char *p_Section, LOGFONT *p_LogFont);
void enRestoreLogFont(char *p_Section, LOGFONT *p_LogFont);
void enSaveLong(char *p_Section, char *p_Entry, unsigned long p_Long);
unsigned long enRestoreLong(char *p_Section, char *p_Entry, unsigned long p_Default);
void enSaveInt(char *p_Section, char *p_Entry, unsigned int p_Int);
unsigned int enRestoreInt(char *p_Section, char *p_Entry, unsigned int p_Default);
void enSaveString(char *p_Section, char *p_Entry, unsigned char *p_Str);
void enRestoreString(char *p_Section, char *p_Entry, char *p_Default,unsigned char *p_Str, unsigned long p_StrLen);
void enSaveLogFont(char *p_Section, LOGFONT *p_LogFont);
void enRestoreLogFont(char *p_Section, LOGFONT *p_LogFont);
void enSaveYesNo(char *p_Section, char *p_Entry, BOOL p_yesno);
BOOL enRestoreYesNo(char *p_Section, char *p_Entry, BOOL p_Default);

