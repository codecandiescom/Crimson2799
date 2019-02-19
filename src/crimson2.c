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
#include <ctype.h>
#include <string.h>

#include "crimson2.h"
#include "macro.h"
#include "str.h"

/* forgive the plus/minus one stuff the problem is
   that rand only returns 0-number, whereas we are
   interested in 1-<die size> to simulate rolling a
   die
 */
LWORD Dice(LWORD dNum, LWORD dSize) {
  LWORD i;
  LWORD range;
  LWORD roll;
  LWORD total = 0;

  MINSET(dSize, 2);
  MINSET(dNum,  1);

  /* the range bit ensures a proper distribution, without it
    their is a slightly better chance of getting the highest die roll
    due to rounding error */
  range = RAND_MAX / (dSize-1); /* must not be zero here ARGH! */
  range *= (dSize-1);
  for (i=0; i<dNum; i++) {
    while ((roll = rand()) >= range) continue;
    total += roll%dSize +1;
  }
  return total;
}

/* Open ended Die rolls can keep going up, allows freak lows
  and highs (ie typically used for percentages, can allow
  for occasional highs above 100 percent or negative rolls)
  by incorporating this into the combat system, you allow
  for "critical" hits.
  */
LWORD DiceOpenEnded(LWORD dNum, LWORD dSize, LWORD oSize) {
  LWORD result;
  LWORD roll;
  LWORD window;
  LWORD oWindow;
  LWORD i;

  result = 0;
  window = dSize/20; /* initial die roll window */
  MINSET(window, 1);
  oWindow = oSize/20; /* open ended window */
  MINSET(oWindow, 1);
  
  for (i=0; i<dNum; i++) {
    roll = Number(1, dSize);
    result += roll;
    if (roll > dSize - window) { /* open ended increase */
      do {
        roll = Number(1, oSize);
        result += roll;
      } while (roll > oSize - oWindow);
    } else if (roll <= window) { /* open ended decrease */
      do {
        roll = Number(1, oSize);
        result -= roll;
      } while (roll > oSize - oWindow);
    }
  }
  return result;
}

LWORD Number(LWORD minV, LWORD maxV) {
  LWORD          range;
  LWORD          roll;
  register LWORD diff;

  MINSET(minV, 0);
  MINSET(maxV, minV+1);

  /* the range bit ensures a proper distribution, without it
    their is a slightly better chance of getting the highest die roll
    due to rounding error */
  diff = (maxV - minV)+1;
  range = RAND_MAX / diff; /* must not be zero here ARGH! */
  range *= diff;
    while ((roll = rand()) >= range) continue;
  return roll%diff+minV;
}

/* returns whether the string is a number TRUE or FALSE */
BYTE IsNumber(BYTE *str) {
  LWORD i;

  if (!str) return FALSE;
  i=0;
  if (str[i]=='-') i++;
  for (; str[i]; i++)
    if (!isdigit(str[i])) return FALSE;
  return TRUE;
}

void TimeSprintf(BYTE *str, ULWORD time) {
  LWORD day;
  LWORD hour;
  LWORD minute;
  LWORD second;

  *str = '\0';
  second = time%60;
  minute = time/60%60;
  hour   = time/3600%24;
  day    = time/86400;

  if (day>0)    sprintf(str+strlen(str), "%ld day(s), ",    day);
  if (hour>0)   sprintf(str+strlen(str), "%ld hour(s), ",   hour);
  if (minute>0) sprintf(str+strlen(str), "%ld minute(s), ", minute);
  sprintf(str+strlen(str), "%ld second(s)",   second);
}


/* ********************************************************************
NOTE: applicable to all the "Type" functions

   remember that in C when an integer is added to a pointer that the integer
   is implicitly multiplied by the size of the pointer element *SNARL*
   this has tripped me up in the past.... (whose idiot idea was this anyways)
   ie remember that pointer+integer is really pointer[integer].

  SOLUTION: Dont use $%#$#$# pointers cast to unsigned long instead.
***********************************************************************
*/

LWORD TypeFind(BYTE *tKey, ULWORD tList, LWORD tListSize) {
  ULWORD i;
  BYTE  *tEntry;

  /* To provide mud-wide inability to select !<typename> this
    function will fail to match against it) */
  if (*tKey == '!')
    return -1;

  /* find an exact match first before looking for a partial one */
  i = TypeFindExact(tKey, tList, tListSize);
  if (i!=-1) return i;

  i=0;
  tEntry = *((BYTE**)(tList+i));
  while(*tEntry != '\0') {
    if (StrAbbrev(tEntry, tKey))
      return (i/tListSize);
    i+=tListSize;
    tEntry = *((BYTE**)(tList+i));
  }
  return -1; /* not found */
}

LWORD TypeFindExact(BYTE *tKey, ULWORD tList, LWORD tListSize) {
  ULWORD i = 0;
  BYTE  *tEntry;

  /* To provide mud-wide inability to select !<typename> this
    function will fail to match against it) */
  if (*tKey == '!')
    return -1;

  tEntry = *((BYTE**)(tList+i));
  while(*tEntry != '\0') {
    if (StrExact(tEntry, tKey))
      return (i/tListSize);
    i+=tListSize;
    tEntry = *((BYTE**)(tList+i));
  }
  return -1; /* not found */
}

/* Force a type to a valid list entry */
LWORD TypeCheck(LWORD tNum, ULWORD tList, LWORD tListSize) {
  ULWORD i = 0;

  if (tNum <= 0)
    return 0;
  while(**((BYTE**)(tList+i))!='\0' && i/tListSize!=tNum) i+=tListSize;
  /* This will make out of bound values clip to end of typelist */
  if (**((BYTE**)(tList+i))=='\0' && i>0) i-=tListSize;
#ifdef TYPECHECK_ALT_BLAH
  /* out of bound values will now all be 0 */
  if (**((BYTE**)(tList+i))=='\0' && i>0) i=0;
#endif
  return i/tListSize;
}

/* Convert a string to a Type either by number or key */
LWORD TypeSscanf(BYTE *str, ULWORD tList, LWORD tListSize) {
  LWORD type;

  if (sscanf(str, " %ld", &type)>0) {
    return TypeCheck(type, tList, tListSize);
  } else {
    type = TypeFind(str, tList, tListSize);
  }
  return type;
}


/* bulletproof conversion of a type number to a type string, not the best
   performer since it has to check throught the entire typeList array since
   array size (number of elements) is generally unknown (not to be confused
   with tListSize which is the size of a single array element)

   assumes that the last element in the array is "", if it seems to be 
   going past the end of the array check to make sure you have a ,
   after the second to last entry.... (took me while to spot that one)
 */
BYTE *TypeSprintf(BYTE *str, LWORD tNum, ULWORD tList, LWORD tListSize, LWORD sMax) {
  ULWORD i;
  LWORD thisLen;
  BYTE *srcStr;
  BYTE  buf[128];

  i = 0;
  if (tNum < 0) {
    sprintf(buf, "ERROR-NEGATIVE!?");
    srcStr = buf;
  } else {
    while(**((BYTE**)(tList+i))!='\0' && i/tListSize!=tNum) {
      /* debugging - printf("%s\n", *((BYTE**)(tList+i))); */
      i+=tListSize;
    }
    if (**((BYTE**)(tList+i))!='\0') {
      srcStr = *((BYTE**)(tList+i));
    } else {
      sprintf(buf, "%ld", tNum);
      srcStr = buf;
    }
  }
  thisLen = strlen(srcStr);
  if (thisLen < sMax-1) { /* leave room for \0 */
    strcpy(str, srcStr);
  } else {
    strncpy(str, srcStr, sMax-1 );
    str[sMax-1] = '\0';
  }
  return str;
}

/* convert a single flag string to numeric value */
FLAG FlagFind(BYTE *fText, BYTE *fList[]) {
  LWORD i;
  FLAG  flag;

  if (sscanf(fText, " %ld", &flag) > 0) return flag;
  flag = 1;
  for (i=0; *fList[i] != '\0'; i++) {
    if (StrAbbrev(fList[i], fText))
      return flag;
    flag <<=1;
  }
  return 0;
}

/* convert a single flag string to numeric value */
FLAG FlagFindExact(BYTE *fText, BYTE *fList[]) {
  LWORD i;
  FLAG  flag;

  if (sscanf(fText, " %ld", &flag) > 0) return flag;
  flag = 1;
  for (i=0; *fList[i] != '\0'; i++) {
    if (StrExact(fList[i], fText))
      return flag;
    flag <<=1;
  }
  return 0;
}

/* convert a FLAG|FLAG|FLAG string to numeric total */
/* note that the fText string is destroyed in place... */
FLAG FlagSscanf(BYTE *fText, BYTE *fList[]) {
  FLAG  flag = 0;
  BYTE *str;

  str = fText+strlen(fText);
  while (str>fText) {
    for (; str>fText && *str!='|'; str-=1);
    if (*str == '|') {
      *str = '\0';
      flag += FlagFind(str+1, fList);
    }
  }
  flag += FlagFind(fText, fList); /* first one */
  return flag;
}

/* generate a FLAG|FLAG|FLAG string from the actual flag, its
   bulletproof! if a string does not exist the numeric string is used
   instead.
*/
BYTE *FlagSprintf(BYTE *str, FLAG flag, BYTE *fList[], BYTE separator, LWORD sMax) {
  LWORD sLen;
  LWORD thisLen;
  LWORD i = 0;
  FLAG  iFlag = 1;
  BYTE  numStr[128];
  BYTE *srcStr;

  str[0] = '\0'; /* ensure we can concat strings on end */
  sLen = 0;
  while (flag >= iFlag) {
    if (BIT(flag, iFlag)) {
      if (fList && *fList[i] != '\0') {
        thisLen = strlen(fList[i]);
        srcStr = fList[i];
      } else { /* outta strings, concat on the raw numbers */
        sprintf(numStr, "%ld", iFlag);
        thisLen = strlen(numStr);
        srcStr = numStr;
      }
      if (sLen > 0) { /* put in separators after first match */
        if (sLen >= sMax-1)
          return str;
        sprintf(str+sLen, "%c", separator);
        sLen += 1;
      }

      if (thisLen+sLen < sMax-1) { /* leave room for \0 */
        strcat(str, srcStr);
        sLen += thisLen;
      } else {
        strncpy(str+sLen, srcStr, (sMax - sLen) );
        str[sMax-1] = '\0';
        return str;
      }
    }

    iFlag <<= 1; /* compare the next flag */
    if (*fList[i] != '\0')
      i++; /* advance to next string if there is one */
  }
  return str;
}

/* convert a single flag to its power of two 
 *
 * ie 1<<0 = 0
 *    1<<1 = 1
 * etc
 * results for multiple flags or no flags at all are undefined
 */
LWORD Flag2Type(FLAG flag) {
  LWORD i;

  i = 0;
  while ( (1<<i) < flag)
    i++;
  return i;
}

/* convert a single flag to the number of bits that are set
 *
 * ie 1        = 1
 *    2        = 1
 *    1|2      = 2
 *    2|128    = 2
 *    4|16|128 = 3
 * etc
 */
LWORD FlagSetNum(FLAG flag) {
  LWORD i;
  LWORD result;
  LWORD bit;

  result = 0;
  bit = 1;

  for(i=0; ( bit <= flag ); i++) {
    if (BITANY(flag, bit)) result++;
    bit *=2;
  }

  return result;
}

/* 
 * designed to handle the divide by zero / negative number problem
 * havent entirely decided what the appropriate result should be for
 * divide by zero, guess another parm
 */
LWORD SafeDivide(LWORD divisee, LWORD divisor, LWORD errorResult) {
  if (divisor > 0)
    return divisee / divisor;
    
  return errorResult;
}
