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

#include "crimson2.h"
#include "log.h"
#include "mem.h"
#include "queue.h"
#include "str.h"
#include "extra.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "base.h"

#define BASELINK_ALLOC_SIZE 256

/* This whole link thing is screwy, if I had been thinking clearer at
   the time that I wrote it, I would have made it so that "ControlLinks"
   were simply a SOCK* field in the Base structure, rather than a special
   type of link... oh well I'm not about to change it now especially since
   it works (saves memory this way anyway... 4 bytes per BASE...)
 */

BYTE *linkTypeList[] = {
  "UNDEFINED",
  "CONTROL",
  "TELEPATHY-SND",
  "TELEPATHY-RCV",
  "HEAR-SND",
  "HEAR-RCV",
  "SEE-SND",
  "SEE-RCV",
  "C4-SND",
  "C4-RCV",
  ""
};

void BaseLinkAlloc(THING *sndThing, THING *rcvThing, WORD lType) {
  BASELINK *link; /* send */
  MEMALLOC(link, BASELINK, BASELINK_ALLOC_SIZE);
  link->lDetail.lThing = rcvThing;
  link->lType = lType;
  link->lNext = Base(sndThing)->bLink;
  Base(sndThing)->bLink = link;
}

void BaseLinkCreate(THING *sndThing, THING *rcvThing, WORD lType) {
  WORD      otherLType;

  if (!sndThing){
    Log(LOG_ERROR, "BaseLinkAlloc(base.c): ACK! NULL sndThing\n"); 
    return;
  }
  if (!rcvThing){
    Log(LOG_ERROR, "BaseLinkAlloc(base.c): ACK! NULL rcvThing\n"); 
    return;
  }
  if (lType%2){
    otherLType = lType-1;
  } else
    otherLType = lType+1;

  if (lType==BL_CONTROL){
    Log(LOG_ERROR, "BaseLinkAlloc(base.c): ACK! Control link attempted\n"); 
    return;
  }

  /* its a link guys... */
  BaseLinkAlloc(sndThing, rcvThing, lType);
  BaseLinkAlloc(rcvThing, sndThing, otherLType);
}

/* find the controlling socket for something, fill in what you know, it doesnt
   need all the parms */
BASELINK *BaseLinkFind(BASELINK *next, WORD lType, THING *thing) {
  BASELINK *i;

  for(i=next; i; i = i->lNext) {
    if (i->lType != BL_CONTROL) {
      if ( (lType == BL_UNDEF || i->lType == lType)
         &&(!thing || i->lDetail.lThing==thing))
        return i;
    }
  }

  return NULL;
}

/* will search and destroy both halves of a link */
void BaseLinkFree(THING *thing, BASELINK *link) {
  BASELINK *i;
  BASELINK *freeLink;
  THING    *otherThing;
  LWORD     otherLType;

  if (!thing){
    Log(LOG_ERROR, "BaseLinkFree(base.c): ACK! NULL thing\n"); 
    return;
  }
  if (!link){
    Log(LOG_ERROR, "BaseLinkFree(base.c): ACK! NULL link\n"); 
    return;
  }

  /* find other side and turf it - except for control links which have only one side */
  if (link->lType != BL_CONTROL) { /* normal link */
    otherThing = link->lDetail.lThing;
    if (link->lType%2){
      otherLType = link->lType-1;
    } else
      otherLType = link->lType+1;
    if (   (Base(otherThing)->bLink->lDetail.lThing == thing)
        && (Base(otherThing)->bLink->lType == otherLType) 
    ) {
      freeLink = Base(otherThing)->bLink;
      Base(otherThing)->bLink = Base(otherThing)->bLink->lNext;
      MEMFREE(freeLink, BASELINK);
    } else {  
      for(i=Base(link->lDetail.lThing)->bLink; i && i->lNext; i=i->lNext)
        if ( (i->lNext->lDetail.lThing == thing)
          && (i->lNext->lType == otherLType) )
          break;
      if (i->lNext) {
        freeLink = i->lNext;
        i->lNext = i->lNext->lNext;
        MEMFREE(freeLink, BASELINK);
      }
    }

  /* set ControlThing field to null now that control field is gone */
  } else {
    link->lDetail.lSock->sControlThing = NULL;
  }
  
  /* ok now do ourselves in... */
  if (Base(thing)->bLink == link) {
    freeLink = Base(thing)->bLink;
    Base(thing)->bLink = Base(thing)->bLink->lNext;
    MEMFREE(freeLink, BASELINK);    
  } else {
    for (i=Base(thing)->bLink; i && i->lNext != link; i=i->lNext);
    if (i) {
      freeLink = i->lNext;
      i->lNext = i->lNext->lNext;
      MEMFREE(freeLink, BASELINK);
    }
  }
      
}


/* free all the links that a base has in preparation to get rid of it */
void BaseFree(THING *thing) {
  STRFREE(Base(thing)->bKey);
  STRFREE(Base(thing)->bLDesc);
  while (Base(thing)->bLink)
    BaseLinkFree(thing, Base(thing)->bLink);
}

/* Set a control link from thing to sock */
void BaseControlAlloc(THING *thing, SOCK *sock) {
  BASELINK *link; 

  if (!thing){
    Log(LOG_ERROR, "BaseLinkAlloc(base.c): ACK! NULL thing\n"); 
    return;
  }
  if (!sock){
    Log(LOG_ERROR, "BaseLinkAlloc(base.c): ACK! NULL sock\n"); 
    return;
  }

  /* special case of a control link which points to a socket */
  MEMALLOC(link, BASELINK, BASELINK_ALLOC_SIZE);
  link->lDetail.lSock = sock;
  link->lType = BL_CONTROL;
  link->lNext = Base(thing)->bLink;
  Base(thing)->bLink = link;

  if (!sock->sHomeThing) {
    sock->sHomeThing = thing;
  }  
  sock->sControlThing = thing;
}


/* find the controlling socket for something */
SOCK *BaseControlFind(THING *thing) {
  BASELINK *i;

  if (!thing) return NULL;

  for(i=Base(thing)->bLink; i; i = i->lNext) {
    if (i->lType==BL_CONTROL)
      return i->lDetail.lSock;
  }
  return NULL;
}

/* find the controlling socket for something */
void BaseControlFree(THING *thing, SOCK *sock) {
  BASELINK *i;
  BASELINK *next;

  if (!thing) return;

  for(i=Base(thing)->bLink; i; i = next) {
    next = i->lNext;
    if (i->lType==BL_CONTROL && i->lDetail.lSock == sock) {
      BaseLinkFree(thing, i);
      sock->sControlThing = NULL;
    }
  }
}
