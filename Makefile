#   XRogue: Expeditions into the Dungeons of Doom
#   Copyright (C) 1991 Robert Pietkivitch
#   All rights reserved.
#
#   Based on "Advanced Rogue"
#   Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
#   All rights reserved.
#
#   Based on "Rogue: Exploring the Dungeons of Doom"
#   Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
#   All rights reserved.
#
#   See the file LICENSE.TXT for full copyright and licensing information.

DISTNAME=xrogue8.0.3
PROGRAM=xrogue

O=o

HDRS  =	rogue.h mach_dep.h network.h

OBJS1 =	vers.$(O) actions.$(O) bolt.$(O) chase.$(O) command.$(O) daemon.$(O) \
        daemons.$(O) eat.$(O) effects.$(O) fight.$(O) encumb.$(O) help.$(O) \
        init.$(O) io.$(O) list.$(O) main.$(O) maze.$(O) misc.$(O) monsters.$(O)
OBJS2 = mons_def.$(O) move.$(O) n_level.$(O) options.$(O) outside.$(O) pack.$(O) \
        passages.$(O) player.$(O) potions.$(O) rings.$(O) rip.$(O) rooms.$(O) \
        save.$(O) scrolls.$(O) sticks.$(O) things.$(O) trader.$(O) util.$(O) \
        weapons.$(O) wear.$(O) wizard.$(O) rogue.$(O) state.$(O) xcrypt.$(O)
OBJS  = $(OBJS1) $(OBJS2)

CFILES=	vers.c actions.c bolt.c chase.c command.c daemon.c daemons.c eat.c \
	effects.c fight.c encumb.c help.c init.c io.c list.c main.c maze.c \
	misc.c monsters.c mons_def.c move.c n_level.c options.c outside.c \
	pack.c passages.c player.c potions.c rings.c rip.c rooms.c save.c \
	scrolls.c sticks.c things.c trader.c util.c weapons.c wear.c wizard.c \
	rogue.c state.c xcrypt.c

MISC  = Makefile README.TXT LICENSE.TXT $(PROGRAM).sln $(PROGRAM).vcproj

CC    = gcc
CFLAGS= -O3
CRLIB = -lcurses
RM    = rm -f
TAR   = tar 
.SUFFIXES: .obj

.c.obj:
	$(CC) $(CFLAGS) /c $*.c

$(PROGRAM): $(HDRS) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(CRLIB) -o $@

clean:
	$(RM) $(OBJS1)
	$(RM) $(OBJS2)
	$(RM) core a.exe a.out a.exe.stackdump $(PROGRAM) $(PROGRAM).exe $(PROGRAM).tar $(PROGRAM).tar.gz $(PROGRAM).zip

dist.src:
	make clean
	tar cf $(DISTNAME)-src.tar $(CFILES) $(HDRS) $(MISC)
	gzip -f $(DISTNAME)-src.tar

dist.irix:
	make clean
	make CC=cc CFLAGS="-woff 1116 -O3" $(PROGRAM)
	tar cf $(DISTNAME)-irix.tar $(PROGRAM) README.TXT LICENSE.TXT
	gzip -f $(DISTNAME)-irix.tar

dist.aix:
	make clean
	make CC=xlc CFLAGS="-qmaxmem=16768 -O3 -qstrict" $(PROGRAM)
	tar cf $(DISTNAME)-aix.tar $(PROGRAM) README.TXT LICENSE.TXT
	gzip -f $(DISTNAME)-aix.tar

dist.linux:
	make clean
	make $(PROGRAM)
	tar cf $(DISTNAME)-linux.tar $(PROGRAM) README.TXT LICENSE.TXT
	gzip -f $(DISTNAME)-linux.tar
	
dist.interix:
	make clean
	make $(PROGRAM)
	tar cf $(DISTNAME)-interix.tar $(PROGRAM) README.TXT LICENSE.TXT
	gzip -f $(DISTNAME)-interix.tar
	
dist.cygwin:
	make clean
	make $(PROGRAM)
	tar cf $(DISTNAME)-cygwin.tar $(PROGRAM).exe README.TXT LICENSE.TXT
	gzip -f $(DISTNAME)-cygwin.tar
	
dist.mingw32:
	$(MAKE) RM="cmd /c del" clean
	$(MAKE) CRLIB="-lpdcurses -lWs2_32" $(PROGRAM)
	cmd /c del $(DISTNAME)-mingw32.zip
	zip $(DISTNAME)-mingw32.zip $(PROGRAM).exe README.TXT LICENSE.TXT
	
dist.msys:
	$(MAKE) clean
	$(MAKE) CRLIB="-lcurses -lWs2_32" $(PROGRAM)
	tar cf $(DISTNAME)-msys.tar $(PROGRAM).exe README.TXT LICENSE.TXT
	gzip -f $(DISTNAME)-msys.tar
	
dist.djgpp:
	make clean
	make LDFLAGS="-L$(DJDIR)/LIB" CRLIB="-lpdcurses" $(PROGRAM)
	rm -f $(DISTNAME)-djgpp.zip
	zip $(DISTNAME)-djgpp.zip $(PROGRAM) README.TXT LICENSE.TXT

dist.win32:
	nmake O="obj" RM="-del" clean
	nmake O="obj" CC="CL" CRLIB="..\pdcurses.lib shell32.lib user32.lib Advapi32.lib Ws2_32.lib" CFLAGS="-DPDC_STATIC_BUILD -nologo -I.. -Ox -wd4033 -wd4716" $(PROGRAM)
	-del $(DISTNAME)-win32.zip
	zip $(DISTNAME)-win32.zip $(PROGRAM).exe README.TXT LICENSE.TXT
