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

//#define RESET_RTC_INDEX



static int _register;
static Uint8 cmos_ram[0x100];
int rtc_update_trigger;





void rtc_set_reg(Uint8 val)
{
	_register = val;
	printf("RTC: register number %02X has been selected\n", val);
}


void rtc_write_reg(Uint8 val)
{
	printf("RTC: write reg %02X with data %02X\n", _register, val);
	if (_register == 0xC || _register == 0xD) return;
	if (_register == 0xA) val &= 127;
	cmos_ram[_register] = val;
#ifdef RESET_RTC_INDEX
	_register = 0xD;
#endif
}


static int _conv(int bin, int is_hour)
{
	int b7 = 0;
	if (is_hour && (!(cmos_ram[0xB] & 2))) { // AM/PM
		if (bin == 0) {
			bin = 12;
		} else if (bin == 12) {
			b7 = 128;
		} else if (bin > 12) {
			bin -= 12;
			b7 = 128;
		}
	}
	if (!(cmos_ram[0xB] & 4)) { // do bin->bcd
		bin = ((bin / 10) << 4) | (bin % 10);
	}
	return bin | b7;
}


static void _update(void)
{
	//time_t now = time(NULL);
	time_t now = emu_getunixtime();
	struct tm *t = localtime(&now);
	cmos_ram[   0] = _conv(t->tm_sec, 0);
	cmos_ram[   2] = _conv(t->tm_min, 0);
	cmos_ram[   4] = _conv(t->tm_hour, 1);
	cmos_ram[   6] = _conv(t->tm_wday + 1, 0);  // day, 1-7 (week)
	cmos_ram[   7] = _conv(t->tm_mday, 0); // date, 1-31
	cmos_ram[   8] = _conv(t->tm_mon + 1, 0); // month, 1 -12
	cmos_ram[   9] = _conv((t->tm_year % 100) + 20, 0); // year, 0 - 99
	cmos_ram[0x32] = _conv(21, 0); // century???
	printf("RTC: time/date has been updated for \"%d-%02d-%02d %02d:%02d:%02d\" at UNIX epoch %ld\n",
		t->tm_year + 1900,
		t->tm_mon + 1,
		t->tm_mday,
		t->tm_hour,
		t->tm_min,
		t->tm_sec,
		now
	);
}


void rtc_reset(void)
{
	memset(cmos_ram, 0, 0x100);
	_register = 0xD;
	rtc_update_trigger = 0;
	cmos_ram[0xA] = 32;
	cmos_ram[0xB] = 2; // 2 | 4;
	cmos_ram[0xC] = 0;
	cmos_ram[0xD] = 128;
	printf("RTC: reset\n");
	_update();
}


Uint8 rtc_read_reg(void)
{
	int i = _register;
#ifdef RESET_RTC_INDEX
	_register = 0xD;
#endif
	if (i > 63)
		return 0xFF;
	if (rtc_update_trigger && (cmos_ram[0xB] & 128) == 0 && i < 10) {
		_update();
		rtc_update_trigger = 0;
	}
	printf("RTC: reading register %02X, result will be: %02X\n", i, cmos_ram[i]);
	return cmos_ram[i];
#if 0
	
	if (cmos_ram[0xB] & 128) { // do not update
		if (DEBUG_RTC) debug("RTC: read (do not update!) reg 0x" + i.toString(16) + " result is 0x" + cmosRam[i].toString(16));
		return cmosRam[i]; // SET bit == 1, do not update
	}
	if (i in [0, 2, 4, 6, 7, 8, 9, 0x32]) rtcDoUpdate();
	if (DEBUG_RTC) debug("RTC: read reg 0x" + i.toString(16) + " result is 0x" + cmosRam[i].toString(16));
	return cmos_ram[i];
#endif
}
