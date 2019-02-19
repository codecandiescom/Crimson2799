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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "crimson2.h"
#include "log.h"
#include "macro.h"
#include "mem.h"
#include "queue.h"

COLORLIST ansiFGColorList[] = {
  { "DGray",  'a', '1', '0' },
  { "Black",  'A', '0', '0' },
  { "Red",    'r', '1', '1' },
  { "DRed",   'R', '0', '1' },
  { "Green",  'g', '1', '2' },
  { "DGreen", 'G', '0', '2' },
  { "Yellow", 'y', '1', '3' },
  { "Brown",  'Y', '0', '3' },
  { "Blue",   'b', '1', '4' },
  { "DBlue",  'B', '0', '4' },
  { "LPurple",'p', '1', '5' },
  { "DPurple",'P', '0', '5' },
  { "Cyan",   'c', '1', '6' },
  { "DCyan",  'C', '0', '6' },
  { "White",  'w', '1', '7' },
  { "Gray",   'W', '0', '7' },
  { "\0" }
};

COLORLIST ansiBGColorList[] = {
  { "Black",  'A', '0', '0' },
  { "DRed",   'R', '0', '1' },
  { "DGreen", 'G', '0', '2' },
  { "Brown",  'Y', '0', '3' },
  { "DBlue",  'B', '0', '4' },
  { "DPurple",'P', '0', '5' },
  { "DCyan",  'C', '0', '6' },
  { "Gray",   'W', '0', '7' },
  { "\0" }
};

COLORPREF defaultPref[COLORPREF_MAX] = {
  {'c', 'A'}, /* ^0 Names L.Cyan on Black */
  {'c', 'B'}, /* ^1 Descriptions L.Cyan on Blue */
  {'g', 'A'}, /* ^2 Object/Char LDesc - lgreen on black */
  {'g', 'A'}, /* ^3 Info text - lgreen on black*/
  {'G', 'A'}, /* ^4 Info filler - dgreen on black*/
  {'c', 'A'}, /* ^5 Info highlight - lcyan on black*/
  {'c', 'A'}, /* ^6 My Speech */
  {'w', 'A'}, /* ^7 Hit something */
  {'r', 'A'}, /* ^8 Took Damage */
  {'R', 'A'}, /* ^9 Fighting messages */
  {'p', 'A'}, /* ^: Channels */
  {'w', 'A'}, /* ^; Hint-Text */
  {'g', 'A'}, /* ^< Hint-Highlight */
  {'g', 'A'}, /* ^= ? - lgreen on black*/
  {'g', 'A'}, /* ^> ? - lgreen on black*/
  {'g', 'A'}, /* ^? ? - lgreen on black*/
  {'g', 'A'}, /* ^@ ? - lgreen on black*/
};

/* reserved for future preallocation of some Q space */
void QInit(void) {
  BYTE buf[256];

  sprintf(buf, "Current Q size [%d]\n", sizeof(Q));
  Log(LOG_BOOT, buf);
}

LWORD QGetAnsiAttrib(BYTE b, BYTE *c) {
  BYTE  intensity = 0; /* error */
  LWORD i;
  
  for (i=0; ansiFGColorList[i].cName[0]; i++)
    if (ansiFGColorList[i].cSymbol == b) {
      *c = ansiFGColorList[i].cFG;
      return ansiFGColorList[i].cIntensity;
    }

  return intensity;
}

/* Never rewind more than you have just taken off */
LWORD QRewind(Q *q, LWORD qIndex, LWORD step) {
  qIndex-=step;
  while (qIndex < 0)
    qIndex += q->qSize; /* wrap to back if necessary */
  q->qLen+=step;
  
  return qIndex;
}

LWORD QAdvance(Q *q, LWORD qIndex, LWORD step) {
  if (step>q->qLen) {
    step = q->qLen; /* How can this happen? bug in QColorTranslate */
                    /* when calcing number read maybe? */
    /* return -1; */
  }
  qIndex+=step;
  while (qIndex >= q->qSize) 
    qIndex -= q->qSize; /* wrap if necessary */
  q->qLen-=step;
    
  return qIndex;
}

LWORD QScan(Q *q, LWORD qIndex, BYTE *buf, LWORD bLen) {
  LWORD i;
  for (i=0; i<bLen-1; i++) {
    if (qIndex == q->qEnd) break;

    buf[i] = q->qText[qIndex];
    
    qIndex+=1;
    while (qIndex >= q->qSize) 
      qIndex -= q->qSize; /* wrap if necessary */
    if (buf[i]=='\n') break;
  }
  buf[i] = '\0';

  return qIndex;
}

/* returns 0 if there isnt room */
/* This is (was) pretty complete in terms of optimizing ANSI strings to be minimal,
 * the only thing it doesnt handle very well is the REVERSE business, frankly
 * I think I'm going to phase out reverse support, its really pointless when
 * you can set the background color.
 *
 * if buf is NULL, this will strip color codes.
 */
LWORD QColorTranslate(Q* q, LWORD qIndex, BYTE *buf, LWORD *bCurLen, LWORD bMaxLen, COLORPREF *colorPref) {
  BYTE  intensity  = '0';
  BYTE  fg         = '7';
  BYTE  bg         = '0'; /* q->qBG; */
  LWORD qRead = 0;
  BYTE  xlate[32];
  LWORD xlateLen = 0;
  BYTE  needSemi;
  
  if (q->qText[qIndex] != '^') return 0;
  qIndex = QAdvance(q, qIndex, 1);
  if (qIndex == -1) return 0;
  qRead++;
  intensity = QGetAnsiAttrib(q->qText[qIndex], &fg);
  if (intensity == 0) {
    switch(q->qText[qIndex]) {
    case '^': strcpy(xlate, "^"); xlateLen=1; qRead++; break;

    case '&': {
      qIndex = QAdvance(q, qIndex, 1);
      qRead++;
      /* Read foreground */
      intensity = QGetAnsiAttrib(q->qText[qIndex], &fg);
      if (intensity==0) {
        switch(q->qText[qIndex]) {
        case '\0': 
        case '\n': xlateLen=-1; qIndex = QRewind(q, qIndex, 1); break;
        } /* switch */
      }
      /* Read Background */
      qIndex = QAdvance(q, qIndex, 1);
      qRead++;
      if (!QGetAnsiAttrib(q->qText[qIndex], &bg)) {
        switch(q->qText[qIndex]) {
        case '\0': 
        case '\n': xlateLen=-1; break;
        default: qRead++;
        } /* switch */
      } else {
        qRead++;
      }
    }  break;

    case '\0': 
    case '\n': xlateLen=-1; break;

    case '0': 
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case ':': 
    case ';': 
    case '<': 
    case '=': 
    case '>': 
    case '?': 
    case '@': 
    {
      intensity = QGetAnsiAttrib(colorPref[q->qText[qIndex]-'0'].cFG, &fg);
      QGetAnsiAttrib(colorPref[q->qText[qIndex]-'0'].cBG, &bg);
      qRead++;
    }  break;

    case 'v': {
      fg = '0';
      bg = '7';
      intensity = '0';
      qRead++;
    }  break;

    case 'V': { /* End color arg */
      if (q->qBG!='0') {
        strcpy(xlate, "\033[0;37;40m\033[K"); /* clear line right of cursors bk color */
      } else
        strcpy(xlate, "\033[0m");
      q->qIntensity = '0';
      q->qFG = '7';
      q->qBG = '0';
      xlateLen=strlen(xlate);
      qRead++;
    }  break;

    default: qRead++;
    } /* switch */
  } else {
    qRead++;
  }
  
  /* build *MINIMAL* color change ANSI/VT100 string */
  /*
   * Too many problems with terms implementations of this,
   * changed to support full ansi strs for everything instead
   */
  if (!xlateLen 
    && buf
    && (q->qIntensity!=intensity || q->qFG!=fg || q->qBG!=bg)
  ) {
    xlate[0] = '\033';
    xlate[1] = '[';
    xlateLen = 2;
    needSemi = FALSE;
    if (1|| q->qIntensity!=intensity) {
      xlate[xlateLen] = intensity;
      xlateLen++;
      needSemi = TRUE;
    }
    if (1|| q->qFG!=fg || intensity=='0') {
      if (needSemi) {
        xlate[xlateLen]=';';
        xlateLen++;
      }
      xlate[xlateLen] = '3';
      xlate[xlateLen+1] = fg;
      xlateLen+=2;
      needSemi = TRUE;
    }
    if (1|| q->qBG != bg || intensity=='0' || q->qFG!=fg) {
      if (needSemi) {
        xlate[xlateLen]=';';
        xlateLen++;
      }
      xlate[xlateLen] = '4';
      xlate[xlateLen+1] = bg;
      xlateLen+=2;
    }
    xlate[xlateLen]='m';
    xlateLen += 1;
    xlate[xlateLen] = '\0';
    if (q->qBG != bg) {
      strcat(xlate, "\033[K"); /* clear line right of cursors bk color */
      xlateLen+=3;
    }
    q->qIntensity = intensity;
    q->qFG = fg;
    q->qBG = bg;
  }
  if (buf && xlateLen>0 && *bCurLen+xlateLen+1<=bMaxLen) {
    strcpy(buf+*bCurLen, xlate);
    *bCurLen+= xlateLen;
  }

  qIndex = QRewind(q, qIndex, qRead-1);
  return qRead;
}


/* create of queue of size qsize */
Q *QAlloc(LWORD qSize) {
  Q *qNew;

  MEMALLOC(qNew, Q, 512); /* allocate a whole wack of Q control structures at once */
  qNew->qText = MemAlloc(qSize, qSize<<1); /* allocate 2 queue buffers at a time */
  qNew->qText[0] = '\0';                  /* can ONLY do this because qSize is power of 2 */
  qNew->qStart = 0;
  qNew->qEnd = 0;
  qNew->qLen = 0;
  qNew->qValid = 0;
  qNew->qFG = '7';
  qNew->qBG = '0';
  qNew->qIntensity = '0';
  qNew->qSize = qSize;
  return qNew;
}



Q *QFree(Q *q) {
  q->qText = (BYTE*)MemFree( (MEM*)q->qText, q->qSize);
  MEMFREE(q, Q);
  return NULL;
}



/* make a queue bigger, for internal use really 
   called by QAppend and QInsert */
void QReSize(Q *q) {
  BYTE *tNew;
  BYTE  buf[256];
  LWORD i; 

  /* if you dont know why I allways double and use powers of two, dont play with 
     my mem management functions, you dont know enough yet.... */
  tNew = MemAlloc(q->qSize<<1, q->qSize<<2); /* only one extra big q at a time */
  if (q->qEnd >= q->qStart) { /* text string doesnt wrap */
    strcpy(tNew, q->qText + q->qStart); /* uh oh , string is wrapped past end */
  } else { /* this is pretty blecherous do something more efficient later */
    /* grab first chunk */
    for (i=q->qStart; i<q->qSize; i++)
      tNew[i - q->qStart] = q->qText[i];
    tNew[i - q->qStart] = '\0';
    /* now grab the chunk that wrapped */
    strcat(tNew, q->qText);
  }
  q->qText = (BYTE*)MemFree( (MEM*)q->qText, q->qSize);
  q->qSize <<= 1; /* Double the size of the input/output Q */

  q->qStart = 0;
  q->qEnd = strlen(tNew);
  q->qText = tNew;

  sprintf(buf, "WARNING: Q too small, resizing to %ld\n", q->qSize);
  Log(LOG_ERROR, buf);
}



/* insert an entry into the front of a queue */
void QInsert(Q *q, BYTE *str) {
  LWORD sLen;
  LWORD qLen;
  /* since Q Inserts are called only to reschedule allready processed commands, no need to
     do any processing to take out \b's etc */
  
  sLen = strlen(str);
  qLen = sLen+q->qLen;
  while (qLen > q->qSize) {
    QReSize(q); /* make it bigger if its too small */
  }
  for (sLen=sLen-1; sLen >= 0; sLen--) { /* copy the string backwards to the front of q */
    q->qStart--;
    if (q->qStart < 0)
      q->qStart = q->qSize -1; /* wrap to back if necessary */
    q->qText[q->qStart]=str[sLen]; /* copy one char of string */
    q->qLen++; /* calc new length of all q entries */
    if (str[sLen] == '\n')
      q->qValid++;
  }
}



/* append an entry onto the end of a queue */
void QAppend(Q *q, BYTE *str, BYTE mode) {
  /* process the str, ECHO as necessary (to support char by char mode if user so wishes!)
     NOTE: char by char mode will give better output BUT will clobber network, do we want?
     jeez I gotta get some documentation on telnet codes.... there must be some way to 
  switch back and forth, and put command entry field on bottom line of screen 
  Hmmm to echo will need QEcho param, put this on hold for now I think */
  
  LWORD srcOffset;
  LWORD dstOffset;
  LWORD qLen;
  
  qLen = strlen(str)+q->qLen;
  while (qLen >= q->qSize) {
    QReSize(q); /* make it bigger if its too small */
  }
  srcOffset = 0;
  dstOffset = q->qEnd;
  
  /* okay here we go */
  while(str[srcOffset] != '\0') {
    /* backspace or delete */
    if (str[srcOffset]=='\b' || str[srcOffset]==127) { 
      srcOffset++;
      if (q->qLen) { /* if there is something to delete */
        dstOffset--;
        q->qLen--;
        if (dstOffset<0)
          dstOffset=q->qSize-1;
        if (q->qText[dstOffset] == '\n') { /* whoops eating into prev command */
          dstOffset++; /* will fall thru to correct for underflow below */
          q->qLen++;
        }
      }
    } else { /* not a backspace */
      if (str[srcOffset]=='\r') { 
        q->qValid++; /* yah! a full line (or more) is in the q */
        q->qLen++;
        if (str[srcOffset+1] == '\n') { /* translate \r\n */
          q->qText[dstOffset] = '\n'; /* copy the newline */
          dstOffset++;
          srcOffset+=2; /* skip the return */
        } else {          /* translate \r */
          q->qText[dstOffset] = '\n'; /* copy the newline instead of \r*/
          dstOffset++;
          srcOffset++; 
        }
      } else if (str[srcOffset]=='\n') {
        q->qValid++;
        q->qLen++;
        if (str[srcOffset+1] == '\r') {  /* translate \n\r */
          q->qText[dstOffset] = '\n'; /* copy the newline */
          dstOffset++;
          srcOffset+=2; /* skip the return */
        } else {                       /* translate \n */
          q->qText[dstOffset] = '\n'; /* copy the char */
          dstOffset++;
          srcOffset++;
        }

      /* convert $ to $$ - prevent user input from using $ substitution */
      } else if ((mode==Q_CHECKSTR) && (str[srcOffset]=='$')) {
        qLen++; while (qLen > q->qSize) QReSize(q); /* allow room for extra char */
        q->qText[dstOffset] = '$'; /* copy the char */
        dstOffset++;
        if (dstOffset >= q->qSize)
          dstOffset = 0;
        q->qText[dstOffset] = '$'; /* copy the char */
        dstOffset++;
        srcOffset++;
        q->qLen+=2;

      } else if ((mode==Q_DONTCHECKSTR) || (isprint(str[srcOffset]))) {
        q->qText[dstOffset] = str[srcOffset]; /* copy the char */
        dstOffset++;
        srcOffset++;
        q->qLen++;
      } else { /* skip non-printable character */
        srcOffset++;
      }
    }
    /* if delete hit a newline it will fall through to here and if it wrapped will now be taken care of */
    if (dstOffset >= q->qSize)
      dstOffset = 0;
  }
  q->qText[dstOffset] = '\0'; /* reterminate the buffer */
  q->qEnd=dstOffset;
}




/* read an entry from the queue, it will be deleted in the process */
/*
 do Color translation here based on qType, user color selection is:
  ^R - dark red
  ^r - light red
  ^B - dark blue
  ^b - light blue etc 
 NOTE: this will let people EASILY embed ansi color strings all over the place
       Because of all the processing, this is quite complex, sigh, the problem
       is to get rid of the complexity I'd have to sacrifice performance in
       a big way. (ie it may look gawdawful but it actually is quite fast)
*/
void QRead(Q *q, BYTE *buf, LWORD bMaxLen, LWORD qType, COLORPREF *colorPref) { 
  /* will limit max read to bMaxLen, each queue entry is separated by a \n */
  /* will return NULL pointer if no commands are in the queue */
  LWORD  bCurLen;
  LWORD  qIndex;
  LWORD  qRead = 1;
  
  /* q Caller must check to ensure that qValid > 0 or this will choke */
  bCurLen = 0;
  buf[0]='\0';
  qIndex = q->qStart; 
  if (!colorPref) colorPref = defaultPref;
  while ( q->qLen>0 && q->qText[qIndex]!='\n' && q->qText[qIndex]!='\0' ) {
    qRead = 1;
    if (bCurLen+2 <= bMaxLen) { /* if we havent maxed out copy it into return buffer */

/* Strip color codes from the text */
      if (qType != Q_COLOR_IGNORE && q->qText[qIndex] == '^') { /* translate a color */
        if (qType == Q_COLOR_ANSI) {
          qRead = QColorTranslate(q, qIndex, buf,  &bCurLen, bMaxLen, colorPref);
        } else {
          qRead = QColorTranslate(q, qIndex, NULL, &bCurLen, bMaxLen, colorPref);
        }
        if (qRead == 0)
          bMaxLen = 0; /* buffer full stop copying */

/* not a color character */
      } else { /* Normal character */
        buf[bCurLen] = q->qText[qIndex];
        bCurLen++;
        buf[bCurLen] = '\0'; /* terminate buffer */
      }
      qIndex = QAdvance(q, qIndex, qRead);
    } else
      break; /* out of room in buffer, break out of loop */
  }
  /* if this wasnt the last command move past \n to next char */
  if (qIndex!=q->qEnd) { /* advance to start of next command */
    if (q->qText[qIndex] == '\n') {
      q->qValid--; /* one less Valid command */
      qIndex = QAdvance(q, qIndex, 1);
    } else if (q->qText[qIndex] == '\0')
      qIndex = QAdvance(q, qIndex, 1);
  }
  q->qStart = qIndex;

  buf[bCurLen] = '\0'; /* terminate buffer */

  /* this just makes things easier to debug since it tends to keep
   * the data near the start of the q->qText buffer
   */
  if (!q->qLen) QFlush(q);
}

/* empty's the queue */
void QFlush(Q *q) {
  if (!q) return;
  q->qValid=q->qLen=q->qStart=q->qEnd=0;
  (q->qText)[q->qStart]=0;
}

/* Since this is only currently used by the MOLE code, qType isn't 
 * used. This proc will read the queue WITHOUT INTERPRETATION of any
 * stuff inside, until bMaxLen, numByte, or qLen. */
void QReadByte(Q *q, BYTE *buf, LWORD bMaxLen, LWORD qType, LWORD numByte) { 
  if ((!q)||(!buf)) return;
  while(numByte && bMaxLen && q->qLen) {
    *buf=q->qText[q->qStart];
    q->qStart++;
    if (q->qStart>=q->qSize) 
      q->qStart=0;
    q->qLen--;
    numByte--;
    bMaxLen--;
    buf++;
  }
}

#ifdef BLAH
/* Read a q without interpretation */
void QReadByte(Q *q, LWORD numByte, BYTE *buf, LWORD bMaxLen) {
  LWORD qIndex;

  if (!q || !buf) return;
  qIndex = q->qStart;
  
  while (numByte>0 && bMaxLen>0 && q->qLen>0) {
    *buf = q->qText[qIndex];
    qIndex = QAdvance(q, qIndex, 1);
    buf++;
    numByte--;
    bMaxLen--;
  }
  q->qStart = qIndex;

  /* this just makes things easier to debug since it tends to keep
   * the data near the start of the q->qText buffer
   */
  if (!q->qLen) QFlush(q);
}
#endif