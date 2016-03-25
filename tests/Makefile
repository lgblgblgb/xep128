include ../Makefile.common

COMS	= z180test.com irclet.com intfreq.com vinttest.com
ELFS	= z80-emulated-cpu-speed-test
CFLAGS	= -Wall -O3 -ffast-math -pipe -DZ80EX_USER_HEADER=\"../z80ex_config.h\" $(shell $(SDLCFG_NATIVE) --cflags)
ARCH	= native
LDFLAGS	= ../z80ex/z180ex-$(ARCH).o

all: $(COMS) $(ELFS)

%.com: %.asm Makefile
	sjasm $< $@ || { rm -f $@ ; false; }

test: $(COMS)
	mcopy -i ../sdcard.img@@32256 -o $(COMS) ::/xep/
	cp $(COMS) /home/lgb/.ep128emu/files/

clean:
	rm -f $(COMS) *.lst $(ELFS)

.PHONY: test clean all

