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
#ifdef _WIN32
#include <sysinfoapi.h>
#endif


struct commands_st {
	const char *name;
	const char *alias;
	const int allowed;
	const char *help;
	void (*handler)(void);
};

static const char SHORT_HELP[] = "XEP   version " VERSION "  (Xep128 EMU)\n";
static const char TOO_LONG_OUTPUT_BUFFER[] = " ...%s*** Too long output%s";
static const char *_dave_ws_descrs[4] = {
	"all", "M1", "no", "no"
};

static volatile int is_queued_command = 0;
static char queued_command[256];

static char input_buffer[256];
static char *input_p;
static char *output_p;
static char *output_limit;
static const char *output_nl;

static Uint16 dump_addr1 = 0;
static Uint8  dump_addr2 = 0;
static int    dump_pagerel = 0;
static Uint16 disasm_addr1 = 0;
static Uint8  disasm_addr2 = 0;
static int    disasm_pagerel = 0;




#define MPRINTF(...) do {			\
	char m__buffer__[1024];			\
	snprintf(m__buffer__, sizeof m__buffer__, __VA_ARGS__);	\
	__mprintf_append_helper(m__buffer__);	\
} while(0)


static void __mprintf_append_helper ( char *s )
{
	while (*s) {
		if (output_p >= output_limit) {
			sprintf(output_limit - strlen(TOO_LONG_OUTPUT_BUFFER), TOO_LONG_OUTPUT_BUFFER, output_nl, output_nl);
			output_p = NULL;
			return;
		} else if (!output_p)
			return;
		if (*s == '\n') {
			const char *n = output_nl;
			while (*n)
				*(output_p++) = *(n++);
			s++;
		} else
			*(output_p++) = *(s++);
	}
	*output_p = '\0';
}


static char *get_mon_arg ( void )
{
	char *r;
	while (*input_p && *input_p <= 32)
		input_p++;
	if (!*input_p) 
		return NULL;		// no argument left
	r = input_p;			// remember position of first printable character ...
	while (*input_p > 32)
		input_p++;
	if (*input_p)
		*(input_p++) = '\0';	// terminate argument
	return r;
}



static int get_mon_arg_hex ( int *hex1, int *hex2 )
{
	char *h = get_mon_arg();
	*hex1 = *hex2 = -1;
	if (h == NULL)
		return 0;
	return sscanf(h, "%x:%x", hex1, hex2);
}



static void cmd_testargs ( void ) {
	int h1, h2, r;
	r = get_mon_arg_hex(&h1, &h2);
	MPRINTF("Tried to parse first arg as hex, result: r=%d, h1=%x, h2=%x\n",
		r, h1, h2
	);
	for (;;) {
		char *p = get_mon_arg();
		if (!p) {
			MPRINTF("No more args found!\n");
			break;
		}
		MPRINTF("Another arg found: \"%s\"\n", p);
	}
}


#define SEGMENT_OF_Z80_ADDR(n) ports[0xB0 | (((n) & 0xFFFF) >> 14)]


static void set_memaddr_by_arg ( Uint16 *da1, Uint8 *da2, int *dm )
{
	int h1, h2;
	get_mon_arg_hex(&h1, &h2);
	if (h1 >= 0)
		*da1 = h1;
	if (h2 >= 0) {
		*dm = 0;
		*da2 = h2;
		MPRINTF("; Absolute addresses from %04X:%02X\n", *da1, *da2);
	} else if (h1 >= 0) {
		MPRINTF("; CPU paging relative from %04X\n", *da1);
		*dm = 1;
	}
	if (*dm)
		*da2 = SEGMENT_OF_Z80_ADDR(*da1);
}



static void cmd_memdump ( void )
{
	int row;
	set_memaddr_by_arg(&dump_addr1, &dump_addr2, &dump_pagerel);
	for (row = 0; row < 10; row++) {
		int col;
		char asciibuf[17];
		MPRINTF("%04X:%c%02X  ",
			dump_addr1,
			dump_pagerel ? '*' : '=',
			dump_addr2
		);
		for (col = 0; col < 16; col++) {
			Uint8 byte = memory[(dump_addr2 << 14) | (dump_addr1 & 0x3FFF)];
			asciibuf[col] = (byte >= 32 && byte < 127) ? byte : '.';
			MPRINTF(" %02X", byte);
			dump_addr1++;
			if (!(dump_addr1 & 0x3FFF)) {
				if (dump_pagerel)
					dump_addr2 = SEGMENT_OF_Z80_ADDR(dump_addr1);
				else
					dump_addr2++;
			}
		}
		asciibuf[col] = 0;
		MPRINTF("  %s\n", asciibuf);
	}
}



static Z80EX_BYTE byte_reader ( Z80EX_WORD addr ) {
	if (disasm_pagerel)
		return memory[(SEGMENT_OF_Z80_ADDR(addr) << 14) | (addr & 0x3FFF)];
	else
		//return memory[((int)user_data + addr - disasm_addr1) & 0x3FFFFF];
		return memory[((disasm_addr2 << 14) + (disasm_addr1 & 0x3FFF) + (addr - disasm_addr1)) & 0x3FFFFF];
}



static void cmd_disasm ( void )
{
	int lines;
	set_memaddr_by_arg(&disasm_addr1, &disasm_addr2, &disasm_pagerel);
	for (lines = 0; lines < 10; lines++) {
		char dasm_out_buffer[128];
		char hex_out_buffer[32];
		char asc_out_buffer[16];
		int t_states, t_states2, r, h;
		//int disasm_base = (disasm_addr2 << 14) | (disasm_addr1 & 0x3FFF);
		char *p;
		r = z80ex_dasm(dasm_out_buffer, sizeof dasm_out_buffer, 0, &t_states, &t_states2, byte_reader, disasm_addr1);
		if (byte_reader(disasm_addr1) == 0xF7) {	// the EXOS call hack!
			h = byte_reader(disasm_addr1 + 1);
			r = 2;
			snprintf(dasm_out_buffer, sizeof dasm_out_buffer, "EXOS $%02X", h);
		}
		for (h = 0, hex_out_buffer[0] = 0, asc_out_buffer[0] = '\''; h < r; h++) {
			Uint8 byte = byte_reader(disasm_addr1 + h);
			sprintf(hex_out_buffer + h * 3, "%02X ", byte);
			asc_out_buffer[h + 1] = (byte >= 32 && byte < 127) ? byte : '.';
			asc_out_buffer[h + 2] = '\0';
		}
		strcat(asc_out_buffer, "'");
		p = strchr(dasm_out_buffer, ' ');
		if (p)
			*(p++) = '\0';
		MPRINTF("%04X:%c%02X   %-12s%-4s %-16s ; %-6s L=%d T=%d/%d\n",
			disasm_addr1,
			disasm_pagerel ? '*' : '=',
			disasm_addr2,
			hex_out_buffer,
			dasm_out_buffer,
			p ? p : "",
			asc_out_buffer,
			r, t_states, t_states2
		);
		if (disasm_addr1 >> 14 != ((disasm_addr1 + r) >> 14)) {
			if (disasm_pagerel)
				disasm_addr2 = SEGMENT_OF_Z80_ADDR(disasm_addr1 + r);
			else
				disasm_addr2++;
		}
		disasm_addr1 += r;
	}
}



static void cmd_registers ( void ) {
	MPRINTF(
		"AF =%04X BC =%04X DE =%04X HL =%04X IX=%04X IY=%04X F=%c%c%c%c%c%c%c%c IM=%d IFF=%d,%d\n"
		"AF'=%04X BC'=%04X DE'=%04X HL'=%04X PC=%04X SP=%04X Prefix=%02X P=%02X,%02X,%02X,%02X\n",
		Z80_AF,  Z80_BC,  Z80_DE,  Z80_HL,  Z80_IX, Z80_IY,
		(Z80_F & 0x80) ? 'S' : 's',
		(Z80_F & 0x40) ? 'Z' : 'z',
		(Z80_F & 0x20) ? '1' : '0',
		(Z80_F & 0x10) ? 'H' : 'h',
		(Z80_F & 0x08) ? '1' : '0',
		(Z80_F & 0x04) ? 'V' : 'v',
		(Z80_F & 0x02) ? 'N' : 'n',
		(Z80_F & 0x01) ? 'C' : 'c',
		Z80_IM, Z80_IFF1 ? 1 : 0, Z80_IFF2 ? 1 : 0,
		Z80_AF_, Z80_BC_, Z80_DE_, Z80_HL_, Z80_PC, Z80_SP,
		z80ex.prefix,
		ports[0xB0], ports[0xB1], ports[0xB2], ports[0xB3]
	);

}



static void cmd_setdate ( void ) {
	char buffer[64];
	xep_set_time_consts(buffer);
	MPRINTF("EXOS time set: %s\n", buffer);
}



static void cmd_ram ( void ) {
	char *arg = get_mon_arg();
	int r = arg ? *arg : 0;
	switch (r) {
		case 0:
			MPRINTF("%s\nDave: WS=%s CLK=%dMHz P=%02X/%02X/%02X/%02X\n\n",
				mem_desc,
				_dave_ws_descrs[(ports[0xBF] >> 2) & 3],
				ports[0xBF] & 1 ? 12 : 8,
				ports[0xB0], ports[0xB1], ports[0xB2], ports[0xB3]
			);
			break;
		case '!':
			INFO_WINDOW("Setting total sum of RAM size to %dKbytes\nEP will reboot now!\nYou can use :XEP EMU command then to check the result.", ep_set_ram_config(arg + 1) << 4);
			ep_reset();
			return;
		default:
			MPRINTF(
				"*** Bad command syntax.\nUse no parameter to query or !128 to set 128K memory, "
				"or even !@E0,E3-E5 (no spaces ever!) to specify given RAM segments. Using '!' is "
				"only for safity not to re-configure or re-boot your EP with no intent. Not so "
				"much error handling is done on the input!\n"
			);
			break;
	}
}



static void cmd_cpu ( void ) {
	//char buf[512] = "";
	char *arg = get_mon_arg();
	if (arg) {
		if (!strcasecmp(arg, "z80"))
			set_ep_cpu(CPU_Z80);
		else if (!strcasecmp(arg, "z80c"))
			set_ep_cpu(CPU_Z80C);
		else if (!strcasecmp(arg, "z180")) {
			set_ep_cpu(CPU_Z180);
			// Zozo's EXOS would set this up, but our on-the-fly change is something can't happen for real, thus we fake it here:
			z180_port_write(0x32, 0x00);
			z180_port_write(0x3F, 0x40);
		} else {
			int clk = atof(arg) * 1000000;
			if (clk < 1000000 || clk > 12000000)
				MPRINTF("*** Unknown CPU type to set or it's not a clock value either (1-12 is OK in MHz): %s\n", arg);
			else {
				INFO_WINDOW("Setting CPU clock to %.2fMhz",
					set_cpu_clock(clk) / 1000000.0
				);
			}
		}
	}
	MPRINTF("CPU : %s %s @ %.2fMHz\n",
		z80ex.z180 ? "Z180" : "Z80",
		z80ex.nmos ? "NMOS" : "CMOS",
		CPU_CLOCK / 1000000.0
	);
}



static void cmd_emu ( void )
{
	char buf[1024];
#ifdef _WIN32

	DWORD siz = sizeof buf;
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
	MPRINTF(
		"Run by: %s@%s %s %s\n"
		"Drivers: %s %s\n"
		"SDL c/l: %d.%d.%d %d.%d.%d\n"
		"Base path: %s\nPref path: %s\nStart dir: %s\nSD img: %s [%ldM]\n",
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
	INFO_WINDOW("XEP ROM/monitor command directs shutting down.");
	exit(0);
}



static void cmd_mouse ( void )
{
	char *arg = get_mon_arg();
	int c = arg ? *arg : 0;
	char buffer[256];
	switch (c) {
		case '1': case '2': case '3': case '4': case '5': case '6':
			mouse_setup(c - '0');
			break;
		case '\0':
			break;
		default:
			MPRINTF("*** Give values 1 ... 6 for mode, or no parameter for query.\n");
			return;
	}
	mouse_mode_description(0, buffer);
	MPRINTF("%s\n", buffer);
}



static void cmd_audio ( void )
{
	audio_init(1);	// NOTE: later it shouldn't be here!
	audio_start();
}



static void cmd_primo ( void )
{
	if (primo_rom_seg == -1) {
		MPRINTF("*** Primo ROM not found in the loaded ROM set.\n");
		return;
	}
	primo_emulator_execute();
}



static void cmd_showkeys ( void )
{
	show_keys = !show_keys;
	MPRINTF("SDL show keys info has been turned %s.\n", show_keys ? "ON" : "OFF");
}


static void cmd_close ( void )
{
	console_close_window();
}


static void cmd_romname ( void )
{
	MPRINTF("%s", SHORT_HELP);
}


static void cmd_exos ( void )
{
	if (exos_version == 0)
		MPRINTF("*** XEP ROM was not called yet\n");
	else {
		char status_line[41];
		exos_get_status_line(status_line);
		MPRINTF("EXOS version: %d.%d\nStatus line: %s\n", exos_version >> 4, exos_version & 0xF, status_line);
		MPRINTF("Working/non-working RAM segs: %d / %d\n", exos_info[5], exos_info[6]);
	}
}


static void cmd_lpt ( void )
{
	char *p = nick_dump_lpt("\n");
	__mprintf_append_helper(p);
	free(p);
}


static void cmd_pause ( void )
{
	paused = !paused;
	OSD("Emulation %s", paused ? "paused" : "resumed");
}


static void cmd_sdl ( void )
{
	SDL_RendererInfo info;
	SDL_Renderer *rendererp;
	int a;
	MPRINTF("Available SDL renderers:\n");
	for (a = 0; a < SDL_GetNumRenderDrivers(); a++ ) {
		int r = SDL_GetRenderDriverInfo(a, &info);
		if (r)
			MPRINTF("  (%d) *** CANNOT QUERY ***\n", a);
		else
			MPRINTF("  (%d) \"%s\"\n", a, info.name);
	}
	rendererp = SDL_GetRenderer(sdl_win);
	if (rendererp && !SDL_GetRendererInfo(rendererp, &info))
		MPRINTF("Used driver: \"%s\"\n", info.name);
}




static void cmd_help ( void );

static const struct commands_st commands[] = {
	{ "AUDIO",	"", 3, "Tries to turn lame audio emulation", cmd_audio },
	{ "CLOSE",	"", 3, "Close console/monitor window", cmd_close },
	{ "CPU",	"", 3, "Set/query CPU type/clock", cmd_cpu },
	{ "DISASM",	"D", 3, "Disassembly memory", cmd_disasm },
	{ "EMU",	"", 3, "Emulation info", cmd_emu },
	{ "EXIT",	"", 3, "Exit Xep128", cmd_exit },
	{ "EXOS",	"", 3, "EXOS information", cmd_exos },
	{ "HELP",	"?", 3, "Guess, what ;-)", cmd_help },
	{ "LPT",	"", 3, "Shows LPT (can be long!)", cmd_lpt },
	{ "MEMDUMP",	"M", 3, "Memory dump", cmd_memdump },
	{ "MOUSE",	"", 3, "Configure or query mouse mode", cmd_mouse },
	{ "PAUSE",	"", 2, "Pause/resume emulation", cmd_pause },
	{ "PRIMO",	"", 3, "Primo emulation", cmd_primo },
	{ "RAM",	"", 3, "Set RAM size/report", cmd_ram },
	{ "REGS",	"R", 3, "Show Z80 registers", cmd_registers },
	{ "ROMNAME",	"", 3, "ROM id string", cmd_romname },
	{ "SDL",        "", 3,  "Get SDL related info", cmd_sdl },
	{ "SETDATE",	"", 1, "Set EXOS time/date by emulator" , cmd_setdate },
	{ "SHOWKEYS",	"", 3, "Show/hide PC/SDL key symbols", cmd_showkeys },
	{ "TESTARGS",   "", 3, "Just for testing monitor statement parsing, not so useful for others", cmd_testargs },
	{ NULL,		NULL, 0, NULL, NULL }
};
static const char help_for_all_desc[] = "\nFor help on all comamnds: (:XEP) HELP\n";



static void cmd_help ( void ) {
        const struct commands_st *cmds = commands;
	char *arg = get_mon_arg();
	if (arg) {
		while (cmds->name) {
			if ((!strcasecmp(arg, cmds->name) || !strcasecmp(arg, cmds->alias)) && cmds->help) {
				MPRINTF("%s: [%s] %s%s",
					cmds->name,
					cmds->alias[0] ? cmds->alias : "-",
					cmds->help,
					help_for_all_desc
				);
				return;
			}
			cmds++;
		}
		MPRINTF("*** No help/command found '%s'%s", arg, help_for_all_desc);
	} else {
	        MPRINTF("Helper ROM: %s%s %s %s\nBuilt on: %s\n%s\nGIT: %s\nCompiler: %s %s\n\nCommands:",
			SHORT_HELP, WINDOW_TITLE, VERSION, COPYRIGHT,
			BUILDINFO_ON, BUILDINFO_AT, BUILDINFO_GIT, CC_TYPE, BUILDINFO_CC
		);
		while (cmds->name) {
			if (cmds->help)
				MPRINTF(" %s%s%s%s", cmds->name,
					cmds->alias[0] ? "[" : "",
					cmds->alias,
					cmds->alias[0] ? "]" : ""
				);
			cmds++;
		}
		MPRINTF("\n\nFor help on a command: (:XEP) HELP CMD\n");
	}
}



void monitor_execute ( char *in_input_buffer, int in_source, char *in_output_buffer, int in_output_max_size, const char *in_output_nl )
{
	char *cmd_name;
	const struct commands_st *cmds = commands;
	/* initialize output parameters for the answer */
	output_p = in_output_buffer;
	*output_p = '\0';
	output_limit = output_p + in_output_max_size;
	output_nl = in_output_nl;
	/* input pre-stuffs */
	strcpy(input_buffer, in_input_buffer);
	input_p = input_buffer;
	/* OK, now it's time to do something ... */
	cmd_name = get_mon_arg();
	if (!cmd_name) {
		if (in_source == 1)
			MPRINTF("*** Use: XEP HELP\n");
		return;	// empty command line
	}
	while (cmds->name) {
		if (!strcasecmp(cmds->name, cmd_name) || !strcasecmp(cmds->alias, cmd_name)) {
			if (cmds->allowed & in_source)
				return (void)(cmds->handler)();
			else {
				MPRINTF("*** Command cannot be used here: %s\n", cmd_name);
				return;
			}
		}
		cmds++;
	}
	MPRINTF("*** Unknown command: %s\n", cmd_name);
}



/* this should be called by the emulator (main thread) regularly, to check console/monitor events queued by the console/monitor thread */
void monitor_process_queued ( void )
{
	if (is_queued_command) {
		char buffer[8192];
		monitor_execute(queued_command, 2, buffer, sizeof buffer, NL);
		printf("%s", buffer);	// TODO, maybe we should ask for sync on stdout?
		is_queued_command = 0;
	}
}


/* for use by the console input thread */
int monitor_queue_used ( void )
{
	return is_queued_command;
}


/* for use by the console input thread */
int monitor_queue_command ( char *buffer )
{
	if (is_queued_command)
		return 1;
	strcpy(queued_command, buffer);
	is_queued_command = 1;
	return 0;
}
