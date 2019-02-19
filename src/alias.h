
#define ALIAS_BLOCK_SIZE 256
#define ALIAS_INDEX_SIZE 256


struct AliasType {
  STR   *aPattern;
  BYTE   aParmNum;
  STR   *aAction;
};

extern void      AliasInit(void);
extern ALIAS    *AliasAlloc(STR *pattern, BYTE parmNum, STR *action);
extern ALIAS    *AliasCreate(INDEX *aIndex, BYTE *pattern, BYTE *action);
extern ALIAS    *AliasFree(INDEX *aIndex, ALIAS *alias);
#define          ALIASFREE(aIndex, alias) AliasFree(aIndex,alias)
extern ALIAS    *AliasFind(INDEX *aIndex, BYTE *pattern);
extern BYTE      AliasParse(SOCK *sock, BYTE *cmd);
extern void      AliasRead(SOCK *sock);
extern void      AliasWrite(SOCK *sock);

#define ALIAS_COMMAND_STR "##ALIAS"

#define Alias(x) ((ALIAS*)x)
