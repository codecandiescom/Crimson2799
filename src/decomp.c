/* decomp.c
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
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "code.h"
#include "codestuf.h"
#include "interp.h"
#include "function.h"
#include "compile.h"
#include "decomp.h"

/** Define Statments **/

/* Decompile Options */
#define DMAX_PROG_LENGTH   (1<<13) /* default size of decompile buffer */
#define DMAX_PROG_RECURSE  (1<<7)  /* number of recursions we can handle */

/* formatting options */
#define DINDENT_STR "  "      /* indent string */



/* Structures and unions */

/* Global declarations */
THING *dThing;  /* the thing we send messages to */
BYTE *dComp;    /* pointer to compiled code - to make it global */
WORD dCompOff;  /* where we currently are in dComp buffer */
WORD dCompEnd;  /* offset pointing to next block (ie end of code) */
WORD dCompStart;/* offset pointing to next block (ie end of code) */
WORD dCurSrcMark; /* current source mark number */
WORD *dSrcMark; 
LWORD dSrcMarkSizeByte;
BYTE *dMarkType; /* operand level of mark */
LWORD dMarkTypeSizeByte;
WORD *dStack;    /* position stack */
LWORD dStackSizeByte;
/*WORD *dStackElse;*//* else stack - watch for else's */
/*LWORD dStackElseSizeByte;*/
BYTE *dSrc;     /* pointer to source (decompiled) code */
LWORD dSrcSizeByte; /* size of our decompiled code buffer */
WORD dNumParam; /* our last COP_PUSHV? value, which, coincidentally, 
                   happens to be our # params when immediately preceeding
                   a COP_EXEC or COP_EXECR call. */

WORD dStackPos;                    /* position in stack */

/*WORD dStackElsePos;*/                /* else stack position */

/* This routine is called by the compiler if an error occurs. This routine */
/* should pass the error message etc. on to the appropriate place.         */
void DecompErrorReport(BYTE *dError) {
  SendThing("^w",dThing); /* change color */
  SendThing(dError,dThing);
}

/** Stack Functions **/

WORD DecompInitStack() {
  dStackPos=0;
/*  dStackElsePos=0;*/
  return(0);
}

WORD DecompPush(WORD offset) {
  REALLOC("WARNING! decomp.c stack overflow... resizing\n",
    dStack,WORD,(dStackPos+1),dStackSizeByte);
/*  REALLOC("WARNING! decomp.c else-stack overflow... resizing\n",
    dStackElse,WORD,(dStackPos+1),dStackElseSizeByte);*/
/*  if (dStackPos==DMAX_PROG_RECURSE) {
    DecompErrorReport("decompile error: Stack overflow\n");
    return(DECOMP_INTERNAL_ERROR);
  }*/
  dStack[dStackPos]=offset;
  dStackPos++;
  return(0);
}
WORD DecompPull() {
  if (!dStackPos) {
    DecompErrorReport("decompile error: Stack underflow\n");
    return(DECOMP_INTERNAL_ERROR);
  }
  dStackPos--;
  return(0);
}
WORD DecompTop() {
  if (dStackPos)
    return(dStack[dStackPos-1]);
  else
    return(0);
}

/** Compiled code Functions **/
WORD DecompGetJump() {
  WORD temp;
  temp=(dComp[dCompOff]&0xFF)+((dComp[dCompOff+1]&0xFF)<<8); 
  dCompOff+=2;
  /* the &'s are to guard against sign extension */
  return(temp);
}

/** Decompile code Functions **/
WORD DecompAddByte(BYTE byte) {
  WORD offset;

  offset=dSrcMark[dCurSrcMark];   /* first, find end of chunk */
  while((dSrc[offset])&&(offset<dSrcSizeByte))
    offset++;
  REALLOC("WARNING! decomp.c source buffer overflow... resizing\n",
    dSrc,BYTE,(offset+2),dSrcSizeByte);
/*  if ((offset+1) >=DMAX_PROG_LENGTH) {
    DecompErrorReport("decompile error: source code buffer overflow\n");
    return(DECOMP_INTERNAL_ERROR);
  } */
  dSrc[offset++]=byte;
  dSrc[offset]=0;
  return(0);
}

WORD DecompAddStr(BYTE *str) {
  WORD offset,rc;

  offset=0;
  while(str[offset]) {
    if ((rc=DecompAddByte(str[offset])))
      return(rc);
    offset++;
  }
  return(0);
}

WORD DecompAddChunk(BYTE *str) {
  WORD offset,rc;

  offset=dSrcMark[dCurSrcMark];   /* first, find end of chunk */
  while((dSrc[offset])&&(offset<dSrcSizeByte))
    offset++;
  offset++;
  REALLOC("WARNING! decomp.c source buffer overflow... resizing\n",
    dSrc,BYTE,(offset+strlen(str)+1),dSrcSizeByte);
/*  if ((offset+strlen(str)) >=DMAX_PROG_LENGTH) {
    DecompErrorReport("decompile error: source code buffer overflow\n");
    return(DECOMP_INTERNAL_ERROR);
  }*/
  dCurSrcMark++;
  REALLOC("WARNING! decomp.c Source marker overflow... resizing\n",
    dSrcMark,WORD,(dCurSrcMark+1),dSrcMarkSizeByte);
  REALLOC("WARNING! decomp.c Source marker-type overflow... resizing\n",
    dMarkType,BYTE,(dCurSrcMark+1),dMarkTypeSizeByte);
/*  if (dCurSrcMark >= DMAX_PROG_RECURSE) {
    DecompErrorReport("decompile error: source code recursion overflow\n");
    return(DECOMP_INTERNAL_ERROR);
  }*/
  dSrcMark[dCurSrcMark]=offset;
  dSrc[offset]=0;

  if ((rc=DecompAddStr(str)))
    return(rc);
  return(0);
}

WORD DecompInsertStr(BYTE *str) {
  WORD offset,counter;
  WORD str_len;

  str_len=strlen(str);
  if (!dCurSrcMark) {
    DecompErrorReport("internal error (decomp.c): source marker underflow.\nThis shouldn't happen; it's an error in the program. Contact your Sys Admin.\n");
    return(DECOMP_INTERNAL_ERROR);
  }
  if (!str_len) { /* null string - just clober 0x00 inbetween */
    offset=dSrcMark[dCurSrcMark]-1;
    do {
      dSrc[offset]=dSrc[offset+1];
    }
    while (dSrc[offset++]);
    dCurSrcMark--;
    return(0);
  }
  offset=dSrcMark[dCurSrcMark];
  while((dSrc[offset])&&(offset<dSrcSizeByte))
    offset++;
  offset+=(str_len-1); /* the -1 is because we 'gain' a char from the 0x00 */
  REALLOC("WARNING! decomp.c source buffer overflow... resizing\n",
    dSrc,BYTE,(offset+1),dSrcSizeByte);
/*  if (offset >=DMAX_PROG_LENGTH) {
    DecompErrorReport("decompile error: source code buffer overflow\n");
    return(DECOMP_INTERNAL_ERROR);
  }*/

  while((offset-(str_len-1))>=dSrcMark[dCurSrcMark]) {
    dSrc[offset]=dSrc[offset-(str_len-1)];
    offset--;
  }
  offset=offset-(str_len-1);
  for (counter=0;counter<str_len;counter++) { /* copy string */
    dSrc[offset+counter]=str[counter];
  }
  dCurSrcMark--;
  return(0);
}


WORD DecompSpecialInsertStr(BYTE *str) { /* doesn't take out dSrcMark */
  WORD offset,counter;
  WORD str_len;

  str_len=strlen(str);
  if (!str_len) { /* null string - just exit */
    return(0);
  }
  offset=dSrcMark[dCurSrcMark];
  while((dSrc[offset])&&(offset<dSrcSizeByte))
    offset++;
  offset+=str_len;
  REALLOC("WARNING! decomp.c source buffer overflow... resizing\n",
    dSrc,BYTE,(offset+1),dSrcSizeByte);
/*  if (offset >=DMAX_PROG_LENGTH) {
    DecompErrorReport("decompile error: source code buffer overflow\n");
    return(DECOMP_INTERNAL_ERROR);
  }*/

  offset++;
  while((offset-str_len)>dSrcMark[dCurSrcMark]) {
    offset--;
    dSrc[offset]=dSrc[offset-str_len];
  }

  offset-=str_len;
  for (counter=0;counter<str_len;counter++) { /* copy string */
    dSrc[offset+counter]=str[counter];
  }
  return(0);
}

WORD DecompInsertOp(BYTE *operator,BYTE type) {
  WORD offset,rc;

  offset=0;
  /* NOTE ON BRACKETS */
  /* We only bracket if needed. IE: If the prescidence of the operators
  ** calls for brackets. We also ALWAYS bracket for '=','==','|='. Why?
  ** These operators can also operate on pointer variables and values.
  ** HOWEVER, the result can then be used in INTEGER expressions!!!!!!
  ** To keep the type correct upon decompilation and re-compilation, we
  ** need to put brackets around them. */
  if ((type>dMarkType[dCurSrcMark])||
    (dMarkType[dCurSrcMark]==COP_EQU)||
    (dMarkType[dCurSrcMark]==COP_BEQU)||
    (dMarkType[dCurSrcMark]==COP_BNEQU)) { /* bracket second operand */
    if ((rc=DecompAddByte(')')))
      return(rc);
    if ((rc=DecompSpecialInsertStr("(")))
      return(rc);
  }
  if ((type>dMarkType[dCurSrcMark-1])||
    (dMarkType[dCurSrcMark-1]==COP_EQU)||
    (dMarkType[dCurSrcMark-1]==COP_BEQU)||
    (dMarkType[dCurSrcMark-1]==COP_BNEQU)) { /* bracket first operand */
    offset=dSrcMark[dCurSrcMark];
    while(dSrc[offset++]);
    REALLOC("WARNING! decomp.c source buffer overflow... resizing\n",
      dSrc,BYTE,(offset+5),dSrcSizeByte); /* why +7? just to make sure: " <<= " + 0x00 + extra to be super-paranoid */
/*    if ((offset+2) >=DMAX_PROG_LENGTH) {
      DecompErrorReport("decompile error: source code buffer overflow\n");
      return(DECOMP_INTERNAL_ERROR);
    }*/
    while(offset >= dSrcMark[dCurSrcMark]) {
      dSrc[offset+2]=dSrc[offset];
      offset--;
    }
    dSrc[offset+2]=0;
    dSrc[offset+1]=')';
    offset--;
    while(offset >= dSrcMark[dCurSrcMark-1]) {
      dSrc[offset+1]=dSrc[offset];
      offset--;
    }
    dSrc[offset+1]='('; 
    dSrcMark[dCurSrcMark]+=2; /* we just moved it, remember? */
  }
  if ((rc=DecompInsertStr(operator)))
    return(rc);
  dMarkType[dCurSrcMark]=type;
  return(0);
}


/* Decomp
 * The guts of this whole thing.
 *
 * Decomp is set up similar to Compile:
 * ddComp    = a pointer to the compiled binary code.
 * ddSrc     = a pointer to a pointer to a STR containing the decompiled
 *             (source) code. This STR will be allocated by Decomp.
 * ddThing   = a pointer to a THING to send error messages to. 
 *
 * Returns a return code as defined in decomp.h
 */
WORD Decomp(STR *ddComp,STR **ddSrc,THING *ddThing) { 
  WORD dRc;    /* return code */
  BYTE dTempBuf[100]; /* temporary string buffer */
  BYTE dError[300]; /* error string buffer */
  WORD dFunction;     /* pointer to a function */
  WORD dOffset;     /* pointer to an offset */
  WORD dOffset2;    /* pointer to an offset */
  WORD dOffset3;    /* pointer to an offset */
  WORD dCounter;       /* counter for counting things */
  LWORD dNumber;      /* number extracted from dComp */
  BYTE dInstruction;  /* instruction we're looking at */
  BYTE dLocalVarTable[CMAX_VAR][CVAR_NAME_LENGTH]; /* local variable table */
  WORD dVariable;     /* variable we're reading */
  WORD dIndent; /* current # spaces to indent */
  void *dPtr;   /* void pointer for pointer arguments */

  dThing=ddThing; /* make calling THING globally available */
  dComp=ddComp->sText;
  if ((dRc=DecompInitStack()))
    return(dRc);

  dSrc[0]=0; /* just in case */

  /* The first thing we have to do is check to see if what we've been passed */
  /* is actually compiled code.                                              */
  /* NOTE: the second check (on sLen); it'd be possible to mistake a STR for */
  /*       a program by someone just making a STR->sText=CODE_PROG_HEADER.   */
  /*       However, compiled code always includes, at a minimum, a           */
  /*       terminating segment, thus increasing sLen by a MINIMUM of 1.      */
  /*       Thus, someone could make sText look like code, but sLen will      */
  /*       always tell us a forgery from the real thing.                     */
  if ( (strcmp(dComp,CODE_PROG_HEADER))||
       ( (ddComp->sLen) <= (CODE_PROG_HEADER_LENGTH) )) {
    DecompErrorReport("decompile error: this is not a compiled program!\n");
    return(DECOMP_INPUT_ERROR);
  }

  /* Initialize dLocalVarTable */
  for (dCounter=0;dCounter<cSystemVariable;dCounter++) {
    if (dCounter<cSystemVariableStatic)
      strcpy(dLocalVarTable[dCounter],cSysVar[dCounter].cText);
    else
      strcpy(dLocalVarTable[dCounter],cSysVar[dCounter+1].cText);
  }

  sprintf(dSrc,"{");  /* initial { */
  dCurSrcMark=0;
  dSrcMark[dCurSrcMark]=0;

  /* first, read the variables */
  dCompOff=CODE_PROG_HEADER_LENGTH;
  while((dComp[dCompOff]!=CODE_BLK_VAR_NAME)&&
        (dComp[dCompOff]!=CODE_BLK_NULL)) {
    dCompOff++;
    dCompOff=DecompGetJump();
  }
  if (dComp[dCompOff]==CODE_BLK_VAR_NAME) { /* We have VARS! */
    dCompOff++;
    dCompEnd=DecompGetJump();
    dCompStart=dCompOff;
    dCounter=cSystemVariable+1;
    while(dCompOff<dCompEnd) {
      strcpy(dLocalVarTable[dCounter],&dComp[dCompOff]);
      dCompOff+=(strlen(&dComp[dCompOff])+1);
      dCounter++;
    }
  }

/* next, set up variable declaration region */
  dCompOff=CODE_PROG_HEADER_LENGTH;
  while((dComp[dCompOff]!=CODE_BLK_VAR)&&
        (dComp[dCompOff]!=CODE_BLK_NULL)) {
    dCompOff++;
    dCompOff=DecompGetJump();
  }
  if (dComp[dCompOff]==CODE_BLK_VAR) { /* We have VARS! */
    dCompOff++;
    dCompEnd=DecompGetJump();
    dCompStart=dCompOff;
    dCounter=cSystemVariable+1;
    while(dCompOff<dCompEnd) {
      if ((dCompOff==dCompStart)||(dComp[dCompOff]!=dComp[dCompOff-1])) {
        if (dComp[dCompOff]==CDT_INT) {
          if ((dRc=DecompAddChunk("int ")))  /* These used to say "local int" etc, but local/global/private are not implemented. */
            return(dRc);
        } else if (dComp[dCompOff]==CDT_STR) {
          if ((dRc=DecompAddChunk("str ")))
            return(dRc);
        } else if (dComp[dCompOff]==CDT_THING) {
          if ((dRc=DecompAddChunk("thing ")))
            return(dRc);
        } else if (dComp[dCompOff]==CDT_EXTRA) {
          if ((dRc=DecompAddChunk("extra ")))
            return(dRc);
        } else if (dComp[dCompOff]==CDT_EXIT) {
          if ((dRc=DecompAddChunk("exit ")))
            return(dRc);
        } else {
          if ((dRc=DecompAddChunk("<unrecognized type> ")))
            return(dRc);
        }
      } else {
        if ((dRc=DecompAddStr(", ")))
          return(dRc);
      }
      if ((dRc=DecompAddStr(dLocalVarTable[dCounter])))
        return(dRc);
      dCompOff++;
      dCounter++;
    } /* while */
  }


  /* next, read the program code */
  dCompOff=CODE_PROG_HEADER_LENGTH;
  while((dComp[dCompOff]!=CODE_BLK_CODE)&&
        (dComp[dCompOff]!=CODE_BLK_NULL)) {
    dCompOff++;
    dCompOff=DecompGetJump();
  }
  if (dComp[dCompOff]!=CODE_BLK_NULL) { /* We have CODE! */
    dCompOff++;
    dCompEnd=DecompGetJump();
    dCompStart=dCompOff;
    while(dCompOff<dCompEnd) { /* Great big decompile `while' - the main loop */
      dInstruction=(dComp[dCompOff]&0xff);
      if (BIT(codeSet,CODE_DEBUGDECOMPILE)) {
        sprintf(dError,"\n^G%04X^A: ^a%02X %04X^G",dCompOff,dInstruction,dCurSrcMark);
        SendThing(dError,dThing);
      }
      while(dCompOff==DecompTop()) {
        if ((dRc=DecompAddChunk("}")))
          return(dRc);
        if ((dRc=DecompPull()))
          return(dRc);
      }
      dCompOff++;
      switch(dInstruction) {
        case COP_COMMENT:
          if ((dRc=DecompAddStr("/*")))
            return(dRc);
          if ((dRc=DecompAddStr(&dComp[dCompOff])))
            return(dRc);
          if ((dRc=DecompAddStr("*/")))
            return(dRc);
          dCompOff+=strlen(&dComp[dCompOff])+1;
          break;
        case COP_EQUSR:
          if ((dRc=DecompInsertOp(">>=",COP_EQUSR)))
            return(dRc);
          break;
        case COP_EQUSL:
          if ((dRc=DecompInsertOp("<<=",COP_EQUSL)))
            return(dRc);
          break;
        case COP_EQUOR:
          if ((dRc=DecompInsertOp("|=",COP_EQUOR)))
            return(dRc);
          break;
        case COP_EQUXOR:
          if ((dRc=DecompInsertOp("\\=",COP_EQUXOR)))
            return(dRc);
          break;
        case COP_EQUAND:
          if ((dRc=DecompInsertOp("&=",COP_EQUAND)))
            return(dRc);
          break;
        case COP_EQUSUB:
          if ((dRc=DecompInsertOp("-=",COP_EQUSUB)))
            return(dRc);
          break;
        case COP_EQUADD:
          if ((dRc=DecompInsertOp("+=",COP_EQUADD)))
            return(dRc);
          break;
        case COP_EQUMOD:
          if ((dRc=DecompInsertOp("%=",COP_EQUMOD)))
            return(dRc);
          break;
        case COP_EQUDIV:
          if ((dRc=DecompInsertOp("/=",COP_EQUDIV)))
            return(dRc);
          break;
        case COP_EQUMULT:
          if ((dRc=DecompInsertOp("*=",COP_EQUMULT)))
            return(dRc);
          break;
        case COP_EQU:
          if ((dRc=DecompInsertOp("=",COP_EQU)))
            return(dRc);
          break;
        case COP_BOR:
          if ((dRc=DecompInsertOp("||",COP_BOR)))
            return(dRc);
          break;
        case COP_BAND:
          if ((dRc=DecompInsertOp("&&",COP_BAND)))
            return(dRc);
          break;
        case COP_OR:
          if ((dRc=DecompInsertOp("|",COP_OR)))
            return(dRc);
          break;
        case COP_XOR:
          if ((dRc=DecompInsertOp("\\",COP_XOR)))
            return(dRc);
          break;
        case COP_AND:
          if ((dRc=DecompInsertOp("&",COP_AND)))
            return(dRc);
          break;
        case COP_BEQU:
          if ((dRc=DecompInsertOp("==",COP_BEQU)))
            return(dRc);
          break;
        case COP_BNEQU:
          if ((dRc=DecompInsertOp("!=",COP_BNEQU)))
            return(dRc);
          break;
        case COP_GEQ:
          if ((dRc=DecompInsertOp(">=",COP_GEQ)))
            return(dRc);
          break;
        case COP_GR:
          if ((dRc=DecompInsertOp(">",COP_GR)))
            return(dRc);
          break;
        case COP_LEQ:
          if ((dRc=DecompInsertOp("<=",COP_LEQ)))
            return(dRc);
          break;
        case COP_LS:
          if ((dRc=DecompInsertOp("<",COP_LS)))
            return(dRc);
          break;
        case COP_SR:
          if ((dRc=DecompInsertOp(">>",COP_SR)))
            return(dRc);
          break;
        case COP_SL:
          if ((dRc=DecompInsertOp("<<",COP_SL)))
            return(dRc);
          break;
        case COP_SUB:
          if ((dRc=DecompInsertOp("-",COP_SUB))) 
            return(dRc);
          break;
        case COP_ADD:
          if ((dRc=DecompInsertOp("+",COP_ADD)))
            return(dRc);
          break;
        case COP_MOD:
          if ((dRc=DecompInsertOp("%",COP_MOD)))
            return(dRc);
          break;
        case COP_DIV:
          if ((dRc=DecompInsertOp("/",COP_DIV)))
            return(dRc);
          break;
        case COP_MULT:
          if ((dRc=DecompInsertOp("*",COP_MULT)))
            return(dRc);
          break;
        case COP_POP:
          break;
        case COP_EXEC:
        case COP_EXECR:
          dCurSrcMark--; /* whip out from src our # params value */
          dFunction=DecompGetJump();
          dCounter=0;
          if (dNumParam) {
            dNumParam--;
            while(dNumParam) {
              if ((dRc=DecompInsertStr(",")))
                return(dRc);
              dCounter++;
              dNumParam--;
            }
            if ((dRc=DecompAddByte(')')))
              return(dRc);
          } else {
            if ((dRc=DecompAddChunk(")")))
              return(dRc);
          }
          sprintf(dTempBuf,"%s(",fTable[dFunction].fText);
          if ((dRc=DecompSpecialInsertStr(dTempBuf)))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_TOP_OP;
          break;
        case COP_NOP:
          break;
        case COP_NULL:
          DecompErrorReport("decompile error: invalid NULL op-code encountered.\nThis shouldn't happen. Notify your Sys Admin of this error.\n" );
          return(DECOMP_INPUT_ERROR);
          break;
        case COP_PUSHV1:
          dNumber=dComp[dCompOff++]; /* sign extends */
          dNumParam=dNumber;
          sprintf(dTempBuf,"%li",dNumber);
          if ((dRc=DecompAddChunk(dTempBuf)))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_TOP_OP;
          break;
        case COP_PUSHV2:
          dNumber=(dComp[dCompOff]&0x00FF)+((dComp[dCompOff+1])<<8);
          dNumParam=dNumber;
          dCompOff+=2;
          sprintf(dTempBuf,"%li",dNumber);
          if ((dRc=DecompAddChunk(dTempBuf)))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_TOP_OP;
          break;
        case COP_PUSHV:
        case COP_PUSHV4:
          dNumber=(dComp[dCompOff]&0x000000ff)+
                  ((dComp[dCompOff+1]&0x000000ff)<<8)+
                  ((dComp[dCompOff+2]&0x000000ff)<<16)+
                  ((dComp[dCompOff+3])<<24); /* last one sign-extends */
          dNumParam=dNumber;
          dCompOff+=4;
          sprintf(dTempBuf,"%li",dNumber);
          if ((dRc=DecompAddChunk(dTempBuf)))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_TOP_OP;
          break;
        /* for the -- and ++ operators, the spaces are important. You see, if */
        /* someone enters a=b++ + ++c; which is completely valid, and we then */
        /* go and decompile it to a=b+++++c; the next time it's compiled, it  */
        /* will be interpreted as a=(b++)++ + c; which is NOT valid and NOT   */
        /* what they originally had! Don't worry, I think this problem is only*/
        /* with ++ and -- operators. If we implemented * and &, there may be  */
        /* a problem with them as well, but they aren't implemented, so yahoo!*/
        /* Oh, now that I think about it, we may have a problem with the      */
        /* unary pre-op `-'. I'll go fix that too.                            */
        case COP_BSUB:
          if ((dRc=DecompSpecialInsertStr(" --")))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_BSUB;
          break;
        case COP_BADD:
          if ((dRc=DecompSpecialInsertStr(" ++")))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_BADD;
          break;
        case COP_ASUB:
          if ((dRc=DecompAddStr("-- ")))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_ASUB;
          break;
        case COP_AADD:
          if ((dRc=DecompAddStr("++ ")))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_AADD;
          break;
        case COP_COMP:
          if (COP_COMP>dMarkType[dCurSrcMark]) { /* bracket second operand */
            if ((dRc=DecompAddByte(')')))
              return(dRc);
            if ((dRc=DecompSpecialInsertStr("(")))
              return(dRc);
          }
          if ((dRc=DecompSpecialInsertStr("`")))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_COMP;
          break;
        case COP_NOT:
          if (COP_NOT>dMarkType[dCurSrcMark]) { /* bracket second operand */
            if ((dRc=DecompAddByte(')')))
              return(dRc);
            if ((dRc=DecompSpecialInsertStr("(")))
              return(dRc);
          }
          if ((dRc=DecompSpecialInsertStr("!")))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_NOT;
          break;
        case COP_NEG:
          if (COP_NEG>dMarkType[dCurSrcMark]) { /* bracket second operand */
            if ((dRc=DecompAddByte(')')))
              return(dRc);
            if ((dRc=DecompSpecialInsertStr("(")))
              return(dRc);
          }
          if ((dRc=DecompSpecialInsertStr(" -")))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_NEG;
          break;
        case COP_TERM:
          if ((dRc=DecompAddChunk("stop")))
            return(dRc);
          break;
        case COP_PUSHL:
          dVariable=(dComp[dCompOff]&0x00FF)+((dComp[dCompOff+1]&0xFF)<<8);
          dCompOff+=2;
          if ((dRc=DecompAddChunk(dLocalVarTable[dVariable])))
            return(dRc);
          dMarkType[dCurSrcMark]=COP_TOP_OP;
          break;
        case COP_GOTO:
          dOffset=DecompGetJump();
          if ((dOffset>dCompOff)&&(dCompOff==DecompTop())) {/* else condition */
            if ((dRc=DecompAddChunk("}")))
              return(dRc);
            if ((dRc=DecompAddChunk("else")))
              return(dRc);
            if ((dRc=DecompAddChunk("{")))
              return(dRc);
            if ((dRc=DecompPull()))
              return(dRc);
            if ((dRc=DecompPush(dOffset)))
              return(dRc);
          }
          break;
        case COP_IFZ:
          dOffset=DecompGetJump();
          if ((dRc=DecompPush(dOffset)))
            return(dRc);
          if ((dRc=DecompAddByte(')')))
            return(dRc);
          if ((dRc=DecompSpecialInsertStr("(")))
            return(dRc);
          if ((dRc=DecompSpecialInsertStr("if")))
            return(dRc);
          if ((dRc=DecompAddChunk("{")))
            return(dRc);
          break;
        case COP_WHILEZ:
          dOffset=DecompGetJump();
          if ((dRc=DecompPush(dOffset)))
            return(dRc);
          if ((dRc=DecompAddByte(')')))
            return(dRc);
          if ((dRc=DecompSpecialInsertStr("(")))
            return(dRc);
          if ((dRc=DecompSpecialInsertStr("while")))
            return(dRc);
          if ((dRc=DecompAddChunk("{")))
            return(dRc);
          break;
        case COP_PUSHPT:
          /* Solaris doesn't like this */ 
          /* dPtr=*((void**)(&dComp[dCompOff])); */
          {
            unsigned long i;
            dPtr=NULL;
            for (i=0;i<CODE_PT_SIZE;i++)
              (unsigned long)(dPtr)|=
                ((unsigned long)(dComp[dCompOff+i]))<<(8*i);
          }
          dCompOff+=CODE_PT_SIZE;
          if ((dRc=DecompAddChunk("\"")))
            return(dRc);
          for (dCounter=0;((STR*)dPtr)->sText[dCounter];dCounter++) {
            if ((((STR*)dPtr)->sText[dCounter]=='\"') ||
                (((STR*)dPtr)->sText[dCounter]=='\'') ||
                (((STR*)dPtr)->sText[dCounter]=='\\') ||
                (((STR*)dPtr)->sText[dCounter]=='\?')) {
              if ((dRc=DecompAddByte('\\')))
                return(dRc);
              if ((dRc=DecompAddByte(((STR*)dPtr)->sText[dCounter])))
                return(dRc);
            } else if (((STR*)dPtr)->sText[dCounter]=='\a') {
              if ((dRc=DecompAddStr("\\a")))
                return(dRc);
            } else if (((STR*)dPtr)->sText[dCounter]=='\b') {
              if ((dRc=DecompAddStr("\\b")))
                return(dRc);
            } else if (((STR*)dPtr)->sText[dCounter]=='\f') {
              if ((dRc=DecompAddStr("\\f")))
                return(dRc);
            } else if (((STR*)dPtr)->sText[dCounter]=='\n') {
              if ((dRc=DecompAddStr("\\n")))
                return(dRc);
            } else if (((STR*)dPtr)->sText[dCounter]=='\r') {
              if ((dRc=DecompAddStr("\\r")))
                return(dRc);
            } else if (((STR*)dPtr)->sText[dCounter]=='\t') {
              if ((dRc=DecompAddStr("\\t")))
                return(dRc);
            } else if (((STR*)dPtr)->sText[dCounter]=='\v') {
              if ((dRc=DecompAddStr("\\v")))
                return(dRc);
            } else {
              if ((dRc=DecompAddByte(((STR*)dPtr)->sText[dCounter])))
                return(dRc);
            }
          } /* for loop */
          if ((dRc=DecompAddByte('\"')))
            return(dRc);
          /* free this string */
          /*STRFREE(dPtr);*/
          dMarkType[dCurSrcMark]=COP_TOP_OP;
          break;
        case COP_PUSHG:
        case COP_PUSHP:
        default:
          sprintf(dError,"decompile error: unrecognized op-code: %02X\nThis shouldn't happen. Notify your Sys Admin of this error.\n" 
            ,dComp[dCompOff]);
          DecompErrorReport(dError);
          return(DECOMP_INPUT_ERROR);
          break;
      } /* co-code switch */
      if (BIT(codeSet,CODE_DEBUGDECOMPILE)) {
        if (dInstruction==COP_POP)
          sprintf(dError," ^a%04X^b : ^y;",dCurSrcMark);
        else
          sprintf(dError," ^a%04X^b : ^y%s",dCurSrcMark,&dSrc[dSrcMark[dCurSrcMark]]);
        SendThing(dError,dThing);
      }
    } /* great decompile while */
  } /* if (we found code) */

  /* Lastly, throw on the brackets, et nous sommes finir (c- in French 12) */

  while(dCompOff==DecompTop()) {  /* wrap up any outstanding {'s */
    if ((dRc=DecompAddChunk("}")))
      return(dRc);
    if ((dRc=DecompPull()))
      return(dRc);
  }

  if (DecompTop()) {
    DecompErrorReport("decompile error: Stack non-zero after decompile.\n");
    return(DECOMP_INTERNAL_ERROR);
  }

  if ((dRc=DecompAddChunk("}"))) /* Add final } */
    return(dRc);

  /* now go through the whole thing putting ;s where appropriate */
  dOffset=dSrcMark[dCurSrcMark];   /* first, find end of the last chunk */
  while((dSrc[dOffset])&&(dOffset<dSrcSizeByte))
    dOffset++;
  dCounter=0; /* last `thing' designator */
  for(dOffset2=0;dOffset2<dOffset;dOffset2++) {
    if (dSrc[dOffset2]=='{') {
      while(dSrc[dOffset2])
        dOffset2++;
      dSrc[dOffset2]='\n';
    } else if (dSrc[dOffset2]=='}') {
      while(dSrc[dOffset2])
        dOffset2++;
      dSrc[dOffset2]='\n';
    } else if ((dSrc[dOffset2]=='i')&&(dSrc[dOffset2+1]=='f')&&
               (dSrc[dOffset2+2]=='(')) {
      while(dSrc[dOffset2])
        dOffset2++;
      dSrc[dOffset2]=' ';
    } else if ((dSrc[dOffset2]=='e')&&(dSrc[dOffset2+1]=='l')&&
               (dSrc[dOffset2+2]=='s')&&(dSrc[dOffset2+3]=='e')&&
               (dSrc[dOffset2+4]==0)) {
      while(dSrc[dOffset2])
        dOffset2++;
      dSrc[dOffset2]=' ';
    } else if ((dSrc[dOffset2]=='w')&&(dSrc[dOffset2+1]=='h')&&
               (dSrc[dOffset2+2]=='i')&&(dSrc[dOffset2+3]=='l')&&
               (dSrc[dOffset2+4]=='e')&&(dSrc[dOffset2+5]=='(')) {
      while(dSrc[dOffset2])
        dOffset2++;
      dSrc[dOffset2]=' ';
    } else {
      while(dSrc[dOffset2])
        dOffset2++;
      dSrc[dOffset2]=';';
    }
  }
  dSrc[dOffset]=0;
  /* and now, since we have one great big string, let's indent */
  dIndent=0;
  for (dOffset2=0;dOffset2<dOffset;dOffset2++) {
    if (dSrc[dOffset2]=='\"') { /* quote - don't process */
      do {
        if (dSrc[dOffset2]=='\\') dOffset2++;
        dOffset2++;
      } while(dSrc[dOffset2]!='\"');
    } else if (dSrc[dOffset2]==';') { /* cr + indent */
      dOffset2++;
      if (dSrc[dOffset2]=='}')
        dIndent--;
      dTempBuf[0]='\n';
      dTempBuf[1]=0;
      for(dCounter=0;dCounter<dIndent;dCounter++)
        strcat(dTempBuf,DINDENT_STR);
      dCounter=strlen(dTempBuf);
      REALLOC("WARNING! decomp.c source buffer overflow... resizing\n",
        dSrc,BYTE,(dOffset+dCounter+1),dSrcSizeByte);
      for(dOffset3=dOffset;dOffset3>=dOffset2;dOffset3--) 
        dSrc[dOffset3+dCounter]=dSrc[dOffset3];
      for(dOffset3=0;dOffset3<dCounter;dOffset3++)
        dSrc[dOffset2+dOffset3]=dTempBuf[dOffset3];
      dOffset+=dCounter;
    } else if (dSrc[dOffset2]=='\n') {
      dOffset2++;
      if (dSrc[dOffset2]=='}')
        dIndent--;
      dTempBuf[0]=0;
      for(dCounter=0;dCounter<dIndent;dCounter++)
        strcat(dTempBuf,DINDENT_STR);
      dCounter=strlen(dTempBuf);
      REALLOC("WARNING! decomp.c source buffer overflow... resizing\n",
        dSrc,BYTE,(dOffset+dCounter+1),dSrcSizeByte);
      for(dOffset3=dOffset;dOffset3>=dOffset2;dOffset3--) 
        dSrc[dOffset3+dCounter]=dSrc[dOffset3];
      for(dOffset3=0;dOffset3<dCounter;dOffset3++)
        dSrc[dOffset2+dOffset3]=dTempBuf[dOffset3];
      dOffset+=dCounter;
    } else if (dSrc[dOffset2]=='{') {
      dIndent++;
    }
  }
  *ddSrc=STRCREATE(dSrc);
  return(DECOMP_OK);
}


WORD DecompDisassemble(STR *ddComp,THING *ddThing) { 
  BYTE dBuf[300]; /* error string buffer */
  WORD dFunction;     /* pointer to a function */
  WORD dCounter;       /* counter for counting things */
  LWORD dNumber;      /* number extracted from dComp */
  BYTE dInstruction;  /* instruction we're looking at */
  WORD dVariable;     /* pointer to a variable */
  BYTE dLocalVarTable[CMAX_VAR][CVAR_NAME_LENGTH]; /* local variable table */
  void *dPtr;         /* general pointer - to STR or something */

  dThing=ddThing; /* make calling THING globally available */
  dComp=ddComp->sText;

  /* The first thing we have to do is check to see if what we've been passed */
  /* is actually compiled code.                                              */
  /* NOTE: the second check (on sLen); it'd be possible to mistake a STR for */
  /*       a program by someone just making a STR->sText=CODE_PROG_HEADER.   */
  /*       However, compiled code always includes, at a minimum, a           */
  /*       terminating segment, thus increasing sLen by a MINIMUM of 1.      */
  /*       Thus, someone could make sText look like code, but sLen will      */
  /*       always tell us a forgery from the real thing.                     */
  if ( (strcmp(dComp,CODE_PROG_HEADER))||
       ( (ddComp->sLen) <= (CODE_PROG_HEADER_LENGTH) )) {
    DecompErrorReport("disassemble error: this is not a compiled program!\n");
    return(DECOMP_INPUT_ERROR);
  }

  /* Initialize dLocalVarTable */
  for (dCounter=0;dCounter<cSystemVariable;dCounter++) {
    if (dCounter<cSystemVariableStatic)
      strcpy(dLocalVarTable[dCounter],cSysVar[dCounter].cText);
    else
      strcpy(dLocalVarTable[dCounter],cSysVar[dCounter+1].cText);
  }

  /* first, read the variables */
  dCompOff=CODE_PROG_HEADER_LENGTH;
  while((dComp[dCompOff]!=CODE_BLK_VAR_NAME)&&
        (dComp[dCompOff]!=CODE_BLK_NULL)) {
    dCompOff++;
    dCompOff=DecompGetJump();
  }
  if (dComp[dCompOff]==CODE_BLK_VAR_NAME) { /* We have VARS! */
    dCompOff++;
    dCompEnd=DecompGetJump();
    dCompStart=dCompOff;
    dCounter=cSystemVariable+1;
    while(dCompOff<dCompEnd) {
      strcpy(dLocalVarTable[dCounter],&dComp[dCompOff]);
      dCompOff+=(strlen(&dComp[dCompOff])+1);
      dCounter++;
    }
  }

  /* next, read the program code */
  dCompOff=CODE_PROG_HEADER_LENGTH;
  while((dComp[dCompOff]!=CODE_BLK_CODE)&&
        (dComp[dCompOff]!=CODE_BLK_NULL)) {
    dCompOff++;
    dCompOff=DecompGetJump();
  }
  if (dComp[dCompOff]!=CODE_BLK_NULL) { /* We have CODE! */
    dCompOff++;
    dCompEnd=DecompGetJump();
    dCompStart=dCompOff;
sprintf(dBuf,"Disassemble: start=%04X, end=%04X\n",dCompStart,dCompEnd);
SendThing(dBuf,dThing);
    while(dCompOff<dCompEnd) { /* Great disassemble `while' */
      dInstruction=(dComp[dCompOff]&0xff);
      sprintf(dBuf,"^G%04X^A:  ^a%02X",dCompOff,dInstruction);
      SendThing(dBuf,dThing);
      dCompOff++;
      switch(dInstruction) {
        case COP_COMMENT:
          sprintf(dBuf," <text>      ^C/*%s*/\n",&dComp[dCompOff]);
          SendThing(dBuf,dThing);
          dCompOff+=strlen(&dComp[dCompOff])+1;
          break;
        case COP_EQUSR:
          SendThing("             ^C>>=\n",dThing);
          break;
        case COP_EQUSL:
          SendThing("             ^C<<=\n",dThing);
          break;
        case COP_EQUOR:
          SendThing("             ^C|=\n",dThing);
          break;
        case COP_EQUXOR:
          SendThing("             ^C\\=\n",dThing);
          break;
        case COP_EQUAND:
          SendThing("             ^C&=\n",dThing);
          break;
        case COP_EQUSUB:
          SendThing("             ^C-=\n",dThing);
          break;
        case COP_EQUADD:
          SendThing("             ^C+=\n",dThing);
          break;
        case COP_EQUMOD:
          SendThing("             ^C%=\n",dThing);
          break;
        case COP_EQUDIV:
          SendThing("             ^C/=\n",dThing);
          break;
        case COP_EQUMULT:
          SendThing("             ^C*=\n",dThing);
          break;
        case COP_EQU:
          SendThing("             ^C=\n",dThing);
          break;
        case COP_BOR:
          SendThing("             ^C||\n",dThing);
          break;
        case COP_BAND:
          SendThing("             ^C&&\n",dThing);
          break;
        case COP_OR:
          SendThing("             ^C|\n",dThing);
          break;
        case COP_XOR:
          SendThing("             ^C\\\n",dThing);
          break;
        case COP_AND:
          SendThing("             ^C&\n",dThing);
          break;
        case COP_BEQU:
          SendThing("             ^C==\n",dThing);
          break;
        case COP_BNEQU:
          SendThing("             ^C!=\n",dThing);
          break;
        case COP_GEQ:
          SendThing("             ^C>=\n",dThing);
          break;
        case COP_GR:
          SendThing("             ^C>\n",dThing);
          break;
        case COP_LEQ:
          SendThing("             ^C<=\n",dThing);
          break;
        case COP_LS:
          SendThing("             ^C<\n",dThing);
          break;
        case COP_SR:
          SendThing("             ^C>>\n",dThing);
          break;
        case COP_SL:
          SendThing("             ^C<<\n",dThing);
          break;
        case COP_SUB:
          SendThing("             ^C-\n",dThing);
          break;
        case COP_ADD:
          SendThing("             ^C+\n",dThing);
          break;
        case COP_MOD:
          SendThing("             ^C%\n",dThing);
          break;
        case COP_DIV:
          SendThing("             ^C/\n",dThing);
          break;
        case COP_MULT:
          SendThing("             ^C*\n",dThing);
          break;
        case COP_POP:
          SendThing("             ^Cpop\n",dThing);
          break;
        case COP_NOP:
          SendThing("             ^Cnop\n",dThing);
          break;
        case COP_EXEC:
        case COP_EXECR: 
          dFunction=DecompGetJump();
          if (dInstruction==COP_EXEC) {
            sprintf(dBuf," ^a%02X %02X       ^Cexec   %s(",
              (dFunction&0xFF),((dFunction&0xFF00)>>8),
              fTable[dFunction].fText);
          } else {
            sprintf(dBuf," ^a%02X %02X       ^Cexecr  %s(",
              (dFunction&0xFF),((dFunction&0xFF00)>>8),
              fTable[dFunction].fText);
          }
          SendThing(dBuf,dThing);
          dCounter=0;
          while(fTable[dFunction].fParamType[dCounter]!=CDT_NULL) {
            if (dCounter) 
              SendThing("^C,",dThing);
            switch(fTable[dFunction].fParamType[dCounter]) {
              case CDT_INT:
                SendThing("^Rint",dThing);
                break;
              case CDT_STR:
                SendThing("^Rstr",dThing);
                break;
              case CDT_THING:
                SendThing("^Rthing",dThing);
                break;
              case CDT_EXTRA:
                SendThing("^Rextra",dThing);
                break;
              case CDT_EXIT:
                SendThing("^Rexit",dThing);
                break;
              case CDT_ETC:
                SendThing("^Retc...",dThing);
                break;
              default:
                SendThing("^R???",dThing);
                break;
            }
            dCounter++;
          }
          SendThing("^C)\n",dThing);
          break;
  case COP_NULL:
          SendThing("             ^Cnull ^r<this should ^ynot^r happen>\n",dThing);
          break;
        case COP_PUSHV1:
          dNumber=dComp[dCompOff++]; /* sign extends */
          sprintf(dBuf," ^a%02lX          ^Cpushv1 %li\n",(dNumber&0xFF),dNumber);
          SendThing(dBuf,dThing);
          break;
        case COP_PUSHV2:
          dNumber=(dComp[dCompOff]&0xFF)+((dComp[dCompOff+1]&0xFF)<<8);
          sprintf(dBuf," ^a%02X %02X       ^Cpushv2 %li\n",
                 (dComp[dCompOff]&0xFF),(dComp[dCompOff+1]&0xFF),dNumber);
          SendThing(dBuf,dThing);
          dCompOff+=2;
          break;
        case COP_PUSHV:
        case COP_PUSHV4:
          dNumber=(dComp[dCompOff]&0xff)+
                  ((dComp[dCompOff+1]&0xff)<<8)+
                  ((dComp[dCompOff+2]&0xff)<<16)+
                  ((dComp[dCompOff+3]&0xff)<<24);
          if (dInstruction==COP_PUSHV) {
            sprintf(dBuf," ^a%02X %02X %02X %02X ^Cpushv  %li\n",
              (dComp[dCompOff]&0xFF),(dComp[dCompOff+1]&0xFF),
              (dComp[dCompOff+2]&0xFF),(dComp[dCompOff+3]&0xFF),dNumber);
          } else {
            sprintf(dBuf," ^a%02X %02X %02X %02X ^Cpushv4 %li\n",
              (dComp[dCompOff]&0xFF),(dComp[dCompOff+1]&0xFF),
              (dComp[dCompOff+2]&0xFF),(dComp[dCompOff+3]&0xFF),dNumber);
          }
          SendThing(dBuf,dThing);
          dCompOff+=4;
          break;
        case COP_BSUB:
          SendThing("             ^CB--\n",dThing);
          break;
        case COP_BADD:
          SendThing("             ^CB++\n",dThing);
          break;
        case COP_ASUB:
          SendThing("             ^CA--\n",dThing);
          break;
        case COP_AADD:
          SendThing("             ^CA++\n",dThing);
          break;
        case COP_COMP:
          SendThing("             ^C`\n",dThing);
          break;
        case COP_NEG:
          SendThing("             ^C- (neg)\n",dThing);
          break;
        case COP_NOT:
          SendThing("             ^C!\n",dThing);
          break;
        case COP_TERM:
          SendThing("             ^Cstop\n",dThing);
          break;
        case COP_GOTO:
          dFunction=DecompGetJump();
          sprintf(dBuf," ^a%02X %02X       ^Cgoto   ^G%04X\n",
              (dFunction&0xFF),((dFunction&0xFF00)>>8),(dFunction&0xFFFF));
          SendThing(dBuf,dThing);
          break;
        case COP_IFZ:
          dFunction=DecompGetJump();
          sprintf(dBuf," ^a%02X %02X       ^Cifz    ^G%04X\n",
              (dFunction&0xFF),((dFunction&0xFF00)>>8),(dFunction&0xFFFF));
          SendThing(dBuf,dThing);
          break;
        case COP_WHILEZ:
          dFunction=DecompGetJump();
          sprintf(dBuf," ^a%02X %02X       ^Cwhilez ^G%04X\n",
              (dFunction&0xFF),((dFunction&0xFF00)>>8),(dFunction&0xFFFF));
          SendThing(dBuf,dThing);
          break;
        case COP_PUSHL:
          dVariable=DecompGetJump();
          sprintf(dBuf," ^a%02X %02X       ^Cpushl  ^R%s\n",
              (dVariable&0xFF),((dVariable&0xFF00)>>8),dLocalVarTable[dVariable]);
          SendThing(dBuf,dThing);
          break;
        case COP_PUSHPT:
          /* Solaris doesn't like this */ 
          /* dPtr=*((void**)(&dComp[dCompOff])); */
          {
            unsigned long i;
            dPtr=NULL;
            for (i=0;i<CODE_PT_SIZE;i++)
              (unsigned long)(dPtr)|=
                ((unsigned long)(dComp[dCompOff+i]))<<(8*i);
          }
          dCompOff+=CODE_PT_SIZE;
          /*
          for (dCounter=0;dCounter<CODE_PT_SIZE;dCounter++) {
            dPtr|=(dComp[dCompOff]&0xFF)<<(8*dCounter);
            dCompOff++;
          }
          */
          sprintf(dBuf,"^a%8p     ^Cpushpt ^R\"%s\"\n",dPtr,((STR *)dPtr)->sText);
          SendThing(dBuf,dThing);
          break;
        case COP_PUSHG:
        case COP_PUSHP:
        default:
          SendThing("             ^C<unknown> ^r<this should ^ynot^r happen>\n",dThing);
          break;
      } /* co-code switch */
    } /* great disassemble while */
  } /* if (we found code) */

  return(DECOMP_OK);
}

/* DecompInit()
 * Initializes structures required by Decomp.c
 */
void DecompInit() {
  /* allocate our decompile buffer */
  dSrcSizeByte=DMAX_PROG_LENGTH*sizeof(BYTE);
  MALLOC((void *)dSrc,BYTE,dSrcSizeByte);

  /* allocate our markers */
  dSrcMarkSizeByte=DMAX_PROG_RECURSE*sizeof(WORD);
  MALLOC((void *)dSrcMark,WORD,dSrcMarkSizeByte);
  dMarkTypeSizeByte=DMAX_PROG_RECURSE*sizeof(BYTE);
  MALLOC((void *)dMarkType,BYTE,dMarkTypeSizeByte);

  /* allocate our stack */
  dStackSizeByte=DMAX_PROG_RECURSE*sizeof(WORD);
  MALLOC((void *)dStack,WORD,dStackSizeByte);

  /* And our "else" statement stack */
/*  dStackElseSizeByte=DMAX_PROG_RECURSE*sizeof(WORD);
  MALLOC((void *)dStackElse,WORD,dStackElseSizeByte);*/
}

/* Take in a STR and free any embedded STR pointers */
void DecompStrFree(STR *ddComp) {
  BYTE dInstruction;  /* instruction we're looking at */
  void *dPtr;         /* general pointer - to STR or something */

  /* Note: This proc has to be re-entrant to a certain degree. Unfortunately, */
  /* this proc is not designed to be re-entrant. Therefore, we compromize     */
  /* (kludge). Since we should be freeing at most only 1 compiled code str    */
  /* (ie: there shouldn't be any embedded pointers to compiled code str's)    */
  /* we just check to see if we're code, and abort if not - and make sure the */
  /* code up and including the check are re-entrant. All the rest can suffer. */

  /* The first thing we have to do is check to see if what we've been passed */
  /* is actually compiled code.                                              */
  /* This would be the proper way to check if it's compiled:  */
/*  if ( (strcmp(ddComp->sText,CODE_PROG_HEADER))||
       ( (ddComp->sLen) <= (CODE_PROG_HEADER_LENGTH) )) {
    return;
  } */
  /* But to speed things up (because this proc is called to much), we settle */
  /* for this:                                                               */
  if (((ddComp->sText)[CODE_PROG_HEADER_LENGTH-1]!=0)||
      (ddComp->sLen<=CODE_PROG_HEADER_LENGTH))
    return;

  dThing=NULL;
  dComp=ddComp->sText;

  /* next, read the program code - search for str pointers */
  dCompOff=CODE_PROG_HEADER_LENGTH;
  while((dComp[dCompOff]!=CODE_BLK_CODE)&&
        (dComp[dCompOff]!=CODE_BLK_NULL)) {
    dCompOff++;
    dCompOff=DecompGetJump();
  }
  if (dComp[dCompOff]!=CODE_BLK_NULL) { /* We have CODE! */
    dCompOff++;
    dCompEnd=DecompGetJump();
    dCompStart=dCompOff;
    while(dCompOff<dCompEnd) { /* Great disassemble `while' */
      dInstruction=(dComp[dCompOff]&0xff);
      dCompOff++;
      switch(dInstruction) {
        case COP_PUSHPT: /* Here's our baby */
          /* Solaris doesn't like this */ 
          /* dPtr=*((void**)(&dComp[dCompOff])); */
          {
            unsigned long i;
            dPtr=NULL;
            for (i=0;i<CODE_PT_SIZE;i++)
              (unsigned long)(dPtr)|=
                ((unsigned long)(dComp[dCompOff+i]))<<(8*i);
          }
          dCompOff+=CODE_PT_SIZE;
          StrFree(Str(dPtr));
          break;
        case COP_COMMENT:
          dCompOff+=strlen(&dComp[dCompOff])+1;
          break;
        case COP_EXEC:
        case COP_EXECR: 
        case COP_GOTO:
        case COP_IFZ:
        case COP_WHILEZ:
        case COP_PUSHL:
          DecompGetJump();
          break;
        case COP_PUSHV1:
          dCompOff++;
          break;
        case COP_PUSHV2:
          dCompOff+=2;
          break;
        case COP_PUSHV:
        case COP_PUSHV4:
          dCompOff+=4;
          break;
/*        case COP_EQUSR:
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
        case COP_NOP:
        case COP_NULL:
        case COP_BSUB:
        case COP_BADD:
        case COP_ASUB:
        case COP_AADD:
        case COP_COMP:
        case COP_NEG:
        case COP_NOT:
        case COP_TERM:
        case COP_PUSHG:
        case COP_PUSHP: */
        default:
          break;
      } /* decode switch */
    } /* great disassemble while */
  } /* if (we found code) */
}
