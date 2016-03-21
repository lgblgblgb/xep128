/* Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
   Copyright (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
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

#include "xep_rom_syms.h"

struct commands_st {
	const char *cmd;
	const char *help;
	void (*handler)(void);
};

static char buffer[256];
static char *carg;

static const char *SHORT_HELP = "XEP   version " VERSION "  (Xep128 EMU)\r\n";

#define COBUF ((char*)(memory + xep_rom_addr + 0x3802))

static const char *_dave_ws_descrs[4] = {
	"all", "M1", "no", "no"
};



static void cmd_ram ( void ) {
	switch (*carg) {
		case 0:
			sprintf(COBUF, "%s\r\nDave: WS=%s CLK=%dMHz P=%02X/%02X/%02X/%02X\r\n\r\n",
				mem_desc,
				_dave_ws_descrs[(ports[0xBF] >> 2) & 3],
				ports[0xBF] & 1 ? 12 : 8,
				ports[0xB0], ports[0xB1], ports[0xB2], ports[0xB3]
			);
			break;
		case '!':
			INFO_WINDOW("Setting total sum of RAM size to %dKbytes\nEP will reboot now!\nYou can use :XEP EMU command then to check the result.", ep_set_ram_config(carg + 1) << 4);
			ep_reset();
			return;
		default:
			sprintf(COBUF,
				"*** Bad command syntax.\r\nUse no parameter to query or !128 to set 128K memory, "
				"or even !@E0,E3-E5 (no spaces ever!) to specify given RAM segments. Using '!' is "
				"only for safity not to re-configure or re-boot your EP with no intent. Not so "
				"much error handling is done on the input!\r\n"
			);
			break;
	}
}


static void cmd_cpu ( void ) {
	char buf[512] = "";
	if (*carg) {
		if (!strcmp(carg, "z80"))
			set_ep_cpu(CPU_Z80);
		else if (!strcmp(carg, "z80c"))
			set_ep_cpu(CPU_Z80C);
		else if (!strcmp(carg, "z180")) {
			set_ep_cpu(CPU_Z180);
			// Zozo's EXOS would set this up, but our on-the-fly change is something can't happen for real, thus we fake it here:
			z180_port_write(0x32, 0x00);
			z180_port_write(0x3F, 0x40);
		} else {
			int clk = atof(carg) * 1000000;
			if (clk < 1000000 || clk > 12000000)
				sprintf(buf, "*** Unknown CPU type to set or it's not a clock value either (1-12 is OK in MHz): %s\r\n", carg);
			else {
				INFO_WINDOW("Setting CPU clock to %.2fMhz",
					set_cpu_clock(clk) / 1000000.0
				);
			}
		}
	}
	sprintf(COBUF, "%sCPU : %s %s @ %.2fMHz\r\n",
		buf,
		z80ex.z180 ? "Z180" : "Z80",
		z80ex.nmos ? "NMOS" : "CMOS",
		CPU_CLOCK / 1000000.0
	);
}


#ifdef _WIN32
// /usr/i686-w64-mingw32/include/sysinfoapi.h:
//#define SECURITY_WIN32
#include "sysinfoapi.h"
//#include "secext.h"
#endif

static void cmd_emu ( void )
{
	char buf[1024];
#ifdef _WIN32
	int siz = sizeof buffer;
#endif
	//SDL_VERSION(&sdlver_c);
	//SDL_GetVersion(&sdlver_l);
#ifdef _WIN32
	//GetUserName(buf, &siz);
	GetComputerNameEx(ComputerNamePhysicalNetBIOS, buf, &siz);
#define OS_KIND "Win32"
#else
	gethostname(buf, sizeof buf);
#define OS_KIND "POSIX"
#endif
	sprintf(COBUF, "Run by: %s@%s %s %s\r\nDrivers: %s %s\r\nSDL c/l: %d.%d.%d %d.%d.%d\r\nBase path: %s\r\nPref path: %s\r\nStart dir: %s\r\nSD img: %s [%ldM]\r\n",
#ifdef _WIN32
		getenv("USERNAME"),
#else
		getenv("USER"),
#endif
		buf, OS_KIND, SDL_GetPlatform(), SDL_GetCurrentVideoDriver(), SDL_GetCurrentAudioDriver(),
		sdlver_compiled.major, sdlver_compiled.minor, sdlver_compiled.patch,
		sdlver_linked.major, sdlver_linked.minor, sdlver_linked.patch,
		app_base_path, app_pref_path, current_directory,
		sdimg_path, sd_card_size >> 20
	);
}


static void cmd_exit ( void )
{
	INFO_WINDOW("XEP ROM command directs shutting down.");
	exit(0);
}


static void cmd_mouse ( void )
{
	int c = *carg;
	switch (c) {
		case '1': case '2': case '3': case '4': case '5': case '6':
			mouse_setup(c - '0');
			break;
		case '\0':
			break;
		default:
			sprintf(COBUF, "*** Give values 1 ... 6 for mode, or no parameter for query.\r\n");
			return;
	}
	mouse_mode_description(0, buffer);
	sprintf(COBUF, "%s\r\n", buffer);
}


static void cmd_audio ( void )
{
	audio_init(1);	// NOTE: later it shouldn't be here!
	audio_start();
}

static void cmd_primo ( void )
{
	if (primo_rom_seg == -1) {
		sprintf(COBUF, "*** Primo ROM not found in the loaded ROM set.\r\n");
		return;
	}
	primo_emulator_execute();
}


static void cmd_showkeys ( void )
{
	show_keys = !show_keys;
	sprintf(COBUF, "SDL show keys info has been turned %s.\r\n", show_keys ? "ON" : "OFF");
}


static void cmd_help ( void );

static const struct commands_st commands[] = {
	{ "audio",	"Tries to turn lame audio emulation", cmd_audio },
	{ "cpu",	"Set/query CPU type/clock", cmd_cpu },
	{ "ram",        "Set RAM size/report", cmd_ram },
	{ "emu",	"Emulation info", cmd_emu },
	{ "mouse",	"Configure or query mouse mode", cmd_mouse },
	{ "help",	"This help screen", cmd_help },
	{ "exit",	"Exit Xep128", cmd_exit },
	{ "primo",	"Primo emulation", cmd_primo },
	{ "showkeys",	NULL, cmd_showkeys },
	{ NULL,		NULL, NULL }
};


static void cmd_help ( void ) {
        const struct commands_st *cmds = commands;
        char *p = sprintf(COBUF, "Helper ROM: %s%s %s %s\r\nBuilt on: %s\r\n%s\r\nGIT: %s\r\nCompiler: %s %s\r\n\r\n",
		SHORT_HELP, WINDOW_TITLE, VERSION, COPYRIGHT,
		BUILDINFO_ON, BUILDINFO_AT, BUILDINFO_GIT, CC_TYPE, BUILDINFO_CC
	) + COBUF;
        while (cmds->cmd) {
		if (cmds->help)
	                p += sprintf(p, "%s\t%s\r\n", cmds->cmd, cmds->help);
                cmds++;
        }
}


static int exos_cmd_name_match ( const char *that, Uint16 addr )
{
	if (strlen(that) != Z80_B) return 0;
	while (*that)
		if (*(that++) != read_cpu_byte(addr++))
			return 0;
	return 1;
}


static void xep_exos_command_trap ( void )
{
	Uint8 c = Z80_C, b = Z80_B;
	Uint16 de = Z80_DE;
	*COBUF = 0; // no ans by def
	DEBUG("XEP: TRAP: C=%02Xh, B=%02Xh, DE=%04Xh" NL, c, b, de);
	switch (c) {
		case 2: // EXOS command
			if (exos_cmd_name_match("XEP", de + 1)) {
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
				Z80_A = 0;
				Z80_C = 0;
			}
			break;
		case 3: // EXOS help
			if (b == 0) {
				sprintf(COBUF, "%s", SHORT_HELP);
				Z80_A = 0;
			} else if (exos_cmd_name_match("XEP", de + 1)) {
				cmd_help();
				Z80_A = 0;
				Z80_C = 0;
			}
			break;
	}
	// set answer size for XEP ROM
	de = strlen(COBUF);
	if (de)
		DEBUG("XEP ANSWER [%d bytes] = \"%s\"" NL, de, COBUF);
	if (de > 2045) {
		ERROR_WINDOW("FATAL: XEP ROM answer is too large, %d bytes.", de);
		exit(1);
	}
	*(Uint8*)(COBUF - 2) = de & 0xFF;
	*(Uint8*)(COBUF - 1) = de >> 8;
}


void xep_rom_trap ( Uint16 pc, Uint8 opcode )
{
	DEBUG("XEP: ROM trap at PC=%04Xh OPC=%02Xh" NL, pc, opcode);
	switch (opcode) {
		case 0xBC:
			xep_exos_command_trap();
			break;
		default:
			ERROR_WINDOW("FATAL: Unknown ED-trap opcode in XEP ROM: PC=%04Xh ED_OP=%02Xh", pc, opcode);
			exit(1);
	}
}

