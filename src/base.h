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
 * Phone number: (604) 591-5295
 * snail mail:
 * 14095 57A Ave
 * Surrey, BC
 * V3X 2W2
 * Canada
 */


/* notes on the linking system:
 Well every link has a send (SND) and recieve (RCV) side, for example say
 Joe is being telepathically snooped by Bob. Joe has a SND type link
 that points to Bobs Thing, Bob has a RCV type link to Joes Thing and both
 of them have a CONTROL link to their respective controlling sockets (ie
 Control links are the exception to the two sided rule... complicated enuff
 fer ya?)
*/



/* even is SND, odd is RCV */
/* so ltype%2 will get you absolute link type */

#define BL_UNDEF         0 /* we're confused */
#define BL_CONTROL       1 /* link to a socket */
#define BL_TELEPATHY_SND 2 
#define BL_TELEPATHY_RCV 3 
#define BL_HEAR_SND      4 
#define BL_HEAR_RCV      5 
#define BL_SEE_SND       6 
#define BL_SEE_RCV       7 
#define BL_C4_SND        8 
#define BL_C4_RCV        9 

struct BaseLinkType {
  union blDetailType {
    SOCK                 *lSock;
    THING                *lThing;
  } lDetail;

  WORD                  lType;
  BASELINK             *lNext;
};

/* for things that can be moved around, ie everything but rooms */
struct BaseType {
  THING                 bThing;

  STR                  *bKey;       /* player name for players, keylist for others */
  STR                  *bLDesc;     /* long string A Black Dagger is lying here/title */
  THING                *bInside;    /* one level up in the hierarchy, anything can be in anything this way */
  WORD                  bConWeight; /* weight of everthing contained inside us */
  WORD                  bWeight;    /* hmm everything must have a weight this way */
  BASELINK             *bLink;      /* any&everything can be linked to sockets, */
                                    /* among other things this should allow bugs&cameras in the rooms */
  SOCK                 *bControl;
};

extern BYTE     *linkTypeList[];

extern void      BaseLinkAlloc(THING *sndThing, THING *rcvThing, WORD lType);
extern void      BaseLinkCreate(THING *sndThing, THING *rcvThing, WORD lType);
extern BASELINK *BaseLinkFind(BASELINK *next, WORD lType, THING *thing);
extern void      BaseLinkFree(THING *thing, BASELINK *link);
extern void      BaseFree(THING *thing);
extern void      BaseControlAlloc(THING *thing, SOCK *sock);
extern SOCK     *BaseControlFind(THING *thing);
extern void      BaseControlFree(THING *thing, SOCK *sock);
#define Base(x) ((BASE*)(x))
