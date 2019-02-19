^cNews:
^C=====
^g  
We might still be getting the bugs out but tell all your friends to play here
anyway! I'm not kidding, , please recruit other mud friends, this can be a
really cool mud if we can just get some players on here!

News on forthcoming changes (ie when I find the time)
-Blob race, shifts shape and can wear items in any position. Each position
active will increase move cost and/or decrease strength. I'm open to feedback
-Going to change practices so you get them as you gain experience, not all at
once as you level.

News as of June 24th
-SenseLife is coded, allows you to see hidden
-Whoa, cell-repair, Heal-Minor & Heal-Major were really mangled, luckily they
are all fixed now. (and much more effective)
-I reorged all the skills again, get a god to resetskill you, I'll be on 
tomorrow morning, hopefully I can get slizckga & Ratt to re'PR right away
and see what their chars come out looking like. Depending on what their chars
come out like, this could be the last re-org for awhile. (I hope)
-Gratz to Ratt/Light he solved the OreTrans area, (mostly) there's 3 couches
in it to find, and he's only found tow.
-Added Captain mob into his cabin in the oretrans area.

News as of June 17th
-Fixes to ThingCanSee, so you can see equipped invis items, and also carried
hidden items. (Sorry I took so long Gauntlet)
-Fix so that weapons dont use up ammo while equipped.

News as of June 12th
-SkillMax command added check what skills you'll get at whatever level you
want to know about.
-ResetSkill command to let gods blank a char & get their pracs back
-Skill table massively edited, much easier to practice now, get a god to reset
you.

News as of June 11th
-Various bugs in maze3 fixed
-psi items will wink out when you die not be left on your corpse
-weapon skills are easier to practice
-title $N bug is fixed
-group xp now increased to 1.5X base
-Salamanders are now now spirit:bad, not spirit:cant
-some other stuff, which I cant remember at the moment

News as of June 9th
-Laggy cpu-load problem maybe fixed. Creature movement is now WAY ore efficient
(hopefully that should take care of it)
-Admin/Imps can now ares all, areb all etc, Areb all should fix any lag probs
because it will cut down on the # of mobs currently loaded into the game.
-Umm some other stuff but I forget now.

News as of June 7th
-Holy massive-mud-updates-batman! Corbin and I got together this past weekend
and between sessions stripping the tread off the wheels of the 'stang managed 
to make some progress on the mud.
-New version of MOLE released, bugs are fixed, resets are in, security is in 
and it will now work for editors who arent IMP's.. hey Sliz this means you!
-Lots of the areas have been refined slightly. (Doors should reset properly
on the station, and you cant get stuck in maze2 anymore etc)
-The OreTransport level is open for business, as is Maze3, both mostly by
yours truly, Cryogen.
-NOTE: Corbin did most of the scripting in OreTransport and came up with the maze
and Maze concept for Maze3... ouch this one will make your brain hurt
-The other races embassies have been added on Anomaly Station, not much there
yet but more will be added soon.
-Getting invisible object bug is fixed.
-Many expansions to the C4 language. (StrIn and StrIsCmd are now much cooler!)
-Fame and Infamy have been implemented. Beware pickpockets and pkillers -
Infamy is a very bad thing.
-You can solve SilverDale now and it will eventually wear off.
-Mobs equipping items bug has been fixed.
-Many more things that I can never remember when I'm banging out these news
updates.

News as of June 4th
-Players will now respawn by the eclipse shuttle if they die before brain
dumping
-Tweaked board logic, hopefully @#$@#$ BRD channel is working properly now

News as of June 2nd
-Bug in the new board beeping logic, hopefully it should be fixed now

News as of May 28th
-Ratt uncovered a infinite loop bug in the follow code, fixed. Not sure what
I was doing. If anyone has thoughts on how grouping should work, lemme know.

News as of May27th
-Mud moved to its new home DEDICATED machine.
-Sneak command added
-Boards now beep when someone finishes writing a message on them
-Level gain message xp to level thing should be fixed
-FList command added for area builders
-Board channel added (for gods) to monitor all the boards
-ChHistory now supports searches for messages from a specific person
-ChHistory now allows you to specify the # of lines to show you (up to 100)
-Colorizing of ChHistory scrollback buffer is fixed
-Some other stuff I'm drawing a blank on

News as of May 19th
-FINALLY! I think the OS upgrade/network changes are straightened out! (yay)
-Had to rewrite logging module cuz it broke on the new directory mounting
setup. Something to do with non-blocking IO and symbolic links, arg.
-Unlimited length say logic + formatting extended to cover gossip/channel
commands, tell etc.
-: now works the same as ' so you dont need a space after it
-Added a message so you know when equip survives a deathtrap, doh really should
have done this a long time ago, Thanx Mer.

News as of May 6th
-More tweaks to autoassist/rescue code
-Fixes to long say/tell logic, it was breaking the : and ' shortcuts and also
aliases. All should be right in the world now though....

News as of May 5th
-Mobs will no longer assist non-grouped followers (thanx Xarkoff)
-Group will no longer show stats on non-group members

News as of April 30th
-New prompt string support for arbitrary prompts, check out the prompt command
-Finger command shows last login machine
-Autoeat/drink recurse through all containers properly now
-entermsg/exitmsg commands will show current msg if used with no args

News as of April 27th
-Fixed ANOTHER bug in say formatting logic, pesky critters, but I think I got
'em all now.

News as of April 20th
-Fixed a bug in the new say formatting logic. Assuming this does the trick I'll
adapt the code to gossip, gtell, tell etc in a few days.

News as of April 16th
-Say now formats speech output to be 79 columns or so. Try it you'll like it.
-There is now no limit on how long a "say" command can be, you are free to 
write a book if you like. (Randor asks and Randor receives)
-If someone attempts to seize your link you will get to see their IP address
-You have two chances to logon now before the server drops link
-Buncha new SYS logging. (dropped connects etc)
-Bug in QRead fixed. (Ouch, it allowed stack corruption - how DO these things
get by so long without being discovered)
-Other stuff which I'm forgetting at the moment.
-The mud is now permanently aliased to crimson.imaginary.com, thanx Randor!

News as of Mar. 30th
-Items can use ammo while equipped (ie the space suit should use of oxygen 
cannisters).
-Some types of armor now require ammo. (ie power armor is now possible)

News as of Mar. 25th
-respond command added, sends a message to whomever last did a tell at ya
-finger/email privs fixed for Randor. Sorry about that...

News as of Mar. 24th
-junk all bread will no longer be interpreted as junk all
-grouping code has been debugged AGAIN, maybe fixed now.
-look with autolook turned off will let you see room desc now
-output from "group" command slightly tweaked
-enter now synonymous with go, so if you type enter south it will work
-default msgs's put in for title,entermsg & exitmsg.

News as of Mar.8th
-Set plan/email etc works again (finally)
-Tickcounter on while AFK will warn you that you do not heal while you are AFK.
Sorry that was a bit confusing Aco.
-Group code seems slightly buggy still, I put in a quick guard against what was
crashing the server, that should help until I track downt he real problem.
-Sorry about the lack of progress I'm working on a game for the voodooextreme
web contest. (http://www.voodooextreme.com). Entry date is Mar.16th so wont be
long now until I put in a few more changes here.

News as of Feb.16th
-ChHistory revised, now supports gossip,auction & godtalk. More to come.
-ChHistory now keeps as many lines of personal history as your screensize, and
50 lines of gossip etc.
-Bug in the wholist fixed, should show idle properly now(again)
-Beep command added, sends an BELL sound, if they have private turned off it will be blocked.

News as of Feb.16th
-ChHistory command added, see the last page or so of tells and says. I'll
be enhancing this further later to show gossips etc. as well.
-!<str> now matches the last command rather than the first command
-History buffer now increased to 2K in size


^wNOTE: No, this is ^r*NOT*^w based on the dikuMUD source.!^V
