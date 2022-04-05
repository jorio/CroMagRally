/****************************/
/*   	SELECT VEHICLE.C 	*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "miscscreens.h"
#include "network.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupVehicleSelectScreen(short whichPlayer);
static Boolean DoVehicleSelectControls(short whichPlayer, Boolean allowAborting);
static void DrawVehicleSelectCallback(OGLSetupOutputType *info);
static void FreeVehicleSelectArt(void);
static void MoveCarModel(ObjNode *theNode);
static void MakeVehicleName(void);



/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	VEHICLESELECT_SObjType_Arrow_LeftOn,
	VEHICLESELECT_SObjType_Arrow_RightOn,

	VEHICLESELECT_SObjType_Meter1,
	VEHICLESELECT_SObjType_Meter2,
	VEHICLESELECT_SObjType_Meter3,
	VEHICLESELECT_SObjType_Meter4,
	VEHICLESELECT_SObjType_Meter5,
	VEHICLESELECT_SObjType_Meter6,
	VEHICLESELECT_SObjType_Meter7,
	VEHICLESELECT_SObjType_Meter8
};


#define	LEFT_ARROW_X	-.6
#define RIGHT_ARROW_X	.6
#define	ARROW_Y			.2
#define	ARROW_SCALE		.5

#define	CAR_Y			70

#define	VEHICLE_IMAGE_Y		.3
#define	VEHICLE_IMAGE_SCALE	1.2

#define	PARAMETERS_X		-.7
#define	PARAMETERS_Y		-.4
#define	PARAMETERS_SCALE	.5

#define	LINE_SPACING		.14f

#define	NAME_Y			.6


/*********************/
/*    VARIABLES      */
/*********************/

static int		gSelectedVehicleIndex;
static ObjNode	*gVehicleObj, *gMeterIcon[NUM_VEHICLE_PARAMETERS];
static ObjNode	*gVehicleName;

static int		gNumVehiclesToChooseFrom;

int		gVehicleParameters[MAX_CAR_TYPES+1][NUM_VEHICLE_PARAMETERS];					// parameter values 0..7
int		gDefaultVehicleParameters[MAX_CAR_TYPES+1][NUM_VEHICLE_PARAMETERS] =					// parameter values 0..7
{
	//							Spd Acc Tra Sus
	[CAR_TYPE_MAMMOTH]		= {  4,  2,  4,  7 },
	[CAR_TYPE_BONEBUGGY]	= {  5,  4,  4,  2 },
	[CAR_TYPE_GEODE]		= {  3,  5,  6,  7 },
	[CAR_TYPE_LOG]			= {  6,  5,  2,  3 },
	[CAR_TYPE_TURTLE]		= {  3,  6,  4,  3 },
	[CAR_TYPE_ROCK]			= {  4,  3,  5,  2 },
	[CAR_TYPE_TROJANHORSE]	= {  4,  5,  6,  4 },
	[CAR_TYPE_OBELISK]		= {  5,  3,  6,  7 },
	[CAR_TYPE_CATAPULT]		= {  6,  7,  5,  4 },
	[CAR_TYPE_CHARIOT]		= {  7,  7,  4,  3 },
	[CAR_TYPE_SUB]			= {  0,  7,  0,  0 },
};



/******************* DO MULTIPLAYER VEHICLE SELECTIONS ********************/

void DoMultiPlayerVehicleSelections(void)
{
	if (gNetGameInProgress)
	{
		DoCharacterSelectScreen(gMyNetworkPlayerNum, false);		// get player's sex
		DoVehicleSelectScreen(gMyNetworkPlayerNum, false);			// get this player's vehicle
		PlayerBroadcastVehicleType();								// tell other net players about my type
		GetVehicleSelectionFromNetPlayers();						// get types from other net players
	}
	else
	{
		for (gCurrentPlayerNum = 0; gCurrentPlayerNum < gNumLocalPlayers; gCurrentPlayerNum++)
		{
			DoCharacterSelectScreen(gCurrentPlayerNum, false);		// get player's sex
			DoVehicleSelectScreen(gCurrentPlayerNum, false);		// do it for each local player (split screen mode)
		}
	}


}


/********************** DO VEHICLE SELECT SCREEN **************************/
//
// Return true if user aborts.
//

Boolean DoVehicleSelectScreen(short whichPlayer, Boolean allowAborting)
{
	if (gTrackNum == TRACK_NUM_ATLANTIS)		// dont select this on atlantis
		return(false);

	SetupVehicleSelectScreen(whichPlayer);
	MakeFadeEvent(true);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (DoVehicleSelectControls(whichPlayer, allowAborting))
			break;


			/**************/
			/* DRAW STUFF */
			/**************/

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawVehicleSelectCallback);
	}

			/***********/
			/* CLEANUP */
			/***********/

	bool bailed = gSelectedVehicleIndex == -1;								// see if user bailed

	gGameViewInfoPtr->fadePillarbox = !bailed;

	OGL_FadeOutScene(gGameViewInfoPtr, DrawVehicleSelectCallback, MoveObjects);
	FreeVehicleSelectArt();

	if (bailed)
		return(true);


		/* SET VEHICLE TYPE SELECTED */

	gPlayerInfo[whichPlayer].vehicleType = gSelectedVehicleIndex;

	return(false);
}


/********************* SETUP VEHICLESELECT SCREEN **********************/

static void SetupVehicleSelectScreen(short whichPlayer)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .4, .4, .4, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLColorRGBA		fillColor2 = { .5, .5, .5, 1 };
OGLVector3D			fillDirection1 = { .9, -.7, -1 };
OGLVector3D			fillDirection2 = { -1, -.2, -.5 };
int					age;

	gVehicleName = nil;
	gSelectedVehicleIndex = 0;

	age = GetNumAgesCompleted();
	if (age > 2)											// prevent extra cars after winning Iron Age
		age = 2;
	gNumVehiclesToChooseFrom = 6 + (age * 2);				// set # cars we can pick from

	if (gNumRealPlayers > 1)							// let use any car in mutliplayer mode
		gNumVehiclesToChooseFrom = 10;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= 1.1;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.camera.from[0].z		= -700;
	viewDef.camera.from[0].y		= 250;

	viewDef.view.clearColor 		= (OGLColorRGBA) {.76f, .61f, .45f, 1.0f};
	viewDef.styles.useFog			= false;
	viewDef.view.pillarbox4x3		= true;

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

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (uintptr_t) gGameViewInfoPtr, ":images:VehicleSelectScreen.jpg");
	if (!gBackgoundPicture)
		DoFatalAlert("SetupVehicleSelectScreen: MO_CreateNewObjectOfType failed");


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:vehicleselect.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_VEHICLESELECTSCREEN, gGameViewInfoPtr);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:carselect.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_CARSELECT, gGameViewInfoPtr);

			/*****************/
			/* BUILD OBJECTS */
			/*****************/

			/* VEHICLE MODEL */

	{
		NewObjectDefinitionType def =
		{
			.group		= MODEL_GROUP_CARSELECT,
			.type		= gSelectedVehicleIndex,
			.coord		= {0, CAR_Y, 0},
			.slot		= 100,
			.moveCall 	= MoveCarModel,
			.scale		= 1,
		};
		gVehicleObj = MakeNewDisplayGroupObject(&def);
	}


			/* VEHICLE NAME */

	MakeVehicleName();



			/* PARAMETER STRINGS */

	{
		NewObjectDefinitionType def =
		{
			.coord		= {PARAMETERS_X, PARAMETERS_Y, 0},
			.scale		= PARAMETERS_SCALE,
			.slot		= SPRITE_SLOT,
		};

		for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
		{
			TextMesh_New(Localize(STR_CAR_STAT_1 + i), kTextMeshAlignLeft, &def);
			def.coord.y -= LINE_SPACING;
		}
	}

			/* METER ICONS */

	{
		NewObjectDefinitionType def =
		{
			.group		= SPRITE_GROUP_VEHICLESELECTSCREEN,
			.coord		= {PARAMETERS_X + 1.2, PARAMETERS_Y, 0},
			.slot		= SPRITE_SLOT,
			.scale		= PARAMETERS_SCALE,
		};

		for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
		{
			int n = gVehicleParameters[gSelectedVehicleIndex][i];
			if (n > 7)
				n = 7;

			def.type 	= VEHICLESELECT_SObjType_Meter1 + n;

			gMeterIcon[i] = MakeSpriteObject(&def, gGameViewInfoPtr);
			def.coord.y 	-= LINE_SPACING;
		}
	}

			/* SEE IF DOING 2-PLAYER LOCALLY */

	if (gNumLocalPlayers > 1)
	{
		ObjNode	*newObj;

		NewObjectDefinitionType def =
		{
			.coord		= {0, .8, 0},
			.scale		= .5,
			.slot 		= SPRITE_SLOT,
		};
		newObj = TextMesh_New(Localize(STR_PLAYER_1 + whichPlayer), kTextMeshAlignCenter, &def);

		newObj->ColorFilter.r = .5;
		newObj->ColorFilter.g = .3;
		newObj->ColorFilter.b = .2;
	}
}


/******************** MAKE VEHICLE NAME ************************/

static void MakeVehicleName(void)
{
	if (gVehicleName)
	{
		DeleteObject(gVehicleName);
		gVehicleName = nil;
	}

	NewObjectDefinitionType def =
	{
		.coord		= {0, NAME_Y, 0},
		.scale		= .6,
		.slot 		= SPRITE_SLOT,
	};

	gVehicleName = TextMesh_New(Localize(STR_CAR_MODEL_1 + gSelectedVehicleIndex), kTextMeshAlignCenter, &def);

	gVehicleName->ColorFilter.r = .3;
	gVehicleName->ColorFilter.g = .5;
	gVehicleName->ColorFilter.b = .2;
}



/********************** FREE VEHICLESELECT ART **********************/

static void FreeVehicleSelectArt(void)
{
	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}


/***************** DRAW VEHICLESELECT CALLBACK *******************/

static void DrawVehicleSelectCallback(OGLSetupOutputType *info)
{

			/* DRAW BACKGROUND */

	MO_DrawObject(gBackgoundPicture, info);


			/**************************/
			/* DRAW THE SCROLL ARROWS */
			/**************************/

	if (gSelectedVehicleIndex >= 0)		// only draw arrows if selection is valid (it's -1 when fading out after canceling)
	{
			/* LEFT ARROW */

		if (gSelectedVehicleIndex > 0)
		{
			DrawSprite(SPRITE_GROUP_VEHICLESELECTSCREEN, VEHICLESELECT_SObjType_Arrow_LeftOn,
						LEFT_ARROW_X, ARROW_Y, ARROW_SCALE, 0, 0, info);
		}

			/* RIGHT ARROW */

		if (gSelectedVehicleIndex < (gNumVehiclesToChooseFrom-1))
		{
			DrawSprite(SPRITE_GROUP_VEHICLESELECTSCREEN, VEHICLESELECT_SObjType_Arrow_RightOn,
						RIGHT_ARROW_X, ARROW_Y, ARROW_SCALE, 0, 0, info);
		}
	}

	DrawObjects(info);
}




/***************** DO VEHICLESELECT CONTROLS *******************/

static Boolean DoVehicleSelectControls(short whichPlayer, Boolean allowAborting)
{
int		i,n;
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
			gSelectedVehicleIndex = -1;
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

	if (GetNewNeedState(kNeed_UILeft, p) && (gSelectedVehicleIndex > 0))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedVehicleIndex--;
		gVehicleObj->Type = gSelectedVehicleIndex;
		ResetDisplayGroupObject(gVehicleObj);
		MakeVehicleName();

		for (i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
		{
			n = gVehicleParameters[gSelectedVehicleIndex][i];
			if (n > 7)
				n = 7;
			ModifySpriteObjectFrame(gMeterIcon[i], VEHICLESELECT_SObjType_Meter1 + n, gGameViewInfoPtr);
		}
	}
	else
	if (GetNewNeedState(kNeed_UIRight, p) && (gSelectedVehicleIndex < (gNumVehiclesToChooseFrom-1)))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedVehicleIndex++;
		gVehicleObj->Type = gSelectedVehicleIndex;
		ResetDisplayGroupObject(gVehicleObj);
		MakeVehicleName();

		for (i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
		{
			n = gVehicleParameters[gSelectedVehicleIndex][i];
			if (n > 7)
				n = 7;
			ModifySpriteObjectFrame(gMeterIcon[i], VEHICLESELECT_SObjType_Meter1 + n, gGameViewInfoPtr);
		}
	}

	return(false);
}





/****************** MOVE CAR MODEL *********************/

static void MoveCarModel(ObjNode *theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac * 1.5f;
	UpdateObjectTransforms(theNode);
}
