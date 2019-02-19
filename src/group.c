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
 * 
 * group.c by B. Cameron Lesiuk, 1999
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#ifndef WIN32
  #include <unistd.h> /* for unlink */
  #include <dirent.h>
#endif

#include "crimson2.h"
#include "macro.h"
#include "queue.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "ini.h"
#include "extra.h"
#include "property.h"
#include "file.h"
#include "thing.h"
#include "index.h"
#include "edit.h"
#include "history.h"
#include "socket.h"
#include "parse.h"
#include "send.h"
#include "world.h"
#include "base.h"
#include "object.h"
#include "char.h"
#include "fight.h"
#include "affect.h"
#include "mobile.h"
#include "player.h"
#include "skill.h"
#include "group.h"

 /*
 * Ok, here's how the group thing is going to work. 
 * There are three relevant fields: 
 *   Character(thing)->cLead                 - points to our leader
 *   Character(thing)->cFollow               - points to next follower
 *   Character(thing)->cAffectFlag|AF_GROUP  - says if we're grouped or not.
 * Now, this is not many fields to support a multi-treed follow structure, 
 * but it can be done. We just need some rules:
 *   1) A Leader is always considered to be grouped within the group
 *      they are leading, and all sub-groups. IE: the AF_GROUP flag
 *      does NOT serve the same purpose for leaders as for followers.
 *   2) For a follower (member of a group), they need AF_GROUP set in
 *      order to share in any of the groups exp. Furthermore, if the
 *      follower is a leader of a subgroup, that subgroup also only 
 *      shares in the exp if the subgroup leader is flagged AF_GROUP
 *      in the higher level group.
 *   3) ->cLead: for followers, this points to the (sub)group leader.
 *      For leaders, this points either to themselves (for the ultimate
 *      leader of all groups) or to the next higher leader in the 
 *      tree.
 *   4) ->cLead is NULL if the character is not a member of any group. 
 *      cLead==NULL is necessary and sufficient to determine this.
 *   5) ->cFollow simply points to the next character in the follow chain.
 *      There are no guarentees about the ordering within this chain.
 *
 * -Cam
 */

/* GroupIsLeader -- returns TRUE if 'thing' is a leader of a group 
 * (could be a sub-group). Else returns FALSE */
BYTE GroupIsLeader(THING *thing) {
  THING *i;

  if (!thing) return FALSE;
  i=GroupGetHighestLeader(thing);
  /* now we go looking for followers of thing */
  for (;i;i=Character(i)->cFollow) {
    if (Character(i)->cLead==thing)
      return TRUE;
  }
  return FALSE;
}

/* GroupGetHighestLeader -- returns the highest-level leader in the
 * group tree. IE: The character everyone is REALLY following! */
THING *GroupGetHighestLeader(THING *thing) {
  if (!thing) return NULL;
  while ((thing)&&(Character(thing)->cLead!=thing))
    thing=Character(thing)->cLead;
  return thing;
}

/* GroupGetLeader -- returns the highest-level leader which
 * is GROUPED relative to 'thing'. IE: If 'thing' is part of 
 * group B, and group B's leader is part of group A, then
 * GroupGetLeader will return:
 *  - Leader of B if Leader of B is NOT AF_GROUP (ie: is not part of group A)
 *  - Leader of A if Leader of B IS AF_GROUP (ie: part of group A) */
THING *GroupGetLeader(THING *thing) {
  if (!thing) return NULL;
  while((thing)&&
        (Character(thing)->cLead!=thing)&&
	(Character(thing)->cAffectFlag&AF_GROUP) ) {
    thing=Character(thing)->cLead;
  }
  return thing;
}

/* GroupIsMember - returns TRUE if 'thing' is a member of 'group'
 * or a subgroup beneath 'group'. Note this function traverses both
 * up and down the tree, so 'group' can point to ANY element of 'group'. 
 * AF_GROUP is not considered. */
BYTE GroupIsMember(THING *thing, THING *group) {
  THING *i;
  if ((!thing)||(!group)) return FALSE;
  group=GroupGetHighestLeader(group);
  for (i=group;i;i=Character(i)->cFollow)
    if (i==thing) return TRUE;
  return FALSE;
}

/* GroupIsFollower - returns TRUE if 'follower' is, one way or another, 
 * following 'thing'. Else FALSE. */
BYTE GroupIsFollower(THING *thing, THING *follower) {
  if ((!thing)||(!follower))
    return FALSE;

  if (!GroupIsLeader(thing))
    return FALSE;

  while((follower)&&(follower!=thing)) {
    if ((follower==Character(follower)->cLead) ||
        (Character(follower)->cLead==NULL))
      return FALSE;
    follower=Character(follower)->cLead;
  }
  return TRUE;
}

/* GroupIsGroupedMember - similar to GroupIsMember, except it pays
 * attention to AF_GROUP. GroupIsGroupedMember will return TRUE
 * if 'thing' is a grouped member of 'group. IE: if 'thing'
 * should share in experience etc. granted to 'group'. Note
 * hierarchy and permission inheritance via AF_GROUP 
 * affects this procedure!!! */
BYTE GroupIsGroupedMember(THING *thing, THING *group) {

  if ((!thing)||(!group)) 
    return FALSE;

  /* If 'thing' is a full member of 'group', it has to 
   * exist as a group member, group leader, or within a subgroup of 'group'. 
   */
  /* if we are not the leader, then we must exist as a follower or 
   * a sub-group follower/leader. In any case, the following should
   * work... */
  if (GroupGetLeader(group)==GroupGetLeader(thing))
    return TRUE;
  return FALSE;
}

/* GroupGetStat calculates things like the total number of followers
 * in the group, the total level count in the group, etc. 
 * NOTE that ONLY AF_GROUP characters are counted!!! 
 * This function only operates within the local affected group. IE:
 * if 'group' points to a member of a subgroup, and the subgroup
 * does NOT have AF_GROUP to its larger group, then only the subgroup
 * is analyzed.  This proc only goes as far as AF_GROUP validity allows. */
void GroupGetStat(THING *group,LWORD *numChar, LWORD *numLevel) {
  LWORD  nChar;
  LWORD  nLevel;
  THING *i;
  
  if (!group) return;
  /* ok, let's count ourselves! */
  nChar=0;
  nLevel=0;
  for (i=GroupGetHighestLeader(group);
       i;
       i=Character(i)->cFollow) {
    if (GroupIsGroupedMember(i,group)) {
      nChar++;
      nLevel+=Character(i)->cLevel;
    }
  }
  if (numChar)
    *numChar=nChar;
  if (numLevel)
    *numLevel=nLevel;
}

/* GroupGetBefore - returns the thing just before 'thing' in the cFollow 
 * chain, irrespective of AF_GROUP etc. 
 * Will return NULL if you pass it the highest leader */
THING *GroupGetBefore(THING *thing) {
  THING *i;

  if (!thing)
    return NULL;
    
  i=GroupGetHighestLeader(thing);
  
  if (thing==i)
    return NULL;
    
  while((i)&&(Character(i)->cFollow!=thing))
    i=Character(i)->cFollow;
  return i;
}

/* GroupGetLast - returns the last thing in the cFollow chain, 
 * irrespective of AF_GROUP. */
THING *GroupGetLast(THING *group) {
  if (!group) 
    return NULL;
  while((group)&&(Character(group)->cFollow))
    group=Character(group)->cFollow;
  return group;
}

/* GroupVerifySize - returns TRUE if 'group' is OK, else FALSE */
BYTE GroupVerifySize(THING *group) {
  LWORD nChar;
  LWORD nLevel;

  if (!group) return TRUE;
  /* first, let's get some stats */
  GroupGetStat(group,&nChar,&nLevel);
  if (nChar>CHAR_MAX_FOLLOW)
    return FALSE;
  if (nLevel>Character(GroupGetLeader(group))->cLevel*CHAR_MAX_FOLLOW)
    return FALSE;
  return TRUE;
}

/* GroupAddFollow - returns TRUE if successfull, else FALSE */
/* Removes 'thing' from any current group (along with all of 
 * 'thing's followers) and starts following 'followThing'.  */
BYTE GroupAddFollow(THING *thing, THING *followThing) {
  if ((!thing)||(!followThing))
    return FALSE;
    
  /* check against circular groups */
  if (GroupIsFollower(thing, followThing))
    return FALSE;

  /* ok, first we need to detatch ourselves from our existing group. */
  GroupRemoveFollow(thing);
  
  /* ok, at this point, 'thing' has been detatched from it's previous group.
   * 'thing' should be the highest leader of a new hierarchy. */


  /* Now we need to join 'thing' and its followers to the new group */
  Character(thing)->cLead=followThing;  /* adds link via cLead */
  Character(GroupGetLast(followThing))->cFollow=thing; /* appends onto cFollow chain */
  BITCLR(Character(thing)->cAffectFlag,AF_GROUP); /* clears group status */

  /* we may need to elevate the leader's status */
  if (!Character(followThing)->cLead) {
    Character(followThing)->cLead=followThing;
  }

  /* and we're done! */
  return TRUE;
}

/* GroupRemoveFollow - returns TRUE if successfull, else FALSE */
/* This proc removes 'thing' and any followers from any group
 * 'thing' may be following at the moment. */
BYTE GroupRemoveFollow(THING *thing) {
  THING *oldGroup;
  THING *i;
  THING *j;
  THING *k;
  BYTE   done;
   
  /* do we have input? */
  if (!thing)    
    return TRUE;

  /* are we in a group? */
  if (!Character(thing)->cLead)
    return TRUE;
    
  /* are we already the leader? */
  oldGroup=GroupGetHighestLeader(thing); 
  if (oldGroup==thing)
    return TRUE;

  /* Ok, at this point, we're in a group, and we're not the ultimate leader. */
  /* Darn. We really have to do some work :( */
  /* let's start by spawning off a new group. */
  /* first, extract 'thing' from its group */
  i=GroupGetBefore(thing);
  Character(i)->cFollow=Character(thing)->cFollow;
  Character(thing)->cFollow=NULL;
  Character(thing)->cLead=thing;
  BITCLR(Character(thing)->cAffectFlag,AF_GROUP);

  /* ok, we now have two groups. CAUTION!!!! Currently the group tree 
   * for this group is **BROKEN**!!! Because we have members of oldGroup
   * who may be following 'thing'. We now have to move all members of
   * oldGroup who are following 'thing' over to 'thing's group!!! Ugh! 
   */
  done=0;
  while(!done) {
    done=1;
    /* let's loop through our entire oldGroup and see if anything is
     * following something in 'thing's new group. */
    i=oldGroup;
    while(i&&done) {
      /* see if i is following anyone in the new group */
      j=thing;
      while(j&&done) {
        if (Character(i)->cLead==j) {
          /* i(oldGroup) is following j(thing) -- move it over */
          done=0; /* note this forces us to start right back at the 
                   * beginning again-- oh well. It's easier on my brain
                   * this way. */
          /* note the following only works because I know it'll never
           * run into the special case where i=highest leader */
          k=oldGroup;
          while((k)&&(Character(k)->cFollow!=i))
            k=Character(k)->cFollow;
          /* note: I couldn't use GroupGetBefore() for the above 
           * because the cFollow/cLead tree is broken and things will
           * not work properly */
          Character(k)->cFollow=Character(i)->cFollow;
          Character(i)->cFollow=NULL;
          Character(GroupGetLast(thing))->cFollow=i;
        } else {
          j=Character(j)->cFollow;
        }
      }
      if (done)
        i=Character(i)->cFollow;
    }
  }
  /* finally, let's check if 'thing' actually has followers. */
  if (!Character(thing)->cFollow) {
    /* thing isn't in a group */
    Character(thing)->cLead=NULL; /* mark 'thing' as "on it's own". */
  }
  /* and also check if oldGroup has any left */
  if (!Character(oldGroup)->cFollow) {
    Character(oldGroup)->cLead=NULL;
  }
  return TRUE;
}

/* GroupKillFollow - removes 'thing' from any group it's in, in the
 * event of 'thing's death. Note if 'thing' is a leader, the next
 * group member in line becomes the new leader (inheriting all 
 * cLead etc.) and any dominated followers are freed */
void GroupKillFollow(THING *thing) {
  THING *i;
  THING *j;
  THING *leader;
  THING *newLeader;
  
  /* do we have input? */
  if (!thing)    
    return;

  /* are we in a group? */
  if (!Character(thing)->cLead)
    return;

  /* ok, we're in a group. Bloody hell. */
  leader=GroupGetHighestLeader(thing);
  
  /* first off, let's free anyone who is dominated. */
  /* This is untested, and I don't even know if it has to be here -- Cam 
  for (i=leader;i;i=j) {
    j=Character(i)->cFollow;
    if ((i!=thing)&&
        (Character(i)->cLead==thing)&&
        BIT(Character(i)->cAffectFlag,AF_DOMINATED)) {
      AffectFree(i,EFFECT_DOMINATION);
    }
  } */
     
  /* next, let's drop any non-grouped followers */
  /* IE: if somebody is following this sucker, and they die, 
   * then I suppose they stop following... no inheritance of leader
   * for non-grouped followers! */
  for (i=leader;i;i=j) {
    j=Character(i)->cFollow;
    if (i!=thing) {
      if (Character(i)->cLead==thing) {
        if (!BIT(Character(i)->cAffectFlag,AF_GROUP)) {
          GroupRemoveFollow(i);
        }
      }
    }
  }

  /* at this point, 'thing' is still in the cFollow group etc.. 
   * All we've done so far is strip any non-grouped followers who
   * were following the deceased. */
   
  /* next, if there's still followers, elevate the next grouped member to 
   * leader status. */
  newLeader=NULL;
  for (i=leader;i;i=j) {
    j=Character(i)->cFollow;
    if (i!=thing) {
      if (Character(i)->cLead==thing) {
        /* note because we've already stripped non-group members, we
         * don't have to check for AF_GROUP. */
        /* 'i' is a follower of 'thing' */
        if (newLeader) {
          /* 'i' becomes new follower of 'newLeader' */
          Character(i)->cLead=newLeader;
        } else {
          /* let's promote this sucker to the prodigeous status of 
           * leader... he he he hope they know what they're getting in to. */
          /* we've got a sticky point here... we have a "special case" if
           * thing is the GroupGetHighestLeader. */
          newLeader=i;
          if (thing==leader) {
            /* aw darn. This sucks. 'thing' is the supreme leader. 
             * well, let's move newLeader into the front position, if
             * required. I can't actually see a situation which would
             * require this, because we've stripped all our non-group
             * followers, and this one is the first one we came upon. 
             * but oh well.
             */
            /* move 'i' just below 'thing' */
            Character(GroupGetBefore(i))->cFollow=Character(i)->cFollow;
            Character(i)->cFollow=Character(thing)->cFollow;
            Character(thing)->cFollow=i;

            /* now assign new leadership */
            Character(i)->cLead=i;
            BITCLR(Character(i)->cAffectFlag,AF_GROUP);
          } else {
            /* inherit leadership */
            Character(i)->cLead=Character(thing)->cLead;
            /* inherit group status */
            /* note: we don't really have to do this... we could leave
             * out the BITSET case because we already know they have
             * AF_GROUP... or they would have been dropped. But oh well. */
            if (BIT(Character(thing)->cAffectFlag,AF_GROUP))
              BITSET(Character(i)->cAffectFlag,AF_GROUP);
            else
              BITCLR(Character(i)->cAffectFlag,AF_GROUP);
          }
        }
      }
    }
  }
  
  /* ok, let's remove this sucker from the bonds of his group */
  i=GroupGetBefore(thing);
  if (i) {
    Character(i)->cFollow=Character(thing)->cFollow;
  }
  Character(thing)->cFollow=NULL;
  Character(thing)->cLead=thing;
  BITCLR(Character(thing)->cAffectFlag,AF_GROUP);

  /* and we're done */
  return;
}

/* GroupRemoveAllFollow - removes all followers of 'thing' */
void GroupRemoveAllFollow(THING *thing) {
  THING *i;
  WORD   done;

  
  done=0;
  while(!done) {
    done=1;
    /* search entire group for followers of 'thing' */
    for(i=GroupGetHighestLeader(thing);i&&done;i=Character(i)->cFollow) {
      if ((i!=thing)&&(Character(i)->cLead==thing)) {
        GroupRemoveFollow(i);
        done=0;
      }
    }
  }
}

/* GroupGainExp - distributes experience through a group 
 * according to who has AF_GROUP privledges (relative to 'thing') */
void GroupGainExp(THING *thing, LWORD exp) {
  THING *i;
  LWORD  numChar;
  LWORD  numLevel;
  
  if (!thing)
    return;

  if (!Character(thing)->cLead) {
    /* they are not in a group */
    PlayerGainExp(thing,exp);
    return;
  }
  GroupGetStat(thing,&numChar,&numLevel);

  /* add some extra incentive to form groups */
  if (numChar>1) 
    exp=(exp*3)/2;

  /* now dole out the benefits */
  for (i=GroupGetHighestLeader(thing);i;i=Character(i)->cFollow) {
    if (GroupIsGroupedMember(i,thing)) {
      if (Base(i)->bInside == Base(thing)->bInside) {
        PlayerGainExp(i, exp*Character(i)->cLevel/numLevel);
      }
    }
  }
}

/* GroupGetHighestLevel - will return the 'thing' with the
 * highest level in 'group'. Note only the things in 
 * grouped with 'group' are considered. */
THING *GroupGetHighestLevel(THING *group) {
  THING *i,*best;

  if (!group)
    return NULL;
  if (!Character(group)->cFollow)
    return group;

  i=best=GroupGetHighestLeader(group);
  for (;i;i=Character(i)->cFollow) {
    if (Character(i)->cLevel>Character(best)->cLevel) {
      best=i;
    }
  }
  return best;
}

