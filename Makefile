# Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
# Copyright (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
# http://xep128.lgb.hu/

SRCS_COMMON = lodepng.c screen.c main.c cpu.c z180.c nick.c dave.c input.c exdos_wd.c sdext.c rtc.c printer.c zxemu.c primoemu.c emu_rom_interface.c w5300.c apu.c keyboard_mapping.c configuration.c roms.c console.c emu_monitor.c joystick.c fileio.c gui.c z80.c z80dasm.c snapshot.c

SDIMG	= sdcard.img
SDURL	= http://xep128.lgb.hu/files/sdcard.img
ROM	= combined.rom
DLL	= SDL2.dll
DLLURL	= http://xep128.lgb.hu/files/SDL-2.0.4.dll

all:
	$(MAKE) do-all

RELEASE_TAG =

ARCH	= native

ARCHALL	= native win32 win64

include build/Makefile.$(ARCH)

# -flto is for link time optimization, CHANGE it to -g for debug material, but do NOT mix -g and -flto !!
DEBUG	=

DEBUG_RELEASE = -flto

CFLAGS	= $(DEBUG) $(CFLAGS_ARCH)
LDFLAGS	= $(DEBUG) $(LDFLAGS_ARCH)
LIBS	= $(LIBS_ARCH)
SRCS	= $(SRCS_COMMON) $(SRCS_ARCH)
OPREFIX	= build/objs/$(ARCH)$(RELEASE_TAG)--
DEPFILE	= $(OPREFIX)make.depend

OBJS	= $(addprefix $(OPREFIX), $(SRCS:.c=.o))
ZIP	= xep128-$(ARCH).zip

ifdef WINDRES
WRCSRC	= arch/windres.rc
WRCOBJ	= $(OPREFIX)windres.res
OBJS   += $(WRCOBJ)
$(WRCOBJ): $(WRCSRC)
	$(WINDRES) $< -O coff -o $@
endif

do-all:
	@echo "Compiler:     $(CC) $(CFLAGS)"
	@echo "Linker:       $(CC) $(LDFLAGS) $(LIBS)"
	@echo "Architecture: $(ARCH) [$(ARCH_DESC)]"
	$(MAKE) $(PRG)

build-dist:
	#rm -f build/objs/*
	for arch in $(ARCHALL) ; do $(MAKE) dep ARCH=$$arch RELEASE_TAG=-rel ; $(MAKE) ARCH=$$arch DEBUG=$(DEBUG_RELEASE) RELEASE_TAG=-rel ; $(MAKE) strip ARCH=$$arch DEBUG=$(DEBUG_RELEASE) RELEASE_TAG=-rel ; done

build-dist-test:
	ARCHALL="$(ARCHALL)" RELEASE_TAG=-debug DEBUG=-g STRIP=no build/tester

build-dist-rel:
	ARCHALL="$(ARCHALL)" RELEASE_TAG=-rel DEBUG=$(DEBUG_RELEASE) STRIP=yes build/tester

publish-dist-test:
	$(MAKE) build-dist-test
	$(MAKE) rom
	cat xep128.zip | ssh download.lgb.hu ".download.lgb.hu-files/pump xep128-devel.zip-`date '+%Y%m%d%H%M%S'`"


deb:
	@if [ x$(ARCH) != xnative ]; then echo "*** DEB package building is only allowed for ARCH=native" >&2 ; false ; fi
	$(MAKE) ARCH=native RELEASE_TAG=-rel DEBUG=$(DEBUG_RELEASE)
	$(MAKE) strip ARCH=native RELEASE_TAG=-rel DEBUG=$(DEBUG_RELEASE)
	build/deb-build-simple

$(OPREFIX)%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.s: %.c
	$(CC) -S $(CFLAGS) $< -o $@

install: $(PRG) $(ROM) $(SDIMG)
	$(MAKE) strip
	mkdir -p $(BINDIR) $(DATADIR)
	cp $(PRG) $(BINDIR)/
	cp $(ROM) $(SDIMG) $(DATADIR)/

buildinfo.c:
	if [ -s .git/refs/heads/master ]; then cat .git/refs/heads/master > .git-commit-info ; fi
	echo "const char *BUILDINFO_ON  = \"`whoami`@`uname -n` on `uname -s` `uname -r`\";" > buildinfo.c
	echo "const char *BUILDINFO_AT  = \"`date`\";" >> buildinfo.c
	echo "const char *BUILDINFO_GIT = \"`cat .git-commit-info`\";" >> buildinfo.c
	echo "const char *BUILDINFO_CC  = __VERSION__;" >> buildinfo.c

$(SDIMG):
	@echo "**** Fetching SDcard image from $(SDURL) ..."
	wget -O $(SDIMG) $(SDURL) || { rm -f $(SDIMG) ; false; }

rom/$(ROM):
	$(MAKE) -C rom

$(ROM): rom/$(ROM)
	cp rom/$(ROM) .

data:	$(SDIMG) $(ROM)
	rm -f buildinfo.c

$(PRG): $(DEPFILE) $(OBJS)
	rm -f buildinfo.c
	$(MAKE) $(OPREFIX)buildinfo.o
	$(CC) -o $(PRG) $(OBJS) $(OPREFIX)buildinfo.o $(LDFLAGS) $(LIBS)

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

clean:
	rm -f $(OBJS) buildinfo.c $(OPREFIX)buildinfo.o print.out $(ZIP) .git-commit-info*
	$(MAKE) -C data clean
	$(MAKE) -C rom clean

distclean:
	$(MAKE) clean
	$(MAKE) -C rom distclean
	rm -f $(SDIMG) $(DLL) $(ROM) $(PRG) xep128-*.zip xep128_*.deb
	rm -f build/objs/*

help:
	$(MAKE) $(PRG)
	./$(PRG) -help | grep -Ev '^(PATH:|Platform:|GIT|LICENSE:) ' > doc/help-cli.txt
	./$(PRG) -testparsing -config none | sed -e '1,/^--- /d' -e '/^--- /,$$d' > doc/help-config.txt

$(DEPFILE):
	$(MAKE) depend

dep:
	$(MAKE) depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) | awk '/^[^.:\t ]+\.o:/ { print "$(OPREFIX)" $$0 ; next } { print }' > $(DEPFILE)

valgrind:
	@echo "*** valgrind is useful mainly if you built Xep128 with the -g flag ***"
	valgrind --read-var-info=yes --leak-check=full --track-origins=yes ./$(PRG) -debug /tmp/xep128.debug > /tmp/xep128-valgrind.stdout 2> /tmp/xep128-valgrind.stderr
	ls -l /tmp/xep128.debug /tmp/xep128-valgrind.stdout /tmp/xep128-valgrind.stderr

subrepos:
	mkdir -p gh-pages wiki
	test -d wiki/.git || git clone --depth=1 git@github.com:lgblgblgb/xep128.wiki.git wiki
	test -d gh-pages/.git || git clone --branch=gh-pages --depth=1 git@github.com:lgblgblgb/xep128.git gh-pages
	cd gh-pages && git pull
	cd wiki && git pull

.PHONY: all clean distclean strip publish data install dep depend valgrind zip deb do-all subrepos

ifneq ($(wildcard $(DEPFILE)),)
include $(DEPFILE)
endif

