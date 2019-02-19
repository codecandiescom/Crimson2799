#define AFFECT_NONE              0
#define AFFECT_STR               AFFECT_NONE+1
#define AFFECT_DEX               AFFECT_STR+1
#define AFFECT_INT               AFFECT_DEX+1
#define AFFECT_WIS               AFFECT_INT+1
#define AFFECT_CON               AFFECT_WIS+1
#define AFFECT_HIT               AFFECT_CON+1
#define AFFECT_MOVE              AFFECT_HIT+1
#define AFFECT_POWER             AFFECT_MOVE+1
#define AFFECT_ARMOR             AFFECT_POWER+1
#define AFFECT_HITROLL           AFFECT_ARMOR+1
#define AFFECT_DAMROLL           AFFECT_HITROLL+1
#define AFFECT_RES_PUNCTURE      AFFECT_DAMROLL+1
#define AFFECT_RES_SLASH         AFFECT_RES_PUNCTURE+1
#define AFFECT_RES_CONCUSSIVE    AFFECT_RES_SLASH+1
#define AFFECT_RES_HEAT          AFFECT_RES_CONCUSSIVE+1
#define AFFECT_RES_EMR           AFFECT_RES_HEAT+1
#define AFFECT_RES_LASER         AFFECT_RES_EMR+1
#define AFFECT_RES_PSYCHIC       AFFECT_RES_LASER+1
#define AFFECT_RES_ACID          AFFECT_RES_PSYCHIC+1
#define AFFECT_RES_POISON        AFFECT_RES_ACID+1
#define AFFECT_SPEED             AFFECT_RES_POISON+1

/* Affect flags */
#define AF_GROUP          1<<0 /* if set than thing is grouped with its leader */
#define AF_IMPROVED       1<<1 /* If set then attacking wont wear off invis */
#define AF_INVIS          1<<2 /* if set thing becomes invisible to others */
#define AF_BREATHWATER    1<<3 /* if set thing doesnt take damage underwater */
#define AF_DARKVISION     1<<4 /* if set thing can see in the dark */
#define AF_SEEINVIS       1<<5 /* if set thing can see in the dark */
#define AF_REGENERATION   1<<6 /* fast regen of hit points */
#define AF_ENDURANCE      1<<7 /* half-movement cost */
#define AF_HIDDEN         1<<8 /* hiding and preparing to ambush */
#define AF_PHASEWALK      1<<9 /* No terrain costs */
#define AF_WATERWALK      1<<10 /* walk on water without a boat */
#define AF_VACUUMWALK     1<<11 /* walk in vacuum without taking damage */
#define AF_FIGHTSTART     1<<12 /* started the fight */
#define AF_DOMINATED      1<<13 /* being mind controlled */
#define AF_SNEAK          1<<14 /* sneaking */
#define AF_SENSELIFE      1<<15 /* can see hidden creatures */
#define AF_PHASEDOOR      1<<16 /* walk through doors */

/* special durations */
#define AD_UNLIMITED      -1
#define AD_NONE           -2

struct AffectType {
  BYTE    aNum;
  BYTE    aEffect;
  WORD    aDuration;
  LWORD   aValue; /* could be a pointer */
  AFFECT *aNext;
};

extern BYTE *affectList[];

#define AFFECT_CREATE 0
#define AFFECT_FREE   1
extern void AffectThing(BYTE mode, THING *thing, WORD aNum, LWORD aValue);

/* the following routines use AffectThing to set/remove flags and modifiers */
extern AFFECT *AffectCreate(THING *thing, WORD aNum, LWORD aValue, WORD aDuration, WORD eType);
extern void    AffectReplace(THING *thing, WORD aNum, LWORD aValue, WORD aDuration, WORD eType);
extern void    AffectApply(THING *thing, AFFECT *affect);
extern void    AffectUnapply(THING *thing, AFFECT *affect);
extern void    AffectRead(THING *thing, AFFECT *affect, FILE *file);
extern void    AffectWrite(THING *thing, AFFECT *affect, FILE *file);
extern void    AffectReadPrimitive(THING *thing, AFFECT *affect, FILE *file);
extern void    AffectWritePrimitive(THING *thing, AFFECT *affect, FILE *file);
extern void    AffectApplyAll(THING *thing);
extern void    AffectUnapplyAll(THING *thing);
extern void    AffectRemove(THING *thing, AFFECT *affect);
extern void    AffectFree(THING *thing, AFFECT *affect);
extern AFFECT *AffectFind(THING *thing, LWORD effect);
extern LWORD   AffectTick(THING *thing);
