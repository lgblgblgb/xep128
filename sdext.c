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

/* General SD information:
	http://elm-chan.org/docs/mmc/mmc_e.html
	http://www.mikroe.com/downloads/get/1624/microsd_card_spec.pdf
	http://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf
   Flash IC used (AM29F400BT) on the cartridge:
	http://www.mouser.com/ds/2/380/spansion%20inc_am29f400b_eol_21505e8-329620.pdf
*/

#include "xepem.h"

#ifdef CONFIG_SDEXT_SUPPORT
#define DEBUG_SDEXT
#define CONFIG_SDEXT_FLASH

static const char *sdext_rom_signature = "SDEXT";

int sdext_cart_enabler = SDEXT_CART_ENABLER_OFF;
char sdimg_path[PATH_MAX + 1];
static int rom_page_ofs;
static int is_hs_read;
static Uint8 _spi_last_w;
static int cs0, cs1;
static Uint8 status;

static Uint8 sd_ram_ext[7 * 1024]; // 7K of accessible SRAM
/* The FIRST 64K of flash (sector 0) is structured this way:
   * first 48K is accessed directly at segment 4,5,6, so it's part of the normal EP memory emulated
   * the last 16K is CANNOT BE accessed at all from EP
   It's part of the main memory array at the given offset, see flash[0][addr] below
   The SECOND 64K of flash (sector 1) is stored in sd_rom_ext (see below), it's flash[1][addr] */
static Uint8 sd_rom_ext[0x10000];
static Uint8 *flash[2] = { memory + 0x10000, sd_rom_ext };

static int flash_wr_protect = 0;
static int flash_bus_cycle = 0;
static int flash_command = 0;

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



static int sdext_detect_rom ( void )
{
	Uint8 *p = memory + 7 * 0x4000;
	Uint8 *p2 = p + 0x2000 - strlen(sdext_rom_signature);
	if (memcmp(p, "EXOS_ROM", 8))
		return 1;	// No EXOS_ROM header
	for (; p < p2; p++ ) {
		if (!memcmp(p, sdext_rom_signature, strlen(sdext_rom_signature)))
			return 0;	// found our extra ID
	}
	return 1;	// our ID cannot be found
}



/* SDEXT emulation currently excepts the cartridge area (segments 4-7) to be filled
 * with the FLASH ROM content. Even segment 7, which will be copied to the second 64K "hidden"
 * and pagable flash area of the SD cartridge. Currently, there is no support for the full
 * sized SDEXT flash image */
void sdext_init ( void )
{
	/* try to detect SDEXT ROM extension and only turn on emulation if it exists */
	if (sdext_detect_rom()) {
		WARNING_WINDOW("No SD-card cartridge ROM code found in loaded ROM set. SD card hardware emulation has been disabled!");
		*sdimg_path = 0;
		sdf = NULL;
		printf("SDEXT: init: REFUSE: no SD-card cartridge ROM code found in loaded ROM set.\n");
		return;
	}
	printf("SDEXT: init: cool, SD-card cartridge ROM code seems to be found in loaded ROM set, enabling SD card hardware emulation ...\n");
	sdf = open_emu_file(SDCARD_IMG_FN, "rb", sdimg_path);
	if (sdf == NULL) {
		WARNING_WINDOW("SD card image file \"%s\" cannot be open: %s. You can use Xep128 but SD card access won't work!", sdimg_path, ERRSTR());
		*sdimg_path = 0;
	}
	memset(sd_rom_ext, 0xFF, 0x10000);
	/* Copy ROM image of 16K to the second 64K of the cartridge flash. Currently only 8K is used.
           It's possible to use 64K the ROM set image used by Xep128 can only hold 16K this way, though. */
	memcpy(sd_rom_ext, memory + 7 * 0x4000, 0x4000);
	sdext_clear_ram();
	sdext_cart_enabler = SDEXT_CART_ENABLER_ON;	// turn emulation on
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
	printf("SDEXT: init end\n");
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



static void flash_erase ( int sector )	// erase sectors 0 or 1, or both if -1 is given!
{
	if (sector < 1) {
		memset(flash[0], 0xFF, 0xC000);		// erase sector 0, it's only 48K accessible on SD/EP so it does not matter, real flash would be 64K here too
		printf("SDEXT: FLASH: erasing sector 0!\n");
		WARNING_WINDOW("Erasing flash sector 0! You can safely ignore this warning.");
	}
	if (abs(sector) == 1) {
		memset(flash[1], 0xFF, 0x10000);	// erase sector 1
		printf("SDEXT: FLASH: erasing sector 1!\n");
		WARNING_WINDOW("Erasing flash sector 1! You can safely ignore this warning.");
	}
}


// flash programming allows only 1->0 on data bits, erase must be executed for 0->1
#define FLASH_PROGRAM_BYTE(sector, addr, data) flash[sector][addr] &= (data)


static Uint8 flash_rd_bus_op ( int sector, Uint16 addr )
{

	//if (flash_status_polling > -1)
	//	return flash_status_polling;
//	if (flash_data_mode)
		return flash[sector][addr];
#if 0
	if (base)
		return sd_rom_ext[addr];	// reading from second flash sector
	else
		return sd_rom_ext_low[addr];	// reading from first flash sector
#endif
}


static void flash_cmd_return ( void )
{
	flash_bus_cycle = 0;
	flash_command = 0;
}


static int flash_warn_programming = 1;
static void flash_wr_bus_op ( int sector, Uint16 addr, Uint8 data )
{
	int idaddr = addr & 0x3FFF;
	printf("SDEXT: FLASH: WR OP: sector %d addr %04Xh data %02Xh flash-bus-cycle %d flash-command %02Xh\n", sector, addr, data, flash_bus_cycle, flash_command);
	if (flash_wr_protect)
		return;	// write protection on flash, do not accept any write bus op
	if (flash_command == 0x90)
		flash_bus_cycle = 0;	// autoselect mode does not have wr cycles more (only rd)
	switch (flash_bus_cycle) {
		case 0:
			flash_command = 0;	// invalidate command
			if (data == 0xB0 || data == 0x30) {
				//WARNING_WINDOW("SDEXT FLASH erase suspend/resume (cmd %02Xh) is not emulated yet :-(", data);
				return; // well, erase suspend/resume is currently not supported :-(
			}
			if (data == 0xF0) return; // reset command
			if (idaddr != 0xAAA || data != 0xAA) return; // invalid cmd seq
			flash_bus_cycle = 1;
			return;
		case 1:
			if (idaddr != 0x555 || data != 0x55) { // invalid cmd seq
				flash_bus_cycle = 0;
				return;
			}
			flash_bus_cycle = 2;
			return;
		case 2:
			if (idaddr != 0xAAA) {
				flash_bus_cycle = 0; // invalid cmd seq
				return;
			}
			if (data != 0x90 && data != 0x80 && data != 0xA0) {
				flash_bus_cycle = 0; // invalid cmd seq
				return;
			}
			flash_command = data;
			flash_bus_cycle = 3;
			return;
		case 3:
			if (flash_command == 0xA0) {	// program command!!!!
				// flash programming allows only 1->0 on data bits, erase must be executed for 0->1
				Uint8 oldbyte = flash[sector][addr];
				Uint8 newbyte = oldbyte & data;
				flash[sector][addr] = newbyte;
				printf("SDEXT: FLASH: programming: sector %d address %04Xh data-req %02Xh, result %02Xh->%02Xh\n", sector, addr, data, oldbyte, newbyte);
				if (flash_warn_programming) {
					WARNING_WINDOW("Flash programming detected! There will be no further warnings on more bytes.\nYou can safely ignore this warning.");
					flash_warn_programming = 0;
				}
				flash_command = 0; // end of command
				flash_bus_cycle = 0;
				return;
			}
			// only flash command 0x80 can be left, 0x90 handled before "switch", 0xA0 just before ...
			if (idaddr != 0xAAA || data != 0xAA) { // invalid cmd seq
				flash_command = 0;
				flash_bus_cycle = 0;
				return;
			}
			flash_bus_cycle = 4;
			return;
		case 4:	// only flash command 0x80 can get this far ...
			if (idaddr != 0x555 || data != 0x55) { // invalid cmd seq
				flash_command = 0;
				flash_bus_cycle = 0;
				return;
			}
			flash_bus_cycle = 5;
			return;
		case 5:	// only flash command 0x80 can get this far ...
			if (idaddr == 0xAAA && data == 0x10) {	// CHIP ERASE!!!!
				flash_erase(-1);
			} else if (data == 0x30) {
				flash_erase(sector);
			}
			flash_bus_cycle = 0; // end of erase command?
			flash_command = 0;
			return;
		default:
			ERROR_WINDOW("Invalid SDEXT FLASH bus cycle #%d on WR", flash_bus_cycle);
			exit(1);
			break;
	}


#if 0
	if (flash_cmd_seq == 0 && data == 0xF0) {	// command "reset"
		flash_data_mode = 1;
		return flash_to_data_mode();
		if (data == 0xB0 || data == 0x30)
	}

#endif

	//int flashaddr = base | addr;
	// currently nothing ...
//	printf("SDEXT: FLASH: not supported yet ...\n");
}



/* We expects all 4-7 seg reads/writes to be handled, as for re-flashing emu etc will need it!
   Otherwise only segment 7 would be enough if flash is not emulated other than only "some kind of ROM". */

Uint8 sdext_read_cart ( Uint16 addr )
{
#ifdef DEBUG_SDEXT
	int pc = z80ex_get_reg(z80, regPC);
	printf("SDEXT: read cart @ %04X [CPU: seg=%02X, pc=%04X]\n", addr, ports[0xB0 | (pc >> 14)], pc);
#endif
	if (addr < 0xC000) {
		Uint8 byte = flash_rd_bus_op(0, addr);
#ifdef DEBUG_SDEXT
		printf("SDEXT: reading base ROM, ROM offset = %04X, result = %02X\n", addr, byte);
#endif
		return byte;
	}
	if (addr < 0xE000) {
		Uint8 byte;
		addr = rom_page_ofs + (addr & 0x1FFF);
		byte = flash_rd_bus_op(1, addr);
#ifdef DEBUG_SDEXT
		printf("SDEXT: reading paged ROM, ROM offset = %04X, result = %02X\n", addr, byte);
#endif
		return byte;
	}
	if (addr < 0xFC00) {
		addr -= 0xE000;
#ifdef DEBUG_SDEXT
		printf("SDEXT: reading RAM at offset %04X, result = %02X\n", addr, sd_ram_ext[addr]);
#endif
		return sd_ram_ext[addr];
	}
	if (is_hs_read) {
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


void sdext_write_cart ( Uint16 addr, Uint8 data )
{
#ifdef DEBUG_SDEXT
	int pc = z80ex_get_reg(z80, regPC);
	printf("SDEXT: write cart @ %04X with %02X [CPU: seg=%02X, pc=%04X]\n", addr, data, ports[0xB0 | (pc >> 14)], pc);
#endif
	if (addr < 0xC000) {		// segments 4-6, call flash WR emulation
		flash_wr_bus_op(0, addr, data);
		return;
	}
	if (addr < 0xE000) {		// pageable ROM (8K), call flash WR emulation
		flash_wr_bus_op(1, (addr & 0x1FFF) + rom_page_ofs, data);
		return;
	}
	if (addr < 0xFC00) {		// SDEXT's RAM (7K), writable
		addr -= 0xE000;
#ifdef DEBUG_SDEXT
		printf("SDEXT: writing RAM at offset %04X\n", addr);
#endif
		sd_ram_ext[addr] = data;
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
			if (data & 32) // clear change signal
				status &= 255 - 32;
			cs0 = data & 128;
			cs1 = data & 64;
#ifdef DEBUG_SDEXT
			printf("SDEXT: REGIO: W: control register to %02X CS0=%d CS1=%d\n", data, cs0, cs1);
#endif
			break;
		case 2: // ROM pager register
			rom_page_ofs = (data & 0xE0) << 8;	// only high 3 bits count
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
