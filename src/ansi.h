/**************************************************************************/
/* ANSI Color codes                                                       */
/**************************************************************************/

#define TEST     "[1;36;44m[K"
/* This is some sample text that should all be
 * blah blah blah, some bloody foreground and
 * background
 */
#define TEST2    "[1;33;43m[K"
/* This is some sample text that should all be
 * blah blah blah, some bloody foreground and
 * background
 */
#define ENDTEST  "[0m[K"     /* END - use default text ie LGRAY on BLACK */   

#define BKRED    "[41m[K" /* ON RED */
#define BKGREEN  "[42m[K" /* ON GREEN */
#define BKBROWN  "[43m[K" /* ON BROWN */
#define BKBLUE   "[44m[K" /* ON BLUE */
#define BKPURPLE "[45m[K" /* ON PURPLE */
#define BKCYAN   "[46m[K" /* ON CYAN */
#define BKGRAY   "[47m[K" /* ON GRAY */
#define BKBLACK  "[40m[K" /* ON BLACK */

#define ENDBK    "[0m[K"     /* END - use default text ie LGRAY on BLACK */   

#define BLACK    "[0;30;40m[K" /* darker shades */
#define DKRED    "[0;31;40m[K" /* DKRED */
#define DKGREEN  "[0;32;40m[K" /* DKGREEN */
#define BROWN    "[0;33;40m[K" /* BROWN */
#define DKBLUE   "[0;34;40m[K" /* DKBLUE */
#define DKPURPLE "[0;35;40m[K" /* DKPURPLE */
#define DKCYAN   "[0;36;40m[K" /* DKCYAN */
#define LGRAY    "[0;37;40m[K" /* LGRAY */
#define DKGRAY   "[1;30;40m[K" /* DKGRAY */
#define LRED     "[1;31;40m[K" /* LRED */
#define LGREEN   "[1;32;40m[K" /* LGREEN */
#define YELLOW   "[1;33;40m[K" /* YELLOW */
#define LBLUE    "[1;34;40m[K" /* LBLUE */
#define LPURPLE  "[1;35;40m[K" /* LPURPLE */
#define LCYAN    "[1;36;40m[K" /* LCYAN */
#define WHITE    "[1;37;40m[K" /* WHITE */


#define REVERSE  "[7m"    /* test  - so you can more ansi.h to see it */
#define BLINK    "[5m"    /* test */
#define END      "[0m[K"     /* END - use default text ie LGRAY on BLACK */   


/**************************************************************************/
/* Notes on ANSI/VT100 codes                                              */
/**************************************************************************/

/* Some ansi escape sequences, heres how ansi works:
 * Set Character Attributes:
 * <ESCAPE> [ <code1> ; <code2> ; ... <coden> m
 *  0- 1 sets foreground color intensity
 * 30-37 sets foreground color hue
 * 40-47 sets bkground color
 *
 * There are of course a lot more, but that's all I use right at the moment
 */

