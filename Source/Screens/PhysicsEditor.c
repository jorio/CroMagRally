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

static const MenuItem gPhysicsMenuTree[] =
{
	{.id='phys'},
	{kMISubtitle, .text=STR_PHYSICS_SETTINGS_RESET_INFO },
	{kMISpacer, .customHeight=1.5f },
	{kMIPick, STR_PHYSICS_EDIT_CAR_STATS, .next='stat', },
	{kMIPick, STR_PHYSICS_EDIT_CONSTANTS, .next='cons', },
	{kMIPick, STR_PHYSICS_RESET, .callback=OnPickResetPhysics, .next='EXIT' },

	{.id='stat'},
	{
			kMICycler1,    STR_CAR, .cycler =
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
	{ kMIPick, .text=STR_CAR_STAT_1,	},
	{ kMIPick, .text=STR_CAR_STAT_2,	},
	{ kMIPick, .text=STR_CAR_STAT_3,	},
	{ kMIPick, .text=STR_CAR_STAT_4,	},

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
	physicsEditorMenuStyle.standardScale = .3f;

	StartMenu(gPhysicsMenuTree, &physicsEditorMenuStyle, MoveObjects, DrawObjects);

			/* CLEANUP */

	DeleteAllObjects();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
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

