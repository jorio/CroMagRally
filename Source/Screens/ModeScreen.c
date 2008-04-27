/****************************/
/*   	GAMEMODE.C 	    */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "miscscreens.h"
#include "objects.h"
#include "windows.h"
#include "input.h"
#include "sound2.h"
#include	"file.h"
#include	"ogl_support.h"
#include "ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "bg3d.h"
#include "sobjtypes.h"
#include "modescreen.h"
#include "sprites.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow;
extern	FSSpec		gDataSpec;
extern	Boolean		gLowVRAM;
extern	KeyMap gKeyMap,gNewKeys;
extern	short		gNumRealPlayers,gNumLocalPlayers;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds,gSongPlayingFlag;
extern	PrefsType	gGamePrefs;
extern	OGLPoint3D	gCoord;
extern  MOPictureObject 	*gBackgoundPicture;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	Boolean		gHostNetworkGame,gNetGameInProgress;
extern	Boolean		gJoinNetworkGame;
extern	Byte		gDesiredSplitScreenMode;
extern	int			gNumSplitScreenPanes;

  /****************************/
/*     PROTOTYPES            */
/****************************/

static void DrawGameMode(OGLSetupOutputType *info);
static void SetupGameModeScreen(void);
static void MakeGameModeIcons(void);
static void DoGameModeControls(void);
static void MoveCursor(ObjNode *theNode);
static void ShowModeOptions(void);
static void SelectOption(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define MODE_ICON_SCALE		0.9f

#define	ICON_SPACING		(.6f * MODE_ICON_SCALE)
#define	OPTION_SPACING		(.4f * MODE_ICON_SCALE)


/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode	*gModeIcons[3], *gCursorObj;
static ObjNode	*gOptions[2];

static Byte		gSelectedMode,gSelectedOption;

static const Byte gPartBaseModel[] =
{
	MODE_SObjType_1Player,
	MODE_SObjType_2Player,
	MODE_SObjType_NetPlay
};

static const short gNumOptions[] = {0, 2, 2};


/********************** DO MODE SCREEN **************************/
//
// Returns true if player cancelled with ESC
//

Boolean DoGameModeScreen(void)
{
Boolean	done = false;
Boolean	cancel = false;

	gHostNetworkGame = false;								// assume I'm not hosting
	gJoinNetworkGame = false;								// assume I'm not joining either
	gNetGameInProgress = false;
	gNumLocalPlayers = 1;									// assume just 1 local player on this machine

	SetupGameModeScreen();



				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(!done)
	{
			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawGameMode);


			/* SEE IF MAKE SELECTION */

		if (GetNewKeyState_Real(KEY_ESC))				// see if cancel
		{
			cancel = true;
			break;
		}

		DoGameModeControls();

		if (GetNewKeyState(kKey_MakeSelection_P1))
		{
			switch(gSelectedMode)
			{
				case	0:								// 1 player mode
						gNumRealPlayers = 1;
						done = true;
						break;

				case	1:								// 2 player mode
						gNumRealPlayers = 2;
						switch(gSelectedOption)
						{
							case	1:					// horiz split
									gDesiredSplitScreenMode = SPLITSCREEN_MODE_HORIZ;
									gNumLocalPlayers = 2;
									done = true;
									break;

							case	2:					// vert split
									gDesiredSplitScreenMode = SPLITSCREEN_MODE_VERT;
									gNumLocalPlayers = 2;
									done = true;
									break;
						}
						break;

				case	2:								// network play mode
						gNumRealPlayers = 1;
						switch(gSelectedOption)
						{
							case	1:					// host net game
									gHostNetworkGame = true;
									gNetGameInProgress = true;
									done = true;
									break;

							case	2:					// join net game
									gJoinNetworkGame = true;
									gNetGameInProgress = true;
									done = true;
									break;
						}
						break;


			}
		}
	}





			/* CLEANUP */

    DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
	DisposeSpriteGroup(SPRITE_GROUP_MODESCREEN);

	return(cancel);
}


/****************** SETUP GAME MODE SCREEN **********************/

static void SetupGameModeScreen(void)
{
OGLSetupInputType	viewDef;
FSSpec				spec;
OGLColorRGBA		ambientColor = { .1, .1, .1, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLColorRGBA		fillColor2 = { .3, .3, .3, 1 };
OGLVector3D			fillDirection1 = { .9, -.7, -1 };
OGLVector3D			fillDirection2 = { -1, -.2, -.5 };
short				i;

	for (i = 0; i < 2; i++)								// no option icons yet
		gOptions[i] = nil;

	for (i = 0; i < 3; i++)
	{
		gModeIcons[i] = nil;
	}

	gSelectedMode = 0;
	gSelectedOption = 0;


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.fov 				= 90;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 1;
	viewDef.styles.useFog			= false;

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

			/* CREATE BACKGROUND OBJECT */

	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:GameModeScreen", &spec))
		DoFatalAlert("\pDoGameMode: background pict not found.");
	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("\pDoGameMode: MO_CreateNewObjectOfType failed");


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:gamemode.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_MODESCREEN, gGameViewInfoPtr);


			/***************/
			/* BUILD ICONS */
			/***************/

			/* SET PARTS MENU */

	MakeGameModeIcons();


			/* CURSOR */

	gNewObjectDefinition.group 		= SPRITE_GROUP_MODESCREEN;
	gNewObjectDefinition.type 		= MODE_SObjType_Cursor;
	gNewObjectDefinition.coord		= gModeIcons[gSelectedMode]->Coord;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveCursor;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = MODE_ICON_SCALE;
	gCursorObj = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

}



/************* MAKE GAME MODE ICONS *****************/
//
// Sets the icon for each of the parts to correct value
//

static void MakeGameModeIcons(void)
{
short			i;

			/* NUKE OLD ICONS */

	for (i = 0; i < 3; i++)
	{
		if (gModeIcons[i])
		{
			DeleteObject(gModeIcons[i]);
			gModeIcons[i] = nil;
		}
	}


			/* 1 PLAYER ICON */

	gNewObjectDefinition.group 		= SPRITE_GROUP_MODESCREEN;
	gNewObjectDefinition.type 		= MODE_SObjType_1Player;
	gNewObjectDefinition.coord.x 	= -(ICON_SPACING * (3-1))/2;
	gNewObjectDefinition.coord.y 	= .3;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = MODE_ICON_SCALE;
	gModeIcons[0] 					= MakeSpriteObject(&gNewObjectDefinition,gGameViewInfoPtr);


			/* 2 PLAYER ICON */

	gNewObjectDefinition.type 		= MODE_SObjType_2Player;
	gNewObjectDefinition.coord.x 	+= ICON_SPACING;
	gModeIcons[1] 					= MakeSpriteObject(&gNewObjectDefinition,gGameViewInfoPtr);


			/* NET PLAY ICON */

	gNewObjectDefinition.type 		= MODE_SObjType_NetPlay;
	gNewObjectDefinition.coord.x 	+= ICON_SPACING;
	gModeIcons[2] 					= MakeSpriteObject(&gNewObjectDefinition,gGameViewInfoPtr);


		/* SHOW OPTIONS */

	ShowModeOptions();
}


 /***************** DRAW GAME MODE *******************/

static void DrawGameMode(OGLSetupOutputType *info)
{
	MO_DrawObject(gBackgoundPicture, info);
	DrawObjects(info);
}



/***************** DO GAME MODE CONTROLS *******************/

static void DoGameModeControls(void)
{

		/* SEE IF CHANGE PART SELECTION */

	if (GetNewKeyState(kKey_Left_P1) && (gSelectedMode > 0))
	{
		gSelectedMode--;
		gSelectedOption = 0;
		gCursorObj->Coord = gModeIcons[gSelectedMode]->Coord;
		ShowModeOptions();
	}
	else
	if (GetNewKeyState(kKey_Right_P1) && (gSelectedMode < 2))
	{
		gSelectedMode++;
		gSelectedOption = 0;
		gCursorObj->Coord = gModeIcons[gSelectedMode]->Coord;
		ShowModeOptions();
	}


		/* SEE IF CHANGE OPTION SELECTION */

	else
	if (GetNewKeyState(kKey_Forward_P1) && (gSelectedOption > 0))
	{
		gSelectedOption--;
		gCursorObj->Coord.y += OPTION_SPACING;
	}
	else
	if (GetNewKeyState(kKey_Backward_P1) && (gSelectedOption < gNumOptions[gSelectedMode]))
	{
		gSelectedOption++;
		gCursorObj->Coord.y -= OPTION_SPACING;
	}
}


/******************** MOVE CURSOR ************************/

static void MoveCursor(ObjNode *theNode)
{
float	s;

	theNode->SpecialF[0] += gFramesPerSecondFrac * 8.0f;

	s = (sin(theNode->SpecialF[0])+1.0f) * .01f;

	theNode->Scale.x =
	theNode->Scale.y = MODE_ICON_SCALE + s;
}


/***************** SHOW MODE OPTIONS **************************/

static void ShowModeOptions(void)
{
short			i;

			/* NUKE OLD OPTION ICONS */

	for (i = 0; i < 2; i++)
	{
		if (gOptions[i])
		{
			DeleteObject(gOptions[i]);
			gOptions[i] = nil;
		}
	}

		/* BUILD OPTIONAL ICONS */

	for (i = 1; i <= gNumOptions[gSelectedMode]; i++)
	{
		gNewObjectDefinition.group 		= SPRITE_GROUP_MODESCREEN;
		gNewObjectDefinition.type 		= gPartBaseModel[gSelectedMode] + i;
		gNewObjectDefinition.coord.x 	= gModeIcons[gSelectedMode]->Coord.x;
		gNewObjectDefinition.coord.y 	= gModeIcons[gSelectedMode]->Coord.y - (i * OPTION_SPACING);
		gNewObjectDefinition.coord.z 	= gModeIcons[gSelectedMode]->Coord.z;
		gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = MODE_ICON_SCALE;
		gOptions[i-1] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);
	}
}



/*************************** SELECT OPTION **********************************/
//
// OUTPUT:  true = selected Exit
//

static void SelectOption(void)
{
}















