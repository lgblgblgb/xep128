include ../arch/Makefile.native

COMS	= z180test.com irclet.com intfreq.com vinttest.com exos10ts.com
ELFS	= z80-emulated-cpu-speed-test cpm-minimal-emulator
CFLAGS	= -Wall -O3 -ffast-math -pipe -DZ80EX_USER_HEADER=\"../z80ex_config.h\" $(shell $(SDLCFG) --cflags)
ARCH	= native
LDFLAGS	= ../z80ex/z180ex-$(ARCH).o

all: $(COMS) $(ELFS)

%.com: %.asm Makefile
	sjasm $< $@ || { rm -f $@ ; false; }

win32:
	rm -f cpm-minimal-emulator.exe
	i686-w64-mingw32-gcc -Wall -O3 -ffast-math -pipe -DZ80EX_USER_HEADER=\"../z80ex_config.h\" -I/usr/local/cross-tools/i686-w64-mingw32/include/SDL2 -mconsole -o cpm-minimal-emulator.exe cpm-minimal-emulator.c ../z80ex/z180ex-win32.o
	i686-w64-mingw32-strip cpm-minimal-emulator.exe
	ls -l cpm-minimal-emulator.exe

test: $(COMS)
	mcopy -i /home/lgb/.local/share/nemesys.lgb/xep128/sdcard.img@@32256 -o $(COMS) ::/xep/
	cp $(COMS) /home/lgb/.ep128emu/files/
	cp $(COMS) /home/lgb/.local/share/nemesys.lgb/xep128/files/

clean:
	rm -f $(COMS) *.lst $(ELFS)

.PHONY: test clean all

