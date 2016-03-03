COMS	= z180test.com irclet.com intfreq.com vinttest.com

all: $(COMS)

%.com: %.asm Makefile
	sjasm $< $@ || { rm -f $@ ; false; }

test: $(COMS)
	for a in $(COMS) ; do b=`echo $$a|sed 's/\.com$$//'|tr 'a-z' 'A-Z'` ; echo "$$a -> $$b" ; cp $$a /home/lgb/.ep128emu/files/$$b ; mcopy -o $$a m:XEP/$$b ; done

clean:
	rm -f $(COMS) *.lst

.PHONY: test clean all

