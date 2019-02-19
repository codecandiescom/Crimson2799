/* Crimson2 Mud Server 
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary, if you are using it without my express
 * permission you are violating the copyright act and can potentially be
 * sued.
 * That said, if you would like to use it, I'm not an ogre, gimme a call
 * and maybe we can work something out.
 *
 * Current email address: cryogen@unix.infoserve.net
 * Phone number: (604) 591-9746
 */


struct ExtraType {
  STR                     *eKey;
  STR                     *eDesc;
  EXTRA                   *eNext;
};

extern EXTRA *ExtraAlloc(EXTRA *eNext, STR *eKey, STR *eDesc);
extern EXTRA *ExtraCreate(EXTRA *eNext, BYTE *eKey, BYTE *eDesc);
extern EXTRA *ExtraCopy(EXTRA *eChain);
extern EXTRA *ExtraFind(EXTRA *eList, BYTE *eKey);
extern EXTRA *ExtraFindWithin(THING *thing, BYTE *eKey, LWORD *offset);
extern EXTRA *ExtraFree(EXTRA *eList, EXTRA *extra);

#define EXTRAFREE(eList, extra) eList=ExtraFree(eList, extra);

#define Extra(x) ((EXTRA*)(x))
