/* See function.c for a description as to how to add a function to the 
 * C function library.
 */

#define FMAX_FUNCTION_PARAMETER 20  /* maximum number of function params */
#define FBUF_COMMON_SIZE 256 /* common buffer size used throughout function.c */

#define FNPROC(proc) void (proc)(INTERPSTACK *Return,INTERPVARTYPE *Param)

/* function table */
typedef struct FTableType {
  BYTE   *fText;                               /* function name */
  FNPROC(*fFunction);                          /* function to call */
  WORD    fDataType;                           /* data type of returned value */
  BYTE    fParamType[FMAX_FUNCTION_PARAMETER]; /* parameter data type */
} FTABLETYPE;

extern FTABLETYPE fTable[];
extern void FunctionFlushRegistry();
extern void FunctionCheckRegistry(INTERPVARTYPE *Param);

/*** C4 Function Constants ***/

/* FnObjectRedeem */
#define OR_CHIP (1<<0)
#define OR_BIO  (1<<1)
#define OR_RICH (1<<2)
