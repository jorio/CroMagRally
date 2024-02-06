/****************************/
/*     SETTINGS.C           */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include "game.h"
#include "menu.h"
#include <SDL.h>

static void OnPickLanguage(const MenuItem* mi)
{
	gGamePrefs.language = mi->id;
	LoadLocalizedStrings(gGamePrefs.language);
}

static void OnToggleFullscreen(const MenuItem* mi)
{
	SetFullscreenMode(true);
}

static void OnAdjustMusicVolume(const MenuItem* mi)
{
	UpdateGlobalVolume();
}

static void OnAdjustSFXVolume(const MenuItem* mi)
{
	UpdateGlobalVolume();
}

static void OnPickResetKeyboardBindings(const MenuItem* mi)
{
	MyFlushEvents();
	ResetDefaultKeyboardBindings();
	PlayEffect(EFFECT_BOOM);
	LayoutCurrentMenuAgain();
}

static void OnPickResetGamepadBindings(const MenuItem* mi)
{
	MyFlushEvents();
	ResetDefaultGamepadBindings();
	PlayEffect(EFFECT_BOOM);
	LayoutCurrentMenuAgain();
}

static int GetNumDisplays(void)
{
	return SDL_GetNumVideoDisplays();
}

static const char* GetDisplayName(Byte value)
{
	static char textBuf[8];
	snprintf(textBuf, sizeof(textBuf), "%d", value+1);
	return textBuf;
}

static int ShouldDisplayMonitorCycler(const MenuItem* mi)
{
#if __APPLE__
	// Monitor hot-switching doesn't work too well on macOS,
	// so don't expose the option unless the game was started on a non-default display.

	if (gGamePrefs.monitorNum == 0)
	{
		return kMILayoutFlagHidden;
	}
#endif

	// Expose the option if we have more than one display
	return (GetNumDisplays() <= 1) ? kMILayoutFlagHidden : 0;
}

static int ShouldDisplayMSAA(const MenuItem* mi)
{
#if __APPLE__
	// macOS's OpenGL driver doesn't seem to handle MSAA very well,
	// so don't expose the option unless the game was started with MSAA.

	if (gCurrentAntialiasingLevel)
	{
		return 0;
	}
	else
	{
		return kMILayoutFlagHidden;
	}
#else
	return 0;
#endif
}

static void OnChangeMSAA(const MenuItem* mi)
{
	const long msaaWarningCookie = 'msaa';
	ObjNode* object = GetCurrentMenuItemObject();
	ObjNode* tailNode = GetChainTailNode(object);

	ObjNode* msaaWarning = NULL;
	if (tailNode->Special[0] == msaaWarningCookie)
	{
		msaaWarning = GetChainTailNode(object);
	}

	if (gCurrentAntialiasingLevel == gGamePrefs.antialiasingLevel)
	{
		if (msaaWarning != NULL)
			DeleteObject(msaaWarning);
		return;
	}
	else if (msaaWarning != NULL)
	{
		return;
	}

	NewObjectDefinitionType def =
	{
		.coord = {0, 200, 0},
		.scale = 0.2f,
		.slot = tailNode->Slot + 1,
		.flags = STATUS_BIT_OVERLAYPANE | STATUS_BIT_MOVEINPAUSE,
	};

	msaaWarning = TextMesh_New(Localize(STR_ANTIALIASING_CHANGE_WARNING), 0, &def);
	msaaWarning->ColorFilter = (OGLColorRGBA){ 1, 0, 0, 1 };
	msaaWarning->Special[0] = msaaWarningCookie;

	AppendNodeToChain(object, msaaWarning);

	MakeTwitch(msaaWarning, kTwitchPreset_MenuSelect);
}

const MenuItem gSettingsMenuTree[] =
{
	{.id='sett'},
	{kMIPick, STR_CONTROLS, .next='ctrl'},
	{kMIPick, STR_SOUND, .next='soun'},
	{kMIPick, STR_GRAPHICS, .next='graf'},
	{kMIPick, STR_LANGUAGE, .next='lang'},

	{.id='ctrl'},
	{kMIPick, STR_CONFIGURE_KEYBOARD, .next='keyb' },
	{kMIPick, STR_CONFIGURE_GAMEPAD, .next='gpad' },

	{.id='soun'},
	{
		kMICycler1, STR_MUSIC,
		.callback=OnAdjustMusicVolume,
		.cycler=
		{
			.valuePtr=&gGamePrefs.musicVolumePercent,
			.choices=
			{
				{STR_VOLUME_000, 0},
				{STR_VOLUME_020, 20},
				{STR_VOLUME_040, 40},
				{STR_VOLUME_060, 60},
				{STR_VOLUME_080, 80},
				{STR_VOLUME_100, 100},
			}
		},
	},
	{
		kMICycler1, STR_SFX,
		.callback=OnAdjustSFXVolume,
		.cycler=
		{
			.valuePtr=&gGamePrefs.sfxVolumePercent,
			.choices=
			{
				{STR_VOLUME_000, 0},
				{STR_VOLUME_020, 20},
				{STR_VOLUME_040, 40},
				{STR_VOLUME_060, 60},
				{STR_VOLUME_080, 80},
				{STR_VOLUME_100, 100},
			}
		},
	},

	{.id='graf'},
	{
		kMICycler1, STR_FULLSCREEN,
		.callback=OnToggleFullscreen,
		.cycler=
		{
			.valuePtr=&gGamePrefs.fullscreen,
			.choices={ {STR_OFF, 0}, {STR_ON, 1} },
		},
	},
	{
		kMICycler1, STR_PREFERRED_DISPLAY,
		.callback = OnToggleFullscreen,
		.getLayoutFlags = ShouldDisplayMonitorCycler,
		.cycler =
		{
			.valuePtr = &gGamePrefs.monitorNum,
			.isDynamicallyGenerated = true,
			.generator =
			{
				.generateNumChoices = GetNumDisplays,
				.generateChoiceString = GetDisplayName,
			},
		},
	},
	{
		kMICycler1, STR_ANTIALIASING,
		.callback = OnChangeMSAA,
		.getLayoutFlags = ShouldDisplayMSAA,
		.cycler =
		{
			.valuePtr = &gGamePrefs.antialiasingLevel,
			.choices =
			{
				{STR_OFF, 0},
				{STR_MSAA_2X, 1},
				{STR_MSAA_4X, 2},
				{STR_MSAA_8X, 3},
			},
		},
	},
	{kMISpacer, .customHeight=.5f },
	{kMILabel, STR_FULLSCREEN_HINT, .customHeight=.5f },
	{kMISpacer, .customHeight=.5f },

	{.id='lang'},
	{kMIPick, STR_ENGLISH,	.id=LANGUAGE_ENGLISH,	.callback=OnPickLanguage,	.next='BACK'},
	{kMIPick, STR_FRENCH,	.id=LANGUAGE_FRENCH,	.callback=OnPickLanguage,	.next='BACK'},
	{kMIPick, STR_GERMAN,	.id=LANGUAGE_GERMAN,	.callback=OnPickLanguage,	.next='BACK'},
	{kMIPick, STR_SPANISH,	.id=LANGUAGE_SPANISH,	.callback=OnPickLanguage,	.next='BACK'},
	{kMIPick, STR_ITALIAN,	.id=LANGUAGE_ITALIAN,	.callback=OnPickLanguage,	.next='BACK'},
	{kMIPick, STR_SWEDISH,	.id=LANGUAGE_SWEDISH,	.callback=OnPickLanguage,	.next='BACK'},

	{ .id='keyb' },
	{kMISpacer, .customHeight=.35f },
	{kMILabel, STR_CONFIGURE_KEYBOARD_HELP, .customHeight=.5f },
	{kMISpacer, .customHeight=.4f },
	{kMIKeyBinding, .inputNeed=kNeed_Forward },
	{kMIKeyBinding, .inputNeed=kNeed_Backward },
	{kMIKeyBinding, .inputNeed=kNeed_Left },
	{kMIKeyBinding, .inputNeed=kNeed_Right },
	{kMIKeyBinding, .inputNeed=kNeed_Brakes },
	{kMIKeyBinding, .inputNeed=kNeed_ThrowForward },
	{kMIKeyBinding, .inputNeed=kNeed_ThrowBackward },
	{kMIKeyBinding, .inputNeed=kNeed_CameraMode },
	{kMIKeyBinding, .inputNeed=kNeed_RearView },
	{kMISpacer, .customHeight=.25f },
	{kMIPick, STR_RESET_KEYBINDINGS, .callback=OnPickResetKeyboardBindings, .customHeight=.5f },

	{ .id='gpad' },
	{kMISpacer, .customHeight=.35f },
	{kMILabel, STR_CONFIGURE_GAMEPAD_HELP, .customHeight=.5f },
	{kMISpacer, .customHeight=.4f },
	{kMIPadBinding, .inputNeed=kNeed_Forward },
	{kMIPadBinding, .inputNeed=kNeed_Backward },
	{kMIPadBinding, .inputNeed=kNeed_Left },
	{kMIPadBinding, .inputNeed=kNeed_Right },
	{kMIPadBinding, .inputNeed=kNeed_Brakes },
	{kMIPadBinding, .inputNeed=kNeed_ThrowForward },
	{kMIPadBinding, .inputNeed=kNeed_ThrowBackward },
	{kMIPadBinding, .inputNeed=kNeed_CameraMode },
	{kMIPadBinding, .inputNeed=kNeed_RearView },
	{kMISpacer, .customHeight=.25f },
	{kMIPick, STR_RESET_KEYBINDINGS, .callback=OnPickResetGamepadBindings, .customHeight=.5f },

	{ .id=0 },
};

void RegisterSettingsMenu(const MenuItem* mi)
{
	RegisterMenu(gSettingsMenuTree);
}
