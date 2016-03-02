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


Uint8 dave_int_read;
Uint8 kbd_matrix[16]; // the "real" EP kbd matrix only uses the first 10 bytes though
static Uint8 dave_int_write;
static int _cnt_1hz, _cnt_50hz, _cnt_31khz, _cnt_1khz, _cnt_tg0, _cnt_tg1, _cnt_tg2;
static int _state_tg0, _state_tg1, _state_tg2;
int kbd_selector;
int cpu_cycles_per_dave_tick;




void dave_set_clock ( void )
{
	// FIXME maybe: Currently, it's assumed that Dave and CPU has fixed relation about clock
	//double turbo_rate = (double)CPU_CLOCK / (double)DEFAULT_CPU_CLOCK;
	if (ports[0xBF] & 2)
		cpu_cycles_per_dave_tick = 24; // 12MHz (??)
	else
		cpu_cycles_per_dave_tick = 16; // 8MHz  (??)
	//printf("DAVE: CLOCK: assumming %dMHz input, CPU clock divisor is %d, CPU cycles per Dave tick is %d\n", (ports[0xBF] & 2) ? 12 : 8, CPU_CLOCK / cpu_cycles_per_dave_tick, cpu_cycles_per_dave_tick);
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
	_cnt_1hz = 0; _cnt_50hz = 0; _cnt_31khz = 0; _cnt_1khz = 0; _cnt_tg0 = 0; _cnt_tg1 = 0; _cnt_tg2 = 0;
	_state_tg0 = 0; _state_tg1 = 0; _state_tg2 = 0;
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



static inline void dave_int_tg ( void )
{
	if (dave_int_write & 1)
		dave_int_read |= 2;	// set latch if TG int is enabled
	dave_int_read ^= 1;		// negate level
}



void dave_tick ( void )
{
#if 0
	/* besides the name, it's 31.25KHz - NOTE, currently it is not used
	   and maybe double freq is needed, anyway! I will examine this later,
	   this code fragment is just a reminder for myself. */
	_cnt_31khz -= ticks;
	if (_cnt_31khz < 0) {
		_cnt_31khz += 8 - 1;
	}
#endif
	/* 50Hz counter */
	if ((--_cnt_50hz) < 0) {
		_cnt_50hz = 5000 - 1;
		if ((ports[0xA7] & 96) == 32)
			dave_int_tg();
	}
	/* 1KHz counter */
	if ((--_cnt_1khz) < 0) {
		_cnt_1khz = 250 - 1;
		if ((ports[0xA7] & 96) ==  0)
			dave_int_tg();
	}
	/* counter for tone channel #0 */
	if (ports[0xA7] & 1) { // sync mode?
		_cnt_tg0 = ports[0xA0] | ((ports[0xA1] & 15) << 8);
		_state_tg0 = 0;
	} else if ((--_cnt_tg0) < 0) {
		_cnt_tg0 = ports[0xA0] | ((ports[0xA1] & 15) << 8);
		_state_tg0 ^= 1;
		if ((ports[0xA7] & 96) == 64)
			dave_int_tg();
	}
	/* counter for tone channel #1 */
	if (ports[0xA7] & 2) { // sync mode?
		_cnt_tg1 = ports[0xA2] | ((ports[0xA3] & 15) << 8);
		_state_tg1 = 0;
	} else if ((--_cnt_tg1) < 0) {
		_cnt_tg1 = ports[0xA2] | ((ports[0xA3] & 15) << 8);
		_state_tg1 ^= 1;
		if ((ports[0xA7] & 96) == 96)
			dave_int_tg();
	}
	/* counter for tone channel #2 */
	if (ports[0xA7] & 4) { // sync mode?
		_cnt_tg2 = ports[0xA4] | ((ports[0xA5] & 15) << 8);
		_state_tg2 = 0;
	} else if ((--_cnt_tg2) < 0) {
		_cnt_tg2 = ports[0xA4] | ((ports[0xA5] & 15) << 8);
		_state_tg2 ^= 1;
	}
	/* handling the 1Hz interrupt */
	if ((--_cnt_1hz) < 0) {
		_cnt_1hz = 250000 - 1;
		if (dave_int_write & 4)
			dave_int_read |= 8; // set latch, if 1Hz int source is enabled
		dave_int_read ^= 4; // negate 1Hz interrupt level bit (actually the freq is 0.5Hz, but int is generated on each edge, thus 1Hz)
		//printf("DAVE: 1HZ interrupt level: %d\n", dave_int_read & 4);
	}
}


void dave_configure_interrupts ( Uint8 n )
{
	dave_int_write = n;
	dave_int_read &= (0x55 | ((~n) & 0xAA)); // this "nice" stuff resets desired latches
	dave_int_read &= (0x55 | ((n << 1) & 0xAA)); // TODO / FIXME: not sure if it is needed!
}
