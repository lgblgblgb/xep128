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

int zxemu_on = 0;
Uint8 zxemu_ports[5];

/*
Not working at the moment.

TODO: if emulator is turned on (zxemu_on is not zero) reading or writing port 0xFE
(zxemu_read_ula and zxemu_write_ula functions) should generate an NMI and filling
ports 0x40 - 0x43 so the ZX emu ROM NMI handler can query the "trapped" ZX op
and emulates it. 
*/


int zxemu_nmi ( void )
{
	if (zxemu_on) {
		z80ex_nmi(z80);
		fprintf(stderr, "ZXEMU: NMI!\n");
		return 1;
	} else
		return 0;
}


void zxemu_write_ula ( Uint8 hiaddr, Uint8 data )
{
	zxemu_ports[0] = hiaddr;	// high I/O address
	zxemu_ports[1] = 0xFE;		// low I/O address, the ULA port
	zxemu_ports[2] = data;		// data on the bus
	zxemu_ports[3] = 0;		// ?? 9 = I/O kind of op
	if (!zxemu_nmi())
		fprintf(stderr, "ZXEMU: ULA write: no NMI (switched off)\n");
}


Uint8 zxemu_read_ula ( Uint8 hiaddr )
{
	zxemu_ports[0] = hiaddr;
	zxemu_ports[1] = 0xFE;
	zxemu_ports[2] = 0xFF; // ???????
	zxemu_ports[3] = 0;
	if (!zxemu_nmi())
		fprintf(stderr, "ZXEMU: ULA read: no NMI (switched off)\n");
	return 0xFF;
}

