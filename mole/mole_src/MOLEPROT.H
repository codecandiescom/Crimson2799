#include<time.h>

/* MPREQUEST Special ID tags */
//#define MP_REQUESTID_NULL  (0L)
//#define MP_REQUESTID_START (1L) /* first available general-purpose ID */

typedef struct MPBUFtag {
  char *bData; /* pointer to memory data */
  unsigned long bLen; /* size of bData buffer */
  HGLOBAL bMemory; /* global handle of memory */
  unsigned long bPos; /* current position in buffer */
} MPBUF;

typedef struct MPREQUESTtag {
  unsigned long rCmd;  /* command for this request */
  unsigned long rID;   /* ID for this request */
  HGLOBAL rMemory;     /* global handle for this structure's memory */
  char *rData;         /* pointer to data */
  unsigned long rDataLen; /* # bytes of data in rData */
  HGLOBAL rDataMemory; /* global handle for data memory */
  time_t rTime;        /* time request was sent - for timeout & retransmission purposes */
  struct MPREQUESTtag *rNext,*rPrevious;
} MPREQUEST;

extern unsigned long g_mpIDPool; /* contains our next available packet ID */
extern MPBUF g_mpPktBuf; /* intermediate buffer to collect a single complete mole packet */
extern MPBUF g_mpCmdBuf; /* intermediate buffer to collect a packet's data after formatting and checksum checks have verified packet as valid */
extern MPREQUEST *g_mpRequestQ;
extern char *g_mpSendBuffer; /* buffer for send characters - formatted & mixed mole/normal - HUGE and STATIC */
extern HGLOBAL g_mpSendBufferMemory;
extern unsigned long g_mpSendBufferSize;
extern char *g_mpSendBufferHead,*g_mpSendBufferTail;
extern unsigned long g_mpHostVersion; /* contains host MOLE and Crimson2 versions */

BOOL mpBufAlloc(MPBUF *p_buf,unsigned long p_len);
void mpBufFree(MPBUF *p_buf);
BOOL mpBufReAlloc(MPBUF *p_buf,unsigned long p_len);
void mpBufAppend(MPBUF *p_buf,char *p_data,unsigned long p_datalen);
void mpBufFlush(MPBUF *p_buf);
void mpReqFree(MPREQUEST *p_req);
void mpReqSubmit(MPREQUEST *p_req,MPREQUEST **p_list);
void mpReqExtract(MPREQUEST *p_req,MPREQUEST **p_list);
MPREQUEST *mpReqAlloc(unsigned long p_Cmd,char *p_Data,unsigned long p_DataLen);
void mpReqFlush(MPREQUEST **p_list);
void mpSendBufferAppend(char *p_buf,unsigned int p_len);
void mpSendBufferFlush(void);
unsigned long mpSendBufferSize(void);
unsigned long mpSendBufferRead(char *p_buf,unsigned long p_buflen);
unsigned long mpSendBufferUnRead(char *p_buf,unsigned long p_buflen);
BOOL mpInitMoleProt(void);
void mpShutdownMoleProt(void);
void mpMolePacketFilter(char *p_buf,unsigned long p_buflen);
void mpStripPkt(MPBUF *p_pkt);
void mpMoleSendData(void);
unsigned long mpDecodeULWORD(char *p_buf);
int mpEncodeULWORD(char *p_buf,unsigned long p_data);
void mpProcessPkt(unsigned long p_Cmd, MPBUF *p_Data);
void mpCheckTimeout(void);
void mpWriteBinToText(char *p_buf,char p_data);
unsigned long mpReqSubmitEx(unsigned long p_Cmd,char *p_Data,unsigned long p_DataLen);
unsigned long mpReqSubmitNum(unsigned long p_Cmd,unsigned long p_Data);

