#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>


/* Global constants */
#define TRUE 1
#define FALSE 0

void nocr(char *filename) {
  FILE *theInFile, *theOutFile;
  size_t i, size;
  struct stat theStat;
  char *buf;

  printf("Reading in %s", filename);
  /* get the size of the file */
  stat(filename,&theStat);
  size = (size_t) theStat.st_size;
  buf = (char *) malloc(size);
  if (!buf) {
    printf("\nInsufficient memory to scan in file\n");
    exit(0);
  }
  /* read in the file */
  if (!(theInFile = fopen(filename, "rb"))) {
    printf("Error: couldnt open inFile\n");
    exit(1);
  }
  fread(buf, size, 1, theInFile);
  fclose(theInFile);

  /* write out the file */
  if (!(theOutFile = fopen(filename, "wb"))) {
    printf("Error: couldnt open outFile\n");
    exit(1);
  }
  printf(", Writing it out less <CR>'s to %s\n", filename);
  for (i=0; i<size; i++) {
    if (buf[i] != 13){
      if (buf[i] == '\t') {
        fputc(' ', theOutFile);
        fputc(' ', theOutFile);
      } else
        fputc(buf[i], theOutFile);
    }
  }
  fclose(theOutFile);
}

void main( int argc, char *argv[] ) {
   DIR           *theDir;
   struct dirent *theEnt;
   char           dirBuf[256];
   char           fileBuf[256];
   int            i;
   int            fileLen;
   int            sLen;

/*
   if (argc < 2) {
     printf("Usage: nocr <file>\n");
     printf("\nthe file will be read in, then written out less <CR> characters\n");
     exit(0);
   }
   
   strcpy(dirBuf,  argv[1]);
   strcpy(fileBuf, argv[1]);
   for (i=strlen(dirBuf); i>0 && dirBuf[i]!='/'; i--);
   if (i>0) {
     dirBuf[i]='\0';
     strcpy(fileBuf, dirBuf+i+1);
   } else
     strcpy(dirBuf, ".");
   fileLen = strlen(fileBuf);
*/

   /* look for things matching argv[1] - ah, skip it just nocr *.c *.h */
   printf("Searching for all *.c and *.h files\n");
   strcpy(dirBuf, ".");

   /* there's gotta be an OS kinda way to do this but I dont know how offhand */
   theDir = opendir(dirBuf);
   theEnt=readdir(theDir);/* . */
   theEnt=readdir(theDir);/* .. */

   while( (theEnt=readdir(theDir)) ) {
     sLen = strlen(theEnt->d_name);
     if (sLen > 2 && !strcmp(theEnt->d_name+sLen-2, ".c")) {
       nocr(theEnt->d_name);
     } else if (sLen > 2 && !strcmp(theEnt->d_name+sLen-2, ".h")) {
       nocr(theEnt->d_name);
     } else if (sLen > 4 && !strcmp(theEnt->d_name+sLen-4, ".map")) {
       nocr(theEnt->d_name);
     } else if (sLen > 4 && !strcmp(theEnt->d_name+sLen-4, ".wld")) {
       nocr(theEnt->d_name);
     } else if (sLen > 4 && !strcmp(theEnt->d_name+sLen-4, ".hlp")) {
       nocr(theEnt->d_name);
     } else if (sLen > 4 && !strcmp(theEnt->d_name+sLen-4, ".obj")) {
       nocr(theEnt->d_name);
     } else if (sLen > 4 && !strcmp(theEnt->d_name+sLen-4, ".mob")) {
       nocr(theEnt->d_name);
     } else if (sLen > 4 && !strcmp(theEnt->d_name+sLen-4, ".rst")) {
       nocr(theEnt->d_name);
     }/*  else if (!strcmp(theEnt->d_name, "makefile")) {
       nocr(theEnt->d_name);
     } */
   }
   closedir(theDir);

}
