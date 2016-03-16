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


#ifndef __XEPEM_H
#define __XEPEM_H

#define Z80EX_Z180_SUPPORT
#define Z80EX_ED_TRAPPING_SUPPORT

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
#define VERSION "v0.3"
#define COPYRIGHT "(C)2015,2016 LGB Gabor Lenart"

#define USE_LODEPNG

// more accurate :) from IstvanV
#define NICK_SLOTS_PER_SEC 889846
#define DEFAULT_CPU_CLOCK 4000000
#define APU_CLOCK 2500000

#define CONFIG_SDEXT_SUPPORT
#define CONFIG_W5300_SUPPORT
#define W5300_IO_BASE 0x90

#define COMBINED_ROM_FN "combined.rom"
#define SDCARD_IMG_FN "sdcard.img"
#define PRINT_OUT_FN "@print.out"
#define DEFAULT_CONFIG_FILE "@config"
#define DEFAULT_CONFIG_SAMPLE_FILE "@config-sample"


#define OSD(...) do { \
	char _buf_for_win_msg_[4096]; \
	sprintf(_buf_for_win_msg_, __VA_ARGS__); \
	fprintf(stderr, "OSD: %s\n", _buf_for_win_msg_); \
	osd_notification(_buf_for_win_msg_); \
} while(0)

int _sdl_emu_secured_message_box_ ( Uint32 sdlflag, const char *msg );
#define _REPORT_WINDOW_(sdlflag, str, ...) do { \
	char _buf_for_win_msg_[4096]; \
	sprintf(_buf_for_win_msg_, __VA_ARGS__); \
	fprintf(stderr, str ": %s\n", _buf_for_win_msg_); \
	_sdl_emu_secured_message_box_(sdlflag, _buf_for_win_msg_); \
} while(0)
#define INFO_WINDOW(...)	_REPORT_WINDOW_(SDL_MESSAGEBOX_INFORMATION, "INFO", __VA_ARGS__)
#define WARNING_WINDOW(...)	_REPORT_WINDOW_(SDL_MESSAGEBOX_WARNING, "WARNING", __VA_ARGS__)
#define ERROR_WINDOW(...)	_REPORT_WINDOW_(SDL_MESSAGEBOX_ERROR, "ERROR", __VA_ARGS__)

int _sdl_emu_secured_modal_box_ ( const char *items_in, const char *msg );
#define QUESTION_WINDOW(items, msg) _sdl_emu_secured_modal_box_(items, msg)

//#define ERRSTR() sys_errlist[errno]
#define ERRSTR() strerror(errno)

#ifdef _WIN32
#define DIRSEP "\\"
#define NL "\r\n"
#else
#define DIRSEP "/"
#define NL "\n"
#endif

#define SCREEN_WIDTH	736
#define SCREEN_HEIGHT	288
#define SCREEN_FORMAT	SDL_PIXELFORMAT_ARGB8888

extern char *app_pref_path, *app_base_path;
extern char current_directory[PATH_MAX + 1];
extern char *rom_desc;
extern char sdimg_path[PATH_MAX + 1];


void xep_rom_install ( int offset );

extern int CPU_CLOCK;
int set_ep_ramsize(int kbytes);
int set_cpu_clock ( int hz );
int set_cpu_clock_with_osd ( int hz );
void z80_reset ( void );
void ep_reset ( void );
void ep_clear_ram ( void );
extern int rom_size, xep_rom_seg, xep_rom_addr, ram_start;
FILE *open_emu_file ( const char *name, const char *mode, char *pathbuffer );
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

void audio_init ( int enable );
void audio_start ( void );
void audio_stop ( void );
void audio_close ( void );
void dave_reset ( void );
void dave_set_clock ( void );
void dave_int1(int level);
void dave_configure_interrupts ( Uint8 n );
void dave_tick ( void );
extern int cpu_cycles_per_dave_tick;
void kbd_matrix_reset ( void );
void mouse_reset_button ( void );
int mouse_is_enabled ( void );
//extern int mem_ws_all, mem_ws_m1;

//int z80_disasm(char *buffer, int buffer_size, int flags, int *t_states, int *t_states_2, Uint16 pc, int seg);
int z80_dasm(char *buffer, Uint16 pc, int seg);
Uint32 *nick_init ( void );
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
int mouse_entermice ( int entermice );
void check_malloc ( const void *p );

time_t emu_getunixtime(void);

void emu_one_frame(int usecs, int frameksip);

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
extern int sdext_cart_enabler;
#define SDEXT_CART_ENABLER_ON    0x10000
#define SDEXT_CART_ENABLER_OFF         1
void sdext_init ( void );
Uint8 sdext_read_cart ( Uint16 addr );
void sdext_write_cart ( Uint16 addr, Uint8 data );
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
void zxemu_switch ( Uint8 data );
#define IO16_HI_BYTE(port16) (((ports[(((port16) >> 14) & 3) | 0xB0] & 3) << 6) | (((port16) >> 8) & 0x3F))

extern int primo_nmi_enabled, primo_on;
void primo_write_io ( Uint8 port, Uint8 data );
Uint8 primo_read_io ( Uint8 port );
void primo_switch ( Uint8 data );
void primo_emulator_execute ( void );
void primo_emulator_exit ( void );
int primo_search_rom ( void );
extern int primo_rom_seg;

extern Uint8 memory[0x400000];
extern Uint8 ports[0x100];
extern Uint8 dave_int_read;
extern Uint8 kbd_matrix[16];
extern int kbd_selector;

void xep_rom_trap ( Uint16 pc, Uint8 opcode);

void w5300_reset ( void );
void w5300_init ( void (*cb)(int) );
void w5300_shutdown ( void );
void w5300_write_mr0 ( Uint8 data );
void w5300_write_mr1 ( Uint8 data );
void w5300_write_idm_ar0 ( Uint8 data );
void w5300_write_idm_ar1 ( Uint8 data );
void w5300_write_idm_dr0 ( Uint8 data );
void w5300_write_idm_dr1 ( Uint8 data );
Uint8 w5300_read_mr0 ( void );
Uint8 w5300_read_mr1 ( void );
Uint8 w5300_read_idm_ar0 ( void );
Uint8 w5300_read_idm_ar1 ( void );
Uint8 w5300_read_idm_dr0 ( void );
Uint8 w5300_read_idm_dr1 ( void );

/* apu.c */

Uint8 apu_read_data ( void );
Uint8 apu_read_status ( void );
void apu_write_data ( Uint8 value );
void apu_write_command ( Uint8 value );
void apu_reset ( void );

/* screen.c */

void screen_grab ( SDL_bool state );
void screen_set_fullscreen ( int state );
void screen_present_frame (Uint32 *ep_pixels);
void screen_window_resized ( int new_xsize, int new_ysize );
int screen_shot ( Uint32 *ep_pixels, const char *directory, const char *filename );
int screen_init ( void );
extern Uint32 sdl_winid;
extern SDL_Window *sdl_win;
extern int is_fullscreen;
void osd_disable ( void );
void osd_notification ( const char *s );
void osd_replay ( int fade );
extern int warn_for_mouse_grab;
#define OSD_FADE_START 300
#define OSD_FADE_STOP    0x80
#define OSD_FADE_DEC    3

int keymap_resolve_event ( SDL_Keysym sym, int press, Uint8 *matrix );
void keymap_preinit_config_internal ( void );
void keymap_dump_config ( FILE *f );
int keymap_set_key_by_name ( const char *name, int posep );


int config_init ( int argc, char **argv );
void *config_getopt ( const char *name, const int subopt, void *value );
void config_getopt_pointed ( void *st_in, void *value );
static inline int config_getopt_int ( const char *name ) {
	int n;
	config_getopt(name, -1, &n);
	return n;
}
static inline const char *config_getopt_str ( const char *name ) {
	char *s;
	config_getopt(name, -1, &s);
	return s;
}

extern const char *BUILDINFO_ON;
extern const char *BUILDINFO_AT;
extern const char *BUILDINFO_GIT;
extern const char *BUILDINFO_CC;

#if defined(__clang__)
#define CC_TYPE "clang"
#elif defined(__MINGW32__)
#define CC_TYPE "mingw32"
#elif defined(__MINGW64__)
#define CC_TYPE "mingw64"
#elif defined(__GNUC__)
#define CC_TYPE "gcc"
#else
#define CC_TYPE "Something"
#endif

int roms_load ( void );

#endif
