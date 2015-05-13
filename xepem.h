#ifndef __XEPEM_H
#define __XEPEM_H
#include <stdio.h>
#include <stdlib.h>
#include "z80ex.h"
#include "z80ex/z80ex_dasm.h"
#include "SDL.h"
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#define WINDOW_TITLE "XEPem"
#define VERSION "v1.0"
#define COPYRIGHT "(C)2015 Gabor Lenart"

// more accurate :) from IstvanV
#define NICK_SLOTS_PER_SEC 889846
#define CPU_CLOCK 4000000

#define CONFIG_SDEXT_SUPPORT

#define COMBINED_ROM_PATH "combined.rom"
#define SDCARD_IMG_PATH "sdcard.img"

int set_ep_ramsize(int kbytes);
int z80_reset ( void );

void dave_reset ( void );
void dave_int1(int active);
void dave_configure_interrupts ( Uint8 n );
void dave_ticks ( int slots );
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

extern Uint8 memory[0x400000];
extern Z80EX_CONTEXT *z80;
extern Uint8 ports[0x100];
extern Uint8 dave_int_read;
extern Uint8 kbd_matrix[16];
extern int kbd_selector;


#endif

