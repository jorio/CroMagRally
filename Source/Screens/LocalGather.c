/****************************/
/* LOCAL GATHER.C           */
/* (C) 2022 Iliyas Jorio    */
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



/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/


/********************** DO CHARACTER SELECT SCREEN **************************/
//
// Return true if user aborts.
//

Boolean DoLocalGatherScreen(void)
{
	int whichPlayer = 0;
	bool allowAborting = false;

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


		/* SET CHARACTER TYPE SELECTED */

	return(false);
}


/********************* SETUP CHARACTERSELECT SCREEN **********************/

static void SetupCharacterSelectScreen(short whichPlayer)
{
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .5, .5, .5, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLVector3D			fillDirection1 = { .9, -.3, -1 };

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= .3;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.camera.from[0].z		= 700;

	viewDef.view.clearColor 		= (OGLColorRGBA) { 0, 0, 0, 1 };
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

	LoadASkeleton(SKELETON_TYPE_FEMALESTANDING, gGameViewInfoPtr);

			/* MAKE BACKGROUND PICTURE OBJECT */

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (uintptr_t) gGameViewInfoPtr, ":images:CharSelectScreen.jpg");
	if (!gBackgoundPicture)
		DoFatalAlert("SetupCharacterSelectScreen: MO_CreateNewObjectOfType failed");


			/*****************/
			/* BUILD OBJECTS */
			/*****************/

	// NewObjectDefinitionType def2 =  { .genre = TEXTMESH_GENRE, .scale = 0.35f, .coord={0,0.33f,0} };
	// TextMesh_New(Localize(STR_CONNECT_CONTROLLERS_PREFIX), 0, &def2);
	// def2.coord.y -= 0.12f;
	// TextMesh_New(Localize(STR_CONNECT_2_CONTROLLERS), 0, &def2);
	// def2.coord.y -= 0.12f;
	// TextMesh_New(Localize(STR_CONNECT_CONTROLLERS_SUFFIX_1), 0, &def2);
	// def2.coord.y -= 0.12f;
	// TextMesh_New(Localize(STR_CONNECT_CONTROLLERS_SUFFIX_2), 0, &def2);
	// def2.coord.y -= 0.12f;

	NewObjectDefinitionType def2 =  { .genre = TEXTMESH_GENRE, .scale = 0.35f, .coord={0,0.33f,0} };
	TextMesh_New(Localize(STR_CONNECT_CONTROLLERS_PREFIX), 0, &def2);
	def2.coord.y -= 0.12f;
	TextMesh_New(Localize(STR_CONNECT_1_CONTROLLER), 0, &def2);
	def2.coord.y -= 0.12f;
	TextMesh_New(Localize(STR_CONNECT_CONTROLLERS_SUFFIX_KBD1), 0, &def2);
	def2.coord.y -= 0.12f;
	TextMesh_New(Localize(STR_CONNECT_CONTROLLERS_SUFFIX_KBD2), 0, &def2);
	def2.coord.y -= 0.12f;
	TextMesh_New(Localize(STR_CONNECT_CONTROLLERS_SUFFIX_KBD3), 0, &def2);
	def2.coord.y -= 0.12f;


	NewObjectDefinitionType newObjDef_Character =
	{
		.type = SKELETON_TYPE_FEMALESTANDING,
		.animNum = 0,
		.coord = {-130, 0, 0},
		.slot = 100,
		.rot = PI,
		.scale = .2f
	};
	MakeNewSkeletonObject(&newObjDef_Character);

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

	//MO_DrawObject(gBackgoundPicture, info);


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
			return(true);
		}
	}

		/* SEE IF SELECT THIS ONE */

	if (GetNewNeedState(kNeed_UIConfirm, p))
	{
		PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
		return(true);
	}


	// 	/* SEE IF CHANGE SELECTION */

	// if (GetNewNeedState(kNeed_UILeft, p) && (gSelectedCharacterIndex > 0))
	// {
	// 	PlayEffect(EFFECT_SELECTCLICK);
	// 	gSelectedCharacterIndex--;
	// 	MorphToSkeletonAnim(gSex[0]->Skeleton, 1, 5.0);
	// 	MorphToSkeletonAnim(gSex[1]->Skeleton, 0, 5.0);

	// }
	// else
	// if (GetNewNeedState(kNeed_UIRight, p) && (gSelectedCharacterIndex < 1))
	// {
	// 	PlayEffect(EFFECT_SELECTCLICK);
	// 	gSelectedCharacterIndex++;
	// 	MorphToSkeletonAnim(gSex[0]->Skeleton, 0, 5.0);
	// 	MorphToSkeletonAnim(gSex[1]->Skeleton, 1, 5.0);
	// }


	return(false);
}


