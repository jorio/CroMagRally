/****************************/
/*   	SELECT CHARACTER.C 	*/
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
#include "window.h"
#include "input.h"
#include "sound2.h"
#include	"file.h"
#include "ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "sobjtypes.h"
#include "player.h"
#include "sprites.h"
#include "mobjtypes.h"
#include "bg3d.h"
#include "skeletonanim.h"
#include "localization.h"
#include "atlas.h"
#include <string.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow;
extern	FSSpec			gDataSpec;
extern	KeyMap 			gKeyMap,gNewKeys;
extern	short			gMyNetworkPlayerNum,gNumRealPlayers,gNumLocalPlayers,gCurrentPlayerNum;
extern	Boolean			gSongPlayingFlag,gResetSong,gDisableAnimSounds,gSongPlayingFlag;
extern	PrefsType		gGamePrefs;
extern	OGLPoint3D		gCoord;
extern  MOPictureObject 	*gBackgoundPicture;
extern	Boolean					gNetGameInProgress,gIsNetworkClient;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D		gDelta;
extern	PlayerInfoType	gPlayerInfo[];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupCharacterSelectScreen(short whichPlayer);
static Boolean DoCharacterSelectControls(short whichPlayer, Boolean allowAborting);
static void DrawCharacterSelectCallback(OGLSetupOutputType *info);
static void FreeCharacterSelectArt(void);
static void GetCharacterSelectionFromNetPlayers(void);
static void MoveCarModel(ObjNode *theNode);
static void MakeCharacterName(void);



/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	CHARACTERSELECT_SObjType_Arrow

};


#define	ARROW_SCALE		.5



/*********************/
/*    VARIABLES      */
/*********************/

static int		gSelectedCharacterIndex;

static ObjNode	*gSex[2];


/********************** DO CHARACTER SELECT SCREEN **************************/
//
// Return true if user aborts.
//

Boolean DoCharacterSelectScreen(short whichPlayer, Boolean allowAborting)
{
	SetupCharacterSelectScreen(whichPlayer);
	MakeFadeEvent(true);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (DoCharacterSelectControls(whichPlayer, allowAborting))
			break;


			/**************/
			/* DRAW STUFF */
			/**************/

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawCharacterSelectCallback);
	}

			/***********/
			/* CLEANUP */
			/***********/

	OGL_FadeOutScene(gGameViewInfoPtr, DrawCharacterSelectCallback, MoveObjects);
	FreeCharacterSelectArt();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);

	if (gSelectedCharacterIndex == -1)										// see if user bailed
		return(true);


		/* SET CHARACTER TYPE SELECTED */

	gPlayerInfo[whichPlayer].sex = gSelectedCharacterIndex;

	return(false);
}


/********************* SETUP CHARACTERSELECT SCREEN **********************/

static void SetupCharacterSelectScreen(short whichPlayer)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .5, .5, .5, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLVector3D			fillDirection1 = { .9, -.3, -1 };
ObjNode	*newObj;

	gSelectedCharacterIndex = 0;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= .3;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.camera.from[0].z		= 700;

	viewDef.view.clearColor 		= (OGLColorRGBA) { .51f, .39f, .27f, 1 };
	viewDef.styles.useFog			= false;
	viewDef.view.pillarbox4x3		= true;

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 1;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0] 	= fillColor1;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);



				/************/
				/* LOAD ART */
				/************/


			/* MAKE BACKGROUND PICTURE OBJECT */

	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:CharSelectScreen.jpg", &spec))
		DoFatalAlert("SetupCharacterSelectScreen: background pict not found.");

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (uintptr_t) gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("SetupCharacterSelectScreen: MO_CreateNewObjectOfType failed");


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:charselect.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_CHARACTERSELECTSCREEN, gGameViewInfoPtr);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_MALESTANDING, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_FEMALESTANDING, gGameViewInfoPtr);



			/*****************/
			/* BUILD OBJECTS */
			/*****************/


			/* SEE IF DOING 2-PLAYER LOCALLY */

	if (gNumLocalPlayers > 1)
	{
		NewObjectDefinitionType newObjDef =
		{
			.coord = {0, -.85, 0},
			.scale = .4,
			.slot = SPRITE_SLOT
		};
		newObj = TextMesh_New(Localize(STR_PLAYER_1 + whichPlayer), kTextMeshAlignCenter, &newObjDef);

		newObj->ColorFilter.r = .2;
		newObj->ColorFilter.g = .7;
		newObj->ColorFilter.b = .2;
	}

			/* CREATE NAME STRINGS */

	NewObjectDefinitionType newObjDef_NameString =
	{
		.coord = {-.43, .8, .6},
		.scale = .6f
	};
	TextMesh_New(Localize(STR_BROG), kTextMeshAlignCenter, &newObjDef_NameString);

	newObjDef_NameString.coord.x 	= -newObjDef_NameString.coord.x;
	TextMesh_New(Localize(STR_GRAG), kTextMeshAlignCenter, &newObjDef_NameString);

			/* CREATE MALE CHARACTER */

	NewObjectDefinitionType newObjDef_Character =
	{
		.type = SKELETON_TYPE_MALESTANDING,
		.animNum = 1,
		.coord = {-60, 0, 0},
		.slot = 100,
		.rot = PI,
		.scale = .5f
	};
	gSex[0] = MakeNewSkeletonObject(&newObjDef_Character);

			/* CREATE FEMALE CHARACTER */

	newObjDef_Character.type 		= SKELETON_TYPE_FEMALESTANDING;
	newObjDef_Character.coord.x 	= -newObjDef_Character.coord.x;
	newObjDef_Character.animNum	= 0;
	gSex[1] = MakeNewSkeletonObject(&newObjDef_Character);
}



/********************** FREE CHARACTERSELECT ART **********************/

static void FreeCharacterSelectArt(void)
{
	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
}


/***************** DRAW CHARACTERSELECT CALLBACK *******************/

static void DrawCharacterSelectCallback(OGLSetupOutputType *info)
{

			/* DRAW BACKGROUND */

	MO_DrawObject(gBackgoundPicture, info);


			/* ARROW */

	if (gSelectedCharacterIndex == 0)
	{
		DrawSprite(SPRITE_GROUP_VEHICLESELECTSCREEN, CHARACTERSELECT_SObjType_Arrow,
					-.43, -.85, ARROW_SCALE, 0, 0, info);
	}
	else
	{
		DrawSprite(SPRITE_GROUP_VEHICLESELECTSCREEN, CHARACTERSELECT_SObjType_Arrow,
					.43, -.85, ARROW_SCALE, 0, 0, info);
	}


	DrawObjects(info);
}




/***************** DO CHARACTERSELECT CONTROLS *******************/

static Boolean DoCharacterSelectControls(short whichPlayer, Boolean allowAborting)
{
short	p;


	if (gNetGameInProgress)										// if net game, then use P0 controls
		p = 0;
	else
		p = whichPlayer;										// otherwise, use P0 or P1 controls depending.


		/* SEE IF ABORT */

	if (allowAborting)
	{
		if (GetNewNeedState(kNeed_UIBack, p))
		{
			gSelectedCharacterIndex = -1;
			return(true);
		}
	}

		/* SEE IF SELECT THIS ONE */

	if (GetNewNeedState(kNeed_UIConfirm, p))
	{
		PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
		return(true);
	}


		/* SEE IF CHANGE SELECTION */

	if (GetNewNeedState(kNeed_UILeft, p) && (gSelectedCharacterIndex > 0))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedCharacterIndex--;
		MorphToSkeletonAnim(gSex[0]->Skeleton, 1, 5.0);
		MorphToSkeletonAnim(gSex[1]->Skeleton, 0, 5.0);

	}
	else
	if (GetNewNeedState(kNeed_UIRight, p) && (gSelectedCharacterIndex < 1))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedCharacterIndex++;
		MorphToSkeletonAnim(gSex[0]->Skeleton, 0, 5.0);
		MorphToSkeletonAnim(gSex[1]->Skeleton, 1, 5.0);
	}


	return(false);
}


