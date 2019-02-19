/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary. Use in whole or in part without
 * explicity permission by the author is strictly prohibited
 *
 * Current email address(es): cryogen@infoserve.net
 * Phone number: (604) 591-5295
 *
 * C4 Script Language written/copyright Cam Lesiuk 1995
 * Email: clesiuk@engr.uvic.ca
 */

/* 
 * Okay this sucker is finally starting to approach stability, it now is way kewl
 * it still maintains private str headers but only WHILE they are being edited
 * as they provide a way to determine whether the original string is turfed
 * while it is being edited, in which case it fires the user out of the editor
 * (definately hot) this means that I can add mob and obj deleting routines
 * and dont have to worry about deleting edits in progress. It also lets the
 * editor block at attempts to edit a string someone else is trying to edit!
 * and just to make sure, it bails if someone is trying to edit a public str
 * (not supported!)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "queue.h"
#include "mem.h"
#include "str.h"
#include "extra.h"
#include "property.h"
#include "thing.h"
#include "index.h"
#include "board.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "player.h"
#include "send.h"
#include "parse.h"
#include "help.h"

void EditAppend(SOCK *sock, BYTE *arg) {
  LWORD i;
  LWORD argLen;

  /* find the length of the arg */
  for(i=0;arg[i]!='\0' && arg[i]!='~';i++);
  argLen = i;
  if (arg[i]=='\0') argLen++; /* allow room for implied \n too */

  /* allow room for trailing \0 too (hence + 1)*/
  if ( (sock->sEdit.eBuf->sLen + argLen + 1) > (sock->sEdit.eSize) ) {
    SendSmartColor("^rWARNING^G Maximum buffer size exceeded - append aborted!\n", sock);
    sock->sSubMode = SUBM_EDITMENU;
    return;
  }

  /* now copy over our new ARG string into our text body 
     - wont copy trailing \0 or ~ */
  memcpy(&sock->sEdit.eBuf->sText[sock->sEdit.eBuf->sLen], arg, i);
  sock->sEdit.eBuf->sLen += i;
  if (arg[i]=='~') {
    /* stop immediately - we're done! */
    sock->sSubMode = SUBM_EDITMENU;
  } else if (BIT(sock->sEdit.ePref, EP_ONELINE)) {
    SendSmartColor("^gOne line field - append aborted!\n", sock);
    sock->sSubMode = SUBM_EDITMENU;
  } else {
    /* although there is no \n on the end of the input, it is implied */
    sock->sEdit.eBuf->sText[sock->sEdit.eBuf->sLen]='\n'; /* final terminating zero */
    sock->sEdit.eBuf->sLen++;
  }

  sock->sEdit.eBuf->sText[sock->sEdit.eBuf->sLen]='\0'; /* final terminating zero */
}

void EditStartInsert(SOCK *sock, BYTE *arg) {
  BYTE buf[256];
  LWORD start;
  LWORD i;
  LWORD curLine;

  start=1; /* note: if sscanf cant find anything it will be this by default */
  sscanf(arg,"%ld",&start);

  curLine = 1;
  for(i=0; ; i++) {
    if (curLine==start || sock->sEdit.eBuf->sText[i]=='\0') break;
    if (sock->sEdit.eBuf->sText[i]=='\n') curLine++;
  }
  sock->sEdit.eInsert = i;
  if (i == sock->sEdit.eBuf->sLen) start = curLine;

  sprintf(buf,"\n^bInsert text beginning at line %ld.^V\n",start);
  SendSmartColor(buf, sock);
}

/* More efficient redone 100% the way Cryogen likes it */
/* Old version worked much the same way search and replace does,
   which I felt was too rube goldberg... my fault I wrote the
   search and replace <sigh> */
   
/* incidentally if sEdit.eInsert = sEdit.eBuf->sLen than this 
   will actually perform an append allthough a (very) slightly 
   overly complex inefficient one
 */
void EditInsert(SOCK *sock, BYTE *arg) {
  LWORD i;
  LWORD argLen;

  /* crash protection... */
  BOUNDSET(0, sock->sEdit.eInsert, sock->sEdit.eBuf->sLen);  

  /* find the length of the arg */
  for(i=0;arg[i]!='\0' && arg[i]!='~';i++);
  argLen = i;
  if (arg[i]=='\0') argLen++; /* allow room for implied \n too */

  /* allow room for trailing \0 too (hence +1)*/
  if ( (sock->sEdit.eBuf->sLen + argLen + 1) > (sock->sEdit.eSize) ) {
    SendSmartColor("^rWARNING^G Maximum buffer size exceeded - insert aborted!\n", sock);
    sock->sSubMode = SUBM_EDITMENU;
    return;
  }

  /* move text to allow for insert, including terminating \0 */
  memmove(&sock->sEdit.eBuf->sText[sock->sEdit.eInsert+argLen], 
          &sock->sEdit.eBuf->sText[sock->sEdit.eInsert], 
          sock->sEdit.eBuf->sLen - sock->sEdit.eInsert + 1);
          
  /* now copy over our new ARG string into our text body - wont copy trailing \0 or ~ */
  memcpy(&sock->sEdit.eBuf->sText[sock->sEdit.eInsert], arg, argLen);
  /* Correct the offsets */
  sock->sEdit.eInsert    += argLen;
  sock->sEdit.eBuf->sLen += argLen;

  if (arg[i]=='~') {
    /* stop immediately - we're done! */
    sock->sSubMode = SUBM_EDITMENU;
  } else if (BIT(sock->sEdit.ePref, EP_ONELINE)) {
    SendSmartColor("^gOne line field - insert aborted!\n", sock);
    sock->sSubMode = SUBM_EDITMENU;
  } else {
    /* although there is no \n on the end of the input, it is implied */
    sock->sEdit.eBuf->sText[sock->sEdit.eInsert-1]='\n'; 
  }

}

/* Joins a line to the next line */
void EditJoin(SOCK *sock, BYTE *arg) {
  BYTE  buf[256];
  LWORD i;
  LWORD start;
  LWORD curLine;

  start=1; /* note: if sscanf cant find anything it will be this by default */
  sscanf(arg,"%ld",&start);

  curLine = 0;
  for(i=0; sock->sEdit.eBuf->sText[i]; i++) {
    if (sock->sEdit.eBuf->sText[i]=='\n') curLine++;
    if (curLine==start) break;
  }
  if (i == sock->sEdit.eBuf->sLen) {
    sprintf(buf,"\n^bJoin the next line to line %ld - 'cept there isnt one...^V\n",start);
    SendSmartColor(buf, sock);
    return;
  }

  sprintf(buf,"\n^bJoin the next line to line %ld.^V\n",start);
  SendSmartColor(buf, sock);

  /* crash protection... doesnt hurt but shouldnt do anything */
  BOUNDSET(0, i, sock->sEdit.eBuf->sLen);  

  /* move text overtop of \n, including terminating \0 */
  memmove(&sock->sEdit.eBuf->sText[i], 
          &sock->sEdit.eBuf->sText[i+1], 
          sock->sEdit.eBuf->sLen - i);
  
  /* correct sLen for missing LF */
  sock->sEdit.eBuf->sLen -= 1;
}

/* Delete a section of next */
void EditDelete(SOCK *sock, BYTE *arg) {
  BYTE arg2[256];
  BYTE arg3[256];
  BYTE buf[256];
  BYTE *position;
  BYTE *delpoint;
  LWORD start;
  LWORD end;
  LWORD currentline;

  arg=StrOneWord(arg, arg2);
  arg=StrOneWord(arg, arg3);

  start=0;
  end=0x0FFFFFFF;
  if (arg2[0] != '\0') {
    sscanf(arg2,"%li",&start);
    end=start;
  }
  if (arg3[0] != '\0') {
    sscanf(arg3,"%li",&end);
  }
  if (arg3[0]!='\0')
    sprintf(buf,"\n^bDeleting lines %li to %li^V\n",start,end);
  else if (arg2[0]!='\0')
    sprintf(buf,"\n^bDeleting line %li^V\n",start);
  else
    sprintf(buf,"\n^bDeleting ALL text^V\n");
  SendSmartColor(buf, sock);

  position=sock->sEdit.eBuf->sText;
  delpoint=NULL;
  currentline=0;
  while(*position) {
    currentline++;
    if (delpoint) { /* we've got our deleting start point */
      if (currentline>end) { /* we're beyond the end point - copy */
        while((*position)&&(*position!='\r')&&(*position!='\n')) {
          *delpoint=*position;
          position++;
          delpoint++;
        }
        *delpoint=*position;
        if (*position)
          delpoint++;
      } else { /* not at the end point - just read a line */
        while((*position)&&(*position!='\r')&&(*position!='\n')) position++;
      }
    } else if (currentline>=start) { /* begin delete here */
      delpoint=position;
      /* now just read a line */
      while((*position)&&(*position!='\r')&&(*position!='\n')) position++;
    } else { /* just read a line */
      while((*position)&&(*position!='\r')&&(*position!='\n')) position++;
    }
    if (*position)
      position++;
  } /* while */
  if (delpoint)
    *delpoint='\0';

  /* correct sLen */
  sock->sEdit.eBuf->sLen = strlen(sock->sEdit.eBuf->sText);
}



void EditReplaceFind(SOCK *sock, BYTE *arg) {
  LWORD i;
  LWORD sMax;
  LWORD editMaxSize;
  BYTE *sText;
  LWORD sLen;
  BYTE found;
  
  sText = sock->sEdit.eBuf->sText + sock->sEdit.eBuf->sLen+1;
  sLen = strlen(sText);
  editMaxSize = sock->sEdit.eBuf->sLen; /* cant search for a longer str than this */
  for (i=0; 
       (sLen < editMaxSize) && (arg[i] != '\0') && (arg[i] != '~'); 
       i++) {
    sText[sLen] = arg[i];
    sLen++;  
  }
  /* put return on end of line if necessary */
  if ((sLen < editMaxSize) && (arg[i] == '\0')) {
    sText[sLen] = '\n';
    sLen++;  
  }
  sText[sLen] = '\0'; /* make sure its terminated */
  
  if (!(sLen < editMaxSize))  {
    SendSmartColor("^rWARNING^G Maximum size exceeded aborting replace!\n", sock);
    sock->sSubMode = SUBM_EDITMENU;
  }
  if (arg[i] == '~') {/* done editing */
    /* confirm existence of search string here */
    sMax = sock->sEdit.eBuf->sLen - sLen;
    found = FALSE;
    for (i=0; i < sMax; i++) {
      if (!strncmp(sText, sock->sEdit.eBuf->sText+i, sLen)) {
        found = TRUE;
        break;
      }
    }
    
    if (found) {
      SendSmartColor("^bEnter Text to replace with, ~ to exit^V\n", sock);
      sock->sEdit.eBuf->sLen += 1 + strlen(sText);
      sock->sEdit.eBuf->sText[sock->sEdit.eBuf->sLen+1] = '\0';
      sock->sSubMode = SUBM_EDITREPLACEWITH;
    } else {
      SendSmartColor("^bERROR! Couldnt find string to be replaced^V\n", sock);
      sock->sSubMode = SUBM_EDITMENU;
    }
  }
}



/* arg... the picture so far 
 *  
 *  <original text>\0<text to be replaced>\0<text being captured currently>\0
 *  |_____________|
 *  | sLenOrig
 *  |____________________________________|
 *     sock->sEdit.eBuf->sLen
 *                                          |_____________________________|
 *                                          sLen
 *  
 */
void EditReplaceWith(SOCK *sock, BYTE *arg) {
  LWORD i;
  LWORD editMaxSize;
  BYTE *sTextOrig; /* what we're currently capturing */
  BYTE *sTextFind; /* what we're currently capturing */
  BYTE *sTextRepl; /* what we're currently capturing */
  LWORD sLenOrig;
  LWORD sLenFind;
  LWORD sLenRepl;
  LWORD sMax;
  BYTE *newText;
  
  sTextOrig = sock->sEdit.eBuf->sText;
  sLenOrig  = strlen(sTextOrig);

  sTextFind = sock->sEdit.eBuf->sText+sLenOrig+1;
  sLenFind  = sock->sEdit.eBuf->sLen-1-sLenOrig;

  sTextRepl = sock->sEdit.eBuf->sText+sock->sEdit.eBuf->sLen+1;
  sLenRepl  = strlen(sTextRepl);
  
  /* ouch this is tricky to calculate */
  editMaxSize = (sock->sEdit.eSize-1) - (sLenOrig-sLenFind);
  for (i=0; 
       (sLenRepl < editMaxSize) && (arg[i] != '\0') && (arg[i] != '~'); 
       i++) {
    sTextRepl[sLenRepl] = arg[i];
    sLenRepl++;  
  }
  /* put return on end of line if necessary */
  if ((sLenRepl < editMaxSize) && (arg[i] == '\0')) {
    sTextRepl[sLenRepl] = '\n';
    sLenRepl++;  
  }
  sTextRepl[sLenRepl] = '\0'; /* make sure its terminated */
  
  if (!(sLenRepl < editMaxSize))  {
    SendSmartColor("^rWARNING^G Truncating Entry!\n", sock);
  }
  if ((arg[i] == '~')||!(sLenRepl < editMaxSize)) {/* done editing */
    /* shuffle everything around here */
    /* find search string (we know it exists) */
    sMax = sLenOrig - sLenFind;
    for (i=0; i < sMax; i++) {
      if (!strncmp(sTextFind, sTextOrig+i, sLenFind)) {
        break;
      }
    }

    /* sTextOrig+i is the first char to be replaced */
    MALLOC(newText, BYTE, sock->sEdit.eSize<<2); /* malloc space for the edit working buffer */
    strncpy(newText, sTextOrig, i);
    strcpy(newText+i, sTextRepl);
    strcat(newText, sTextOrig+i+sLenFind);

    FREE(sock->sEdit.eBuf->sText, sock->sEdit.eSize<<2);
    sock->sEdit.eBuf->sText = newText;

    sock->sSubMode = SUBM_EDITMENU;
    sock->sEdit.eBuf->sLen = strlen(sock->sEdit.eBuf->sText);
  }
}


/* Cams hideously hard to follow list routine, oh well at list it works */
void EditList(SOCK *sock, BYTE *arg, STR *str) {
  BYTE arg2[256];
  BYTE arg3[256];
  BYTE buf[256];
  BYTE *srcpoint;
  BYTE *dstpoint;
  BYTE c;
  LWORD start;
  LWORD end;
  LWORD currentline;

  if (!str || !str->sText) {
    SendSmartColor("\n^bCurrent Text:\n", sock);
    SendSmartColor("^wAbsolutely Nothing...^V\n", sock);
    return;
  }
  
  arg=StrOneWord(arg, arg2);
  arg=StrOneWord(arg, arg3);
  
  start=0;
  end=0x0FFFFFFF;
  if (arg2[0] != '\0') {
    sscanf(arg2,"%li",&start);
    end=start;
  } 
  if (arg3[0] != '\0') {
    sscanf(arg3,"%li",&end);
  }
  srcpoint=str->sText;
  currentline=0;
  SendSmartColor("\n^bCurrent Text:^V\n", sock);
  while(*srcpoint) {
    currentline++;
    if (currentline>end)
      break;
    sprintf(buf,"%3li> ",currentline);
    dstpoint=buf+5;
    do {
      c=*dstpoint=*srcpoint;
      if (c) {
        dstpoint++;
        srcpoint++;
      }
      if ((dstpoint-buf)>=230) { /* don't overflow our static buffer, please */
        sprintf(dstpoint,"...<line too long>\n");
        break;  
      }
    } while((c)&&(c!='\r')&&(c!='\n'));
    *dstpoint='\0';
    if (currentline>=start)
      SEND(buf,sock);
  }
  
  /* should only send this if this is actually the last line */
  if (!*srcpoint)
    SendSmartColor("^w<END>^V\n", sock);
}



/* 
 * Copy a string to the clipboard
 * This is done in too steps, first we copy the selected text
 * to the end of the edit buffer ie text\0copied text\0
 * then we make the Clipboard with a call to StrCreate
 */
void EditCopy(SOCK *sock, BYTE *arg) {
  BYTE arg2[256];
  BYTE arg3[256];
  BYTE *srcpoint;
  BYTE *dstpoint;
  BYTE c;
  LWORD start;
  LWORD end;
  LWORD currentline;

  /* Figure out what we're copying */
  arg=StrOneWord(arg, arg2);
  arg=StrOneWord(arg, arg3);
  start=0;
  end=0x0FFFFFFF;
  if (arg2[0] != '\0') {
    sscanf(arg2,"%li",&start);
    end=start;
  } 
  if (arg3[0] != '\0') {
    sscanf(arg3,"%li",&end);
  }

  /* init pointers and dst buffer */  
  srcpoint=sock->sEdit.eBuf->sText;
  dstpoint=sock->sEdit.eBuf->sText + sock->sEdit.eBuf->sLen + 1;
  *dstpoint = '\0';

  currentline=0;
  while(*srcpoint) {
    currentline++;
    if (currentline>end)
      break;
    do {
      c=*srcpoint;
      if (c) {
        if (currentline>=start) {
          *dstpoint=*srcpoint;
          dstpoint++;
        }
        srcpoint++;
      }
    } while((c)&&(c!='\r')&&(c!='\n')); /* will never have a \r actually */
    *dstpoint='\0';
  }
  
  if (sock->sEdit.eClipBoard) STRFREE(sock->sEdit.eClipBoard);
  sock->sEdit.eClipBoard = STRCREATE(sock->sEdit.eBuf->sText + sock->sEdit.eBuf->sLen + 1);
  SendSmartColor("^wText Copied to Clipboard^V\n", sock);
}

/* Paste the clipboard before a given line number */
void EditPaste(SOCK *sock, BYTE *arg) {
  BYTE buf[256];
  LWORD start;
  LWORD i;
  LWORD curLine;
  LWORD argLen;

  if (!sock->sEdit.eClipBoard) {
    SendSmartColor("^rError^G Clipboard is empty - paste aborted!\n", sock);
    return;
  }

  start=1; /* note: if sscanf cant find anything it will be this by default */
  sscanf(arg,"%ld",&start);

  curLine = 1;
  for(i=0; ; i++) {
    if (curLine==start || sock->sEdit.eBuf->sText[i]=='\0') break;
    if (sock->sEdit.eBuf->sText[i]=='\n') curLine++;
  }
  if (i == sock->sEdit.eBuf->sLen) start = curLine;

  /* Get ready to Paste text to sock->sEdit.eBuf->sText+i */
  sprintf(buf,"\n^Text pasted - beginning at line %ld.^V\n",start);
  SendSmartColor(buf, sock);

  /* store this for easier reference later */  
  argLen = sock->sEdit.eClipBoard->sLen;
  
  /* allow room for trailing \0 too (hence +1)*/
  if ( (sock->sEdit.eBuf->sLen + argLen + 1) > (sock->sEdit.eSize) ) {
    SendSmartColor("^rWARNING^G Maximum buffer size exceeded - paste aborted!\n", sock);
    sock->sSubMode = SUBM_EDITMENU;
    return;
  }

  /* move text to allow for insert, including terminating \0 */
  memmove(&sock->sEdit.eBuf->sText[i+argLen], 
          &sock->sEdit.eBuf->sText[i], 
          sock->sEdit.eBuf->sLen - i + 1);
          
  /* now copy over our new ARG string into our text body - wont copy trailing \0 or ~ */
  memcpy(&sock->sEdit.eBuf->sText[i], arg, argLen);

  /* Correct the length */
  sock->sEdit.eBuf->sLen += argLen;
}


void EditReformat(SOCK *sock) {
  #define MIN_LINE_LEN 65 /* hypenate if line isnt at least this long */

  LWORD srcOffset=0;
  LWORD dstOffset=0;
  LWORD curLineLen=0;
  BYTE *newText = NULL;
  BYTE *oldText = NULL;

  SendSmartColor("^GReformatting so each line is as close to 75 characters as possible!\n", sock);
  SendSmartColor("(Lines starting with a space will not be affected, so indent paragraphs)\n", sock);

  /* setup for whats to come */
  MALLOC(newText, BYTE, sock->sEdit.eSize<<2); /* malloc space for the edit working buffer */
  oldText = sock->sEdit.eBuf->sText;

  do {
    if (curLineLen > 79) {
      /* okay here we go, backtrack till we get a decent line length */
      /* oh and we *KNOW* that there is at least 79 chars between us and
         last return that was kept */
      while (curLineLen>MIN_LINE_LEN && oldText[srcOffset] != ' ') {
        srcOffset--;
        if (oldText[srcOffset] == '\n')
          srcOffset--;
        dstOffset--;
      }

      /* okay we found start of last word or we need to hyphenate */
      if (curLineLen<=MIN_LINE_LEN) { /* oh oh gotta hyphenate */
        /* go to where we will plunk down the hyphen */
        while (curLineLen < 78) { 
          srcOffset++;
          if (oldText[srcOffset] == '\n')
            srcOffset++;
          dstOffset++;
        }
        /* hyphen it */
        newText[dstOffset] = '-';
        dstOffset++;
        newText[dstOffset] = '\n';
        dstOffset++;
        curLineLen = 0;
      } else { /* return replaces the space */
        newText[dstOffset] = '\n';
        dstOffset++;         
        srcOffset++;
        curLineLen = 0;
      }

    /* no need to wrap yet, keep copying chars */
    } else {
      if (oldText[srcOffset] == '\n') {
        if ((oldText[srcOffset+1] == '\n') 
            || (oldText[srcOffset+1] == '\0')
            || (oldText[srcOffset+1] == ' ')
        ){ /* keep the return */
          newText[dstOffset] = oldText[srcOffset];
          dstOffset++;         
          srcOffset++;
          curLineLen = 0;
        } else {
          srcOffset++; /* skip over return */
        }
      } else {
        newText[dstOffset] = oldText[srcOffset];
        dstOffset++;         
        srcOffset++;
        curLineLen++;
      }
    }
  } while (oldText[srcOffset]);
  newText[dstOffset] = '\0';

  /* make new buffer the real thing */
  FREE(sock->sEdit.eBuf->sText, sock->sEdit.eSize<<2);
  sock->sEdit.eBuf->sText = newText;
  sock->sEdit.eBuf->sLen = strlen(sock->sEdit.eBuf->sText);
}

void EditCancel(SOCK *sock) {
  if (!sock->sEdit.eStr) return;
  BITSET(sock->sFlag, SF_XLATE);
  SendSmartColor("^bCancelling all changes and exiting editor\n", sock);
  if (sock->sEdit.eStr->sNum == STR_PRIVATE) { /* private str */
    if (sock->sEdit.eStr->sText) {
      /* Restore public header */
      *sock->sEdit.eStrP = StrGetPublic(sock->sEdit.eStr);
      /* Free private header */
      MEMFREE(sock->sEdit.eStr, STR);
    }
  }
  /* free working area */
  if (sock->sEdit.eBuf) {
    FREE(sock->sEdit.eBuf->sText, sock->sEdit.eSize<<2);
    MEMFREE(sock->sEdit.eBuf, STR);
  }

  /* set things back to zero */
  sock->sSubMode  = SUBM_NONE;
  sock->sEdit.eStr  = NULL;
  sock->sEdit.eStrP = NULL;
  sock->sEdit.eFlag = NULL; /* make sure we dont set flag again */
  sock->sEdit.eBit  = 0; 
}

/* Free up clipboard when they leave the mud server */
void EditFree(SOCK *sock) {
  EditCancel(sock);
  if (sock->sEdit.eClipBoard) STRFREE(sock->sEdit.eClipBoard);
}

/* note will strfree sEdit.eStr, Then create a new STR entry with the post
  editing str text. NOTE that by examining sEdit.eStr->sText we can determine
  whether the str we were editing has been FREE'd
*/
void EditSave(SOCK *sock) {
  STR   *str;
  BYTE  *sText;
  LWORD  i;

  BITSET(sock->sFlag, SF_XLATE); /* ensure xlation is on */
  sText=sock->sEdit.eBuf->sText;

  /* Extra validation check just to be sure */
  i = strlen(sText);
  if (i!=sock->sEdit.eBuf->sLen) {
    SendSmartColor("^wsLen was mangled, correcting!\n", sock);
    sock->sEdit.eBuf->sLen = i;
  }

  /* ensure only one line as appropriate */
  if (BIT(sock->sEdit.ePref, EP_ONELINE)) {
    for(i=0; sText[i] && sText[i]!='\n'; i++);
    if (sText[i]=='\n') {
      sText[i+1] = '\0';
      sock->sEdit.eBuf->sLen = i+1;
    }
  }
  /* Look at sEdit.eBuf and ensure it ends with lf as appropriate */
  if (BIT(sock->sEdit.ePref, EP_ENDLF)) {
    if (sock->sEdit.eBuf->sLen==0) {
      /* leave empty */
      /* sText[0] = '\n';
      sText[1] = '\0';
      sock->sEdit.eBuf->sLen = 1; */
    } else if (sText[sock->sEdit.eBuf->sLen-1]!='\n') {
      /* append \n if there is room */
      if (sock->sEdit.eBuf->sLen+2 <= sock->sEdit.eSize) {
        sText[sock->sEdit.eBuf->sLen] = '\n';
        sText[sock->sEdit.eBuf->sLen+1] = '\0';
        sock->sEdit.eBuf->sLen += 1;
      } else {
        sText[sock->sEdit.eBuf->sLen-1] = '\n';
      }
    }
  }
  /* Look at sEdit.eBuf and ensure it doesnt end with lf as appropriate */
  if (BIT(sock->sEdit.ePref, EP_ENDNOLF)) {
    for (i=sock->sEdit.eBuf->sLen-1; i>=0 && sText[i]=='\n'; i--);
    /* i is now -1 or a non-LF character -> next char is null */
    sText[i+1] = '\0';
    sock->sEdit.eBuf->sLen = i+1;
  }

  SendSmartColor("^rSaving ^GText and leaving editor...\n", sock);
  if (sock->sEdit.eStr->sNum == STR_PRIVATE) { /* private str */
    if (sock->sEdit.eStr->sText) { /* if a parent is still around */
      StrFree(StrGetPublic(sock->sEdit.eStr));  
      str = STRCREATE(sock->sEdit.eBuf->sText);
      *sock->sEdit.eStrP = str;
      if (sock->sEdit.eFlag) /* set a flag if required */
        BITSET(*sock->sEdit.eFlag, sock->sEdit.eBit);
    } else {
      SendSmartColor("^wAGH! The STR you are editing has been destroyed, aborting...\n", sock);
    }
    MEMFREE(sock->sEdit.eStr, STR);
  } else {
    SendSmartColor("^wAGH! Trying to edit public str, aborting...\n", sock);
  }
  
  /* free working area */
  if (sock->sEdit.eBuf) {
    FREE(sock->sEdit.eBuf->sText, sock->sEdit.eSize<<2);
    MEMFREE(sock->sEdit.eBuf, STR);
  }
  /* Set things back to null */
  sock->sSubMode    = SUBM_NONE;
  sock->sEdit.eStr  = NULL;
  sock->sEdit.eStrP = NULL;
  sock->sEdit.eFlag = NULL; /* make sure we dont set flag again */
  sock->sEdit.eBit  = 0; 

  BoardIdle(); /* good place to save bulletin boards */
}

void EditSendPrompt(SOCK *sock) {
  BYTE  buf[256];
  
  switch(sock->sSubMode) {
  case SUBM_EDITMENU:
    SendSmartColor("\n^bEditing:^c[", sock);
    SEND(sock->sEdit.eName, sock);
    SendSmartColor("] ^bMax. Size: ^c", sock);
    sprintf(buf, "[%d characters]\n", sock->sEdit.eSize);
    SendSmartColor(buf, sock);
    SendSmartColor("^c[^b? for Menu^c] ^g-+>^V", sock);
    break;
    
  case SUBM_EDITAPPEND:
  case SUBM_EDITINSERT:
  case SUBM_EDITREPLACEFIND:
  case SUBM_EDITREPLACEWITH:
    SendSmartColor("^c>^V", sock);
    break;
  }
}

void EditProcess(SOCK *sock, BYTE *cmd) {
  BYTE arg1[LINE_MAX_LEN];
  BYTE *arg = cmd;
  BYTE *str;

  /* Bunch of error Checking */
  if (!sock->sEdit.eStr->sText) {
    BITSET(sock->sFlag, SF_XLATE); /* ensure xlation is on */
    SendSmartColor("^wAGH! The STR you are editing has been destroyed, aborting...\n", sock);
    EditCancel(sock);
    return;
  }
  if (sock->sEdit.eStr->sNum != STR_PRIVATE) {
    BITSET(sock->sFlag, SF_XLATE); /* ensure xlation is on */
    SendSmartColor("^wAGH! you are attempting to edit a public STR header, aborting...\n", sock);
    EditCancel(sock);
    return;
  }
  
  if (BIT(sock->sEdit.ePref, EP_ALLOWESCAPE))
    StrRestoreEscape(arg);

  switch(sock->sSubMode) {
  case SUBM_EDITMENU:
    if (HelpParse(sock->sHomeThing,arg,"editor"))
      break;

    arg=StrOneWord(arg,arg1);
    
    /* start appending text to the buffer */
    if (StrAbbrev("append", arg1)) {
      SendSmartColor("^bEnter Text to append, ~ to exit^V\n", sock);
      sock->sSubMode = SUBM_EDITAPPEND;
      /* if continuing a partial line echo what we got so far */
      if (sock->sEdit.eBuf->sText[sock->sEdit.eBuf->sLen] != '\n') {
        str=sock->sEdit.eBuf->sText + sock->sEdit.eBuf->sLen;
        while ((str > sock->sEdit.eBuf->sText) && (*(str-1) != '\n')) str--;
        SEND(str, sock);
      }
      
    /* start inserting text to the buffer */
    } else if (StrAbbrev("insert", arg1)) {
      SendSmartColor("^bEnter Text to insert, ~ to exit^V\n", sock);
      sock->sSubMode = SUBM_EDITINSERT;
      EditStartInsert(sock,arg);
      
      /* List the text */
    } else if (StrAbbrev("list", arg1)) {
      EditList(sock, arg, sock->sEdit.eBuf);
      
      /* List the text */
    } else if (StrAbbrev("clipboard", arg1)) {
      EditList(sock, arg, sock->sEdit.eClipBoard);
      
      /* Copy text to the clipboard */
    } else if (StrAbbrev("copy", arg1)) {
      EditCopy(sock, arg);
      
      /* Copy text to the clipboard */
    } else if (StrAbbrev("paste", arg1)) {
      EditPaste(sock, arg);
      
      /* delete lines of text */
    } else if (StrAbbrev("delete", arg1)) {
      EditDelete(sock, arg);
      EditList(sock, "", sock->sEdit.eBuf);

      /* join lines of text */
    } else if (StrAbbrev("join", arg1)) {
      EditJoin(sock, arg);
      EditList(sock, "", sock->sEdit.eBuf);

      /* Start a new text */
    } else if (StrAbbrev("new", arg1)) {
      sock->sEdit.eBuf->sLen=0;
      sock->sEdit.eBuf->sText[0] = '\0';
      SendSmartColor("^bEnter New Text, ~ to exit\n", sock);
      sock->sSubMode = SUBM_EDITAPPEND;

      /* start a search and replace operation */
    } else if (StrAbbrev("replace", arg1)) {
      SendSmartColor("^bEnter Text that is to be replaced, ~ to exit^V\n", sock);
      sock->sSubMode = SUBM_EDITREPLACEFIND;
      sock->sEdit.eBuf->sText[sock->sEdit.eBuf->sLen+1] = '\0';
      
      /* undo all changes */
    } else if (StrAbbrev("undo", arg1)) {
      SendSmartColor("^bReverting to pre-edited text, all changes undone\n", sock);
      sock->sEdit.eBuf->sLen=sock->sEdit.eStr->sLen;
      strcpy(sock->sEdit.eBuf->sText,sock->sEdit.eStr->sText);
      
      /* Reformat */
    } else if (StrAbbrev("format", arg1)) {
      EditReformat(sock);
      
      /* Allow say/tell/gossip etc */
    } else if (StrAbbrev("gossip", arg1)
            || StrAbbrev("tell", arg1)
            || StrAbbrev("say", arg1)
            || StrAbbrev("flee", arg1)
            || StrAbbrev("concentrate", arg1)
    ) {
      BITSET(sock->sFlag, SF_XLATE);
      ParseSock(sock, cmd);

      /* Save Changes */
    } else if (StrAbbrev("save", arg1)) {
      EditSave(sock);
      
      /* Color XLate off/on  */
    } else if (StrAbbrev("color", arg1)) {
      BITFLIP(sock->sFlag, SF_XLATE);
      SendSmartColor("Color code translation: ", sock);
      SendSmartColor(BIT(sock->sFlag, SF_XLATE) ? "On\n" : "Off\n", sock);
      
      /* Cancel */
    } else if (StrAbbrev("cance", arg1)) {
      SendSmartColor("^bType CANCEL, no less, to cancel changes and exit the editor\n", sock);
    } else if (StrAbbrev("cancel", arg1)) {
      EditCancel(sock);
    } else {
      SendSmartColor("^aAck! Ick! Urk! Unrecognized editor command.\n", sock);
    }
    break;
    
  case SUBM_EDITAPPEND:
    EditAppend(sock, arg);
    break;
  case SUBM_EDITINSERT:
    EditInsert(sock, arg);
    break;
  case SUBM_EDITREPLACEFIND:
    EditReplaceFind(sock, arg);
    break;
  case SUBM_EDITREPLACEWITH:
    EditReplaceWith(sock, arg);
    break;
  }
}

/* NOTE: You can call EditStr against NULL's it guards against this */
LWORD EditStr(SOCK *sock, STR **str, LWORD size, BYTE *name, FLAG pref) {
  BYTE buf[256];

  /* error check */
  if (!sock || !str || !name)
    return FALSE;

  /* null pointer becomes null str */
  if (!*str) {
    *str = STRCREATE("");
  }
  
  if ((*str)->sNum==STR_PRIVATE) {
    SEND("^wI'm afraid someone else is allready editing that\n", sock);
    return FALSE;
  }
  STRPRIVATE(*str);
  sock->sEdit.eStr=*str;
  sock->sEdit.eStrP=str;
  sock->sEdit.eSize=size;
  sock->sEdit.eBuf=NULL;
  strncpy(sock->sEdit.eName, name, sizeof(sock->sEdit.eName));
  sock->sEdit.eName[sizeof(sock->sEdit.eName)-1] = 0;
  sock->sEdit.eBit = 0;
  sock->sEdit.eFlag = NULL;
  sock->sEdit.ePref = pref;

  /* allocate edit buffer if hasnt allready been done */
  if (!sock->sEdit.eBuf) {
    MEMALLOC(sock->sEdit.eBuf, STR, 2048);
    MALLOC(sock->sEdit.eBuf->sText, BYTE, sock->sEdit.eSize<<2); /* malloc space for the edit working buffer */
    sock->sEdit.eBuf->sLen=sock->sEdit.eStr->sLen;               /* NOTE: its X4 because the search & replace strings get chucked into the buffer too */
    strcpy(sock->sEdit.eBuf->sText,sock->sEdit.eStr->sText);     /* worst case is 3X = 1maxlength string + 1 maxlength search + 1 maxlength replace */
  }                                                          /* Use MALLOC and not MemAlloc&MEM_MALLOC so preinited space isnt used */

  if (BIT(sock->sEdit.ePref, EP_IMMNEW)) {
    sock->sSubMode=SUBM_EDITAPPEND;
    SendSmartColor("\n^bEditing:^c[", sock);
    SEND(sock->sEdit.eName, sock);
    SendSmartColor("] ^bMax. Size: ^c", sock);
    sprintf(buf, "[%d characters]\n", sock->sEdit.eSize);
    SendSmartColor(buf, sock);
    SendSmartColor("^bEnter Text, ~ to exit^V\n", sock);
    sock->sEdit.eBuf->sLen=0;
    sock->sEdit.eBuf->sText[0] = '\0';
  } else {
    sock->sSubMode=SUBM_EDITMENU;
  }
  return TRUE;
}

/* Have the editor set this flag is they save changes */
void EditFlag(SOCK *sock, FLAG *flag, FLAG bit) {
  if (!sock || !flag || !bit) return;

  if (sock->sEdit.eStr) {
    sock->sEdit.eBit = bit;
    sock->sEdit.eFlag = flag;
  }
}

/* Edit a Property */
LWORD EditProperty(THING *thing, BYTE *commandName, BYTE *cmd, BYTE *targetName, PROPERTY **pList) {
  BYTE      buf[256];
  BYTE      strName[256];
  BYTE      argOp[256];
  PROPERTY *property;

  cmd = StrOneWord(cmd, argOp);
  if (!*argOp) { /* basic help */  
    sprintf(buf, "^GUSUAGE: ^g%s [LIST | CREATE | KEY | DESCRIPTION | DELETE] [<key>]\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s list\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s list @COMMAND\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s create @COMMAND\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s key @COMMAND\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s desc @COMMAND\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s delete @COMMAND\n", commandName);
    SendThing(buf, thing);
    return FALSE;
  }

  if (*cmd)
    property = PropertyFind(*pList, cmd);
  else 
    property = NULL;

  if (property) {
    if (StrAbbrev("create", argOp)) {
      SendThing("That keyword allready exists! Create aborted.\n", thing);
      return FALSE;
    } else if (StrAbbrev("delete", argOp)) {
      PROPERTYFREE(*pList, property);
      SendThing("Property Description deleted.\n", thing);
      return FALSE;
    } else if (StrAbbrev("list", argOp)) {
      SendThing("^cKeyList: ^g", thing);
      SendThing(property->pKey->sText, thing);
      SendThing("\n^b", thing);
      SendThing(property->pDesc->sText, thing);
      SendThing("\n", thing);
      return FALSE;
    }
  } else {
    if (StrAbbrev("list", argOp)) {
      SendThing("^wThe following properties exist:\n", thing);
      for (property=*pList; property; property=property->pNext) {
        SendThing("^cKeyList: ^g", thing);
        SendThing(property->pKey->sText, thing);
        SendThing("\n", thing);
      }
      return FALSE;
    } else if (!StrAbbrev("create", argOp)) {
      SendThing("You must create it before you can edit it!.\n", thing);
      return FALSE;
    } else {
      if (!*cmd) {
        sprintf(buf, "^wYou must specify a keyList ie %s CREATE <keylist>\n", commandName);
        SendThing(buf, thing);
        return FALSE;
      }
      *pList = PropertyCreate(*pList, STRCREATE(cmd), STRCREATE(""));
      property = *pList;
    }
  }
  
  /* Edit the extra description/key as appropiate */
  if (StrAbbrev("key", argOp)) {
    SendHint("^;HINT: Do not end the keylist with a blank line.\n", thing);
    sprintf(strName, "%s^w%s^c - Property Keylist", targetName, property->pKey->sText);
    EDITSTR(thing, property->pKey, 64, strName, EP_ONELINE|EP_ENDNOLF);
  } else {
    SendHint("^;HINT: Descriptions must end with a blank line\n", thing);
    sprintf(strName, "%s^w%s^c - Property Description", targetName, property->pKey->sText);
    EDITSTR(thing, property->pDesc, 8192, strName, EP_ENDLF|EP_ALLOWESCAPE);
  }
  return TRUE;
}

/* Edit a Extra */
LWORD EditExtra(THING *thing, BYTE *commandName, BYTE *cmd, BYTE *targetName, EXTRA **eList) {
  BYTE      buf[256];
  BYTE      strName[256];
  BYTE      argOp[256];
  BYTE      argKey[256];
  EXTRA    *extra;

  cmd = StrOneWord(cmd, argOp);
  if (!*argOp) { /* basic help */  
    sprintf(buf, "^GUSUAGE: ^g%s [LIST | CREATE | KEY | DESCRIPTION | DELETE] [<key>]\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s list\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s list @COMMAND\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s create @COMMAND\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s key @COMMAND\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s desc @COMMAND\n", commandName);
    SendThing(buf, thing);
    sprintf(buf, "^CE.G.    ^c%s delete @COMMAND\n", commandName);
    SendThing(buf, thing);
    return FALSE;
  }

  StrOneWord(cmd, argKey);
  if (*argKey)
    extra = ExtraFind(*eList, argKey);
  else 
    extra = NULL;

  if (extra) {
    if (StrAbbrev("create", argOp)) {
      SendThing("That keyword allready exists! Create aborted.\n", thing);
      return FALSE;
    } else if (StrAbbrev("delete", argOp)) {
      EXTRAFREE(*eList, extra);
      SendThing("Extra Description deleted.\n", thing);
      return FALSE;
    } else if (StrAbbrev("list", argOp)) {
      SendThing("^cKeyList: ^g", thing);
      SendThing(extra->eKey->sText, thing);
      SendThing("\n^b", thing);
      SendThing(extra->eDesc->sText, thing);
      SendThing("\n", thing);
      return FALSE;
    }
  } else {
    if (StrAbbrev("list", argOp)) {
      SendThing("^wThe following extras exist:\n", thing);
      for (extra=*eList; extra; extra=extra->eNext) {
        SendThing("^cKeyList: ^g", thing);
        SendThing(extra->eKey->sText, thing);
        SendThing("\n", thing);
      }
      return FALSE;
    } else if (!StrAbbrev("create", argOp)) {
      SendThing("You must create it before you can edit it!.\n", thing);
      return FALSE;
    } else {
      if (!*cmd) {
        sprintf(buf, "^wYou must specify a keyList ie %s CREATE <keylist>\n", commandName);
        SendThing(buf, thing);
        return FALSE;
      }
      *eList = ExtraCreate(*eList, cmd, "");
      extra = *eList;
    }
  }
  
  /* Edit the extra description/key as appropiate */
  if (StrAbbrev("key", argOp)) {
    SendHint("^;HINT: Do not end the keylist with a blank line.\n", thing);
    sprintf(strName, "%s^w%s^c - Extra Keylist", targetName, argKey);
    EDITSTR(thing, extra->eKey, 256, strName, EP_ONELINE|EP_ENDNOLF);
  } else {
    SendHint("^;HINT: Descriptions must end with a blank line\n", thing);
    sprintf(strName, "%s^w%s^c - Extra Description", targetName, argKey);
    EDITSTR(thing, extra->eDesc, 8192, strName, EP_ENDLF|EP_ALLOWESCAPE);
  }
  return TRUE;
}

LWORD EditSet(THING *thing, BYTE *cmd, void *set, BYTE *setName, SETLIST *setList) {
  BYTE         buf[512];
  BYTE         truncateStr[256];
  LWORD        i;
  LWORD       *lword;
  WORD        *word;
  BYTE        *byte;
  LWORD        oldStat = 0;
  LWORD        newStat = 0;
  PROPERTY   **property;
  PROPERTY    *p;

/*
  setList setList[] = {
    { "EXP",          SET_NUMERIC(     mobTemplate, mobTemplate.mExp )           },
    { "POSITION",     SET_TYPE(        mobTemplate, mobTemplate.mPos, posList )  },
    { "AFFECT",       SET_FLAG(        mobTemplate, mobTemplate.mAffect, affectList ) },
    { "%SPEED",       SET_PROPERTYINT( mobTemplate, mobTemplate.mProperty) },
    { "%WEAPONSDESC", SET_PROPERTYSTR( mobTemplate, mobTemplate.mProperty) },
    { "" }
  };

*/
  if (!setList) return -1;

  if (!*cmd) { /* if they didnt type anything, give 'em a list */
    SendThing("\n^gAvailable Stats:\n^G=-=-=-=-= -=-=-=-\n^c", thing);
    for (i=0; *setList[i].sName; i++) {
      sprintf(buf, "%-19s", StrTruncate(truncateStr,setList[i].sName, 19));
      SendThing(buf, thing);
      if (i%4 == 3)
        SendThing("\n", thing);
      else
        SendThing(" ", thing);
    }
    if (i%4)
      SendThing("\n", thing); /* last line allways gets a return */
    sprintf(buf, "^g%ld ^bstats listed.\n", i);
    SendThing(buf, thing);
    return 0;
  }

  if (!set) return -1;

  cmd = StrOneWord(cmd, buf); /* get the word in question */
  i = TYPEFIND(buf, setList);
  if (i == -1) {
    return -1;
  }

  cmd = StrOneWord(cmd, buf); /* what shall we set it to */
  if (setList[i].sType=='\0') {
    newStat = atol(buf);
  } else if(setList[i].sType=='I') {
    if (*buf) {
      newStat = atol(buf);
      property = (PROPERTY**)((ULWORD)set + setList[i].sOffset);
      p = PropertyFind(*property, setList[i].sName);
      if (p) sscanf(p->pDesc->sText, "%ld", &oldStat);

    } else {
      property = (PROPERTY**)((ULWORD)set + setList[i].sOffset);
      *property = PropertyDelete(*property, setList[i].sName);
      sprintf(buf, "Property %s deleted\n", setList[i].sName);
      SendThing(buf, thing);
      return 1;
    }
  } else if(setList[i].sType=='S') {
    if (!*buf) {
      property = (PROPERTY**)((ULWORD)set + setList[i].sOffset);
      *property = PropertyDelete(*property, setList[i].sName);
      sprintf(buf, "Property %s deleted\n", setList[i].sName);
      SendThing(buf, thing);
      return 1;
    }
  } else {
    newStat = TypeFind(buf, setList[i].sArray, setList[i].sArraySize);
    if (newStat == -1) {
      SendThing("^cValid Values are:\n", thing);
      SendArray(setList[i].sArray, setList[i].sArraySize, 3, thing);
      return 0;
    }
  }
  
  /* change the structure in question */
  switch(setList[i].sSize) {
  case sizeof(BYTE):
    byte = (BYTE*)((ULWORD)set + setList[i].sOffset);
    oldStat = *byte;
    if (setList[i].sType=='F') {
      BITFLIP(*byte, (1<<newStat) );
    } else {
      *byte = newStat;
    }
    newStat = *byte; /* tell them if it overflowed */
    break;
    
  case sizeof(WORD):
    word = (WORD*)((ULWORD)set + setList[i].sOffset);
    oldStat = *word;
    if (setList[i].sType=='F') {
      BITFLIP(*word, (1<<newStat) );
    } else {
      *word = newStat;
    }
    newStat = *word; /* tell them if it overflowed */
    break;
    
  case sizeof(LWORD):
    lword = (LWORD*)((ULWORD)set + setList[i].sOffset);
    oldStat = *lword;
    if (setList[i].sType=='F') {
      BITFLIP(*lword, (1<<newStat) );
    } else {
      *lword = newStat;
    }
    newStat = *lword; /* tell them if it overflowed */
    break;
    
  default:
    /* Skill/Numeric/Property */
    if (setList[i].sType=='I') {
      sprintf(buf, "%ld", newStat);
      property = (PROPERTY**)((ULWORD)set + setList[i].sOffset);
      *property = PropertySet(*property, setList[i].sName, buf);
      SendThing("^gProperty Set\n", thing);

    } else if (setList[i].sType=='S') {
      property = (PROPERTY**)((ULWORD)set + setList[i].sOffset);
      *property = PropertySet(*property, setList[i].sName, cmd);
      SendThing("^gProperty Set\n", thing);

    /* Error Checking - should never happen */
    } else {
      SendThing("I dont know how to change that stat sorry... (illegal stat size)\n", thing);
      return -1;
    }
  }

  if (!setName) return 1;  
  if (setList[i].sType=='F') {
    sprintf(buf,
      "^rYou have changed %s's stat '%s'.\n",
      setName,
      setList[i].sName);
    SendThing(buf, thing);
    SendThing("^gCurrently Set Flags: ^G[^c", thing);
    SendThing(FlagSprintf(buf, newStat, (BYTE**)setList[i].sArray, ' ', 512), thing);
    SendThing("^G]\n", thing);
  } else {
    sprintf(buf,
      "^rYou have changed %s's stat '%s' from %ld to %ld\n",
      setName,
      setList[i].sName,
      oldStat,
      newStat);
    SendThing(buf, thing);
  }
  return 1;
}


