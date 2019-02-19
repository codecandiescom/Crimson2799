extern WORD Decomp(STR *,STR**,THING *);
extern WORD DecompDisassemble(STR *,THING *);
extern void DecompInit();
extern void DecompStrFree(STR *);

/* return codes for Decomp */
#define DECOMP_OK             0  /* Everything went fine */
#define DECOMP_INTERNAL_ERROR 1  /* Internal decomp.c error */
#define DECOMP_INPUT_ERROR    2  /* Invalid data passed to function */
