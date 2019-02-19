/* MOLE Server Identifier String */
#define MOLE_SERVER_NAME "Crimson2799 MUD"

#define GQD_BINARY (0<<0)
#define GQD_STRING (1<<0)

#define MOLEGetQLWORD(sock,lword)   MOLEGetQULWORD(sock,(ULWORD*)lword)

extern void   MOLESend(SOCK *sock, BYTE* cmd, LWORD cmdLen, ULWORD moleCmd);
extern LWORD  MOLEParse(SOCK *sock, BYTE *cmd, ULWORD *moleCmd);
extern void   MOLEFlushQ(SOCK *sock);
extern LWORD  MOLEGetQSize(SOCK *sock);
extern LWORD  MOLEGetQData(SOCK *sock, BYTE *buf, ULWORD *bufLen,FLAG flag);
extern LWORD  MOLEGetQDataCmd(SOCK *sock, ULWORD *pktCmd);
extern LWORD  MOLEGetQStr(SOCK *sock, STR **data);
extern LWORD  MOLEGetQULWORD(SOCK *sock, ULWORD *data);
extern LWORD  MOLEWriteULWORD(SOCK *sock, ULWORD ulWord, BYTE *dstBuf, ULWORD *bufPos);
extern LWORD  MOLEWriteBuf(SOCK *sock, BYTE *srcBuf, BYTE *dstBuf, ULWORD *bufPos);
extern LWORD  MOLEWriteList(SOCK *sock, ULWORD list, LWORD listSize, BYTE *dstBuf,ULWORD *bufPos);
extern LWORD  MOLECommandCheck(SOCK *sock, BYTE *cmd, LWORD virtual);

#define MOLEWRITELIST(sock,list,dstBuf,bufPos) MOLEWriteList(sock,(ULWORD)list,sizeof(*list),dstBuf,bufPos)
