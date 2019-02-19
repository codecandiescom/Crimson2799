/* timing.h */

extern void TvNormalize(struct timeval *tv);
extern void TvSetConst (struct timeval *tv, LWORD sec, LWORD usec);
extern void TvAddConst (struct timeval *tv, LWORD sec, LWORD usec);
extern void TvSubConst (struct timeval *tv, LWORD sec, LWORD usec);
extern void TvGetTime(struct timeval *tv);

/* associated macros */
#define     TVADD(tv1, tv2) TvAddConst(&tv1, tv2.tv_sec, tv2.tv_usec)
#define     TVSUB(tv1, tv2) TvSubConst(&tv1, tv2.tv_sec, tv2.tv_usec)
#define     TVSET(tv1, tv2) TvSetConst(&tv1, tv2.tv_sec, tv2.tv_usec)

/* handy little debugging routine */
#define     TVPRINTF(tv) printf("tv%ld.%ld\n", tv.tv_sec, tv.tv_usec)
