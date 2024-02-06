#include <SDL.h>
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

// Controller button/axis
#define CB(x)		{kInputTypeButton, SDL_CONTROLLER_BUTTON_##x}
#define CAPLUS(x)	{kInputTypeAxisPlus, SDL_CONTROLLER_AXIS_##x}
#define CAMINUS(x)	{kInputTypeAxisMinus, SDL_CONTROLLER_AXIS_##x}
#define CBNULL()	{kInputTypeUnbound, 0}

const InputBinding kDefaultInputBindings[NUM_CONTROL_NEEDS] =
{
	[kNeed_ThrowForward] =
	{
		.key = { SC_THROWF1, SC_THROWF2 },
		.pad = { CB(Y), CAMINUS(RIGHTY) },
	},

	[kNeed_ThrowBackward] =
	{
		.key = { SC_THROWB1, SC_THROWB2 },
		.pad = { CB(X), CAPLUS(RIGHTY) },
	},

	[kNeed_Brakes] =
	{
		.key = { SDL_SCANCODE_SPACE },
		.pad = { CAPLUS(TRIGGERRIGHT) },
	},

	[kNeed_CameraMode] =
	{
		.key = { SDL_SCANCODE_GRAVE, SDL_SCANCODE_RETURN },
		.pad = { CB(LEFTSHOULDER) },
	},

	[kNeed_RearView] =
	{
		.key = { SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT },
		.pad = { CAPLUS(TRIGGERLEFT) },
	},

	[kNeed_Forward] =
	{
		.key = { SDL_SCANCODE_UP, SDL_SCANCODE_W },
		.pad = { CB(A) },
	},

	[kNeed_Backward] =
	{
		.key = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		.pad = { CB(B) },
	},

	[kNeed_Left] =
	{
		.key = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A },
		.pad = { CAMINUS(LEFTX) },
	},

	[kNeed_Right] =
	{
		.key = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		.pad = { CAPLUS(LEFTX) },
	},

	// -----------------------------------------------------------
	// Non-remappable UI bindings below

	[kNeed_UIUp] =
	{
		.key = { SDL_SCANCODE_UP, SDL_SCANCODE_W },
		.pad = { CB(DPAD_UP), CAMINUS(LEFTY) },
	},

	[kNeed_UIDown] =
	{
		.key = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		.pad = { CB(DPAD_DOWN), CAPLUS(LEFTY) },
	},

	[kNeed_UILeft] =
	{
		.key = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A },
		.pad = { CB(DPAD_LEFT), CAMINUS(LEFTX) },
	},

	[kNeed_UIRight] =
	{
		.key = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		.pad = { CB(DPAD_RIGHT), CAPLUS(LEFTX) },
	},

	[kNeed_UIPrev] =
	{
		.pad = { CB(LEFTSHOULDER) },
	},

	[kNeed_UINext] =
	{
		.pad = { CB(RIGHTSHOULDER) },
	},

	[kNeed_UIConfirm] =
	{
		.key = { SDL_SCANCODE_RETURN, SDL_SCANCODE_SPACE, SDL_SCANCODE_KP_ENTER },
		.pad = { CB(A) },
	},

	[kNeed_UIDelete] =
	{
		.key = { SDL_SCANCODE_DELETE, SDL_SCANCODE_BACKSPACE },
		.pad = { CB(X) },
	},

	[kNeed_UIStart] =
	{
		.pad = { CB(START) },
	},

	[kNeed_UIBack] =
	{
		.key = { SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE },
		.pad = {CB(B), CB(BACK) },
		.mouseButton = SDL_BUTTON_X1
	},

	[kNeed_UIPause] =
	{
		.key = { SDL_SCANCODE_ESCAPE },
		.pad = { CB(START) },
	},

};
