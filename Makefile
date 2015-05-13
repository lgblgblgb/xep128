CC	= gcc
DEBUG	=
CFLAGS	= -Wall -O3 -ffast-math $(shell sdl2-config --cflags) $(DEBUG)
CPPFLAGS= -Iz80ex/include 
LDFLAGS	= $(shell sdl2-config --libs) $(DEBUG)
#LIBS	= -lz80ex -lz80ex_dasm
LIBS	= $(Z80EX) z80ex/lib/libz80ex_dasm.a
#LIBS	= -Wl,-Bstatic -lz80ex -lz80ex_dasm -Wl,-Bdynamic

INCS	= xepem.h
SRCS	= main.c cpu.c nick.c dave.c input.c exdos-wd.c sdext.c rtc.c
OBJS	= $(SRCS:.c=.o)
PRG	= xepem
Z80EX	= z80ex/lib/libz80ex.a
SDIMG	= sdcard.img
SDURL	= http://xep128.lgb.hu/files/sdcard.img
ROM	= combined.rom
ROMURL	= http://xep128.lgb.hu/files/combined.rom

all:
	@echo "Compiler: $(CC) $(CFLAGS) $(CPPFLAGS)"
	@echo "Linker:   $(CC) $(LDFLAGS) $(LIBS)"
	$(MAKE) $(PRG)

%.o: %.c $(INCS) Makefile
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.s: %.c $(INCS) Makefile
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

$(SDIMG):
	@echo "**** Fetching SDcard image from $(SDURL) ..."
	wget -O $(SDIMG) $(SDURL)

$(ROM):
	@echo "**** Fetching ROM image from $(ROMURL) ..."
	wget -O $(ROM) $(ROMURL)

$(Z80EX):
	$(MAKE) -C z80ex

$(PRG): $(OBJS) $(INCS) Makefile $(Z80EX) $(SDIMG) $(ROM)
	$(CC) -o $(PRG) $(OBJS) $(LDFLAGS) $(LIBS)

sdl:	sdl.o
	$(CC) -o sdl sdl.o $(LDFLAGS) $(LIBS)

sdl2:	sdl2.o
	$(CC) -o sdl2 sdl2.o $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) $(PRG)
	$(MAKE) -C z80ex clean

distclean:
	$(MAKE) clean
	rm -f $(SDIMG) $(ROM)

.PHONY: all clean distclean

