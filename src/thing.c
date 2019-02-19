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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "str.h"
#include "ini.h"
#include "queue.h"
#include "send.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "extra.h"
#include "property.h"
#include "code.h"
#include "file.h"
#include "thing.h"
#include "exit.h"
#include "world.h"
#include "area.h"
#include "reset.h"
#include "base.h"
#include "object.h"
#include "affect.h"
#include "char.h"
#include "group.h"
#include "fight.h"
#include "player.h"
#include "skill.h"
#include "parse.h"
#include "mobile.h"
#include "cmd_inv.h"
#include "cmd_cbt.h"
#include "cmd_misc.h"

BYTE *tFlagList[] = {
  "COMPILE", /* something uncompiled this, recompile on when needed */
  "TRAVERSED",
  "IDLECODE",
  "FIGHTINGCODE",
  "COMMANDCODE",
  "ENTRYCODE",
  "DEATHCODE",
  "EXITCODE",
  "FLEECODE",
  "RESETCODE",
  "DAMAGECODE",
  "USECODE",
  "AFTERFIGHTINGCODE",
  "AFTERCOMMANDCODE",
  "AFTERENTRYCODE",
  "AFTERDEATHCODE",
  "AFTEREXITCODE",
  "AFTERFLEECODE",
  "AFTERRESETCODE",
  "AFTERDAMAGECODE",
  "AFTERUSECODE",
  "CODETHING",
  "AFTERATTACKCODE",
  ""
};

INDEX  eventThingIndex;
INDEX  trackThingIndex;
INDEX  trackExitIndex;

void ThingInit(void) {
  IndexInit(&eventThingIndex, 256,  "eventThingIndex", IF_ALLOW_DUPLICATE);
  IndexInit(&trackThingIndex, 2048, "trackThingIndex", 0);
  IndexInit(&trackExitIndex,  2048, "trackExitIndex",  0);
}

/* remove a Thing & its contents from whatever it is inside of */
/* probably only going to be called by ThingTo & ThingExtract */
void ThingFrom(THING *thing) {
  THING *insideThing; /* what we're inside */
  THING *i;

  /* Check what we are */
  switch (thing->tType) {
  case TTYPE_WLD:
  case TTYPE_UNDEF:
    Log(LOG_ERROR, "ThingFrom(thing.c): Undefined ThingType.\n");
    return;

  case TTYPE_OBJ:
    insideThing = Base(thing)->bInside;
    InvUnEquip(insideThing, thing, IUE_NONBLOCKABLE);
    break;

  default:
    insideThing = Base(thing)->bInside;
    break;
  }

  if (!insideThing) return;

  /* check what we're inside */
  switch (insideThing->tType) {
  case TTYPE_WLD:
    /* Update light */
    if (thing->tType >= TTYPE_CHARACTER) {
      THING *i;
      for (i=thing->tContain; i; i=i->tNext)
        if (i->tType==TTYPE_OBJ && Obj(i)->oTemplate->oType==OTYPE_LIGHT && Obj(i)->oEquip) {
          LWORD intensity;
          intensity = OBJECTGETFIELD(i, OF_LIGHT_INTENSITY);
          Wld(insideThing)->wLight -= intensity;
        }
    }
    break;

  case TTYPE_PLR:
  case TTYPE_MOB:
    Base(insideThing)->bConWeight -= Base(thing)->bWeight;
    if (Character(insideThing)->cWeapon == thing)
      Character(insideThing)->cWeapon = NULL;
    break;

  case TTYPE_OBJ:
    if (Obj(insideThing)->oTemplate != phantomPocketTemplate)
      Base(insideThing)->bConWeight -= Base(thing)->bWeight;
    break;
  }

  if (insideThing->tContain == thing) {
    insideThing->tContain = insideThing->tContain->tNext;
  } else {
    for (i = insideThing->tContain; i->tNext != NULL && i->tNext != thing; i=i->tNext);
    if (i->tNext == thing) {
      i->tNext = thing->tNext;
    } 
  }
  Base(thing)->bInside = NULL;
  thing->tNext = NULL;
}

/* move a thing so that is Inside another thing */
/* The mechanics of this are thus:
  Insert after any objects of OTYPE_RESETCMD, (they should allways be first
  in the list of things contained)
  after insertion, if we were inserting TTYPE_MOB or TTYPE_OBJ, move all
  other OBJ/MOB's of the same virtual number so that immediately follow the
  thing just inserted
*/
void ThingTo(THING *thing, THING *destThing) {
  /* EXTRA *property; */
  THING *i;
  THING *next;
  THING *firstMatch;
  THING *afterMatch;

  /* make sure thing is valid */
  if (!thing || !destThing) {
    Log(LOG_ERROR, "ThingTo(thing.c): NullPointer.\n");
    return;
  }
  /* make sure thing is a valid type */
  if (thing->tType==TTYPE_WLD || thing->tType==TTYPE_UNDEF) {
    Log(LOG_ERROR, "ThingTo(thing.c): Illegal ThingType.\n");
    return;
  }

  /* Clear important script flags */
  BITCLR(thing->tFlag, TF_CODETHING);

  /* Guard against NO_ENTER bit */
  if (destThing->tType==TTYPE_WLD 
   && thing->tType==TTYPE_PLR
   && BIT(areaList[Wld(destThing)->wArea].aResetFlag, RF_NOENTER)
   && !ParseCommandCheck(PARSE_COMMAND_WGOTO, BaseControlFind(thing), "")) {
    SendThing("^r>>>>> ^wI'm sorry that area isnt open to you yet! ^r<<<<<\n", thing);
    return;
  }

  /* Guard against NO_TAKE bit */
  if (destThing->tType==TTYPE_WLD
   && (thing->tType==TTYPE_PLR || thing->tType==TTYPE_MOB)
   && Base(thing)->bInside
   && Base(thing)->bInside->tType==TTYPE_WLD
   && BIT(areaList[Wld(Base(thing)->bInside)->wArea].aResetFlag, RF_NOTAKE)
   && Wld(Base(thing)->bInside)->wArea != Wld(destThing)->wArea) {
    SendThing("^r>>>>> ^wRemoving objects that are still being tested! ^r<<<<<\n", thing);
    for (i = thing->tContain; i; i=next) {
      next = i->tNext;
      if (i->tType == TTYPE_OBJ
       && AreaOf(Obj(i)->oTemplate->oVirtual) == Wld(Base(thing)->bInside)->wArea)
        THINGFREE(i);
    }
  }

  /* if thing is in a structure remove it */
  if (Base(thing)->bInside) {
    ThingFrom(thing);
  }
  
  /* recompile wld as needed */
  if (destThing && destThing->tType==TTYPE_WLD) {
    PROPERTY *p;

    if (BIT(destThing->tFlag, TF_COMPILE)) {
      BITCLR(destThing->tFlag, TF_COMPILE);
      for (p=destThing->tProperty; p; p=p->pNext) {
        if (!CodeCompileProperty(p, NULL)) {
          CodeSetFlag(destThing, p);
        }
      }
    }
  }

  /* perform the insert right after any resetcmd's */
  Base(thing)->bInside = destThing;
  i = destThing->tContain;
  if (!i || i->tType!=TTYPE_OBJ || Obj(i)->oTemplate->oType!=OTYPE_RESETCMD) {
    thing->tNext = destThing->tContain;
    destThing->tContain = thing;
  } else {
      /* insert after all other zone commands */
      while((i->tNext)
          &&(i->tNext->tType==TTYPE_OBJ)
          &&(Obj(i->tNext)->oTemplate->oType==OTYPE_RESETCMD)) 
        i=i->tNext;
      thing->tNext = i->tNext;
      i->tNext = thing;
  }

  /* guard against privledged settings that a player may no longer be allowed */
  /* note we have to do this after they've moved. Why? Because
   * this will lead to a ParseCommandCheck which operates on the
   * current room of 'thing' */
  if (destThing->tType==TTYPE_WLD
   && thing->tType==TTYPE_PLR
   && Base(thing)->bInside
   && Base(thing)->bInside->tType==TTYPE_WLD) {
   SetFlagCheck(thing);
  }

  /* group like things together in chain */
  switch (thing->tType) {
  case TTYPE_OBJ:
    /* move all other objs of same virtual # to right after this one */
    /* find first similar object */
    for(i=thing; 
        (i->tNext)
        &&((i->tNext->tType!=TTYPE_OBJ)
           ||(Obj(i->tNext)->oTemplate->oVirtual!=Obj(thing)->oTemplate->oVirtual)); 
        i=i->tNext);
    if (i!=thing && i->tNext) { /* if there are other like objects */
      /* find last similar object */
      for(next=i; 
          (next->tNext)
          &&(next->tNext->tType==TTYPE_OBJ)
          &&(Obj(next->tNext)->oTemplate->oVirtual==Obj(thing)->oTemplate->oVirtual); 
          next=next->tNext);
      /* splice chain of things */
      afterMatch = next->tNext;
      firstMatch = i->tNext;
      
      next->tNext = thing->tNext;
      thing->tNext = firstMatch;
      i->tNext = afterMatch;
    }
    /* Keep a vehicles OUT exit up to date */
    if (Obj(thing)->oTemplate->oType == OTYPE_VEHICLE) {
      THING *wld;
      wld = WorldOf(OBJECTGETFIELD(thing, OF_VEHICLE_WVIRTUAL));
      if (wld) {
        EXIT *exit;
        exit = ExitDir(Wld(wld)->wExit, EDIR_OUT);
        if (destThing->tType==TTYPE_WLD) {
          /* Make sure an OUT exit points to the room the
           * vehicle is inside ie so people can get out */
          if (exit) {
            /* free up old stuff */
            STRFREE(exit->eKey);
            /* point to new OUT exit */
            exit->eWorld = destThing;
            exit->eKey = StrAlloc(wld->tSDesc);
          } else {
            /* Create an OUT exit */
            Wld(wld)->wExit = ExitAlloc(Wld(wld)->wExit, 
                                        EDIR_OUT, 
                                        StrAlloc(wld->tSDesc), 
                                        STRCREATE(""), 
                                        0, 
                                        -1, 
                                        destThing);
          }
        } else {
          /* Remove OUT exit if we're inside a MOB or something */
          if (exit) EXITFREE(Wld(wld)->wExit, exit);
        }
      }
    }
    break;

  case TTYPE_MOB:
    /* move like mobs to right after this one */
    break;
  }

  /* If we are moving into a world with uncompiled code, compile on the fly */ 
  switch (destThing->tType) {
  case TTYPE_WLD:
    /* Update light */
    if (thing->tType >= TTYPE_CHARACTER) {
      THING *i;
      for (i=thing->tContain; i; i=i->tNext)
        if (i->tType==TTYPE_OBJ && Obj(i)->oTemplate->oType==OTYPE_LIGHT && Obj(i)->oEquip) {
          LWORD intensity;
          intensity = OBJECTGETFIELD(i, OF_LIGHT_INTENSITY);
          Wld(destThing)->wLight += intensity;
        }
    }
    break;

  case TTYPE_OBJ:
    if (Obj(destThing)->oTemplate != phantomPocketTemplate)
      Base(destThing)->bConWeight += Base(thing)->bWeight;
    break;

  case TTYPE_PLR:
  case TTYPE_MOB:
    Base(destThing)->bConWeight += Base(thing)->bWeight;
    break;
  }
}

/* remove a thing but leave its contents */
void ThingExtract(THING *thing) {

  switch (thing->tType) {
  case TTYPE_WLD:
  case TTYPE_UNDEF:
    Log(LOG_ERROR, "ThingExtract(thing.c): Undefined ThingType.\n");
    return;

  case TTYPE_OBJ:
    InvUnEquip(Base(thing)->bInside, thing, IUE_NONBLOCKABLE);
    break;

  default:
    break;
  }
  if (Base(thing)->bInside) {
    while (thing->tContain) { /* move contents one level up */
      ThingTo(thing->tContain, Base(thing)->bInside);
    }
    ThingFrom(thing);
  }
}


/* redirect the Free call to lower-level calls as appropriate */
THING *ThingFree(THING *thing) {
  if (!thing) return NULL;

  while (IndexFind(&eventThingIndex, thing, NULL)) 
    IndexDelete(&eventThingIndex, thing, NULL);

  /* turf all included things first */
  while (thing->tContain) ThingFree(thing->tContain);

  STRFREE(thing->tSDesc);
  STRFREE(thing->tDesc);

  while(thing->tExtra) 
    EXTRAFREE(thing->tExtra, thing->tExtra);
  while(thing->tProperty) 
    PROPERTYFREE(thing->tProperty, thing->tProperty);

  switch (thing->tType) {
  case TTYPE_UNDEF:
    Log(LOG_ERROR, "ThingFree(thing.c): Undefined ThingType.\n");
    return NULL;

  case TTYPE_WLD:
    break;

  case TTYPE_PLR:
    FightStop(thing); /* Stop fighting */
    GroupRemoveFollow(thing);  /* stop following */
    GroupRemoveAllFollow(thing); /* stop others from following us */
    ThingFrom(thing);   /* remove the thing from game */
    while(Character(thing)->cAffect) AffectFree(thing, Character(thing)->cAffect);
    BaseFree(thing);     /* free up any links to this object */
    PlayerFree(thing);   /* free its memory */
    break;

  case TTYPE_OBJ:
    ThingFrom(thing);   /* remove the thing from game */
    BaseFree(thing);     /* free up any links to this object */
    ObjectFree(thing);   /* free its memory */
    break;

  case TTYPE_MOB:
    FightStop(thing); /* Stop fighting */
    GroupRemoveFollow(thing); /* Stop anyone from following us */
    GroupRemoveAllFollow(thing); /* stop others from following us */
    ThingFrom(thing);   /* remove the thing from game */
    while(Character(thing)->cAffect) AffectFree(thing, Character(thing)->cAffect);
    BaseFree(thing);     /* free up any links to this object */
    MobileFree(thing);   /* free its memory */
    break;

  default:
    Log(LOG_ERROR, "ThingFree(thing.c): Unknown ThingType.\n");
    break;
  }

  return NULL;
}

/* NOTE since as we find things and hand them back they may be removed the
    static variables should point to the thing prior to the one being returned
    as that should not be modified between calls to this routine, the offset
    parm allows you to pass say 2 to find the 2nd match rather than the first
    (which in turn will allow easier implementaion of the 1.key 2.key business

 */
THING *ThingFind(BYTE *key, LWORD virtual, THING *search, FLAG findFlag, LWORD *offset) {
  static LWORD  iPlr;
  static LWORD  iMob;
  static LWORD  iObj;
  static LWORD  iWld;
  static LWORD  iArea;
  static THING *iThing;
  static THING *tPrev;     /* pointer to the previous element in list */
  static THING *tLast;     /* last match */
  BYTE          all;
  THING        *tCompare;


  if (key)
    all = !STRICMP(key, "all");
  else
    all = 0;
  if (!BIT(findFlag, TF_CONTINUE)) { /* offset of -1 indicates find all key matches */
    if ( (!key || !*key) && (virtual<0) ) return FALSE; /* Null key wont match anything */
    iPlr    = 0;
    iMob    = 0;
    iObj    = 0;
    iWld    = 0;
    iArea   = 0;
    tPrev   = NULL;
    tLast   = NULL;
  }
  if (!tPrev) { /* if we have returned the first member of the list we may need to refresh if it was moved */
    if (search)
      iThing  = search->tContain;
    else
      iThing  = NULL;
  }

  /* Look for something in search */
  while (iThing) {
    if (tPrev)
      tCompare = iThing->tNext;
    else
      tCompare = iThing;
    if (!tCompare)
      break;

    if((( (tCompare->tType==TTYPE_PLR) && BIT(findFlag, TF_PLR) )
     || ( (tCompare->tType==TTYPE_OBJ) && BIT(findFlag, TF_OBJ) )
     || ( (tCompare->tType==TTYPE_OBJ) && BIT(findFlag, TF_OBJINV)   &&!Obj(tCompare)->oEquip)
     || ( (tCompare->tType==TTYPE_OBJ) && BIT(findFlag, TF_OBJEQUIP) && Obj(tCompare)->oEquip)
     || ( (tCompare->tType==TTYPE_MOB) && BIT(findFlag, TF_MOB) ))
    && (tCompare != tLast)
    && (  (all || StrIsKey(key, Base(tCompare)->bKey)
        ||((tCompare->tType==TTYPE_OBJ) && (Obj(tCompare)->oTemplate->oVirtual==virtual))
        ||((tCompare->tType==TTYPE_MOB) && (Mob(tCompare)->mTemplate->mVirtual==virtual))
    ))) {
       if (offset) {
         if (*offset>0) (*offset)--;
         if ((*offset)<=0) {
           tLast = tCompare;
           return tCompare;
         }
       } else {
         tLast = tCompare;
         return tCompare;
       }
    }
    /* didnt match for some reason, who's up next... */
    if (tPrev) {
      tPrev = iThing;
      iThing = iThing->tNext;
    } else {
      tPrev = iThing;
    }
  }

  if (BIT(findFlag, TF_PLR_ANYWHERE)) {
    if (key) {
      for(;iPlr<playerIndex.iNum;iPlr++) {
        if (   (tPrev!=playerIndex.iThing[iPlr])
      && (Base(playerIndex.iThing[iPlr])->bInside!=search)
      && (Base(playerIndex.iThing[iPlr])->bInside)
      && (all || StrIsKey(key, Base(playerIndex.iThing[iPlr])->bKey))) {
          tPrev = playerIndex.iThing[iPlr];
          if (offset) {
            if (*offset>0) (*offset)--;
            if ((*offset)<=0)
              return tPrev;
          } else
            return tPrev;
        }
      }
    }
  }

  if (BIT(findFlag, TF_OBJ_ANYWHERE)) {
    for(;iObj<objectIndex.iNum;iObj++) {
      if ((tPrev!=objectIndex.iThing[iObj])
      &&  (Base(objectIndex.iThing[iObj])->bInside!=search)) {
        if ( ((key) && (all || StrIsKey(key, Base(objectIndex.iThing[iObj])->bKey)))
        ||   (Obj(objectIndex.iThing[iObj])->oTemplate->oVirtual==virtual) ) {
          tPrev = objectIndex.iThing[iObj];
          if (offset) {
            if (*offset>0) (*offset)--;
            if ((*offset)<=0)
              return tPrev;
          } else 
            return tPrev;
        }
      }
    }
  }

  if (BIT(findFlag, TF_MOB_ANYWHERE)) {
    for(;iMob<mobileIndex.iNum;iMob++) {
      if ((tPrev!=mobileIndex.iThing[iMob])
      &&  (Base(mobileIndex.iThing[iMob])->bInside!=search)) {
        if ( ((key) && (all || StrIsKey(key, Base(mobileIndex.iThing[iMob])->bKey)))
        ||   (Mob(mobileIndex.iThing[iMob])->mTemplate->mVirtual==virtual)) {
          tPrev = mobileIndex.iThing[iMob];
          if (offset) {
            if (*offset>0) (*offset)--;
            if ((*offset)<=0)
              return tPrev;
          } else
            return tPrev;
        }
      }
    }
  }

  if (BIT(findFlag, TF_PLR_WLD)) {
    if (key) {
      for(;iPlr<playerIndex.iNum;iPlr++) {
        if (   (tPrev!=playerIndex.iThing[iPlr])
         && (Base(playerIndex.iThing[iPlr])->bInside!=search)
         && (Base(playerIndex.iThing[iPlr])->bInside)
         && (Base(playerIndex.iThing[iPlr])->bInside->tType == TTYPE_WLD)
         && (all || StrIsKey(key, Base(playerIndex.iThing[iPlr])->bKey))
        ) {
          tPrev = playerIndex.iThing[iPlr];
          if (offset) {
            if (*offset>0) (*offset)--;
            if ((*offset)<=0)
              return tPrev;
          } else
            return tPrev;
        }
      }
    }
  }

  if (BIT(findFlag, TF_OBJ_WLD)) {
    for(;iObj<objectIndex.iNum;iObj++) {
      if (   (tPrev!=objectIndex.iThing[iObj])
    && (Base(objectIndex.iThing[iObj])->bInside)
    && (Base(objectIndex.iThing[iObj])->bInside->tType == TTYPE_WLD)
    && (Base(objectIndex.iThing[iObj])->bInside!=search)
      ) {
        if ( ((key) && (all || StrIsKey(key, Base(objectIndex.iThing[iObj])->bKey)))
        ||   (Obj(objectIndex.iThing[iObj])->oTemplate->oVirtual==virtual)
        ) {
          tPrev = objectIndex.iThing[iObj];
          if (offset) {
            if (*offset>0) (*offset)--;
            if ((*offset)<=0)
              return tPrev;
          } else
            return tPrev;
        }
      }
    }
  }

  if (BIT(findFlag, TF_MOB_WLD)) {
    for(;iMob<mobileIndex.iNum;iMob++) {
      if ((tPrev!=mobileIndex.iThing[iMob])
      && (Base(mobileIndex.iThing[iMob])->bInside)
      && (Base(mobileIndex.iThing[iMob])->bInside->tType == TTYPE_WLD)
      && (Base(mobileIndex.iThing[iMob])->bInside!=search)) {
        if ( ((key) && (all || StrIsKey(key, Base(mobileIndex.iThing[iMob])->bKey)))
        ||   (Mob(mobileIndex.iThing[iMob])->mTemplate->mVirtual==virtual)) {
          tPrev = mobileIndex.iThing[iMob];
          if (offset) {
            if (*offset>0) (*offset)--;
            if ((*offset)<=0)
              return tPrev;
            } else 
              return tPrev;
        }
      }
    }
  }

  if (BIT(findFlag, TF_WLD)) {
    for(;iArea<areaListMax;iArea++){
      for(;iWld<areaList[iArea].aWldIndex.iNum;iWld++) {
        if ((tPrev!=areaList[iArea].aWldIndex.iThing[iWld])
        && (areaList[iArea].aWldIndex.iThing[iWld]!=search)) {
          if ( ((key) && (all || StrIsKey(key, areaList[iArea].aWldIndex.iThing[iWld]->tSDesc))) 
          ||   (Wld(areaList[iArea].aWldIndex.iThing[iWld])->wVirtual==virtual)) {
            tPrev = areaList[iArea].aWldIndex.iThing[iWld];
            if (offset) {
              if (*offset>0) (*offset)--;
              if ((*offset)<=0)
                return tPrev;
            } else
              return tPrev;
          }
        }
      }
    }
  }

  return NULL;
}

/* use an index (or two) to achieve a breadth first search */
/* Note: in this particular case maxHop isnt the number of rooms away
   it will search rather it is the absolute number of room entries to
   search (initial optimization allows for a index depth of 512 rooms,
   dont worry you can choose a higher number than this it will just
   realloc more memory on the fly)

   unlike the recursive version you can just call this directly as it
   will return NULL for a match in the first room
*/
EXIT *ThingTrack(THING *thing, THING *track, LWORD maxHop) {
  LWORD         i=0;
  THING        *t;
  EXIT         *e;

  if (!thing
      || !track
      || BIT(thing->tFlag, TF_TRAVERSED)
      || thing->tType != TTYPE_WLD)
    return NULL;

  IndexClear(&trackThingIndex);
  IndexClear(&trackExitIndex);
  IndexAppend(&trackThingIndex, thing);
  IndexAppend(&trackExitIndex, NULL);

  for(i=0; i<maxHop && i<trackThingIndex.iNum; i++) {
    /* look for a match in this room */
    for (t=trackThingIndex.iThing[i]->tContain; t; t=t->tNext) {
      if (t==track) { trackExitIndex.iThing[0] = trackExitIndex.iThing[i]; break; }
    }
    BITSET(trackThingIndex.iThing[i]->tFlag, TF_TRAVERSED);
    /* no match queue up the exits */
    for (e=Wld(trackThingIndex.iThing[i])->wExit; e; e=e->eNext) {
      if (e->eWorld 
      && trackThingIndex.iNum < maxHop
      && !BIT(e->eWorld->tFlag, TF_TRAVERSED)) {
        IndexAppend(&trackThingIndex, e->eWorld);
        if (i==0) /* if this is the first room we are checking */
          IndexAppend(&trackExitIndex, e); /* store the first step */
        else
          IndexAppend(&trackExitIndex, trackExitIndex.iThing[i]); /* carry initial exit down */
      }
    }
  }
  /* Clear all the Traversed Flags */
  while (i>=trackThingIndex.iNum) i--;
  for(; i>=0; i--) {
    BITCLR(trackThingIndex.iThing[i]->tFlag, TF_TRAVERSED);
  }

  return ((EXIT*)trackExitIndex.iThing[0]);
}

EXIT *ThingTrackStr(THING *thing, BYTE *key, LWORD maxHop) {
  LWORD         i=0;
  THING        *t;
  EXIT         *e;

  if (!thing
      || !key
      || !*key
      || BIT(thing->tFlag, TF_TRAVERSED)
      || thing->tType != TTYPE_WLD)
    return NULL;

  IndexClear(&trackThingIndex);
  IndexClear(&trackExitIndex);
  IndexAppend(&trackThingIndex, thing);
  IndexAppend(&trackExitIndex, NULL);

  for(i=0; i<maxHop && i<trackThingIndex.iNum; i++) {
    /* look for a match in this room */
    for (t=trackThingIndex.iThing[i]->tContain; t; t=t->tNext) {
      if (StrIsKey(key, Base(t)->bKey)) { trackExitIndex.iThing[0] = trackExitIndex.iThing[i]; break; }
    }
    BITSET(trackThingIndex.iThing[i]->tFlag, TF_TRAVERSED);
    /* no match queue up the exits */
    for (e=Wld(trackThingIndex.iThing[i])->wExit; e; e=e->eNext) {
      if (e->eWorld 
      && trackThingIndex.iNum < maxHop
      && !BIT(e->eWorld->tFlag, TF_TRAVERSED)) {
        IndexAppend(&trackThingIndex, e->eWorld);
        if (i==0) /* if this is the first room we are checking */
          IndexAppend(&trackExitIndex, e); /* store the first step */
        else
          IndexAppend(&trackExitIndex, trackExitIndex.iThing[i]); /* carry initial exit down */
      }
    }
  }
  /* Clear all the Traversed Flags */
  while (i>=trackThingIndex.iNum) i--;
  for(; i>=0; i--) {
    BITCLR(trackThingIndex.iThing[i]->tFlag, TF_TRAVERSED);
  }

  return ((EXIT*)trackExitIndex.iThing[0]);
}

/* return if they can see it */
LWORD ThingCanSee(THING *thing, THING *show) {
  LWORD light;
  LWORD hide;
  LWORD perception;
  SOCK *sock;
  LWORD canSee = TCS_SEENORMAL;

  if (!thing || thing->tType < TTYPE_CHARACTER) return TCS_SEENORMAL;
  sock = BaseControlFind(thing);

  /* Cant see while asleep */
  if (Character(thing)->cPos <= POS_SLEEPING)
    return TCS_CANTSEE;

  /* Is it dark out */
  if (Base(thing)->bInside
   && Base(thing)->bInside->tType == TTYPE_WLD){
    if (!BIT(Wld(Base(thing)->bInside)->wFlag, WF_DARK))
      light = PropertyGetLWord(Base(thing)->bInside, "%Light", 100);
    else
      light = PropertyGetLWord(Base(thing)->bInside, "%Light", 0);
    light += Wld(Base(thing)->bInside)->wLight;
    if (light <= 75 && !BIT(Character(thing)->cAffectFlag, AF_DARKVISION)) {
      if (!ParseCommandCheck(PARSE_COMMAND_WCREATE, sock, "")) {
        canSee = TCS_SEEPARTIAL; /* dimly see */
        if (light <= 25) return TCS_CANTSEE; /* too dark to see */
      }
    }
  }
 

  /* Check out the show object */
  if (!show) return canSee;
  if (show->tType >= TTYPE_CHARACTER) {
    /* Check for hiding People */
    if (BIT(Character(show)->cAffectFlag, AF_HIDDEN)
     && !ParseCommandCheck(PARSE_COMMAND_MSTAT, sock, "") 
     && !BIT(Character(thing)->cAffectFlag, AF_SENSELIFE)
     && (thing != show)
    ){
      /* check hide skill */
      hide = CharGetHide(show);
      /* Check perception */
      perception = CharGetPerception(thing);
  
      if (hide >= perception) return TCS_CANTSEE;
    }
    /* Check for invisible people */
    if ( BIT(Character(show)->cAffectFlag, AF_INVIS)
     && !ParseCommandCheck(PARSE_COMMAND_MSTAT, sock, "") 
     && !BIT(Character(thing)->cAffectFlag, AF_SEEINVIS)
     && (thing != show)
    ){
      return TCS_CANTSEE;
    }
  } else if (show->tType==TTYPE_OBJ) {
    /* if its a reset command hide it from Joe User */
    if (Obj(show)->oTemplate->oType == OTYPE_RESETCMD) {
      /* Choke if they are too weeny to see Reset Cmds by seeing if they can rcreate */
      if (ParseCommandCheck(PARSE_COMMAND_RCREATE, sock, "")){
        return canSee;
      } else {
        return TCS_CANTSEE;
      }
    }
    /* Check for invisible objects */
    if ( BIT(Obj(show)->oAct, OA_INVISIBLE)
     && !ParseCommandCheck(PARSE_COMMAND_OSTAT, sock, "") 
     && !BIT(Character(thing)->cAffectFlag, AF_SEEINVIS)
    ){
      /* can see invis obj if you are holding it */
      if (Base(show)->bInside!=thing || !Obj(show)->oEquip)
        return TCS_CANTSEE;
    }
    /* Check for hidden objects - cant be hidden if we're holding it */
    if ( BIT(Obj(show)->oAct, OA_HIDDEN)
     && !ParseCommandCheck(PARSE_COMMAND_OSTAT, sock, "") 
     && Base(show)->bInside!=thing
    ){
      return TCS_CANTSEE;
    }
  }
  
  return canSee;
}

/* return if they can hear it */
LWORD ThingCanHear(THING *thing, THING *hear) {
  if (thing->tType >= TTYPE_CHARACTER) {
    if (Character(thing)->cPos <= POS_SLEEPING)
      return FALSE;
  }

  return TRUE;
}

/* 
 * show one liner for everything, if mob/player in weird position (ie fighting 
 * etc) add extra information. Dont show anything if thing cant see it!
 * (invis/blinded etc)
 */
THING *ThingShow(THING *show, THING *thing) {
  LWORD see;
  LWORD count;
  BYTE  buf[256];
  SOCK  *sock;

  see = ThingCanSee(thing, show);
  if (see == TCS_CANTSEE)
    return show->tNext;

  switch (show->tType) {
  case TTYPE_WLD:
    SendThing("^c", thing);
    SendThing(show->tSDesc->sText, thing);
    SendThing("\n", thing);
    break;

  case TTYPE_PLR:
    /* fighting */
    if (Character(show)->cFight) {
      SendAction("^r$n is here",   show,                    thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);      
      SendAction("^, fighting $n", Character(show)->cFight, thing, SEND_DST|SEND_VISIBLE);
    } else if (Character(show)->cPos == POS_STANDING) {
      /* normal - show title */
      SendThing("^2", thing);
      SendAction(Base(show)->bLDesc->sText, show, thing, SEND_DST|SEND_VISIBLE);
    } else {
      SendAction(posList[Character(show)->cPos].pDesc, show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);      
    }

    /* show if helper */
    if (BIT(Plr(show)->pSystem, PS_HELPER))
      SendAction(" ^C<HELPER>", show, thing, SEND_DST|SEND_VISIBLE);

    /* show if killer */
    if (BIT(Plr(show)->pSystem, PS_KILLER))
      SendAction(" ^y<KILLER>", show, thing, SEND_DST|SEND_VISIBLE);

    /* show if god */
    if (Character(show)->cLevel >= LEVEL_GOD)
      SendAction(" ^w<GOD>", show, thing, SEND_DST|SEND_VISIBLE);

    /* show if invis */
    if (BIT(Character(show)->cAffectFlag, AF_INVIS))
      SendAction(" ^w<INVIS>", show, thing, SEND_DST|SEND_VISIBLE);

    /* show if hidden */
    if (BIT(Character(show)->cAffectFlag, AF_HIDDEN))
      SendAction(" ^w<HIDDEN>", show, thing, SEND_DST|SEND_VISIBLE);

    /* show following stuff */
    if (Character(show)->cLead) {
      /* show is part of a group */
      if (Character(show)->cLead == show) {
        if (Character(show)->cFollow)
          SendAction(" ^p<LEADER>", show, thing, SEND_DST|SEND_VISIBLE);
      } else {
        if (BIT(Character(show)->cAffectFlag, AF_GROUP)||GroupIsLeader(show))
          SendAction(" ^P<GROUP>", show, thing, SEND_DST|SEND_VISIBLE);
        else
          SendAction(" ^P<FOLLOW>", show, thing, SEND_DST|SEND_VISIBLE);
      }
    }

    /* show if editing */
    sock = BaseControlFind(show);
    if (!sock)
      SendAction(" ^y<LINKDEAD>", show, thing, SEND_DST|SEND_VISIBLE);
    else if (sock && sock->sEdit.eStrP)
      SendAction(" ^y<EDITING>", show, thing, SEND_DST|SEND_VISIBLE);

    if (Plr(show)->pIdleRoom)
      SendAction(" ^y<IDLE>", show, thing, SEND_DST|SEND_VISIBLE);

    SendThing("\n", thing);
    break;

  case TTYPE_MOB:
    /* fighting */
    if (Character(show)->cFight) {
      SendAction("^r$n is here",   show,                    thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);      
      SendAction("^, fighting $n", Character(show)->cFight, thing, SEND_DST|SEND_VISIBLE);
    } else {
      /* non-default position */
      if (Character(show)->cPos != Mob(show)->mTemplate->mPos) {
        SendAction(posList[Character(show)->cPos].pDesc, show, thing, SEND_DST|SEND_VISIBLE|SEND_CAPFIRST);      
      /* default position */
      } else {
        SendThing("^2", thing);
        SendAction(Base(show)->bLDesc->sText, show, thing, SEND_DST|SEND_VISIBLE);
      } 
    } 

    /* show if invis */
    if (BIT(Character(show)->cAffectFlag, AF_INVIS))
      SendAction(" ^w<INVIS>", show, thing, SEND_DST|SEND_VISIBLE);

    /* show if hidden */
    if (BIT(Character(show)->cAffectFlag, AF_HIDDEN))
      SendAction(" ^w<HIDDEN>", show, thing, SEND_DST|SEND_VISIBLE);

    /* show following stuff */
    if (Character(show)->cLead) {
      /* show is part of a group */
      if (Character(show)->cLead == show) {
        if (Character(show)->cFollow)
          SendAction(" ^b<LEADER>", show, thing, SEND_DST|SEND_VISIBLE);
      } else {
        if (BIT(Character(show)->cAffectFlag, AF_GROUP)||GroupIsLeader(show))
          SendAction(" ^p<GROUP>", show, thing, SEND_DST|SEND_VISIBLE);
        else
          SendAction(" ^r<FOLLOW>", show, thing, SEND_DST|SEND_VISIBLE);
      }
    }

    SendThing("\n", thing);
    break;

  case TTYPE_OBJ:
    /* if its in a bag or something override LDesc with SDesc */
    if (Base(show)->bInside->tType != TTYPE_WLD) {
      SendThing("^g", thing);
      SendAction(show->tSDesc->sText, show, thing, SEND_DST|SEND_VISIBLE);
    } else {
      SendThing("^2", thing);
      SendAction(Base(show)->bLDesc->sText, show, thing, SEND_DST|SEND_VISIBLE);
    }
    
    /* Count identical objects */
    count = 0;
    if (!Obj(show)->oEquip)
      count++;
    while(  Obj(show)->oTemplate->oVirtual >= 0
         && show->tNext
         &&show->tType == show->tNext->tType
         &&Obj(show)->oTemplate == Obj(show->tNext)->oTemplate)
    {
      if (!Obj(show)->oEquip)
        count++;
      show=show->tNext;
    }
    if (count > 1) {
      sprintf(buf, " [%ld]", count);
      SendAction(buf, show, thing, SEND_DST|SEND_VISIBLE);
    }
    
    SendThing("\n", thing);
    break;

  default:
    Log(LOG_ERROR, "ThingShow: ARGH! what the hell kinda tType is that\n");
  }

  return show->tNext;
}

/*
 * they have explicitly looked at this
 *
 */
void ThingShowDetail(THING *show, THING *thing, BYTE showDesc) {
  SOCK *sock;
  BYTE  expert = FALSE;
  
  sock = BaseControlFind(thing);
  if (sock && BIT(Plr(sock->sHomeThing)->pAuto, PA_EXPERT)) expert = TRUE;
  
  SendThing("^1",thing);
  SendThing(show->tSDesc->sText, thing);
  SendThing("\n", thing);
  if (showDesc) {
    SendThing("^2",thing);
    SendThing(show->tDesc->sText, thing);
    if (show->tType==TTYPE_OBJ) {
      if (Obj(show)->oTemplate->oType==OTYPE_BOARD) {
        SendThing("^gThere seems to be something written on it... (^bTry ^wREAD^bing it)\n", thing);
      } else if (Obj(show)->oTemplate->oType==OTYPE_LIGHT) {
        ObjectShowLight(show, thing);
      } else if (Obj(show)->oTemplate->oType==OTYPE_SCANNER) {
        ObjectShowScanner(show, thing);
      } else if (Obj(show)->oTemplate->oType==OTYPE_DEVICE) {
        ObjectShowDevice(show, thing);
      } else if (Obj(show)->oTemplate->oType==OTYPE_WEAPON) {
        ObjectShowWeapon(show, thing);
      } else if (Obj(show)->oTemplate->oType==OTYPE_ARMOR) {
        ObjectShowArmor(show, thing);
      } else if (Obj(show)->oTemplate->oType==OTYPE_CONTAINER) {
        if (!ObjectShowContainer(show, thing)) return;  /* cant see inside closed containers */
      } else if (Obj(show)->oTemplate->oType==OTYPE_DRINKCON) {
        ObjectShowDrinkcon(show, thing);
      }
    }
    CharShowHealth(show, thing); /* Wont do anything if wrong type */
    CharShowEquip(show, thing);  /* Wont do anything if wrong type */
    
    /* Check for AutoConsider / during look */
    if (show->tType >= TTYPE_CHARACTER) {
      if (sock && BIT(Plr(sock->sHomeThing)->pAuto, PA_CONSIDER)) {
        SendThing("^cAnd if you're considering a fight:\n", thing);
        CbtConsider(thing, show);
      }
      
      /* Hints to low level characters */
      if (Character(thing)->cLevel <= 5 && !expert) {
        if (show->tType==TTYPE_MOB) {
          SendHint("^;HINT: If you want to start fighting type ^<KILL <target>^;\n", thing);
          if (!Character(thing)->cWeapon)
            SendHint("^;HINT: You should find a weapon before starting a fight!\n", thing);
          if (!Character(thing)->cEquip)
            SendHint("^;HINT: You should find some armor to wear before starting a fight!\n", thing);
        } else if (show->tType==TTYPE_PLR) {
          SendHint("^;HINT: You can speak to other players by typing ^<SAY <message>^;\n", thing);
        }
      }
    }
    
    /* Tell Newbie that they should Equip items */
    if (show->tType==TTYPE_OBJ && Character(thing)->cLevel <= 5 && !expert) {
      /* Hints for objects they are carrying */
      if (Base(show)->bInside==thing) {
        if (Obj(show)->oTemplate->oWear) {
          if (Obj(show)->oEquip)
            SendHint("^;HINT: To stop wearing/wielding an object, type ^<UNEQUIP <object>^;\n", thing);
          else
            SendHint("^;HINT: To start wearing/wielding an object, type ^<EQUIP <object>^;\n", thing);
        } else
          SendHint("^;HINT: To get rid of objects, type ^<DROP <object>^;\n", thing);
      }
      /* Hints for objects in the same room */
      if (Base(show)->bInside==Base(thing)->bInside) {
        if (Base(show)->bWeight>=0) {
          SendHint("^;HINT: To pickup an object, type ^<GET <object>^;\n", thing);
          SendHint("^;HINT: To see what you are allready carrying, type ^<INVENTORY^;\n", thing);
        }
      }
      SendHint("^;HINT: To try using this object, type ^<USE <object>^;\n", thing);
    }
  }
  if (!show->tContain) return;
  
  switch (show->tType) {
  case TTYPE_OBJ:
    SendThing("^cInside it you see:\n", thing);
    break;

  case TTYPE_PLR:
  case TTYPE_MOB:
    return;

  default: /* show everything inside */
    break;
  }
  
  /* show the things inside */
  for (show = show->tContain; show; ) {
    if (show != thing) {
      show = ThingShow(show, thing);
    } else
      show = show->tNext;
  }
}

/* Find first thing in list with FLAG set */
THING *ThingFindFlag(THING *thing, FLAG flag) {
  THING *i;
  for (i=thing; i; i=i->tNext) {
    if (BIT(i->tFlag, flag)) return i;
  }
  return NULL;
}

void ThingClearEvent(void) {
  IndexClear(&eventThingIndex);
}

BYTE ThingIsEvent(void *thing) {
  if (IndexFind(&eventThingIndex, thing, NULL))
    return TRUE;
  return FALSE;
}

void ThingSetEvent(void *thing) {
  IndexAppend(&eventThingIndex, thing);
}

void ThingDeleteEvent(void *thing) {
  if (ThingIsEvent(thing))
    IndexDelete(&eventThingIndex, thing, NULL);
}
