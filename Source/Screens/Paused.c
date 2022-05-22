/****************************/
/*   	PAUSED.C			*/
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "menu.h"
#include "network.h"
#include "miscscreens.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static int ShouldDisplaySplitscreenModeCycler(const MenuItem* mi);
static void OnToggleSplitscreenMode(const MenuItem* mi);


/****************************/
/*    CONSTANTS             */
/****************************/

static const MenuItem gPauseMenuTree[] =
{
	{ .id='paus' },

	{kMIPick, STR_RESUME_GAME, .id='resu', .next='EXIT' },

	{kMIPick, STR_SETTINGS, .callback=RegisterSettingsMenu, .next='sett' },

	// 2P split-screen mode chooser
	{
		.type = kMICycler1,
		.text = STR_SPLITSCREEN_MODE,
		.id = 2,	// ShouldDisplaySplitscreenModeCycler looks at this ID to know it's meant for 2P
		.getLayoutFlags = ShouldDisplaySplitscreenModeCycler,
		.callback = OnToggleSplitscreenMode,
		.cycler =
		{
			.valuePtr = &gGamePrefs.splitScreenMode2P,
			.choices =
			{
				{ .text = STR_SPLITSCREEN_HORIZ, .value = SPLITSCREEN_MODE_2P_TALL },
				{ .text = STR_SPLITSCREEN_VERT, .value = SPLITSCREEN_MODE_2P_WIDE },
			},
		},
	},

	// 3P split-screen mode chooser
	{
		.type = kMICycler1,
		.text = STR_SPLITSCREEN_MODE,
		.id = 3,	// ShouldDisplaySplitscreenModeCycler looks at this ID to know it's meant for 3P
		.getLayoutFlags = ShouldDisplaySplitscreenModeCycler,
		.callback = OnToggleSplitscreenMode,
		.cycler =
		{
			.valuePtr = &gGamePrefs.splitScreenMode3P,
			.choices =
			{
				{ .text = STR_SPLITSCREEN_HORIZ, .value = SPLITSCREEN_MODE_3P_TALL },
				{ .text = STR_SPLITSCREEN_VERT, .value = SPLITSCREEN_MODE_3P_WIDE },
			},
		},
	},


	{kMIPick, STR_RETIRE_GAME, .id='bail', .next='EXIT' },

	{kMIPick, STR_QUIT_APPLICATION, .id='quit', .next='EXIT' },

	{ 0 },
};


/*********************/
/*    VARIABLES      */
/*********************/

Boolean gGamePaused = false;


/****************** TOGGLE SPLIT-SCREEN MODE ********************/

int ShouldDisplaySplitscreenModeCycler(const MenuItem* mi)
{
	if (gNumSplitScreenPanes == mi->id)
		return 0;
	else
		return kMILayoutFlagHidden | kMILayoutFlagDisabled;
}

void OnToggleSplitscreenMode(const MenuItem* mi)
{
	switch (gNumSplitScreenPanes)
	{
		case 2:
			gActiveSplitScreenMode = gGamePrefs.splitScreenMode2P;
			break;

		case 3:
			gActiveSplitScreenMode = gGamePrefs.splitScreenMode3P;
			break;

		default:
			printf("%s: what am I supposed to do with %d split-screen panes?\n", __func__, gNumSplitScreenPanes);
	}
}


/********************** DO PAUSED **************************/

static void UpdatePausedMenuCallback(void)
{
	MoveObjects();
	DoPlayerTerrainUpdate();							// need to call this to keep supertiles active


			/* IF DOING NET GAME, LET OTHER PLAYERS KNOW WE'RE STILL GOING SO THEY DONT TIME OUT */

	if (gNetGameInProgress)
		PlayerBroadcastNullPacket();
}

void DoPaused(void)
{
	MenuStyle style = kDefaultMenuStyle;
	style.canBackOutOfRootMenu = true;
	style.fadeOutSceneOnExit = false;
	style.darkenPaneOpacity = .6f;
	style.labelColor = (OGLColorRGBA){.7,.7,.7,1};
	style.startButtonExits = true;

	PushKeys();										// save key state so things dont get de-synced during net games

	PauseAllChannels(true);

	gGamePaused = true;
	gHideInfobar = true;

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	int outcome = StartMenu(gPauseMenuTree, &style, UpdatePausedMenuCallback, DrawTerrain);

	gGamePaused = false;
	PauseAllChannels(false);
	
	PopKeys();										// restore key state

	switch (outcome)
	{
		case	'resu':								// RESUME
		default:
			gHideInfobar = false;
			break;

		case	'bail':								// EXIT
			gGameOver = true;
			break;

		case	'quit':								// QUIT
			CleanQuit();
			break;
	}
}
