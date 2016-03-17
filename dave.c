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


Uint8 dave_int_read;
Uint8 kbd_matrix[16]; // the "real" EP kbd matrix only uses the first 10 bytes though
static Uint8 dave_int_write;
static int _cnt_1hz, _cnt_50hz, _cnt_31khz, _cnt_1khz, _cnt_tg0, _cnt_tg1, _cnt_tg2;
static int _state_tg0, _state_tg1, _state_tg2;
int kbd_selector;
int cpu_cycles_per_dave_tick;

static SDL_AudioDeviceID audio = 0;
static SDL_AudioSpec audio_spec;
#define AUDIO_BUFFER_SIZE 0x4000
static Uint8 audio_buffer[AUDIO_BUFFER_SIZE];
static Uint8 *audio_buffer_r = audio_buffer;
static Uint8 *audio_buffer_w = audio_buffer;
static int dave_ticks_per_sample_counter = 0;
static int dave_ticks_per_sample = 6;



static void dave_render_audio_sample ( void )
{
	int left, right;
	/* TODO: missing noise channel, polynom counters/ops, modulation, etc */
	if (ports[0xA7] &  8)
		left  = (ports[0xA8] & 63) << 2;	// left ch is in D/A mode
	else {						// left ch is in "normal" mode
		left  = _state_tg0 * (ports[0xA8] & 63) +
			_state_tg1 * (ports[0xA9] & 63) +
			_state_tg2 * (ports[0xAA] & 63);
	}
	if (ports[0xA7] & 16)
		right = (ports[0xAC] & 63) << 2;	// right ch is in D/A mode
	else {						// right ch is in "normal" mode
		right = _state_tg0 * (ports[0xAC] & 63) +
			_state_tg1 * (ports[0xAD] & 63) +
			_state_tg2 * (ports[0xAE] & 63);
	}
	/* store sample now! */
	//daveAudioBufferLRec[daveAudioBufferWPos] = left  / 126 - 1;
	//daveAudioBufferRRec[daveAudioBufferWPos] = right / 126 - 1;
	//daveAudioBufferWPos = (daveAudioBufferWPos + 1) & SOUND_BUFFER_SIZE_MASK;
	//while (audio_buffer_r == audio_buffer_w && audio) ; // just consume CPU, yack ...
	*(audio_buffer_w++) = left  ;
	*(audio_buffer_w++) = right ;
	if (audio_buffer_w == audio_buffer + AUDIO_BUFFER_SIZE)
		audio_buffer_w = audio_buffer;
}


static void audio_callback(void *userdata, Uint8 *stream, int len)
{
	while (len--) {
		if (audio_buffer_r == audio_buffer_w) {
			//*(stream++) = 0;
			*(stream++) = 0;
		} else {
			//*(stream++) = *(audio_buffer_r++);
			*(stream++) = *(audio_buffer_r++);
			if (audio_buffer_r == audio_buffer + AUDIO_BUFFER_SIZE)
				audio_buffer_r = audio_buffer;
		}
	}
}


void audio_start ( void )
{
	if (audio)
		SDL_PauseAudioDevice(audio, 0);
}

void audio_stop ( void )
{
	if (audio)
		SDL_PauseAudioDevice(audio, 1);
}

void audio_close ( void )
{
	audio_stop();
	if (audio)
		SDL_CloseAudioDevice(audio);
	audio = 0;
}


void audio_init ( int enable )
{
	SDL_AudioSpec want;
	if (!enable) return;
	SDL_memset(&want, 0, sizeof(want));
	want.freq = 41666;
	want.format = AUDIO_U8;
	want.channels = 2;
	want.samples = 4096;
	want.callback = audio_callback;
	want.userdata = NULL;
	audio = SDL_OpenAudioDevice(NULL, 0, &want, &audio_spec, 0);
	if (!audio)
		ERROR_WINDOW("Cannot initiailze audio: %s\n", SDL_GetError());
	else if (want.freq != audio_spec.freq || want.format != audio_spec.format || want.channels != audio_spec.channels) {
		audio_close();
		ERROR_WINDOW("Bad audio parameters (w/h freq=%d/%d, fmt=%d/%d, chans=%d/%d, smpls=%d/%d, cannot use sound",
			want.freq, audio_spec.freq, want.format, audio_spec.format, want.channels, audio_spec.channels, want.samples, audio_spec.samples
		);
	} else
		audio_stop();	// still stopped ... must be audio_start()'ed by the caller
}


void dave_set_clock ( void )
{
	// FIXME maybe: Currently, it's assumed that Dave and CPU has fixed relation about clock
	//double turbo_rate = (double)CPU_CLOCK / (double)DEFAULT_CPU_CLOCK;
	if (ports[0xBF] & 2) {
		cpu_cycles_per_dave_tick = 24; // 12MHz (??)
		dave_ticks_per_sample = 4;
	} else {
		cpu_cycles_per_dave_tick = 16; // 8MHz  (??)
		dave_ticks_per_sample = 6;
	}
	//DEBUG("DAVE: CLOCK: assumming %dMHz input, CPU clock divisor is %d, CPU cycles per Dave tick is %d" NL, (ports[0xBF] & 2) ? 12 : 8, CPU_CLOCK / cpu_cycles_per_dave_tick, cpu_cycles_per_dave_tick);
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
		z80ex_pwrite_cb(a, 0);
	dave_int_read = 0;
	dave_int_write = 0;
	kbd_selector = -1;
	_cnt_1hz = 0; _cnt_50hz = 0; _cnt_31khz = 0; _cnt_1khz = 0; _cnt_tg0 = 0; _cnt_tg1 = 0; _cnt_tg2 = 0;
	_state_tg0 = 0; _state_tg1 = 0; _state_tg2 = 0;
	//mem_ws_all = 0;
	//mem_ws_m1  = 0;
	//NICK_SLOTS_PER_DAVE_TICK_HI = NICK_SLOTS_PER_SEC / 250000.0;
	//_set_timing();
	DEBUG("DAVE: reset" NL);
}

//static int dave_int1_last = 0;

/* Called by Nick */
void dave_int1(int level)
{
	if (level) {
		dave_int_read |= 16; // set level
	} else {
		// the truth is here, if previous level was set (falling edge), and int1 is enabled, then set latch!
		if ((dave_int_read & 16) && (dave_int_write & 16)) {
			DEBUG("DAVE/VINT: LACTH is set!" NL);
			dave_int_read |= 32; // set latch
			if (primo_on)
				nmi_pending = primo_nmi_enabled;
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
		//DEBUG("DAVE: 1HZ interrupt level: %d" NL, dave_int_read & 4);
	}
	// SOUND
	if (audio) {
		if ((--dave_ticks_per_sample_counter) < 0) {
			dave_render_audio_sample();
			dave_ticks_per_sample_counter = dave_ticks_per_sample - 1;
		}
	}
}


void dave_configure_interrupts ( Uint8 n )
{
	dave_int_write = n;
	dave_int_read &= (0x55 | ((~n) & 0xAA)); // this "nice" stuff resets desired latches
	dave_int_read &= (0x55 | ((n << 1) & 0xAA)); // TODO / FIXME: not sure if it is needed!
}
