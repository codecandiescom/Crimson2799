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

#ifdef WIN32

#include <stdio.h>
#include <stdlib.h>

#include "crimson2.h"
#include "WinsockExt.h"

#ifdef WIN32
  /* including winsock.h etc auto includes the windows types */
  /* turn off warning: benign type redefinition */
  #pragma warning( disable : 4142 )
  #include <windows.h>
  #include <winsock.h>
  #pragma warning( default : 4142 )
#endif

BYTE *WSAGetErrorString(LWORD nErr) {
  BYTE *lpszRetStr;

  switch(nErr) {
  case WSAVERNOTSUPPORTED:
    lpszRetStr="version of WinSock not supported";
    break;
  case WSASYSNOTREADY:
    lpszRetStr="WinSock not present or not responding";
    break;
  case WSAEINVAL:
    lpszRetStr="app version not supported by DLL";
    break;
  case WSAHOST_NOT_FOUND:
    lpszRetStr="Authoritive: Host not found";
    break;
  case WSATRY_AGAIN:
    lpszRetStr="Non-authoritive: host not found or server failure";
    break;
  case WSANO_RECOVERY:
    lpszRetStr="Non-recoverable: refused or not implemented";
    break;
  case WSANO_DATA:
    lpszRetStr="Valid name, no data record for type";
    break;
/*
  case WSANO_ADDRESS:
    lpszRetStr="Valid name, no MX record";
    break;
*/
  case WSANOTINITIALISED:
    lpszRetStr="WSA Startup not initialized";
    break;
  case WSAENETDOWN:
    lpszRetStr="Network subsystem failed";
    break;
  case WSAEINTR:
    lpszRetStr="Blocking call cancelled";
    break;
  case WSAEAFNOSUPPORT:
    lpszRetStr="address family not supported";
    break;
  case WSAEMFILE:
    lpszRetStr="no file descriptors available";
    break;
  case WSAENOBUFS:
    lpszRetStr="no buffer space available";
    break;
  case WSAEPROTONOSUPPORT:
    lpszRetStr="specified protocol not supported";
    break;
  case WSAEPROTOTYPE:
    lpszRetStr="protocol wrong type for this socket";
    break;
  case WSAESOCKTNOSUPPORT:
    lpszRetStr="socket type not supported for address family";
    break;
  case WSAENOTSOCK:
    lpszRetStr="descriptor is not a socket";
    break;
  case WSAEWOULDBLOCK:
    lpszRetStr="socket marked as non-blocking and SO_LINGER set not 0";
    break;
  case WSAEADDRINUSE:
    lpszRetStr="address already in use";
    break;
  case WSAECONNABORTED:
    lpszRetStr="connection aborted";
    break;
  case WSAECONNRESET:
    lpszRetStr="connection reset";
    break;
  case WSAENOTCONN:
    lpszRetStr="not connected";
    break;
  case WSAETIMEDOUT:
    lpszRetStr="connection timed out";
    break;
  case WSAECONNREFUSED:
    lpszRetStr="connection refused";
    break;
  case WSAEHOSTDOWN:
    lpszRetStr="host down";
    break;
  case WSAENETUNREACH:
    lpszRetStr="network unreachable";
    break;
  case WSAEHOSTUNREACH:
    lpszRetStr="host unreachable";
    break;
  case WSAEADDRNOTAVAIL:
    lpszRetStr="address not available";
    break;
  case WSAEINPROGRESS:
    lpszRetStr="A blocking Windows Sockets call is in progress.";
    break;
    
/* * Doesnt seem to know what this is *
  case WSAEDESTADDREQ:
    lpszRetStr="A destination address is required.";
    break;
*/
  case WSAEFAULT:
    lpszRetStr="The namelen argument is incorrect.";
    break;
  case WSAEISCONN:
    lpszRetStr="The socket is already connected.";
    break;
  case WSAENETRESET:
    lpszRetStr="The connection has been broken due to the remote host resetting.";
    break;
  case WSAEOPNOTSUPP:
    lpszRetStr="MSG_OOB was specified, but socket not SOCK_STREAM, or the socket is unidirectional and supports only send operations.";
    break;
  case WSAESHUTDOWN:
    lpszRetStr="The socket has been shut down; it is not possible to recv on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.";
    break;
  case WSAEMSGSIZE:
    lpszRetStr="The message was too large to fit into the specified buffer and was truncated.";
    break;
  default:
    lpszRetStr="Undefined error";
    break;
  }
  return lpszRetStr;
}

BYTE *WSAGetLastErrorString() {
  LWORD   nErr;

  nErr = WSAGetLastError();
  return WSAGetErrorString(nErr);
}

#endif