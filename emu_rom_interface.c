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
#include <sys/stat.h>
#include <sys/types.h>

#define COBUF ((char*)(memory + xep_rom_addr + xepsym_cobuf - 0xC000))
#define SET_XEPSYM_BYTE(sym, value) memory[xep_rom_addr + (sym) - 0xC000] = (value)
#define SET_XEPSYM_WORD(sym, value) do {	\
	SET_XEPSYM_BYTE(sym, (value) & 0xFF);	\
	SET_XEPSYM_BYTE((sym) + 1, (value) >> 8);	\
} while(0)
#define BIN2BCD(bin) ((((bin) / 10) << 4) | ((bin) % 10))

static const char EXOS_NEWLINE[] = "\r\n";
Uint8 exos_version = 0;
Uint8 exos_info[8];
char fileio_cwd[PATH_MAX + 1];

#define EXOS_ADDR(n)		(0x3FC000 | ((n) & 0x3FFF))
#define EXOS_BYTE(n)		memory[EXOS_ADDR(n)]
#define EXOS_GET_WORD(n)	(EXOS_BYTE(n) | (EXOS_BYTE((n) + 1) << 8))



void fileio_init ( const char *dir, const char *subdir )
{
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


void exos_get_status_line ( char *buffer )
{
	Uint8 *s = memory + EXOS_ADDR(EXOS_GET_WORD(0xBFF6));
	int a = 40;
	while (a--)
		*(buffer++) = *(s++) & 0x7F;
	*buffer = '\0';
}


void xep_set_time_consts ( char *descbuffer )
{
	time_t now = emu_getunixtime();
	struct tm *t = localtime(&now);
	SET_XEPSYM_BYTE(xepsym_settime_hour,    BIN2BCD(t->tm_hour));
	SET_XEPSYM_WORD(xepsym_settime_minsec,  (BIN2BCD(t->tm_min) << 8) | BIN2BCD(t->tm_sec));
	SET_XEPSYM_BYTE(xepsym_setdate_year,    BIN2BCD(t->tm_year - 80));
	SET_XEPSYM_WORD(xepsym_setdate_monday,  (BIN2BCD(t->tm_mon + 1) << 8) | BIN2BCD(t->tm_mday));
	if (descbuffer)
		sprintf(descbuffer, "%04d-%02d-%02d %02d:%02d:%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec
		);
	SET_XEPSYM_WORD(xepsym_jump_on_rom_entry, xepsym_set_time);
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
	int size;
	*COBUF = 0; // no ans by def
	DEBUG("XEP: COMMAND TRAP: C=%02Xh, B=%02Xh, DE=%04Xh" NL, c, b, de);
	/* restore exos command handler jump address */
	SET_XEPSYM_WORD(xepsym_jump_on_rom_entry, xepsym_print_xep_buffer);
	switch (c) {
		case 2: // EXOS command
			if (exos_cmd_name_match("XEP", de + 1)) {
				char buffer[256];
				char *p = buffer;
				b = read_cpu_byte(de) - 3;
				de += 4;
				while (b--)
					*(p++) = read_cpu_byte(de++);
				*p = '\0';
				monitor_execute(
					buffer,			// input buffer
					1,			// source system (XEP ROM)
					COBUF,			// output buffer (directly into the co-buffer area!)
					xepsym_cobuf_size - 1,	// max allowed output size
					EXOS_NEWLINE		// newline delimiter requested (for EXOS we use this fixed value! unlike with console/monitor where it's host-OS dependent!)
				);
				Z80_A = 0;
				Z80_C = 0;
			}
			break;
		case 3: // EXOS help
			if (!b) {
				// eg on :HELP (ROM list) we patch the request as ROMNAME monitor command ...
				monitor_execute("ROMNAME", 1, COBUF, xepsym_cobuf_size - 1, EXOS_NEWLINE);
				Z80_A = 0;
			} else if (exos_cmd_name_match("XEP", de + 1)) {
				monitor_execute("HELP", 1, COBUF, xepsym_cobuf_size - 1, EXOS_NEWLINE);
				Z80_A = 0;
				Z80_C = 0;
			}
			break;
		case 8:	// Initialization
			// Tell XEP ROM to set EXOS date/time with setting we will provide here
			xep_set_time_consts(NULL);
			SET_XEPSYM_WORD(xepsym_jump_on_rom_entry, xepsym_system_init);
			break;
		case 1:	// Cold reset
			SET_XEPSYM_WORD(xepsym_jump_on_rom_entry, xepsym_cold_reset);
			break;
	}
	size = strlen(COBUF);
	if (size)
		DEBUG("XEP: ANSWER: [%d bytes] = \"%s\"" NL, size, COBUF);
	// just a sanity check, monitor_execute() would not allow - in theory ... - to store more data than specified (by MPRINTF)
	if (size > xepsym_cobuf_size - 1) {
		ERROR_WINDOW("FATAL: XEP ROM answer is too large, %d bytes.", size);
		exit(1);
	}
	SET_XEPSYM_WORD(xepsym_print_size, size);	// set print-out size (0 = no print)
}


#include <dirent.h>
#include <fcntl.h>


static int fileio_fd = -1;
static int fileio_channel = -1;






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
			return open(buffer, mode);
		}
	}
	closedir(dir);
	return -1;
}



static Uint8 fileio_open_channel ( void )
{
	int de = Z80_DE;
	int fnlen = read_cpu_byte(de);
	char fnbuf[128], *p = fnbuf;
	if (fileio_channel == Z80_A)
		return 0xF9;	// channel number is already used! (not so much useful here though to check, maybe?)
	if (fileio_channel != -1)
		return 0xE9;	// we support only single channel mode, ie, device already in use error ...
	while (fnlen--)
		*(p++) = read_cpu_byte(++de);
	*p = '\0';
	DEBUGPRINT("Filename = \"%s\" Channel = %d" NL, fnbuf, Z80_A);
	de = fileio_open_existing_hostfile(fileio_cwd, fnbuf, O_RDONLY);
	DEBUGPRINT("Result of open: %d (cwd=%s)" NL, de, fileio_cwd);
	if (de < 0)
		return 0xCF;	// file not found, but this is an EXDOS error code for real!
	fileio_channel = Z80_A;
	fileio_fd = de;
	return 0;
}


static Uint8 fileio_close_channel ( void )
{
	if (fileio_channel != Z80_A)
		return 0xFB;	// invalid channel
	close(fileio_fd);
	fileio_fd = -1;
	fileio_channel = -1;
	return 0;		// OK, closed.
}



static Uint8 fileio_read_block ( void )
{
	int r;
	Uint8 fileio_buffer[0xFFFF], *p = fileio_buffer;
	if (Z80_A != fileio_channel)
		return 0xFB;	// invalid channel
	if (!Z80_BC)
		return 0;	// read to zero amount of data should be handled normally!
	r = read(fileio_fd, fileio_buffer, Z80_BC);
	if (!r) 		// attempt to read after end of file?
		return 0xE4;
	else if (r < 0)		// host file read error!
		return 0xD0;	// somewhat funny error code for here: casette CRC error :)
	while (r--) {
		write_cpu_byte_by_segmap(Z80_DE++, memory + EXOS_ADDR(0xBFFC), *(p++));
		Z80_BC--;
		if (!Z80_DE && r)
			return 0x9B;	// OV64K, this is an EXDOS error for real ...
	}
	return 0;
}


static Uint8 fileio_read_character ( void )
{
	int r;
	Uint8 fileio_buffer;
	if (Z80_A != fileio_channel)
		return 0xFB;	// invalid channel
	r = read(fileio_fd, &fileio_buffer, 1);
	if (!r) 		// attempt to read after end of file?
		return 0xE4;
	else if (r < 0)		// host file read error!
		return 0xD0;	// somewhat funny error code for here: casette CRC error :)
	Z80_B = fileio_buffer;
	return 0;
}






void xep_rom_trap ( Uint16 pc, Uint8 opcode )
{
	xep_rom_write_support(0);	// to be safe, let's switch writable XEP ROM off (maybe it was enabled by previous trap?)
	DEBUG("XEP: ROM trap at PC=%04Xh OPC=%02Xh" NL, pc, opcode);
	if (opcode != xepsym_ed_trap_opcode) {
		ERROR_WINDOW("FATAL: Unknown ED-trap opcode in XEP ROM: PC=%04Xh ED_OP=%02Xh", pc, opcode);
		exit(1);
	}
	switch (pc) {
		case xepsym_trap_enable_rom_write:
			xep_rom_write_support(1);	// special ROM request to enable ROM write ... Danger Will Robinson!!
			break;
		case xepsym_trap_exos_command:
			xep_exos_command_trap();
			break;
		case xepsym_trap_on_system_init:
			exos_version = Z80_B;	// store EXOS version number we got ...
			memcpy(exos_info, memory + ((xepsym_exos_info_struct & 0x3FFF) | (xep_rom_seg << 14)), 8);
			if (config_getopt_int("skiplogo")) {
				DEBUG("XEP: skiplogo option requested logo skip, etting EXOS variable 0xBFEF to 1 on system init ROM call" NL);
				EXOS_BYTE(0xBFEF) = 1; // use this, to skip Enterprise logo when it would come :-)
			}
			break;
		/* ---- FILEIO RELATED TRAPS ---- */
		case xepsym_fileio_no_used_call:
			DEBUGPRINT("File I/O trap xepsym_fileio_no_used_call is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		case xepsym_fileio_open_channel:
			//DEBUGPRINT("File I/O trap xepsym_fileio_open_channel is not implemented yet." NL);
			Z80_A = fileio_open_channel();
			break;
		case xepsym_fileio_create_channel:
			DEBUGPRINT("File I/O trap xepsym_fileio_create_channel is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		case xepsym_fileio_close_channel:
			//DEBUGPRINT("File I/O trap xepsym_fileio_close_channel is not implemented yet." NL);
			Z80_A = fileio_close_channel();
			break;
		case xepsym_fileio_destroy_channel:
			DEBUGPRINT("File I/O trap xepsym_fileio_destroy_channel is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		case xepsym_fileio_read_character:
			//DEBUGPRINT("File I/O trap xepsym_fileio_read_character is not implemented yet." NL);
			Z80_A = fileio_read_character();
			break;
		case xepsym_fileio_read_block:
			//DEBUGPRINT("File I/O trap xepsym_fileio_read_block is not implemented yet." NL);
			Z80_A = fileio_read_block();
			break;
		case xepsym_fileio_write_character:
			DEBUGPRINT("File I/O trap xepsym_fileio_write_character is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		case xepsym_fileio_write_block:
			DEBUGPRINT("File I/O trap xepsym_fileio_write_block is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		case xepsym_fileio_channel_read_status:
			DEBUGPRINT("File I/O trap xepsym_fileio_channel_read_status is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		case xepsym_fileio_set_channel_status:
			DEBUGPRINT("File I/O trap xepsym_fileio_set_channel_status is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		case xepsym_fileio_special_function:
			DEBUGPRINT("File I/O trap xepsym_fileio_special_function is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		case xepsym_fileio_init:
			//DEBUGPRINT("File I/O trap xepsym_fileio_init is not implemented yet." NL);
			//Z80_A = 0xE7;
			if (fileio_fd != -1)
				close(fileio_fd);
			fileio_fd = -1;
			fileio_channel = -1;
			break;
		case xepsym_fileio_buffer_moved:
			DEBUGPRINT("File I/O trap xepsym_fileio_buffer_moved is not implemented yet." NL);
			Z80_A = 0xE7;
			break;
		default:
			ERROR_WINDOW("FATAL: Unknown ED-trap location in XEP ROM: PC=%04Xh (ED_OP=%02Xh)", pc, opcode);
			exit(1);
	}
}

