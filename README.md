# xep128
Enterprise-128 emulator (uses sdl2 and libz80ex) with the main focus on emulating somewhat "exotic" hardware additions.

Currently it's for Linux and/or UNIX-like systems, but since it's an SDL based project, it would be not that hard to port to Windows, I guess. LibZ80ex (great Z80 emulator, http://z80ex.sourceforge.net/) is included just in case, if your OS distribution does not contain it.

Please note, that it's not the "best" Enterprise-128 emulator on the planet, for that, you should use ep128emu project instead. Also, my emulator is not so cycle exact correctly, it does not emulate sound (currently), it also lack debugger what ep128emu has. However it emulates some "more exotic" (not so much traditional) hardware additions becomes (or becoming) popular among EP users recently: mouse support, APU ("FPU"), SD card reader and soon limited wiznet w5300 emulation (Ethernet connection with built-in TCP/IP support).
