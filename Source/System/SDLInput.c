// SDL INPUT
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/CroMagRally

#include "game.h"
#include <SDL.h>

extern SDL_Window* gSDLWindow;

// Provide GameController stubs for CI runners that have old SDL packages.
// This lets us run quick compile checks on the CI without recompiling SDL.
#if !(SDL_VERSION_ATLEAST(2,0,12))
	#warning "Multiplayer controller support requires SDL 2.0.12 or later. The game will compile but controllers won't work!"
	static void SDL_GameControllerSetPlayerIndex(SDL_GameController *c, int i) {}
	static SDL_GameController *SDL_GameControllerFromPlayerIndex(int i) { return NULL; }
	#if !(SDL_VERSION_ATLEAST(2,0,9))
		static int SDL_GameControllerGetPlayerIndex(SDL_GameController *c) { return 0; }
	#endif
#endif

/***************/
/* CONSTANTS   */
/***************/

enum
{
	KEYSTATE_ACTIVE_BIT		= 0b001,
	KEYSTATE_CHANGE_BIT		= 0b010,
	KEYSTATE_IGNORE_BIT		= 0b100,

	KEYSTATE_OFF			= 0b000,
	KEYSTATE_PRESSED		= KEYSTATE_ACTIVE_BIT | KEYSTATE_CHANGE_BIT,
	KEYSTATE_HELD			= KEYSTATE_ACTIVE_BIT,
	KEYSTATE_UP				= KEYSTATE_OFF | KEYSTATE_CHANGE_BIT,
	KEYSTATE_IGNOREHELD		= KEYSTATE_OFF | KEYSTATE_IGNORE_BIT,
};

#define kJoystickDeadZone				(20 * 32767 / 100)
#define kJoystickDeadZone_UI			(66 * 32767 / 100)
#define kJoystickDeadZoneFrac			(kJoystickDeadZone / 32767.0f)
#define kJoystickDeadZoneFracSquared	(kJoystickDeadZoneFrac * kJoystickDeadZoneFrac)

#define kDefaultSnapAngle				OGLMath_DegreesToRadians(10)

/**********************/
/*     PROTOTYPES     */
/**********************/

typedef uint8_t KeyState;

typedef struct Controller
{
	bool					open;
	bool					fallbackToKeyboard;
	SDL_GameController*		controllerInstance;
	SDL_JoystickID			joystickInstance;
	KeyState				needStates[NUM_CONTROL_NEEDS];
} Controller;

Boolean				gUserPrefersGamepad = false;

static Boolean		gControllerPlayerMappingLocked = false;
Controller			gControllers[MAX_LOCAL_PLAYERS];

static KeyState		gKeyboardStates[SDL_NUM_SCANCODES];
static KeyState		gMouseButtonStates[NUM_SUPPORTED_MOUSE_BUTTONS];
static KeyState		gNeedStates[NUM_CONTROL_NEEDS];

Boolean				gMouseMotionNow = false;
char				gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];

static void OnJoystickRemoved(SDL_JoystickID which);
static SDL_GameController* TryOpenControllerFromJoystick(int joystickIndex);
static SDL_GameController* TryOpenAnyController(bool showMessage);
static int GetControllerSlotFromSDLJoystickInstanceID(SDL_JoystickID joystickInstanceID);

#pragma mark -
/**********************/

static inline void UpdateKeyState(KeyState* state, bool downNow)
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

		case KEYSTATE_IGNOREHELD:
			*state = downNow ? KEYSTATE_IGNOREHELD : KEYSTATE_OFF;
			break;
	}
}

void InvalidateNeedState(int need)
{
	gNeedStates[need] = KEYSTATE_IGNOREHELD;
}

void InvalidateAllInputs(void)
{
	_Static_assert(1 == sizeof(KeyState), "sizeof(KeyState) has changed -- Rewrite this function without memset()!");

	memset(gNeedStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
	memset(gKeyboardStates, KEYSTATE_IGNOREHELD, SDL_NUM_SCANCODES);
	memset(gMouseButtonStates, KEYSTATE_IGNOREHELD, NUM_SUPPORTED_MOUSE_BUTTONS);

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		memset(gControllers[i].needStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
	}

}

static void UpdateRawKeyboardStates(void)
{
	int numkeys = 0;
	const UInt8* keystate = SDL_GetKeyboardState(&numkeys);

	int minNumKeys = GAME_MIN(numkeys, SDL_NUM_SCANCODES);

	for (int i = 0; i < minNumKeys; i++)
		UpdateKeyState(&gKeyboardStates[i], keystate[i]);

	// fill out the rest
	for (int i = minNumKeys; i < SDL_NUM_SCANCODES; i++)
		UpdateKeyState(&gKeyboardStates[i], false);
}

static void ParseAltEnter(void)
{
	if (GetNewKeyState(SDL_SCANCODE_RETURN)
		&& (GetKeyState(SDL_SCANCODE_LALT) || GetKeyState(SDL_SCANCODE_RALT)))
	{
		gGamePrefs.fullscreen = !gGamePrefs.fullscreen;
		SetFullscreenMode(false);

		InvalidateAllInputs();
	}

}

static void UpdateMouseButtonStates(int mouseWheelDelta)
{
	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

	for (int i = 1; i < NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL; i++)	// SDL buttons start at 1!
	{
		bool buttonBit = 0 != (mouseButtons & SDL_BUTTON(i));
		UpdateKeyState(&gMouseButtonStates[i], buttonBit);
	}

	// Fake buttons for mouse wheel up/down
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELUP], mouseWheelDelta > 0);
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELDOWN], mouseWheelDelta < 0);
}

static void UpdateInputNeeds(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		const KeyBinding* kb = &gGamePrefs.keys[i];

		bool downNow = false;

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
		{
			int16_t scancode = kb->key[j];
			if (scancode && scancode < SDL_NUM_SCANCODES)
			{
				downNow |= gKeyboardStates[scancode] & KEYSTATE_ACTIVE_BIT;
			}
		}

//		downNow |= gMouseButtonStates[kb->mouseButton] & KEYSTATE_ACTIVE_BIT;

		UpdateKeyState(&gNeedStates[i], downNow);
	}
}

static void UpdateControllerSpecificInputNeeds(int controllerNum)
{
	if (!gControllers[controllerNum].open)
	{
		return;
	}

	SDL_GameController* controllerInstance = gControllers[controllerNum].controllerInstance;

	for (int needNum = 0; needNum < NUM_CONTROL_NEEDS; needNum++)
	{
		const KeyBinding* kb = &gGamePrefs.keys[needNum];

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

#pragma mark -

/**********************/
/* PUBLIC FUNCTIONS   */
/**********************/

void DoSDLMaintenance(void)
{
	gTextInput[0] = '\0';
	gMouseMotionNow = false;
	int mouseWheelDelta = 0;

			/**********************/
			/* DO SDL MAINTENANCE */
			/**********************/

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				CleanQuit();			// throws Pomme::QuitRequest
				return;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						CleanQuit();	// throws Pomme::QuitRequest
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

				case SDL_MOUSEMOTION:
					gMouseMotionNow = true;
					break;

				case SDL_MOUSEWHEEL:
					mouseWheelDelta += event.wheel.y;
					mouseWheelDelta += event.wheel.x;
					break;

				case SDL_CONTROLLERDEVICEADDED:
					// event.cdevice.which is the joystick's DEVICE INDEX (not an instance id!)
					TryOpenControllerFromJoystick(event.cdevice.which);
					break;

				case SDL_CONTROLLERDEVICEREMOVED:
					// event.cdevice.which is the joystick's UNIQUE INSTANCE ID (not an index!)
					OnJoystickRemoved(event.cdevice.which);
					break;

				/*
				case SDL_CONTROLLERDEVICEREMAPPED:
					printf("C-Device remapped! %d\n", event.cdevice.which);
					break;
				*/

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


	// Refresh the state of each individual keyboard key
	UpdateRawKeyboardStates();

	// On ALT+ENTER, toggle fullscreen, and ignore ENTER until keyup
	ParseAltEnter();

	// Refresh the state of each mouse button
	UpdateMouseButtonStates(mouseWheelDelta);

	// Refresh the state of each input need
	UpdateInputNeeds();

	//-------------------------------------------------------------------------
	// Multiplayer gamepad input
	//-------------------------------------------------------------------------

	for (int controllerNum = 0; controllerNum < MAX_LOCAL_PLAYERS; controllerNum++)
	{
		UpdateControllerSpecificInputNeeds(controllerNum);
	}

}

#pragma mark -

Boolean GetKeyState(uint16_t sdlScancode)
{
	if (sdlScancode >= SDL_NUM_SCANCODES)
		return false;
	return 0 != (gKeyboardStates[sdlScancode] & KEYSTATE_ACTIVE_BIT);
}

Boolean GetNewKeyState(uint16_t sdlScancode)
{
	if (sdlScancode >= SDL_NUM_SCANCODES)
		return false;
	return gKeyboardStates[sdlScancode] == KEYSTATE_PRESSED;
}

#pragma mark -

Boolean GetClickState(int mouseButton)
{
	if (mouseButton >= NUM_SUPPORTED_MOUSE_BUTTONS)
		return false;
	return 0 != (gMouseButtonStates[mouseButton] & KEYSTATE_ACTIVE_BIT);
}

Boolean GetNewClickState(int mouseButton)
{
	if (mouseButton >= NUM_SUPPORTED_MOUSE_BUTTONS)
		return false;
	return gMouseButtonStates[mouseButton] == KEYSTATE_PRESSED;
}

#pragma mark -

Boolean GetNeedState(int needID, int playerID)
{
	const Controller* controller = &gControllers[playerID];

	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	if (controller->open && (controller->needStates[needID] & KEYSTATE_ACTIVE_BIT))
	{
		return true;
	}

	// Fallback to KB/M
	if (gNumLocalPlayers <= 1 || controller->fallbackToKeyboard)
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
	const Controller* controller = &gControllers[playerID];

	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	if (controller->open && controller->needStates[needID] == KEYSTATE_PRESSED)
	{
		return true;
	}

	// Fallback to KB/M
    if (gNumLocalPlayers <= 1 || controller->fallbackToKeyboard)
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

Boolean UserWantsOut(void)
{
	return GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| GetNewNeedStateAnyP(kNeed_UIBack)
		|| GetNewNeedStateAnyP(kNeed_UIPause)
//		|| GetNewClickState(SDL_BUTTON_LEFT)
        ;
}

Boolean IsCheatKeyComboDown(void)
{
	// The original Mac version used B-R-I, but some cheap PC keyboards can't register
	// this particular key combo, so C-M-R is available as an alternative.
	return (GetKeyState(SDL_SCANCODE_B) && GetKeyState(SDL_SCANCODE_R) && GetKeyState(SDL_SCANCODE_I))
		|| (GetKeyState(SDL_SCANCODE_C) && GetKeyState(SDL_SCANCODE_M) && GetKeyState(SDL_SCANCODE_R));
}

#pragma mark -

float GetControllerAnalogSteeringAxis(SDL_GameController* sdlController, SDL_GameControllerAxis axis)
{
			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

	float steer = 0; 											// assume no control input

			/* FIRST CHECK ANALOG AXES */

	if (sdlController)
	{
		Sint16 dxRaw = SDL_GameControllerGetAxis(sdlController, axis);

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

	return steer;
}


OGLVector2D GetAnalogSteering(int playerID)
{
	OGLVector2D steer = {0, 0};								// assume no control input

	SDL_GameController* sdlController = SDL_GameControllerFromPlayerIndex(playerID);

			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

			/* FIRST CHECK ANALOG AXES */

	if (sdlController)
	{
		steer.x = GetControllerAnalogSteeringAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTX);
		steer.y = GetControllerAnalogSteeringAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTY);

		if (SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
		{
			steer.x = -1.0f;
		}
		else if (SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
		{
			steer.x = 1.0f;
		}

		if (SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_DPAD_UP))
		{
			steer.y = -1.0f;
		}
		else if (SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
		{
			steer.y = 1.0f;
		}
	}

			/* NEXT CHECK THE DIGITAL KEYS */

	if (GetNeedState(kNeed_Left, playerID))					// is Left Key pressed?
	{
		steer.x = -1.0f;
	}
	else if (GetNeedState(kNeed_Right, playerID))			// is Right Key pressed?
	{
		steer.x = 1.0f;
	}

	if (GetNeedState(kNeed_Forward, playerID))					// is Up Key pressed?  (up key makes submarine dive deeper)
	{
		steer.y = -1.0f;
	}
	else if (GetNeedState(kNeed_Backward, playerID))			// is Down Key pressed?  (down key makes submarine float up)
	{
		steer.y = 1.0f;
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

SDL_GameController* GetController(int n)
{
	if (gControllers[n].open)
	{
		return gControllers[n].controllerInstance;
	}
	else
	{
		return NULL;
	}
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

static int GetControllerSlotFromJoystick(int joystickIndex)
{
	SDL_JoystickID joystickInstanceID = SDL_JoystickGetDeviceInstanceID(joystickIndex);

	for (int controllerSlot = 0; controllerSlot < MAX_LOCAL_PLAYERS; controllerSlot++)
	{
		if (gControllers[controllerSlot].open &&
			gControllers[controllerSlot].joystickInstance == joystickInstanceID)
		{
			return controllerSlot;
		}
	}

	return -1;
}

static SDL_GameController* TryOpenControllerFromJoystick(int joystickIndex)
{
	int controllerSlot = -1;

	// First, check that it's not in use already
	controllerSlot = GetControllerSlotFromJoystick(joystickIndex);
	if (controllerSlot >= 0)	// in use
	{
		return gControllers[controllerSlot].controllerInstance;
	}
	
	// If we can't get an SDL_GameController from that joystick, don't bother
	if (!SDL_IsGameController(joystickIndex))
	{
		return NULL;
	}

	// Reserve a controller slot
	controllerSlot = FindFreeControllerSlot();
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
		.joystickInstance = SDL_JoystickGetDeviceInstanceID(joystickIndex),
	};

	printf("Opened joystick %d as controller: %s\n",
		gControllers[controllerSlot].joystickInstance,
		SDL_GameControllerName(gControllers[controllerSlot].controllerInstance));

	return gControllers[controllerSlot].controllerInstance;
}

static SDL_GameController* TryOpenAnyUnusedController(bool showMessage)
{
	int numJoysticks = SDL_NumJoysticks();
	int numJoysticksAlreadyInUse = 0;

	if (numJoysticks == 0)
	{
		return NULL;
	}

	for (int i = 0; i < numJoysticks; ++i)
	{
		// Usable as an SDL GameController?
		if (!SDL_IsGameController(i))
		{
			continue;
		}

		// Already in use?
		if (GetControllerSlotFromJoystick(i) >= 0)
		{
			numJoysticksAlreadyInUse++;
			continue;
		}

		// Use this one
		SDL_GameController* newController = TryOpenControllerFromJoystick(i);
		if (newController)
		{
			return newController;
		}
	}

	if (numJoysticksAlreadyInUse == numJoysticks)
	{
		// All joysticks already in use
		return NULL;
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

static void MoveController(int oldSlot, int newSlot)
{
	if (oldSlot == newSlot)
		return;

	printf("Remapped player controller %d ---> %d\n", oldSlot, newSlot);

	gControllers[newSlot] = gControllers[oldSlot];
	
	// TODO: Does this actually work??
	if (gControllers[newSlot].open)
	{
		SDL_GameControllerSetPlayerIndex(gControllers[newSlot].controllerInstance, newSlot);
	}

	// Clear duplicate slot so we don't read it by mistake in the future
	gControllers[oldSlot].controllerInstance = NULL;
	gControllers[oldSlot].joystickInstance = -1;
	gControllers[oldSlot].open = false;
}

static void CompactControllerSlots(void)
{
	int writeIndex = 0;

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		GAME_ASSERT(writeIndex <= i);

		if (gControllers[i].open)
		{
			MoveController(i, writeIndex);
			writeIndex++;
		}
	}
}

static void TryFillUpVacantControllerSlots(void)
{
	while (TryOpenAnyUnusedController(false) != NULL)
	{
		// Successful; there might be more joysticks available, keep going
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
	TryFillUpVacantControllerSlots();
}

void LockPlayerControllerMapping(void)
{
	int keyboardPlayer = gNumLocalPlayers-1;

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		gControllers[i].fallbackToKeyboard = (i == keyboardPlayer);
	}

	gControllerPlayerMappingLocked = true;
}

void UnlockPlayerControllerMapping(void)
{
	gControllerPlayerMappingLocked = false;
	CompactControllerSlots();
	TryFillUpVacantControllerSlots();
}

const char* GetPlayerName(int whichPlayer)
{
	static char playerName[64];

	snprintf(playerName, sizeof(playerName),
			"%s %d", Localize(STR_PLAYER), whichPlayer+1);

	return playerName;
}

const char* GetPlayerNameWithInputDeviceHint(int whichPlayer)
{
	static char playerName[128];

	playerName[0] = '\0';

	snprintfcat(playerName, sizeof(playerName),
			"%s %d", Localize(STR_PLAYER), whichPlayer+1);

	if (gGameMode == GAME_MODE_CAPTUREFLAG)
	{
		snprintfcat(playerName, sizeof(playerName),
			", %s", Localize(gPlayerInfo[whichPlayer].team == 0 ? STR_RED_TEAM : STR_GREEN_TEAM));
	}

	bool enoughControllers = GetNumControllers() >= gNumLocalPlayers;

	if (!enoughControllers)
	{
		bool hasGamepad = gControllers[whichPlayer].open;
		snprintfcat(playerName, sizeof(playerName),
					"\n[%s]", Localize(hasGamepad? STR_GAMEPAD: STR_KEYBOARD));
	}

	return playerName;
}

#pragma mark -

void ResetDefaultKeyboardBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		memcpy(gGamePrefs.keys[i].key, kDefaultKeyBindings[i].key, sizeof(gGamePrefs.keys[i].key));
	}
}

void ResetDefaultGamepadBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		memcpy(gGamePrefs.keys[i].gamepad, kDefaultKeyBindings[i].gamepad, sizeof(gGamePrefs.keys[i].gamepad));
	}
}

void ResetDefaultMouseBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		gGamePrefs.keys[i].mouseButton = kDefaultKeyBindings[i].mouseButton;
	}
}
