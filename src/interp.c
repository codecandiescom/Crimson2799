/* interp.c
 * Written by B. Cameron Lesiuk, 1995
 * Written for use with Crimson2 MUD (written/copyright Ryan Haksi 1995).
 * This source is proprietary. Use of this code without permission from 
 * Ryan Haksi or Cam Lesiuk is strictly prohibited. 
 * 
 * (wi961@freenet.victoria.bc.ca)
 */  

/* General note on the philosophy of this code:
 * Interp is going to run lots. Tons. All the time, in fact. Thus, we want 
 * it to run *FAST*. To accomplish this, all error checking etc. was 
 * left for the compiler to handle and watch for; Interp assumes clean, valid
 * code all the way. Also, some things are done in a bit memory-hogish or 
 * inellegant way; they're done like that to save time. Final word = speed.
 * We have arrays and stuff set up in stupid ways to save a few cycles - it 
 * adds up. 
 */
#include <stdio.h>
#include <string.h>
#ifndef WIN32
  #include <sys/time.h>
#else
  #include <time.h>
#endif
#include <stdlib.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "send.h"
#include "extra.h"
#include "thing.h"
#include "exit.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "code.h"
#include "codestuf.h"
#include "compile.h"
#include "interp.h"
#include "function.h"
#include "parse.h"
#include "base.h"
#include "send.h"
#include "queue.h"

/** Define Statements **/

/* Program defaults */
#define INTERP_PARAMETER  (1<<8) /* default parameter memory */
#define INTERP_LOCAL_VAR     20 /* default # local vars, not incl. sys vars   */
#define INTERP_STACK_SIZE (1<<8) /* stack size (in bytes, not stack units) */

/* structures and unions */

/* Globals */
WORD iCodeOff;        /* offset of current program position */
WORD iCodeStart;      /* offset of start of program         */
WORD iCodeEnd;        /* offset of end of program           */
BYTE *iCode;          /* global pointer to code             */
INTERPVARTYPE *iLocal; /* local variable table    */
LWORD iLocalByte; /* size of iLocal */
INTERPSTACK *iStack; /* system stack  */
LWORD iStackByte; /* size of iStack */
void (*iOpTable[256])(); /* op-code instruction jump table   */
WORD iStackPos;       /* stack position */
WORD iOpCode;         /* current op-code */
INTERPVARTYPE *iParameterList;
LWORD iParameterListByte;
LWORD iMaxInstruction; /* max # instructions we'll execute before exiting */

/* offsets we wish to save so that we can modify them at run-time */
ULWORD iOffset_EVENT_THING;
ULWORD iOffset_CODE_THING;
ULWORD iOffset_SPARE_THING;
ULWORD iOffset_EXIT;
ULWORD iOffset_SEGMENT;
ULWORD iOffset_COMMAND;
ULWORD iOffset_CMD_CMD;
ULWORD iOffset_CMD_SRCKEY;
ULWORD iOffset_CMD_SRCOFFSET;
ULWORD iOffset_CMD_SRCNUM;
ULWORD iOffset_CMD_DSTKEY;
ULWORD iOffset_CMD_DSTOFFSET;
ULWORD iOffset_TIME;
ULWORD iOffset_BLOCK_CMD;



/* sub-functions (code functions, etc) */

/*** DEBUG Macros & Functions ***/

/*** Op-code functions ***/
/* NOTE: remember, these are operating the actual instructions, so try
 * to make them FAST. You may see duplicated code - it's faster than
 * a procedure call. Also, there's no error checking on iCodeOff or iStackPos. 
 * They had better be correct!
 */
/* op-code vars are global so each fn' doesn't have to define it's own (speed)*/
/* Otherwise there's stack allocations and stack offsets to calculate - slow. */
LWORD iValue1,iValue2;
BYTE iDataType1,iDataType2;
STR *iStr1,iStr2;
THING *iThing1,iThing2;
BYTE iDomain1,iDomain2;
WORD iOpCounter;
INTERPVARTYPE iOpRc;
WORD iFunction;
WORD iVariable;

void InterpCOP_NULL() {
  BYTE buf[50];
  if (BIT(codeSet,CODE_DEBUGINTERP)) {
    sprintf(buf,"^gOp-Code recieved: ^y%02X\n",iOpCode);
    SendThing(buf,iLocal[iOffset_EVENT_THING].iPtr);
  }
  return;
}
void InterpCOP_EQUSR() {
  iStackPos--;
  iStack[iStackPos-1].iInt>>=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUSL() {
  iStackPos--;
  iStack[iStackPos-1].iInt<<=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUOR() {
  iStackPos--;
  iStack[iStackPos-1].iInt|=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUXOR() {
  iStackPos--;
  iStack[iStackPos-1].iInt^=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUAND() {
  iStackPos--;
  iStack[iStackPos-1].iInt&=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUSUB() {
  iStackPos--;
  iStack[iStackPos-1].iInt-=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUADD() {
  iStackPos--;
  iStack[iStackPos-1].iInt+=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUMOD() {
  iStackPos--;
  iStack[iStackPos-1].iInt%=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUDIV() {
  iStackPos--;
  if (iStack[iStackPos].iInt) {
    iStack[iStackPos-1].iInt/=iStack[iStackPos].iInt;
  } else {
    iStack[iStackPos-1].iInt=0;
  }
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQUMULT() {
  iStackPos--;
  iStack[iStackPos-1].iInt*=iStack[iStackPos].iInt;
  iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos-1].iInt;
  return;
}
void InterpCOP_EQU() {
  iStackPos--;
  iDataType1=iStack[iStackPos-1].iDataType;
  if (iDataType1 == CDT_INT) {
    iStack[iStackPos-1].iInt=iStack[iStackPos].iInt;
    iStack[iStackPos-1].iVariable->iInt=iStack[iStackPos].iInt;
  } else {
    iStack[iStackPos-1].iPtr=iStack[iStackPos].iPtr;
    iStack[iStackPos-1].iVariable->iPtr=iStack[iStackPos].iPtr;
  }
  return;
}
void InterpCOP_BOR() {
  iStackPos--;
  iStack[iStackPos-1].iInt=(iStack[iStackPos-1].iInt||iStack[iStackPos].iInt);
  return;
}
void InterpCOP_BAND() {
  iStackPos--;
  iStack[iStackPos-1].iInt=(iStack[iStackPos-1].iInt&&iStack[iStackPos].iInt);
  return;
}
void InterpCOP_OR() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt|iStack[iStackPos].iInt;
  return;
}
void InterpCOP_XOR() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt^iStack[iStackPos].iInt;
  return;
}
void InterpCOP_AND() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt&iStack[iStackPos].iInt;
  return;
}
void InterpCOP_BEQU() {
  iStackPos--;
  if (iStack[iStackPos-1].iDataType==CDT_INT) {
    iStack[iStackPos-1].iInt=(iStack[iStackPos-1].iInt==iStack[iStackPos].iInt);
  } else {
    iStack[iStackPos-1].iDataType=CDT_INT;
    iStack[iStackPos-1].iInt=(iStack[iStackPos-1].iPtr==iStack[iStackPos].iPtr);
  }
  return;
}
void InterpCOP_BNEQU() {
  iStackPos--;
  if (iStack[iStackPos-1].iDataType==CDT_INT) {
    iStack[iStackPos-1].iInt=(iStack[iStackPos-1].iInt!=iStack[iStackPos].iInt);
  } else {
    iStack[iStackPos-1].iDataType=CDT_INT;
    iStack[iStackPos-1].iInt=(iStack[iStackPos-1].iPtr!=iStack[iStackPos].iPtr);
  }
  return;
}
void InterpCOP_GEQ() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt>=iStack[iStackPos].iInt;
  return;
}
void InterpCOP_GR() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt>iStack[iStackPos].iInt;
  return;
}
void InterpCOP_LEQ() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt<=iStack[iStackPos].iInt;
  return;
}
void InterpCOP_LS() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt<iStack[iStackPos].iInt;
  return;
}
void InterpCOP_SR() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt>>iStack[iStackPos].iInt;
  return;
}
void InterpCOP_SL() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt<<iStack[iStackPos].iInt;
  return;
}
void InterpCOP_SUB() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt-iStack[iStackPos].iInt;
  return;
}
void InterpCOP_ADD() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt+iStack[iStackPos].iInt;
  return;
}
void InterpCOP_MOD() {
  iStackPos--;
  if (iStack[iStackPos].iInt) {
    iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt%iStack[iStackPos].iInt;
  }
  return;
}
void InterpCOP_DIV() {
  iStackPos--;
  if (iStack[iStackPos].iInt) {
    iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt/iStack[iStackPos].iInt;
  } else {
    iStack[iStackPos-1].iInt=0;
  }
  return;
}
void InterpCOP_MULT() {
  iStackPos--;
  iStack[iStackPos-1].iInt=iStack[iStackPos-1].iInt*iStack[iStackPos].iInt;
  return;
}
void InterpCOP_BSUB() {
  iStack[iStackPos-1].iInt-=1;
  iStack[iStackPos-1].iVariable->iInt-=1;
  return;
}
void InterpCOP_BADD() {
  iStack[iStackPos-1].iInt+=1;
  iStack[iStackPos-1].iVariable->iInt+=1;
  return;
}
void InterpCOP_ASUB() {
  iStack[iStackPos-1].iVariable->iInt-=1;
  return;
}
void InterpCOP_AADD() {
  iStack[iStackPos-1].iVariable->iInt+=1;
  return;
}
void InterpCOP_COMP() {
  iStack[iStackPos-1].iInt=~(iStack[iStackPos-1].iInt);
  return;
}
void InterpCOP_NOT() {
  iStack[iStackPos-1].iInt=!(iStack[iStackPos-1].iInt);
  return;
}
void InterpCOP_NEG() {
  iStack[iStackPos-1].iInt=-(iStack[iStackPos-1].iInt);
  return;
}

void InterpSnoop(BYTE *msg,THING *thing) {
  BASELINK *link;
  SOCK     *sock;

  if ((!msg)||(!thing))
    return;
  if (thing->tType<TTYPE_BASE)
    return;

  for (link=Base(thing)->bLink; 
    (link); link=link->lNext) {
    if (link->lType == BL_C4_SND) {
      sock=BaseControlFind(link->lDetail.lThing);
      if (sock) {
        SEND("^w", sock);
        SEND(msg, sock);
      }
    }
  }
}

void InterpSnoopStr(BYTE *msg) {
  InterpSnoop(msg,iLocal[iOffset_CODE_THING].iPtr);
}

void InterpSnoopStack(ULWORD pos) {
  BYTE      buf[20];

  if (iStack[pos].iDataType==CDT_INT) {
    sprintf(buf,"%li",iStack[pos].iInt);
    InterpSnoopStr(buf);
  } else if (iStack[pos].iDataType==CDT_STR) {
    if(iStack[pos].iPtr) {
      InterpSnoopStr("\"");
      InterpSnoopStr(Str(iStack[pos].iPtr)->sText);
      InterpSnoopStr("\"");
    } else 
      InterpSnoopStr("NULL");
  } else if (iStack[pos].iDataType==CDT_THING) {
    if(iStack[pos].iPtr) {
      InterpSnoopStr(Thing(iStack[pos].iPtr)->tSDesc->sText);
    } else 
      InterpSnoopStr("TNULL");
  } else if (iStack[pos].iDataType==CDT_EXTRA) {
    if(iStack[pos].iPtr) {
      InterpSnoopStr(Extra(iStack[pos].iPtr)->eKey->sText);
    } else 
      InterpSnoopStr("ENULL");
  } else if (iStack[pos].iDataType==CDT_EXIT) {
    if(iStack[pos].iPtr) {
      InterpSnoopStr(Exit(iStack[pos].iPtr)->eKey->sText);
    } else 
      InterpSnoopStr("XNULL");
  } else {
    InterpSnoopStr("???");
  }
}

void InterpCOP_TERM() {return;} /* this fn shouldn't actually ever be called */
void InterpCOP_EXEC() {
  iFunction=(iCode[iCodeOff]&0xFF)|
    ((iCode[iCodeOff+1])<<8); /* allow last one to sign-extend */
  iCodeOff+=2;
  
  /* generate parameter list */
/*
  iOpCounter=0;
  while(fTable[iFunction].fParamType[iOpCounter]!=CDT_NULL) {
    iOpCounter++;
  }
*/
  iStackPos--;
  iOpCounter=iStack[iStackPos].iInt;
  iParameterList[iOpCounter].iDataType=CDT_NULL;
  iParameterList[iOpCounter].iInt=0;
  iParameterList[iOpCounter].iPtr=NULL;
  InterpSnoopStr("C4: ");
  InterpSnoopStr(fTable[iFunction].fText);
  InterpSnoopStr("\n");

  while(iOpCounter) {
    iOpCounter--;
    iStackPos--;
    /* you get it all! I duno. Is it quicker to do if-then's or just this: */
    iParameterList[iOpCounter].iDataType=iStack[iStackPos].iDataType;
    iParameterList[iOpCounter].iInt=iStack[iStackPos].iInt;
    iParameterList[iOpCounter].iPtr=iStack[iStackPos].iPtr;
  }
  iStack[iStackPos].iDataType=fTable[iFunction].fDataType;
  FunctionCheckRegistry(iParameterList); /* check invalid pointer registry */
  (fTable[iFunction].fFunction)(&iStack[iStackPos],iParameterList);
  return;
}
void InterpCOP_EXECR() {
  LWORD i;

  iFunction=(iCode[iCodeOff]&0xFF)|
    ((iCode[iCodeOff+1])<<8); /* allow last one to sign-extend */
  iCodeOff+=2;
  
  /* generate parameter list */
/*
  iOpCounter=0;
  while(fTable[iFunction].fParamType[iOpCounter]!=CDT_NULL) {
    iOpCounter++;
  }
*/
  iStackPos--;
  iOpCounter=iStack[iStackPos].iInt;
  iParameterList[iOpCounter].iDataType=CDT_NULL;
  iParameterList[iOpCounter].iInt=0;
  iParameterList[iOpCounter].iPtr=NULL;
  InterpSnoopStr("C4: ");
  InterpSnoopStr(fTable[iFunction].fText);
  InterpSnoopStr("(");
  for (i=iOpCounter;i>0;i--) {
    InterpSnoopStack(iStackPos-i);
    if (i>1)
      InterpSnoopStr(",");
  }

  while(iOpCounter) {
    iOpCounter--;
    iStackPos--;
    /* you get it all! I duno. Is it quicker to do if-then's or just this: */
    iParameterList[iOpCounter].iDataType=iStack[iStackPos].iDataType;
    iParameterList[iOpCounter].iInt=iStack[iStackPos].iInt;
    iParameterList[iOpCounter].iPtr=iStack[iStackPos].iPtr;
  }
  iStack[iStackPos].iDataType=fTable[iFunction].fDataType;
  FunctionCheckRegistry(iParameterList); /* check invalid pointer registry */
  (fTable[iFunction].fFunction)(&iStack[iStackPos],iParameterList);
  if (fTable[iFunction].fDataType) {
    /* print return value */
    InterpSnoopStr(") return: ");
    InterpSnoopStack(iStackPos);
    InterpSnoopStr("\n");
  } else {
    InterpSnoopStr(")\n");
  }
  iStackPos++;
  return;
}
void InterpCOP_PUSHV() {
  iStack[iStackPos].iDataType=CDT_INT;
  iStack[iStackPos].iInt=(iCode[iCodeOff]&0xFF)|
    ((iCode[iCodeOff+1]&0xFF)<<8)|
    ((iCode[iCodeOff+2]&0xFF)<<16)|
    ((iCode[iCodeOff+3])<<24); /* allow last one to sign-extend */
  iCodeOff+=4;
  iStackPos++;
  return;
}
void InterpCOP_PUSHV1() {
  iStack[iStackPos].iDataType=CDT_INT;
  iStack[iStackPos].iInt=iCode[iCodeOff]; /*  allow sign-extend */
  iCodeOff++;
  iStackPos++;
  return;
}
void InterpCOP_PUSHV2() {
  iStack[iStackPos].iDataType=CDT_INT;
  iStack[iStackPos].iInt=(iCode[iCodeOff]&0xFF)|
    ((iCode[iCodeOff+1])<<8); /* allow last one to sign-extend */
  iCodeOff+=2;
  iStackPos++;
  return;
}
void InterpCOP_PUSHV4() {
  iStack[iStackPos].iDataType=CDT_INT;
  iStack[iStackPos].iInt=(iCode[iCodeOff]&0xFF)|
    ((iCode[iCodeOff+1]&0xFF)<<8)|
    ((iCode[iCodeOff+2]&0xFF)<<16)|
    ((iCode[iCodeOff+3])<<24); /* allow last one to sign-extend */
  iCodeOff+=4;
  iStackPos++;
  return;
}
void InterpCOP_PUSHL() {
  iVariable=(iCode[iCodeOff]&0xFF)| ((iCode[iCodeOff+1]%0xFF)<<8);
  iStack[iStackPos].iDataType=iLocal[iVariable].iDataType;
  iStack[iStackPos].iInt=iLocal[iVariable].iInt;
  iStack[iStackPos].iPtr=iLocal[iVariable].iPtr;
  iStack[iStackPos].iVariable=&iLocal[iVariable];
  iCodeOff+=2;
  iStackPos++;
  return;
}
void InterpCOP_PUSHG() {return;}
void InterpCOP_PUSHP() {return;}
void InterpCOP_PUSHPT() {
  ULWORD i;
  iStack[iStackPos].iDataType=CDT_STR;
  /* Solaris doesn't like this.... */
  /* iStack[iStackPos].iPtr=*((void**)(&iCode[iCodeOff])); */
  (unsigned long)(iStack[iStackPos].iPtr)=0;
  for (i=0;i<CODE_PT_SIZE;i++)
    (unsigned long)(iStack[iStackPos].iPtr)|=
      ((unsigned long)(iCode[iCodeOff+i]))<<(8*i);
  iCodeOff+=CODE_PT_SIZE;
  iStackPos++;
  return;
}

void InterpCOP_GOTO() {
  iCodeOff=(iCode[iCodeOff]&0xFF)|((iCode[iCodeOff+1]&0xFF)<<8);
  return;
}
void InterpCOP_IFZ() {
  iStackPos--;
  if (iStack[iStackPos].iInt) {
    iCodeOff+=2;
  } else {
    iCodeOff=(iCode[iCodeOff]&0xFF)|((iCode[iCodeOff+1]&0xFF)<<8);
  } 
  return;
}
void InterpCOP_WHILEZ() {
  iStackPos--;
  if (iStack[iStackPos].iInt) {
    iCodeOff+=2;
  } else {
    iCodeOff=(iCode[iCodeOff]&0xFF)|((iCode[iCodeOff+1]&0xFF)<<8);
  } 
  return;
}
void InterpCOP_COMMENT() {
  while(iCode[iCodeOff++]);
  return;
}
void InterpCOP_POP() {
  iStackPos--;
  return;
}
void InterpCOP_NOP() {return;}


/* InterpInit initializes things so that Interp doesn't have to do it every 
 * single time. Interp has enough to do. 
 */
void InterpInit(LWORD iMaxInstr) { 
  ULWORD i,iOffset;
  WORD counter;

  iMaxInstruction=iMaxInstr; /* fix our max instruction count */

  /* First, allocate some memory for our various things. */

  /* Local variable table */
  iLocalByte=1;
  i=cSystemVariable+INTERP_LOCAL_VAR; /* take our # system variables */
  i*=sizeof(INTERPVARTYPE);
  while(i) {
    iLocalByte<<=1; /* shift left 1 */
    i>>=1; /* shift right 1 */
  }
  /* we should now have roughly twice what we figure we will use. That's good. */
  MALLOC((void*)iLocal,BYTE,iLocalByte);

  iStackByte=INTERP_STACK_SIZE; /* default stack size */
  MALLOC((void*)iStack,BYTE,iStackByte);

  iParameterListByte=INTERP_PARAMETER; /* default parameter memory */
  MALLOC((void*)iParameterList,BYTE,iParameterListByte);

  iOffset=0;
  /* initialize system local variable table */
  for (i=0;i<=cSystemVariable;i++) {
    if (cSysVar[i].cText!=NULL) {
      iLocal[iOffset].iDataType=cSysVar[i].cType;
      iLocal[iOffset].iInt=cSysVar[i].cInt;
      iLocal[iOffset].iPtr=cSysVar[i].cPtr;
      /* now take note of offsets for variables which Interp changes */
      if (!strcmp(cSysVar[i].cText,"EVENT_THING")) {
        iOffset_EVENT_THING=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"CODE_THING")) {
        iOffset_CODE_THING=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"SPARE_THING")) {
        iOffset_SPARE_THING=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"TIME")) {
        iOffset_TIME=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"SEGMENT")) {
        iOffset_SEGMENT=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"COMMAND")) {
        iOffset_COMMAND=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"CMD_CMD")) {
        iOffset_CMD_CMD=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"CMD_SRCKEY")) {
        iOffset_CMD_SRCKEY=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"CMD_SRCOFFSET")) {
        iOffset_CMD_SRCOFFSET=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"CMD_SRCNUM")) {
        iOffset_CMD_SRCNUM=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"CMD_DSTKEY")) {
        iOffset_CMD_DSTKEY=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"CMD_DSTOFFSET")) {
        iOffset_CMD_DSTOFFSET=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"EXIT")) {
        iOffset_EXIT=iOffset;
      } else if (!strcmp(cSysVar[i].cText,"BLOCK_CMD")) {
        iOffset_BLOCK_CMD=iOffset;
      }
      iOffset++;
    }
  }

  /* initialize op-code jump table */
  for (counter=0;counter<256;counter++) {
    iOpTable[counter]=InterpCOP_NULL; 
  }
  iOpTable[COP_EQUSR]=InterpCOP_EQUSR;
  iOpTable[COP_EQUSL]=InterpCOP_EQUSL;
  iOpTable[COP_EQUOR]=InterpCOP_EQUOR;
  iOpTable[COP_EQUXOR]=InterpCOP_EQUXOR;
  iOpTable[COP_EQUAND]=InterpCOP_EQUAND;
  iOpTable[COP_EQUSUB]=InterpCOP_EQUSUB;
  iOpTable[COP_EQUADD]=InterpCOP_EQUADD;
  iOpTable[COP_EQUMOD]=InterpCOP_EQUMOD;
  iOpTable[COP_EQUDIV]=InterpCOP_EQUDIV;
  iOpTable[COP_EQUMULT]=InterpCOP_EQUMULT;
  iOpTable[COP_EQU]=InterpCOP_EQU;
  iOpTable[COP_BOR]=InterpCOP_BOR;
  iOpTable[COP_BAND]=InterpCOP_BAND;
  iOpTable[COP_OR]=InterpCOP_OR;
  iOpTable[COP_XOR]=InterpCOP_XOR;
  iOpTable[COP_AND]=InterpCOP_AND;
  iOpTable[COP_BEQU]=InterpCOP_BEQU;
  iOpTable[COP_BNEQU]=InterpCOP_BNEQU;
  iOpTable[COP_GEQ]=InterpCOP_GEQ;
  iOpTable[COP_GR]=InterpCOP_GR;
  iOpTable[COP_LEQ]=InterpCOP_LEQ;
  iOpTable[COP_LS]=InterpCOP_LS;
  iOpTable[COP_SR]=InterpCOP_SR;
  iOpTable[COP_SL]=InterpCOP_SL;
  iOpTable[COP_SUB]=InterpCOP_SUB;
  iOpTable[COP_ADD]=InterpCOP_ADD;
  iOpTable[COP_MOD]=InterpCOP_MOD;
  iOpTable[COP_DIV]=InterpCOP_DIV;
  iOpTable[COP_MULT]=InterpCOP_MULT;
  iOpTable[COP_BSUB]=InterpCOP_BSUB;
  iOpTable[COP_BADD]=InterpCOP_BADD;
  iOpTable[COP_ASUB]=InterpCOP_ASUB;
  iOpTable[COP_AADD]=InterpCOP_AADD;
  iOpTable[COP_COMP]=InterpCOP_COMP;
  iOpTable[COP_NOT]=InterpCOP_NOT;
  iOpTable[COP_NEG]=InterpCOP_NEG;
  iOpTable[COP_NULL]=InterpCOP_NULL;
  iOpTable[COP_TERM]=InterpCOP_TERM;
  iOpTable[COP_EXEC]=InterpCOP_EXEC;
  iOpTable[COP_EXECR]=InterpCOP_EXECR;
  iOpTable[COP_PUSHV]=InterpCOP_PUSHV;
  iOpTable[COP_PUSHV1]=InterpCOP_PUSHV1;
  iOpTable[COP_PUSHV2]=InterpCOP_PUSHV2;
  iOpTable[COP_PUSHV4]=InterpCOP_PUSHV4;
  iOpTable[COP_PUSHL]=InterpCOP_PUSHL;
  iOpTable[COP_PUSHG]=InterpCOP_PUSHG;
  iOpTable[COP_PUSHP]=InterpCOP_PUSHP;
  iOpTable[COP_PUSHPT]=InterpCOP_PUSHPT;
  iOpTable[COP_GOTO]=InterpCOP_GOTO;
  iOpTable[COP_IFZ]=InterpCOP_IFZ;
  iOpTable[COP_WHILEZ]=InterpCOP_WHILEZ;
  iOpTable[COP_COMMENT]=InterpCOP_COMMENT;
  iOpTable[COP_POP]=InterpCOP_POP;
  iOpTable[COP_NOP]=InterpCOP_NOP;
}


/* interp
 * This is it. The big-kahuuna. It's supposed to run FAST, so error checking
 * and the like is left for the compiler to take care of. The only thing we
 * try to do here is avoid a segmentation fault should something go really
 * wrong. Best case, nothing will go wrong. Worst case, pointer gets thrown
 * into left field, so try to trap it, ignore the consequences (who cares 
 * what it does in the game - just avoid craping out all together!) and 
 * continue as best we can.
 *
 * As per code.c, iEventThing is the thing the event happened in the presence
 * of. iCodeThing is the thing with the code attached. iCode is the STR 
 * with the actual code in it. (so theoretically, you could call this function
 * to run a script which has absolutely nothing to do with iEventThing or
 * iCodeThing. Hmmmm) It's this way so that a THING can have several scripts
 * of the same type - and it's the calling function's responsibility to sort
 * out what to do. This `division of labour' is arbitrary.
 *
 * iCodeStr is where the code is located. 
 * iSpareThing is used by some events (such as @ENTRY & @EXIT) where 
 *   THREE, not just two, THINGs may be involved.
 * iBlockCmd, if set, blocks the event from happening.
 * iExit is for events (such as @ENTRY & @EXIT) which involve exits.
 *
 * Enjoy.
 */
WORD Interp(STR *iCodeStr,THING *iEventThing,THING *iCodeThing, 
            THING *iSpareThing, STR *iCommand,LWORD *iBlockCmd,EXIT *iExit) {
  WORD iCounter;  /* counter for misc for-loops etc. */
  BYTE buf[256],*iCmd,iWord[256],iSrcKey[256],iDstKey[256];
  LWORD iInstructions;

  if ( !((!strcmp(iCodeStr->sText,CODE_PROG_HEADER)) && (iCodeStr->sLen>CODE_PROG_HEADER_LENGTH)) )
    return 1; /* error */

  iCode=iCodeStr->sText;
  iStackPos=0;
  iInstructions=0;
  /* Flush function.c's pointer registry */
  FunctionFlushRegistry();

  /* update dynamic local system variables */
  if (BIT(codeSet,CODE_DEBUGINTERP)) {
    SendThing("^ainterp: ^Cinitializing local vars\n",iEventThing);
  }
  iLocal[iOffset_EVENT_THING].iPtr=(void *)iEventThing;   
  iLocal[iOffset_CODE_THING].iPtr=(void *)iCodeThing;   
  iLocal[iOffset_SPARE_THING].iPtr=(void *)iSpareThing;
  iLocal[iOffset_EXIT].iPtr=(void *)iExit;
  iLocal[iOffset_TIME].iInt=time(0)-startTime;  
  iLocal[iOffset_SEGMENT].iInt=tmSegment;
  iLocal[iOffset_COMMAND].iPtr=(void *)iCommand;
  if (iCommand) {
    iCmd=StrOneWord(iCommand->sText,iWord);
    ParseFind(iCmd,
              iSrcKey,
              &(iLocal[iOffset_CMD_SRCOFFSET].iInt),
              &(iLocal[iOffset_CMD_SRCNUM].iInt),
              iDstKey,
              &(iLocal[iOffset_CMD_DSTOFFSET].iInt));

    if (iWord[0]) 
      iLocal[iOffset_CMD_CMD].iPtr=(void *)STRCREATE(iWord);
    else 
      iLocal[iOffset_CMD_CMD].iPtr=(void *)NULL;

    if (iSrcKey[0])
      iLocal[iOffset_CMD_SRCKEY].iPtr=(void *)STRCREATE(iSrcKey);
    else
      iLocal[iOffset_CMD_SRCKEY].iPtr=(void *)NULL;

    if (iDstKey[0])
      iLocal[iOffset_CMD_DSTKEY].iPtr=(void *)STRCREATE(iDstKey);
    else
      iLocal[iOffset_CMD_DSTKEY].iPtr=(void *)NULL;

  } else {
    iLocal[iOffset_CMD_CMD].iPtr=(void *)NULL;
    iLocal[iOffset_CMD_SRCKEY].iPtr=(void *)NULL;
    iLocal[iOffset_CMD_SRCOFFSET].iPtr=0;
    iLocal[iOffset_CMD_SRCNUM].iPtr=0;
    iLocal[iOffset_CMD_DSTKEY].iPtr=(void *)NULL;
    iLocal[iOffset_CMD_DSTOFFSET].iPtr=0;
  }
  iLocal[iOffset_BLOCK_CMD].iInt=0; /* default to NO! */

  /* Initialize local variable table */
  iCodeOff=CODE_PROG_HEADER_LENGTH;
  while((iCode[iCodeOff])&&(iCode[iCodeOff]!=CODE_BLK_VAR)) {
    iCodeOff=(iCode[iCodeOff+1]&0xFF)|((iCode[iCodeOff+2]&0xFF)<<8);
  }
  if (iCode[iCodeOff]) {
    if (BIT(codeSet,CODE_DEBUGINTERP)) {
      SendThing("^ainterp: ^Cfound vars - initializing\n",iEventThing);
    }
    iCodeEnd=(iCode[iCodeOff+1]&0xFF)|((iCode[iCodeOff+2]&0xFF)<<8);
    iCodeOff+=3;
    iCodeStart=iCodeOff;
    iCounter=cSystemVariable+1;
    while(iCodeOff<iCodeEnd) {
      iLocal[iCounter].iDataType=iCode[iCodeOff];
      iLocal[iCounter].iInt=0;
      iLocal[iCounter].iPtr=NULL;
      iCodeOff++;
      iCounter++;
    } /* while */
    if (BIT(codeSet,CODE_DEBUGINTERP)) {
      sprintf(buf,"^wVariables: ^c%i local\n",iCounter);
      SendThing(buf,iEventThing);
    }
  } /* init locals */

  /* start running the code */
  if (BIT(codeSet,CODE_DEBUGINTERP)) {
    SendThing("^ainterp: ^Csearching for code\n",iEventThing);
  }
  iCodeOff=CODE_PROG_HEADER_LENGTH;
  while((iCode[iCodeOff])&&(iCode[iCodeOff]!=CODE_BLK_CODE)) {
    iCodeOff=(iCode[iCodeOff+1]&0xFF)|((iCode[iCodeOff+2]&0xFF)<<8);
  }
  if (iCode[iCodeOff]) {
    if (BIT(codeSet,CODE_DEBUGINTERP)) {
      SendThing("^ainterp: ^Cfound code - running\n",iEventThing);
    }
    iCodeEnd=(iCode[iCodeOff+1]&0xFF)|((iCode[iCodeOff+2]&0xFF)<<8);
    iCodeOff+=3;
    iCodeStart=iCodeOff;
    if (BIT(codeSet,CODE_DEBUGINTERP)) { /* print debug info */
      while((iCodeOff<iCodeEnd)&&(iInstructions++<iMaxInstruction)) {
        iOpCode=iCode[iCodeOff]&0xFF;
        iCodeOff++;
        if (iOpCode==COP_TERM) {
          sprintf(buf,"^G%04X^A: ^c%02X",iCodeOff,iOpCode);
          SendThing(buf,iEventThing);
          SendThing("^a\ninterp: ^CTERM encountered - exiting.\n",iEventThing);
          if (iCommand) {
            if (iLocal[iOffset_CMD_CMD].iPtr)
              STRFREE(iLocal[iOffset_CMD_CMD].iPtr);
            if (iLocal[iOffset_CMD_SRCKEY].iPtr)
              STRFREE(iLocal[iOffset_CMD_SRCKEY].iPtr);
            if (iLocal[iOffset_CMD_DSTKEY].iPtr)
              STRFREE(iLocal[iOffset_CMD_DSTKEY].iPtr);
          }
          return(0);
        } else {
          (iOpTable[iOpCode])();
          sprintf(buf,"^G%04X^A: ^c%02X",iCodeOff,iOpCode);
          SendThing(buf,iEventThing);
          sprintf(buf," ^y% 3i",iStackPos);
          SendThing(buf,iEventThing);
          for(iCounter=0;iCounter<iStackPos;iCounter++) {
            if (iStack[iCounter].iDataType==CDT_INT) {
              sprintf(buf," ^P%08lX",iStack[iCounter].iInt);
            } else if (iStack[iCounter].iDataType==CDT_STR) {
              sprintf(buf," ^PS%p",iStack[iCounter].iPtr);
            } else if (iStack[iCounter].iDataType==CDT_THING) {
              sprintf(buf," ^PT%p",iStack[iCounter].iPtr);
            } else if (iStack[iCounter].iDataType==CDT_EXTRA) {
              sprintf(buf," ^PE%p",iStack[iCounter].iPtr);
            } else if (iStack[iCounter].iDataType==CDT_EXIT) {
              sprintf(buf," ^PX%p",iStack[iCounter].iPtr);
            } else if (iStack[iCounter].iDataType==CDT_NULL) {
              sprintf(buf," ^P<NULL>");
            } else {
              sprintf(buf," ^P<type unknown>");
            }
            SendThing(buf,iEventThing);
          }
          SendThing("\n",iEventThing);
        }
  
      }
    } else { /* no debug info */
      while((iCodeOff<iCodeEnd)&&(iInstructions++<iMaxInstruction)) {
        iOpCode=iCode[iCodeOff]&0xFF;
        iCodeOff++;
        if (iOpCode==COP_TERM) {
          if (iCommand) {
            if (iBlockCmd) 
              *iBlockCmd=iLocal[iOffset_BLOCK_CMD].iInt;
            if (iLocal[iOffset_CMD_CMD].iPtr)
              STRFREE(iLocal[iOffset_CMD_CMD].iPtr);
            if (iLocal[iOffset_CMD_SRCKEY].iPtr)
              STRFREE(iLocal[iOffset_CMD_SRCKEY].iPtr);
            if (iLocal[iOffset_CMD_DSTKEY].iPtr)
              STRFREE(iLocal[iOffset_CMD_DSTKEY].iPtr);
          }
          return(0);
        } else {
          (iOpTable[iOpCode])();
        }
      }
    }
    if (BIT(codeSet,CODE_DEBUGINTERP)) {
      sprintf(buf,"^wInstructions executed: ^y%li\n",iInstructions);
      SendThing(buf,iEventThing);
    }
    /* we're done. to get here, we've reached iCodeEnd*/
    if (iBlockCmd) *iBlockCmd=iLocal[iOffset_BLOCK_CMD].iInt;
  }

  if (BIT(codeSet,CODE_DEBUGINTERP)) {
    SendThing("^ainterp: ^Ccode finished - exiting\n",iEventThing);
  }
  /* Finished running code. Exit */
  if (iCommand) {
    if (iLocal[iOffset_CMD_CMD].iPtr)
      STRFREE(iLocal[iOffset_CMD_CMD].iPtr);
    if (iLocal[iOffset_CMD_SRCKEY].iPtr)
      STRFREE(iLocal[iOffset_CMD_SRCKEY].iPtr);
    if (iLocal[iOffset_CMD_DSTKEY].iPtr)
      STRFREE(iLocal[iOffset_CMD_DSTKEY].iPtr);
  }
  return(0);
}


/* InterpDump
 * This function dumps the code of a compiled program to the thing 
 * iThing. Both the hex and ascii code are dumped. No block or code
 * translation is done whatsoever. iThing is where to send the output.
 */
void InterpDump(STR *iComp,THING *iThing) {
  BYTE buf[200],extra_buf[10]; 
  BYTE buf2[60],extra_buf2[10];
  BYTE *pointer;
  WORD i,j;

  buf2[0]=0;
  sprintf(buf,"^G%04X^A:  ^a",0);
  pointer=iComp->sText;
  for (i=0;i<iComp->sLen;i++)
    {

    if ((!(i%16))&&(i))
      {
      strcat(buf,"  ^R");
      strcat(buf,buf2);
      strcat(buf,"\n");
      SendThing(buf,iThing);
      buf2[0]=0;
      sprintf(buf,"^G%04X^A:  ^a",i);
      }
    sprintf(extra_buf,"%02X ",*pointer);
    if ((*pointer>=32)&&(*pointer!=127))
      sprintf(extra_buf2,"%c",*pointer);
    else
      sprintf(extra_buf2,".");

    strcat(buf,extra_buf);
    strcat(buf2,extra_buf2);
    pointer++;
    }
  if (i%16) {
    for(j=0;j<(16-(i%16));j++)
      strcat(buf,"   ");
  }
  strcat(buf,"  ^R");
  strcat(buf,buf2);
  strcat(buf,"\n");
  SendThing(buf,iThing);
  }

/* InterpStackAlloc
 * This proc re-allocates the interpreter stack size if required. The 
 * compiler counts each program's stack requirements, and then calls
 * this procedure; If a C4 program is compiled requiring a stack larger
 * than is currently provided, this proc allocates a larger stack. 
 * NOTE: The interpreter stack will only GROW. If this proc is passed a
 * value SMALLER than the current stack, this proc does nothing. */
void InterpStackAlloc(LWORD iStackSize,LWORD iMaxParameter,LWORD iLocalVar) {
  REALLOC("WARNING! interp.c stack size overflow... resizing\n",
    iStack,INTERPSTACK,iStackSize,iStackByte);
  REALLOC("WARNING! interp.c variable table overflow... resizing\n",
    iLocal,INTERPVARTYPE,iLocalVar+cSystemVariable,iLocalByte);
  REALLOC("WARNING! interp.c function parameter overflow... resizing\n",
    iParameterList,INTERPVARTYPE,iMaxParameter,iParameterListByte);
}
