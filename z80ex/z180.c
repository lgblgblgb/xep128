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

#ifdef Z80EX_Z180_SUPPORT

#warning "LGB: Z180 support is _very_ limited, please read file z80ex/README."
int z80ex_z180 = 0;
extern void z80ex_invalid_for_z180(void);
#define MULT_OP_T_STATES 13

#else

#define z80ex_z180 0
static void z80ex_invalid_for_z180 ( void ) {}

#endif




static const int opcodes_ddfd_bad_for_z180[0x100] = {
/*	0 1 2 3 4 5 6 7 8 9 A B C D E F */

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0. */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1. */
	0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,0,	/* 2. */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 3. */
	0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,	/* 4. */
	0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,	/* 5. */
	1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,	/* 6. */
	0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,	/* 7. */
	0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,	/* 8. */
	0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,	/* 9. */
	0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,	/* A. */
	0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,	/* B. */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* C. */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* D. */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* E. */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0		/* F. */
};

