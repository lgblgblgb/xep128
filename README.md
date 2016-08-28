# Xep128 - An Enterprise-128 emulator

[![Build Status](https://api.travis-ci.org/lgblgblgb/xep128.svg?branch=master)](https://travis-ci.org/lgblgblgb/xep128)
[![Join the chat at https://gitter.im/lgblgblgb/xep128](https://badges.gitter.im/lgblgblgb/xep128.svg)](https://gitter.im/lgblgblgb/xep128?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Download](https://api.bintray.com/packages/lgblgblgb/generic/xep128/images/download.svg)](https://bintray.com/lgblgblgb/generic/xep128/_latestVersion)

Xep128 is an Enterprise-128 (a Z80 based, 8 bit computer) emulator (uses SDL2
and modified z80ex for Z80 emulation) with the main focus on emulating somewhat
"exotic" hardware additions. Currently it runs on Linux/UNIX, Windows and also
on OSX (but OSX build may have some problems).

Written by (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

Xep128 main site: http://xep128.lgb.hu/
Source repository: http://github.com/lgblgblgb/xep128

Xep128 uses (modified, by me) Z80 emulator "Z80ex": https://sourceforge.net/projects/z80ex/
and lodePNG to write screenshots: http://lodev.org/lodepng/

Xep128 is licensed under the terms of GNU/GPL v2, for more information please
read file LICENSE. You can find the source on github, see above.

WARNING! Xep128 is in early alpha stage currently! It lacks many important
features (no/ugly sound, not so precise nick emulation, etc etc), and it's not
comfortable to use, there is only CLI/config file based configuration, etc.

Currently it's mainly for Linux and/or UNIX-like systems and Windows, however
since I don't use Windows, I can't test if it really works (cross compiled on
Linux).

Please note, that it's not the "best" Enterprise-128 emulator on the planet,
for that, you should use ep128emu project instead. Also, my emulator is not so
cycle exact for now, it does not emulate sound quite well (currently), it also lacks
debugger what ep128emu has. However it emulates some "more exotic" (not so much
traditional) hardware additions becomes (or becoming) popular among EP users
recently: mouse support, APU ("FPU"), SD card reader and soon limited wiznet
w5300 emulation (Ethernet connection with built-in TCP/IP support).

# Installation on Linux / UNIX like OS (from source)

You can download the ZIP'ed repository from https://github.com/lgblgblgb/xep128
or you can clone the repository, whatever. You need the SDL2 (2.0.4 or newer!),
libreadline and GTK3 _development_ libraries installed (also some additiona
 tools like the C compulter), on Debian/Ubuntu like systems, it can be done
something like this (do not forget, that you should do this as root, so probably
you need "sudo" before this command):

 apt-get install libsdl2-dev make gcc wget libreadline6-dev libgtk-3-dev

You will also need the sjasm (v0.42) Z80 assembler somewhere in your PATH, if
you modified xep_rom.asm at least (otherwise sjasm is not needed).

In the source, you need to issue the command "make" (though if you modified
source I would recommend to say "make dep" first). It will compile the
emulator, you should have an executable "xep128" at the end. Now you need
the ROM image and the SD card image. You can download it by hand (please
read the Windows section about the URLs), or you can do it with the following
commands:

 make combined.rom

 make sdcard.img

Now, you can execute the emulator from the current directory. Optionally you
can say "make install" (you need root access) which - by default - installs
emulator and the ROM/SD card images into /usr/local.

Warning, the SD-card image is 256Mbyte long! You can try the emulator without
the SD card image, but you won't be able to access its content then, of
course.

# Installation on Windows (binary, .exe)

I've never used Windows, nor I have Windows installed. So the best I will
be able to do is trying to cross-compile for Windows on Linux.

You can try to compile Xep128 yourself using Linux with cross compiling
target for win32 or win64 (*make ARCH=win32* or *make ARCH=win64*) which
itself needs mingw 32 and/or 64 bit cross-compiler installed, also with
SDL2 (2.0.4, or newer!).

**Or you can try my build (WARNING! Not tested, I have no windows!)**

This is how:

http://download.lgb.hu/?class=xep128-rel.zip
http://download.lgb.hu/?class=xep128-test.zip

Download and unzip this archive somewhere (the two links are "rel" for release
and "test" for even more brave people, but please note, that even "release"
does not mean too much at this phase of development). It contains xep128.exe,
the ROM file, and the SDL2 DLL. If you want to try the 64 bit version, you
need to use xep128_64.exe, and SDL2_64.dll renamed to SDL2.dll back.

You also need the SD card image (warning, 256Mbyte!), from here:

http://xep128.lgb.hu/files/sdcard.img

Put the file into the same directory where you put the content of xep128.zip.

Now, you can try to execute xep128.exe ...

# Usage

Mouse emulation more or less works: it emulates the "boxsoft mouse interface".
If you click into the emulation window, it enters into "mouse grab" mode,
and you can use your mouse (if the software running on the emulated Ep128
supports mouse, of course). You can exit from this grab mode, if you press
the ESC button.

Joystick (currently both of them at the _same_ time ...) is emulated with the
numeric keypad arrow keys. Because of some conflict between the "boxsoft mouse
interface" and joystick, there is an odd solution now: if you are in "mouse
grab" mode (see above) EP software will read data as mouse events, otherwise
joystick. There is some auto detection, that shift pulses used by the mouse
query routines cause to switch into mouse mode after that read only. Thus,
SymbOS (see later) will detect the mouse, but also there is an odd behaviour
the mouse behaves oddly if you are not in the grab mode though ...

Emulation window can be resized, and/or you can switch between fullscreen and
window mode by pressing F11. Note, these functions can have some bugs
currently.

You can leave emulator with closing its window, by pressing F9, or giving
the EXOS command "_XEP exit_" (":XEP" exit in IS-BASIC of course).

F10 creates a screenshot in PNG format, but it does not work currently, or
has bugs (it depends when you read this file ...).

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

Please continue with the next chapter for more advanced topics.

# More information on emulated hardware add-ons

The SD card image is a normal VHD file, you can try to replace it with your own.
SD card emulation is read-only currently. Card info is not correct, but it seems
the SDext ROM software does not mind it too much.

RTC is emulated enough for time keeping (query only, you can't set the time).
ZT (ZozoTools) ROM is included in the combined ROM package, you can see the
clock with typing this at the IS-BASIC prompt (_Warning: because of conflict
with Z180 and ZozoTools, this may not available in the current ROM image.
Clock/RTC would work, just you can't test it with this command, sorry_):

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

There is an on-going work to emulate Z180 CPU. At least one EP exists with
Z180 hacked in :) The problem: Z180 does not support some of the undocumented
(but widely used) Z80 features, like separating IX and IY as two 8 bit
registers. The goal of the planned Z180 emulation is not about the correctness
of Z180 timing, not the on-chip features (like DMA) currently, but _only_
to be able to test softwares if it works with Z180 too (not using Z80
undocumented features).

For normal Z80, one can select the good old NMOS, or the CMOS emulation.
You can issue command :XEP CPU to query CPU emulation status. You can also
use this to set the CPU type, with another word after CPU, it can be Z80
(NMOS Z80), Z80C (CMOS Z80) and Z180 (it is always CMOS). EXOS command
:XEP CPU can also be used with a number as an argument, it sets the CPU
clock, for example (NOTE: this feature has bugs currently!):

 :XEP CPU 7.12

RAM size can be queried with :XEP RAM command, also, you can use this
construct to set RAM size up with :XEP RAM 128 (it will set 128 Kbytes of
RAM). This also causes the emulated EP to reboot.

# Configuration and command line

Xep128 is *not* a GUI oriented emulator. Its only GUI level component is the
emulator window, and the OSD (On-Screen Display). Though it's possible to
configure the emulator.

First, you should be familiar with the file system usage of Xep128:

It's important to note the syntax and meaning of file names for Xep128
on the "PC" side (Linux or Windows, which runs Xep128). In any situation
when Xep128 needs a file (reading config file, loading ROM image, using
the SD card disk image, and so on) it uses a common function which the
file names in a specific way.

* If a file name has the form of "@filename" then it means "filename" in the
  user preferences directory. Still, it's possible to use "@dir/filename"
  (or @dir\filename in case of Windows) for a sub-directory in the "pref
  dir". So '@' basically means prefixing the file name with pref dir.
* If a file name "seems to be" an absolute path (that is: the file name begins
  with '/' in case of Unix/Linux or either of begins with '\' or have
  the ":\" part in case of Windows) then the file will be tried to use
  as-is, as an absolute path without any prefixing.
* Otherwise, the file name is tried to be used with various prefixes probed,
  including the current directory, the data directory (on Linux/UNIX), and
  even the pref directory. You must be careful, as it's possible to use
  some other file in an other directory by Xep128 what you meant ... The
  best way is to use the pref dir if possible, and it's also kinda private
  storage only for Xep128.

Now, the configuration possibilities:

First, Xep128 has some built-in defaults, so it can run without any previous
configuration step, without any configuration file, etc.

Second, Xep128 can read a text based config file. By default it tries to use
@config (@ means the user preference directory - remember). On start-up, it
writes the @config-sample file (it is also noted in a message window, with
the full path, so you can learn what '@' means). You can use this file to
rename it to config (in the pref directory of course) with some customization.
You can also override the config file to be used with the -config command line
switch (there, you should use the '@' syntax as well, if you need the pref
directory). That sample config file can be useful, as it has even comments
about the syntax and meaning of a given option. You can even delete the sample
config file, Xep128 will re-create it. It can be useful, if a new version of
the emulator has more options etc and you need to examine all of them (Xep128
only writes the sample config file if the file does not exist).

Third, you can use command line switches, which overrides both the built-in
config, and the configuration read from a file. You can even use the
"-config none" option to de-activate config file reading, if you need only
your options.

To learn about command line switches, you can use the -help switch.
**It also prints the pref directory** and other emulator related information,
which can be useful in bug reports, etc.

Basically, both the command line and configuration file syntax has the
same options, but in the config file you say "key = value" where in command
line you need to specify "-key value" format. If you use the "-config"
switch, it must be the first command line argument though, "-config @my"
will use file "my" in the pref dir instead of default @config.

By default, Xep128 tries to load "combined.rom" from segment zero as the
ROM image. It's simple a concated series of ROM images, starting from
segment zero (thus the EXOS). Of course you can override even this with
"-rom@00 @myexos.rom" for example in the command line (here again, the
'@' before the file name means the pref dir, however rom@00 is simply
the ROM at segment 00) or with the "rom@00 = @myexos.rom" config file line.
Since combined.rom is one big file loaded from segment zero, if you override
the rom@00 value, to load only your EXOS, still you will loose all of the
other concated images, as combined.rom treated a single entity. Of course
you can specify other ROM images as well from different segments like
"rom@0c" (ROM image from segment 0x0C) or such.

Further readings:

* [Summary of CLI options, output of -help](doc/help-cli.txt)
* [Example of a sample configuration file](doc/help-config.txt)

# Known problems

There are many! I repeat myself: Xep128 is not a generic, or good emulator for
an average EP user. It's more about emulation of some unusal add-ons.

Just to mention some problems with Xep128:

* no EXDOS/WD emulation
* no menu/UI whatever
* no debugger
* no precise Nick emulation
* no slowdown of VRAM access, would be on a real machine
* no sound [in-progress now]

# Credits

JSep (a JavaScript based Enterprise-128 emulator, also written by myself) was
great source for Xep128. Enteprise-128 forever forum is an essential site,
people there helped a lot (especially with JSep), I would mention IstvanV -
he has the great ep128emu emulator - and Zozosoft. LibZ80ex is the Z80
emulator used in Xep128 (not written by me!). FUSE Spectrum emulator is
important: it seems Z80Ex contains some code from FUSE, also they have
wiznet w5100 emulation which is similar to the w5300 so it can help me
somewhat. LodePNG (not written by me!) project is used to create PNG
screenshots. It has been somewhat modified to exclude C++ parts and some
defaults though.
