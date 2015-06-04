CC	= gcc
DEBUG	=
CFLAGS	= -Wall -O3 -ffast-math -pipe $(shell sdl2-config --cflags) $(DEBUG)
CPPFLAGS= -Iz80ex/include -I.
LDFLAGS	= $(shell sdl2-config --libs) -lSDL2_net $(DEBUG)
#LIBS	= -lz80ex -lz80ex_dasm
LIBS	= $(Z80EX) z80ex/lib/libz80ex_dasm.a
#LIBS	= -Wl,-Bstatic -lz80ex -lz80ex_dasm -Wl,-Bdynamic

INCS	= xepem.h
SRCS	= main.c cpu.c nick.c dave.c input.c exdos-wd.c sdext.c rtc.c printer.c zxemu.c emu_rom_interface.c
OBJS	= $(SRCS:.c=.o)
PRG	= xep128
PRG_EXE	= xep128.exe
Z80EX	= z80ex/lib/libz80ex.a
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

$(DLL):
	@echo "**** Fetching Win32 SDL2 DLL from $(DLLURL) ..."
	wget -O $(DLL) $(DLLURL) || { rm -f $(DLL) ; false; }

$(SDIMG):
	@echo "**** Fetching SDcard image from $(SDURL) ..."
	wget -O $(SDIMG) $(SDURL) || { rm -f $(SDIMG) ; false; }

$(ROM):
	$(MAKE) -C rom
	cp rom/$(ROM) .

$(Z80EX):
	$(MAKE) -C z80ex static

$(PRG): $(OBJS) $(INCS) Makefile $(Z80EX) $(SDIMG) $(ROM)
	$(CC) -o $(PRG) $(OBJS) $(LDFLAGS) $(LIBS)

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
	rm -f $(OBJS) $(PRG) $(PRG_EXE) $(ZIP32) $(ROM) print.out
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

