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
	CALL	main
	JP	exit_to_basic.now


EXOS_FN_VIDEO:		DB 6, "VIDEO:"
EXOS_FN_KEYBOARD:	DB 9, "KEYBOARD:"
EXOS_FN_BASIC:		DB 5, "BASIC"

EXOS_CH_VIDEO		= 1
EXOS_CH_KEYBOARD	= 2

SCREEN_WIDTH		= 42
SCREEN_HEIGHT		= 27
COLOUR_BORDER		= 0

W5300_IO		= $90


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
	IPRINT	"Press any key to exit to BASIC"
	CALL	getchar
.now:
        LD      DE, EXOS_FN_BASIC
        EXOS    26

w5300:
; Initializes W5300
.main_init:
	; Request reset
	LD	A, 0x38		; default values
	OUT	(W5300_IO), A
	LD	A, 0x81		; reset + indirect access
	OUT	(W5300_IO + 1), A
	; Waiting for W5300 to reset completed.
	IPRINT  "Waiting for W5300 to complete RESET "
	LD	H, 0
.resetting:
	INC	H
	JP	NZ, .no_reset_timeout
	; now we should give up, but since it's a test program,
	; and I want to try to run (without too much useful result of course)
	; without the hardware too, we allow to continue ...
	IPRINT	"Timeout??\r\nERROR: No w5300? Trying to continue anyway!\r\n"
	JR	.end_of_reset
.error:
	IPRINT	"ERROR: w5300 data write/verify failed. Trying to continue anyway!\r\n"
	RET
.no_reset_timeout:
	LD	A, EXOS_CH_VIDEO
	LD	B, '.'			; "nice" progress indicator, also it creates delay ...
	EXOS	7
	IN	A, (W5300_IO)		; dummy read for high byte of MR (MR0)
	IN	A, (W5300_IO + 1)	; read of low byte of MR1
	AND	$80			; we need to test bit 7 in MR1
	JP	NZ, .resetting		; it it's still 1, w5300 is under the reset process, we must wait
	; reset completed, re-set MR
	IPRINT	" OK\r\n"
.end_of_reset:
	LD	A, 0x38
	OUT	(W5300_IO), A
	LD	A, 1		; indirect mode is what we need
	OUT	(W5300_IO + 1), A
	; Setting up w5300 internal memory stuff
	; I have not so much idea here (and Xep128 emulation does not care too much)
	; It's commented out, since it seems (???) default after reset is OK for us for now.
	;IPRINT	"Setting w5300 memconfig up\r\n"
	;LD	IX, .MEMCFG_DATA
	;LD	DE, 0x20
	;LD	B, 18/2
	;CALL	.filler
	;CALL	NZ, .error
	; Check w5300 ID 
	IPRINT	"Checking W5300 IDR\r\n"
	XOR	A
	OUT	(W5300_IO + 2), A
	LD	A, 0xFE
	OUT	(W5300_IO + 3), A
	IN	A, (W5300_IO + 4)
	CP	$53
	JR	NZ, .bad_id
	IN	A, (W5300_IO + 5)
	CP	0
	JR	Z, .ok_id
.bad_id:
	IPRINT	"ERROR: W5300 ID check failed. Trying to continue anyway!\r\n"
.ok_id:
	; Set IP MAC address
	IPRINT	"Setting MAC address up\r\n"
	LD	IX, .MAC_DATA
	LD	DE, 8
	LD	B, 6 / 2	; 3 words, 6 bytes!
	CALL	.filler
	CALL	NZ, .error
	; Set IP related data, in one step
	IPRINT	"Setting LAN IP config up\r\n"
	LD	IX, .IP_DATA
	LD	DE, 0x10
	LD	B, 12 / 2
	CALL	.filler
	CALL	NZ, .error
	; END of init.
	RET
; Fill continogous w5300 ports with values reading from a table.
; IX = memory address
; DE = w5300 port (as starting port) LOWEST BIT MUST BE ZERO
; B = number of TWO (!) 8 bit ports to fill/bytes read, actually B*2 bytes, B words
; (w5300 is word based, even if you use in 8 bit bus mode!)
; Output:
;	IX points to the next unprocessed byte
;	DE contains the next w5300 reg number
;	zero flag is set on no error, otherwise port read-back verify failure
; WARNING: if data is interpreted as words, the byte order is meant in
; w5300 native byte order, which is the opposite of Z80's!
; Note: the verify stuff seems to be overkill. It's OK to that once to detect
; if w5300 exists, but it's slow to use all the time. But hey, it's even good
; for testing, this little app is for that purpose exactly!
.filler:
	LD	C, 0xFF
.filler_loop:
	; select w5300 register
	LD	A, D	; indirect address register, high byte
	OUT	(W5300_IO + 2), A
	LD	A, E	; indirect address register, low byte
	OUT	(W5300_IO + 3), A
	; write out 16 bit data
	LD	A, (IX)
	OUT	(W5300_IO + 4), A
	LD	A, (IX + 1)
	OUT	(W5300_IO + 5), A
	; check if data was OK to be written
	; NOTE: I'm not sure if it is needed to re-set indirect address register
	; Be safe to do anyway ...
	LD	A, D
	OUT	(W5300_IO + 2), A
	LD	A, E
	OUT	(W5300_IO + 3), A
	; now increment DE for next round (increment by TWO! as w5300 is word based even with 8 bit data bus)
	INC	DE
	INC	DE
	; Now do the data verification!
	IN	A, (W5300_IO + 4)
	CP	(IX)
	JR	Z, .filler_verify0_ok
	INC	C
.filler_verify0_ok:
	IN	A, (W5300_IO + 5)
	CP	(IX + 1)
	JR	Z, .filler_verify1_ok
	INC	C
.filler_verify1_ok:
	; Increment data pointer (two INCs, as we send/very two bytes at every iteration here)
	INC	IX
	INC	IX
	DJNZ	.filler_loop
	INC	C
	RET
	; Ethernet/IP related data follows
	; Please do not change to order of these data,
	; .filler subroutine is called with no mem address re-set
;.MEMCFG_DATA:
;	DB 
.MAC_DATA:
	;DB 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03	; SHAR: the MAC address
	DB 0x00, 0x08, 0xdc, 0x11, 0x22, 0x86	; SHAR: the MAC address
.IP_DATA:
	DB 192, 168, 0, 1			; GAR: gateway IP address
	DB 255, 255, 255, 0			; SUBR: subnet mask
	DB 192, 168, 0, 128			; SIPR: source IP (our IP!)
.SERVER:
	DB 0, 0, 0, 0				; iRC server IP address
	DB 0, 0					; iRC server port
	DB 0, 0					; our local port for connection
	



main:
	CALL	exos_init_std
	IPRINT	"Proof of concept iRC client for Enterprise-128 + WizNet w5300\r\n(C)2015 LGB Gabor Lenart http://xep128.lgb.hu\r\n"
	CALL	w5300.main_init
	IPRINT	"\r\nEnd of init + test. Press X to exit, any other key to continue\r\n\r\n"
	CALL	getchar
	CP	'x'
	RET	Z
	CP	'X'
	RET	Z
	IPRINT	"Ok, let's continue"
	JP	exit_to_basic


END_OF_PRG:

