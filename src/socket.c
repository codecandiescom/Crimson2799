/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary. Use in whole or in part without
 * explicity permission by the author is strictly prohibited
 *
 * Current email address(es): cryogen@infoserve.net
 * Phone number: (604) 591-5295
 *
 * C4 Script Language written/copyright Cam Lesiuk 1995
 * Email: clesiuk@engr.uvic.ca
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>    /* for memset et al */
#include <ctype.h>      /* for memset et al */
#include <signal.h>     /* for signal, SIGPIPE etc */
#include <sys/types.h>  /* for set sockopt etc */
#include <time.h>
#ifndef WIN32
  #include <sys/time.h>   /* for select/timezone etc */
  #include <sys/resource.h> /* for getrlimit */
  #include <sys/socket.h>
  #include <sys/utsname.h> /* for uname */
  #include <sys/param.h> /* for uname */
  #include <netinet/in.h>
  #include <unistd.h>     /* for close */
  #include <netdb.h>      /* for gethostbyaddr */
#ifndef LINUX
  #include <crypt.h>
#endif
  #define CRYPT(s,k) crypt(s,k)
  #define CLOSE(s) close(s)
  #define WRITE(s, buf, bLen) write(s, buf, bLen)
  #define READ(s, buf, maxLen) read(s, buf, maxLen)
#else
  #define CRYPT(s,k) s
  #define CLOSE(s) closesocket(s)
  #define WRITE(s, buf, bLen) send(s, buf, bLen, 0)
  #define READ(s, buf, maxLen) recv(s, buf, maxLen, 0)
#endif
/* FD_SETSIZE is 256 by default, if you change it
  RIGHT HERE!!!! then you can increase it to allow 
  more simultaneous open connections */
#include <fcntl.h>      /* for nonblocking io */

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "ini.h"
#include "mem.h"
#include "queue.h"
#include "timing.h" /* for Tv routines */

/* because send.h also contains routines to send to THINGS we need the full set of includes here */
#include "str.h"
#include "extra.h"
#include "code.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h" /* this can point to things */
#include "site.h"
#include "alias.h"
#include "base.h"   /* has links that point to sockets */
#include "effect.h"
#include "object.h"
#include "board.h"
#include "char.h"
#include "mobile.h"
#include "skill.h"
#include "player.h"
#include "file.h"
#include "social.h"
#include "help.h"
#include "exit.h"
#include "world.h"
#include "reset.h"
#include "area.h"
#include "fight.h"
#include "send.h"
#include "parse.h"
#include "cmd_move.h"
#include "cmd_talk.h"
#ifdef WIN32
  /* including winsock.h etc auto includes the windows types */
  /* turn off warning: benign type redefinition */
  #pragma warning( disable : 4142 )
  #include <winsock.h>
  #pragma warning( default : 4142 )
  #include "winsockext.h"
#endif

/* system coding stuff */
#define SOCK_DISABLE_TIMEOUT 0
#define OBJECT_IDLE_NOCODE   0
#define USE_RLIMIT_OFILE     0 /* Linux doesnt support this */
#define USE_ALARM
#define ILOOP_CHECK_DELAY    99 /* check for an Infinite loop situation every this many seconds */

/* data init */
LWORD  sockPortNum; /* port number to bind to, telnet here (from INI file 4000 def)! */
WORD   sockReport = TRUE;

LWORD  crimsonRun = TRUE;
ULWORD startTime;
ULWORD tmSegment = 0; /* unsigned long int */

/* sFlag will be this by default *PLUS* other flags are controlled by INI file */
LWORD sockDefFlag = SP_CHANGOSSIP
                  | SP_CHANUSAGE
                  | SP_CHANGROUP
                  | SP_CHANPRIVATE
                  | SP_PROMPTHP;

BYTE *sModeList[] = {
  "LOGIN",
  "PASSWORD",
  "MAINMENU",
  "PLAYING",
  "CONFIRMNAME",
  "CHOOSEANSI",
  "CHOOSEEXPERT",
  "CHOOSESEX",
  "CHOOSERACE",
  "CHOOSECLASS",
  "SHOWCOMBO",
  "ROLLABILITIES",
  "KILLSOCKET",
  ""
};

BYTE *sfPrefList[] = {
  "CR",
  "LF",
  "CONTINUE",
  "TELNETGA",
  "ANSI",
  "PROMPTHP",
  "PROMPTMV",
  "PROMPTPP",
  "PROMPTEX",
  "PROMPTRM",
  "COMPACT",
  "CHANPORT",
  "CHANERROR",
  "CHANUSUAGE",
  "CHANGOD",
  "CHANAREA",
  "CHANSPY",
  "CHANGOSSIP",
  "CHANAUCTION",
  "CHANGROUP",
  "CHANPRIVATE",
  "CHANTICK",
  ""
};


LWORD sockMaxNum;
LWORD sockMaxOffsite;
LWORD sockNumOffsite = 0;
LWORD sockNum = 0; /* current number of socket entries used */
SOCK *sockList = NULL; /* array of socket structs */

Q    *sockOverflow;

/* Kill Signal Callback */
void KillSignalProc(int i) {
  Log(LOG_ERROR, "Killed by console. Shutting Down\n");
  crimsonRun = FALSE;
}

/* Infinite Loop Monitor Signal Callback */
void AlarmSignalProc(int i) {
  Log(LOG_ERROR, "Killed by Infinite Loop Monitor. Shutting Down\n");
  printf("Dying a horrible death....\n");
  abort(); /* abort and generate a core dump */
  /* abort doesnt work under linux? - or at least no core dump */
  *((BYTE*)0x0FFFFFFFF)=0; /* I want me a core dump... She's a going down Ma... */
  exit(1);
}

/* init socket data and open up mother connection */
LWORD SockInit(LWORD port) {
#define LISTEN_MAX_BACKLOG 5

  LWORD              portFD; /* socket descriptor */
  struct sockaddr_in portAddr; /* internet port address */
  LWORD              error;
  int                optval;
  BYTE               buf[256];

#ifndef WIN32

  /* broken connections will signal us with a SIGPIPE, but since
     I could really care less just ignore it
     note: if you dont do this SIGPIPE will kill process: ouch! */
  signal(SIGPIPE,   SIG_IGN);
 
  /* get the password encryption key */
  if (!getenv("CRIMSON2CRYPTKEY")) {
    Log(LOG_ERROR, "There is no password encryption key defined!\n");
    Log(LOG_ERROR, "define an environment variable CRIMSON2CRYPTKEY\n");
    Log(LOG_ERROR, "Typically this involves setting lines like:\n");
    Log(LOG_ERROR, "CRIMSON2CRYPTKEY='someencryptionkeytext'\n");
    Log(LOG_ERROR, "export CRIMSON2CRYPTKEY\n");
    Log(LOG_ERROR, "or\n");
    Log(LOG_ERROR, "setenv CRIMSON2CRYPTKEY someencryptionkeytext\n");
    Log(LOG_ERROR, "in your .kshrc, .bashrc, .cshrc, or .tshrc (or whatever you use) file\n");
    Log(LOG_ERROR, "This way nobody should be able to find out the players passwords by\n");
    Log(LOG_ERROR, "reading the players file. UNLESS YOU GIVE THEM THE KEY!\n");
    EXIT(1);
  }

  #if (USE_RLIMIT_OFILE)
    struct rlimit      rLimit;

    /* resource limits */
    Log(LOG_BOOT, "Resource Settings:\n");
    error = getrlimit(RLIMIT_CORE, &rLimit);
    rLimit.rlim_cur = INILWordRead("crimson2.ini", "coreSize", RLIM_INFINITY);
    if (rLimit.rlim_cur == -1) rLimit.rlim_cur = RLIM_INFINITY;
    rLimit.rlim_max = rLimit.rlim_max;
    error = setrlimit(RLIMIT_CORE, &rLimit);
    error = getrlimit(RLIMIT_CORE, &rLimit);
    if (!error)
      sprintf(buf, "Core Dump Size: %d / %d (cur/max)\n", rLimit.rlim_cur, rLimit.rlim_max);
    else
      sprintf(buf, "Unable to determine RLIMIT_CORE setting\n");
    Log(LOG_BOOT, buf);

    error = getrlimit(RLIMIT_OFILE, &rLimit);
    if (!error)
      sprintf(buf, "Open Files : %d / %d (Resident Set Size)\n", rLimit.rlim_cur, rLimit.rlim_max);
    else
      sprintf(buf, "Unable to determine RLIMIT_OFILE setting\n");
    Log(LOG_BOOT, buf);
  #endif
#else
  WORD               wVersionRequested;
  WSADATA            wsaData;

  wVersionRequested = MAKEWORD( 1, 1 );
  error = WSAStartup( wVersionRequested, &wsaData );
  if ( error ) {
    printf("WSA Startup failed: Ensure that Winsock is installed\n");
    PSOCKETERROR("SockInit");
    EXIT(ERROR_SOCKET);
  }
  
  /* Confirm that the Windows Sockets DLL supports 1.1.*/
  if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 ) {
    WSACleanup( );
    printf("Invalid version of Windows Sockets\n");
    EXIT(ERROR_SOCKET);
  }
#endif

  /* open up the socket */
  portFD = socket(AF_INET, SOCK_STREAM, 0);
  if (portFD < 0) {
    Log(LOG_BOOT, "SockInit failed to open socket\n");
    PSOCKETERROR("SockInit");
    EXIT(ERROR_SOCKET);
  }

  /* set reuseaddr sock opt */
  optval = TRUE;
  error = setsockopt(portFD, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof(optval));
  if (error) { /* will return -1 on error */
    Log(LOG_BOOT, "SockInit failed to REUSEADDR\n");
    PSOCKETERROR("SockInit");
    EXIT(ERROR_SOCKET);
  }

#ifndef WIN32
  /* Not all implementations of win 32 support this */

  /* set keepalive on, this instructs to the OS to periodically send a "are
   * you still there" message to the telnet client, and if a reply is not
   * recieved to kill the connection.... Vanilla diku did not do this and
   * as such periodically suffered from people unable to reconnect to a
   * broken session because the server didnt thing the connection was broken.
   * I'm unsure what the network traffic implications of this are.  
   */
  optval = TRUE;
  error = setsockopt(portFD, SOL_SOCKET, SO_KEEPALIVE, (void*)&optval, sizeof(optval));
  if (error) { /* will return -1 on error */
    Log(LOG_BOOT, "SockInit failed to KEEPALIVE\n");
    PSOCKETERROR("SockInit");
    EXIT(ERROR_SOCKET);
  }

/*
 * Set a linger delay, this will allow a connection to get the data we
 * have sent (ie the Ciao screen) after the connect has been terminated
 * (otherwise it drops unsent data on the floor after the terminate)
 *
 * Winsock tries to flush data itself so this is unnecessary for WIN32
 * WIN32 also gives a socket error if you close on a linger'ing socket
 */
{
  struct linger      sockLinger;

  sockLinger.l_onoff = TRUE;
  sockLinger.l_linger = 1000; /* dont even know what units these are in */
  error = setsockopt(portFD, SOL_SOCKET, SO_LINGER, (void*)&sockLinger, sizeof(sockLinger));
  if (error) { /* will return -1 on error */
    Log(LOG_BOOT, "SockInit failed to LINGER\n");
    PSOCKETERROR("SockInit");
    EXIT(ERROR_SOCKET);
  }
}
#endif

  /* if you start getting lots of EWOULDBLOCK's increase rcv & snd timeouts */

  /* bind the socket to a port # so we can telnet to it */
  memset(&portAddr, 0, sizeof(portAddr));
  portAddr.sin_family = AF_INET;
  portAddr.sin_port = htons( port );
  error = bind(portFD, (struct sockaddr *)&portAddr, sizeof(portAddr));
  if (error) { /* will return -1 on error */
    Log(LOG_BOOT, "SockInit failed to bind\n");
    PSOCKETERROR("SockInit");
    EXIT(ERROR_SOCKET);
  }

  /* listen for new connects, specify how many connects may be outstanding */
  error = listen(portFD, LISTEN_MAX_BACKLOG);
  if (error) { /* will return -1 on error */
    Log(LOG_BOOT, "SockInit failed to listen\n");
    PSOCKETERROR("SockInit");
    EXIT(ERROR_SOCKET);
  }
  sprintf(buf, "Current SOCK size [%d]\n", sizeof(SOCK));
  Log(LOG_BOOT, buf);
  sockList = NULL; /* init the sock list */

  /* normally we would do an accept and wait for connects now but instead
     we will be doing selects so that we can wake up periodically and do
     creature actions etc. (select/accept takes place in the main loop) */


  /* Figure out what our comm session defaults are */
  INIRead("crimson2.ini", "sfCR", "0", buf);
  if (atoi(buf)>0)
    BITSET(sockDefFlag, SP_CR);

  INIRead("crimson2.ini", "sfLF", "0", buf);
  if (atoi(buf)>0)
    BITSET(sockDefFlag, SP_LF);

  INIRead("crimson2.ini", "sfTELNETGA", "0", buf);
  if (atoi(buf)>0)
    BITSET(sockDefFlag, SP_TELNETGA);

  INIRead("crimson2.ini", "sfANSI", "0", buf);
  if (atoi(buf)>0)
    BITSET(sockDefFlag, SP_ANSI);

  INIRead("crimson2.ini", "sfCONTINUE", "0", buf);
  if (atoi(buf)>0)
    BITSET(sockDefFlag, SP_CONTINUE);

  sprintf(buf, "Default SockFlags: ");
  FlagSprintf(buf+strlen(buf), sockDefFlag, sfPrefList, ' ', 256);
  strcat(buf,"\n");
  Log(LOG_BOOT, buf);

  /* read sockMaxNum and sockMaxOffsite */
  sockMaxNum = INILWordRead("crimson2.ini", "sockMaxNum", 50);
  sprintf(buf, "sockMaxNum = %ld\n", sockMaxNum);
  Log(LOG_BOOT, buf);

  sockMaxOffsite = INILWordRead("crimson2.ini", "sockMaxOffsite", 20);
  sprintf(buf, "sockMaxOffsite = %ld\n", sockMaxOffsite);
  Log(LOG_BOOT, buf);

  Log(LOG_BOOT, "Alloc'ing overflow Q\n");
  sockOverflow = QAlloc(512);

  /* Yay! we made it */
  Log(LOG_BOOT, "SockInit'd successfully\n");
  return portFD;
}


/* create a new socket (yay some players at last...) */
LWORD SockCreate(LWORD portFD) {
#define ACCEPT_ERROR -1

  struct hostent     *hostEnt;
  struct sockaddr_in  addr;
  int                 addrlen;
  SOCK               *thisSock;
  LWORD               sockFD;
  LWORD               error;
  BYTE                buf[256];
  BYTE                siteName[SITE_MAX_NAME];

  /* according to the dox all we gotta do is accept the connect, since accept
     returns the sockaddr as well.... does this work for all unixes? cuz circle
     merc and diku all do getsockname calls to determine the address in advance!
     */

  addrlen = sizeof(struct sockaddr_in);
  sockFD = accept(portFD, (struct sockaddr *)&addr, &addrlen);
  if (sockFD == ACCEPT_ERROR) { /* will return -1 on error */
    Log(LOG_PORT, "SockCreate failed! (accept returned error)\n");
    PSOCKETERROR("SockCreate");
    return FALSE;
  }
#ifdef WIN32
  {
    u_long optval = TRUE;
    error = ioctlsocket(sockFD, FIONBIO, &optval);
  }
#else
  error = fcntl(sockFD, F_SETFL, O_NONBLOCK);
#endif
  if (error) {
    CLOSE(sockFD);
    Log(LOG_PORT, "SockCreate failed (unable to nonblock sockFD)!\n");
    PSOCKETERROR("SockCreate");
    return FALSE;
  }
  /* perform DNS lookup to determine name of the calling address */
  hostEnt = NULL;
  hostEnt = gethostbyaddr((char *)&(addr.sin_addr), sizeof(addr.sin_addr), AF_INET);
  if (hostEnt) {
    strncpy(siteName, hostEnt->h_name, sizeof(siteName)-1);
    siteName[sizeof(siteName)-1] = '\0';
  } else
    siteName[0]=0;

  /* okay dokey now stick the puppy in our active socket descriptor table */

  if (sockNum+1 == sockMaxNum)  { /* uh oh we're full */
    sprintf(buf, "Connection refused: Socket Table is full\n");
    Log(LOG_PORT, buf);
    sprintf(buf, "We're sorry too many players are allready in the game (try again l8r)\n");
    WRITE(sockFD, buf, strlen(buf));
    CLOSE(sockFD); /* Better set linger on so that close will wait until its sent */
    return FALSE;
  }
  /* we got room, add 'em to the the game */
  MEMALLOC(thisSock, SOCK, 2048);
  memset(thisSock, 0, sizeof(SOCK));
  thisSock->sNext = sockList;
  sockList = thisSock;

  sockList->sFD   = sockFD;
  sockList->sAddr = ntohl( addr.sin_addr.s_addr );
  sprintf(sockList->sSiteIP,
    "%ld.%ld.%ld.%ld",
    sockList->sAddr/256/256/256%256,
    sockList->sAddr/256/256%256,
    sockList->sAddr/256%256,
    sockList->sAddr%256
  );
  if (*siteName)
    strcpy(sockList->sSiteName, siteName);
  else
    strcpy(sockList->sSiteName, sockList->sSiteIP);
  sockList->sPref = sockDefFlag;
  sockList->sFlag = SF_XLATE;
  sockList->sIn   = QAlloc(512);
  sockList->sOut  = QAlloc(8192);
  sockList->sMole = QAlloc(32);
  sockList->sPersonal = QAlloc(512);

  IndexInit(&sockList->sAliasIndex, ALIAS_INDEX_SIZE, "AliasInit(alias.c)", 0);

  HistoryClear(&sockList->sHistory);
  sockList->sLastRepeat   = 0;
  sockList->sScreenLines  = 99;
  sockList->sCurrentLines = 0;
  sockList->sSubMode      = SUBM_NONE;
  sockList->sMode         = MODE_LOGIN;
  sockList->sControlThing = NULL;
  sockList->sHomeThing    = NULL;

  SEND(fileList[FILE_GREETING].fileStr->sText, sockList);

  sockNum++;
  sockList->sSiteType = SiteGetType(sockList);
  if (sockList->sSiteType != SITE_LOCAL) sockNumOffsite++;

  if (sockList->sSiteType == SITE_BANNED) {
    SEND(fileList[FILE_BANNED].fileStr->sText, sockList);
    sockList->sMode         = MODE_KILLSOCKET;
  } else {
    SEND("^&WbPlease enter the name you wish to be known by:\n^W^V", sockList);
    SockWrite(sockList);
    BITCLR(sockList->sPref, SP_ANSI);
    sockList->sMode         = MODE_LOGIN;
  }
  sprintf(buf,
    "New connect from %s (%s) [%s]\n",
    sockList->sSiteName,
    sockList->sSiteIP,
    sTypeList[sockList->sSiteType]
  );
  Log(LOG_PORT, buf);

  return TRUE;
}



/* read socket input into input buffer - returns TRUE on error */
BYTE SockRead(SOCK *sock) {
  BYTE  buf[LINE_MAX_LEN];
  LWORD numRead;

  numRead = READ(sock->sFD, buf, LINE_MAX_LEN-1);
  if (numRead <= 0)
    return TRUE; /* socket read error of some kind */
  buf[numRead] = '\0'; /* terminate string */
  QAppend(sock->sIn, buf, Q_CHECKSTR);

  return FALSE;
}


/* write output buffer to socket */
void SockWrite(SOCK *sock) {
  BYTE  buf[1024+4]; /* enuff space for tacking CRLF's on end and then some */
  LWORD bufSent;
  LWORD thisSent;
  LWORD bufUnsent;
  LWORD bufLen;
  LWORD qValid;
  LWORD eolnSequence;


  /* for now just jam through everything we're trying to send, later we will
    track pending writes so that they dont hold up the main loop */

  while (sock->sOut->qLen) { /* if theres something to send */
    /* Wait till they finish typing in their line when editing */
    if (BIT(sock->sFlag, SF_ATEDIT))
      break;

    /* will have to check for screen overflow here too */
    if (BIT(sock->sPref, SP_CONTINUE) && (sock->sCurrentLines>=sock->sScreenLines) &&(sock->sScreenLines>0)) {
      if (!BIT(sock->sFlag, SF_ATCONTINUE)) {
        sprintf(buf,"^g<+- ^CHit ^wReturn ^Cto Continue ^w(%ld more pages)^g-+>\n", sock->sOut->qValid/sock->sScreenLines+1);
        QInsert(sock->sOut, buf);
        BITSET(sock->sFlag, SF_ATCONTINUE); /* no longer need message just block from now on */
        /* dont break now, break after we've sent return to continue message */
      } else
        break;
    }

    qValid = sock->sOut->qValid;
    memset(buf, 0, sizeof(buf)); /* easier debugging */
    /* translate or strip color sequences as appropriate */
    if (!BIT(sock->sFlag, SF_XLATE)) {
      QRead(sock->sOut, buf, 1024, Q_COLOR_IGNORE, NULL);
    } else {
      if (BIT(sock->sPref, SP_ANSI)) {
        if (sock->sHomeThing)
          QRead(sock->sOut, buf, 1024, Q_COLOR_ANSI, Plr(sock->sHomeThing)->pColorPref);
        else
          QRead(sock->sOut, buf, 1024, Q_COLOR_ANSI, NULL);
      } else {
        QRead(sock->sOut, buf, 1024, Q_COLOR_STRIP, NULL);
      }
    }
    /* cheap insurance */
    buf[sizeof(buf)-1] = '\0';
    /* do this somewhere in here */
    BITCLR(sock->sFlag, SF_PENDINGWRITE);

    /* if qValid is changed line needs EOLN sequence as determined by prefs */
    if (qValid!=sock->sOut->qValid && !BIT(sock->sFlag, SF_ATCONTINUE))
      eolnSequence = TRUE;
    else
      eolnSequence = FALSE;

    if (eolnSequence) {
      if (BIT(sock->sPref, SP_LF)) {
        strcat(buf, "\n");
      }
      if (BIT(sock->sPref, SP_CR)) {
        strcat(buf, "\r");
      }
      sock->sCurrentLines++;
    }

    /* send one complete line */
    bufSent = 0;
    bufLen=strlen(buf);
    bufUnsent=bufLen;
    while(bufUnsent > 0) {
      thisSent = WRITE(sock->sFD, buf+bufSent, bufUnsent);
      if (thisSent>=0) {
        bufSent += thisSent;
        bufUnsent = bufLen - bufSent;
      } else {
        /* remove EOLN sequence, replace with \n */
        if (eolnSequence){
          if (BIT(sock->sPref, SP_CR) && bufLen>0)
            buf[bufLen - 1] = '\0';
          if (!BIT(sock->sPref, SP_LF))
            strcat(buf, "\n");
          sock->sCurrentLines--; /* postponed until later */
        }
        /* prepend unsent string portion back onto queue */
        QInsert(sock->sOut, buf+bufSent);
        BITSET(sock->sFlag, SF_PENDINGWRITE);
        break;
      }
    }

    /* If we have Overflow'd stop here */
    if (BIT(sock->sFlag, SF_PENDINGWRITE))
      break;
    if (BIT(sock->sFlag, SF_ATCONTINUE)) {
      break;
    }

  }

  /* If they are editing, reset blocking */
  if ((sock->sEdit.eStr) && (sock->sSubMode!=SUBM_EDITMENU)) {
    BITSET(sock->sFlag, SF_ATEDIT);
  }
}

void SockWriteCheckAll(void) {
  SOCK *i;
  SOCK *nextSock;

  /* everyone who needs a prompt gets sent a prompt */
  for (i=sockList; i; i=i->sNext) {
    if (!BIT(i->sFlag, SF_ATPROMPT)
     && !BIT(i->sFlag, SF_ATEDIT)
     && !BIT(i->sFlag, SF_ATCONTINUE)
     && !(BIT(i->sPref, SP_CONTINUE) && (i->sCurrentLines+i->sOut->qValid>=i->sScreenLines))
    ) {
      SendPrompt(i); /* give the user a new prompt */
    }
  }
  /* maybe somebody has been sent something */
  for (i=sockList; i; i=nextSock) {
    nextSock = i->sNext;
    /* SockWrite will return TRUE FALSE at some point */
    SockWrite(i); /* will transmit output (including prompts) */
  }
}

/* this sock is outta here */
void SockFree(SOCK *sock) {
  BYTE buf[256];
  SOCK *sockFree;
  LWORD error;

  if (sockNum == 0) return; /* this shouldnt be necessary but cant hurt either  */
  sprintf(buf,
    "Closing connect from %s (%s)\n",sock->sSiteName, sock->sSiteIP);
  Log(LOG_PORT, buf);

  /* if non-local make connect available to pool */
  if (sock->sSiteType != SITE_LOCAL) sockNumOffsite--;

  /* should probably write out output q prior to close yes?/no? */
  error = CLOSE(sock->sFD);
  if (error) { /* will return -1 on error */
    Log(LOG_ERROR, "SockFree(socket.c) failed to close sock->sFD\n");
    PSOCKETERROR("SockFree");
    EXIT(ERROR_SOCKET);
  }

  /* Abort them out of editor as necessary */
  EditFree(sock);

  /* free the aliases */
  while (sock->sAliasIndex.iNum>0)
    AliasFree(&sock->sAliasIndex, Alias(sock->sAliasIndex.iThing[0]));
  IndexFree(&sock->sAliasIndex);

  /* turf input/output Q's */
  QFREE(sock->sIn);
  QFREE(sock->sOut);
  QFREE(sock->sMole);
  QFREE(sock->sPersonal);

  if (sock == sockList) { /* well well how handy .... */
    sockList = sockList->sNext;
  } else { /* find the sock in the list and remove it */
    for (sockFree = sockList;
         sockFree && sockFree->sNext != sock;
         sockFree = sockFree->sNext);
    if (!sockFree) {
      /* how the hell did this happen */
      Log(LOG_ERROR, "SockFree(socket.c) Tried to free non-existent SOCK\n");
      EXIT(ERROR_CORRUPT);
    }
    sockFree->sNext = sockFree->sNext->sNext; /* take out sock to be removed */
  }
  sockNum--;
  MEMFREE(sock, SOCK);
}

void SockProcess(SOCK *sock) {
/* only called when we know a full line of text is in the input queue */
  BYTE   cmd[LINE_MAX_LEN];
  BYTE   buf[LINE_MAX_LEN];
  BYTE  *next;
  LWORD  i;
  THING *player;
  SOCK  *oldSock;
  LWORD  qValid;

  if (sock->sControlThing && sock->sControlThing->tWait>0) {
    BITSET(sock->sFlag, SF_PENDINGREAD);
    return;
  }

  BITCLR(sock->sFlag, SF_ATPROMPT);
  BITCLR(sock->sFlag, SF_ATEDIT);
  BITCLR(sock->sFlag, SF_MORE);

  /* first 256 bytes go into cmd buffer */
  qValid = sock->sIn->qValid;
  QRead(sock->sIn, cmd, LINE_MAX_LEN, Q_COLOR_IGNORE, NULL);
  QFlush(sockOverflow);

  /* clear out any line fragment that may be left in the buffer */
  /* bytes 256-???? end up in the overflow buffer */
  while (qValid==sock->sIn->qValid) {
    BYTE buf[LINE_MAX_LEN];

    BITSET(sock->sFlag, SF_MORE);
    QRead(sock->sIn, buf, sizeof(buf), Q_COLOR_IGNORE, NULL);
    QAppend(sockOverflow, buf, Q_DONTCHECKSTR);
  }

  next = StrOneWord(cmd, buf); /* see if there's more than one word */

  sock->sCurrentLines = 0; /* # of lines since last input now 0 */
  if (BIT(sock->sFlag, SF_ATCONTINUE)) {
    BITCLR(sock->sFlag, SF_ATCONTINUE);
    return;
  }

  /* editor takes precedence */
  if (sock->sEdit.eStr) {
    EditProcess(sock, cmd);
    return;
  }

  switch(sock->sSubMode) {
  case SUBM_NONE:
    break;

  case SUBM_NEWPASSWORD:
    HistoryAdd(&sock->sHistory, cmd);
    SEND("^V\n^wEnter your password ^yagain ^wto confirm the change\n", sock);
    sock->sSubMode = SUBM_CONFIRMPASSWORD;
    return;

  case SUBM_CONFIRMPASSWORD:
    player = sock->sHomeThing;
    if (!player) {
      SEND("\n^yGACK! ^wnull player thing.. what the hell?\n", sock);
    } else if (!strcmp(cmd, HistoryGetLast(&sock->sHistory))) {
      STRFREE(Plr(player)->pPassword);
      Plr(player)->pPassword = STRCREATE(CRYPT(cmd, getenv("CRIMSON2CRYPTKEY")));
      SEND("^V\n^yPassword changed\n", sock);
      HistoryClear(&sock->sHistory);
    } else {
      SEND("^V\n^wPassword didnt match!! password unchanged\n", sock);
    }
    SendEchoOn(sock);
    sock->sSubMode = SUBM_NONE;
    if (sock->sMode==MODE_CHOOSEEXPERT) SendChooseExpert(sock);
    return;

  /* Used to Make new chars only */
  case SUBM_ENTEREMAIL:
    player = sock->sHomeThing;
    sock->sSubMode = SUBM_NONE;
    STRFREE(Plr(player)->pEmail);
    Plr(player)->pEmail = STRCREATE(cmd);
    if (sock->sMode==MODE_CHOOSEEXPERT) {
      SEND("^V\n^gEnter the ^Cpassword ^gyou are going to use. (write it down!)\n", sock);
      sock->sSubMode = SUBM_NEWPASSWORD;
    }
    return;
  }

  /* hmm well they arent editing what are they doing */
  switch(sock->sMode) {
  case MODE_LOGIN:
    if (PlayerName(buf)==PNAME_INVALID) {
      SEND("^&WbPlayer Names must be 3-15 alphabetic characters, Please re-enter!\n", sock);
      break;
    }
    /* Store the name for later reference */
    HistoryAdd(&sock->sHistory, buf);
    player = PlayerFind(buf);
    oldSock = BaseControlFind(player);
    if (player) {
      if (oldSock) {
        SEND("^&WbThat person is allready connected!\n", sock);
        SEND("^&WbContinuing will terminate the existing connection!\n", sock);
      }
      SendEchoOff(sock);
      SEND("^V\n^&WbEnter your password: (Hidden for security)^V\n", sock);
      sock->sMode = MODE_PASSWORD;
      break;
    }
    if (!player) {
      player = PlayerRead(buf, PREAD_NORMAL);
    }

    /* set socket prefs from player data  */
    if (player) {
      sock->sScreenLines = Plr(player)->pScreenLines;
      sock->sPref        = Plr(player)->pSockPref;
      /* unload the player */
      PlayerWrite(player, PWRITE_PLAYER);
      THINGFREE(player);
      SEND("^V\n^&WbEnter your password: (Hidden for security)\n", sock);
      SendEchoOff(sock);
      sock->sMode = MODE_PASSWORD;
      break;
    }
    /* guess they are a newbie */
    sock->sScreenLines = 23;
    SEND("^&WbIf you wish to create a new character, enter your name again^V\n", sock);
    sock->sMode = MODE_CONFIRMNAME;
    break;

  case MODE_PASSWORD: {
    LWORD playerLoaded;

    SendEchoOn(sock);
    SEND("\n", sock);

    /* load the player struct */
    playerLoaded=FALSE;
    oldSock = NULL;
    player = PlayerFind(HistoryGetLast(&sock->sHistory));
    if (player) {
      oldSock = BaseControlFind(player);
    } else {
      playerLoaded = TRUE;
      player = PlayerRead(HistoryGetLast(&sock->sHistory), PREAD_NORMAL);
    }
    /* error of some kind - bail */
    if (!player) {
      sock->sMode = MODE_KILLSOCKET;
      break;
    }


    /* Check security */
    if (!( StrExact(Plr(player)->pPassword->sText, CRYPT(cmd, getenv("CRIMSON2CRYPTKEY")))
           ||(!Plr(player)->pPassword->sText[0] && !*cmd )
    )) {
      Plr(player)->pAttempts++;
      SEND("^rThat aint it I'm afraid...\n", sock);
      /* if we just failed to match passwords with a crash file
         then it will be moved back to the normal player dir
       */
      if (playerLoaded) {
        PlayerWrite(player, PWRITE_PLAYER);
        THINGFREE(player);
      } else if (oldSock) {
        SEND("^rSomeone just attempted to seize your link from you - ", oldSock);
        SEND(sock->sSiteName, oldSock);
        SEND(" [", oldSock);
        SEND(sock->sSiteIP, oldSock);
        SEND("]\n", oldSock);
        SEND("^r(But they got your password wrong)\n", oldSock);
      }
      if (BIT(sock->sFlag, SF_1BADPW)) {
        sock->sMode = MODE_KILLSOCKET; /* that'll do it */
      } else {
        sock->sMode = MODE_LOGIN;
        SEND("Okay, lets try again, what's your name?\n", sock);
        BITCLR(sock->sPref, SP_ANSI);
        BITSET(sock->sFlag, SF_1BADPW);
      }
      break;
    }

    /* Check for limit on offsite connections */
    if (sock->sSiteType!=SITE_LOCAL && sockNumOffsite==sockMaxOffsite && Character(player)->cLevel < LEVEL_GOD) {
      SEND(fileList[FILE_OFFSITE].fileStr->sText, sock);
      if (playerLoaded) {
        PlayerWrite(player, PWRITE_PLAYER);
        THINGFREE(player);
      }
      sock->sMode = MODE_KILLSOCKET; /* that'll do it */
      break;
    }

    /* If an oldSock exists - get rid of 'em */
    if (oldSock) {
      SEND("^rSomeone just seized your link from you - ", oldSock);
      SEND(sock->sSiteName, oldSock);
      SEND(" [", oldSock);
      SEND(sock->sSiteIP, oldSock);
      SEND("]\n", oldSock);
      BaseControlFree(player, oldSock);
      oldSock->sHomeThing = NULL;
      oldSock->sControlThing = NULL;
      oldSock->sMode = MODE_KILLSOCKET;
    }

    if (!playerLoaded) {
      SEND("^&wBReconnecting\n^V", sock);
      sock->sPref = Plr(player)->pSockPref;
    }

    /* well their password is correct (wahoo!) */
    BaseControlAlloc(player, sock); /* keep a pointer back from it */
    PlayerWrite(player, PWRITE_CRASH); /* in the game now */
    AliasRead(sock);
    SendMainMenu(sock);
    SEND("^V", sock);
    sprintf(buf, "%s has signed onto the game! %s[%s]\n", player->tSDesc->sText, sock->sSiteName, sock->sSiteIP);
    Log(LOG_USAGE, buf);
    SEND("^gLast connect was from: ^b", sock);
    SEND(Plr(player)->pLastLogin->sText, sock);
    sprintf(buf, "%s [%s]", sock->sSiteName, sock->sSiteIP);
    STRFREE(Plr(player)->pLastLogin);
    Plr(player)->pLastLogin = STRCREATE(buf);
    SEND("\n^gLast connect was on: ^b", sock);
    SEND(ctime(&Plr(player)->pTimeLastOn), sock);
    SEND("^gTime since last connect: ^b", sock);
    TimeSprintf(buf, time(0)-Plr(player)->pTimeLastOn);
    SEND(buf, sock);
    SEND("\n", sock);
    if (Plr(player)->pTimeLastOn < fileList[FILE_NEWS].fileDate) {
      SEND("^wThere's new NEWS!\n", sock);
    }
    if (!*Plr(player)->pEmail->sText) {
      SEND("^yYou have no EMAIL address\n", sock);
    }
    if (!*Plr(player)->pPlan->sText) {
      SEND("^yYou have no PLAN\n", sock);
    }
    if (!strcmp(player->tDesc->sText, "You see nothing special.\n")) {
      SEND("^yYou are using the default DESCRIPTION\n", sock);
    }
    if (Plr(player)->pAttempts>0) {
      sprintf(buf, "^rWARNING! ^gThere were ^w%hd ^gunsuccessfull connect attempts in your absence\n", Plr(player)->pAttempts);
      SEND(buf, sock);
      Plr(player)->pAttempts=0;
    }
    Plr(player)->pTimeLastOn = time(0);
    sock->sMode = MODE_MAINMENU;
  } break;

  case MODE_CONFIRMNAME:
    /* their first entry for name is in history */
    if (!StrExact(HistoryGetLast(&sock->sHistory), buf)) {
      SEND("Okay so what's your name again?\n", sock);
      sock->sMode = MODE_LOGIN; /* that'll do it */
      break;
    }
    if (sock->sSiteType == SITE_BANNEW) {
      SEND(fileList[FILE_BANNEW].fileStr->sText, sock);
      sock->sMode = MODE_KILLSOCKET; /* that'll do it */
      break;
    }

    player = PlayerAlloc(HistoryGetLast(&sock->sHistory));
    Plr(player)->pSockPref = sock->sPref;
    HistoryClear(&sock->sHistory);
    SEND("\nCreating NEW character: ", sock);
    SEND(player->tSDesc->sText, sock);
    SEND("\n", sock);
    Plr(player)->pScreenLines = sock->sScreenLines;
    Plr(player)->pSockPref  = sock->sPref; /* give 'em the default prefs */
    Plr(player)->pTimeLastOn = time(0);
    BaseControlAlloc(player, sock); /* keep a pointer back from it */
    SEND("I'm afraid you're going to have to answer a few skill-testing questions\n", sock);
    /* SendChooseSex(sock); */
    sock->sMode = MODE_CHOOSEANSI;
    SendChooseAnsi(sock);
    break;
    
  case MODE_CHOOSEANSI:
    player = sock->sHomeThing;
    if (StrExact("cancel", buf)) { /* they wanna cancel... */
      BaseControlFree(sock->sControlThing, sock);
      sock->sControlThing = NULL;
      THINGFREE(sock->sHomeThing);
      sock->sMode = MODE_LOGIN;
      BITCLR(sock->sPref, SP_CONTINUE);
      SEND(fileList[FILE_GREETING].fileStr->sText, sock);
      SEND("Please enter the name you wish to be known by:\n", sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) && !*next ) {
      SendChooseAnsi(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) ){
      HelpParse(sock->sHomeThing, cmd, NULL);
      SEND("\n", sock);
    } else if (StrAbbrev("yes", buf)) { 
      BITSET(sock->sPref, SP_ANSI);
      Plr(player)->pSockPref = sock->sPref;
      SEND("\n^gEnter your ^Cemail ^gaddress (Hit Enter if you wish anonymity)\n", sock);
      sock->sSubMode = SUBM_ENTEREMAIL;
      sock->sMode = MODE_CHOOSEEXPERT;
    } else if (StrAbbrev("no", buf)) { 
      BITCLR(sock->sPref, SP_ANSI);
      Plr(player)->pSockPref = sock->sPref;
      SEND("\n^gEnter your ^Cemail ^gaddress (Hit Enter if you wish anonymity)\n", sock);
      sock->sSubMode = SUBM_ENTEREMAIL;
      sock->sMode = MODE_CHOOSEEXPERT;
    } else {
      SEND("^gCome again? - Type YES or NO\n\n", sock);
    }
    break;

  case MODE_CHOOSEEXPERT:
    player = sock->sHomeThing;
    if (StrExact("cancel", buf)) { /* they wanna cancel... */
      BaseControlFree(sock->sControlThing, sock);
      sock->sControlThing = NULL;
      THINGFREE(sock->sHomeThing);
      sock->sMode = MODE_LOGIN;
      BITCLR(sock->sPref, SP_CONTINUE);
      SEND(fileList[FILE_GREETING].fileStr->sText, sock);
      SEND("Please enter the name you wish to be known by:\n", sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) && !*next ) {
      SendChooseExpert(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) ){
      HelpParse(sock->sHomeThing, cmd, NULL);
      SEND("\n", sock);
    } else if (StrAbbrev("yes", buf)) { 
      BITSET(Plr(player)->pAuto, PA_EXPERT);
      sock->sMode = MODE_CHOOSESEX;
      SendChooseSex(sock);
    } else if (StrAbbrev("no", buf)) { 
      sock->sMode = MODE_CHOOSESEX;
      SendChooseSex(sock);
    } else {
      SEND("^gCome again? - Type YES or NO\n\n", sock);
    }
    break;

  case MODE_CHOOSESEX:
    /* need this for later */
    player = sock->sHomeThing;
    if (StrExact("cancel", buf)) { /* they wanna cancel... */
      BaseControlFree(sock->sControlThing, sock);
      sock->sControlThing = NULL;
      THINGFREE(sock->sHomeThing);
      sock->sMode = MODE_LOGIN;
      BITCLR(sock->sPref, SP_CONTINUE);
      SEND(fileList[FILE_GREETING].fileStr->sText, sock);
      SEND("Please enter the name you wish to be known by:\n", sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) && !*next ) {
      SendChooseSex(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) ){
      HelpParse(sock->sHomeThing, cmd, NULL);
      SEND("\n", sock);
    } else {
      i = TYPEFIND(buf, sexList);
      if (i == -1) {
        SEND("^gCome again?\n\n", sock);
      } else {
        Character(player)->cSex = i;
        SendChooseRace(sock);
        sock->sMode = MODE_CHOOSERACE;
      }
    } 
    break;

  case MODE_CHOOSERACE:
    /* need this for later */
    player = sock->sHomeThing;
    if (StrExact("cancel", buf)) { /* they wanna cancel... */
      BaseControlFree(sock->sControlThing, sock);
      sock->sControlThing = NULL;
      THINGFREE(sock->sHomeThing);
      sock->sMode = MODE_LOGIN; /* that'll do it */
      BITCLR(sock->sPref, SP_CONTINUE);
      SEND(fileList[FILE_GREETING].fileStr->sText, sock);
      SEND("Please enter the name you wish to be known by:\n", sock);
    } else if (StrExact("back", buf)) { /* they wanna back a step */
      sock->sMode = MODE_CHOOSESEX;
      SendChooseSex(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) && !*next ) {
      SendChooseRace(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) ){
      HelpParse(sock->sHomeThing, cmd, NULL);
      SEND("\n", sock);
    } else {
      i = TYPEFIND(buf, raceList);
      if (i == -1) {
        SEND("^gCome again?\n\n", sock);
      } else {
        Plr(player)->pRace = i;
        SendChooseClass(sock);
        sock->sMode = MODE_CHOOSECLASS;
      }
    } 
    break;

  case MODE_CHOOSECLASS:
    /* need this for later */
    player = sock->sHomeThing;
    if (StrExact("cancel", buf)) { /* they wanna cancel... */
      BaseControlFree(sock->sControlThing, sock);
      sock->sControlThing = NULL;
      THINGFREE(sock->sHomeThing);
      sock->sMode = MODE_LOGIN; /* that'll do it */
      BITCLR(sock->sPref, SP_CONTINUE);
      SEND(fileList[FILE_GREETING].fileStr->sText, sock);
      SEND("Please enter the name you wish to be known by:\n", sock);
    } else if (StrExact("back", buf)) { /* they wanna back a step */
      sock->sMode = MODE_CHOOSERACE;
      SendChooseRace(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) && !*next ) {
      SendChooseClass(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) ){
      HelpParse(sock->sHomeThing, cmd, NULL);
      SEND("\n", sock);
    } else {
      i = TYPEFIND(buf, classList);
      if (i == -1) { 
        SEND("^gCome again?\n\n", sock);
      } else {
        Plr(player)->pClass = i;
        if (BIT(Plr(player)->pAuto, PA_EXPERT)) {
          sock->sMode = MODE_SHOWCOMBO;
          SendShowCombo(sock);
        } else {
          PlayerRollAbilities(player, 30); /* take the best of 30 rerolls */
          Plr(player)->pTimeLastOn = time(0);
          PlayerWrite(player, PWRITE_CRASH);
          sprintf(buf, "%s has signed onto the game! (New Player)\n", sock->sHomeThing->tSDesc->sText);
          Log(LOG_USAGE, buf);
          sock->sMode = MODE_MAINMENU;
          SendMainMenu(sock);
          SEND("^;HINT: Type ^<PLAY^; and start playing the game immediately!\n", sock);
        }
      }
    } 
    break;

  case MODE_SHOWCOMBO:
    /* need this for later */
    player = sock->sHomeThing;
    if (StrExact("cancel", buf)) { /* they wanna cancel... */
      BaseControlFree(sock->sControlThing, sock);
      sock->sControlThing = NULL;
      THINGFREE(sock->sHomeThing);
      sock->sMode = MODE_LOGIN; /* that'll do it */
      BITCLR(sock->sPref, SP_CONTINUE);
      SEND(fileList[FILE_GREETING].fileStr->sText, sock);
      SEND("Please enter the name you wish to be known by:\n", sock);
    } else if (StrExact("back", buf)) { /* they wanna back a step */
      sock->sMode = MODE_CHOOSERACE;
      SendChooseRace(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) && !*next ) {
      SendShowCombo(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) ){
      HelpParse(sock->sHomeThing, cmd, NULL);
      SEND("\n", sock);
    } else if ( StrAbbrev("accept", buf) ){
      PlayerRollAbilities(player, 5);
      SendRollAbilities(sock);
      sock->sMode = MODE_ROLLABILITIES;
    } else {
      SEND("^gCome again?\n\n", sock);
    } 
    break;


  case MODE_ROLLABILITIES:
    /* need this for later */
    player = sock->sHomeThing;
    if (StrExact("cancel", buf)) { /* they wanna cancel... */
      BaseControlFree(sock->sControlThing, sock);
      sock->sControlThing = NULL;
      THINGFREE(sock->sHomeThing);
      sock->sMode = MODE_LOGIN; /* that'll do it */
      BITCLR(sock->sPref, SP_CONTINUE);
      SEND(fileList[FILE_GREETING].fileStr->sText, sock);
      SEND("Please enter the name you wish to be known by:\n", sock);
    } else if (StrExact("back", buf)) { /* they wanna back a step */
      sock->sMode = MODE_CHOOSECLASS;
      SendChooseClass(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) && !*next ) {
      SendRollAbilities(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) ){
      HelpParse(sock->sHomeThing, cmd, NULL);
      SEND("\n", sock);
    } else if ( StrAbbrev("reroll", buf) ){
      PlayerRollAbilities(player, 1);
      SendRollAbilities(sock);
    } else if ( StrAbbrev("accept", buf) ){
      Plr(player)->pTimeLastOn = time(0);
      PlayerWrite(player, PWRITE_CRASH);
      sprintf(buf, "%s has signed onto the game! (New Player)\n", sock->sHomeThing->tSDesc->sText);
      Log(LOG_USAGE, buf);
      SendMainMenu(sock);
      sock->sMode = MODE_MAINMENU;
    } else {
      SEND("^gCome again?\n\n", sock);
    } 
    break;
    
  case MODE_MAINMENU:
    if (StrAbbrev("quit", buf)) { /* they wanna leave... */
      SendAction(Plr(sock->sHomeThing)->pExit->sText, sock->sHomeThing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE);
      sprintf(buf, "%s just quit the game!\n", sock->sHomeThing->tSDesc->sText);
      Log(LOG_USAGE, buf);
      PlayerUpdateTime(sock->sHomeThing);
      if (Base(sock->sHomeThing)->bInside && Base(sock->sHomeThing)->bInside->tType==TTYPE_WLD)
        Plr(sock->sHomeThing)->pStartRoom = Wld(Base(sock->sHomeThing)->bInside)->wVirtual;
      else
        Plr(sock->sHomeThing)->pStartRoom = -1;
      PlayerWrite(sock->sHomeThing, PWRITE_PLAYER);
      /* this next bit takes them right out of the game */
      BaseControlFree(sock->sControlThing, sock);
      sock->sControlThing = NULL;
      sprintf(buf, "%s has quit the game %s[%s]\n", sock->sHomeThing->tSDesc->sText, sock->sSiteName, sock->sSiteIP);
      Log(LOG_USAGE, buf);
      THINGFREE(sock->sHomeThing);
      sock->sMode = MODE_KILLSOCKET; /* that'll do it */
    } else if (StrAbbrev("play", buf)) { /* they wanna play */
      if (!Base(sock->sHomeThing)->bInside) {
        THING *dest = NULL;
      
        if (Plr(sock->sHomeThing)->pStartRoom>0)
          dest = WorldOf(Plr(sock->sHomeThing)->pStartRoom);
        if (!dest)
          dest = WorldOf(playerStartRoom);
        if (!dest) {
          Log(LOG_ERROR, "Nowhere to enter the game: check playerStartRoom in Crimson2.INI\n");
          EXIT(1);
        }
        ThingTo(sock->sHomeThing, dest);
        if (!Base(sock->sHomeThing)->bInside) {
          ThingTo(sock->sHomeThing, WorldOf(playerStartRoom));
          if (!Base(sock->sHomeThing)->bInside) {
            SendThing("^wDue to a problem with the room you were in, you have been relocated\n", sock->sHomeThing);
            sprintf(buf, 
                    "Problem with Wld#%ld, %s bounced off the game!\n", 
                    Plr(sock->sHomeThing)->pStartRoom,
                    sock->sHomeThing->tSDesc->sText);
            Log(LOG_ERROR, buf);
            Log(LOG_ERROR, "Nowhere to enter the game: check playerStartRoom in Crimson2.INI\n");
            EXIT(1);
          } else {
            SendThing("^wDue to a problem with the room you were in, you have been relocated\n", sock->sHomeThing);
            sprintf(buf, 
                    "Problem with Wld#%ld, %s bounced to start room!\n", 
                    Plr(sock->sHomeThing)->pStartRoom,
                    sock->sHomeThing->tSDesc->sText);
            Log(LOG_ERROR, buf);
          }
        }
        SendAction(Plr(sock->sHomeThing)->pEnter->sText, sock->sHomeThing, NULL, SEND_ROOM|SEND_VISIBLE|SEND_AUDIBLE);
        sprintf(buf, "%s just entered the game!\n", sock->sHomeThing->tSDesc->sText);
        Log(LOG_USAGE, buf);
      }
      sock->sMode = MODE_PLAY;
      if (!PlayerCloneExists(sock->sHomeThing))
        PlayerWrite(sock->sHomeThing, PWRITE_CLONE);
      CmdLook(sock->sHomeThing, "");
      if (!BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT) 
       && Character(sock->sHomeThing)->cLevel < 4) {
        SendHint("^;HINT: If you're having problems playing, try ^<LOOK^;ing at things\n", sock->sHomeThing);
        SendHint("^;HINT: To check your progress in the game type ^<SCORE^;\n", sock->sHomeThing);
      }
    } else if (StrAbbrev("desc", buf)) {
      /* they wanna edit */
      EditStr(sock, &sock->sHomeThing->tDesc, 512, "Player Description", EP_ENDLF);
    } else if (StrAbbrev("email", buf)) {
      /* they wanna edit */
      EditStr(sock, &Plr(sock->sHomeThing)->pEmail, 128, "Email Address", EP_IMMNEW|EP_ONELINE|EP_ENDNOLF);
    } else if (StrAbbrev("plan", buf)) {
      /* they wanna edit */
      EditStr(sock, &Plr(sock->sHomeThing)->pPlan, 512, "Plan Description", EP_ENDNOLF);
    } else if (StrAbbrev("news", buf)) {
      SEND(fileList[FILE_NEWS].fileStr->sText, sock);
    } else if (StrAbbrev("stats", buf)) {
      SendShowCombo(sock);
    } else if (StrAbbrev("credits", buf)) {
      SEND(fileList[FILE_CREDIT].fileStr->sText, sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) && !*next ) {
      SendMainMenu(sock);
    } else if ( (StrAbbrev("?", buf)||StrAbbrev("help", buf)) ){
      HelpParse(sock->sHomeThing, cmd, NULL);
#ifdef BLAH
    } else if (strcmp(buf, "shutdown") == 0 
               && Character(sock->sHomeThing)->cLevel >= LEVEL_CODER) {
      crimsonRun = FALSE;
      unlink(REBOOT_FLAG_FILE);
#endif
    } else if (strcmp(buf, "delete") == 0) {
      /* Turf this char */
      SEND("You're outta here buddy...\n", sock);
      PlayerDelete(sock->sHomeThing);

    } else if (StrAbbrev("delete", buf)) {
      SEND("If you REALLY wish to delete this char, type DELETE in full!\n", sock);
    } else if (strcmp(buf, "reboot") == 0 
               && Character(sock->sHomeThing)->cLevel >= LEVEL_CODER) {
      crimsonRun = FALSE;
    } else if (strcmp(buf, "pass")==0) {
      SEND("^wEnter your ^rNEW ^wpassword:\n", sock);
      SendEchoOff(sock);
      sock->sSubMode = SUBM_NEWPASSWORD;
    } else { /* we dont know what they want... */
      SEND("How do you mean by that exactly?!?\n", sock);
    }
    break;

  case MODE_PLAY:
    ParseSock(sock, cmd);
    break;
  }

}



void SockCloseAll(LWORD portFD) {
  LWORD error;
  BYTE  buf[256];

  while (sockList) { /* turf out all the connections */
    BITCLR(sockList->sPref, SP_CONTINUE);
    SEND(fileList[FILE_CIAO].fileStr->sText, sockList);
    if(sockList->sHomeThing) {
      if (Base(sockList->sHomeThing)->bInside && Base(sockList->sHomeThing)->bInside->tType==TTYPE_WLD)
        Plr(sockList->sHomeThing)->pStartRoom = Wld(Base(sockList->sHomeThing)->bInside)->wVirtual;
      else
        Plr(sockList->sHomeThing)->pStartRoom = -1;
      PlayerWrite(sockList->sHomeThing, PWRITE_PLAYER);
      BaseControlFree(sockList->sControlThing, sockList);
      sprintf(buf, "Closing connect to %s %s[%s]\n", sockList->sHomeThing->tSDesc->sText, sockList->sSiteName, sockList->sSiteIP);
      Log(LOG_USAGE, buf);
      ThingFree(sockList->sHomeThing);
    }
    SockWrite(sockList); /* clear out all the stuff pending */
    SockFree(sockList);
  }
  error = CLOSE(portFD);
  if (error) { /* will return -1 on error */
    Log(LOG_ERROR, "SockCloseAll(socket.c) failed to close portFD\n");
    PSOCKETERROR("SockCloseAll");
    EXIT(ERROR_SOCKET);
  }
}

LWORD main(LWORD argc, char *argv[]){
  LWORD           portFD;
  LWORD           error;
  SOCK           *i;
  SOCK           *nextSock;
  BYTE            buf[256];

  /* socket descriptor sets */
  fd_set          readfds;
  fd_set          writefds;
  fd_set          exceptfds;

  /* timing information */
  struct timeval  tvLast;
  struct timeval  tvNow;
  struct timeval  tvNext;
  struct timeval  tvTimeout;
  LWORD           x;
  LWORD           area;

  /* Set flag files to indicate boot in progress */
  {
    FILE *theFile;
    theFile = fopen(BOOT_FLAG_FILE, "wb");
    if (theFile) {
      fprintf(theFile,"\n");
      fclose(theFile);
    } else { /* cant log yet - unlikely error anyways */
      printf("Unable to create boot flag file!\n");
    }
    unlink(REBOOT_FLAG_FILE);
  }

  /* initialize the time stuff */
  srand(time(0)); /* seed the number generator */
  TvGetTime(&tvLast);
  TVSET(tvNext, tvLast);
  TvAddConst(&tvNext, 0, USEC_PER_SEG);

  /* Initialize the mud server */
  printf("\nStarting Crimson2 server:\n");
  printf("========================\n");
  MemInit();
  LogInit();
  FileInit();
  SocialInit();
  AliasInit();
  HelpInit();
  EffectInit();
  ParseInit();
  ThingInit();
  SkillInit();
  FightInit();
  QInit();
  if (argc>1)
    sockPortNum = atol(argv[1]);
  else
    sockPortNum = INILWordRead("crimson2.ini", "sockPortNum", 4000);
  portFD = SockInit(sockPortNum);
  SiteInit();
  BoardInit();
  AreaInit();
  PlayerInit(); /* must come after SkillInit */
  CodeInit();
  TalkInit();
  MemInitDone();
  ResetAll();
  Log(LOG_BOOT, "Waiting for connects at time=");
  startTime = time(0);
  LogPrintf(LOG_BOOT, ctime(&startTime));
  LogInitDone();
  printf("\n");
  printf("=====================================\n");
  printf("        Mud Server is Copacetic\n");
  printf("\n");
  {
    /* Solaris doesnt declare this for some reason */
#ifdef SOLARIS
    extern int gethostname(char *name, int namelen);
#endif

    BYTE buf[256];
    gethostname(buf, sizeof(buf));
    printf("telnet %s %ld to play!\n", buf, sockPortNum);
  }
  printf("=====================================\n\n");

  /* Set flag files to indicate successfull boot */
  {
    FILE *theFile;
    theFile = fopen(REBOOT_FLAG_FILE, "wb");
    if (theFile) {
      fprintf(theFile,"\n");
      fclose(theFile);
    } else {
      Log(LOG_ERROR, "Unable to create reboot flag file!\n");
    }
    unlink(BOOT_FLAG_FILE);
  }

#ifndef WIN32
  signal(SIGHUP,    KillSignalProc);
  signal(SIGINT,    KillSignalProc);
  signal(SIGTERM,   KillSignalProc);
  signal(SIGALRM,   AlarmSignalProc);
  signal(SIGVTALRM, AlarmSignalProc);

  #ifdef USE_ITIMER
  /* Start the infinite loop monitor */
    {
      struct itimerval   iTimerVal;
      TvSetConst(&iTimerVal.it_interval, ILOOP_CHECK_DELAY, 0);
      TvSetConst(&iTimerVal.it_value,    ILOOP_CHECK_DELAY, 0);
      /* Something is screwed with this thing, dont know what */
      setitimer(ITIMER_REAL, &iTimerVal, 0);
    }
  #endif
#endif

  while (crimsonRun) {
#ifndef WIN32
  #ifdef USE_ALARM
    alarm(ILOOP_CHECK_DELAY); /* keep pushing the kill alarm into the future */
  #endif
#endif

    /* you must reset the FD sets everytime since select changes them */
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(portFD, &readfds); /* monitor the mother socket */
    for (i=sockList; i; i=i->sNext){ /* monitor all active sockets */
      FD_SET(i->sFD, &readfds);
      if (BIT(i->sFlag, SF_PENDINGWRITE))
        FD_SET(i->sFD, &writefds); /* will return as soon as socket is ready for writing */
      FD_SET(i->sFD, &exceptfds);
    }

    /* timeout for next segment */
    TvGetTime(&tvNow);
    TVSET(tvTimeout, tvNext);
    TVSUB(tvTimeout, tvNow); /* max timeout is time remaining for this segment */

#if (SOCK_DISABLE_TIMEOUT)
/* disable timeouts for now */
TvSetConst(&tvTimeout, 500000, 500000);
#endif

    /* now sleep until either input/output/exception or next segment */
    error = select(FD_SETSIZE, &readfds, &writefds, &exceptfds, &tvTimeout);
    if (error == -1) {
      Log(LOG_ERROR, "main(socket.c) select error\n");
      PSOCKETERROR("main");
    } else {
      
      /* okay something has happened - maybe new players! */
      if (FD_ISSET(portFD, &readfds)) /* action on the "port" mother socket */
        SockCreate(portFD); /* allright a player */
      
      /* kill off the dead weight! (or exceptions anyways whatever they are) */
      for (i=sockList; i; i=nextSock) {
        nextSock = i->sNext; /* unreliable after free so store it now */
        if (FD_ISSET(i->sFD, &exceptfds)){
          FD_CLR(i->sFD, &readfds); /* guess we dont need to take its input nay more huh? */
          FD_CLR(i->sFD, &writefds);
          /* they dropped link or something */
          i->sMode = MODE_KILLSOCKET;
        }
      }
      
      /* maybe somebody typed something */
      for (i=sockList; i; i=nextSock) {
        nextSock = i->sNext; /* unreliable after free so store it now */
        /* dont take input from sockets that have been killed */
        if (i->sMode != MODE_KILLSOCKET 
            && FD_ISSET(i->sFD, &readfds)){
          error = SockRead(i);
          if(error) {
            FD_CLR(i->sFD, &writefds);
            /* they dropped link or something */
            i->sMode = MODE_KILLSOCKET;
          } 
        } 
        BITCLR(i->sFlag, SF_PENDINGREAD);
        while (i->sMode != MODE_KILLSOCKET 
               && i->sIn->qValid 
               && !BIT(i->sFlag, SF_PENDINGREAD) ) { /* at least one full line in the queue */
          SockProcess(i); /* process the action whatever it was */
        }
      }
      
      /* kill off the slain connections! (ie MODE_KILLSOCKET) */
      /* (To get a socket killed off set its mode to this */
      for (i=sockList; i; i=nextSock) {
        nextSock = i->sNext; /* unreliable after free so store it now */
        if (i->sMode == MODE_KILLSOCKET){
          BITCLR(i->sPref, SP_CONTINUE);
          SEND(fileList[FILE_CIAO].fileStr->sText, i);
          SockWrite(i); /* clear out all the stuff pending */
          BaseControlFree(i->sControlThing, i);
          SockFree(i);
        }
      }
      
      /* is it the next segment yet? */
      TvGetTime(&tvNow);
      TVSUB(tvNow, tvNext); /* negative value is equal to zero */
      if (tvNow.tv_usec > 0) {
        TVSET(tvLast, tvNext); /* last is now old next */
        TvAddConst(&tvNext, 0, USEC_PER_SEG); /* calculate start of next segment */
        tmSegment++; /* increment segment count */
  
        /* decrement Wait states */
        if (tmSegment%SEG_PER_WAIT==0) {
          /* do mobs */
          for(x=0; x<mobileIndex.iNum; x++) {
            if (mobileIndex.iThing[x]->tWait>0)
              mobileIndex.iThing[x]->tWait--;
          }
          /* do objects */
          for(x=0; x<objectIndex.iNum; x++) {
            if (objectIndex.iThing[x]->tWait>0)
              objectIndex.iThing[x]->tWait--;
          }
          /* do wlds */
          for(area=0; area<areaListMax; area++) {
            for(x=0; x<areaList[area].aWldIndex.iNum; x++) {
              if (areaList[area].aWldIndex.iThing[x]->tWait>0)
                areaList[area].aWldIndex.iThing[x]->tWait--;
            }
          }
          /* do players */
          for(x=0; x<playerIndex.iNum; x++) {
            /* they must be in the game for this to happen */
            if (Base(playerIndex.iThing[x])->bInside) {
              PlayerUpdateTime(playerIndex.iThing[x]);
              if (playerIndex.iThing[x]->tWait>0)
                playerIndex.iThing[x]->tWait--;
            }
          }
        }
  
        /* Give everyone a chance to heal up */
        if (tmSegment%SEG_PER_FASTTICK==0) {
          for(x=0; x<playerIndex.iNum; x++) {
            CharFastTick(playerIndex.iThing[x]);
          }
        }
        if (tmSegment%SEG_PER_FASTTICK==2) {
          for(x=0; x<mobileIndex.iNum; x++) {
            CharFastTick(mobileIndex.iThing[x]);
          }
        }

        /* Give everyone a chance to heal up */
        if (tmSegment%SEG_PER_TICK==0) {
          SendChannel("TICK!\n", NULL, SP_CHANTICK);
          PlayerTick();
          BoardIdle();
        }
        if (tmSegment%SEG_PER_FIGHT==0) {
          FightIdle();
        }
        if (tmSegment%SEG_PER_TICK==1) {
          ObjectTick();
        }
        if (tmSegment%SEG_PER_TICK==2) {
          MobileTick();
        }
  
        /* socket usage reports */
        if (sockReport && tmSegment%SEG_PER_SOCKREPORT == 0) {
          sprintf(buf, "%ld/%ld active sockets (%ld/%ld offsite)\n", sockNum, sockMaxNum, sockNumOffsite, sockMaxOffsite);
          Log(LOG_PORT, buf);
        }
  
        /* Give flagged areas a chance to reset (at most one per idle) */
        if (tmSegment%SEG_PER_RESETIDLE==0) {
          ResetIdle();
        }
  
        /* let even #'d mobiles with no attached code move etc. */
        if (tmSegment%SEG_PER_CHARIDLE==1) {
          for(x=0; x<mobileIndex.iNum; x++) {
            if (!BIT(mobileIndex.iThing[x]->tFlag, TF_IDLECODE)
               &&Mob(mobileIndex.iThing[x])->mTemplate->mVirtual%2==0)
              MobileIdle(mobileIndex.iThing[x]);
          }
        }
  
        /* let odd #'d mobiles with no attached code move etc. */
        if (tmSegment%SEG_PER_CHARIDLE==2) {
          for(x=0; x<mobileIndex.iNum; x++) {
            if (!BIT(mobileIndex.iThing[x]->tFlag, TF_IDLECODE)
               &&Mob(mobileIndex.iThing[x])->mTemplate->mVirtual%2==1)
              MobileIdle(mobileIndex.iThing[x]);
          }
        }
  
        /* let mobiles with attached code execute */
        if (tmSegment%SEG_PER_CHARIDLE==3) {
          for(x=0; x<mobileIndex.iNum; x++) 
            BITSET(mobileIndex.iThing[x]->tFlag, TF_CODETHING);
          for(x=0; x<mobileIndex.iNum; x++) {
            if (BIT(mobileIndex.iThing[x]->tFlag, TF_CODETHING)) {
              BITCLR(mobileIndex.iThing[x]->tFlag, TF_CODETHING);
              if (BIT(mobileIndex.iThing[x]->tFlag, TF_IDLECODE)) {
                MobileIdle(mobileIndex.iThing[x]);
                x=-1; /* Index may be scrambled, search from start */
              }
            }
          }
        }
  
        #if (OBJECT_IDLE_NOCODE) /* what would they do? */
          /* lets objects without attached code execute */
          if (tmSegment%SEG_PER_OBJIDLE==4) {
            for(x=0; x<objectIndex.iNum; x++) {
              if (!BIT(objectIndex.iThing[x]->tFlag, TF_IDLECODE))
                ObjectIdle(objectIndex.iThing[x]);
            }
          }
        #endif
          
        /* lets objects with attached code execute */
        if (tmSegment%SEG_PER_OBJIDLE==5) {
          for(x=0; x<objectIndex.iNum; x++)
            BITSET(objectIndex.iThing[x]->tFlag, TF_CODETHING);
          for(x=0; x<objectIndex.iNum; x++) {
            if (BIT(objectIndex.iThing[x]->tFlag, TF_CODETHING)) {
              BITCLR(objectIndex.iThing[x]->tFlag, TF_CODETHING);
              if (BIT(objectIndex.iThing[x]->tFlag, TF_IDLECODE)) {
                ObjectIdle(objectIndex.iThing[x]);
                x=-1; /* cant trust Index now, search from start */
              }
            }
          }
        }
          
        /* let worlds with attached code execute */
        if (tmSegment%SEG_PER_WLDIDLE==6) {
          for(area=0; area<areaListMax; area++) {
            for(x=0; x<areaList[area].aWldIndex.iNum; x++) {
              if (BIT(areaList[area].aWldIndex.iThing[x]->tFlag, TF_IDLECODE))
                CodeParseIdle(areaList[area].aWldIndex.iThing[x]);
            }
          }
        }
        
        /* let players without attached code execute */
        if (tmSegment%SEG_PER_CHARIDLE==7) {
          PlayerIdle(); /* constantly update crash files */
        }
  
  
      }
      
      /* prompts and send output as necessary */
      SockWriteCheckAll();
    }
  }
 
  /* Save the boards */
  BoardIdle();
 
  /* Normal Exit - do normal exit stuff */
  printf("\n");
  printf("=====================================\n");
  printf("      Normal Game Termination\n");
  printf("=====================================\n");
  SockCloseAll(portFD);
  EXIT(ERROR_NOERROR);

  /* MS Crap compiler complains otherwise */
  return ERROR_NOERROR;
}

