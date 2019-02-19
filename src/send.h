/* send.h
 * this module contains routines related to sending messages to players
 */

extern void SendEchoOff(SOCK *sock);
extern void SendEchoOn(SOCK *sock);
extern void SendSmartColor(BYTE *str, SOCK *sock);
extern void SendPrompt(SOCK *sock);
extern void SendMainMenu(SOCK *sock);
extern void SendChooseAnsi(SOCK *sock);
extern void SendChooseExpert(SOCK *sock);
extern void SendChooseSex(SOCK *sock);
extern void SendChooseRace(SOCK *sock);
extern void SendChooseClass(SOCK *sock);
extern void SendShowCombo(SOCK *sock);
extern void SendRollAbilities(SOCK *sock);
extern BYTE SendThing(BYTE *str, THING *thing);
extern void SendChannel(BYTE *str, THING *thing, FLAG chanFlag);
extern void SendHint(BYTE *str, THING *thing);
extern void SendAll(BYTE *str);
extern void SendArray(ULWORD tList, LWORD tListSize, LWORD column, THING *thing);

#define SEND_ROOM       1<<0   /* everyone exect SRC and DST */
#define SEND_SRC        1<<1
#define SEND_DST        1<<2
#define SEND_VISIBLE    1<<3   /* action is a VISIBLE action hide if victim cant see SRC */
#define SEND_AUDIBLE    1<<4   /* action is an AUDIBLE action hide if the victim cant hear SRC */
#define SEND_CAPFIRST   1<<5   /* capitalize first letter, usefull for actions beginning with $n */
#define SEND_GROUP      1<<6   /* Send to the group that the src thing belongs to, but not SRC*/

/* system reserved */
#define SEND_PARTIALSRC 1<<7   /* can only partially see src */
#define SEND_PARTIALDST 1<<8   /* can only partially see src */

#define SEND_SELF       1<<9   /* send actions that we did to ourselves (normally hidden) */
#define SEND_HISTORY    1<<10   /* Add to the send history of the dest sockets */

extern BYTE *sendFlagList[];

extern void SendAction(BYTE *str, THING *srcThing, THING *dstThing, FLAG actionFlag);

/* further notes on SEND_VISIBLE/AUDIBLE
 * -If the action is strictly visible, then 
 *   + if they cant see the src, the victim will not see it at all
 *   + if they can partially see the src, the victim will get someone/something
 * -If the action is strictly audible, then the victim will not get the action at
 * all if they cant hear
 * -If the action is both audible and visible then the victim will get:
 *   + nothing if s/he is both deaf and and cant see src
 *   + Names replaced with Someone, if can hear but cant see.
 *   + business as usual if they cant hear, but can see.
 */

/* this should not be used to send stuff to sockets that may be at a prompt */
#define SEND(str, sock) QAppend(sock->sOut, str, Q_DONTCHECKSTR)
#define SENDARRAY(list, column, thing) SendArray((ULWORD)list, sizeof(*list), column, thing)
