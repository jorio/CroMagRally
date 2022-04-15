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
static void RefreshSelectedCarGraphics(void);
static const char* GetBoneString(int n);



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

#define	PARAMETERS_X		-224
#define	PARAMETERS_Y		96
#define	PARAMETERS_SCALE	.5

#define	LINE_SPACING		34

#define	NAME_Y				-144

#define	NUM_PARAM_BONES		8


/*********************/
/*    VARIABLES      */
/*********************/

static int		gSelectedVehicleIndex;
static ObjNode	*gVehicleObj;
static ObjNode	*gVehicleName;
static ObjNode	*gBoneMeters[NUM_VEHICLE_PARAMETERS];

static int		gNumVehiclesToChooseFrom;

int		gVehicleParameters[NUM_CAR_TYPES_TOTAL][NUM_VEHICLE_PARAMETERS];					// parameter values 0..7
int		gDefaultVehicleParameters[NUM_CAR_TYPES_TOTAL][NUM_VEHICLE_PARAMETERS] =			// parameter values 0..7
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

	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
		gBoneMeters[i] = nil;

	age = GetNumAgesCompleted();
	if (age > 2)											// prevent extra cars after winning Iron Age
		age = 2;
	gNumVehiclesToChooseFrom = 6 + (age * 2);				// set # cars we can pick from

	if (gNumRealPlayers > 1)							// let use any car in mutliplayer mode
		gNumVehiclesToChooseFrom = NUM_LAND_CAR_TYPES;

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

	LoadSpriteGroup(SPRITE_GROUP_MAINMENU, "menus", 0, gGameViewInfoPtr);

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
			OGLPoint3D leftCoord = def.coord;

			TextMesh_New(Localize(STR_CAR_STAT_1 + i), kTextMeshAlignLeft, &def);

			def.coord.x += 320;
			gBoneMeters[i] = TextMesh_New(GetBoneString(gVehicleParameters[gSelectedVehicleIndex][i]), kTextMeshAlignLeft, &def);
	
			def.coord = leftCoord;
			def.coord.y += LINE_SPACING;
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
		
		newObj = TextMesh_New(GetPlayerNameWithInputDeviceHint(whichPlayer), kTextMeshAlignCenter, &def);

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

	const char* nameStr = "???";
	if (gSelectedVehicleIndex < gNumVehiclesToChooseFrom)
	{
		nameStr = Localize(STR_CAR_MODEL_1 + gSelectedVehicleIndex);
	}

	gVehicleName = TextMesh_New(nameStr, kTextMeshAlignCenter, &def);

	gVehicleName->ColorFilter = (OGLColorRGBA) {.3, .5, .2, 1};
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


			/* DRAW OBJECTS */

	DrawObjects(info);


			/**************************/
			/* DRAW THE SCROLL ARROWS */
			/**************************/

	if (gSelectedVehicleIndex >= 0)		// only draw arrows if selection is valid (it's -1 when fading out after canceling)
	{
			/* LEFT ARROW */

		if (gSelectedVehicleIndex > 0)
		{
			DrawSprite(SPRITE_GROUP_MAINMENU, MENUS_SObjType_LeftArrow,
						LEFT_ARROW_X, ARROW_Y, ARROW_SCALE, 0, 0, info);
		}

			/* RIGHT ARROW */

		if (gSelectedVehicleIndex < NUM_LAND_CAR_TYPES-1)
		{
			DrawSprite(SPRITE_GROUP_MAINMENU, MENUS_SObjType_RightArrow,
						RIGHT_ARROW_X, ARROW_Y, ARROW_SCALE, 0, 0, info);
		}

			/* DRAW PADLOCK */

		if (gSelectedVehicleIndex >= gNumVehiclesToChooseFrom)
		{
			DrawSprite(SPRITE_GROUP_MAINMENU, MENUS_SObjType_Padlock,
					0, ARROW_Y, ARROW_SCALE, 0, 0, info);
		}
	}
}



/*************** REFERSH GRAPHICS FOR SELECTED CAR ********************/


static void RefreshSelectedCarGraphics(void)
{
	gVehicleObj->Type = gSelectedVehicleIndex;
	ResetDisplayGroupObject(gVehicleObj);
	MakeVehicleName();

	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		int n = gVehicleParameters[gSelectedVehicleIndex][i];
		if (n > 7)
			n = 7;
		TextMesh_Update(GetBoneString(n), kTextMeshAlignLeft, gBoneMeters[i]);
	}

	if (gSelectedVehicleIndex >= gNumVehiclesToChooseFrom)
	{
		gVehicleObj->ColorFilter = (OGLColorRGBA) {0,0,0,1};
	}
	else
	{
		gVehicleObj->ColorFilter = (OGLColorRGBA) {1,1,1,1};
	}
}


/*********** GET "BONE METER" STRING TO DISPLAY VEHICLE STATS **********/

static const char* GetBoneString(int n)
{
	static char boneString[NUM_PARAM_BONES+1];

	if (n > NUM_PARAM_BONES-1)
		n = NUM_PARAM_BONES-1;

	int i = 0;
	for (i = 0; i < n; i++)
		boneString[i] = '\\';
	for (; i < NUM_PARAM_BONES; i++)
		boneString[i] = '`';

	boneString[i] = '\0';

	return boneString;
}



/***************** DO VEHICLESELECT CONTROLS *******************/

static Boolean DoVehicleSelectControls(short whichPlayer, Boolean allowAborting)
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
			gSelectedVehicleIndex = -1;
			return(true);
		}
	}

		/* SEE IF SELECT THIS ONE */

	if (GetNewNeedState(kNeed_UIConfirm, p))
	{
		if (gSelectedVehicleIndex < gNumVehiclesToChooseFrom)
		{
			PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
			return(true);
		}
		else
		{
			PlayEffect(EFFECT_BADSELECT);
		}
	}


		/* SEE IF CHANGE SELECTION */

	if (GetNewNeedState(kNeed_UILeft, p) && (gSelectedVehicleIndex > 0))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedVehicleIndex--;
		RefreshSelectedCarGraphics();
	}
	else
	if (GetNewNeedState(kNeed_UIRight, p) && (gSelectedVehicleIndex < NUM_LAND_CAR_TYPES-1))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedVehicleIndex++;
		RefreshSelectedCarGraphics();
	}

	return(false);
}





/****************** MOVE CAR MODEL *********************/

static void MoveCarModel(ObjNode *theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac * 1.5f;
	UpdateObjectTransforms(theNode);
}
