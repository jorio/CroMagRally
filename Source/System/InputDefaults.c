#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include "input.h"

#if __APPLE__
	#define SC_THROWF1 SDL_SCANCODE_LGUI
	#define SC_THROWF2 SDL_SCANCODE_RGUI
	#define SC_THROWB1 SDL_SCANCODE_LALT
	#define SC_THROWB2 SDL_SCANCODE_RALT
#else
	#define SC_THROWF1 SDL_SCANCODE_LCTRL
	#define SC_THROWF2 SDL_SCANCODE_RCTRL
	#define SC_THROWB1 SDL_SCANCODE_LALT
	#define SC_THROWB2 SDL_SCANCODE_RALT
#endif

// Gamepad button/axis
#define GB(x)		{kInputTypeButton, SDL_GAMEPAD_BUTTON_##x}
#define GAPLUS(x)	{kInputTypeAxisPlus, SDL_GAMEPAD_AXIS_##x}
#define GAMINUS(x)	{kInputTypeAxisMinus, SDL_GAMEPAD_AXIS_##x}
#define GBNULL()	{kInputTypeUnbound, 0}

const InputBinding kDefaultInputBindings[NUM_CONTROL_NEEDS] =
{
	[kNeed_ThrowForward] =
	{
		.key = { SC_THROWF1, SC_THROWF2 },
		.pad = { GB(NORTH), GAMINUS(RIGHTY) },
	},

	[kNeed_ThrowBackward] =
	{
		.key = { SC_THROWB1, SC_THROWB2 },
		.pad = { GB(WEST), GAPLUS(RIGHTY) },
	},

	[kNeed_Brakes] =
	{
		.key = { SDL_SCANCODE_SPACE },
		.pad = { GAPLUS(RIGHT_TRIGGER) },
	},

	[kNeed_CameraMode] =
	{
		.key = { SDL_SCANCODE_GRAVE, SDL_SCANCODE_RETURN },
		.pad = { GB(LEFT_SHOULDER) },
	},

	[kNeed_RearView] =
	{
		.key = { SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT },
		.pad = { GAPLUS(LEFT_TRIGGER) },
	},

	[kNeed_Forward] =
	{
		.key = { SDL_SCANCODE_UP, SDL_SCANCODE_W },
		.pad = { GB(SOUTH) },
	},

	[kNeed_Backward] =
	{
		.key = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		.pad = { GB(EAST) },
	},

	[kNeed_Left] =
	{
		.key = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A },
		.pad = { GAMINUS(LEFTX) },
	},

	[kNeed_Right] =
	{
		.key = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		.pad = { GAPLUS(LEFTX) },
	},

	// -----------------------------------------------------------
	// Non-remappable UI bindings below

	[kNeed_UIUp] =
	{
		.key = { SDL_SCANCODE_UP, SDL_SCANCODE_W },
		.pad = { GB(DPAD_UP), GAMINUS(LEFTY) },
	},

	[kNeed_UIDown] =
	{
		.key = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		.pad = { GB(DPAD_DOWN), GAPLUS(LEFTY) },
	},

	[kNeed_UILeft] =
	{
		.key = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A },
		.pad = { GB(DPAD_LEFT), GAMINUS(LEFTX) },
	},

	[kNeed_UIRight] =
	{
		.key = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		.pad = { GB(DPAD_RIGHT), GAPLUS(LEFTX) },
	},

	[kNeed_UIPrev] =
	{
		.pad = { GB(LEFT_SHOULDER) },
	},

	[kNeed_UINext] =
	{
		.pad = { GB(RIGHT_SHOULDER) },
	},

	[kNeed_UIConfirm] =
	{
		.key = { SDL_SCANCODE_RETURN, SDL_SCANCODE_SPACE, SDL_SCANCODE_KP_ENTER },
		.pad = { GB(SOUTH) },
	},

	[kNeed_UIDelete] =
	{
		.key = { SDL_SCANCODE_DELETE, SDL_SCANCODE_BACKSPACE },
		.pad = { GB(WEST) },
	},

	[kNeed_UIStart] =
	{
		.pad = { GB(START) },
	},

	[kNeed_UIBack] =
	{
		.key = { SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE },
		.pad = { GB(EAST), GB(BACK) },
		.mouseButton = SDL_BUTTON_X1
	},

	[kNeed_UIPause] =
	{
		.key = { SDL_SCANCODE_ESCAPE },
		.pad = { GB(START) },
	},
};
