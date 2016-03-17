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


static FILE *fp = NULL;
static int fp_to_open = 1;

#define BUFFER_SIZE 1024

static Uint8 buffer[BUFFER_SIZE];
static int buffer_pos;


static void write_printer_buffer ( void )
{
	if (buffer_pos && fp != NULL) {
		if (fwrite(buffer, buffer_pos, 1, fp) != 1) {
			WARNING_WINDOW("Cannot write printer output: %s\nFurther printer I/O has been disabled.", ERRSTR());
			fclose(fp);
			fp = NULL;
		}
	}
	buffer_pos = 0;
}


void printer_close ( void )
{
	if (fp) {
		write_printer_buffer();
		fclose(fp);
		DEBUG("Closing printer output file." NL);
		fp_to_open = 1;
		fp = NULL;
	}
}


void printer_send_data ( Uint8 data )
{
	//DEBUG("PRINTER GOT DATA: %d" NL, data);
	if (fp_to_open) {
		const char *printfile = config_getopt_str("printfile");
		char path[PATH_MAX + 1];
		fp = open_emu_file(printfile, "ab", path);
		if (fp == NULL)
			WARNING_WINDOW("Cannot create/append printer output file \"%s\": %s.\nYou can use Xep128 but printer output will not be logged!", path, ERRSTR());
		else
			INFO_WINDOW("Printer event, file \"%s\" has been opened for the output.", path);
		fp_to_open = 0;
		buffer_pos = 0;
	}
	if (fp != NULL) {
		buffer[buffer_pos++] = data;
		if (buffer_pos == BUFFER_SIZE)
			write_printer_buffer();
		// fprintf(fp, "%c", data);
	}
}

