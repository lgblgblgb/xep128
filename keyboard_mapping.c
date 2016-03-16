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


/* _default_ scancode mapping. This is used to initialize configuration,
   but config file, command line options can override this. So, this table is
   _not_ used directly by the emulator! */


struct keyMappingDefault_st {
	const SDL_Scancode	code;
	const int		pos;
	const char		*description;
};


static const struct keyMappingDefault_st keyMappingDefaults[] = {
	{ SDL_SCANCODE_1,		0x31, "1"	},
	{ SDL_SCANCODE_2,		0x36, "2"	},
	{ SDL_SCANCODE_3,		0x35, "3"	},
	{ SDL_SCANCODE_4,		0x33, "4"	},
	{ SDL_SCANCODE_5,		0x34, "5"	},
	{ SDL_SCANCODE_6,		0x32, "6"	},
	{ SDL_SCANCODE_7,		0x30, "7"	},
	{ SDL_SCANCODE_8,		0x50, "8"	},
	{ SDL_SCANCODE_9,		0x52, "9"	},
	{ SDL_SCANCODE_0,		0x54, "0"	},
	{ SDL_SCANCODE_Q,		0x21, "Q"	},
	{ SDL_SCANCODE_W,		0x26, "W"	},
	{ SDL_SCANCODE_E,		0x25, "E"	},
	{ SDL_SCANCODE_R,		0x23, "R"	},
	{ SDL_SCANCODE_T,		0x24, "T"	},
	{ SDL_SCANCODE_Y,		0x22, "Y"	},
	{ SDL_SCANCODE_U,		0x20, "U"	},
	{ SDL_SCANCODE_I,		0x90, "I"	},
	{ SDL_SCANCODE_O,		0x92, "O"	},
	{ SDL_SCANCODE_P,		0x94, "P"	},
	{ SDL_SCANCODE_A,		0x16, "A"	},
	{ SDL_SCANCODE_S,		0x15, "S"	},
	{ SDL_SCANCODE_D,		0x13, "D"	},
	{ SDL_SCANCODE_F,		0x14, "F"	},
	{ SDL_SCANCODE_G,		0x12, "G"	},
	{ SDL_SCANCODE_H,		0x10, "H"	},
	{ SDL_SCANCODE_J,		0x60, "J"	},
	{ SDL_SCANCODE_K,		0x62, "K"	},
	{ SDL_SCANCODE_L,		0x64, "L"	},
	{ SDL_SCANCODE_RETURN,		0x76, "ENTER"	},
	{ SDL_SCANCODE_LSHIFT,		0x07, "SHIFT"	},
	{ SDL_SCANCODE_CAPSLOCK,	0x11, "CAPS"	},
	{ SDL_SCANCODE_Z,		0x06, "Z"	},
	{ SDL_SCANCODE_X,		0x05, "X"	},
	{ SDL_SCANCODE_C,		0x03, "C"	},
	{ SDL_SCANCODE_V,		0x04, "V"	},
	{ SDL_SCANCODE_B,		0x02, "B"	},
	{ SDL_SCANCODE_N,		0x00, "N"	},
	{ SDL_SCANCODE_M,		0x80, "M"	},
	{ SDL_SCANCODE_LCTRL,		0x17, "CTRL" 	},
	{ SDL_SCANCODE_SPACE,		0x86, "SPACE"	},
	{ SDL_SCANCODE_SEMICOLON,	0x63, ";"	},
	{ SDL_SCANCODE_LEFTBRACKET,	0x95, "["	},
	{ SDL_SCANCODE_RIGHTBRACKET,	0x66, "]"	},
	{ SDL_SCANCODE_APOSTROPHE,	0x65, ":"	},	// for EP : we map PC '
	{ SDL_SCANCODE_MINUS,		0x53, "-"	},
	{ SDL_SCANCODE_BACKSLASH,	0x01, "\\"	},
	{ SDL_SCANCODE_TAB,		0x27, "TAB"	},
	{ SDL_SCANCODE_ESCAPE,		0x37, "ESC"	},
	{ SDL_SCANCODE_INSERT,		0x87, "INS"	},
	{ SDL_SCANCODE_BACKSPACE,	0x56, "ERASE"	},
	{ SDL_SCANCODE_DELETE,		0x81, "DEL"	},
	{ SDL_SCANCODE_LEFT,		0x75, "LEFT"	},
	{ SDL_SCANCODE_RIGHT,		0x72, "RIGHT"	},
	{ SDL_SCANCODE_UP,		0x73, "UP"	},
	{ SDL_SCANCODE_DOWN,		0x71, "DOWN"	},
	{ SDL_SCANCODE_SLASH,		0x83, "/"	},
	{ SDL_SCANCODE_PERIOD,		0x84, "."	},
	{ SDL_SCANCODE_COMMA,		0x82, ","	},
	{ SDL_SCANCODE_EQUALS,		0x93, "@"	},	// for EP @ we map PC =
	{ SDL_SCANCODE_F1,		0x47, "F1"	},
	{ SDL_SCANCODE_F2,		0x46, "F2"	},
	{ SDL_SCANCODE_F3,		0x42, "F3"	},
	{ SDL_SCANCODE_F4,		0x40, "F4"	},
	{ SDL_SCANCODE_F5,		0x44, "F5"	},
	{ SDL_SCANCODE_F6,		0x43, "F6"	},
	{ SDL_SCANCODE_F7,		0x45, "F7"	},
	{ SDL_SCANCODE_F8,		0x41, "F8"	},
	{ SDL_SCANCODE_F9,		0x77, "F9"	},
	{ SDL_SCANCODE_HOME,		0x74, "HOLD"	},	// for EP HOLD we map PC HOME
	{ SDL_SCANCODE_END,		0x70, "STOP"	},	// for EP STOP we map PC END
	/* ---- Not real EP kbd matrix, used for extjoy emulation with numeric keypad ---- */
	{ SDL_SCANCODE_KP_5,		0xA0, "ExtJoy FIRE"	},	// for EP external joy FIRE  we map PC num keypad 5
	{ SDL_SCANCODE_KP_8,		0xA1, "ExtJoy UP"	},	// for EP external joy UP    we map PC num keypad 8
	{ SDL_SCANCODE_KP_2,		0xA2, "ExtJoy DOWN"	},	// for EP external joy DOWN  we map PC num keypad 2
	{ SDL_SCANCODE_KP_4,		0xA3, "ExtJoy LEFT"	},	// for EP external joy LEFT  we map PC num keypad 4
	{ SDL_SCANCODE_KP_6,		0xA4, "ExtJoy RIGHT"	},	// for EP external joy RIGHT we map PC num keypad 6
	/* ---- TODO move emu related keys (like screenshot, exit, fullscreen ...) here, with same "faked" position number to allow configurable settings! ---- */
	/* ---- end of table marker, must be the last entry ---- */
	{ 0, -1, NULL }
};


struct keyMappingTable_st {
	SDL_Scancode	code;
	Uint8		sel;
	Uint8		mask;
	Uint8		posep;
};


//static int keyMappingTable[256][3];
static struct keyMappingTable_st *keyMappingTable = NULL;
static int keyMappingTableSize = 0;



static void keymap_set_key ( SDL_Scancode code, int posep )
{
	int n = keyMappingTableSize;
	struct keyMappingTable_st *p = keyMappingTable;
	while (n && p->code != code) {
		n--;
		p++;
	}
	if (!n) {
		keyMappingTable = realloc(keyMappingTable, (keyMappingTableSize + 1) * sizeof(struct keyMappingTable_st));
		check_malloc(keyMappingTable);
		p = keyMappingTable + (keyMappingTableSize++);
		p->code = code;
	}
	p->sel  = posep >> 4;
	p->mask = 1 << (posep & 15);
	p->posep = posep;
}


int keymap_set_key_by_name ( const char *name, int posep )
{
	SDL_Scancode code = SDL_GetScancodeFromName(name);
	if (code == SDL_SCANCODE_UNKNOWN)
		return 1;
	keymap_set_key(code, posep);
	return 0;
}


void keymap_preinit_config_internal ( void )
{
	const struct keyMappingDefault_st *p = keyMappingDefaults;
	while (p->pos != -1) {
		keymap_set_key(p->code, p->pos);
		p++;
	}
}


static const char *keymap_get_key_description ( int pos )
{
	const struct keyMappingDefault_st *p = keyMappingDefaults;
	while (p->pos != -1) {
		if (p->pos == pos)
			return p->description;
		p++;
	}
	return NULL;
}


void keymap_dump_config ( FILE *fp )
{
	int n = keyMappingTableSize;
	struct keyMappingTable_st *p = keyMappingTable;
	fprintf(fp,
		"# Note: key names are SDL scan codes! Sometimes it's nothing to do with the letters" NL
		"# on your keyboard (eg some national layout, like Hungarian etc) but the \"physical\"" NL
		"# scan code assignment, eg the right neighbour of key \"L\" is \";\" even if your layout" NL
		"# means something different there!" NL NL
	);
	while (n--) {
		const char *name = SDL_GetScancodeName(p->code);
		const char *desc = keymap_get_key_description(p->posep);
		if (!name) {
			fprintf(fp, "# WARNING: cannot get SDL key name for epkey@%02x with SDL scan code of %d!" NL, p->posep, p->code);
		} else {
			fprintf(fp, "epkey@%02x = %s%s%s" NL,
				p->posep, name,
				desc ? "\t# for Enterprise key " : "",
				desc ? desc  : ""
			);
		}
		p++;
	}
}



int keymap_resolve_event ( SDL_Keysym sym, int press, Uint8 *matrix )
{
	int a;
	printf("KEY: scan=%d sym=%d press=%d\n", sym.scancode, sym.sym, press);
	for (a = 0; a < keyMappingTableSize; a++)
		if (keyMappingTable[a].code == sym.scancode) {
			if (press)
				matrix[keyMappingTable[a].sel] &= 255 - keyMappingTable[a].mask;
			else
				matrix[keyMappingTable[a].sel] |= keyMappingTable[a].mask;
			printf("  to EP key %dd mask=%02Xh\n", keyMappingTable[a].sel, keyMappingTable[a].mask);
			return 1;
		}
	return 0;
}

