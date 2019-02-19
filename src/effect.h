struct EffectListType {
  BYTE          *eName;
  EFFECTPROC   (*eProc);
  WORD           eCheck; /* double check constant versus this entry */
};

#define EFFECT_NONE           0                 /* silly option really */
#define EFFECT_UNCODED        EFFECT_NONE+1     /* silly option really */
/* Body */
#define EFFECT_CELL_REPAIR    EFFECT_UNCODED+1
#define EFFECT_REFRESH        EFFECT_CELL_REPAIR+1
#define EFFECT_ENDURANCE      EFFECT_REFRESH+1
#define EFFECT_BREATHWATER    EFFECT_ENDURANCE+1
#define EFFECT_STRENGTH       EFFECT_BREATHWATER+1
#define EFFECT_DARKVISION     EFFECT_STRENGTH+1
#define EFFECT_SLOW_POISON    EFFECT_DARKVISION+1
#define EFFECT_CURE_POISON    EFFECT_SLOW_POISON+1
#define EFFECT_POISON         EFFECT_CURE_POISON+1
#define EFFECT_HEAL_MINOR     EFFECT_POISON+1
#define EFFECT_REGENERATION   EFFECT_HEAL_MINOR+1
#define EFFECT_HEAL_MAJOR     EFFECT_REGENERATION+1
#define EFFECT_DEXTERITY      EFFECT_HEAL_MAJOR+1
#define EFFECT_CONSTITUTION   EFFECT_DEXTERITY+1
#define EFFECT_HASTE          EFFECT_CONSTITUTION+1
#define EFFECT_QUENCH         EFFECT_HASTE+1
#define EFFECT_SUSTENANCE     EFFECT_QUENCH+1
#define EFFECT_ACIDTOUCH      EFFECT_SUSTENANCE+1
#define EFFECT_POISONTOUCH    EFFECT_ACIDTOUCH+1

  /* Telekinetic */                    
#define EFFECT_CRUSH          EFFECT_POISONTOUCH+1
#define EFFECT_FORCESTORM     EFFECT_CRUSH+1
#define EFFECT_GHOSTFIST      EFFECT_FORCESTORM+1
#define EFFECT_KINETIC_SHIELD EFFECT_GHOSTFIST+1
#define EFFECT_IMMOBILIZE     EFFECT_KINETIC_SHIELD+1
#define EFFECT_HEARTSTOP      EFFECT_IMMOBILIZE+1
#define EFFECT_ASPHYXIATE     EFFECT_HEARTSTOP+1
#define EFFECT_INVISIBILITY   EFFECT_ASPHYXIATE+1
#define EFFECT_SLOW           EFFECT_INVISIBILITY+1
#define EFFECT_IMPROVEDINVIS  EFFECT_SLOW+1
#define EFFECT_VACUUMWALK     EFFECT_IMPROVEDINVIS+1

  /* Telepathy */                     
#define EFFECT_PHANTOMEAR     EFFECT_VACUUMWALK+1
#define EFFECT_PHANTOMEYE     EFFECT_PHANTOMEAR+1
#define EFFECT_MINDLINK       EFFECT_PHANTOMEYE+1
#define EFFECT_DOMINATION     EFFECT_MINDLINK+1
#define EFFECT_THOUGHTBLADE   EFFECT_DOMINATION+1
#define EFFECT_MINDCRUSH      EFFECT_THOUGHTBLADE+1
#define EFFECT_DEATHDREAM     EFFECT_MINDCRUSH+1
#define EFFECT_MINDSHIELD     EFFECT_DEATHDREAM+1
#define EFFECT_SLEEP          EFFECT_MINDSHIELD+1
#define EFFECT_BERSERK        EFFECT_SLEEP+1
#define EFFECT_MINDCLEAR      EFFECT_BERSERK+1

  /* Apportation */                     
#define EFFECT_TELEPORT       EFFECT_MINDCLEAR+1
#define EFFECT_SUMMON         EFFECT_TELEPORT+1
#define EFFECT_SUCCOR         EFFECT_SUMMON+1
#define EFFECT_BANISH         EFFECT_SUCCOR+1
#define EFFECT_DISRUPTDOOR    EFFECT_BANISH+1
#define EFFECT_PHASEDOOR      EFFECT_DISRUPTDOOR+1
#define EFFECT_PHASEWALK      EFFECT_PHASEDOOR+1
#define EFFECT_PHANTOMPOCKET  EFFECT_PHASEWALK+1
#define EFFECT_WATERWALK      EFFECT_PHANTOMPOCKET+1
#define EFFECT_TELETRACK      EFFECT_WATERWALK+1
#define EFFECT_RECALL         EFFECT_TELETRACK+1
#define EFFECT_MARK           EFFECT_RECALL+1
#define EFFECT_TRANSLOCATE    EFFECT_MARK+1

  /* Spirit */                     
#define EFFECT_SPIRITWALK     EFFECT_TRANSLOCATE+1
#define EFFECT_SENSELIFE      EFFECT_SPIRITWALK+1
#define EFFECT_SEEINVISIBLE   EFFECT_SENSELIFE+1
#define EFFECT_LUCKSHIELD     EFFECT_SEEINVISIBLE+1
#define EFFECT_IDENTIFY       EFFECT_LUCKSHIELD+1
#define EFFECT_STAT           EFFECT_IDENTIFY+1
#define EFFECT_LUCKYHITS      EFFECT_STAT+1
#define EFFECT_LUCKYDAMAGE    EFFECT_LUCKYHITS+1

  /* Pyrokinetic */                     
#define EFFECT_BURNINGFIST    EFFECT_LUCKYDAMAGE+1
#define EFFECT_FLAMESTRIKE    EFFECT_BURNINGFIST+1
#define EFFECT_INCINERATE     EFFECT_FLAMESTRIKE+1
#define EFFECT_IGNITE         EFFECT_INCINERATE+1
#define EFFECT_HEATSHIELD     EFFECT_IGNITE+1
#define EFFECT_FIREBLADE      EFFECT_HEATSHIELD+1
#define EFFECT_FIRESHIELD     EFFECT_FIREBLADE+1
#define EFFECT_FIREARMOR      EFFECT_FIRESHIELD+1

  /* Misc. */
#define EFFECT_REVITALIZE     EFFECT_FIREARMOR+1
 

/* Targetting flags for effects */
#define TAR_NODEFAULT      1<<0  /* no default dont do anything without cmd str */
#define TAR_SELF_DEF       1<<1  /* you */
#define TAR_FIGHT_DEF      1<<2  /* what you are fighting */
#define TAR_GROUP_DEF      1<<3  /* the group (in the same location, and not you) */
#define TAR_NOTGROUP_DEF   1<<4  /* everything but your group */
#define TAR_NOTSELF_DEF    1<<5  /* everything but you */
#define TAR_MULTIPLE       1<<6  /* allow user to MassFireball all.orc */
#define TAR_PLR_ROOM       1<<7  /* player in the same room/inv */
#define TAR_PLR_WLD        1<<8  /* player anywhere */
#define TAR_MOB_ROOM       1<<9  /* mob in the same room/inv */
#define TAR_MOB_WLD        1<<10 /* mob anywhere */
#define TAR_OBJ_INV        1<<11 /* object in inventory but *NOT* in room */
#define TAR_OBJ_ROOM       1<<12 /* object in the same room but *NOT* in room */
#define TAR_OBJ_WLD        1<<13 /* object anywhere */
#define TAR_ON_WEAR        1<<14 /* apply to default when worn */
#define TAR_GROUPWLD_DEF   1<<15 /* the group (anywhere in the world, not you) */
#define TAR_AFFECT         1<<16 /* doesnt create an AFFECT structure, direct application */

#define EVENT_EFFECT      0 /* typically the You feel XXX message and then fall thru to EVENT_APPLY */
#define EVENT_UNEFFECT    1 /* typically the You feel less XXX message and then fall thru to EVENT_UNAPPLY */
#define EVENT_APPLY       2 /* if you build your own apply types then */
#define EVENT_UNAPPLY     3 /* I must be able to unapply and re-apply the effect for PlayerWrite */
#define EVENT_GROUP       4
#define EVENT_UNGROUP     5 
#define EVENT_FREE        6 /* being free'd, so free up any custom structs */
#define EVENT_READ        7 /* chance to read custom data */
#define EVENT_WRITE       8 /* override write with custom data */

extern void  EffectInit(void);
extern LWORD Effect(WORD effect, FLAG tarFlag, THING *thing, BYTE *cmd, LWORD data);
extern LWORD EffectFree(WORD effect, FLAG tarFlag, THING *thing, LWORD data);

extern struct EffectListType effectList[];

extern INDEX  tarIndex;

extern EFFECTPROC(EffectUncoded);
extern EFFECTPROC(EffectCellRepair);
extern EFFECTPROC(EffectRefresh);
extern EFFECTPROC(EffectEndurance);
extern EFFECTPROC(EffectBreathwater);
extern EFFECTPROC(EffectStrength);
extern EFFECTPROC(EffectDarkvision);
extern EFFECTPROC(EffectSlowPoison);
extern EFFECTPROC(EffectCurePoison);
extern EFFECTPROC(EffectPoison);
extern EFFECTPROC(EffectHealMinor);
extern EFFECTPROC(EffectRegeneration);
extern EFFECTPROC(EffectHealMajor);
extern EFFECTPROC(EffectDexterity);
extern EFFECTPROC(EffectConstitution);
extern EFFECTPROC(EffectHaste);
extern EFFECTPROC(EffectQuench);
extern EFFECTPROC(EffectSustenance);
extern EFFECTPROC(EffectAcidtouch);
extern EFFECTPROC(EffectPoisontouch);
extern EFFECTPROC(EffectCrush);
extern EFFECTPROC(EffectForcestorm);
extern EFFECTPROC(EffectGhostfist);
extern EFFECTPROC(EffectKineticShield);
extern EFFECTPROC(EffectInvisibility);
extern EFFECTPROC(EffectSlow);
extern EFFECTPROC(EffectImprovedInvis);
extern EFFECTPROC(EffectVacuumwalk);
extern EFFECTPROC(EffectDomination);
extern EFFECTPROC(EffectThoughtblade);
extern EFFECTPROC(EffectMindcrush);
extern EFFECTPROC(EffectDeathdream);
extern EFFECTPROC(EffectMindshield);
extern EFFECTPROC(EffectMindclear);
extern EFFECTPROC(EffectBerserk);
extern EFFECTPROC(EffectSummon);
extern EFFECTPROC(EffectSuccor);
extern EFFECTPROC(EffectPhasedoor);
extern EFFECTPROC(EffectPhasewalk);
extern EFFECTPROC(EffectPhantompocket);
extern EFFECTPROC(EffectWaterwalk);
extern EFFECTPROC(EffectTeletrack);
extern EFFECTPROC(EffectRecall);
extern EFFECTPROC(EffectMark);
extern EFFECTPROC(EffectTranslocate);
extern EFFECTPROC(EffectSpiritwalk);
extern EFFECTPROC(EffectSenseLife);
extern EFFECTPROC(EffectSeeinvisible);
extern EFFECTPROC(EffectLuckshield);
extern EFFECTPROC(EffectIdentify);
extern EFFECTPROC(EffectStat);
extern EFFECTPROC(EffectLuckyhits);
extern EFFECTPROC(EffectLuckydamage);
extern EFFECTPROC(EffectBurningfist);
extern EFFECTPROC(EffectFlamestrike);
extern EFFECTPROC(EffectIncinerate);
extern EFFECTPROC(EffectHeatshield);
extern EFFECTPROC(EffectFireblade);
extern EFFECTPROC(EffectFireshield);
extern EFFECTPROC(EffectFirearmor);
extern EFFECTPROC(EffectRevitalize);

