^cNews:
^C=====
^g  
News on forthcoming major changes (ie when I find the time)
-Blob race, shifts shape and can wear items in any position. Each position
active will increase move cost and/or decrease strength. I'm open to feedback
-Going to change practices so you get them as you gain experience, not all at
once as you level.

News as of Feb. 6th
-Grouping system rewritten, much clearer now. Think all is working.
-Stat command now shows leader/follower info, mostly for debugging.
-Grouping works a little differently now, bit more flexible and it handles
group leaders dying better.

News as of Jan. 4th
-ResetSkill, Stat and SetStat all work on offline players now. Randor lemme
know how it goes.

News as of Jan. 3rd
-Where now shows vnum for gods

News as of Jan. 1st
-Infamy gains are in for PKILLing, watch out PK'ers gain enough infamy and the
guards will attack you, and bounty hunters will be dispatched to hunt you down
and kill you.
-Been tweaking the skill table again, too many of the supposedly tuff mobs are
being butchered by (relatively) low-level chars, so the damage skill maxes have 
been revised downwards. 
-Minimum gain while starving is now 1 pt, not 0, so you won't be stuck unable
to move.

News as of Dec. 21st
-Added a @AFTERATTACK event, Psi-attack mobs should be immediately forthcoming
as a result.

News as of Dec. 18th
-Fixed a bug in the startup script compilation code.
-Fixed a bug in the area boottime renumbering code
-Ambush damage/hit bonus only applies to the first attack with multi-attack
weapons. (No more 700 dam. needlers people!)
-Lets hear it for Moby, now working on the very cool sounding BugCamp area.
-Corbin is working on a new sewer area.
-Fixed a bug in the help editing code. (might even work now)
-Players that havent signed on in ages are now auto-turfed by the server. (Reboots should be faster as a result)

News as of Nov. 9th
-Added Dex and Con psi's to the castable list :-)
-Fixed exact matching, you should be able to pr "slow" now.
-Fixed attacking / stop invis problem.
-Fixed obscure bug in spiritwalk

News as of Oct. 15th
-Where screen shows area name for gods now

News as of Sept. 3rd
-Last login / IP section of finger screen not shown if you are not an
administrator or higher. Some people had hacking concerns.

News as of Aug. 6th
-Can now close/open items in your inventory.
-Cant use socials on people/items you cannot see

News as of Aug. 5th
-Armor/Neutronium ring bug fixed, was the new PlayerRead code, fixed affects,
but broke the inventory item stuff. Should be all fixed now.
-Endurance is now working properly
-Reading news before signing in is fixed now.
-Took a stab at fire-damage bug... think I got it. a #define was pointing 
nowhere

News as of July 31st
-Fill container bug is fixed
-SenseLife wasnt working, the source file got overwritten, fixed
-You can see yourself even while hidden or invis.
-Str bug fixed, when you loaded into the game it used to apply every affect
again, twice... whoops.
-If nobody noticed yet, Maze4 has been loaded into the game for awhile.

News as of July 7th
-AFK people will now idle out properly if they have dropped link
-AFK people will disconnect after 6 hours
-Phasedoor psi power added, lets you walk thru closed doors
-Spirits (ala the spiritwalk power) can walk thru doors, see in the dark and
a bunch of other stuff now. (breathwater, breathvacuum etc)
-fixed an obscure bug in the command parse code

News as of June 24th
-SenseLife is coded, allows you to see hidden
-Whoa, cell-repair, Heal-Minor & Heal-Major were really mangled, luckily they
are all fixed now. (and much more effective)
-I reorged all the skills again, get a god to resetskill you, I'll be on 
tomorrow morning, hopefully I can get slizckga & Ratt to re'PR right away
and see what their chars come out looking like. Depending on what their chars
come out like, this could be the last re-org for awhile. (I hope)
-Gratz to Ratt/Light he solved the OreTrans area, (mostly) there's 3 couches
in it to find, and he's only found two.
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


^wNOTE: No, this is ^r*NOT*^w based on the dikuMUD source.!^V
