/* social.h */

typedef struct SocialType {
  STR   *sName;
  WORD   sPos;
  STR   *sNullSrc;
  STR   *sNullRoom;
  STR   *sCharSrc;
  STR   *sCharDst;
  STR   *sCharRoom;
  STR   *sObjSrc;
  STR   *sObjRoom;
} SOCIAL;

#define SOCIAL_INDEX_SIZE 512
#define SOCIAL_ALLOC_SIZE 128

/* Global List of all social entries for search purposes */
extern INDEX     socialIndex;

extern INDEXPROC(SocialCompareProc);
extern INDEXFINDPROC(SocialFindProc);

extern void    SocialInit(void);
extern void    SocialRead();
extern SOCIAL *SocialFind(BYTE *key);
extern BYTE    SocialParse(THING *thing, BYTE *cmd);

#define Social(x) ((SOCIAL*)(x))
