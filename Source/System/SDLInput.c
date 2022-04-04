#include <SDL.h>
#include "globals.h"
#include "misc.h"
#include "input.h"
#include "window.h"
#include "ogl_support.h"
#include "miscscreens.h"
#include "main.h"
#include "player.h"
#include "file.h"

extern SDL_Window* gSDLWindow;
extern	PrefsType			gGamePrefs;

/***************/
/* CONSTANTS   */
/***************/

enum
{
	KEYSTATE_OFF		= 0b00,
	KEYSTATE_UP			= 0b01,

	KEYSTATE_PRESSED	= 0b10,
	KEYSTATE_HELD		= 0b11,

	KEYSTATE_ACTIVE_BIT	= 0b10,
};

#define kJoystickDeadZone				(20 * 32767 / 100)
#define kJoystickDeadZone_UI			(66 * 32767 / 100)
#define kJoystickDeadZoneFrac			(kJoystickDeadZone / 32767.0f)
#define kJoystickDeadZoneFracSquared	(kJoystickDeadZoneFrac * kJoystickDeadZoneFrac)

#define kDefaultSnapAngle				OGLMath_DegreesToRadians(10)

/**********************/
/*     PROTOTYPES     */
/**********************/

typedef struct
{
	bool					open;
	SDL_GameController*		controllerInstance;
	SDL_JoystickID			joystickInstance;
	Byte					needStates[NUM_CONTROL_NEEDS];
} Controller;

Boolean				gUserPrefersGamepad = false;

Boolean				gControllerPlayerMappingLocked = false;
int					gNumControllersOpen = 0;
Controller			gControllers[MAX_LOCAL_PLAYERS];

Byte				gRawKeyboardState[SDL_NUM_SCANCODES];
char				gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];

Byte				gNeedStates[NUM_CONTROL_NEEDS];

/**********************/
/* STATIC FUNCTIONS   */
/**********************/

static void OnJoystickRemoved(SDL_JoystickID which);
static SDL_GameController* TryOpenControllerFromJoystick(int joystickIndex);
static SDL_GameController* TryOpenAnyController(bool showMessage);
static int GetControllerSlotFromSDLJoystickInstanceID(SDL_JoystickID joystickInstanceID);

static inline void UpdateKeyState(Byte* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
		case KEYSTATE_HELD:
		case KEYSTATE_PRESSED:
			*state = downNow ? KEYSTATE_HELD : KEYSTATE_UP;
			break;
		case KEYSTATE_OFF:
		case KEYSTATE_UP:
		default:
			*state = downNow ? KEYSTATE_PRESSED : KEYSTATE_OFF;
			break;
	}
}

void DoSDLMaintenance(void)
{
	gTextInput[0] = '\0';

			/**********************/
			/* DO SDL MAINTENANCE */
			/**********************/

	int mouseWheelDelta = 0;

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				ExitToShell();			// throws Pomme::QuitRequest
				return;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						ExitToShell();	// throws Pomme::QuitRequest
						return;

					/*
					case SDL_WINDOWEVENT_RESIZED:
						QD3D_OnWindowResized(event.window.data1, event.window.data2);
						break;
					*/
				}
				break;

				case SDL_TEXTINPUT:
					memcpy(gTextInput, event.text.text, sizeof(gTextInput));
					_Static_assert(sizeof(gTextInput) == sizeof(event.text.text), "size mismatch: gTextInput/event.text.text");
					break;

				case SDL_MOUSEWHEEL:
					// if (!gEatMouse)
					{
						mouseWheelDelta += event.wheel.y;
						mouseWheelDelta += event.wheel.x;
					}
					break;

				case SDL_JOYDEVICEADDED:	 // event.jdevice.which is the joy's INDEX (not an instance id!)
					TryOpenControllerFromJoystick(event.cdevice.which);
					break;

				case SDL_JOYDEVICEREMOVED:	// event.jdevice.which is the joy's UNIQUE INSTANCE ID (not an index!)
					OnJoystickRemoved(event.jdevice.which);
					break;

				case SDL_KEYDOWN:
					gUserPrefersGamepad = false;
					break;

				case SDL_CONTROLLERBUTTONDOWN:
				case SDL_CONTROLLERBUTTONUP:
				case SDL_JOYBUTTONDOWN:
					gUserPrefersGamepad = true;
					break;
		}
	}

	// --------------------------------------------
	// Refresh the state of each individual keyboard key

	int numkeys = 0;
	const UInt8* keystate = SDL_GetKeyboardState(&numkeys);

	{
		int minNumKeys = numkeys < SDL_NUM_SCANCODES ? numkeys : SDL_NUM_SCANCODES;

		for (int i = 0; i < minNumKeys; i++)
			UpdateKeyState(&gRawKeyboardState[i], keystate[i]);

		// fill out the rest
		for (int i = minNumKeys; i < SDL_NUM_SCANCODES; i++)
			UpdateKeyState(&gRawKeyboardState[i], false);
	}

	// --------------------------------------------

	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		const KeyBinding* kb = &gGamePrefs.keys[i][0];

		bool downNow = false;

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
			if (kb->key[j] && kb->key[j] < numkeys)
				downNow |= 0 != keystate[kb->key[j]];

//		downNow |= gMouseButtonState[kb->mouseButton] & KEYSTATE_ACTIVE_BIT;

		UpdateKeyState(&gNeedStates[i], downNow);
	}

	//-------------------------------------------------------------------------
	// Multiplayer gamepad input
	//-------------------------------------------------------------------------

	for (int controllerNum = 0; controllerNum < MAX_LOCAL_PLAYERS; controllerNum++)
	{
		if (!gControllers[controllerNum].open)
		{
			continue;
		}

		SDL_GameController* controllerInstance = gControllers[controllerNum].controllerInstance;
		
		for (int needNum = 0; needNum < NUM_CONTROL_NEEDS; needNum++)
		{
			const KeyBinding* kb = &gGamePrefs.keys[needNum][controllerNum];  // ish...

			int16_t deadZone = needNum >= NUM_REMAPPABLE_NEEDS
							? kJoystickDeadZone_UI
							: kJoystickDeadZone;

			bool downNow = false;

			for (int buttonNum = 0; buttonNum < KEYBINDING_MAX_GAMEPAD_BUTTONS; buttonNum++)
			{
				switch (kb->gamepad[buttonNum].type)
				{
					case kInputTypeButton:
						downNow |= 0 != SDL_GameControllerGetButton(controllerInstance, kb->gamepad[buttonNum].id);
						break;

					case kInputTypeAxisPlus:
						downNow |= SDL_GameControllerGetAxis(controllerInstance, kb->gamepad[buttonNum].id) > deadZone;
						break;

					case kInputTypeAxisMinus:
						downNow |= SDL_GameControllerGetAxis(controllerInstance, kb->gamepad[buttonNum].id) < -deadZone;
						break;

					default:
						break;
				}
			}

			UpdateKeyState(&gControllers[controllerNum].needStates[needNum], downNow);
		}
	}
}

#pragma mark -

Boolean GetKeyState(uint16_t sdlScancode)
{
	if (sdlScancode >= SDL_NUM_SCANCODES)
		return false;
	return 0 != (gRawKeyboardState[sdlScancode] & KEYSTATE_ACTIVE_BIT);
}

Boolean GetNewKeyState(uint16_t sdlScancode)
{
	if (sdlScancode >= SDL_NUM_SCANCODES)
		return false;
	return gRawKeyboardState[sdlScancode] == KEYSTATE_PRESSED;
}

#pragma mark -

Boolean GetNeedState(int needID, int playerID)
{
	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	if (gControllers[playerID].open
		&& (gControllers[playerID].needStates[needID] & KEYSTATE_ACTIVE_BIT))
	{
		return true;
	}

	// Fallback to KB/M
	if (playerID == 0)
	{
		return gNeedStates[needID] & KEYSTATE_ACTIVE_BIT;
	}

	return false;
}

Boolean GetNeedStateAnyP(int needID)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gControllers[i].open
			&& (gControllers[i].needStates[needID] & KEYSTATE_ACTIVE_BIT))
		{
			return true;
		}
	}

	// Fallback to KB/M
	return gNeedStates[needID] & KEYSTATE_ACTIVE_BIT;
}

Boolean GetNewNeedState(int needID, int playerID)
{
	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	if (gControllers[playerID].open
		&& (gControllers[playerID].needStates[needID] == KEYSTATE_PRESSED))
	{
		return true;
	}

	// Fallback to KB/M
	if (playerID == 0)
	{
		return gNeedStates[needID] == KEYSTATE_PRESSED;
	}

	return false;
}

Boolean GetNewNeedStateAnyP(int needID)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gControllers[i].open
			&& (gControllers[i].needStates[needID] == KEYSTATE_PRESSED))
		{
			return true;
		}
	}

	// Fallback to KB/M
	return gNeedStates[needID] == KEYSTATE_PRESSED;
}

Boolean AreAnyNewKeysPressed(void)
{
	return GetNewNeedStateAnyP(kNeed_UIConfirm);
}

#pragma mark -

float GetAnalogSteering(int playerID)
{
			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

	float steer = 0; 											// assume no control input

			/* FIRST CHECK ANALOG AXES */

	SDL_GameController* sdlController = SDL_GameControllerFromPlayerIndex(playerID);

	if (sdlController)
	{
		Sint16 dxRaw = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTX);

		steer = dxRaw / 32767.0f;
		float steerMag = fabsf(steer);

		if (steerMag < kJoystickDeadZoneFrac)
		{
			steer = 0;
		}
		else if (steer < -1.0f)
		{
			steer = -1.0f;
		}
		else if (steer > 1.0f)
		{
			steer = 1.0f;
		}
		else
		{
			// Avoid magnitude bump when thumbstick is pushed past dead zone:
			// Bring magnitude from [kJoystickDeadZoneFrac, 1.0] to [0.0, 1.0].
			float steerSign = steer < 0 ? -1.0f : 1.0f;
			steer = steerSign * (steerMag - kJoystickDeadZoneFrac) / (1.0f - kJoystickDeadZoneFrac);
		}
	}

			/* NEXT CHECK THE DIGITAL KEYS */

	if (GetNeedState(kNeed_Left, playerID))					// is Left Key pressed?
	{
		steer = -1.0f;
	}
	else if (GetNeedState(kNeed_Right, playerID))			// is Right Key pressed?
	{
		steer = 1.0f;
	}

	return steer;
}

#pragma mark -

/****************************** SDL JOYSTICK FUNCTIONS ********************************/

int GetNumControllers(void)
{
	int count = 0;

#if 0
	for (int i = 0; i < SDL_NumJoysticks(); ++i)
	{
		if (SDL_IsGameController(i))
		{
			count++;
		}
	}
#else
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gControllers[i].open)
		{
			count++;
		}
	}
#endif

	return count;
}

static int FindFreeControllerSlot()
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (!gControllers[i].open)
		{
			return i;
		}
	}

	return -1;
}

static SDL_GameController* TryOpenControllerFromJoystick(int joystickIndex)
{
	SDL_JoystickID joystickInstanceID = SDL_JoystickGetDeviceInstanceID(joystickIndex);

	// First, check that it's not in use already
	for (int controllerSlot = 0; controllerSlot < MAX_LOCAL_PLAYERS; controllerSlot++)
	{
		if (gControllers[controllerSlot].open &&
			gControllers[controllerSlot].joystickInstance == joystickInstanceID)
		{
			return gControllers[controllerSlot].controllerInstance;
		}
	}

	// If we can't get an SDL_GameController from that joystick, don't bother
	if (!SDL_IsGameController(joystickIndex))
	{
		return NULL;
	}

	// Reserve a controller slot
	int controllerSlot = FindFreeControllerSlot();
	if (controllerSlot < 0)
	{
		printf("All controller slots used up.\n");
		// TODO: when a controller is unplugged, if all controller slots are used up, re-scan connected joysticks and try to open any unopened joysticks.
		return NULL;
	}

	// Use this one
	SDL_GameController* controllerInstance = SDL_GameControllerOpen(joystickIndex);

	// Assign player ID
	SDL_GameControllerSetPlayerIndex(controllerInstance, controllerSlot);

	gControllers[controllerSlot] = (Controller)
	{
		.open = true,
		.controllerInstance = controllerInstance,
		.joystickInstance = joystickInstanceID,
	};

	printf("Opened joystick %d as controller: %s\n",
		gControllers[controllerSlot].joystickInstance,
		SDL_GameControllerName(gControllers[controllerSlot].controllerInstance));

	return gControllers[controllerSlot].controllerInstance;
}

static SDL_GameController* TryOpenAnyController(bool showMessage)
{
	if (SDL_NumJoysticks() == 0)
	{
		return NULL;
	}

	for (int i = 0; i < SDL_NumJoysticks(); ++i)
	{
		if (!SDL_IsGameController(i))
		{
			continue;
		}

		// Use this one
		SDL_GameController* newController = TryOpenControllerFromJoystick(i);
		if (newController)
		{
			return newController;
		}
	}

	printf("Joystick(s) found, but none is suitable as an SDL_GameController.\n");
	if (showMessage)
	{
		char messageBuf[1024];
		snprintf(messageBuf, sizeof(messageBuf),
					"The game does not support your controller yet (\"%s\").\n\n"
					"You can play with the keyboard and mouse instead. Sorry!",
					SDL_JoystickNameForIndex(0));
		SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_WARNING,
				"Controller not supported",
				messageBuf,
				gSDLWindow);
	}

	return NULL;
}

void Rumble(float strength, uint32_t ms)
{
	#if 0	// TODO: Rumble for specific player
	if (NULL == gSDLController || !gGamePrefs.gamepadRumble)
		return;

#if !(SDL_VERSION_ATLEAST(2,0,9))
	#warning Rumble support requires SDL 2.0.9 or later
#else
	SDL_GameControllerRumble(gSDLController, (Uint16)(strength * 65535), (Uint16)(strength * 65535), ms);
#endif
	#endif
}

static int GetControllerSlotFromSDLJoystickInstanceID(SDL_JoystickID joystickInstanceID)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gControllers[i].open && gControllers[i].joystickInstance == joystickInstanceID)
		{
			return i;
		}
	}

	return -1;
}

static void CloseController(int controllerSlot)
{
	GAME_ASSERT(gControllers[controllerSlot].open);
	GAME_ASSERT(gControllers[controllerSlot].controllerInstance);

	SDL_GameControllerClose(gControllers[controllerSlot].controllerInstance);
	gControllers[controllerSlot].open = false;
	gControllers[controllerSlot].controllerInstance = NULL;
	gControllers[controllerSlot].joystickInstance = -1;
}

static void CompactControllerSlots(void)
{
	int writeIndex = 0;

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		GAME_ASSERT(writeIndex <= i);

		if (gControllers[i].open)
		{
			if (writeIndex != i)
			{
				printf("Remapped player controller %d ---> %d\n", i, writeIndex);

				gControllers[writeIndex] = gControllers[i];
				SDL_GameControllerSetPlayerIndex(gControllers[writeIndex].controllerInstance, writeIndex);

				// Clear duplicate slot so we don't read it by mistake in the future
				gControllers[i].controllerInstance = NULL;
				gControllers[i].joystickInstance = -1;
				gControllers[i].open = false;
			}

			writeIndex++;
		}
	}
}

static void OnJoystickRemoved(SDL_JoystickID joystickInstanceID)
{
	int controllerSlot = GetControllerSlotFromSDLJoystickInstanceID(joystickInstanceID);

	if (controllerSlot >= 0)		// we're using this joystick
	{
		printf("Joystick %d was removed, was used by controller slot #%d\n", joystickInstanceID, controllerSlot);

		// Nuke reference to this controller+joystick
		CloseController(controllerSlot);
	}

	if (!gControllerPlayerMappingLocked)
	{
		CompactControllerSlots();
	}

	// Fill up any controller slots that are vacant
	while (TryOpenAnyController(false) != NULL)
	{
		// Successful; there might be more joysticks available, keep going
	}
}
