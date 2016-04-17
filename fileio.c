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
#include "emu_rom_interface.h"
#include "gui.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>


#define EXOS_USER_SEGMAP_P (0X3FFFFC + memory)
#define HOST_OS_STR "Host OS "

char fileio_cwd[PATH_MAX + 1];
static int exos_channels[0x100];
static int channel = 0;



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


static void fileio_host_errstr ( void )
{
	char buffer[65];
	snprintf(buffer, sizeof buffer, HOST_OS_STR "%s", ERRSTR());
	xep_set_error(buffer);
}


/* Opens a file on host-OS/FS side.
   Note: EXOS is case-insensitve on file names.
   Host OS FS "under" Xep128 may (Win32) or may not (UNIX) be case sensitve, thus we walk through the directory with tring to find matching file name.
   Argument "create" must be non-zero for create channel call, otherwise it should be zero.
*/
static int open_host_file ( const char *dirname, const char *filename, int create )
{
	DIR *dir;
	struct dirent *entry;
	int mode = (create ? O_TRUNC | O_CREAT | O_RDWR : O_RDONLY) | O_BINARY;
	dir = opendir(dirname);
	if (!dir) {
		//xep_set_error("Cannot open host dir");
		return -1;
	}
	while ((entry = readdir(dir))) {
		if (!strcasecmp(entry->d_name, filename)) {
			int ret;
			char buffer[PATH_MAX + 1];
			closedir(dir);
			snprintf(buffer, sizeof buffer, "%s%s%s", dirname, DIRSEP, entry->d_name);
			DEBUGPRINT("FILEIO: opening file \"%s\"" NL, buffer);
			ret = open(buffer, mode, 0666);
			if (ret < 0) {
				fileio_host_errstr();
				DEBUGPRINT("FILEIO: open in directory walk failed: %s" NL, ERRSTR());
			}
			return ret;
		}
	}
	closedir(dir);
	if (create) {
		int ret;
		char buffer[PATH_MAX + 1];
		snprintf(buffer, sizeof buffer, "%s%s%s", dirname, DIRSEP, filename);
		ret = open(buffer, mode, 0666);
		if (ret < 0) {
			fileio_host_errstr();
			DEBUGPRINT("FILEIO: open in create case for new file failed: %s" NL, ERRSTR());
		}
		return ret;
	}
	xep_set_error(HOST_OS_STR "File not found");
	DEBUGPRINT("FILE: open at last restort failed" NL);
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


void fileio_func_open_channel_remember ( void )
{
	channel = Z80_A;
}


// channel number is set up with fileio_func_open_channel_remember() *before* this call! from XEP ROM separated trap!
void fileio_func_open_or_create_channel ( int create )
{
	int r;
	char fnbuf[PATH_MAX + 1];
	if (exos_channels[channel] >= 0) {
		DEBUGPRINT("FILEIO: open/create channel, already used channel for %d, fd is %d" NL, channel, exos_channels[channel]);
		Z80_A = 0xF9;	// channel number is already used! (maybe it's useless to be tested, as EXOS wouldn't allow that anyway?)
		return;
	}
	get_file_name(fnbuf);
	if (!*fnbuf) {
		r = xepgui_file_selector(
			XEPGUI_FSEL_OPEN | XEPGUI_FSEL_FLAG_STORE_DIR,
			WINDOW_TITLE " - Select file for load via FILE: device",
			fileio_cwd,
			fnbuf,
			sizeof fnbuf
		);
		if (r) {
			xep_set_error(HOST_OS_STR "No file selected");
			Z80_A = XEP_ERROR_CODE;
			return;
		}
		memmove(fnbuf, fnbuf + strlen(fileio_cwd), strlen(fnbuf + strlen(fileio_cwd)) + 1);
	}
	r = open_host_file(fileio_cwd, fnbuf, create);
	//xep_set_error(ERRSTR());
	DEBUGPRINT("FILEIO: %s channel #%d result = %d filename = \"%s\" (in \"%s\")" NL, create ? "create" : "open", channel, r, fnbuf, fileio_cwd);
	if (r < 0) {
		// open_host_file() already issued the xep_set_error() call to set a message up ...
		Z80_A = XEP_ERROR_CODE;
	} else {
		exos_channels[channel] = r;
		Z80_A = 0;
	}
}


void fileio_func_close_channel ( void )
{
	if (exos_channels[Z80_A] < 0) {
		DEBUGPRINT("FILEIO: close, invalid channel for %d, fd is %d" NL, Z80_A, exos_channels[Z80_A]);
		Z80_A = 0xFB;	// invalid channel
	} else {
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
		DEBUGPRINT("FILEIO: read block, invalid channel for %d, fd is %d" NL, Z80_A, exos_channels[Z80_A]);
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
			fileio_host_errstr();
			Z80_A = XEP_ERROR_CODE;
			break;
		}
	}
	p = buffer;
	while (rb--)
		write_cpu_byte_by_segmap(Z80_DE++, EXOS_USER_SEGMAP_P, *(p++));
}


void fileio_func_read_character ( void )
{
	if (exos_channels[Z80_A] < 0) {
		DEBUGPRINT("FILEIO: read character, invalid channel for %d, fd is %d" NL, Z80_A, exos_channels[Z80_A]);
		Z80_A = 0xFB;	// invalid channel
	} else {
		int r = read(exos_channels[Z80_A], &Z80_B, 1);
		if (r == 1)
			Z80_A = 0;
		else if (!r)
			Z80_A = 0xE4;	// attempt to read after end of file
		else {
			fileio_host_errstr();
			Z80_A = XEP_ERROR_CODE;
		}
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
			xep_set_error(HOST_OS_STR "Cannot write block");
			Z80_A = XEP_ERROR_CODE;
			break;
		} else {
			fileio_host_errstr();
			Z80_A = XEP_ERROR_CODE;
			break;
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
		else if (!r) {
			xep_set_error(HOST_OS_STR "Cannot write character");
			Z80_A = XEP_ERROR_CODE;
		} else {
			fileio_host_errstr();
			Z80_A = XEP_ERROR_CODE;
		}
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
