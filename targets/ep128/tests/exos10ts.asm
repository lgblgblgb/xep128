/* Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
   Copyright (C)2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
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
	CALL	main
	JP	exit_to_basic


EXOS_FN_VIDEO:		DB 6, "VIDEO:"
EXOS_FN_KEYBOARD:	DB 9, "KEYBOARD:"
EXOS_FN_BASIC:		DB 5, "BASIC"

TEST_FILE_EXDOS		DB 10, 'F:TEST.FIL'
TEST_FILE_FILEIO	DB 13, 'FILE:TEST.FIL'
TEST_FILE_NAKED		DB 8, 'TEST.FIL'

CHANNEL_VIDEO           = 1
CHANNEL_KEYBOARD        = 2
CHANNEL_FILE		= 16

SCREEN_WIDTH		= 42
SCREEN_HEIGHT		= 25
COLOUR_BORDER		= 0

test_file:	DW	TEST_FILE_EXDOS


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
	SETEXOS	22,    2	; video mode (we use character mode as the starting point!)
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


getkey:
        LD      A, CHANNEL_KEYBOARD
        EXOS    5
        LD      A, B
        RET


exit_to_basic:
	IPRINT	"\r\n\r\nPress any key to exit to BASIC"
	CALL	getkey
        LD      DE, EXOS_FN_BASIC
        EXOS    26
	DI
	HALT


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


print_exos_string:
	LD	A, (DE)
	INC	DE
	LD	C, A
	LD	B, 0
	LD	A, CHANNEL_VIDEO
	EXOS	8
	RET


show_exos_error:
	PUSH	AF
	JR	Z, .ok
	CALL	printhexbyte
	CALL	space
	LD	DE, BUFFER_ERROR
	POP	AF
	PUSH	AF
	EXOS	28			; Explain error code EXOS call
	CALL	print_exos_string
	IPRINT	"\r\n"
	POP	AF
	RET
.ok:
	CALL	printhexbyte
	IPRINT	" (OK)\r\n"
	POP	AF
	RET


; C must be write flags on input!
exos10_test:
	PUSH	BC
	IPRINT	"*** Testing C = "
	POP	BC
	PUSH	BC
	LD	A, C
	CALL	printhexbyte
	IPRINT	"\r\nBuffer before: "
	CALL	print_st	; print the status buffer BEFORE the call
	; Issue the call itself
	IPRINT	"return code (A): "
	POP	BC
	LD	A, CHANNEL_FILE
	LD	DE, BUFFER_ST
	EXOS	10
	PUSH	BC
	CALL	show_exos_error
	; Print C
	IPRINT	"C on output   = "
	POP	BC
	LD	A, C
	CALL	printhexbyte
	IPRINT	"\r\nBuffer after : "
	CALL	print_st	; print the status buffer AFTER the call
	CALL	getkey
	IPRINT	"\r\n"
	RET



print_st:
	LD	HL, BUFFER_ST
	LD	B, 16
.loop:
	PUSH	BC
	PUSH	HL
	LD	A, (HL)
	CALL	printhexbyte
	CALL	space
	POP	HL
	POP	BC
	INC	HL
	DJNZ	.loop
	IPRINT	"\r\n"
	RET


	; HL = pointer value ... only 16 bit now :D
set_p_in_st:
	LD	IX, BUFFER_ST
	LD	(IX), L
	LD	(IX+1), H
	LD	(IX+2), 0
	LD	(IX+3), 0
	RET


select_test_mode:
	CALL	getkey
	LD	DE, TEST_FILE_EXDOS
	CP	'e'
	RET	Z
	LD	DE, TEST_FILE_NAKED
	CP	'n'
	RET	Z
	LD	DE, TEST_FILE_FILEIO
	CP	'f'
	RET	Z
	CP	'x'
	JR	NZ, select_test_mode
	OR	A	; will set NZ
	RET


; -----------------------------------------------------------------------------
; -----------------------------------------------------------------------------


main:
	CALL	ui_init_std
.again:
	IPRINT	"LGB's ugly EXOS-10 test.\r\nPlease select with keys: e = EXDOS or f = FILE: or n = naked or x = to exit\r\nYour selection (e/f/n/x)? "
	CALL	select_test_mode
	RET	NZ
	LD	(test_file), DE
	IPRINT	"\r\nTest file selected = "
	LD	DE, (test_file)
	CALL	print_exos_string
	; Create/recreate our test file ...
	IPRINT  "\r\nPress a key after each steps where printing stops.\r\n\r\nCreating channel (EXOS 2): "
	LD	A, CHANNEL_FILE
	LD	DE, (test_file)
	EXOS	2
	CALL	show_exos_error
	RET	NZ
	IPRINT	"\r\nFill file with some bytes (EXOS 8): "
	LD	A, CHANNEL_FILE
	LD	DE, 0
	LD	BC, 4096	; 4K of data
	EXOS	8
	CALL	show_exos_error
	RET	NZ
	IPRINT	"\r\nClosing channel (EXOS 3): "
	LD	A, CHANNEL_FILE
	EXOS	3
	CALL	show_exos_error
	RET	NZ
	; Open file
	IPRINT	"\r\nNow opening channel (EXOS 1): "
	LD	A, CHANNEL_FILE
	LD	DE, (test_file)
	EXOS	1
	CALL	show_exos_error
	RET	NZ
	; Buffer with some values just to check what EXOS fills
	LD	HL, BUFFER_ST
	LD	B, 16
	LD	A, 0xFF
.filler:
	LD	(HL), A
	INC	HL
	DJNZ	.filler
	; Issue EXOS 10 without writing anything!
	LD	C, 0
	CALL	exos10_test
	; Now with setting FP to zero
	LD	HL, 0
	CALL	set_p_in_st
	LD	C, 1
	CALL	exos10_test
	; Now with setting FP to a large value ...
	LD	HL, 0xFFFF	; Unsane offset
	CALL	set_p_in_st
	LD	C, 1
	CALL	exos10_test
	; Trying to read ...
	IPRINT	"Trying to read after seeking to 0xFFFF\r\nreturn (A) = "
	LD	A, CHANNEL_FILE
	EXOS	5
	CALL	show_exos_error
	CALL	getkey
	; Re-read FP (after a failed read to see if FP is changed or not ...) ...
	IPRINT	"\r\nRe-read FP ...\r\n"
	LD	C, 0
	CALL 	exos10_test
	; With some bogus C
	LD	C, 0xFF
	CALL	exos10_test
	; Try to write a single character into the file ...
	IPRINT	"\r\nWrite a single char (EXOS 7): "
	LD	A, CHANNEL_FILE
	LD	B, 0x76	; write this byte ...
	EXOS	7
	CALL	show_exos_error
	;
	IPRINT	"\r\nCheck EXOS 10 now (re-read FP)\r\n"
	LD	C, 0
	CALL	exos10_test

	; END!!
	IPRINT	"\r\nThat's all ... Repeating tests, you can also select exit there\r\n\r\n"
	JP	.again

END_OF_PRG:

BUFFER_ERROR	= $
BUFFER_ST	= $ + 128


