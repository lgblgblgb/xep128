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

#define BUFFER_SIZE 4096

static char buffer[BUFFER_SIZE];
static int buffer_pos = 0;
static char buffer_out[BUFFER_SIZE + 2];
static int buffer_out_pos = 0;

static const char *SHORT_HELP = "XEP   version 0.1  (Xep128 ROM)\r\n";


static void todo ( void )
{
}


static void cmd_ver ( void ) {
	sprintf(buffer_out, "%s", SHORT_HELP);
}

static void cmd_z180 ( void ) {
	z80ex_set_z180(z80, 1);
	sprintf(buffer_out, "CPU: set to Z180\r\n");
}

static void cmd_z80 ( void ) {
	z80ex_set_z180(z80, 0);
	sprintf(buffer_out, "CPU: set to Z80\r\n");
}

static void cmd_info ( void ) {
	sprintf(buffer_out, "CPU: %s %fMHz\r\n", z80ex_get_z180(z80) ? "Z180" : "Z80", CPU_CLOCK / 1000000.0);
}

static void cmd_cpu ( void ) {
	sprintf(buffer_out, "CPU: %s %fMHz\r\n", z80ex_get_z180(z80) ? "Z180" : "Z80", CPU_CLOCK / 1000000.0);
}


static void cmd_help ( void );

static const struct commands_st commands[] = {
	{ "reset",	"Resets EP", todo },
	{ "hreset",	"Hard-resets EP", todo },
	{ "clock",	"Set CPU clock (eg: 7.12)", todo },
	{ "z80", 	"Set Z80 emulation (instead of Z180)", cmd_z80 },
	{ "z180",	"Set partial/broken Z180 emulation", cmd_z180 },
	{ "info",	"Emulator information", cmd_info },
	{ "help",	"Help", cmd_help },
	{ "ver",	"Short help / version", cmd_ver },
	{ NULL,		NULL, NULL }
};

extern const char *BUILDINFO_ON; //  = "lgb@vega on Linux 3.19.0-15-generic";
extern const char *BUILDINFO_AT; //  = "Thu, 04 Jun 2015 22:56:28 +0200";
extern const char *BUILDINFO_GIT; // = "62ecfa47df1223524a0600db68d79c4e60832b9f";


static void cmd_help ( void ) {
        const struct commands_st *cmds = commands;
        char *p = sprintf(buffer_out, "Helper ROM: %s%s %s %s\r\nBuilt on: %s\r\n%s\r\nGIT: %s\r\n\r\n", 
		SHORT_HELP, WINDOW_TITLE, VERSION, COPYRIGHT,
		BUILDINFO_ON, BUILDINFO_AT, BUILDINFO_GIT
	) + buffer_out;
        while (cmds->cmd) {
                p += sprintf(p, "%s\t%s\r\n", cmds->cmd, cmds->help);
                cmds++;
        }
}



static void execute_command ()
{
	const struct commands_st *cmds = commands;
	char *p = strchr(buffer, 32);
	if (p) *(p++) = 0;
	

	sprintf(buffer_out, "Your command was: [%s]\r\nParameters were: [%s]\r\n", buffer, p ? p : "NOPE :)");
	
	while (cmds->cmd) {
		if (!strcmp(cmds->cmd, buffer)) {
			sprintf(buffer_out + strlen(buffer_out), "Found command: [%s]\r\n", cmds->cmd);
			(cmds->handler)();
			return;
		}
		cmds++;
	}
	sprintf(buffer_out, "XEP: sub-command \"%s\" is unknown\r\n", buffer);
}



void emurom_send ( Uint8 data )
{
	buffer_out_pos = 0;
	buffer_out[0] = 0;
	if (data == 0) {
		if (buffer_pos) {
			if (buffer_pos == BUFFER_SIZE) {
				buffer_pos = 0;
				sprintf(buffer_out, "ERROR: too long command, ignored.\r\n");
				return;
			}
			buffer[buffer[buffer_pos - 1] == 32 ? buffer_pos - 1 : buffer_pos] = 0;
			buffer_pos = 0;
			execute_command();
		} else {
			cmd_help();
		}
		return;
	}
	if (data == 9) data = 32;
	if (data < 32 || data > 127) return;
	if (buffer_pos == 0 && data == 32) return;
	if (buffer_pos && data == 32 && buffer[buffer_pos - 1] == 32) return;
	if (data >= 'A' && data <= 'Z') data += 32;
	if (buffer_pos < BUFFER_SIZE)
		buffer[buffer_pos++] = data;
}


Uint8 emurom_receive ( void )
{
	Uint8 result = buffer_out[buffer_out_pos];
	if (result) buffer_out_pos++;
	buffer_pos = 0;
	return result;
}

