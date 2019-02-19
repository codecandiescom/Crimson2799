/* all the various macros used by Crimson2 */
/* "log.h" must be included prior to using a few of these */

#define MINV(a,b)\
(\
 ((a) < (b)) ? (a):(b) \
 )

#define MAXV(a,b)\
(\
 ((a) > (b)) ? (a):(b) \
 )

#define BOUNDV(a,b,c)\
(\
 MINV( MAXV(a,b), c ) \
 )

/* absolute value */
#define ABSV(a)\
(\
 ((a) > 0) ? (a):(-1*(a)) \
 )

#define MAXSET(n,max) n=MINV(n,max)
#define MINSET(n,min) n=MAXV(min,n)
#define BOUNDSET(min,n,max) n=BOUNDV(min,n,max)

/* Bit operations */
#define BIT(flag,bit) (((flag)&(bit))==(bit)) /* if all the bits are set */
#define BITANY(flag,bit) ((flag)&(bit)) /* if any bit is set */
#define BITSET(flag,bit) flag|=(bit)
#define BITCLR(flag,bit) flag&=(~(bit))
#define BITFLIP(flag,bit) if (BIT( (flag),(bit) )) BITCLR( (flag),(bit) ); else BITSET( (flag),(bit) )

/* Perror routine */
#define PERROR(routineName) \
do { \
  BYTE buf[256]; \
  sprintf(buf, "%s(%s:%d) ", routineName, __FILE__, __LINE__); \
  perror(buf); \
} while(0)

#ifdef WIN32
  /* Perror routine */
  #define PSOCKETERROR(routineName) \
  do { \
    printf("%s(%s:%d): %s\n", routineName, __FILE__, __LINE__, WSAGetLastErrorString() ); \
  } while(0)

  #define EXIT(error) \
  do { \
    WSACleanup(); \
    exit(error); \
  } while(0)

#else
  /* Unix is just PERROR */
  #define PSOCKETERROR(routineName) PERROR(routineName)

  #define EXIT(error) exit(error)
#endif

