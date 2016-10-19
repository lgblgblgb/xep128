ROM		= combined.rom
URL_TEMPLATE	= https://raw.githubusercontent.com/lgblgblgb/xep128/gh-pages/files/roms/
ROM_LIST_REMOTE	= README
ROM_LIST_LOCAL	= rom-listing.txt
#ROM_FILES	= EXOS24UK-NOILLOPS.rom sd-cart-0.3-FTU-57k.rom filler-8k.rom zx-emu-40.rom EPDOS-1.9-beta.rom primo-b64.rom
ROM_FILES	= EXOS24UK-NOILLOPS.rom EXDOS14ISDOS10UK-1770.rom filler-16k.rom SDEXT05-16k.rom zx41uk.rom EPDOS-1.9-beta.rom primo-b64.rom

all: $(ROM)

%.rom:
	rm -f $@.gz*
	wget -O $@.gz $(URL_TEMPLATE)$@.gz || { rm -f $@.gz ; false; }
	rm -f $@
	gunzip $@.gz

$(ROM_LIST_LOCAL):
	rm -f $(ROM_LIST_LOCAL)
	wget -O $@ $(URL_TEMPLATE)$(ROM_LIST_REMOTE) || { rm -f $@ ; false; }

$(ROM): Makefile $(ROM_FILES) $(ROM_LIST_LOCAL)
	cat $(ROM_FILES) > $@
	cp $@ ../

clean:
	rm -f $(ROM)

distclean:
	$(MAKE) clean
	rm -f $(ROM_PACK) *.rom *.rom.gz $(ROM_FILES) $(ROM_LIST_LOCAL)
