// clipbrd.h
// Two-letter Module Descriptor: cb

BOOL cbInitClipboard(void);
void cbShutdownClipboard(void);
BOOL cbCopyStruct(DSSTRUCT *p_Struct);
BOOL cbPasteStruct(DSSTRUCT *p_Struct);

