include ../arch/Makefile.native

COMS	= z180test.com irclet.com intfreq.com vinttest.com
ELFS	= z80-emulated-cpu-speed-test
CFLAGS	= -Wall -O3 -ffast-math -pipe -DZ80EX_USER_HEADER=\"../z80ex_config.h\" $(shell $(SDLCFG) --cflags)
ARCH	= native
LDFLAGS	= ../z80ex/z180ex-$(ARCH).o

all: $(COMS) $(ELFS)

%.com: %.asm Makefile
	sjasm $< $@ || { rm -f $@ ; false; }

test: $(COMS)
	mcopy -i /home/lgb/.local/share/nemesys.lgb/xep128/sdcard.img@@32256 -o $(COMS) ::/xep/
	cp $(COMS) /home/lgb/.ep128emu/files/
	cp $(COMS) /home/lgb/.local/share/nemesys.lgb/xep128/files/

clean:
	rm -f $(COMS) *.lst $(ELFS)

.PHONY: test clean all

