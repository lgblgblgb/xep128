# Xep128
Enterprise-128 emulator (uses sdl2 and libz80ex) with the main focus on
emulating somewhat "exotic" hardware additions.

WARNING! Xep128 is in early alpha stage currently! It lacks many important
features (no sound, not so precise nick emulation, no dave interrupts other
than 1Hz and VINT from Nick, etc etc), and it's not comfortable to use, there
is no configuration but a single built-in config. It will change later,
hopefully.

Xep128 is licensed under the terms of GNU/GPL v2, for more information please
read file LICENSE

Currently it's for Linux and/or UNIX-like systems, but since it's an SDL based
project, it would be not that hard to port to Windows, I guess. There is some
on-going tries (from me) to build for Windows on Linux, see later in this
document for details, if you want to try Xep128 on Windows.

LibZ80ex (great Z80 emulator, http://z80ex.sourceforge.net/) is included just
in case, if your OS distribution does not contain it (or older version, etc).

Please note, that it's not the "best" Enterprise-128 emulator on the planet,
for that, you should use ep128emu project instead. Also, my emulator is not so
cycle exact correctly, it does not emulate sound (currently), it also lacks
debugger what ep128emu has. However it emulates some "more exotic" (not so much
traditional) hardware additions becomes (or becoming) popular among EP users
recently: mouse support, APU ("FPU"), SD card reader and soon limited wiznet
w5300 emulation (Ethernet connection with built-in TCP/IP support).

Z80 emulation is done with the nice z80ex Z80 emulator, which is a separated
(not my!) project also with licence of GNU/GPL (so importing it into the
repository of my emulator can't be a problem). Some Linux distribution indeed
contains it, but I guess it's better in this way.

# Installation on Linux / UNIX like OS (from source)

You can download the ZIP'ed repository from https://github.com/lgblgblgb/xep128
or you can clone the repository, whatever. You need the SDL2 development
libraries installed, on Debian/Ubuntu like systems, it can be done something
like this:

 apt-get install libsdl2-dev

You also need GNU variant of make (BSD make probably won't work). And of
course a C compiler, gcc (it should be OK with LLVM's clang as well, however
I haven't tried that, and also you need to modify Makefile). You may want to
install the mentioned packages, just to be sure:

 apt-get install libsdl2-dev make gcc wget

In the source, you need to issue the command "make". It will compile the
emulator, and also fetching (with wget) the SD card image through the Net
(about 256Mbyte in size). If everything is OK, you will get an executable
binary named "xep128", you can execute it from the current directory (otherwise
it won't found the ROM and the SD card image - missing the SD card image
will cause I/O error in EXOS if you try to use, but the ROM package is
essential of course).

# Installation on Windows (binary, .exe)

I've never used Windows, nor I have Windows installed. So the best I will
be able to do is trying to cross-compile for Windows on Linux. This would
also require some modification and architecture dependent parts, maybe.
You can try to compile Xep128 yourself using Linux with cross compiling
target for win32.

Or you can try my build (WARNING! Not tested, I have no windows!)

http://xep128.lgb.hu/files/xep128.zip

Download and unzip this archive somewhere. It contains xep128.exe, the
ROM file, and the SDL2 DLL.

You also need the SD card image (warning, 256Mbyte!), from here:

http://xep128.lgb.hu/files/sdcard.img

Put the file into the same directory where you put the content for the
xep128.zip.

Now, you can try to execute xep128.exe ...

# Usage

Currently, the emulator outputs tons of information to the console/terminal
you started from, which can slow down the whole X server. You may want to
redirect the stdout to /dev/null or into a file (if you need it later) so
the terminal won't make your X server busy to render that amount of text
emited by the emulator :)

Mouse emulation more or less works: it emulates the "boxsoft mouse interface".
If you click into the emulation window, it enters into "mouse grab" mode,
and you can use your mouse (if the software running on the emulated Ep128
supports mouse, of course). You can exit from this grab mode, if you press
the ESC button.

You can try SymbOS out, it's included on the SD card image. It's a very nice
multitasking (!) window-oriented graphical operating system ported to multiple
Z80 based computers, including Enterprise-128, MSX and Amstrad CPC too. To
try it, type the following command at the IS-BASIC prompt:

 LOAD"SYM"

Note, that keyboard mapping is more-or-less "positional", eg, for quotation mark
you need to press shift+2 because quotation mark on the Enterprise keyboard
can be accessed with shifted key 2. And so on, you may get the idea.

RTC is emulated enough for time keeping (query only, you can't set the time).
ZT (ZozoTools) ROM is included in the combined ROM package, you can see the
clock with typing this at the IS-BASIC prompt:

 :CLOCK

