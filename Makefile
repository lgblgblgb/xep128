PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
DATADIR	= $(PREFIX)/lib/xep128
CC	= gcc
DEBUG	=
CFLAGS	= -Wall -O3 -ffast-math -pipe $(shell sdl2-config --cflags) $(DEBUG) -DDATADIR=\"$(DATADIR)\"
ZCFLAGS	= -fno-common -ansi -pedantic -Wall -pipe -O3 -Iz80ex -Iz80ex/include -DWORDS_LITTLE_ENDIAN -DZ80EX_VERSION_STR=1.1.21 -DZ80EX_API_REVISION=1 -DZ80EX_VERSION_MAJOR=1 -DZ80EX_VERSION_MINOR=21 $(DEBUG) -DZ80EX_ED_TRAPPING_SUPPORT -DZ80EX_RELEASE_TYPE=
CPPFLAGS= -Iz80ex/include -I.
LDFLAGS	= $(shell sdl2-config --libs) $(DEBUG)
LIBS	=
#LIBS	= -lz80ex -lz80ex_dasm
#LIBS	= $(Z80EX) z80ex/lib/libz80ex_dasm.a
#LIBS	= -Wl,-Bstatic -lz80ex -lz80ex_dasm -Wl,-Bdynamic

INCS	= xepem.h
SRCS	= main.c cpu.c cpu_z180.c nick.c dave.c input.c exdos_wd.c sdext.c rtc.c printer.c zxemu.c emu_rom_interface.c w5300.c
OBJS	= $(SRCS:.c=.o)
Z80EX	= z80ex.o z80ex_dasm.o
PRG	= xep128
PRG_EXE	= xep128.exe
SDIMG	= sdcard.img
SDURL	= http://xep128.lgb.hu/files/sdcard.img
ROM	= combined.rom
DLL	= SDL2.dll
DLLURL	= http://xep128.lgb.hu/files/SDL2.dll
ZIP32	= xep128-win32.zip
ZDEPS	= z80ex/ptables.c z80ex/z80ex.c z80ex/typedefs.h z80ex/opcodes/opcodes_ed.c z80ex/opcodes/opcodes_fdcb.c z80ex/opcodes/opcodes_base.c z80ex/opcodes/opcodes_fd.c z80ex/opcodes/opcodes_cb.c z80ex/opcodes/opcodes_ddcb.c z80ex/opcodes/opcodes_dd.c z80ex/opcodes/opcodes_dasm.c z80ex/z80ex_dasm.c z80ex/macros.h z80ex/include/z80ex_common.h z80ex/include/z80ex_dasm.h z80ex/include/z80ex.h z80ex/z180ex.c

all:
	@echo "Compiler: $(CC) $(CFLAGS) $(CPPFLAGS)"
	@echo "Linker:   $(CC) $(LDFLAGS) $(LIBS)"
	$(MAKE) $(PRG)

%.o: %.c $(INCS) Makefile
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.s: %.c $(INCS) Makefile
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

z80ex.o: z80ex/z80ex.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -DZ80EX_Z180_SUPPORT -c -o $@ z80ex/z80ex.c
z80ex_dasm.o: z80ex/z80ex_dasm.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -DZ80EX_Z180_SUPPORT -c -o $@ z80ex/z80ex_dasm.c

ui-gtk.o: ui-gtk.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(shell pkg-config --cflags gtk+-3.0) $< -o $@

main.o: xep_rom.hex app_icon.c

xep_rom.rom: xep_rom.asm
	sjasm xep_rom.asm xep_rom.rom || { rm -f xep_rom.rom ; false; }

xep_rom.hex: xep_rom.rom
	od -A n -t x1 -v xep_rom.rom | sed 's/ /,0x/g;s/^,/ /;s/$$/,/' > xep_rom.hex

install: $(PRG) $(ROM) $(SDIMG)
	mkdir -p $(BINDIR) $(DATADIR)
	cp $(PRG) $(BINDIR)/
	cp $(ROM) $(SDIMG) $(DATADIR)/

buildinfo.c:
	echo "const char *BUILDINFO_ON  = \"`whoami`@`uname -n` on `uname -s` `uname -r`\";" > buildinfo.c
	echo "const char *BUILDINFO_AT  = \"`date -R`\";" >> buildinfo.c
	echo "const char *BUILDINFO_GIT = \"`git show | head -n 1 | cut -f2 -d' '`\";" >> buildinfo.c

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

$(PRG): $(OBJS) $(Z80EX) $(INCS) Makefile
	rm -f buildinfo.c
	$(MAKE) buildinfo.o
	$(CC) -o $(PRG) $(OBJS) buildinfo.o $(Z80EX) $(LDFLAGS) $(LIBS)

win32:	$(DLL) xep_rom.hex
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

clean:
	rm -f $(OBJS) $(Z80EX) $(PRG) $(PRG_EXE) $(ZIP32) buildinfo.c buildinfo.o print.out xep_rom.hex xep_rom.lst
	$(MAKE) -C rom clean

distclean:
	$(MAKE) clean
	$(MAKE) -C rom distclean
	rm -f $(SDIMG) $(DLL) $(ROM)

commit:
	git diff
	git status
	EDITOR="vim -c 'startinsert'" git commit -a
	git push

.PHONY: all clean distclean strip commit win32 publish data install

