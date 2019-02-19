// unpack.h
// two-char identifier: un

/* error codes for arGet* routines */
#define UN_GET_BADINPUT (-1)  /* null pointer or something */
#define UN_GET_OK        (0)  /* everything is hunky-dorey */
#define UN_GET_NODATA    (1)  /* not enough data */
#define UN_GET_STRLEN    (2)  /* string isn't long enough */
#define UN_GET_SYSTEM    (3)  /* system error */
/* Control for arGet* */
#define UN_GET_BYTE   (1)
#define UN_GET_SBYTE  (2)
#define UN_GET_WORD   (3)
#define UN_GET_UWORD  (4)
#define UN_GET_LWORD  (5)
#define UN_GET_ULWORD (6)
#define UN_GET_FLAG   (7)

void unNewIden(char *p_data,unsigned long p_datalen);
void unNewAreaList(char *p_data,unsigned long p_datalen);
void unNewWorldList(char *p_data,unsigned long p_datalen);
void unNewMobileList(char *p_data,unsigned long p_datalen);
void unNewObjectList(char *p_data,unsigned long p_datalen);
void unNewWorld(char *p_data,unsigned long p_datalen);
void unNewMobile(char *p_data,unsigned long p_datalen);
void unNewObject(char *p_data,unsigned long p_datalen);
void unNewAreaDetail(char *p_data,unsigned long p_datalen);
void unNewResetList(char *p_data,unsigned long p_datalen);
int unGetStrAlloc(char **p_data,unsigned long *p_datalen,DSSTR *p_str);
int unGetString(char **p_data,unsigned long *p_datalen,char *p_dest,unsigned long p_destlen);
int unGetInteger(char **p_data,unsigned long *p_datalen,void *p_dest,int p_destType);

