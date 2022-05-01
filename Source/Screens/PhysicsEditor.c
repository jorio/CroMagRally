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

static void OnPickResetPhysics(const MenuItem* mi);
static void SetupPhysicsEditorScreen(void);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	MENU_ID_NULL,		// keep ID=0 unused
	MENU_ID_PHYSICS_ROOT,
	MENU_ID_PHYSICS_CAR_STATS,
	MENU_ID_PHYSICS_CONSTANTS,
	NUM_MENU_IDS
};


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

static Byte gSelectedCarPhysics = 0;

static const MenuItem
	gMenuPhysicsRoot[] =
	{
		{ kMenuItem_Subtitle, .text = STR_PHYSICS_SETTINGS_RESET_INFO },
		{ kMenuItem_Spacer, .text = STR_NULL },
		{ kMenuItem_Spacer, .text = STR_NULL },
		{ kMenuItem_Spacer, .text = STR_NULL },
		{ kMenuItem_Pick, STR_PHYSICS_EDIT_CAR_STATS, .gotoMenu=MENU_ID_PHYSICS_CAR_STATS, },
		{ kMenuItem_Pick, STR_PHYSICS_EDIT_CONSTANTS, .gotoMenu=MENU_ID_PHYSICS_CONSTANTS, },
		{ kMenuItem_Pick, STR_PHYSICS_RESET, .callback=OnPickResetPhysics },
		{.type = kMenuItem_END_SENTINEL },
	},

	gMenuPhysicsCarStats[] =
	{
		{
			kMenuItem_CMRCycler, STR_CAR, .cycler =
			{
				.valuePtr = &gSelectedCarPhysics, .choices =
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
		},

		{ kMenuItem_Pick, STR_CAR_STAT_1,	},
		{ kMenuItem_Pick, STR_CAR_STAT_2,	},
		{ kMenuItem_Pick, STR_CAR_STAT_3,	},
		{ kMenuItem_Pick, STR_CAR_STAT_4,	},
		{.type = kMenuItem_END_SENTINEL },
	},

	gMenuPhysicsConstants[] =
	{
		{ kMenuItem_FloatRange, STR_PHYSICS_CONSTANT_STEERING_RESPONSIVENESS,	.floatRange={.valuePtr=&gSteeringResponsiveness }	},
		{ kMenuItem_FloatRange, STR_PHYSICS_CONSTANT_MAX_TIGHT_TURN,			.floatRange={.valuePtr=&gCarMaxTightTurn}	},
		{ kMenuItem_FloatRange, STR_PHYSICS_CONSTANT_TURNING_RADIUS,			.floatRange={.valuePtr=&gCarTurningRadius} },
		{ kMenuItem_FloatRange, STR_PHYSICS_CONSTANT_TIRE_TRACTION,				.floatRange={.valuePtr=&gTireTractionConstant}	},
		{ kMenuItem_FloatRange, STR_PHYSICS_CONSTANT_TIRE_FRICTION,				.floatRange={.valuePtr=&gTireFrictionConstant}	},
		{ kMenuItem_FloatRange, STR_PHYSICS_CONSTANT_GRAVITY,					.floatRange={.valuePtr=&gCarGravity}	},
		{ kMenuItem_FloatRange, STR_PHYSICS_CONSTANT_SLOPE_RATIO_ADJUSTER,		.floatRange={.valuePtr=&gSlopeRatioAdjuster}	},
		{.type = kMenuItem_END_SENTINEL },
	}

	;



static const MenuItem* gPhysicsMenuTree[NUM_MENU_IDS] =
{
	[MENU_ID_PHYSICS_ROOT] = gMenuPhysicsRoot,
	[MENU_ID_PHYSICS_CAR_STATS] = gMenuPhysicsCarStats,
	[MENU_ID_PHYSICS_CONSTANTS] = gMenuPhysicsConstants,
};



void DoPhysicsEditor(void)
{
	SetupPhysicsEditorScreen();



				/*************/
				/* MAIN LOOP */
				/*************/



	MenuStyle physicsEditorMenuStyle = kDefaultMenuStyle;
	physicsEditorMenuStyle.canBackOutOfRootMenu = true;
	physicsEditorMenuStyle.standardScale = .3f;

	int outcome = StartMenuTree(gPhysicsMenuTree, &physicsEditorMenuStyle, MoveObjects, DrawObjects);

			/* CLEANUP */

	DeleteAllObjects();
	DisposeAllSpriteGroups();
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

	viewDef.camera.fov 				= 1.0;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor 		= (OGLColorRGBA) {0,0,0, 1.0f};
	viewDef.styles.useFog			= false;
	viewDef.view.pillarbox4x3		= true;
	viewDef.view.fontName			= "rockfont";

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


			/* MAKE BACKGROUND PICTURE OBJECT */

//	MakeBackgroundPictureObject(":images:MainMenuBackground.jpg");


			/* SETUP TITLE MENU */

	MakeFadeEvent(true);
}

#pragma mark -

static void OnPickResetPhysics(const MenuItem* mi)
{
	SetDefaultPhysics();
}

