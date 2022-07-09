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

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveCarModel(ObjNode* theNode);

static void UpdateShadowCarStats(void);

static void OnPickResetPhysics(const MenuItem* mi);
static void OnChangeCar(const MenuItem* mi);
static void OnTweakCarStat(const MenuItem* mi);
static void OnPickResetCar(const MenuItem* mi);
static void OnTweakPhysicsConst(const MenuItem* mi);
static void SetupPhysicsEditorScreen(void);
static Boolean RefreshUserTamperedWithPhysicsFlag(void);
static int ShouldDisplayTamperWarning(const MenuItem* mi);


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

static Byte gCurrentCar = 0;
static int gLastWoahCar = -1;
static int gLastCrappyCar = -1;
static float gShadowCarStats[NUM_VEHICLE_PARAMETERS];
static ObjNode* gCarModel = nil;

static const float kOneF = 1;

#define MI_CARSTAT(strID, n) 	\
	{ \
		kMIFloatRange, \
		.text = strID, \
		.callback = OnTweakCarStat, \
		.customHeight = 0.6f, \
		.floatRange = \
		{ \
			.valuePtr = &gShadowCarStats[n], \
			.equilibriumPtr = &kOneF, \
			.incrementFrac = 1.0f / 7.0f, \
			.xSpread = 120, \
		} \
	}

#define MI_PHYSCONST(strID, varName) \
	{ \
		kMIFloatRange, \
		strID, \
		.callback = OnTweakPhysicsConst, \
		.floatRange = \
		{ \
			.valuePtr = &gUserPhysics.varName, \
			.equilibriumPtr = &kDefaultPhysics.varName, \
			.incrementFrac = 5.0f / 100.0f, \
			.xSpread = 200, \
		}	\
	}

static const MenuItem gPhysicsMenuTree[] =
{
	{.id='phys'},
	{kMILabel, .text=STR_PHYSICS_SETTINGS_RESET_INFO, .customHeight=.66f, .getLayoutFlags=ShouldDisplayTamperWarning },
	{kMISpacer, .customHeight=1.5f, .getLayoutFlags=ShouldDisplayTamperWarning },
	{kMIPick, STR_PHYSICS_EDIT_CAR_STATS, .next='stat', .customHeight=.8f },
	{kMIPick, STR_PHYSICS_EDIT_CONSTANTS, .next='cons', .customHeight=.8f },
	{kMIPick, STR_PHYSICS_RESET, .callback=OnPickResetPhysics, .next='EXIT', .customHeight=.8f },

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
	MI_CARSTAT(STR_CAR_STAT_1, 0),
	MI_CARSTAT(STR_CAR_STAT_2, 1),
	MI_CARSTAT(STR_CAR_STAT_3, 2),
	MI_CARSTAT(STR_CAR_STAT_4, 3),
	{kMISpacer, .customHeight=0.25f },
	{kMIPick, STR_RESET_THIS_CAR, .callback=OnPickResetCar, .customHeight=.6f },

	{.id='cons'},
	MI_PHYSCONST(STR_PHYSICS_CONSTANT_STEERING_RESPONSIVENESS,	steeringResponsiveness),
	MI_PHYSCONST(STR_PHYSICS_CONSTANT_MAX_TIGHT_TURN,			carMaxTightTurn),
	MI_PHYSCONST(STR_PHYSICS_CONSTANT_TURNING_RADIUS,			carTurningRadius),
	MI_PHYSCONST(STR_PHYSICS_CONSTANT_TIRE_TRACTION,			tireTraction),
	MI_PHYSCONST(STR_PHYSICS_CONSTANT_TIRE_FRICTION,			tireFriction),
	MI_PHYSCONST(STR_PHYSICS_CONSTANT_GRAVITY,					carGravity),
	MI_PHYSCONST(STR_PHYSICS_CONSTANT_SLOPE_RATIO_ADJUSTER,		slopeRatioAdjuster),
	MI_PHYSCONST(STR_PHYSICS_CONSTANT_TERRAIN_HEIGHT,			terrainHeight),

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
	SetupGenericMenuScreen(false);


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


	UpdateShadowCarStats();
}

#pragma mark -

static void OnPickResetPhysics(const MenuItem* mi)
{
	SetDefaultPhysics();
}

static void MoveCarModel(ObjNode* theNode)
{
	SetObjectVisible(theNode, GetCurrentMenu() == 'stat');
	theNode->Rot.y += gFramesPerSecondFrac;
	UpdateObjectTransforms(theNode);
}

static void UpdateShadowCarStats(void)
{
	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		int n = gUserPhysics.carStats[gCurrentCar].params[i];

		gShadowCarStats[i] = n / 7.0f;
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
	int awesomeness = 0;
	int crappiness = 0;

	// Commit temp car stats to global car stats
	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		float parm = gShadowCarStats[i];
		int intParm = (int) roundf(parm * 7.0f);
		gUserPhysics.carStats[gCurrentCar].params[i] = intParm;

		if (intParm == 7)
			awesomeness++;

		if (intParm == 0)
			crappiness++;
	}

	// Announcer feedback if the car is awesome or crappy
	if (awesomeness == NUM_VEHICLE_PARAMETERS && gLastWoahCar != gCurrentCar)
	{
		gLastWoahCar = gCurrentCar;
		gLastCrappyCar = -1;
		PlayEffect_Parms(EFFECT_OHYEAH, 3 * FULL_CHANNEL_VOLUME, 3 * FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE);
	}
	else if (crappiness == NUM_VEHICLE_PARAMETERS && gLastCrappyCar != gCurrentCar)
	{
		gLastCrappyCar = gCurrentCar;
		gLastWoahCar = -1;
		PlayEffect_Parms(EFFECT_YOULOSE, 3 * FULL_CHANNEL_VOLUME, 3 * FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE);
	}

	RefreshUserTamperedWithPhysicsFlag();
}

static void OnPickResetCar(const MenuItem* mi)
{
	for (int i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		gUserPhysics.carStats[gCurrentCar].params[i] = kDefaultPhysics.carStats[gCurrentCar].params[i];
	}
	UpdateShadowCarStats();

	PlayEffect(EFFECT_BOOM);
	LayoutCurrentMenuAgain();
 }

static void OnTweakPhysicsConst(const MenuItem* mi)
{
	RefreshUserTamperedWithPhysicsFlag();
}

static Boolean RefreshUserTamperedWithPhysicsFlag(void)
{
	gUserTamperedWithPhysics = 0 != memcmp(&gUserPhysics, &kDefaultPhysics, sizeof(UserPhysics));
	return gUserTamperedWithPhysics;
}

static int ShouldDisplayTamperWarning(const MenuItem* mi)
{
	return gUserTamperedWithPhysics? 0: kMILayoutFlagHidden;
}
