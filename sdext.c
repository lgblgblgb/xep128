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


//	http://elm-chan.org/docs/mmc/mmc_e.html
//	http://www.mikroe.com/downloads/get/1624/microsd_card_spec.pdf
//	http://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf

#include "xepem.h"

#ifdef CONFIG_SDEXT_SUPPORT

int sdext_cp3m_usability = SDEXT_PHYSADDR_CART_P3_SELMASK_OFF;
static int rom_page_ofs;
static int is_hs_read;
static Uint8 _spi_last_w;
static int cs0, cs1;
static Uint8 status;

static Uint8 sd_ram_ext[0x1C00]; // 7K of useful SRAM
static Uint8 sd_rom_ext[0x10000]; // 64K (second 64K flash, can only be accessed within a 8K window)

static Uint8 cmd[6], cmd_index, _read_b, _write_b, _write_specified;
static const Uint8 *ans_p;
static int ans_index, ans_size;
static void (*ans_callback)(void);

static FILE *sdf;
static Uint8 _buffer[1024];

/* ID files:
 * C0 71 00 00 │ 00 5D 01 32 │ 13 59 80 E3 │ 76 D9 CF FF │ 16 40 00 4F │ 01 50 41 53
 * 30 31 36 42 │ 41 35 E4 39 │ 06 00 35 03 │ 80 FF 80 00 │ 00 00 00 00 │ 00 00 00 00
 * 4 bytes: size in sectors:   C0 71 00 00
 * CSD register	 00 5D 01 32 │ 13 59 80 E3 │ 76 D9 CF FF │ 16 40 00 4F
 * CID register  01 50 41 53 | 30 31 36 42 │ 41 35 E4 39 │ 06 00 35 03
 * OCR register  80 FF 80 00
 */


static const Uint8 _stop_transmission_answer[] = {
	0, 0, 0, 0, // "stuff byte" and some of it is the R1 answer anyway
	0xFF // SD card is ready again
};
static const Uint8 _read_csd_answer[] = {
	0xFF, // waiting a bit
	0xFE, // data token
	// the CSD itself
	0x00, 0x5D, 0x01, 0x32, 0x13, 0x59, 0x80, 0xE3, 0x76, 0xD9, 0xCF, 0xFF, 0x16, 0x40, 0x00, 0x4F,
	0, 0  // CRC bytes
};
static const Uint8 _read_cid_answer[] = {
	0xFF, // waiting a bit
	0xFE, // data token
	// the CID itself
	0x01, 0x50, 0x41, 0x53, 0x30, 0x31, 0x36, 0x42, 0x41, 0x35, 0xE4, 0x39, 0x06, 0x00, 0x35, 0x03,
	0, 0  // CRC bytes
};
static const Uint8 _read_ocr_answer[] = { // no data token, nor CRC! (technically this is the R3 answer minus the R1 part at the beginning ...)
	// the OCR itself
	0x80, 0xFF, 0x80, 0x00
};
#if 0
static const Uint8 _read_sector_answer_faked[] = { // FAKE, we read the same for all CMD17 commands!
	0xFF, // wait a bit
	0xFE, // data token
	// the read block itself
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	
	0, 0 // CRC bytes
};
#endif




#define ADD_ANS(ans) { ans_p = (ans); ans_index = 0; ans_size = sizeof(ans); }





void sdext_clear_ram(void)
{
	memset(sd_ram_ext, 0xFF, 0x1C00);
}


/* SDEXT emulation currently excepts the cartridge area (segments 4-7) to be filled
 * with the FLASH ROM content. Even segment 7, which will be copied to the second 64K "hidden"
 * and pagable flash area of the SD cartridge. Currently, there is no support for the full
 * sized SDEXT flash image */
void sdext_init ( void )
{
	char path[PATH_MAX + 1];
	sdf = open_emu_file(SDCARD_IMG_FN, "rb", path);
	if (sdf == NULL) WARNING_WINDOW("SD card image file \"%s\" cannot be open: %s. You can use Xep128 but SD card access won't work!", path, ERRSTR());
	memset(sd_rom_ext, 0xFF, 0x10000);
	memcpy(sd_rom_ext, memory + 7 * 0x4000, 0x4000); // copy ROM image 16K to the extended area (the FLASH.ROM I have 8K is used only though)
	sdext_clear_ram();
	sdext_cp3m_usability = SDEXT_PHYSADDR_CART_P3_SELMASK_ON; // turn emulation on
	rom_page_ofs = 0;
	is_hs_read = 0;
	cmd_index = 0;
	ans_size = 0;
	ans_index = 0;
	ans_callback = NULL;
	status = 0;
	_read_b = 0;
	_write_b = 0xFF;
	_spi_last_w = 0xFF;
	printf("SDEXT: init\n");
}


static int blocks;

static void _block_read ( void )
{
	int ret;
	blocks++;
	_buffer[0] = 0xFF; // wait a bit
	_buffer[1] = 0xFE; // data token
	ret = fread(_buffer + 2, 1, 512, sdf);
#ifdef DEBUG_SDEXT
	printf("SDEXT: REGIO: fread retval = %d\n", ret);
#endif
	_buffer[512 + 2] = 0; // CRC
	_buffer[512 + 3] = 0; // CRC
	ans_p = _buffer;
	ans_index = 0;
	ans_size = 512 + 4;
}



/* SPI is a read/write in once stuff. We have only a single function ... 
 * _write_b is the data value to put on MOSI
 * _read_b is the data read from MISO without spending _ANY_ SPI time to do shifting!
 * This is not a real thing, but easier to code this way.
 * The implementation of the real behaviour is up to the caller of this function.
 */
static void _spi_shifting_with_sd_card ()
{
	if (!cs0) { // Currently, we only emulate one SD card, and it must be selected for any answer
		_read_b = 0xFF;
		return;
	}
	if (cmd_index == 0 && (_write_b & 0xC0) != 0x40) {
		if (ans_index < ans_size) {
#ifdef DEBUG_SDEXT
			printf("SDEXT: REGIO: streaming answer byte %d of %d-1 value %02X\n", ans_index, ans_size, ans_p[ans_index]);
#endif
			_read_b = ans_p[ans_index++];
		} else {
			if (ans_callback)
				ans_callback();
			else {
				//_read_b = 0xFF;
				ans_index = 0;
				ans_size = 0;
#ifdef DEBUG_SDEXT
				printf("SDEXT: REGIO: dummy answer 0xFF\n");
#endif
			}
			_read_b = 0xFF;
		}
		return;
	}
	if (cmd_index < 6) {
		cmd[cmd_index++] = _write_b;
		_read_b = 0xFF;
		return;
	}
#ifdef DEBUG_SDEXT
	printf("SDEXT: REGIO: command (CMD%d) received: %02X %02X %02X %02X %02X %02X\n", cmd[0] & 63, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
#endif
	cmd[0] &= 63;
	cmd_index = 0;
	ans_callback = NULL;
	switch (cmd[0]) {
		case 0:	// CMD 0
			_read_b = 1; // IDLE state R1 answer
			break;
		case 1:	// CMD 1 - init
			_read_b = 0; // non-IDLE now (?) R1 answer
			break;
		case 16:	// CMD16 - set blocklen (?!) : we only handles that as dummy command oh-oh ...
			_read_b = 0; // R1 answer
			break;
		case 9:  // CMD9: read CSD register
#ifdef DEBUG_SDEXT
			printf("SDEXT: REGIO: command is read CSD register\n");
#endif
			ADD_ANS(_read_csd_answer);
			_read_b = 0; // R1
			break;
		case 10: // CMD10: read CID register
			ADD_ANS(_read_cid_answer);
			_read_b = 0; // R1
			break;
		case 58: // CMD58: read OCR
			ADD_ANS(_read_ocr_answer);
			_read_b = 0; // R1 (R3 is sent as data in the emulation without the data token)
			break;
		case 12: // CMD12: stop transmission (reading multiple)
			ADD_ANS(_stop_transmission_answer);
			_read_b = 0;
			// actually we don't do too much, as on receiving a new command callback will be deleted before this switch-case block
#ifdef DEBUG_SDEXT
			printf("SDEXT: REGIO: block counter before CMD12: %d\n", blocks);
#endif
			blocks = 0;
			break;
		case 17: // CMD17: read a single block, babe
		case 18: // CMD18: read multiple blocks
			blocks = 0;
			if (sdf == NULL)
				_read_b = 32; // address error, if no SD card image ... [this is bad TODO, better error handling]
			else {
				int _offset = (cmd[1] << 24) | (cmd[2] << 16) | (cmd[3] << 8) | cmd[4];
#ifdef DEBUG_SDEXT
				printf("SDEXT: REGIO: seek to %d in the image file.\n", _offset);
#endif
				fseek(sdf, _offset, SEEK_SET);
				_block_read();
				/*
				fseek(sdf, (cmd[1] << 24) | (cmd[2] << 16) | (cmd[3] << 8) | cmd[4], SEEK_SET);
				_buffer[0] = 0xFF; // wait a bit
				_buffer[1] = 0xFE; // data token
				printf("SDEXT: REGIO: fread retval = %d\n", fread(_buffer + 2, 1, 512, sdf));
				_buffer[512 + 2] = 0; // CRC
				_buffer[512 + 3] = 0; // CRC
				ans_p = _buffer;
				ans_index = 0;
				ans_size = 512 + 4;
				*/
				if (cmd[0] == 18) ans_callback = _block_read; // in case of CMD18, continue multiple sectors, register callback for that!
				_read_b = 0; // R1
			}
			break;
		default: // unimplemented command, heh!
			printf("SDEXT: REGIO: unimplemented command %d = %02Xh\n", cmd[0], cmd[0]);
			_read_b = 4; // illegal command :-/
			break;
	}
}






/* Warning:
 * Some resources mention addresses like 0xFC00 for the I/O area.
 * Here, I mean addresses within segment 7 only, so it becomes 0x3C00 ...
 */



Uint8 sdext_read_cart_p3 ( Uint16 addr )
{
	//return 0xFF;
#ifdef DEBUG_SDEXT
	int pc = z80ex_get_reg(z80, regPC);
	printf("SDEXT: read P3 @ %04X [CPU: seg=%02X, pc=%04X]\n", addr, ports[0xB0 | (pc >> 14)], pc);
#endif
	if (addr < 0x2000) {
		Uint8 byte = sd_rom_ext[(rom_page_ofs + addr) & 0xFFFF];
#ifdef DEBUG_SDEXT
		printf("SDEXT: reading paged ROM, ROM offset = %04X, result = %02X\n", (addr + rom_page_ofs) & 0xFFFF, byte);
#endif
		//byte = 0xFF; // FUCK!
		return byte;
	}
	if (addr < 0x3C00) {
#ifdef DEBUG_SDEXT
		printf("SDEXT: reading RAM at offset %04X\n", addr - 0x2000);
#endif
		return sd_ram_ext[addr - 0x2000];
	} if (is_hs_read) {
		// in HS-read (high speed read) mode, all the 0x3C00-0x3FFF acts as data _read_ register (but not for _write_!!!)
		// also, there is a fundamental difference compared to "normal" read: each reads triggers SPI shifting in HS mode, but not in regular mode, there only write does that!
		Uint8 old = _read_b; // HS-read initiates an SPI shift, but the result (AFAIK) is the previous state, as shifting needs time!
		_spi_shifting_with_sd_card();
#ifdef DEBUG_SDEXT
		printf("SDEXT: REGIO: R: DATA: SPI data register HIGH SPEED read %02X [future byte %02X] [shited out was: %02X]\n", old, _read_b, _write_b);
#endif
		return old;
	} else
		switch (addr & 3) {
			case 0: 
				// regular read (not HS) only gives the last shifted-in data, that's all!
#ifdef DEBUG_SDEXT
				printf("SDEXT: REGIO: R: DATA: SPI data register regular read %02X\n", _read_b);
#endif
				return _read_b;
				//printf("SDEXT: REGIO: R: SPI, result = %02X\n", a);
			case 1: // status reg: bit7=wp1, bit6=insert, bit5=changed (insert/changed=1: some of the cards not inserted or changed)
#ifdef DEBUG_SDEXT
				printf("SDEXT: REGIO: R: status\n");
#endif
				return status;
				//return 0xFF - 32 + changed;
				//return changed | 64;
			case 2: // ROM pager [hmm not readble?!]
#ifdef DEBUG_SDEXT
				printf("SDEXT: REGIO: R: rom pager\n");
#endif
				return 0xFF;
				return rom_page_ofs >> 8;
			case 3: // HS read config is not readable?!]
#ifdef DEBUG_SDEXT
				printf("SDEXT: REGIO: R: HS config\n");
#endif
				return 0xFF;
				return is_hs_read;
			default:
				ERROR_WINDOW("SDEXT: FATAL, unhandled (RD) case");
				exit(1);
		}
	ERROR_WINDOW("SDEXT: FATAL, control should not get here");
	exit(1);
	return 0; // make GCC happy :)
}


void sdext_write_cart_p3 ( Uint16 addr, Uint8 data )
{
	//return;
#ifdef DEBUG_SDEXT
	int pc = z80ex_get_reg(z80, regPC);
	printf("SDEXT: write P3 @ %04X with %02X [CPU: seg=%02X, pc=%04X]\n", addr, data, ports[0xB0 | (pc >> 14)], pc);
#endif
	if (addr < 0x2000) return;	// pageable ROM (8K), do not overwrite [reflash is currently not supported]
	if (addr < 0x3C00) {		// SDEXT's RAM (7K), writable
#ifdef DEBUG_SDEXT
		printf("SDEXT: writing RAM at offset %04X\n", addr - 0x2000);
#endif
		sd_ram_ext[addr - 0x2000] = data;
		return;
	}
	// rest 1K is the (memory mapped) I/O area
	switch (addr & 3) {
		case 0:	// data register
#ifdef DEBUG_SDEXT
			printf("SDEXT: REGIO: W: DATA: SPI data register to %02X\n", data);
#endif
			if (!is_hs_read) _write_b = data;
			_write_specified = data;
			_spi_shifting_with_sd_card();
			break;
		case 1: // control register (bit7=CS0, bit6=CS1, bit5=clear change card signal
#ifdef DEBUG_SDEXT
			printf("SDEXT: REGIO: W: control register to %02X\n", data);
#endif
			if (data & 32) // clear change signal
				status &= 255 - 32;
			cs0 = data & 128;
			cs1 = data & 64;
			break;
		case 2: // ROM pager register
			rom_page_ofs = data << 8;
#ifdef DEBUG_SDEXT
			printf("SDEXT: REGIO: W: paging ROM to %02X\n", data);
#endif
			break;
		case 3: // HS (high speed) read mode to set: bit7=1
			is_hs_read = data & 128;
			_write_b = is_hs_read ? 0xFF : _write_specified;
#ifdef DEBUG_SDEXT
			printf("SDEXT: REGIO: W: HS read mode is %s\n", is_hs_read ? "set" : "reset");
#endif
			break;
		default:
			ERROR_WINDOW("SDEXT: FATAL, unhandled (WR) case");
			exit(1);
	}
}

#endif
