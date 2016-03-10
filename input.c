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

static int _mouse_dx, _mouse_dy, _mouse_grab = 0;
static Uint8 _mouse_data_byte, _mouse_data_half, _mouse_last_shift, _mouse_read_state, _mouse_button_state;
static int _mouse_pulse = 0; // try to detect SymbOS or other simllar tests
static int _mouse_wait_warn = 1;
static int _mouse_entermice = 0;


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
}


void emu_mouse_button(Uint8 button, int press)
{
	printf("MOUSE BUTTON %d press = %d\n", button, press);
	_mouse_button_state = press;
	if (press && _mouse_grab == 0) {
		//emu_osd_msg("Mouse grab. Press ESC to exit.");
		screen_grab(SDL_TRUE);
		_mouse_grab = 1;
		mouse_reset_button();
	}
}


void emu_mouse_motion(int dx, int dy)
{
	if (!_mouse_grab) return; // not in mouse grab mode
	printf("MOUSE MOTION event dx = %d dy = %d\n", dx, dy);
	_mouse_dx -= dx;
	if (_mouse_dx > 127) _mouse_dx = 127;
	else if (_mouse_dx < -128) _mouse_dx = -128;
	_mouse_dy -= dy;
	if (_mouse_dy > 127) _mouse_dy = 127;
	else if (_mouse_dy < -128) _mouse_dy = -128;
}


void mouse_reset(void)
{
	// _mouse_grab = 0; // fix to comment our: reset pressed with grabbed mouse
	_mouse_read_state = 0;
	_mouse_last_shift = 0;
	_mouse_data_byte = 0;
	_mouse_data_half = 0;
	_mouse_dx = 0;
	_mouse_dy = 0;
}


Uint8 mouse_read(void)
{
	Uint8 data = _mouse_button_state ? 0 : (_mouse_entermice ? 2 : 4);
	if (kbd_selector > 0 && kbd_selector < 5)
		data |= (_mouse_data_half >> (kbd_selector - 1)) & (_mouse_entermice ? 2 : 1);
	_mouse_pulse = 0;
	return data;
}


void mouse_check_data_shift(Uint8 val)
{
	if ((val & 2) == _mouse_last_shift) return;
	_mouse_last_shift = val & 2;
	_mouse_pulse = 1;
	switch (_mouse_read_state) {
		case 0:
			_mouse_data_byte = ((unsigned int)_mouse_dx) & 0xFF;	// signed will be converted to unsigned
			_mouse_dx = 0;
			_mouse_data_half = _mouse_data_byte >> 4;
			break;
		case 1:
			_mouse_data_half = _mouse_data_byte & 15;
			break;
		case 2:
			_mouse_data_byte = ((unsigned int)_mouse_dy) & 0xFF;	// signed will be converted to unsigned
			_mouse_dy = 0;
			_mouse_data_half = _mouse_data_byte >> 4;
			break;
		case 3:
			_mouse_data_half = _mouse_data_byte & 15;
			break;
		default:
			_mouse_data_half = 0;					// for EnterMice, having 8 nibbles, but 0 for the last four ones
			break;
	}
	_mouse_read_state = (_mouse_read_state + 1) & (_mouse_entermice ? 7 : 3);	// 4 or 8 nibble protocol
}


int mouse_entermice ( int entermice )
{
	if (entermice >= 0)
		_mouse_entermice = entermice;
	return _mouse_entermice;
}


void emu_kbd(SDL_Keysym sym, int press)
{
	if (_mouse_grab && sym.scancode == SDL_SCANCODE_ESCAPE && press) {
		_mouse_grab = 0;
		screen_grab(SDL_FALSE);
	} else
		(void)keymap_resolve_event(sym, press, kbd_matrix);
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
