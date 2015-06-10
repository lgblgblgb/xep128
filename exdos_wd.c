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

#ifdef CONFIG_EXDOS_SUPPORT

Uint8 wd_sector, wd_track;
static Uint8 wd_status, wd_data, wd_command, wd_interrupt, wd_DRQ;
static Uint8 *disk = NULL;
static int driveSel, diskSide, diskInserted;

#define WDINT_ON	0x3E
#define WDINT_OFF	0x3C
#define WDDRQ		2
// bit mask to set in WD status in case of seek error [spin-up-completed, seek-error, CRC]
#define SEEK_ERROR 	(32 | 16 | 8)
// bit mask to set WD status in case of seek OK  [spin-up-completed]
#define SEEK_OK		32


static const int disk_formats[][2] = {
	{40,  8},
	{40,  9},
	{80,  8},
	{80,  9},
	{80, 15},
	{80, 18},
	{80, 21},
	{82, 21},
	{80, 36},
	{-1, -1}
};
static int max_tracks, max_sectors;


int wd_attach_disk_image ( char *fn )
{
	int a;
	int max = -1, min = 999999999;
	for (a = 0; disk_formats[a][0] != -1; a++) {
		int j = (disk_formats[a][0] * disk_formats[a][1]) << 10;
		if (j < min) min = j;
		if (j > max) max = j;
	}
}


void wd_exdos_reset ( void )
{
	wd_track = 0;
	wd_sector = 0;
	wd_status = 4; // track 0 flag is on at initialization
	wd_data = 0;
	wd_command = 0xD0; // fake last command as force interrupt
	wd_interrupt = WDINT_OFF; // interrupt output is OFF
	wd_DRQ = 0; // no DRQ (data request)
	driveSel = (disk != NULL); // drive is selected by default if there is disk image!
	diskSide = 0;
	diskInserted = (disk == NULL) ? 1 : 0; // 0 means inserted disk, 1 means = not inserted
	printf("WD: reset\n");
}


Uint8 wd_read_status ( void )
{
	wd_interrupt = WDINT_OFF; // on reading WD status, interrupt is reset!
	return 128 | wd_status | wd_DRQ; // motor is always on, the given wdStatus bits, DRQ handled separately (as we need it for exdos status too!)
}

Uint8 wd_read_data ( void )
{
	if (wd_DRQ) {
		wd_data = wdBuffer[wdBufferPos++];
		if ((--wdBufferSize) == 0)
			wd_DRQ = 0; // end of data, switch of DRQ!
	}
	return wd_data;
}

Uint8 wd_read_exdos_status ( void )
{
	return wd_interrupt | (wd_DRQ << 6) | diskInserted | 0x40; // 0x40 -> disk not changed
}

void wd_send_command ( Uint8 value )
{
	wd_command = value;
	wd_DRQ = 0;
	wd_interrupt = WDINT_OFF;
	switch (value >> 4) {
		case  0: // restore (type I), seeks to track zero
			if (driveSel) {
				wd_status = 4 | SEEK_OK; // 4->set track0
				wd_track = 0;
			} else
				wd_status = SEEK_ERROR | 4; // set track0 flag (4) is needed here not to be mapped A: as B: by EXDOS, or such?!
			wd_interrupt = WDINT_ON;
			break;
		case  1: // seek (type I)
			if (wd_data < MAX_TRACKS && driveSel) {
				wd_track = wd_data;
				wd_status = SEEK_OK;
			} else
				wd_status = SEEK_ERROR;
			wd_interrupt = WDINT_ON;
			if (!wd_track) wd_status |= 4;
			break;
		case 13: // force interrupt (type IV)
			if (value & 15) wd_interrupt = WDINT_ON;
			wd_status = (wd_track == 0) ? 4 : 0;
                        break;
		default:
			printf("WD: unknown command: %d\n", value);
			wd_status = 4 | 8 | 16 | 64; // unimplemented command results in large set of problems reported :)
			wd_interrupt = WDINT_ON;
			break;
	}
}

void wd_write_data (Uint8 value)
{
	wd_data = value;
}

void wd_set_exdos_control (Uint8 value)
{
	driveSel = (disk != NULL) && ((value & 15) == 1);
	diskSide = (value >> 4) & 1;
	diskInserted = driveSel ? 0 : 1;
}

#else
#warning "EXDOS/WD support is not compiled in / not ready"
#endif

