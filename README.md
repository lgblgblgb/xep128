# Xep128
Xep128 is an Enterprise-128 (a Z80 based, 8 bit computer) emulator (uses SDL2
and z80ex) with the main focus on emulating somewhat "exotic" hardware
additions.

Written by (C)2015 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

Xep128 main site: http://xep128.lgb.hu/
Source repository: http://github.com/lgblgblgb/xep128

Xep128 uses Z80 emulator "Z80ex": https://sourceforge.net/projects/z80ex/

Xep128 is licensed under the terms of GNU/GPL v2, for more information please
read file LICENSE. You can find the source on github, see above.

WARNING! Xep128 is in early alpha stage currently! It lacks many important
features (no sound, not so precise nick emulation, no dave interrupts other
than 1Hz and VINT from Nick, etc etc), and it's not comfortable to use, there
is no configuration but a single built-in config. It will change later,
hopefully.

Currently it's mainly for Linux and/or UNIX-like systems, but since it's an SDL
based project, it would be not that hard to port to Windows. There is some
on-going try (by me) to build for Windows on Linux, see later in this document
for details, if you want to try Xep128 on Windows.

Please note, that it's not the "best" Enterprise-128 emulator on the planet,
for that, you should use ep128emu project instead. Also, my emulator is not so
cycle exact for now, it does not emulate sound (currently), it also lacks
debugger what ep128emu has. However it emulates some "more exotic" (not so much
traditional) hardware additions becomes (or becoming) popular among EP users
recently: mouse support, APU ("FPU"), SD card reader and soon limited wiznet
w5300 emulation (Ethernet connection with built-in TCP/IP support).

# Installation on Linux / UNIX like OS (from source)

You can download the ZIP'ed repository from https://github.com/lgblgblgb/xep128
or you can clone the repository, whatever. You need the SDL2 development
libraries installed, on Debian/Ubuntu like systems, it can be done something
like this:

 apt-get install libsdl2-dev

You also need GNU variant of make (BSD make probably won't work). And of
course a C compiler, gcc (it should be OK with LLVM's clang as well, however
I haven't tried that, and also you need to modify Makefile). You may want to
install the mentioned packages, just to be sure (remember, if you are not
the root user, you may need to do it with command 'sudo'):

 apt-get install libsdl2-dev make gcc wget

You will also need the sjasm Z80 assembler somewhere in your PATH.

In the source, you need to issue the command "make". It will compile the
emulator, and also fetching (with wget) the SD card image through the Net
(about 256Mbyte in size). If everything is OK, you will get an executable
binary named "xep128", you can execute it from the current directory (otherwise
it won't found the ROM and the SD card image - missing the SD card image
will cause I/O error in EXOS if you try to use, but the ROM package is
essential of course).

# Installation on Windows (binary, .exe)

I've never used Windows, nor I have Windows installed. So the best I will
be able to do is trying to cross-compile for Windows on Linux.

You can try to compile Xep128 yourself using Linux with cross compiling
target for win32 (*make win32*).

*Or you can try my build (WARNING! Not tested, I have no windows!)*

This is how:

http://xep128.lgb.hu/files/xep128-win32.zip

Download and unzip this archive somewhere. It contains xep128.exe, the
ROM file, and the SDL2 DLL.

You also need the SD card image (warning, 256Mbyte!), from here:

http://xep128.lgb.hu/files/sdcard.img

Put the file into the same directory where you put the content of xep128.zip.

Now, you can try to execute xep128.exe ...

# Usage

Currently, the emulator outputs tons of information to the console/terminal
you started from, which can slow down the whole X server. You may want to
redirect the stdout to /dev/null or into a file (if you need it later) so
the terminal won't make your X server busy to render that amount of text
emited by the emulator :) Note: *this is not the case for Windows port,
you can ignore this warning then*.

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

Pause/break on your keyboard works as "soft reset". Press with shift key for
hard reset.

# More information on emulated hardware add-ons

The SD card image is a normal VHD file, you can try to replace it with your own.
SD card emulation is read-only currently. Card info is not correct, but it seems
the SDext ROM software does not mind it too much.

RTC is emulated enough for time keeping (query only, you can't set the time).
ZT (ZozoTools) ROM is included in the combined ROM package, you can see the
clock with typing this at the IS-BASIC prompt:

 :CLOCK

Quirky "printer emulation" can be used: in case of "printing" emulator tries to
create/append a file named "print.out" in the current directory. It's simply the
content of bytes sent to the printer port, including all escape sequences!
Some would be able to write an utility which renders the file as an EP80
printer like printout (including graphical mode, etc). Since print.out contains
every bytes sent to the printer, it's not a problem.

ZX Spectrum emulator card emulation :) does not work correctly. The provided
combined ROM pack file contains the ROM code (version 4.0). You can try that
with :ZX command from IS-BASIC prompt, but it's quite ugly.

There is an on-going work to emulate Z180 CPU. At least on EP exists with
Z180 hacked in :) The problem: Z180 does not support some of the undocumented
(but widely used) Z80 features, like separating IX and IY as two 8 bit
registers. The goal of the planned Z180 emulation is not about the correctness
of Z180 timing, not the on-chip features (like DMA) currently, but _only_
to be able to test softwares if it works with Z180 too (not using Z80
undocumented features).

# Known problems

There are many! I repeat myself: Xep128 is not a generic, or good emulator for
an average EP user. It's more about emulation of some unusal add-ons.

Just to mention some problems with Xep128:

* no configuration of the emulator too much
* no write disk access (with SD card)
* no EXDOS/WD emulation
* no menu/UI whatever
* no debugger
* no precise Nick emulation
* no slowdown of VRAM access, would be on a real machine
* no sound
* missing many features of Dave, including any interrupts than VINT and 1Hz
* no joystick emulation yet

# Credits

JSep (a JavaScript based Enterprise-128 emulator, also written by myself) was
great source for Xep128. Enteprise-128 forever forum is an essential site,
people there helped a lot (especially with JSep), I would mention IstvanV and
Zozosoft. LibZ80ex is the Z80 emulator used in Xep128 (not written by me!).

