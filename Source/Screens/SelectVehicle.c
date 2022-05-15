/****************************/
/*   	SELECT VEHICLE.C 	*/
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "miscscreens.h"
#include "network.h"
#include "uielements.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupVehicleSelectScreen(short whichPlayer);
static Boolean DoVehicleSelectControls(short whichPlayer, Boolean allowAborting);
static void FreeVehicleSelectArt(void);
static void MoveCarModel(ObjNode *theNode);
static void MakeVehicleName(void);
static void UpdateSelectedVehicle(void);
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


#define	LEFT_ARROW_X	-192
#define RIGHT_ARROW_X	192
#define	ARROW_Y			-48
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
static ObjNode	*gVehicleLeftArrow = nil;
static ObjNode	*gVehicleRightArrow = nil;
static ObjNode	*gVehiclePadlock = nil;

static int		gNumVehiclesToChooseFrom;



/******************* DO MULTIPLAYER VEHICLE SELECTIONS ********************/

Boolean DoMultiPlayerVehicleSelections(void)
{
	if (gNetGameInProgress)
	{
		// TODO: We should allow bailing out of Character/Vehicle Select screens, even if we started a net game.
		DoCharacterSelectScreen(gMyNetworkPlayerNum, false);		// get player's sex
		DoVehicleSelectScreen(gMyNetworkPlayerNum, false);			// get this player's vehicle
		PlayerBroadcastVehicleType();								// tell other net players about my type
		GetVehicleSelectionFromNetPlayers();						// get types from other net players
		return false;
	}
	else
	{
		short currentScreen = 0;
		short screensPerPlayer = 2;
		bool bailed;

		while (currentScreen < screensPerPlayer * gNumLocalPlayers)
		{
			gCurrentPlayerNum = currentScreen / screensPerPlayer;
			int screenToShow = currentScreen % screensPerPlayer;

			switch (screenToShow)
			{
				case 0:
					bailed = DoCharacterSelectScreen(gCurrentPlayerNum, true);
					break;

				case 1:
					bailed = DoVehicleSelectScreen(gCurrentPlayerNum, true);
					break;

				default:
					DoFatalAlert("Unknown MP prep screen %d", screenToShow);
			}

			if (bailed)
			{
				if (currentScreen == 0)
				{
					return true;	// bail
				}
				else
				{
					currentScreen--;
				}
			}
			else
			{
				currentScreen++;
			}
		}

		return false;
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
		OGL_DrawScene(DrawObjects);
	}

			/***********/
			/* CLEANUP */
			/***********/

	bool bailed = gSelectedVehicleIndex == -1;								// see if user bailed

	if (!bailed)
	{
		gGameView->fadePillarbox = true;

		if (whichPlayer == gNumRealPlayers-1)
		{
			gGameView->fadeSound = true;
			gGameView->fadeOutDuration = .3f;
		}
	}

	OGL_FadeOutScene(DrawObjects, MoveObjects);
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
	viewDef.view.pillarboxRatio	= PILLARBOX_RATIO_4_3;

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= fillColor1;
	viewDef.lights.fillColor[1] 	= fillColor2;

	OGL_SetupGameView(&viewDef);



				/************/
				/* LOAD ART */
				/************/


			/* MAKE BACKGROUND PICTURE OBJECT */

	MakeBackgroundPictureObject(":images:VehicleSelectScreen.jpg");


			/* LOAD SPRITES */

	LoadSpriteGroup(SPRITE_GROUP_MAINMENU, "menus", 0);

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:carselect.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_CARSELECT);

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
			gBoneMeters[i] = TextMesh_New(GetBoneString(i), kTextMeshAlignLeft, &def);
	
			def.coord = leftCoord;
			def.coord.y += LINE_SPACING;
		}
	}


			/* ARROWS, PADLOCK */


	{
		NewObjectDefinitionType def =
		{
			.group = SPRITE_GROUP_MAINMENU,
			.type = MENUS_SObjType_LeftArrow,
			.coord = { LEFT_ARROW_X, ARROW_Y, 0 },
			.slot = SPRITE_SLOT,
			.moveCall = MoveUIArrow,
			.scale = ARROW_SCALE,
		};
		gVehicleLeftArrow = MakeSpriteObject(&def);

		def.type = MENUS_SObjType_RightArrow;
		def.coord.x = RIGHT_ARROW_X;
		gVehicleRightArrow = MakeSpriteObject(&def);
	}

	{
		NewObjectDefinitionType def =
		{
			.group = SPRITE_GROUP_MAINMENU,
			.type = MENUS_SObjType_Padlock,
			.coord = { 0, ARROW_Y, 0 },
			.slot = SPRITE_SLOT,
			.moveCall = MoveUIPadlock,
			.scale = ARROW_SCALE,
		};
		gVehiclePadlock = MakeSpriteObject(&def);
	}



			/* SEE IF DOING 2-PLAYER LOCALLY */

	if (gNumLocalPlayers > 1)
	{
		ObjNode	*newObj;

		NewObjectDefinitionType def =
		{
			.coord		= {0, -.8*240, 0},
			.scale		= .5,
			.slot 		= SPRITE_SLOT,
		};
		
		newObj = TextMesh_New(GetPlayerName(whichPlayer), kTextMeshAlignCenter, &def);

		newObj->ColorFilter.r = .5;
		newObj->ColorFilter.g = .3;
		newObj->ColorFilter.b = .2;
	}



	UpdateSelectedVehicle();
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
	DisposeAllBG3DContainers();
	OGL_DisposeGameView();
}


/*************** REFERSH GRAPHICS FOR SELECTED CAR ********************/


static void UpdateSelectedVehicle(void)
{

	gVehicleObj->Type = gSelectedVehicleIndex;
	ResetDisplayGroupObject(gVehicleObj);
	MakeVehicleName();

	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		TextMesh_Update(GetBoneString(i), kTextMeshAlignLeft, gBoneMeters[i]);
	}

			/* FOR LOCKED CARS SHOW OUTLINE ONLY */

	if (gSelectedVehicleIndex >= gNumVehiclesToChooseFrom)
	{
		gVehicleObj->ColorFilter = (OGLColorRGBA) {0,0,0,1};
	}
	else
	{
		gVehicleObj->ColorFilter = (OGLColorRGBA) {1,1,1,1};
	}

			/* SHOW/HIDE ARROWS, PADLOCK */

	SetObjectVisible(gVehicleLeftArrow,		gSelectedVehicleIndex > 0);
	SetObjectVisible(gVehicleRightArrow,	gSelectedVehicleIndex < NUM_LAND_CAR_TYPES-1);
	SetObjectVisible(gVehiclePadlock,		gSelectedVehicleIndex >= gNumVehiclesToChooseFrom);
}


/*********** GET "BONE METER" STRING TO DISPLAY VEHICLE STATS **********/

static const char* GetBoneString(int statID)
{
	int n = gUserPhysics.carStats[gSelectedVehicleIndex].params[statID];

	static char boneString[NUM_PARAM_BONES+1];

	if (n < 1 || n > 7)
	{
		// Show percentage for crazy values (most likely tweaked by user in physics sandbox)
		snprintf(boneString, sizeof(boneString), "%d%%", (int) roundf(100.0f * n / 7.0f));
		return boneString;
	}

	if (n > NUM_PARAM_BONES-1)
		n = NUM_PARAM_BONES-1;

	n++;		// Always draw an extra bone

	int i = 0;

	for (; i < n; i++)
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

	if (allowAborting && GetNewNeedStateAnyP(kNeed_UIBack))		// anyone can abort
	{
		PlayEffect(EFFECT_GETPOW);
		gSelectedVehicleIndex = -1;
		return(true);
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
			WiggleUIPadlock(gVehiclePadlock);
		}
	}
	else
	if (IsCheatKeyComboDown())		// useful to test local multiplayer without having all controllers plugged in
	{
		PlayEffect(EFFECT_ROMANCANDLE_LAUNCH);
		return true;
	}


		/* SEE IF CHANGE SELECTION */

	if (GetNewNeedState(kNeed_UILeft, p) && (gSelectedVehicleIndex > 0))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedVehicleIndex--;
		UpdateSelectedVehicle();
		TwitchUIArrow(gVehicleLeftArrow, -1, 0);
	}
	else
	if (GetNewNeedState(kNeed_UIRight, p) && (gSelectedVehicleIndex < NUM_LAND_CAR_TYPES-1))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedVehicleIndex++;
		UpdateSelectedVehicle();
		TwitchUIArrow(gVehicleRightArrow, 1, 0);
	}

	return(false);
}





/****************** MOVE CAR MODEL *********************/

static void MoveCarModel(ObjNode *theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac * 1.5f;
	UpdateObjectTransforms(theNode);
}
