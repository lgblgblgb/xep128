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

struct commands_st {
	const char *cmd;
	const char *help;
	void (*handler)(void);
};

static char buffer[256];
static char *carg;

static const char *SHORT_HELP = "XEP   version 0.1  (Xep128 ROM)\r\n";

#define COBUF ((char*)(memory + xep_rom_addr + 0x3802))
#define SET_A(v) z80ex_set_reg(z80, regAF, (z80ex_get_reg(z80, regAF) & 0xFF) | ((v) << 8))
#define SET_C(v) z80ex_set_reg(z80, regBC, (z80ex_get_reg(z80, regBC) & 0xFF00) | (v))

static void cmd_cpu ( void ) {
	char buf[512] = "";
	if (*carg) {
		if (!strcmp(carg, "z80")) {
			z80ex_set_z180(z80, 0);
			z80ex_set_nmos(z80, 1);
		}
		else if (!strcmp(carg, "z80c")) {
			z80ex_set_z180(z80, 0);
			z80ex_set_nmos(z80, 0);
		}
		else if (!strcmp(carg, "z180")) {
			z80ex_set_z180(z80, 1);
			z80ex_set_nmos(z80, 0);
		} else {
			sprintf(buf, "*** Unknown CPU type to set: %s\r\n", carg);
		}
	}
	sprintf(COBUF, "%sCPU: %s %s @ %.2fMHz\r\n",
		buf,
		z80ex_get_z180(z80) ? "Z180" : "Z80",
		z80ex_get_nmos(z80) ? "NMOS" : "CMOS",
		CPU_CLOCK / 1000000.0
	);
}


static void cmd_help ( void );

static const struct commands_st commands[] = {
	{ "cpu",	"Set/query CPU type/clock", cmd_cpu },
	{ NULL,		NULL, NULL }
};

extern const char *BUILDINFO_ON; //  = "lgb@vega on Linux 3.19.0-15-generic";
extern const char *BUILDINFO_AT; //  = "Thu, 04 Jun 2015 22:56:28 +0200";
extern const char *BUILDINFO_GIT; // = "62ecfa47df1223524a0600db68d79c4e60832b9f";


static void cmd_help ( void ) {
        const struct commands_st *cmds = commands;
        char *p = sprintf(COBUF, "Helper ROM: %s%s %s %s\r\nBuilt on: %s\r\n%s\r\nGIT: %s\r\n\r\n",
		SHORT_HELP, WINDOW_TITLE, VERSION, COPYRIGHT,
		BUILDINFO_ON, BUILDINFO_AT, BUILDINFO_GIT
	) + COBUF;
        while (cmds->cmd) {
                p += sprintf(p, "%s\t%s\r\n", cmds->cmd, cmds->help);
                cmds++;
        }
}


static void xep_exos_command_trap ( void )
{
	Uint8 c, b;
	Uint16 de;
	*COBUF = 0; // no ans by def
	c = z80ex_get_reg(z80, regBC) & 0xFF;
	b = z80ex_get_reg(z80, regBC) >> 8;
	de = z80ex_get_reg(z80, regDE);
	switch (c) {
		case 2: // EXOS command
			if (b == 3 && read_cpu_byte(de + 1) == 'X' && read_cpu_byte(de + 2) == 'E' && read_cpu_byte(de + 3) == 'P') {
				char *p = buffer;
				b = read_cpu_byte(de) - 3;
				de += 4;
				while (b) {
					c = read_cpu_byte(de++);
					b--;
					if (c == 9) c = 32;
					if (c < 32 || c > 127) continue;
					if (c >= 'A' && c <= 'Z') c += 32;
					if (p == buffer && c == 32) continue;
					if (p > buffer && c == 32 && p[-1] == 32) continue;
					*(p++) = (c == 32 ? 0 : c);
				}
				*p = 0;
				p[1] = 0;
				if (p == buffer) {
					sprintf(COBUF, "No sub-command was requested\r\n");
				} else {
					const struct commands_st *cmds = commands;
					c = 1;
					while (cmds->cmd) {
						if (!strcmp(cmds->cmd, buffer)) {
							//sprintf(buffer_out + strlen(buffer_out), "Found command: [%s]\r\n", cmds->cmd);
							carg = buffer + strlen(buffer) + 1;
							(cmds->handler)();
							c = 0;
							break;
						}
						cmds++;
					}
					if (c)
						sprintf(COBUF, "XEP: sub-command \"%s\" is unknown\r\n", buffer);
				}
				SET_A(0);
				SET_C(0);
			}
			break;
		case 3: // EXOS help
			if (b == 0)
				sprintf(COBUF, "%s", SHORT_HELP);
			else if (b == 3 && read_cpu_byte(de + 1) == 'X' && read_cpu_byte(de + 2) == 'E' && read_cpu_byte(de + 3) == 'P') {
				cmd_help();
				SET_A(0);
				SET_C(0);
			}
			break;
	}
	// set answer size for XEP ROM
	c = strlen(COBUF);
	if (c > 2045) {
		ERROR_WINDOW("FATAL: XEP ROM answer is too large, %d bytes.", c);
		exit(1);
	}
	*(Uint8*)(COBUF - 2) = c & 0xFF;
	*(Uint8*)(COBUF - 1) = c >> 8;
}


void xep_rom_trap ( Uint16 pc, Uint8 opcode )
{
	switch (opcode) {
		case 0xBC:
			xep_exos_command_trap();
			break;
		default:
			ERROR_WINDOW("FATAL: Unknown ED-trap opcode in XEP ROM: PC=%04Xh ED_OP=%02Xh", pc, opcode);
			exit(1);
	}
}

