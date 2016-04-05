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
int CPU_CLOCK = DEFAULT_CPU_CLOCK;
static const int _cpu_speeds[4] = { 4000000, 6000000, 7120000, 10000000 };
static int _cpu_speed_index = 0;
static int guarded_exit = 0;
static unsigned int ticks;
int paused = 0;
static int cpu_cycles_for_dave_sync = 0;
static double td_balancer = 0;
static struct timeval tv_old;
static int td_em_ALL = 0, td_pc_ALL = 0, td_count_ALL = 0;
static double balancer;
static double SCALER;
static int sram_ready = 0;




/* Ugly indeed, but it seems some architecture/OS does not support "standard"
   aligned allocations or give strange error codes ... Note: this one only
   works, if you don't want to free() the result pointer!! */
void *alloc_xep_aligned_mem ( size_t size )
{
	int o;
	Uint8 *p = malloc(size + 15);
	if (p == NULL)
		return p;
	o = 16 - ((int)p & 0xF);
	DEBUG("ALIGNED-ALLOC: base_pointer=%p o=%d" NL, p, o);
	return p + o;
}



static void shutdown_sdl(void)
{
	if (guarded_exit) {
		audio_close();
		printer_close();
#ifdef CONFIG_W5300_SUPPORT
		w5300_shutdown();
#endif
		if (sram_ready)
			sram_save_all_segments();
		DEBUGPRINT("Shutdown callback, return." NL);
	}
	if (sdl_win)
		SDL_DestroyWindow(sdl_win);
	console_close_window_on_exit();
	/* last stuff! */
	if (debug_file) {
		DEBUGPRINT("Closing debug messages log file on exit." NL);
		fclose(debug_file);
		debug_file = NULL;
	}
	SDL_Quit();
}



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
	DEBUG("DELAY: pc=%d em=%d sleep=%d" NL, td_pc, td_em, td);
	/* for reporting only: BEGIN */
	td_em_ALL += td_em;
	td_pc_ALL += td_pc;
	if (td_count_ALL == 50) {
		char buf[256];
		//DEBUG("STAT: count = %d, EM = %d, PC = %d, usage = %f%" NL, td_count_ALL, td_em_ALL, td_pc_ALL, 100.0 * (double)td_pc_ALL / (double)td_em_ALL);
		snprintf(buf, sizeof buf, "%s [%.2fMHz ~ %d%%]%s", WINDOW_TITLE " v" VERSION " ",
			CPU_CLOCK / 1000000.0,
			td_pc_ALL * 100 / td_em_ALL,
			paused ? " PAUSED" : ""
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
	DEBUG("Really slept = %d" NL, td);
	if (td < 0) return; // invalid, sleep was about for _minus_ time? eh, give me that time machine, dude! :)
	td_balancer -= td;
	if (td_balancer >  1000000)
		td_balancer = 0;
	else if (td_balancer < -1000000)
		td_balancer = 0;
	DEBUG("Balancer = %lf" NL, td_balancer);
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
}



// called by nick.c
void emu_one_frame(int rasters, int frameskip)
{
	SDL_Event e;
	if (!frameskip)
		screen_present_frame(ep_pixels);
	while (SDL_PollEvent(&e) != 0)
		switch (e.type) {
			case SDL_WINDOWEVENT:
				if (!is_fullscreen && e.window.event == SDL_WINDOWEVENT_RESIZED) {
					DEBUG("UI: Window is resized to %d x %d" NL,
						e.window.data1,
						e.window.data2
					);
					screen_window_resized(e.window.data1, e.window.data2);
				}
				break;
			case SDL_QUIT:
				if (QUESTION_WINDOW("?No|!Yes", "Are you sure to exit?") == 1)
					exit(0);
				return;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (e.key.repeat == 0 && (e.key.windowID == sdl_winid || e.key.windowID == 0)) {
					int code = emu_kbd(e.key.keysym, e.key.state == SDL_PRESSED);
					if (code == 0xF9)		// // OSD REPLAY, default key GRAVE
						osd_replay(e.key.state == SDL_PRESSED ? 0 : OSD_FADE_START);
					else if (code && e.key.state == SDL_PRESSED)
						switch(code) {
							case 0xFF:	// FULLSCREEN toogle, default key F11
								screen_set_fullscreen(!is_fullscreen);
								break;
							case 0xFE:	// EXIT, default key F9
								if (QUESTION_WINDOW("?No|!Yes", "Are you sure to exit?") == 1)
									exit(0);
								break;
							case 0xFD:	// SCREENSHOT, default key F10
								screen_shot(ep_pixels, current_directory, "screenshot-*.png");
								break;
							case 0xFC:	// RESET, default key PAUSE
								if (e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
									zxemu_on = 0;
									(void)ep_init_ram();
								}
								ep_reset();
								break;
							case 0xFB:	// DOWNGRADE CPU SPEED, default key PAGE DOWN
								if (_cpu_speed_index)
									set_cpu_clock_with_osd(_cpu_speeds[-- _cpu_speed_index]);
								break;
							case 0xFA:	// UPGRADE CPU SPEED, default key PAGE UP
								if (_cpu_speed_index < 3)
									set_cpu_clock_with_osd(_cpu_speeds[++ _cpu_speed_index]);
								break;
							case 0xF8:	// CONSOLE, key pad minus
								if (!console_is_open)
									console_open_window();
								break;
						}
				} else if (e.key.repeat == 0)
					DEBUG("UI: NOT HANDLED KEY EVENT: repeat = %d windowid = %d [our win = %d]" NL, e.key.repeat, e.key.windowID, sdl_winid);
				break;
			case SDL_MOUSEMOTION:
				if (e.motion.windowID == sdl_winid)
					emu_mouse_motion(e.motion.xrel, e.motion.yrel);
				break;
			case SDL_MOUSEWHEEL:
				if (e.wheel.windowID == sdl_winid)
					emu_mouse_wheel(
						e.wheel.x, e.wheel.y,
#if SDL_VERSION_ATLEAST(2, 0, 4)
						sdl_v204 ? e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED : 0
#else
						0
#endif
					);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (e.button.windowID == sdl_winid)
					emu_mouse_button(e.button.button, e.button.state == SDL_PRESSED);
				break;
			default:
				joy_sdl_event(&e);
				break;
		}
	monitor_process_queued();
	rtc_update_trigger = 1; // triggers RTC update on the next RTC register read. Woooo!
	emu_timekeeping_delay(1000000.0 * rasters * 57.0 / (double)NICK_SLOTS_PER_SEC);
}



int set_cpu_clock ( int hz )
{
	if (hz <  1000000) hz =  1000000;
	if (hz > 12000000) hz = 12000000;
	CPU_CLOCK = hz;
	SCALER = (double)NICK_SLOTS_PER_SEC / (double)CPU_CLOCK;
	DEBUG("CPU: clock = %d scaler = %f" NL, CPU_CLOCK, SCALER);
	dave_set_clock();
	return hz;
}



int set_cpu_clock_with_osd ( int hz )
{
	hz = set_cpu_clock(hz);
	OSD("CPU speed: %.2f MHz", hz / 1000000.0);
	return hz;
}




int main (int argc, char *argv[])
{
	atexit(shutdown_sdl);
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		ERROR_WINDOW("Fatal SDL initialization problem: %s", SDL_GetError());
		return 1;
	}
	if (config_init(argc, argv))
		return 1;
	guarded_exit = 1;	// turn on guarded exit, with custom de-init stuffs
	fileio_init(app_pref_path, "files");
	if (screen_init())
		return 1;
	audio_init(config_getopt_int("audio"));
	z80ex_init();
	set_ep_cpu(CPU_Z80);
	ep_pixels = nick_init();
	if (ep_pixels == NULL)
		return 1;
	if (roms_load() <= 0)
		return 1;
	primo_rom_seg = primo_search_rom();
	ep_set_ram_config(config_getopt_str("ram"));
	mouse_setup(config_getopt_int("mousemode"));
	ep_reset();
	kbd_matrix_reset();
	joy_sdl_event(NULL); // this simply inits joy layer ...
#ifdef CONFIG_SDEXT_SUPPORT
	sdext_init();
#endif
#ifdef CONFIG_W5300_SUPPORT
	w5300_init(NULL);
#endif
	ticks = SDL_GetTicks();
	balancer = 0;
	set_cpu_clock(DEFAULT_CPU_CLOCK);
	emu_timekeeping_start();
	audio_start();
	if (config_getopt_str("fullscreen"))
		screen_set_fullscreen(1);
	DEBUGPRINT(NL "EMU: entering into main emulation loop" NL);
	sram_ready = 1;
	if (strcmp(config_getopt_str("primo"), "none")) {
		// TODO: da stuff ...
		primo_emulator_execute();
		OSD("Primo Emulator Mode");
	}
	console_monitor_ready();	// OK to run monitor on console now!
	/* Main loop of the emulator, an infinite one :) Use exit() to exit, as atexit() is
	used to register proper "cleaner" function, including calling SDL_Quit() as well
	This is also an optimization that it's an unconditional, infinite loop and no test is needed eg for 'exit' ... */
	for (;;) {
		int t;
		// This needs somewhat optimazed solution, only a single, simple "if" to test, and all more complex stuff inside!
		// So the creative usage of goto and continue can be seen here as well :)
		// This is because in case of not-paused mode the "price" should be minimal at every opcodes, maybe just three machine code ops on x86 ...
		// TODO this stuff should be called "request" mode not only for pause, but single stepping, well-defined snapshot save place (no-in opcode stuff), etc ...
		if (paused) {
			if (z80ex.prefix)
				goto processing;	// do not allow in-opcode pause for the emulator! I *always* wanted to use GOTO, I am so happy now :D
			/* Paused is non-zero for special cases, like pausing emulator :) or single-step execution mode */
			emu_one_frame(312, 0); // keep UI stuffs (and some timing) intact ... with a faked about 312 scanline (normal frame) timing needed ...
			continue; // but do not emulate EP related stuff ...
		}
	processing:
		if (nmi_pending) {
			t = z80ex_nmi();
			DEBUG("NMI: %d" NL, t);
			if (t)
				nmi_pending = 0;
		} else
			t = 0;
		if ((t == 0) && (dave_int_read & 0xAA)) {
			t = z80ex_int();
			if (t)
				DEBUG("CPU: int and accepted = %d" NL, t);
		} else
			t = 0;
		if (!t)
			t = z80ex_step();
		cpu_cycles_for_dave_sync += t;
		//DEBUG("DAVE: SYNC: CPU cycles = %d, Dave sync val = %d, limit = %d" NL, t, cpu_cycles_for_dave_sync, cpu_cycles_per_dave_tick);
		while (cpu_cycles_for_dave_sync >= cpu_cycles_per_dave_tick) {
			dave_tick();
			cpu_cycles_for_dave_sync -= cpu_cycles_per_dave_tick;
		}
		balancer += t * SCALER;
		//DEBUG("%s [balance=%f t=%d]" NL, buffer, balancer, t);
		while (balancer >= 0.5) {
			nick_render_slot();
			balancer -= 1.0;
		}
		//DEBUG("[balance=%f t=%d]" NL, balancer, t);
	}
	return 0;
}

