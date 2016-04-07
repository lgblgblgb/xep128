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



static int fileio_open_existing_hostfile ( const char *dirname, const char *filename, int mode )
{
	DIR *dir;
	struct dirent *entry;
	dir = opendir(dirname);
	if (!dir)
		return -1;
	while ((entry = readdir(dir))) {
		if (!strcasecmp(entry->d_name, filename)) {
			char buffer[PATH_MAX + 1];
			closedir(dir);
			snprintf(buffer, sizeof buffer, "%s%s%s", dirname, DIRSEP, entry->d_name);
			DEBUGPRINT("FILEIO: opening file \"%s\"" NL, buffer);
			return open(buffer, mode | O_BINARY);
		}
	}
	closedir(dir);
	return -1;
}



Uint8 fileio_func_open_channel ( void )
{
	int channel = Z80_A;
	int de = Z80_DE;
	int fnlen = read_cpu_byte(de);
	char fnbuf[128], *p = fnbuf;
	if (exos_channels[channel] != -1)
		return 0xF9;	// channel number is already used! (maybe it's useless to be tested, as EXOS wouldn't allow that anyway?)
	while (fnlen--)
		*(p++) = read_cpu_byte(++de);
	*p = '\0';
	DEBUGPRINT("Filename = \"%s\" Channel = %d O_BINARY=%d" NL, fnbuf, Z80_A, O_BINARY);
	de = fileio_open_existing_hostfile(fileio_cwd, fnbuf, O_RDONLY);
	DEBUGPRINT("Result of open: %d (cwd=%s)" NL, de, fileio_cwd);
	if (de < 0)
		return 0xCF;	// file not found, but this is an EXDOS error code for real!
	exos_channels[channel] = de;
	return 0;
}


Uint8 fileio_func_close_channel ( void )
{
	int channel = Z80_A;
	int channel_fd = exos_channels[channel];
	if (channel_fd == -1)
		return 0xFB;	// invalid channel
	close(channel_fd);
	exos_channels[channel] = -1;
	return 0;		// OK, closed.
}



static ssize_t safe_read ( int fd, void *buffer, size_t requested )
{
	ssize_t done = 0, r;
	do {
		r = read(fd, buffer, requested);
		DEBUGPRINT("FILEIO: safe_read: fd=%d, requested=%d, result=%d" NL, fd, requested, r);
		if (r > 0) {
			requested -= r;
			buffer += r;
			done += r;
		}
	} while (requested && r > 0);
	return r < 0 ? r : done;
}



Uint8 fileio_func_read_block ( void )
{
	int r, channel_fd = exos_channels[Z80_A];
	Uint8 fileio_buffer[0xFFFF], *p = fileio_buffer;
	if (channel_fd == -1)
		return 0xFB;	// invalid channel
	if (!Z80_BC)
		return 0;	// read to zero amount of data should be handled normally!
	r = safe_read(channel_fd, fileio_buffer, Z80_BC);
	DEBUGPRINT("FILEIO: HOST-read(block): fd=%d, requested_bytes=%d result=%d" NL, channel_fd, Z80_BC, r);
	if (!r) 		// attempt to read after end of file?
		return 0xE4;
	else if (r < 0)		// host file read error!
		return 0xD0;	// somewhat funny error code for here: casette CRC error :)
	while (r--) {
		write_cpu_byte_by_segmap(Z80_DE++, EXOS_USER_SEGMAP_P, *(p++));
		Z80_BC--;
		if (!Z80_DE && r)
			return 0x9B;	// OV64K, this is an EXDOS error for real ...
	}
	return 0;
}


Uint8 fileio_func_read_character ( void )
{
	int r;
	Uint8 fileio_buffer;
	int channel_fd = exos_channels[Z80_A];
	if (channel_fd == -1)
		return 0xFB;	// invalid channel
	r = safe_read(channel_fd, &fileio_buffer, 1);
	DEBUGPRINT("FILEIO: HOST-read(character): fd=%d, result=%d" NL, channel_fd, r);
	if (!r) 		// attempt to read after end of file?
		return 0xE4;
	else if (r < 0)		// host file read error!
		return 0xD0;	// somewhat funny error code for here: casette CRC error :)
	Z80_B = fileio_buffer;
	return 0;
}


Uint8 fileio_func_not_used_call ( void )
{
	return 0xE7;
}


Uint8 fileio_func_write_block ( void )
{
        return 0xE7;
}



Uint8 fileio_func_channel_read_status ( void )
{
        return 0xE7;
}



Uint8 fileio_func_set_channel_status ( void )
{
        return 0xE7;
}



Uint8 fileio_func_special_function ( void )
{
        return 0xE7;
}



Uint8 fileio_func_init ( void )
{
	int a;
	for (a = 0; a < 0x100; a++)
		if (exos_channels[a] != -1) {
			close(exos_channels[a]);
			exos_channels[a] = -1;
		}
        return 0;
}



Uint8 fileio_func_buffer_moved ( void )
{
        return 0xE7;
}



Uint8 fileio_func_destroy_channel ( void )
{
        return 0xE7;
}



Uint8 fileio_func_create_channel ( void )
{
        return 0xE7;
}



Uint8 fileio_func_write_character ( void )
{
        return 0xE7;
}
