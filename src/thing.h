/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * (yes *ALL* code was written by me from scratch! any similarities to other
 *  muds is strictly coincidence)
 *
 * This source code is proprietary, if you are using it without my express
 * permission you are violating the copyright act and can potentially be
 * sued.
 * That said, if you would like to use it, I'm not an ogre, gimme a call
 * and maybe we can work something out.
 * (NOTE: File formats are mostly diku compatible so migrating to crimson2
 *  from your current diku flavour should prove reasonably easy!)
 * (and by the way it MURDERS dikumud servers in every respect *GRIN*!)
 *
 * Current email address: cryogen@unix.infoserve.net
 * Phone number: (604) 591-9746
 * snail mail:
 * 14095 57A Ave
 * Surrey, BC
 * V3X 2W2
 * Canada
 */

/* "object" hierarchy:

  THING
   |____________________________
   |                            |
  WLD                          BASE
                                |__________
                                |          |
                               CHR        OBJ
                               _|_____
                              |       |
                             PLR     MOB

NOTE: the 3 letter "leaf" entries are stored in files of the same extension,
ie WLD structures are stored in <filename>.wld files etc etc

I'm still not entirely sold on this being the best way to do this, since things
like worldList and objectList are now of type WLD/OBJ rather than something more
intuitive like WORLDLIST and OBJECTLIST.... oh well just remember that "things"
in the game have 3 letter types and you should be okay
*/

#define TTYPE_UNDEF     0
#define TTYPE_WLD       1

#define TTYPE_BASE      2 /* everything greater than this is a base type */
#define TTYPE_OBJ       2

#define TTYPE_CHARACTER 3 /* so far everything greater than this is a character */
#define TTYPE_MOB       3
#define TTYPE_PLR       4

/* for everything */
struct ThingType {
  BYTE                  tType;

  STR                  *tSDesc;   /* name/short string ie Kobold/A Dagger/PName */
  STR                  *tDesc;    /* paragraph or so description, what you see when you look <key> */

  EXTRA                *tExtra;   /* if we have any extras associated, player/mob/obj desc is an extra */
  LWORD                 tWait;    /* control for how soon next action takes place */
  LWORD                 tIdleWait;/* control for how soon next @IDLE event takes place */
  PROPERTY             *tProperty;/* property list - including attached scripts/special procs */
  FLAG                  tFlag;    /* system flags */

  THING                *tNext;
  THING                *tContain; /* any Thing can contain other Things - ie players can carry other players! */
};

#define TF_COMPILE            1<<0 /* thing contains uncompiled code, only used for WLD's */
#define TF_TRAVERSED          1<<1 /* to prevent Track routine from circling, only used for WLD's */

#define TF_IDLECODE           1<<2
#define TF_FIGHTINGCODE       1<<3
#define TF_COMMANDCODE        1<<4
#define TF_ENTRYCODE          1<<5
#define TF_DEATHCODE          1<<6
#define TF_EXITCODE           1<<7
#define TF_FLEECODE           1<<8
#define TF_RESETCODE          1<<9
#define TF_DAMAGECODE         1<<10
#define TF_USECODE            1<<11
#define TF_AFTERFIGHTINGCODE  1<<12
#define TF_AFTERCOMMANDCODE   1<<13
#define TF_AFTERENTRYCODE     1<<14
#define TF_AFTERDEATHCODE     1<<15
#define TF_AFTEREXITCODE      1<<16
#define TF_AFTERFLEECODE      1<<17
#define TF_AFTERRESETCODE     1<<18
#define TF_AFTERDAMAGECODE    1<<19
#define TF_AFTERUSECODE       1<<20
#define TF_CODETHING          1<<21
#define TF_AFTERATTACKCODE    1<<22

extern INDEX  eventThingIndex;
extern BYTE  *tFlagList[];

extern void   ThingInit(void);
extern void   ThingFrom(THING *thing);
extern void   ThingTo(THING *thing, THING *destThing);
extern void   ThingExtract(THING *thing);
extern THING *ThingFree(THING *thing);

#define TF_CONTINUE     1<<0 /* continue last search */
#define TF_PLR          1<<1 /* find a plr in searches contain field */
#define TF_OBJ          1<<2 /* find an obj in searches contain field */
#define TF_MOB          1<<3 /* find a mob in searches contain field */
#define TF_PLR_ANYWHERE 1<<4 /* any plr in game, people inside NULL *DONT* count */
#define TF_MOB_ANYWHERE 1<<5 /* any mob in game, NOTE: wont search "search" twice */
#define TF_OBJ_ANYWHERE 1<<6 /* any obj in game */
#define TF_PLR_WLD      1<<7 /* any plr inside a wld structure */
#define TF_MOB_WLD      1<<8 /* any mob inside a wld structure */
#define TF_OBJ_WLD      1<<9 /* any obj inside a wld structure (ie not carried) */
#define TF_WLD          1<<10 /* find a wld structure treat name as a keylist */
#define TF_OBJINV       1<<11 /* find an inv obj in searches contain field */
#define TF_OBJEQUIP     1<<12 /* find an equiped obj in searches contain field */

#define TF_ALLMATCH     -1

extern THING *ThingFind(BYTE *key, LWORD virtual, THING *search, FLAG findFlag, LWORD *offset);
extern EXIT  *ThingTrack(THING *thing, THING *track, LWORD maxHop);
extern EXIT  *ThingTrackStr(THING *thing, BYTE *key, LWORD maxHop);

#define TCS_CANTSEE     0
#define TCS_SEEPARTIAL  1
#define TCS_SEENORMAL   2

extern LWORD  ThingCanSee(THING *thing, THING *show);
extern LWORD  ThingCanHear(THING *thing, THING *hear);
extern THING *ThingShow(THING *show, THING *thing);
extern void   ThingShowDetail(THING *show, THING *thing, BYTE showDesc);
extern THING *ThingFindFlag(THING *thing, FLAG flag);
extern void   ThingClearEvent();
extern BYTE   ThingIsEvent(void *thing);
extern void   ThingSetEvent(void *thing);
extern void   ThingClearEvent();
extern void   ThingDeleteEvent(void *thing);

#define THINGFREE(thing) (thing=ThingFree(thing))
#define Thing(x) ((THING*)(x))
