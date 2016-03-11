ROM		= combined.rom
ROM_PACK	= rom_pack.zip
ROM_PACK_URL	= http://xep128.lgb.hu/files/$(ROM_PACK)
#ROM_PACK_FILES	= EXOS24UK-NOILLOPS.rom sdext-cart-64k.rom EPDOS-Z.rom ZT18UK-emuhoz.rom zx-emu-40.rom
#ROM_PACK_FILES	= EXOS24UK-NOILLOPS.rom sd-cart-0.3-FTU-57k.rom filler-8k.rom zx-emu-40.rom ZT18UK-emuhoz.rom EPDOS-1.9-beta.rom
ROM_PACK_FILES	= EXOS24UK-NOILLOPS.rom sd-cart-0.3-FTU-57k.rom filler-8k.rom zx-emu-40.rom EPDOS-1.9-beta.rom primo-b64.rom

all: $(ROM)

$(ROM_PACK):
	wget -O $(ROM_PACK) $(ROM_PACK_URL) || { rm -f $(ROM_PACK) ; false; }
	unzip $(ROM_PACK)
	@ls -l $(ROM_PACK_FILES) || { echo "*** ERROR: missing ROM image(s) from ZIP?" ; false; }
	@echo "OK, downloaded ROMs are available for building combined ROM image."

$(ROM): $(ROM_PACK) Makefile
	cat $(ROM_PACK_FILES) > $(ROM)
	cp $(ROM) ../

clean:
	rm -f $(ROM)

distclean:
	$(MAKE) clean
	rm -f $(ROM_PACK) *.rom $(ROM_PACK_FILES)

