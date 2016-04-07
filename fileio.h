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

#ifndef __XEP128_FILEIO_H_INCLUDED
#define __XEP128_FILEIO_H_INCLUDED

extern char  fileio_cwd[PATH_MAX + 1];

extern void fileio_init ( const char *dir, const char *subdir );
extern Uint8 fileio_open_channel ( void );
extern Uint8 fileio_close_channel ( void );
extern Uint8 fileio_read_block ( void );
extern Uint8 fileio_read_character ( void );


extern Uint8 fileio_func_not_used_call();
extern Uint8 fileio_func_open_channel();
extern Uint8 fileio_func_create_channel();
extern Uint8 fileio_func_close_channel();
extern Uint8 fileio_func_destroy_channel();
extern Uint8 fileio_func_read_character();
extern Uint8 fileio_func_read_block();
extern Uint8 fileio_func_write_character();
extern Uint8 fileio_func_write_block();
extern Uint8 fileio_func_channel_read_status();
extern Uint8 fileio_func_set_channel_status();
extern Uint8 fileio_func_special_function();
extern Uint8 fileio_func_init();
extern Uint8 fileio_func_buffer_moved();

#endif
