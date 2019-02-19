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

/* 
 Technically, iThing should be of type void* but since 9 times out of 10
 we are indexing a list of THING's, I chose to default it to that type
*/

struct IndexType {
  BYTE    iErrorStr[128];
  THING **iThing;
  LWORD   iNum;
  LWORD   iByte;
  FLAG    iFlag;
};

#define IF_ALLOW_DUPLICATE 1<<0

extern BYTE indexError;

extern void   IndexInit(INDEX *index, LWORD iByte, BYTE* errorStr, FLAG flag); /* Setup the Index's fields */
extern void IndexFree(INDEX *index);
extern void   IndexAlloc(INDEX *index); /* allocate space for another entry */
extern void   IndexAppend(INDEX *index, void *thing);
extern void   IndexInsert(INDEX *index, void *thing, INDEXPROC(*indexProc) );
extern void  *IndexFind(INDEX *index, void *key, INDEXFINDPROC(*iFindProc) );
extern void   IndexDelete(INDEX *index, void *thing, INDEXPROC(*indexProc) );
extern void   IndexClear(INDEX *index); /* erase all entries */
