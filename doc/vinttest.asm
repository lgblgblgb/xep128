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
	DI
	LD	A, $C
	OUT	($BF), A

	; Page VRAM $FC segment (from Nick address 0) as page 3
	; Yes, it's ugly, we should ask EXOS etc, but this is only a crude test program!
	; It won't work on EP64 though ...
	LD	A, $FC
	OUT	($B3), A

	; Copy LPT to VRAM
	LD	HL, lpt
	LD	DE, $C000
	LD	BC, lpt.size
	LDIR

	; use our new LPT from Nick address 0
	XOR	A
	OUT	($82), A	; set LPL
	OUT	($83), A	; set LPH b6/b7 clear
	LD	A, 64
	OUT	($83), A	; set LPH, b6 set
	LD	A, 64 + 128
	OUT	($83), A	; set LPH, b6/b7 set
	
	; Set up a "JP addr" opcode to the "irq" routine to handle interrupts
	LD	A, $C3		; JP opcode
	LD	($38), A
	LD	HL, irq
	LD	($39), HL
	
	; Call irq routine, to save some bytes, it will reset Dave latches and enable only VINT,
	; and also contain an EI to enable interrupts on Z80
	CALL	irq

	; As the main program, we copy the VINT (INT1 on Dave) level bit as the border colour in an infinte loop
neverending:
	IN	A, ($B4)
	AND	16
	OUT	($81), A
	JP	neverending


; Our Interrupt handler, JP from 0x38 will jump here on VINT
; (also used as an "interrupt init" routine before the main loop)
; This routine simply "ACKs" (dave latch reset) interrupts,
; and set a border colour. So while main program loop sets colours
; based on VINT level, this will create some other colour where
; int occured, thus we can see that!


irq:
	EX	AF
	LD	A, 4
	OUT	($81), A
	[32] NOP		; some more time, to have larger coloured area
	LD	A, $BA
	OUT	($B4), A
	EX	AF
	EI
	RET


; Our LPT
lpt:

	; 95 lines of "something"
	DB 256-95, 2,       63, 0,    0,0,0,0,     0,0,0,0,0,0,0,0
	; 97 lines of "VINT LPB"
	DB 256-97, 2 + 128, 63, 0,    0,0,0,0,     0,0,0,0,0,0,0,0
	; 95 lines of "something"
	DB 256-95, 2,       63, 0,    0,0,0,0,     0,0,0,0,0,0,0,0


    ;+--------+--------+----+----+--//--+
    ;| 256-3  | VBLANK | 63 |  0 |      |
    ;+--------+--------+----+----+--//--+
    ;| 256-2  | VBLANK |  6 | 63 |      |
    ;+--------+--------+----+----+--//--+
    ;| 256-1  | VBLANK | 63 | 32 |      |
    ;+--------+--------+----+----+--//--+
    ;| 256-19 | PIXEL  |  6 | 63 |      |
    ;+--------+--------+----+----+--//--+

	DB 256-3 , 0, 63,  0,  0,0,0,0,     0,0,0,0,0,0,0,0
	DB 256-2 , 0,  6, 63,  0,0,0,0,     0,0,0,0,0,0,0,0
	DB 256-1 , 0, 63, 32,  0,0,0,0,     0,0,0,0,0,0,0,0
	DB 256-19, 3,  6, 63,  0,0,0,0,     0,0,0,0,0,0,0,0


.size = $ - lpt



END_OF_PRG:
