#include "xepem.h"

#define MAX_JOYSTICKS 10

#define EPJOY_DISABLED	-2
#define EPJOY_KP	-1

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
static struct {
	int	id;	// Joystick ID (in joysticks array), use -1 for numeric keypad, or -2 for disable!, for vals < 0, all other options here ARE NOT used at all!
	Uint32	button_masks[3];
	Uint32	v_axis_mask, h_axis_mask;
} epexjoys[3];	// emulation details for the two external joysticks of Enterprise (note: we have 0,1 on EP they're named as 1,2!) index 2 is for internal joystick emu ...
static int joy_to_init = 1;
static const char unknown_joystick_name[] = "Unknown joystick";



#define BIT_TO_SET(storage,mask) (storage) |= mask
#define BIT_TO_RESET(storage,mask) (storage) &= (4294967295U - mask)


#define AXIS_LIMIT_HIGH	20000
#define AXIS_LIMIT_LOW	10000




static void set_axis_state ( int joy_id, Uint32 axis, int value )
{
	Uint32 axis_mask, oldp, oldn;
	if (joy_id >= MAX_JOYSTICKS || !joysticks[joy_id].joy || axis >= joysticks[joy_id].num_of_axes)
		return;
	axis_mask = 1U << axis;
	oldp = joysticks[joy_id].axisp; 
	oldn = joysticks[joy_id].axisn;
	if (value > AXIS_LIMIT_HIGH) {
		BIT_TO_SET  (joysticks[joy_id].axisp, axis_mask);
		BIT_TO_RESET(joysticks[joy_id].axisn, axis_mask);
	} else if (value < AXIS_LIMIT_LOW && value > -AXIS_LIMIT_LOW) {
		BIT_TO_RESET(joysticks[joy_id].axisp, axis_mask);
		BIT_TO_RESET(joysticks[joy_id].axisn, axis_mask);
	} else if (value < -AXIS_LIMIT_HIGH) {
		BIT_TO_RESET(joysticks[joy_id].axisp, axis_mask);
		BIT_TO_SET  (joysticks[joy_id].axisn, axis_mask);
	}
	if (show_keys && (oldp != joysticks[joy_id].axisp || oldn != joysticks[joy_id].axisn))
		OSD("PC joystick #%d axis %d [%c %c]\nmask=%X value=%d",
			joy_id,
			axis,
			(joysticks[joy_id].axisn & axis_mask) ? '-' : ' ',
			(joysticks[joy_id].axisp & axis_mask) ? '+' : ' ',
			axis_mask,
			value
	);
}



static void set_button_state ( int joy_id, Uint32 button, int value )
{
	Uint32 button_mask, old;
	if (joy_id >= MAX_JOYSTICKS || !joysticks[joy_id].joy || button >= joysticks[joy_id].num_of_buttons)
		return;
	button_mask = 1U << button;
	old = joysticks[joy_id].button;
	if (value)
		BIT_TO_SET(joysticks[joy_id].button, button_mask);
	else
		BIT_TO_RESET(joysticks[joy_id].button, button_mask);
	if (show_keys && old != joysticks[joy_id].button)
		OSD("PC joystick #%d button %d %s", joy_id, button, value ? "pressed" : "released");
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
	joysticks[joy_id].button = joysticks[joy_id].axisp = joysticks[joy_id].axisn = 0;	// don't leave state in a "stucked" situation!
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
		for (a = 0; a < 2; a++) {
			epexjoys[a].id = 0;	// assigned to SDL joystick #0
			epexjoys[a].button_masks[0] = 1;
			epexjoys[a].button_masks[1] = 0;
			epexjoys[a].button_masks[2] = 0;
			epexjoys[a].v_axis_mask = 2;
			epexjoys[a].h_axis_mask = 1;
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
   dir is the direction / button to scan, see JOY_SCAN_* defines in the header file.
   The return value is '0' for inactive and '1' for active (well, or any non-zero value) for a given scan, warning, it's the
   opposite behaviour as you can see on EP HW level!
   DO NOT call this function with other num than 0, 1! (2 is reserved for internal joy emu, but it's not handled by this func!)
*/
int joystick_scan ( int num, int dir )
{
	if (num != 0 && num != 1)
		return 0;
	switch (epexjoys[num].id) {
		case EPJOY_KP:
			return !(kbd_matrix[10] & (1 << dir));  // keyboard-matrix row #10 (not a real EP one!) is used to maintain status of numeric keypad joy emu keys ...
		case EPJOY_DISABLED:
			return 0;		// disabled, always zero answer
		default:
			// hack: give priority to the numeric keypad ;-)
			if (!(kbd_matrix[10] & (1 << dir)))
				return 1;
			switch (dir) {
				case JOY_SCAN_FIRE1:
					return joysticks[epexjoys[num].id].button & epexjoys[num].button_masks[0];
				case JOY_SCAN_UP:
					return joysticks[epexjoys[num].id].axisn  & epexjoys[num].v_axis_mask;
				case JOY_SCAN_DOWN:
					return joysticks[epexjoys[num].id].axisp  & epexjoys[num].v_axis_mask;
				case JOY_SCAN_LEFT:
					return joysticks[epexjoys[num].id].axisn  & epexjoys[num].h_axis_mask;
				case JOY_SCAN_RIGHT:
					return joysticks[epexjoys[num].id].axisp  & epexjoys[num].h_axis_mask;
				case JOY_SCAN_FIRE2:
					return joysticks[epexjoys[num].id].button & epexjoys[num].button_masks[1];
				case JOY_SCAN_FIRE3:
					return joysticks[epexjoys[num].id].button & epexjoys[num].button_masks[2];
			}
	}
	return 0;	// unknown?!
}
