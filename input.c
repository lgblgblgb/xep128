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

static int _mouse_dx, _mouse_dy, _mouse_grab = 0, nibble_counter;
static int setup_value; // used only the setup func, not so much internally ...
static Uint8 nibble, _mouse_last_shift, _mouse_button_state;
static int _mouse_pulse = 0; // try to detect SymbOS or other simllar tests
static int _mouse_wait_warn = 1;
static int is_entermice = 0;	// shows to use boxsoft-used lines, or entermice (J/K line)
extern Uint16 raster_time; // we use Nick's raster_time "timing source" because it's kinda free without introducing another timer source
static Uint16 watchdog;		// watchdog value (compared to raster_time)
static int mouse_protocol_nibbles = 4;


/* The mouse buffer. nibble_counter shows which nibble is to read (thus "nibble_counter >> 1" is the byte pointer actually.
   mouse_protocol_nibbles limits the max nibbles to read, ie it's 4 (= 2 bytes) for boxsoft protocol for the default setting */
static Uint8 mouse_buffer[] = {
	0x00,	// BOXSOFT: delta X as signed, positive-left, updated from mouse_dx
	0x00,	// BOXSOFT: delta Y as signed, positive-up, updates from mouse_dy
	0x10,	// EXTENDED MSX: proto ID + extra buttons, TODO lower nibble should be updated with extra mouse buttons
	0x00,	// EXTENDED MSX: horizontal wheel, TODO horizontal wheel value should be updated by mouse wheel events
	0x40,	// ENTERMICE: extra bytes to read (incl this) + PS/2 Mouse ID [??] TODO: what is PS/2 mouse ID?
	0x11,	// ENTERMICE: hardware version major.minor TODO: what value should be here?
	0x11,	// ENTERMICE: firmware version major.minor TODO: what value should be here?
	0x5D	// ENTERMICE: Device ID, should be 5D for Entermice
};




int mouse_is_enabled ( void )
{
	if (!_mouse_grab && _mouse_pulse) {
		if (_mouse_wait_warn) {
			OSD("App may wait for mouse\nClick for grabbing");
			_mouse_wait_warn = 0;
			warn_for_mouse_grab = 0;
		}
	}
	return _mouse_grab || _mouse_pulse;
}



void mouse_reset_button ( void )
{
	_mouse_button_state = 0;
	mouse_buffer[2] &= 0xF0;	// extra buttons for MSX extended protocol should be cleared as well
}



void emu_mouse_button ( Uint8 button, int press )
{
	DEBUG("MOUSE BUTTON %d press = %d" NL, button, press);
	_mouse_button_state = press;
	if (press && _mouse_grab == 0) {
		//emu_osd_msg("Mouse grab. Press ESC to exit.");
		screen_grab(SDL_TRUE);
		_mouse_grab = 1;
		mouse_reset_button();
	}
}



void emu_mouse_motion ( int dx, int dy )
{
	if (!_mouse_grab) return; // not in mouse grab mode
	DEBUG("MOUSE MOTION event dx = %d dy = %d" NL, dx, dy);
	_mouse_dx -= dx;
	if (_mouse_dx > 127) _mouse_dx = 127;
	else if (_mouse_dx < -128) _mouse_dx = -128;
	_mouse_dy -= dy;
	if (_mouse_dy > 127) _mouse_dy = 127;
	else if (_mouse_dy < -128) _mouse_dy = -128;
}



void mouse_reset ( void )
{
	// _mouse_grab = 0; // fix to comment our: reset pressed with grabbed mouse
	nibble_counter = 0;
	_mouse_last_shift = 0;
	nibble = 0;
	_mouse_dx = 0;
	_mouse_dy = 0;
	watchdog = raster_time;
	mouse_reset_button();
	mouse_buffer[0] = mouse_buffer[1] = mouse_buffer[3] = 0;
}



static void check_watchdog ( void )
{
	/* mouse watchdog, using raster line times (~ 64 usec), 64 usec * 23 is about 1500 usec */
	int time = raster_time - watchdog;	// elapsed raster times since the last mouse_read()
	watchdog = raster_time;		// reset watchdog
	if (time > 22 || time < -22)	// negative case in case of raster_time counter warp-around (integer overflow)
		nibble_counter = 0;	// in case of timeout, nibble counter resets to zero
}



// Called from cpu.c in case of read port 0xB6, if mouse is enabled
Uint8 mouse_read ( void )
{
	Uint8 data = _mouse_button_state ? 0 : (is_entermice ? 2 : 4);
	if (kbd_selector > 0 && kbd_selector < 5)
		data |= (nibble >> (kbd_selector - 1)) & (is_entermice ? 2 : 1);
	_mouse_pulse = 0;
	check_watchdog();
	return data;
}



// Called from cpu.c in case of write of port 0xB7
void mouse_check_data_shift ( Uint8 val )
{
	if ((val & 2) == _mouse_last_shift)	// check of change on the RTS signal change
		return;
	_mouse_last_shift = val & 2;
	_mouse_pulse = 1;			// this variable is only for the emulator to keep track of mouse reading tries and display OSD, etc
	if (nibble_counter == 0) {
		// update mouse buffer byte 0 with delta X
		mouse_buffer[0] = ((unsigned int)_mouse_dx) & 0xFF;    // signed will be converted to unsigned
		_mouse_dx = 0;
	} else if (nibble_counter == 2) {
		// update mouse buffer byte 1 with delta Y
		mouse_buffer[1] = ((unsigned int)_mouse_dy) & 0xFF;    // signed will be converted to unsigned
		_mouse_dy = 0;
	}
	if (nibble_counter < mouse_protocol_nibbles) {
		// if nibble counter is below the constraint of the used mouse protocol, provide the upper or lower nibble of the buffer
		// based on the counter's lowest bit (ie, odd or even)
		nibble = ((nibble_counter & 1) ? (mouse_buffer[nibble_counter >> 1] & 15) : (mouse_buffer[nibble_counter >> 1] >> 4));
		nibble_counter++;
	} else
		nibble = 0;	// if we hit the max number of nibbles, we constantly provide zero as the nibble, until watchdog resets nibble counter in mouse_read()
}



int mouse_setup ( int cfg )
{
	if (cfg < 0)
		return setup_value;
	if (cfg == 0 || cfg > 6)
		cfg = 1;
	if (cfg > 3) {
		is_entermice = 1;
		mouse_protocol_nibbles = 1 << (cfg - 2);
	} else {
		is_entermice = 0;
		mouse_protocol_nibbles = 1 << (cfg + 1);
	}
	DEBUG("MOUSE: SETUP: using %s lines, with %d nibbles." NL, is_entermice ? "EnterMice" : "BoxSoft", mouse_protocol_nibbles);
	setup_value = cfg;
	return setup_value;
}



int emu_kbd(SDL_Keysym sym, int press)
{
	if (_mouse_grab && sym.scancode == SDL_SCANCODE_ESCAPE && press) {
		_mouse_grab = 0;
		screen_grab(SDL_FALSE);
	} else {
		const struct keyMappingTable_st *ke = keymap_resolve_event(sym);
		if (ke) {
			int sel  = ke->posep >> 4;
			int mask = 1 << (ke->posep & 15);
			if (mask < 0x100) {
				if (press)
					kbd_matrix[sel] &= 255 - mask;
				else
					kbd_matrix[sel] |= mask;
			} else
				return ke->posep;	// give special code back to be handled by the caller!
		}
	}
	return 0;	// no kbd should be handled by the caller ...
}


/* The rest of the file should be moved to an UI related source file if it worth later ... */


void check_malloc ( const void *p )
{
	if (p == NULL) {
		ERROR_WINDOW("Memory allocation error. Not enough memory?");
		exit(1);
	}
}



int _sdl_emu_secured_message_box_ ( Uint32 sdlflag, const char *msg )
{
        int mg = _mouse_grab, r;
        kbd_matrix_reset();
        mouse_reset_button();
	audio_stop();
	if (mg == SDL_TRUE) screen_grab(SDL_FALSE);
	r = SDL_ShowSimpleMessageBox(sdlflag, "Xep128", msg, sdl_win);
	if (mg == SDL_TRUE) screen_grab(mg);
	audio_start();
	return r;
}



int _sdl_emu_secured_modal_box_ ( const char *items_in, const char *msg )
{
	char items_buf[512], *items = items_buf;
	int buttonid, mg = _mouse_grab;
	SDL_MessageBoxButtonData buttons[16];
	SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_INFORMATION, /* .flags */
		sdl_win, /* .window */
		"Xep128", /* .title */
		msg, /* .message */
		0,	/* number of buttons, will be updated! */
		buttons,
		NULL	// &colorScheme
	};
	strcpy(items_buf, items_in);
	for (;;) {
		char *p = strchr(items, '|');
		switch (*items) {
			case '!':
				buttons[messageboxdata.numbuttons].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
				items++;
				break;
			case '?':
				buttons[messageboxdata.numbuttons].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
				items++;
				break;
			default:
				buttons[messageboxdata.numbuttons].flags = 0;
				break;
		}
		buttons[messageboxdata.numbuttons].text = items;
		buttons[messageboxdata.numbuttons].buttonid = messageboxdata.numbuttons;
		messageboxdata.numbuttons++;
		if (p == NULL) break;
		*p = 0;
		items = p + 1;
	}
	/* win grab, kbd/mouse emu reset etc before the window! */
	kbd_matrix_reset();
	mouse_reset_button();
	audio_stop();
	if (mg == SDL_TRUE) screen_grab(SDL_FALSE);
	SDL_ShowMessageBox(&messageboxdata, &buttonid);
	if (mg == SDL_TRUE) screen_grab(mg);
	audio_start();
	return buttonid;
}
