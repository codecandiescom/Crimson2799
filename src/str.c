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

/* str.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "code.h"

/* note the editor (edit.c) relies on this modules implementation since it
   keeps the STR structure and swaps in a new sText buffer to replace the old
   one... why do I mention this here... because if you change this you may
   have to change it too....
   */

#define STR_BLOCK_SIZE 4096

LWORD strMemTotal = 0; /* total memory used by strings - for my curiosity really */
STR  *sHashIndex[STR_HASH_SIZE]; /* the hash table */
BYTE *strNull="";


/* allocate a string and put it in hash table */
void StrInit(void){
  LWORD i;
  BYTE buf[256];

  /* YAY! memSpace is implemented */
  /* the idea is that since all the world descriptions etc wont change (for the most part)
     you can save space by concat'ing the strings onto the end of one *BIG* text buffer
     (since malloc rounds up to the nearest power of two it wastes space - and its slow) */

  sprintf(buf, "Current STR size [%d]\n", sizeof(STR));
  Log(LOG_BOOT, buf);
  for(i=0;i<STR_HASH_SIZE;i++)
    sHashIndex[i]=NULL;
}

/* behaves differently for STR_PRIVATE and STR_UNHASHED but basically: */
/* allocate a string and put it in hash table */
STR *StrAlloc(STR *str){
  STR *search;

  if (!str) return NULL;
  if (str->sNum == STR_UNHASHED) { /* create another unhashed string */
    return StrCreate(str->sText, str->sLen, DONTHASH);
  } else if (str->sNum == STR_PRIVATE) { /* find public declaration */
    search = StrGetPublic(str);
    if (!search) {
      Log(LOG_ERROR, "StrAlloc: no public struct for private header\n");
      return StrCreate("",0,HASH); /* just something to keep it alive */
    }
    str = search;
  }
  
  str->sNum++; /* increase allocated number by one and thats it */
  if (str->sNum == 1) {
    Log(LOG_ERROR, "StrAlloc: sNum==1, shouldnt be possible!\n");
  }
  if (str->sNum < 0) {
    Log(LOG_ERROR, "StrAlloc: struct sNum overflow, going down....\n");
    exit(ERROR_BADFILE);
  }
  return str;
}


/* This function duplicates a char* as a STR */
/* NOTE: For SLEN_UNKNOWN strings, Any text in the string up to the first 
   have CRLF | LFCR | CR converted to just LF, and any ~'s eliminated
*/
/* WARNING: If you pass this a BINARY value 
   you must remember that the BINARY length is assumed
   to be sLen+1 to allow room for the trailing \0
   ie a malloced block of length 256 say would be created
   using StrCreate(buffer, 255) because StrCreate will do 
   a memcpy(str->buffer, buffer, 255+1) at one point
*/
STR *StrCreate(BYTE *str, LWORD sLen, BYTE sHash){
  STR  *search;
  BYTE *sText;
  LWORD srcOffset;
  LWORD dstOffset;

  if (!str) str=strNull; /* if null pointer convert to null string instead */
  sText=str; /* faster this way - saves 1 mem ref in the conversion loop or
                it did at one point, now I have no idea what if anything it does */
  if (sLen == SLEN_UNKNOWN) {
    /* the Price to be paid for this degree of error-checking is doubling
       in string allocation time since it needs to be processed TWICE */
    /* cant use STRDUP here, its too dumb have to use something more intelligent */
    /* needs to do following:
       1. replace crlf, lfcr, cr, and lf with lf
       2. Make sure no tilda's exist, since they are reserved for string termination
       */
    /* everyone remember which goes where, the following does conversion in
      place, since "corrected" strings are allways shorter, the goofy if !=
      checks here and there also allow passing constant string declarations
      which would otherwise cause seg. faults (ie StrCreate("blah") is valid
      if it does seg. fault on a constant string that means it contains illegal
      chars ie \r or ~
     */
    srcOffset = 0;
    dstOffset = 0;
    sLen = strlen(str);
    while (srcOffset<sLen) {/* wont include \0 at end so is safe to look one char in future */
      if (str[srcOffset]=='\r') {
        if (str[srcOffset+1] == '\n') { /* translate \r\n just in case */
          sText[dstOffset] = '\n'; /* copy the newline */
          dstOffset++;
          srcOffset+=2; /* skip the return */
        } else {            /* translate \r */
          sText[dstOffset] = '\n'; /* copy the newline instead of \r*/
          dstOffset++;
          srcOffset++;
        }
      } else if (str[srcOffset]=='\n') {
        if (str[srcOffset+1] == '\r') {  /* translate \n\r */
          sText[dstOffset] = '\n'; /* copy the newline */
          dstOffset++;
          srcOffset+=2; /* skip the return */
        } else {                       /* translate \n */
           if (dstOffset != srcOffset)
         sText[dstOffset] = '\n'; /* copy the char */
       dstOffset++;
       srcOffset++;
        }
      } else if (str[srcOffset]=='~') {
        srcOffset++; /* skip over the tilda */
      } else {
        if (dstOffset != srcOffset)
          sText[dstOffset] = str[srcOffset]; /* copy the char */
        dstOffset++;
        srcOffset++;
      }
    }
    if (dstOffset != srcOffset) {
      sText[dstOffset]='\0'; /* terminate the string cuz loop just translates */
      /* Length has changed */
      sLen = dstOffset; /* strlen(sText) */
    }
  }

  /* Hash list search */
  if (sHash) {
    for (search=sHashIndex[sLen%STR_HASH_SIZE]; search; search=search->sNext) {
      if (sLen == search->sLen) {
        if (!memcmp(search->sText, str, sLen+1)) {
          search->sNum++;
          return search;
        }
      }
    }
  }

  /* note: better performance could be obtained by Insertion Sorting the list */
  /* macros will check malloc success, if failure log it, shutdown then assert it */
  MEMALLOC(search, STR, STR_BLOCK_SIZE);
  search->sLen = sLen;
  search->sText = (BYTE*) MemAlloc(sLen+1, MEM_MALLOC);
  memcpy(search->sText, sText, sLen+1);
  strMemTotal += (sLen+1);

  /* now that its created insert into hash table in MRU order */
  if (sHash) {
    search->sNum = 1;
    search->sNext = sHashIndex[sLen%STR_HASH_SIZE];
    sHashIndex[sLen%STR_HASH_SIZE] = search;
  } else {
    search->sNum=STR_UNHASHED;
  }

  return search;
}

STR *StrHashFree(STR *str){
  STR *search;

  if (str->sNum == STR_UNHASHED) return NULL;
  if (str->sNum<0) {
    Log(LOG_ERROR, "StrHashFree: AGH!, negative sNum\n");
    return NULL;
  }
/* - Happens whenever StrReHash is called *
  if (str->sNum>0) {
    printf("ERR: StrHashFree: AGH!, positive sNum\n");
  }
*/

  /* finds entry and turfs it out of the table, meant to be called by StrFree */
  /* will crash if passed a string that isnt in the hash table ! */
  if (sHashIndex[str->sLen%STR_HASH_SIZE] == str) {
    sHashIndex[str->sLen%STR_HASH_SIZE] = str->sNext;
  } else {
    /* find previous STR to str */
    for (search=sHashIndex[str->sLen%STR_HASH_SIZE]; 
         search && search->sNext!=str; 
         search=search->sNext);
    if (!search) {
      Log(LOG_ERROR, "StrHashFree: argh! no hash entry\n");
      LogPrintf(LOG_ERROR, str->sText);
      LogPrintf(LOG_ERROR, "\n");
      ((STR*)(NULL))->sText=0;
      return NULL;
    }
    search->sNext = str->sNext; /* remove str from chain */
  }
  return NULL;
}

/* return a private STR structure, StrCreate returns a public one,
   really only expected to be used by edit module.
   example: objects use public strs for their description, that way
    if you edit the parent description, all objects are updated!
   but keywords should be private, since otherwise if you edited a keyword of
   say rock then you would simultaneously be editing all other instances of it
   NOTE: Only the header is private the buffer will still point to a shared
   buffer of all identical strings (if any)
   
   Note that the private STR header block must be free'd manually, StrFree
   leaves it dangling assuming that something else (ie the editor) is relying
   on it still being there. The Editor of course will free the private header
   when it is done with it.
   */
STR *StrPrivate(STR *str){
   STR *search;

   if (!str || str->sNum == STR_PRIVATE) return str;
   MEMALLOC(search, STR, STR_BLOCK_SIZE);
   search->sNum  = STR_PRIVATE; /* indicate that this is a private STR structure */
   search->sText = str->sText;
   search->sLen  = str->sLen;
   search->sNext = NULL;

   return search;
}

/* Find Public Declaration - doubt if anything but the edit module will
   ever use PRIVATE strs but you never know I guess.
*/
STR *StrGetPublic(STR *str){
  STR *search;

  if (str->sNum == STR_PRIVATE) {
    for (search=sHashIndex[str->sLen%STR_HASH_SIZE]; search; search=search->sNext) {
      if (str->sLen == search->sLen) {
        if (!memcmp(search->sText, str->sText, str->sLen+1)) {
          return search;
        }
      }
    } 
    Log(LOG_ERROR, "StrGetPublic: couldnt find STR in hash table!\n");
  }

  /* return str otherwise */
  return str;
}

/* free a string, if instances of a string hit zero really free it - return NULL*/
STR *StrFree(STR *str){
  STR *search;

  if (!str) {
    Log(LOG_ERROR, "StrFree passed Null Pointer\n");
    return NULL;
  }

  if (!str->sNum) { /* how is this possible */
    Log(LOG_ERROR, "StrFree passed a Str->sNum==0\n");
    MEMFREE(str, STR); /* free the STR struct */
    return NULL;
  }

  if (str->sNum == STR_PRIVATE) { /* okay find the public declaration of this string */
    search = StrGetPublic(str); 
    /* didnt find it major error! - mem must be scrambled -> LOG IT! */
    if (!search) {
      Log(LOG_ERROR, "StrFree couldnt find Public declaration STR in hash table!\n");
      return NULL;
    }

    str->sText = NULL; /* Editor will free private header 
                        * It can check whether the str is still valid by examining sText */
    str = search;
  }

  if (str->sNum > 0) {
    str->sNum--;
  } 
  if (str->sNum==0 || str->sNum==STR_UNHASHED) { /* last one free up its memory */
    CodeStrFree(str);
    StrHashFree(str);
    MemFree( (MEM*)str->sText, MEM_MALLOC*(str->sLen+1)); /* free the text string */
    strMemTotal -= (str->sLen+1);
    MEMFREE(str, STR); /* free the STR struct */
  }
  return NULL; /* allways return null - I dont like dangling pointers */
}


/* Rehash a STR (sText has been modified in place, for editor mainly)*/
/* str->len *MUST* point to the old length so we can know where to remove
   from the hash table!!!! */
void StrReHash(STR *str, LWORD newLen){
  StrHashFree(str);
  strMemTotal -= (str->sLen+1);
  str->sLen = newLen;
  strMemTotal += (str->sLen+1);
  str->sNext = sHashIndex[str->sLen%STR_HASH_SIZE];
  sHashIndex[str->sLen%STR_HASH_SIZE] = str;
}


/* is string a substr match of following keylist (case insensitive) */
BYTE StrIsKey(BYTE *key, STR *keyList){
  BYTE *thisKey;
  LWORD keyLen;

  if (!key || !keyList || !keyList->sText || !key[0] || !keyList->sText[0]) 
    return FALSE;
  thisKey=keyList->sText;
  keyLen = strlen(key);
  while (*thisKey) {
    for (;*thisKey && *thisKey==' ';thisKey++); /* find first non-space character */
    if (!*thisKey) return FALSE;
    if (!STRNICMP(key, thisKey, keyLen)) return TRUE;
    for (;*thisKey && *thisKey!=' ';thisKey++); /* advance to next space */
  }
  return FALSE; /* didnt find it */
}

/* NOTE: None of the following are case sensitive */
/* is string a substr match of following str (case insensitive) */
BYTE StrAbbrev(BYTE *str, BYTE *substr){

  if (!substr || !str || !*substr || !*str) return FALSE;
  if (!STRNICMP(str, substr, strlen(substr))) return TRUE;
  return FALSE; /* didnt find it */
}

/* is str an exact case insensitive match */
BYTE StrExact(BYTE *fullstr, BYTE *str){

  if (!fullstr || !str) return FALSE;
  if (!STRICMP(fullstr, str)) return TRUE;
  return FALSE; /* didnt find it */
}


/* is key exact match of keylist (case insensitive) */
BYTE StrIsExactKey(BYTE *key, STR *keyList){
  BYTE *thisKey;
  LWORD keyLen;

  if (!key || !keyList || !keyList->sText || !key[0] || !keyList->sText[0]) 
    return FALSE;
  thisKey=keyList->sText;
  keyLen = strlen(key);
  while (*thisKey) {
    for (;*thisKey && *thisKey==' ';thisKey++); /* find first non-space character */
    if (!*thisKey) return FALSE;
    if (!STRNICMP(key, thisKey, keyLen)) {
      if (thisKey[keyLen]=='\0' || thisKey[keyLen]==' ') return TRUE;
    }
    for (;*thisKey && *thisKey!=' ';thisKey++); /* advance to next space */
  }
  return FALSE; /* didnt find it */
}

/* get one word from a string - should I smash case here? */
/* Right now I'm not */
BYTE *StrOneWord(BYTE *str, BYTE *word) {
  LWORD i;

  if (!str) return NULL;
  if (word) {
    for(i=0; str[i] != '\0' && str[i]!=' '; i++)
      word[i] = str[i];
    word[i] = '\0';
  } else {
    for(i=0; str[i] != '\0' && str[i]!=' '; i++);
  }
  /* dont re-init i to 0 here cuz we wanna keep going */
  for(; str[i] != '\0' && str[i]==' '; i++);
  return (str+i);
}

/*
 * Pretty much the same as StrOneWord but returns
 * pointer to Word so it can be used within a sprintf
 * line, also separates on either space or /
 */
BYTE *StrFirstWord(BYTE *str, BYTE *word) {
  LWORD i;

  if (!str) return NULL;
  if (word) {
    for(i=0; str[i] != '\0' && str[i]!=' ' && str[i]!='/'; i++)
      word[i] = str[i];
    word[i] = '\0';
  }
  return word;
}

/* ensure a string has a limited size */
BYTE *StrTruncate(BYTE *truncateStr, BYTE *str, LWORD maxLen) {
  LWORD i;

  for(i=0; i<maxLen-1 && str[i]!='\0'; i++)
    truncateStr[i] = str[i];
  truncateStr[i] = '\0';

  /* cutesy append */
  if (str[i]!='\0' && i>2){ /* add trailing .. if its truncated */
    truncateStr[i-1] ='.';
    truncateStr[i-2] ='.';
  }
  return truncateStr;
}

/* find a substring match */
BYTE *StrFind(BYTE *str, BYTE *strFind) {
  LWORD sLen;

  if (!str) return NULL;
  if (!strFind) return NULL;
  sLen = strlen(strFind);

  while (*str) {
    if (!STRNICMP(str, strFind, sLen)) {
      return str;
    } else {
      str += 1;
    }
  }
  return NULL;
}

/* make a string all lower case */
void StrToLower(BYTE *str) {
  LWORD sLen;
  LWORD i;

  sLen = strlen(str);
  for (i=0; i<sLen; i++)
    str[i] = tolower(str[i]);
}

/* make a string all upper case */
void StrToUpper(BYTE *str) {
  LWORD sLen;
  LWORD i;

  sLen = strlen(str);
  for (i=0; i<sLen; i++)
    str[i] = toupper(str[i]);
}

/* return # of lines in the string (equal to # of <CR>+1)*/
LWORD StrNumLine(STR *str) {
  LWORD i;
  LWORD sLen;
  LWORD numLine = 0;

  sLen = str->sLen;
  for (i=0; i<sLen; i++) {
    if (str->sText[i] == '\n') numLine++;
  }
  return numLine+1;
}

/* is the str a number */
BYTE StrIsNumber(BYTE *str) {
  LWORD i;

  if (!str) return FALSE;
  if (str[0]=='-') /* allow preceding '-' sign */
    i=1;
  else
    i=0;
  for (; str[i]; i++) {
    if (!isdigit(str[i])) return FALSE;
  }
  return TRUE;
}

/* Get rid of both receding and trailing spaces */
BYTE *StrTrim(BYTE *str) {
  LWORD i;

  if (str) {
    /* trim preceding spaces */
    while (*str==' ') str++;
    /* trim trailing spaces/CR/LF */
    for(i=strlen(str)-1;
        i>0 && (str[i]==' ' || str[i]=='\n' || str[i]=='\r');
        i--)
      str[i]='\0';
  }
  return str;
}

/* ensure all text, no punctuation etc */
BYTE StrIsAlpha(BYTE *str) {
  LWORD i;

  if (!str) return FALSE;
  for (i=0; str[i]; i++) {
    if (!isalpha(str[i])) return FALSE;
  }
  return TRUE;
}


/* ensure all text, no punctuation etc */
BYTE StrIsAlNum(BYTE *str) {
  LWORD i;

  if (!str) return FALSE;
  for (i=0; str[i]; i++) {
    if (!isalnum(str[i])) return FALSE;
  }
  return TRUE;
}

/* mud input routines translate $ into $$ to
 * prevent the users from entering escape sequences
 * when editing code properties escape sequences are
 * allowable so we need to convert back
 */
BYTE *StrRestoreEscape(BYTE *str) {
  BYTE *src;
  BYTE *dst;
  
  if (str) {
    dst = src = str;
    while (1) {
      if (*src == '$') src++; /* skip one of the $'s */
      if (src != dst) *dst = *src;
      if (!*src) break;

      src++;
      dst++;
    }
  }
  return str;
}

/* terminate string at first \n ensure there arent any \n's in the str
   once we're done */
BYTE *StrOneLine(BYTE *str) {
  LWORD i;
  if (str) {
    for (i=0; str[i]; i++) {
      if (str[i]=='\n') {
        str[i]='\0';
        break;
      }
    }
  }
  return str;
}

/* See if a str ends in pattern */
LWORD StrEnd(BYTE *str, BYTE *pattern) {
  LWORD strLen;
  LWORD patternLen;

  strLen = strlen(str);
  patternLen = strlen(pattern);
  if (strLen < patternLen) return FALSE;
  
  strLen -= patternLen;
  if (!STRICMP(str+strLen, pattern)) return TRUE;
  return FALSE;
}

LWORD StrSubToNumber(BYTE *str, LWORD digits) {
  LWORD i = 0;

  while (*str && digits) {
    i*=10;
    if (isdigit(*str))
      i+= (*str - '0');

    str+=1;
    digits-=1;
  }
  return i;
}

void StrTrimCRLF(BYTE *str) {
  LWORD sLen;

  if (!str) return;
  sLen = strlen(str)-1;
  while (sLen>=0 && (str[sLen]=='\n'||str[sLen]=='\r')) {
    str[sLen]='\0';
    sLen -= 1;
  }
}

/* make slashes or backslashes match OS */
void StrPath(BYTE *str) {
  LWORD i;

  if (!str) return;
  for(i=0;str[i];i++) {
    #ifdef WIN32
      if (str[i]=='/') str[i]='\\';
    #else
      if (str[i]=='\\') str[i]='/';
    #endif
  }
}