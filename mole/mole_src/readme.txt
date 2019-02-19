***************************************************
MOLE - Mud On-Line Editor
By B. Cameron Lesiuk, 1997-1999

Copyright (c) B. Cameron Lesiuk, 1997-199
All Rights Reserved

Permission is hereby granted to copy, modify, use
in other programs, or otherwise do anything you
want with this source code AS LONG AS you do not
use any of this source code in a commercial product
or in support of a commercial product (eg: a pay-mud).

If you wish to use any of this source code in a
commercial product, drop me an email and we can
work something out.

B. Cameron Lesiuk
wi961@freenet.victoria.bc.ca
****************************************************

DISCLAIMER: I don't guarentee this program will work
at all for any purpose whatsoever. USE OF THIS PROGRAM
AND ASSOCIATED FILES IS COMPLETELY AND WHOLELY AT YOUR
OWN RISK!!! Don't come crying to me if this program
causes you financial, physical, emotional, or any other
form of harm.

***************************************************

HOW TO COMPILE THIS SUCKER:

This program was written with Borland C++ V4.52. If you
use Borland V4.5X, be sure to go through the options hierarchy
and reset the directories to point to where your source
and BC45 directories reside. You'll also need to make a 
"16" and "32" directories under the mole directory; either
that or change where Borland C++ throws all the .obj etc
files - again, check the options hierarchy.

I don't know if it will work under any other compiler, so you're
on your own. It is basically straight C code because I
still haven't become comfortable with all that C++ object
class inheritance method mumbo jumbo.

I wrote the code as generically as I could, using standard
Winblows calls etc (if there is such a thing as a "standard
winblows call"). I didn't use any Borland extensions I'm aware
of, so all is not lost ;)

For the 16-bit version (I'm not sure it still compiles, but...)
you will need the following files:
  terminal.c
  editadt.c
  editrst.c
  clipbrd.c
  editobj.c
  editwld.c
  editflag.c
  pack.c
  unpack.c
  editprop.c
  editmob.c
  edit.c
  enviromt.c
  areawnd.c
  dstruct.c
  timer.c
  moleprot.c
  infobox.c
  about.c
  dialog.c
  host.c
  molem.c
  main.c
  debug.c
  ctl3dl.c
  winsockl.c
and of course the .rc and all the .h files. I didn't use any special
#define's for the 16-bit code, so theoretically if you throw all the
above files in a project and hit "compile", it should work.

The 32-bit version is a little different. You need all the above files
as well as the following:
  ansitbl.c
  ansiterm.c
  termbuf.c
  mem.c
You also need the following #define's (I put them in the project file -
that's why they don't appear in any .c or .h file)

#define MSDOS
#define WIN32_LEAN_AND_MEAN

Now, you may or may not need the MSDOS one - I'm not sure anymore.
The Win32_LEAN_AND_MEAN was just so that it didn't completely clober
my 486 (which this program was originally developed on). You may
not need this either under a 32-bit compiler environment. Remember,
my Borland C++ 4.52 was essentially a 16-bit compiler that could do
32-bit programs as well, but it was targeted towards Win32s, NOT the
32-bit Win95/98/NT/2000 platforms. So you may have to play around a bit.

One place I KNOW you'll want to look is in dstruct.h - there are
IMPORTANT typedefs for the different integer/byte/etc formats.
IE: That's where I typedef DSUWORD to "unsigned short" in 32-bit
or "unsigned int" in 16-bit environments. So you'll want to check
it out. I think all you'll need is to #define WIN32 if it's not
already done for you.

Well, that's all I can really suggest. If you're compiling this
in a MafiaSoft compiler, then may god help you. If you're compiling this
under Borland's new C++ Builder, may god help you. If you're compiling this
using GCC under Linux, DOS, or Winblows, may god help you. Let's just
say I'm not terribly fond of any Winsuck compilers available these days. :(
And I'm not very inclined to help anyone who insists on using them :(

Use the source, Luke!

B. Cameron Lesiuk
December 14, 1999
wi961@freenet.victoria.bc.ca



