ROM		= combined.rom
ROM_PACK	= rom_pack.zip
ROM_PACK_URL	= http://xep128.lgb.hu/files/rom_pack.zip
ROM_XEP		= XEP_EXOS.rom
ROM_XEP_SRC	= xep_exos.asm
ROM_PACK_FILES	= EXOS24UK-NOILLOPS.rom sdext-cart-64k.rom EPDOS-Z.rom ZT18UK-emuhoz.rom zx-emu-40.rom

all: $(ROM)

$(ROM_PACK):
	wget -O $(ROM_PACK) $(ROM_PACK_URL) || { rm -f $(ROM_PACK) ; false; }
	unzip $(ROM_PACK)
	@ls -l $(ROM_PACK_FILES) || { echo "*** ERROR: missing ROM image(s) from ZIP?" ; false; }
	@echo "OK, downloaded ROMs are available for building combined ROM image."

$(ROM_XEP): $(ROM_XEP_SRC)
	sjasm $(ROM_XEP_SRC) $(ROM_XEP) || { rm -f $(ROM_XEP) ; false; }

$(ROM): $(ROM_XEP) $(ROM_PACK)
	cat $(ROM_PACK_FILES) $(ROM_XEP) > $(ROM)

clean:
	rm -f $(ROM_XEP) $(ROM) *.lst

distclean:
	$(MAKE) clean
	rm -f $(ROM_PACK) *.rom $(ROM_PACK_FILES)

