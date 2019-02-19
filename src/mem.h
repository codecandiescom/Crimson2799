/* since mallocs are a blow to performance and wastefull of memory besides
   malloc multiple blocks of a given size at once to minimize memory waste
   and never really free anything just keep it in a linked list of free blocks
   of that size (since we will be re-using blocks of the same size very
   frequently)  
   
   if you dont know why I allways double and use powers of two, dont play with 
   my mem management functions, you dont know enough yet....
   
   by the way dont mix up MemFree&MemAlloc with malloc/free cuz bad things will happen
*/

typedef struct memType { /* kinda a small structure heh? */
  struct memType  *mNext;
} MEM; /* is just overlaid on top of allready (and much larger) allocated blocks */

extern BYTE *memSpace;
extern LWORD memSpaceUsed; /* nothing in here is *EVER* free'd */
extern LWORD memSpaceByte;
extern LWORD memUsed;

extern void  MemInit(void);
extern void  MemInitDone(void);

#define      MEM_MALLOC -1 /* use this for size/blocksize to directly MALLOC things */
extern BYTE *MemFree(MEM *memFree, LWORD size);
extern BYTE *MemAlloc(LWORD size, LWORD blockSize);
extern void *MemReallocDouble(BYTE *errorStr, void *memPtr, LWORD memSize, LWORD memNum, LWORD *memByte); 

extern void *MemMoveHigher(void *dst, void *src, LWORD size);

/* use the macros, they're more convenient */
#define      MEMFREE(mem, type)               mem=(type*)MemFree((MEM*)mem,sizeof(type))
#define      MEMALLOC(mem, type, blocksize)   mem=(type*)MemAlloc(sizeof(type),blocksize)
#define      REALLOC(errorstr, memptr, memtype, memnum, membyte) memptr = MemReallocDouble(errorstr, memptr, sizeof(memtype), memnum, &membyte)

/* exit on an error cuz we're going down anyways */
#define MALLOC(mem, type, num) \
do { \
  if (! (mem = (type *)malloc(num*sizeof(type))) ) { \
    Log(LOG_ERROR, "malloc failure: dying a horrible death...\n"); \
    exit(ERROR_NOMEM);\
  }\
  memUsed += (num*sizeof(type)); \
} while(0)  \

#define FREE(mem, size) \
do { \
  /* memset(mem, 0, size) */ \
  free(mem); \
  memUsed -= size; \
} while(0)  \


#ifdef BLAH
/* double mem requests every time */
/* note can only access < curMaxSize */
/* str should be something like "WARNING! resizing XXYYZZ\n" */
/* mem should be a pointer pre-inited to NULL if empty */
/* curMaxByte determines the size of the initial malloc too, curMaxSize is recalc'd for any change */
#define REALLOC(errorstr, memptr, memtype, memnum, membyte) \
do { \
  if ( (memptr == NULL) || (memnum >= (membyte/sizeof(memtype)) ) ){ \
    if (memptr) \
      membyte *=2; \
    if (!memptr) { \
      if ( !(memptr = (memtype *)malloc(membyte)) ) { \
        Log(LOG_ERROR, "realloc/malloc failure: dying a horrible death...\n"); \
        exit(ERROR_NOMEM); \
      } \
      break; \
    } else { \
      if ( !(memptr = (memtype *)realloc(memptr, membyte)) ) { \
        Log(LOG_ERROR, "realloc failure: dying a horrible death...\n"); \
        exit(ERROR_NOMEM); \
      } \
    } \
    Log(LOG_ERROR, errorstr); \
  } \
} while(0) 

#endif
