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



void primo_switch ( Uint8 data )
{
	if (data & 128) {
		if (data & 64)
			primo_write_io(0, 0);
		if (primo_on)
			return;
		primo_on = 0x40;	// the value if important, used by port "limit" for lower primo area!
		zxemu_switch(0);	// turn off ZX Spectrum emulation if already enabled for some reason ...
	} else {
		if (!primo_on)
			return;
		primo_on = 0;
	}
	fprintf(stderr, "PRIMOEMU: emulation is turned %s.\n", primo_on ? "ON" : "OFF");
	nmi_pending = 0;
}


Uint8 primo_read_io ( Uint8 port )
{
	return 0xFF;
}


void primo_write_io ( Uint8 port, Uint8 data )
{
	primo_nmi_enabled = data & 128;				// bit 7: NMI enabled, this variable is "cloned" as nmi_pending by VINT in dave.c
	ports[0xA8] = ports[0xAC] = (data & 16) ? 63 : 0;	// bit 4, speaker control
	memory[0x3F4005] = (data & 8) ? 0xE8 : 0xC8;		// bit 3: display page: set LPB LD1 high (LPB should be at segment 0xFD, and 0xFC is for primo 0xC000-0xFFFF)
}


void primo_emulator_execute ( void )
{
}

