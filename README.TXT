Crimson2799 MUD
Copyright (C) 1995-2000 Ryan Haksi, all rights reserved.
Copyright (C) 1995-2000 B. Cameron Lesiuk, all rights reserved.

This is a 100% custom text-based Multi-User Dimention (MUD) game
server and tool set. 

Please read the licence.txt document. If you do not agree to the conditions
of the licence.txt document, you may not use this software.

-----------------------------------------------------------------------
Crimson2799 MUD

Welcome!
 
This file is intended to serve as your initial guide to the Crimson2799
server and associated tool set. Unfortunately, this doesn't really 
constitute "documentation", but it's the best we've got time for now.

This version of Crimson2799 is configured to operate a Science Fiction
Text-based MUD. The storyline etc. can be found in the help files in the
help directory (although the files are better read from within the MUD).
This archive comes complete with sample areas which we used on our
Crimson2799 MUD at phantom.me.uvic.ca:5005 for 5 years.

I am going to assume you are reading this document with the intention
of compiling and running the server as a MUD. It doesn't really matter
if you're going to have 1000 users, or you're just doing this out
of curiosity - you'll have to do the same setup either way. 

Let's start by looking at what's in this archive:


  GENERAL STUFF
  -------------
  README.TXT       - This document
  LICENCE.TXT      - Licence agreement. READ IT!
  
  RUNME            - script to automatically run the MUD under UNIX
                     (run it nice'ed and auto-reboot if it crashes)
  crimson2.bat     -   called by RUNME
  maketar          - script which tars up all the pertinent directories.
                     Usefull for backups or updates.
  doc/*            - helpfull documentation on different parts of C2799

  
  Crimson2799 MUD SERVER COMPONENTS
  ----------------------------------
  These are needed to RUN the server

  crimson2.ini     - Crimson2799 configuration file!!
  c4.ini           - C4 Script language config file (this should have
                     been part of crimson2.ini, but oh well. Unless 
                     you are debugging C4, you probably don't need
                     to change anything in here)
  area/*           - area files. These are the files that describe
                     the "virtual world" that players walk around in.
  lib/*            - supplementary C2799 server files
  help/*           - in-game help files
  msg/*            - messages (eg: initial greeting screen)

  log/*            - C2799 log files
  board/*          - message board files 

  player/*         - player files
  crash/*          - player backup files in case server crashes
  clone/*          - player clone files 
  alias/*          - player aliases
 

  Crimson2799 MUD SERVER SOURCE CODE COMPONENTS
  ---------------------------------------------
  These are needed to COMPILE the server

  src/*            - All the source code (just the single directory)
                     Makefile (for UNIX) is included here.
  util/*           - utility programs usefull for compiling etc. In 
                     the very least you will need to compile the 
                     program "mkdep" from "mkdep.c" in this directory
                     if you are compiling the server under UNIX.

  Microsoft Visual C - specific shit
  
  Crimson2.dsp     - stupid files made by the Winsuck compiler.
  Crimson2.dsw       If you're compiling the MUD with M$ C++ under
  Crimson2.mak       windows, I guess these files are for you. Otherwise, 
                     (ie: in Linux) they are useless.


  Miscellaneous
  -------------
  java/*           - almost-working JAVA Telnet terminal
  mole/*           - Windows-based on-line editing tool. See the file
                     mole/mole_src/readme.txt for more information on 
                     compiling and using MOLE. MOLE also comes with a 
                     mole.hlp help file. mole/mole16.zip and 
                     mole/mole32.zip are precompiled binaries for windows. 
                     I'd suggest you use the 32-bit version - it's got the 
                     colour ansi terminal (way better). I am not sure
                     the source for mole even compiles the 16-bit version
                     any more. 
  wldedit/*        - Windows-based off-line area editing tool. Usefull for
                     doing a rapid layout of an area's world and door objects.
                     wldedit/wldedit.zip contains a pre-compiled Windows binary
                     along with a readme.txt file. wldedit/wlde_src.zip contains
                     the source code for wldedit. 



Ok, the rest of this document will hopefully walk you through the
setup of a running MUD.


Step 1: Extract Crimson2799 archive. 

  You've already done this or you wouldn't be reading this file.
  Congratz! You're doing great so far!

Step 2: Compile the Crimson2799 Server

  The server has been compiled under Linux (lots of versions), 
  SUN Solaris, and Windows NT (console). If you are compiling
  the server under Windows, then you're on your own. I should 
  think you just have to throw all the src/*.c files into a 
  project and hit the "compile" button. If you're compiling 
  the server under Unix/Linux, follow these sub-steps:
 
  Step 2a: Make the mkdep utility.
    In the util/ directory, you'll find a file "mkdep.c". Compile
    it (eg:"cc mkdep.c -o mkdep"). You need this utility when you 
    compile the server.
  Step 2b: Go into src/ directory.
    Everything you need is in this directory (except for things
    like mkdep, gcc, etc.) You will need the standard set of libraries,
    and header files, and C compiling utilities installed on your system.
    I always used GCC under Linux, or cc under Solaris. 
  Step 2c: Edit the "makefile"
    Because this archive was made on a fucked up windows machine, the
    makefile is currently called "MAKEFILE". Rename it to "makefile" 
    - case is important. The makefile has some documentation in it already. 
    You should check through it to make sure everything matches your 
    system requirements. The only thing you should have to change the 
    first time is the CFLAGS and LFLAGS. You will find the flags we used 
    on different distributions of Linux as well as for SUN Solaris. You may 
    need different flags - I duno.
    
    Be carefull you don't change anything below the point marked

    ##########################################################################
    #                   DO NOT EDIT BELOW THIS POINT!!!!                     #
    #   Everything below this point is automatically recreated every time    #
    #                          you call "make dep".                          #
    ##########################################################################

  Step 2d: type "make clean"
    This will delete any object files etc that may be laying around.
  Step 2e: type "make dep"
    This will use the "mkdep" utility (which you compiled in Step 2a above)
    to automatically scan all the .c files and generate dependency information
    for them. This information will be appended to the makefile. 
  Step 2f: type "make"
    This will compile all the *.c files into *.o files, and then link them
    into the file ../crimson2.exe
  Step 2g: if everything compiled and linked, you're done! If you get odd 
    errors, you most likely have to change the CFLAGS or LFLAGS in the 
    makefile. It would seem different systems are fickle about what libraries
    are explicitly or implicitly linked at compile time. :(
    But by in large, you're on your own. Sorry.
  Step 2h: get out of src/ back into the main C2799 directory (go up one
    directory) 

Step 3: Set up your Crypt key.

    Assuming you have your server compiled and ready to go, you can probably
    start it up right away... and you'll get an error saying 
      "define an environment variable CRIMSON2CRYPTKEY"
    You see, the C2799 stores passwords in encrypted form. So under UNIX 
    you need to "setenv CRIMSON2CRYPTKEY <something>" (csh) or 
    "CRIMSON2CRYPTKEY=<something>" and then "export CRIMSON2CRYPTKEY" (bash).
    It doesn't really matter what you set it to, but remember if you ever
    lose your cryptkey, all the passwords for all the player files will 
    NO LONGER WORK!!!! (footnote: you can delete the password for
    players by editing their player file - just erase the password completely)
    We set it to something like "setenv CRIMSON2CRYPTKEY SomeCoolCryptKey".
    Any word(s) will do (no spaces).
    Crimson2799 uses this key to en/decrypt the passwords for the player files.

Step 4: Edit the config files.

    crimson2.ini
      This is the main server initialization file. You can go through it
      and see what it has to offer -- I think much of it is self explanitory.
      In particular you'll want to look at "sockPortNum" - that's the port
      number you'll have to telnet to in order to connect to the mud!!!

      You may also want to look at "sockMaxNum" and "sockMaxOffsite". C2799 
      was originally intended to be a pay mud. Consequently it has two types
      of connections: local and not-local. The idea is you could have local
      (ie: pay) sites, and then allow a limited number of non-local (ie: free)
      connections -- but not too many or your paying customers would start
      to bitch, right? Realistically you probably just want to set these
      both to huge numbers so that anyone who wants to connect will be able
      to, regardless of how busy the MUD is. ;)
      The sites considered "local" are stored in the file lib/sitelist.tbl
      and can either be edited directly (before the server is started) or 
      via the "site" command within the MUD when the server is running.

      the "pStartRoom" setting is the virtual number of the world that newbies
      start in.
   
      Don't use the "areaWriteBinary"... I don't think it was ever debugged.

      You will need to consult the source code to figure out what the rest
      of the settings do, but the defaults should work just fine for now.

    c4.ini
      This is the config file for the C4 script engine. Currently you will
      find just one option in this file The option "codeBootComp" enables
      (1) or disables (0) the scripts to be compiled at boot time. If you 
      are having problems booting (ie the server crashes during compiling) 
      this is a handy way to turn off boot-time compiling of scripts. 
      Refer to the on-line C4 help for more info on C4 scripts. 
     
      I think there are other options that can go into c4.ini, but the defaults
      should be fine. You'll have to refer to the source code (code.c I think)
      to find out what the other options are.

Step 5: Start'er up!

      At your UNIX prompt (or command prompt in Winblows) just run the
      file "crimson2.exe". You may want to look at the messages as they
      stream by; they are also written to the file log/boot.log. Really, 
      all you care about is errors (hopefully there are none). If it 
      successfully boots, it will say "server copacetic" and tell you
      the address and port you can telnet into to play!!! You're done!!!

      almost.

Step 6: Make yourself a GOD character.

      Sign on ("telnet localhost 5005") and create yourself a character. 
      Once you've had a chance to look around, exit out and stop the 
      server. Now go into your file in the player/ directory. If you 
      called yourself "stud" in the game, then your player file will 
      be called "stud.plr". It may also reside in the crash/ directory 
      so check there if it's not in the player directory. 
    
      In your player file, look for the line "Level: 1". Change the "1"
      to "249" or "250". These are the highest-level unrestricted god
      levels. The only difference between them is that when you go 
      "who" in the game, lv 249 characters will have an "ADM" beside
      their name and lv 250 characters will have an "IMP" beside
      their name. Any character level >= 200 is a god and thus any
      character lvl 200 to 248 will have "GOD" beside their name.
 
      You have to create your first GOD character manually by editing
      your player file. After this, you can promote other players
      using the "setstat <player> level <new level>" command in the 
      game.

      Note it won't be much fun if you play as a god kuz gods don't 
      get any experience and don't take any damage ;)

Step 7: Play the game!!!!

      You're done! You're running your very own Crimson2799 MUD!! 
      CONGRATZ!!!!!!!

Step 8: MOLE

      You can edit the area files (area/*) manually. In the least you'll
      have to edit area/area.tbl manually to add new areas. But I'd suggest
      you do everything else either using the built-in commands within the
      game, or using MOLE. 

      MOLE only runs on Windows platforms. I have personally used the
      precompiled versions on Win3.1, Win95, Win98, and WInNT 4.0. 

      I would suggest you use the 32-bit version. 

      MOLE has it's own readme to help you compile it, and it has
      it's own help file. So refer to those for more information.

      BIG NOTE: The help files have their own security levels associated
      with them. So you have to be a GOD (level >= 200) in order to read 
      the help on C4!!!

Step 9: Send us some email!

      After 5 years of development, Ryan and I are pretty burned out
      for MUD stuff. But it would really make our day if you emailed
      us telling us what you thought of Crimson2799. We really poured
      our hearts into it, so if you like it, please consider paying for
      it with a simple email. Include the words "you guys rock" in the
      message body and you'll probably get all the free user support
      you could ask for <ha ha ha ha>


I hope it goes well for you. Good Luck, and happy hunting. 
May the Anomaly, er, well, may the anomaly not kill you ;)

B. Cameron Lesiuk                     Ryan Haksi
wi961@freenet.victoria.bc.ca          cryogen@infoserve.net
