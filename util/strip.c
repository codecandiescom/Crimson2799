#define NULL  0
#define FALSE  0
#define TRUE  1

/* Use mallocs rather than a static sized buffer of 32k */
#define USE_MALLOC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


void main (int argc, char *argv[]) {

  long size = 0, carvedSize = 32768, i;
  FILE *theFile;                         
#ifdef USE_MALLOC
  unsigned char *buffer;
#else
  unsigned char buffer[32768];
#endif
  unsigned char c = 0;
  char name[15];

  /* setup stuff */
#ifdef USE_MALLOC
  buffer = (char *) malloc(carvedSize);
  if (!buffer) { printf("(initial malloc) Out of memory.. argh!\n"); exit(1); }
#endif
  if (argc < 3) {
    printf("Usage: strip <infile> <outfile>\n");
    exit(1);
  }


  /* read in the file */
  printf("Opening the file %s\n", argv[1]);
  if (!(theFile = fopen(argv[1], "rb"))) {
    printf("Error opening file\n");
    fclose(theFile);
    exit(1);
  }

  while (!feof(theFile)) {
    c = fgetc(theFile);
    if (feof(theFile))
      break;
    if (c > 127) c-=128;
    if ( ((c > 26) && (c < 127)) || c==9 ) {
      if (size >= carvedSize) {
#ifdef USE_MALLOC
        carvedSize *= 2;
        buffer = realloc(buffer, carvedSize);
        if (!buffer) { printf("(realloc) Out of memory.. argh!\n"); exit(1); }
#else
      printf("Out of memory.. argh!\n");
      exit(1);
#endif
      }
      buffer[size] = c;
      size++;
    }
  }

  /* make sure file isnt empty */
  if (size == 0) {
    printf("Empty input file!\n");
    fclose(theFile);
    exit(1);
  }
  fclose(theFile);


  /* write out the new file */
  printf("Opening the file %s\n", argv[2]);
  if (!(theFile = fopen(argv[2], "wb"))) {
    printf("Error opening file\n");
    fclose(theFile);
    exit(1);
  }
  for (i = 0; i<size; i++)
    fprintf(theFile, "%c", buffer[i]);
  printf("Wrote the file.\n");
  fclose(theFile);
#ifdef USE_MALLOC
  free(buffer);
#endif
}
