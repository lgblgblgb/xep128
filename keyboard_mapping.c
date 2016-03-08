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


static const int keyMappingTable[][3]={
	{ SDL_SCANCODE_1,3,0x02},		/* 1 */
	{ SDL_SCANCODE_2,3,0x40},		/* 2 */
	{ SDL_SCANCODE_3,3,0x20},		/* 3 */
	{ SDL_SCANCODE_4,3,0x08},		/* 4 */
	{ SDL_SCANCODE_5,3,0x10},		/* 5 */
	{ SDL_SCANCODE_6,3,0x04},		/* 6 */
	{ SDL_SCANCODE_7,3,0x01},		/* 7 */
	{ SDL_SCANCODE_8,5,0x01},		/* 8 */
	{ SDL_SCANCODE_9,5,0x04},		/* 9 */
	{ SDL_SCANCODE_0,5,0x10},		/* 0 */
	{ SDL_SCANCODE_Q,2,0x02},		/* Q */
	{ SDL_SCANCODE_W,2,0x40},		/* W */
	{ SDL_SCANCODE_E,2,0x20},		/* E */
	{ SDL_SCANCODE_R,2,0x08},		/* R */
	{ SDL_SCANCODE_T,2,0x10},		/* T */
	{ SDL_SCANCODE_Y,2,0x04},		/* Y */
	{ SDL_SCANCODE_U,2,0x01},		/* U */
	{ SDL_SCANCODE_I,9,0x01},		/* I */
	{ SDL_SCANCODE_O,9,0x04},		/* O */
	{ SDL_SCANCODE_P,9,0x10},		/* P */
	{ SDL_SCANCODE_A,1,0x40},		/* A */
	{ SDL_SCANCODE_S,1,0x20},		/* S */
	{ SDL_SCANCODE_D,1,0x08},		/* D */
	{ SDL_SCANCODE_F,1,0x10},		/* F */
	{ SDL_SCANCODE_G,1,0x04},		/* G */
	{ SDL_SCANCODE_H,1,0x01},		/* H */
	{ SDL_SCANCODE_J,6,0x01},		/* J */
	{ SDL_SCANCODE_K,6,0x04},		/* K */
	{ SDL_SCANCODE_L,6,0x10},		/* L */
	{ SDL_SCANCODE_RETURN,7,0x40},		/* enter */
	{ SDL_SCANCODE_LSHIFT,0,0x80},		/* Shift */
	{ SDL_SCANCODE_CAPSLOCK,1,0x02},	/* capslock */
	//{ SDL_SCANCODE_LOCKINGCAPSLOCK, 0, 0x01},
	{ SDL_SCANCODE_Z,0,0x40},		/* Z */
	{ SDL_SCANCODE_X,0,0x20},		/* X */
	{ SDL_SCANCODE_C,0,0x08},		/* C */
	{ SDL_SCANCODE_V,0,0x10},		/* V */
	{ SDL_SCANCODE_B,0,0x04},		/* B */
	{ SDL_SCANCODE_N,0,0x01},		/* N */
	{ SDL_SCANCODE_M,8,0x01},		/* M */
	{ SDL_SCANCODE_LCTRL,1,0x80},		/* CTRL */
	{ SDL_SCANCODE_SPACE,8,0x40},		/* space */
	{ SDL_SCANCODE_SEMICOLON,6,0x08},	/* ; */
	{ SDL_SCANCODE_LEFTBRACKET,9,0x20},	/* [ */
	{ SDL_SCANCODE_RIGHTBRACKET,6,0x40},	/* ] */
	{ SDL_SCANCODE_APOSTROPHE,6,0x20},	/* ' for real, but we map EP : here */
	{ SDL_SCANCODE_MINUS,5,0x08},		/* - */
	{ SDL_SCANCODE_BACKSLASH,0,0x02},	/* \ */
	{ SDL_SCANCODE_TAB,2,0x80},		/* TAB */
	{ SDL_SCANCODE_ESCAPE,3,0x80},		/* ESC */
	{ SDL_SCANCODE_INSERT,8,0x80},		/* INS */
	{ SDL_SCANCODE_BACKSPACE,5,0x40},	/* ERASE */
	{ SDL_SCANCODE_DELETE,8,0x02},		/* DEL */
	{ SDL_SCANCODE_LEFT,7,0x20},		/* LEFT */
	{ SDL_SCANCODE_RIGHT,7,0x04},		/* RIGHT */
	{ SDL_SCANCODE_UP,7,0x08},		/* UP */
	{ SDL_SCANCODE_DOWN,7,0x02},		/* DOWN */
	{ SDL_SCANCODE_SLASH,8,0x08},		/* / */
	{ SDL_SCANCODE_PERIOD,8,0x10},		/* . */
	{ SDL_SCANCODE_COMMA,8,0x04},		/* , */
	{ SDL_SCANCODE_EQUALS,9,0x08},		/* = but we map as @ */
	{ SDL_SCANCODE_F1,4,0x80},		/* F1 */
	{ SDL_SCANCODE_F2,4,0x40},		/* F2 */
	{ SDL_SCANCODE_F3,4,0x04},		/* F3 */
	{ SDL_SCANCODE_F4,4,0x01},		/* F4 */
	{ SDL_SCANCODE_F5,4,0x10},		/* F5 */
	{ SDL_SCANCODE_F6,4,0x08},		/* F6 */
	{ SDL_SCANCODE_F7,4,0x20},		/* F7 */
	{ SDL_SCANCODE_F8,4,0x02},		/* F8 */
	{ SDL_SCANCODE_F9,7,0x80},		/* F9 */
	{ SDL_SCANCODE_HOME,7,0x10},		/* home, we map this as HOLD */
	{ SDL_SCANCODE_END,7,0x01},		/* end, we map this as STOP */
	/* Not real EP kbd matrix, used for extjoy emulation with numeric keypad */
	{ SDL_SCANCODE_KP_5,10,0x01},		/* FIRE (num keypad 5) */
	{ SDL_SCANCODE_KP_8,10,0x02},		/* UP   (num keypad 8) */
	{ SDL_SCANCODE_KP_2,10,0x04},		/* DOWN (num keypad 2) */
	{ SDL_SCANCODE_KP_4,10,0x08},		/* LEFT (num keypad 4) */
	{ SDL_SCANCODE_KP_6,10,0x10},		/* RIGHT(num keypad 6) */
	{ -1, -1, -1 }
};




int keymap_resolve_event ( SDL_Keysym sym, int press, Uint8 *matrix )
{
	int a = 0;
	printf("KEY: scan=%d sym=%d press=%d\n", sym.scancode, sym.sym, press);
	while (keyMappingTable[a][0] != -1)
		if (keyMappingTable[a][0] == sym.scancode) {
			if (press)
				matrix[keyMappingTable[a][1]] &= 255 - keyMappingTable[a][2];
			else
				matrix[keyMappingTable[a][1]] |= keyMappingTable[a][2];
			printf("  to EP key %dd mask=%02Xh\n", keyMappingTable[a][1], keyMappingTable[a][2]);
			return 1;
		} else
			a++;
	return 0;
}
