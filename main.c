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

static Uint32 *ep_pixels;

int rom_size;
int CPU_CLOCK = DEFAULT_CPU_CLOCK;

char *app_pref_path, *app_base_path;
char current_directory[PATH_MAX + 1];
char rom_path[PATH_MAX + 1];

static const int _cpu_speeds[4] = { 4000000, 6000000, 7120000, 10000000 };
static int _cpu_speed_index = 0;



static void shutdown_sdl(void)
{
	audio_close();
	printer_close();
#ifdef CONFIG_W5300_SUPPORT
	w5300_shutdown();
#endif
	fprintf(stderr, "Shutdown callback, return.\n");
	if (sdl_win)
		SDL_DestroyWindow(sdl_win);
	SDL_Quit();
}


FILE *open_emu_file ( const char *name, const char *mode, char *pathbuffer )
{
	const char *prefixes[] = {
		current_directory,	// try in the current directory first
#ifndef _WIN32
		DATADIR "/",		// try in the DATADIR, it makes sense on UNIX like sys
#endif
		app_base_path,		// try at base path (where executable is)
		app_pref_path,		// try at pref path (user writable area)
		NULL
	};
	int a = 0;
	FILE *f;
	while (prefixes[a] != NULL)
		if (strcmp(prefixes[a], "?")) {
			sprintf(pathbuffer, "%s%s", prefixes[a], name);
			fprintf(stderr, "OPEN: trying file \"%s\" as path \"%s\": ",
				name, pathbuffer
			);
			f = fopen(pathbuffer, mode);
			if (f == NULL) {
				a++;
				fprintf(stderr, "FAILED\n");
			} else {
				fprintf(stderr, "OK\n");
				return f;
			}
		}
	fprintf(stderr, "OPEN: no file could be open for \"%s\"\n", name);
	strcpy(pathbuffer, name);
	return NULL;
}


static int load_roms ( const char *basename, char *path )
{
	FILE *f;
	int ret;
	printf("ROM: loading %s\n", basename);
	f = open_emu_file(basename, "rb", path);
	if (f == NULL) {
		ERROR_WINDOW("Cannot open ROM image \"%s\": %s", basename, ERRSTR());
		return -1;
	}
	ret = fread(memory, 1, 0x100001, f);
	printf("Read = %d\n", ret);
	fclose(f);
	if (ret < 0) {
		ERROR_WINDOW("Cannot read ROM image \"%s\": %s", path, ERRSTR());
	}
	if (ret < 0x8000) {
		ERROR_WINDOW("ROM image \"%s\" is too short.", path);
		return -1;
	}
	if (ret == 0x100001) {
		ERROR_WINDOW("ROM image \"%s\" is too large.", path);
		return -1;
	}
	if (ret & 0x3FFF) {
		ERROR_WINDOW("ROM image \"%s\" size is not multiple of 0x4000 bytes.", path);
		return -1;
	}
	printf("ROM: ROM-set is loaded, %06Xh bytes (%d Kbytes), segments 00-%02Xh\n", ret, ret >> 10, (ret >> 14) - 1);
	return ret;
}



static unsigned int ticks;
static int running;



//static int tstates_all = 0;
static int cpu_cycles_for_dave_sync = 0;

static double td_balancer = 0;

//time_t unix_time;


static struct timeval tv_old;

static int td_em_ALL = 0, td_pc_ALL = 0, td_count_ALL = 0;

/* This is the emulation timing stuff
 * Should be called at the END of the emulation loop.
 * Input parameter: microseconds needed for the "real" (emulated) computer to do our loop 
 * This function also does the sleep itself */
static void emu_timekeeping_delay ( int td_em )
{
	int td, td_pc;
	struct timeval tv_new;
	gettimeofday(&tv_new, NULL);
	td_pc = (tv_new.tv_sec - tv_old.tv_sec) * 1000000 + (tv_new.tv_usec - tv_old.tv_usec); // get realtime since last call in microseconds
	if (td_pc < 0) return; // time goes backwards? maybe time was modified on the host computer. Skip this delay cycle
	//td_ep = 1000000 * rasters * 57 / NICK_SLOTS_PER_SEC; // microseconds would need for an EP128 to do this
	td = td_em - td_pc; // the time difference (+X = PC is faster - real time EP emulation, -X = EP is faster - real time EP emulation is not possible)
	printf("DELAY: pc=%d em=%d sleep=%d\n", td_pc, td_em, td);
	/* for reporting only: BEGIN */
	td_em_ALL += td_em;
	td_pc_ALL += td_pc;
	if (td_count_ALL == 50) {
		char buf[256];
		//fprintf(stderr, "STAT: count = %d, EM = %d, PC = %d, usage = %f%\n", td_count_ALL, td_em_ALL, td_pc_ALL, 100.0 * (double)td_pc_ALL / (double)td_em_ALL);
		sprintf(buf, "%s [%.2fMHz ~ %d%%]", WINDOW_TITLE " " VERSION " ",
			CPU_CLOCK / 1000000.0,
			td_pc_ALL * 100 / td_em_ALL
		);
		SDL_SetWindowTitle(sdl_win, buf);
		td_count_ALL = 0;
		td_pc_ALL = 0;
		td_em_ALL = 0;
	} else
		td_count_ALL++;
	/* for reporting only: END */
	if (td > 0) {
		td_balancer += td;
		if (td_balancer > 0)
			usleep((int)td_balancer);
	}
	/* Purpose:
	 * get the real time spent sleeping (sleep is not an exact science on a multitask OS)
	 * also this will get the starter time for the next frame
	 */
	gettimeofday(&tv_old, NULL); // to calc
	// calculate real time slept
	td = (tv_old.tv_sec - tv_new.tv_sec) * 1000000 + (tv_old.tv_usec - tv_new.tv_usec);
	printf("Really slept = %d\n", td);
	if (td < 0) return; // invalid, sleep was about for _minus_ time? eh, give me that time machine, dude! :)
	td_balancer -= td;
	if (td_balancer >  1000000)
		td_balancer = 0;
	else if (td_balancer < -1000000)
		td_balancer = 0;
	printf("Balancer = %lf\n", td_balancer);
	//unix_time = tv_old.tv_sec; // publish current time
}


time_t emu_getunixtime ( void )
{
	return tv_old.tv_sec;
}


/* Should be started on each time, emulation is started/resumed (ie after any delay in emulation like pause, etc)
 * You DO NOT need this during the active emulation loop! */
void emu_timekeeping_start ( void )
{
	gettimeofday(&tv_old, NULL);
	td_balancer = 0.0;
	running = 1;
}



// called by nick.c
void emu_one_frame(int rasters, int frameksip)
{
	SDL_Event e;
	//if (!frameskip)
	screen_present_frame(ep_pixels);
	while (SDL_PollEvent(&e) != 0)
		switch (e.type) {
			case SDL_WINDOWEVENT:
				if (!is_fullscreen && e.window.event == SDL_WINDOWEVENT_RESIZED) {
					fprintf(stderr, "Window is resized to %d x %d\n",
						e.window.data1,
						e.window.data2
					);
					screen_window_resized(e.window.data1, e.window.data2);
				}
				break;
			case SDL_QUIT:
				if (QUESTION_WINDOW("?No|!Yes", "Are you sure to exit?") == 1) running = 0; 
				//running = 0;
				//INFO_WINDOW("You are about leaving Xep128. Good bye!");
				return;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (e.key.repeat == 0 && (e.key.windowID == sdl_winid || e.key.windowID == 0)) {
					if (e.key.keysym.scancode == SDL_SCANCODE_F11 && e.key.state == SDL_PRESSED) {
						screen_set_fullscreen(!is_fullscreen);
					} else if (e.key.keysym.scancode == SDL_SCANCODE_F9 && e.key.state == SDL_PRESSED) {
						if (QUESTION_WINDOW("?No|!Yes", "Are you sure to exit?") == 1) running = 0;
					} else if (e.key.keysym.scancode == SDL_SCANCODE_F10 && e.key.state == SDL_PRESSED)
						screen_shot(ep_pixels, current_directory, "screenshot-*.png");
					else if (e.key.keysym.scancode == SDL_SCANCODE_PAUSE && e.key.state == SDL_PRESSED) {
						if (e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
							zxemu_on = 0;
							ep_clear_ram();
						}
						ep_reset();
					} else if (e.key.keysym.scancode == SDL_SCANCODE_PAGEDOWN && e.key.state == SDL_PRESSED && _cpu_speed_index) {
						set_cpu_clock_with_osd(_cpu_speeds[-- _cpu_speed_index]);
					} else if (e.key.keysym.scancode == SDL_SCANCODE_PAGEUP && e.key.state == SDL_PRESSED && _cpu_speed_index < 3) {
						set_cpu_clock_with_osd(_cpu_speeds[++ _cpu_speed_index]);
					} else if (e.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
						osd_replay(e.key.state == SDL_PRESSED ? 0 : OSD_FADE_START);
					} else
						emu_kbd(e.key.keysym, e.key.state == SDL_PRESSED);
				} else if (e.key.repeat == 0)
					fprintf(stderr, "NOT HANDLED KEY EVENT: repeat = %d windowid = %d [our win = %d]\n", e.key.repeat, e.key.windowID, sdl_winid);
				break;
			case SDL_MOUSEMOTION:
				if (e.button.windowID == sdl_winid)
					emu_mouse_motion(e.motion.xrel, e.motion.yrel);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (e.button.windowID == sdl_winid)
					emu_mouse_button(e.button.button, e.button.state == SDL_PRESSED);
				break;
		}
	rtc_update_trigger = 1; // triggers RTC update on the next RTC register read. Woooo!
	emu_timekeeping_delay(1000000.0 * rasters * 57.0 / (double)NICK_SLOTS_PER_SEC);
#if 0
	ulate the precise time spent sleeping with SDL_Delay()
	ticks_new = SDL_GetTicks();
	used = ticks_new - ticks;
	usecs = usecs * 64 / 1000; // "usecs" for real are the raster counter got, however 64usec is one raster, then convert to msec
	printf("usecs = %d used = %d\n", usecs, used);
	//usecs = usecs * 64 / 1000;
	ticks = ticks_new;
	if (used)
		printf("Speed: %d%% TSTATES_ALL=%d\n", 100*usecs/used, tstates_all);
	else
		printf("DIVIDE BY ZERO!\n");
	tstates_all = 0;
	if (used > usecs) {
		printf("Too slow! realtime=%d used=%d\n", usecs, used);
		SDL_Delay(10);
	} else {
		printf("Delay: %d\n", usecs - used);
		int foo = SDL_GetTicks();
		SDL_Delay(usecs - used);
		printf("Real sleep was: %d\n", SDL_GetTicks() - foo);
	}
#endif
}


static double balancer;
//static float SCALER = 1.1228070175438596 * 2;
//static float SCALER = 5.685098823006883;
//static float SCALER = 0.2217543859649123;
//static float SCALER = 0.2223;
static double SCALER;


int set_cpu_clock ( int hz )
{
	if (hz <  1000000) hz =  1000000;
	if (hz > 12000000) hz = 12000000;
	CPU_CLOCK = hz;
	SCALER = (double)NICK_SLOTS_PER_SEC / (double)CPU_CLOCK;
	fprintf(stderr, "CPU: clock = %d scaler = %f\n", CPU_CLOCK, SCALER);
	dave_set_clock();
	return hz;
}


int set_cpu_clock_with_osd ( int hz )
{
	hz = set_cpu_clock(hz);
	OSD("CPU speed: %.2f MHz", hz / 1000000.0);
	return hz;
}



static int get_sys_dirs ( const char *path )
{
	fprintf(stderr, "Program path: %s\n", path);
	//fprintf(stderr, "XEP ROM size: %d\n", sizeof _xep_rom);
	app_pref_path = SDL_GetPrefPath("nemesys.lgb", "xep128");
	app_base_path = SDL_GetBasePath();
	if (app_pref_path == NULL) app_pref_path = SDL_strdup("?");
	if (app_base_path == NULL) app_base_path = SDL_strdup("?");
	fprintf(stderr, "SDL base path: %s\n", app_base_path);
	fprintf(stderr, "SDL pref path: %s\n", app_pref_path);
	if (getcwd(current_directory, sizeof current_directory) == NULL) {
		ERROR_WINDOW("Cannot query the current directory: %s", ERRSTR());
		return 1;
	}
	strcat(current_directory, DIRSEP);
	fprintf(stderr, "Current directory: %s\n", current_directory);
	return 0;
}



int main (int argc, char *argv[])
{
	atexit(shutdown_sdl);
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		ERROR_WINDOW("Fatal SDL initialization problem: %s", SDL_GetError());
		return 1;
	}
	if (get_sys_dirs(argv[0]))
		return 1;
	if (screen_init())
		return 1;
	audio_init(0);
	z80ex_init();
	set_ep_cpu(CPU_Z80);
	ep_pixels = nick_init();
	if (ep_pixels == NULL)
		return 1;
	rom_size = load_roms(COMBINED_ROM_FN, rom_path);
	if (rom_size <= 0)
		return 1;
	primo_rom_seg = primo_search_rom();
	xep_rom_install(rom_size);
	//memset(memory + rom_size, 0, 0x4000);
	//memcpy(memory + rom_size, _xep_rom, sizeof _xep_rom);
	xep_rom_seg = rom_size >> 14;
	xep_rom_addr = rom_size;
	fprintf(stderr, "XEP ROM segment will be %02Xh @ %06Xh\n", xep_rom_seg, xep_rom_addr);
	rom_size += 0x4000;
	set_ep_ramsize(1024);
	mouse_entermice(0);	// 1=Entermice protocol (8 nibbles), 0=boxsoft (4 nibbles), also different lines
	ep_reset();
	kbd_matrix_reset();
#ifdef CONFIG_SDEXT_SUPPORT
	sdext_init();
#endif
#ifdef CONFIG_W5300_SUPPORT
	w5300_init(NULL);
#endif
	ticks = SDL_GetTicks();
	running = 1;
	balancer = 0;
#if 0
	int last_optype = 0;
#endif
	//printf("CPU: clock = %d scaler = %f\n", CPU_CLOCK, SCALER);
	set_cpu_clock(DEFAULT_CPU_CLOCK);
	emu_timekeeping_start();
	audio_start();
	//osd_disable();
	while (running) {
		int t;
#if 0
		char buffer[256];
		int pc = z80ex_get_reg(z80, regPC);
		////printf("PC=%04Xh\n", z80ex_get_reg(z80, regPC));
		//z80ex_dasm(buffer, sizeof(buffer) - 1, 0, &t_states, &t_states2, pc);
		//printf("%04X %s\n", pc, buffer);
		z80_dasm(buffer, pc, -1);
#endif
		if (nmi_pending) {
			t = z80ex_nmi();
			fprintf(stderr, "NMI: %d\n", t);
			if (t)
				nmi_pending = 0;
		} else
			t = 0;
		if ((t == 0) && (dave_int_read & 0xAA)) {
			t = z80ex_int();
			if (t)
				printf("CPU: int and accepted = %d\n", t);
		} else
			t = 0;
		if (!t)
			t = z80ex_step();
		cpu_cycles_for_dave_sync += t;
		//printf("DAVE: SYNC: CPU cycles = %d, Dave sync val = %d, limit = %d\n", t, cpu_cycles_for_dave_sync, cpu_cycles_per_dave_tick);
		while (cpu_cycles_for_dave_sync >= cpu_cycles_per_dave_tick) {
			dave_tick();
			cpu_cycles_for_dave_sync -= cpu_cycles_per_dave_tick;
		}
		balancer += t * SCALER;
		//tstates_all += t;
		//printf("%s [balance=%f t=%d]\n", buffer, balancer, t);
		while (balancer >= 0.5) {
			nick_render_slot();
			balancer -= 1.0;
		}
		//printf("[balance=%f t=%d]\n", balancer, t);
#if 0
		if (!last_optype)
			printf("%s [balance=%f t=%d] TYPE=%d\n", buffer, balancer, t, z80ex_last_op_type(z80));
		last_optype = z80ex_last_op_type(z80);
#endif
	}
	nick_dump_lpt();
	return 0;
}

