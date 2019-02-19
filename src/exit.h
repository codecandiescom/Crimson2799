/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary, if you are using it without my express
 * permission you are violating the copyright act and can potentially be
 * sued.
 * That said, if you would like to use it, I'm not an ogre, gimme a call
 * and maybe we can work something out.
 *
 * Current email addresses: cryogen@unix.infoserve.net
                            rhaksi@freenet.vancouver.bc.ca
 * Phone number: (604) 591-9746
 */

#define EDIR_MIN       -1
#define EDIR_NORTH      0
#define EDIR_EAST       1
#define EDIR_SOUTH      2
#define EDIR_WEST       3
#define EDIR_UP         4
#define EDIR_DOWN       5
#define EDIR_OUT        6
#define EDIR_UNDEFINED  7
#define EDIR_MAX        8

#define EF_ISDOOR       1<<0
#define EF_PICKPROOF    1<<1
#define EF_LOCKED       1<<2
#define EF_CLOSED       1<<3
#define EF_HIDDEN       1<<4
#define EF_ELECTRONIC   1<<5 /* hacking skill not picklock */
#define EF_NOPHASE      1<<6 /* cant phase through */

extern LWORD exitNum;
extern BYTE *dirList[];
extern LWORD eOrderList[];
extern BYTE *eFlagList[];

struct ExitType {
   STR             *eKey;
   STR             *eDesc;
   LWORD            eKeyObj;
   FLAG             eFlag;
   THING           *eWorld;
   BYTE             eDir;
   EXIT            *eNext;
};

extern EXIT *ExitAlloc(EXIT *eNext, BYTE eDir, STR *eKey, STR *eDesc, FLAG eFlag, WORD eKeyObj, THING *eWorld);
extern EXIT *ExitCreate(EXIT *eNext, BYTE eDir, BYTE *eKey, BYTE *eDesc, FLAG eFlag, WORD eKeyObj, THING *eWorld);
extern EXIT *ExitFind(EXIT *eList, BYTE *eKey);
extern EXIT *ExitDir(EXIT *eList, BYTE eDir);
extern EXIT *ExitReverse(THING *world, EXIT *exit);
extern LWORD ExitIsCorner(EXIT *exit, EXIT *reverse);
extern EXIT *ExitFree(EXIT *eList, EXIT *exit);
extern BYTE *ExitGetName(EXIT *exit, BYTE *buf);

#define EXITFREE(eList, exit) eList=ExitFree(eList, exit);

#define Exit(x) ((EXIT*)(x))
