/* Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
   Copyright (C)2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
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


#define ROM_SEG_LIMIT 0xF0

char *rom_desc = NULL;


int roms_load ( void )
{
	int used[ROM_SEG_LIMIT];
	int seg;
	int last = 0;
	char path[PATH_MAX + 1];
	for (seg = 0; seg < ROM_SEG_LIMIT; seg++ )
		used[seg] = 0;
	for (seg = 0; seg < ROM_SEG_LIMIT; seg++ ) {
		void *option = config_getopt("rom", seg, NULL);
		if (option) {
			const char *name;
			int lseg = seg;
			FILE *f;
			config_getopt_pointed(option, &name);
			DEBUG("CONFIG: ROM: segment %02Xh file %s" NL, seg, name);
			f = open_emu_file(name, "rb", path);
			if (f == NULL) {
				ERROR_WINDOW("Cannot open ROM image \"%s\" (to be used from segment %02Xh): %s", name, seg, ERRSTR());
				if (!strcmp(name, COMBINED_ROM_FN)) { // this should be the auto-install functionality, with downloading stuff?
				}
				return -1;
			}
			DEBUG("CONFIG: ROM: ... file path is %s" NL, path);
			for (;;) {
				int ret;
				if (lseg >= ROM_SEG_LIMIT) {
					fclose(f);
					ERROR_WINDOW("While reading ROM image \"%s\" into segment %02Xh: too long ROM image or above the ROM segment limit (%02Xh) fatal error occured!", path, lseg, ROM_SEG_LIMIT);
					return -1;
				}
				if (used[lseg]) {
					fclose(f);
					ERROR_WINDOW("While reading ROM image \"%s\" into segment %02Xh: already used ROM segment!", path, lseg);
					return -1;
				}
				ret = fread(memory + (lseg << 14), 1, 0x4000, f);
				if (ret)
					DEBUG("CONFIG: ROM: ... trying read 0x4000 bytes in segment %02Xh, result is %d" NL, lseg, ret);
				if (ret < 0) {
					ERROR_WINDOW("Cannot read ROM image \"%s\" (to be used in segment %02Xh): %s", path, lseg, ERRSTR());
					fclose(f);
					return -1;
				} else if (ret == 0) {
					if (lseg == seg) {
						fclose(f);
						ERROR_WINDOW("Null-sized ROM image \"%s\" (to be used in segment %02Xh).", path, lseg);
						return -1;
					}
					break;
				} else if (ret != 0x4000) {
					fclose(f);
					ERROR_WINDOW("Bad ROM image \"%s\": not multiple of 16K bytes!", path);
					return -1;
				}
				used[lseg] = 1;
				if (lseg > last)
					last = lseg;
				if (ret != 0x4000)
					break;
				lseg++;
			}
			fclose(f);
			if (rom_desc) {
				rom_desc = realloc(rom_desc, strlen(rom_desc) + PATH_MAX + 10);
				check_malloc(rom_desc);
			} else {
				rom_desc = malloc(PATH_MAX + 10);
				check_malloc(rom_desc);
				rom_desc[0] = 0;
			}
			sprintf(rom_desc + strlen(rom_desc), "%02X-%02X %s\r\n", seg, lseg - 1, path);
		} else if (!seg) {
			ERROR_WINDOW("Fatal ROM image error: No ROM defined for segment 00h, no EXOS is requested!");
			return -1;
		}
	}
	rom_desc = realloc(rom_desc, strlen(rom_desc) + 1);
	check_malloc(rom_desc);
	DEBUG("CONFIG: ROM: DONE :-) Last used segment is %02Xh." NL, last);
	return last << 14;
}

