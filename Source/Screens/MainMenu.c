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
#include "version.h"

#include <SDL.h>

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupMainMenuScreen(void);

static void OnPickQuitApplication(const MenuItem* mi);
static void OnPickCredits(const MenuItem* mi);
static void OnConfirmPlayMenu(const MenuItem* mi);
static void OnPickGameMode(const MenuItem* mi);
static void OnPickTournamentAge(const MenuItem* mi);
static void OnPickHostOrJoin(const MenuItem* mi);
static void OnPickLanguage(const MenuItem* mi);
static void OnToggleFullscreen(const MenuItem* mi);
static void OnAdjustMusicVolume(const MenuItem* mi);
static void OnAdjustSFXVolume(const MenuItem* mi);
static void OnPickClearSavedGame(const MenuItem* mi);
static void OnPickTagDuration(const MenuItem* mi);

static int IsClearSavedGameAvailable(const MenuItem* mi);
static int IsTournamentAgeAvailable(const MenuItem* mi);
static int GetLayoutFlagsForTournamentObjective(const MenuItem* mi);

/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	MENU_EXITCODE_HELP		= 1000,
	MENU_EXITCODE_CREDITS,
	MENU_EXITCODE_SCOREBOARD,
	MENU_EXITCODE_PHYSICS,
	MENU_EXITCODE_SELFRUNDEMO,
	MENU_EXITCODE_QUITGAME,
};

#define	DEMO_DELAY	20.0f			// # seconds of idle until self-running demo kicks in


/*********************/
/*    VARIABLES      */
/*********************/

static const MenuItem gMainMenuTree[] =
{
	{ .id='titl' },
	{kMIPick, STR_NEW_GAME,		.next='play', },
	{kMIPick, STR_OPTIONS,		.next='optn', },
	{kMIPick, STR_EXTRAS,		.next='xtra', },
	{kMIPick, STR_QUIT,			.next='EXIT', .callback=OnPickQuitApplication, .id=MENU_EXITCODE_QUITGAME },

	{ .id='play' },
	{kMIPick, STR_1PLAYER,	.id=1, .callback=OnConfirmPlayMenu, .next='spgm' },
	{kMIPick, STR_2PLAYER,	.id=2, .callback=OnConfirmPlayMenu, .next='mpgm' },
	{kMIPick, STR_3PLAYER,	.id=3, .callback=OnConfirmPlayMenu, .next='mpgm' },
	{kMIPick, STR_4PLAYER,	.id=4, .callback=OnConfirmPlayMenu, .next='mpgm' },
	{kMIPick, STR_NET_GAME,	.id=0, .callback=OnConfirmPlayMenu, .next='netg' },

	{ .id='optn' },
	{
		kMICycler1, STR_DIFFICULTY, .cycler=
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
	{kMIPick, STR_SETTINGS, .callback=RegisterSettingsMenu, .next='sett' },
	{kMIPick, STR_CLEAR_SAVED_GAME, .next='clrs', .getLayoutFlags=IsClearSavedGameAvailable, .customHeight=.7 },

	{ .id='xtra' },
	{kMIPick, STR_SCOREBOARD,		.id=MENU_EXITCODE_SCOREBOARD,	.next='EXIT' },
	{kMIPick, STR_PHYSICS_EDITOR,	.id=MENU_EXITCODE_PHYSICS,	.next='EXIT' },
	{kMIPick, STR_CREDITS,			.id=MENU_EXITCODE_CREDITS,	.next='EXIT', .callback=OnPickCredits },
	{kMIPick, STR_HELP,				.id=MENU_EXITCODE_HELP,		.next='EXIT' },

	{ .id='mpgm' },
	{kMIPick, STR_RACE,			.callback=OnPickGameMode, .id=GAME_MODE_MULTIPLAYERRACE,	.next='EXIT' },
	{kMIPick, STR_TAG1,			.callback=OnPickGameMode, .id=GAME_MODE_TAG1,				.next='tag1' },
	{kMIPick, STR_TAG2,			.callback=OnPickGameMode, .id=GAME_MODE_TAG2,				.next='tag2' },
	{kMIPick, STR_SURVIVAL,		.callback=OnPickGameMode, .id=GAME_MODE_SURVIVAL,			.next='EXIT' },
	{kMIPick, STR_CAPTUREFLAG,	.callback=OnPickGameMode, .id=GAME_MODE_CAPTUREFLAG,		.next='EXIT' },

	{ .id='spgm' },
	{kMIPick, STR_PRACTICE,		.callback=OnPickGameMode, .id=GAME_MODE_PRACTICE,			.next='EXIT' },
	{kMIPick, STR_TOURNAMENT,	.callback=OnPickGameMode, .id=GAME_MODE_TOURNAMENT,			.next='tour' },

	{ .id='tour' },
	{kMILabel, .text=STR_TOURNAMENT_OBJECTIVE,		.getLayoutFlags=GetLayoutFlagsForTournamentObjective },
	{kMILabel, .text=STR_TOURNAMENT_OBJECTIVE_EASY,	.getLayoutFlags=GetLayoutFlagsForTournamentObjective },
	{kMISpacer, .customHeight=1.0f},
	{kMIPick, STR_STONE_AGE,	.callback=OnPickTournamentAge, .id=STONE_AGE,	.next='EXIT'},
	{kMIPick, STR_BRONZE_AGE,	.callback=OnPickTournamentAge, .id=BRONZE_AGE,	.next='EXIT', .getLayoutFlags=IsTournamentAgeAvailable},
	{kMIPick, STR_IRON_AGE,		.callback=OnPickTournamentAge, .id=IRON_AGE,	.next='EXIT', .getLayoutFlags=IsTournamentAgeAvailable},

	{ .id='netg' },
	{kMIPick, STR_HOST_NET_GAME, .callback=OnPickHostOrJoin, .id=0, .next='mpgm' }, // host gets to select game type
	{kMIPick, STR_JOIN_NET_GAME, .callback=OnPickHostOrJoin, .id=1, .next='EXIT' },

	{ .id='clrs' },
	{kMILabel, .text=STR_CLEAR_SAVED_GAME_TEXT_1 },
	{kMISpacer, .text=STR_NULL, .customHeight=0.5 },
	{kMILabel, .text=STR_CLEAR_SAVED_GAME_TEXT_2 },
	{kMISpacer, .text=STR_NULL, .customHeight=1 },
	{kMIPick, .text=STR_CLEAR_SAVED_GAME_CANCEL, .next='BACK' },
	{kMIPick, .text=STR_CLEAR_SAVED_GAME, .callback=OnPickClearSavedGame, .next='BACK', .customHeight=.7, },

	{ .id='tag1' },
	{kMILabel, .text=STR_KEEPAWAYTAG_HELP },
	{kMISpacer, .text=STR_NULL },
	{kMILabel, .text=STR_TAG_DURATION },
	{kMISpacer, .text=STR_NULL, .customHeight=1 },
	{kMIPick, STR_2_MINUTES, .callback=OnPickTagDuration, .id=2, .next='EXIT' },
	{kMIPick, STR_3_MINUTES, .callback=OnPickTagDuration, .id=3, .next='EXIT' },
	{kMIPick, STR_4_MINUTES, .callback=OnPickTagDuration, .id=4, .next='EXIT' },

	{ .id='tag2' },
	{kMILabel, .text=STR_STAMPEDETAG_HELP },
	{kMISpacer, .text=STR_NULL },
	{kMILabel, .text=STR_TAG_DURATION },
	{kMISpacer, .text=STR_NULL, .customHeight=1 },
	{kMIPick, STR_2_MINUTES, .callback=OnPickTagDuration, .id=2, .next='EXIT' },
	{kMIPick, STR_3_MINUTES, .callback=OnPickTagDuration, .id=3, .next='EXIT' },
	{kMIPick, STR_4_MINUTES, .callback=OnPickTagDuration, .id=4, .next='EXIT' },

	{ .id=0 }
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
	static bool saidYeah = false;

	MoveObjects();

	if (GetCurrentMenu() == 'titl')		// root menu page
	{
				/* INITIATE SRD */

		if (GetMenuIdleTime() > DEMO_DELAY || GetNewKeyState(SDL_SCANCODE_F1))
		{
			gGameView->fadePillarbox = true;
			KillMenu(MENU_EXITCODE_SELFRUNDEMO);
		}

				/* SET 100% TOURNAMENT PROGRESSION */

		else if (IsCheatKeyComboDown() && (!saidYeah || GetNumTracksCompletedTotal() < NUM_RACE_TRACKS))
		{
			saidYeah = true;

			PlayEffect_Parms(EFFECT_ARROWHEAD, FULL_CHANNEL_VOLUME*2, FULL_CHANNEL_VOLUME*2, NORMAL_CHANNEL_RATE);
			SetPlayerProgression(NUM_RACE_TRACKS);

			NewObjectDefinitionType def = {.coord={0, -150, 0}, .slot=SPRITE_SLOT, .scale=1};

			ObjNode* fatText = TextMesh_New("UNLOCKED\nEVERYTHING", 0, &def);
			fatText->StatusBits |= STATUS_BIT_KEEPBACKFACES;

			MakeTwitch(fatText, kTwitchPreset_WeaponFlip);
		}
	}
}


void DoMainMenuScreen(void)
{
do_again:
	SetupMainMenuScreen();



				/*************/
				/* MAIN LOOP */
				/*************/

	MenuStyle style = kDefaultMenuStyle;
	style.yOffset = 16;

	int outcome = StartMenu(gMainMenuTree, &style, UpdateMainMenuScreen, DrawObjects);

			/* SAVE PREFS IF THEY CHANGED */

	SavePrefs();

			/* CLEANUP */

	DeleteAllObjects();
	OGL_DisposeGameView();


			/* RUN NEXT SCREEN */

	switch (outcome)
	{
		case MENU_EXITCODE_HELP:
			DoHelpScreen();
			goto do_again;

		case MENU_EXITCODE_CREDITS:
			DoCreditsScreen();
			goto do_again;

		case MENU_EXITCODE_SCOREBOARD:
			DoScoreboardScreen();
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
	viewDef.view.pillarboxRatio		= PILLARBOX_RATIO_4_3;

	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);
	OGLVector3D_Normalize(&fillDirection2, &fillDirection2);

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= fillColor1;
	viewDef.lights.fillColor[1] 	= fillColor2;

	OGL_SetupGameView(&viewDef);


				/************/
				/* LOAD ART */
				/************/


			/* MAKE BACKGROUND PICTURE OBJECT */

	MakeBackgroundPictureObject(":images:MainMenuBackground.jpg");


	NewObjectDefinitionType versionDef =
	{
		.coord = {-318, -230, 0},
		.scale = .25f,
		.slot = MENU_SLOT
	};
	ObjNode* versionText = TextMesh_New(PROJECT_VERSION, kTextMeshAlignLeft, &versionDef);
	versionText->ColorFilter = (OGLColorRGBA) {.75f, .75f, .75f, .75f};


			/* SETUP TITLE MENU */

	MakeFadeEvent(true);
}




#pragma mark - Menu Callbacks

static void OnPickQuitApplication(const MenuItem* mi)
{
	gGameView->fadeSound = true;
	gGameView->fadePillarbox = true;
	gGameView->fadeOutDuration = .3f;
}

static void OnPickCredits(const MenuItem* mi)
{
	gGameView->fadePillarbox = true;
}

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


static void OnPickClearSavedGame(const MenuItem* mi)
{
	SetPlayerProgression(0);
	SavePlayerFile();
}

static void OnPickTagDuration(const MenuItem* mi)
{
	gGamePrefs.tagDuration = mi->id;
}

#pragma mark -

static int IsClearSavedGameAvailable(const MenuItem* mi)
{
	(void) mi;

	if ((GetNumTracksCompletedTotal() > 0) || IsCheatKeyComboDown())
		return 0;
	else
		return kMILayoutFlagDisabled;
}

static int IsTournamentAgeAvailable(const MenuItem* mi)
{
	if (mi->id <= GetNumAgesCompleted())
		return 0;
	else
		return kMILayoutFlagDisabled;
}

static int GetLayoutFlagsForTournamentObjective(const MenuItem* mi)
{
	bool isEasy = gGamePrefs.difficulty <= DIFFICULTY_EASY;

	if (mi->text == STR_TOURNAMENT_OBJECTIVE_EASY)
		return isEasy? 0: kMILayoutFlagHidden;
	else
		return isEasy? kMILayoutFlagHidden: 0;
}
