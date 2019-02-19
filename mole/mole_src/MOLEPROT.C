// MOLE Client for MS Windows 3.11
// moleprot.c identifier: mp
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1997-1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************
//
// Written by B. Cameron Lesiuk
// March, 1997

// This module does the actual MOLE interaction, including buffering and
// queueing of requests and tracking of responses. This module also does
// some socket stuff for sending, unfortunately.

#include<windows.h>
#include<windowsx.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include"telnet.h"
#include"moledefs.h"
#include"molem.h"
#include"molerc.h"
#include"dstruct.h"
#include"host.h"
#include"infobox.h"
#include"moleprot.h"
#include"unpack.h"
#include"terminal.h"
#include"timer.h"
#include"debug.h"
#include"edit.h"
#include"areawnd.h"
#include"help.h"

/* Defines */
#define MP_SEND_BUFFER_SIZE 65500L

/* Globals */
unsigned long g_mpIDPool; /* contains our next available packet ID */
MPBUF g_mpPktBuf; /* intermediate buffer to collect a single complete mole packet */
MPBUF g_mpCmdBuf; /* intermediate buffer to collect a packet's data after formatting and checksum checks have verified packet as valid */
MPREQUEST *g_mpRequestQ;
/* the SendBuffer is not a MPBUF type - it's just a huge build-in
 * circular text buffer. It's allocated once and it's static in size. If
 * it fills up - we're probably in trouble anyways. */
char *g_mpSendBuffer; /* buffer for send characters - formatted & mixed mole/normal - HUGE and STATIC */
HGLOBAL g_mpSendBufferMemory;
unsigned long g_mpSendBufferSize;
char *g_mpSendBufferHead,*g_mpSendBufferTail;
unsigned long g_mpHostVersion; /* contains host MOLE and Crimson2 versions */

/* Buffer handling procs */
BOOL mpBufAlloc(MPBUF *p_buf,unsigned long p_len) {
  if (!p_buf) return FALSE;
  p_buf->bMemory=GlobalAlloc(GHND|GMEM_NOCOMPACT,p_len);
  p_buf->bData=(char *)GlobalLock(p_buf->bMemory);
  if ((!p_buf->bData)||(!p_buf->bMemory)) {
    ibInfoBox(g_aahWnd,"Cannot allocate memory for buffers. Please Free some memory!","Whoops!",IB_OK,NULL,0L);
    return FALSE;
  }
  p_buf->bLen=GlobalSize(p_buf->bMemory);
  p_buf->bPos=0L;
  return TRUE;
}

void mpBufFree(MPBUF *p_buf) {
  if (!p_buf) return;
  GlobalUnlock(p_buf->bMemory);
  GlobalFree(p_buf->bMemory);
  p_buf->bMemory=NULL;
  p_buf->bData=NULL;
  p_buf->bLen=p_buf->bPos=0L;
  return;
}

BOOL mpBufReAlloc(MPBUF *p_buf,unsigned long p_len) {
  HGLOBAL l_GlobalTemp;
  unsigned long l_i;
  char *l_BufTemp;

  if (!p_buf) return FALSE;
  if (p_len<p_buf->bLen) return TRUE;
  /* first, allocate a new buffer */
  l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,p_len);
  l_BufTemp=(char *)GlobalLock(l_GlobalTemp);
  if ((!l_BufTemp)||(!l_GlobalTemp)) {
    ibInfoBox(g_aahWnd,"Cannot allocate memory for buffers. Please Free some memory!","Whoops!",IB_OK,NULL,0L);
    return FALSE;
  }
  /* next, copy existing data over */
  for (l_i=0;l_i<p_buf->bPos;l_i++)
    l_BufTemp[(unsigned int)l_i]=p_buf->bData[(unsigned int)l_i];
  /* now free our old buffer */
  GlobalUnlock(p_buf->bMemory);
  GlobalFree(p_buf->bMemory);
  /* and move our new buffer into place */
  p_buf->bMemory=l_GlobalTemp;
  p_buf->bData=l_BufTemp;
  return TRUE;
}

void mpBufAppend(MPBUF *p_buf,char *p_data,unsigned long p_datalen) {
  unsigned int l_i;
  if ((!p_buf) || (!p_data)) return;
  if (!mpBufReAlloc(p_buf,p_datalen+p_buf->bPos)) /* resize buffer if required */
    return; /* couldn't allocate memory - abort operation */
  for (l_i=0;l_i<p_datalen;l_i++,p_buf->bPos++)
    p_buf->bData[(unsigned int)(p_buf->bPos)]=p_data[l_i];
}

void mpBufFlush(MPBUF *p_buf) {
  p_buf->bPos=0L;
}

/* Request handling functions */
/* This proc assumes that the request is NOT linked into any request list */
void mpReqFree(MPREQUEST *p_req) {
  HGLOBAL l_GlobalTemp;

  if (!p_req) return;

  /* free data memory */
  if (p_req->rDataMemory) {
    GlobalUnlock(p_req->rDataMemory);
    GlobalFree(p_req->rDataMemory);
  }

  /* and free this request's memory */
  l_GlobalTemp=p_req->rMemory;
  GlobalUnlock(l_GlobalTemp);
  GlobalFree(l_GlobalTemp);
  return;
}

/* This proc writes p_data into p_buf as hexadecimal text */
void mpWriteBinToText(char *p_buf,char p_data) {
  char l_tmp;

  if (!p_buf) return;

  l_tmp=(char)((p_data&0xF0)>>4);
  if (l_tmp<10) { /* numerical */
    *p_buf=(char)(l_tmp+'0');
  } else {
    *p_buf=(char)(l_tmp+'A'-10);
  }
  p_buf++;
  l_tmp=(char)(p_data&0x0F);
  if (l_tmp<10) { /* numerical */
    *p_buf=(char)(l_tmp+'0');
  } else {
    *p_buf=(char)(l_tmp+'A'-10);
  }
}

/* wrapper for mpReqSubmit for external modules */
unsigned long mpReqSubmitEx(unsigned long p_Cmd,char *p_Data,unsigned long p_DataLen) {
  MPREQUEST *l_req;

  mpReqSubmit(l_req=mpReqAlloc(p_Cmd,p_Data,p_DataLen),&g_mpRequestQ);
  return (l_req)?l_req->rID:MOLE_PKID_NULL;

}

/* wrapper for mpReqSubmit for external modules needing numerical packets */
unsigned long mpReqSubmitNum(unsigned long p_Cmd,unsigned long p_Data) {
  char l_buf[5];
  MPREQUEST *l_req;

  l_buf[0]=(char)(p_Data&0x000000FFL);
  l_buf[1]=(char)((p_Data&0x0000FF00L)>>8);
  l_buf[2]=(char)((p_Data&0x00FF0000L)>>16);
  l_buf[3]=(char)((p_Data&0xFF000000L)>>24);
  mpReqSubmit(l_req=mpReqAlloc(p_Cmd,l_buf,4),&g_mpRequestQ);
  return (l_req)?l_req->rID:MOLE_PKID_NULL;

}

/* Insert request into queue */
/* Note that this is the real kahuna. It takes the request, and inserts it into
 * the queue. */
void mpReqSubmit(MPREQUEST *p_req,MPREQUEST **p_list) {
  char l_buf[(MOLE_PKT_MAX*2)+MOLE_HDR_SIZE+8+4]; /* what we need + 4 for good measure (ie: trailing \n & 0x00 etc.*/
  int l_count,l_pos;
  char *l_src;
  char l_checksum;

  if (!p_req) return;

  /* if this request doesn't have an ID, give it a new one */
  /* NOTE we keep IDs the same for re-transmitted requests so that for
   * really slow connections, multiple requests can be sent, but the
   * first confirmation removes the it from the Q and the other confirmations
   * are discarded */
  if (!p_req->rID) {
    /* The first thing we do is assign a unique ID to this request */
    p_req->rID=g_mpIDPool;
    g_mpIDPool++;
    if (!g_mpIDPool) g_mpIDPool=MOLE_PKID_START; /* check for wrap around */
      /* like there's ever going to be wrap around with a 32-bit unsigned int! */
      /* how long would that take? Let's see... even if we somehow generated   */
      /* 100 packets per second, we'd need to run for over 1 year, 4 months    */
      /* straight. I suspect this client, or the machine it's running on, will */
      /* crash way way way before then. Ah, windows. Being able to COUNT on    */
      /* crashes is SUCH a lovely thing. Fuck, whatever.                       */
    }
  p_req->rNext=*p_list;
  p_req->rPrevious=NULL;
  if (*p_list)
    (*p_list)->rPrevious=p_req;
  *p_list=p_req;
  /* time stamp request */
  p_req->rTime=time(NULL);
  /* and fire off our data to the socket. */
  /* This step is a little ugly; we do all the MOLE formatting here. */
  l_pos=MOLE_HDR_SIZE+6; /* this counts where we are in our buffer */
  l_count=0; /* this counts # bytes sent - counts up to MOLE_PKT_MAX */
  l_src=p_req->rData; /* this tracks our source data buffer */
  strcpy(l_buf,MOLE_HDR); /* copy in header - doesn't change anyways */
  l_buf[MOLE_HDR_SIZE]=' ';
  l_buf[MOLE_HDR_SIZE+1]=(char)((MOLE_CMD_MORE&0xFF000000L)>>24);
  l_buf[MOLE_HDR_SIZE+2]=(char)((MOLE_CMD_MORE&0x00FF0000L)>>16);
  l_buf[MOLE_HDR_SIZE+3]=(char)((MOLE_CMD_MORE&0x0000FF00L)>>8);
  l_buf[MOLE_HDR_SIZE+4]=(char)((MOLE_CMD_MORE&0x000000FFL));
  l_buf[MOLE_HDR_SIZE+5]=' ';
  l_checksum=0;

  /* note that first we have to insert our packet ID */
  /* Note that for data fields, LSB comes FIRST, MSB comes LAST */
  mpWriteBinToText(&(l_buf[l_pos]),(char)(p_req->rID&0x000000FFL));
  l_checksum^=(char)(p_req->rID&0x000000FFL);
  l_pos+=2;
  mpWriteBinToText(&(l_buf[l_pos]),(char)((p_req->rID&0x0000FF00L)>>8));
  l_checksum^=(char)((p_req->rID&0x0000FF00L)>>8);
  l_pos+=2;
  mpWriteBinToText(&(l_buf[l_pos]),(char)((p_req->rID&0x00FF0000L)>>16));
  l_checksum^=(char)((p_req->rID&0x00FF0000L)>>16);
  l_pos+=2;
  mpWriteBinToText(&(l_buf[l_pos]),(char)((p_req->rID&0xFF000000L)>>24));
  l_checksum^=(char)((p_req->rID&0xFF000000L)>>24);
  l_pos+=2;
  l_count+=4;

  if (p_req->rData) {
    while((l_src-p_req->rData)<p_req->rDataLen) {
      /* add in this byte */
      mpWriteBinToText(&(l_buf[l_pos]),*l_src);
      l_checksum^=*l_src;
      l_pos+=2;
      l_src++;
      l_count++;

      /* check for maximum packet size */
      if (l_count>=MOLE_PKT_MAX) {
        /* append checksum */
        mpWriteBinToText(&(l_buf[l_pos]),l_checksum);
        l_pos+=2;

        /* and trailing ## */
        l_buf[l_pos]=MOLE_DELIM_CHAR;
        l_pos++;
        l_buf[l_pos]=MOLE_DELIM_CHAR;
        l_pos++;

        /* append a carridge return */
        l_buf[l_pos]='\r';
        l_pos++;
        /* terminate our buffer string */
        l_buf[l_pos]=0;
        /* and copy the sucker into our big ugly send buffer. */
//        dbPrint("Queueing the following data into Send Buffer:");
//        dbPrintNum(l_buf,l_pos);
        mpSendBufferAppend(l_buf,l_pos);

        /* Lastly, reset our local buffer variables */
        l_count=0;
        l_checksum=0;
        l_pos=MOLE_HDR_SIZE+6;
      }
    } /* while */
  } /* if data */
  /* now send off our final data */
  /* append checksum */
  mpWriteBinToText(&(l_buf[l_pos]),l_checksum);
  l_pos+=2;

  /* and trailing ## */
  l_buf[l_pos]=MOLE_DELIM_CHAR;
  l_pos++;
  l_buf[l_pos]=MOLE_DELIM_CHAR;
  l_pos++;

  /* append a carridge return */
  l_buf[l_pos]='\r';
  l_pos++;
  /* terminate our buffer string */
  l_buf[l_pos]=0;
  /* can't forget to put the proper command in! */
  l_buf[MOLE_HDR_SIZE+1]=(char)((p_req->rCmd&0xFF000000L)>>24);
  l_buf[MOLE_HDR_SIZE+2]=(char)((p_req->rCmd&0x00FF0000L)>>16);
  l_buf[MOLE_HDR_SIZE+3]=(char)((p_req->rCmd&0x0000FF00L)>>8);
  l_buf[MOLE_HDR_SIZE+4]=(char)((p_req->rCmd&0x000000FFL));
  /* and copy the sucker into our big ugly send buffer. */
//  dbPrint("Queueing the following data into Send Buffer:");
//  dbPrintNum(l_buf,16);
  mpSendBufferAppend(l_buf,l_pos);
  mpMoleSendData();
  return;
}

/* This proc takes p_req out of the p_list linked list. It doesn't free it though. */
void mpReqExtract(MPREQUEST *p_req,MPREQUEST **p_list) {
  if (!p_req) return;
  if (p_req->rNext)
    p_req->rNext->rPrevious=p_req->rPrevious;
  if (p_req->rPrevious)
    p_req->rPrevious->rNext=p_req->rNext;
  else
    *p_list=p_req->rNext;
  p_req->rPrevious=p_req->rNext=NULL;
}

/* This proc allocates the specified request, and returns a handle to it */
MPREQUEST *mpReqAlloc(unsigned long p_Cmd,char *p_Data,unsigned long p_DataLen) {
  HGLOBAL l_GlobalTemp;
  MPREQUEST *l_req;
  unsigned long l_i;

  /* first, alloc a request structure */
  l_GlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(MPREQUEST));
  l_req=(MPREQUEST *)GlobalLock(l_GlobalTemp);
  if ((!l_GlobalTemp)||(!l_req)) {
    ibInfoBox(g_aahWnd,"Cannot allocate memory for buffers. Please Free some memory!","Whoops!",IB_OK,NULL,0L);
    return NULL;
  }
  l_req->rCmd=p_Cmd;
  l_req->rID=MOLE_PKID_NULL;
  l_req->rMemory=l_GlobalTemp;
  l_req->rTime=0L;
  l_req->rNext=l_req->rPrevious=NULL;
  if (p_Data) {
    l_req->rDataLen=p_DataLen;
    /* now alloc space for our data */
    l_req->rDataMemory=GlobalAlloc(GHND|GMEM_NOCOMPACT,p_DataLen);
    l_req->rData=(char *)GlobalLock(l_req->rDataMemory);
    if ((!l_req->rDataMemory)||(!l_req->rData)) {
      ibInfoBox(g_aahWnd,"Cannot allocate memory for buffers. Please Free some memory!","Whoops!",IB_OK,NULL,0L);
      return NULL;
    }
    /* and copy over data into request structure */
    for (l_i=0;l_i<p_DataLen;l_i++)
      l_req->rData[(unsigned int)l_i]=p_Data[(unsigned int)l_i];
  } else {
    l_req->rDataMemory=NULL;
    l_req->rData=NULL;
    l_req->rDataLen=0L;
  }

  /* and finally, return our new structure pointer */
  return l_req;
}

void mpReqFlush(MPREQUEST **p_list) {
  MPREQUEST *l_req;
  while(*p_list) {
    l_req=*p_list;
    mpReqExtract(l_req,p_list);
    mpReqFree(l_req);
  }
  return;
}

/* Send Buffer Functions */
void mpSendBufferAppend(char *p_buf,unsigned int p_len) {
  unsigned long l_space_left,l_i;
  char *l_p;

//  dbPrint("SendBufferAppend: queue the following data:");
//  dbPrintNum(p_buf,p_len);

  /* first, calculate our remaining space */
  if (g_mpSendBufferHead >= g_mpSendBufferTail)
    l_space_left=g_mpSendBufferSize-(g_mpSendBufferHead-g_mpSendBufferTail);
  else
    l_space_left=(g_mpSendBufferTail-g_mpSendBufferHead);

  if ((p_len+2)>l_space_left) {
    dbPrint("NO ROOM!");
    return; /* just don't append the packet at all. Note that this could really screw some things up.... but oh well */
    /* I'm adding 2 just to leave a little room ... just in case */
  }

//  dbPrint("Copying data over to SendBuffer");
  /* ok, there's room. Copy the sucker over, checking for wrap-around */
  for (l_p=p_buf,l_i=0;l_i<p_len;l_p++,l_i++) {
    *g_mpSendBufferHead=*l_p;
    g_mpSendBufferHead++;
    if ((g_mpSendBufferHead-g_mpSendBuffer)>=g_mpSendBufferSize)
      g_mpSendBufferHead=g_mpSendBuffer;
  }
  /* and we're done. */
  return;
}

void mpSendBufferFlush() {
  g_mpSendBufferHead=g_mpSendBufferTail=g_mpSendBuffer;
  return;
}

unsigned long mpSendBufferSize() {
  unsigned long l_i;
  if (g_mpSendBufferHead >= g_mpSendBufferTail)
    l_i=g_mpSendBufferHead-g_mpSendBufferTail;
  else
    l_i=g_mpSendBufferSize-(g_mpSendBufferTail-g_mpSendBufferHead);
  return l_i;
}

/* read up to p_buflen bytes from buffer */
unsigned long mpSendBufferRead(char *p_buf,unsigned long p_buflen) {
  unsigned long l_i;

  if (!p_buf) return 0L;
  l_i=0;
  while((l_i<p_buflen)&&(g_mpSendBufferTail!=g_mpSendBufferHead)) {
    p_buf[(unsigned int)l_i]=*g_mpSendBufferTail;
    l_i++;
    g_mpSendBufferTail++;
    if ((g_mpSendBufferTail-g_mpSendBuffer)>=g_mpSendBufferSize)
      g_mpSendBufferTail=g_mpSendBuffer;
  }
  return l_i;
}

/* puts data BACK onto the stack - note it has to have JUST been read */
/* because it's not copied - just the pointers are adjusted */
#pragma argsused
unsigned long mpSendBufferUnRead(char *p_buf,unsigned long p_buflen) {
  g_mpSendBufferTail-=(unsigned int)p_buflen;
  if (g_mpSendBufferTail<g_mpSendBuffer)
    g_mpSendBufferTail+=(unsigned int)g_mpSendBufferSize;
  return p_buflen;
}

/* MOLEPROT functions */
BOOL mpInitMoleProt () {
  if (!mpBufAlloc(&g_mpCmdBuf,8192)) return FALSE;
  if (!mpBufAlloc(&g_mpPktBuf,8192)) return FALSE;
  mpBufFlush(&g_mpCmdBuf);
  mpBufFlush(&g_mpPktBuf);
  g_mpRequestQ=NULL;

  /* allocate our huge gigantic output send buffer */
  g_mpSendBufferMemory=GlobalAlloc(GHND|GMEM_NOCOMPACT,MP_SEND_BUFFER_SIZE);
  g_mpSendBuffer=(char *)GlobalLock(g_mpSendBufferMemory);
  if ((!g_mpSendBufferMemory)||(!g_mpSendBuffer)) {
    ibInfoBox(g_aahWnd,"Cannot allocate memory for buffers. Please Free some memory!","Whoops!",IB_OK,NULL,0L);
    return FALSE;
  }
  g_mpSendBufferSize=GlobalSize(g_mpSendBufferMemory);
  g_mpSendBufferHead=g_mpSendBufferTail=g_mpSendBuffer;
  /* Phew. What a monster. */

  g_mpIDPool=MOLE_PKID_START; /* note that 0 is an invalid ID */

  return TRUE;
}

void mpShutdownMoleProt() {
  mpBufFree(&g_mpCmdBuf);
  mpBufFree(&g_mpPktBuf);
  mpReqFlush(&g_mpRequestQ);
//  while (g_mpRequestQ) {
//    l_req=g_mpRequestQ;
//    mpReqExtract(l_req,&g_mpRequestQ);
//    mpReqFree(l_req);
//  }
  GlobalUnlock(g_mpSendBufferMemory);
  GlobalFree(g_mpSendBufferMemory);

  return;
}


/* This proc accepts data form the socket and filters out
 * the mole packets, performing buffering etc. as required.
 * If this proc gets a completed mole packet, it calls the
 * appropriate function and ensures the request management
 * software is notified. */
void mpMolePacketFilter(char *p_buf,unsigned long p_buflen) {
  static l_ParsePacket=FALSE; /* TRUE if we're in the middle of parsing a MOLE packet */
  char *l_p,*l_mark;
  BOOL l_setmark;
  BOOL l_StripReturn=FALSE;

//  dbPrint("mpMolePacketFilter called with new data!");
//  dbPrintNum(p_buf,p_buflen);
  l_p=l_mark=p_buf;
  while ((l_p-p_buf)<p_buflen) {
    l_setmark=FALSE;
    if (l_ParsePacket) {
      if ((*l_p)==MOLE_PKT_STOP) {
//        dbPrint("END OF MOLE PACKET");
        /* end of packet marker */
        /* first, append what data we've gathered here to our mole packet buffer */
        mpBufAppend(&g_mpPktBuf,l_mark,l_p-l_mark);
        /* process our g_mpPktBuf MOLE packet */
        mpStripPkt(&g_mpPktBuf);
        /* reset the packet buffer */
        mpBufFlush(&g_mpPktBuf);
        /* and wrap up - switch back to normal text mode */
        l_StripReturn=TRUE;
        l_setmark=TRUE;
        l_ParsePacket=FALSE;
      }
    } else {
      /* we're looking for a Start-Of-Packet character */
      if ((*l_p)==MOLE_PKT_START) {
//        dbPrint("START OF MOLE PACKET");
        /* mark us as "packet start" and send what we've got so far to our terminal */
//        dbPrint("Sending this junk to the terminal");
        trSendTerminal(l_mark,l_p-l_mark);
//        dbPrintNum(l_mark,l_p-l_mark);
        mpBufFlush(&g_mpPktBuf); /* flush our packet buffer */
        l_setmark=TRUE;
        l_ParsePacket=TRUE;
      }
      /* otherwise, just increment our pointer */
    }
    l_p++;
    if ( (l_StripReturn) && ((*l_p==0x0D)||(*l_p==0x0A)) )
      {
      l_p++;
      if ((*l_p==0x0D)||(*l_p==0x0A))
        {
        l_p++;
        }
      }
    if (l_setmark)
      l_mark=l_p;
  }
  /* we've reached the end of our buffer. Send what we have left to the terminal */
  /* or to the MOLE buffer */
  if (l_ParsePacket) {
    mpBufAppend(&g_mpPktBuf,l_mark,l_p-l_mark);
  } else {
//    dbPrint("Sending this junk to the terminal");
    trSendTerminal(l_mark,l_p-l_mark);
//    dbPrintNum(l_mark,l_p-l_mark);
  }
}

/* when we've buffered in what we think is a complete MOLE packet, */
/* this proc is called to ensure formatting is correct, strip      */
/* extraneous characters, translate data into binary, buffer       */
/* data for _MORE packets, and finally call mpProcessPkt() when    */
/* a complete and proper packet is fully received.                 */
void mpStripPkt(MPBUF *p_pkt) {
  char *l_p;
  unsigned long l_Cmd,l_i;
  char l_data[MOLE_PKT_MAX];
  char l_Checksum;

  if (!p_pkt) return;

//  /* TAG TAG */
//  dbPrint("MOLE PACKET RECEIVED!!!!!!!!!!");
  p_pkt->bData[(unsigned int)(p_pkt->bPos)]=0;
//  dbPrint(p_pkt->bData);

  if (p_pkt->bPos<(MOLE_HDR_SIZE+8)) return;
  l_p=&(p_pkt->bData[MOLE_HDR_SIZE]);
  if (strncmp(p_pkt->bData,MOLE_HDR,MOLE_HDR_SIZE)) return;
  /* read in command */
  l_Cmd=l_p[1];
  l_Cmd<<=8;
  l_Cmd|=l_p[2];
  l_Cmd<<=8;
  l_Cmd|=l_p[3];
  l_Cmd<<=8;
  l_Cmd|=l_p[4];

  /* and now extract data into binary format */
  l_Checksum=0;
  l_p+=6; /* jump to start of data */
  l_i=0; /* our position in l_data[] */
  while(1) {
    if ((l_i+1)>=p_pkt->bLen)
      return; /* premature end of packet - error */
    else if ((*l_p==MOLE_DELIM_CHAR)&&(*(l_p+1)==MOLE_DELIM_CHAR)) {
      /* end of packet. */
      /* check checksum */
      if (l_Checksum) return; /* should be 0x00 */
      /* packet looks good. Let's get out of here. */
      if (l_i) l_i--; /* subtract back before checksum byte */
      break;
    } else {
      /* let's decode this byte */
      if ((*l_p>='0')&&(*l_p<='9')) {
        l_data[(unsigned int)l_i]=(char)((*l_p-'0')<<4);
      } else if ((*l_p>='A')&&(*l_p<='F')) {
        l_data[(unsigned int)l_i]=(char)((*l_p-'A'+0x0A)<<4);
      } else {
        /* error - invalid character */
        return;
      }
      l_p++;
      if ((*l_p>='0')&&(*l_p<='9')) {
        l_data[(unsigned int)l_i]|=(char)((*l_p-'0'));
      } else if ((*l_p>='A')&&(*l_p<='F')) {
        l_data[(unsigned int)l_i]|=(char)((*l_p-'A'+0x0A));
      } else {
        /* error - invalid character */
        return;
      }
      l_Checksum^=l_data[(unsigned int)l_i];
      l_p++;
      l_i++;
    }
  } /* while (1) */

  /* we have our data decoded - append it to our command parameter buffer */
  mpBufAppend(&g_mpCmdBuf,l_data,l_i);

//  dbPrint("Appended data to command buffer - now what?");
  /* now process packet command */
  if (l_Cmd!=MOLE_CMD_MORE) {
//    dbPrint("PROCESS IT!");
    mpProcessPkt(l_Cmd,&g_mpCmdBuf);
    /* clear command buffer */
    mpBufFlush(&g_mpCmdBuf);
  } //else dbPrint("Oh, is a MORE pkt. Wait for more info");

  return;
}

/* This proc handles sending data from our SendBuffer to the socket. */
/* This proc is called when the socket is ready for writing.    */
#define MP_MOLE_SEND_DATA_SIZE 80   /* arbitrary length really */
void mpMoleSendData() {
  char l_buf[MP_MOLE_SEND_DATA_SIZE];
  int l_rc;
  unsigned long l_count;

//  sprintf(l_buf,"Sending data now... (%lu bytes)",mpSendBufferSize());
//  dbPrint(l_buf);
  while (mpSendBufferSize()) {
    l_count=mpSendBufferRead(l_buf,MP_MOLE_SEND_DATA_SIZE);
    if (l_count) {
      /* send to socket */
//      dbPrintNum(l_buf,l_count);
      l_rc=send(g_hoSock,l_buf,(int)l_count,0);
      if (l_rc==SOCKET_ERROR) {
        switch (WSAGetLastError()) {
          case WSAENETRESET:
          case WSAENOTCONN:
          case WSAENOTSOCK:
          case WSAESHUTDOWN:
          case WSAECONNRESET:
          case WSAECONNABORTED:
            /* our socket is dead. */
            hoSocketKilled();
            ibInfoBox(g_aahWnd,"Connection closed due to a network error.","Darn",IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_NETERR);
            return;
          default:
            mpSendBufferUnRead(l_buf,l_count);
            return;
        } /* if error */
      } else if (l_rc<l_count) {
        mpSendBufferUnRead(&l_buf[l_rc],l_count-l_rc);
      }
    }
  }
  /* now send a TELNET-GA code */
  l_buf[0]=IAC;
  l_buf[1]=GA;
  l_buf[3]=0;
  l_rc=send(g_hoSock,l_buf,2,0);
  if (l_rc==SOCKET_ERROR) {
    switch (WSAGetLastError()) {
      case WSAENETRESET:
      case WSAENOTCONN:
      case WSAENOTSOCK:
      case WSAESHUTDOWN:
      case WSAECONNRESET:
      case WSAECONNABORTED:
        /* our socket is dead. */
        hoSocketKilled();
        ibInfoBox(g_aahWnd,"Connection closed due to a network error.","Darn",IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_NETERR);
        return;
      default:
        return;
    } /* switch */
  } /* if error */
}

/* This proc strips the first 4 bytes from p_buf and returns them as a decoded
 * unsigned long. No error checking is done - the buffer MUST be 4 or more
 * characters long to avoid errors. */
unsigned long mpDecodeULWORD(char *p_buf) {
  unsigned long l_i;
  char *l_p;

  l_p=p_buf+3;
  l_i=(*l_p)&0xFF;
  l_i<<=8;
  l_p--;
  l_i|=(*l_p)&0xFF;
  l_i<<=8;
  l_p--;
  l_i|=(*l_p)&0xFF;
  l_i<<=8;
  l_p--;
  l_i|=(*l_p)&0xFF;
  return l_i;
}

/* This proc writes the given unsigned long word to the data buffer provided */
/* the return value is the # bytes written to the buffer */
int mpEncodeULWORD(char *p_buf,unsigned long p_data) {
  if (!p_buf) return 0;

  p_buf[0]=(char)((p_data&0x000000FFL));
  p_buf[1]=(char)((p_data&0x0000FF00L)>>8);
  p_buf[2]=(char)((p_data&0x00FF0000L)>>16);
  p_buf[3]=(char)((p_data&0xFF000000L)>>24);
  return 4;
}

/* This proc is part of the RequestQ Loop. It takes incoming packets
 * and matches them with our outgoing request packets. It then removes
 * the outgoing request packets and then takes whatever action is required. */
void mpProcessPkt(unsigned long p_Cmd, MPBUF *p_Data) {
  unsigned long l_pktID;
  unsigned long l_dataLeft;
  unsigned long l_returnCode; /* for NACK messages */
  char *l_p;
  MPREQUEST *l_req,*l_nextReq;
//  char l_buf[100];

  if (!p_Data) {
//    dbPrint("Aborting mpProcessPkt - null pointer");
    return;
  }
//  dbPrint("Processing (now binary) packet...");

  l_dataLeft=p_Data->bPos;
  l_p=p_Data->bData;
  /* first, parse off the packet ID */
  if (l_dataLeft < 4 ) {
//    dbPrint("Less than bytes - can't get packet ID");
    return;
  }

//  sprintf(l_buf,">> %02X %02X %02X %02X ...",l_p[0],l_p[1],l_p[2],l_p[3]);
//  dbPrint(l_buf);

  l_pktID=mpDecodeULWORD(l_p);
  l_p+=4;
  l_dataLeft-=4;

//  sprintf(l_buf,"looking for packet ID=%lu",l_pktID);
//  dbPrint(l_buf);
  /* now, find the requesting packet in our RequestQ */
  for (l_req=g_mpRequestQ;l_req;l_req=l_req->rNext) {
    if (l_req->rID==l_pktID)
      break;
  }

//  dbPrint("Did we find our packet in our Q?");
  if (!l_req) {
//    dbPrint("No... gasp, I'm DIEING");
    return; /* couldn't find packet request - turffed, unsolicited, or corrupted (error) */
  }
//  dbPrint("Yup!");

  l_returnCode=0L;
  /* Ok, let's process our packet, depending on its type */
  switch (p_Cmd) {
    case MOLE_CMD_ACKP:
      /* find the originator and mark it's structure as "up-to-date"
       * instead of "pending" */
      break;
    case MOLE_CMD_NACK:
      /* find the originator and announce that operation was refused */
      if (l_dataLeft >= 4) {
        l_returnCode=mpDecodeULWORD(l_p);
        l_p+=4;
        l_dataLeft-=4;
      } else {
        l_returnCode=0xFFFFFFFFL;
      }
      break;
    case MOLE_CMD_IDEN:
//      dbPrint("Wow, an IDEN packet! We know with whom we speak!");
      /* This is only used during login */
      if (l_dataLeft >= 4 ) {
        g_mpHostVersion=mpDecodeULWORD(l_p);
        l_p+=4;
        l_dataLeft-=4;
      } else {
        g_mpHostVersion=0L;
      }
      if (g_hoLoginWnd)
        PostMessage(g_hoLoginWnd,WM_USER_LOGIN_IDENTIFIED,0,0L);
      break;
    case MOLE_CMD_SYSD:
      /* This is only used during login - let's get out of login mode */
      unNewIden(l_p,l_dataLeft);
      g_hoConnectionState=HO_CONNECT_STATE_ONLINE;
      if (g_hoLoginWnd)
        PostMessage(g_hoLoginWnd,WM_USER_LOGIN_COMPLETE,0,0L);
      break;
    case MOLE_CMD_ALST:
//      dbPrint("Oh cool, an area listing!");
      unNewAreaList(l_p,l_dataLeft);
      break;
    case MOLE_CMD_WLST:
//      dbPrint("whoah! A world list!");
      unNewWorldList(l_p,l_dataLeft);
      break;
    case MOLE_CMD_MLST:
//      dbPrint("Hey, like, 69! A Mobile list!");
      unNewMobileList(l_p,l_dataLeft);
      break;
    case MOLE_CMD_OLST:
//      dbPrint("Party on! An Object list!");
      unNewObjectList(l_p,l_dataLeft);
      break;
    case MOLE_CMD_ADTL:
      unNewAreaDetail(l_p,l_dataLeft);
      break;
    case MOLE_CMD_WDTL:
//      dbPrint("Bazam! Worlds-o-rama.");
      unNewWorld(l_p,l_dataLeft);
      break;
    case MOLE_CMD_MDTL:
//      dbPrint("Oo-oo! a MOBILE!");
      unNewMobile(l_p,l_dataLeft);
      break;
    case MOLE_CMD_ODTL:
//      dbPrint("Objects R Us - new one!");
      unNewObject(l_p,l_dataLeft);
      break;
    case MOLE_CMD_RLST:
//      dbPrint("Reset list!!!!!");
      unNewResetList(l_p,l_dataLeft);
      break;
    case MOLE_CMD_PLST:
    case MOLE_CMD_RLRQ:
    case MOLE_CMD_MDRQ:
    case MOLE_CMD_MLRQ:
    case MOLE_CMD_OLRQ:
    case MOLE_CMD_WLRQ:
    case MOLE_CMD_ADRQ:
    case MOLE_CMD_ALRQ:
    case MOLE_CMD_PLRQ:
    case MOLE_CMD_IDRQ:
//    case MOLE_CMD_ECRQ: /* we don't support this in the client */
//    case MOLE_CMD_ECHO: /* we ignore these packets */
    case MOLE_CMD_MORE:
    default: /* unrecognized or unimplemented command */
      break;
  } /* switch */

  /* Notify our Edit Management facility that a packet has arrived */
  edPacketHasArrived(l_pktID,p_Cmd,l_returnCode);

  /* now remove request from the RequestQ - it's been answered */
  /* NOTE this is an important step - any requests which are AHEAD
   * of this are assumed to be UNANSWERED (you see, commands are
   * responded to sequentially, so if we skip one, we can assume
   * it's request was not answered or properly transmitted */
  /* First, remove this request from the queue */
  l_nextReq=l_req->rNext;
  mpReqExtract(l_req,&g_mpRequestQ);
  mpReqFree(l_req);
  /* now re-submit all requests found LATER in the linked list - note
     that new requests are appended to the FRONT of the list */
  while(l_nextReq) {
    l_req=l_nextReq;
    l_nextReq=l_req->rNext;
    /* extract this request from the queue */
    mpReqExtract(l_req,&g_mpRequestQ);
    /* and re-submit this request */
    mpReqSubmit(l_req,&g_mpRequestQ);
  }
  /* and we're done */
  return;
}

/* This proc monitors the request Q and, if there's a request which has been
 * waiting too long for a reply, it re-submits it. */
void mpCheckTimeout() {
  MPREQUEST *l_req,*l_nextreq;
  time_t l_time;
  char l_buf[100];
  int l_i;

//  dbPrint("Let's check up on our RequestQ for timeout errors");
  for (l_i=0,l_req=g_mpRequestQ;l_req;l_req=l_req->rNext,l_i++) {
    l_buf[0]=(char)(((l_req->rCmd)&0xFF000000L)>>24);
    l_buf[1]=(char)(((l_req->rCmd)&0x00FF0000L)>>16);
    l_buf[2]=(char)(((l_req->rCmd)&0x0000FF00L)>>8);
    l_buf[3]=(char)(((l_req->rCmd)&0x000000FFL));
    l_buf[4]=' ';
//    sprintf(&(l_buf[5]),"timestamp: %lu",(unsigned long)l_req->rTime);
//    dbPrint(l_buf);
  }
//  sprintf(l_buf,"requests queued: %i",l_i);
//  dbPrint(l_buf);


  /* work from oldest request back */
  if (!g_mpRequestQ) return; /* nothing to do */
  for (l_req=g_mpRequestQ;l_req->rNext;l_req=l_req->rNext);

  /* we should point to the last request now */
  l_time=time(NULL);
//  sprintf(l_buf,"current time= %lu",(unsigned long)l_time);
//  dbPrint(l_buf);
  do {
    l_nextreq=l_req->rPrevious;
    /* check the time stamp on this sucker. */
    if ((l_time-(g_tmTimerPeriod/500))>l_req->rTime) {
      /* oooooo, this one is toooo old. In our impatience, resubmit the request */
      /* extract this request from the queue */
//      dbPrint("Request Re-Queued");
      mpReqExtract(l_req,&g_mpRequestQ);
      /* and re-submit this request */
      mpReqSubmit(l_req,&g_mpRequestQ);
    }
    /* and go on to the next (er, I mean PREVIOUS) request */
    l_req=l_nextreq;
  } while (l_req);
  return;
}
