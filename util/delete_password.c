/* This file is intended to be used when transporting the MUD (with player 
 * files) over to another machine with an incompatible encryption routine.
 * This proc will strip out the password from the specified player file.
 * if no files are specified, STDIN is the assumed input, and STDOUT is
 * the assumed output.
 */

#include<stdio.h>

int main(int argc,char **argv) {
  int infile;
  int i,stdio;

  stdio=1;
  for (i=1;i<argc;i++) {
    if (argv[i][0]=='-') {
      switch (argv[i][1]) {
        case 'h':
        case 'H':
        case '?': 
          printf("USEAGE: %s [-h -H -?] [<plr file> <plr file> ...]\n",argv[0]);
          exit;
        default:
          printf("%s: unrecognized parameter: %s\n",argv[0],argv[i]);
          exit;
          break;
      }
    } else {
      stdio=0;
    }
  }
  
  /* ok, let's delete the passwords now! */ 
  for (i=1;i<argc;i++) {
    if (argv[i][0]!='-') {
      if (stdio) {
        infile=0;
      } else {
        
      }
    }
  }

}
