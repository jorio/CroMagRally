//
// input.h
//

#pragma once

#include "Pomme.h"

		/* NEEDS */

#define KEYBINDING_MAX_KEYS					2
#define KEYBINDING_MAX_GAMEPAD_BUTTONS		2

#define NUM_SUPPORTED_MOUSE_BUTTONS			31
#define NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL	(NUM_SUPPORTED_MOUSE_BUTTONS-2)
#define SDL_BUTTON_WHEELUP					(NUM_SUPPORTED_MOUSE_BUTTONS-2)		// make wheelup look like it's a button
#define SDL_BUTTON_WHEELDOWN				(NUM_SUPPORTED_MOUSE_BUTTONS-1)		// make wheeldown look like it's a button

#define NUM_MOUSE_SENSITIVITY_LEVELS		8
#define DEFAULT_MOUSE_SENSITIVITY_LEVEL		(NUM_MOUSE_SENSITIVITY_LEVELS/2)

typedef struct KeyBinding
{
	int16_t			key[KEYBINDING_MAX_KEYS];

	int8_t			mouseButton;

	struct
	{
		int8_t		type;
		int8_t		id;
	} gamepad[KEYBINDING_MAX_GAMEPAD_BUTTONS];
} KeyBinding;

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
	kNeed_CameraMode,
	kNeed_Forward,
	kNeed_Backward,
	NUM_CONTROL_BITS,

	kNeed_Left = NUM_CONTROL_BITS,
	kNeed_Right,
	kNeed_RearView,
	kNeed_ToggleMusic,
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
	kControlBit_CameraMode    = kNeed_CameraMode,
	kControlBit_Forward       = kNeed_Forward,
	kControlBit_Backward      = kNeed_Backward,
};

#if 0
enum
{
	kKey_Pause				= KEY_ESC,

	kKey_ToggleMusic 		= KEY_M,
	kKey_RaiseVolume 		= KEY_PLUS,
	kKey_LowerVolume 		= KEY_MINUS,

	kKey_ThrowForward_P1	= KEY_APPLE,
	kKey_ThrowForward_P2	= KEY_SHIFT,

	kKey_ThrowBackward_P1	= KEY_OPTION,
	kKey_ThrowBackward_P2	= KEY_3,

	kKey_Brakes_P1			= KEY_SPACE,
	kKey_Brakes_P2			= KEY_TAB,

	kKey_CameraMode_P1		= KEY_TILDE,
	kKey_CameraMode_P2		= KEY_2,

	kKey_Forward_P1			= KEY_UP,
	kKey_Backward_P1		= KEY_DOWN,

	kKey_Left_P1			= KEY_LEFT,
	kKey_Right_P1			= KEY_RIGHT,

	kKey_Left_P2			= KEY_A,
	kKey_Right_P2			= KEY_D,

	kKey_Forward_P2			= KEY_W,
	kKey_Backward_P2		= KEY_S,

	kKey_Quit				= KEY_Q
};

#define	kKey_MakeSelection_P1	kKey_Brakes_P1
#define	kKey_MakeSelection_P2	kKey_Brakes_P2





enum				// must match gControlBitToKey list!!!
{
	kControlBit_ThrowForward = 0,
	kControlBit_ThrowBackward,
	kControlBit_Brakes,
	kControlBit_CameraMode,
	kControlBit_Forward,
	kControlBit_Backward,

	NUM_CONTROL_BITS
};


//#define	NUM_CONTROL_NEEDS		14

#endif


//============================================================================================


void InitInput(void);
extern	void ReadKeyboard(void);

Boolean GetKeyState(uint16_t sdlScancode);
Boolean GetNewKeyState(uint16_t sdlScancode);

Boolean GetClickState(int mouseButton);
Boolean GetNewClickState(int mouseButton);

Boolean GetNeedState(int needID, int playerID);
Boolean GetNeedStateAnyP(int needID);

Boolean GetNewNeedState(int needID, int playerID);
Boolean GetNewNeedStateAnyP(int needID);

float GetAnalogSteering(int playerID);

void TurnOnISp(void);
void TurnOffISp(void);

void DoKeyConfigDialog(void);

Boolean AreAnyNewKeysPressed(void);
Boolean IsCheatKeyComboDown(void);
void InitControlBits(void);
void GetLocalKeyState(void);

Boolean GetControlState(short player, uint32_t control);
Boolean GetControlStateNew(short player, uint32_t control);

void PushKeys(void);
void PopKeys(void);

void DoSDLMaintenance(void);

int GetNumControllers(void);
void Rumble(float strength, uint32_t ms);

void LockPlayerControllerMapping(void);
void UnlockPlayerControllerMapping(void);
const char* GetPlayerNameWithInputDeviceHint(int whichPlayer);
