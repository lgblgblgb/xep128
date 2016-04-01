#include "xepem.h"

#define MAX_JOYSTICKS 10

static struct {
	SDL_Joystick *joy;
	int sdl_index;
	int	num_of_buttons;
	int	num_of_axes;
	Uint32	button;
	Uint32	axisp;
	Uint32	axisn;
	const char *name;
	SDL_JoystickGUID guid;
	char guid_str[40];
	Uint32 button_mask;
	Uint32 axis_mask;
} joysticks[MAX_JOYSTICKS];
static int joy_to_init = 1;
static const char unknown_joystick_name[] = "Unknown joystick";


#define BIT_MASK(n) (1U << ((Uint32)(n)))
#define IS_BIT_SET(storage,index) ((storage) & MASK(index))
#define BIT_TO_SET(storage,index) (storage) |= BIT_MASK(index)
#define BIT_TO_RESET(storage,index) (storage) &= ~BIT_MASK(index)


#define AXIS_LIMIT_HIGH	20000
#define AXIS_LIMIT_LOW	13000




static void set_axis_state ( int joy_id, Uint32 axis, Sint16 value )
{
	if (joy_id >= MAX_JOYSTICKS || !joysticks[joy_id].joy || axis >= joysticks[joy_id].num_of_axes)
		return;
	axis = 1 << axis;
	if (value > AXIS_LIMIT_HIGH) {
		BIT_TO_SET(joysticks[joy_id].axisp, axis);
		BIT_TO_RESET(joysticks[joy_id].axisn, axis);
	} else if (value < AXIS_LIMIT_LOW && value > -AXIS_LIMIT_LOW) {
		BIT_TO_RESET(joysticks[joy_id].axisp, axis);
		BIT_TO_RESET(joysticks[joy_id].axisn, axis);
	} else if (value < -AXIS_LIMIT_HIGH) {
		BIT_TO_RESET(joysticks[joy_id].axisp, axis);
		BIT_TO_SET(joysticks[joy_id].axisn, axis);
	}
}



static void set_button_state ( int joy_id, Uint32 button, int value )
{
	if (joy_id >= MAX_JOYSTICKS || !joysticks[joy_id].joy || button >= joysticks[joy_id].num_of_buttons)
		return;
	button = 1 << button;
	if (value)
		BIT_TO_SET(joysticks[joy_id].button, button);
	else
		BIT_TO_RESET(joysticks[joy_id].button, button);
}



static void joy_sync ( int joy_id )
{
	int a;
	if (joy_id >= MAX_JOYSTICKS || !joysticks[joy_id].joy)
		return;
	joysticks[joy_id].button = joysticks[joy_id].axisp = joysticks[joy_id].axisn = 0;
	for (a = 0; a < 32; a++) {
		if (a < joysticks[joy_id].num_of_axes)
			set_axis_state(joy_id, a, SDL_JoystickGetAxis(joysticks[joy_id].joy, a));
		if (a < joysticks[joy_id].num_of_buttons)
			set_button_state(joy_id, a, SDL_JoystickGetButton(joysticks[joy_id].joy, a));
	}
}



static void joy_detach ( int joy_id )
{
	if (joy_id >= MAX_JOYSTICKS || !joysticks[joy_id].joy)
		return;
	DEBUG("JOY: device removed #%d" NL,
		joy_id
	);
	SDL_JoystickClose(joysticks[joy_id].joy);
	joysticks[joy_id].joy = NULL;
	OSD("Joy detached #%d", joy_id);
}



static void joy_attach ( int joy_id )
{
	SDL_Joystick *joy;
	if (joy_id >= MAX_JOYSTICKS)
		return;
	if (joysticks[joy_id].joy)	// already attached joystick?
		return;
	joysticks[joy_id].joy = joy = SDL_JoystickOpen(joy_id);
	if (!joy)
		return;
	joysticks[joy_id].sdl_index = joy_id;
	joysticks[joy_id].num_of_buttons = SDL_JoystickNumButtons(joy);
	joysticks[joy_id].num_of_axes =  SDL_JoystickNumAxes(joy);
	joysticks[joy_id].name = SDL_JoystickName(joy);
	if (!joysticks[joy_id].name)
		joysticks[joy_id].name = unknown_joystick_name;
	joysticks[joy_id].guid = SDL_JoystickGetGUID(joy);
	SDL_JoystickGetGUIDString(
		joysticks[joy_id].guid,
		joysticks[joy_id].guid_str,
		40
	);
	if (joysticks[joy_id].num_of_buttons > 32)
		joysticks[joy_id].num_of_buttons = 32;
	if (joysticks[joy_id].num_of_axes > 32)
		joysticks[joy_id].num_of_axes = 32;
	joy_sync(joy_id);
	DEBUG("JOY: new device added #%d \"%s\" axes=%d buttons=%d balls=%d hats=%d guid=%s" NL,
		joy_id,
		joysticks[joy_id].name,
		joysticks[joy_id].num_of_axes,
		joysticks[joy_id].num_of_buttons,
		SDL_JoystickNumBalls(joy),
		SDL_JoystickNumHats(joy),
		joysticks[joy_id].guid_str
	);
	OSD("Joy attached:\n#%d %s", joy_id, joysticks[joy_id].name);
}



void joy_sdl_event ( SDL_Event *e )
{
	if (joy_to_init) {
		int a;
		for (a = 0; a < MAX_JOYSTICKS; a++) {
			joysticks[a].joy = NULL;
			joysticks[a].button = joysticks[a].axisp = joysticks[a].axisn = 0;
		}
		SDL_GameControllerEventState(SDL_DISABLE);
		SDL_JoystickEventState(SDL_ENABLE);
		joy_to_init = 0;
	}
	if (e) switch (e->type) {
		case SDL_JOYAXISMOTION:	// joystick axis motion
			set_axis_state(e->jaxis.which, e->jaxis.axis, e->jaxis.value);
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			set_button_state(e->jbutton.which, e->jbutton.button, e->jbutton.state);
			break;
		case SDL_JOYDEVICEADDED:
			joy_attach(e->jdevice.which);
			break;
		case SDL_JOYDEVICEREMOVED:
			joy_detach(e->jdevice.which);
			break;
	}
}



/* The SCAN function called from input.c
   num is the Enterprise (!) external joystick number, 0 or 1 (beware, on EP it's called 1 or 2)
   dir is the direction / button to scan, see JOY_SCAN_* defines in the header file */
int joystick_scan ( int num, int dir )
{
	return !(kbd_matrix[10] & (1 << dir));  // keyboard-matrix row #10 (not a real EP one!) is used to maintain status of numeric keypad joy emu keys ...
}
