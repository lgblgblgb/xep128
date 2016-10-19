ROM		= combined.rom
URL_TEMPLATE	= https://raw.githubusercontent.com/lgblgblgb/xep128/gh-pages/files/roms/
ROM_FILES	= EXOS24UK-NOILLOPS.rom sd-cart-0.3-FTU-57k.rom filler-8k.rom zx-emu-40.rom EPDOS-1.9-beta.rom primo-b64.rom

all: $(ROM)

%.rom:
	rm -f $@.gz*
	wget -O $@.gz $(URL_TEMPLATE)$@.gz || { rm -f $@.gz ; false; }
	rm -f $@
	gunzip $@.gz

$(ROM): Makefile $(ROM_FILES)
	cat $(ROM_FILES) > $@
	cp $@ ../

clean:
	rm -f $(ROM)

distclean:
	$(MAKE) clean
	rm -f $(ROM_PACK) *.rom *.rom.gz $(ROM_FILES)
