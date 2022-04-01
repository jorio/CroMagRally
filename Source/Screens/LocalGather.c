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
static int DoCharacterSelectControls(short whichPlayer, Boolean allowAborting);
static void DrawCharacterSelectCallback(OGLSetupOutputType *info);
static void FreeCharacterSelectArt(void);



/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode* gGatherPrompt = NULL;
static int gNumControllersMissing = 4;




/********************** DO CHARACTER SELECT SCREEN **************************/
//
// Return true if user aborts.
//

Boolean DoLocalGatherScreen(void)
{
	int whichPlayer = 0;
	bool allowAborting = true;

	SetupCharacterSelectScreen(whichPlayer);
	MakeFadeEvent(true);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	int outcome = 0;

	while (outcome == 0)
	{
			/* SEE IF MAKE SELECTION */

		outcome = DoCharacterSelectControls(whichPlayer, allowAborting);

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

	return (outcome < 0);
}


static void UpdateGatherPrompt(int numControllersMissing)
{
	char message[256];

	snprintf(
		message,
		sizeof(message),
		"%s %s\n%s",
		Localize(STR_CONNECT_CONTROLLERS_PREFIX),
		Localize(STR_CONNECT_1_CONTROLLER + numControllersMissing - 1),
		Localize(numControllersMissing==1? STR_CONNECT_CONTROLLERS_SUFFIX_KBD: STR_CONNECT_CONTROLLERS_SUFFIX));

	TextMesh_Update(message, 0, gGatherPrompt);
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

	viewDef.view.fontName			= "rockfont";

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

	NewObjectDefinitionType def2 =
	{
		.genre = TEXTMESH_GENRE,
		.scale = 0.4f,
		.coord = {0, 0, 0},
	};

	gGatherPrompt = TextMesh_NewEmpty(256, &def2);

	UpdateGatherPrompt(4);
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

static int DoCharacterSelectControls(short whichPlayer, Boolean allowAborting)
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
			return -1;
		}
	}

	if (GetNewNeedStateAnyP(kNeed_UILeft))
	{
		gNumControllersMissing--;
		UpdateGatherPrompt(gNumControllersMissing);
	}
	else if (GetNewNeedStateAnyP(kNeed_UIRight))
	{
		gNumControllersMissing++;
		UpdateGatherPrompt(gNumControllersMissing);
	}

		/* SEE IF SELECT THIS ONE */

	if (GetNewNeedState(kNeed_UIConfirm, p))
	{
		PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
		return 1;
	}

	return 0;
}


