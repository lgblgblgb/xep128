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


static FILE *fp = NULL;
static int fp_to_open = 1;

void printer_close ( void )
{
	if (fp) {
		fclose(fp);
		fprintf(stderr, "Closing printer output file.\n");
		fp_to_open = 1;
		fp = NULL;
	}
}

void printer_send_data ( Uint8 data )
{
	//fprintf(stderr, "PRINTER GOT DATA: %d\n", data);
	if (fp_to_open) {
		fp = fopen("print.out", "a");
		fp_to_open = 0;
	}
	if (fp)
		fprintf(fp, "%c", data);
}


