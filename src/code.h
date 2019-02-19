extern void      CodeInit();
extern WORD      CodeCompileProperty(PROPERTY *property, THING *errorthing);
extern WORD      CodeDecompProperty(PROPERTY *property, THING *errorthing);
extern BYTE      CodeIsCompiled(PROPERTY *property);
extern void      CodeCheckFlag(THING *thing);
extern void      CodeSetFlag(THING *thing, PROPERTY *property);
extern void      CodeClearFlag(THING *thing, PROPERTY *property);
extern PROPERTY *CodeFind(PROPERTY *property, BYTE *eventStr);
extern STR      *CodeStrFree(STR *str);

extern BYTE      CodeParseIdle(THING *codeThing);
extern BYTE      CodeParseFighting(THING *eventThing, THING *codeThing);
extern BYTE      CodeParseAfterFighting(THING *eventThing, THING *codeThing);
extern BYTE      CodeParseCommand(THING *eventThing, THING *codeThing, BYTE *cmd);
extern BYTE      CodeParseAfterCommand(THING *eventThing, THING *codeThing, BYTE *cmd);
extern BYTE      CodeParseEntry(THING *eventThing, THING *codeThing, EXIT *exit);
extern BYTE      CodeParseAfterEntry(THING *eventThing, THING *codeThing, EXIT *exit);
extern BYTE      CodeParseExit(THING *eventThing, THING *codeThing, EXIT *exit);
extern BYTE      CodeParseAfterExit(THING *eventThing, THING *codeThing, EXIT *exit);
extern BYTE      CodeParseDeath(THING *eventThing, THING *codeThing, THING *deathThing);
extern BYTE      CodeParseAfterDeath(THING *eventThing, THING *codeThing, THING *deathThing);
extern BYTE      CodeParseFlee(THING *eventThing, THING *codeThing);
extern BYTE      CodeParseAfterFlee(THING *eventThing, THING *codeThing);
extern BYTE      CodeParseReset(THING *eventThing, THING *codeThing);
extern BYTE      CodeParseAfterReset(THING *eventThing, THING *codeThing);
extern BYTE      CodeParseDamage(THING *eventThing, THING *codeThing, THING *damageThing);
extern BYTE      CodeParseAfterDamage(THING *eventThing, THING *codeThing, THING *damageThing);
extern BYTE      CodeParseUse(THING *eventThing, THING *codeThing);
extern BYTE      CodeParseAfterUse(THING *eventThing, THING *codeThing);
extern BYTE      CodeParseAfterAttack(THING *eventThing, THING *codeThing, THING *targetThing);
extern ULWORD codeSet;

/* bit flags for CodeSet value */
#define CODE_BOOTCOMPILE         (1<<0) /* compile code upon boot */
#define CODE_SAVECOMMENT         (1<<1) /* compiler saves comments */
#define CODE_DEBUGCOMPILE        (1<<2) /* generate compiler debug messages */
#define CODE_DEBUGINTERP         (1<<3) /* generate interpreter debug messages */
#define CODE_DEBUGDECOMPILE      (1<<4) /* generate decompiler debug messages */

