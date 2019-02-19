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

#include <sys/types.h> /* for open/fcntl */
#include <sys/stat.h> /* for chmod */
#include <fcntl.h>  /* for fcntl */
#ifndef WIN32
  #include <unistd.h> /* for unlink */
#else
  #include <io.h>
#endif
#include <assert.h>
#include <time.h> /* for time, local time etc */


#include "crimson2.h"
#include "log.h"
#include "macro.h"
#include "mem.h"
#include "queue.h"
#include "str.h"
#include "extra.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "send.h"

/*  filename, channel name, echo to console, used to store file descriptor */
LOGF logF[] = {
#ifdef WIN32
  {"log\\port.log"  ,"PRT", TRUE,  0, SP_CHANPORT   },
  {"log\\error.log" ,"ERR", TRUE,  0, SP_CHANERROR  },
  {"log\\boot.log"  ,"BT!", TRUE,  0, 0             },
  {"log\\usage.log", "SYS", TRUE,  0, SP_CHANUSAGE },
  {"log\\god.log"   ,"GOD", TRUE,  0, SP_CHANGOD    },
  {"log\\area.log"  ,"AREA",TRUE,  0, SP_CHANAREA   },
#else
  {"log/port.log"   ,"PRT", TRUE,  0, SP_CHANPORT   },
  {"log/error.log"  ,"ERR", TRUE,  0, SP_CHANERROR  },
  {"log/boot.log"   ,"BT!", TRUE,  0, 0             },
  {"log/usage.log"  ,"SYS", TRUE,  0, SP_CHANUSAGE },
  {"log/god.log"    ,"GOD", TRUE,  0, SP_CHANGOD    },
  {"log/area.log"   ,"AREA",TRUE,  0, SP_CHANAREA   },
#endif
  {""               ,""   , FALSE, 0, 0             }
};
WORD logFMax = 5;

BYTE logStrScreen = TRUE;

/* turf the old boot log */
void LogInit(void) {
  LWORD       i;
  struct stat s;
  BYTE        buf[256];

  printf("Deleting old boot log\n");
  unlink(logF[LOG_BOOT].logFile);

  /* make sure they dont get too big */
  for (i=0; *logF[i].logFile; i++) {

    /* Check if statistics are valid: */
    if( !(stat( logF[i].logFile, &s )) ) {
      if (s.st_size > LOG_SIZE_MAX) {
        #ifdef WIN32
          unlink(logF[i].logFile);
          sprintf(buf, "rename %s %s", logF[i].logFile, logF[i].logFile);
        #else
          sprintf(buf, "mv %s %s", logF[i].logFile, logF[i].logFile);
        #endif
        buf[strlen(buf)-3] = 'o';
        buf[strlen(buf)-2] = 'l';
        buf[strlen(buf)-1] = 'd';
        system(buf);
      }
    }
  }
}

void LogInitDone(void) {
  Log(LOG_BOOT, "Server initialization complete, closing log/boot.log\n");
  if (logF[LOG_BOOT].logDesc) {
    fclose(logF[LOG_BOOT].logDesc);
    logF[LOG_BOOT].logDesc = NULL;
  }
}

/* two log procedures should use non-blocking writes */
void Log(WORD logType, BYTE *logStr) {
  BYTE  buf[128];
  time_t theTime;
  struct tm *theTm;

  logType = BOUNDV(0,logType,logFMax);
  if (logF[logType].logDesc == 0) {
    #ifdef WIN32
      _chmod(logF[logType].logFile, _S_IREAD | _S_IWRITE);
    #endif
    #ifndef WIN32
      chmod(logF[logType].logFile,
        S_IRUSR |
        S_IWUSR |
        S_IRGRP |
        S_IWGRP |
        S_IROTH |
        S_IWOTH
      );
    #endif
    logF[logType].logDesc = fopen(logF[logType].logFile, "a");
    if (logF[logType].logDesc == NULL) {
      PERROR("Log: Failed to Open/Create file");
      exit(ERROR_LOG);
    }
  }
  theTime = time(0);
  theTm = localtime(&theTime);
  sprintf(buf, "%02d/%02d(%02d:%02d) ", theTm->tm_mon+1, theTm->tm_mday, theTm->tm_hour, theTm->tm_min);
  if (buf[6]==' ') buf[6]='0';
  if (buf[9]==' ') buf[9]='0'; /* convert ' 1: 4' to '01:04' */
  fprintf(logF[logType].logDesc, "%s", buf);
  fprintf(logF[logType].logDesc, "%s", logStr);
 
  if(  (logType == LOG_BOOT 
     ||!logF[LOG_BOOT].logDesc)
   && logF[logType].logScreen)
    printf("%s: %s", logF[logType].logChannel, logStr);
  sprintf(buf, "%s: ", logF[logType].logChannel);
  SendChannel(buf, NULL, logF[logType].logChannelFlag);
  SendChannel(logStr, NULL, logF[logType].logChannelFlag);

  /* close the log file */
  if (logType != LOG_BOOT) {
    fclose(logF[logType].logDesc);
    logF[logType].logDesc = NULL;
  }

  if(logType != LOG_BOOT && logF[LOG_BOOT].logDesc)
    Log(LOG_BOOT, logStr);
}

void LogPrintf(WORD logType, BYTE *logStr) {
  logType = BOUNDV(0,logType,logFMax);
  if (logF[logType].logDesc == NULL) {
    #ifdef WIN32
      _chmod(logF[logType].logFile, _S_IREAD | _S_IWRITE);
    #endif
    #ifndef WIN32
      chmod(logF[logType].logFile,
        S_IRUSR |
        S_IWUSR |
        S_IRGRP |
        S_IWGRP |
        S_IROTH |
        S_IWOTH
      );
    #endif
    logF[logType].logDesc = fopen(logF[logType].logFile, "a");
    if (logF[logType].logDesc == NULL) {
      PERROR("LogPrintf");
      exit(ERROR_LOG);
    }
  }
  fprintf(logF[logType].logDesc, "%s", logStr);

  if(  (logType == LOG_BOOT 
     ||!logF[LOG_BOOT].logDesc)
   && logF[logType].logScreen)
    printf("%s", logStr);
  SendChannel(logStr, NULL, logF[logType].logChannelFlag);

  /* close the log file */
  if (logType != LOG_BOOT) {
    fclose(logF[logType].logDesc);
    logF[logType].logDesc = NULL;
  }

  if(logType != LOG_BOOT && logF[LOG_BOOT].logDesc)
    LogPrintf(LOG_BOOT, logStr);
}


void LogStr(BYTE *logName, BYTE *logStr) {
  FILE  *logFile;
  BYTE   buf[128];
  BYTE   fileName[256];
  time_t theTime;
  struct tm *theTm;

  if (logStrScreen)
    printf("%s: %s", logName, logStr);
  sprintf(fileName, "log/%s.log", logName);
  sprintf(buf, "%s: ", logName);
  SendChannel(buf, NULL, SP_CHANSPY);
  SendChannel(logStr, NULL, SP_CHANSPY);
  logFile = fopen(fileName, "a");
  if (logFile) {
    theTime = time(0);
    theTm = localtime(&theTime);
    sprintf(buf, "%02d/%02d(%02d:%02d) ", theTm->tm_mon+1, theTm->tm_mday, theTm->tm_hour, theTm->tm_min);
    if (buf[6]==' ') buf[6]='0';
    if (buf[9]==' ') buf[9]='0'; /* convert ' 1: 4' to '01:04' */
    fprintf(logFile, "%s: %s", buf, logStr);
    fclose(logFile);
  }
}

void LogStrPrintf(BYTE *logName, BYTE *logStr) {
  FILE *logFile;
  BYTE  fileName[256];

  if (logStrScreen)
    printf("%s", logStr);
  SendChannel(logStr, NULL, SP_CHANSPY);
  sprintf(fileName, "log/%s.log", logName);
  logFile = fopen(fileName, "a");
  if (logFile) {
    fprintf(logFile, "%s", logStr);
    fclose(logFile);
  }
}

