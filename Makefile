# Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
# Copyright (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
# http://xep128.lgb.hu/

include Makefile.common

ifneq ($(wildcard .arch),)
include .arch
else
ARCH	= native
endif
include arch/Makefile.$(ARCH)

# -flto is for link time optimization, CHANGE it to -g for debug material, but do NOT mix -g and -flto !!
DEBUG	= -flto

CFLAGS	= $(DEBUG) $(CFLAGS_ARCH)
ZCFLAGS = $(DEBUG) $(ZCFLAGS_ARCH)
CPPFLAGS= -DXEP128_ARCH=$(ARCH) -DXEP128_ARCH_$(shell echo $(ARCH) | tr 'a-z' 'A-Z')
LDFLAGS	= $(DEBUG) $(LDFLAGS_ARCH)
LIBS	= $(LIBS_ARCH)
SRCS	= $(SRCS_COMMON) $(SRCS_ARCH)

OBJS	= $(SRCS:.c=.o)
ZIP	= xep128-$(ARCH).zip


do-all:
	@echo "Compiler:     $(CC) $(CFLAGS) $(CPPFLAGS)"
	@echo "Linker:       $(CC) $(LDFLAGS) $(LIBS)"
	@echo "Architecture: $(ARCH) [$(ARCH_DESC)]"
	$(MAKE) $(PRG)

deb:
	if [ x$(ARCH) != xnative ]; then echo "*** You must set architecture to native first, with: make set-arch TO=native" ; false ; fi
	$(MAKE) all
	@echo "*** Debian package building is TODO ..."
	@false

set-arch:
	if [ x$(TO) = x ]; then echo "*** Must specify architecture with TO=..." ; false ; fi
	if [ x$(TO) = x$(ARCH) ]; then echo "*** Already this ($(ARCH)) architecture is set" ; false ; fi
	if [ ! -f arch/Makefile.$(TO) ]; then echo "*** This architecture ($(TO)) is not supported" ; false ; fi
	mkdir -p arch/objs.$(TO) arch/objs.$(ARCH)
	echo "ARCH = $(TO)" > .arch
	mv *.o arch/objs.$(ARCH)/ 2>/dev/null || true
	mv arch/objs.$(TO)/*.o . 2>/dev/null || true
	rm -f $(PRG)
	@echo "OK, architecture is set to $(TO) (from $(ARCH))."

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.s: %.c
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

z80ex/z180ex-$(ARCH).o: z80ex/z80ex.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -c -o $@ z80ex/z80ex.c
z80ex/z180ex_dasm-$(ARCH).o: z80ex/z80ex_dasm.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -c -o $@ z80ex/z80ex_dasm.c
z80ex/z80ex-$(ARCH).o: z80ex/z80ex.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -c -o $@ z80ex/z80ex.c
z80ex/z80ex_dasm-$(ARCH).o: z80ex/z80ex_dasm.c $(ZDEPS)
	$(CC) $(ZCFLAGS) -c -o $@ z80ex/z80ex_dasm.c

xep_rom.rom: xep_rom.asm
	sjasm -s xep_rom.asm xep_rom.rom || { rm -f xep_rom.rom xep_rom.lst xep_rom.sym ; false; }

xep_rom.sym: xep_rom.asm
	$(MAKE) xep_rom.rom

xep_rom_syms.h: xep_rom.sym
	awk '$$1 ~ /xepsym_[^:. ]+:/ { gsub(":$$","",$$1); gsub("h$$","",$$3); print "#define " $$1 " 0x" $$3 }' xep_rom.sym > xep_rom_syms.h

xep_rom.hex: xep_rom.rom
	od -A n -t x1 -v xep_rom.rom | sed 's/ /,0x/g;s/^,/ /;s/$$/,/' > xep_rom.hex

install: $(PRG) $(ROM) $(SDIMG)
	$(MAKE) strip
	mkdir -p $(BINDIR) $(DATADIR)
	cp $(PRG) $(BINDIR)/
	cp $(ROM) $(SDIMG) $(DATADIR)/

buildinfo.c:
	if [ -s .git/refs/heads/master ]; then cat .git/refs/heads/master > .git-commit-info ; fi
	echo "const char *BUILDINFO_ON  = \"`whoami`@`uname -n` on `uname -s` `uname -r`\";" > buildinfo.c
	echo "const char *BUILDINFO_AT  = \"`date -R`\";" >> buildinfo.c
	echo "const char *BUILDINFO_GIT = \"`cat .git-commit-info`\";" >> buildinfo.c
	echo "const char *BUILDINFO_CC  = __VERSION__;" >> buildinfo.c

$(SDIMG):
	@echo "**** Fetching SDcard image from $(SDURL) ..."
	wget -O $(SDIMG) $(SDURL) || { rm -f $(SDIMG) ; false; }

$(ROM):
	$(MAKE) -C rom

data:	$(SDIMG) $(ROM)
	rm -f buildinfo.c

$(PRG): .depend.$(ARCH) $(OBJS) z80ex/z180ex-$(ARCH).o z80ex/z180ex_dasm-$(ARCH).o
	rm -f buildinfo.c
	$(MAKE) buildinfo.o
	$(CC) -o $(PRG) $(OBJS) buildinfo.o z80ex/z180ex-$(ARCH).o z80ex/z180ex_dasm-$(ARCH).o $(LDFLAGS) $(LIBS)

zip:
	$(MAKE) $(ZIP)

$(ZIP): $(PRG) $(ROM) $(ZIP_FILES_ARCH)
	$(STRIP) $(PRG)
	zip $(ZIP) $(PRG) $(ROM) .git-commit-info $(ZIP_FILES_ARCH) README.md LICENSE CHANGES
	@ls -l $(ZIP)

publish: $(ZIP)
	test -f rom/$(ROM) && cp rom/$(ROM) www/files/ || true
	test -f $(ZIP) && cp $(ZIP) www/files/ || true
	@ls -l www/files/

strip:	$(PRG)
	$(STRIP) $(PRG)

zclean:
	rm -f z80ex/*.o

clean:
	rm -f $(OBJS) buildinfo.c buildinfo.o print.out xep_rom.hex xep_rom.lst xep_rom_syms.h $(ZIP) .git-commit-info*
	$(MAKE) -C rom clean

distclean:
	$(MAKE) clean
	$(MAKE) -C rom distclean
	$(MAKE) zclean
	rm -f $(SDIMG) $(DLL) $(ROM) $(PRG) xep128-*.zip .arch .depend.*
	rm -f arch/objs.*/*.o || true
	rmdir arch/objs.* 2>/dev/null || true

help:
	$(MAKE) $(PRG)
	./$(PRG) -help | grep -Ev '^(PATH:|Platform:|GIT|LICENSE:) ' > doc/help-cli.txt
	./$(PRG) -testparsing -config none | sed -e '1,/^--- /d' -e '/^--- /,$$d' > doc/help-config.txt

commit:
	git diff
	git status
	EDITOR="vim -c 'startinsert'" git commit -a
	git push

.depend.$(ARCH):
	$(MAKE) depend

dep:
	$(MAKE) depend

depend:
	$(MAKE) xep_rom.hex
	$(MAKE) xep_rom_syms.h
	$(CC) -MM $(CFLAGS) $(CPPFLAGS) $(SRCS) > .depend.$(ARCH)

valgrind:
	@echo "*** valgrind is useful mainly if you built Xep128 with the -g flag ***"
	valgrind --read-var-info=yes --leak-check=full --track-origins=yes ./$(PRG) -debug /tmp/xep128.debug > /tmp/xep128-valgrind.stdout 2> /tmp/xep128-valgrind.stderr
	ls -l /tmp/xep128.debug /tmp/xep128-valgrind.stdout /tmp/xep128-valgrind.stderr

.PHONY: all clean distclean strip commit publish data install zclean dep depend valgrind set-arch zip deb do-all

ifneq ($(wildcard .depend.$(ARCH)),)
include .depend.$(ARCH)
endif

