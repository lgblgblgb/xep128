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

SDL_Window *sdl_win = NULL;
static SDL_Surface *sdl_surf;
static Uint32 winid;

int rom_size;


void emu_win_grab ( SDL_bool state )
{
	printf("GRAB: %d\n", state);
	//SDL_SetWindowGrab(sdl_win, state);
	SDL_SetRelativeMouseMode(state);
	SDL_SetWindowGrab(sdl_win, state);
}


static void exit_on_SDL_problem(const char *msg)
{
        ERROR_WINDOW("SDL: PROBLEM: %s: %s\n", msg, SDL_GetError());
        exit(1);
}


static void shutdown_sdl(void)
{
 //       if (sdl_win) {
   //             printf("SDL: quit\n");
                //SDL_Quit();
     //   }
	//SQL_Quit();
	printer_close();
        printf("Shutdown callback, return.\n");
}


static int load_roms ( void )
{
	FILE *f;
	int ret;
	printf("ROM: loading %s\n", COMBINED_ROM_PATH);
	f = fopen(COMBINED_ROM_PATH, "rb");
	if (f == NULL) {
		ERROR_WINDOW("Cannot open ROM image \"%s\": %s", COMBINED_ROM_PATH, ERRSTR());
		return -1;
	}
	ret = fread(memory, 1, 0x100001, f);
	printf("Read = %d\n", ret);
	fclose(f);
	if (ret < 0) {
		ERROR_WINDOW("Cannot read ROM image \"%s\": %s", COMBINED_ROM_PATH, ERRSTR());
	}
	if (ret < 0x8000) {
		ERROR_WINDOW("ROM image \"%s\" is too short.", COMBINED_ROM_PATH);
		return -1;
	}
	if (ret == 0x100001) {
		ERROR_WINDOW("ROM image \"%s\" is too large.", COMBINED_ROM_PATH);
		return -1;
	}
	if (ret & 0x3FFF) {
		ERROR_WINDOW("ROM image \"%s\" size is not multiple of 0x4000 bytes.", COMBINED_ROM_PATH);
		return -1;
	}
	return ret;
}



static unsigned int ticks;
static int running;



static void deinterlacer ( SDL_Surface *surf )
{
	Uint32 *p = surf->pixels;
	Uint32 *p_end = p + surf->w * surf->h;
	while (p < p_end) {
		memcpy(p + surf->w, p, surf->w * 4);
		p += surf->w * 2;
	}
}



static int tstates_all = 0;

static double td_balancer = 0;

//time_t unix_time;


static struct timeval tv_old;

/* This is the emulation timing stuff
 * Should be called at the END of the emulation loop.
 * Input parameter: microseconds needed for the "real" (emulated) computer to do our loop 
 * This function also does the sleep itself */
void emu_timekeeping_delay ( int td_em )
{
	int td, td_pc;
	struct timeval tv_new;
	gettimeofday(&tv_new, NULL);
	td_pc = (tv_new.tv_sec - tv_old.tv_sec) * 1000000 + (tv_new.tv_usec - tv_old.tv_usec); // get realtime since last call in microseconds
	if (td_pc < 0) return; // time goes backwards? maybe time was modified on the host computer. Skip this delay cycle
	//td_ep = 1000000 * rasters * 57 / NICK_SLOTS_PER_SEC; // microseconds would need for an EP128 to do this
	td = td_em - td_pc; // the time difference (+X = PC is faster - real time EP emulation, -X = EP is faster - real time EP emulation is not possible)
	printf("DELAY: pc=%d em=%d sleep=%d\n", td_pc, td_em, td);
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
	//emu_gui_iterations();
	//int ticks_new, used;
	SDL_Event e;
	deinterlacer(sdl_surf);
	SDL_UpdateWindowSurface(sdl_win); // update window content
	while (SDL_PollEvent(&e) != 0)
		switch (e.type) {
			case SDL_QUIT: {
				FILE *f = fopen("/tmp/vram", "wb");
				fwrite(memory + 0x3F0000, 1, 0x10000, f);
				fclose(f);
				running = 0;
				SDL_Log("Szivacs van!");
				}
				return;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (e.key.repeat == 0 && (e.key.windowID == winid || e.key.windowID == 0)) {
					if (e.key.keysym.scancode == SDL_SCANCODE_F11 && e.key.state == SDL_PRESSED)
						fprintf(stderr,"TODO: TOOGLE FULLSCREEN!\n");
					else if (e.key.keysym.scancode == SDL_SCANCODE_PAUSE && e.key.state == SDL_PRESSED) {
						if (shift_pressed) ep_clear_ram();
						ep_reset();
					} else
						emu_kbd(e.key.keysym, e.key.state == SDL_PRESSED);
				} else
					fprintf(stderr, "NOT HANDLED KEY EVENT: repeat = %d windowid = %d [our win = %d]\n", e.key.repeat, e.key.windowID, winid);
				break;
			case SDL_MOUSEMOTION:
				if (e.button.windowID == winid)
					emu_mouse_motion(e.motion.xrel, e.motion.yrel);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (e.button.windowID == winid)
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
static double SCALER = (double)NICK_SLOTS_PER_SEC / (double)CPU_CLOCK; // 0.222462 for 4MHz Z80 [?]



//emu_define_keyboard(10, 8, kbd_press, kbd_release, []);

int main (int argc, char *argv[]) {
	atexit(shutdown_sdl);
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) exit_on_SDL_problem("initialization problem");
	sdl_win = SDL_CreateWindow(
                WINDOW_TITLE " " VERSION,
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                736, 288*2,
                SDL_WINDOW_SHOWN // | SDL_WINDOW_FULLSCREEN_DESKTOP
        );
        if (!sdl_win) exit_on_SDL_problem("cannot open window");
        winid = SDL_GetWindowID(sdl_win);
	sdl_surf = SDL_GetWindowSurface(sdl_win);
	if (!sdl_surf) exit_on_SDL_problem("cannot get window surface");
	if (z80_reset()) return 1;
	if (!nick_init(sdl_surf)) return 1;
	SDL_FillRect(sdl_surf, NULL, 0);
	SDL_UpdateWindowSurface(sdl_win);
	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Xep Window", "This is only a test dialog window for Xep128. Click on OK to continue :)", sdl_win);
	//ERROR_WINDOW("Ez itt a %s a valasz %d\nHolla!","szep",42);
	//if (z80_reset()) return 1;
	rom_size = load_roms();
	if (rom_size <= 0) return 1;
	if (search_xep_rom() == -1) ERROR_WINDOW("Cannot find XEP EXOS ROM signature inside ROM image \"%s\". You can use Xep128 but internal :XEP access won't work!", COMBINED_ROM_PATH);
	set_ep_ramsize(1024);
	ep_reset();
#ifdef CONFIG_SDEXT_SUPPORT
	sdext_init();
#endif
	ticks = SDL_GetTicks();
	running = 1;
	balancer = 0;
	int last_optype = 0;
	printf("CPU: clock = %d scaler = %f\n", CPU_CLOCK, SCALER);
	emu_timekeeping_start();
	while (running) {
		int t, nts = 0;
#if 0
		char buffer[256];
		int pc = z80ex_get_reg(z80, regPC);
		////printf("PC=%04Xh\n", z80ex_get_reg(z80, regPC));
		//z80ex_dasm(buffer, sizeof(buffer) - 1, 0, &t_states, &t_states2, pc);
		//printf("%04X %s\n", pc, buffer);
		z80_dasm(buffer, pc, -1);
#endif
		if (nmi_pending) {
			t = z80ex_nmi(z80);
			fprintf(stderr, "NMI: %d\n", t);
			if (t)
				nmi_pending = 0;
		} else
			t = 0;
		if ((t == 0) && (dave_int_read & 0xAA)) {
			t = z80ex_int(z80);
			if (t)
				printf("CPU: int and accepted = %d\n", t);
		} else
			t = 0;
		if (!t)
			t = z80ex_step(z80);
		balancer += t * SCALER;
		tstates_all += t;
		//printf("%s [balance=%f t=%d]\n", buffer, balancer, t);
		while (balancer >= 0.5) {
			nick_render_slot();
			balancer -= 1.0;
			nts++;
		}
		dave_ticks(nts);
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

