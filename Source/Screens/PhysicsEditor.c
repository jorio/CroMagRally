/****************************/
/*   	PHYSICS EDITOR.C	*/
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "menu.h"
#include "miscscreens.h"
#include "uielements.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveCarModel(ObjNode* theNode);

static void UpdateShadowCarStats(void);

static void OnPickResetPhysics(const MenuItem* mi);
static void OnChangeCar(const MenuItem* mi);
static void OnTweakCarStat(const MenuItem* mi);
static void SetupPhysicsEditorScreen(void);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	MENU_EXITCODE_HELP = 1000,
	MENU_EXITCODE_CREDITS,
	MENU_EXITCODE_PHYSICS,
	MENU_EXITCODE_SELFRUNDEMO,
	MENU_EXITCODE_QUITGAME,
};

/*********************/
/*    VARIABLES      */
/*********************/

#define CAR_METER_CHOICES \
	{ \
	{STR_CAR_STAT_METER_1, 0}, \
	{STR_CAR_STAT_METER_2, 1}, \
	{STR_CAR_STAT_METER_3, 2}, \
	{STR_CAR_STAT_METER_4, 3}, \
	{STR_CAR_STAT_METER_5, 4}, \
	{STR_CAR_STAT_METER_6, 5}, \
	{STR_CAR_STAT_METER_7, 6}, \
	{STR_CAR_STAT_METER_8, 7}, \
	}

static Byte gCurrentCar = 0;
static Byte gShadowCarStats[NUM_VEHICLE_PARAMETERS];
static ObjNode* gCarModel = nil;

static Byte gLastWoahCar = -1;
static Byte gLastCrappyCar = -1;

static const MenuItem gPhysicsMenuTree[] =
{
	{.id='phys'},
	{kMILabel, .text=STR_PHYSICS_SETTINGS_RESET_INFO },
	{kMISpacer, .customHeight=1.5f },
	{kMIPick, STR_PHYSICS_EDIT_CAR_STATS, .next='stat', },
	{kMIPick, STR_PHYSICS_EDIT_CONSTANTS, .next='cons', },
	{kMIPick, STR_PHYSICS_RESET, .callback=OnPickResetPhysics, .next='EXIT' },

	{.id='stat'},
	{kMISpacer, .customHeight=3.0f},
	{
		kMICycler1,
		STR_NULL,
		.callback = OnChangeCar,
		.cycler =
		{
			.valuePtr = &gCurrentCar, .choices =
			{
				{STR_CAR_MODEL_1, 0},
				{STR_CAR_MODEL_2, 1},
				{STR_CAR_MODEL_3, 2},
				{STR_CAR_MODEL_4, 3},
				{STR_CAR_MODEL_5, 4},
				{STR_CAR_MODEL_6, 5},
				{STR_CAR_MODEL_7, 6},
				{STR_CAR_MODEL_8, 7},
				{STR_CAR_MODEL_9, 8},
				{STR_CAR_MODEL_10, 9},
				//{STR_CAR_MODEL_11, 1},	// don't expose submarine
			}
		},
		.customHeight = 1,
	},

	{
		kMICycler2,
		.text = STR_CAR_STAT_1,
		.callback = OnTweakCarStat,
		.customHeight = 0.5f,
		.cycler = {.valuePtr = &gShadowCarStats[0], .choices = CAR_METER_CHOICES }
	},

	{
		kMICycler2,
		.text = STR_CAR_STAT_2,
		.callback = OnTweakCarStat,
		.customHeight = 0.5f,
		.cycler = {.valuePtr = &gShadowCarStats[1], .choices = CAR_METER_CHOICES }
	},

	{
		kMICycler2,
		.text = STR_CAR_STAT_3,
		.callback = OnTweakCarStat,
		.customHeight = 0.5f,
		.cycler = {.valuePtr = &gShadowCarStats[2], .choices = CAR_METER_CHOICES }
	},

	{
		kMICycler2,
		.text = STR_CAR_STAT_4,
		.callback = OnTweakCarStat,
		.customHeight = 0.5f,
		.cycler = {.valuePtr = &gShadowCarStats[3], .choices = CAR_METER_CHOICES }
	},

	{.id='cons'},
	{ kMIFloatRange, STR_PHYSICS_CONSTANT_STEERING_RESPONSIVENESS,	.floatRange={.valuePtr=&gSteeringResponsiveness }	},
	{ kMIFloatRange, STR_PHYSICS_CONSTANT_MAX_TIGHT_TURN,			.floatRange={.valuePtr=&gCarMaxTightTurn}	},
	{ kMIFloatRange, STR_PHYSICS_CONSTANT_TURNING_RADIUS,			.floatRange={.valuePtr=&gCarTurningRadius} },
	{ kMIFloatRange, STR_PHYSICS_CONSTANT_TIRE_TRACTION,			.floatRange={.valuePtr=&gTireTractionConstant}	},
	{ kMIFloatRange, STR_PHYSICS_CONSTANT_TIRE_FRICTION,			.floatRange={.valuePtr=&gTireFrictionConstant}	},
	{ kMIFloatRange, STR_PHYSICS_CONSTANT_GRAVITY,					.floatRange={.valuePtr=&gCarGravity}	},
	{ kMIFloatRange, STR_PHYSICS_CONSTANT_SLOPE_RATIO_ADJUSTER,		.floatRange={.valuePtr=&gSlopeRatioAdjuster}	},

	{ 0 },
};



void DoPhysicsEditor(void)
{
	SetupPhysicsEditorScreen();

				/*************/
				/* MAIN LOOP */
				/*************/



	MenuStyle physicsEditorMenuStyle = kDefaultMenuStyle;
	physicsEditorMenuStyle.canBackOutOfRootMenu = true;

	StartMenu(gPhysicsMenuTree, &physicsEditorMenuStyle, MoveObjects, DrawObjects);

			/* CLEANUP */

	DeleteAllObjects();
	OGL_DisposeGameView();
}


/********************* SETUP PHYSICS EDITOR SCREEN **********************/


static void SetupPhysicsEditorScreen(void)
{
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .1, .1, .1, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLColorRGBA		fillColor2 = { .3, .3, .3, 1 };
OGLVector3D			fillDirection1 = { .9, -.7, -1 };
OGLVector3D			fillDirection2 = { -1, -.2, -.5 };


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= 1;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.camera.from[0].z		= 700;
	viewDef.camera.from[0].y		= 250;
	viewDef.view.clearColor 		= (OGLColorRGBA) {0,0,0, 1.0f};
	viewDef.styles.useFog			= false;
	viewDef.view.pillarboxRatio	= PILLARBOX_RATIO_4_3;
	viewDef.view.fontName			= "rockfont";

	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);
	OGLVector3D_Normalize(&fillDirection2, &fillDirection2);

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


			/* LOAD MODELS */

	FSSpec spec;
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:carselect.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_CARSELECT);


			/* VEHICLE MODEL */

	NewObjectDefinitionType modelDef =
	{
		.group = MODEL_GROUP_CARSELECT,
		.type = gCurrentCar,
		.coord = {0, 125, 0},
		.slot = SPRITE_SLOT,
		.moveCall =  MoveCarModel,
		.scale = 0.85f,
		.rot = 0,
	};
	gCarModel = MakeNewDisplayGroupObject(&modelDef);

			/* BACKGROUND */

	MakeScrollingBackgroundPattern();

			/* FADE IN */

	MakeFadeEvent(true);


	UpdateShadowCarStats();
}

#pragma mark -

static void OnPickResetPhysics(const MenuItem* mi)
{
	SetDefaultPhysics();
}

static void MoveCarModel(ObjNode* theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac;
	UpdateObjectTransforms(theNode);
}

static void UpdateShadowCarStats(void)
{
	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		gShadowCarStats[i] = gVehicleParameters[gCurrentCar][i];
	}
}

static void OnChangeCar(const MenuItem* mi)
{
	gCarModel->Type = gCurrentCar;
	ResetDisplayGroupObject(gCarModel);

	UpdateShadowCarStats();
	LayoutCurrentMenuAgain();
}

static void OnTweakCarStat(const MenuItem* mi)
{
	int totalBones = 0;

	// Commit temp car stats to global car stats
	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		int parm = gShadowCarStats[i];
		gVehicleParameters[gCurrentCar][i] = parm;
		totalBones += parm;
	}

	// Announcer feedback if the car is awesome or crappy
	if (totalBones >= 7*NUM_VEHICLE_PARAMETERS
		&& gLastWoahCar != gCurrentCar)
	{
		gLastWoahCar = gCurrentCar;
		gLastCrappyCar = -1;
		PlayEffect_Parms(EFFECT_OHYEAH, 3 * FULL_CHANNEL_VOLUME, 3 * FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE);
	}
	else if (totalBones == 0
		&& gLastCrappyCar != gCurrentCar)
	{
		gLastCrappyCar = gCurrentCar;
		gLastWoahCar = -1;
		PlayEffect_Parms(EFFECT_YOULOSE, 3 * FULL_CHANNEL_VOLUME, 3 * FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE);
	}
}

static void OnPickResetCar(const MenuItem* mi)
{
	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		gVehicleParameters[gCurrentCar][i] = gDefaultVehicleParameters[gCurrentCar][i];
		gShadowCarStats[i] = gVehicleParameters[gCurrentCar][i];
	}

	PlayEffect(EFFECT_BOOM);
	LayoutCurrentMenuAgain();
 }
