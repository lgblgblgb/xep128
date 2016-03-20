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

static int move_dx, move_dy, _mouse_grab = 0, nibble_counter;
static int wheel_dx, wheel_dy;
static Uint8 nibble, mouse_data_row0;
static int _mouse_last_shift = -1;
static int _mouse_pulse = 0; // try to detect SymbOS or other simllar tests
static int _mouse_wait_warn = 1;
extern Uint32 raster_time; // we use Nick's raster_time "timing source" because it's kinda free without introducing another timer source
static Uint32 watchdog;		// watchdog value (compared to raster_time)

// Currently we don't modify this, so it's a macro (upper 5 bits of port B6 on read), tape in '1' for top bits
#define port_b6_misc 0xC0

/* The mouse buffer. nibble_counter shows which nibble is to read (thus "nibble_counter >> 1" is the byte pointer actually.
   mouse_protocol_nibbles limits the max nibbles to read, ie it's 4 (= 2 bytes) for boxsoft protocol for the default setting */
static Uint8 mouse_buffer[] = {
	0x00,	// BOXSOFT: delta X as signed, positive-left, updated from mouse_dx
	0x00,	// BOXSOFT: delta Y as signed, positive-up, updates from mouse_dy
	0x10,	// EXTENDED MSX: proto ID + extra buttons on lower nibble (I update those - lower nibble - on mouse SDL events)
	0x00,	// EXTENDED MSX: horizontal wheel (it's splitted for horizontal/Z, but according to entermice it's handled as a 8 bit singed now for a single wheel, so I do this as well)
	0x44,	// ENTERMICE: extra bytes to read (incl this: 4 now) + PS/2 Mouse ID [it's 4 now ...]
	0x14,	// ENTERMICE: hardware version major.minor
	0x19,	// ENTERMICE: firmware version major.minor
	0x5D	// ENTERMICE: Device ID, should be 5D for Entermice
};



#define WATCHDOG_USEC(n) (n / 64)

/* Values can be used in mouse modes, buttons[] array to map PC mouse buttons to EP related mouse buttons 
   The first two are mapped then according to the button*_mask of the mode struct.
   The EX buttons instructs setting the button status in mouse buffer directly at byte 3, lower nibble
*/
#define BUTTON_MAIN	1
#define BUTTON_OTHER	2
#define BUTTON_EX3	3
#define BUTTON_EX4	4
#define BUTTON_EX5	5

/* Values can be used in *_mask options in mouse modes struct */
#define J_COLUMN	1
#define K_COLUMN	2
#define L_COLUMN	4

struct mouse_modes_st {
	const char *name;	// some name for the given mouse protocol/mode ...
	int buttons[5];		// button map: indexed by emulator given PC left/middle/right/x1/x2 (in this order), with values of the BUTTON_* macros above to map to protcol values
	int nibbles;		// nibbles in the protocol (before the constant zero answer or warping around if warp is non-zero)
	int wrap;		// wraps around nibble counter automatically or not (not = constant zero nibble answer, watchdog can only reset the counter)
	int watchdog;		// watchdog time-out, or -1, for not using watchdog, times are Xep128 specific use the WATCHDOG_USEC macro to convert from usec!
	int data_mask;		// bit mask for mouse data read (J/K/L = 1 / 2 / 4), you can use *_COLUMN macros
	int button_mask;	// bit mask for "main" button J/K/L like with data_mask
	int button2_mask;	// bit mask for the "other" (second) button, J/K/L like with data_mask OR in this case it can be 0 to disable! (not with other *_mask settings!)
};

/* Definition of mouse modes follows :) */
static const struct mouse_modes_st mouse_modes[] = {
	{	// MODE - 0: NOT USED POSITION
	}, {	// MODE - 1:
		name:		"BoxSoft J-col",
		buttons:	{ BUTTON_MAIN, BUTTON_EX3, BUTTON_OTHER, BUTTON_EX4, BUTTON_EX5 },	// mapping for SDL left/middle/right/X1/X2 events to EP
		nibbles:	4,
		wrap:		0,
		watchdog:	WATCHDOG_USEC(1500),
		data_mask:	J_COLUMN,
		button_mask:	J_COLUMN,
		button2_mask:	L_COLUMN
	}, {	// MODE - 2:
		name:		"ExtMSX J-col",
		buttons:	{ BUTTON_MAIN, BUTTON_EX3, BUTTON_OTHER, BUTTON_EX4, BUTTON_EX5 },	// mapping for SDL left/middle/right/X1/X2 events to EP
		nibbles:	8,
		wrap:		0,
		watchdog:	WATCHDOG_USEC(1500),
		data_mask:	J_COLUMN,
		button_mask:	J_COLUMN,
		button2_mask:	L_COLUMN
	}, {	// MODE - 3:
		name:		"EnterMice J-col",
		buttons:	{ BUTTON_MAIN, BUTTON_EX3, BUTTON_OTHER, BUTTON_EX4, BUTTON_EX5 },	// mapping for SDL left/middle/right/X1/X2 events to EP
		nibbles:	16,
		wrap:		0,
		watchdog:	WATCHDOG_USEC(1500),
		data_mask:	J_COLUMN,
		button_mask:	J_COLUMN,
		button2_mask:	L_COLUMN
	}, {	// MODE - 4:
		name:		"BoxSoft K-col",
		buttons:	{ BUTTON_MAIN, BUTTON_EX3, BUTTON_OTHER, BUTTON_EX4, BUTTON_EX5 },	// mapping for SDL left/middle/right/X1/X2 events to EP
		nibbles:	4,
		wrap:		0,
		watchdog:	WATCHDOG_USEC(1500),
		data_mask:	K_COLUMN,
		button_mask:	K_COLUMN,
		button2_mask:	L_COLUMN
	}, {	// MODE - 5:
		name:		"ExtMSX K-col",
		buttons:	{ BUTTON_MAIN, BUTTON_EX3, BUTTON_OTHER, BUTTON_EX4, BUTTON_EX5 },	// mapping for SDL left/middle/right/X1/X2 events to EP
		nibbles:	8,
		wrap:		0,
		watchdog:	WATCHDOG_USEC(1500),
		data_mask:	K_COLUMN,
		button_mask:	K_COLUMN,
		button2_mask:	L_COLUMN
	}, {	// MODE - 6:
		name:		"EnterMice K-col",
		buttons:	{ BUTTON_MAIN, BUTTON_EX3, BUTTON_OTHER, BUTTON_EX4, BUTTON_EX5 },	// mapping for SDL left/middle/right/X1/X2 events to EP
		nibbles:	16,
		wrap:		0,
		watchdog:	WATCHDOG_USEC(1500),
		data_mask:	K_COLUMN,
		button_mask:	K_COLUMN,
		button2_mask:	L_COLUMN
	}
};

#define LAST_MOUSE_MODE ((sizeof(mouse_modes) / sizeof(const struct mouse_modes_st)) - 1)

static const struct mouse_modes_st *mode;	// current mode mode, pointer to the selected mouse_modes
int mouse_mode;					// current mode, with an integer




int mouse_mode_description ( int cfg, char *buffer )
{
	if (cfg == 0)
		cfg = mouse_mode;
	if (cfg < 1 || cfg > LAST_MOUSE_MODE) {
		sprintf(buffer, "#%d *** Invalid mouse mode ***", cfg);
		return 1;
	} else {
		sprintf(
			buffer,
			"#%d (%s) nibbles=%d wrap=%d watchdog=%d data_mask=%d button_mask=%d button2_mask=%d",
			cfg,
			mouse_modes[cfg].name,
			mouse_modes[cfg].nibbles,
			mouse_modes[cfg].wrap,
			mouse_modes[cfg].watchdog,
			mouse_modes[cfg].data_mask,
			mouse_modes[cfg].button_mask,
			mouse_modes[cfg].button2_mask
		);
		return 0;
	}
}



static int mouse_is_enabled ( void )
{
	if (!_mouse_grab && _mouse_pulse) {
		if (_mouse_wait_warn) {
			OSD("Application may wait for mouse\nLeft click for grabbing, please!");
			_mouse_wait_warn = 0;
			warn_for_mouse_grab = 0;
		}
	}
	return _mouse_grab || _mouse_pulse;
}



void mouse_reset_button ( void )
{
	mouse_data_row0 = 7;		// leave top bits clear (used for another thing on port B6 read ...)
	mouse_buffer[2] &= 0xF0;	// extra buttons for MSX extended protocol should be cleared as well
}


static inline void set_button ( Uint8 *storage, int mask, int pressed )
{
	if (pressed)
		*storage |= mask;
	else
		*storage &= 255 - mask;
}



void emu_mouse_button ( Uint8 sdl_button, int press )
{
	const char *name;
	int id;
	switch (sdl_button) {
		case SDL_BUTTON_LEFT:
			name = "left";
			id = 0;
			break;
		case SDL_BUTTON_MIDDLE:
			name = "middle";
			id = 1;
			break;
		case SDL_BUTTON_RIGHT:
			name = "right";
			id = 2;
			break;
		case SDL_BUTTON_X1:
			name = "x1";
			id = 3;
			break;
		case SDL_BUTTON_X2:
			name = "x2";
			id = 4;
			break;
		default:
			name = "UNKNOWN";
			id = -1;
			break;
	}
	DEBUG("MOUSE: button SDL#%d XEP#%d (%s) %s" NL, sdl_button, id, name, press ? "pressed" : "released");
	if (id == -1) {
		DEBUG("MOUSE: unknown mouse button on SDL level (see previous MOUSE: line)!!" NL);
		return;	// unknown mouse button??
	}
	if (sdl_button == SDL_BUTTON_LEFT && press && _mouse_grab == 0) {
		//emu_osd_msg("Mouse grab. Press ESC to exit.");
		screen_grab(SDL_TRUE);
		_mouse_grab = 1;
		mouse_reset_button();
	}
	switch (mode->buttons[id]) {
		case BUTTON_MAIN:
			set_button(&mouse_data_row0, mode->button_mask, !press);
			break;
		case BUTTON_OTHER:
			set_button(&mouse_data_row0, mode->button2_mask, !press);
			break;
		case BUTTON_EX3:
			set_button(&mouse_buffer[2], 1, press);
			break;
		case BUTTON_EX4:
			set_button(&mouse_buffer[2], 2, press);
			break;
		case BUTTON_EX5:
			set_button(&mouse_buffer[2], 4, press);
			break;
		default:
			DEBUG("MOUSE: used mouse button cannot be mapped for the given mouse mode (map=%d), ignored" NL, mode->buttons[id]);
			break;
	}
}



void emu_mouse_motion ( int dx, int dy )
{
	if (!_mouse_grab) return; // not in mouse grab mode
	DEBUG("MOUSE MOTION event dx = %d dy = %d" NL, dx, dy);
	move_dx -= dx;
	if (move_dx > 127) move_dx = 127;
	else if (move_dx < -128) move_dx = -128;
	move_dy -= dy;
	if (move_dy > 127) move_dy = 127;
	else if (move_dy < -128) move_dy = -128;
}



void emu_mouse_wheel ( int x, int y, int flipped )
{
	flipped = flipped ? -1 : 1;
	wheel_dx -= x * flipped;
	if (wheel_dx > 127) wheel_dx = 127;
	else if (wheel_dx < -128) wheel_dx = -128;
	wheel_dy -= y * flipped;
	if (wheel_dy > 127) wheel_dy = 127;
	else if (wheel_dy < -128) wheel_dy = -128;
}



void mouse_reset ( void )
{
	// _mouse_grab = 0; // fix to comment our: reset pressed with grabbed mouse
	nibble_counter = 0;
	//if (_mouse_last_shift == -1)
	_mouse_last_shift = 0;
	nibble = 0;
	move_dx = 0;
	move_dy = 0;
	wheel_dx = 0;
	wheel_dy = 0;
	watchdog = raster_time;
	mouse_reset_button();
	mouse_buffer[0] = mouse_buffer[1] = mouse_buffer[3] = 0;
}



static void check_watchdog ( void )
{
	/* mouse watchdog, using raster line times (~ 64 usec), 64 usec * 23 is about 1500 usec */
	int time = raster_time - watchdog;	// elapsed raster times since the last mouse_read()
	watchdog = raster_time;		// reset watchdog
	if (mode->watchdog >= 0 && (time > mode->watchdog || time < 0))	// negative case in case of raster_time counter warp-around (integer overflow)
		nibble_counter = 0;	// in case of timeout, nibble counter resets to zero
}




// Called from cpu.c in case of read port 0xB6
Uint8 read_port_b6 ( void )
{
	_mouse_pulse = 0;
	switch (kbd_selector) {
		/* joystick-1 or mouse related */
		case  0:
			if (mouse_is_enabled())
				return mouse_data_row0 | port_b6_misc;
			else
				return ( kbd_matrix[10]       & 1) | 6 | port_b6_misc;
		case  1:
			if (mouse_is_enabled())
				return ((nibble & 1) ? mode->data_mask : 0) | (7 - mode->data_mask) | port_b6_misc;
			else
				return ((kbd_matrix[10] >> 1) & 1) | 6 | port_b6_misc;
		case  2:
			if (mouse_is_enabled())
				return ((nibble & 2) ? mode->data_mask : 0) | (7 - mode->data_mask) | port_b6_misc;
			else
				return ((kbd_matrix[10] >> 2) & 1) | 6 | port_b6_misc;
		case  3:
			if (mouse_is_enabled())
				return ((nibble & 4) ? mode->data_mask : 0) | (7 - mode->data_mask) | port_b6_misc;
			else
				return ((kbd_matrix[10] >> 3) & 1) | 6 | port_b6_misc;
		case  4:
			if (mouse_is_enabled())
				return ((nibble & 8) ? mode->data_mask : 0) | (7 - mode->data_mask) | port_b6_misc;
			else
				return ((kbd_matrix[10] >> 4) & 1) | 6 | port_b6_misc;
		/* always joystick#2 on J-column (bit 0) */
		case  5:	return ( kbd_matrix[10]       & 1) | 6 | port_b6_misc;	// replicate external joy emu here as joy-2
		case  6:	return ((kbd_matrix[10] >> 1) & 1) | 6 | port_b6_misc;
		case  7:	return ((kbd_matrix[10] >> 2) & 1) | 6 | port_b6_misc;
		case  8:	return ((kbd_matrix[10] >> 3) & 1) | 6 | port_b6_misc;
		case  9:	return ((kbd_matrix[10] >> 4) & 1) | 6 | port_b6_misc;
		/* and if not ... */
		case -1:	return 7 | port_b6_misc;
		default:	return 0 | port_b6_misc;	// should not happen, I guess ...
	}
}



// Called from cpu.c in case of write of port 0xB7
void mouse_check_data_shift ( Uint8 val )
{
	if ((val & 2) == _mouse_last_shift)	// check of change on the RTS signal change
		return;
	_mouse_last_shift = val & 2;
	_mouse_pulse = 1;			// this variable is only for the emulator to keep track of mouse reading tries and display OSD, etc
	check_watchdog();
	if (nibble_counter >= mode->nibbles && mode->wrap)	// support nibble counter wrap-around mode, if the current mouse mode directs that
		nibble_counter = 0;
	// note:  information larger than one nibble shouldn't updated by the mouse SDL events directly in mouse_buffer, because it's possible
	// that between the reading of two nibbles that is modified. To avoid this, these things are updated here at a well defined counter state only:
	if (nibble_counter == 0) {
		// update mouse buffer byte 0 with delta X
		//mouse_buffer[0] = ((unsigned int)move_dx) & 0xFF;    // signed will be converted to unsigned
		mouse_buffer[0] = move_dx;
		move_dx = 0;
	} else if (nibble_counter == 2) {
		// update mouse buffer byte 1 with delta Y
		//mouse_buffer[1] = ((unsigned int)move_dy) & 0xFF;    // signed will be converted to unsigned
		mouse_buffer[1] = move_dy;
		move_dy = 0;
	} else if (nibble_counter == 6) {	// this may not be used at all, if mouse_protocol_nibbles limits the available nibbles to read, boxsoft will not read this ever!
		mouse_buffer[3] = wheel_dy;
		wheel_dy = 0;
	}
	if (nibble_counter < mode->nibbles) {
		// if nibble counter is below the constraint of the used mouse protocol, provide the upper or lower nibble of the buffer
		// based on the counter's lowest bit (ie, odd or even)
		nibble = ((nibble_counter & 1) ? (mouse_buffer[nibble_counter >> 1] & 15) : (mouse_buffer[nibble_counter >> 1] >> 4));
		nibble_counter++;
	} else
		nibble = 0;	// if we hit the max number of nibbles, we constantly provide zero as the nibble, until watchdog resets nibble counter in mouse_read()
}



int mouse_setup ( int cfg )
{
	char buffer[128];
	if (cfg < 0)
		return mouse_mode;
	if (cfg < 1 || cfg > LAST_MOUSE_MODE)
		cfg = 1;
	mouse_mode = cfg;
	mode = &mouse_modes[cfg];
	mouse_mode_description(cfg, buffer);
	DEBUG("MOUSE: SETUP: %s" NL, buffer);
	mouse_reset();
	return cfg;
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
