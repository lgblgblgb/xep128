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


#define NICK_TO_DAVE_TCONV_6MHZ NICK_SLOTS_PER_SEC * 6
#define NICK_TO_DAVE_TCONV_8MHZ NICK_SLOTS_PER_SEC * 8

Uint8 dave_int_read;
Uint8 kbd_matrix[16]; // the "real" EP kbd matrix only uses the first 10 bytes though
static Uint8 dave_int_write;
static int _cnt_1hz;
int kbd_selector;


void _set_timing ( int mhz )
{
	//1000000 * mhz / (double)NICK_SLOTS_PER_SEC
}


void kbd_matrix_reset ( void )
{
	memset(kbd_matrix, 0xFF, sizeof(kbd_matrix));
}


void dave_reset ( void )
{
	int a;
	//kbd_matrix_reset();
	for (a = 0xA0; a <= 0xBF; a++)
		port_write(a, 0);
	dave_int_read = 0;
	dave_int_write = 0;
	kbd_selector = -1;
	_cnt_1hz = 0;
	//mem_ws_all = 0;
	//mem_ws_m1  = 0;
	//NICK_SLOTS_PER_DAVE_TICK_HI = NICK_SLOTS_PER_SEC / 250000.0;
	//_set_timing();
	printf("Dave: reset\n");
}

//static int dave_int1_last = 0;

/* Called by Nick */
void dave_int1(int active)
{
	//if (active == dave_int1_last) return;
	//dave_int1_last = active;
	if (active) {
		dave_int_read |= 16; // set level
	} else {
		// the truth is here, if previous level was set (falling edge), and int1 is enabled, then set latch!
		if ((dave_int_read & 16) && (dave_int_write & 16)) {
			printf("DAVE/VINT: LACTH is set!\n");
			dave_int_read |= 32; // set latch
		}
		dave_int_read &= 239; // reset level
	}
}


/* call this periodically. since even Dave/CPU clock can differ, we use a stable frequency source in emulator:
 * the parameter is the number of nick slots [since the last call!], as it seems to be a constant frequency in the system at least */
void dave_ticks ( int slots )
{
	//return;
	if (!slots) return; // if 0 slots, there is really no need to re-examine the situation
	//printf("DAVE: %d Nick slots elapsed\n", slots);
	/* handling the 1Hz interrupt */
	_cnt_1hz -= slots;
        if (_cnt_1hz < 0) {
                //debug("1HZ int @ " + Date.now());
                _cnt_1hz = NICK_SLOTS_PER_SEC - 1;
                if (dave_int_write & 4)
                        dave_int_read |= 8; // set latch, if 1Hz int source is enabled
                dave_int_read ^= 4; // negate 1Hz interrupt level bit (actually the freq is 0.5Hz, but int is generated on each edge, thus 1Hz)
        }
}


void dave_configure_interrupts ( Uint8 n )
{
	dave_int_write = n;
	dave_int_read &= (0x55 | ((~n) & 0xAA)); // this "nice" stuff resets desired latches
	dave_int_read &= (0x55 | ((n << 1) & 0xAA)); // TODO / FIXME: not sure if it is needed!
}
