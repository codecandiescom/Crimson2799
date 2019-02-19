#define LINE_MAX_LEN     256 /* the maximum length of one line of input */
#define LINE_MAX_HISTORY 5
#define SITE_MAX_NAME    256
#define REBOOT_FLAG_FILE "autoreboot.flag"
#define BOOT_FLAG_FILE   "boot.flag"


struct SockType {
  LWORD              sFD;
  ULWORD             sAddr;               /* IP address */
  BYTE               sSiteName[SITE_MAX_NAME]; /* equal to the client machines name */
  BYTE               sSiteIP[16];         /* IP address as text ie "127.0.0.1" */
  BYTE               sSiteType;
  FLAG               sPref;               /* are we Telnet GA, Linefeed, CR etc point to a default flag then player prefs*/
  BYTE               sFlag;
  Q                 *sIn;                 /* input buffer */
  Q                 *sOut;                /* output buffer */
  Q                 *sMole;               /* MOLE buffer */
  Q                 *sPersonal;           /* personal history, tells, says and such */
  BYTE               sSubMode;            /* allows editor to have its own modes */
  BYTE               sMode;               /* connection mode, menu, playing etc */
  THING             *sControlThing;       /* thing being controlled */
  THING             *sHomeThing;          /* who we really are */

  EDIT               sEdit;

  HISTORY            sHistory;            /* for ! command */
  WORD               sLastRepeat;         /* to prevent spamming */
  BYTE               sCurrentLines;       /* lines they have received so far */
  BYTE               sScreenLines;        /* # of lines per screen (for continue prompt) */

  INDEX              sAliasIndex;         /* user defined aliases */

  SOCK              *sNext;               /* next array entry in list */
};

/* time stuff for loop */
#define USEC_PER_SEG          200000 /* basic heartbeat 1/5 of a second */
#define SEG_PER_SOCKREPORT    9000   /* socket report every 30 minutes */
#define SEG_PER_RESETIDLE     10
#define SEG_PER_CHARIDLE      10
#define SEG_PER_OBJIDLE       10
#define SEG_PER_WLDIDLE       10
#define SEG_PER_TICK          300     /* 1 tick per minutes */
#define SEG_PER_FASTTICK      25      /* 1 fasttick per 5 seconds */
#define SEG_PER_FIGHT         5       /* One combat round every 1 seconds */
#define SEG_PER_WAIT          5       

#define MODE_LOGIN            0
#define MODE_PASSWORD         1
#define MODE_MAINMENU         2
#define MODE_PLAY             3

/* for making a new character */
#define MODE_CONFIRMNAME      4
#define MODE_CHOOSEANSI       5
#define MODE_CHOOSEEXPERT     6
#define MODE_CHOOSESEX        7
#define MODE_CHOOSERACE       8
#define MODE_CHOOSECLASS      9
#define MODE_SHOWCOMBO        10
#define MODE_ROLLABILITIES    11
#define MODE_KILLSOCKET       12

extern BYTE *sModeList[];

#define SUBM_NONE             0
#define SUBM_EDITMENU         1
#define SUBM_EDITINSERT       2
#define SUBM_EDITAPPEND       3
#define SUBM_EDITREPLACEFIND  4
#define SUBM_EDITREPLACEWITH  5
#define SUBM_CONTINUE         6
#define SUBM_NEWPASSWORD      7
#define SUBM_CONFIRMPASSWORD  8
#define SUBM_ENTEREMAIL       9

/* these are flags that are associated with the socket, not with the player
  important distinction since these must be set prior to a player being loaded */

#define SP_CR            1<<0  /* carriage returns */
#define SP_LF            1<<1  /* line feeds */
#define SP_CONTINUE      1<<2  /* prompt to continue if screen overflows */
#define SP_TELNETGA      1<<3  /* send telnet ga after every prompt */
#define SP_ANSI          1<<4  /* strip out ansi color strings unless this is set */
#define SP_PROMPTHP      1<<5  /* prompt shows hit points */
#define SP_PROMPTMV      1<<6  /* prompt shows move points left */
#define SP_PROMPTPP      1<<7  /* prompt shows power points */
#define SP_PROMPTEX      1<<8  /* prompt shows exits */
#define SP_PROMPTRM      1<<9  /* prompt shows room number */
#define SP_COMPACT       1<<10 /* take out blank line preceding prompts */
#define SP_CHANPORT      1<<11 /* port binds etc */
#define SP_CHANERROR     1<<12 /* critical errors - memory issues etc */
#define SP_CHANUSAGE     1<<13 /* sign-on/off, level gain, death - joe user gets this as a channel */
#define SP_CHANGOD       1<<14 /* logged god commands */
#define SP_CHANAREA      1<<15 /* logged area editing commands - zonelords get this as an option */
#define SP_CHANSPY       1<<16 /* watch players that are logged */
#define SP_CHANGOSSIP    1<<17 /* watch players that are logged */
#define SP_CHANAUCTION   1<<18 /* watch players that are logged */
#define SP_CHANGROUP     1<<19 /* watch players that are logged */
#define SP_CHANPRIVATE   1<<20 /* watch players that are logged */
#define SP_CHANTICK      1<<21 /* Watch ticks occur */
#define SP_CHANBOARD     1<<22 /* Watch ticks occur */

/* system socket flags */
#define SF_PENDINGWRITE  1<<0 /* write blocked, more pending - system bit only */
#define SF_ATPROMPT      1<<1 /* for prompt management - internal system bit */
#define SF_XLATE         1<<2 /* color translation/stripping */
#define SF_ATCONTINUE    1<<3 /* for More(?) Return to Continue */
#define SF_ATEDIT        1<<4 /* in the middle of something block input */
#define SF_PENDINGREAD   1<<5 /* read blocked, ie Wait was set - system bit only */
#define SF_RECONNECT     1<<6 /* this socket is attempting to reconnect to someone allready online - system bit only */
#define SF_MORE          1<<7 /* last read of sIn overflow'd buffer */
#define SF_1BADPW        1<<8 /* bad password */


/* externally accessed vars */
extern LWORD  sockPortNum;  /* telnet to this port # , set in INI file */
extern LWORD  sockDefFlag;
extern LWORD  crimsonRun;
extern ULWORD startTime; /* time started according to time(0) */
extern ULWORD tmSegment; /* total segments mud has been running */
extern SOCK *sockList;
extern BYTE *sfPrefList[];
extern Q    *sockOverflow;

extern void SockWrite(SOCK *sock);

