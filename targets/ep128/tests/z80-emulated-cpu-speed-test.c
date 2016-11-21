#include <stdio.h>
#include <sys/time.h>

// #define Z80EX_OPSTEP_FAST_AND_ROUGH

#include "../z80.h"
#include "../z80ex/z80ex.c"


Z80EX_CONTEXT z80ex;

Z80EX_BYTE z80ex_mread_cb(Z80EX_WORD addr, int m1_state) {
	return 0;	// always provides NOP ...
}

void z80ex_mwrite_cb(Z80EX_WORD addr, Z80EX_BYTE value) {
}

Z80EX_BYTE z80ex_pread_cb(Z80EX_WORD port16) {
	return 0xFF;
}

void z80ex_pwrite_cb(Z80EX_WORD port16, Z80EX_BYTE value) {
}

Z80EX_BYTE z80ex_intread_cb( void ) {
	return 0xFF;
}

void z80ex_reti_cb ( void ) {
}

int z80ex_ed_cb(Z80EX_BYTE opcode) {
	return 0;
}

void z80ex_z180_cb (Z80EX_WORD pc, Z80EX_BYTE prefix, Z80EX_BYTE series, Z80EX_BYTE opcode, Z80EX_BYTE itc76) {
}


int main ( void )
{
	Uint64 t_all = 0;
	struct timeval tv1, tv2;
	int elapsed;
	double mhz;
	z80ex_init();
	z80ex_reset();
	printf("Testing free-run Z80 emulation execution speed (NOPs only for now), please wait ...\n");
	gettimeofday(&tv1, NULL);
	do {
		int t;
		t = 0;
		while (t < 4000000)
			t += z80ex_step();
		t_all += t;
		gettimeofday(&tv2, NULL);
		elapsed = (tv2.tv_sec - tv1.tv_sec) * 1000 * 1000 + (tv2.tv_usec - tv1.tv_usec); 
	} while (elapsed < 10000000);
	mhz = t_all / (double)elapsed;
	printf("Microseconds for test: %d\nT-states for Z80: %lld\nEstimated Z80 clock speed: %lfMHz\nZ80 4MHz compared: x%02lf\n",
		elapsed,
		(long long int)t_all,
		mhz,
		mhz / 4.0
	);
	return 0;
}


