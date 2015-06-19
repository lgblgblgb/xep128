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
#include "lodepng.h"

int is_fullscreen = 0;
SDL_Window   *sdl_win = NULL;
static SDL_Renderer *sdl_ren = NULL;
static SDL_Texture  *sdl_tex = NULL;
static int warn_for_mouse_grab = 1;
Uint32 sdl_winid;
static int win_xsize, win_ysize;



#include "app_icon.c"


void screen_grab ( SDL_bool state )
{
	if (warn_for_mouse_grab) {
		INFO_WINDOW("Clicking in emulator window causes to enter BoxSoft mouse emulation mode.\nThis will try to grab your mouse pointer. To exit, press key ESC.\nYou won't get this notice next time within this session of Xep128");
		warn_for_mouse_grab = 0;
	}
	printf("GRAB: %d\n", state);
	SDL_SetRelativeMouseMode(state);
	SDL_SetWindowGrab(sdl_win, state);
}


void screen_window_resized ( void )
{
	SDL_GetWindowSize(sdl_win, &win_xsize, &win_ysize);
	fprintf(stderr, "WINDOW: query window geometry: %d x %d\n", win_xsize, win_ysize);
}


void screen_set_fullscreen ( int state )
{
	if (is_fullscreen == state) return;
	is_fullscreen = state;
	if (state) {
		screen_window_resized();
		SDL_SetWindowFullscreen(sdl_win, SDL_WINDOW_FULLSCREEN_DESKTOP);
		SDL_RaiseWindow(sdl_win);
		fprintf(stderr, "UI: entering full screen mode\n");
	} else {
		SDL_SetWindowFullscreen(sdl_win, 0);
		SDL_SetWindowSize(sdl_win, win_xsize, win_ysize); // see the comment below
		SDL_RaiseWindow(sdl_win);
		fprintf(stderr, "UI: leaving full screen mode\n");
	}
}


void screen_present_frame (Uint32 *ep_pixels)
{
	SDL_UpdateTexture(sdl_tex, NULL, ep_pixels, SCREEN_WIDTH * sizeof (Uint32));
	SDL_RenderClear(sdl_ren);
	SDL_RenderCopy(sdl_ren, sdl_tex, NULL, NULL);
#if 0
	if (osd_on && sdl_osdtex != NULL)
		SDL_RenderCopy(sdl_ren, sdl_osdtex, &screenrect, &osdrect);
	else
		osd_fade = 0;
#endif
	SDL_RenderPresent(sdl_ren);
#if 0
	if (osd_fade > 0) {	// OSD / fade mode
		osd_fade--;
		if (osd_fade > 0) {
			_osd_set_alpha(osd_fade);
			int a;
			for (a = 0; a < SCREEN_WIDTH * SCREEN_HEIGHT * 4; a += 4)
				osd_pixels[a] = osd_fade;
		} else
			osd_on = 0; // faded off, switch OSD texture rendering off
	}
#endif
}



int screen_shot ( Uint32 *ep_pixels )
{
	Uint8 *pix = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 2 * 3);
	int a;
	if (pix == NULL) {
		ERROR_WINDOW("Not enough memory for taking a screenshot :(");
		return 1;
	}
	for (a = 0; a < SCREEN_HEIGHT * SCREEN_WIDTH; a++) {
		int d = (a / SCREEN_WIDTH) * SCREEN_WIDTH * 6 + (a % SCREEN_WIDTH) * 3;
		pix[d + 0] = pix[d + 0 + SCREEN_WIDTH * 3] = (ep_pixels[a] >> 16) & 0xFF;
		pix[d + 1] = pix[d + 1 + SCREEN_WIDTH * 3] = (ep_pixels[a] >> 8) & 0xFF;
		pix[d + 2] = pix[d + 2 + SCREEN_WIDTH * 3] = ep_pixels[a] & 0xFF;
	}
	if (lodepng_encode24_file("screenshot.png", (unsigned char*)pix, SCREEN_WIDTH, SCREEN_HEIGHT * 2)) {
		free(pix);
		ERROR_WINDOW("LodePNG screenshot taking error");
		return 0;
	} else {
		free(pix);
		INFO_WINDOW("Screenshot has been saved.");
		return 1;
	}
}


static void set_app_icon ( SDL_Window *win, const void *app_icon )
{
	SDL_Surface *surf = SDL_CreateRGBSurfaceFrom((void*)app_icon,96,96,32,96*4,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
	if (surf == NULL)
		fprintf(stderr, "Cannot create surface for window icon: %s\n", SDL_GetError());
	else {
		SDL_SetWindowIcon(win, surf);
		SDL_FreeSurface(surf);
	}
}


int screen_init ( void )
{
	sdl_win = SDL_CreateWindow(
                WINDOW_TITLE " " VERSION,
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                SCREEN_WIDTH, SCREEN_HEIGHT * 2,
                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        if (!sdl_win) {
		ERROR_WINDOW("cannot open window");
		return 1;
	}
	SDL_SetWindowMinimumSize(sdl_win, SCREEN_WIDTH, SCREEN_HEIGHT * 2);
	screen_window_resized();
	set_app_icon(sdl_win, _icon_pixels);
	sdl_ren = SDL_CreateRenderer(sdl_win, -1, 0);
	if (sdl_ren == NULL) {
		ERROR_WINDOW("Cannot create SDL renderer: %s", SDL_GetError());
		return 1;
	}
	SDL_RenderSetLogicalSize(sdl_ren, SCREEN_WIDTH, SCREEN_HEIGHT * 2);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0"); // disable vsync
	sdl_tex = SDL_CreateTexture(sdl_ren, SCREEN_FORMAT, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (sdl_tex == NULL) {
		ERROR_WINDOW("Cannot create SDL texture: %s", SDL_GetError());
		return 1;
	}
        sdl_winid = SDL_GetWindowID(sdl_win);
	fprintf(stderr, "SDL: everything seems to be OK ...\n");
	return 0;
}

