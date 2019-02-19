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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "crimson2.h"
#include "log.h"
#include "str.h"

/* more or less standard windows-type ini file, sans groups:
setting=value
  setting=value ; your comment here
; commented line
# commented line
setting one=blah ; invalid will be interpreted as setting=one

As with windows an INI file > 64K will cause a crash....
white space is mostly ignored
# and ; are both reserved for comments
dont use lines > 256 chars in length, as weird stuff occurs...
*/

/* result should be >= 256 char buffer */
void INIRead(BYTE *fileName, BYTE *setting, BYTE *defResult, BYTE *result) {
  FILE *iniFile;
  BYTE  iniSetting[256];
  BYTE  iniValue[256];
  BYTE  buf[256];
  
  result[0]='\0';
  iniFile = fopen(fileName, "rb");
  if (!iniFile) {
    sprintf(buf, "Unable to open INI file %s.\n", fileName);
    Log(LOG_ERROR, buf);
    return;
  }
  
  fgets(buf, 256, iniFile);
  while (!feof(iniFile)) {
    iniSetting[0] = '\0';
    iniValue[0]   = '\0';
    sscanf(buf, " %s = %s", iniSetting, iniValue);
    if (iniSetting[0] != '#' && iniSetting[0] != ';') {
      if (!STRICMP(iniSetting, setting)) {
        strcpy(result, iniValue);
        break;
      }
    }
  
    fgets(buf, 256, iniFile);
  }
  
  if (result[0] == '\0')
    strcpy(result, defResult);
  fclose(iniFile);
}

LWORD INILWordRead(BYTE *fileName, BYTE *setting, LWORD defResult) {
  FILE *iniFile;
  BYTE  iniSetting[256];
  BYTE  iniValue[256];
  LWORD result;
  BYTE  buf[256];
  
  result = defResult;
  iniFile = fopen(fileName, "rb");
  if (!iniFile) {
    sprintf(buf, "Unable to open INI file %s.\n", fileName);
    Log(LOG_ERROR, buf);
    return result;
  }
  
  fgets(buf, 256, iniFile);
  while (!feof(iniFile)) {
    iniSetting[0] = '\0';
    iniValue[0]   = '\0';
    sscanf(buf, " %s = %s", iniSetting, iniValue);
    if (iniSetting[0] != '#' && iniSetting[0] != ';') {
      if (!STRICMP(iniSetting, setting)) {
        result = atol(iniValue);
        break;
      }
    }
  
    fgets(buf, 256, iniFile);
  }
  
  fclose(iniFile);
  return result;
}


void INIWrite(BYTE *fileName, BYTE *setting, BYTE *result) {
  #define INI_MAX_SIZE 65535

  FILE       *iniFile;
  BYTE        iniSetting[256];
  LWORD       iniSettingLen;
  BYTE        iniValue[256];
  BYTE        iniBuf[INI_MAX_SIZE];  
  struct stat iniStat;
  LWORD       i;
  LWORD       iFound;
  LWORD       iMax;


  stat(fileName, &iniStat);
  if (iniStat.st_size >= INI_MAX_SIZE) {
    sprintf(iniBuf, "Unable to open INI file %s. (file to large!)\n", fileName);
    Log(LOG_ERROR, iniBuf);
    return;
  }
  
  iniFile = fopen(fileName, "rb+");
  if (!iniFile) {
    result[0]='\0';
    sprintf(iniBuf, "Unable to open INI file %s.\n", fileName);
    Log(LOG_ERROR, iniBuf);
    return;
  } 
  fread(iniBuf, iniStat.st_size, 1, iniFile);
  iniBuf[iniStat.st_size] = '\0'; /* ensure termination */
  iniSettingLen = strlen(iniSetting);
  
  /* room for setting\t= value */
  iMax = iniStat.st_size - strlen(iniSetting) - strlen(iniValue) - 3;
  iFound = -1;
  for (i=0;i<iMax;i++){
    if (!STRNICMP(iniSetting, iniBuf+i, iniSettingLen)) {
      iFound = i;
      break;
    }
  }
  for (;iniBuf[i]!='\0' && iniBuf[i]!='#' && iniBuf[i]!=';' && iniBuf[i]!='\n'; i++);
  if (iFound == -1)
    fseek(iniFile, 0, SEEK_END);
  else 
    fseek(iniFile, iFound, SEEK_SET);
  fprintf(iniFile, "\t%-15s=%s%s", iniSetting, iniValue, iniBuf+i);
  fclose(iniFile);
}
