//
// input.h
//

#pragma once

#include "Pomme.h"
#include "structs.h"

		/* NEEDS */

#define MAX_USER_BINDINGS_PER_NEED		2
#define MAX_HARD_BINDINGS_PER_NEED		1
#define MAX_BINDINGS_PER_NEED		(MAX_USER_BINDINGS_PER_NEED + MAX_HARD_BINDINGS_PER_NEED)

#define NUM_SUPPORTED_MOUSE_BUTTONS			31
#define NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL	(NUM_SUPPORTED_MOUSE_BUTTONS-2)
#define SDL_BUTTON_WHEELUP					(NUM_SUPPORTED_MOUSE_BUTTONS-2)		// make wheelup look like it's a button
#define SDL_BUTTON_WHEELDOWN				(NUM_SUPPORTED_MOUSE_BUTTONS-1)		// make wheeldown look like it's a button

#define NUM_MOUSE_SENSITIVITY_LEVELS		8
#define DEFAULT_MOUSE_SENSITIVITY_LEVEL		(NUM_MOUSE_SENSITIVITY_LEVELS/2)

typedef struct
{
	int8_t		type;
	int8_t		id;
} PadBinding;

typedef struct
{
	// SDL scancodes
	union
	{
		int16_t			key[MAX_BINDINGS_PER_NEED];

		struct
		{
			int16_t		userKey[MAX_USER_BINDINGS_PER_NEED];
			int16_t		hardKey[MAX_HARD_BINDINGS_PER_NEED];
		};
	};

	// Controller buttons
	union
	{
		PadBinding		pad[MAX_BINDINGS_PER_NEED];

		struct
		{
			PadBinding	userPad[MAX_USER_BINDINGS_PER_NEED];
			PadBinding	hardPad[MAX_HARD_BINDINGS_PER_NEED];
		};
	};


	int8_t				mouseButton;

} InputBinding;

enum
{
	kInputTypeUnbound = 0,
	kInputTypeButton,
	kInputTypeAxisPlus,
	kInputTypeAxisMinus,
};


	/* KEYBOARD EQUATE */


enum
{
	kNeed_ThrowForward,
	kNeed_ThrowBackward,
	kNeed_Brakes,
	kNeed_Forward,
	kNeed_Backward,
	kNeed_CameraMode,
	kNeed_RearView,
	NUM_CONTROL_BITS,

	kNeed_Left = NUM_CONTROL_BITS,
	kNeed_Right,
	NUM_REMAPPABLE_NEEDS,

	kNeed_UILeft = NUM_REMAPPABLE_NEEDS,
	kNeed_UIRight,
	kNeed_UIUp,
	kNeed_UIDown,
	kNeed_UIPrev,
	kNeed_UINext,
	kNeed_UIConfirm,
	kNeed_UIBack,
	kNeed_UIDelete,
	kNeed_UIPause,
	kNeed_UIStart,
	NUM_CONTROL_NEEDS
};

enum
{
	kControlBit_ThrowForward  = kNeed_ThrowForward,
	kControlBit_ThrowBackward = kNeed_ThrowBackward,
	kControlBit_Brakes        = kNeed_Brakes,
	kControlBit_Forward       = kNeed_Forward,
	kControlBit_Backward      = kNeed_Backward,
	kControlBit_CameraMode    = kNeed_CameraMode,
	kControlBit_RearView      = kNeed_RearView,
};


//============================================================================================


void InitInput(void);
void ReadKeyboard(void);
void InvalidateAllInputs(void);

Boolean GetKeyState(uint16_t sdlScancode);
Boolean GetNewKeyState(uint16_t sdlScancode);

Boolean GetClickState(int mouseButton);
Boolean GetNewClickState(int mouseButton);

Boolean GetNeedState(int needID, int playerID);
Boolean GetNeedStateAnyP(int needID);

Boolean GetNewNeedState(int needID, int playerID);
Boolean GetNewNeedStateAnyP(int needID);

OGLVector2D GetAnalogSteering(int playerID);

Boolean UserWantsOut(void);
Boolean IsCheatKeyComboDown(void);
void InitControlBits(void);
void GetLocalKeyState(void);

Boolean GetControlState(short player, uint32_t control);
Boolean GetControlStateNew(short player, uint32_t control);

void PushKeys(void);
void PopKeys(void);

void DoSDLMaintenance(void);

int GetNumControllers(void);
struct _SDL_GameController* GetController(int n);
void Rumble(float strength, uint32_t ms);

void LockPlayerControllerMapping(void);
void UnlockPlayerControllerMapping(void);
const char* GetPlayerName(int whichPlayer);
const char* GetPlayerNameWithInputDeviceHint(int whichPlayer);

void ResetDefaultKeyboardBindings(void);
void ResetDefaultGamepadBindings(void);
void ResetDefaultMouseBindings(void);
