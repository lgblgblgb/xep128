CC	= gcc
DEBUG	=
CFLAGS	= -Wall -O3 -ffast-math -pipe $(shell sdl2-config --cflags) $(DEBUG)
ZCFLAGS	= -fno-common -ansi -pedantic -Wall -pipe -O3 -Iz80ex -Iz80ex/include -DWORDS_LITTLE_ENDIAN -DZ80EX_VERSION_STR=1.1.21 -DZ80EX_API_REVISION=1 -DZ80EX_VERSION_MAJOR=1 -DZ80EX_VERSION_MINOR=21 $(DEBUG) -DZ80EX_RELEASE_TYPE=
CPPFLAGS= -Iz80ex/include -I.
LDFLAGS	= $(shell sdl2-config --libs) -lSDL2_net $(DEBUG)
LIBS	=
#LIBS	= -lz80ex -lz80ex_dasm
#LIBS	= $(Z80EX) z80ex/lib/libz80ex_dasm.a
#LIBS	= -Wl,-Bstatic -lz80ex -lz80ex_dasm -Wl,-Bdynamic

INCS	= xepem.h
SRCS	= main.c cpu.c nick.c dave.c input.c exdos-wd.c sdext.c rtc.c printer.c zxemu.c emu_rom_interface.c
OBJS	= $(SRCS:.c=.o)
PRG	= xep128
PRG_EXE	= xep128.exe
SDIMG	= sdcard.img
SDURL	= http://xep128.lgb.hu/files/sdcard.img
ROM	= combined.rom
DLL	= SDL2.dll
DLLURL	= http://xep128.lgb.hu/files/SDL2.dll
ZIP32	= xep128-win32.zip

all:
	@echo "Compiler: $(CC) $(CFLAGS) $(CPPFLAGS)"
	@echo "Linker:   $(CC) $(LDFLAGS) $(LIBS)"
	$(MAKE) $(PRG)

%.o: %.c $(INCS) Makefile
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.s: %.c $(INCS) Makefile
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

z80ex.o: z80ex/z80ex.c
	$(CC) $(ZCFLAGS) -c -o z80ex.o z80ex/z80ex.c
z80ex_dasm.o: z80ex/z80ex_dasm.c
	$(CC) $(ZCFLAGS) -c -o z80ex_dasm.o z80ex/z80ex_dasm.c

$(DLL):
	@echo "**** Fetching Win32 SDL2 DLL from $(DLLURL) ..."
	wget -O $(DLL) $(DLLURL) || { rm -f $(DLL) ; false; }

$(SDIMG):
	@echo "**** Fetching SDcard image from $(SDURL) ..."
	wget -O $(SDIMG) $(SDURL) || { rm -f $(SDIMG) ; false; }

$(ROM):
	$(MAKE) -C rom
	cp rom/$(ROM) .


$(PRG): $(OBJS) z80ex.o z80ex_dasm.o $(INCS) Makefile $(SDIMG) $(ROM)
	$(CC) -o $(PRG) $(OBJS) z80ex.o z80ex_dasm.o $(LDFLAGS) $(LIBS)

win32:	$(DLL) $(SDIMG) $(ROM)
	@echo "*** BUILDING FOR WINDOWS ***"
	$(MAKE) -f Makefile.win32
	@ls -l $(PRG_EXE)
	@file $(PRG_EXE)

$(ZIP32): $(PRG_EXE) $(ROM) $(DLL) README.md LICENSE
	$(MAKE) win32
	zip $(ZIP32) $(PRG_EXE) $(ROM) $(DLL) README.md LICENSE

strip:	$(PRG)
	strip $(PRG)

sdl:	sdl.o
	$(CC) -o sdl sdl.o $(LDFLAGS) $(LIBS)

sdl2:	sdl2.o
	$(CC) -o sdl2 sdl2.o $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) $(PRG) $(PRG_EXE) $(ZIP32) $(ROM) print.out z80ex.o z80ex_dasm.o
	$(MAKE) -C z80ex clean
	$(MAKE) -C rom clean

distclean:
	$(MAKE) clean
	$(MAKE) -C rom distclean
	rm -f $(SDIMG) $(DLL)

commit:
	git diff
	git status
	EDITOR="vim -c 'startinsert'" git commit -a
	git push

.PHONY: all clean distclean strip commit win32

