/* compile.c
 * Written by B. Cameron Lesiuk, 1995
 * Written for use with Crimson2 MUD (written/copyright Ryan Haksi 1995).
 * This source is proprietary. Use of this code without permission from 
 * Ryan Haksi or Cam Lesiuk is strictly prohibited. 
 * 
 * (wi961@freenet.victoria.bc.ca)
 */  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "send.h"
#include "extra.h"
#include "thing.h"
#include "index.h"
#include "code.h"
#include "codestuf.h"
#include "interp.h"
#include "function.h"
#include "compile.h"

/** Define Statments **/

/* compile-time options */

/* Compile defaults */
#define CSTACK_SIZE      (1<<12)  /* size of stack */
#define CSTACK_FRAME     (CSTACK_SIZE>>1) /* # stack frames */
#define CCODE_SIZE       (1<<12) /* size of compiled code buffer */

/* These statements are the return codes for CompileReadNext (CRN) */
#define CRN_EOF      0x01
#define CRN_OPERATOR 0x02
#define CRN_COMMENT  0x03
#define CRN_COMMA    0x04
#define CRN_PERIOD   0x05
#define CRN_OSBRAC   0x06 /* [ bracket set  (Square bracket) */
#define CRN_CSBRAC   0x07
#define CRN_OBRAC    0x08 /* { brace set (bracket) */
#define CRN_CBRAC    0x09
#define CRN_ORBRAC   0x0A /* ( parenthesis set (Round bracket) */
#define CRN_CRBRAC   0x0B
#define CRN_STRING   0x0C /* string */
#define CRN_SEMI     0x0D /* semicolon */
#define CRN_NUMBER   0x0E /* numerical constant */
#define CRN_LABEL    0x0F /* Label - this is further divided, but not in CRN */
/* CRN_LABEL is interpreted into the following: */
#define CRN_RESWRD   0x10 /* reserved word (if, while, int) */
#define CRN_FUNCT    0x11 /* function */
#define CRN_VAR      0x12 /* variable */
#define CRN_UNDEF    0x13 /* undefined label */

/* Compile uses these flags in cValidMask to determine which ops are valid */
/* NOTE: the value must correspond to the CRN return codes defined above */
#define CV_EOF       (1<<CRN_EOF)
#define CV_OPERATOR  (1<<CRN_OPERATOR)
#define CV_COMMENT   (1<<CRN_COMMENT)
#define CV_COMMA     (1<<CRN_COMMA)
#define CV_PERIOD    (1<<CRN_PERIOD)
#define CV_OSBRAC    (1<<CRN_OSBRAC)
#define CV_CSBRAC    (1<<CRN_CSBRAC)
#define CV_OBRAC     (1<<CRN_OBRAC)
#define CV_CBRAC     (1<<CRN_CBRAC)
#define CV_ORBRAC    (1<<CRN_ORBRAC)
#define CV_CRBRAC    (1<<CRN_CRBRAC)
#define CV_STRING    (1<<CRN_STRING)
#define CV_SEMI      (1<<CRN_SEMI)
#define CV_NUMBER    (1<<CRN_NUMBER)
#define CV_LABEL     (1<<CRN_LABEL)
#define CV_RESWRD    (1<<CRN_RESWRD)
#define CV_FUNCT     (1<<CRN_FUNCT)
#define CV_VAR       (1<<CRN_VAR)
#define CV_UNDEF     (1<<CRN_UNDEF)

/* return codes for CompileReadNext */
#define CRN_RC_OK     0
#define CRN_RC_EOF    1
#define CRN_RC_SYNTAX 2

/* Compile State machine- major state codes */
#define CS_END        0xFFFFF000 /* end of program (looking for EOF) */
#define CS_START      0x00000100 /* start of program (looking for { ) */
#define CS_MAIN       0x00010000 /* Main State Machine Loop */
#define CS_VAR        0x00018000 /* Variable declarations */
#define CS_IF         0x00020000 /* Reserved word "if" expr handler */
#define CS_WHILE      0x00030000 /* Reserved word "while" expr handler */
#define CS_STOP       0x00040000 /* Reserved word "stop" expr handler */
#define CS_GET_EXP    0x00050000 /* Get Expression state machine loop */
#define CS_FUNCTION   0x00060000 /* handles function calls etc. */
#define CS_GET_NEXT   0x00070000 /* gets next Var or Function */

/* Compile state machine - expression type flag values 
 * Confused? Here, let me explain:
 * In the following example:
 *          a=b+c;
 * `a' is an lvalue. lvalues must be variables. `1=b+c' is invalid!!!
 * `b+c' is an rvalue. rvalues can be variables, functions, constants, etc.
 * `a=b+c' is an expression. It can also be used as an rvalue.
 *
 * In the following example:
 *          function(a,b+c);
 * `a' is a rvalue. Functions must be passed rvalues. Don't worry though,
 *     just about everything can be used as an rvalue.
 * `b+c' is also an rvalue.
 * `function' is a function call. It's also an rvalue.
 *
 * Got it?
 */
#define CEFLAG_EXP    1<<0 /* expression */
#define CEFLAG_RVALUE 1<<1 /* rvalue */
#define CEFLAG_LVALUE 1<<2 /* lvalue */
#define CEFLAG_FUNCT  1<<3 /* function */
/* Compile state machine - data type flag values */
#define CDFLAG_UNDEF  (1<<CDT_UNDEF)
#define CDFLAG_NULL   (1<<CDT_NULL)
#define CDFLAG_STR    (1<<CDT_STR)
#define CDFLAG_INT    (1<<CDT_INT)
#define CDFLAG_THING  (1<<CDT_THING)
#define CDFLAG_EXTRA  (1<<CDT_EXTRA)
#define CDFLAG_EXIT   (1<<CDT_EXIT)

/* Structures and unions */
typedef union CompileUnion {
  UWORD cuPointer;    /* pointer to a variable, function, string, etc */
  SBYTE cuSByte;      /* signed byte */
  WORD  cuSWord;      /* signed word */
  LWORD cuSLWord;     /* 32-bit signed data */
  ULWORD cuUnsigned;  /* 32-bit unsigned data */
  ULWORD cuState;     /* state for state machine */
  ULWORD cuValid;     /* valid flags for state machine */
  UWORD cuFunction;   /* function being called */
  UWORD cuVar;        /* variable being referenced */
  BYTE cuParamType;   /* parameter type for functions */
  ULWORD cuVarFlag;   /* Data Type flag */
  ULWORD cuExpFlag;   /* Expression Type Flag */
  UWORD cuOffset;     /* Offset - a saved location in program */
  void *cuPtr;        /* pointer data - used for things like COP_PUSHPT */
} COMPILEUNION;

typedef struct CompileData { /* this type is used for the stack, for */
  BYTE cInstruction;         /* parameter passing, etc. etc. etc.    */
  COMPILEUNION cData;
} COMPILEDATA;

typedef struct CharMark { /* used by CompileReadNext */
  BYTE *SCMstr;
  WORD SCMoperation;
} CHARMARK;

typedef struct CompileVarTableType { /* variable table */
  BYTE cText[CVAR_NAME_LENGTH]; /* variable name */
  WORD cType;     /* variable type */
  WORD cDomain;   /* variable domain */
  ULWORD cOffset;   /* location in variable table specified by cDomain */
} COMPILEVARTABLETYPE;

typedef struct CompileReservedWord { /* table entry for reserved words */
  BYTE cText[10]; /* reserved word name */
} COMPILERESERVEDWORD;

/* Globals - used everywhere in compile.c */
WORD cComp;   /* pointer to where we are in the compiled code */
BYTE *cCompBuf;/* compile buffer */
LWORD cCompBufSizeByte;
BYTE *cSrc;    /* pointer to where we are in the soure (text) code */
BYTE *cSrcTop; /* pointer to top of soure (text) code */
BYTE *cLastSrc;/* pointer to the last place we were in the source code */
BYTE *cNextSrc;/* pointer to the next place in source code - if we know it. */
COMPILEDATA *cStack; /* stack used by all compile routines */
COMPILEDATA *cStackP; /* pointer to current stack - used to access stack */
LWORD cStackSizeByte; /* current stack size in bytes */
WORD cSP;      /* current stack pointer */
WORD cSF;      /* current stack frame */
WORD *cSFP;    /* stack frame base position pointer*/
LWORD cSFPSizeByte; /* current stack frame size in bytes */
THING *cThing;  /* thing to report stuff (like errors) to */
WORD cRc;       /* return code - use everywhere */
UWORD cCurVar;   /* current variable we're at in the variable table */
COMPILEVARTABLETYPE cVarTable[CMAX_VAR]; /* variable table */
LWORD cReqStackSize;    /* size of required Interp.c stack required. */ 
LWORD cReqStackSizeMax; /* Max value of cReqStackSize */
LWORD cReqParameter;   /* Max parameters required by Interp */

/* Macros - used everywhere in compile.c */
#define COMPILE_GETLASTSRC()  \
do { \
  if ((cRc=CompileGetLastSrc())) \
    return (cRc); \
} while(0)

#define COMPILE_ADDBYTE(byte) \
do { \
  if ((cRc=CompileAddByte(byte))) \
    return (cRc); \
} while(0)

#define COMPILE_STRING(ptr)   \
do { \
  if ((cRc=CompileString(ptr))) \
    return (cRc); \
} while(0)

#define COMPILE_PUSH(ptr)  \
do { \
  if ((cRc=CompilePush(ptr))) \
    return (cRc); \
} while(0)

#define COMPILE_PULL(ptr)  \
do { \
  if ((cRc=CompilePull(ptr))) \
    return (cRc); \
} while(0)

#define COMPILE_UPSTACK()  \
do { \
  if ((cRc=CompileUpStack())) \
    return (cRc); \
} while(0)

#define COMPILE_DOWNSTACK()  \
do { \
  if ((cRc=CompileDownStack())) \
    return (cRc); \
} while(0)

#define COMPILE_COMPILE(ptr)  \
do { \
  if ((cRc=CompileCompile(ptr))) \
    return (cRc); \
} while(0)

#define COMPILE_COMPILESTACK()  \
do { \
  if ((cRc=CompileCompileStack())) \
    return (cRc); \
} while(0)

#define COMPILE_COMPILECOMMENT(ptr)  \
do { \
  if ((cRc=CompileComment(ptr))) \
    return (cRc); \
} while(0)

#define COMPILE_ADDPOINTER(ptr)  \
do { \
  if ((cRc=CompileAddPointer(ptr))) \
    return (cRc); \
} while(0)

/* This routine is called by the compiler if a warning occurs. This routine */
/* should pass the warning message etc. on to the appropriate place.        */
void CompileWarningReport(BYTE *cError) {
  if (cThing) {
    SendThing("^a",cThing);
    SendThing(cError,cThing);
  }
}

/* This routine is called by the compiler if an error occurs. This routine */
/* should pass the error message etc. on to the appropriate place.         */
void CompileErrorReport(BYTE *cError) {
  BYTE temp,temp2,buf[40];
  BYTE *cP,*cSP,*cNP; /* pointer, start pointer, Next pointer */
  ULWORD cLine;

  if (cThing) {
    /* First, count lines, and find "line before" error */
    cLine=1; 
    for (cP=cSP=cNP=cSrcTop;(*cP)&&(cP!=cSrc);cP++) {
      if ((*cP)=='\n') {
        cLine++;
        cSP=cNP;
        cNP=cP+1;
        }
      }
    /* Increment past error to "line after" error */
    if (*cP)
      cP++;
    for (;(*cP)&&((*cP)!='\n');cP++);
    if (*cP)
      cP++;
    for (;(*cP)&&((*cP)!='\n');cP++);

    temp=*cSrc;
    temp2=*cP;
    *cSrc=*cP=0;
    sprintf(buf,"^wError at line %li:\n",cLine);
    SendThing(buf,cThing);
    SendThing("^a",cThing);
    SendThing(cSP,cThing);
    SendThing(" ^r<<*ERROR*\n^a",cThing);
    *cSrc=temp;
    SendThing(cSrc,cThing);
    SendThing("\n",cThing);
    *cP=temp2;
    SendThing("^w",cThing);
    SendThing(cError,cThing);
    SendThing("^a",cThing);
  }
}

/* This procedure sets cSrc to a new point and keeps track of cLastSrc */
void CompileSetSrc(BYTE *newpoint) {
  cLastSrc=cSrc;
  cSrc=newpoint;
  cNextSrc=NULL;
}

WORD CompileGetLastSrc() { /* retrieves last cSrc pointer */
  if (cLastSrc==NULL) { 
    /* If this happens, the state machine is screwed up. FIX IT! */
    /* We are only allowed to go 'back' one step!                */
    CompileErrorReport("internal error (compile.c): cLastSrc is NULL. This should not happen.\nRecord this message and your program, and notify your Sys Admin.\n");
    return(COMPILE_INTERNAL_ERROR);
  }
  else {
    cNextSrc=cSrc;
    cSrc=cLastSrc;
    cLastSrc=NULL;
    return(0);
  }
}

WORD CompileGetNextSrc() { /* oposite of CompileGetLastSrc */
  if (cNextSrc==NULL) {
    /* If this happens, the state machine is screwed up. FIX IT! */
    /* We are only allowed to go 'back' one step!                */
    CompileErrorReport("internal error (compile.c): cNextSrc is NULL. This should not happen.\nRecord this message and your program, and notify your Sys Admin.\n");
    return(COMPILE_INTERNAL_ERROR);
  }
  else {
    cLastSrc=cSrc;
    cSrc=cNextSrc;
    cNextSrc=NULL;
    return(0);
  }
}

/** Stack Functions **/

WORD CompileInitStack() {
  cSP=0; /* zero'th stack position in the... */ 
  cSF=0; /* zero'th stack frame, which starts at... */
  cSFP[cSF]=0; /* zero'th position in the stack */
  cStackP=&cStack[cSFP[cSF]]; /* set up pointer to this stack frame */
  return(0);
}

WORD CompileUpStack() { /* go up one stack frame */
  cSF++;
  REALLOC("WARNING! compile.c stack frame overflow... resizing\n",
    cSFP,WORD,(cSF+1),cSFPSizeByte);
  cSFP[cSF]=cSFP[cSF-1]+cSP; /* set new stack frame */
  cSP=0; /* zero'th position in new stack frame */
  cStackP=&cStack[cSFP[cSF]]; /* set up pointer to this stack frame */
  return(0);
}

WORD CompileDownStack() { /* go down one stack frame */
  BYTE error[300];
  if (cSP) { /* send WARNING message, not error, that stack is non-empty. */
             /* HOPEFULLY, if this code is modified correctly and runs    */
             /* smoothly, this condition will never happen. HOWEVER, the  */
             /* whole idea behind a stack frame is that each section of   */
             /* code gets its own stack, and can't screw up other peoples */
             /* stacks. (peoples? well, I guess we could consider states  */
             /* of a state machine as people. :)                          */
    if (cSP==1) {
      sprintf(error,"internal warning (compile.c): %i datum on stack lost with CompileDownStack().\nThis may cause your program to compile incorrectly. Notify your Sys Admin.\n",cSP);
    }
    else {
      sprintf(error,"internal warning (compile.c): %i data on stack lost with CompileDownStack().\nThis may cause your program to compile incorrectly. Notify your Sys Admin.\n",cSP);
    }
    CompileWarningReport(error);
  }
  if (cSF<=0) {
    CompileErrorReport("compile error: stack frame underflow\n");
    return(COMPILE_INTERNAL_ERROR);
  }
  cSP=cSFP[cSF]-cSFP[cSF-1];
  cSF--;
  cStackP=&cStack[cSFP[cSF]]; /* set up pointer to this stack frame */
  return(0);
}

WORD CompilePush(COMPILEDATA *data) {
  REALLOC("WARNING! compile.c stack overflow... resizing\n",
    cStack,COMPILEDATA,(cSP+cSFP[cSF]+1),cStackSizeByte);
  /* if we've just REALLOC'ed, our cStackP pointer is no longer valid. 
   * Instead of checking for a changed cStack, I'm just going to 
   * ALWAYS re-load cStackP from cStack */
  cStackP=&cStack[cSFP[cSF]]; /* set up pointer to this stack frame */

  cStackP[cSP].cInstruction = data->cInstruction; /* copy instruction */
  cStackP[cSP].cData.cuUnsigned=data->cData.cuUnsigned;
    /* copy data. ALL the data should be copied using the Unsigned */
  cSP++;
  return(0);
}

WORD CompilePull(COMPILEDATA *data) {
  if (!cSP) {
    CompileErrorReport("compile error: stack underflow\n");
    return(COMPILE_INTERNAL_ERROR);
  }
  cSP--;
  data->cInstruction=cStackP[cSP].cInstruction;
  data->cData.cuUnsigned=cStackP[cSP].cData.cuUnsigned;
  return(0);
}







/** Compile Functions **/

WORD CompileAddByte(BYTE data) {
  REALLOC("WARNING! compile.c buffer overflow... resizing\n",
    cCompBuf,BYTE,(cComp+1),cCompBufSizeByte);
  cCompBuf[cComp++]=data;
  return(0);
}

/* This procedure adds a pointer to the compiled code */
WORD CompileAddPointer(void *ptr) {
  ULWORD i;
  REALLOC("WARNING! compile.c buffer overflow... resizing\n",
    cCompBuf,BYTE,(cComp+CODE_PT_SIZE+1),cCompBufSizeByte);
  /* the following doesn't work on Solaris for some reason */
  /*
  *((void**)(&(cCompBuf[cComp])))=(void *)ptr;
  */

  /* alternate */
  for (i=0;i<CODE_PT_SIZE;i++)
    cCompBuf[cComp+i]=((((unsigned long)(ptr))>>(8*i))&0x0FF);
  cComp+=CODE_PT_SIZE;
  return(0);
}

/* this compiles, as a comment, the string passed to it */
WORD CompileComment(BYTE *comment) { /* compiles a comment into code */
  BYTE *pointer;

  if (BIT(codeSet,CODE_SAVECOMMENT)) {
    COMPILE_ADDBYTE(COP_COMMENT);
    pointer=comment;
    pointer--;
    do {
      pointer++;
      COMPILE_ADDBYTE(*pointer);
    }
    while(*pointer);
  } else {
    CompileWarningReport("compile warning: comments are turf'd.\n");
  }
  return(0);
} /* CompileComment */

/* this function appends a string onto the compiled program */
WORD CompileString(BYTE *string) { /* compiles a string into code */
  BYTE *pointer;

  pointer=string;
  pointer--;
  do {
    pointer++;
    COMPILE_ADDBYTE(*pointer);
  }
  while(*pointer);
  return(0);
} /* CompileString */

/* this fuction takes `data' and adds it on to the compiled program. */
WORD CompileCompile(COMPILEDATA *instruction) {
  LWORD temp;
  static LWORD cLastPush;

  /* optimize space useage by evaluating the generic COP_PUSHV and replacing */
  /* it with PUSHV1 (1-byte), PUSHV2 (2-bytes), or PUSHV4 (4-bytes).         */
  if (instruction->cInstruction==COP_PUSHV) {
    temp=instruction->cData.cuSLWord;
    if ((-128<=temp)&&(temp<=127)) {
      instruction->cInstruction=COP_PUSHV1;
      instruction->cData.cuSByte=temp;
    }
    else if ((-32768<=temp)&&(temp<=32767)) {
      instruction->cInstruction=COP_PUSHV2;
      instruction->cData.cuSWord=temp;
    }
    else {
      instruction->cInstruction=COP_PUSHV4;
      instruction->cData.cuSLWord=temp;
    }
  } /* if instruction of variety PUSHV */

  switch (instruction->cInstruction) {
      /* These operations decrement by 1 the stack pointer */
    case COP_EQUSR:
    case COP_EQUSL:
    case COP_EQUOR:
    case COP_EQUXOR:
    case COP_EQUAND:
    case COP_EQUSUB:
    case COP_EQUADD:
    case COP_EQUMOD:
    case COP_EQUDIV:
    case COP_EQUMULT:
    case COP_EQU:
    case COP_BOR:
    case COP_BAND:
    case COP_OR:
    case COP_XOR:
    case COP_AND:
    case COP_BEQU:
    case COP_BNEQU:
    case COP_GEQ:
    case COP_GR:
    case COP_LEQ:
    case COP_LS:
    case COP_SR:
    case COP_SL:
    case COP_SUB:
    case COP_ADD:
    case COP_MOD:
    case COP_DIV:
    case COP_MULT:
    case COP_POP:
      cReqStackSize--;

      /* These operations don't affect the stack pointer */
    case COP_BSUB:
    case COP_BADD:
    case COP_ASUB:
    case COP_AADD:
    case COP_COMP:
    case COP_NOT:
    case COP_NEG:
    case COP_TERM:
      COMPILE_ADDBYTE(instruction->cInstruction);
      break;

      /* These operations affect the stack in different ways */
    case COP_EXEC:
    case COP_EXECR:
      cReqStackSize-=cLastPush;
      if (cLastPush>cReqParameter) /* get max # parameters required */
        cReqParameter=cLastPush;
      COMPILE_ADDBYTE(instruction->cInstruction);
      COMPILE_ADDBYTE((instruction->cData.cuPointer&0x00ff));
      COMPILE_ADDBYTE((instruction->cData.cuPointer&0xff00)>>8);
      break;
    case COP_PUSHL:
    case COP_PUSHG:
    case COP_PUSHP:
      cReqStackSize++;
      COMPILE_ADDBYTE(instruction->cInstruction);
      COMPILE_ADDBYTE((instruction->cData.cuPointer&0x00ff));
      COMPILE_ADDBYTE((instruction->cData.cuPointer&0xff00)>>8);
      break;
    case COP_IFZ:
    case COP_WHILEZ:
      cReqStackSize--;
    case COP_GOTO:
    case COP_COMMENT:
      COMPILE_ADDBYTE(instruction->cInstruction);
      COMPILE_ADDBYTE((instruction->cData.cuPointer&0x00ff));
      COMPILE_ADDBYTE((instruction->cData.cuPointer&0xff00)>>8);
      break;
    case COP_PUSHV1:
      cLastPush=(LWORD)(instruction->cData.cuSByte);
      cReqStackSize++;
      COMPILE_ADDBYTE(instruction->cInstruction);
      COMPILE_ADDBYTE(instruction->cData.cuSByte&0xff);
      break;
    case COP_PUSHV2:
      cLastPush=(LWORD)(instruction->cData.cuSWord);
      cReqStackSize++;
      COMPILE_ADDBYTE(instruction->cInstruction);
      COMPILE_ADDBYTE(instruction->cData.cuSWord&0x00ff);
      COMPILE_ADDBYTE((instruction->cData.cuSWord&0xff00)>>8);
      break;
    case COP_PUSHV:
    case COP_PUSHV4:
      cLastPush=(LWORD)(instruction->cData.cuSLWord);
      cReqStackSize++;
      COMPILE_ADDBYTE(instruction->cInstruction);
      COMPILE_ADDBYTE(instruction->cData.cuSLWord&0x000000ff);
      COMPILE_ADDBYTE((instruction->cData.cuSLWord&0x0000ff00)>>8);
      COMPILE_ADDBYTE((instruction->cData.cuSLWord&0x00ff0000)>>16);
      COMPILE_ADDBYTE((instruction->cData.cuSLWord&0xff000000)>>24);
      break;
    case COP_PUSHPT:
      cReqStackSize++;
      COMPILE_ADDBYTE(instruction->cInstruction);
      COMPILE_ADDPOINTER(instruction->cData.cuPtr);
      break;
    case COP_NULL:
    default:
      CompileWarningReport("internal warning (compile.c): compiler passed illegal instruction.\nThis may cause your program to compile incorrectly. Notify your Sys Admin.\n");
      break;
  }
  if (cReqStackSize>cReqStackSizeMax)
    cReqStackSizeMax=cReqStackSize;
  return(0);
}

/* This pulls the top instruction off the stack and compiles it */
WORD CompileCompileStack() {
  COMPILEDATA instruction; 

  COMPILE_PULL(&instruction);
  COMPILE_COMPILE(&instruction);
  return(0);
}


LWORD CompileStrToNum(BYTE *string) { /* converts string to LWORD */
  BYTE *pointer;
  LWORD num;

  num=0;
  pointer=string;
  while(*pointer) {
    num=(num*10)+(*pointer-'0');
    pointer++;
    }
  return(num);
}

/** Parser Functions **/

/* CompileReadNext ()
** *c_label   - returns: text of label, comment, string, whatever
** *c_operation - 1 WORD code as defined by the #define statements
**                starting with CRN_
** returns COMPILE_CRN_RC_OK if operation ok, errorcode otherwise: 
**      (*label=errorcode)
**  CRN_RC_EOF - EOF reached unexpectedly
**  CRN_RC_SYNTAX - syntactical error
**
** This procedure reads the next 'bit' of code which is of importance
** to the compiler. It doesn't read very far, and it's up to the compiler
** to keep track of what is valid and when. What I mean by the next 'bit'
** is this:
**
**   if you fed this routine the following line, repeatedly, until EOLN,
**
**      a = c * (d-e) /o  hi mom o/ + some_funct(g,h)+strlen("hi there");
**                    ^^         ^^
**                    pretend these are real comment markers :)
**       
**   you would get:
**      c_label      c_operation
**      a            CRN_LABEL<<8
**      NULL         CRN_OPERATOR<<8 + COP_EQU
**      c            CRN_LABEL<<8
**      NULL         CRN_OPERATOR<<8 + COP_MULT
**      NULL         CRN_ORBRAC<<8
**      d            CRN_LABEL<<8
**      NULL         CRN_OPERATOR<<8 + COP_SUB
**      e            CRN_LABEL<<8
**      NULL         CRN_CRBRAC<<8
**      hi mom       CRN_COMMENT<<8
**      NULL         CRN_OPERATOR<<8 + COP_ADD
**      some_funct   CRN_LABEL<<8
**      NULL         CRN_ORBRAC<<8
**      g            CRN_LABEL<<8
**      NULL         CRN_COMMA<<8
**      h            CRN_LABEL<<8
**      NULL         CRN_CRBRAC<<8
**      NULL         CRN_OPERATOR<<8 + COP_ADD
**      strlen       CRN_LABEL<<8
**      NULL         CRN_ORBRAC<<8
**      hi there     CRN_STRING<<8
**      NULL         CRN_CRBRAC<<8
**      NULL         CRN_SEMI<<8
**      NULL         CRN_EOF<<8   (assuming this is the end of the file )
*/
WORD CompileReadNext(BYTE *c_label, WORD *c_operation) {

  BYTE *p_label; /* pointers to these things. */
  BYTE *pointer;
  WORD SCM_offset;
  static CHARMARK SCM[]= { /* make sure you put triple-char, then double, then single */
    { ">>=",(CRN_OPERATOR<<8) + COP_EQUSR     },
    { "<<=",(CRN_OPERATOR<<8) + COP_EQUSL     },
    { "|=" ,(CRN_OPERATOR<<8) + COP_EQUOR     },
    { "^=" ,(CRN_OPERATOR<<8) + COP_EQUXOR    },
    { "&=" ,(CRN_OPERATOR<<8) + COP_EQUAND    },
    { "-=" ,(CRN_OPERATOR<<8) + COP_EQUSUB    },
    { "+=" ,(CRN_OPERATOR<<8) + COP_EQUADD    },
    { "%=" ,(CRN_OPERATOR<<8) + COP_EQUMOD    },
    { "/=" ,(CRN_OPERATOR<<8) + COP_EQUDIV    },
    { "*=" ,(CRN_OPERATOR<<8) + COP_EQUMULT   },
    { "&&" ,(CRN_OPERATOR<<8) + COP_BAND      },
    { "||" ,(CRN_OPERATOR<<8) + COP_BOR       },
    { "!=" ,(CRN_OPERATOR<<8) + COP_BNEQU     },
    { "==" ,(CRN_OPERATOR<<8) + COP_BEQU      },
    { "<=" ,(CRN_OPERATOR<<8) + COP_LEQ       },
    { ">=" ,(CRN_OPERATOR<<8) + COP_GEQ       },
    { "<<" ,(CRN_OPERATOR<<8) + COP_SL        },
    { ">>" ,(CRN_OPERATOR<<8) + COP_SR        },
    { "--" ,(CRN_OPERATOR<<8) + COP_ASUB      },
    { "++" ,(CRN_OPERATOR<<8) + COP_AADD      },
    { "`"  ,(CRN_OPERATOR<<8) + COP_COMP      },
    { "<"  ,(CRN_OPERATOR<<8) + COP_LS        },
    { ">"  ,(CRN_OPERATOR<<8) + COP_GR        },
    { "!"  ,(CRN_OPERATOR<<8) + COP_NOT       },
    { "="  ,(CRN_OPERATOR<<8) + COP_EQU       },
    { "+"  ,(CRN_OPERATOR<<8) + COP_ADD       },
    { "-"  ,(CRN_OPERATOR<<8) + COP_SUB       },
    { "*"  ,(CRN_OPERATOR<<8) + COP_MULT      },
    { "/"  ,(CRN_OPERATOR<<8) + COP_DIV       },
    { "%"  ,(CRN_OPERATOR<<8) + COP_MOD       },
    { "&"  ,(CRN_OPERATOR<<8) + COP_AND       },
    { "|"  ,(CRN_OPERATOR<<8) + COP_OR        },
    { "\\"  ,(CRN_OPERATOR<<8) + COP_XOR      },
    { "{"  ,CRN_OBRAC<<8                   }, /* grammar marks */
    { "}"  ,CRN_CBRAC<<8                   },
    { "("  ,CRN_ORBRAC<<8                  },
    { ")"  ,CRN_CRBRAC<<8                  },
    { ";"  ,CRN_SEMI<<8                    },
    { ","  ,CRN_COMMA<<8                   },
    { "["  ,CRN_OSBRAC<<8                  },
    { "]"  ,CRN_CSBRAC<<8                  },
    { NULL ,0                              },
  };

  pointer=cSrc; /* start at beginning of program */
  *c_label=0; /* null-out all our return codes */
  *c_operation=0;

  /* first, lets find first non-space, non-LF, non-etc char*/
  while(((*pointer == ' ')||(*pointer < 32))&&(*pointer !=0))
    pointer++;

  if (*pointer == 0) /* check for EOF */ {
    *c_operation=CRN_EOF<<8;
    CompileSetSrc(pointer);
    return(CRN_RC_OK);
  }

  if ((*pointer == '/') && (*(pointer+1)=='/')) { /* we've a '/'/' comment! */
    pointer=pointer+2; /* advance pointer to comment 'guts' */
    *c_operation=CRN_COMMENT<<8; /* define as comment */
    p_label=c_label;  /* start our 'c_label' filling! */
    while ((*pointer!='\n')&&(*pointer!='\r')&&(*pointer!=10)&&
          (*pointer!=0)) {
      *p_label=*pointer; /* fill up label with comment */
      pointer++; /* advance pointer & p_label */
      p_label++;
    }
    /* at this point, whatever we reached, it's 'end of comment' */
    *p_label=0; /* mark as end of string */
    CompileSetSrc(pointer); /* send back 'the rest */
    return(CRN_RC_OK);
  }

  if ((*pointer == '/') && (*(pointer+1)=='*')) { /* we've a '/'*' comment! */
    pointer=pointer+2; /* advance pointer to comment text */
    *c_operation=CRN_COMMENT<<8; /* define as comment */
    p_label=c_label;  /* start our 'label' filling! */
    while (((*pointer!='*')||(*(pointer+1)!='/'))&&
          (*pointer!=0)) {
      *p_label=*pointer; /* fill up label with comment */
      pointer++; /* advance pointer & p_label */
      p_label++;
    }
    /* at this point, if EOF, error!  */
    if (*pointer == 0) {
      sprintf(c_label,"'/*' comment missing matching '*/' before EOF");
      CompileSetSrc(pointer);
      return(CRN_RC_EOF);
    }
    /* ok, we've found the end marker */
    pointer=pointer+2; /* advance pointer to outside comment */
    *p_label=0; /* mark as end of string */
    CompileSetSrc(pointer); /* send back 'the rest */
    return(CRN_RC_OK);
  }
        
  if (*pointer == '"') { /* check if this is a quoted string */ 
    *c_operation=CRN_STRING<<8; /* define as string */
    p_label=c_label; /* start our 'label' filling! */
    pointer++; /* skip past open " */
    while ((*pointer !='"')&&(*pointer != 0)) { 
      /* copy text into label - quote string so anything goes */
      if ((*pointer=='\\')&&(*(pointer+1)!=0)) { /* escape sequence */
        pointer++;
        if (*pointer=='a') *p_label='\a';
        else if (*pointer=='b') *p_label='\b';
        else if (*pointer=='f') *p_label='\f';
        else if (*pointer=='n') *p_label='\n';
        else if (*pointer=='r') *p_label='\r';
        else if (*pointer=='t') *p_label='\t';
        else if (*pointer=='v') *p_label='\v';
        else *p_label=*pointer;
      } else *p_label=*pointer;
      p_label++;
      pointer++;
    }
    if (*pointer==0) { /* an EOF? Error! */
      sprintf(c_label,"Unexpected EOF! You are missing a final \" to your quote!");
      CompileSetSrc(pointer);
      return(CRN_RC_EOF);
    }
    *p_label=0; /* terminate c_label */
    pointer++; /* advance pointer past ending " */
    CompileSetSrc(pointer);
    return(CRN_RC_OK);
  }

  if (*(pointer+1)!=0) {/* we don't want to go over - segmentation faults :) */
    if ((*pointer=='\'')&&(*(pointer+2)=='\'')) { /* ' quoted number  */
      *c_operation=CRN_NUMBER<<8; /* define as number */
      p_label=c_label; /* start our 'label' filling! */
      pointer++;
      *(p_label++)=((*pointer)/100)+'0';
      *(p_label++)=((*pointer)/10)-(((*pointer)/100)*10)+'0';
      *(p_label++)=(*pointer)-(((*pointer)/10)*10)+'0';
      *(p_label++)=0; /* terminate string */
      pointer+=2;
      CompileSetSrc(pointer);
      return(CRN_RC_OK);
    }
  }


  /* now, the following while-loop is just to save code space and     */
  /* whip through all the cases where we would return after reading a */
  /* simple char string. This is what the big structure def'n is for. */
  SCM_offset=0; /* start at the beginning */
  while (SCM[SCM_offset].SCMstr!=NULL) {
    if ((strlen(SCM[SCM_offset].SCMstr)==3)&& /* first give triple a crack */
        (*pointer==SCM[SCM_offset].SCMstr[0])&&
        (*(pointer+1)==SCM[SCM_offset].SCMstr[1])&&
        (*(pointer+2)==SCM[SCM_offset].SCMstr[2])) {
      *c_operation=SCM[SCM_offset].SCMoperation;
      CompileSetSrc(pointer+3);
      strcpy(c_label,SCM[SCM_offset].SCMstr);
      return(CRN_RC_OK);
    } 
    else if ((strlen(SCM[SCM_offset].SCMstr)==2)&& /* then doubles */
             (*pointer==SCM[SCM_offset].SCMstr[0])&&
             (*(pointer+1)==SCM[SCM_offset].SCMstr[1])) {
      *c_operation=SCM[SCM_offset].SCMoperation;
      CompileSetSrc(pointer+2);
      strcpy(c_label,SCM[SCM_offset].SCMstr);
      return(CRN_RC_OK);
    }
    else if ((strlen(SCM[SCM_offset].SCMstr)==1)&& /* finally singles */
             (*pointer==SCM[SCM_offset].SCMstr[0])) {
      *c_operation=SCM[SCM_offset].SCMoperation;
      CompileSetSrc(pointer+1);
      strcpy(c_label,SCM[SCM_offset].SCMstr);
      return(CRN_RC_OK);
    }
    SCM_offset++;
  }

  if ((*pointer>='0')&&(*pointer<='9')) { /* we've got a number */
    *c_operation=CRN_NUMBER<<8; /* define as number */
    p_label=c_label; /* initialize p_label to start of c_label */
    while((*pointer>='0')&&(*pointer<='9')) {
      *p_label=*pointer; /* copy number into 'label' */
      pointer++; /* advance pointers */
      p_label++;
    }
    *p_label=0; /* terminate label */
    CompileSetSrc(pointer);
    return(CRN_RC_OK);
  }

  if (((*pointer>='a')&&(*pointer<='z'))|| /* we're lowercase alpha */
     ((*pointer>='A')&&(*pointer<='Z'))|| /* we're uppercase alpha */
     (*pointer == '_')) { /* and of course, the underscore is valid */
    /* ok, for us to have made it this far, we've got to be looking */
    /* at an alphabetic label. Let's read it into c_label now.      */

    /* Start copying label, and boogey along until we hit a non-label*/
    /* character (eg: 'int a' or 'a = 2;' ) or a '(' (eg: printf("hi");)*/
    *c_operation=CRN_LABEL<<8; /* define as label */
    p_label=c_label; /* initialize p_label to start of c_label */
    while(((*pointer>='a')&&(*pointer<='z'))|| /* we're lowercase alpha */
         ((*pointer>='A')&&(*pointer<='Z'))|| /* we're uppercase alpha */
         ((*pointer>='0')&&(*pointer<='9'))|| /* numbers just can't be the first character in a label */
         (*pointer == '_')) { /* and of course, the underscore is valid */
      *p_label=*pointer; /* copy label into 'label' */
      pointer++; /* advance pointer & p_label */
      p_label++;
    } /* while */
    *p_label=0; /* terminate label */
    CompileSetSrc(pointer);
    return(CRN_RC_OK);
  } 
  else { /* we've got an invalid/unrecognized character! */
    sprintf(c_label,"I don't recognize this character: %c",*pointer);
    CompileSetSrc(pointer+1);
    return(CRN_RC_SYNTAX);
  } 
} /* CompileReadNext */



/* Compile
 * The guts of this whole thing.
 *
 * Compile is a great big huge gigantic enormous ugly disgusting state machine.
 * I hope you like it :)
 *
 * ccSrc     = pointer to a STR containing source code to compile.
 * ccComp    = pointer to a pointer to a STR (which will be created by Compile)
 *             containing compiled binary code. NOTE: ccComp will contain, 
 *             upon return, a pointer to the STR.
 * ccThing   = the THING to send informational messages to (eg compile errors).
 *
 */
WORD Compile(STR *ccSrc,STR **ccComp,THING *ccThing) {
  ULWORD cValid; /* mask as to what operation is currently valid */
  ULWORD cState;  /* current state for state machine */
  BYTE cLabel[1000]; /* Label storage - use with CompileReadNext */
  UWORD cOperation; /* operation - use with CompileReadNext */
  UWORD cLastOperation; /* used to save last CompileReadNext operation */
  BYTE cError[200]; /* for error messages */
  BYTE cError2[200]; /* for error messages */
  UWORD counter; /* used here and there as a local counter */
  UWORD cReserved; /* offset to Reserved word in cResWord */
  UWORD cFunction; /* offset to function in fTable */
  UWORD cVar;      /* offset to variable in cVarTable */
  COMPILEDATA cTemp; /* data variable used for communicating with stacks etc. */
  UWORD cCurLocal; /* current local variable we're at */
  UWORD cTotalLocal,cTotalGlobal,cTotalPrivate; /* variable totals */

  /* flags used by compile state machine */
  ULWORD cVarFlag; /* variable type flag */
  ULWORD cExpFlag; /* Expression Type flag */
  ULWORD cNumParam; /* # parameters we've had in this function call */

  *ccComp=NULL; /* if we return w/ an error, return NULL */
  cComp=0;
  cSrc=cSrcTop=cLastSrc=ccSrc->sText;  /* setup source code for global access */
  cNextSrc=NULL;
  cThing=ccThing; /* set up Thing so that we have someplace to dump errors */
  cNumParam=0;
  cReqStackSize=cReqStackSizeMax=0; /* reset our interp stack size */
  cReqParameter=0;                  /* reset our interp parameter list size */

  if ((cRc=CompileInitStack())) /* initialize (empty) stack */
    return(cRc);

  /* set up start of code */
  strcpy(cCompBuf,CODE_PROG_HEADER);
  cComp=CODE_PROG_HEADER_LENGTH;

  *cVarTable[cSystemVariable].cText=0; /* initialize (empty) variable table */
  cVarTable[cSystemVariable].cType=CDT_NULL;
  cVarTable[cSystemVariable].cDomain=CDOMAIN_UNDEF;
  cVarTable[cSystemVariable].cOffset=0;
  cCurVar=cSystemVariable+1;
  cCurLocal=cSystemVariable+1; /* locals reside along side system vars. :) */

  cState=CS_START; /* State 1 is the starting state. State 0 exits */
  cValid=CV_COMMENT|CV_VAR|CV_CBRAC|CV_FUNCT|CV_RESWRD|CV_ORBRAC|
         CV_OPERATOR|CV_OBRAC|CV_EOF|CV_COMMENT|CV_SEMI;

  /** start of Compile state machine **/
  while (1) {
    if (cNextSrc) { /* we've already parsed the next block - just skip ahead */
      cRc=CompileGetNextSrc();
      cOperation=cLastOperation;
      if (cRc)
        return(cRc);
    } else {
      cRc=CompileReadNext(cLabel,&cOperation);
      cLastOperation=cOperation;
      if (cRc != CRN_RC_OK) { /* check for error codes from CompileReadNext */
        switch(cRc) {
          case CRN_RC_EOF:
            sprintf(cError,"parse error (EOF): %s\n",cLabel);
            break;
          case CRN_RC_SYNTAX:
            sprintf(cError,"parse error (SYNTAX): %s\n",cLabel);
            break;
          default:
            sprintf(cError,"parse error (unknown): %s\n",cLabel);
            break;
        }
        CompileErrorReport(cError);
        return(COMPILE_SYNTAX_ERROR);
      }
    }

    if (((cOperation&0xFF00)>>8)==CRN_LABEL) { /* chng label to fn,var,etc. */
      /* first, check for reserved word */
      counter=0;
      while((*cResWord[counter].cText!=0)&&
            (strcmp(cResWord[counter].cText,cLabel))) {
        counter++;
      }
      if (*cResWord[counter].cText!=0) { /* we've found it */
        cOperation=(CRN_RESWRD<<8)+(cOperation&0x00FF);
        cReserved=counter;
      }
      else { /* not reserved, must be a function, variable, or undefined. */
        /* first, check if it's a function */
        counter=0;
        while((fTable[counter].fText!=NULL)&& /*NOTE: Function calls are all*/
          (STRICMP(fTable[counter].fText,cLabel))) { /* NOT case dependent!!*/
          counter++;
  }
        if (fTable[counter].fText !=NULL) { /* we've found it */
          cOperation=(CRN_FUNCT<<8)+(cOperation&0x00FF);
          cFunction=counter;
        }
        else { /* not function, must be variable or undefined */
          counter=0;
          while((counter<cCurVar)&&
                (strcmp(cVarTable[counter].cText,cLabel))) {
            counter++;
          }
          if (counter<cCurVar) {  /* found it */
            cOperation=(CRN_VAR<<8)+(cOperation&0x00FF);
            cVar=counter;
          }
          else { /* not variable, must be undefined */
            cOperation=(CRN_UNDEF<<8)+(cOperation&0x00FF);
          } /* variable check */
        } /* function check */
      } /* reserved check */
    } /* if (cOoperation==CRN_LABEL) */

    cState=(cState&0xFFFFFF00)+((cOperation&0xFF00)>>8);

    if (BIT(codeSet,CODE_DEBUGCOMPILE)) {
      sprintf(cError,"^a  0x%08lX : %04X `%s'\n",cState,cOperation,cLabel);
      printf(cError);
      if (cThing) SendThing(cError,cThing);
    }

    /* Now, before we enter the state machine, we do a preliminary error */
    /* check. This looks at what is valid at the current time. It allows */
    /* the current state to filter out certain types of stuff, thus      */
    /* saving gobs of error checking in the state machine itself. It     */
    /* results in error codes not being quite so specific, but hey, too  */
    /* bad. If you want, you can code ALL the error codes, one by one,   */
    /* and someday have pretty error codes for every possible exception, */
    /* but I hope you've got a computer with lots of memory!             */
    /* (well, I guess you do - if you've got that much time to kill, you */
    /* must have money to spend on a big computer)                       */
    if (!(cValid&(1<<((cOperation&0xFF00)>>8)))) { /* invalid at this time */
      /* define type of error (borrow cLabel as temporary buffer) */
      switch((cOperation&0xFF00)>>8) {
        case CRN_EOF:
          sprintf(cError2,"EOF");
          break;
        case CRN_OPERATOR:
          sprintf(cError2,"operator");
          break;
        case CRN_COMMENT:
          sprintf(cError2,"comment `%s'",cLabel);
          break;
        case CRN_COMMA:
          sprintf(cError2,"comma (,)");
          break;
        case CRN_STRING:
          sprintf(cError2,"string constant \"%s\"",cLabel);
          break;
        case CRN_SEMI:
          sprintf(cError2,"semicolon (;)");
          break;
        case CRN_NUMBER:
          sprintf(cError2,"use of constant `%s'",cLabel);
          break;
        case CRN_RESWRD:
          sprintf(cError2,"use of reserved word `%s'",cLabel);
          break;
        case CRN_VAR:
          sprintf(cError2,"reference to variable `%s'",cLabel);
          break;
        case CRN_FUNCT:
          sprintf(cError2,"call to function `%s'",cLabel);
          break;
        case CRN_UNDEF:
          sprintf(cError2,"undefined label `%s' ",cLabel);
          break;
        case CRN_OBRAC:
          sprintf(cError2,"brace `{'");
          break;
        case CRN_CBRAC:
          sprintf(cError2,"brace `}'");
          break;
        case CRN_OSBRAC:
          sprintf(cError2,"bracket `['");
          break;
        case CRN_CSBRAC:
          sprintf(cError2,"bracket `]'");
          break;
        case CRN_ORBRAC:
          sprintf(cError2,"parenthesis `('");
          break;
        case CRN_CRBRAC:
          sprintf(cError2,"parenthesis `)'");
          break;
        case CRN_LABEL:
          sprintf(cError2,"unrecognized label `%s'",cLabel);
          break;
        default:
          sprintf(cError2,"this operation");
          break;
      }
      sprintf(cError,"compile error: %s is invalid here.\n",cError2);
      CompileErrorReport(cError);
      return(COMPILE_SYNTAX_ERROR);
    }

    /* ok, here it is - the state machine, in the form of a switch */
    switch (cState) {

/*  EEEEEE  NNN  NN  DDDDD
 *  EE      NNNN NN  DD  DD
 *  EEEE    NN NNNN  DD  DD
 *  EE      NN  NNN  DD  DD
 *  EEEEEE  NN   NN  DDDDD   */

      /* CS_END: end of program */
      case CS_END+CRN_EOF:  /* end of program */
        COMPILE_PULL(&cTemp); /* set `next block' for code section */
        cCompBuf[cTemp.cData.cuPointer]=(BYTE)(cComp&0x00ff);
        cCompBuf[cTemp.cData.cuPointer+1]=(BYTE)((cComp&0xff00)>>8);
        /* The following WACK of code takes the variable table cVarTable,
         * and translates it into it's component `compiled' parts. It starts
         * by tallying the Local, Global, and Private vars. This way, we know
         * how many of each we're dealing with. Then, it does what is required
         * for each domain's variables.
         */
        cTotalLocal=cTotalGlobal=cTotalPrivate=0;
        for(counter=cSystemVariable+1;counter<cCurVar;counter++) {
          if (cVarTable[counter].cDomain==CDOMAIN_LOCAL) {
            cTotalLocal++;
          } else if (cVarTable[counter].cDomain==CDOMAIN_GLOBAL) {
            cTotalGlobal++;
          } else if (cVarTable[counter].cDomain==CDOMAIN_PRIVATE) {
            cTotalPrivate++;
          } else {
            sprintf(cError,"compile error: unrecognized domain for `%s'. This shouldn't happen.\nRecord your program and this message, and contact your Sys Admin.\n",cVarTable[counter].cText);
            CompileErrorReport(cError);
            return(COMPILE_INTERNAL_ERROR);
          }
        } /* tally domain totals */
        /* uncomment these two lines if you want a status line
         * like this: "Variables: 6 local, 1 global, 0 private" printed. */
/*        sprintf(cError,"^aVariables: ^c%i ^alocal, ^c%i ^aglobal, ^c%i ^aprivate.\n",
                cTotalLocal,cTotalGlobal,cTotalPrivate);
        CompileWarningReport(cError);*/
        /* Translate local variables into compiled code */
        if (cTotalLocal) {
          COMPILE_ADDBYTE(CODE_BLK_VAR);  /* create VAR block */
          cTemp.cInstruction=COP_NULL; /* save location */
          cTemp.cData.cuPointer=cComp; 
          COMPILE_PUSH(&cTemp);
          COMPILE_ADDBYTE(0); /* 'next block' pointer (alloc space) */
          COMPILE_ADDBYTE(0); /* 'next block' pointer (alloc space) */
          for(counter=cSystemVariable+1;counter<cCurVar;counter++) { /* types */
            if (cVarTable[counter].cDomain==CDOMAIN_LOCAL) {
              COMPILE_ADDBYTE(cVarTable[counter].cType);
            }
          }
          COMPILE_PULL(&cTemp); /* set `next block' for code section */
          cCompBuf[cTemp.cData.cuPointer]=(BYTE)(cComp&0x00ff);
          cCompBuf[cTemp.cData.cuPointer+1]=(BYTE)((cComp&0xff00)>>8);
          COMPILE_ADDBYTE(CODE_BLK_VAR_NAME); /* create VAR_NAME blk */
          cTemp.cInstruction=COP_NULL; /* save location */
          cTemp.cData.cuPointer=cComp; 
          COMPILE_PUSH(&cTemp);
          COMPILE_ADDBYTE(0); /* 'next block' pointer (alloc space) */
          COMPILE_ADDBYTE(0); /* 'next block' pointer (alloc space) */
          for(counter=cSystemVariable+1;counter<cCurVar;counter++) { /* types */
            if (cVarTable[counter].cDomain==CDOMAIN_LOCAL) {
              COMPILE_STRING(cVarTable[counter].cText);
            }
          }
          COMPILE_PULL(&cTemp); /* set `next block' for code section */
          cCompBuf[cTemp.cData.cuPointer]=(BYTE)(cComp&0x00ff);
          cCompBuf[cTemp.cData.cuPointer+1]=(BYTE)((cComp&0xff00)>>8);
        }
        if (cTotalGlobal) {
          CompileErrorReport("compile error: global variables unsupported.\n");
          return(COMPILE_INTERNAL_ERROR);
        /******* CONVERT VARIABLE TABLE INTO GLOBAL*****/
        /******* ADD GLOBAL VARIABLE SECTIONS TO CODE ******/
        }
        if (cTotalPrivate) {
          CompileErrorReport("compile error: private variables unsupported.\n");
          return(COMPILE_INTERNAL_ERROR);
        /******* CONVERT VARIABLE TABLE INTO PRIVATE*****/
        /******* ADD PRIVATE VARIABLE SECTIONS TO CODE ******/
        }
        COMPILE_ADDBYTE(CODE_BLK_NULL);
        *ccComp=StrCreate(cCompBuf, cComp-1, HASH);
        InterpStackAlloc(cReqStackSizeMax,cReqParameter,cTotalLocal);
        return(COMPILE_OK);
        break;

/*  SSSS  TTTTTT   AAAA    RRRRR   TTTTTT
 * SS       TT    AA  AA   RR  RR    TT
 *  SSS     TT    AAAAAA   RRRR      TT
 *    SS    TT    AA  AA   RR RR     TT
 * SSSS     TT    AA  AA   RR  RR    TT */

      /* CS_START: start of program */
      case CS_START+CRN_SEMI: 
      case CS_START+CRN_COMMENT: 
      case CS_START+CRN_VAR: 
      case CS_START+CRN_CBRAC: 
      case CS_START+CRN_FUNCT: 
      case CS_START+CRN_RESWRD: 
      case CS_START+CRN_ORBRAC: 
      case CS_START+CRN_OPERATOR: 
      case CS_START+CRN_OBRAC: 
        COMPILE_GETLASTSRC(); /* Whoah! backup! */
        COMPILE_ADDBYTE(CODE_BLK_CODE); /* code block header */
        cTemp.cInstruction=COP_NULL;
        cTemp.cData.cuOffset=cComp;
        COMPILE_PUSH(&cTemp); /* push marker for 'next blk' offset */
        COMPILE_ADDBYTE(0); /* 'next block' pointer (alloc space) */
        COMPILE_ADDBYTE(0); /* 'next block' pointer (alloc space) */
        cTemp.cInstruction=COP_NULL; /* push cValid and cState for */
        cTemp.cData.cuValid=CV_EOF|CV_COMMENT;  /* when we find closing `}'   */
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_END;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cState=CS_MAIN; /* goto normal handler */
        cValid=CV_SEMI|CV_FUNCT|CV_VAR|CV_OPERATOR|CV_ORBRAC|CV_OBRAC|
               CV_RESWRD|CV_COMMENT;
        break;
      case CS_START+CRN_EOF:
        sprintf(cError,"compile error: nothing to compile\n");
        CompileErrorReport(cError);
        return(COMPILE_SYNTAX_ERROR);
        break;

/* VV     VV   AAAA   RRRRR   
 *  VV   VV   AA  AA  RR  RR 
 *   VV VV    AAAAAA  RRRR
 *    VVV     AA  AA  RR RR 
 *     V      AA  AA  RR  RR */

      /* CS_VAR: allocate & specify variables */
      case CS_VAR+CRN_RESWRD: /* domain reader */
        if (cCurVar>=CMAX_VAR) {
          sprintf(cError,"compile error: variable table full. You've got too many variables!\n");
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        if ((cReserved==CRES_INT)||
            (cReserved==CRES_STR)||
            (cReserved==CRES_EXIT)||
            (cReserved==CRES_EXTRA)||
            (cReserved==CRES_THING)) {
          cVarTable[cCurVar].cDomain=CDOMAIN_LOCAL; /* default to local */
          cVarTable[cCurVar].cOffset=cCurLocal; 
          cCurLocal++;
          COMPILE_GETLASTSRC(); /* backup so our `type' can be read */
        } else if(cReserved==CRES_LOCAL) {
          cVarTable[cCurVar].cDomain=CDOMAIN_LOCAL;
          cVarTable[cCurVar].cOffset=cCurLocal; 
          cCurLocal++;
        } else if(cReserved==CRES_GLOBAL) {
          cVarTable[cCurVar].cDomain=CDOMAIN_GLOBAL; 
          cVarTable[cCurVar].cOffset=0; /*****CHANGE*****/
        } else if(cReserved==CRES_PRIVATE) {
          cVarTable[cCurVar].cDomain=CDOMAIN_PRIVATE;
          cVarTable[cCurVar].cOffset=0; /*****CHANGE*****/
        } else {
          sprintf(cError,"compile error: unrecognized variable domain `%s'. This should not happen.\nRecord your program and this message, and contact your Sys Admin.\n",cLabel);
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        cValid=CV_COMMENT|CV_RESWRD;
        cState=CS_VAR+0x100;
        break;
      case CS_VAR+0x100+CRN_RESWRD: /* type reader */
        if (cReserved==CRES_INT) {
          cVarTable[cCurVar].cType=CDT_INT;
        } else if (cReserved==CRES_STR) {
          cVarTable[cCurVar].cType=CDT_STR;
        } else if (cReserved==CRES_THING) {
          cVarTable[cCurVar].cType=CDT_THING;
        } else if (cReserved==CRES_EXTRA) {
          cVarTable[cCurVar].cType=CDT_EXTRA;
        } else if (cReserved==CRES_EXIT) {
          cVarTable[cCurVar].cType=CDT_EXIT;
        } else {
          sprintf(cError,"compile error: unrecognized variable type `%s'. This should not happen.\nRecord your program and this message, and contact your Sys Admin.\n",cLabel);
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        cValid=CV_COMMENT|CV_UNDEF|CV_VAR;
        cState=CS_VAR+0x200;
        break;
      case CS_VAR+0x200+CRN_UNDEF: /* we know type,domain. Define the sucker */
        if (strlen(cLabel)>CVAR_NAME_LENGTH) {
          sprintf(cError,"compile error: variable name `%s' exceeds length limit of %i chars.\n",cLabel,CVAR_NAME_LENGTH);
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        strcpy(cVarTable[cCurVar].cText,cLabel);
        cValid=CV_COMMENT|CV_COMMA|CV_SEMI;
        cState=CS_VAR+0x300;
        cCurVar++;
        break;
      case CS_VAR+0x200+CRN_VAR:
        sprintf(cError,"compile error: variable `%s' is already defined!\n",cLabel);
        CompileErrorReport(cError);
        return(COMPILE_SYNTAX_ERROR);
        break;
      case CS_VAR+0x300+CRN_COMMA: /* define another of the same */
        if (cCurVar>=CMAX_VAR) {
          sprintf(cError,"compile error: variable table full. You've got too many variables!\n");
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        cVarTable[cCurVar].cType=cVarTable[cCurVar-1].cType;
        cVarTable[cCurVar].cDomain=cVarTable[cCurVar-1].cDomain;
        if (cVarTable[cCurVar].cDomain==CDOMAIN_LOCAL) {
          cVarTable[cCurVar].cOffset=cCurLocal; 
          cCurLocal++;
        } else if (cVarTable[cCurVar].cDomain==CDOMAIN_GLOBAL) {
          cVarTable[cCurVar].cOffset=0; /*****CHANGE*****/
           /* ADD VARIABLE TO GLOBAL TABLE */
        } else if (cVarTable[cCurVar].cDomain==CDOMAIN_PRIVATE) {
          cVarTable[cCurVar].cOffset=0; /*****CHANGE*****/
           /* ADD VARIABLE TO PRIVATE TABLE */
        } else {
          sprintf(cError,"compile error: unrecognized variable domain. This should not happen.\nRecord your program and this message, and contact your Sys Admin.\n");
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        cValid=CV_COMMENT|CV_UNDEF|CV_VAR;
        cState=CS_VAR+0x200; /* go back and define another */
        break;
      case CS_VAR+0x300+CRN_SEMI:
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;

/* MMM    MMM   AAAA   IIIIII  NNN   NN
 * MMMM  MMMM  AA  AA    II    NNNN  NN
 * MM MMMM MM  AAAAAA    II    NN NN NN
 * MM  MM  MM  AA  AA    II    NN  NNNN
 * MM      MM  AA  AA  IIIIII  NN   NNN  */

      /* CS_MAIN: main loop - process a 'statement'. */
      case CS_MAIN+CRN_OBRAC: /* we've encountered a { - compound statement */
        cTemp.cInstruction=COP_NULL;
        cTemp.cData.cuValid=CV_SEMI|CV_FUNCT|CV_VAR|CV_OPERATOR|CV_ORBRAC|
                            CV_OBRAC|CV_RESWRD|CV_COMMENT|CV_CBRAC;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_MAIN+0x200; /* brace handler */
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cState=CS_MAIN; /* call statement handler */
        cValid=CV_COMMENT|CV_VAR|CV_FUNCT|CV_RESWRD|CV_ORBRAC|CV_OPERATOR|
               CV_OBRAC|CV_SEMI|CV_CBRAC;
        /* NOTE: THIS IS THE ONLY PLACE ALLOWD TO CALL CS_MAIN WITH CV_CBRAC */
        break;
      case CS_MAIN+CRN_CBRAC: /* closing char - empty { } */
        /* THIS SHOULD ONLY HAPPEN IMMEDIATELY FOLLOWING CS_MAIN+CRN_OBRAC */
        /* IN THE EVENT THAT THE CODER ENTERED " { } "                     */
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp);
        COMPILE_PULL(&cTemp);
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_MAIN+CRN_SEMI: /* null statement */
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_MAIN+CRN_FUNCT: /* expression statement */
      case CS_MAIN+CRN_VAR:
      case CS_MAIN+CRN_OPERATOR:
      case CS_MAIN+CRN_ORBRAC:
        COMPILE_GETLASTSRC(); /* whoah! backup! */
        cTemp.cInstruction=COP_NULL;
        cTemp.cData.cuValid=CV_SEMI|CV_COMMENT;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_MAIN+0x100; /* goto wrapup after this */
        COMPILE_PUSH(&cTemp);
        cState=CS_GET_EXP;  /* Ok: we're just throwing the whole line at   */
                            /* CS_GET_EXP. It's going to return us cVarFlag*/
                            /* and cExpFlag. We then let our wrapup state  */
                            /* compare what we get back to what we expected*/
                            /* (ie what we just put on the stack) and let  */
                            /* it handle any errors. The CS_GET_EXP should */
                            /* be able to handle anything with the exprsn. */
        cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR|
               CV_OPERATOR;
        COMPILE_UPSTACK();
        break;
      case CS_MAIN+CRN_RESWRD:
        if ((cReserved==CRES_INT)||
            (cReserved==CRES_STR)||
            (cReserved==CRES_THING)||
            (cReserved==CRES_EXIT)||
            (cReserved==CRES_EXTRA)||
            (cReserved==CRES_LOCAL)||
            (cReserved==CRES_GLOBAL)||
            (cReserved==CRES_PRIVATE)) {
          COMPILE_GETLASTSRC();
          cState=CS_VAR; /* goto variable handler - let it return to caller. */
          cValid=CV_RESWRD|CV_COMMENT;
        } else if (cReserved==CRES_IF) {
          COMPILE_GETLASTSRC();
          cState=CS_IF; /* goto if handler - let it return to caller. */
          cValid=CV_RESWRD|CV_COMMENT;
        } else if (cReserved==CRES_WHILE) {
          COMPILE_GETLASTSRC();
          cState=CS_WHILE; /* goto if handler - let it return to caller. */
          cValid=CV_RESWRD|CV_COMMENT;
        } else if (cReserved==CRES_ELSE) {
          sprintf(cError,"compile error: reserved word `%s' is invalid here.\n",cLabel);
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        } else if((cReserved==CRES_STOP)||
                  (cReserved==CRES_RETURN)) {
          COMPILE_GETLASTSRC();
          cState=CS_STOP; /* goto if handler - let it return to caller. */
          cValid=CV_RESWRD|CV_COMMENT;
        } else if ((cReserved==CRES_CHAR)||
                   (cReserved==CRES_CONST)||
                   (cReserved==CRES_DOUBLE)||
                   (cReserved==CRES_FLOAT)||
                   (cReserved==CRES_LONG)||
                   (cReserved==CRES_PUBLIC)||
                   (cReserved==CRES_SHORT)||
                   (cReserved==CRES_SIGNED)||
                   (cReserved==CRES_STRUCT)||
                   (cReserved==CRES_TYPEDEF)||
                   (cReserved==CRES_UNION)||
                   (cReserved==CRES_UNSIGNED)||
                   (cReserved==CRES_CONTINUE)||
                   (cReserved==CRES_VOID)) {
          sprintf(cError,"compile error: reserved word `%s' not supported.\n",cLabel);
          CompileErrorReport(cError);
          sprintf(cError,"Only `str', `int', `thing', `local', `global', and `private' are valid\n");
          CompileWarningReport(cError);
          sprintf(cError,"variable types.\n");
          CompileWarningReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        } else {
          sprintf(cError,"compile error: reserved word `%s' not supported\n",cLabel);
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        break;
      case CS_MAIN+0x100+CRN_SEMI: /* this is our CS_MAIN expr wrapup state */
        if ((!(cExpFlag & CEFLAG_FUNCT))&&(!(cExpFlag&CEFLAG_EXP))) {
          sprintf(cError,"compile error: this is not a valid assignment or function call.\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        /* Everything came back ok! */
        /* Since this is the MAIN loop, and both a function and an     */
        /* expression will leave a value on the stack, compile a pop.  */
        cTemp.cInstruction=COP_POP;
        COMPILE_COMPILE(&cTemp);
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_MAIN+0x200+CRN_CBRAC: /* close brace `}' */
        COMPILE_DOWNSTACK(); /* return to caller */
        COMPILE_PULL(&cTemp);
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_MAIN+0x200+CRN_SEMI: /* more of this compound statement! */
      case CS_MAIN+0x200+CRN_FUNCT:
      case CS_MAIN+0x200+CRN_VAR:
      case CS_MAIN+0x200+CRN_OPERATOR:
      case CS_MAIN+0x200+CRN_ORBRAC:
      case CS_MAIN+0x200+CRN_OBRAC:
      case CS_MAIN+0x200+CRN_RESWRD: 
        COMPILE_GETLASTSRC(); /* there's more in this {}! */
                              /* backup & do it again.    */
        cTemp.cInstruction=COP_NULL;
        cTemp.cData.cuValid=CV_SEMI|CV_FUNCT|CV_VAR|CV_OPERATOR|CV_ORBRAC|
                            CV_OBRAC|CV_RESWRD|CV_COMMENT|CV_CBRAC;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_MAIN+0x200;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cValid=CV_SEMI|CV_FUNCT|CV_VAR|CV_OPERATOR|CV_ORBRAC|CV_OBRAC|
               CV_RESWRD|CV_COMMENT;
        cState=CS_MAIN;
        break;

/* IIIIII FFFFFF
 *   II   FF
 *   II   FFFF
 *   II   FF
 * IIIIII FF         */

      case CS_IF+CRN_RESWRD:
        cValid=CV_ORBRAC|CV_COMMENT; /* now, expect `(' */
        cState=CS_IF+0x100;
        break;
      case CS_IF+0x100+CRN_ORBRAC:
        cTemp.cInstruction=COP_NULL;
        cTemp.cData.cuValid=CV_COMMENT|CV_CRBRAC; 
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_IF+0x200;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR
               |CV_OPERATOR|CV_CRBRAC;
        cState=CS_GET_EXP; /* get expression to evaluate */
        break;
      case CS_IF+0x200+CRN_CRBRAC:
        if (!(cExpFlag&CEFLAG_RVALUE)) { /* nothing to evaluate */
          CompileErrorReport("compile error: empty `if' (nothing to evaluate)\n");
          return(COMPILE_SYNTAX_ERROR);
        }
        if (!(cVarFlag&CDFLAG_INT)) { /* no int to evaluate! */
          CompileErrorReport("compile error: `if' must have integer expression!\n");
          return(COMPILE_SYNTAX_ERROR);
        }
        cTemp.cInstruction=COP_NULL; /* save `if' location to adj. offset jmp */
        cTemp.cData.cuOffset=cComp;
        COMPILE_PUSH(&cTemp);
        cTemp.cInstruction=COP_IFZ;
        cTemp.cData.cuPointer=0;
        COMPILE_COMPILE(&cTemp);
        /* we don't know what we're returning to, nor do we care! */
        /* Unless, of course, it's an `else'.                     */
        cTemp.cData.cuValid=CV_EOF|CV_OPERATOR|CV_COMMENT|CV_COMMA|CV_PERIOD|
                            CV_OSBRAC|CV_CSBRAC|CV_OBRAC|CV_CBRAC|CV_ORBRAC|
                            CV_CRBRAC|CV_STRING|CV_SEMI|CV_NUMBER|CV_RESWRD|
                            CV_FUNCT|CV_VAR|CV_UNDEF;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_IF+0x300;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cState=CS_MAIN; /* compile statement */
        cValid=CV_SEMI|CV_FUNCT|CV_VAR|CV_OPERATOR|CV_ORBRAC|CV_OBRAC|
               CV_RESWRD|CV_COMMENT;
        break;
      case CS_IF+0x300+CRN_EOF:
      case CS_IF+0x300+CRN_OPERATOR:
      case CS_IF+0x300+CRN_COMMENT:
      case CS_IF+0x300+CRN_COMMA:
      case CS_IF+0x300+CRN_PERIOD:
      case CS_IF+0x300+CRN_OSBRAC:
      case CS_IF+0x300+CRN_CSBRAC:
      case CS_IF+0x300+CRN_OBRAC:
      case CS_IF+0x300+CRN_CBRAC:
      case CS_IF+0x300+CRN_ORBRAC:
      case CS_IF+0x300+CRN_CRBRAC:
      case CS_IF+0x300+CRN_STRING:
      case CS_IF+0x300+CRN_SEMI:
      case CS_IF+0x300+CRN_NUMBER:
      case CS_IF+0x300+CRN_FUNCT:
      case CS_IF+0x300+CRN_UNDEF:
      case CS_IF+0x300+CRN_VAR:
      case CS_IF+0x400+CRN_EOF:      /* yes, overlapping states. But look at it.*/
      case CS_IF+0x400+CRN_OPERATOR: /* This is saving a fair amount of code.   */
      case CS_IF+0x400+CRN_COMMENT:
      case CS_IF+0x400+CRN_COMMA:
      case CS_IF+0x400+CRN_PERIOD:
      case CS_IF+0x400+CRN_OSBRAC:
      case CS_IF+0x400+CRN_CSBRAC:
      case CS_IF+0x400+CRN_OBRAC:
      case CS_IF+0x400+CRN_CBRAC:
      case CS_IF+0x400+CRN_ORBRAC:
      case CS_IF+0x400+CRN_CRBRAC:
      case CS_IF+0x400+CRN_STRING:
      case CS_IF+0x400+CRN_SEMI:
      case CS_IF+0x400+CRN_NUMBER:
      case CS_IF+0x400+CRN_FUNCT:
      case CS_IF+0x400+CRN_UNDEF:
      case CS_IF+0x400+CRN_VAR:
      case CS_IF+0x400+CRN_RESWRD: /* second time - no check for an 'else' */
        COMPILE_PULL(&cTemp);
        cCompBuf[cTemp.cData.cuPointer+1]=(BYTE)(cComp&0x00ff); /* adj if jmp */
        cCompBuf[cTemp.cData.cuPointer+2]=(BYTE)((cComp&0xff00)>>8);
        /* boogey on back to home base - ie return to caller */
        COMPILE_GETLASTSRC();
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_IF+0x300+CRN_RESWRD:
        if (cReserved==CRES_ELSE) { /* there's an else! */
          COMPILE_PULL(&cTemp); /* mark zero  */
          cCompBuf[cTemp.cData.cuPointer+1]=(BYTE)((cComp+3)&0x00ff); /* adj if jmp */
          cCompBuf[cTemp.cData.cuPointer+2]=(BYTE)(((cComp+3)&0xff00)>>8);
          cTemp.cInstruction=COP_NULL; /* save `goto' location to adj. offset jmp */
          cTemp.cData.cuOffset=cComp;
          COMPILE_PUSH(&cTemp);
          cTemp.cInstruction=COP_GOTO;
          cTemp.cData.cuPointer=0;
          COMPILE_COMPILE(&cTemp);
          /* we don't know what we're returning to, nor do we care! */
          /* Unless, of course, it's an `else'.                     */
          cTemp.cData.cuValid=CV_EOF|CV_OPERATOR|CV_COMMENT|CV_COMMA|CV_PERIOD|
                              CV_OSBRAC|CV_CSBRAC|CV_OBRAC|CV_CBRAC|CV_ORBRAC|
                              CV_CRBRAC|CV_STRING|CV_SEMI|CV_NUMBER|CV_RESWRD|
                              CV_FUNCT|CV_VAR|CV_UNDEF;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuState=CS_IF+0x400;
          COMPILE_PUSH(&cTemp);
          COMPILE_UPSTACK();
          cState=CS_MAIN; /* compile statement */
          cValid=CV_SEMI|CV_FUNCT|CV_VAR|CV_OPERATOR|CV_ORBRAC|CV_OBRAC|
                 CV_RESWRD|CV_COMMENT;
          break;
        } else {
          COMPILE_PULL(&cTemp);
          cCompBuf[cTemp.cData.cuPointer+1]=(BYTE)(cComp&0x00ff); /* adj if jmp */
          cCompBuf[cTemp.cData.cuPointer+2]=(BYTE)((cComp&0xff00)>>8);
          /* boogey on back to home base - ie return to caller */
          COMPILE_GETLASTSRC();
          COMPILE_DOWNSTACK();
          COMPILE_PULL(&cTemp); /* return to calling state */
          cState=cTemp.cData.cuState;
          COMPILE_PULL(&cTemp);
          cValid=cTemp.cData.cuValid;
        }
        break;

/* WW     WW  HH   HH  IIIIII  LL      EEEEEE
 * WW WWW WW  HH   HH    II    LL      EE
 * WWWW WWWW  HHHHHHH    II    LL      EEEE
 * WWW   WWW  HH   HH    II    LL      EE
 * WW     WW  HH   HH  IIIIII  LLLLLL  EEEEEE          */

      case CS_WHILE+CRN_RESWRD:
        cValid=CV_ORBRAC|CV_COMMENT; /* now, expect `(' */
        cState=CS_WHILE+0x100;
        break;
      case CS_WHILE+0x100+CRN_ORBRAC:
        cTemp.cInstruction=COP_GOTO; /* save `goto' location to adj. goto */
        cTemp.cData.cuOffset=cComp;
        COMPILE_PUSH(&cTemp);
        cTemp.cInstruction=COP_NULL;
        cTemp.cData.cuValid=CV_COMMENT|CV_CRBRAC; 
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_WHILE+0x200;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR
               |CV_OPERATOR|CV_CRBRAC;
        cState=CS_GET_EXP; /* get expression to evaluate */
        break;
      case CS_WHILE+0x200+CRN_CRBRAC:
        if (!(cExpFlag&CEFLAG_RVALUE)) { /* nothing to evaluate */
          CompileErrorReport("compile error: empty `while' (nothing to evaluate)\n");
          return(COMPILE_SYNTAX_ERROR);
        }
        if (!(cVarFlag&CDFLAG_INT)) { /* no int to evaluate! */
          CompileErrorReport("compile error: `if' must have integer expression!\n");
          return(COMPILE_SYNTAX_ERROR);
        }
        cTemp.cInstruction=COP_NULL; /* save `while' location to adj. goto */
        cTemp.cData.cuOffset=cComp;
        COMPILE_PUSH(&cTemp);
        cTemp.cInstruction=COP_WHILEZ;
        cTemp.cData.cuPointer=0;
        COMPILE_COMPILE(&cTemp);
        /* we don't know what we're returning to, nor do we care! */
        /* Unless, of course, it's an `else'.                     */
        cTemp.cData.cuValid=CV_EOF|CV_OPERATOR|CV_COMMENT|CV_COMMA|CV_PERIOD|
                            CV_OSBRAC|CV_CSBRAC|CV_OBRAC|CV_CBRAC|CV_ORBRAC|
                            CV_CRBRAC|CV_STRING|CV_SEMI|CV_NUMBER|CV_RESWRD|
                            CV_FUNCT|CV_VAR|CV_UNDEF;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_WHILE+0x300;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cState=CS_MAIN; /* compile statement */
        cValid=CV_SEMI|CV_FUNCT|CV_VAR|CV_OPERATOR|CV_ORBRAC|CV_OBRAC|
               CV_RESWRD|CV_COMMENT;
        break;
      case CS_WHILE+0x300+CRN_EOF:
      case CS_WHILE+0x300+CRN_OPERATOR:
      case CS_WHILE+0x300+CRN_COMMENT:
      case CS_WHILE+0x300+CRN_COMMA:
      case CS_WHILE+0x300+CRN_PERIOD:
      case CS_WHILE+0x300+CRN_OSBRAC:
      case CS_WHILE+0x300+CRN_CSBRAC:
      case CS_WHILE+0x300+CRN_OBRAC:
      case CS_WHILE+0x300+CRN_CBRAC:
      case CS_WHILE+0x300+CRN_ORBRAC:
      case CS_WHILE+0x300+CRN_CRBRAC:
      case CS_WHILE+0x300+CRN_STRING:
      case CS_WHILE+0x300+CRN_SEMI:
      case CS_WHILE+0x300+CRN_NUMBER:
      case CS_WHILE+0x300+CRN_FUNCT:
      case CS_WHILE+0x300+CRN_UNDEF:
      case CS_WHILE+0x300+CRN_RESWRD:
      case CS_WHILE+0x300+CRN_VAR:
        COMPILE_PULL(&cTemp); /* mark zero  */
        cCompBuf[cTemp.cData.cuPointer+1]=(BYTE)((cComp+3)&0x00ff); /* adj while jmp */
        cCompBuf[cTemp.cData.cuPointer+2]=(BYTE)(((cComp+3)&0xff00)>>8);
        COMPILE_COMPILESTACK();
        /* boogey on back to home base - ie return to caller */
        COMPILE_GETLASTSRC();
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;

/* EEEEEE  XX    XX  IIIIII  TTTTTT
 * EE       XX  XX     II      TT 
 * EEEE      XXXX      II      TT 
 * EE       XX  XX     II      TT 
 * EEEEEE  XX    XX  IIIIII    TT      */

      case CS_STOP+CRN_RESWRD:
        cValid=CV_SEMI|CV_COMMENT; /* now, expect `;' */
        cState=CS_STOP+0x100;
        cTemp.cInstruction=COP_TERM;
        COMPILE_COMPILE(&cTemp);
        break;
      case CS_STOP+0x100+CRN_SEMI:
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;

/*  GGGGG  EEEEEE  TTTTTT         EEEEEE  XX    XX  PPPPP
 * GG      EE        TT           EE       XX  XX   PP  PP
 * GG GGG  EEEE      TT           EEEE      XXXX    PPPPP
 * GG  GG  EE        TT           EE       XX  XX   PP
 *  GGGGG  EEEEEE    TT   ______  EEEEEE  XX    XX  PP      */

      /* just a note before we get into this.                          */
      /* Get_Exp returns, in cExpFlag and cVarFlag, the type of        */
      /* expression read in. cExpFlag indicates: is it a valid lvalue? */
      /* rvalue? Is it a full equation (ie a=b+c as opposed to b+c)?   */
      /* Is it a valid function call?                                  */
      /* cVarFlag indicates the type of variables involved.            */
      case CS_GET_EXP+CRN_SEMI: /* null expression */
        cTemp.cInstruction=COP_PUSHV;
        cTemp.cData.cuSLWord=0;
        COMPILE_GETLASTSRC(); /* whoah! backup and return ; */
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_GET_EXP+CRN_CRBRAC:
        cNumParam=0; /* we got a SomeFunct() type call - no params */
        COMPILE_GETLASTSRC(); /* whoah! backup and return ) */
        cVarFlag=cExpFlag=0;
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_GET_EXP+CRN_FUNCT:
      case CS_GET_EXP+CRN_VAR:
      case CS_GET_EXP+CRN_STRING:
      case CS_GET_EXP+CRN_NUMBER:
      case CS_GET_EXP+CRN_ORBRAC:
      case CS_GET_EXP+CRN_OPERATOR: /* for -, !, ++, --, ~ */
        COMPILE_GETLASTSRC(); /* backup! */
        cTemp.cInstruction=COP_NULL; /* start with "all possibilities" */
        cTemp.cData.cuExpFlag=CEFLAG_FUNCT|CEFLAG_LVALUE|CEFLAG_RVALUE|
                              CEFLAG_EXP; 
        COMPILE_PUSH(&cTemp); /* and slowly whittle them away */
        cTemp.cData.cuVarFlag=CDFLAG_STR|CDFLAG_INT|CDFLAG_THING|CDFLAG_NULL|
                              CDFLAG_EXTRA|CDFLAG_EXIT;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuValid=CV_COMMENT|CV_OPERATOR|CV_COMMA|CV_SEMI|CV_CRBRAC;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_GET_EXP+0x100;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cValid=CV_COMMENT|CV_FUNCT|CV_NUMBER|CV_ORBRAC|CV_VAR|CV_STRING|CV_OPERATOR;
        cState=CS_GET_NEXT;
        break;
      case CS_GET_EXP+0x100+CRN_SEMI:
      case CS_GET_EXP+0x100+CRN_CRBRAC:
      case CS_GET_EXP+0x100+CRN_COMMA:
        COMPILE_PULL(&cTemp); /* pull cVarFlag */
        cVarFlag=cVarFlag & cTemp.cData.cuVarFlag;
        COMPILE_PULL(&cTemp); /* pull cExpFlag */
        cExpFlag=cExpFlag & cTemp.cData.cuExpFlag;
        if (!(cVarFlag)) { /* wrong data type */
          sprintf(cError,"compile error: incompatible data types.\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        /* compile anything left on the stack */
        while(cSP) {
          COMPILE_COMPILESTACK();
          } 
        /* backup and exit */
        COMPILE_GETLASTSRC(); 
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* pull cState */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp); /* pull cValid */
        cValid=cTemp.cData.cuValid;
        break;
      case CS_GET_EXP+0x100+CRN_OPERATOR:
        COMPILE_PULL(&cTemp); /* pull cVarFlag */
        cVarFlag=cVarFlag & cTemp.cData.cuVarFlag;
        COMPILE_PULL(&cTemp); /* pull cExpFlag */
        cExpFlag=cExpFlag & cTemp.cData.cuExpFlag; /* compare data types */
        if (!(cVarFlag)) { /* wrong data type */
          sprintf(cError,"compile error: incompatible data types.\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        if ((cOperation&0xff)<=COP_EQU) { /* equate operation */
          if (!(cExpFlag&CEFLAG_LVALUE)) { /* invalid lvalue */
            sprintf(cError,"compile error: invalid lvalue in expression <lvalue>=<rvalue>\n");
            CompileErrorReport(cError);
            return(COMPILE_SYNTAX_ERROR);
          }
          if (((cOperation&0xff)!=COP_EQU)&& /* check for non-`=' and non-ints!*/
              (!(cVarFlag&CDFLAG_INT))) {
            sprintf(cError,"compile error: can only use this operator on `int' types.\n");
            CompileErrorReport(cError);
            return(COMPILE_SYNTAX_ERROR);
          }
          cTemp.cInstruction=cOperation&0xff;
          COMPILE_PUSH(&cTemp);
          /* just call CS_GET_EXP, make sure it's a valid rvalue, & exit */
          cTemp.cInstruction=COP_NULL;
          cTemp.cData.cuExpFlag=cExpFlag;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuVarFlag=cVarFlag;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuValid=CV_COMMENT|CV_SEMI|CV_COMMA|CV_CRBRAC;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuState=CS_GET_EXP+0x200;
          COMPILE_PUSH(&cTemp);
          /* and call CS_GET_EXP */
          COMPILE_UPSTACK();
          cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR|CV_OPERATOR;
          cState=CS_GET_EXP;
        } /* if (equate) */
        else if (((cOperation&0xff)==COP_ASUB)||
                 ((cOperation&0xff)==COP_AADD)) { /* ++ or -- */
          if (!(cVarFlag&CDFLAG_INT)) { /* invalid op with this type */
            sprintf(cError,"compile error: can only use this operator on `int' types.\n");
            CompileErrorReport(cError);
            return(COMPILE_SYNTAX_ERROR);
          }
          if (!(cExpFlag&CEFLAG_LVALUE)) { /* no lvalue! AAAAA */
            sprintf(cError,"compile error: modifiable lvalue (ie a variable) is needed to use ++ or --\n");
            CompileErrorReport(cError);
            return(COMPILE_SYNTAX_ERROR);
          }
          cExpFlag=(cExpFlag|CEFLAG_LVALUE|CEFLAG_FUNCT)
                  -(CEFLAG_LVALUE|CEFLAG_FUNCT);
          cExpFlag|=CEFLAG_EXP;
          cTemp.cInstruction=cOperation&0xFF;
          COMPILE_COMPILE(&cTemp); /* compile unary operator */
          cTemp.cInstruction=COP_NULL; /* and push back cExpFlag & cVarFlag */
          cTemp.cData.cuExpFlag=cExpFlag;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuVarFlag=cVarFlag;
          COMPILE_PUSH(&cTemp);
        } /* -- or ++ operators */
        else if ( (((cOperation&0xff)==COP_BEQU)||
                   ((cOperation&0xff)==COP_BNEQU))&& 
                  (!(cVarFlag&CDFLAG_INT)) )  { /* == or != w/ non-int */
          /* this is a special case because we have to turn a non-int  */
          /* expression such as `a==b' (assuming a & b are non-ints)   */
          /* into an int expression so it can be evaluated properly!   */
          cTemp.cInstruction=cOperation&0xff;
          COMPILE_PUSH(&cTemp);
          /* just call CS_GET_EXP, make sure it's a valid rvalue, modify cVarFlags, & exit */
          cTemp.cInstruction=COP_NULL;
          cTemp.cData.cuExpFlag=cExpFlag;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuVarFlag=cVarFlag;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuValid=CV_COMMENT|CV_SEMI|CV_COMMA|CV_CRBRAC;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuState=CS_GET_EXP+0x300;
          COMPILE_PUSH(&cTemp);
          /* and call CS_GET_EXP */
          COMPILE_UPSTACK();
          cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR|CV_OPERATOR;
          cState=CS_GET_EXP;
        } /* == or != w/ non-int */
        else { /* everything else (non-equate operation) */
          if (!(cVarFlag&CDFLAG_INT)) { /* invalid op with this type */
            sprintf(cError,"compile error: can only use this operator on `int' types.\n");
            CompileErrorReport(cError);
            return(COMPILE_SYNTAX_ERROR);
          }
          /* adjust cExpFlag - it can't be these & have an operator! */
          cExpFlag=(cExpFlag|CEFLAG_LVALUE|CEFLAG_FUNCT|CEFLAG_EXP)
                  -(CEFLAG_LVALUE|CEFLAG_FUNCT|CEFLAG_EXP);
          /* push the instruction */
          cTemp.cInstruction=0xff; 
          while((cTemp.cInstruction > (cOperation&0xff)) && cSP) {
            COMPILE_PULL(&cTemp);
            if (cTemp.cInstruction > (cOperation&0xff)) {
              COMPILE_COMPILE(&cTemp);
            } /* if */
            else {
              COMPILE_PUSH(&cTemp);
            } /* else */
          } /* while */
          cTemp.cInstruction=cOperation&0xff;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuExpFlag=cExpFlag; /* save everything & get next thing */
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuVarFlag=cVarFlag;
          COMPILE_PUSH(&cTemp);  
          cTemp.cData.cuValid=CV_COMMENT|CV_OPERATOR|CV_COMMA|CV_SEMI|CV_CRBRAC;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuState=CS_GET_EXP+0x100;
          COMPILE_PUSH(&cTemp);
          COMPILE_UPSTACK();
          cValid=CV_COMMENT|CV_FUNCT|CV_NUMBER|CV_ORBRAC|CV_VAR|CV_STRING|CV_OPERATOR;
          cState=CS_GET_NEXT;
        } /* else non-equate operator */
        break;
      case CS_GET_EXP+0x200+CRN_SEMI:   /* this state handles expressions */
      case CS_GET_EXP+0x200+CRN_CRBRAC:
      case CS_GET_EXP+0x200+CRN_COMMA:
        COMPILE_PULL(&cTemp); /* pull cVarFlag */
        cVarFlag=cVarFlag & cTemp.cData.cuVarFlag;
        if (!(cVarFlag)) { /* wrong data type */
          sprintf(cError,"compile error: assignment involves incompatible data types.\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        COMPILE_PULL(&cTemp); /* pull cExpFlag */
        if (!(cExpFlag&CEFLAG_RVALUE)) { /* invalid rvalue */
          sprintf(cError,"compile error: invalid rvalue in expression <lvalue>=<rvalue>\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        /* note: we KNOW we have an expression, thus the CEFLAG_EXP etc above */
        cExpFlag=(cExpFlag&cTemp.cData.cuExpFlag)|CEFLAG_EXP;
        /* compile anything left on the stack */
        while(cSP) {
          COMPILE_COMPILESTACK();
        } 
        /* backup and exit */
        COMPILE_GETLASTSRC(); 
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* pull cState */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp); /* pull cValid */
        cValid=cTemp.cData.cuValid;
        break;
      case CS_GET_EXP+0x300+CRN_SEMI:   /* this state handles pointer == & != */
      case CS_GET_EXP+0x300+CRN_CRBRAC:
      case CS_GET_EXP+0x300+CRN_COMMA:
        COMPILE_PULL(&cTemp); /* pull cVarFlag */
        cVarFlag=cVarFlag & cTemp.cData.cuVarFlag;
        if (!(cVarFlag)) { /* wrong data type */
          sprintf(cError,"compile error: equality involves incompatible data types.\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        COMPILE_PULL(&cTemp); /* pull cExpFlag */
        if (!(cExpFlag&CEFLAG_RVALUE)) { /* invalid rvalue */
          sprintf(cError,"compile error: invalid rvalue in expression <lvalue>=<rvalue>\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        cExpFlag=((cExpFlag&cTemp.cData.cuExpFlag)|
                  (CEFLAG_EXP|CEFLAG_FUNCT|CEFLAG_LVALUE))
                 -(CEFLAG_EXP|CEFLAG_FUNCT|CEFLAG_LVALUE);
        cVarFlag=CDFLAG_INT; /* mark our equate function as a proper int */
        /* compile anything left on the stack */
        while(cSP) {
          COMPILE_COMPILESTACK();
        } 
        /* backup and exit */
        COMPILE_GETLASTSRC(); 
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* pull cState */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp); /* pull cValid */
        cValid=cTemp.cData.cuValid;
        break;

/* FFFFFF  UU   UU  NNN   NN   CCCC   TTTTTT  IIIIII   OOOO   NNN   NN
 * FF      UU   UU  NNNN  NN  CC  CC    TT      II    OO  OO  NNNN  NN
 * FFFF    UU   UU  NN NN NN  CC        TT      II    OO  OO  NN NN NN
 * FF      UU   UU  NN  NNNN  CC  CC    TT      II    OO  OO  NN  NNNN
 * FF       UUUUU   NN   NNN   CCCC     TT    IIIIII   OOOO   NN   NNN */

      case CS_FUNCTION+CRN_FUNCT:
        cTemp.cInstruction=COP_EXECR;  /* push function on stack */
        cTemp.cData.cuFunction=cFunction;
        COMPILE_PUSH(&cTemp);
        cTemp.cInstruction=COP_NULL; /* push this fn's return type */
        cTemp.cData.cuParamType=fTable[cFunction].fDataType;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuUnsigned=cNumParam; /* push last fn's # params */
        COMPILE_PUSH(&cTemp);
        cNumParam=1; /* assume 1 param for this procedure - we are reset
                         to 0 if we have a ) as our first entry to our
                         expression! */
        counter=0;                     /* and push param types on stack */
        while(fTable[cFunction].fParamType[counter]!=CDT_NULL)
          counter++;
        /* you see, we want to put the parameters on the stack in reverse */
        /* order (it's a stack - get it? - FILO ring a bell?)             */
        if (!counter) { /* ie NO parameters */
          cTemp.cData.cuValid=CV_CRBRAC|CV_COMMENT|CV_COMMA;
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuState=CS_FUNCTION+0x400; /* empty ) handler */
          COMPILE_PUSH(&cTemp);
        }
        else { /* put parameter types on stack */
          counter--;
          cTemp.cData.cuParamType=fTable[cFunction].fParamType[counter];
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuValid=CV_CRBRAC|CV_COMMENT|CV_COMMA; /* first, our ) handler */
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuState=CS_FUNCTION+0x300; /* ) handler */
          COMPILE_PUSH(&cTemp);
          while(counter) { /* here's where we put them on the stack! */
            counter--;
            cTemp.cData.cuParamType=fTable[cFunction].fParamType[counter];
            COMPILE_PUSH(&cTemp);
            cTemp.cData.cuValid=CV_COMMENT|CV_COMMA|CV_CRBRAC;
            COMPILE_PUSH(&cTemp);
            cTemp.cData.cuState=CS_FUNCTION+0x200; /* , handler */
            COMPILE_PUSH(&cTemp);
          } /* while(counter) */
        } /* else */
  COMPILE_UPSTACK();
        cValid=CV_ORBRAC|CV_COMMENT; /* read ( */
        cState=CS_FUNCTION+0x100;
        break;
      case CS_FUNCTION+0x100+CRN_ORBRAC:
        /* the stack has been set up and everything, so we just have to */
        /* call CS_GET_EXP. This state is just to make sure there's a ( */
        cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR|
               CV_CRBRAC|CV_OPERATOR;
        /* NOTE: we allow a CV_CRBRAC because we might have somefunct() */
        /*       but this should not be normal practice with CS_GET_EXP.*/
        cState=CS_GET_EXP;
        break;
      case CS_FUNCTION+0x200+CRN_COMMA:
        COMPILE_PULL(&cTemp);
        if ((!(cVarFlag&(1<<cTemp.cData.cuParamType)))||
          (!(cExpFlag&CEFLAG_RVALUE))) {
          sprintf(cError,"compile error: data type of parameter to function is incorrect.\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        cNumParam++; /* increment our # parameters in this function call */
        COMPILE_UPSTACK();
        cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR|CV_OPERATOR;
        cState=CS_GET_EXP;
        break;
      case CS_FUNCTION+0x200+CRN_CRBRAC:
        /* unless our param type is CDT_ETC, too few parameters to function */
        COMPILE_PULL(&cTemp);
        if (cTemp.cData.cuParamType==CDT_ETC) { 
          /* we've finally run out of parameters with a CDT_ETC param type. */
          /* let's pack up our bags and go home. */
          COMPILE_GETLASTSRC(); 
          COMPILE_PULL(&cTemp); /* return to calling state */
          cState=cTemp.cData.cuState;
          COMPILE_PULL(&cTemp);
          cValid=cTemp.cData.cuValid;
          break;
        }
        sprintf(cError,"compile error: Bill, while I agree that, in time, our band will be\n           most triumphant, there's too few parameters passed to this function.\n");
        CompileErrorReport(cError);
        return(COMPILE_SYNTAX_ERROR);
        break;
      case CS_FUNCTION+0x300+CRN_COMMA:
        COMPILE_PULL(&cTemp);
        if (cTemp.cData.cuParamType==CDT_ETC) { /* anything goes... */
          /* anything goes - as many variables, of any type */
          /* push our CDT_ETC back on the stack - for next time, and
           * see if there's "one more variable" - which will land us
           * back here!                                                */
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuValid=CV_CRBRAC|CV_COMMENT|CV_COMMA; /* first, our ) handler */
          COMPILE_PUSH(&cTemp);
          cTemp.cData.cuState=CS_FUNCTION+0x300; /* ) handler */
          COMPILE_PUSH(&cTemp);
          cNumParam++; /* increment our # parameters in this function call */
          COMPILE_UPSTACK();
          cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR|CV_OPERATOR;
          cState=CS_GET_EXP;
          break;
        }
        /* too many parameters to function */
        sprintf(cError,"compile error: But Ted, there's too many parameters passed to this function!\n");
        CompileErrorReport(cError);
        return(COMPILE_SYNTAX_ERROR);
        break;
      case CS_FUNCTION+0x300+CRN_CRBRAC:
        COMPILE_PULL(&cTemp);
        if (cTemp.cData.cuParamType!=CDT_ETC) /* type is specified  */
           if ((!(cVarFlag&(1<<cTemp.cData.cuParamType)))||
              (!(cExpFlag&CEFLAG_RVALUE))) {
            sprintf(cError,"compile error: data type of parameter to function is incorrect.\n");
            CompileErrorReport(cError);
            return(COMPILE_SYNTAX_ERROR);
          }
        cTemp.cInstruction=COP_PUSHV; /* load in our # params specifier */
        cTemp.cData.cuSLWord=cNumParam;
        COMPILE_COMPILE(&cTemp);
        COMPILE_PULL(&cTemp); /* retrieve our last fn's cNumParam value */
        cNumParam=cTemp.cData.cuUnsigned; 
        COMPILE_PULL(&cTemp); /* get our return data type */
        cVarFlag=1<<cTemp.cData.cuParamType;
        cExpFlag=CEFLAG_RVALUE|CEFLAG_FUNCT; /* while we're setting flags */
        COMPILE_COMPILESTACK(); /* compile our COP_EXEC */
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_FUNCTION+0x400+CRN_CRBRAC: /* no-parameter ) handler */
        if (cVarFlag | cExpFlag) {
          sprintf(cError,"compile error: this function expects no parameters!\n");
          CompileErrorReport(cError);
          return(COMPILE_SYNTAX_ERROR);
        }
        cTemp.cInstruction=COP_PUSHV; /* load in our # params specifier */
        cTemp.cData.cuSLWord=cNumParam;
        COMPILE_COMPILE(&cTemp);
        COMPILE_PULL(&cTemp); /* retrieve our last fn's cNumParam value */
        cNumParam=cTemp.cData.cuUnsigned; 
        COMPILE_PULL(&cTemp); /* get our return data type */
        cVarFlag=1<<cTemp.cData.cuParamType;
        cExpFlag=CEFLAG_RVALUE|CEFLAG_FUNCT; /* while we're setting flags */
        COMPILE_COMPILESTACK(); /* compile our COP_EXEC */
        COMPILE_DOWNSTACK();
        COMPILE_PULL(&cTemp); /* return to calling state */
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_FUNCTION+0x400+CRN_COMMA: 
        /* this state is just to print out a pleasant error message          */ 
        sprintf(cError,"compile error: this function expects no parameters!\n");
        CompileErrorReport(cError);
        return(COMPILE_SYNTAX_ERROR);
        break;

/*  GGGGG  EEEEEE  TTTTTT         NNN   NN  EEEEEE  XX    XX  TTTTTT
 * GG      EE        TT           NNNN  NN  EE       XX  XX     TT
 * GG GGG  EEEE      TT           NN NN NN  EEEE      XXXX      TT
 * GG  GG  EE        TT           NN  NNNN  EE       XX  XX     TT
 *  GGGGG  EEEEEE    TT   ______  NN   NNN  EEEEEE  XX    XX    TT   */

      case CS_GET_NEXT+CRN_FUNCT:
        COMPILE_GETLASTSRC(); /* whoah! backup! */
        cState=CS_FUNCTION; /* pass function over to function handler and */
                            /* let IT return to our previous state.       */
        cValid=CV_FUNCT;
        break;
      case CS_GET_NEXT+CRN_NUMBER:
        cTemp.cInstruction=COP_PUSHV;
        cTemp.cData.cuSLWord=CompileStrToNum(cLabel);
        COMPILE_COMPILE(&cTemp);
        cVarFlag=CDFLAG_INT;
        cExpFlag=CEFLAG_RVALUE;
        COMPILE_DOWNSTACK(); /* and return to our caller */
        COMPILE_PULL(&cTemp);
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_GET_NEXT+CRN_STRING:
        cTemp.cInstruction=COP_PUSHPT;
        cTemp.cData.cuPtr=STRCREATE(cLabel);
        COMPILE_COMPILE(&cTemp);
        cVarFlag=CDFLAG_STR;
        cExpFlag=CEFLAG_RVALUE;
        COMPILE_DOWNSTACK(); /* and return to our caller */
        COMPILE_PULL(&cTemp);
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_GET_NEXT+CRN_ORBRAC:
        cValid=CV_COMMENT|CV_ORBRAC|CV_STRING|CV_NUMBER|CV_FUNCT|CV_VAR|
               CV_CRBRAC|CV_OPERATOR;
        cState=CS_GET_EXP; /* let's get the expression in this bracket */
        cTemp.cInstruction=COP_NULL;
        cTemp.cData.cuValid=CV_CRBRAC|CV_COMMENT; /* and return to ) handler */
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_GET_NEXT+0x100;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        break;
      case CS_GET_NEXT+CRN_VAR:
        if (cVarTable[cVar].cDomain==CDOMAIN_LOCAL) {
          cTemp.cInstruction=COP_PUSHL;
        } else if (cVarTable[cVar].cDomain==CDOMAIN_GLOBAL) {
          cTemp.cInstruction=COP_PUSHG;
        } else if (cVarTable[cVar].cDomain==CDOMAIN_PRIVATE) {
          cTemp.cInstruction=COP_PUSHP;
        } else {
          sprintf(cError,"internal error (compile.c): Unknown domain for `%s'.\nThis shouldn't happen. Notify your Sys Admin.\n",cVarTable[cVar].cText);
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        cTemp.cData.cuVar=cVarTable[cVar].cOffset;
        COMPILE_COMPILE(&cTemp);
        if (cVarTable[cVar].cType==CDT_INT) {
          cVarFlag=CDFLAG_INT;
        } else if (cVarTable[cVar].cType==CDT_STR) {
          cVarFlag=CDFLAG_STR;
        } else if (cVarTable[cVar].cType==CDT_THING) {
          cVarFlag=CDFLAG_THING;
        } else if (cVarTable[cVar].cType==CDT_EXTRA) {
          cVarFlag=CDFLAG_EXTRA;
        } else if (cVarTable[cVar].cType==CDT_EXIT) {
          cVarFlag=CDFLAG_EXIT;
        } else {
          sprintf(cError,"internal error (compile.c): Unknown variable type for `%s'.\nThis shouldn't happen. Notify your Sys Admin.\n",cVarTable[cVar].cText);
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        if ((cVarTable[cVar].cDomain==CDOMAIN_LOCAL)&&
            (cVarTable[cVar].cOffset<cSystemVariableStatic)) {
          cExpFlag=CEFLAG_RVALUE;
        } else {
          cExpFlag=CEFLAG_RVALUE|CEFLAG_LVALUE;
        }
        COMPILE_DOWNSTACK(); /* and return to our caller */
        COMPILE_PULL(&cTemp);
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_GET_NEXT+CRN_OPERATOR: /* for `, !, \, ++, -- */
        if ((cOperation&0xff)==COP_SUB) { /* conversion of `-' */
          cOperation=((cOperation&0xff00)+COP_NEG);
        }
        if ((cOperation&0xff)==COP_ASUB) { /* conversion of `--' */
          cOperation=((cOperation&0xff00)+COP_BSUB);
        }
        if ((cOperation&0xff)==COP_AADD) { /* conversion of `++' */
          cOperation=((cOperation&0xff00)+COP_BADD);
        }

        if (((cOperation&0xff)!=COP_COMP)&&
            ((cOperation&0xff)!=COP_BSUB)&&
            ((cOperation&0xff)!=COP_BADD)&&
            ((cOperation&0xff)!=COP_NOT)&&
            ((cOperation&0xff)!=COP_NEG)) { /* non-unary operation */
          CompileErrorReport("compile error: unexpected operator.\n");
          return(COMPILE_SYNTAX_ERROR);
        }
        cTemp.cInstruction=cOperation&0xff;  /* save instruction */
        COMPILE_PUSH(&cTemp);
        cTemp.cInstruction=COP_NULL; 
        cTemp.cData.cuValid=CV_COMMENT|CV_OPERATOR|CV_COMMA|CV_SEMI|CV_CRBRAC;
        COMPILE_PUSH(&cTemp);
        cTemp.cData.cuState=CS_GET_NEXT+0x200;
        COMPILE_PUSH(&cTemp);
        COMPILE_UPSTACK();
        cValid=CV_COMMENT|CV_FUNCT|CV_NUMBER|CV_ORBRAC|CV_VAR|CV_STRING|CV_OPERATOR;
        cState=CS_GET_NEXT;
        break;
      case CS_GET_NEXT+0x100+CRN_CRBRAC:
        /* cVarFlag is set. We just adjust cExpFlag to reflect ()'s and exit */
        cExpFlag=(cExpFlag|CEFLAG_LVALUE|CEFLAG_FUNCT)
                -(CEFLAG_LVALUE|CEFLAG_FUNCT);
        /* ie: since there's ()'s around it, it can't be funct, lvalue, exp.*/
        COMPILE_DOWNSTACK(); /* and return to our caller */
        COMPILE_PULL(&cTemp);
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;
      case CS_GET_NEXT+0x200+CRN_OPERATOR: /* process uniary operator */
      case CS_GET_NEXT+0x200+CRN_SEMI:
      case CS_GET_NEXT+0x200+CRN_COMMA:
      case CS_GET_NEXT+0x200+CRN_CRBRAC:
        COMPILE_GETLASTSRC(); /* whoah! backup! */
        COMPILE_PULL(&cTemp); /* get instruction */
        if ((cTemp.cInstruction==COP_BSUB)||
            (cTemp.cInstruction==COP_BADD)) { /* we need an lvalue for these */
          if (!(cExpFlag&CEFLAG_LVALUE)) { /* no lvalue! AAAAA */
            sprintf(cError,"compile error: modifiable lvalue (ie a variable) is needed to use ++ or --\n");
            CompileErrorReport(cError);
            return(COMPILE_SYNTAX_ERROR);
          }
        }
        /* modify cExpFlag - it can't be any of these: */
        cExpFlag=(cExpFlag|CEFLAG_EXP|CEFLAG_LVALUE|CEFLAG_FUNCT)
                -(CEFLAG_EXP|CEFLAG_LVALUE|CEFLAG_FUNCT);
        cExpFlag|=CEFLAG_EXP;
        COMPILE_COMPILE(&cTemp); /* compile unary operator */
        COMPILE_DOWNSTACK(); /* and return to our caller */
        COMPILE_PULL(&cTemp);
        cState=cTemp.cData.cuState;
        COMPILE_PULL(&cTemp);
        cValid=cTemp.cData.cuValid;
        break;



      /* default: this state handles comments, or should never occur */
      default: 
        if ((cState & 0xFF)==CRN_COMMENT) { /* catch-all comment routine */
          COMPILE_COMPILECOMMENT(cLabel);
          cState=cState&0xFFFFFF00;
        }
        else {
          sprintf(cError,"internal error (compile.c): Unknown state 0x%08lX\nRecord this message and your program, and notify your Sys Admin.\n",cState);
          CompileErrorReport(cError);
          return(COMPILE_INTERNAL_ERROR);
        }
        break;
    } /* State machine switch */
  } /* State machine while(1) loop */
} /* Compile */

/* CompileInit
 * Initializes all structures required by Compile.c 
 */
void CompileInit() {
  ULWORD i;
  ULWORD cOffset;

  /* allocate our stack */
  cStackSizeByte=CSTACK_SIZE; /* default stack size */
  MALLOC((void *)cStack,BYTE,cStackSizeByte);

  /* allocate our stack frame */
  cSFPSizeByte=CSTACK_FRAME; /* default stack frame size */
  MALLOC((void *)cSFP,BYTE,cSFPSizeByte);

  /* allocate our compile buffer (where the binary code is stored) */
  cCompBufSizeByte=CCODE_SIZE; /* default buffer size */
  MALLOC((void *)cCompBuf,BYTE,cCompBufSizeByte);

  /* generate our global variable tables */
  cOffset=0;
  for (i=0;i<=cSystemVariable;i++) {
    if (cSysVar[i].cText!=NULL) {
      strcpy(cVarTable[cOffset].cText,cSysVar[i].cText);
      cVarTable[cOffset].cType=cSysVar[i].cType;
      cVarTable[cOffset].cDomain=CDOMAIN_LOCAL;
      cVarTable[cOffset].cOffset=cOffset;
      cOffset++;
    }
  }
}
