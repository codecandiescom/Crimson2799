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


#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
  #include <sys/types.h>
  #include <sys/time.h>
  #include <unistd.h>
#else
  #include <time.h>
#endif

#include "crimson2.h"
#include "timing.h"

#ifdef WIN32
  /* including winsock.h etc auto includes the windows types */
  /* turn off warning: benign type redefinition */
  #pragma warning( disable : 4142 )
  #include <winsock.h>
  #include <mmsystem.h>
  #pragma warning( default : 4142 )
#endif



/* rather overkill but play with these to control error checking and the like */
#define TV_OVERKILL_CHECKING
#define TV_NO_NEGATIVE

/* constants */
#define USEC_PER_SEC       1000000     


/* make sure microsecond field hasnt under/over-flowed */
void TvNormalize(struct timeval *tv) {
  /* check for overflow */
  while (tv->tv_usec > USEC_PER_SEC) {
    tv->tv_sec += 1;
    tv->tv_usec -= USEC_PER_SEC;
  }
  /* check for underflow */
  while (tv->tv_usec < 0) {
    tv->tv_sec -= 1;
    tv->tv_usec += USEC_PER_SEC;
  }
  /* cant have a negative time */
  if (tv->tv_sec < 0) {
    tv->tv_sec = 0;
    tv->tv_usec = 0;
  }
}

/* Set a timeval field to sec + usec */
void TvSetConst(struct timeval *tv, LWORD sec, LWORD usec) {
  tv->tv_sec = sec;
  tv->tv_usec = usec;
#ifdef TV_OVERKILL_CHECKING
  TvNormalize(tv);
#endif
}

/* Add a constant to Tv */
void TvAddConst(struct timeval *tv, LWORD sec, LWORD usec) {
  tv->tv_sec += sec;
  tv->tv_usec += usec;
  /* check for overflow */
  while (tv->tv_usec > USEC_PER_SEC) {
    tv->tv_sec += 1;
    tv->tv_usec -= USEC_PER_SEC;
  }

#ifdef TV_OVERKILL_CHECKING
  /* check for underflow */
  while (tv->tv_usec < 0) {
    tv->tv_sec -= 1;
    tv->tv_usec += USEC_PER_SEC;
  }
#endif
#ifdef TV_NO_NEGATIVE
  /* cant have a negative time */
  if (tv->tv_sec < 0) {
    tv->tv_sec = 0;
    tv->tv_usec = 0;
  }
#endif
}

/* Subtract a constant from tv */
void TvSubConst(struct timeval *tv, LWORD sec, LWORD usec) {
  tv->tv_sec -= sec;
  tv->tv_usec -= usec;
  /* check for underflow */
  while (tv->tv_usec < 0) {
    tv->tv_sec -= 1;
    tv->tv_usec += USEC_PER_SEC;
  }

#ifdef TV_OVERKILL_CHECKING
  /* check for overflow */
  while (tv->tv_usec > USEC_PER_SEC) {
    tv->tv_sec += 1;
    tv->tv_usec -= USEC_PER_SEC;
  }
#endif
#ifdef TV_NO_NEGATIVE
  /* cant have a negative time */
  if (tv->tv_sec < 0) {
    tv->tv_sec = 0;
    tv->tv_usec = 0;
  }
#endif
}

/* This will cause a strange time return after 49 days,
 * under Win32, because timeGetTime will wrap, should
 * use time(0) to build the seconds and timeGetTime for
 * the milliseconds - Will this work?
 */
void TvGetTime(struct timeval *tv) {
#ifndef WIN32
  struct timezone tz; /* just a throwaway I dont care about timezones */

  gettimeofday(tv, &tz);
#else
  ULWORD        thisTime;
  thisTime = timeGetTime();
  tv->tv_sec = thisTime/1000;
  tv->tv_usec = thisTime%1000 * 1000;
#endif
}