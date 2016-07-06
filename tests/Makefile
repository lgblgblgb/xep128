include ../arch/Makefile.native

COMS	= z180test.com irclet.com intfreq.com vinttest.com exos10ts.com
ELFS	= z80-emulated-cpu-speed-test
CFLAGS	= -Wall -Ofast -ffast-math -pipe $(shell $(SDLCFG) --cflags)
ARCH	= native
LDFLAGS	=

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

