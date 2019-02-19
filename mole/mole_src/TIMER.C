// timer.c
// Two-letter Module Descriptor: tm
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

#include<windows.h>
#include"molem.h"
#include"timer.h"
#include"debug.h"

#define TM_TIMER_ID 1 // Timer ID

/* Globals */
UINT g_tmTimerID;
UINT g_tmTimerPeriod; /* period (ms) for timer event to check status of input nodes.*/
UINT g_tmSetTimerRC; /* Return code from SetTimer routine.*/

BOOL tmInitTimer() {
  g_tmTimerID=TM_TIMER_ID;
  g_tmTimerPeriod=TM_DEFAULT_TIMER_PERIOD;

  g_tmSetTimerRC=SetTimer(g_aahWnd,g_tmTimerID,g_tmTimerPeriod,NULL);

  if (!g_tmSetTimerRC) return FALSE;
  return TRUE;
}

void tmShutdownTimer() {
#ifdef WIN32
  KillTimer(NULL,g_tmSetTimerRC);
#else
  if (g_tmSetTimerRC)
    KillTimer(g_tmSetTimerRC,g_tmTimerID);
#endif
  g_tmSetTimerRC=0;
  return;
}

/* this proc re-installs the timer with the specified period. The previous
 * period is returned. */
UINT tmTimerAdjust(UINT p_newperiod) {
  UINT l_temp;

#ifdef WIN32
  KillTimer(NULL,g_tmSetTimerRC);
#else
  if (g_tmSetTimerRC)
    KillTimer(g_tmSetTimerRC,g_tmTimerID);
#endif

  l_temp=g_tmTimerPeriod;
  g_tmTimerPeriod=p_newperiod;
  g_tmSetTimerRC=SetTimer(g_aahWnd,g_tmTimerID,g_tmTimerPeriod,NULL);
  return l_temp;
}
