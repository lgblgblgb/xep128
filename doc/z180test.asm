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

; *** EXOS Type-5 header
        DW      $500, END_OF_PRG - $100, 0, 0, 0, 0, 0, 0
; *** Execution starts here
        ORG     $100
	LD	SP, $100
	JP	main


EXOS_FN_VIDEO:		DB 6, "VIDEO:"
EXOS_FN_KEYBOARD:	DB 9, "KEYBOARD:"
EXOS_FN_BASIC:		DB 5, "BASIC"

CHANNEL_VIDEO           = 1
CHANNEL_KEYBOARD        = 2

SCREEN_WIDTH		= 42
SCREEN_HEIGHT		= 25
COLOUR_BORDER		= 0

ZOZO_Z180_PORT_BASE	= $40	; Zozo's EXOS mapped Z180 internal ports here

ITC_PORT		= $34 + ZOZO_Z180_PORT_BASE

MACRO EXOS n
	RST     $30
	DB      n
ENDMACRO

MACRO SETEXOS var,val
	LD      BC, $100 + (var)
	LD      D, val
	EXOS    16
ENDM

MACRO IPRINT n
	CALL	print_inline_text
	DB	.str_end - .str_start
.str_start:
	BYTE    n
.str_end:
ENDM

; used by the IPRINT macro, do not call it directly!
print_inline_text:
	POP	DE	; return address from the stack
	LD	A, (DE)
	LD	C, A
	LD	B, 0
	INC	DE
	PUSH	DE
	PUSH	BC
	LD	A, CHANNEL_VIDEO
	EXOS	8
	POP	HL
	POP	DE
	ADD	HL, DE
	JP	(HL)
	
	


ui_init_std:
	; Open keyboard channel
	LD	DE, EXOS_FN_KEYBOARD
	LD	A, CHANNEL_KEYBOARD
	EXOS	1
	; Prepare video parameters
	SETEXOS	22,    0	; video mode (we use character mode as the starting point!)
	SETEXOS	23,    0	; colour mode
	SETEXOS	24, SCREEN_WIDTH	; columns
	SETEXOS	25, SCREEN_HEIGHT	; rows
	SETEXOS 26, 0		; show status
	SETEXOS 27, COLOUR_BORDER	; border colour
	SETEXOS 28, 0 ; fix-BIAS
	; Open video channel
	LD	DE, EXOS_FN_VIDEO
	LD	A, CHANNEL_VIDEO
	EXOS	1
	; Font reset
	LD	A, CHANNEL_VIDEO
	LD	B, 4
	EXOS	11
	; Display video page
	LD	A, CHANNEL_VIDEO
	LD	BC, $101
	LD	DE, SCREEN_HEIGHT * $100 + 1
	EXOS	11
	SETEXOS	4, CHANNEL_VIDEO ; set default channel
	RET


getchar:
        LD      A, CHANNEL_KEYBOARD
        EXOS    5
        LD      A, B
        RET


exit_to_basic:
	IPRINT	"Press any key to exit to BASIC"
	CALL	getchar
        LD      DE, EXOS_FN_BASIC
        EXOS    26


save_regs:
	LD	(saved.bc), BC
	LD	(saved.de), DE
	LD	(saved.hl), HL
	LD	(saved.ix), IX
	LD	(saved.iy), IY
	PUSH	AF
	POP	BC
	LD	(saved.af), BC
	RET

load_regs:
	LD	BC, (saved.af)
	PUSH	BC
	POP	AF
	LD	BC, (saved.bc)
	LD	DE, (saved.de)
	LD	HL, (saved.hl)
	LD	IX, (saved.ix)
	LD	IY, (saved.iy)
	RET


printhexword:		; BC = hex word to print
	PUSH	BC
	LD	A, B
	CALL	printhexbyte
	POP	BC
	LD	A, C
printhexbyte:		; A = byte to print
	PUSH	AF
	[4] RRA
	CALL	printhexdigit
	POP	AF
printhexdigit:
	AND	$0F
	ADD	$90
	DAA
	ADC	$40
	DAA
	LD	B, A
	LD	A, CHANNEL_VIDEO
	EXOS	7
	RET


space:
	LD	A, CHANNEL_VIDEO
	LD	B, $20
	EXOS	7
	RET


show_regs:
	;IPRINT	"AF="
	LD	BC, (saved.af)
	CALL	printhexword
	;IPRINT	" BC="
	CALL	space
	LD	BC, (saved.bc)
	CALL	printhexword
	;IPRINT	" DE="
	CALL	space
	LD	BC, (saved.de)
	CALL	printhexword
	;IPRINT	" HL="
	CALL	space
	LD	BC, (saved.hl)
	CALL	printhexword
	;IPRINT	" IX="
	CALL	space
	LD	BC, (saved.ix)
	CALL	printhexword
	;IPRINT	" IY="
	CALL	space
	LD	BC, (saved.iy)
	CALL	printhexword
	CALL	space
	LD	A, (test_byte)
	CALL	printhexbyte
	IPRINT	"\r\n"
	RET

z180_opcode_trap_handler:
	DI			; anyway ...
	CALL	save_regs
	LD	BC, ITC_PORT	; high 8 bits must be zero, thus we use ...
	IN	A, (C)		; ... this form of I/O!
	PUSH	AF
	AND	$7F		; reset TRAP bit (bit7)
	OUT	(C), A		; test todo: try the test without this opc too!
	EI
	POP	AF
	CALL	printhexbyte	; write the value of the ITC register
	IPRINT	" Z180 TRAP "
	POP	BC		; POPs the address on the stack (?)
	CALL	printhexword	; show PC (BC, POPed by the prev. POP BC)
	CALL	space
	POP	BC   		; return address pushed by us during tests
	PUSH	BC		; also leave on the stack that value
	PUSH	BC		; this if for our "faked" RET at the end
	CALL	printhexword
	IPRINT	"\r\n"
	JP	load_regs
	; RET in load_regs will cause to return to our "prepared" ret.addr!


header:
	IPRINT  "Stupid Z180 test from LGB\r\n\r\nAF   BC   DE   HL   IX   IY   M\r\n"
	RET


main:
	CALL	ui_init_std
	CALL	header
	; Register Z180 trap routine, or such ...
	LD	A, $C3		; JP opcode
	LD	(0), A
	LD	BC, z180_opcode_trap_handler
	LD	(1), BC

	CALL	show_regs

	LD	BC, .test0
	PUSH	BC
	IPRINT	"-> INC IXH\r\n"
	CALL	load_regs
	INC	IXH
.test0: CALL	save_regs
	CALL	show_regs
	POP	BC
	
	LD	BC, .test1
	PUSH	BC
	IPRINT	"-> INC IXL\r\n"
	CALL	load_regs
	INC	IXL
.test1: CALL	save_regs
	CALL	show_regs
	POP	BC

	LD	BC, .test2
	PUSH	BC
	IPRINT	"-> INC IYH\r\n"
	CALL	load_regs
	INC	IYH
.test2: CALL	save_regs
	CALL	show_regs
	POP	BC

	LD	BC, .test3
	PUSH	BC
	IPRINT	"-> INC IYL\r\n"
	CALL	load_regs
	INC	IYL
.test3:	CALL	save_regs
	CALL	show_regs
	POP	BC

	LD	BC, .test4
	PUSH	BC
	IPRINT	"-> DD prefixed INC A !!\r\n"
	CALL	load_regs
	DB	$DD
	INC	A
.test4:	CALL	save_regs
	CALL	show_regs
	POP	BC

	IPRINT	"\r\nPress a key for next page"
	CALL	getchar
	IPRINT	"\r\n\r\n"
	CALL	header
	LD	BC, $00B3
	LD	(saved.bc), BC
	LD	BC, $AA00
	LD	(saved.af), BC
	LD	BC, test_byte
	LD	(saved.iy), BC
	CALL	show_regs

	LD	BC, .test5
	PUSH	BC
	IPRINT	"-> OUT 0 and IN (result: CMOS=FF NMOS=0)\r\n"
	CALL	load_regs
	BYTE	$ED, $71
	IN	A, (C)
.test5:	CALL	save_regs
	CALL	show_regs
	POP	BC

	LD	BC, .test6
	PUSH	BC
	IPRINT	"-> set 6,(iy+0)->a\r\n"
	CALL	load_regs
	DB	$FD, $CB, $00, $F7
.test6:	CALL	save_regs
	CALL	show_regs
	POP	BC

	
	XOR	A
	LD	(0), A
	LD	(1), A
	LD	(2), A

	IPRINT	"\r\nEnd of tests\r\n"

	JP	exit_to_basic
	

test_byte:	DB 2

saved:
.af:	DW 0
.bc:	DW 0
.de:	DW 0
.hl:	DW 0
.ix:	DW 0
.iy:	DW 0

END_OF_PRG:

