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


#include "xepem.h"

Z80EX_CONTEXT *z80 = NULL;
static int memsegs[4];
static int ram_start;
Uint8 memory[0x400000];
Uint8 ports[0x100];
static int used_mem_segments[0x100];
static int mem_ws_all, mem_ws_m1;

int z180 = 0;


#if 0
void set_ep_memseg(int seg, int val)
{
	if (seg < 0 || seg > 255 || val <0 || val > 255) {
		fprintf(stderr, "FATAL: invalid seg (%d) and/or val (%d) in set_ep_memseg()\n", seg, val);
		exit(1);
	}
	memsegs[seg] = (val << 14) - (seg << 14);
}
#endif


int set_ep_ramsize(int kbytes)
{
	int a;
	memset(memory, 0xFF, 0x400000);
	if (kbytes < 64)
		kbytes = 64;
	else if (kbytes > 0xF80)
		kbytes = 0xF80;
	kbytes &= 0xFFF0;
	ram_start = 0x400000 - (kbytes << 10);
	for (a = 0; a < 0x100; a++)
		used_mem_segments[a] = a >= (0x100 - (kbytes >> 4));
	printf("Config: %d Kbytes RAM, starting at %Xh\n", kbytes, ram_start);
	return kbytes;
}


void ep_clear_ram ( void )
{
	memset(memory + ram_start, 0xFF, 0x400000 - ram_start);
#ifdef CONFIG_SDEXT_SUPPORT
	sdext_clear_ram();
#endif
}




static Z80EX_BYTE _mread(Z80EX_CONTEXT *unused_1, Z80EX_WORD addr, int m1_state, void *unused_2) {
	register int phys = memsegs[addr >> 14] + addr;
	if (phys >= 0x3F0000) { // VRAM access, no "$BF port" wait states ever, BUT TODO: Nick CPU clock strechting ...
		return memory[phys];
	}
	if (mem_ws_all || (m1_state && mem_ws_m1))
		z80ex_w_states(z80, 1);
#ifdef CONFIG_SDEXT_SUPPORT
	if ((phys & 0x3FC000) == sdext_cp3m_usability)
		return sdext_read_cart_p3(addr & 0x3FFF);
	else
#endif
		return memory[phys];
}



static void      _mwrite(Z80EX_CONTEXT *unused_1, Z80EX_WORD addr, Z80EX_BYTE value, void *unused_2) {
	register int phys = memsegs[addr >> 14] + addr;
	if (phys >= 0x3F0000) { // VRAM access, no "$BF port" wait states ever, BUT TODO: Nick CPU clock strechting ...
		memory[phys] = value;
		if (zxemu_on && phys >= 0x3f9800 && phys <= 0x3f9aff)
			zxemu_attribute_memory_write(phys & 0xFFFF, value);
		return;
	}
	if (mem_ws_all) 
		z80ex_w_states(z80, 1);
	if (phys >= ram_start)
		memory[phys] = value;
#ifdef CONFIG_SDEXT_SUPPORT
	else if ((phys & 0x3FC000) == sdext_cp3m_usability)
		sdext_write_cart_p3(addr & 0x3FFF, value);
#endif
	else
		printf("WRITE to NON-decoded memory area %08X\n", phys);
}



static Z80EX_BYTE _pread(Z80EX_CONTEXT *unused_1, Z80EX_WORD port16, void *unused_2) {
	Uint8 port = port16 & 0xFF;
	//printf("IO: READ: IN (%02Xh)\n", port);
	switch (port) {
		/* EXDOS/WD registers */
#ifdef CONFIG_EXDOS_SUPPORT
		case 0x10:
		case 0x14:
			return wd_read_status();
		case 0x11:
		case 0x15:
			return wd_track;
		case 0x12:
		case 0x16:
			return wd_sector;
		case 0x13:
		case 0x17:
			return wd_read_data();
		case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
			return wd_read_exdos_status();
#endif

		case 0x30:
			return emurom_receive();

		/* ZX Spectrum emulator */
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
			fprintf(stderr, "ZXEMU: reading port %02Xh\n", port);
			return ports[port];

		/* RTC registers */
		case 0x7F:
			return rtc_read_reg();
		/* NICK registers */
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
			return nick_get_last_byte();
		/* DAVE registers */
		case 0xB0: case 0xB1: case 0xB2: case 0xB3:
			return ports[port];
		case 0xB4:
			return dave_int_read;
		case 0xB5:
			return (kbd_selector == -1) ? 0xFF : kbd_matrix[kbd_selector];
		case 0xB6:
			return mouse_read();
		case 0xFE:
			return zxemu_read_ula(IO16_HI_BYTE(port16));
	}
	fprintf(stderr, "IO: READ: unhandled port %02Xh read\n", port);
	return 0xFF;
	//return ports[port];
}




static void _pwrite(Z80EX_CONTEXT *unused_1, Z80EX_WORD port16, Z80EX_BYTE value, void *unused_2) {
	Z80EX_BYTE old_value;
	Uint8 port = port16 & 0xFF;
	old_value = ports[port];
	ports[port] = value;
	//printf("IO: WRITE: OUT (%02Xh),%02Xh\n", port, value);
	//if ((port & 0xF0) == 0x80) printf("NICK WRITE!\n");
	switch (port) {
		/* EXDOS/WD registers */
#ifdef CONFIG_EXDOS_SUPPORT
		case 0x10:
		case 0x14:
			return wd_send_command(value);
		case 0x11:
		case 0x15:
			wd_track = value;
		case 0x12:
		case 0x16:
			wd_sector = value;
		case 0x13:
		case 0x17:
			wd_write_data(value);
		case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
			wd_set_exdos_control(value);
#endif
		/* DOH! */
		case 0x30:
			emurom_send(value);
			break;
		case 0x32:
		case 0x3F:
			fprintf(stderr, "Z180: would be Z180 config port, ignored <no Z180 emulation> for writing port %02Xh with value of %02Xh.\n", port, value);
			break;

		case 0x44:
			if (zxemu_on != (value & 128)) {
				zxemu_on = value & 128;
				fprintf(stderr, "ZXEMU: emulation is turned %s.\n", zxemu_on ? "ON" : "OFF");
			}
			break;

		/* RTC registers */
		case 0x7E:
			rtc_set_reg(value);
			break;
		case 0x7F:
			rtc_write_reg(value);
			break;

		case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7:
		case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
			break;

		/* DAVE registers */
		case 0xB0:
			memsegs[0] =  value << 14;
			break;
		case 0xB1:
			memsegs[1] = (value << 14) - 0x4000;
			break;
		case 0xB2:
			memsegs[2] = (value << 14) - 0x8000;
			break;
		case 0xB3:
			memsegs[3] = (value << 14) - 0xC000;
			break;
#if 0
		case 0xB0: case 0xB1: case 0xB2: case 0xB3:
			memsegs[port & 3] = (value << 14) - ((port & 3) << 14);
			//set_ep_memseg(port & 3, value);
			break;
#endif
		case 0xB4:
			dave_configure_interrupts(value);
			break;
		case 0xB5:
			kbd_selector = ((value & 15) < 10) ? (value & 15) : -1;
			/*if ((old_value & 16) != (value & 16))
				fprintf(stderr, "PRINTER STROBE: %d -> %d\n", old_value & 16, value & 16);*/
			if ((old_value & 16) && (!(value & 16)))
				printer_send_data(ports[0xB6]);
			break;
		case 0xB6:
			// fprintf(stderr, "PRINTER DATA: %d\n", value);
			break;
		case 0xB7:
			mouse_check_data_shift(value);
			break;
		case 0xBF:
			// Note: 16K/64K RAM config is not implemented!
			//dave_set_clock((value & 2) ? DAVE_CLOCK_12MHZ : DAVE_CLOCK_8MHZ);
			value &= 0xC;
			if (value == 0) {
				mem_ws_all = 1;
				mem_ws_m1  = 0;
			} else if (value == 4) {
				mem_ws_all = 0;
				mem_ws_m1  = 1;
			} else {
				mem_ws_all = 0;
				mem_ws_m1  = 0;
			}
			//dave_set_clock((value & 2) ? DAVE_CLOCK_12MHZ : DAVE_CLOCK_8MHZ);
			fprintf(stderr, "BF register is written -> W_ALL=%d W_M1=%d CLOCK=%dMhz\n", mem_ws_all, mem_ws_m1, (value & 2) ? 12 : 8);
			break;
		/* NICK registers */
		case 0x80: case 0x84: case 0x88: case 0x8C:
			nick_set_bias(value);
			break;
		case 0x81: case 0x85: case 0x89: case 0x8D:
			nick_set_border(value);
			break;
		case 0x82: case 0x86: case 0x8A: case 0x8E:
			nick_set_lptl(value);
			break;
		case 0x83: case 0x87: case 0x8B: case 0x8F:
			nick_set_lpth(value);
			break;
		case 0xFE:
			zxemu_write_ula(IO16_HI_BYTE(port16), value);
			break;
		default:
			fprintf(stderr, "IO: WRITE: unhandled port %02Xh write with data %02Xh\n", port, value);
			break;
	}
}

void port_write ( Z80EX_WORD port, Z80EX_BYTE value )
{
	_pwrite(z80, port, value, NULL);
}


static Z80EX_BYTE _iread(Z80EX_CONTEXT *unused_1, void *unused_2) {
	return 0xFF; // hmmm.
}




static Z80EX_BYTE _rdcb_for_disasm(Z80EX_WORD addr, void *seg) {
	return memory[(*(int*)seg < 0) ? (memsegs[addr >> 14] + addr) : (((*(int*)seg << 14) + addr) & 0x3FFFFF)];
}
int z80_dasm(char *buffer, Uint16 pc, int seg)
{
	//char _buffer[buffer_size];
	int bytes, t1, t2;
	if (seg >= 0) {
		seg = (seg + (pc >> 14)) & 0xFF;
		pc &= 0x3FFF;
	}
//	sprintf(_dasm_result, seg < 0 ? "%s:%04X %s %s %s" : " %02X:%04X %s %s %s", seg < 0 ? "Z80" : seg, pc);
	if (seg < 0)
		sprintf(buffer, "@%02X:%04X %02X ", ports[0xB0 | (pc >> 14)], pc, _rdcb_for_disasm(pc, (void*)&seg));
	else
		sprintf(buffer, ">%02X:%04X %02X ", seg, pc, _rdcb_for_disasm(pc, (void*)&seg));
	bytes = z80ex_dasm(buffer + 12, 128, 0, &t1, &t2, _rdcb_for_disasm, pc, (void*)&seg);
/*	p = strchr(_buffer, ' ');
	if (p) {
	
	for (a = 0; a < bytes; a++) {
		sprintf(p, "%02X", _rdcb_for_disasm(pc, (void*)&seg));
	}*/
	return bytes;
}




int z80_reset ( void )
{
	if (z80 == NULL) {
		z80 = z80ex_create(
			_mread,  NULL,
			_mwrite, NULL,
			_pread,  NULL,
			_pwrite, NULL,
			_iread,  NULL
		);
		if (z80 == NULL) {
			fprintf(stderr, "Cannot create CPU emulation!\n");
			return 1;
		}
		printf("Z80: emulation has been created.\n");
		memset(ports, 0xFF, 0x100);
		ports[0xB5] = 0; // for printer strobe signal not to trigger output a character on reset or so?
	}
	z80ex_reset(z80);
	srand((unsigned int)time(NULL));
	z80ex_set_reg(z80, regAF,  rand() & 0xFFFF);
	z80ex_set_reg(z80, regBC,  rand() & 0xFFFF);
	z80ex_set_reg(z80, regDE,  rand() & 0xFFFF);
	z80ex_set_reg(z80, regHL,  rand() & 0xFFFF);
	z80ex_set_reg(z80, regIX,  rand() & 0xFFFF);
	z80ex_set_reg(z80, regIY,  rand() & 0xFFFF);
	z80ex_set_reg(z80, regSP,  rand() & 0xFFFF);
	z80ex_set_reg(z80, regAF_, rand() & 0xFFFF);
	z80ex_set_reg(z80, regBC_, rand() & 0xFFFF);
	z80ex_set_reg(z80, regDE_, rand() & 0xFFFF);
	z80ex_set_reg(z80, regHL_, rand() & 0xFFFF);
	printf("Z80: reset\n");
	return 0;
}

