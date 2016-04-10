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

#include "xep128.h"
#include "fileio.h"
#include "z80.h"
#include "cpu.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>


#define EXOS_USER_SEGMAP_P (0X3FFFFC + memory)

char fileio_cwd[PATH_MAX + 1];
static int exos_channels[0x100];



void fileio_init ( const char *dir, const char *subdir )
{
	int a;
	for (a = 0; a < 0x100; a++)
		exos_channels[a] = -1;
	if (dir && *dir && *dir != '?') {
		strcpy(fileio_cwd, dir);
		if (subdir) {
			strcat(fileio_cwd, subdir);
			if (subdir[strlen(subdir) - 1] != DIRSEP[0])
				strcat(fileio_cwd, DIRSEP);
		}
		DEBUGPRINT("FILEIO: base directory is: %s" NL, fileio_cwd);
		mkdir(fileio_cwd
#ifndef _WIN32
			, 0777
#endif
		);
	} else
		fileio_cwd[0] = '\0';
}


/* Opens a file on host-OS/FS side.
   Note: EXOS is case-insensitve on file names.
   Host OS FS "under" Xep128 may (Win32) or may not (UNIX) be case sensitve, thus we walk over the directory and tries to find matching file name.
*/
static int open_host_file ( const char *dirname, const char *filename, int create )
{
	DIR *dir;
	struct dirent *entry;
	int mode = (create ? O_TRUNC | O_CREAT | O_RDWR : O_RDONLY) | O_BINARY;
	dir = opendir(dirname);
	if (!dir)
		return -1;
	while ((entry = readdir(dir))) {
		if (!strcasecmp(entry->d_name, filename)) {
			char buffer[PATH_MAX + 1];
			closedir(dir);
			snprintf(buffer, sizeof buffer, "%s%s%s", dirname, DIRSEP, entry->d_name);
			DEBUGPRINT("FILEIO: opening file \"%s\"" NL, buffer);
			return open(buffer, mode, 0666);
		}
	}
	closedir(dir);
	if (create) {
		char buffer[PATH_MAX + 1];
		snprintf(buffer, sizeof buffer, "%s%s%s", dirname, DIRSEP, filename);
		return open(buffer, mode, 0666);
	}
	return -1;
}


static void get_file_name ( char *p )
{
	int de = Z80_DE;
	int len = read_cpu_byte(de);
	while (len--)
		*(p++) = tolower(read_cpu_byte(++de));
	*p = '\0';
}


void fileio_func_open_or_create_channel ( int create )
{
	int r;
	char fnbuf[256];
	if (exos_channels[Z80_A] >= 0) {
		Z80_A = 0xF9;	// channel number is already used! (maybe it's useless to be tested, as EXOS wouldn't allow that anyway?)
		return;
	}
	get_file_name(fnbuf);
	r = open_host_file(fileio_cwd, fnbuf, create);
	DEBUGPRINT("FILEIO: %s channel #%d result = %d filename = \"%s\" (in \"%s\")" NL, create ? "create" : "open", Z80_A, r, fnbuf, fileio_cwd);
	if (r < 0)
		Z80_A = 0xCF;	// file not found, but this is an EXDOS error code for real! also TODO for creation it's odd error ...
	else {
		exos_channels[Z80_A] = r;
		Z80_A = 0;
	}
}


void fileio_func_close_channel ( void )
{
	if (exos_channels[Z80_A] < 0)
		Z80_A = 0xFB;	// invalid channel
	else {
		close(exos_channels[Z80_A]);
		exos_channels[Z80_A] = -1;
		Z80_A = 0;
	}
}


void fileio_func_read_block ( void )
{
	int channel_fd = exos_channels[Z80_A], rb;
	Uint8 buffer[0xFFFF], *p;
	if (channel_fd < 0) {
		Z80_A = 0xFB;	// invalid channel
		return;
	}
	Z80_A = 0;
	rb = 0;
	while (Z80_BC) {
		int r = read(channel_fd, buffer + rb, Z80_BC);
		if (r > 0) {
			rb += r;
			Z80_BC -= r;
		} else if (!r) {
			Z80_A = 0xE4;	// attempt to read after end of file
			break;
		} else {
			Z80_A = 0xD0;	// somewhat funny error code for here: casette CRC error :)
			break;
		}
	}
	p = buffer;
	while (rb--)
		write_cpu_byte_by_segmap(Z80_DE++, EXOS_USER_SEGMAP_P, *(p++));
}


void fileio_func_read_character ( void )
{
	if (exos_channels[Z80_A] < 0)
		Z80_A = 0xFB;	// invalid channel
	else {
		int r = read(exos_channels[Z80_A], &Z80_B, 1);
		if (r == 1)
			Z80_A = 0;
		else if (!r)
			Z80_A = 0xE4;	// attempt to read after end of file
		else
			Z80_A = 0xD0;	// somewhat funny error code for here: casette CRC error :)
	}
}



void fileio_func_write_block ( void )
{
	int channel_fd = exos_channels[Z80_A], wb, de;
	Uint8 buffer[0xFFFF], *p;
	if (channel_fd < 0) {
		Z80_A = 0xFB;	// invalid channel
		return;
	}
	wb = Z80_BC;
	p = buffer;
	de = Z80_DE;
	Z80_A = 0;
	while (wb--)
		*(p++) = read_cpu_byte_by_segmap(de++, EXOS_USER_SEGMAP_P);
	p = buffer;
	while (Z80_BC) {
		int r = write(channel_fd, p, Z80_BC);
		if (r > 0) {
			Z80_BC -= r;
			Z80_DE += r;
		} else if (!r) {
			Z80_A = 0xE4;	// TODO: no write, "read after end of file" answer is odd ... what should be?!
			break;
		} else {
			Z80_A = 0xD0;	// somewhat funny error code for here: casette CRC error :)
		}
	}
}


void fileio_func_write_character ( void )
{
	if (exos_channels[Z80_A] < 0)
		Z80_A = 0xFB;	// invalid channel
	else {
		int r = write(exos_channels[Z80_A], &Z80_B, 1);
		if (r == 1)
			Z80_A = 0;
		else
			Z80_A = 0xD0;	// somewhat funny error code for here: casette CRC error :)
	}
}


void fileio_func_channel_read_status ( void )
{
	Z80_A = 0xE7;
}



void fileio_func_set_channel_status ( void )
{
	Z80_A = 0xE7;
}



void fileio_func_special_function ( void )
{
	Z80_A = 0xE7;
}



void fileio_func_init ( void )
{
	int a;
	for (a = 0; a < 0x100; a++)
		if (exos_channels[a] != -1) {
			close(exos_channels[a]);
			exos_channels[a] = -1;
		}
}


void fileio_func_buffer_moved ( void )
{
	Z80_A = 0xE7;
}



void fileio_func_destroy_channel ( void )
{
	Z80_A = 0xE7;
}



void fileio_func_not_used_call ( void )
{
	Z80_A = 0xE7;
}
