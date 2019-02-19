/* SMTP relayed email service */

#include <stdio.h>
#include <stdlib.h>

char *mailServer = "mail";

#define MAIL_SMTP_PORT 25

int MailConnect() {
  char            hostName[128];
  SOCKET          mailSock;
  struct linger   theLinger;
  int             err;
  unsigned long  *addr;
  SOCKADDR_IN     saHost;
  struct hostent *hostEnt;

  /* set linger to 0 */
  theLinger.l_onoff = 1;
  theLinger.l_linger = 0;
  err = setsockopt(mailSock, SOL_SOCKET, SO_LINGER, (char far *)&theLinger, sizeof(theLinger));

  /* determine host addr */
  sprintf(hostName, "mail");
  hostEnt = gethostbyname(hostName);
  addr = (unsigned long *) hostEnt->h_addr_list;

  /* form host address */
  memset(&saHost, 0, sizeof(saHost));
  saHost.sin_family = AF_INET;
  saHost.sin_addr.s_addr = *addr;
  saHost.sin_port = htons( MAIL_SMTP_PORT ); /* xlnet port */
    
  /*
   * Connect to the host (CO protocol) connect() will block 
   * since mailSock is a SOCK_STREAM type socket
   */
  err = connect(mailSock, (struct sockaddr *)&saHost, sizeof(saHost));

}

void MailSend(int mailSock, char *str) {
  send(mailSock, str, strlen(str), 0);
}

/* Convert \n to \r\n and . at the front of lines to .. */
void MailSendBody(int mailSock, char *body) {
  if (!body) return;
  while (*body) {
    if (*body=='\n')
      send(mailSock, "\r", 1, 0);
    send(mailSock, *body, 1, 0);
    if (body[1]=='.')
      send(mailSock, ".", 1, 0);
    body++;
  }
}

void MailGetReply(int mailSock, char *str) {
  /* if no data do it again until we get a single line */

  recv(mailSock, str, 256);
}

void MailLetter(char *fromName, char *fromAddr, char *toName, char *toAddr, char *subject, char *body) {
  int   mailSocket;
  char  buf[256];
  char  reply[256];

  /* Connect to Mail Server */
  mailSocket = MailConnect();

  /* Send HELO<CRLF> */
  MailSend(mailSock, "HELO\r\n");
  /* Wait for reply -> 250 OK */
  MailGetReply(mailSock, reply);

  /* VRFY toAddr<CRLF> */ 
  MailSend(mailSock, "VRFY ");
  MailSend(mailSock, toAddr);
  MailSend(mailSock, "\r\n");
  /* Wait for reply -> 250 OK or 5XX Error */
  MailGetReply(mailSock, reply);
  if (strncmp(reply, "250", 3)) {
    printf("Verify failed/Illegal Destination address\n");
    return;
  }
  
  /* MAIL FROM: fromAddr<CRLF> */
  MailSend(mailSock, "MAIL FROM: ");
  MailSend(mailSock, fromAddr);
  MailSend(mailSock, "\r\n");
  /* Wait for reply -> 250 OK */
  MailGetReply(mailSock, reply);
  if (strncmp(reply, "250", 3)) {
    printf("Illegal MAIL FROM: address\n");
    return;
  }
  
  
  /* RCPT TO: toAddr<CRLF> */
  MailSend(mailSock, "RCPT TO: ");
  MailSend(mailSock, toAddr);
  MailSend(mailSock, "\r\n");
  /* Wait for reply -> 250 OK or 5XX Error */
  MailGetReply(mailSock, reply);
  if (strncmp(reply, "250", 3)) {
    printf("Illegal RCPT TO: address\n");
    return;
  }
  
  
  /* DATA<CRLF> */
  MailSend(mailSock, "DATA\r\n");
  /* Wait for reply -> 354 Ready to Send */
  MailGetReply(mailSock, reply);
  
  /* Send FROM: fromName<CRLF> */
  MailSend(mailSock, "FROM:");
  MailSend(mailSock, fromName);
  MailSend(mailSock, "\r\n");
  /* Send TO: toName<CRLF> */
  MailSend(mailSock, "TO:");
  MailSend(mailSock, toName);
  MailSend(mailSock, "\r\n");
  /* Send SUBJECT: subject<CRLF> */
  MailSend(mailSock, "SUBJECT:");
  MailSend(mailSock, subject);
  MailSend(mailSock, "\r\n");
  /* Send body */
  MailSendBody(mailSock, body);
  /* Send <CRLF>.<CRLF> */
  MailSend(mailSock, "\r\n.\r\n");
  
  /* Wait for reply -> 250 OK */
  MailGetReply(mailSock, reply);
  
  /* Kill Connection */
  MailDisconnect(mailSock);
  
}

void main(void) {
  char *fromName = "Cryogen at CrimsonMud/2799"
  char *fromAddr = "rhaksi@vcn.bc.ca"
  char *toName   = "Ansi at CrimsonMud/2799"
  char *toAddr   = "cryogen@unix.infoserve.net"
  char *subject  = "Test Subject";
  char *body     = "Test Message";

  /* Should fork() off a separate thread at some point */
  MailLetter(fromName, fromAddr, toName, toAddr, subject, body);
}