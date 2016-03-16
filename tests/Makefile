COMS	= z180test.com irclet.com intfreq.com vinttest.com

all: $(COMS)

%.com: %.asm Makefile
	sjasm $< $@ || { rm -f $@ ; false; }

test: $(COMS)
	mcopy -i ../sdcard.img@@32256 -o $(COMS) ::/xep/
	cp $(COMS) /home/lgb/.ep128emu/files/

clean:
	rm -f $(COMS) *.lst

.PHONY: test clean all

