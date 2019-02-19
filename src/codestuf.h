/* version numbers for code.c, compile.c, interp.c, decomp.c */
#define CODE_MAJOR_VERSION 1
#define CODE_MINOR_VERSION 9

/* definitions concerning the compiled program structure */
#define CODE_PROG_HEADER        "<code>"
#define CODE_PROG_HEADER_LENGTH        7 /* including 0x00 at end of string */
#define CODE_BLK_NULL                  0 /* terminating/final block         */
#define CODE_BLK_VAR                   1 /* variable type def'n block       */
#define CODE_BLK_VAR_NAME              2 /* variable names (ascii) block    */
#define CODE_BLK_CODE                  3 /* code block                      */

/* Just a brief description of our compiled code before we go on:           */
/* The compiled code is held in a PROPERTY. The name of the PROPERTY, the   */
/* PROPERTY.pKey field, contains the routine name for the code. This routine*/
/* name determines when the routine is called. The name always starts with  */
/* the `@' sign. eg:  @tick will be run every system tick event             */
/*                                                                          */
/* Now, the actual code is contained in PROPERTY.pDesc. pDesc is type STR.  */
/* NOTE THAT to properly handle the compiled code, since it is in binary    */
/* format, you have to go by PROPERTY.eDesc.sLen to get the proper length.  */
/* the code may (and in fact probably does) contain 0x00 characters embeded.*/
/* So don't go using functions like strlen() on compiled code, kay!         */
/*                                                                          */
/* OK, so the actual layout of the code in PROPERTY.pDesc.sText is thus:    */
/* [ CODE_PROG_HEADER ]   -  the header of the program. The header is a     */
/* length:                   short text blip so if someone tries to look    */
/*   CODE_PROG_HEADER_LENGTH at it via text routines, they'll get a short   */
/*                           little non-descript message like "<code>".     */
/* [ block header type ]   - immediately following the header are the data  */
/* length: 1 byte            blocks. The first data block is usually        */
/*                           the CODE_BLK_VAR variable def'n block.         */
/* [ pointer to next block ] The pointer to the next block is a 16-bit      */
/* length: 2 bytes           unsigned offset to the location in sText where */
/*                           the start of the next block can be found.      */
/* [ block guts ]          - the guts of the block                          */
/* length: variable                                                         */
/*                                                                          */
/* So, we have, in summary: [ header ] [ block ] [ block ] [ block ... ]    */
/* The last block should be CODE_BLK_NULL to indicate the end of the        */
/* data file. This is the only block def'n that does not have or need a     */
/* 'next block' offset right behind the block header.                       */
/* So, other than that, the blocks can be in any order - the interpreter    */
/* and decompiler do not depend on them being in any specific order.        */
/* GOOD LUCK.                                                               */


/* These are the op-codes for the compiled program */

#define C__EXPAND  (1<<7) /* This bit is set if it's an expanding op-code */
                          /* NOTE: Version 1.xx does NOT support          */
                          /* expanding op-codes!                          */
#define C__OPS     (1<<6) /* This bit divides operators from the rest.    */
                          /* Operators are + - * / * & | etc.  This is    */
                          /* convenient simply because they are all so    */
                          /* alike (in the way they're handled) and there */
                          /* are just so many of them!                    */

/* Operators - listed in order of prescedence in an equation! Small number */
/* means small prescedence. Thus, * is bigger than +, so + has smaller #.  */
/* NOTE: put all 'equates' (ie =, +=, etc) BEFORE COP_EQU. This is used    */
/*       to determine if an operation is an equate operation in various    */
/*       parts of compile.c. You can then go:                              */ 
/*            if (instruction<=COP_EQU) {blah} else {blah}.                */
/*       This is helpful since equates are treated a bit differently than  */
/*       the rest of the operators.                                        */
    /* Assignment operators */
#define COP_EQUSR   C__OPS+0x01 /* >>=  */
#define COP_EQUSL   C__OPS+0x02 /* <<=  */
#define COP_EQUOR   C__OPS+0x03 /* |=   */
#define COP_EQUXOR  C__OPS+0x04 /* ^= aka \=  */
#define COP_EQUAND  C__OPS+0x05 /* &=   */
#define COP_EQUSUB  C__OPS+0x06 /* -=   */
#define COP_EQUADD  C__OPS+0x07 /* +=   */
#define COP_EQUMOD  C__OPS+0x08 /* %=   */
#define COP_EQUDIV  C__OPS+0x09 /* /=   */
#define COP_EQUMULT C__OPS+0x0A /* *=   */
#define COP_EQU     C__OPS+0x0B /* =    */

    /* Logical Operators */
#define COP_BOR     C__OPS+0x10 /* ||   */
#define COP_BAND    C__OPS+0x11 /* &&   */

    /* Bitwise Operators */
#define COP_OR      C__OPS+0x12 /* |    */
#define COP_XOR     C__OPS+0x13 /* ^ aka \   */
#define COP_AND     C__OPS+0x14 /* &    */

    /* Equality operators */
#define COP_BEQU    C__OPS+0x15 /* ==   */
#define COP_BNEQU   C__OPS+0x16 /* !=   */
 
    /* Relational operators */
#define COP_GEQ     C__OPS+0x17 /* >=   */
#define COP_GR      C__OPS+0x18 /* >    */
#define COP_LEQ     C__OPS+0x19 /* <=   */
#define COP_LS      C__OPS+0x1A /* <    */

    /* Shift operators */
#define COP_SR      C__OPS+0x1B /* >>   */
#define COP_SL      C__OPS+0x1C /* <<   */

    /* Additive operators */
#define COP_SUB     C__OPS+0x1D /* -    */
#define COP_ADD     C__OPS+0x1E /* +    */

    /* Multiplicative operators */
#define COP_MOD     C__OPS+0x1F /* %    */
#define COP_DIV     C__OPS+0x20 /* /    */
#define COP_MULT    C__OPS+0x21 /* *    */

    /* Unary operators */
#define COP_ASUB    C__OPS+0x22 /* `after' -- (ie: a--)  */ 
#define COP_AADD    C__OPS+0x23 /* `after' ++ (ie: a++)  */
#define COP_BSUB    C__OPS+0x24 /* `before'-- (ie: --a)  */
#define COP_BADD    C__OPS+0x25 /* `before'++ (ie: ++a)  */
#define COP_COMP    C__OPS+0x26 /* ~ aka ` */
#define COP_NEG     C__OPS+0x27 /* - (ie: -<value>  -a, -982, etc.     */
                                /* THIS IS DIFFERENT FROM `-' in `1-3' */
#define COP_NOT     C__OPS+0x28 /* !    */
#define COP_TOP_OP  C__OPS+0x29 /* marker - highest prescidence possible */
                                /* NOTE: THIS IS NOT A VALID OP-CODE! */

/* The Rest - stack functions, subroutine calls, goto's, etc. */
#define COP_NULL      0x00  /* null (invalid) instruction */
#define COP_TERM      0x01  /* Terminate execution */
#define COP_EXEC      0x02  /* Exec function */
#define COP_EXECR     0x03  /* Exec w/ return code */
#define COP_PUSHV     0x04  /* Push Value (32-bit signed by default)*/
#define COP_PUSHV1    0x05  /* Push Value ( 8-bit signed)  */
#define COP_PUSHV2    0x06  /* Push Value (16-bit signed) */
#define COP_PUSHV4    0x07  /* Push Value (32-bit signed) */
#define COP_PUSHL     0x08  /* Push Local variable */
#define COP_PUSHG     0x09  /* Push Global variable */
#define COP_PUSHP     0x0A  /* Push Private variable */
#define COP_PUSHPT    0x0B  /* Push pointer constant */
#define COP_GOTO      0x0C  /* Goto <offset> */
#define COP_IFZ       0x0D  /* If Zero, branch (ie if non-zero, don't branch) */
#define COP_WHILEZ    0x0E  /* While Zero, branch (ie while non-zero, don't) */
#define COP_COMMENT   0x0F  /* While Zero, branch (ie while non-zero, don't) */
#define COP_POP       0x10  /* Pop top value off stack (value is turfed) */
#define COP_NOP       0x11  /* No operation. Nothing done. */

/* Data Type definitions */
#define CDT_UNDEF  0
#define CDT_NULL   0 /* this must be zero - function table now depends on
                         * it, (the function table wasnt too readable b4) 
                         */
#define CDT_INT    1 /* all ints are signed, 32-bit integers */
#define CDT_STR    2 /* pointer to STR */
#define CDT_THING  3 /* pointer to THING */
#define CDT_EXTRA  4 /* pointer to EXTRA */
#define CDT_EXIT   5 /* pointer to EXIT */

/* Why Cam used 31 I cant tell ya, makes for a lot of space in list though */
#define CDT_ETC   31 /* used in parameter def'n where an open-ended list of
                        any data type variables are valid */
extern BYTE *cDataType[];


/* Variable Domains */
#define CDOMAIN_UNDEF   0
#define CDOMAIN_CONST   0
#define CDOMAIN_LOCAL   1
#define CDOMAIN_GLOBAL  2
#define CDOMAIN_PRIVATE 3

/* Local Table - System variable section */
extern ULWORD cSystemVariable; /* all variables below this in the local var */
/*#define CSYSTEM_VARIABLE 32*/   /* all variables below this in the local var */
                               /* table are system variables.               */
extern ULWORD cSystemVariableStatic;/* all system variables at or below this */
/*#define CSYSTEM_VARIABLE_STATIC 25 *//* all system variables at or below this */
                               /* in the local var table are non-changeable */
                               /* (ie: they cannot be used for lvalues)     */

/* Local Table - location of Sys vars in local table */
/*
#define CODE_LOCAL_NULL           0
#define CODE_LOCAL_TNULL          1
#define CODE_LOCAL_ENULL          2
#define CODE_LOCAL_TRUE           3
#define CODE_LOCAL_FALSE          4
#define CODE_LOCAL_YES            5
#define CODE_LOCAL_NO             6
#define CODE_LOCAL_EVENT_THING    7
#define CODE_LOCAL_CODE_THING     8
#define CODE_LOCAL_TIME           9
#define CODE_LOCAL_SEGMENT       10
#define CODE_LOCAL_COMMAND       11
#define CODE_LOCAL_SEND_ROOM     12
#define CODE_LOCAL_SEND_SRC      13
#define CODE_LOCAL_SEND_DST      14
#define CODE_LOCAL_SEND_VISIBLE  15
#define CODE_LOCAL_SEND_AUDIBLE  16
#define CODE_LOCAL_SEND_CAPFIRST 17
#define CODE_LOCAL_BLOCK_CMD     26
*/

/* Reserved Word - data */
/* NOTE: these correspond to the reserved word's position in cResWord[]!! */
#define CRES_INT       0
#define CRES_STR       1
#define CRES_THING     2
#define CRES_LOCAL     3
#define CRES_GLOBAL    4
#define CRES_PRIVATE   5
#define CRES_FOR       6
#define CRES_WHILE     7
#define CRES_ELSE      8
#define CRES_STOP      9
#define CRES_IF       10
#define CRES_SWITCH   11
#define CRES_BREAK    12
#define CRES_CASE     13
#define CRES_GOTO     14
#define CRES_CHAR     15
#define CRES_CONST    16
#define CRES_DEFAULT  17
#define CRES_DOUBLE   18
#define CRES_EXTERN   19
#define CRES_FLOAT    20
#define CRES_ASM      21
#define CRES_LONG     22
#define CRES_PUBLIC   23
#define CRES_RETURN   24
#define CRES_SHORT    25
#define CRES_SIGNED   26
#define CRES_SIZEOF   27
#define CRES_STRUCT   28
#define CRES_TYPEDEF  29
#define CRES_UNION    30
#define CRES_UNSIGNED 31
#define CRES_VOID     32
#define CRES_EXTRA    33
#define CRES_CONTINUE 34
#define CRES_EXIT     35

#define CODE_PT_SIZE  (sizeof(void*)) /* size (in bytes) of a pointer in this system */

typedef struct CodeReservedWord { /* table entry for reserved words */
  BYTE cText[10]; /* reserved word name */
} CODERESERVEDWORD;

typedef struct CodeSysVar { /* System variable table */
  BYTE *cText;                  /* variable name */
  WORD cType;                   /* variable type */
  LWORD cInt;                   /* variable integer value */
  void *cPtr;                   /* variable pointer value */
} CODESYSVAR;

extern CODERESERVEDWORD cResWord[];/* table of reserved words */
extern CODESYSVAR cSysVar[]; /* table of system variables */

extern void CodeStufInit(); /* init C4 common */
