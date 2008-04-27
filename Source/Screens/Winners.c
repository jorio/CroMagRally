/****************************/
/*   	WINNERS.C		    */
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
#include "ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "skeletonobj.h"
#include "bg3d.h"
#include "camera.h"
#include "mobjtypes.h"
#include "winners.h"
#include "network.h"
#include "player.h"
#include "sprites.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow;
extern	FSSpec			gDataSpec;
extern	Boolean			gLowVRAM;
extern	KeyMap 			gKeyMap,gNewKeys;
extern	short			gNumRealPlayers,gNumTotalPlayers;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean			gSongPlayingFlag,gResetSong,gDisableAnimSounds,gSongPlayingFlag;
extern	PrefsType		gGamePrefs;
extern	OGLPoint3D		gCoord;
extern  MOPictureObject 	*gBackgoundPicture;
extern	Boolean					gNetGameInProgress,gIsNetworkClient;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D		gDelta;
extern	int				gTrackNum;
extern	PlayerInfoType	gPlayerInfo[];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void CleanupWinnersScreen(void);
static void SetupWinnersScreen(void);
static void BuildWinnerCars(void);
static void MoveWinnerCar(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	WINNERS_ObjType_Background
};


/*********************/
/*    VARIABLES      */
/*********************/


/********************** DO WINNERS SCREEN **************************/

void DoWinnersScreen(void)
{
float	timer = 10;

	SetupWinnersScreen();
	MakeFadeEvent(true);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	do
	{
		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_MoveCameraFromTo(gGameViewInfoPtr, 40.0f * gFramesPerSecondFrac, 0,0, 0,0,0, 0);

		OGL_DrawScene(gGameViewInfoPtr, DrawObjects);

		timer -= gFramesPerSecondFrac;

		if (GetNewKeyState(kKey_MakeSelection_P1) || GetNewKeyState(kKey_MakeSelection_P2))
			break;

	}while(timer > 0.0f);


			/***********/
			/* CLEANUP */
			/***********/

	CleanupWinnersScreen();
}


/********************* SETUP WINNERS SCREEN **********************/

static void SetupWinnersScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
static OGLColorRGBA			ambientColor = { .1, .1, .1, 1 };
static OGLColorRGBA			fillColor1 = { 1.0, 1.0, 1.0, 1 };
static OGLColorRGBA			fillColor2 = { .3, .3, .3, 1 };
static OGLVector3D			fillDirection1 = { .9, -.7, -1 };
static OGLVector3D			fillDirection2 = { -1, -.2, -.5 };
static OGLPoint3D			cameraFrom = {-400,500,1000};
static OGLPoint3D			cameraTo = {0,300,0};

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.from[0]			= cameraFrom;
	viewDef.camera.to[0] 			= cameraTo;

	viewDef.camera.fov 				= 1.0;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;

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

			/* LOAD SPRITES */

//	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:trackselect.sprites", &spec);
//	LoadSpriteFile(&spec, SPRITE_GROUP_TRACKSELECTSCREEN, gGameViewInfoPtr);


			/* LOAD BG3D GEOMETRY */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:models:winners.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINNERS, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:models:carparts.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_CARPARTS, gGameViewInfoPtr);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_PLAYER1, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_PLAYER2, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_PLAYER3, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_PLAYER4, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_PLAYER5, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_PLAYER6, gGameViewInfoPtr);



			/********************/
			/* BUILD BACKGROUND */
			/********************/

	gNewObjectDefinition.group		= MODEL_GROUP_WINNERS;
	gNewObjectDefinition.type 		= WINNERS_ObjType_Background;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 3;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);


	BuildWinnerCars();
}


/******************* BUILD WINNER CARS ***************************/

static void BuildWinnerCars(void)
{
ObjNode				*newObj;
int					i,p;
static OGLPoint3D	coords[3] =
{
	0,380,0,						// 1st place
	-300,180,0,						// 2nd place
	300,180,0						// 3rd place
};


	for (i = 0; i < 3; i++)								// there will be 3 cars
	{
			/* FIND THIS WINNER */

		for (p = 0; p < gNumTotalPlayers; p++)			// look for player that came in i'th place
			if (gPlayerInfo[p].place == p)
				break;


		/* CREATE CAR BODY AS MAIN OBJECT FOR PLAYER */

		gNewObjectDefinition.group 		= MODEL_GROUP_CARPARTS;
		gNewObjectDefinition.type 		= 0 + gPlayerInfo[p].vehicleType;
		gNewObjectDefinition.coord		= coords[i];
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= MoveWinnerCar;
		gNewObjectDefinition.rot 		= RandomFloat()*PI2;
		gNewObjectDefinition.scale 		= .8;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->PlayerNum = p;


			/* CREATE WHEELS & HEAD */

		CreateCarWheelsAndHead(newObj, p);
	}
}




/**********************CLEAN UP WINNERS SCREEN **********************/

static void CleanupWinnersScreen(void)
{
	DeleteAllObjects();
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	FreeAllSkeletonFiles(-1);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}




/************************ MOVE WINNER CAR *************************/

static void MoveWinnerCar(ObjNode *theNode)
{
	GetObjectInfo(theNode);
	theNode->Rot.y += gFramesPerSecondFrac;
	UpdateObject(theNode);
	AlignWheelsAndHeadOnCar(theNode);

}













