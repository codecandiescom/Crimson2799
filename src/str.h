/* str.h */
struct StrType {
  BYTE             *sText; /* people who dont know about StrType should be able to treat as CHAR* */
  LWORD             sLen; /* length of this string (we hash against sLen) */
  LWORD             sNum; /* number of this string alloc'd */
  struct StrType   *sNext; /* next str in hash table */
};

#define STR_HASH_SIZE 1024

#define SLEN_UNKNOWN      -1
#define STR_PRIVATE       -1 /* private header - still a shared buffer! */
#define STR_UNHASHED      -2 /* completely unhashed and unshared */

#define HASH              TRUE
#define DONTHASH          FALSE

extern LWORD strMemTotal; /* monitor the muds current mem usuage due to strings */
extern BYTE *strNull;
extern STR  *sHashIndex[STR_HASH_SIZE]; /* the hash table */

extern STR  *StrAlloc(STR *str);   /* allocate a string and put it in hash table */
extern STR  *StrCreate(BYTE *str, LWORD sLen, BYTE sHash); /* allocate a string and put it in hash table */
extern STR  *StrPrivate(STR *str); /* get a private STR structure for this str */
extern STR  *StrGetPublic(STR *str); /* find hash entry/public string for a private one */
extern STR  *StrFree(STR *str);    /* free a string, if instances of a string hit zero really free it - return NULL*/
extern void  StrReHash(STR *str, LWORD newLen); /* probably should never be called now */
extern BYTE  StrIsKey(BYTE *key, STR *keyList);      /* is string a substr match of following keylist */
extern BYTE  StrIsExactKey(BYTE *key, STR *keyList); /* is key exact match of keylist */
extern BYTE *StrOneWord(BYTE *str, BYTE *word); /* returns str less word, copies word into word buffer */
extern BYTE *StrFirstWord(BYTE *str, BYTE *word); /* return the first word of str */
extern BYTE  StrAbbrev(BYTE *str, BYTE *substr); /* returns TRUE if substr matches first part of str */
extern BYTE  StrExact(BYTE *fullstr, BYTE *str); /* returns TRUE if case insensitive match */
extern BYTE *StrTruncate(BYTE *truncateStr, BYTE *str, LWORD maxLen); /* truncate a str if necessary */
extern BYTE *StrFind(BYTE *str, BYTE *strFind); /* find a substring */
extern void  StrToLower(BYTE *str); /* convert a string to all lower case */
extern void  StrToUpper(BYTE *str); /* convert a string to all upper case */
extern LWORD StrNumLine(STR *str);  /* find the # of lines in the string (# of CR's +1) */
extern BYTE  StrIsNumber(BYTE *str); /* returns True if the str is a number */
extern BYTE *StrTrim(BYTE *str); /* returns str less preceding spaces and trailing spaces/CR/LF */
extern BYTE  StrIsAlpha(BYTE *str); /* returns True if the str is all alpha ie no punctuation */
extern BYTE  StrIsAlNum(BYTE *str); /* returns True if the str is all alphanumeric ie no punctuation */
extern BYTE *StrRestoreEscape(BYTE *str); /* convert $$ back to $ */
extern BYTE *StrOneLine(BYTE *str); /* ensure str has no \n, terminate at first one */
extern LWORD StrEnd(BYTE *str, BYTE *pattern); /* returns TRUE if str ends in pattern */
extern LWORD StrSubToNumber(BYTE *str, LWORD digits); /* turns first # of digits from str as a number */
extern void  StrTrimCRLF(BYTE *str); /* whacks off any trailings cr's or lf's */
extern void  StrPath(BYTE *str);

/* str related macros */
#define      STRFREE(str) str=StrFree(str)     /* macro for freeing str's */
#define      STRCREATE(str) StrCreate(str, SLEN_UNKNOWN, HASH)
#define      STRPRIVATE(str) str=StrPrivate(str)     /* macro for freeing str's */
#define      STRDUP(a,b) if(b) a=strdup(b) else a=NULL /* improve later to log out of memory exception */
#ifdef WIN32
  #define      STRNICMP(a,b,c) strnicmp(a,b,c)
  #define      STRICMP(a,b) stricmp(a,b)
#else
  #define      STRNICMP(a,b,c) strncasecmp(a,b,c)
  #define      STRICMP(a,b) strcasecmp(a,b)
#endif

#define      Str(x) ((STR *) (x))

