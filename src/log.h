
#define LOG_SIZE_MAX  200000

/* each log type is a "communications" channel that a god can turn on/off */
#define LOG_PORT   0 /* port binds etc */
#define LOG_ERROR  1 /* critical errors - memory issues etc */
#define LOG_BOOT   2 /* deleted at boot startup */
#define LOG_USAGE  3 /* sign-on/off, level gain, death - joe user gets this as a channel */
#define LOG_GOD    4 /* logged god commands */
#define LOG_AREA   5 /* logged area editing commands - zonelords get this as an option */

/* a typical type array, terminated by \n entry */
typedef struct logFType {
  BYTE *logFile;
  BYTE *logChannel; /* channel name */
  BYTE  logScreen;
  FILE *logDesc;
  FLAG  logChannelFlag;
} LOGF;

extern void LogInit(void);
extern void LogInitDone(void);
extern void Log(WORD logType, BYTE *logStr); /* with date/time stamp */
extern void LogPrintf(WORD logType, BYTE *logStr); /* no data/time stamp */
extern void LogStr(BYTE *logName, BYTE *logStr); /* pick your filename */
extern void LogStrPrintf(BYTE *logName, BYTE *logStr); /* no data/time stamp */

/* config variables */
extern BYTE logStrScreen; /* echo logging to console for undefined log files ie players etc */
