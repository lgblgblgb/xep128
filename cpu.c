/* Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
   Copyright (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
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

Z80EX_CONTEXT z80ex VARALIGN;
static int memsegs[4] VARALIGN;
Uint8 memory[0x400000] VARALIGN;
Uint8 ports[0x100] VARALIGN;
const char *memory_segment_map[0x100];
static int is_ram_seg[0x100] VARALIGN;
static int mem_ws_all, mem_ws_m1;
int nmi_pending = 0;

const char ROM_SEGMENT[] = "ROM";
const char RAM_SEGMENT[] = "RAM";
const char VRAM_SEGMENT[] = "VRAM";
const char SRAM_SEGMENT[] = "SRAM";
const char UNUSED_SEGMENT[] = "unused";

char *mem_desc = NULL;


// TODO: this should be written ... it's called when VRAM access, or $80...$8F I/O ports are accessed
#define nick_clock_align()


void set_ep_cpu ( int type )
{
	z80ex.internal_int_disable = 0;
	switch (type) {
		case CPU_Z80:
			z80ex.nmos = 1;
			z80ex.z180 = 0;
			break;
		case CPU_Z80C:
			z80ex.nmos = 0;
			z80ex.z180 = 0;
			break;
		case CPU_Z180:
			z80ex.nmos = 0;
			z80ex.z180 = 1;
			z180_port_start = 0;
			break;
		default:
			ERROR_WINDOW("Unknown CPU type was requested: %d", type);
			exit(1);
	}
	DEBUG("CPU: set to %s %s" NL,
		z80ex.z180 ? "Z180" : "Z80",
		z80ex.nmos ? "NMOS" : "CMOS"
	);
}



static inline void add_ram_segs ( int seg, int seg_end, const char *type )
{
	while (seg <= seg_end) {
		if (memory_segment_map[seg] == UNUSED_SEGMENT) {
			memory_segment_map[seg] = type;
			if (type == SRAM_SEGMENT)
				sram_load_segment(seg);
		} else
			DEBUGPRINT("CONFIG: RAM: segment %02Xh cannot be defined as %s since it's already %s" NL, seg, type, memory_segment_map[seg]);
		seg++;
	}
}




int ep_set_ram_config ( const char *spec )
{
	int a;
	for (a = 0; a < 0xFC; a++) {
		if (memory_segment_map[a] == SRAM_SEGMENT) {
			sram_save_segment(a);	// that is, we *HAD* configured SRAM before, so save it, before drop it ...
			memory_segment_map[a] = UNUSED_SEGMENT;
		}
		if (memory_segment_map[a] == RAM_SEGMENT)
			memory_segment_map[a] = UNUSED_SEGMENT;
		if (memory_segment_map[a] == VRAM_SEGMENT || memory_segment_map[a] == UNUSED_SEGMENT)
			memset(memory + (a << 14), 0xFF, 0x4000);
	}
	if (*spec == '@') {	// segment list format is requested ...
		while (spec && *spec) {
			int sb, se;
			const char *type;
			spec++;
			if (*spec == '=') {
				type = SRAM_SEGMENT;
				spec++;
			} else
				type = RAM_SEGMENT;
			switch (sscanf(spec, "%x-%x,", &sb, &se)) {
				case 1:
					DEBUG("CONFIG: RAM: requesting single segment %02Xh as %s" NL, sb, type);
					if (sb >= 0 && sb < 0x100)
						add_ram_segs(sb, sb, type);
					else
						DEBUGPRINT("CONFIG: RAM: WARNING: ignoring bad single %s segment definition %02X" NL, type, sb);
					break;
				case 2:
					DEBUG("CONFIG: RAM: requesting segment range %02Xh-%02Xh as %s" NL, sb, se, type);
					if (se >= sb && sb >= 0 && se < 0x100)
						add_ram_segs(sb, se, type);
					else
						DEBUGPRINT("CONFIG: RAM: WARNING: ignoring bad %s segment range definition %02X-%02X" NL, type, sb, se);
					break;
			}
			spec = strchr(spec, ',');
		}
	} else {
		int es = (atoi(spec) - 64) >> 4;
		if (es < 0)
			es = 0;
		else if (es > 252)
			es = 252;
		DEBUG("CONFIG: RAM: requesting simple memory range as RAM for %d segments" NL, es);
		if (es)
			add_ram_segs(0xFC - es, 0xFB, RAM_SEGMENT);
	}
	return ep_init_ram();
}



int ep_init_ram ( void )
{
	int a, sum = 0, from;
	const char *type = NULL;
	char dbuf[PATH_MAX + 80];
	if (mem_desc)
		*mem_desc = '\0';
	for (a = 0; a < 0x101; a++) {	// yeah, 0x101 is by intent!!
		if (a < 0x100) {
			int is_sram = (memory_segment_map[a] == SRAM_SEGMENT);
			is_ram_seg[a] = (memory_segment_map[a] == RAM_SEGMENT || memory_segment_map[a] == VRAM_SEGMENT || is_sram);
			if (is_ram_seg[a]) {
				if (!is_sram)
					memset(memory + (a << 14), 0xFF, 0x4000);
				sum++;
			}
		}
		if (a == 0x100 || type != memory_segment_map[a] || rom_name_tab[a]) {
			if (type) {
				int s;
				sprintf(dbuf, "%02X-%02X %s %s", from, a - 1, type, rom_name_tab[from] ? rom_name_tab[from] : "");
				DEBUGPRINT("CONFIG: MEM: %s" NL, dbuf);
				strcat(dbuf, "\n");
				s = mem_desc ? strlen(mem_desc) : 0;
				mem_desc = realloc(mem_desc, s + strlen(dbuf) + 256);
				check_malloc(mem_desc);
				if (!s)
					*mem_desc = '\0';
				strcat(mem_desc, dbuf);
			}
			type = memory_segment_map[a];
			from = a;
		}
	}
	sprintf(dbuf, "RAM:  %d segments (%d Kbytes)", sum, sum << 4);
	strcat(mem_desc, dbuf);
	DEBUGPRINT("CONFIG: MEM: found %s" NL, dbuf);
#ifdef CONFIG_SDEXT_SUPPORT
	sdext_clear_ram();
#endif
	return sum;
}




Z80EX_BYTE z80ex_mread_cb(Z80EX_WORD addr, int m1_state) {
	register int phys = memsegs[addr >> 14] + addr;
	//DEBUG("M1 state at PC=%04Xh Phys=%08Xh seg=%02Xh" NL, addr, phys, ports[0xB0 | (addr >> 14)]);
	if (phys >= 0x3F0000) { // VRAM access, no "$BF port" wait states ever, BUT TODO: Nick CPU clock strechting ...
		nick_clock_align();
		return memory[phys];
	}
	if (mem_ws_all || (m1_state && mem_ws_m1))
		z80ex_w_states(mem_wait_states);
#ifdef CONFIG_SDEXT_SUPPORT
	if ((phys & 0x3F0000) == sdext_cart_enabler)
		return sdext_read_cart(phys & 0xFFFF);
	else
#endif
		return memory[phys];
}


Uint8 read_cpu_byte ( Uint16 addr )
{
	return memory[memsegs[addr >> 14] + addr];
}


void z80ex_mwrite_cb(Z80EX_WORD addr, Z80EX_BYTE value) {
	register int phys = memsegs[addr >> 14] + addr;
	if (phys >= 0x3F0000) { // VRAM access, no "$BF port" wait states ever, BUT TODO: Nick CPU clock strechting ...
		nick_clock_align();
		memory[phys] = value;
		if (zxemu_on && phys >= 0x3f9800 && phys <= 0x3f9aff)
			zxemu_attribute_memory_write(phys & 0xFFFF, value);
		return;
	}
	if (mem_ws_all) 
		z80ex_w_states(mem_wait_states);
	//if (phys >= ram_start)
	if (is_ram_seg[phys >> 14])
		memory[phys] = value;
#ifdef CONFIG_SDEXT_SUPPORT
	else if ((phys & 0x3F0000) == sdext_cart_enabler)
		sdext_write_cart(phys & 0xFFFF, value);
#endif
	else
		DEBUG("WRITE to NON-decoded memory area %08X" NL, phys);
}



Z80EX_BYTE z80ex_pread_cb(Z80EX_WORD port16) {
	Uint8 port;
	if (z80ex.z180 && (port16 & 0xFFC0) == z180_port_start) {
		if (z180_port_start == 0x80) {
			ERROR_WINDOW("FATAL: Z180 internal ports configured from 0x80. This conflicts with Dave/Nick, so EP is surely unusable.");
			exit(1);
		}
		return z180_port_read(port16 & 0x3F);
	}
	port = port16 & 0xFF;
	if (port < primo_on)
		return primo_read_io(port);
	switch (port) {
#ifdef CONFIG_W5300_SUPPORT
		case W5300_IO_BASE + 0: return w5300_read_mr0();
		case W5300_IO_BASE + 1: return w5300_read_mr1();
		case W5300_IO_BASE + 2: return w5300_read_idm_ar0();
		case W5300_IO_BASE + 3: return w5300_read_idm_ar1();
		case W5300_IO_BASE + 4: return w5300_read_idm_dr0();
		case W5300_IO_BASE + 5: return w5300_read_idm_dr1();
#endif
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
#else
		case 0x10: case 0x14: case 0x11: case 0x15: case 0x12: case 0x16: case 0x13: case 0x17:
		case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
			DEBUG("EXDOS: not implemented, port read %02X" NL, port);
			return 0xFF;
#endif
		/* ZX Spectrum emulator */
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
			DEBUG("ZXEMU: reading port %02Xh" NL, port);
			return ports[port];

		case 0x50:
			return apu_read_data();
		case 0x51:
			return apu_read_status();

		/* RTC registers */
		case 0x7F:
			return rtc_read_reg();
		/* NICK registers */
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
			nick_clock_align();
			return nick_get_last_byte();
		/* DAVE registers */
		case 0xB0: case 0xB1: case 0xB2: case 0xB3:
			return ports[port];
		case 0xB4:
			return dave_int_read;
		case 0xB5:
			return (kbd_selector == -1) ? 0xFF : kbd_matrix[kbd_selector];
		case 0xB6:
			return read_control_port_bits() | PORT_B6_READ_OTHERS;	// used for control ports (joystick/mouse) but also some misc features as input (tape in, printer status in, serial in)
		case 0xFE:
			return zxemu_read_ula(IO16_HI_BYTE(port16));
	}
	DEBUG("IO: READ: unhandled port %02Xh read" NL, port);
	return 0xFF;
	//return ports[port];
}




void z80ex_pwrite_cb(Z80EX_WORD port16, Z80EX_BYTE value) {
	Z80EX_BYTE old_value;
	Uint8 port;
	if (z80ex.z180 && (port16 & 0xFFC0) == z180_port_start) {
		if (z180_port_start == 0x80) {
			ERROR_WINDOW("FATAL: Z180 internal ports configured from 0x80. This conflicts with Dave/Nick, so EP is surely unusable.");
			exit(1);
		}
		z180_port_write(port16 & 0x3F, value);
		return;
	}
	port = port16 & 0xFF;
	if (port < primo_on)
		return primo_write_io(port, value);
	old_value = ports[port];
	ports[port] = value;
	//DEBUG("IO: WRITE: OUT (%02Xh),%02Xh" NL, port, value);
	switch (port) {
#ifdef CONFIG_W5300_SUPPORT
		case W5300_IO_BASE + 0: w5300_write_mr0(value); break;
		case W5300_IO_BASE + 1: w5300_write_mr1(value); break;
		case W5300_IO_BASE + 2: w5300_write_idm_ar0(value); break;
		case W5300_IO_BASE + 3: w5300_write_idm_ar1(value); break;
		case W5300_IO_BASE + 4: w5300_write_idm_dr0(value); break;
		case W5300_IO_BASE + 5: w5300_write_idm_dr1(value); break;
#endif
		/* EXDOS/WD registers */
#ifdef CONFIG_EXDOS_SUPPORT
		case 0x10:
		case 0x14:
			wd_send_command(value);
			break;
		case 0x11:
		case 0x15:
			wd_track = value;
			break;
		case 0x12:
		case 0x16:
			wd_sector = value;
			break;
		case 0x13:
		case 0x17:
			wd_write_data(value);
			break;
		case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
			wd_set_exdos_control(value);
			break;
#else
		case 0x10: case 0x14: case 0x11: case 0x15: case 0x12: case 0x16: case 0x13: case 0x17:
		case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
			DEBUG("EXDOS: not implemented, port write %02X with value %02X" NL, port, value);
			break;
#endif
		case 0x32:
		case 0x3F:
			DEBUG("Z180: ignored <no Z180 emulation is active> for writing port = %02Xh, data = %02Xh." NL, port, value);
			break;

		case 0x44:
			zxemu_switch(value);
			break;
		case 0x45:
			primo_switch(value);
			break;

		case 0x50:
			apu_write_data(value);
			break;
		case 0x51:
			apu_write_command(value);
			break;

		/* RTC registers */
		case 0x7E:
			rtc_set_reg(value);
			break;
		case 0x7F:
			rtc_write_reg(value);
			break;

		/* DAVE audio etc related registers */
		case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7:
		case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
			printer_disable_covox();	// disable COVOX mode, if any
			audio_source = AUDIO_SOURCE_DAVE;
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
				DEBUG("PRINTER STROBE: %d -> %d" NL, old_value & 16, value & 16);*/
			if ((old_value & 16) && (!(value & 16)))
				printer_port_strobe();
			//	printer_send_data(ports[0xB6]);
			//printer_port_strobe((old_value & 16) != (value & 16));	// storbe event
			break;
		case 0xB6:
			// DEBUG("PRINTER DATA: %d" NL, value);
			printer_port_set_data(value);
			break;
		case 0xB7:
			mouse_check_data_shift(value);
			break;
		case 0xBF:
			// Note: 16K/64K RAM config is not implemented!
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
			dave_set_clock();
			DEBUG("BF register is written -> W_ALL=%d W_M1=%d CLOCK=%dMhz" NL, mem_ws_all, mem_ws_m1, (value & 2) ? 12 : 8);
			break;
		/* NICK registers */
		case 0x80: case 0x84: case 0x88: case 0x8C:
			nick_clock_align();
			nick_set_bias(value);
			break;
		case 0x81: case 0x85: case 0x89: case 0x8D:
			nick_clock_align();
			nick_set_border(value);
			break;
		case 0x82: case 0x86: case 0x8A: case 0x8E:
			nick_clock_align();
			nick_set_lptl(value);
			break;
		case 0x83: case 0x87: case 0x8B: case 0x8F:
			nick_clock_align();
			nick_set_lpth(value);
			break;
		/* DTM DAC 4 channel */
		case 0xF0: case 0xF1: case 0xF2: case 0xF3:
			printer_disable_covox();        // disable COVOX mode, if any
			audio_source = AUDIO_SOURCE_DTM_DAC4;
			break;
		/* ZXemu card */
		case 0xFE:
			zxemu_write_ula(IO16_HI_BYTE(port16), value);
			break;
		default:
			DEBUG("IO: WRITE: unhandled port %02Xh write with data %02Xh" NL, port, value);
			break;
	}
}


Z80EX_BYTE z80ex_intread_cb( void ) {
	return 0xFF; // hmmm.
}


void z80ex_reti_cb ( void ) {
}

int z80ex_ed_cb(Z80EX_BYTE opcode)
{
	if (Z80_PC >= 0xC000 && ports[0xB3] == xep_rom_seg) {
		xep_rom_trap(Z80_PC, opcode);
		return 1; // handled in XEP
	}
	return 0; // unhandled ED op!
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




void z80_reset ( void )
{
	memset(ports, 0xFF, 0x100);
	ports[0xB5] = 0; // for printer strobe signal not to trigger output a character on reset or so?
	//set_ep_cpu(CPU_Z80);
	z80ex_reset();
	z180_internal_reset();
	srand((unsigned int)time(NULL));
	Z80_AF	= rand() & 0xFFFF;
	Z80_BC	= rand() & 0xFFFF;
	Z80_DE	= rand() & 0xFFFF;
	Z80_HL	= rand() & 0xFFFF;
	Z80_IX	= rand() & 0xFFFF;
	Z80_IY	= rand() & 0xFFFF;
	Z80_SP	= rand() & 0xFFFF;
	Z80_AF_	= rand() & 0xFFFF;
	Z80_BC_	= rand() & 0xFFFF;
	Z80_DE_	= rand() & 0xFFFF;
	Z80_HL_	= rand() & 0xFFFF;
	DEBUG("Z80: reset" NL);
}


void ep_reset ( void )
{
	if (primo_on)
		primo_emulator_exit();
	z80_reset();
	dave_reset();
	rtc_reset();
	mouse_reset();
	apu_reset();
#ifdef CONFIG_EXDOS_SUPPORT
	wd_exdos_reset();
#endif
	primo_switch(0);
	nmi_pending = 0;
}

