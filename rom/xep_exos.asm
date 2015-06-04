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
	JP	main

MACRO	EXOS n
	RST	0x30
	DB	n
ENDMACRO


main:
	LD	A, C
	CP	2
	JR	Z, exos_command
	CP	3
	JR	Z, exos_help
	RET

exos_help:
	LD	A, B
	CP	0
	JR	Z, rom_list_help
	LD	A, "H"
	OUT	(0x30), A
	LD	A, "E"
	OUT	(0x30), A
	LD	A, "L"
	OUT	(0x30), A
	LD	A, "P"
	OUT	(0x30), A
	JR	exos_command.answer

rom_list_help:
	PUSH	BC
	PUSH	DE
	LD	A, "V"
	OUT	(0x30), A
	LD	A, "E"
	OUT	(0x30), A
	LD	A, "R"
	OUT	(0x30), A
	CALL	exos_command.answer
	POP	DE
	POP	BC
	RET


exos_command:

	LD	A, B
	CP	3
	RET	NZ
	PUSH	DE
	POP	IX
	LD	A, (IX+1)
	CP	'X'
	RET	NZ
	LD	A, (IX+2)
	CP	'E'
	RET	NZ
	LD	A, (IX+3)
	CP	'P'
	RET	NZ
	; Seems to be our XEP command!

	LD	A, (DE)
	SUB	3
	LD	B, A
	INC	DE
	INC	DE
	INC	DE
	INC	DE
.send_command:
	LD	A, (DE)
	OUT	(0x30), A
	INC	DE
	DJNZ	.send_command
.answer:
	XOR	A
	OUT	(0x30), A ; send end of command signal

.answer_loop:
	IN	A, (0x30)
	OR	A
	JR	Z, .end_of_answer
	LD	B, A
	LD	A, 0xFF
	EXOS	7
	JR	.answer_loop
.end_of_answer:
	XOR	A ; no error code
	LD	C, A
	RET



[0x10000 - $] DB 0xFF


