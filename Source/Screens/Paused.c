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

/****************************/
/*    PROTOTYPES            */
/****************************/

static void OnToggleSplitscreenMode(const MenuItem* mi);


/****************************/
/*    CONSTANTS             */
/****************************/

static const MenuItem
	gMenuPause[] =
	{
		{ kMenuItem_Pick, STR_RESUME_GAME, .id=0 },
		{ kMenuItem_Pick, STR_RETIRE_GAME, .id=1 },
		{ kMenuItem_CMRCycler, STR_SPLITSCREEN_MODE,
			.callback = OnToggleSplitscreenMode,
			.cycler =
			{
				.valuePtr = &gGamePrefs.desiredSplitScreenMode,
				.choices =
				{
					{ .text = STR_SPLITSCREEN_HORIZ, .value = SPLITSCREEN_MODE_2X1 },
					{ .text = STR_SPLITSCREEN_VERT, .value = SPLITSCREEN_MODE_1X2 },
				},
			}
		},
		{ kMenuItem_Pick, STR_QUIT_APPLICATION, .id=2 },
		{ .type=kMenuItem_END_SENTINEL },
	};


/*********************/
/*    VARIABLES      */
/*********************/

Boolean gGamePaused = false;


/****************** TOGGLE SPLIT-SCREEN MODE ********************/

void OnToggleSplitscreenMode(const MenuItem* mi)
{
	gActiveSplitScreenMode = gGamePrefs.desiredSplitScreenMode;
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

static void DrawPausedMenuCallback(OGLSetupOutputType* setupInfo)
{

	DrawTerrain(setupInfo);
}

void DoPaused(void)
{
	Boolean	oldMute = gMuteMusicFlag;

	MenuStyle style = kDefaultMenuStyle;
	style.canBackOutOfRootMenu = true;
	style.fadeOutSceneOnExit = false;

	PushKeys();										// save key state so things dont get de-synced during net games

	if (!gMuteMusicFlag)							// see if pause music
		ToggleMusic();

	gGamePaused = true;

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();
	
	int outcome = StartMenu(
		gMenuPause,
		&style,
		UpdatePausedMenuCallback,
		DrawPausedMenuCallback
		);

	gGamePaused = false;
	
	PopKeys();										// restore key state

	if (!oldMute)									// see if restart music
		ToggleMusic();

	switch (outcome)
	{
		case	0:								// RESUME
			break;

		case	1:								// EXIT
			gGameOver = true;
			break;

		case	2:								// QUIT
			CleanQuit();
			break;
	}
}
