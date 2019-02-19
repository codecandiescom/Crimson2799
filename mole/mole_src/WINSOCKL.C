/* winsockl.c
 * This library is designed to provide WinSock functionality, but without
 * the requirement that WINSOCK.DLL be present and/or functioning.
 * This module frees the programmer from having to:
 *  a) link in WINSOCK.DLL and forever be bound to a single implementation
 *     of it, or
 *  b) dynamically link in WINSOCK.DLL, preventing the program from ever
 *     being run if WINSOCK.DLL is not available.
 * For applications which use WINSOCK.DLL, but which still want to function
 * without it being present, this file provides the WinSock Link!
 * All your winsock functions are implemented here, and these routes
 * call the real WINSOCK.DLL routines by using more tedious language.
 * IE: the dll is manually loaded and called from within this library.
 * Thus, WINSOCK.DLL doesn't have to be present for your program to link
 * and run.
 *
 * This program uses the standard WINSOCK.H header file as defined in the
 * standard WinSock 1.1 documentation.
 */                  
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

#include<windows.h>
#include"winsock.h"

/* Globals */
/* All globals in this module start with the letters "wl" for Winsock Link. */
HINSTANCE wlhInst=NULL;
BOOL lwNetReady=FALSE;

SOCKET PASCAL (FAR *wl_accept) (SOCKET s, struct sockaddr FAR *addr,int FAR *addrlen);
int PASCAL (FAR *wl_bind) (SOCKET s, const struct sockaddr FAR *addr, int namelen);
int PASCAL (FAR *wl_closesocket) (SOCKET s);
int PASCAL (FAR *wl_connect) (SOCKET s, const struct sockaddr FAR *name, int namelen);
int PASCAL (FAR *wl_ioctlsocket) (SOCKET s, long cmd, u_long FAR *argp);
int PASCAL (FAR *wl_getpeername) (SOCKET s, struct sockaddr FAR *name,int FAR * namelen);
int PASCAL (FAR *wl_getsockname) (SOCKET s, struct sockaddr FAR *name,int FAR * namelen);
int PASCAL (FAR *wl_getsockopt) (SOCKET s, int level, int optname,char FAR * optval, int FAR *optlen);
u_long PASCAL (FAR *wl_htonl) (u_long hostlong);
u_short PASCAL (FAR *wl_htons) (u_short hostshort);
unsigned long PASCAL (FAR *wl_inet_addr) (const char FAR * cp);
char FAR * PASCAL (FAR *wl_inet_ntoa) (struct in_addr in);
int PASCAL (FAR *wl_listen) (SOCKET s, int backlog);
u_long PASCAL (FAR *wl_ntohl) (u_long netlong);
u_short PASCAL (FAR *wl_ntohs) (u_short netshort);
int PASCAL (FAR *wl_recv) (SOCKET s, char FAR * buf, int len, int flags);
int PASCAL (FAR *wl_recvfrom) (SOCKET s, char FAR * buf, int len, int flags,struct sockaddr FAR *from, int FAR * fromlen);
int PASCAL (FAR *wl_select) (int nfds, fd_set FAR *readfds, fd_set FAR *writefds,fd_set FAR *exceptfds, const struct timeval FAR *timeout);
int PASCAL (FAR *wl_send) (SOCKET s, const char FAR * buf, int len, int flags);
int PASCAL (FAR *wl_sendto) (SOCKET s, const char FAR * buf, int len, int flags,const struct sockaddr FAR *to, int tolen);
int PASCAL (FAR *wl_setsockopt) (SOCKET s, int level, int optname,const char FAR * optval, int optlen);
int PASCAL (FAR *wl_shutdown) (SOCKET s, int how);
SOCKET PASCAL (FAR *wl_socket) (int af, int type, int protocol);

/* Database function prototypes */
struct hostent FAR * PASCAL (FAR *wl_gethostbyaddr)(const char FAR * addr,int len, int type);
struct hostent FAR * PASCAL (FAR *wl_gethostbyname)(const char FAR * name);
int PASCAL (FAR *wl_gethostname) (char FAR * name, int namelen);
struct servent FAR * PASCAL (FAR *wl_getservbyport)(int port, const char FAR * proto);
struct servent FAR * PASCAL (FAR *wl_getservbyname)(const char FAR * name,const char FAR * proto);
struct protoent FAR * PASCAL (FAR *wl_getprotobynumber)(int proto);
struct protoent FAR * PASCAL (FAR *wl_getprotobyname)(const char FAR * name);

/* Microsoft Windows Extension function prototypes */
int PASCAL (FAR *wl_WSAStartup)(WORD, LPWSADATA);
int PASCAL (FAR *wl_WSACleanup)(void);
void PASCAL (FAR *wl_WSASetLastError)(int iError);
int PASCAL (FAR *wl_WSAGetLastError)(void);
BOOL PASCAL (FAR *wl_WSAIsBlocking)(void);
int PASCAL (FAR *wl_WSAUnhookBlockingHook)(void);
FARPROC PASCAL (FAR *wl_WSASetBlockingHook)(FARPROC lpBlockFunc);
int PASCAL (FAR *wl_WSACancelBlockingCall)(void);
HANDLE PASCAL (FAR *wl_WSAAsyncGetServByName)(HWND hWnd, u_int wMsg,
																				const char FAR * name,
																				const char FAR * proto,
																				char FAR * buf, int buflen);
HANDLE PASCAL (FAR *wl_WSAAsyncGetServByPort)(HWND hWnd, u_int wMsg, int port,
																				const char FAR * proto, char FAR * buf,
																				int buflen);
HANDLE PASCAL (FAR *wl_WSAAsyncGetProtoByName)(HWND hWnd, u_int wMsg,
																				 const char FAR * name, char FAR * buf,
																				 int buflen);
HANDLE PASCAL (FAR *wl_WSAAsyncGetProtoByNumber)(HWND hWnd, u_int wMsg,
																					 int number, char FAR * buf,
																					 int buflen);
HANDLE PASCAL (FAR *wl_WSAAsyncGetHostByName)(HWND hWnd, u_int wMsg,
																				const char FAR * name, char FAR * buf,
																				int buflen);
HANDLE PASCAL (FAR *wl_WSAAsyncGetHostByAddr)(HWND hWnd, u_int wMsg,
																				const char FAR * addr, int len, int type,
																				char FAR * buf, int buflen);
int PASCAL (FAR *wl_WSACancelAsyncRequest)(HANDLE hAsyncTaskHandle);
int PASCAL (FAR *wl_WSAAsyncSelect)(SOCKET s, HWND hWnd, u_int wMsg,long lEvent);
int PASCAL (FAR *wl_WSAFDIsSet)(SOCKET s, fd_set FAR *f);
/********************** End of Globals *************************************/

/********** WinSockL Specialty Calls ***********/
/* This procedure detects for WINSOCK.DLL */
/* Basically this procedure does a (reletively) non-intrusive
 * probe of the network to see if we have networking capabilities.
 * Return value is -1 if it looks like we can network, or an error
 * code > -1 if we ran into a problem */
int WSAWinsockDetected()
	{
	UINT l_emf;
	int l_rc=-1;

#ifdef WIN32
	if (wlhInst != NULL)
  	return l_rc;
#else
	if (wlhInst>HINSTANCE_ERROR) // check if we're already installed
		return l_rc;
#endif

	/* this is the function which checks if we've got winsock.dll available */
	l_emf=SetErrorMode(SEM_NOOPENFILEERRORBOX);
#ifdef WIN32
	wlhInst=LoadLibrary("WSOCK32.DLL");
#else
	wlhInst=LoadLibrary("WINSOCK.DLL");
#endif
	SetErrorMode(l_emf);

#ifdef WIN32
	if (!wlhInst)
  	l_rc=GetLastError();
#else
	if (wlhInst<=HINSTANCE_ERROR)
		l_rc=wlhInst;
#endif
	else
		FreeLibrary(wlhInst);
	wlhInst=NULL;
	return(l_rc);
	}
/********** WinSock Calls ***********/
SOCKET PASCAL FAR accept (SOCKET s, struct sockaddr FAR *addr,
                          int FAR *addrlen)
  {
  return (*wl_accept)(s,addr,addrlen);
  }

int PASCAL FAR bind (SOCKET s, const struct sockaddr FAR *addr, int namelen)
  {
  return (*wl_bind)(s,addr,namelen);
  }


int PASCAL FAR closesocket (SOCKET s)
  {
  return (*wl_closesocket)(s);
  }


int PASCAL FAR connect (SOCKET s, const struct sockaddr FAR *name, int namelen)
  {
  return (*wl_connect)(s,name,namelen);
  }


int PASCAL FAR ioctlsocket (SOCKET s, long cmd, u_long FAR *argp)
  {
  return (*wl_ioctlsocket)(s,cmd,argp);
  }


int PASCAL FAR getpeername (SOCKET s, struct sockaddr FAR *name,
                            int FAR * namelen)
  {
  return (*wl_getpeername)(s,name,namelen);
  }


int PASCAL FAR getsockname (SOCKET s, struct sockaddr FAR *name,
                            int FAR * namelen)
  {
  return (*wl_getsockname)(s,name,namelen);
  }


int PASCAL FAR getsockopt (SOCKET s, int level, int optname,
                           char FAR * optval, int FAR *optlen)
  {
  return (*wl_getsockopt)(s,level,optname,optval,optlen);
  }


u_long PASCAL FAR htonl (u_long hostlong)
  {
  return (*wl_htonl)(hostlong);
  }


u_short PASCAL FAR htons (u_short hostshort)
  {
  return (*wl_htons)(hostshort);
  }


unsigned long PASCAL FAR inet_addr (const char FAR * cp)
  {
  return (*wl_inet_addr)(cp);
  }


char FAR * PASCAL FAR inet_ntoa (struct in_addr in)
  {
  return (*wl_inet_ntoa)(in);
  }


int PASCAL FAR listen (SOCKET s, int backlog)
  {
  return (*wl_listen)(s,backlog);
  }


u_long PASCAL FAR ntohl (u_long netlong)
  {
  return (*wl_ntohl)(netlong);
  }


u_short PASCAL FAR ntohs (u_short netshort)
  {
  return (*wl_ntohs)(netshort);
  }


int PASCAL FAR recv (SOCKET s, char FAR * buf, int len, int flags)
  {
  return (*wl_recv)(s,buf,len,flags);
  }


int PASCAL FAR recvfrom (SOCKET s, char FAR * buf, int len, int flags,
                         struct sockaddr FAR *from, int FAR * fromlen)
  {
  return (*wl_recvfrom)(s,buf,len,flags,from,fromlen);
  }


int PASCAL FAR select (int nfds, fd_set FAR *readfds, fd_set FAR *writefds,
                       fd_set FAR *exceptfds, const struct timeval FAR *timeout)
  {
  return (*wl_select)(nfds,readfds,writefds,exceptfds,timeout);
  }


int PASCAL FAR send (SOCKET s, const char FAR * buf, int len, int flags)
  {
  return (*wl_send)(s,buf,len,flags);
  }


int PASCAL FAR sendto (SOCKET s, const char FAR * buf, int len, int flags,
                       const struct sockaddr FAR *to, int tolen)
  {
  return (*wl_sendto)(s,buf,len,flags,to,tolen);
  }


int PASCAL FAR setsockopt (SOCKET s, int level, int optname,
                           const char FAR * optval, int optlen)
  {
  return (*wl_setsockopt)(s,level,optname,optval,optlen);
  }


int PASCAL FAR shutdown (SOCKET s, int how)
  {
  return (*wl_shutdown)(s,how);
  }


SOCKET PASCAL FAR socket (int af, int type, int protocol)
  {
  return (*wl_socket)(af,type,protocol);
  }


/* Database function prototypes */

struct hostent FAR * PASCAL FAR gethostbyaddr(const char FAR * addr,
                                              int len, int type)
  {
  return (*wl_gethostbyaddr)(addr,len,type);
  }


struct hostent FAR * PASCAL FAR gethostbyname(const char FAR * name)
  {
  return (*wl_gethostbyname)(name);
  }


int PASCAL FAR gethostname (char FAR * name, int namelen)
  {
  return (*wl_gethostname)(name,namelen);
  }


struct servent FAR * PASCAL FAR getservbyport(int port, const char FAR * proto)
  {
  return (*wl_getservbyport)(port,proto);
  }


struct servent FAR * PASCAL FAR getservbyname(const char FAR * name,
                                              const char FAR * proto)
  {
  return (*wl_getservbyname)(name,proto);
  }


struct protoent FAR * PASCAL FAR getprotobynumber(int proto)
  {
  return (*wl_getprotobynumber)(proto);
  }


struct protoent FAR * PASCAL FAR getprotobyname(const char FAR * name)
  {
  return (*wl_getprotobyname)(name);
  }


/* Microsoft Windows Extension function prototypes */

int PASCAL FAR WSAStartup(WORD wVersionRequired, LPWSADATA lpWSAData)
  {
  UINT l_emf;

  /* this is the function which checks if we've got winsock.dll available, and loads it. */
  l_emf=SetErrorMode(SEM_NOOPENFILEERRORBOX);

#ifdef WIN32
	wlhInst=LoadLibrary("WSOCK32.DLL");
	if (wlhInst==0)
#else
	wlhInst=LoadLibrary("WINSOCK.DLL");
	if (wlhInst<=HINSTANCE_ERROR)
#endif
    {
//    switch(wlhInst)
//      {
//      case 0: // system error
//      case 2: // file not found
//      case 3: // path not found
//      case 5: // sharing/protection error
//      case 8: // lib required separate data segment for each task
//      case 10: // Incorrect Windows version
//      case 11: // Executable file is invalid
//      case 12: // Wrong O/S
//      case 13: // Designed for DOS 4.0
//      case 14: // Unknown executable type/format
//      case 15: // attempt to load real-mode executable (earlier version of Windows)
//      case 16: // Attempt was made to load a second instance of an executable file containing multiple data segments that were not marked read-only
//      case 19: // Attempt to load compressed executable file. Decompress it first.
//      case 20: // DLL file is invalid. DLL file is corrupt.
//      case 21: // Requires 32-bit extension
//      default:
//        break;
//      }
    SetErrorMode(l_emf);
    return WSASYSNOTREADY;
    }
  if (((FARPROC)wl_WSAStartup=GetProcAddress(wlhInst,"WSAStartup"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSACleanup=GetProcAddress(wlhInst,"WSACleanup"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_accept=GetProcAddress(wlhInst,"accept"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_bind=GetProcAddress(wlhInst,"bind"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_closesocket=GetProcAddress(wlhInst,"closesocket"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_connect=GetProcAddress(wlhInst,"connect"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_ioctlsocket=GetProcAddress(wlhInst,"ioctlsocket"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_getpeername=GetProcAddress(wlhInst,"getpeername"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_getsockname=GetProcAddress(wlhInst,"getsockname"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_getsockopt=GetProcAddress(wlhInst,"getsockopt"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_htonl=GetProcAddress(wlhInst,"htonl"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_htons=GetProcAddress(wlhInst,"htons"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_inet_addr=GetProcAddress(wlhInst,"inet_addr"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_inet_ntoa=GetProcAddress(wlhInst,"inet_ntoa"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_listen=GetProcAddress(wlhInst,"listen"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_ntohl=GetProcAddress(wlhInst,"ntohl"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_ntohs=GetProcAddress(wlhInst,"ntohs"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_recv=GetProcAddress(wlhInst,"recv"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_recvfrom=GetProcAddress(wlhInst,"recvfrom"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_select=GetProcAddress(wlhInst,"select"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_send=GetProcAddress(wlhInst,"send"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_sendto=GetProcAddress(wlhInst,"sendto"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_setsockopt=GetProcAddress(wlhInst,"setsockopt"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_shutdown=GetProcAddress(wlhInst,"shutdown"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_socket=GetProcAddress(wlhInst,"socket"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_gethostbyaddr=GetProcAddress(wlhInst,"gethostbyaddr"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_gethostbyname=GetProcAddress(wlhInst,"gethostbyname"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_gethostname=GetProcAddress(wlhInst,"gethostname"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_getservbyport=GetProcAddress(wlhInst,"getservbyport"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_getservbyname=GetProcAddress(wlhInst,"getservbyname"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_getprotobynumber=GetProcAddress(wlhInst,"getprotobynumber"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_getprotobyname=GetProcAddress(wlhInst,"getprotobyname"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSASetLastError=GetProcAddress(wlhInst,"WSASetLastError"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAGetLastError=GetProcAddress(wlhInst,"WSAGetLastError"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAIsBlocking=GetProcAddress(wlhInst,"WSAIsBlocking"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAUnhookBlockingHook=GetProcAddress(wlhInst,"WSAUnhookBlockingHook"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSASetBlockingHook=GetProcAddress(wlhInst,"WSASetBlockingHook"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSACancelBlockingCall=GetProcAddress(wlhInst,"WSACancelBlockingCall"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAAsyncGetServByName=GetProcAddress(wlhInst,"WSAAsyncGetServByName"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAAsyncGetServByPort=GetProcAddress(wlhInst,"WSAAsyncGetServByPort"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAAsyncGetProtoByName=GetProcAddress(wlhInst,"WSAAsyncGetProtoByName"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAAsyncGetProtoByNumber=GetProcAddress(wlhInst,"WSAAsyncGetProtoByNumber"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAAsyncGetHostByName=GetProcAddress(wlhInst,"WSAAsyncGetHostByName"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAAsyncGetHostByAddr=GetProcAddress(wlhInst,"WSAAsyncGetHostByAddr"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSACancelAsyncRequest=GetProcAddress(wlhInst,"WSACancelAsyncRequest"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAAsyncSelect=GetProcAddress(wlhInst,"WSAAsyncSelect"))==NULL)
    return WSASYSNOTREADY;
  if (((FARPROC)wl_WSAFDIsSet=GetProcAddress(wlhInst,"__WSAFDIsSet"))==NULL)
    return WSASYSNOTREADY;
  lwNetReady=TRUE;
  return (*wl_WSAStartup)(wVersionRequired,lpWSAData);
  }


int PASCAL FAR WSACleanup(void)
  {
  int rc=0;
#ifdef WIN32
	if (wlhInst)
#else
	if (wlhInst>HINSTANCE_ERROR)
#endif
		{
    rc=(*wl_WSACleanup)();
    FreeLibrary(wlhInst);
    lwNetReady=FALSE;
    }
  return(rc);
  }


void PASCAL FAR WSASetLastError(int iError)
  {
  (*wl_WSASetLastError)(iError);
  return;
  }


int PASCAL FAR WSAGetLastError(void)
  {
  return (*wl_WSAGetLastError)();
  }


BOOL PASCAL FAR WSAIsBlocking(void)
  {
  if (lwNetReady)
    return (*wl_WSAIsBlocking)();
  else
    return FALSE;
  }


int PASCAL FAR WSAUnhookBlockingHook(void)
  {
  return (*wl_WSAUnhookBlockingHook)();
  }


FARPROC PASCAL FAR WSASetBlockingHook(FARPROC lpBlockFunc)
  {
  return (*wl_WSASetBlockingHook)(lpBlockFunc);
  }


int PASCAL FAR WSACancelBlockingCall(void)
  {
  if (lwNetReady)
		return (*wl_WSACancelBlockingCall)();
	return 0;
  }


HANDLE PASCAL FAR WSAAsyncGetServByName(HWND hWnd, u_int wMsg,
                                        const char FAR * name,
                                        const char FAR * proto,
                                        char FAR * buf, int buflen)
  {
  return (*wl_WSAAsyncGetServByName)(hWnd,wMsg,name,proto,buf,buflen);
  }


HANDLE PASCAL FAR WSAAsyncGetServByPort(HWND hWnd, u_int wMsg, int port,
                                        const char FAR * proto, char FAR * buf,
                                        int buflen)
  {
  return (*wl_WSAAsyncGetServByPort)(hWnd,wMsg,port,proto,buf,buflen);
  }


HANDLE PASCAL FAR WSAAsyncGetProtoByName(HWND hWnd, u_int wMsg,
                                         const char FAR * name, char FAR * buf,
                                         int buflen)
  {
  return (*wl_WSAAsyncGetProtoByName)(hWnd,wMsg,name,buf,buflen);
  }


HANDLE PASCAL FAR WSAAsyncGetProtoByNumber(HWND hWnd, u_int wMsg,
                                           int number, char FAR * buf,
                                           int buflen)
  {
  return (*wl_WSAAsyncGetProtoByNumber)(hWnd,wMsg,number,buf,buflen);
  }


HANDLE PASCAL FAR WSAAsyncGetHostByName(HWND hWnd, u_int wMsg,
                                        const char FAR * name, char FAR * buf,
                                        int buflen)
  {
  return (*wl_WSAAsyncGetHostByName)(hWnd,wMsg,name,buf,buflen);
  }


HANDLE PASCAL FAR WSAAsyncGetHostByAddr(HWND hWnd, u_int wMsg,
                                        const char FAR * addr, int len, int type,
                                        char FAR * buf, int buflen)
  {
  return (*wl_WSAAsyncGetHostByAddr)(hWnd,wMsg,addr,len,type,buf,buflen);
  }


int PASCAL FAR WSACancelAsyncRequest(HANDLE hAsyncTaskHandle)
  {
  return (*wl_WSACancelAsyncRequest)(hAsyncTaskHandle);
  }


int PASCAL FAR WSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg,
                               long lEvent)
  {
  return (*wl_WSAAsyncSelect)(s,hWnd,wMsg,lEvent);
  }

int PASCAL FAR __WSAFDIsSet(SOCKET s, fd_set FAR *f)
  {
  return (*wl_WSAFDIsSet)(s,f);
  }

