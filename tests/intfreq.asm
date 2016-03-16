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

EXOS_CH_VIDEO		= 1
EXOS_CH_KEYBOARD	= 2

SCREEN_WIDTH		= 42
SCREEN_HEIGHT		= 27
COLOUR_BORDER		= 0

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
	LD	A, EXOS_CH_VIDEO
	EXOS	8
	POP	HL
	POP	DE
	ADD	HL, DE
	JP	(HL)


print_hex_word:		; HL = hex word to print
	PUSH	HL
	LD	A, H
	CALL	print_hex_byte
	POP	HL
	LD	A, L
print_hex_byte:		; A = byte to print
	PUSH	AF
	[4] RRA
	CALL	print_hex_digit
	POP	AF
print_hex_digit:
	AND	$0F
	ADD	$90
	DAA
	ADC	$40
	DAA
	LD	B, A
.exosch:
	LD	A, EXOS_CH_VIDEO
	EXOS	7
	RET
print_space:
	LD	B, $20
	JR	print_hex_digit.exosch





exos_init_std:
	; Open keyboard channel
	LD	DE, EXOS_FN_KEYBOARD
	LD	A, EXOS_CH_KEYBOARD
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
	LD	A, EXOS_CH_VIDEO
	EXOS	1
	; Font reset
	LD	A, EXOS_CH_VIDEO
	LD	B, 4
	EXOS	11
	; Display video page
	LD	A, EXOS_CH_VIDEO
	LD	BC, $101
	LD	DE, SCREEN_HEIGHT * $100 + 1
	EXOS	11
	SETEXOS	4, EXOS_CH_VIDEO ; set default channel
	RET


getchar:
        LD      A, EXOS_CH_KEYBOARD
        EXOS    5
        LD      A, B
        RET


exit_to_basic:
	IPRINT	"\r\n\r\nPress any key to exit to BASIC"
	CALL	getchar
.now:
        LD      DE, EXOS_FN_BASIC
        EXOS    26


; **** MAIN ****


; 12 bit counter values to be used as test for TG0 and TG1
TG0_COUNTER_VAL = 4095
TG1_COUNTER_VAL = 2047


main:
	CALL	exos_init_std
	IPRINT  "Dave interrupt level sense testing for emulators (C)2016 LGB\r\nSome moments, please ...\r\n"
	; Test 1Hz interrupt
	IPRINT	"\r\nTiming of 1Hz interrupt:              "
	LD	BC, 4 * 256 + 0
	CALL	run_test
	; Test TG interrupt on 1KHz
	IPRINT	"\r\nTiming of TG interrupt on 1KHz source:"
	LD	BC, 1 * 256 + 0
	CALL	run_test
	; Test TG interrupt on 50Hz
	IPRINT	"\r\nTiming of TG interrupt on 50Hz source:"
	LD	BC, 1 * 256 + 32
	CALL	run_test
	; Test TG interrupt on TG-0
	IPRINT	"\r\nTiming of TG interrupt on TG channel0:"
	LD	BC, 1 * 256 + 64
	CALL	run_test
	; Test TG interrupt on TG-1
	IPRINT	"\r\nTiming of TG interrupt on TG channel1:"
	LD	BC, 1 * 256 + 32 + 64
	CALL	run_test
	; Just for fun, test video interrupt. This needs some special care though ...
	IPRINT  "\r\nTiming of VINT/INT1 interrupt:        "
	LD	BC, 16 * 256 + 0
	DI			; special care of VINT, wait for well defined state ...
.wait_vint1:
	IN	A, ($B4)
	AND	B
	JR	NZ, .wait_vint1
	CALL	run_test
	LD	BC, 16 * 256 + 0
	DI
.wait_vint2:
	IN	A, ($B4)
	AND	B
	JR	Z, .wait_vint2
	CALL	run_test
	; And the good old exit ...
	RET





run_test:
	DI
	LD	A, C
	OUT	($A7), A	; put TG interrupt source
	; Prepare TG tests, set some osc freq for TG0 and TG1
        LD      A, TG0_COUNTER_VAL & $FF
        OUT     ($A0), A
        LD      A, TG0_COUNTER_VAL >> 8
        OUT     ($A1), A
        LD      A, TG1_COUNTER_VAL & $FF
        OUT     ($A2), A
        LD      A, TG1_COUNTER_VAL >> 8
        OUT     ($A3), A
	; Now the rest
	LD	A, $E
	CALL	.run_test_one
	PUSH	HL
	LD	A, $C
	CALL	.run_test_one
	EI
	CALL	print_space
	CALL	print_hex_word
	CALL	print_space
	POP	HL
	JP	print_hex_word


; Input: A = BF port init, B = bit mask for B4 port
; Output: HL = number of test cycles run between state change
.run_test_one:
	OUT	($BF), A
	LD	HL, 0
	IN	A, ($B4)
	AND	B		; mask value we are interested in
	LD	C, A		; store bit state
.wait0:
	IN	A, ($B4)
	AND	B
	CP	C
	JR	Z, .wait0	; wait for change of bit value to start the test
.wait1:
	INC	HL		;  6 cycles		; since we don't convert HL as result to cycles or anything, it's
	[8] NOP			;  4 cycles * 8		; not so much important to count cycles anyway :-P
	IN	A, ($B4)	; 11 cycles
	AND	B		;  4 cycles
	CP	C		;  4 cycles
	JP	NZ, .wait1	; 10 cycles        ... again wait for another change, continue test otherwise
	RET 

END_OF_PRG:

