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


struct PropertyType {
  STR                     *pKey;
  STR                     *pDesc;
  PROPERTY                *pNext;
};

extern PROPERTY *PropertyAlloc(PROPERTY *pNext, STR *pKey, STR *pDesc);
extern PROPERTY *PropertyCreate(PROPERTY *pHead, STR *pKey, STR *pDesc);
extern PROPERTY *PropertySet(PROPERTY *pHead, BYTE *pKey, BYTE *pDesc);
extern PROPERTY *PropertyCopy(PROPERTY *pChain);
extern PROPERTY *PropertyFind(PROPERTY *pList, BYTE *pKey);
extern PROPERTY *PropertyDelete(PROPERTY *pList, BYTE *pKey);
extern PROPERTY *PropertyFree(PROPERTY *pList, PROPERTY *property);
extern LWORD     PropertyGetLWord(THING *thing, BYTE *key, LWORD defaultVal);
extern void      PropertySetLWord(THING *thing, BYTE *key, LWORD setVal);

#define PROPERTYFREE(pList, property) pList=PropertyFree(pList, property);
