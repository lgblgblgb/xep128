/*
 * Z80Ex, ZILoG Z80 CPU emulator.
 *
 * by Pigmaker57 aka boo_boo [pigmaker57@kahoh57.info]
 *
 * contains some code from the FUSE project (http://fuse-emulator.sourceforge.net)
 * Released under GNU GPL v2
 *
 */

#ifndef _Z80EX_H_INCLUDED
#define _Z80EX_H_INCLUDED

#if defined(__SYMBIAN32__)
typedef unsigned char Z80EX_BYTE;
typedef signed char Z80EX_SIGNED_BYTE;
typedef unsigned short Z80EX_WORD;
typedef unsigned int Z80EX_DWORD;
#elif defined(__GNUC__)
#include <stdint.h>
typedef uint8_t Z80EX_BYTE;
typedef int8_t Z80EX_SIGNED_BYTE;
typedef uint16_t Z80EX_WORD;
typedef uint32_t Z80EX_DWORD;
#elif defined(_MSC_VER)
typedef unsigned __int8 Z80EX_BYTE;
typedef signed __int8 Z80EX_SIGNED_BYTE;
typedef unsigned __int16 Z80EX_WORD;
typedef unsigned __int32 Z80EX_DWORD;
#else
typedef unsigned char Z80EX_BYTE;
typedef signed char Z80EX_SIGNED_BYTE;
typedef unsigned short Z80EX_WORD;
typedef unsigned int Z80EX_DWORD;
#endif

/* Union allowing a register pair to be accessed as bytes or as a word */
typedef union {
#ifdef WORDS_BIG_ENDIAN
	struct { Z80EX_BYTE h,l; } b;
#else
	struct { Z80EX_BYTE l,h; } b;
#endif
	Z80EX_WORD w;
} Z80EX_REGPAIR_T;

typedef enum { IM0 = 0, IM1 = 1, IM2 = 2 } IM_MODE;

/* Macros used for accessing the registers */
#if 0
#define A   z80ex.af.b.h
#define F   z80ex.af.b.l
#define AF  z80ex.af.w

#define B   z80ex.bc.b.h
#define C   z80ex.bc.b.l
#define BC  z80ex.bc.w

#define D   z80ex.de.b.h
#define E   z80ex.de.b.l
#define DE  z80ex.de.w

#define H   z80ex.hl.b.h
#define L   z80ex.hl.b.l
#define HL  z80ex.hl.w

#define A_  z80ex.af_.b.h
#define F_  z80ex.af_.b.l
#define AF_ z80ex.af_.w

#define B_  z80ex.bc_.b.h
#define C_  z80ex.bc_.b.l
#define BC_ z80ex.bc_.w

#define D_  z80ex.de_.b.h
#define E_  z80ex.de_.b.l
#define DE_ z80ex.de_.w

#define H_  z80ex.hl_.b.h
#define L_  z80ex.hl_.b.l
#define HL_ z80ex.hl_.w

#define IXH z80ex.ix.b.h
#define IXL z80ex.ix.b.l
#define IX  z80ex.ix.w

#define IYH z80ex.iy.b.h
#define IYL z80ex.iy.b.l
#define IY  z80ex.iy.w

#define SPH z80ex.sp.b.h
#define SPL z80ex.sp.b.l
#define SP  z80ex.sp.w

#define PCH z80ex.pc.b.h
#define PCL z80ex.pc.b.l
#define PC  z80ex.pc.w

#define I  z80ex.i
#define R  z80ex.r
#define R7 z80ex.r7

#define IFF1 z80ex.iff1
#define IFF2 z80ex.iff2
#define IM   z80ex.im

#define MEMPTRh z80ex.memptr.b.h
#define MEMPTRl z80ex.memptr.b.l
#define MEMPTR z80ex.memptr.w


/* The flags */

#define FLAG_C	0x01
#define FLAG_N	0x02
#define FLAG_P	0x04
#define FLAG_V	FLAG_P
#define FLAG_3	0x08
#define FLAG_H	0x10
#define FLAG_5	0x20
#define FLAG_Z	0x40
#define FLAG_S	0x80
#endif


typedef enum {regAF,regBC,regDE,regHL,regAF_,regBC_,regDE_,regHL_,regIX,regIY,regPC,regSP,regI,regR,regR7,regIM/*0,1 or 2*/,regIFF1,regIFF2} Z80_REG_T;

struct _z80_cpu_context {
	Z80EX_REGPAIR_T af,bc,de,hl;
	Z80EX_REGPAIR_T af_,bc_,de_,hl_;
	Z80EX_REGPAIR_T ix,iy;
	Z80EX_BYTE i;
	Z80EX_WORD r;	
	Z80EX_BYTE r7; /* The high bit of the R register */
	Z80EX_REGPAIR_T sp,pc;
	Z80EX_BYTE iff1, iff2; /*interrupt flip-flops*/
	Z80EX_REGPAIR_T memptr; /*undocumented internal register*/
	IM_MODE im;
	int halted;

	unsigned long tstate; /*t-state clock of current/last step*/
	unsigned char op_tstate; /*clean (without WAITs and such) t-state of currently executing instruction*/
	
	int noint_once; /*disable interrupts before next opcode?*/
	int reset_PV_on_int; /*reset P/V flag on interrupt? (for LD A,R / LD A,I)*/
	int doing_opcode; /*is there an opcode currently executing?*/
	char int_vector_req; /*opcode must be fetched from IO device? (int vector read)*/
	Z80EX_BYTE prefix;
	
	/*callbacks*/
#if 0
	z80ex_tstate_cb tstate_cb;
	void *tstate_cb_user_data;
	z80ex_pread_cb pread_cb;
	void *pread_cb_user_data;
	z80ex_pwrite_cb pwrite_cb;
	void *pwrite_cb_user_data;
	z80ex_mread_cb mread_cb;
	void *mread_cb_user_data;
	z80ex_mwrite_cb	mwrite_cb;
	void *mwrite_cb_user_data;
	z80ex_intread_cb intread_cb;
	void *intread_cb_user_data;
	z80ex_reti_cb reti_cb;
	void *reti_cb_user_data;
#endif

#ifdef Z80EX_TSTATE_CALLBACK
	int tstate_cb;  /* use tstate callback? */
#endif
	
	/*other stuff*/
	Z80EX_REGPAIR_T tmpword;
	Z80EX_REGPAIR_T tmpaddr;
	Z80EX_BYTE tmpbyte;
	Z80EX_SIGNED_BYTE tmpbyte_s;

	int nmos; /* NMOS Z80 mode if '1', CMOS if '0' */
	/* Z180 related - LGB */
#ifdef Z80EX_Z180_SUPPORT
	int z180;
	int internal_int_disable;
#endif
};

typedef struct _z80_cpu_context Z80EX_CONTEXT;

extern Z80EX_CONTEXT z80ex;

/* statically linked callbacks */

#ifdef Z80EX_TSTATE_CALLBACK
void z80ex_tstate_cb ( void );
#endif
Z80EX_BYTE z80ex_mread_cb ( Z80EX_WORD addr, int m1_state );
void z80ex_mwrite_cb ( Z80EX_WORD addr, Z80EX_BYTE value );
Z80EX_BYTE z80ex_pread_cb ( Z80EX_WORD port );
void z80ex_pwrite_cb ( Z80EX_WORD port, Z80EX_BYTE value);
Z80EX_BYTE z80ex_intread_cb(void);
void z80ex_reti_cb ( void );
#ifdef Z80EX_ED_TRAPPING_SUPPORT
int z80ex_ed_cb (Z80EX_BYTE opcode);
#endif
#ifdef Z80EX_Z180_SUPPORT
void z80ex_z180_cb (Z80EX_WORD pc, Z80EX_BYTE prefix, Z80EX_BYTE series, Z80EX_BYTE opcode, Z80EX_BYTE itc76);
#endif



/*create and initialize CPU*/
extern void z80ex_init(void);

/*do next opcode (instruction or prefix), return number of T-states*/
extern int z80ex_step(void);

/*returns type of the last opcode, processed with z80ex_step.
type will be 0 for complete instruction, or dd/fd/cb/ed for opcode prefix.*/
#define z80ex_last_op_type() z80ex.prefix


extern void z80ex_set_nmos(int nmos);
extern int  z80ex_get_nmos(void);

#ifdef Z80EX_Z180_SUPPORT
extern int  z80ex_get_z180(void);
extern void z80ex_set_z180(int z180);
#endif

/*maskable interrupt*/
/*returns number of T-states if interrupt was accepted, otherwise 0*/
extern int z80ex_int();

/*non-maskable interrupt*/
/*returns number of T-states (11 if interrupt was accepted, or 0 if processor
is doing an instruction right now)*/
extern int z80ex_nmi();

/*reset CPU*/
extern void z80ex_reset(void);

extern void z80ex_init(void);

/*get register value*/
extern Z80EX_WORD z80ex_get_reg(Z80_REG_T reg);

/*set register value (for 1-byte registers lower byte of <value> will be used)*/
extern void z80ex_set_reg(Z80_REG_T reg, Z80EX_WORD value);

/*returns 1 if CPU doing HALT instruction now*/
#define z80ex_doing_halt() z80ex.halted


/*when called from callbacks, returns current T-state of the executing opcode (instruction or prefix),
else returns T-states taken by last opcode executed*/
#define z80ex_op_tstate() z80ex.tstate

/*generate <w_states> Wait-states. (T-state callback will be called <w_states> times, when defined).
should be used to simulate WAIT signal or disabled CLK*/ 
extern void z80ex_w_states(unsigned w_states);

/*spend one T-state doing nothing (often IO devices cannot handle data request on
the first T-state at which RD/WR goes active).
for I/O callbacks*/
extern void z80ex_next_t_state();

/*returns 1 if maskable interrupts are possible in current z80 state*/
extern int z80ex_int_possible(void);

/*returns 1 if non-maskable interrupts are possible in current z80 state*/
extern int z80ex_nmi_possible(void);

#endif
