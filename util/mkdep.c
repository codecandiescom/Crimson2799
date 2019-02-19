/* This utility is based on a program which I found in the 
 * linux kernel source tree. I hacked at it to be a non-kernel 
 * specific. 
 * I am not aware of any copyrights associated with this file, nor
 * am I aware of even who the author is. But looking at the way 
 * it's written, I don't think the author would want to admit to
 * writing this abhorent thing.
 */


#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

char *filename, *command, __depname[256] = "\n\t@touch ";
int hasdep;
char srcname[256];

#define depname (__depname+9)

struct path_struct {
	int len;
	char buffer[256-sizeof(int)];
} path_array[2] = {
	{ 23, "/usr/src/linux/include/" },
	{  0, "" }
};

static void handle_include(int type, char *name, int len)
{
	int plen;
	struct path_struct *path = path_array+type;

	plen = path->len;
	memcpy(path->buffer+plen, name, len);
	len += plen;
	path->buffer[len] = '\0';
	if (access(path->buffer, F_OK))
		return;

	if (!hasdep) {
		hasdep = 1;
		printf("%s: %s ", depname,srcname);
	}
	/*printf(" \\\n   %s", path->buffer);*/
	printf(" %s", path->buffer);
}

#if defined(__alpha__) || defined(__i386__)
#define LE_MACHINE
#endif

#ifdef LE_MACHINE
#define next_byte(x) (x >>= 8)
#define current ((unsigned char) __buf)
#else
#define next_byte(x) (x <<= 8)
#define current (__buf >> 8*(sizeof(unsigned long)-1))
#endif

#define GETNEXT { \
next_byte(__buf); \
if (!__nrbuf) { \
	__buf = *(unsigned long *) next; \
	__nrbuf = sizeof(unsigned long); \
	if (!__buf) \
		break; \
} next++; __nrbuf--; }
#define CASE(c,label) if (current == c) goto label
#define NOTCASE(c,label) if (current != c) goto label

static void state_machine(register char *next)
{
	for(;;) {
	register unsigned long __buf = 0;
	register unsigned long __nrbuf = 0;

normal:
	GETNEXT
__normal:
	CASE('/',slash);
	CASE('"',string);
	CASE('\'',char_const);
	CASE('#',preproc);
	goto normal;

slash:
	GETNEXT
	CASE('*',comment);
	goto __normal;

string:
	GETNEXT
	CASE('"',normal);
	NOTCASE('\\',string);
	GETNEXT
	goto string;

char_const:
	GETNEXT
	CASE('\'',normal);
	NOTCASE('\\',char_const);
	GETNEXT
	goto char_const;

comment:
	GETNEXT
__comment:
	NOTCASE('*',comment);
	GETNEXT
	CASE('/',normal);
	goto __comment;

preproc:
	GETNEXT
	CASE('\n',normal);
	CASE(' ',preproc);
	CASE('\t',preproc);
	CASE('i',i_preproc);
	GETNEXT

skippreproc:
	CASE('\n',normal);
	CASE('\\',skippreprocslash);
	GETNEXT
	goto skippreproc;

skippreprocslash:
	GETNEXT;
	GETNEXT;
	goto skippreproc;

i_preproc:
	GETNEXT
	CASE('f',if_line);
	NOTCASE('n',skippreproc);
	GETNEXT
	NOTCASE('c',skippreproc);
	GETNEXT
	NOTCASE('l',skippreproc);
	GETNEXT
	NOTCASE('u',skippreproc);
	GETNEXT
	NOTCASE('d',skippreproc);
	GETNEXT
	NOTCASE('e',skippreproc);

/* "# include" found */
include_line:
	GETNEXT
	CASE('\n',normal);
	CASE('<', std_include_file);
	NOTCASE('"', include_line);

/* "local" include file */
{
	char *incname = next;
local_include_name:
	GETNEXT
	CASE('\n',normal);
	NOTCASE('"', local_include_name);
	handle_include(1, incname, next-incname-1);
	goto skippreproc;
}

/* <std> include file */
std_include_file:
{
	char *incname = next;
std_include_name:
	GETNEXT
	CASE('\n',normal);
	NOTCASE('>', std_include_name);
	handle_include(0, incname, next-incname-1);
	goto skippreproc;
}

if_line:
if_start:
	GETNEXT
	CASE('\n', normal);
	CASE('_', if_middle);
	if (current >= 'a' && current <= 'z')
		goto if_middle;
	if (current < 'A' || current > 'Z')
		goto if_start;

if_middle:
	GETNEXT
	CASE('\n', normal);
	CASE('_', if_middle);
	if (current >= 'a' && current <= 'z')
		goto if_middle;
	if (current < 'A' || current > 'Z')
		goto if_start;
	goto if_middle;
	}
}

static void do_depend(void)
{
	char *map;
	int mapsize;
	int pagesizem1 = getpagesize()-1;
	int fd = open(filename, O_RDONLY);
	struct stat st;

	if (fd < 0) {
		perror("mkdep: open");
		return;
	}
	fstat(fd, &st);
	mapsize = st.st_size + 2*sizeof(unsigned long);
	mapsize = (mapsize+pagesizem1) & ~pagesizem1;
	map = mmap(NULL, mapsize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (-1 == (long)map) {
		perror("mkdep: mmap");
		close(fd);
		return;
	}
	close(fd);
	state_machine(map);
	munmap(map, mapsize);
	if (hasdep) {
		puts(command);
		printf("\t$(CC) $(CFLAGS) %s -o %s\n\n",srcname,depname);
  }
}

int main(int argc, char **argv)
{
	int len;
	char * hpath;
	int makefile,readlen;
	char makefilename[1024],*p;
	char *marker="#### MARKER - DO NOT EDIT THIS LINE ####";

	hpath = getenv("HPATH");
	if (!hpath)
		hpath = "/usr/src/linux/include/";
	len = strlen(hpath);
	memcpy(path_array[0].buffer, hpath, len);
	if (len && hpath[len-1] != '/') {
		path_array[0].buffer[len] = '/';
		len++;
	}
	path_array[0].buffer[len] = '\0';
	path_array[0].len = len;

	/* the first thing we do is copy our old makefile to a 
	 * new (temporary) location. Note: we actually leave it 
 	 * there - let the makefile move it back. :) */

	if (argc<2) {
		printf("USEAGE: %s <makefile> [<.c, .S, .o files>]\n",argv[0]);
		exit(0);
	}

	makefile=open(argv[1],O_RDONLY);
	if (makefile<0) {
	  printf("%s: cannot open source makefile %s\n",argv[0],argv[1]);
	  exit(1);
	}
	/* Ok! Let's copy this sucker over! */
	/* use "makefilename" as a buffer */
	readlen=1;
	while(readlen>0) {
		/* read a line */
		p=makefilename-1;
		do {
			p++;
			readlen=read(makefile,p,1);
			*(p+1)=0;
		} while((readlen>0)&&(*p!=0x0a));
		/* we have a line in makefilename */
		printf("%s",makefilename);
		if (strlen(makefilename)>=16) {
			if (!strncmp(makefilename,marker,strlen(marker))) {
				break;
			}
		}
	}

	/* we don't need our source makefile anymore */
	close(makefile);

	argc--; /* skip past our first <makefile> parameter */
	argv++;
	while (--argc > 0) {
		int len;
		char *name = *++argv;

		filename = name;
		len = strlen(name);
		memcpy(depname, name, len+1);
		memcpy(srcname,name,len+1);
		command = __depname;
		if (len > 2 && name[len-2] == '.') {
			switch (name[len-1]) {
				case 'o':
					name[len-1]='c';
					srcname[len-1]='c';
				case 'c':
				case 'S':
					depname[len-1] = 'o';
					command = "";
			}
		}

 /*   hasdep = 0;*/
		hasdep = 1;
		printf("%s: %s ", depname,srcname);

		do_depend();
	}
	return 0;
}
