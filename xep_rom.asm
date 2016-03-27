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

; Note, symbols having name starting with "xepsym_" are extracted
; into a .h file can can be used by Xep128 itself!


	ORG 0xC000
	DB "EXOS_ROM"
	DW 0		; device chain (0 = no)
	JP	rom_main_entry_point
	DB	"!XEP_ROM"

MACRO	EXOS n
	RST	0x30
	DB	n
ENDMACRO




rom_main_entry_point:
	; The following invalid ED opcode is handled by the "ED trap" of the CPU emulator in Xep128
	; The trap handler itself will check EXOS action code, register values, and may modify
	; registers C and/or A as well. Also, it can pass data through the ROm area :) from 0xF8000
	DB	0xED, 0xBC
	; Argument of "JP" (the word at xepsym_jump) is filled/modified by Xep128 on the ED trap above!
	; Usually it's set to xepsym_print_xep_buffer if there is something to print at least :)
xepsym_jump_on_rom_entry = $ + 1
	JP	0

xepsym_print_xep_buffer:
	; Let save registers may be used by the check to print anything and/or print stuff
	; This also includes the possibly already modified A/C by the ED trap above!
	PUSH	AF
	PUSH	BC
	PUSH	DE
.nopush:
	LD	DE, xepsym_cobuf	; the ED trap modifies memory from here
	LD	A, (DE)
	LD	C, A
	INC	DE
	LD	A, (DE)
	LD	B, A
	OR	C		; word at F800 is the print size, 0 => no print
	JR	Z, xepsym_pop_and_ret
	INC	DE
	LD	A, 0xFF		; default channel
	EXOS	8		; write block EXOS function
xepsym_pop_and_ret:
	POP	DE
	POP	BC
	POP	AF
xepsym_ret:
	RET



set_exos_time:
xepsym_settime_hour = $ + 1
	LD	C, 0x88
xepsym_settime_minsec = $ + 1
	LD	DE, 0x8888
	EXOS	31		; EXOS set time
xepsym_setdate_year = $ + 1
	LD	C, 0x88
xepsym_setdate_monday = $ + 1
	LD	DE, 0x8888
	EXOS	33		; EXOS set date
	RET


xepsym_set_time:
	PUSH	AF
	PUSH	BC
	PUSH	DE
	CALL	set_exos_time
	JP	xepsym_print_xep_buffer.nopush


; Called on system initialization (EXOS action code 8)
; Currently it just sets date/time as xepsym_set_time would do as well ...
xepsym_system_init:
	PUSH	AF
	PUSH	BC
	PUSH	DE
	CALL	set_exos_time
	JP	xepsym_pop_and_ret

; Called on EXOS action code 1
xepsym_cold_reset:
	RET



; **** !! YOU MUST NOT PUT ANYTHING EXTRA AFTER THIS LINE, EMULATOR OVERWRITES THE AREA !! ****
xepsym_cobuf:
xepsym_cobuf_size = 0xFFF0 - xepsym_cobuf
