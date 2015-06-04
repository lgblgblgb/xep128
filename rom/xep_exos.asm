; Iface

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

