/* part of Crimson2 */
/* Written by B. Cameron Lesiuk */
/* Written for use with Crimson2 MUD (written/copyright Ryan Haksi 1995).
 * This source is proprietary. Use of this code without permission from 
 * Ryan Haksi or Cam Lesiuk is strictly prohibited. 
 * 
 * (clesiuk@engr.uvic.ca)
 */

/* MOLE related buffer/socket management tools */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
  #include <unistd.h>
#endif

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "str.h"
#include "queue.h"
#include "send.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "file.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "mem.h"
#include "socket.h"
#include "site.h"
#include "exit.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "affect.h"
#include "fight.h"
#include "mobile.h"
#include "skill.h"
#include "player.h"
#include "parse.h"
#include "area.h"
#include "moledefs.h"
#include "mole.h"

LWORD MOLEParse(SOCK *sock, BYTE *cmd, ULWORD *moleCmd) {
  LWORD       i;
  LWORD       j;
  ULWORD      checkSum;
  BYTE       *p;
  BYTE        buf[2*(MOLE_PKT_MAX+1)];
  BYTE        data;

  if ((!cmd)||(!sock)||(!moleCmd)) return -1;
  /* First, check the header */
  i=0;

  /* Zip past any garbage at the start */
  while((cmd[i])&&(cmd[i]!=MOLE_DELIM_CHAR))
    i++;
  /* Check integrity of MOLE header */
  p=MOLE_HDR;
  for (j=0;(cmd[i+j])&&(j<MOLE_HDR_SIZE);j++)
    if (p[j]!=cmd[i+j]) 
      break;
  if (j!=MOLE_HDR_SIZE) return -1;

  if (cmd[i+j])
    i+=j+1; /* increment past header and mandatory space */
  else 
    return -1;

  *moleCmd=0;
  for (j=0;(j<4)&&(cmd[i+j]);j++) {
    (*moleCmd)<<=8;
    (*moleCmd)|=cmd[i+j];
  }
  if (j<4) return -1;

  /* we've just read in our cmd. */
  if (cmd[i+j])
    i+=j+1; /* increment past command word */
  else 
    return -1;

  /* now we just need to read in our buffer */
  j=0;
  checkSum=0;
  while((cmd[i+(j<<1)])&&(cmd[i+(j<<1)+1])) {
    if ((cmd[i+(j<<1)]==MOLE_DELIM_CHAR)&&(cmd[i+(j<<1)+1]==MOLE_DELIM_CHAR)) {
      if (checkSum) return -1; /* checksum should be 0 */
      if (j) j--;              /* whack off trailing byte (checksum) */
      buf[(j<<1)]=0; /* terminate string */
      if (j) QAppend(sock->sMole,buf,Q_DONTCHECKSTR); 
      return j;
    } else {
      if (j==(MOLE_PKT_MAX+1))
        return -1;              /* packet too large - error */
 
      /* decode data - check for data integrity */
      if ((cmd[i+(j<<1)]>='0')&&(cmd[i+(j<<1)]<='9'))
        data=(cmd[i+(j<<1)]-'0')<<4;
      else if ((cmd[i+(j<<1)]>='A')&&(cmd[i+(j<<1)]<='F'))
        data=(cmd[i+(j<<1)]-'A'+10)<<4;
      else return -1; /* error in format - only 1-9, A-F allowed */

      if ((cmd[i+(j<<1)+1]>='0')&&(cmd[i+(j<<1)+1]<='9'))
        data+=(cmd[i+(j<<1)+1]-'0');
      else if ((cmd[i+(j<<1)+1]>='A')&&(cmd[i+(j<<1)+1]<='F'))
        data+=(cmd[i+(j<<1)+1]-'A'+10);
      else return -1; /* error in format - only 1-9, A-F allowed */

      /* now save data */
      buf[(j<<1)]=cmd[i+(j<<1)];
      buf[(j<<1)+1]=cmd[i+(j<<1)+1];
      checkSum^=data;

      j++;
    } /* if .. else */
  } /* while */
  return -1;
}

void MOLEFlushQ(SOCK *sock) {
  if (sock) {
    QFlush(sock->sMole);
  }
}

LWORD MOLEGetQSize(SOCK *sock) {
  if (sock) return sock->sMole->qLen;
  return -1;
}

LWORD MOLEGetQData(SOCK *sock, BYTE *buf, ULWORD *bufLen,FLAG flag) {
  BYTE   tmpBuf[4];
  ULWORD i;
  BYTE   data;

  if ((!sock)||(!buf)||(!bufLen)) return -1;
  i=0;
  if (flag & GQD_STRING)
    (*bufLen)--;
  while ((MOLEGetQSize(sock)>=2)&&(i<*bufLen)) {
    /* read in 2 characters */
    QReadByte(sock->sMole,tmpBuf,4,Q_COLOR_IGNORE,2);

    if (tmpBuf[0] && tmpBuf[1]) tmpBuf[2]=0;

    /* process 2-byte string into a number */
    if ((tmpBuf[0]>='0')&&(tmpBuf[0]<='9'))
      data=(tmpBuf[0]-'0')<<4;
    else if ((tmpBuf[0]>='A')&&(tmpBuf[0]<='F'))
      data=(tmpBuf[0]-'A'+10)<<4;
    else return -1; /* error in format - only 1-9, A-F allowed */

    if ((tmpBuf[1]>='0')&&(tmpBuf[1]<='9'))
      data+=(tmpBuf[1]-'0');
    else if ((tmpBuf[1]>='A')&&(tmpBuf[1]<='F'))
      data+=(tmpBuf[1]-'A'+10);
    else return -1;
  
    buf[i]=data;
    if ((data==0x00)&&(flag & GQD_STRING))
      break;
    i++;
  }
  *bufLen=i;
  if (flag&GQD_STRING)
    buf[i+1]=0x00; /* terminate if we've filled a string buffer fully */
  return i;
}

/* reads the first 4 bytes as a packet Cmd - ie straight out of the
 * data stream. This may be different from MOLEGetQULWORD. */
LWORD MOLEGetQDataCmd(SOCK *sock, ULWORD *pktCmd) {
  ULWORD bufLen;

  if ((!sock)||(!pktCmd))
    return -1;
  bufLen=4;
  MOLEGetQData(sock,(BYTE*)pktCmd,&bufLen,GQD_BINARY);
  if (bufLen!=4)
    return -1;
  return 0;
}

/* reads the first 4 bytes from the queue and stuffs them in *data.
 * If there's not 4 bytes for the full ULWORD, the bytes are still 
 * read, and -1 is returned (error). */
LWORD MOLEGetQULWORD(SOCK *sock, ULWORD *data) {
  BYTE   buf[6];
  ULWORD bufLen;

  *data=0L;
  if ((!sock)||(!data))
    return -1;
  bufLen=4;
  MOLEGetQData(sock,buf,&bufLen,GQD_BINARY);
  if (bufLen!=4) 
    return -1;
  *data=(((ULWORD)(buf[0])&0x0FFL))|(((ULWORD)(buf[1])&0x0FFL)<<8)|
        (((ULWORD)(buf[2])&0x0FFL)<<16)|(((ULWORD)(buf[3]))<<24);
  return 0;
}

LWORD MOLEGetQStr(SOCK *sock, STR **data) {
  static BYTE  *buf=NULL;
  static LWORD  bufLen=1024;
         LWORD  rc;
         STR   *str;
         LWORD  size;

  if ((!sock)||(!data))
    return -1;

  /* Resize our buffer if it's too small. This part just assumes
   * the string we are about to read is the size of the remaining
   * mole buffer - an incorrect assumption, but a safe one! */
  /* Note that this should alloc us a buffer when buf==NULL */
  while(((MOLEGetQSize(sock)/2+1)>=bufLen)||(!buf))
    REALLOC("WARNING! MOLE string buffer overflow... resizing\n",
      buf,BYTE,(MOLEGetQSize(sock)/2+1),bufLen);

  /* go get our string */
  size=bufLen;
  rc=MOLEGetQData(sock,buf,&size,GQD_STRING);
  str=NULL;
  if (rc>=0) {
    /* now create a STR to return */
    str=StrCreate(buf,SLEN_UNKNOWN,HASH);
  }
  if (data)
    *data=str;
  if (str)
    return 0;
  return -1;
}

/* This proc inserts srcBuf into dstBuf, sending MORE packets if req'd. */
/* NOTE This proc assumes dstBuf is at LEAST MOLE_PKT_MAX bytes long.   */
LWORD MOLEWriteULWORD(SOCK *sock, ULWORD ulWord, BYTE *dstBuf,ULWORD *bufPos) {
  if ((!sock)||(!dstBuf)||(!bufPos)) return -1;
  if ((*bufPos)>=(MOLE_PKT_MAX-4)) {
    MOLESend(sock,dstBuf,*bufPos,MOLE_CMD_MORE);
    (*bufPos)=0;
    }
  dstBuf[*bufPos]=(BYTE)((ulWord & 0x000000FFL));
  (*bufPos)++;
  dstBuf[*bufPos]=(BYTE)((ulWord & 0x0000FF00L)>>8);
  (*bufPos)++;
  dstBuf[*bufPos]=(BYTE)((ulWord & 0x00FF0000L)>>16);
  (*bufPos)++;
  dstBuf[*bufPos]=(BYTE)((ulWord & 0xFF000000L)>>24);
  (*bufPos)++;
  return 0;
}

/* This proc inserts srcBuf into dstBuf, sending MORE packets if req'd. */
/* NOTE This proc assumes dstBuf is at LEAST MOLE_PKT_MAX bytes long.   */
LWORD MOLEWriteBuf(SOCK *sock,BYTE *srcBuf,BYTE *dstBuf,ULWORD *bufPos) {
  if ((!sock)||(!srcBuf)||(!dstBuf)||(!bufPos)) return -1;
  /* copy file name into buffer */
  srcBuf--; /* we do this so that we INCLUDE the trailing 0x00 */
  do {
    srcBuf++;
    dstBuf[*bufPos]=*srcBuf;
    (*bufPos)++;
    if ((*bufPos)>= MOLE_PKT_MAX) {
      MOLESend(sock,dstBuf,*bufPos,MOLE_CMD_MORE);
      (*bufPos)=0;
    }
  } while (*srcBuf);
  return 0;
}

/* This proc calls MOLEWriteBuf to insert flag and type lists into
 * the MOLE packet */
LWORD MOLEWriteList(SOCK *sock,ULWORD list,LWORD listSize,BYTE *dstBuf,ULWORD *bufPos) {
  LWORD i;


  i=-1; /* this is so we get & send our last NULL string */
  do {
    i++;
    MOLEWriteBuf(sock,*((BYTE**)(list+i*listSize)),dstBuf,bufPos);
  } while (**((BYTE**)(list+i*listSize))!='\0');
  return 0;
}

/* This proc sends a MOLE-formatted packet to sock. No buffer length */
/* checking is done - it's the caller's responsibility to enseure    */
/* cmdLen doesn't exceed MOLE_PKT_MAX                                */
void MOLESend(SOCK *sock, BYTE* cmd, LWORD cmdLen, ULWORD moleCmd) {
  ULWORD     i;
  ULWORD     counter;
  BYTE       buf[MOLE_HDR_SIZE+(MOLE_PKT_MAX*2)+14];
  UWORD      checkSum;

  if ((!sock)||(!cmd)) return;

  i=0; /* this is our buffer position marker. */

  /**** MOLE Package format is defined here! ****/
  /* start with a start-of-packet character */
  buf[i]=MOLE_PKT_START;
  i++;

  /* append our header */ 
  strcpy(&(buf[i]),MOLE_HDR);
  i+=MOLE_HDR_SIZE;
 
  /* <space><command><space> */ 
  buf[i]=' ';
  i++;
  buf[i+0]=(moleCmd>>24)&0xFF;
  buf[i+1]=(moleCmd>>16)&0xFF;
  buf[i+2]=(moleCmd>>8)&0xFF;
  buf[i+3]=(moleCmd)&0xFF;
  i+=4;
  buf[i]=' ';
  i++;

  /* <data><data><data><...><data><checksum> */
  checkSum=0;
  for (counter=0;counter<cmdLen;counter++) {
    sprintf(&(buf[i]),"%02X",(UWORD)cmd[counter]);
    checkSum^=cmd[counter];
    i+=2;
  }
  sprintf(&buf[i],"%02X",checkSum);
  i+=2;

  /* Ending delimiter */
  buf[i]=MOLE_DELIM_CHAR;
  i++;
  buf[i]=MOLE_DELIM_CHAR;
  i++;
 
  /* end-of-packet character */ 
  buf[i]=MOLE_PKT_STOP;
  i++;

  /* oh! Don't forget to send a '\n'! */
  buf[i]='\n';
  i++;

  /* and terminate our string */
  buf[i]=0;
/*
  buf[0]=MOLE_PKT_START;
  buf[1]=0;
  SEND(buf,sock);
  SEND(MOLE_HDR,sock);
  SEND(" ",sock); 
  buf[0]=(moleCmd>>24)&0xFF;
  buf[1]=(moleCmd>>16)&0xFF;
  buf[2]=(moleCmd>>8)&0xFF;
  buf[3]=(moleCmd)&0xFF;
  buf[4]=0;
  SEND(buf,sock);
  SEND(" ",sock);
  checkSum=0;
  for (counter=0;counter<cmdLen;counter++) {
    sprintf(buf,"%02X",(UWORD)cmd[counter]);
    SEND(buf,sock);
    checkSum^=cmd[counter];
  }
  sprintf(buf,"%02X",checkSum);
  SEND(buf,sock);
  SEND(MOLE_DELIM,sock);
  buf[0]=MOLE_PKT_STOP;
  buf[1]=0;
  SEND(buf,sock);
  buf[0]='\n';
  buf[1]=0;
  */
  SEND(buf,sock);
}

LWORD MOLECommandCheck(SOCK *sock, BYTE *cmd, LWORD virtual) {
  BYTE  fullCmd[256];

  if (virtual<0)
    return ParseCommandCheck(TYPEFIND(cmd,commandList),sock,cmd);
  
  sprintf(fullCmd,"%s %li",cmd,virtual);
  return ParseCommandCheck(TYPEFIND(cmd,commandList),sock,fullCmd);
}
