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

int z180_port_start;
static Uint8 z180_ports[0x40];
static int z180_incompatibility_reported = 0;


static void invalid_opcode (Z80EX_CONTEXT *unused_1, Z80EX_BYTE prefix, Z80EX_BYTE series, Z80EX_BYTE opcode, void *unused_2)
{
	int pc = z80ex_get_reg(z80, regPC);
	fprintf(stderr, "Z180: Invalid Z180 opcode <prefix=%02Xh series=%02Xh opcode=%02Xh> at PC=%04Xh [%02Xh:%04Xh]\n",
		prefix, series, opcode,
		pc,
		ports[0xB0 | (pc >> 14)],
		pc & 0x3FFF
	);
	if (z180_incompatibility_reported) return;
	z180_incompatibility_reported = 1;
	ERROR_WINDOW("Z180: Invalid Z180 opcode <prefix=%02Xh series=%02Xh opcode=%02Xh> at PC=%04Xh [%02Xh:%04Xh]\nThere will be NO further error reports about this kind of problem to avoid window flooding :)",
		prefix, series, opcode,
		pc,
		ports[0xB0 | (pc >> 14)],
		pc & 0x3FFF
	);
}


void z180_internal_reset ( void )
{
	z180_port_start = 0;
	memset(z180_ports, 0, sizeof z180_ports);
	z80ex_set_z180_callback(z80, invalid_opcode, NULL);
	z180_incompatibility_reported = 0;
}


void z180_port_write ( Uint8 port, Uint8 value )
{
	fprintf(stderr, "Z180: write internal port (%02Xh/%02Xh) data = %02Xh\n", port, port | z180_port_start, value);
	switch (port) {
		case 0x3F:
			z180_port_start = value & 0xC0;
			fprintf(stderr, "Z180: internal ports are moved to %02Xh-%02Xh\n", z180_port_start, z180_port_start + 0x3F);
			break;
	}
	z180_ports[port] = value;
}


Uint8 z180_port_read ( Uint8 port )
{
	fprintf(stderr, "Z180: read internal port (%02Xh/%02Xh)\n", port, port | z180_port_start);
	return z180_ports[port];
}


