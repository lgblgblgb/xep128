/* Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
   Copyright (C)2015 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
   http://xep128.lgb.hu/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "xepem.h"


int primo_on = 0;
int primo_nmi_enabled = 0;

extern int vsync;



void primo_switch ( Uint8 data )
{
	if (data & 128) {
		if (data & 64)
			primo_write_io(0, 0);
		if (primo_on)
			return;
		primo_on = 0x40;	// the value is important, used by port "limit" for lower primo area!
		zxemu_switch(0);	// switch ZX Spectrum emulation off, if already enabled for some reason ...
	} else {
		if (!primo_on)
			return;
		primo_on = 0;
	}
	fprintf(stderr, "PRIMOEMU: emulation is turned %s.\n", primo_on ? "ON" : "OFF");
	nmi_pending = 0;
}


static int primo_scan_key ( int scan )
{
	return (kbd_matrix[scan >> 3] & (1 << (scan & 7))) ? 0 : 1;
}


Uint8 primo_read_io ( Uint8 port )
{
	return (vsync ? 32 : 0) | primo_scan_key(port);
}


void primo_write_io ( Uint8 port, Uint8 data )
{
	primo_nmi_enabled = data & 128;				// bit 7: NMI enabled, this variable is "cloned" as nmi_pending by VINT in dave.c
	ports[0xA8] = ports[0xAC] = (data & 16) ? 63 : 0;	// bit 4, speaker control
	memory[0x3F4005] = (data & 8) ? 0x28 : 0x08;		// bit 3: display page: set LPB LD1 high (LPB should be at segment 0xFD, and 0xFC is for primo 0xC000-0xFFFF)
}


static int search_primo_rom ( void )
{
	int a;
	for (a = 0x168; a < rom_size; a += 0x4000)
		if (!memcmp(memory + a, "PRIMO", 5))
			return a >> 14;
	return -1;
}


static const Uint8 primo_lpt[] = {
	// the USEFULL area of the screen, this must be the FIRST LPB in our LPT.
	// LPIXEL 2 colour mode is used, 192 scanlines are used
	256-192,14|16,  14, 46,  0,0,0,0,     1,0xFF,0,0,0,0,0,0,
	// bottom not used area ... 47 scanlines
	256-47,  2 | 128,  6, 63,  0,0,0,0,     0,0,0,0,0,0,0,0,	// 128 = ask for VINT, which is cloned as NMI in primo mode
	// SYNC etc stuffs ...
	256-3 ,  0, 63,  0,  0,0,0,0,     0,0,0,0,0,0,0,0,
	256-2 ,  0,  6, 63,  0,0,0,0,     0,0,0,0,0,0,0,0,
	256-1 ,  0, 63, 32,  0,0,0,0,     0,0,0,0,0,0,0,0,
	256-19,  2,  6, 63,  0,0,0,0,     0,0,0,0,0,0,0,0,
	// upper not used area ... 48 scanlines, the RELOAD bit must be set!!!!
	256-48,  3,  6, 63,  0,0,0,0,     0,0,0,0,0,0,0,0
};



void primo_emulator_execute ( void )
{
	int romseg = search_primo_rom();
	printf("PRIMO: ROM segment is %d\n", romseg);
	if (romseg == -1) return;
	/* set an LPT */
	memcpy(memory + 0x3F4000, primo_lpt, sizeof primo_lpt);
	z80ex_pwrite_cb(0x82, 0);	// LPT address, low byte
	z80ex_pwrite_cb(0x83, 4 | 64 | 128);
	z80ex_pwrite_cb(0x81, 0);	// border color
	/* do our stuffs ... */
	z80ex_pwrite_cb(0xB4, 0);	// disable all interrupts on Dave
	z80ex_pwrite_cb(0xB0, romseg);	// first segment is the Primo ROM now
	z80ex_pwrite_cb(0xB1, 0xFA);	// normal RAM segment
	z80ex_pwrite_cb(0xB2, 0xFB);	// normal RAM segment
	z80ex_pwrite_cb(0xB3, 0xFC);	// a video segment as the Primo video RAM
	primo_switch(128 | 64);		// turn on Primo I/O mode
	Z80_PC = 0;			// Z80 reset address to the Primo ROM
}

