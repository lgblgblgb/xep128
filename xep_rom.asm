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


	ORG 0xC000
	DB "EXOS_ROM"
	DW 0		; device chain (0 = no)
	JP	rom_main_entry_point

MACRO	EXOS n
	RST	0x30
	DB	n
ENDMACRO


rom_main_entry_point:
	DB	0xED, 0xBC	; ED trap for the emulator, it may modify A, C
	PUSH	AF
	PUSH	BC
	PUSH	DE
	LD	DE, 0xF800 ; the ED trap modifies memory from here
	LD	A, (DE)
	LD	C, A
	INC	DE
	LD	A, (DE)
	LD	B, A
	OR	C		; word at F800 is the print size, 0 => no print
	JR	Z, .no_print
	INC	DE
	LD	A, 0xFF		; default channel
	EXOS	8		; write block EXOS function
.no_print:
	POP	DE
	POP	BC
	POP	AF
	RET

