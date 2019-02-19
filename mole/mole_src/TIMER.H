#define TM_DEFAULT_TIMER_PERIOD 10000 // initial (default) timer period (in ms)

extern UINT g_tmTimerID;
extern UINT g_tmTimerPeriod; /* period (ms) for timer event to check status of input nodes.*/
extern UINT g_tmSetTimerRC; /* Return code from SetTimer routine.*/

BOOL tmInitTimer(void);
void tmShutdownTimer(void);
UINT tmTimerAdjust(UINT p_newperiod);
