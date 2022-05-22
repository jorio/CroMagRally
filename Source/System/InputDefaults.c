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

// Scancode
#define SC(x)		SDL_SCANCODE_##x

// Mouse button/wheel
#define MB(x)		SDL_BUTTON_##x

// Controller button/axis
#define CB(x)		{kInputTypeButton, SDL_CONTROLLER_BUTTON_##x}
#define CAPLUS(x)	{kInputTypeAxisPlus, SDL_CONTROLLER_AXIS_##x}
#define CAMINUS(x)	{kInputTypeAxisMinus, SDL_CONTROLLER_AXIS_##x}
#define CBNULL()	{kInputTypeUnbound, 0}

const KeyBinding kDefaultKeyBindings[NUM_CONTROL_NEEDS] =
{
//Need-------------------     Keys---------------------------  Mouse-----  Gamepad----------------------------------------
[kNeed_ThrowForward		] = { { SC_THROWF1  , SC_THROWF2	}, 0		, { CB(Y)				, CAMINUS(RIGHTY)	, CBNULL()			} },
[kNeed_ThrowBackward	] = { { SC_THROWB1	, SC_THROWB2	}, 0		, { CB(X)				, CAPLUS(RIGHTY)	, CBNULL()			} },
[kNeed_Brakes			] = { { SC(SPACE)	, 0				}, 0		, { CAPLUS(TRIGGERRIGHT), CBNULL()			, CBNULL()			} },
[kNeed_CameraMode		] = { { SC(GRAVE)	, 0				}, 0		, { CB(LEFTSHOULDER)	, CBNULL()			, CBNULL()			} },
[kNeed_RearView			] = { { SC(LSHIFT)	, 0				}, 0		, { CAPLUS(TRIGGERLEFT)	, CBNULL()			, CBNULL()			} },
[kNeed_Forward			] = { { SC(UP)		, SC(W)			}, 0		, { CB(A)				, CBNULL()			, CB(DPAD_UP)		} },
[kNeed_Backward			] = { { SC(DOWN)	, SC(S)			}, 0		, { CB(B)				, CBNULL()			, CB(DPAD_DOWN)		} },
[kNeed_Left			    ] = { { SC(LEFT)	, SC(A)			}, 0		, { CBNULL()			, CBNULL()			, CB(DPAD_LEFT)		} },
[kNeed_Right		    ] = { { SC(RIGHT)	, SC(D)			}, 0		, { CBNULL()			, CBNULL()			, CB(DPAD_RIGHT)	} },

[kNeed_UIUp				] = { { SC(UP)		, SC(W)			}, 0		, { CB(DPAD_UP)			, CAMINUS(LEFTY)		} },
[kNeed_UIDown			] = { { SC(DOWN)	, SC(S)			}, 0		, { CB(DPAD_DOWN)		, CAPLUS(LEFTY)			} },
[kNeed_UILeft			] = { { SC(LEFT)	, SC(A)			}, 0		, { CB(DPAD_LEFT)		, CAMINUS(LEFTX)		} },
[kNeed_UIRight			] = { { SC(RIGHT)	, SC(D)			}, 0		, { CB(DPAD_RIGHT)		, CAPLUS(LEFTX)			} },
[kNeed_UIPrev			] = { { 0			, 0				}, 0		, { CB(LEFTSHOULDER)	, CBNULL()				} },
[kNeed_UINext			] = { { 0			, 0				}, 0		, { CB(RIGHTSHOULDER)	, CBNULL()				} },
[kNeed_UIConfirm		] = { { SC(RETURN)	, SC(SPACE)		}, 0		, { CB(A)				, CBNULL()				} },
[kNeed_UIDelete			] = { { SC(DELETE)	, SC(BACKSPACE)	}, 0		, { CB(X)				, CBNULL()				} },
[kNeed_UIStart			] = { { 0			, 0				}, 0		, { CB(START)			, CBNULL()				} },
[kNeed_UIBack			] = { { SC(ESCAPE)	, SC(BACKSPACE)	}, MB(X1)	, { CB(B)				, CB(BACK)				} },
[kNeed_UIPause			] = { { SC(ESCAPE)	, 0				}, 0		, { CB(START)			, CBNULL()				} },

/*
[kNeed_TextEntry_Left	] = { { SC(LEFT)	, 0				}, 0			, { CB(DPAD_LEFT)		, CB(LEFTSHOULDER)		} },
[kNeed_TextEntry_Left2	] = { { 0			, 0				}, 0			, { CAMINUS(LEFTX)		, 						} },
[kNeed_TextEntry_Right	] = { { SC(RIGHT)	, 0				}, 0			, { CB(DPAD_RIGHT)		, CB(RIGHTSHOULDER)		} },
[kNeed_TextEntry_Right2	] = { { 0			, 0				}, 0			, { CAPLUS(LEFTX)		, CBNULL()				} },
[kNeed_TextEntry_Home	] = { { SC(HOME)	, 0				}, 0			, { CBNULL()			, CBNULL()				} },
[kNeed_TextEntry_End	] = { { SC(END)		, 0				}, 0			, { CBNULL()			, CBNULL()				} },
[kNeed_TextEntry_Bksp	] = { { SC(BACKSPACE),0				}, 0			, { CB(X)				, CBNULL()				} },
[kNeed_TextEntry_Del	] = { { SC(DELETE)	, 0				}, 0			, { CBNULL()			, CBNULL()				} },
[kNeed_TextEntry_Done	] = { { SC(RETURN)	, SC(KP_ENTER)	}, 0			, { CB(START)			, CBNULL()				} },
[kNeed_TextEntry_CharPP	] = { { 0			, 0				}, 0			, { CB(A)				, CB(DPAD_UP)			} },
[kNeed_TextEntry_CharMM	] = { { 0			, 0				}, 0			, { CB(B)				, CB(DPAD_DOWN)			} },
[kNeed_TextEntry_Space	] = { { 0			, 0				}, 0			, { CB(Y)				, CBNULL()				} },
*/
};
