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


#ifndef __XEPEM_H
#define __XEPEM_H

#define Z80EX_Z180_SUPPORT

#include <stdio.h>
#include <stdlib.h>
#include "z80ex.h"
#include "z80ex_dasm.h"
#include "SDL.h"
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>

#define WINDOW_TITLE "Xep128"
#define VERSION "v0.1"
#define COPYRIGHT "(C)2015 LGB Gabor Lenart"

// more accurate :) from IstvanV
#define NICK_SLOTS_PER_SEC 889846
#define DEFAULT_CPU_CLOCK 4000000

#define CONFIG_SDEXT_SUPPORT

#define COMBINED_ROM_FN "combined.rom"
#define SDCARD_IMG_FN "sdcard.img"
#define PRINT_OUT_FN "print.out"

#define ERROR_WINDOW(...) { \
	char _buf_for_win_msg[4096]; \
	sprintf(_buf_for_win_msg, __VA_ARGS__); \
	fprintf(stderr, "ERROR: %s\n", _buf_for_win_msg); \
	kbd_matrix_reset(); \
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Xep128 Report", _buf_for_win_msg, sdl_win); \
}
//#define ERRSTR() sys_errlist[errno]
#define ERRSTR() strerror(errno)

#ifdef _WIN32
#define DIRSEP "\\"
#else
#define DIRSEP "/"
#endif

extern SDL_Window *sdl_win;
char *app_pref_path, *app_base_path;

extern int CPU_CLOCK;
int set_ep_ramsize(int kbytes);
int set_cpu_clock ( int hz );
int z80_reset ( void );
void ep_reset ( void );
void ep_clear_ram ( void );
extern int rom_size, xep_rom_seg, xep_rom_addr, ram_start;
FILE *open_emu_file ( const char *name, const char *mode );
Uint8 read_cpu_byte ( Uint16 addr );
void set_ep_cpu ( int type );
#define CPU_Z80		0
#define CPU_Z80C	1
#define CPU_Z180	2
extern int cpu_type;

void z180_internal_reset ( void );
void z180_port_write ( Uint8 port, Uint8 value );
Uint8 z180_port_read ( Uint8 port );
extern int z180_port_start;

void dave_reset ( void );
void dave_int1(int active);
void dave_configure_interrupts ( Uint8 n );
void dave_ticks ( int slots );
void kbd_matrix_reset ( void );
//extern int mem_ws_all, mem_ws_m1;

//int z80_disasm(char *buffer, int buffer_size, int flags, int *t_states, int *t_states_2, Uint16 pc, int seg);
int z80_dasm(char *buffer, Uint16 pc, int seg);
Uint32 *nick_init ( SDL_Surface *surface );
void nick_set_border ( Uint8 bcol );
void nick_set_bias ( Uint8 value );
void nick_set_lptl ( Uint8 value );
void nick_set_lpth ( Uint8 value );
Uint8 nick_get_last_byte ( void );
void nick_render_slot ( void );
void nick_dump_lpt ( void );

void set_ep_memseg(int seg, int val);
void port_write ( Z80EX_WORD port, Z80EX_BYTE value );


void emu_kbd(SDL_Keysym sym, int press);
void emu_mouse_button(Uint8 button, int press);
void emu_mouse_motion(int dx, int dy);
Uint8 mouse_read(void);
void mouse_check_data_shift(Uint8 val);
void mouse_reset(void);
extern int shift_pressed;

time_t emu_getunixtime(void);

void emu_one_frame(int usecs, int frameksip);
void emu_win_grab ( SDL_bool state );

#ifdef CONFIG_EXDOS_SUPPORT
extern Uint8 wd_sector;
extern Uint8 wd_track;
Uint8 wd_read_status ( void );
Uint8 wd_read_data ( void );
Uint8 wd_read_exdos_status ( void );
void wd_send_command ( Uint8 value );
void wd_write_data (Uint8 value);
void wd_set_exdos_control (Uint8 value);
void wd_exdos_reset ( void );
#endif

#ifdef CONFIG_SDEXT_SUPPORT
extern int sdext_cp3m_usability;
#define SDEXT_PHYSADDR_CART_P3_SELMASK_ON    0x1C000
#define SDEXT_PHYSADDR_CART_P3_SELMASK_OFF         1
void sdext_init ( void );
Uint8 sdext_read_cart_p3 ( Uint16 addr );
void sdext_write_cart_p3 ( Uint16 addr, Uint8 data );
void sdext_clear_ram(void);
#endif

void rtc_reset(void);
void rtc_set_reg(Uint8 val);
void rtc_write_reg(Uint8 val);
Uint8 rtc_read_reg(void);
extern int rtc_update_trigger;

void printer_send_data(Uint8 data);
void printer_close(void);

void zxemu_write_ula ( Uint8 hiaddr, Uint8 data );
Uint8 zxemu_read_ula ( Uint8 hiaddr );
void zxemu_attribute_memory_write ( Uint16 address, Uint8 data );
extern int zxemu_on, nmi_pending;
#define IO16_HI_BYTE(port16) (((ports[(((port16) >> 14) & 3) | 0xB0] & 3) << 6) | (((port16) >> 8) & 0x3F))

extern Uint8 memory[0x400000];
extern Z80EX_CONTEXT *z80;
extern Uint8 ports[0x100];
extern Uint8 dave_int_read;
extern Uint8 kbd_matrix[16];
extern int kbd_selector;

void xep_rom_trap ( Uint16 pc, Uint8 opcode);

#endif

