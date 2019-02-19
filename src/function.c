/*
 * Written by B. Cameron Lesiuk, 1995
 * Written for use with Crimson2 MUD (written/copyright Ryan Haksi 1995).
 * This source is proprietary. Use of this code without permission from 
 * Ryan Haksi or Cam Lesiuk is strictly prohibited. 
 * 
 * (clesiuk@engr.uvic.ca)
 */  

/* Function.c
 * This file contains all functions available in the c-script language.
 */ 

/* HOW TO ADD A FUNCTION 
 * ---------------------
 *
 * Step 1:   Decide on a function name and the parameters.
 * Step 2:   Write a function using FNPROC(function_name) similar to 
 *           the ones already there.
 * Step 3:   a return code or return information is passed back via
 *           the "Return" parameter. See existing functions and interp.h.
 * Step 4:   Enter into fTable (at the end of this file) a table entry
 *           for your newly-created function. The table's entries are:
 *             "" enclosed string - this is the name of the function as 
 *                called within the C4 script language
 *             Procedure pointer (see existing entries for examples)
 *             Return data type - see codestuf.h for data types
 *             Parameter data types - see examples for format. REMEMBER to end
 *               your parameter list with a CDT_NULL!!!!!!!!!!!!!!!!!!!
 *           That's all. It's pretty simple once you get the hang of it.
 *           NOTE: you don't have to put a reference to your function in 
 *           function.h because all references are through fTable (which is 
 *           already in function.h) Cool, eh.
 *
 * WARNING!  If your function allocates or frees a data object (memory)
 *           make sure you call the appropriate pointer registry function.
 *           (FnAddToRegistry() or FnRemoveFromRegistry())!!!
 *           NOTE: This doesn't apply to Property objects. Why? Because
 *           the way we've got it, the C4 coder can never get their
 *           hands on a pointer to a property - there aren't any functions
 *           to support it. Therefore, they can't use an invalid pointer
 *           through them. Prrrrrrfect. 
 *
 * WARNING!  Don't modify this file if you have any of your area files
 *           saved in binary format. IE: all your obj, mob, rst, and 
 *           wld files should be DECOMPILED and in source format!
 *           Otherwise, modifying this file may cause your area files
 *           to become SCRAMBLED AND UNUSABLE. Read the huge warning
 *           in codestuf.c for an expanded explanation.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "queue.h"
#include "send.h"
#include "extra.h"
#include "property.h"
#include "thing.h"
#include "index.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "mobile.h"
#include "player.h"
#include "area.h"
#include "code.h"
#include "codestuf.h"
#include "compile.h"
#include "interp.h"
#include "decomp.h"
#include "function.h"
#include "parse.h"
#include "exit.h"
#include "skill.h"
#include "fight.h"
#include "cmd_move.h"

/***********************************************************************
 *
 * function.c Pointer Registry Functions
 *
 ***********************************************************************
 * There's a small problem with C4 and pointers. Specifically, it is
 * possible for the coder to:
 *   a) assign a variable to a data object 
 *   b) free the object
 *   c) try to use the variable which points to the just-deleted object! 
 *     WHOOOOOPS!
 * The problem had 2 solutions. One, don't let users anywhere near pointers.
 * This sucks as it severely limits flexibility of the C4 code.
 * Two, somehow have a way of verifying a pointer before using it. This
 * registry was set up towards this end. We only have a problem within
 * a single program; after the program ends, all variables are flushed
 * and there's no way to get them back if they have been free'ed. 
 * Therefore, this registry gets flushed each time Interp does it's stuff.
 * 
 * What we do is maintain a log of all pointers which have been free'ed.
 * If an attempt to use one of these pointers is made, block it. Of course,
 * Creating data objects has to also access this log; if we free something, 
 * then allocate a new one and the new one get's the old one's pointer,
 * we need to not-block the pointer anymore.
 *
 * SO, what we do is each call to a function.c function must call this
 * registry to validate it's pointers. If an invalid pointer is detected,
 * this registry sets that pointer to NULL. */
 /* Note: what we SHOULD do is, upon a FREE of a pointer, search all our
 * stack and data variables for that pointer and set any references to it
 * to NULL. I didn't do this originally and now I can't remember why. Hmmm.. */
/* registry - global declaration */
#define FN_DEF_REGISTRY_SIZE (1<<5)
void **FnPointerRegistry;
LWORD FnPointerRegistrySizeByte=0; /* registry starts empty with no memory */
LWORD FnPointerRegistryPos=0;      /* points to first empty (unused) entry */

void FunctionCheckRegistry(INTERPVARTYPE *Param) {
  WORD i,j;
  BYTE datatype;
  void *ptr;

  /* Little bit of optimization here */
  /* I don't really expect the majority of C4 routines to be free-ing */
  if (!FnPointerRegistryPos) return;

  for (i=0;(datatype=Param[i].iDataType)!=CDT_NULL;i++) {
    if (datatype!=CDT_INT) {
      /* pointer type - we gotta check it. */
      ptr=Param[i].iPtr;
      for(j=0; j<FnPointerRegistryPos; j++) {
        if (FnPointerRegistry[j]==ptr) {
          Param[i].iPtr=NULL;
          break;
        }
      }
    }
  }
}

void FnAddToRegistry(void *ptr) {
  REALLOC("WARNING! function.c pointer registry overflow... resizing\n",
    FnPointerRegistry,void*,FnPointerRegistryPos+2,FnPointerRegistrySizeByte);
  FnPointerRegistry[FnPointerRegistryPos]=ptr;
  FnPointerRegistryPos++;
}

void FnRemoveFromRegistry(void *ptr) {
  WORD i;
  for(i=0; i<FnPointerRegistryPos; i++) {
    if (FnPointerRegistry[i]==ptr) {
      FnPointerRegistryPos--;
      FnPointerRegistry[i]=FnPointerRegistry[FnPointerRegistryPos];
      return;
    }
  }
}

void FunctionFlushRegistry() {
  if (FnPointerRegistrySizeByte) {
    FnPointerRegistryPos=0;
  } else {
    /* Initialize registry */
    FnPointerRegistrySizeByte=FN_DEF_REGISTRY_SIZE*sizeof(void*);
    MALLOC((void*)FnPointerRegistry,void*,FnPointerRegistrySizeByte);
  }
}

/***********************************************************************
 *
 * function.c support functions (not called directly by C4 code)
 *
 ***********************************************************************/

/* FnProcessStrSubstitute 
 * This procedure takes a string, such as "%s says hi to %i people", 
 * and the parameters passed to a function, and substitutes the 
 * parameters into the string, copying the final string over to *dest.
 * The size of dest MUST be set. */
BYTE *FnProcessStrSubstitute(BYTE *buf,int size,BYTE *src,INTERPVARTYPE *Param) {
  BYTE buf2[20],*dest,*end;
  BYTE *c1;
  int CurParam;

  CurParam=0; 
  end=&(buf[size-1]); /* find end of buffer */
  dest=buf;
  /* note: we don't use the buffer as an array - instead we just shimmy
   * a pointer along. We do this becuase shimmying it along requires
   * ADDITION, whereas array functions require MULTIPLICATION - slower! */
  if (buf&&src) {
    while((*src)&&(dest!=end)) {
      if (*src=='%') { /* string substitution! */
        src++;
        if (*src) { /* if it exists, LEAVE it incremented - we need src+=2 ! */
          if (*src=='s') { /* str substitution */
            if ((Param[CurParam].iDataType==CDT_STR)&&
                (Param[CurParam].iPtr)) {
              for (c1=Str(Param[CurParam].iPtr)->sText;
                   (*c1)&&(dest!=end);dest++,c1++)
                *dest=*c1;
              dest--;
            }
          }
          else if (*src == 'i') { /* int substitution */
            if (Param[CurParam].iDataType==CDT_INT) {
              sprintf(buf2,"%li",Param[CurParam].iInt);
              for (c1=buf2;(*c1)&&(dest!=end);dest++,c1++)
                *dest=*c1;
              dest--;
            }
          }
          else if (*src == 'c') { /* character substitution */
            if (Param[CurParam].iDataType==CDT_INT)
              *dest=Param[CurParam].iInt;
          }
          /* else ignore character - unknown type */
          if (Param[CurParam].iDataType!=CDT_NULL)
            CurParam++; /* increment our parameter list! */
        }
        else
          src--;
      }
      else if (*src=='\\') { /* override */
        src++;
        if (*(src)) /* notice how we leave the incrementation of src  */
          *dest=*src;  
        else
          src--;
      }
      else {
        *dest=*src;
      }
      src++;
      dest++;
    } /* while */
    *dest=0; /* mark our end of string */
  }
  else
    *dest=0;
  return buf;
}

/* This procedure looks at "command_string" and determines if the
 * command string is calling the command "cmd", checking for 
 * appropriate abreviations, etc. This proc returns the number
 * of letters matched if successful, or 0 if no valid match is found*/
LWORD FnIsCommand(BYTE *command_string,BYTE *cmd) {
  LWORD len;
  BYTE word[FBUF_COMMON_SIZE];
  if (cmd && command_string) {
    StrOneWord(command_string,word);
    len=strlen(word);
    if ((len)&&(strlen(cmd))) {
      if (!(STRNICMP(word,cmd,len))) {
        return len;
      }
    }
  }
  return 0;
}

#define OS_INVENTORY (1<<0)
#define OS_EQUIPPED  (1<<1)
/* ObjectStripRaw :
 *   THING - thing containing other things which you want to remove items from.
 *   INT   - low virtual number
 *   INT   - high virtual number
 *   THING - MOVE flag. If NULL, items are destroyed, else moved to THIS TTYPE_WLD
 *   INT   - Operation flags:
 *            OS_INVENTORY - strips all stuff in inventory (not equipped)
 *            OS_EQUIPPED  - strips all stuff equipped 
 * This proc will remove all objects from THING with virtual numbers
 * greater than or equal to one INT, and lower or equal to other INT.
 * This is used to remove a range of objects, all at the same time.
 * You can use this to remove, say, from a players inventory all objects
 * from a specific area. 
 */
LWORD FnObjectStripRaw(THING *tfrom,ULWORD low,ULWORD high,THING *tto,WORD flags) {
  THING *t,*tnext;
  ULWORD i,count;

  if (!tfrom) {
    return 0;
  }

  if (tto) {
/*    if (tto->tType!=TTYPE_WLD)
      return;*/
    if (tfrom==tto) /* if we'd be moving them right back here */
      return 0;
  }

  if (low>high) {
    i=low;
    low=high;
    high=i;
  }
  for (t=tfrom->tContain,count=0;t;t=tnext) {
    tnext=t->tNext;
    if ( (t->tType == TTYPE_OBJ) && 
         (Obj(t)->oTemplate->oVirtual>=low) &&
         (Obj(t)->oTemplate->oVirtual<=high) &&
         ( ((flags&OS_INVENTORY)&&(!Obj(t)->oEquip))||
           ((flags&OS_EQUIPPED)&&(Obj(t)->oEquip)) ) ) {
      if (tto) {
        ThingTo(t,tto);
      } else {
        ThingFree(t);
        FnAddToRegistry(t);
      }
      count++;
    } /* if object */
  } /* For --next */
  return count;
} /* FnObjectStripRaw */

/***********************************************************************
 *
 * System functions
 *
 ***********************************************************************/

FNPROC(FnVersion) { /* Returns version information for Crimson II */
  Return->iInt=((CRIMSON_MAJOR_VERSION<<8)+CRIMSON_MINOR_VERSION);
}

FNPROC(FnC4Version) { /* Returns version information for code */
  Return->iInt=((CODE_MAJOR_VERSION<<8)+CODE_MINOR_VERSION);
}

/* generate a number between min and max */
FNPROC(FnNumber) {
  Return->iInt=Number(Param[0].iInt, Param[1].iInt);
}

/* Dice1,6 is the same as 1D6 ie roll a 6 sided die once */
FNPROC(FnDice) {
  Return->iInt=Dice(Param[0].iInt, Param[1].iInt);
}

/***********************************************************************
 *
 * String functions
 *
 ***********************************************************************/

FNPROC(FnStrcmp) {
  Return->iInt=1;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    Return->iInt=strcmp(Str(Param[0].iPtr)->sText,Str(Param[1].iPtr)->sText);
}

/* Should be StrExact & should call StrExact */
FNPROC(FnStricmp) {
  Return->iInt=1;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    Return->iInt=STRICMP(Str(Param[0].iPtr)->sText,Str(Param[1].iPtr)->sText);
}

FNPROC(FnStrncmp) {
  Return->iInt=1;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    Return->iInt=strncmp(Str(Param[0].iPtr)->sText,
                       Str(Param[1].iPtr)->sText,
                       Param[2].iInt);
}

FNPROC(FnStrnicmp) {
  Return->iInt=1;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    Return->iInt=STRNICMP(Str(Param[0].iPtr)->sText,
                       Str(Param[1].iPtr)->sText,
                       Param[2].iInt);
}

FNPROC(FnHello) {
  BYTE word[FBUF_COMMON_SIZE],*ptr;

  Return->iInt=0;
  if (Param[0].iPtr) {
    if (FnIsCommand(Str(Param[0].iPtr)->sText,"say")) {
      ptr=Str(Param[0].iPtr)->sText;
      ptr=StrOneWord(ptr,NULL); /* get rid of "say" */
      ptr=StrOneWord(ptr,word); /* get our first word */
      if ((!(STRICMP(word,"hi")))||(!(STRICMP(word,"hello")))) {
        /* our first word is hi or hello - exactly */
        /* now make sure there's no extra bagage. */
        Return->iInt=1;
        for (;*ptr;ptr++) {
          if (*ptr!=' ') {
            Return->iInt=0;
            return;
          }
        }
      }
    }
  }
}

FNPROC(FnYesNo) {
  BYTE word[FBUF_COMMON_SIZE],*ptr;

  Return->iInt=0;
  if (Param[0].iPtr) {
    if (FnIsCommand(Str(Param[0].iPtr)->sText,"say")) {
      ptr=Str(Param[0].iPtr)->sText;
      ptr=StrOneWord(ptr,NULL); /* get rid of "say" */
      ptr=StrOneWord(ptr,word); /* get our first word */
      if (!(STRICMP(word,"yes"))) {
        Return->iInt=1;
      } else if (!(STRICMP(word,"no"))) {
        Return->iInt=-1;
      }

      if (Return->iInt) {
        /* our first word is yes or no - exactly */
        /* now make sure there's no extra bagage. */
        for (;*ptr;ptr++) {
          if (*ptr!=' ') {
            Return->iInt=0;
            return;
          }
        }
      }
    }
  }
}


FNPROC(FnStrlen) {
  Return->iInt=0;
  if (Param[0].iPtr)
    Return->iInt=strlen(Str(Param[0].iPtr)->sText);
}

/* Should use StrFind */
FNPROC(FnStrIn) { /* checks if s1 is in s0, anywhere. Case insensitive. */
  INTERPVARTYPE *str;

  if (Param[0].iPtr) {
    str=&Param[1];
    while(str->iDataType==CDT_STR && str->iPtr) {
      if (StrFind(Str(Param[0].iPtr)->sText, Str(str->iPtr)->sText)) {
        Return->iInt=1;
        return;
      }
      str++;
    }
  }
  Return->iInt=0;
}

FNPROC(FnStrAt) { /* checks if s1 is in s0 at the specified position */
  BYTE *p0,*p1;
  WORD p0len,word;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    p0=Str(Param[0].iPtr)->sText;
    p1=Str(Param[1].iPtr)->sText;
    p0len=strlen(p0);
    while(*p1==' ') p1++;
    for (word=0;(word<Param[2].iInt)&&(*p1);) {
      if (*p1==' ') {
        word++;
        while(*p1==' ') p1++;
      } else p1++;
    }
    if (!STRNICMP(p1,p0,p0len)) Return->iInt=1;
    else Return->iInt=0; 
  } else Return->iInt=0;
}

FNPROC(FnStrAtExact) { /* checks if s0 is in s1 at the specified position -exactly */
  BYTE *p0,*p1;
  WORD p0len,word;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    p0=Str(Param[0].iPtr)->sText;
    p1=Str(Param[1].iPtr)->sText;
    p0len=strlen(p0);
    while(*p1==' ') p1++;
    for (word=0;(word<Param[2].iInt)&&(*p1);) {
      if (*p1==' ') {
        word++;
        while(*p1==' ') p1++;
      } else p1++;
    }
    if (strlen(p1)<p0len) Return->iInt=0;
    else {
      if (!STRNICMP(p1,p0,p0len)) Return->iInt=1;
      else Return->iInt=0; 
      if ((p1[p0len]!=' ')&&(p1[p0len])) /* check for exact - end of word */
        Return->iInt=0;
    }
  } else Return->iInt=0;
}

FNPROC(FnStrIsCmd) { /* checks if s0 is a call for cmd spec'd by s1 */
  INTERPVARTYPE *str;

  Return->iInt=0;
  if (Param[0].iPtr) {
    str = &Param[1];
    while(str->iDataType==CDT_STR && str->iPtr) {
      Return->iInt=FnIsCommand(Str(Param[0].iPtr)->sText, Str(str->iPtr)->sText);
      if (Return->iInt) return;
      str++;
    }
  }
}

FNPROC(FnStrExact) { 
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    Return->iInt=StrExact(Str(Param[0].iPtr)->sText,
      Str(Param[1].iPtr)->sText);
  }
}
/***********************************************************************
 *
 * Send functions
 *
 ***********************************************************************/

/* SendThing */
FNPROC(FnSendThing) { 
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    SendThing(Str(Param[1].iPtr)->sText,Param[0].iPtr);
}

FNPROC(FnSendThingStr) { 
  BYTE buf[FBUF_COMMON_SIZE];
  if (Param[0].iPtr&&Param[1].iPtr) {
    SendThing(FnProcessStrSubstitute(buf,FBUF_COMMON_SIZE,
      Str(Param[1].iPtr)->sText, &(Param[2])),Param[0].iPtr);
  }
}

FNPROC(FnSendAction) { 
  if (Param[3].iPtr) {
    SendAction(Str(Param[3].iPtr)->sText,Param[0].iPtr,
      Param[1].iPtr,Param[2].iInt);
  }
}

FNPROC(FnSendActionStr) { 
  BYTE buf[FBUF_COMMON_SIZE];
  if (Param[3].iPtr) {
    FnProcessStrSubstitute(buf,FBUF_COMMON_SIZE,
      Str(Param[3].iPtr)->sText, &(Param[4]));
    SendAction(buf,Param[0].iPtr, Param[1].iPtr,Param[2].iInt);
  }
}

/***********************************************************************
 *
 * Extra functions
 *
 ***********************************************************************/

FNPROC(FnExtraGetNext) {
  if (Param[0].iPtr) 
    Return->iPtr=((EXTRA *)(Param[0].iPtr))->eNext;
  else 
    Return->iPtr=NULL;
}

FNPROC(FnExtraGetKey) {
  if (Param[0].iPtr) 
    Return->iPtr=((EXTRA *)(Param[0].iPtr))->eKey;
  else 
    Return->iPtr=NULL;
}

FNPROC(FnExtraGetDesc) {
  if (Param[0].iPtr) 
    Return->iPtr=((EXTRA *)(Param[0].iPtr))->eDesc;
  else 
    Return->iPtr=NULL;
}

FNPROC(FnExtraFind) {
  Return->iPtr=NULL;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    Return->iPtr=ExtraFind(Param[0].iPtr,Str(Param[1].iPtr)->sText);
  }
}
  
FNPROC(FnExtraFree) {
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    FnAddToRegistry(Thing(Param[0].iPtr)->tExtra);
    Thing(Param[0].iPtr)->tExtra=ExtraFree(Thing(Param[0].iPtr)->tExtra,
      Param[1].iPtr);
    Return->iInt=1;
  }
}

FNPROC(FnExtraCreate) {
  Return->iPtr=NULL;

  if ((Param[0].iPtr)&&(Param[1].iPtr)&&(Param[2].iPtr)) {
    /* I use ExtraAlloc here because we already have STRs. */
    Thing(Param[0].iPtr)->tExtra=ExtraAlloc(Thing(Param[0].iPtr)->tExtra,
      Param[1].iPtr,Param[2].iPtr);
    FnRemoveFromRegistry(Thing(Param[0].iPtr)->tExtra);
    Return->iPtr=Thing(Param[0].iPtr)->tExtra;
  }
}


FNPROC(FnExtraSetKey) {
  if ((Param[0].iPtr)&&(Param[1].iPtr)) { 
    if (((EXTRA*)(Param[0].iPtr))->eKey) {
      StrFree(Extra(Param[0].iPtr)->eKey);
      FnAddToRegistry(Extra(Param[0].iPtr)->eKey);
    }
    ((EXTRA*)(Param[0].iPtr))->eKey=StrAlloc(Str(Param[1].iPtr));
    FnRemoveFromRegistry(Extra(Param[0].iPtr)->eKey);
    Return->iPtr=Param[0].iPtr;
  }
  else 
    Return->iPtr=NULL;
}

FNPROC(FnExtraSetDesc) {
  if ((Param[0].iPtr)&&(Param[1].iPtr)) { 
    if (((EXTRA*)(Param[0].iPtr))->eDesc) {
      StrFree(((EXTRA*)(Param[0].iPtr))->eDesc);
      FnAddToRegistry(Extra(Param[0].iPtr)->eKey);
    }
    ((EXTRA*)(Param[0].iPtr))->eDesc=StrAlloc(Str(Param[1].iPtr));
    FnRemoveFromRegistry(Extra(Param[0].iPtr)->eKey);
    Return->iPtr=Param[0].iPtr;
  }
  else 
    Return->iPtr=NULL;
}

/***********************************************************************
 *
 * Thing functions
 *
 ***********************************************************************/

FNPROC(FnThingGetType) {
  if (Param[0].iPtr) 
    Return->iInt=(Thing(Param[0].iPtr)->tType);
  else
    Return->iInt=TTYPE_UNDEF;
}

FNPROC(FnThingGetName) {
  if (Param[0].iPtr) 
    Return->iPtr=((THING *)(Param[0].iPtr))->tSDesc;
  else 
    Return->iPtr=NULL;
}

FNPROC(FnThingGetDesc) {
  if (Param[0].iPtr) 
    Return->iPtr=((THING *)(Param[0].iPtr))->tDesc;
  else 
    Return->iPtr=NULL;
}

FNPROC(FnThingGetExtra) {
  if (Param[0].iPtr) 
    Return->iPtr=((THING *)(Param[0].iPtr))->tExtra;
  else 
    Return->iPtr=NULL;
}

FNPROC(FnThingGetNext) {
  if (Param[0].iPtr) 
    Return->iPtr=((THING *)(Param[0].iPtr))->tNext;
  else 
    Return->iPtr=NULL;
}

FNPROC(FnThingGetContain) {
  if (Param[0].iPtr) 
    Return->iPtr=((THING *)(Param[0].iPtr))->tContain;
  else 
    Return->iPtr=NULL;
}

/* Moves PLR, MOB or OBJ from one THING to another THING. Stops FIGHTING, 
   etc. if required */
FNPROC(FnThingTo) {
  WORD srcType,dstType;

  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    srcType=Thing(Param[0].iPtr)->tType;
    dstType=Thing(Param[1].iPtr)->tType;

    if (((srcType==TTYPE_MOB)||(srcType==TTYPE_PLR))&&
        (dstType==TTYPE_WLD)) {
      /* now, let's take care of a few other things before our move */
      /* stop fighting */
      FightStop(Param[0].iPtr);

      /* ok, let's move. */
      ThingTo(Param[0].iPtr,Param[1].iPtr);
      Return->iInt=1;
    } else if (srcType==TTYPE_OBJ) {
      ThingTo(Param[0].iPtr,Param[1].iPtr);
      Return->iInt=1;
    }
  }
}

FNPROC(FnThingFind) {
  BYTE *key;

  if (Param[0].iPtr)
    key=Str(Param[0].iPtr)->sText;   /* key string   */
  else
    key=NULL;

  Return->iPtr=ThingFind(
    key,                                /* key string   */
    Param[1].iInt,                      /* virtual #    */
    Param[2].iPtr,                      /* search thing */
    Param[3].iInt,                      /* search flag  */
    &(Param[4].iInt));                  /* offset (ie Nth item) */
}

FNPROC(FnThingGetWait) {
  if (Param[0].iPtr) 
    Return->iInt=(Thing(Param[0].iPtr)->tWait);
  else
    Return->iInt=0;
}

FNPROC(FnThingSetWait) {
  if (Param[0].iPtr) {
    Return->iInt=(Thing(Param[0].iPtr)->tWait);
    Thing(Param[0].iPtr)->tWait = Param[1].iInt;
  }
  else
    Return->iInt=0;
}

FNPROC(FnThingGetIdleWait) {
  if (Param[0].iPtr) 
    Return->iInt=(Thing(Param[0].iPtr)->tIdleWait);
  else
    Return->iInt=0;
}

FNPROC(FnThingSetIdleWait) {
  if (Param[0].iPtr) {
    Return->iInt=(Thing(Param[0].iPtr)->tIdleWait);
    Thing(Param[0].iPtr)->tIdleWait = Param[1].iInt;
  }
  else
    Return->iInt=0;
}



/***********************************************************************
 *
 * World functions
 *
 ***********************************************************************/

/* Should be WorldOf */
FNPROC(FnWorldOf) {         
  Return->iPtr=WorldOf(Param[0].iInt);
}

FNPROC(FnWorldGetVirtual) {
  Return->iInt=0;  
  if (Param[0].iPtr)
    Return->iInt=Wld(Param[0].iPtr)->wVirtual;
}

FNPROC(FnWorldGetFlag) {
  Return->iInt=0;  
  if (Param[0].iPtr)
    Return->iInt=Wld(Param[0].iPtr)->wFlag;
}

FNPROC(FnWorldSetFlag) {
  Return->iInt=0;  
  if (Param[0].iPtr) {
    Return->iInt=Wld(Param[0].iPtr)->wFlag;
  Wld(Param[0].iPtr)->wFlag=Param[1].iInt;
  }  
}

FNPROC(FnWorldGetFlagBit) {
  Return->iInt=0;  
  if (Param[0].iPtr)
    Return->iInt=(Wld(Param[0].iPtr)->wFlag)&Param[1].iInt;
}

FNPROC(FnWorldSetFlagBit) {
  Return->iInt=0;  
  if (Param[0].iPtr) {
    Return->iInt=(Wld(Param[0].iPtr)->wFlag)&Param[1].iInt;
  BITSET(Wld(Param[0].iPtr)->wFlag,Param[1].iInt);
  }
}

FNPROC(FnWorldClearFlagBit) {
  Return->iInt=0;  
  if (Param[0].iPtr) {
    Return->iInt=(Wld(Param[0].iPtr)->wFlag)&Param[1].iInt;
  BITCLR(Wld(Param[0].iPtr)->wFlag,Param[1].iInt);
  }
}

FNPROC(FnWorldGetType) {
  Return->iInt=0;  
  if (Param[0].iPtr)
    Return->iInt=Wld(Param[0].iPtr)->wType;
}

FNPROC(FnWorldSetType) {
  Return->iInt=0;  
  if (Param[0].iPtr) {
    Return->iInt=Wld(Param[0].iPtr)->wType;
  Wld(Param[0].iPtr)->wType=Param[1].iInt;
  }
}



/***********************************************************************
 *
 * Base functions
 *
 ***********************************************************************/

FNPROC(FnBaseGetKey) {
  Return->iPtr=NULL;
  if (Param[0].iPtr) 
    if ((Thing(Param[0].iPtr)->tType==TTYPE_OBJ) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_PLR))
      Return->iPtr=(Base(Param[0].iPtr)->bKey);
}

FNPROC(FnBaseGetLDesc) {
  Return->iPtr=NULL;
  if (Param[0].iPtr) 
    if ((Thing(Param[0].iPtr)->tType==TTYPE_OBJ) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_PLR))
      Return->iPtr=(Base(Param[0].iPtr)->bLDesc);
}

FNPROC(FnBaseGetInside) {
  Return->iPtr=NULL;
  if (Param[0].iPtr) 
    if ((Thing(Param[0].iPtr)->tType==TTYPE_OBJ) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_PLR))
      Return->iPtr=(Base(Param[0].iPtr)->bInside);
}

FNPROC(FnBaseGetConWeight) {
  Return->iInt=0;
  if (Param[0].iPtr) 
    if ((Thing(Param[0].iPtr)->tType==TTYPE_OBJ) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_PLR))
      Return->iInt=(Base(Param[0].iPtr)->bConWeight);
}

FNPROC(FnBaseGetWeight) {
  Return->iInt=0;
  if (Param[0].iPtr) 
    if ((Thing(Param[0].iPtr)->tType==TTYPE_OBJ) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_PLR))
      Return->iInt=(Base(Param[0].iPtr)->bWeight);
}



/***********************************************************************
 *
 * Object functions
 *
 ***********************************************************************/


FNPROC(FnObjectCreate) {
  Return->iPtr=NULL;
  if (Param[0].iPtr) {
    Return->iPtr=ObjectCreate(ObjectOf(Param[1].iInt),Param[0].iPtr);
    FnRemoveFromRegistry(Return->iPtr);
  }
}

FNPROC(FnObjectCreateNum) {
  ULWORD i;
 
  Return->iPtr=NULL;
  if (Param[2].iInt<1)
    return;
  if (Param[2].iInt>200)
    Param[2].iInt=200;
  if (!Param[0].iPtr)
    return;

  for (i=0;i<Param[2].iInt;i++) {
    Return->iPtr=ObjectCreate(ObjectOf(Param[1].iInt),Param[0].iPtr);
    FnRemoveFromRegistry(Return->iPtr);
  }
}

FNPROC(FnObjectRemove) {
  THING *t;
  OBJTEMPLATE *o;

  Return->iInt=0;
  o=ObjectOf(Param[1].iInt);
  if ((Param[0].iPtr) && (o)) {
    for (t=Thing(Param[0].iPtr)->tContain;t;t=t->tNext) {
      if ( (t->tType == TTYPE_OBJ) && (Obj(t)->oTemplate==o) ) {
        ThingFree(t);
        FnAddToRegistry(t);
        Return->iInt=1;
        return;
      }
    }
  }
}


/* ObjectStrip :
 *   THING - thing containing other things which you want to remove items from.
 *   INT   - low virtual number
 *   INT   - high virtual number
 *   THING - MOVE flag. If NULL, items are destroyed, else moved to THIS TTYPE_WLD
 * This proc will remove all objects from THING with virtual numbers
 * greater than or equal to one INT, and lower or equal to other INT.
 * This is used to remove a range of objects, all at the same time.
 * You can use this to remove, say, from a players inventory all objects
 * from a specific area. 
 */
FNPROC(FnObjectStrip) {
  Return->iInt=FnObjectStripRaw(Param[0].iPtr,Param[1].iInt,
    Param[2].iInt,Param[3].iPtr,OS_INVENTORY|OS_EQUIPPED);
} /* FnObjectStrip */

FNPROC(FnObjectStripInv) {
  Return->iInt=FnObjectStripRaw(Param[0].iPtr,Param[1].iInt,
    Param[2].iInt,Param[3].iPtr,OS_INVENTORY);
} /* FnObjectStripInv */

FNPROC(FnObjectStripEqu) {
  Return->iInt=FnObjectStripRaw(Param[0].iPtr,Param[1].iInt,
    Param[2].iInt,Param[3].iPtr,OS_EQUIPPED);
} /* FnObjectStripEqu */

FNPROC(FnObjectFree) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    if (Thing(Param[0].iPtr)->tType==TTYPE_OBJ) {
      ThingFree(Thing(Param[0].iPtr));
      FnAddToRegistry(Param[0].iPtr);
      Return->iInt=1;
    }
  }
}

FNPROC(FnObjectContain) {
  THING *t;
  OBJTEMPLATE *o;

  Return->iPtr=NULL;
  o=ObjectOf(Param[1].iInt);
  if ((Param[0].iPtr) && (o)) {
    for (t=Thing(Param[0].iPtr)->tContain;t;t=t->tNext) {
      if ( (t->tType == TTYPE_OBJ) && (Obj(t)->oTemplate==o) ) {
        Return->iPtr=t;
        return;
      }
    }
  }
}

FNPROC(FnObjectCount) {
  Return->iInt=0;
  if (Param[0].iPtr) 
    Return->iInt=ObjectPresent(ObjectOf(Param[1].iInt),Param[0].iPtr,OP_ALL);
}

FNPROC(FnObjectCountInv) {
  Return->iInt=0;
  if (Param[0].iPtr) 
    Return->iInt=ObjectPresent(ObjectOf(Param[1].iInt),Param[0].iPtr,OP_INVENTORY);
}

FNPROC(FnObjectCountEqu) {
  Return->iInt=0;
  if (Param[0].iPtr) 
    Return->iInt=ObjectPresent(ObjectOf(Param[1].iInt),Param[0].iPtr,OP_EQUIPPED);
}
                          
/* This proc processes the raw command line of a CHR to uniformly handle the 
 * the process of trading an item from a MOB to a CHR in exchange for the 
 * CHR's money. The params are as follows:
 *   CDT_THING thing1   - merchant MOB
 *   CDT_THING thing2   - customer CHR
 *   CDT_STR   cmd      - CHR's command line
 *   CDT_INT   mark up  - mark up on items (must be 100 or better) 100=list price
 *   CDT_ETC   obj1,obj2... - virtual object #'s of items merchant has unlimited stock.
 *             NOTE: if you use a negative virtual number, the effect is that
 *             the merchant will NOT sell any of that type of object which may
 *             be in the inventory (but will still sell them if they are
 *             marked elsewhere as being an unlimited supply item).
 *             Also, the items will not be removed from inventory in the event
 *             that they are unlimited supply items!!!
 * If this proc does NOT process the command, the return value is 0. If this
 * proc processes the command but no purchases were made, -1 is returned. 
 * If this proc processes the command and purchases WERE made, the number
 * of purchases made is returned */
FNPROC(FnObjectBuy) {
  BYTE          *cmd;
  BYTE           buyKey[FBUF_COMMON_SIZE];
  LWORD          buyOffset;
  LWORD          buyNum;
  LWORD          showVirtual;
  LWORD          lastVirtual;
  BYTE           bufPlr[FBUF_COMMON_SIZE];
  BYTE           bufRoom[FBUF_COMMON_SIZE];
  BYTE           bufObj[FBUF_COMMON_SIZE];
  INTERPVARTYPE *stock;
  INTERPVARTYPE *checkInv;
  THING         *thing;
  THING         *next;
  OBJTEMPLATE   *obj;
  LWORD          i;
  LWORD          foundObj;
  LWORD          actionType;
  LWORD          salesTotal;
  LWORD          markup;
  THING         *tempHolder;
  BYTE           buyAction; /* 0 if "buy", 1 if "list" */

  Return->iInt=0;

  if (!((Param[0].iPtr) && (Param[1].iPtr) && (Param[2].iPtr) )) {
    return;
  } /* non-NULL pointers */

  markup=Param[3].iInt;
  if (markup<100)
    markup=100;

  /* first thing, check to make sure we've been given the correct
   * input */
  cmd=StrOneWord(Str(Param[2].iPtr)->sText,buyKey);
  if (!((Thing(Param[0].iPtr)->tType==TTYPE_MOB) &&
      ((Thing(Param[1].iPtr)->tType==TTYPE_PLR)||
       (Thing(Param[1].iPtr)->tType==TTYPE_MOB)))) {
    return;
  } /* valid THINGs */

  /* must type at least 2 letters of buy,purchase,list */
  if (strlen(buyKey)<2) return;

  /* ok, we've got a MOB, a CHR, and a command line to parse. */
  /* Let's take a look at the command line and see if the CHR */
  /* wants to BUY something */
  if (FnIsCommand(buyKey,"buy") || FnIsCommand(buyKey,"purchase")) {
    buyAction=0;
  } else if (FnIsCommand(buyKey,"list")) {
    buyAction=1;
  } else if (FnIsCommand(buyKey,"show")||FnIsCommand(buyKey,"inspect")||FnIsCommand(buyKey,"examine")) {
    buyAction=2;
  } else {
    return;
  } /* command line is "buy" or "list" */

  /* it's a BUY command all righty! */
  /* mark return code as "yes, we processed this cmd" 
   * however, mark as a "-1": ie: we haven't bought 
   * any YET 
   */
  Return->iInt=-1;  

  /* Note: there's a small quirk in things here. What we need to do is
   * somehow separate what BELONGS TO THE VENDOR, and what is in the
   * INVENTORY and can thus be sold. What we do is look at the parameter
   * list and take out anything which is marked with a minus (-) sign.
   * This separates what the vendor has as its store inventory, and what the
   * vendor has as it's own equipment and personal inventory... so it 
   * (for example) doesn't sell all its clips for its gun. */
  tempHolder=ObjectCreate(corpseTemplate,NULL);
  /* load up our corpse, er I mean temp holder */
  for (checkInv=&Param[4];checkInv->iDataType==CDT_INT;checkInv++) {
    if (checkInv->iInt<0) {
      /* This is an item which we NEED to keep. */
      foundObj=FALSE; /* set to 1 if we found and moved it */
      /* Check inventory for it first. */
      for (thing=Thing(Param[0].iPtr)->tContain;thing;thing=next) {
        next=thing->tNext;
        if ( (thing->tType==TTYPE_OBJ)
          && (Obj(thing)->oTemplate->oVirtual == -checkInv->iInt)
        ) {
          ThingTo(thing,tempHolder);
          foundObj=TRUE;
          break;
        }
      }
      /* What the hell is Cam doing here!?!???? */
      /* Why would we create objects to add to the tempHolder? */
      /* In fact this will create enormous amounts of objects to add back
        into the mobs inventory since they transfer back at the end of this
        routine */
      /* Ahhhh wait I get it this means that this mob will replenish its
         supply of these items for its own personal use */
      if (!foundObj) { /* ie if NOT FOUND IN INVENTORY, check infinite list and
                    * take out of "infinite" store inventory */
        FnRemoveFromRegistry(
          ObjectCreate(ObjectOf(-(checkInv->iInt)), tempHolder)
        );
      }
    }
  }

  /* no parameters given - do inventory listing */
  if ((!*cmd)||(buyAction==1)) {
    SendAction("^b$n has the following items for sale:\n\n"
      ,Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
    /* first do stock items */
    stock=&Param[4];
    while(stock->iDataType==CDT_INT) {
      if ((stock->iInt>=0)&&((obj=ObjectOf(stock->iInt)))) {
        sprintf(bufRoom,"^c     %s  ^b",
          StrTruncate(bufObj,obj->oSDesc->sText,40));
        for (i=strlen(bufRoom);i<54;i++)
          strcat(bufRoom,".");
        sprintf(bufPlr,"%s ^c%8li^Cc\n",bufRoom,
          ((obj->oValue)*markup)/100);
        SendAction(bufPlr,Param[0].iPtr,Param[1].iPtr,
          SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      }
      stock++;
    }

    /* We want to check here to make sure the inventory doesnt contain
     * any unlimited items.*/
    for (thing=Thing(Param[0].iPtr)->tContain;thing;thing=next) {
      next = thing->tNext;
      stock=&Param[4];
      while(stock->iDataType==CDT_INT) {
        if (stock->iDataType==CDT_INT
         && thing
         && thing->tType == TTYPE_OBJ
         && Obj(thing)->oTemplate->oVirtual==stock->iInt
         && !Obj(thing)->oEquip
        ) {
          FnAddToRegistry(thing);
          THINGFREE(thing);
        }
        stock++;
      }
    }

    /* next do inventory items */
    showVirtual=-1;
    /* we use the showVirtual to track our virtual obj # */
    /* so that we only print 1 of each object type  */
    while(1) {
      lastVirtual=showVirtual; /* save last showVirtual */
      /* first, find our smallest (un-displayed) object */
      for (thing=Thing(Param[0].iPtr)->tContain;thing;thing=next) {
        next = thing->tNext;
        if ((thing->tType==TTYPE_OBJ)&&(!Obj(thing)->oEquip)) {
          if ((Obj(thing)->oTemplate->oVirtual>lastVirtual)&&
              ((Obj(thing)->oTemplate->oVirtual<showVirtual)  ||
               (lastVirtual==showVirtual)))
            showVirtual=Obj(thing)->oTemplate->oVirtual;
        }
      }

      /* Display this object. */
      if (showVirtual==lastVirtual) {
        /* we are done - move everything back from our tempHolder */
        while(tempHolder->tContain)
          ThingTo(tempHolder->tContain,Thing(Param[0].iPtr));
        THINGFREE(tempHolder);
        return; /* we're done */
      }
      obj=ObjectOf(showVirtual);
      sprintf(bufRoom,"^c     %s  ^b",
        StrTruncate(bufObj,obj->oSDesc->sText,40));
      for (i=strlen(bufRoom);i<54;i++)
        strcat(bufRoom,".");
      sprintf(bufPlr,"%s ^c%8li^Cc ^b(%li in stock)\n",bufRoom,
        ((obj->oValue)*markup)/100,
        ObjectPresent(obj,Param[0].iPtr,OP_ALL));
      SendAction(bufPlr,Param[0].iPtr,Param[1].iPtr,
        SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
    } /* while (1) inventory loop */
  } /* if... no parameters, show inventory */

  /* See if user specified HOW MANY to buy */
  ParseFind(cmd, buyKey, &buyOffset, &buyNum, NULL, NULL);
  /* buyNum==-1 means buy all */
  if (buyNum == TF_ALLMATCH) {
    SendAction("^p$n raises an eyebrow, and says ^C\"Perhaps a more reasonable number\"\n",Param[0].iPtr,Param[1].iPtr,
      SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
    /* we are done - move everything back from our tempHolder */
    while(tempHolder->tContain)
      ThingTo(tempHolder->tContain,Thing(Param[0].iPtr));
    THINGFREE(tempHolder);
    return;
  }

  /* let's see if it's in our regular stock... */
  stock=&Param[4];
  while(stock->iDataType==CDT_INT) {
    if (stock->iInt>=0 && (obj=ObjectOf(stock->iInt))) {
      /* this object exists... check it's name */
      if (StrIsKey(buyKey,obj->oKey)) {
        /* Hey hey hey! We've found it! */
        buyOffset--;
        if (buyOffset>0) { stock++; continue; }

        /* just show it to them */
        if (buyAction==2) {
          thing = ObjectCreate(obj, NULL);
          ThingShowDetail(thing, Param[1].iPtr, TRUE);
          THINGFREE(thing);
          Return->iInt=0;
          return;
        }

        salesTotal=buyNum*obj->oValue*markup/100;

        /* check value of item, and CHR's money situation */
        if ((Character(Param[1].iPtr)->cMoney>=salesTotal)) {
          /* they have enough money, but can they carry it all? */
          if (1) { /* check their weight here */
            /* purchase is made! */

            /* exchange cash */
            Character(Param[1].iPtr)->cMoney-=salesTotal;
            Character(Param[0].iPtr)->cMoney+=salesTotal;

            /* and now create "buyNum" obj's in chr's inventory */
            for (i=0;i<buyNum;i++) 
              FnRemoveFromRegistry(ObjectCreate(obj,Param[1].iPtr));

            /* tell Plr how much they spent, etc. */
            sprintf(bufPlr,"^bYou buy %li %s%s for %li credit%s.\n",
              buyNum, obj->oSDesc->sText, 
              (buyNum>1)?"s":"", 
              salesTotal,
              (salesTotal>1)?"s":"");
            sprintf(bufRoom,"^b$N buys %li %s%s.\n",
              buyNum, obj->oSDesc->sText,
              (buyNum>1)?"s":"");
            actionType=SEND_VISIBLE;

            /* and lastly, return the number of items purchased */
            Return->iInt=buyNum;
          } else {
            /* can't carry it all! Too much weight! */
            sprintf(bufPlr,"^bYou cannot carry all that weight!\n");
            bufRoom[0]=0;
            actionType=0;
          }
        } else {
          /* not enough cash!!! */
          sprintf(bufPlr,"^bYou don't have %li credit%s to pay for %li %s%s!\n",
            salesTotal,(salesTotal>1)?"s":"",
            buyNum,obj->oSDesc->sText,
            (buyNum>1)?"s":"");
          bufRoom[0]=0;
          actionType=0;
        }
        SendAction(bufPlr,Param[0].iPtr,Param[1].iPtr,
          SEND_DST|actionType|SEND_CAPFIRST);
        if (bufRoom[0])
          SendAction(bufRoom,Param[0].iPtr,Param[1].iPtr,
            SEND_ROOM|actionType|SEND_CAPFIRST);
        /* we are done - move everything back from our tempHolder */
        while(tempHolder->tContain)
          ThingTo(tempHolder->tContain,Thing(Param[0].iPtr));
        THINGFREE(tempHolder);
        return;
      } /* we've found object they want in stock */
    } /* virtual numbered object exists */
    stock++;
  } /* while... check to see if item is in stock */

  /* it's not in stock... let's see if it's in MOB's inventory... */
  /* note: we only check MOB's INVENTORY, not EQUIPMENT! This way,
   * the mob can use/keep personal items, and have an inventory
   * of sellable items. */
  /* Look for the first Object */
  thing = CharThingFind(Thing(Param[0].iPtr), buyKey, -1, Thing(Param[0].iPtr), TF_OBJ, &buyOffset);

  if (!thing) {
  /* Hmmmm... not in stock or inventory... sorry! */

    sprintf(bufPlr,"^p$n says \"Sorry, I haven't got any of those in stock.\"\n");
    SendAction(bufPlr,Param[0].iPtr,Param[1].iPtr,
      SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);

  } else {
    /* we have it in our inventory! */
    /* just show it to them */
    if (buyAction==2) {
      ThingShowDetail(thing, Param[1].iPtr, TRUE);
      Return->iInt=0;
      return;
    }

    /* first, count how many we have to see if we have enough */
    if (buyNum<=ObjectPresent(
      Obj(thing)->oTemplate,Param[0].iPtr,OP_ALL)) {
      /* we have enough! Does the CHR have the cash? */
      salesTotal=buyNum*Obj(thing)->oTemplate->oValue*markup/100;
      if ((Character(Param[1].iPtr)->cMoney>=salesTotal)) {
        obj=Obj(thing)->oTemplate;
        /* they have enough money, but can they carry it all? */
        if ( (obj->oWeight*buyNum)
            +(Base(Param[1].iPtr)->bConWeight)
            <= CharGetCarryMax(Param[1].iPtr)
        ) { /* check their weight here */
          /* purchase is made! */

          /* exchange cash */
          Character(Param[1].iPtr)->cMoney-=salesTotal;
          Character(Param[0].iPtr)->cMoney+=salesTotal;

          /* transfer items from merchant to customer */
          for (i=0;i<buyNum; ) {
            for (thing=Thing(Param[0].iPtr)->tContain; thing; thing=next) {
              next = thing->tNext;
              if ((thing->tType==TTYPE_OBJ)&&(!Obj(thing)->oEquip)) {
                if (Obj(thing)->oTemplate->oVirtual==obj->oVirtual) {
                  ThingTo(thing,Param[1].iPtr);
                  i++;
                  break;
                } /* matching Virtual #'s */
              } /* if Obj */
            } /* loop through inventory */
            if (!thing) { /* double check - if we run out of items, exit */
              break;
            }
          } /* loop for # items desired */

          /* tell Plr how much they spent, etc. */
          sprintf(bufPlr,"^bYou buy %li %s%s for %li credit%s.\n",
            buyNum , obj->oSDesc->sText, 
            (buyNum>1)?"s":"", 
            salesTotal,
            (salesTotal>1)?"s":"");
          sprintf(bufRoom,"^b$N buys %li %s%s.\n",
            buyNum, obj->oSDesc->sText,
            (buyNum>1)?"s":"");
          actionType=SEND_VISIBLE;

          /* and lastly, return the number of items purchased */
          Return->iInt=buyNum;
        } else {
          /* can't carry it all! Too much weight! */
          sprintf(bufPlr,"^bYou cannot carry all that weight!\n");
          bufRoom[0]=0;
          actionType=0;
        }
      } else {
        /* not enough cash!!! */
        sprintf(bufPlr,"^bYou don't have %li credit%s to pay for %li %s%s!\n",
          salesTotal,(salesTotal>1)?"s":"",
          buyNum,Obj(thing)->oTemplate->oSDesc->sText,
          (buyNum>1)?"s":"");
        bufRoom[0]=0;
        actionType=0;
      } /* not enough cash */
    } else {
      /* haven't got enough of those items! */
      sprintf(bufPlr,"^p$n says \"I only have %li of those in stock.\"\n",
        ObjectPresent(Obj(thing)->oTemplate,
        Param[0].iPtr,OP_ALL));
      bufRoom[0]=0;
      actionType=SEND_AUDIBLE;
    } /* not enough in inventory */

    SendAction(bufPlr,Param[0].iPtr,Param[1].iPtr,
      SEND_DST|actionType|SEND_CAPFIRST);
    if (bufRoom[0])
      SendAction(bufRoom,Param[0].iPtr,Param[1].iPtr,
        SEND_ROOM|actionType|SEND_CAPFIRST);
  } 

  /* we are done - move everything back from our tempHolder */
  while(tempHolder->tContain)
    ThingTo(tempHolder->tContain,Thing(Param[0].iPtr));
  THINGFREE(tempHolder);
} /* FnObjectBuy */

/* This proc processes the raw command line of a CHR to uniformly handle the 
 * the process of trading an item from a CHR to a MOB in exchange for the 
 * MOB's money. The params are as follows:
 *   CDT_THING thing1   - merchant MOB
 *   CDT_THING thing2   - customer CHR (person with the item)
 *   CDT_STR   cmd      - CHR's command line
 *   CDT_INT   markdown - markdown of item from list price for purchase (0-100)
 *                        Item is purchased for ((list price)*markdown)/100
 *                        NOTE: if you include a negative virtual number, 
 *                        the effect is that the THING bought will be free'd,
 *                        instead of being added to the inventory of the
 *                        vendor. Now, you don't need to put in every object
 *                        you have unlimited supply of because ObjectBuy will
 *                        take care of that. However, if you have something,
 *                        say some ammo clips with which the MOB protects
 *                        itself, which you don't want added to or sold off, 
 *                        this give you a way to make those few items an
 *                        exception.
 *   CDT_ETC   type1,type2... - objects & types in which merchant will buy.
 * This proc returns TRUE if SELL command was dealt with, including if the
 * customer tried to SELL an item which the merchant does not carry (ie deal
 * in),  else FALSE if command was not a SELL command. */
FNPROC(FnObjectSell) {
  BYTE          *cmd;
  BYTE           sellKey[FBUF_COMMON_SIZE];
  LWORD          sellOffset;
  LWORD          sellNum;
  LWORD          markdown;
  LWORD          sellOperation; /* = 0 if "sell", =1 if "value" */
  LWORD          sellPrice;
  BYTE           bufPlr[FBUF_COMMON_SIZE];
  THING         *thing;
  INTERPVARTYPE *stock;
  LWORD          sellThis;

  Return->iInt=0;

  if (!((Param[0].iPtr) && (Param[1].iPtr) && (Param[2].iPtr) )) {
    return;
  } /* non-NULL pointers*/

  markdown=Param[3].iInt;
  if (markdown>100)
    markdown=100;

  /* first thing, check to make sure we've been given the correct
   * input */
  cmd=StrOneWord(Str(Param[2].iPtr)->sText,sellKey);
  if (!((Thing(Param[0].iPtr)->tType==TTYPE_MOB) &&
      ((Thing(Param[1].iPtr)->tType==TTYPE_PLR)||
       (Thing(Param[1].iPtr)->tType==TTYPE_MOB)))) {
    return;
  } /* valid THINGs */

  /* ok, we've got a MOB, a CHR, and a command line to parse. */
  /* Let's take a look at the command line and see if the CHR */
  /* wants to SELL something */
  if (FnIsCommand(sellKey,"sell")&&(strlen(sellKey)>1)) {
    /* command line is "sell"  */
    sellOperation=0;
  } else if (FnIsCommand(sellKey,"value")) {
    sellOperation=1;
  } else {
    return;
  }
  /* it's a SELL/VALUE command all righty! */
  /* mark return code as "yes, we processed this cmd" 
   * however, mark as a "-1": ie: we haven't done anything yet.
   */
  Return->iInt=-1;  

  /* no objects to sell / value */ 
  if (!*cmd) {
    if (sellOperation) { /* empty value command */
      SendAction("^aYou try to get $n to value NOTHING.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$N tries to get $n to value NOTHING.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^p$n says, \"Nothing would be worth, hm, let me see... NOTHING!\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n says to $N, \"Nothing would be worth, hm, let me see... NOTHING!\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    } else { /* empty sell command */
      SendAction("^aYou try to sell NOTHING to $n.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$N tries to sell NOTHING to $n.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^p$n asks, \"Ah, yes, but what would you like to sell?\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n asks $N, \"Ah, yes, but what would you like to sell?\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    }
    return;
  } /* no params to sell/value command */

  /* Parse player's command line */
  ParseFind(cmd, sellKey, &sellOffset, &sellNum, NULL, NULL);
  /* sellNum==-1 means buy all */

  /* now loop through the player's inventory - use ThingFind to process
   * our command line appropriately */
  /* Look for the first Object */
  thing = CharThingFind(Param[1].iPtr, sellKey, -1, Thing(Param[1].iPtr), TF_OBJINV, &sellOffset);
  if (!thing) {
    /* Hmmmm... player doesn't have that item... sorry! */
    if (sellOperation) { /* value command */
      SendAction("^aYou want to value what?!\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$N tries to have $n value a non-existent item.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^p$n says, \"That particular imaginary object is worth 24884727c.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n tells $N, \"That particular imaginary object is worth 23883727c.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    } else {
      SendAction("^aYou try to sell what?! to $n.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$N tries to sell a non-existent item to $n.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^p$n says, \"Hmmm, sorry, I don't deal in imaginary objects.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n tells $N, \"Hmmm, sorry, I don't deal in imaginary objects.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    }
    return;
  } 
  while (thing && sellNum != 0) {
    /* plr has it in their inventory! */

    /* precalc sellPrice */
    sellPrice=Obj(thing)->oTemplate->oValue;
    sellPrice=(sellPrice*markdown)/100; /* mark down item cost */

    /* initial action report to plr/room */
    if (sellOperation) { /*  value command */
      SendAction("^a$N asks $n to value an item.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    } else {
      SendAction("^a$N tries to sell an object to $n.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    }

    /* First, check to see if we deal in this item...
     * Check parameter list for objects and object types. 
     */
    sellThis = FALSE;
    stock = &Param[4];
    while (stock->iDataType==CDT_INT || stock->iDataType==CDT_STR) {
      if (stock->iDataType==CDT_INT) {
        if (Obj(thing)->oTemplate->oVirtual == stock->iInt) {
          sellThis = TRUE;
          break;
        }
      } else if (stock->iDataType == CDT_STR) {
        if (Obj(thing)->oTemplate->oType == TYPEFIND(Str(stock->iPtr)->sText, oTypeList)) {
          sellThis = TRUE;
          break;
        }
      }
      stock++;
    }

    if (!sellThis) {
      SendAction("^p$n says, ",Param[0].iPtr,Param[1].iPtr,
        SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n tells $N, ",Param[0].iPtr,Param[1].iPtr,
        SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
      sprintf(bufPlr,"\"I dont deal in things like $n.\"\n");
      SendAction(bufPlr,thing,Param[1].iPtr,
        SEND_DST|SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);

    /* Next, check if we have the cash (and are willing to spend it)
     * for this item. Note: MOBS will not buy an object if the purchase
     * price is more than 50% of their remaining cash. This prevents the
     * MOB from buying one thing, and being stuck with it, basically 
     * disabled because it can't buy any more, and nobody can buy the item.
     */
    } else if (sellOperation) { /*  value command */
      SendAction("^p$n says, ",Param[0].iPtr,Param[1].iPtr,
        SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n tells $N, ",Param[0].iPtr,Param[1].iPtr,
        SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
      sprintf(bufPlr,"\"That $n is worth %ldc.\"\n",sellPrice);
      SendAction(bufPlr,thing,Param[1].iPtr,
        SEND_DST|SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    } else {
      /* check money situation */

      /* not enough cash to cover it */
      if (sellPrice>(Character(Param[0].iPtr)->cMoney>>1)) {
        SendAction("^p$n says, ",Param[0].iPtr,Param[1].iPtr,
          SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("^p$n tells $N, ",Param[0].iPtr,Param[1].iPtr,
          SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
        sprintf(bufPlr,"\"I don't have enough cash to buy your $n.\"\n");
        SendAction(bufPlr,thing,Param[1].iPtr,
          SEND_DST|SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);

      /* It's a deal! transfer item to MOB and exchange cash */
      } else {
        /* exchange cash */
        Character(Param[1].iPtr)->cMoney+=sellPrice;
        Character(Param[0].iPtr)->cMoney-=sellPrice;
        if (Return->iInt==-1)
          Return->iInt=0;  
        Return->iInt+=sellPrice;
        ThingTo(thing,Param[0].iPtr);

        /* announce what happened */
        SendAction("^pYou sell $n",Param[0].iPtr,Param[1].iPtr,
          SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("^p$N sells",Param[0].iPtr,Param[1].iPtr,
          SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
        sprintf(bufPlr," $n.\n");
        SendAction(bufPlr,thing,Param[1].iPtr,
          SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
        sprintf(bufPlr," $n for %ldc.\n",sellPrice);
        SendAction(bufPlr,thing,Param[1].iPtr,
          SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      }
    }

    /* setup for next object to sell/value */
    thing = CharThingFind(Thing(Param[1].iPtr), sellKey, -1, Thing(Param[1].iPtr), TF_OBJINV|TF_CONTINUE, &sellOffset);
    if (sellNum>0) sellNum--;
  } /* while Thing Find Loop */
} /* FnObjectSell */

FNPROC(FnObjectGetVirtual) {
  Return->iInt=0;
  if (Param[0].iPtr) 
    if (Thing(Param[0].iPtr)->tType==TTYPE_OBJ) {
      Return->iInt=Obj(Param[0].iPtr)->oTemplate->oVirtual;
    }
}

/* FnObjectRedeem - redeem scanned-in electronic or bio technology
 *   CDT_THING thing1   - merchant MOB
 *   CDT_THING thing2   - customer CHR (person with the item)
 *   CDT_STR   cmd      - CHR's command line
 *   CDT_INT   markdown - markdown of item from list price for purchase (0-100)
 *   CDT_INT   flag     - flags 
 *      
 * Flags: OR_BIO  - MOB will buy biological scans
 *        OR_CHIP - MOB will buy electrical scans
 *        OR_RICH - money will just be GENERATED. It will not come from the 
 *                  MOB. Without this flag, the MOB will only redeem 
 *                  scanner information if it has cash to cover the cost.
 */
FNPROC(FnObjectRedeem) {
  LWORD          markdown;
  LWORD          flag;
  FLAG           sFlag;
  LWORD          bioScanned;
  LWORD          chipScanned;
  BYTE          *cmd;
  BYTE           redeemKey[FBUF_COMMON_SIZE];
  LWORD          redeemOffset;
  LWORD          redeemNum;
  LWORD          redeemOperation;
  THING         *thing;
  LWORD          redeemPrice;
  BYTE           bufPlr[FBUF_COMMON_SIZE];

  Return->iInt=0;
  if (!(Param[0].iPtr && Param[1].iPtr && Param[2].iPtr))
    return;

  markdown=Param[3].iInt;
  if (markdown>100)
    markdown=100;
  if (markdown<10)
    markdown=10;

  flag=Param[4].iInt;

  /* first thing, check to make sure we've been given the correct
   * input */
  if (!((Thing(Param[0].iPtr)->tType==TTYPE_MOB) &&
      ((Thing(Param[1].iPtr)->tType==TTYPE_PLR)||
       (Thing(Param[1].iPtr)->tType==TTYPE_MOB)))) {
    return;
  } /* valid THINGs */
  cmd=StrOneWord(Str(Param[2].iPtr)->sText,redeemKey);

  /* ok, we've got a MOB, a CHR, and a command line to parse. */
  /* Let's take a look at the command line and see if the CHR */
  /* wants to SELL something */
  if (FnIsCommand(redeemKey,"redeem")) {
    /* command line is "redeem"  */
    redeemOperation=0;
  } else if (FnIsCommand(redeemKey,"examine")) {
    redeemOperation=1;
  } else {
    return;
  }
  /* it's a REDEEM/VALUE command all righty! */
  /* mark return code as "yes, we processed this cmd" 
   * however, mark as a "-1": ie: we haven't done anything yet.
   */
  Return->iInt=-1;  

  /* no objects to sell / examine */ 
  if (!*cmd) {
    if (redeemOperation) { /* empty examine command */
      SendAction("^aYou try to get $n to examine NOTHING.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$N tries to get $n to examine NOTHING.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^p$n says, \"Nothing would be worth, hm, let me see... NOTHING!\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n says to $N, \"Nothing would be worth, hm, let me see... NOTHING!\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    } else { /* empty sell command */
      SendAction("^aYou try to redeem NOTHING to $n.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$N tries to redeem NOTHING to $n.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^p$n asks, \"Ah, yes, but what would you like to redeem?\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n asks $N, \"Ah, yes, but what would you like to redeem?\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    }
    return;
  } /* no params to redeem/examine command */

  /* Parse player's command line */
  ParseFind(cmd, redeemKey, &redeemOffset, &redeemNum, NULL, NULL);
  /* redeemNum==-1 means redeem all */

  /* now loop through the player's inventory - use ThingFind to process
   * our command line appropriately */
  /* Look for the first Object */
  thing = CharThingFind(Thing(Param[1].iPtr), redeemKey, -1, Thing(Param[1].iPtr), TF_OBJINV, &redeemOffset);
  if (!thing) {
    /* Hmmmm... player doesn't have that item... sorry! */
    if (redeemOperation) { /* examine command */
      SendAction("^aYou want to examine what?!\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$N tries to have $n examine a non-existent item.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^p$n says, \"That particular imaginary merchandise is worth 24884727c.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n tells $N, \"That particular imaginary merchandise is worth 23883727c.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    } else {
      SendAction("^aYou try to redeem what?! to $n.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$N tries to get $n redeem a non-existent item.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^p$n says, \"Hmmm, sorry, I don't deal in imaginary merchandise.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n tells $N, \"Hmmm, sorry, I don't deal in imaginary merchandise.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    }
    return;
  } 
  while (thing && redeemNum != 0) {
    /* plr has it in their inventory! */

    SendAction("^a$n checks out $A $N...\n",
      Param[0].iPtr,thing,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    /* First, check if object IS a scanner, and if it is, check that */
    /* it scans material we are interested in.                       */
    if (Obj(thing)->oTemplate->oType!=OTYPE_SCANNER) {
      SendAction("^p$n says, \"Hmmm, this isn't a scanner.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
      SendAction("^p$n tells $N, \"Hmmm, this isn't a scanner.\"\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
    } else {
      sFlag       = OBJECTGETFIELD(thing, OF_SCANNER_SFLAG);
      bioScanned  = OBJECTGETFIELD(thing, OF_SCANNER_BIO);
      chipScanned = OBJECTGETFIELD(thing, OF_SCANNER_CHIP);
      if ( ((BIT(flag,OR_CHIP)) && (BIT(sFlag,OSF_SCANCHIP)) )
       ||  ((BIT(flag,OR_BIO )) && (BIT(sFlag,OSF_SCANBIO )) )
      ) {
        /* OK! We have a scanner, and we deal in it's type. */
        /* Let's do the redeem/examine!                       */

        /* precalc redeemPrice */
        redeemPrice=0;
        if (BIT(flag,OR_BIO))
          redeemPrice+=bioScanned;
        if (BIT(flag,OR_CHIP))
          redeemPrice+=chipScanned;
          
        /* We have our redeem/examine price */
        if (redeemOperation) { /* examine command */
          SendAction("^p$n says, ",Param[0].iPtr,Param[1].iPtr,
            SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
          SendAction("^p$n tells $N, ",Param[0].iPtr,Param[1].iPtr,
            SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
          sprintf(bufPlr,"\"The scans in this $N is worth %ldc.\"\n",redeemPrice);
          SendAction(bufPlr,Param[1].iPtr,thing,
            SEND_SRC|SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
        } else { /* redeem command */
          /* Next, check if we have the cash (and are willing to spend it)
           * for this item. Note: MOBS will not buy scans if the purchase
           * price is more than 50% of their remaining cash. This prevents the
           * MOB from buying one thing, and being stuck with it, basically 
           * disabled because it can't buy any more, and nobody can buy the item.
           */
          /* check money situation */
          /* not enough cash to cover it */
          if ((redeemPrice>(Character(Param[0].iPtr)->cMoney>>1))
           &&(!(BIT(flag,OR_RICH)))) {
            SendAction("^p$n says, ",Param[0].iPtr,Param[1].iPtr,
              SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
            SendAction("^p$n tells $N, ",Param[0].iPtr,Param[1].iPtr,
              SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
            SendAction("\"I don't have enough cash to buy these scans.\"\n",
              Param[1].iPtr,thing,
              SEND_SRC|SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
          } else {
            /* exchange cash */
            /* exchange cash */
            Character(Param[1].iPtr)->cMoney+=redeemPrice;
            if (!(BIT(flag,OR_RICH)))
              Character(Param[0].iPtr)->cMoney-=redeemPrice;
            if (Return->iInt==-1)
              Return->iInt=0;  
            Return->iInt+=redeemPrice;
            /* Empty scanner */
            if (BIT(flag,OR_BIO))
              OBJECTSETFIELD(thing,OF_SCANNER_BIO, 0);
            if (BIT(flag,OR_CHIP))
              OBJECTSETFIELD(thing,OF_SCANNER_CHIP,0);

            /* announce what happened */
            SendAction("^pYou redeem $N",Param[1].iPtr,thing,
              SEND_SRC|SEND_AUDIBLE|SEND_CAPFIRST);
            SendAction("^p$n redeems $N",Param[1].iPtr,thing,
              SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
            sprintf(bufPlr," for %ldc.\n",redeemPrice);
            SendAction(bufPlr,Param[1].iPtr,thing,
              SEND_SRC|SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
          }
        }
        /* in case no transaction actually occurred, make sure we mark as
         * parsing this command */
        if (!Return->iInt)
          Return->iInt=-1;
      } else {
        SendAction("^p$n says, \"I don't deal in this type of scanner.\"\n",
          Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_AUDIBLE|SEND_CAPFIRST);
        SendAction("^p$n tells $N, \"I don't deal in this type of scanner.\"\n",
          Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_AUDIBLE|SEND_CAPFIRST);
      }
    }
    /* setup for next object to sell/examine */
    thing = CharThingFind(Thing(Param[1].iPtr), redeemKey, -1, Thing(Param[1].iPtr), TF_OBJINV|TF_CONTINUE, &redeemOffset);
    if (redeemNum>0) redeemNum--;
  } /* while Thing Find Loop */
} /* FnObjectRedeem */

/***********************************************************************
 *
 * Character functions
 *
 ***********************************************************************/

FNPROC(FnCharThingFind) {
  BYTE *key;

  if (Param[1].iPtr)
    key=Str(Param[1].iPtr)->sText;   /* key string   */
  else
    key=NULL;

  Return->iPtr=CharThingFind(
    Param[0].iPtr,
    key,                                /* key string   */
    Param[2].iInt,                      /* virtual #    */
    Param[3].iPtr,                      /* search thing */
    Param[4].iInt,                      /* search flag  */
    &(Param[5].iInt));                  /* offset (ie Nth item) */
}

FNPROC(FnCharAction) { /* send a command by someone */
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if ( (((THING*)(Param[0].iPtr))->tType==TTYPE_PLR)||
         (((THING*)(Param[0].iPtr))->tType==TTYPE_MOB) ) {
      Return->iInt=ParseCommand(Param[0].iPtr,Str(Param[1].iPtr)->sText);
    } else Return->iInt=0;
  } else Return->iInt=0;
}

FNPROC(FnCharActionStrip) { /* send a command by someone - strip first word */
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if ( (((THING*)(Param[0].iPtr))->tType==TTYPE_PLR)||
         (((THING*)(Param[0].iPtr))->tType==TTYPE_MOB) ) {
      Return->iInt=ParseCommand(Param[0].iPtr,StrOneWord(Str(Param[1].iPtr)->sText,NULL));
    } else Return->iInt=0;
  } else Return->iInt=0;
}

FNPROC(FnCharActionStr) { /* send a command by someone */
  BYTE cmd[FBUF_COMMON_SIZE];
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if ((((THING*)(Param[0].iPtr))->tType==TTYPE_PLR)||
        (((THING*)(Param[0].iPtr))->tType==TTYPE_MOB)) {
      FnProcessStrSubstitute(cmd,FBUF_COMMON_SIZE,
        Str(Param[1].iPtr)->sText, &(Param[2]));
      Return->iInt=ParseCommand(Param[0].iPtr,cmd);
    } else Return->iInt=0;
  } else Return->iInt=0;
}


/* gets Char's current level */
FNPROC(FnCharGetLevel) {
  Return->iInt=0;
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      Return->iInt=Character(Param[0].iPtr)->cLevel;
    }
}

/* gets Char's current Hit Points */
FNPROC(FnCharGetHitP) {
  Return->iInt=0;
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      Return->iInt=Character(Param[0].iPtr)->cHitP;
    }
}

/* sets Char's current Hit Points */
FNPROC(FnCharSetHitP) {
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      if (Param[1].iInt>CharGetHitPMax(Param[0].iPtr))
        Character(Param[0].iPtr)->cHitP = CharGetHitPMax(Param[0].iPtr);
      else
        Character(Param[0].iPtr)->cHitP = Param[1].iInt;
      Return->iInt = Character(Param[0].iPtr)->cHitP;
    }
}

/* Gets the Characters current Hit Point Max */
FNPROC(FnCharGetHitPMax) {
  Return->iInt=0;
  if (Param[0].iPtr) /* errorchecking by CharGetHitP */
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      Return->iInt=CharGetHitPMax(Param[0].iPtr);
}

FNPROC(FnCharGetMovePMax) {
  Return->iInt=0;
  if (Param[0].iPtr) /* errorchecking by CharGetHitP */
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      Return->iInt=CharGetMovePMax(Param[0].iPtr);
}

FNPROC(FnCharGetPowerPMax) {
  Return->iInt=0;
  if (Param[0].iPtr) /* errorchecking by CharGetHitP */
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      Return->iInt=CharGetPowerPMax(Param[0].iPtr);
}

FNPROC(FnCharGetMoney) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      Return->iInt=Character(Param[0].iPtr)->cMoney;
    }
  }
}

/* Set Money to a parm determined amount */
FNPROC(FnCharSetMoney) {
  if (Param[0].iPtr) {
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      Return->iInt = Character(Param[0].iPtr)->cMoney;
      Character(Param[0].iPtr)->cMoney=Param[1].iInt;
    }
  }
}
FNPROC(FnCharGetExp) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      Return->iInt=Character(Param[0].iPtr)->cExp;
    }
  }
}


/* this player gets experience */
FNPROC(FnCharGainExp) {
  Return->iInt = 0;
  if (Param[0].iPtr) {
    if (Thing(Param[0].iPtr)->tType==TTYPE_PLR) {
      Return->iInt = Character(Param[0].iPtr)->cExp;
      PlayerGainExp(Param[0].iPtr, Param[1].iInt);
    }
  }
}

/* group splits experience */
FNPROC(FnCharGainExpGroup) {
  Return->iInt = 0;
  if (Param[0].iPtr) {
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      CharGainExpFollow(Param[0].iPtr, Param[1].iInt);
    }
  }
}


FNPROC(FnCharPractice) {
  Return->iInt = 0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR)||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      if (FnIsCommand(Str(Param[1].iPtr)->sText,"practice")) {
        /* this is indeed a valid practice call */
        Return->iInt = 1;
        SkillPractice(Param[0].iPtr,Str(Param[1].iPtr)->sText,
          Param[2].iInt,Param[3].iInt,Param[4].iInt);
      }
    }
  }
}

FNPROC(FnCharGetFighting) {
  Return->iPtr=NULL;
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      Return->iPtr=Character(Param[0].iPtr)->cFight;
}

FNPROC(FnCharIsLeader) {
  Return->iInt=0;
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      if ((Character(Param[0].iPtr)->cLead)==Param[0].iPtr)
        Return->iInt=1;
}

FNPROC(FnCharGetLeader) {
  Return->iPtr=NULL;
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      Return->iPtr=Character(Param[0].iPtr)->cLead;
}

FNPROC(FnCharGetFollower) {
  Return->iPtr=NULL;
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      Return->iPtr=Character(Param[0].iPtr)->cFollow;
}

FNPROC(FnCharAddFollower) {
  Return->iPtr=NULL;
  if (Param[0].iPtr && Param[0].iPtr)
    if ( (Thing(Param[0].iPtr)->tType==TTYPE_PLR ||
          Thing(Param[0].iPtr)->tType==TTYPE_MOB)
       &&(Thing(Param[1].iPtr)->tType==TTYPE_PLR ||
          Thing(Param[1].iPtr)->tType==TTYPE_MOB)
       )
      CharAddFollow(Param[0].iPtr, Param[1].iPtr);
}

FNPROC(FnCharRemoveFollower) {
  Return->iPtr=NULL;
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      CharRemoveFollow(Param[0].iPtr);
}

/***********************************************************************
 *
 * Player functions
 *
 ***********************************************************************/
FNPROC(FnPlayerWrite) {
  THING *inside;
  THING *player;

  player = Param[0].iPtr;
  if (player && player->tType==TTYPE_PLR) {
    inside = Base(player)->bInside;
    if (inside && inside->tType==TTYPE_WLD)
      Plr(player)->pDeathRoom = Wld(inside)->wVirtual;
    PlayerWrite(player,PWRITE_CLONE);
  }
}

FNPROC(FnPlayerGetBank) {
  Return->iInt=0;
  if (Param[0].iPtr)
    if (Thing(Param[0].iPtr)->tType==TTYPE_PLR) {
      Return->iInt=Plr(Param[0].iPtr)->pBank;
    }
}

FNPROC(FnPlayerSetBank) {
  Return->iInt=0;
  if (Param[0].iPtr)
    if (Thing(Param[0].iPtr)->tType==TTYPE_PLR) {
      Return->iInt=Plr(Param[0].iPtr)->pBank;
      Plr(Param[0].iPtr)->pBank=Param[1].iInt;
    }
}

FNPROC(FnPlayerBank) {
  BYTE          *cmd;
  WORD           bankOperation;
  BYTE           bankKey[FBUF_COMMON_SIZE];
  LWORD          bankOffset;
  LWORD          bankNum;
  BYTE           bankBuf[FBUF_COMMON_SIZE];

  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)&&(Param[2].iPtr))
    if (Thing(Param[1].iPtr)->tType==TTYPE_PLR) {
      cmd=StrOneWord(Str(Param[2].iPtr)->sText,bankKey);
    } else {
      return;
    }

  /* ok, we've got a PLR, and a command line to parse. */
  /* Let's take a look at the command line and see if the PLR */
  /* wants to DEPOSIT something */
  if (strlen(bankKey)<2) 
    return;

  if (FnIsCommand(bankKey,"deposit")) {
    bankOperation=0;
  } else if (FnIsCommand(bankKey,"withdraw")) {
    bankOperation=1;
  } else if (FnIsCommand(bankKey,"balance")) {
    bankOperation=2;
  } else {
    return;
  }
  /* it's a bank command all righty! */
  /* mark return code as "yes, we processed this cmd" */
  Return->iInt=1;

  /* Parse player's command line */
  ParseFind(cmd, bankKey, &bankOffset, &bankNum, NULL, NULL);
  /* bankNum==-1 means buy all */

  if (bankOperation==0) { /* deposit */
    if (bankNum==-1)
      bankNum=Character(Param[1].iPtr)->cMoney;

    if (bankNum<0) 
      bankNum=0;

    if (Thing(Param[0].iPtr)->tType==TTYPE_OBJ) {
      if (bankNum>Character(Param[1].iPtr)->cMoney) {
        sprintf(bankBuf,"^aThe $n shows \"Insufficient funds provided for deposit.\"\n"); 
        SendAction(bankBuf,
          Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^a$n shows something to $N.\n",
          Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      }

      sprintf(bankBuf,"^aThe $n shows \"Deposit for account '$N' of %lic accepted. Thank you.\"\n",
        bankNum); 
      SendAction(bankBuf,
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$n shows something to $N.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    } else {
      if (bankNum>Character(Param[1].iPtr)->cMoney) {
        sprintf(bankBuf,"^p$n says \"It doesn't seem, $N, that you have that much cash.\"\n"); 
        SendAction(bankBuf,
          Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^a$n says something to $N.\n",
          Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      }
      sprintf(bankBuf,"^p$n says \"Thank you, $N. %lic will be added to your account.\"\n",
        bankNum); 
      SendAction(bankBuf,
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$n says something to $N.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    }
    /* now move money */
    Character(Param[1].iPtr)->cMoney-=bankNum;
    Plr(Param[1].iPtr)->pBank+=bankNum;
  } else if (bankOperation==1) { /* withdraw */
    if (bankNum==-1)
      bankNum=Plr(Param[1].iPtr)->pBank;
    
    if (bankNum<0) 
      bankNum=0;

    if (Thing(Param[0].iPtr)->tType==TTYPE_OBJ) {
      if (bankNum>Plr(Param[1].iPtr)->pBank) {
        sprintf(bankBuf,"^aThe $n shows \"Insufficient funds for withdrawal.\"\n"); 
        SendAction(bankBuf,
          Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^a$n shows something to $N.\n",
          Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      }
      sprintf(bankBuf,"^aThe $n shows \"Withdraw from account '$N' of %lic.\"\n",
        bankNum); 
      SendAction(bankBuf,
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$n shows something to $N.",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    } else {
      if (bankNum>Plr(Param[1].iPtr)->pBank) {
        sprintf(bankBuf,"^p$n says \"You don't have that many funds in your account, $N.\"\n"); 
        SendAction(bankBuf,
          Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
        SendAction("^a$n says something to $N.\n",
          Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
        return;
      }
      sprintf(bankBuf,"^p$n says \"Here you go, $N, %lic. Have a nice day.\"\n",bankNum); 
      SendAction(bankBuf,
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$n says something to $N.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    }
    /* now move money */
    Plr(Param[1].iPtr)->pBank-=bankNum;
    Character(Param[1].iPtr)->cMoney+=bankNum;

  } else { /* Balance */
    if (Thing(Param[0].iPtr)->tType==TTYPE_OBJ) {
      sprintf(bankBuf,"^aThe $n shows \"Account Balance for $N: %lic\"\n",
        Plr(Param[1].iPtr)->pBank); 
      SendAction(bankBuf,
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$n shows something to $N.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    } else {
      sprintf(bankBuf,"^p$n says \"$N, your account balance is %lic.\"\n",
        Plr(Param[1].iPtr)->pBank); 
      SendAction(bankBuf,
        Param[0].iPtr,Param[1].iPtr,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);
      SendAction("^a$n says something to $N.\n",
        Param[0].iPtr,Param[1].iPtr,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST);
    }
  }
}

FNPROC(FnPlayerGainPractice) {
  Return->iInt = 0;
  if (Param[0].iPtr) {
    if (Thing(Param[0].iPtr)->tType==TTYPE_PLR) {
      BYTE buf[256];

      Return->iInt = Plr(Param[0].iPtr)->pPractice;
      Plr(Param[0].iPtr)->pPractice += Param[1].iInt;
      sprintf(buf, "^wYou gain %ld practices (You now have %ld available)\n", 
        Param[1].iInt,
        Plr(Param[0].iPtr)->pPractice
      );
      SendThing(buf, Param[0].iPtr);
    }
  }
}

/* this player gets fame */
FNPROC(FnPlayerGainFame) {
  THING * player;

  Return->iInt = 0;
  player = Param[0].iPtr;
  if (player) {
    if (player->tType==TTYPE_PLR) {
      LWORD fame = Param[1].iInt;

      MINSET( fame, Plr(player)->pFame*-1 );
      MAXSET( fame, 3 );
      PlayerGainFame(player, fame);
      Return->iInt = Plr(player)->pFame;
    }
  }
}

/* this player gets infamy */
FNPROC(FnPlayerGainInfamy) {
  THING * player;

  Return->iInt = 0;
  player = Param[0].iPtr;
  if (player) {
    if (player->tType==TTYPE_PLR) {
      LWORD infamy = Param[1].iInt;

      MINSET( infamy, Plr(player)->pInfamy*-1 );
      MAXSET( infamy, 3 );
      PlayerGainInfamy(player, infamy, NULL);
      Return->iInt = Plr(player)->pInfamy;
    }
  }
}

/* player, message, exp, fame, property, value */
FNPROC(FnPlayerSolvedQuest) {
  THING *player;
  LWORD  fame = Param[3].iInt;

  Return->iInt = 0;
  player = Param[0].iPtr;
  if (!player || player->tType!=TTYPE_PLR) return;
  if (!Param[4].iPtr) return;
  if (PropertyGetLWord(player, Str(Param[4].iPtr)->sText, 0) >= Param[5].iInt) return;
  if (Param[1].iPtr)
    SendThing(Str(Param[1].iPtr)->sText,Param[0].iPtr);
  PropertySetLWord(player, Str(Param[4].iPtr)->sText, Param[5].iInt);
  PlayerGainExp(Param[0].iPtr, Param[2].iInt);

  MINSET( fame, Plr(player)->pFame*-1 );
  MAXSET( fame, 3 );
  PlayerGainFame(player, fame);
  Return->iInt = Param[5].iInt;
}

FNPROC(FnPlayerGetRace) {
  Return->iInt=0;
  if (Param[0].iPtr)
    if (Thing(Param[0].iPtr)->tType==TTYPE_PLR) {
      Return->iInt=Plr(Param[0].iPtr)->pRace;
    }
}


/***********************************************************************
 *
 * Fight functions
 *
 ***********************************************************************/
FNPROC(FnFightStop) {
  if (Param[0].iPtr)
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB))
      FightStop(Param[0].iPtr);
}

FNPROC(FnFightStart) {
  if (Param[0].iPtr&&Param[1].iPtr)
    if (((Thing(Param[0].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) &&
        ((Thing(Param[1].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[1].iPtr)->tType==TTYPE_MOB))) 
      FightStart(Param[0].iPtr,Param[1].iPtr,1,NULL);
}

FNPROC(FnFightDamage) {
  Return->iInt=0;
  if (Param[0].iPtr&&Param[5].iPtr) {
    if ((Thing(Param[0].iPtr)->tType==TTYPE_PLR) ||
        (Thing(Param[0].iPtr)->tType==TTYPE_MOB)) {
      Return->iInt=FightDamage(Param[0].iPtr,Param[1].iInt,Param[2].iInt,Param[3].iInt,
        Param[4].iInt,Str(Param[5].iPtr)->sText);
    } 
  }
}

FNPROC(FnFightDamageRaw) {
  Return->iInt=0;
  if ((Param[0].iPtr) && (Thing(Param[0].iPtr)->tType<TTYPE_CHARACTER))
    return;
  if ((!Param[1].iPtr) ||(Thing(Param[1].iPtr)->tType<TTYPE_CHARACTER))
    return;

  /* Thing, Target, Damage to do - thing is optional (ie for crediting xp for a kill */
  Return->iInt=FightDamagePrimitive(Param[0].iPtr,Param[1].iPtr,Param[2].iInt);
}


FNPROC(FnMobileCreate) {
  Return->iPtr=NULL;
  if (Param[0].iPtr) 
    if (Thing(Param[0].iPtr)->tType==TTYPE_WLD) {
      Return->iPtr=MobileCreate(MobileOf(Param[1].iInt),Param[0].iPtr);
      FnRemoveFromRegistry(Return->iPtr);
    }
}

FNPROC(FnMobileGetVirtual) {
  Return->iInt=0;
  if (Param[0].iPtr) 
    if (Thing(Param[0].iPtr)->tType==TTYPE_MOB) {
      Return->iInt=Mob(Param[0].iPtr)->mTemplate->mVirtual;
    }
}


FNPROC(FnMobileCreateNum) {
  ULWORD i;
 
  Return->iPtr=NULL;
  if (Param[2].iInt<1)
    return;
  if (Param[2].iInt>200)
    Param[2].iInt=200;
  if (!Param[0].iPtr)
    return;

  if (Thing(Param[0].iPtr)->tType==TTYPE_WLD)
    for (i=0;i<Param[2].iInt;i++) {
      Return->iPtr=MobileCreate(MobileOf(Param[1].iInt),Param[0].iPtr);
      FnRemoveFromRegistry(Return->iPtr);
    }
}

FNPROC(FnMobileFree) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    if (Thing(Param[0].iPtr)->tType==TTYPE_MOB) {
      ThingFree(Thing(Param[0].iPtr));
      FnAddToRegistry(Param[0].iPtr);
      Return->iInt=1;
    }
  }
}

/***********************************************************************
 *
 * Exit functions
 *
 ***********************************************************************/

FNPROC(FnExitFind) {
  Return->iPtr=NULL;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    if (Thing(Param[0].iPtr)->tType==TTYPE_WLD) 
      Return->iPtr=ExitFind(Wld(Param[0].iPtr)->wExit,Str(Param[1].iPtr)->sText);
}

FNPROC(FnExitDir) {
  Return->iPtr=NULL;
  if (Param[0].iPtr)
    if (Thing(Param[0].iPtr)->tType==TTYPE_WLD) 
      Return->iPtr=ExitDir(Wld(Param[0].iPtr)->wExit,Param[1].iInt);
}

FNPROC(FnExitReverse) {
  Return->iPtr=NULL;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    if (Thing(Param[0].iPtr)->tType==TTYPE_WLD) 
      Return->iPtr=ExitReverse(Param[0].iPtr,Param[1].iPtr);
}

FNPROC(FnExitIsCorner) {
  Return->iInt=EDIR_UNDEFINED;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    Return->iInt=ExitIsCorner(Param[0].iPtr,Param[1].iPtr);
}

FNPROC(FnExitFree) {
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    if (Thing(Param[0].iPtr)->tType==TTYPE_WLD) {
      FnAddToRegistry(Wld(Param[0].iPtr)->wExit);
      Wld(Param[0].iPtr)->wExit=ExitFree(Wld(Param[0].iPtr)->wExit,Param[1].iPtr);
      Return->iInt=1;
    }
}

FNPROC(FnExitCreate) {
  Return->iPtr=NULL;
  if ((Param[0].iPtr)&&(Param[6].iPtr))
    if ((Thing(Param[0].iPtr)->tType==TTYPE_WLD) &&
        (Thing(Param[6].iPtr)->tType==TTYPE_WLD)) {
      /* Ok, here we go. Gulp. */
      Wld(Param[0].iPtr)->wExit=Return->iPtr=ExitCreate(
        Wld(Param[0].iPtr)->wExit, /* eList */
        Param[1].iInt,             /* eDir */
        Param[2].iPtr?Str(Param[2].iPtr)->sText:NULL, /* eKey */
        Param[3].iPtr?Str(Param[3].iPtr)->sText:NULL, /* eDesc */
        Param[4].iInt,             /* eFlag */
        Param[5].iInt,             /* eKeyObj */
        Param[6].iPtr);            /* eWorld */
      FnRemoveFromRegistry(Return->iPtr);
    }
}


FNPROC(FnExitGetDir) {
  Return->iInt=EDIR_UNDEFINED;
  if (Param[0].iPtr)
    Return->iInt=Exit(Param[0].iPtr)->eDir;
}

FNPROC(FnExitSetDir) {
  LWORD i;
  Return->iInt=EDIR_UNDEFINED;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    if (Thing(Param[0].iPtr)->tType==TTYPE_WLD) 
      if ((Param[2].iInt>EDIR_MIN)&&(Param[2].iInt<EDIR_UNDEFINED)) {
        /* Check if this direction exists - abort if it does */
        i=Exit(Param[1].iPtr)->eDir;
        Exit(Param[1].iPtr)->eDir=EDIR_UNDEFINED;
        if (ExitDir(Wld(Param[1].iPtr)->wExit,Param[2].iInt)) {
          Exit(Param[1].iPtr)->eDir=i;
          return; 
        }
        Return->iInt=Exit(Param[1].iPtr)->eDir;
        Exit(Param[1].iPtr)->eDir=Param[2].iInt;
      }
}

FNPROC(FnExitGetFlag) {
  Return->iInt=0;
  if (Param[0].iPtr)
    Return->iInt=Exit(Param[0].iPtr)->eFlag;
}

FNPROC(FnExitSetFlag) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    Return->iInt=Exit(Param[0].iPtr)->eFlag;
    Exit(Param[0].iPtr)->eFlag=Param[1].iInt;
  }
}


FNPROC(FnExitGetKeyObj) {
  Return->iInt=0;
  if (Param[0].iPtr)
    Return->iInt=Exit(Param[0].iPtr)->eKeyObj;
}

FNPROC(FnExitSetKeyObj) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    Return->iInt=Exit(Param[0].iPtr)->eKeyObj;
    Exit(Param[0].iPtr)->eKeyObj=Param[1].iInt;
  }
}

FNPROC(FnExitGetKey) {
  Return->iPtr=NULL;
  if (Param[0].iPtr)
    Return->iPtr=Exit(Param[0].iPtr)->eKey;
}

FNPROC(FnExitSetKey) {
  STR *s;
  Return->iInt=0;
  if (Param[0].iPtr) {
    if (!Param[1].iPtr) {
      if (Exit(Param[0].iPtr)->eKey) {
        FnAddToRegistry(Exit(Param[0].iPtr)->eKey);
        StrFree(Exit(Param[0].iPtr)->eKey);
      }
      Exit(Param[0].iPtr)->eKey=Param[1].iPtr;
      Return->iInt=1;
    } else if ((s=StrAlloc(Param[1].iPtr))) {
      FnRemoveFromRegistry(s);
      if (Exit(Param[0].iPtr)->eKey) {
        FnAddToRegistry(Exit(Param[0].iPtr)->eKey);
        StrFree(Exit(Param[0].iPtr)->eKey);
      }
      Exit(Param[0].iPtr)->eKey=s;
      Return->iInt=1;
    }
  }
}

FNPROC(FnExitGetDesc) {
  Return->iPtr=NULL;
  if (Param[0].iPtr)
    Return->iPtr=Exit(Param[0].iPtr)->eDesc;
}

FNPROC(FnExitSetDesc) {
  STR *s;
  Return->iInt=0;
  if (Param[0].iPtr) {
    if (!Param[1].iPtr) {
      if (Exit(Param[0].iPtr)->eDesc) {
        FnAddToRegistry(Exit(Param[0].iPtr)->eDesc);
        StrFree(Exit(Param[0].iPtr)->eDesc);
      }
      Exit(Param[0].iPtr)->eDesc=Param[1].iPtr;
      Return->iInt=1;
    } else if ((s=StrAlloc(Param[1].iPtr))) {
      FnRemoveFromRegistry(s);
      if (Exit(Param[0].iPtr)->eDesc) {
        FnAddToRegistry(Exit(Param[0].iPtr)->eDesc);
        StrFree(Exit(Param[0].iPtr)->eDesc);
      }
      Exit(Param[0].iPtr)->eDesc=s;
      Return->iInt=1;
    }
  }
}

FNPROC(FnExitGetWorld) {
  Return->iPtr=NULL;
  if (Param[0].iPtr)
    Return->iPtr=Exit(Param[0].iPtr)->eWorld;
}

FNPROC(FnExitSetWorld) {
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr))
    if (Thing(Param[1].iPtr)->tType==TTYPE_WLD) {
      Return->iInt=1;
      Exit(Param[0].iPtr)->eWorld=Param[1].iPtr;
    }
}

FNPROC(FnExitGetFlagBit) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    Return->iInt=(Exit(Param[0].iPtr)->eFlag)&Param[1].iInt;
  }
}

FNPROC(FnExitSetFlagBit) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    Return->iInt=(Exit(Param[0].iPtr)->eFlag)&Param[1].iInt;
    BITSET(Exit(Param[0].iPtr)->eFlag,Param[1].iInt);
  }
}

FNPROC(FnExitClearFlagBit) {
  Return->iInt=0;
  if (Param[0].iPtr) {
    Return->iInt=(Exit(Param[0].iPtr)->eFlag)&Param[1].iInt;
    BITCLR(Exit(Param[0].iPtr)->eFlag,Param[1].iInt);
  }
}

FNPROC(FnDoorSetFlag) {
  EXIT *e;
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    Return->iInt=Exit(Param[1].iPtr)->eFlag;
    Exit(Param[1].iPtr)->eFlag=Param[2].iInt;
    e=ExitReverse(Param[0].iPtr,Param[1].iPtr);
    if (e) {
      Exit(e)->eFlag=Param[2].iInt;
    }
  }
}

FNPROC(FnDoorSetFlagBit) {
  EXIT *e;
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    Return->iInt=(Exit(Param[1].iPtr)->eFlag)&Param[2].iInt;
    BITSET(Exit(Param[1].iPtr)->eFlag,Param[2].iInt);
    e=ExitReverse(Param[0].iPtr,Param[1].iPtr);
    if (e) {
      BITSET(Exit(e)->eFlag,Param[2].iInt);
    }
  }
}

FNPROC(FnDoorClearFlagBit) {
  EXIT *e;
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    Return->iInt=(Exit(Param[1].iPtr)->eFlag)&Param[2].iInt;
    BITCLR(Exit(Param[1].iPtr)->eFlag,Param[2].iInt);
    e=ExitReverse(Param[0].iPtr,Param[1].iPtr);
    if (e) {
      BITCLR(Exit(e)->eFlag,Param[2].iInt);
    }
  }
}

FNPROC(FnDoorSetKeyObj) {
  EXIT *e;
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    Return->iInt=Exit(Param[1].iPtr)->eKeyObj;
    Exit(Param[1].iPtr)->eKeyObj=Param[2].iInt;
    e=ExitReverse(Param[0].iPtr,Param[1].iPtr);
    if (e) {
      Exit(e)->eKeyObj=Param[2].iInt;
    }
  }
}



/***********************************************************************
 *
 * Property functions
 *
 ***********************************************************************/

/* PropertySetStr - creates a property with the specified Key and Desc */
/* int PropertyCreateStr(thing,key,desc); 
 * returns 1 if successful, 0 if an error occurred. */
FNPROC(FnPropertySetStr) {
  PROPERTY *i;

  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)&&(Param[2].iPtr)) {
    if (*(Str(Param[1].iPtr)->sText)=='@')
      return;

    /* First, find & delete any properties with the same
     * key list as this new property */
    while ( (i=PropertyFind(Thing(Param[0].iPtr)->tProperty, Str(Param[1].iPtr)->sText)) ) {
      Thing(Param[0].iPtr)->tProperty = PropertyFree(Thing(Param[0].iPtr)->tProperty, i);
    }

    /* Now, let's create our "property" */
    Thing(Param[0].iPtr)->tProperty=PropertySet(Thing(Param[0].iPtr)->tProperty,
      Str(Param[1].iPtr)->sText,Str(Param[2].iPtr)->sText);
    Return->iInt=1;
  }
}

/* FnPropertySetInt - creates a property with the specified Key and Int 
 *                       as the Text.
 * int PropertySetStr(thing,key,text); 
 * returns the EXTRA of the newly created property. */
FNPROC(FnPropertySetInt) {

  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if (*(Str(Param[1].iPtr)->sText)=='@')
      return;

    PropertySetLWord(Thing(Param[0].iPtr), Str(Param[1].iPtr)->sText, Param[2].iInt);
    Return->iInt=1;
  }
}

/* PropertyExist - returns 1 (TRUE) if property exists, 0 (FALSE) otherwise.*/
/* NOTES:
  Cant find code properties, ie @ properties 
  Cant find LWORD type properties inherited from mob/obj template, ie % properties
*/
FNPROC(FnPropertyExist) {
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if (*(Str(Param[1].iPtr)->sText)=='@')
      return;
    if (PropertyFind(Thing(Param[0].iPtr)->tProperty, Str(Param[1].iPtr)->sText))
      Return->iInt=1;
  }
}

/* PropertyGetInt - Finds the property with the specified Key, and parses 
 *                  the text as an int, returning this value. Returns 0 if
 *                  the property doesn't exist. */
FNPROC(FnPropertyGetInt) {
  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if (*(Str(Param[1].iPtr)->sText)=='@')
      return;
    Return->iInt = PropertyGetLWord(Thing(Param[0].iPtr), Str(Param[1].iPtr)->sText, 0);
  }
}

/* PropertyGetStr - Finds the property with the specified Key, and returns
 *                  the Text as a STR pointer. Returns NULL if property not 
 *                  found. */
FNPROC(FnPropertyGetStr) {
  PROPERTY *i;

  Return->iPtr=NULL;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if (*(Str(Param[1].iPtr)->sText)=='@')
      return;
    i = PropertyFind(Thing(Param[0].iPtr)->tProperty, Str(Param[1].iPtr)->sText);
    if (i) {
      Return->iPtr=i->pDesc;
    }
  }
}

/* PropertyFree - Finds the specified property and Frees it. 
 *                Returns 1 when successful, 0 if the property doesn't exist */
FNPROC(FnPropertyFree) {
  PROPERTY *i;

  Return->iInt=0;
  if ((Param[0].iPtr)&&(Param[1].iPtr)) {
    if (*(Str(Param[1].iPtr)->sText)=='@')
      return;
    /* Find & delete any properties with the same
     * key list as the specified property */
    i = PropertyFind(Thing(Param[0].iPtr)->tProperty, Str(Param[1].iPtr)->sText);
    if (i) {
      Thing(Param[0].iPtr)->tProperty = PropertyFree(Thing(Param[0].iPtr)->tProperty,i);
      Return->iInt=1;
    }
  }
}

/******************* Table Declarations *****************/
FTABLETYPE fTable[]={
  /* System Functions */
  { "Version",           FnVersion,          CDT_INT,   {0}},
  { "C4Version",         FnC4Version,        CDT_INT,   {0}},
  { "Number",            FnNumber,           CDT_INT,   {CDT_INT,  CDT_INT,  0}},
  { "Dice",              FnDice,             CDT_INT,   {CDT_INT,  CDT_INT,  0}},

  /* string functions */
  { "StrCmp",            FnStrcmp,           CDT_INT,   {CDT_STR,  CDT_STR,  0}},
  { "StrICmp",           FnStricmp,          CDT_INT,   {CDT_STR,  CDT_STR,  0}},
  { "StrNCmp",           FnStrncmp,          CDT_INT,   {CDT_STR,  CDT_STR,  CDT_INT,0}},
  { "StrNICmp",          FnStrnicmp,         CDT_INT,   {CDT_STR,  CDT_STR,  CDT_INT,0}},
  { "StrLen",            FnStrlen,           CDT_INT,   {CDT_STR,  0}},
  { "StrIn",             FnStrIn,            CDT_INT,   {CDT_STR,  CDT_ETC,0}},
  { "StrAt",             FnStrAt,            CDT_INT,   {CDT_STR,  CDT_STR,  CDT_INT,0}},
  { "StrAtExact",        FnStrAtExact,       CDT_INT,   {CDT_STR,  CDT_STR,  CDT_INT,0}},
  { "StrIsCmd",          FnStrIsCmd,         CDT_INT,   {CDT_STR,  CDT_ETC,  0}},
  { "StrExact",          FnStrExact,         CDT_INT,   {CDT_STR,  CDT_STR,  0}},
  { "Hello",             FnHello,            CDT_INT,   {CDT_STR,  0}},
  { "YesNo",             FnYesNo,            CDT_INT,   {CDT_STR,  0}},
/* NOTE NOTE NOTE!!! StrFree or some equivalent function MUST NEVER be implemented! */
/* If you implement StrFree, the onus is on the C4 coder to make sure Str's etc.    */
/* are properly allocated, linked, & freed. IT'S DANGEROUS. Don't do it!            */
/* Example: suppose you did this in C4: StrFree("hello");                                */
/* You now have freed a str which was embedded in code!!! NOW YOU ARE FUCKED.       */
/* Just don't do it. Anyplace like ExitSetKey, the Str allocation and freeing       */
/* is setup by the function.c coder, NOT the C4 coder. That's the way it should be. */

  /* Send Functions */
  { "SendThing",         FnSendThing,        0,         {CDT_THING,CDT_STR,0}},
  { "SendThingStr",      FnSendThingStr,     0,         {CDT_THING,CDT_STR,CDT_ETC,0}},
  { "SendAction",        FnSendAction,       0,         {CDT_THING,CDT_THING,CDT_INT,CDT_STR,0}},
  { "SendActionStr",     FnSendActionStr,    0,         {CDT_THING,CDT_THING,CDT_INT,CDT_STR,CDT_ETC,0}},

  /* Extra functions */
  { "ExtraGetNext",      FnExtraGetNext,     CDT_EXTRA, {CDT_EXTRA,0}},
  { "ExtraGetKey",       FnExtraGetKey,      CDT_STR,   {CDT_EXTRA,0}},
  { "ExtraGetDesc",      FnExtraGetDesc,     CDT_STR,   {CDT_EXTRA,0}},
  { "ExtraFind",         FnExtraFind,        CDT_EXTRA, {CDT_THING,CDT_STR,0}},
  { "ExtraFree",         FnExtraFree,        CDT_INT,   {CDT_THING,CDT_EXTRA,0}},
  { "ExtraCreate",       FnExtraCreate,      CDT_EXTRA, {CDT_THING,CDT_STR,CDT_STR,0}},
  { "ExtraSetKey",       FnExtraSetKey,      CDT_EXTRA, {CDT_EXTRA,CDT_STR,0}},
  { "ExtraSetDesc",      FnExtraSetDesc,     CDT_EXTRA, {CDT_EXTRA,CDT_STR,0}},

  /* Thing Functions */
  { "ThingGetType",      FnThingGetType,     CDT_INT,   {CDT_THING,0}},
  { "ThingGetName",      FnThingGetName,     CDT_STR,   {CDT_THING,0}},
  { "ThingGetDesc",      FnThingGetDesc,     CDT_STR,   {CDT_THING,0}},
  { "ThingGetExtra",     FnThingGetExtra,    CDT_EXTRA, {CDT_THING,0}},
  { "ThingGetNext",      FnThingGetNext,     CDT_THING, {CDT_THING,0}},
  { "ThingGetContain",   FnThingGetContain,  CDT_THING, {CDT_THING,0}},
  { "ThingTo",           FnThingTo,          CDT_INT,   {CDT_THING,CDT_THING, 0}},
  { "ThingFind",         FnThingFind,        CDT_THING, {CDT_STR,CDT_INT,CDT_THING,CDT_INT,CDT_INT,0}},
  { "ThingGetWait",      FnThingGetWait,     CDT_INT,   {CDT_THING,0}},
  { "ThingSetWait",      FnThingSetWait,     CDT_INT,   {CDT_THING,CDT_INT, 0}},
  { "ThingGetIdleWait",  FnThingGetIdleWait, CDT_INT,   {CDT_THING,0}},
  { "ThingSetIdleWait",  FnThingSetIdleWait, CDT_INT,   {CDT_THING,CDT_INT, 0}},

  /* World Functions */
  { "WorldOf",           FnWorldOf,          CDT_THING, {CDT_INT,  0}},
  { "WorldGetVirtual",   FnWorldGetVirtual,  CDT_INT,   {CDT_THING,  0}},
  { "WorldGetFlag",      FnWorldGetFlag,     CDT_INT,   {CDT_THING,  0}},
  { "WorldSetFlag",      FnWorldSetFlag,     CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "WorldGetFlagBit",   FnWorldGetFlagBit,  CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "WorldSetFlagBit",   FnWorldSetFlagBit,  CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "WorldClearFlagBit", FnWorldClearFlagBit,CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "WorldGetType",      FnWorldGetType,     CDT_INT,   {CDT_THING,  0}},
  { "WorldSetType",      FnWorldSetType,     CDT_INT,   {CDT_THING,CDT_INT,  0}},

  /* Base Functions */
  { "BaseGetKey",        FnBaseGetKey,       CDT_STR,   {CDT_THING,0}},
  { "BaseGetLDesc",      FnBaseGetLDesc,     CDT_STR,   {CDT_THING,0}},
  { "BaseGetInside",     FnBaseGetInside,    CDT_THING, {CDT_THING,0}},
  { "BaseGetConWeight",  FnBaseGetConWeight, CDT_INT,   {CDT_THING,0}},
  { "BaseGetWeight",     FnBaseGetWeight,    CDT_INT,   {CDT_THING,0}},

  /* Object Functions */
  { "ObjectCreate",      FnObjectCreate,     CDT_THING, {CDT_THING,CDT_INT,  0}},
  { "ObjectCreateNum",   FnObjectCreateNum,  CDT_THING, {CDT_THING,CDT_INT,CDT_INT,  0}},
  { "ObjectFree",        FnObjectFree,       CDT_INT,   {CDT_THING,  0}},
  { "ObjectRemove",      FnObjectRemove,     CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "ObjectStrip",       FnObjectStrip,      CDT_INT,   {CDT_THING,CDT_INT,CDT_INT,CDT_THING,  0}},
  { "ObjectStripInv",    FnObjectStripInv,   CDT_INT,   {CDT_THING,CDT_INT,CDT_INT,CDT_THING,  0}},
  { "ObjectStripEqu",    FnObjectStripEqu,   CDT_INT,   {CDT_THING,CDT_INT,CDT_INT,CDT_THING,  0}},
  { "ObjectContain",     FnObjectContain,    CDT_THING, {CDT_THING,CDT_INT,  0}},
  { "ObjectCount",       FnObjectCount,      CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "ObjectCountInv",    FnObjectCountInv,   CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "ObjectCountEqu",    FnObjectCountEqu,   CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "ObjectBuy",         FnObjectBuy,        CDT_INT,   {CDT_THING,CDT_THING,CDT_STR,CDT_INT,CDT_ETC,  0}},
  { "ObjectSell",        FnObjectSell,       CDT_INT,   {CDT_THING,CDT_THING,CDT_STR,CDT_INT,CDT_ETC,  0}},
  { "ObjectGetVirtual",  FnObjectGetVirtual, CDT_INT,   {CDT_THING,  0}},
  { "ObjectRedeem",      FnObjectRedeem,     CDT_INT,   {CDT_THING,CDT_THING,CDT_STR,CDT_INT,CDT_INT,  0}},

  /* Character Functions */
  { "CharThingFind",     FnCharThingFind,    CDT_THING, {CDT_THING,CDT_STR,CDT_INT,CDT_THING,CDT_INT,CDT_INT,0}},
  { "CharAction",        FnCharAction,       CDT_INT,   {CDT_THING,CDT_STR,  0}},
  { "CharActionStrip",   FnCharActionStrip,  CDT_INT,   {CDT_THING,CDT_STR,  0}},
  { "CharActionStr",     FnCharActionStr,    CDT_INT,   {CDT_THING,CDT_STR,  CDT_ETC,0}},
  { "CharGetLevel",      FnCharGetLevel,     CDT_INT,   {CDT_THING,0}},
  { "CharGetHitP",       FnCharGetHitP,      CDT_INT,   {CDT_THING,0}},
  { "CharSetHitP",       FnCharSetHitP,      CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "CharGetHitPMax",    FnCharGetHitPMax,   CDT_INT,   {CDT_THING,0}},
  { "CharGetMovePMax",   FnCharGetMovePMax,  CDT_INT,   {CDT_THING,0}},
  { "CharGetPowerPMax",  FnCharGetPowerPMax, CDT_INT,   {CDT_THING,0}},
  { "CharGetMoney",      FnCharGetMoney,     CDT_INT,   {CDT_THING,0}},
  { "CharSetMoney",      FnCharSetMoney,     CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "CharGetExp",        FnCharGetExp,       CDT_INT,   {CDT_THING,0}},
  { "CharGainExp",       FnCharGainExp,      CDT_INT,   {CDT_THING,CDT_INT,  0}},
  { "CharGainExpGroup",  FnCharGainExpGroup, 0,         {CDT_THING,CDT_INT,  0}},
  { "CharPractice",      FnCharPractice,     CDT_INT,   {CDT_THING,CDT_STR,CDT_INT,CDT_INT,CDT_INT,  0}},
  { "CharGetFighting",   FnCharGetFighting,  CDT_THING, {CDT_THING,  0}}, 
  { "CharIsLeader",      FnCharIsLeader,     CDT_INT,   {CDT_THING,  0}},
  { "CharGetLeader",     FnCharGetLeader,    CDT_THING, {CDT_THING,  0}},
  { "CharGetFollower",   FnCharGetFollower,  CDT_THING, {CDT_THING,  0}},
  { "CharAddFollower",   FnCharAddFollower,  CDT_THING, {CDT_THING,CDT_THING, 0}},
  { "CharRemoveFollower",FnCharRemoveFollower,CDT_THING,{CDT_THING,  0}},

  { "PlayerWrite",       FnPlayerWrite,      0,         {CDT_THING,  0}},
  { "PlayerGetBank",     FnPlayerGetBank,    CDT_INT,   {CDT_THING,  0}},
  { "PlayerSetBank",     FnPlayerSetBank,    CDT_INT,   {CDT_THING,CDT_INT, 0}},
  { "PlayerBank",        FnPlayerBank,       CDT_INT,   {CDT_THING,CDT_THING,CDT_STR, 0}},
  /*{ "PlayerGainPractice",FnPlayerGainPractice,CDT_INT,  {CDT_THING,CDT_INT, 0}},*/
  { "PlayerGainFame",    FnPlayerGainFame,   CDT_INT,   {CDT_THING,CDT_INT, 0}},
  { "PlayerGainInfamy",  FnPlayerGainInfamy, CDT_INT,   {CDT_THING,CDT_INT, 0}},
  { "PlayerSolvedQuest", FnPlayerSolvedQuest,CDT_INT,   {CDT_THING,CDT_STR,CDT_INT,CDT_INT,CDT_STR,CDT_INT, 0}},
  { "PlayerGetRace",     FnPlayerGetRace,    CDT_INT,   {CDT_THING,  0}},

  { "FightStop",         FnFightStop,        0,         {CDT_THING,  0}},
  { "FightStart",        FnFightStart,       0,         {CDT_THING, CDT_THING, 0}},
  { "FightDamage",       FnFightDamage,      CDT_INT,   {CDT_THING, CDT_INT, CDT_INT, CDT_INT, CDT_INT, CDT_STR, 0}},
  { "FightDamageRaw",    FnFightDamageRaw,   CDT_INT,   {CDT_THING, CDT_THING, CDT_INT, 0}},
  
  { "MobileCreate",      FnMobileCreate,     CDT_THING, {CDT_THING,CDT_INT,  0}},
  { "MobileCreateNum",   FnMobileCreateNum,  CDT_THING, {CDT_THING,CDT_INT,CDT_INT,  0}},
  { "MobileFree",        FnMobileFree,       CDT_INT,   {CDT_THING,  0}},
  { "MobileGetVirtual",  FnMobileGetVirtual, CDT_INT,   {CDT_THING,  0}},

  { "PropertySetStr",    FnPropertySetStr,   CDT_INT,   {CDT_THING,CDT_STR,CDT_STR,   0}},
  { "PropertySetInt",    FnPropertySetInt,   CDT_INT,   {CDT_THING,CDT_STR,CDT_INT,   0}},
  { "PropertyExist",     FnPropertyExist,    CDT_INT,   {CDT_THING,CDT_STR,   0}},
  { "PropertyGetInt",    FnPropertyGetInt,   CDT_INT,   {CDT_THING,CDT_STR,   0}},
  { "PropertyGetStr",    FnPropertyGetStr,   CDT_STR,   {CDT_THING,CDT_STR,   0}},
  { "PropertyFree",      FnPropertyFree,     CDT_INT,   {CDT_THING,CDT_STR,   0}},

  { "ExitFind",          FnExitFind,         CDT_EXIT,  {CDT_THING,CDT_STR,   0}},
  { "ExitDir",           FnExitDir,          CDT_EXIT,  {CDT_THING,CDT_INT,   0}},
  { "ExitReverse",       FnExitReverse,      CDT_EXIT,  {CDT_THING,CDT_EXIT,  0}},
  { "ExitGetDir",        FnExitGetDir,       CDT_INT,   {CDT_EXIT,  0}},
  { "ExitSetDir",        FnExitSetDir,       CDT_INT,   {CDT_THING,CDT_EXIT,CDT_INT,  0}},
  { "ExitIsCorner",      FnExitIsCorner,     CDT_INT,   {CDT_EXIT, CDT_EXIT,  0}},
  { "ExitCreate",        FnExitCreate,       CDT_EXIT,  {CDT_THING, CDT_INT, CDT_STR, CDT_STR, CDT_INT, CDT_INT, CDT_THING, 0}},
  { "ExitFree",          FnExitFree,         CDT_INT,   {CDT_THING, CDT_EXIT,  0}},
  { "ExitGetFlag",       FnExitGetFlag,      CDT_INT,   {CDT_EXIT,  0}},
  { "ExitSetFlag",       FnExitSetFlag,      CDT_INT,   {CDT_EXIT, CDT_INT,  0}},
  { "ExitGetFlagBit",    FnExitGetFlagBit,   CDT_INT,   {CDT_EXIT, CDT_INT,  0}},
  { "ExitSetFlagBit",    FnExitSetFlagBit,   CDT_INT,   {CDT_EXIT, CDT_INT,  0}},
  { "ExitClearFlagBit",  FnExitClearFlagBit, CDT_INT,   {CDT_EXIT, CDT_INT,  0}},
  { "DoorSetFlag",       FnDoorSetFlag,      CDT_INT,   {CDT_THING, CDT_EXIT, CDT_INT,  0}},
  { "DoorSetFlagBit",    FnDoorSetFlagBit,   CDT_INT,   {CDT_THING, CDT_EXIT, CDT_INT,  0}},
  { "DoorClearFlagBit",  FnDoorClearFlagBit, CDT_INT,   {CDT_THING, CDT_EXIT, CDT_INT,  0}},
  { "ExitGetKeyObj",     FnExitGetKeyObj,    CDT_INT,   {CDT_EXIT,  0}},
  { "ExitSetKeyObj",     FnExitSetKeyObj,    CDT_INT,   {CDT_EXIT, CDT_INT,  0}},
  { "DoorSetKeyObj",     FnDoorSetKeyObj,    CDT_INT,   {CDT_THING, CDT_EXIT, CDT_INT,  0}},
  { "ExitGetKey",        FnExitGetKey,       CDT_STR,   {CDT_EXIT, 0}},
  { "ExitSetKey",        FnExitSetKey,       CDT_INT,   {CDT_EXIT, CDT_STR, 0}},
  { "ExitGetDesc",       FnExitGetDesc,      CDT_STR,   {CDT_EXIT, 0}},
  { "ExitSetDesc",       FnExitSetDesc,      CDT_INT,   {CDT_EXIT, CDT_STR, 0}},
  { "ExitGetWorld",      FnExitGetWorld,     CDT_THING, {CDT_EXIT, 0}},
  { "ExitSetWorld",      FnExitSetWorld,     CDT_INT,   {CDT_EXIT, CDT_THING, 0}},

  {  NULL,               NULL,               0,         {0}}
};


