/* Dynamic Structure header file */
// 2-char identifier: ds

#ifndef __DSTRUCT_H__
#define __DSTRUCT_H__

/* structure types */
#define DS_STYPE_NONE         0
#define DS_STYPE_EXIT         1
#define DS_STYPE_EXTRA        2
#define DS_STYPE_PROPERTY     3
#define DS_STYPE_AREADETAIL   4
#define DS_STYPE_RESET        5
#define DS_STYPE_LIST         6 /* ALL list sub-types must come AFTER this type!! */
#define DS_STYPE_AREA         7
#define DS_STYPE_WORLD        8
#define DS_STYPE_MOBILE       9
#define DS_STYPE_OBJECT      10

/* Network states */
#define DS_NS_NONE        0 /* data does not exist */
#define DS_NS_NOTAVAIL    1 /* data exists but hasn't been fetched yet */
#define DS_NS_PENDING     2 /* data exists and a request has been sent - awaiting return */
#define DS_NS_AVAIL       3 /* data exists and is available presently */

/* Flags */
#define DS_FLAG_WINOPEN   (1<<0) /* 'open' in the hierarchy window */

/* constants */
#define DS_FTL_NAMELEN 50
#define DS_OBJECT_MAX_APPLY 4 /* This must correspond to the server */

/* FLAG Macros */
#define DSBITSET(flag,bit)    (flag)|=(bit)
#define DSBITCLEAR(flag,bit)  (flag)=((flag)|(bit))^(bit)
#define DSBITFLIP(flag,bit)   (flag)^=(bit)
#define DSISBITSET(flag,bit)  ((flag)&(bit))

/* Structure pre-definitions */
struct DSREFtag;
struct DSSTRtag;
struct DSSTRUCTtag;
struct DSSLISTtag;

/***** Master structures *****/

/* types compatible with server */
#ifdef WIN32
typedef unsigned char   DSBYTE;
typedef char            DSSBYTE;
typedef short           DSWORD;
typedef unsigned short  DSUWORD;
typedef int             DSLWORD;
typedef unsigned int    DSULWORD;
typedef unsigned int    DSFLAG;
#else
typedef unsigned char   DSBYTE;
typedef char            DSSBYTE;
typedef int             DSWORD;
typedef unsigned int    DSUWORD;
typedef long            DSLWORD;
typedef unsigned long   DSULWORD;
typedef unsigned long   DSFLAG;
#endif

/* reference structure - all references pieces of data are done through this */
typedef struct DSREFtag {
  int rState;
  unsigned long rLoadTime; /* time structure was received from host */
  unsigned long rFlag; /* flags */
  struct DSSTRUCTtag *rList; /* list of data objects */
  HWND rEditWindow; /* used by some editing functions (ie: reset )*/
} DSREF;

/* String structure */
typedef struct DSSTRtag {
  char *sData; /* pointer to data buffer */
  HGLOBAL sMemory; /* memory handle for structure */
} DSSTR;

/* flag/type List structure */
typedef struct DSFTLtag { /* I called it this to avoid confusion with DSSLIST */
  char fName[DS_FTL_NAMELEN];
  HGLOBAL fMemory;
  struct DSFTLtag *fNext,*fDown;
  int fType;
  int fList;
} DSFTL;

/* generic structure type */
typedef struct DSSTRUCTtag {
  int sType;       /* type of structure */
  int sState;      /* network state */
  unsigned long sLoadTime; /* time structure was received from host */
  HWND sEditWindow; /* window currently open to edit this structure */
  HGLOBAL sMemory; /* memory handle for this structure */
  struct DSSTRUCTtag *sPrevious,*sNext; /* links to prev, next, and "up" structures */
  struct DSREFtag *sUpRef; /* reference to upper reference */
} DSSTRUCT;

typedef struct DSSEXTRAtag {
  DSSTRUCT  eStruct;
  DSSTR     eKey;
  DSSTR     eDesc;
} DSSEXTRA;

typedef struct DSSPROPERTYtag {
  DSSTRUCT  pStruct;
  DSSTR     pKey;
  DSSTR     pDesc;
} DSSPROPERTY;

typedef struct DSSEXITtag {
  DSSTRUCT  eStruct;
  DSSTR             eKey;
  DSSTR             eDesc;
  DSLWORD           eKeyObj;
  DSFLAG            eFlag;
  DSLWORD           eWorld; /* virtual # of the destination world */
  DSBYTE            eDir;
} DSSEXIT;

/* holds all area details / settings */
typedef struct DSSAREADETAILtag {
  DSSTRUCT aStruct;
  DSSTR                  aEditor;
  DSSTR                  aDesc;
  DSFLAG                 aResetFlag;
  DSWORD                 aResetDelay;
  DSREF                  aProperty;   /* from aResetThing, reset scripts */
} DSSAREADETAIL;

/* holds RST data */
typedef struct DSSRESETtag {
  DSSTRUCT               rStruct;
  DSBYTE                 rCmd;
  DSWORD                 rIf;

  DSLWORD                rArg1;
  DSLWORD                rArg2;
  DSLWORD                rArg3;
  DSLWORD                rArg4;
} DSSRESET;

/***** Special DSS sub-structures *****/
/* DSSLIST is for character string lists: */
/* Area list
 * WLD lists
 * OBJ lists
 * MOB lists
 * etc. */
typedef struct DSSLISTtag {
  DSSTRUCT  lStruct;
  DSSTR    lName;
  long int lVNum,lVNum2; /* general purpose virtual number storage */
} DSSLIST;


/***** Standard DSS sub-structures *****/
typedef struct DSSAREAtag {
  DSSLIST aList;
  unsigned long aFlag; /* flags */
  DSREF aOBJ,aMOB,aWLD,aRST,aDetail; /* area lists */
} DSSAREA;

/* holds WLD data */
typedef struct DSSWORLDtag {
  DSSLIST wList;

//  DSSTR                  wSDesc; /* lName */
  DSSTR                  wDesc;
  DSREF                  wExit,wExtra,wProperty;

  DSFLAG                 wFlag;
  DSWORD                 wType;

} DSSWORLD;

/* holds MOB data */
typedef struct DSSMOBILEtag {
  DSSLIST mList;
  /* NOTE: mSDesc, the mob's name, is stored in the lName str */
  /* and the mVirtual is stored in the lVNum field.           */
  /* The following are copied directly from the server's code.*/

  /* special (alloc-able) parts */
  DSSTR                  mKey;
//  DSSTR                  mSDesc; /* lName */
  DSSTR                  mLDesc;
  DSSTR                  mDesc;
  DSREF                  mExtra,mProperty;

  /* normal parts */
  DSFLAG                 mAct;
  DSFLAG                 mAffect;
  DSWORD                 mAura;
  DSWORD                 mLevel;
  DSWORD                 mHitBonus;
  DSWORD                 mArmor;
  DSWORD                 mHPDiceNum;
  DSWORD                 mHPDiceSize;
  DSWORD                 mHPBonus;
  DSWORD                 mDamDiceNum; /* unarmed damage */
  DSWORD                 mDamDiceSize;
  DSWORD                 mDamBonus;
  DSLWORD                mMoney;
  DSLWORD                mExp;
  DSBYTE                 mPos; /* Position default */
  DSBYTE                 mType;/* Type information ie monster race */
  DSBYTE                 mSex;

  DSWORD                 mWeight;
} DSSMOBILE;

/* holds OBJ data */
typedef struct DSSOBJECTtag {
  DSSLIST oList;

  DSSTR                  oKey;
//  DSSTR                  oSDesc; /* lName */
  DSSTR                  oLDesc;
  DSSTR                  oDesc;
  DSREF                  oExtra,oProperty;

  DSBYTE                 oType;
  DSFLAG                 oAct;
  DSBYTE                 oWear;
  DSWORD                 oWeight;
  DSLWORD                oValue;
  DSLWORD                oRent;

  /* Details */
  DSLWORD                oDetail[16];

  /* Apply Information */
  DSBYTE                 oApplyType[DS_OBJECT_MAX_APPLY];
  DSSBYTE                oApplyValue[DS_OBJECT_MAX_APPLY];

} DSSOBJECT;

/* Structure type-casting macros */
#define DSStruct(x) ((DSSTRUCT *)(x))
#define DSSList(x) ((DSSLIST *)(x))
#define DSSArea(x) ((DSSAREA *)(x))
#define DSSAreaDetail(x) ((DSSAREADETAIL *)(x))
#define DSSMobile(x) ((DSSMOBILE *)(x))
#define DSSObject(x) ((DSSOBJECT *)(x))
#define DSSWorld(x) ((DSSWORLD *)(x))
#define DSSReset(x) ((DSSRESET *)(x))
#define DSSExtra(x) ((DSSEXTRA *)(x))
#define DSSProperty(x) ((DSSPROPERTY *)(x))
#define DSSExit(x) ((DSSEXIT *)(x))
#define DSStr(x) ((DSSTR *)(x))
#define DSRef(x) ((DSREF *)(x))

void dsRefFree(DSREF *p_ref);
void dsRefClear(DSREF *p_ref);
void dsStrFree(DSSTR *p_str);
void dsStrClear(DSSTR *p_str);
unsigned long dsStrAlloc(DSSTR *p_str,unsigned long p_len);
void dsStrCopy(DSSTR *p_src,DSSTR *p_dst);
unsigned long dsStrReplicate(DSSTR *p_src,DSSTR *p_dst);
void dsStructFree(DSSTRUCT *p_struct);
DSSTRUCT *dsStructAlloc(int p_type);
void dsStructExtract(DSSTRUCT *p_struct);
void dsStructInsert(DSREF *p_upref, DSSTRUCT *p_prev, DSSTRUCT *p_struct);
void dsStructInsertList(DSREF *p_upref, DSSTRUCT *p_struct);
void dsStructCopy(DSSTRUCT *p_src,DSSTRUCT *p_dst);
DSSTRUCT *dsChangeListType(DSSTRUCT *p_old, int p_newtype);
unsigned long dsStrCreate(DSSTR *p_str,char *p_data);
DSFTL *dsFTLAlloc(char *p_name);
void dsFTLFree(DSFTL **p_FTList);
void dsFTLAppendNext(DSFTL **p_FTList,DSFTL *p_newEntry);
void dsFTLAppendDown(DSFTL **p_FTList,DSFTL *p_newEntry);
DSFTL *dsFTLOf(DSFTL **p_FTList,int p_FTLNum);

#endif /* __DSTRUCT_H__ */
