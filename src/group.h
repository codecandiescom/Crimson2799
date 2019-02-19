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
 * group.h by B. Cameron Lesiuk, 1999
 *
 */
 
extern BYTE   GroupIsLeader(THING *thing);
extern THING *GroupGetHighestLeader(THING *thing);
extern THING *GroupGetLeader(THING *thing);
extern BYTE   GroupIsMember(THING *thing, THING *group);
extern BYTE   GroupIsFollower(THING *thing, THING *follower);
extern BYTE   GroupIsGroupedMember(THING *thing, THING *group);
extern void   GroupGetStat(THING *group,LWORD *numChar, LWORD *numLevel);
extern THING *GroupGetBefore(THING *thing);
extern THING *GroupGetLast(THING *group);
extern BYTE   GroupAddFollow(THING *thing, THING *followThing);
extern BYTE   GroupRemoveFollow(THING *thing);
extern void   GroupKillFollow(THING *thing);
extern void   GroupRemoveAllFollow(THING *thing);
extern void   GroupGainExp(THING *thing, LWORD exp);
extern BYTE   GroupVerifySize(THING *group);
extern THING *GroupGetHighestLevel(THING *group);
