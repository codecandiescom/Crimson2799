// MOLE Client for MS Windows 3.11 (WIN32)
// host.c identifier: ho
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1997-1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************
//
// Written by B. Cameron Lesiuk
// March, 1997

// This module looks after the Hosts Database, as well as host<->client
// interaction services (ie: protocol services)

#include<windows.h>
#include<windowsx.h>
#include<ctl3d.h>
#include<commdlg.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include"winsock.h"
#include"moledefs.h"
#include"molem.h"
#include"molerc.h"
#include"main.h"
#include"host.h"
#include"infobox.h"
#include"debug.h"
#include"dialog.h"
#include"moleprot.h"
#include"edit.h"
#include"areawnd.h"
#include"timer.h"
#include"help.h"
#include"ctl3dl.h"

/* host.c globals */
char g_hoHostName[HO_STRLEN]; /* server DNS name (if available) */
char g_hoHostIPStr[HO_STRLEN]; /* server network IP address (in numerical form) */
unsigned long g_hoHostIP; /* server IP address */
long g_hoHostPort; /* server port */
unsigned long g_hoTime; /* time connection to host was made */
SOCKET g_hoSock; /* socket for connection to host */
BOOL g_hoWSAStartup; /* startup successfully called */
HOHOST *g_hoHostList; /* list of recorded hosts */
BOOL g_hoConnectedToHost; /* set to TRUE if connected to host */
int g_hoConnectionState; /* connection is a state machine - offline,login,online,logoff */
HWND g_hoLoginWnd; /* handle of Login dialog window */

/* Globals used in this module exclusively */
char g_hoLoginUserName[HO_STRLEN];
char g_hoLoginPassword[HO_STRLEN];

BOOL hoInitHost() {
  unsigned int l_i,l_j;
  HGLOBAL l_hGlobalTemp;
  HOHOST *l_host;
  char l_sect[20];
  char l_buf[HO_STRLEN];

  g_hoLoginWnd=NULL;
  g_hoHostName[0]=0;
  g_hoHostIP=0L;
  g_hoHostPort=0L;
  g_hoTime=0L;
  g_hoWSAStartup=FALSE;
  g_hoConnectedToHost=FALSE;
	g_hoSock=INVALID_SOCKET;
  g_hoConnectionState=HO_CONNECT_STATE_OFFLINE;

  /* read in host database from .ini file */
  g_hoHostList=NULL;
  l_i=GetPrivateProfileInt(HO_INI_SECT,"num_hosts",0,g_aaInitFile);
  for (l_j=0;l_j<l_i;l_j++) {
    sprintf(l_sect,"host_%i",l_j);
    /* allocate a structure for this sucker */
  	l_hGlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(HOHOST));
  	l_host=(HOHOST *)GlobalLock(l_hGlobalTemp);
	  if ((!l_hGlobalTemp)||(!l_host)) {
	  	MessageBox(g_aahWnd,"Cannot allocate memory!?\rTry to free up some memory.","Very very bad.",MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);
		  return(FALSE);
 		}
	  l_host->hMemory=l_hGlobalTemp;

    /* read in values */
    GetPrivateProfileString(l_sect,"description","",l_host->hDescription,HO_STRLEN,g_aaInitFile);
    GetPrivateProfileString(l_sect,"IP_address","",l_host->hIP,HO_STRLEN,g_aaInitFile);
    l_host->hPort=GetPrivateProfileInt(l_sect,"port",4000,g_aaInitFile);
    GetPrivateProfileString(l_sect,"login","",l_host->hLoginID,HO_STRLEN,g_aaInitFile);
    GetPrivateProfileString(l_sect,"password","",l_buf,HO_STRLEN,g_aaInitFile);
    hoDecryptPassword(l_buf,l_host->hPassword);
    l_host->hID=hoGetHostID();

    /* and lastly, insert into linked list */
    l_host->hNext=g_hoHostList;
    g_hoHostList=l_host;
  }
  return TRUE;
}

void hoShutdownHost() {
  unsigned int l_i;
  HGLOBAL l_hGlobalTemp;
  HOHOST *l_host;
  char l_sect[20];
  char l_buf[HO_STRLEN];

  hoDisconnectHost(TRUE);

  if (g_hoWSAStartup)
    WSACleanup();

  /* write host database to .ini file */
  l_i=0;
  while(g_hoHostList) {
    sprintf(l_sect,"host_%i",l_i);

    /* Write values */
    WritePrivateProfileString(l_sect,"description",g_hoHostList->hDescription,g_aaInitFile);
    WritePrivateProfileString(l_sect,"IP_address",g_hoHostList->hIP,g_aaInitFile);
    sprintf(l_buf,"%i",g_hoHostList->hPort);
    WritePrivateProfileString(l_sect,"port",l_buf,g_aaInitFile);
    WritePrivateProfileString(l_sect,"login",g_hoHostList->hLoginID,g_aaInitFile);
    hoEncryptPassword(g_hoHostList->hPassword,l_buf);
    WritePrivateProfileString(l_sect,"password",l_buf,g_aaInitFile);

    /* and lastly, remove from linked list */
    l_host=g_hoHostList;
    g_hoHostList=g_hoHostList->hNext;
    /* and free the sucker */
    l_hGlobalTemp=l_host->hMemory;
  	GlobalUnlock(l_hGlobalTemp);
	  GlobalFree(l_hGlobalTemp);

    l_i++;
  }
  sprintf(l_buf,"%i",l_i);
  WritePrivateProfileString(HO_INI_SECT,"num_hosts",l_buf,g_aaInitFile);
  return;
}

void hoConnectHost() {
  WSADATA l_wsaData;
  int l_rc;

  if (!g_hoWSAStartup) {
  	l_rc=WSAStartup(0x0101,&l_wsaData);
    if((l_rc) ||((LOBYTE(l_wsaData.wVersion)!=1)||(HIBYTE(l_wsaData.wVersion)!=1))) { // Windows Sockets startup code (request version 1.1 of WINSOCK.DLL) {
      if (!l_rc)
        WSACleanup();
  		MessageBeep(MB_ICONEXCLAMATION);
  		MessageBox(NULL,"Could not open a useable version of WINSOCK.DLL.\nMOLE requires WinSock version 1.1.",
        "WinSock Error",MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);
      return;
    }
    g_hoWSAStartup=TRUE;
  }

  /* check for active connection already - close if found */
  hoDisconnectHost(FALSE);
  if (g_hoConnectedToHost) /* if active connection, and they kept it, this'll still be TRUE */
    return;

  /* Bring up dialog box with possible connections */
  DialogBox(g_aahInst,MAKEINTRESOURCE(DIALOG_OPENHOST),g_aahWnd,hoDialogOpenHostProc);

  /* we're done. */
  return;
}

void hoDisconnectHost(BOOL p_force) {
  char l_buf[300];

  if (g_hoConnectedToHost) {
    if (!p_force) {
      sprintf(l_buf,"You have an active connection with %s.\nAre you sure you wish to disconnect?",
        g_hoHostName);
      if (ibInfoBox(g_aahWnd,l_buf,"Disconnect from host",IB_YESNO|IB_RUNHELP,NULL,HP_IB_DISCONNECT_WARN)==IB_RNO)
        return;
    }

    /* disconnect from host */
    /* first, do friendly disconnect */
    if (!hoSocketFriendlyDisconnect())
      return;
    /* Then, clean up any garbage */
    hoSocketKilled();
  }
  return;
}

void hoEncryptPassword(char *p_password,char *p_encrypted) {
  char *l_p,*l_q;;
  l_p=p_password;
  l_q=p_encrypted;
  while(*l_p) {
    *l_q=*l_p;
    l_p++;
    l_q++;
  }
  *l_q=0;
  return;
}

void hoDecryptPassword(char *p_password,char *p_decrypted) {
  char *l_p,*l_q;;
  l_p=p_password;
  l_q=p_decrypted;
  while(*l_p) {
    *l_q=*l_p;
    l_p++;
    l_q++;
  }
  *l_q=0;
  return;
}

#pragma argsused
BOOL CALLBACK _export hoDialogOpenHostProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam)
	{
  HOHOST *l_host,*l_lasthost;
  long l_i,l_j;
  HWND l_hWnd;
  HGLOBAL l_hGlobalTemp;

  switch (p_message)
		{
    case WM_INITDIALOG:
      Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCentreDialogBox(p_hWnd);
      SendMessage(p_hWnd,WM_USER_OPENHOST_UPDATE,0,0L);
      return TRUE;
    case WM_USER_OPENHOST_UPDATE:
      l_hWnd=GetDlgItem(p_hWnd,IDC_OPENHOSTLIST);
      if (p_wParam) {
        /* First, delete all existing list boxes */
        l_i=SendMessage(l_hWnd,LB_GETCOUNT,0,0L);
        while(l_i)
          l_i=SendMessage(l_hWnd,LB_DELETESTRING,0,0L);
      }
      /* load up our list box with available hosts */
      for (l_host=g_hoHostList;l_host;l_host=l_host->hNext) {
        SendMessage(l_hWnd,LB_SETITEMDATA,
          (WPARAM)SendMessage(l_hWnd,LB_ADDSTRING,0,
          (LPARAM)(LPCSTR)(l_host->hDescription)),
          (LPARAM)(l_host->hID));
      }
      SendMessage(l_hWnd,LB_SETCARETINDEX,0,0L);
      return TRUE;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          EndDialog(p_hWnd,0);
					return TRUE;
				default:
					break;
				}
      break;
    case WM_COMMAND:
      l_hWnd=GetDlgItem(p_hWnd,IDC_OPENHOSTLIST);
      switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case IDC_OPENHOSTLIST:
          if (GET_WM_COMMAND_CMD(p_wParam,p_lParam)!=LBN_DBLCLK)
            break;
          /* else fall through to IDOK */
        case IDOK:
          /* try to open connection */
          l_j=SendMessage(l_hWnd,LB_GETCURSEL,0,0L);
          if (l_j==LB_ERR)
            return TRUE;
          l_i=SendMessage(l_hWnd,LB_GETITEMDATA,(WPARAM)l_j,0L);
          /* now pull up "connecting..." dialog box */
          if (DialogBoxParam(g_aahInst,MAKEINTRESOURCE(DIALOG_CONNECT),
                 p_hWnd,hoDialogConnectProc,(LPARAM)l_i)) {
            /* and now just exit! Yippie! */
            EndDialog(p_hWnd,1);
          }
          return TRUE;
//        case IDC_OPENHOSTTELNET:
//          /* try to open a telnet connection */
//          l_j=SendMessage(l_hWnd,LB_GETCURSEL,0,0L);
//          if (l_j==LB_ERR)
//            return TRUE;
//          l_i=SendMessage(l_hWnd,LB_GETITEMDATA,(WPARAM)l_j,0L);
//          /* now pull up "connecting..." dialog box */
//          if (DialogBoxParam(g_aahInst,MAKEINTRESOURCE(DIALOG_CONNECT),
//                 p_hWnd,hoDialogConnectProc,(LPARAM)l_i)) {
//            /* and now just exit! Yippie! */
//            EndDialog(p_hWnd,1);
//          }
				case IDHELP:
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_HO_DIALOG_OPENHOST_HELP);
          return TRUE;
        case IDCANCEL:
          EndDialog(p_hWnd,0);
          return TRUE;
        case IDC_OPENHOSTEDITHOST:
          l_j=SendMessage(l_hWnd,LB_GETCURSEL,0,0L);
          if (l_j==LB_ERR)
            return TRUE;
          l_i=SendMessage(l_hWnd,LB_GETITEMDATA,(WPARAM)l_j,0L);
          DialogBoxParam(g_aahInst,MAKEINTRESOURCE(DIALOG_NEWHOST),
            p_hWnd,hoDialogNewHostProc,(LPARAM)l_i);
          SendMessage(p_hWnd,WM_USER_OPENHOST_UPDATE,1,0L);
          return TRUE;
        case IDC_OPENHOSTNEWHOST:
          DialogBoxParam(g_aahInst,MAKEINTRESOURCE(DIALOG_NEWHOST),
            p_hWnd,hoDialogNewHostProc,0L);
          SendMessage(p_hWnd,WM_USER_OPENHOST_UPDATE,1,0L);
          return TRUE;
        case IDC_OPENHOSTDELETEHOST:
          l_j=SendMessage(l_hWnd,LB_GETCURSEL,0,0L);
          if (l_j==LB_ERR)
            return TRUE;
          l_i=SendMessage(l_hWnd,LB_GETITEMDATA,(WPARAM)l_j,0L);
          for (l_lasthost=l_host=g_hoHostList;l_host&&(l_host->hID != l_i);
            l_host=l_host->hNext) {
            l_lasthost=l_host;
          }
          if (l_host) {
            SendMessage(l_hWnd,LB_DELETESTRING,(WPARAM)l_j,0L);
            /* and remove it from the linked list */
            if (l_lasthost==l_host) {
              g_hoHostList=l_host->hNext;
            } else {
              l_lasthost->hNext=l_host->hNext;
            }
            l_hGlobalTemp=l_host->hMemory;
          	GlobalUnlock(l_hGlobalTemp);
        	  GlobalFree(l_hGlobalTemp);
          }
          return TRUE;
        default:
          break;
        }
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
	}

/* This function just returns an unused ID for use in linked list structures */
/* Note that 0 is considered an invalid ID. */
int hoGetHostID() {
  HOHOST *l_host;
  int l_id;

  l_id=1;
  while(l_id<32000) {
    for (l_host=g_hoHostList;l_host;l_host=l_host->hNext) {
      if (l_host->hID==l_id)
        break;
    }
    if (!l_host) /* we've found one! */
      return l_id;
    l_id++;
  }
  return 0;
}

BOOL CALLBACK _export hoDialogNewHostProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam)
	{
	HOHOST *l_host;
  unsigned long l_i;
  HGLOBAL l_hGlobalTemp;

  switch (p_message)
		{
    case WM_INITDIALOG:
      Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCentreDialogBox(p_hWnd);
      SetProp(p_hWnd,"HostID",(HANDLE)p_lParam);
      if (p_lParam) {
        /* Fill in the blanks with what's already there */
        for (l_host=g_hoHostList;l_host&&(l_host->hID != p_lParam);
          l_host,l_host=l_host->hNext);
        if (l_host) {
          SetDlgItemText(p_hWnd,IDC_NEWHOSTDESCRIPTION,l_host->hDescription);
          SetDlgItemText(p_hWnd,IDC_NEWHOSTIP,l_host->hIP);
          SetDlgItemInt(p_hWnd,IDC_NEWHOSTPORT,(WPARAM)(l_host->hPort),0);
          SetDlgItemText(p_hWnd,IDC_NEWHOSTLOGINID,l_host->hLoginID);
          SetDlgItemText(p_hWnd,IDC_NEWHOSTPASSWORD,l_host->hPassword);
        }
      }
      return TRUE;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          EndDialog(p_hWnd,0);
					return TRUE;
				default:
					break;
				}
      break;
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case IDOK:
          /* Save info into data structure */
          l_i=(unsigned long)GetProp(p_hWnd,"HostID");
          l_host=NULL;
          if (l_i) {
            for (l_host=g_hoHostList;l_host&&(l_host->hID != l_i);l_host=l_host->hNext);
          }
          if (!l_host) {
            /* Create the host first */
          	l_hGlobalTemp=GlobalAlloc(GHND|GMEM_NOCOMPACT,sizeof(HOHOST));
          	l_host=(HOHOST *)GlobalLock(l_hGlobalTemp);
        	  if ((!l_hGlobalTemp)||(!l_host)) {
        	  	MessageBox(g_aahWnd,"Cannot allocate memory!?\rTry to free up some memory.","Very very bad.",MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);
        		  return(FALSE);
         		}
        	  l_host->hMemory=l_hGlobalTemp;
            l_host->hID=hoGetHostID();
            /* insert into linked list */
            l_host->hNext=g_hoHostList;
            g_hoHostList=l_host;
          }
          /* now fill in host structure */
          GetDlgItemText(p_hWnd,IDC_NEWHOSTDESCRIPTION,l_host->hDescription,HO_STRLEN);
          GetDlgItemText(p_hWnd,IDC_NEWHOSTIP,l_host->hIP,HO_STRLEN);
          l_host->hPort=GetDlgItemInt(p_hWnd,IDC_NEWHOSTPORT,NULL,FALSE);
          GetDlgItemText(p_hWnd,IDC_NEWHOSTLOGINID,l_host->hLoginID,HO_STRLEN);
          GetDlgItemText(p_hWnd,IDC_NEWHOSTPASSWORD,l_host->hPassword,HO_STRLEN);
          EndDialog(p_hWnd,1);
          return TRUE;
        case IDCANCEL:
          EndDialog(p_hWnd,0);
          return TRUE;
				case IDHELP:
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_HO_DIALOG_NEWHOST_HELP);
          return TRUE;
        default:
          break;
        }
      break;
    case WM_DESTROY:
      RemoveProp(p_hWnd,"HostID");
      break;
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
	}

BOOL CALLBACK _export hoDialogConnectProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam)
	{
  HOHOST *l_host;
  char l_buf[3*HO_STRLEN];
  BOOL l_bool,l_error;
  SOCKADDR_IN l_sin;
  struct hostent *l_hostent;
  int l_ec;
  unsigned long l_addr;

  switch (p_message)
		{
    case WM_INITDIALOG:
      Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
      dlCentreDialogBox(p_hWnd);
      g_hoLoginWnd=p_hWnd;
      PostMessage(p_hWnd,WM_USER_CONNECT_CONNECT,0,p_lParam);
      return TRUE;
    case WM_DESTROY:
      g_hoLoginWnd=NULL;
      break;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          EndDialog(p_hWnd,0);
					return TRUE;
				default:
					break;
				}
      break;
    case WM_USER_LOGIN_IDENTIFIED:
      /* check versions */
      /* Major versions must match in order for a connection to be viable. */
      /* Minor versions can vary. */
      /* There's no backward-compatibility here, folks, so don't even think about it.
       * This ain't DOS, you know. You're lucky I don't put an expiry-date in
       * this sucker to FORCE you to upgrade. */
      tmTimerAdjust(TM_DEFAULT_TIMER_PERIOD);
      if (((g_mpHostVersion&0x0000FF00L)>>8)!=MOLE_MAJOR_VERSION) {
        ibInfoBox(p_hWnd,"MOLE cannot talk to this server (incompatible versions).\nContact your SysAdmin regarding gettting the proper version.",
          "Incorrect MOLE Version",IB_OK|IB_RUNHELP,NULL,HP_IB_BAD_MOLE_VERSION);
        hoSocketKilled();
      } else {
        mpReqSubmitEx(MOLE_CMD_SYSR,NULL,0);
        /* now wait until we're out of _LOGIN mode */
        sprintf(l_buf,"Initializing MOLE Session");
        SetDlgItemText(p_hWnd,IDC_CONNECTTEXT,l_buf);
        dbPrint(l_buf);
        mnMenuMainUpdate();
      }
      return TRUE;
    case WM_USER_LOGIN_COMPLETE:
      edFetchItem(EDFI_AREALIST,NULL,NULL,NULL);
      EndDialog(p_hWnd,1);
      return TRUE;
    case WM_USER_CONNECT_CONNECT:
//      SetProp(p_hWnd,"HostID",(HANDLE)p_lParam);
      if (p_lParam) {
        for (l_host=g_hoHostList;l_host&&(l_host->hID != p_lParam);
          l_host,l_host=l_host->hNext);
        if (l_host) {
          /* Let's give'er */
          sprintf(l_buf,"Acquiring socket...");
          SetDlgItemText(p_hWnd,IDC_CONNECTTEXT,l_buf);
          dbPrint(l_buf);
          g_hoSock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
          if (g_hoSock==INVALID_SOCKET) {
            l_ec=WSAGetLastError();
            sprintf(l_buf,"CONNECT ERROR: error=%i",l_ec);
            dbPrint(l_buf);
            if (l_ec!=WSAEINTR)
              ibInfoBox(p_hWnd,"Cannot get a free socket!","Darn",IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_NO_SOCK);
          } else {
            /* WO WO! We have a socket descriptor */
            sprintf(l_buf,"Setting socket options...");
            SetDlgItemText(p_hWnd,IDC_CONNECTTEXT,l_buf);
            dbPrint(l_buf);
            l_bool=FALSE;
            l_error=FALSE;
            /* Note on socket options:
             * Neither of these are really critical. And one user on a novell
             * network was getting errors (probably their winsock version).
             * I am guessing their winsock didn't support SO_KEEPALIVE. Anyways,
             * neither of these options is critical, so I've changed the code
             * to allow for errors here; we try to set the option, but if it
             * doesn't happen, we don't care. */
            /* Further note: what I *really* should do is check for the
             * error code WSAENOPROTOOPT and puke on any other error code,
             * but what the hey; there's no guarentee winsock implementers
             * properly use it anyways. */
            if (setsockopt(g_hoSock,SOL_SOCKET,SO_DONTLINGER,(char *)&l_bool,sizeof(BOOL))) {
              l_error=TRUE;
            }
//            l_bool=TRUE;
//            if (setsockopt(g_hoSock,SOL_SOCKET,SO_KEEPALIVE,(char *)&l_bool,sizeof(BOOL))) {
//              l_error=TRUE;
//            }
//            if (l_error) {
//              l_ec=WSAGetLastError();
//              sprintf(l_buf,"CONNECT ERROR: error=%i",l_ec);
//              dbPrint(l_buf);
//              if (l_ec!=WSAEINTR)
//                ibInfoBox(p_hWnd,"Error setting socket options.","Darn",IB_OK,NULL,0L);
//            }
//         else
            {
              /* WO WO! We've made it through socket options */
              /* let's check what kind of address we've been fed */
              l_addr=inet_addr(l_host->hIP);
              if (l_addr==INADDR_NONE) {
                sprintf(l_buf,"Looking up host %s in DNS...",l_host->hIP);
                SetDlgItemText(p_hWnd,IDC_CONNECTTEXT,l_buf);
                dbPrint(l_buf);
                l_hostent=gethostbyname(l_host->hIP);
                /* copy over our info to our local record */
                l_sin.sin_family=AF_INET;
                l_sin.sin_port=htons((u_short)l_host->hPort);
                l_sin.sin_addr.S_un.S_un_b.s_b1=l_hostent->h_addr[0];
                l_sin.sin_addr.S_un.S_un_b.s_b2=l_hostent->h_addr[1];
                l_sin.sin_addr.S_un.S_un_b.s_b3=l_hostent->h_addr[2];
                l_sin.sin_addr.S_un.S_un_b.s_b4=l_hostent->h_addr[3];
                if (!l_hostent) {
                  l_ec=WSAGetLastError();
                  sprintf(l_buf,"CONNECT ERROR: error=%i",l_ec);
                  dbPrint(l_buf);
                  sprintf(l_buf,"Couldn't resolve host name '%s'.",l_host->hIP);
                  if (l_ec!=WSAEINTR)
                    ibInfoBox(p_hWnd,l_buf,"Darn",IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_HOSTNAME);
                }
                /* and let's collect some data for ourselves while we're here */
                strcpy(g_hoHostName,l_hostent->h_name);
                strcpy(g_hoHostIPStr,inet_ntoa(l_sin.sin_addr));
                g_hoHostIP=l_sin.sin_addr.S_un.S_addr;
                g_hoHostPort=l_host->hPort;
              } else { /* address given to us as a number -- don't do any DNS lookup */
                /* copy over our info to our local record */
                l_sin.sin_family=AF_INET;
                l_sin.sin_port=htons((u_short)l_host->hPort);
                l_sin.sin_addr.S_un.S_un_b.s_b1=((unsigned char*)(&l_addr))[0];
                l_sin.sin_addr.S_un.S_un_b.s_b2=((unsigned char*)(&l_addr))[1];
                l_sin.sin_addr.S_un.S_un_b.s_b3=((unsigned char*)(&l_addr))[2];
                l_sin.sin_addr.S_un.S_un_b.s_b4=((unsigned char*)(&l_addr))[3];
                /* and let's collect some data for ourselves while we're here */
                strcpy(g_hoHostName,l_host->hIP);
                strcpy(g_hoHostIPStr,inet_ntoa(l_sin.sin_addr));
                g_hoHostIP=l_sin.sin_addr.S_un.S_addr;
                g_hoHostPort=l_host->hPort;
              }
              sprintf(l_buf,"Connecting to %s (%s) on port %i",
                g_hoHostName,g_hoHostIPStr,(int)(l_host->hPort));
              SetDlgItemText(p_hWnd,IDC_CONNECTTEXT,l_buf);
              dbPrint(l_buf);
              if (connect(g_hoSock,(struct sockaddr *)&l_sin,sizeof(struct sockaddr_in))) {
                l_ec=WSAGetLastError();
                sprintf(l_buf,"CONNECT ERROR: error=%i",l_ec);
                dbPrint(l_buf);
                if (l_ec==WSAECONNREFUSED)
                  ibInfoBox(p_hWnd,"Connection refused by host:\rServer isn't listening.","Connection Refused",
                    IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_REFUSED);
                else if (l_ec!=WSAEINTR)
                  ibInfoBox(p_hWnd,"Error connecting to host.","Darn",
                    IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_MISC);
              } else {
                /* WO WO! We're connected!!!!!!! OH MY GOD! */
                g_hoConnectedToHost=TRUE;
                g_hoConnectionState=HO_CONNECT_STATE_LOGIN;
                g_hoTime=time(NULL);
                strcpy(g_hoLoginUserName,l_host->hLoginID);
                strcpy(g_hoLoginPassword,l_host->hPassword);
                if ((!strlen(g_hoLoginUserName))||(!strlen(g_hoLoginPassword))) {
                  l_ec=DialogBox(g_aahInst,MAKEINTRESOURCE(DIALOG_LOGIN),p_hWnd,
                    hoDialogLoginProc);
                } else {
                  l_ec=1;
                }
                if (l_ec) {
                  sprintf(l_buf,"Logging in as %s",g_hoLoginUserName);
                  SetDlgItemText(p_hWnd,IDC_CONNECTTEXT,l_buf);
                  dbPrint(l_buf);
                  sprintf(l_buf,"%s\n%s\npl\n",g_hoLoginUserName,g_hoLoginPassword);
                  send(g_hoSock,l_buf,strlen(l_buf),0);
									if (WSAAsyncSelect(g_hoSock,g_aahWnd,WM_USER_HOST_SOCKET,
										FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE)) {
										sprintf(l_buf,"Error setting socket to Async mode. Connection aborted.");
                    dbPrint(l_buf);
                    ibInfoBox(p_hWnd,l_buf,"Darn",IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_ASYNC);
                  } else {
                    /* submit request for MOLE version information */
                    tmTimerAdjust(2000); /* set timer to 2 seconds */
                    mpReqSubmitEx(MOLE_CMD_IDRQ,NULL,0);
                    /* now wait until we're out of _LOGIN mode */
                    sprintf(l_buf,"Retrieving server information",l_host->hLoginID);
                    SetDlgItemText(p_hWnd,IDC_CONNECTTEXT,l_buf);
                    dbPrint(l_buf);
                    mnMenuMainUpdate();
                    return TRUE;
                  }
                }
              }
            }
          }
        }
      }
      hoSocketKilled();
      EndDialog(p_hWnd,0);
      return TRUE;
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case IDCANCEL:
          /* halt blocking socket call and exit */
          if (WSAIsBlocking())
            WSACancelBlockingCall();
          else {
            hoSocketKilled();
            EndDialog(p_hWnd,0);
          }
          return TRUE;
//				case IDHELP:
//          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,0);
//          return TRUE;
        default:
          break;
        }
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
	}

#define HOSOCKETHANDLER_BUFSIZE 1024
void hoSocketHandler(WPARAM p_wParam, LPARAM p_lParam) {
	long l_rc;
	unsigned long l_argp,l_i;
	char l_buf[HOSOCKETHANDLER_BUFSIZE];

	if (p_wParam==g_hoSock) {
		switch(WSAGETSELECTEVENT(p_lParam)) {
			case FD_READ:
				/* There's data to read - let's READ IT! */
				/* First, find out how much data there is to read */
				l_rc=ioctlsocket(g_hoSock,FIONREAD,&l_argp);
				if (l_rc) {
					switch(WSAGetLastError()) {
						case WSAENOTSOCK:
						case WSAEINVAL:
							hoSocketKilled();
							ibInfoBox(g_aahWnd,"Connection has been unexpectedly terminated.","Darn",
								IB_OK|IB_RUNHELP,NULL,0L);
							return;
						default:
							dbPrint("Error with socket during ioctlsocket()");
							break;
					}
					return; /* abort on error */
				}
				if (!l_argp) {
					/* socket has been closed */
					hoSocketKilled();
					ibInfoBox(g_aahWnd,"Connection closed by host.","Darn",
						IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_CLOSED);
					return; /* abort on error */
				}
				/* read in however much there is to read in, and feed it to our MOLE parser */
				/* we don't have to do this because WinSock will re-transmit
				 * a message for us to read again if we don't get all the data the first time */
				/* plus this way we avoid other problems by allowing other messages to
				 * get serviced in the mean time */
				l_i=l_argp;
				if (l_i>=HOSOCKETHANDLER_BUFSIZE-2) l_i=HOSOCKETHANDLER_BUFSIZE-2;
				l_rc=recv(g_hoSock,l_buf,(int)l_i,0);
				if (!l_rc) {
					/* socket has been closed */
					hoSocketKilled();
					ibInfoBox(g_aahWnd,"Connection closed by host.","Darn",IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_CLOSED);
					return; /* closed connection */
				} else if (l_rc==SOCKET_ERROR) {
					switch(WSAGetLastError()) {
						case WSAENOTCONN:
						case WSAENOTSOCK:
						case WSAESHUTDOWN:
						case WSAECONNABORTED:
							/* socket has been closed */
							hoSocketKilled();
							ibInfoBox(g_aahWnd,"Connection closed by host.","Darn",IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_CLOSED);
							return;
						case WSAECONNRESET: /* ??? what's this? */
						default:
							break; /* maybe we can still write ? */
					} /* switch */
        } else {
					/* WE HAVE DATA! */
					/* send to MOLE parser */
					mpMolePacketFilter(l_buf,l_rc);
					l_argp-=l_rc; /* subtract # bytes actually read in */
				}
				break;
			case FD_WRITE:
				/* socket ready for writing - do we need to write anything? */
				mpMoleSendData();
				break;
			case FD_OOB:
//				dbPrint("HO: OOB Data");
				l_rc=recv(g_hoSock,l_buf,HOSOCKETHANDLER_BUFSIZE,MSG_OOB);
				if (l_rc==SOCKET_ERROR) {
					switch(WSAGetLastError()) {
						case WSAENOTCONN:
						case WSAENOTSOCK:
						case WSAESHUTDOWN:
						case WSAECONNABORTED:
							/* socket has been closed */
							hoSocketKilled();
							MessageBeep(MB_ICONEXCLAMATION);
							ibInfoBox(g_aahWnd,"Connection closed by host.","Darn",IB_OK|IB_RUNHELP,NULL,HP_IB_SOCKET_ERROR_CLOSED);
							return;
						case WSAECONNRESET: /* ??? what's this? */
						default:
							break; /* maybe we can still write ? */
					} /* switch */
				} else if (!l_rc) {
					dbPrint("return value 0 on OOB data read.");
				} else {
//					dbPrint("HO: Received OOB Data:");
//					dbPrintNumHex(l_buf,l_rc);
					if ((l_rc==1)&&(((unsigned char*)(l_buf))[0]==0x0FF)) {
						dbPrint("OOB: Ctrl-C");
					} else {
						dbPrint("unrecognized OOB data received");
					}
				}
				break;
			case FD_ACCEPT:
				dbPrint("FD_ACCEPT");
				dbPrint("HO: Accept received on socket.");
				break;
			case FD_CONNECT:
				dbPrint("FD_CONNECT");
				dbPrint("HO: Connect received on socket.");
				break;
			case FD_CLOSE:
				hoSocketKilled();
				dbPrint("FD_CLOSE");
//				aaWrapup();
				break;
		}
	}
	return;
}

/* The following proc cleans everything up in the event of a messy socket closure
 * or an error */
void hoSocketKilled() {
//  dbPrint("Socket Killed");
	if (g_hoSock!=INVALID_SOCKET)
		closesocket(g_hoSock);
	if (g_hoLoginWnd)
		EndDialog(g_hoLoginWnd,0);
	g_hoSock=INVALID_SOCKET;
	g_hoConnectedToHost=FALSE;
	g_hoConnectionState=HO_CONNECT_STATE_OFFLINE;
	mpBufFlush(&g_mpPktBuf);
	mpBufFlush(&g_mpCmdBuf);
	mpSendBufferFlush();
  mpReqFlush(&g_mpRequestQ);
  awResetConnection();
  edResetConnection();
  mnMenuMainUpdate();
  return;
}

/* The following assumes we want to do a friendly disconnection from the host. This
 * proc should therefore close any host-dependent windows, warn about pending
 * transactions, etc. and try to log off. */
BOOL hoSocketFriendlyDisconnect() {
  /* check for changed data structures or other changes needing transmission */
  /* bring up dialog box and block until we're OK to log off, or until user
   * manually cancells and says "LOG OFF **NOW**"  */
//  g_hoConnectionState=HO_CONNECT_STATE_LOGOFF; /* This should be done in dialog box handler */
  return TRUE;
}

#pragma argsused
BOOL CALLBACK _export hoDialogLoginProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam)
	{
	switch (p_message)
		{
		case WM_INITDIALOG:
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);
			dlCentreDialogBox(p_hWnd);
      SetDlgItemText(p_hWnd,IDC_LOGINLOGINID,g_hoLoginUserName);
      SetDlgItemText(p_hWnd,IDC_LOGINPASSWORD,g_hoLoginPassword);
      if (g_hoLoginUserName[0]&&(!g_hoLoginPassword[0])) {
        SetFocus(GetDlgItem(p_hWnd,IDC_LOGINPASSWORD));
        return FALSE;
      }
      return TRUE;
		case WM_SYSCOMMAND:
			switch(p_wParam) /* Process Control Box / Max-Min function */
				{
				case SC_CLOSE:
          EndDialog(p_hWnd,0);
					return TRUE;
				default:
					break;
				}
			break;
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
        {
        case IDOK:
          GetDlgItemText(p_hWnd,IDC_LOGINLOGINID,g_hoLoginUserName,HO_STRLEN);
          GetDlgItemText(p_hWnd,IDC_LOGINPASSWORD,g_hoLoginPassword,HO_STRLEN);
          EndDialog(p_hWnd,1);
          return TRUE;
        case IDCANCEL:
          EndDialog(p_hWnd,0);
          return TRUE;
				case IDHELP:
          WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,HP_HO_DIALOG_LOGIN_HELP);
          return TRUE;
        default:
          break;
        }
    default:
      break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
	}



