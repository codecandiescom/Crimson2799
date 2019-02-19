extern WORD Compile(STR *,STR **,THING *);
extern void CompileInit();

/* return codes for Compile() */
#define COMPILE_OK                     0 /* Everything went ok */
#define COMPILE_SYNTAX_ERROR           1 /* Syntax error in source code */
#define COMPILE_INTERNAL_ERROR         2 /* Internal compile.c error */

/* program limitations */
#define CVAR_NAME_LENGTH              32
#define CMAX_VAR                     512 /* max # vars */
