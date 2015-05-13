CC	= gcc
CFLAGS	= -Wall -O3 -ffast-math $(shell sdl2-config --cflags) -g
CPPFLAGS= 
LDFLAGS	= $(shell sdl2-config --libs) -g
#LIBS	= -lz80ex -lz80ex_dasm
LIBS	= $(Z80EX) z80ex/lib/libz80ex_dasm.a
#LIBS	= -Wl,-Bstatic -lz80ex -lz80ex_dasm -Wl,-Bdynamic

INCS	= xepem.h
SRCS	= main.c cpu.c nick.c dave.c input.c exdos-wd.c sdext.c rtc.c
OBJS	= $(SRCS:.c=.o)
PRG	= xepem
Z80EX	= z80ex/lib/libz80ex.a

all: $(PRG)

%.o: %.c $(INCS) Makefile
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.s: %.c $(INCS) Makefile
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

$(Z80EX):
	$(MAKE) -C z80ex

$(PRG): $(OBJS) $(INCS) Makefile $(Z80EX)
	$(CC) -o $(PRG) $(OBJS) $(LDFLAGS) $(LIBS)

sdl:	sdl.o
	$(CC) -o sdl sdl.o $(LDFLAGS) $(LIBS)

sdl2:	sdl2.o
	$(CC) -o sdl2 sdl2.o $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) $(PRG)
	$(MAKE) -C z80ex clean

.PHONY: all clean

