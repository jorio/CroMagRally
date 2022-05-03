/****************************/
/*   	MAINMENU SCREEN.C	*/
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "menu.h"
#include "miscscreens.h"
#include "network.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupMainMenuScreen(void);
static void FreeMainMenuArt(void);

static void OnConfirmPlayMenu(const MenuItem* mi);
static void OnPickGameMode(const MenuItem* mi);
static void OnPickTournamentAge(const MenuItem* mi);
static void OnPickHostOrJoin(const MenuItem* mi);
static void OnPickGameMode(const MenuItem* mi);
static void OnPickLanguage(const MenuItem* mi);
static void OnToggleFullscreen(const MenuItem* mi);
static void OnToggleMusic(const MenuItem* mi);
static void OnPickClearSavedGame(const MenuItem* mi);
static void OnPickTagDuration(const MenuItem* mi);
static void OnPickResetKeyboardBindings(const MenuItem* mi);
static void OnPickResetGamepadBindings(const MenuItem* mi);

static bool IsClearSavedGameAvailable(const MenuItem* mi);
static bool IsTournamentAgeAvailable(const MenuItem* mi);

/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	MENU_ID_NULL			= 0,		// keep ID=0 unused
	MENU_ID_TITLE,
	MENU_ID_PLAY,
	MENU_ID_OPTIONS,
	MENU_ID_EXTRAS,
	MENU_ID_MULTIPLAYERGAMETYPE,
	MENU_ID_1PLAYERGAMETYPE,
	MENU_ID_TOURNAMENT,
	MENU_ID_KEEPAWAYTAG_DURATION,
	MENU_ID_STAMPEDETAG_DURATION,
	MENU_ID_NETGAME,
	MENU_ID_SETTINGS,
	MENU_ID_CONFIRM_CLEAR_SAVE,
	MENU_ID_REMAP_KEYBOARD,
	MENU_ID_REMAP_GAMEPAD,
	NUM_MENU_IDS
};

enum
{
	MENU_EXITCODE_HELP		= 1000,
	MENU_EXITCODE_CREDITS,
	MENU_EXITCODE_PHYSICS,
	MENU_EXITCODE_SELFRUNDEMO,
	MENU_EXITCODE_QUITGAME,
};

#define	DEMO_DELAY	20.0f			// # seconds of idle until self-running demo kicks in


/*********************/
/*    VARIABLES      */
/*********************/

static const MenuItem
	gMenuTitle[] =
	{
		{ kMenuItem_Pick, STR_NEW_GAME, .gotoMenu=MENU_ID_PLAY, },
//		{ kMenuItem_Pick, STR_LOAD_GAME },  // DoSavedPlayerDialog
		{ kMenuItem_Pick, STR_OPTIONS, .gotoMenu=MENU_ID_OPTIONS, },
		{ kMenuItem_Pick, STR_EXTRAS, .gotoMenu=MENU_ID_EXTRAS, },
		{ kMenuItem_Pick, STR_QUIT, .id=MENU_EXITCODE_QUITGAME },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuPlay[] =
	{
		{ kMenuItem_Pick, STR_1PLAYER,	.id=1, .callback=OnConfirmPlayMenu, .gotoMenu=MENU_ID_1PLAYERGAMETYPE },
		{ kMenuItem_Pick, STR_2PLAYER,	.id=2, .callback=OnConfirmPlayMenu, .gotoMenu=MENU_ID_MULTIPLAYERGAMETYPE },
		{ kMenuItem_Pick, STR_3PLAYER,	.id=3, .callback=OnConfirmPlayMenu, .gotoMenu=MENU_ID_MULTIPLAYERGAMETYPE },
		{ kMenuItem_Pick, STR_4PLAYER,	.id=4, .callback=OnConfirmPlayMenu, .gotoMenu=MENU_ID_MULTIPLAYERGAMETYPE },
#if 0	// TODO!
		{ kMenuItem_Pick, STR_NET_GAME,	.id=0, .callback=OnConfirmPlayMenu, .gotoMenu=MENU_ID_NETGAME },
#endif
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuOptions[] =
	{
		{ kMenuItem_Pick, STR_SETTINGS, .gotoMenu=MENU_ID_SETTINGS },
		{ kMenuItem_Pick, STR_CONFIGURE_KEYBOARD, .gotoMenu=MENU_ID_REMAP_KEYBOARD },
		{ kMenuItem_Pick, STR_CONFIGURE_GAMEPAD, .gotoMenu=MENU_ID_REMAP_GAMEPAD },
		{ kMenuItem_Pick, STR_CLEAR_SAVED_GAME, .gotoMenu=MENU_ID_CONFIRM_CLEAR_SAVE, .enableIf=IsClearSavedGameAvailable },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuExtras[] =
	{
		{ kMenuItem_Pick, STR_HELP, .id=MENU_EXITCODE_HELP },
		{ kMenuItem_Pick, STR_CREDITS, .id=MENU_EXITCODE_CREDITS },
		{ kMenuItem_Pick, STR_PHYSICS_EDITOR, .id=MENU_EXITCODE_PHYSICS },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuMPGameModes[] =
	{
		{ kMenuItem_Pick,	STR_RACE,			.callback=OnPickGameMode, .id=GAME_MODE_MULTIPLAYERRACE	},
		{ kMenuItem_Pick,	STR_KEEP_AWAY_TAG,	.callback=OnPickGameMode, .id=GAME_MODE_TAG1,				.gotoMenu=MENU_ID_KEEPAWAYTAG_DURATION, },
		{ kMenuItem_Pick,	STR_STAMPEDE_TAG,	.callback=OnPickGameMode, .id=GAME_MODE_TAG2,				.gotoMenu=MENU_ID_STAMPEDETAG_DURATION, },
		{ kMenuItem_Pick,	STR_SURVIVAL,		.callback=OnPickGameMode, .id=GAME_MODE_SURVIVAL		},
		{ kMenuItem_Pick,	STR_QUEST_FOR_FIRE,	.callback=OnPickGameMode, .id=GAME_MODE_CAPTUREFLAG		},
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenu1PGameModes[] =
	{
		{ kMenuItem_Pick,	STR_PRACTICE,		.callback=OnPickGameMode, .id=GAME_MODE_PRACTICE		 },
		{ kMenuItem_Pick,	STR_TOURNAMENT,		.callback=OnPickGameMode, .id=GAME_MODE_TOURNAMENT,			.gotoMenu=MENU_ID_TOURNAMENT, },
		// ^^^ TODO: if picking tournament, pick saved game file?
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuTournament[] =
	{
		{ kMenuItem_Pick, STR_STONE_AGE,	.callback=OnPickTournamentAge, .id=STONE_AGE,	},
		{ kMenuItem_Pick, STR_BRONZE_AGE,	.callback=OnPickTournamentAge, .id=BRONZE_AGE,	.enableIf=IsTournamentAgeAvailable },
		{ kMenuItem_Pick, STR_IRON_AGE,		.callback=OnPickTournamentAge, .id=IRON_AGE,	.enableIf=IsTournamentAgeAvailable },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuNetGame[] =
	{
		{ kMenuItem_Pick, STR_HOST_NET_GAME, .callback=OnPickHostOrJoin, .id=0, .gotoMenu=MENU_ID_MULTIPLAYERGAMETYPE }, // host gets to select game type
		{ kMenuItem_Pick, STR_JOIN_NET_GAME, .callback=OnPickHostOrJoin, .id=1 },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuConfirmClearSave[] =
	{
		{ kMenuItem_Subtitle, .text=STR_CLEAR_SAVED_GAME_TEXT_1 },
		{ kMenuItem_Spacer, .text=STR_NULL },
		{ kMenuItem_Spacer, .text=STR_NULL },
		{ kMenuItem_Subtitle, .text=STR_CLEAR_SAVED_GAME_TEXT_2 },
		{ kMenuItem_Spacer, .text=STR_NULL },
		{ kMenuItem_Spacer, .text=STR_NULL },
		{ kMenuItem_Pick, .text=STR_CLEAR_SAVED_GAME_CANCEL, .gotoMenu=kGotoMenu_GoBack },
		{ kMenuItem_Pick, .text=STR_CLEAR_SAVED_GAME, .callback=OnPickClearSavedGame, .gotoMenu=kGotoMenu_GoBack },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuKeepAwayTagDuration[] =
	{
		{ kMenuItem_Subtitle, .text = STR_KEEPAWAYTAG_HELP },
		{ kMenuItem_Spacer, .text = STR_NULL },
		{ kMenuItem_Subtitle, .text = STR_TAG_DURATION },
		{ kMenuItem_Spacer, .text = STR_NULL },
		{ kMenuItem_Spacer, .text = STR_NULL },
		{ kMenuItem_Pick, STR_2_MINUTES, .callback=OnPickTagDuration, .id=2 },
		{ kMenuItem_Pick, STR_3_MINUTES, .callback=OnPickTagDuration, .id=3 },
		{ kMenuItem_Pick, STR_4_MINUTES, .callback=OnPickTagDuration, .id=4 },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuStampedeTagDuration[] =
	{
		{ kMenuItem_Subtitle, .text=STR_STAMPEDETAG_HELP },
		{ kMenuItem_Spacer, .text=STR_NULL },
		{ kMenuItem_Subtitle, .text = STR_TAG_DURATION },
		{ kMenuItem_Spacer, .text = STR_NULL },
		{ kMenuItem_Spacer, .text = STR_NULL },
		{ kMenuItem_Pick, STR_2_MINUTES, .callback=OnPickTagDuration, .id=2 },
		{ kMenuItem_Pick, STR_3_MINUTES, .callback=OnPickTagDuration, .id=3 },
		{ kMenuItem_Pick, STR_4_MINUTES, .callback=OnPickTagDuration, .id=4 },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuSettings[] =
	{
		{ 
			kMenuItem_CMRCycler, STR_DIFFICULTY, .cycler=
			{
				.valuePtr=&gGamePrefs.difficulty, .choices=
				{
					{STR_DIFFICULTY_1, DIFFICULTY_SIMPLISTIC},
					{STR_DIFFICULTY_2, DIFFICULTY_EASY},
					{STR_DIFFICULTY_3, DIFFICULTY_MEDIUM},
					{STR_DIFFICULTY_4, DIFFICULTY_HARD},
				}
			}
		},

		{
			kMenuItem_CMRCycler, STR_MUSIC,
			.callback=OnToggleMusic,
			.cycler=
			{
				.valuePtr=&gGamePrefs.music,
				.choices={ {STR_OFF, 0}, {STR_ON, 1} }
			}
		},

		{
			kMenuItem_CMRCycler, STR_FULLSCREEN,
			.callback=OnToggleFullscreen,
			.cycler=
			{
				.valuePtr=&gGamePrefs.fullscreen,
				.choices={ {STR_OFF, 0}, {STR_ON, 1} },
			}
		},

		{
			kMenuItem_CMRCycler, STR_LANGUAGE, .cycler=
			{
				.valuePtr=&gGamePrefs.language, .choices=
				{
					{STR_LANGUAGE_NAME, LANGUAGE_ENGLISH},
					{STR_LANGUAGE_NAME, LANGUAGE_FRENCH},
					{STR_LANGUAGE_NAME, LANGUAGE_GERMAN},
					{STR_LANGUAGE_NAME, LANGUAGE_SPANISH},
					{STR_LANGUAGE_NAME, LANGUAGE_ITALIAN},
					{STR_LANGUAGE_NAME, LANGUAGE_SWEDISH},
				}
			},
			.callback=OnPickLanguage,
		},

		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuRemapKeyboard[] =
	{
		{ kMenuItem_Spacer, .customHeight=.2f },
		{ kMenuItem_Subtitle, STR_CONFIGURE_KEYBOARD_HELP, .customHeight=.5f },
		{ kMenuItem_Spacer, .customHeight=.4f },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_Forward },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_Backward },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_Left },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_Right },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_Brakes },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_ThrowForward },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_ThrowBackward },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_CameraMode },
		{ kMenuItem_KeyBinding, .inputNeed=kNeed_RearView },
		{ kMenuItem_Spacer, .customHeight=.25f },
		{ kMenuItem_Pick, STR_RESET_KEYBINDINGS, .callback=OnPickResetKeyboardBindings, .gotoMenu=kGotoMenu_NoOp, .customHeight=.5f },
		{ .type=kMenuItem_END_SENTINEL },
	},

	gMenuRemapGamepad[] =
	{
		{ kMenuItem_Spacer, .customHeight=.2f },
		{ kMenuItem_Subtitle, STR_CONFIGURE_GAMEPAD_HELP, .customHeight=.5f },
		{ kMenuItem_Spacer, .customHeight=.4f },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_Forward },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_Backward },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_Left },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_Right },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_Brakes },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_ThrowForward },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_ThrowBackward },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_CameraMode },
		{ kMenuItem_PadBinding, .inputNeed=kNeed_RearView },
		{ kMenuItem_Spacer, .customHeight=.25f },
		{ kMenuItem_Pick, STR_RESET_KEYBINDINGS, .callback=OnPickResetGamepadBindings, .gotoMenu=kGotoMenu_NoOp, .customHeight=.5f },
		{ .type=kMenuItem_END_SENTINEL },
	}

	;



static const MenuItem* gMainMenuTree[NUM_MENU_IDS] =
{
	[MENU_ID_TITLE] = gMenuTitle,
	[MENU_ID_PLAY] = gMenuPlay,
	[MENU_ID_OPTIONS] = gMenuOptions,
	[MENU_ID_EXTRAS] = gMenuExtras,
	[MENU_ID_MULTIPLAYERGAMETYPE] = gMenuMPGameModes,
	[MENU_ID_1PLAYERGAMETYPE] = gMenu1PGameModes,
	[MENU_ID_TOURNAMENT] = gMenuTournament,
	[MENU_ID_KEEPAWAYTAG_DURATION] = gMenuKeepAwayTagDuration,
	[MENU_ID_STAMPEDETAG_DURATION] = gMenuStampedeTagDuration,
	[MENU_ID_NETGAME] = gMenuNetGame,
	[MENU_ID_SETTINGS] = gMenuSettings,
	[MENU_ID_CONFIRM_CLEAR_SAVE] = gMenuConfirmClearSave,
	[MENU_ID_REMAP_KEYBOARD] = gMenuRemapKeyboard,
	[MENU_ID_REMAP_GAMEPAD] = gMenuRemapGamepad,
};



/********************** PRIME SELF-RUNNING DEMO **************************/

static void PrimeSelfRunningDemo(void)
{
	gIsSelfRunningDemo = true;
	gIsNetworkHost = false;								// assume I'm not hosting
	gIsNetworkClient = false;							// assume I'm not joining either
	gNetGameInProgress = false;
	gNumLocalPlayers = 1;								// assume just 1 local player on this machine
	gNumRealPlayers = 1;
	gGameMode = GAME_MODE_PRACTICE;
}




/********************** DO MAINMENU SCREEN **************************/

static void UpdateMainMenuScreen(void)
{
	MoveObjects();

	if (GetCurrentMenu() == gMenuTitle &&
		(GetMenuIdleTime() > DEMO_DELAY || IsCheatKeyComboDown()))
	{
		gGameViewInfoPtr->fadePillarbox = true;
		KillMenu(MENU_EXITCODE_SELFRUNDEMO);
	}
}


void DoMainMenuScreen(void)
{
do_again:
	SetupMainMenuScreen();



				/*************/
				/* MAIN LOOP */
				/*************/

	PrefsType oldPrefs;
	memcpy(&oldPrefs, &gGamePrefs, sizeof(oldPrefs));

	int outcome = StartMenuTree(gMainMenuTree, NULL, UpdateMainMenuScreen, DrawObjects);

			/* SAVE PREFS IF THEY CHANGED */
	
	if (0 != memcmp(&oldPrefs, &gGamePrefs, sizeof(oldPrefs)))
	{
		puts("Saving prefs");
		SavePrefs();
	}

			/* CLEANUP */

	FreeMainMenuArt();

	switch (outcome)
	{
		case MENU_EXITCODE_HELP:
			DoHelpScreen();
			goto do_again;

		case MENU_EXITCODE_CREDITS:
			DoCreditsScreen();
			goto do_again;
		
		case MENU_EXITCODE_PHYSICS:
			DoPhysicsEditor();
			goto do_again;
		
		case MENU_EXITCODE_SELFRUNDEMO:
			PrimeSelfRunningDemo();
			break;
		
		case MENU_EXITCODE_QUITGAME:
			CleanQuit();
			break;
	}

		/******************************/
		/* HANDLE GAME MODE SPECIFICS */
		/******************************/

	switch(gGameMode)
	{
			/* BATTLE MODE:  SELECT TRACK NOW */

		case	GAME_MODE_MULTIPLAYERRACE:
		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
		case	GAME_MODE_SURVIVAL:
		case	GAME_MODE_CAPTUREFLAG:
				if (!gIsNetworkClient)				// clients dont select tracks since they get the info when they join
				{
					if (SelectSingleTrack())
						goto do_again;
				}
				break;
	}



			/********************/
			/* DO NETWORK SETUP */
			/********************/
			//
			// Let's do this all now so that we can easily abort out and so we can transfer the
			// necessary information about the game from the Host to the other players.
			//

	if (gNetGameInProgress)
	{
		if (gIsNetworkHost)
		{
			if (SetupNetworkHosting())						// do hosting dialog and check for abort
				goto do_again;
		}
		else
		{
			if (SetupNetworkJoin())							// get joining info and check for abort
				goto do_again;
		}
	}
	else
		gMyNetworkPlayerNum = 0;
}


/********************* SETUP MAINMENU SCREEN **********************/

static void SetupMainMenuScreen(void)
{
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .1, .1, .1, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLColorRGBA		fillColor2 = { .3, .3, .3, 1 };
OGLVector3D			fillDirection1 = { .9, -.7, -1 };
OGLVector3D			fillDirection2 = { -1, -.2, -.5 };


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= 1.0;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor 		= (OGLColorRGBA) {.76f, .61f, .45f, 1.0f};
	viewDef.styles.useFog			= false;
	viewDef.view.pillarbox4x3		= true;

	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);
	OGLVector3D_Normalize(&fillDirection2, &fillDirection2);

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= fillColor1;
	viewDef.lights.fillColor[1] 	= fillColor2;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/


			/* MAKE BACKGROUND PICTURE OBJECT */

	MakeBackgroundPictureObject(":images:MainMenuBackground.jpg");


			/* SETUP TITLE MENU */

	MakeFadeEvent(true);
}


/********************** FREE MAINMENU ART **********************/

static void FreeMainMenuArt(void)
{
	DeleteAllObjects();
	DisposeAllSpriteGroups();
}


#pragma mark - Menu Callbacks

static void OnConfirmPlayMenu(const MenuItem* mi)
{
	gIsNetworkHost = false;								// assume I'm not hosting
	gIsNetworkClient = false;							// assume I'm not joining either
	gGameMode = -1;										// no game mode selected yet

	switch (mi->id)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			gNetGameInProgress = false;
			gNumLocalPlayers = mi->id;					// assume only local player(s) on this machine
			gNumRealPlayers = mi->id;
			break;

		case 0:
			gNetGameInProgress = true;
			gNumRealPlayers = 1;
			gNumLocalPlayers = 1;
			break;

		default:
			DoFatalAlert("Unsupported pick ID in play menu: %d", mi->id);
	}
}

static void OnPickGameMode(const MenuItem* mi)
{
	gGameMode = GAME_CLAMP(mi->id, 0, NUM_GAME_MODES);
}

static void OnPickTournamentAge(const MenuItem* mi)
{
	gTheAge = GAME_CLAMP(mi->id, 0, NUM_AGES-1);
}

static void OnPickHostOrJoin(const MenuItem* mi)
{
	switch (mi->text)
	{
		case STR_HOST_NET_GAME:
			gIsNetworkHost = true;
			gIsNetworkClient = false;
			break;

		case STR_JOIN_NET_GAME:
			gIsNetworkHost = false;
			gIsNetworkClient = true;
			break;

		default:
			DoAlert("Unsupported host/join mode: %d", mi->text);
			break;
	}
}

static void OnPickLanguage(const MenuItem* mi)
{
	LoadLocalizedStrings(gGamePrefs.language);
	LayoutCurrentMenuAgain();
}

static void OnToggleFullscreen(const MenuItem* mi)
{
	SetFullscreenMode(true);
}

static void OnToggleMusic(const MenuItem* mi)
{
	if ((!gMuteMusicFlag) != gGamePrefs.music)
	{
		ToggleMusic();
	}
}

static void OnPickClearSavedGame(const MenuItem* mi)
{
	SetPlayerProgression(0);
	SavePlayerFile();
}

static void OnPickTagDuration(const MenuItem* mi)
{
	gGamePrefs.tagDuration = mi->id;
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

#pragma mark -

static bool IsClearSavedGameAvailable(const MenuItem* mi)
{
	(void) mi;
	return GetNumTracksCompletedTotal() > 0;
}

static bool IsTournamentAgeAvailable(const MenuItem* mi)
{
	return mi->id <= GetNumAgesCompleted();
}
