### 
# HOW TO MAKE crimson2.exe
# Step 1) "make clean;make dep"
# Step 2) "make"
####

####
# Just change OFILES and TARGET and you should be done.
# Note that "makefile.tmp" will be created (and then deleted) when you 
# run "make dep". 
####

####
# OFILES is a list of all "*.o" compiled files. Note this script and the
# mkdep utility assume "blah.o" comes from "blah.c", and NOT from "blah.cc"
# or any other forms. 
OFILES = \
	crimson2.o \
	log.o \
	mem.o \
	str.o \
	ini.o \
	extra.o \
	property.o \
	affect.o \
	effect.o \
	object.o \
	board.o \
	char.o \
	mobile.o \
	player.o \
	queue.o \
	timing.o \
	thing.o \
	socket.o \
	site.o \
	alias.o \
	base.o \
	edit.o \
	file.o \
	help.o \
	social.o \
	exit.o \
	index.o \
	world.o \
	reset.o \
	area.o \
	skill.o \
	fight.o \
	parse.o \
	code.o \
        interp.o \
        codestuf.o \
        decomp.o \
        compile.o \
        function.o \
        mole.o \
        mole_msc.o \
        mole_are.o \
        mole_wld.o \
        mole_mob.o \
        mole_obj.o \
	mole_rst.o \
        cmd_code.o \
	cmd_move.o \
	cmd_talk.o \
	cmd_inv.o \
	cmd_cbt.o \
	cmd_misc.o \
	cmd_god.o \
	cmd_wld.o \
	cmd_mob.o \
	cmd_obj.o \
	cmd_help.o \
	cmd_brd.o \
	cmd_area.o \
	cmd_rst.o \
        cmd_mole.o \
	history.o \
	group.o \
	send.o 

####
# TARGET is the final name of your executable. 
TARGET = ../crimson2.exe

###########################################################################
# I don't think you should have to change anything else, except maybe the
# CFLAGS / LFLAGS.
MAKEFILE = Makefile
CC=gcc

#LFLAGS = -L /usr/lib -L /lib -l/lib/libc.a
#LFLAGS = -L ./lib -L /usr/lib -L /lib -lc -lgcc -lsocket -lnsl

## Debug version flags
## Debian Linux:
# CFLAGS = -c -Wall -g -DLINUX
# LFLAGS = 
## RedHat Linux:
CFLAGS = -c -Wall -g
LFLAGS = -lcrypt
#### BSD:
#CFLAGS = -g -Wall -Wno-unused
#### Solaris: (daydream)
#CFLAGS = -g -Wall -DSOLARIS -I ../../include
#LFLAGS = -L ../../lib -lsocket -lnsl


$(TARGET): $(OFILES)
	$(CC) $(LFLAGS) $(OFILES) -o $(TARGET)

dep:
	rm -f Makefile.tmp
	../util/mkdep $(MAKEFILE) $(OFILES) > Makefile.tmp
	mv Makefile.tmp $(MAKEFILE)
	
clean: 
	rm -f $(OFILES) $(TARGET)



##########################################################################
#                   DO NOT EDIT BELOW THIS POINT!!!!                     #
#   Everything below this point is automatically recreated every time    #
#                          you call "make dep".                          #
##########################################################################
#### MARKER - DO NOT EDIT THIS LINE ####
crimson2.o: crimson2.c  crimson2.h macro.h str.h
	$(CC) $(CFLAGS) crimson2.c -o crimson2.o

log.o: log.c  crimson2.h log.h macro.h mem.h queue.h str.h extra.h thing.h index.h edit.h history.h socket.h send.h
	$(CC) $(CFLAGS) log.c -o log.o

mem.o: mem.c  crimson2.h macro.h log.h mem.h str.h
	$(CC) $(CFLAGS) mem.c -o mem.o

str.o: str.c  crimson2.h macro.h log.h mem.h str.h code.h
	$(CC) $(CFLAGS) str.c -o str.o

ini.o: ini.c  crimson2.h log.h str.h
	$(CC) $(CFLAGS) ini.c -o ini.o

extra.o: extra.c  crimson2.h macro.h log.h mem.h str.h extra.h thing.h code.h
	$(CC) $(CFLAGS) extra.c -o extra.o

property.o: property.c  crimson2.h macro.h log.h mem.h str.h extra.h code.h property.h thing.h index.h edit.h history.h socket.h alias.h base.h effect.h object.h char.h mobile.h
	$(CC) $(CFLAGS) property.c -o property.o

affect.o: affect.c  crimson2.h macro.h mem.h str.h log.h file.h queue.h thing.h base.h object.h char.h player.h fight.h affect.h effect.h cmd_inv.h skill.h
	$(CC) $(CFLAGS) affect.c -o affect.o

effect.o: effect.c  crimson2.h macro.h mem.h log.h str.h index.h edit.h queue.h history.h socket.h send.h thing.h parse.h world.h base.h object.h char.h group.h mobile.h player.h skill.h fight.h affect.h cmd_inv.h cmd_move.h effect.h
	$(CC) $(CFLAGS) effect.c -o effect.o

object.o: object.c  crimson2.h macro.h log.h mem.h str.h ini.h send.h extra.h property.h code.h file.h thing.h world.h index.h base.h affect.h effect.h fight.h object.h board.h area.h cmd_inv.h
	$(CC) $(CFLAGS) object.c -o object.o

board.o: board.c  crimson2.h macro.h log.h mem.h str.h extra.h file.h ini.h send.h thing.h index.h edit.h history.h socket.h social.h base.h object.h board.h
	$(CC) $(CFLAGS) board.c -o board.o

char.o: char.c  crimson2.h macro.h queue.h log.h mem.h str.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h socket.h parse.h send.h world.h base.h object.h char.h group.h fight.h affect.h mobile.h player.h skill.h cmd_inv.h
	$(CC) $(CFLAGS) char.c -o char.o

mobile.o: mobile.c  crimson2.h macro.h log.h queue.h mem.h str.h ini.h extra.h property.h code.h file.h thing.h exit.h world.h send.h base.h index.h object.h char.h group.h fight.h player.h affect.h reset.h area.h cmd_move.h cmd_cbt.h mobile.h
	$(CC) $(CFLAGS) mobile.c -o mobile.o

player.o: player.c  crimson2.h macro.h queue.h log.h mem.h str.h ini.h send.h extra.h property.h code.h file.h thing.h index.h world.h area.h reset.h edit.h history.h socket.h base.h object.h char.h affect.h effect.h skill.h player.h parse.h cmd_inv.h cmd_misc.h
	$(CC) $(CFLAGS) player.c -o player.o

queue.o: queue.c  crimson2.h log.h macro.h mem.h queue.h
	$(CC) $(CFLAGS) queue.c -o queue.o

timing.o: timing.c  crimson2.h timing.h
	$(CC) $(CFLAGS) timing.c -o timing.o

thing.o: thing.c  crimson2.h macro.h log.h str.h ini.h queue.h send.h index.h edit.h history.h socket.h extra.h property.h code.h file.h thing.h exit.h world.h area.h reset.h base.h object.h affect.h char.h group.h fight.h player.h skill.h parse.h mobile.h cmd_inv.h cmd_cbt.h cmd_misc.h
	$(CC) $(CFLAGS) thing.c -o thing.o

socket.o: socket.c  crimson2.h macro.h log.h ini.h mem.h queue.h timing.h str.h extra.h code.h thing.h index.h edit.h history.h socket.h site.h alias.h base.h effect.h object.h board.h char.h mobile.h skill.h player.h file.h social.h help.h exit.h world.h reset.h area.h fight.h send.h parse.h cmd_move.h cmd_talk.h
	$(CC) $(CFLAGS) socket.c -o socket.o

site.o: site.c  crimson2.h macro.h log.h ini.h mem.h queue.h str.h thing.h index.h edit.h history.h socket.h base.h char.h mobile.h player.h file.h send.h site.h
	$(CC) $(CFLAGS) site.c -o site.o

alias.o: alias.c  crimson2.h macro.h log.h ini.h mem.h queue.h str.h thing.h index.h edit.h history.h socket.h base.h char.h mobile.h player.h file.h send.h alias.h
	$(CC) $(CFLAGS) alias.c -o alias.o

base.o: base.c  crimson2.h log.h mem.h queue.h str.h extra.h thing.h index.h edit.h history.h socket.h base.h
	$(CC) $(CFLAGS) base.c -o base.o

edit.o: edit.c  crimson2.h macro.h log.h queue.h mem.h str.h extra.h property.h thing.h index.h board.h edit.h history.h socket.h base.h object.h char.h player.h send.h parse.h help.h
	$(CC) $(CFLAGS) edit.c -o edit.o

file.o: file.c  crimson2.h macro.h mem.h str.h log.h file.h
	$(CC) $(CFLAGS) file.c -o file.o

help.o: help.c  crimson2.h macro.h log.h mem.h str.h extra.h file.h ini.h send.h thing.h index.h edit.h history.h socket.h social.h help.h queue.h base.h char.h player.h parse.h
	$(CC) $(CFLAGS) help.c -o help.o

social.o: social.c  crimson2.h macro.h log.h mem.h str.h extra.h file.h ini.h send.h thing.h index.h edit.h history.h socket.h base.h char.h parse.h social.h
	$(CC) $(CFLAGS) social.c -o social.o

exit.o: exit.c  crimson2.h macro.h log.h mem.h str.h index.h extra.h thing.h exit.h world.h
	$(CC) $(CFLAGS) exit.c -o exit.o

index.o: index.c  crimson2.h macro.h log.h mem.h queue.h str.h extra.h thing.h index.h edit.h history.h socket.h base.h object.h char.h mobile.h reset.h area.h
	$(CC) $(CFLAGS) index.c -o index.o

world.o: world.c  crimson2.h macro.h log.h mem.h str.h ini.h extra.h property.h code.h file.h thing.h exit.h index.h world.h area.h
	$(CC) $(CFLAGS) world.c -o world.o

reset.o: reset.c  crimson2.h macro.h log.h mem.h str.h ini.h file.h extra.h property.h code.h thing.h index.h edit.h history.h socket.h exit.h world.h base.h object.h char.h mobile.h area.h reset.h cmd_inv.h
	$(CC) $(CFLAGS) reset.c -o reset.o

area.o: area.c  crimson2.h macro.h mem.h queue.h log.h str.h ini.h extra.h file.h thing.h exit.h index.h world.h edit.h history.h socket.h base.h object.h char.h mobile.h player.h reset.h area.h
	$(CC) $(CFLAGS) area.c -o area.o

skill.o: skill.c  crimson2.h macro.h log.h str.h send.h queue.h thing.h base.h char.h player.h cmd_inv.h skill.h
	$(CC) $(CFLAGS) skill.c -o skill.o

fight.o: fight.c  crimson2.h macro.h log.h mem.h str.h property.h file.h queue.h send.h thing.h index.h edit.h history.h socket.h code.h exit.h world.h base.h char.h group.h mobile.h player.h area.h skill.h object.h affect.h effect.h cmd_move.h cmd_inv.h cmd_cbt.h fight.h group.h
	$(CC) $(CFLAGS) fight.c -o fight.o

parse.o: parse.c  crimson2.h macro.h queue.h log.h str.h ini.h extra.h code.h file.h thing.h exit.h index.h social.h help.h area.h world.h edit.h history.h socket.h alias.h send.h base.h char.h player.h moledefs.h cmd_move.h cmd_talk.h cmd_inv.h cmd_cbt.h cmd_misc.h cmd_god.h cmd_area.h cmd_wld.h cmd_mob.h cmd_obj.h cmd_rst.h cmd_help.h cmd_code.h cmd_brd.h cmd_mole.h parse.h
	$(CC) $(CFLAGS) parse.c -o parse.o

code.o: code.c  crimson2.h macro.h timing.h log.h mem.h str.h ini.h queue.h send.h property.h thing.h index.h edit.h history.h socket.h world.h base.h object.h char.h mobile.h player.h area.h reset.h code.h codestuf.h compile.h interp.h decomp.h function.h exit.h
	$(CC) $(CFLAGS) code.c -o code.o

interp.o: interp.c  crimson2.h macro.h log.h mem.h str.h ini.h send.h extra.h thing.h exit.h index.h edit.h history.h socket.h code.h codestuf.h compile.h interp.h function.h parse.h base.h send.h queue.h
	$(CC) $(CFLAGS) interp.c -o interp.o

codestuf.o: codestuf.c  crimson2.h macro.h log.h mem.h str.h ini.h queue.h send.h extra.h thing.h index.h edit.h history.h socket.h world.h base.h object.h char.h mobile.h player.h area.h codestuf.h compile.h interp.h function.h decomp.h exit.h skill.h fight.h
	$(CC) $(CFLAGS) codestuf.c -o codestuf.o

decomp.o: decomp.c  crimson2.h macro.h log.h mem.h str.h ini.h send.h extra.h thing.h index.h edit.h history.h socket.h code.h codestuf.h interp.h function.h compile.h decomp.h
	$(CC) $(CFLAGS) decomp.c -o decomp.o

compile.o: compile.c  crimson2.h macro.h log.h mem.h str.h ini.h send.h extra.h thing.h index.h code.h codestuf.h interp.h function.h compile.h
	$(CC) $(CFLAGS) compile.c -o compile.o

function.o: function.c  crimson2.h macro.h log.h mem.h str.h ini.h queue.h send.h extra.h property.h thing.h index.h world.h base.h object.h char.h mobile.h player.h area.h code.h codestuf.h compile.h interp.h decomp.h function.h parse.h exit.h skill.h fight.h cmd_move.h
	$(CC) $(CFLAGS) function.c -o function.o

mole.o: mole.c  crimson2.h macro.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h mem.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h area.h moledefs.h mole.h
	$(CC) $(CFLAGS) mole.c -o mole.o

mole_msc.o: mole_msc.c  crimson2.h macro.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h mem.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h area.h reset.h cmd_inv.h moledefs.h mole.h mole_msc.h
	$(CC) $(CFLAGS) mole_msc.c -o mole_msc.o

mole_are.o: mole_are.c  crimson2.h macro.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h mem.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h area.h code.h moledefs.h mole.h mole_msc.h
	$(CC) $(CFLAGS) mole_are.c -o mole_are.o

mole_wld.o: mole_wld.c  crimson2.h macro.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h mem.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h area.h code.h moledefs.h mole.h mole_msc.h
	$(CC) $(CFLAGS) mole_wld.c -o mole_wld.o

mole_mob.o: mole_mob.c  crimson2.h macro.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h mem.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h area.h code.h moledefs.h mole.h mole_msc.h
	$(CC) $(CFLAGS) mole_mob.c -o mole_mob.o

mole_obj.o: mole_obj.c  crimson2.h macro.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h mem.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h area.h code.h moledefs.h mole.h mole_msc.h
	$(CC) $(CFLAGS) mole_obj.c -o mole_obj.o

mole_rst.o: mole_rst.c  crimson2.h macro.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h mem.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h area.h code.h reset.h moledefs.h mole.h mole_msc.h
	$(CC) $(CFLAGS) mole_rst.c -o mole_rst.o

cmd_code.o: cmd_code.c  crimson2.h macro.h queue.h log.h str.h ini.h extra.h property.h file.h thing.h exit.h index.h world.h area.h edit.h history.h socket.h send.h base.h char.h skill.h player.h parse.h cmd_inv.h cmd_move.h cmd_god.h cmd_code.h code.h compile.h interp.h decomp.h function.h codestuf.h
	$(CC) $(CFLAGS) cmd_code.c -o cmd_code.o

cmd_move.o: cmd_move.c  crimson2.h macro.h queue.h log.h str.h ini.h extra.h property.h file.h thing.h exit.h index.h area.h world.h edit.h history.h socket.h send.h base.h object.h char.h affect.h fight.h mobile.h player.h skill.h parse.h code.h cmd_move.h
	$(CC) $(CFLAGS) cmd_move.c -o cmd_move.o

cmd_talk.o: cmd_talk.c  crimson2.h macro.h queue.h log.h str.h ini.h extra.h file.h thing.h index.h edit.h history.h socket.h send.h base.h affect.h char.h player.h parse.h cmd_talk.h
	$(CC) $(CFLAGS) cmd_talk.c -o cmd_talk.o

cmd_inv.o: cmd_inv.c  crimson2.h macro.h queue.h log.h str.h ini.h extra.h property.h file.h thing.h index.h area.h reset.h code.h exit.h world.h edit.h history.h socket.h send.h base.h affect.h effect.h object.h board.h char.h fight.h player.h skill.h parse.h cmd_move.h cmd_inv.h
	$(CC) $(CFLAGS) cmd_inv.c -o cmd_inv.o

cmd_cbt.o: cmd_cbt.c  crimson2.h macro.h log.h str.h property.h queue.h send.h parse.h thing.h exit.h world.h base.h char.h group.h mobile.h player.h affect.h object.h fight.h skill.h effect.h cmd_move.h cmd_cbt.h
	$(CC) $(CFLAGS) cmd_cbt.c -o cmd_cbt.o

cmd_misc.o: cmd_misc.c  crimson2.h macro.h queue.h log.h str.h ini.h extra.h file.h thing.h index.h edit.h history.h socket.h alias.h area.h exit.h world.h send.h base.h object.h char.h mobile.h fight.h affect.h effect.h player.h skill.h parse.h cmd_god.h cmd_misc.h
	$(CC) $(CFLAGS) cmd_misc.c -o cmd_misc.o

cmd_god.o: cmd_god.c  crimson2.h macro.h mem.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h help.h area.h cmd_talk.h cmd_inv.h cmd_move.h cmd_god.h
	$(CC) $(CFLAGS) cmd_god.c -o cmd_god.o

cmd_wld.o: cmd_wld.c  crimson2.h macro.h mem.h queue.h log.h str.h ini.h extra.h property.h code.h file.h thing.h reset.h exit.h index.h world.h area.h edit.h history.h socket.h send.h base.h char.h fight.h player.h parse.h cmd_move.h cmd_wld.h
	$(CC) $(CFLAGS) cmd_wld.c -o cmd_wld.o

cmd_mob.o: cmd_mob.c  crimson2.h macro.h mem.h queue.h log.h str.h ini.h extra.h property.h code.h file.h thing.h reset.h exit.h index.h world.h area.h edit.h history.h socket.h send.h base.h char.h affect.h mobile.h player.h parse.h cmd_wld.h cmd_mob.h
	$(CC) $(CFLAGS) cmd_mob.c -o cmd_mob.o

cmd_obj.o: cmd_obj.c  crimson2.h macro.h mem.h queue.h log.h str.h ini.h extra.h property.h code.h file.h thing.h reset.h exit.h index.h world.h area.h edit.h history.h socket.h send.h base.h object.h char.h player.h parse.h cmd_wld.h cmd_inv.h cmd_obj.h
	$(CC) $(CFLAGS) cmd_obj.c -o cmd_obj.o

cmd_help.o: cmd_help.c  crimson2.h macro.h mem.h log.h queue.h str.h parse.h thing.h index.h edit.h history.h socket.h send.h base.h exit.h world.h help.h cmd_help.h
	$(CC) $(CFLAGS) cmd_help.c -o cmd_help.o

cmd_brd.o: cmd_brd.c  crimson2.h macro.h mem.h log.h queue.h str.h parse.h thing.h index.h edit.h history.h socket.h send.h base.h exit.h world.h char.h player.h board.h cmd_brd.h
	$(CC) $(CFLAGS) cmd_brd.c -o cmd_brd.o

cmd_area.o: cmd_area.c  crimson2.h macro.h mem.h queue.h log.h str.h ini.h extra.h property.h code.h file.h thing.h reset.h exit.h index.h world.h area.h edit.h history.h socket.h send.h base.h object.h char.h player.h parse.h cmd_area.h
	$(CC) $(CFLAGS) cmd_area.c -o cmd_area.o

cmd_rst.o: cmd_rst.c  crimson2.h macro.h mem.h queue.h log.h str.h ini.h extra.h file.h thing.h reset.h exit.h index.h world.h area.h edit.h history.h socket.h send.h base.h object.h char.h mobile.h player.h parse.h cmd_area.h cmd_rst.h
	$(CC) $(CFLAGS) cmd_rst.c -o cmd_rst.o

cmd_mole.o: cmd_mole.c  crimson2.h macro.h log.h str.h queue.h send.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h mem.h socket.h site.h exit.h world.h base.h object.h char.h affect.h fight.h mobile.h skill.h player.h parse.h area.h moledefs.h mole.h cmd_mole.h mole_msc.h mole_are.h mole_wld.h mole_mob.h mole_obj.h mole_rst.h
	$(CC) $(CFLAGS) cmd_mole.c -o cmd_mole.o

history.o: history.c  crimson2.h str.h index.h queue.h edit.h alias.h history.h socket.h send.h
	$(CC) $(CFLAGS) history.c -o history.o

group.o: group.c  crimson2.h macro.h queue.h log.h mem.h str.h ini.h extra.h property.h file.h thing.h index.h edit.h history.h socket.h parse.h send.h world.h base.h object.h char.h fight.h affect.h mobile.h player.h skill.h group.h
	$(CC) $(CFLAGS) group.c -o group.o

send.o: send.c  crimson2.h macro.h queue.h str.h extra.h thing.h index.h edit.h history.h socket.h exit.h parse.h area.h world.h base.h object.h char.h group.h affect.h mobile.h player.h fight.h skill.h file.h send.h
	$(CC) $(CFLAGS) send.c -o send.o

