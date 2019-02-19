#define SITE_INDEX_SIZE   256
#define SITE_BLOCK_SIZE   512
#define SITE_FILE         "lib/sitelist.tbl"

typedef struct SiteType {
  BYTE      sSite[SITE_MAX_NAME];/* site name or suffix */
  BYTE      sType;               /* local,banned etc */
  LWORD     sDate;               /* date banned */
  STR      *sPlayer;             /* player who banned this site */
} SITE;

#define SITE_OFFSITE 0
#define SITE_LOCAL   1
#define SITE_BANNED  2
#define SITE_BANNEW  3

extern INDEX     siteIndex;
extern BYTE     *sTypeList[];

extern void      SiteInit(void);
extern SITE     *SiteAlloc(BYTE *siteName, BYTE type, LWORD date, STR *player);
extern SITE     *SiteCreate(THING *thing, BYTE *siteName, BYTE type);
extern SITE     *SiteFree(SITE *site);
#define          SITEFREE(site) site=SiteFree(site)
extern SITE     *SiteFindExact(BYTE *key);
extern SITE     *SiteFind(BYTE *key);
extern BYTE      SiteGetType(SOCK *sock);
extern void      SiteRead(void);
extern void      SiteWrite(void);

#define Site(x) ((SITE*)x)
