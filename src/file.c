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

/* file.c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h> /* both for stat */

#include "crimson2.h"
#include "macro.h"
#include "mem.h"
#include "str.h"
#include "log.h"
#include "file.h"

BYTE fileVerbose = 1;
BYTE fileError = FALSE;

FILELIST fileList[] = {
  {"msg/greeting.msg", NULL, 0},
  {"msg/ciao.msg",     NULL, 0},
  {"msg/news.msg",     NULL, 0},
  {"msg/motd.msg",     NULL, 0},
  {"msg/quit.msg",     NULL, 0},
  {"msg/credit.msg",   NULL, 0},
  {"msg/death.msg",    NULL, 0},
  {"msg/banned.msg",   NULL, 0},
  {"msg/bannew.msg",   NULL, 0},
  {"msg/offsite.msg",  NULL, 0},
  {"",                 NULL, 0}
};

#define FILE_STR_MAX 1<<14 /* 32K character strings can be saved/read */

void FileInit(void) {
  WORD i;
  BYTE buf[256];

  Log(LOG_BOOT, "Initing Files\n");
  sprintf(buf, "Maximum file string size: %d\n", FILE_STR_MAX);
  Log(LOG_BOOT, buf);
  for (i=0; *fileList[i].fileName; i++) {
    sprintf(buf, "Reading: %s\n", fileList[i].fileName);
    Log(LOG_BOOT, buf);
    FileRead(&fileList[i]);
  }
}

void FileRead(FILELIST *fileList){ /* read in whole file as a string */
  /* stat the file, malloc a buffer equal to its length, yes MALLOC
     if we called MemAlloc, then it would be stored in a chunk of the
     initial mem space allocation (which we dont want) cuz this is just
     temporary */
  /* read the whole thing in with fread or whatever */
  /* then StrAlloc buffer - hmm possibly a special case do we want a STR or char *
     chances of multiple instances are slim to none, except editor only works
     on STR so STR it is...
   */
#define STAT_ERROR -1

  struct stat theStat;
  WORD        error;
  FILE       *theFile;
  BYTE       *fileBuf; /* no translation necessary from *UNIX* format */
  BYTE        buf[256];
  STR        *fileStr;

  fileError = FALSE;
  error = stat(fileList->fileName, &theStat);
  if (error == STAT_ERROR) {
    sprintf(buf, "WARNING! FileRead(%s): Read Error (file not found?)\n", fileList->fileName);
    Log(LOG_ERROR, buf);
    PERROR("FileRead");
    fileError = TRUE;
    return;
  }
  MALLOC(fileBuf, char, theStat.st_size+1);
  theFile=fopen(fileList->fileName, "rb");
  if (!theFile) {
    sprintf(buf, "FileRead(%s): Read Error! (file does exist though)\n", fileList->fileName);
    Log(LOG_ERROR, buf);
    PERROR("FileRead");
    fileError = TRUE;
    return;
  }
  fread(fileBuf, theStat.st_size+1, 1, theFile);
  fileBuf[theStat.st_size]='\0'; /*should have got this as EOF char of file but doesnt hurt */
  fclose(theFile);

  fileStr = STRCREATE(fileBuf);

  fileList->fileStr = fileStr;
  FREE(fileBuf, theStat.st_size+1);
  fileList->fileDate= theStat.st_mtime;
}

STR  *FileBinaryRead   (FILE *file){
  BYTE         *buf;
  BYTE          line[256];
  unsigned int  hexValue;
  BYTE          sText[FILE_STR_MAX];
  LWORD         sLen = 0;
  LWORD         i;

  fileError = FALSE;
  printf("FileBinaryRead: Untested!\n");
  while (1) {
    fgets(line, 256, file);
    if (feof(file)) {
      Log(LOG_ERROR, "FileBinaryRead(file.c): Unexpected End of File\n");
      fileError = TRUE;
      break;
    }
    
    buf = line; while(*buf==' ') buf++;
    while (*buf && *buf!='\n') {
      if (*buf == '~') { /* end of str found */
  /* see if there is crap floating on the end of the str */
  for(i=1;((buf[i]=='\n') || (buf[i]==' ')) && buf[i] !='\0'; i++);
  if (buf[i] != '\0') {
    Log(LOG_ERROR, "FileBinaryRead(file.c): Ignoring extraneous chars (\"");
    LogPrintf(LOG_ERROR, sText);
    LogPrintf(LOG_ERROR, "\")\n");
          fileError = TRUE;
  }
  return StrCreate(sText, sLen-1, DONTHASH);
      }

      /* copy the just read hex value into str */
      sscanf(buf, " %x", &hexValue);
      sText[sLen] = hexValue;
      sLen++;
      buf = StrOneWord(buf, NULL);
    }
  }
  /*
   * dont believe we'll ever fall through to here 
   * but compiler will generate warnings
   */
  return StrCreate(sText, sLen-1, DONTHASH);
}

/* read a string line at a time until a tilda is read,
   NOTE: anything after the ~ on the SAME line will be IGNORED! (I'll warn you dont worry)
   other than that can format the data files however you like with whitespace */
STR  *FileStrRead(FILE *file){
  BYTE  buf[256];
  BYTE  sText[FILE_STR_MAX];
  LWORD sLen = 0;
  LWORD i;

  fileError = FALSE;
  while (1) {
    fgets(buf, 256, file);
    if (feof(file)) {
      Log(LOG_ERROR, "FStrRead(file.c): Unexpected End of File\n");
      fileError = TRUE;
      break;
    }
    /* copy the just read str */
    for(i=0; buf[i] != '\0' && buf[i] != '~'; i++) {
      sText[sLen] = buf[i];
      sLen++;
    }
    if (buf[i] == '~') { /* end of str found */
      /* see if its binary */
      if (!strcmp(buf+i+1, "BINARY")) {
        return FileBinaryRead(file);
      }
      /* see if there is crap floating on the end of the str */
      for(i++;((buf[i]=='\n') || (buf[i]==' ') || (buf[i]=='\r')) && buf[i] !='\0'; i++);
      if (buf[i] != '\0') {
        sText[sLen] = '\0';
        Log(LOG_ERROR, "FStrRead(file.c): Ignoring extraneous chars (\"");
        LogPrintf(LOG_ERROR, sText);
        LogPrintf(LOG_ERROR, "\")\n");
        fileError = TRUE;
      }
      break;
    }
  }
  sText[sLen] = '\0';
  /* return STRCREATE(sText); */
  return StrCreate(sText, SLEN_UNKNOWN, HASH);
}

LWORD FileFlagRead(FILE *file, BYTE *fList[]) {
  BYTE  buf[FILE_STR_MAX];
  FLAG  flag;

  fileError = FALSE;
  fscanf(file, " %s", buf);
  if (StrIsNumber(buf)) {
    return atol(buf);
  } else {
    flag = FlagSscanf(buf, fList);
  }
  return flag;
}

LWORD FileTypeRead(FILE *file, ULWORD tList, LWORD tSize) {
  BYTE  buf[FILE_STR_MAX];
  LWORD type;

  fileError = FALSE;
  fscanf(file, " %s", buf);
  type = TypeSscanf(buf, tList,tSize);
  if (type == -1) {
    type = 0;
    Log(LOG_ERROR, "Unknown type entry encountered on FileTypeRead:");
    LogPrintf(LOG_ERROR, *((BYTE**)tList));
    LogPrintf(LOG_ERROR, "\n");
    Log(LOG_ERROR, buf);
    LogPrintf(LOG_ERROR, "\n");
    fileError = TRUE;
  }
  return type;
}

BYTE FileByteRead(FILE *file) {
  WORD temp=0;

  fileError = FALSE;
  fscanf(file, " %hd", &temp);
  return (BYTE)temp;
}


/* write modes, if verbose is selected the routines will expand the flag/type etc
   to the name, thus making it much easier to text edit the files */

/* currently Writing is not asynchronous (although it probably should be) so beware
   performance implications! (although disk should buffer output streams so that it is sorta) */
BYTE FileStrWrite(FILE *file, STR *str){

  fileError = FALSE;
  /* this one is easy */
  if (!file) {
    Log(LOG_ERROR, "FStrWrite: No file defined\n");
    return FALSE;
  }
  if (!str) { /* this should NEVER HAPPEN! */
    Log(LOG_ERROR, "FStrWrite(file.c): Null STR passed!\n");
    fprintf(file,"~\n");
    return TRUE; /* well it worked didnt it? */
  }
  if (str->sLen > FILE_STR_MAX) { /* this should NEVER HAPPEN! */
    Log(LOG_ERROR, "WARNING! FStrWrite(file.c): STR too long for read!\n");
  }
  fprintf(file, "%s~\n", str->sText);
  return TRUE;
}

/* Write a string out in Binary format */
BYTE FileBinaryWrite(FILE *file, STR *str){
  LWORD i;

  fileError = FALSE;
  /* this one is easy */
  if (!file) {
    Log(LOG_ERROR, "FStrWrite: No file defined\n");
    return FALSE;
  }
  if (!str) { /* this should NEVER HAPPEN! */
    Log(LOG_ERROR, "FStrWrite(file.c): Null STR passed!\n");
    fprintf(file,"~\n");
    return TRUE; /* well it worked didnt it? */
  }
  if (str->sLen > FILE_STR_MAX) { /* this should NEVER HAPPEN! */
    Log(LOG_ERROR, "WARNING! FStrWrite(file.c): STR too long for read!\n");
  }

  fprintf(file, "~BINARY\n");
  for(i=0; 1; i++) {
    fprintf(file, " %02X", str->sText[i]);
    if (i == str->sLen-1) break;
    if (!i%26) fprintf(file, "\n");
  }
  fprintf(file, "\n~\n");
  return TRUE;
}

void  FileTypeWrite(FILE *file, LWORD type, ULWORD tList, LWORD tSize, BYTE next){
  BYTE buf[512];

  fileError = FALSE;
  if (fileVerbose) {
    TypeSprintf(buf, type, tList, tSize, 512);
    if (!buf[0]) strcpy(buf, "0");
    fprintf(file, "%s%c", buf, next);
  } else {
    fprintf(file, "%ld%c", type, next);
  }
}

void  FileFlagWrite(FILE *file, FLAG flag, BYTE *fList[], BYTE next){
  BYTE buf[512];

  fileError = FALSE;
  if (fileVerbose) {
    FlagSprintf(buf, flag, fList, '|', 512);
    if (!buf[0]) strcpy(buf, "0");
    fprintf(file, "%s%c", buf, next);
  } else {
    fprintf(file, "%ld%c", flag, next);
  }
}


void FileByteWrite(FILE *file, BYTE value, BYTE separator) {
  WORD temp;

  fileError = FALSE;
  temp=value;
  fprintf(file, "%hd%c", temp, separator);
}

