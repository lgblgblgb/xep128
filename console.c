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


#include "xepem.h"
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#endif

#define USE_MONITOR	1

int console_is_open = 0;
static int ok_for_monitor = 0;
static volatile int monitor_running = 0;
static SDL_Thread *mont = NULL;




static int monitor_thread ( void *ptr )
{
	printf("Welcome to " WINDOW_TITLE " monitor. Use ? for help" NL);
	for (;;) {
		char buffer[200];
		char *p;
		if (!monitor_running)
			break;
		printf(WINDOW_TITLE "> ");
		p = fgets(buffer, sizeof buffer, stdin);
		if (!monitor_running)
			break;
		if (p == NULL) {
			sleep(1);
		} else {
			for (p = strlen(buffer) + buffer - 1; p >= buffer && (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t'); p--)
				*p = '\0';
			p[strlen(p) + 1] = '\0';
			for (p = buffer; *p && (*p == ' ' || *p == '\t'); p++)
				;
			printf("Entered: \"%s\" [%d]" NL, p, strlen(p));
			printf("AF =%04X BC =%04X DE =%04X HL =%04X IX=%04X IY=%04X" NL "AF'=%04X BC'=%04X DE'=%04X HL'=%04X PC=%04X SP=%04X" NL "Pages: %02X %02X %02X %02X" NL,
				Z80_AF,  Z80_BC,  Z80_DE,  Z80_HL,  Z80_IX, Z80_IY,
				Z80_AF_, Z80_BC_, Z80_DE_, Z80_HL_, Z80_PC, Z80_SP,
				ports[0xB0], ports[0xB1], ports[0xB2], ports[0xB3]
			);
		}
	}
	printf("MONITOR: thread is about to exit" NL);
	return 0;
}



static void monitor_start ( void )
{
	if (!ok_for_monitor || !console_is_open || monitor_running || !USE_MONITOR)
		return;
	DEBUGPRINT("MONITOR: start" NL);
	monitor_running = 1;
	mont = SDL_CreateThread(monitor_thread, WINDOW_TITLE " monitor", NULL);
	if (mont == NULL)
		monitor_running = 0;
}



static int monitor_stop ( void )
{
	int ret;
	if (!monitor_running)
		return 0;
	DEBUGPRINT("MONITOR: stop" NL);
	monitor_running = 0;
	if (mont != NULL) {
		printf(NL NL "*** PRESS ENTER TO EXIT ***" NL);
		// Though Info window here is overkill, I am still interested why it causes a segfault when I've tried ...
		//INFO_WINDOW("Monitor runs on console. You must press ENTER there to continue");
		SDL_WaitThread(mont, &ret);
		mont = NULL;
		DEBUGPRINT("MONITOR: thread joined, status code is %d" NL, ret);
	}
	return 1;
}



void console_open_window ( void )
{
#ifdef _WIN32
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;
	if (console_is_open)
		return;
	console_is_open = 0;
	FreeConsole();
	if (!AllocConsole()) {
		ERROR_WINDOW("Cannot allocate windows console!");
		return;
	}
	SetConsoleTitle(WINDOW_TITLE " console");
	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = 1024;
	//coninfo.dwSize.X = 100;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );
	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );
	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;
	setvbuf( stderr, NULL, _IONBF, 0 );
	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
	// ios::sync_with_stdio();
	// Attributes
	//SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	DEBUGPRINT("WINDOWS: console is open" NL);
#endif
	console_is_open = 1;
	monitor_start();
}



void console_close_window ( void )
{
	if (!console_is_open)
		return;
	monitor_stop();
#ifdef _WIN32
	if (!FreeConsole())
		ERROR_WINDOW("Cannot release windows console!");
	else
		console_is_open = 0;
#else
	console_is_open = 0;
#endif
}



void console_close_window_on_exit ( void )
{
#ifdef _WIN32
	if (console_is_open && !monitor_stop())
		INFO_WINDOW("Click to close console window");
#else
	monitor_stop();
#endif
	console_close_window();
}



void console_monitor_ready ( void )
{
	ok_for_monitor = 1;
	monitor_start();
}
