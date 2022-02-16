/****************************/
/*   	PAUSED.C			*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "window.h"
#include "input.h"
#include "sound2.h"
#include	"file.h"
#include	"ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "terrain.h"
#include "miscscreens.h"
#include "sobjtypes.h"
#include "sprites.h"
#include "network.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	FSSpec		gDataSpec;
extern	Boolean		gNetGameInProgress,gGameOver,gMuteMusicFlag;
extern	KeyMap gKeyMap,gNewKeys;
extern	short		gNumRealPlayers,gNumLocalPlayers;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds,gIsNetworkHost,gIsNetworkClient;
extern	PrefsType	gGamePrefs;
extern	OGLPoint3D	gCoord;
extern	MOPictureObject 	*gBackgoundPicture;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	Byte				gActiveSplitScreenMode;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void BuildPausedMenu(void);
static Boolean NavigatePausedMenu(void);
static void FreePausedMenu(void);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static short	gPausedMenuSelection;

static ObjNode	*gPausedIcons[3];



static const float	gLineSpacing[NUM_SPLITSCREEN_MODES] =
{
	.12,					// fullscreen
	.2,						// 2 horiz
	.1						// 2 vert
};

static const float	gTextScale[NUM_SPLITSCREEN_MODES] =
{
	.4,					// fullscreen
	.3,					// 2 horiz
	.6					// 2 vert
};


static const OGLColorRGBA gPausedMenuHiliteColor = {.3,.5,.2,1};



/********************** DO PAUSED **************************/

void DoPaused(void)
{
short	i;
Boolean	oldMute = gMuteMusicFlag;

	PushKeys();										// save key state so things dont get de-synced during net games

	if (!gMuteMusicFlag)							// see if pause music
		ToggleMusic();

	BuildPausedMenu();


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (NavigatePausedMenu())
			break;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		DoPlayerTerrainUpdate();							// need to call this to keep supertiles active
		OGL_DrawScene(gGameViewInfoPtr, DrawTerrain);


			/* FADE IN TEXT */

		for (i = 0; i < 3; i++)
		{
			gPausedIcons[i]->ColorFilter.a += gFramesPerSecondFrac * 3.0f;
			if (gPausedIcons[i]->ColorFilter.a > 1.0f)
				gPausedIcons[i]->ColorFilter.a = 1.0;
		}


			/* IF DOING NET GAME, LET OTHER PLAYERS KNOW WE'RE STILL GOING SO THEY DONT TIME OUT */

		if (gNetGameInProgress)
			PlayerBroadcastNullPacket();
	}

	FreePausedMenu();

	PopKeys();										// restore key state

	if (!oldMute)									// see if restart music
		ToggleMusic();

}


/**************************** BUILD PAUSED MENU *********************************/

static void BuildPausedMenu(void)
{
short	i;

	gPausedMenuSelection = 0;



			/* BUILD NEW TEXT STRINGS */

	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 2.0f * gLineSpacing[gActiveSplitScreenMode] / 2;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = gTextScale[gActiveSplitScreenMode];
	gNewObjectDefinition.slot 		= SPRITE_SLOT;

	for (i = 0; i < 3; i++)
	{
		Str255	s;

		GetIndStringC(s, 2000 + gGamePrefs.language, i + 1);

		gPausedIcons[i] = MakeFontStringObject(s, &gNewObjectDefinition, gGameViewInfoPtr, true);
		gPausedIcons[i]->ColorFilter.a = 0;
		gNewObjectDefinition.coord.y 	-= gLineSpacing[gActiveSplitScreenMode];
	}
}


/******************* MOVE PAUSED ICONS **********************/
//
// Called only when done to fade them out
//

static void MovePausedIcons(ObjNode *theNode)
{
	theNode->ColorFilter.a -= gFramesPerSecondFrac * 3.0f;
	if (theNode->ColorFilter.a < 0.0f)
		DeleteObject(theNode);

}



/******************** FREE PAUSED MENU **********************/
//
// Free objects of current menu
//

static void FreePausedMenu(void)
{
int		i;

	for (i = 0; i < 3; i++)
	{
		gPausedIcons[i]->MoveCall = MovePausedIcons;
	}
}


/*********************** NAVIGATE PAUSED MENU **************************/

static Boolean NavigatePausedMenu(void)
{
short	i;
Boolean	continueGame = false;


		/* SEE IF CHANGE SELECTION */

	if (GetNewNeedStateAnyP(kNeed_UIUp) && (gPausedMenuSelection > 0))
	{
		gPausedMenuSelection--;
		PlayEffect(EFFECT_SELECTCLICK);
	}
	else
	if (GetNewNeedStateAnyP(kNeed_UIDown) && (gPausedMenuSelection < 2))
	{
		gPausedMenuSelection++;
		PlayEffect(EFFECT_SELECTCLICK);
	}

		/* SET APPROPRIATE FRAMES FOR ICONS */

	for (i = 0; i < 3; i++)
	{
		if (i != gPausedMenuSelection)
		{
			gPausedIcons[i]->ColorFilter.r = 										// normal
			gPausedIcons[i]->ColorFilter.g =
			gPausedIcons[i]->ColorFilter.b =
			gPausedIcons[i]->ColorFilter.a = 1.0;
		}
		else
		{
			gPausedIcons[i]->ColorFilter = gPausedMenuHiliteColor;										// hilite
		}

	}


			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		switch(gPausedMenuSelection)
		{
			case	0:								// RESUME
					continueGame = true;
					break;

			case	1:								// EXIT
					gGameOver = true;
					continueGame = true;
					break;

			case	2:								// QUIT
					CleanQuit();
					break;

		}
	}


			/*****************************/
			/* SEE IF CANCEL A SELECTION */
			/*****************************/

	else
	if (GetNewNeedStateAnyP(kNeed_UIBack))
	{
		continueGame = true;
	}


	return(continueGame);
}
















