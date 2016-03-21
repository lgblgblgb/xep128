# Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
# Copyright (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
# http://xep128.lgb.hu/

include Makefile.common

PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
DATADIR	= $(PREFIX)/lib/xep128
CC	= $(CC_NATIVE)
DEBUG	=
CFLAGS	= -Wall -O3 -ffast-math -pipe $(shell $(SDLCFG_NATIVE) --cflags) $(DEBUG) -DDATADIR=\"$(DATADIR)\" -DZ80EX_USER_HEADER=\"z80ex_config.h\"
ZCFLAGS	= -ansi -fno-common -Wall -pipe -O3 -Iz80ex -I. -DZ80EX_USER_HEADER=\"z80ex_config.h\" $(shell $(SDLCFG_NATIVE) --cflags | cut -f1 -d' ') $(DEBUG) 
CPPFLAGS= -Iz80ex -I.
LDFLAGS	= $(shell $(SDLCFG_NATIVE) --libs) -lm $(DEBUG)
LIBS	=
INCS	= xepem.h z80ex_config.h
SRCS	= $(LINSRCS) $(SRCS_COMMON)
OBJS	= $(SRCS:.c=.o)
PRG	= xep128
PRG_EXE	= xep128.exe
SDIMG	= sdcard.img
SDURL	= http://xep128.lgb.hu/files/sdcard.img
ROM	= combined.rom
DLL	= SDL2.dll
DLLURL	= http://xep128.lgb.hu/files/SDL2.dll
ZIP32	= xep128-win32.zip
ARCH	= native

all:
	@echo "Compiler: $(CC) $(CFLAGS) $(CPPFLAGS)"
	@echo "Linker:   $(CC) $(LDFLAGS) $(LIBS)"
	$(MAKE) $(PRG)

%.o: %.c $(INCS) Makefile Makefile.common
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.s: %.c $(INCS) Makefile Makefile.common
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

z80ex/z180ex-$(ARCH).o: z80ex/z80ex.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -c -o $@ z80ex/z80ex.c
z80ex/z180ex_dasm-$(ARCH).o: z80ex/z80ex_dasm.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -c -o $@ z80ex/z80ex_dasm.c
z80ex/z80ex-$(ARCH).o: z80ex/z80ex.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -c -o $@ z80ex/z80ex.c
z80ex/z80ex_dasm-$(ARCH).o: z80ex/z80ex_dasm.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -c -o $@ z80ex/z80ex_dasm.c

ztest:	z80ex/z180ex-$(ARCH).o z80ex/z180ex_dasm-$(ARCH).o z80ex/z80ex-$(ARCH).o z80ex/z80ex_dasm-$(ARCH).o
	$(MAKE) -f Makefile.win32 ztest
	@echo "**** RESULT:" ; ls -l z80ex/*.o

ui-gtk.o: ui-gtk.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(shell pkg-config --cflags gtk+-3.0) $< -o $@

screen.o: app_icon.c

emu_rom_interface.o: xep_rom_syms.h

roms.o: xep_rom.hex

xep_rom.rom: xep_rom.asm
	sjasm -s xep_rom.asm xep_rom.rom || { rm -f xep_rom.rom xep_rom.lst xep_rom.sym ; false; }

xep_rom_syms.h: xep_rom.sym
	awk '$$1 ~ /xepsym_[^:. ]+:/ { gsub(":$$","",$$1); gsub("h$$","",$$3); print "#define " $$1 " 0x" $$3 }' xep_rom.sym > xep_rom_syms.h

xep_rom.hex: xep_rom.rom
	od -A n -t x1 -v xep_rom.rom | sed 's/ /,0x/g;s/^,/ /;s/$$/,/' > xep_rom.hex

install: $(PRG) $(ROM) $(SDIMG)
	$(MAKE) strip
	mkdir -p $(BINDIR) $(DATADIR)
	cp $(PRG) $(BINDIR)/
	cp $(ROM) $(SDIMG) $(DATADIR)/

buildinfo.c:
	echo "const char *BUILDINFO_ON  = \"`whoami`@`uname -n` on `uname -s` `uname -r`\";" > buildinfo.c
	echo "const char *BUILDINFO_AT  = \"`date -R`\";" >> buildinfo.c
	echo "const char *BUILDINFO_GIT = \"`git show | head -n 1 | cut -f2 -d' '`\";" >> buildinfo.c
	echo "const char *BUILDINFO_CC  = __VERSION__;" >> buildinfo.c

$(DLL):
	@echo "**** Fetching Win32 SDL2 DLL from $(DLLURL) ..."
	wget -O $(DLL) $(DLLURL) || { rm -f $(DLL) ; false; }

$(SDIMG):
	@echo "**** Fetching SDcard image from $(SDURL) ..."
	wget -O $(SDIMG) $(SDURL) || { rm -f $(SDIMG) ; false; }

$(ROM):
	$(MAKE) -C rom

data:	$(SDIMG) $(ROM)
	rm -f buildinfo.c

$(PRG): .depend $(OBJS) z80ex/z180ex-$(ARCH).o z80ex/z180ex_dasm-$(ARCH).o $(INCS) Makefile Makefile.common
	rm -f buildinfo.c
	$(MAKE) buildinfo.o
	$(CC) -o $(PRG) $(OBJS) buildinfo.o z80ex/z180ex-$(ARCH).o z80ex/z180ex_dasm-$(ARCH).o $(LDFLAGS) $(LIBS)

win32:	$(DLL) xep_rom.hex xep_rom_syms.h
	@echo "*** BUILDING FOR WINDOWS ***"
	rm -f buildinfo.c
	$(MAKE) buildinfo.c
	$(MAKE) -f Makefile.win32
	@ls -l $(PRG_EXE)
	@file $(PRG_EXE)
	zip $(ZIP32) $(PRG_EXE) $(ROM) $(DLL) README.md LICENSE
	@ls -l $(ZIP32)

publish:
	test -f rom/$(ROM) && cp rom/$(ROM) www/files/ || true
	test -f $(ZIP32) && cp $(ZIP32) www/files/ || true
	@ls -l www/files/

strip:	$(PRG)
	strip $(PRG)

zclean:
	rm -f z80ex/*.o

clean:
	rm -f $(OBJS) buildinfo.c buildinfo.o print.out xep_rom.hex xep_rom.lst xep_rom_syms.h .depend
	$(MAKE) -C rom clean

distclean:
	$(MAKE) clean
	$(MAKE) -C rom distclean
	$(MAKE) zclean
	rm -f $(SDIMG) $(DLL) $(ROM) $(PRG) $(PRG_EXE) $(ZIP32)

help:
	$(MAKE) $(PRG)
	./$(PRG) -help | grep -Ev '^(PATH:|Platform:|GIT|LICENSE:) ' > doc/help-cli.txt
	./$(PRG) -testparsing -config none | sed -e '1,/^--- /d' -e '/^--- /,$$d' > doc/help-config.txt

commit:
	git diff
	git status
	EDITOR="vim -c 'startinsert'" git commit -a
	git push

.depend:
	$(MAKE) depend

dep:
	$(MAKE) depend

depend:
	$(MAKE) xep_rom.hex
	$(MAKE) xep_rom_syms.h
	$(CC) -MM $(CFLAGS) $(CPPFLAGS) $(SRCS) > .depend

.PHONY: all clean distclean strip commit win32 publish data install ztest zclean dep depend

ifneq ($(wildcard .depend),)
include .depend
endif

