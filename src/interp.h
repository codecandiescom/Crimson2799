extern void InterpDump(STR *,THING *);
extern WORD Interp(STR *,THING *,THING *,THING *,STR *,LWORD *,EXIT *);
extern void InterpInit(LWORD iMaxInstr);
extern void InterpStackAlloc(LWORD iStackSize,LWORD iMaxParameter,LWORD iLocalVar);
extern void InterpSnoop(BYTE *msg, THING *thing);
extern void InterpSnoopStr(BYTE *msg);
extern void InterpSnoopStack(ULWORD pos);

typedef struct InterpVarType {
  BYTE    iDataType;  /* type of data */
  LWORD   iInt;       /* integer data */
  void    *iPtr;      /* pointer data */
} INTERPVARTYPE; 

typedef struct InterpStack {
  INTERPVARTYPE *iVariable;  /* used internally by interp - don't use!! */
  BYTE    iDataType;  /* type of data - set for you by interp. */
  LWORD   iInt;       /* integer data goes here */
  void    *iPtr;      /* pointer data (thing, str, extra) goes here */
} INTERPSTACK; 


#define INTERP_MAX_INSTR 1000 /* Maximum # instructions Interp will execute */
                              /* before automatically exiting. This is to   */
                              /* guard against infinite loops like          */
                              /* "while (1);" in the code.                  */
