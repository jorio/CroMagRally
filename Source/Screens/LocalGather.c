/****************************/
/* LOCAL GATHER.C           */
/* (C) 2022 Iliyas Jorio    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupLocalGatherScreen(void);
static int DoLocalGatherControls(void);



/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode* gGatherPrompt = NULL;
static int gNumGamepadsMissing = 4;




static void UpdateGatherPrompt(int numGamepadsMissing)
{
	char message[256];

	if (numGamepadsMissing <= 0)
	{
		TextMesh_Update("OK", 0, gGatherPrompt);
		gGatherPrompt->Scale.x = 1;
		gGatherPrompt->Scale.y = 1;
		UpdateObjectTransforms(gGatherPrompt);
		gGameView->fadeOutDuration = .3f;
	}
	else
	{
		SDL_snprintf(
			message,
			sizeof(message),
			"%s %s\n%s",
			Localize(STR_CONNECT_CONTROLLERS_PREFIX),
			Localize(STR_CONNECT_1_CONTROLLER + numGamepadsMissing - 1),
			Localize(numGamepadsMissing==1? STR_CONNECT_CONTROLLERS_SUFFIX_KBD: STR_CONNECT_CONTROLLERS_SUFFIX));

		TextMesh_Update(message, 0, gGatherPrompt);
	}
}




/********************** DO GATHER SCREEN **************************/
//
// Return true if user aborts.
//

Boolean DoLocalGatherScreen(void)
{
	gNumGamepadsMissing = gNumLocalPlayers;
	UnlockPlayerGamepadMapping();

	if (GetNumGamepads() >= gNumLocalPlayers)
	{
		// Skip gather screen if we already have enough gamepads
		return false;
	}

	SetupLocalGatherScreen();
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

		outcome = DoLocalGatherControls();


		int numGamepads = GetNumGamepads();
		
		gNumGamepadsMissing = gNumLocalPlayers - numGamepads;
		if (gNumGamepadsMissing < 0)
			gNumGamepadsMissing = 0;

		UpdateGatherPrompt(gNumGamepadsMissing);


			/**************/
			/* DRAW STUFF */
			/**************/

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(DrawObjects);
	}

			/* SHOW 'OK!' */

	if (outcome >= 0)
	{
		UpdateGatherPrompt(0);
	}


			/***********/
			/* CLEANUP */
			/***********/

	OGL_FadeOutScene(DrawObjects, MoveObjects);

	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllBG3DContainers();
	OGL_DisposeGameView();


		/* SET CHARACTER TYPE SELECTED */

	return (outcome < 0);
}


/********************* SETUP LOCAL GATHER SCREEN **********************/

static void SetupLocalGatherScreen(void)
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
	viewDef.view.pillarboxRatio		= PILLARBOX_RATIO_4_3;

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 1;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0] 	= fillColor1;

	viewDef.view.fontName			= "rockfont";

	OGL_SetupGameView(&viewDef);



				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */

//	MakeBackgroundPictureObject(":Images:CharSelectScreen.jpg");
	MakeScrollingBackgroundPattern();


			/*****************/
			/* BUILD OBJECTS */
			/*****************/

	NewObjectDefinitionType def2 =
	{
		.scale = 0.4f,
		.coord = {0, 0, 0},
		.slot = SPRITE_SLOT,
	};

	gGatherPrompt = TextMesh_NewEmpty(256, &def2);

	def2.coord.y = 220;
	def2.scale = 0.27f;
	ObjNode* pressEsc = TextMesh_New(Localize(STR_PRESS_ESC_TO_GO_BACK), 0, &def2);
	pressEsc->ColorFilter = (OGLColorRGBA) {.5, .5, .5, 1};
	MakeTwitch(pressEsc, kTwitchPreset_PressKeyPrompt);
}



/***************** DO CHARACTERSELECT CONTROLS *******************/

static int DoLocalGatherControls(void)
{
	if (gNumGamepadsMissing == 0)
		return 1;

	if (GetNewNeedStateAnyP(kNeed_UIBack))
	{
		return -1;
	}

		/* SEE IF SELECT THIS ONE */

	if (GetNewKeyState(SDL_SCANCODE_RETURN) || GetNewKeyState(SDL_SCANCODE_KP_ENTER))
	{
		// User pressed [ENTER] on keyboard
		if (gNumGamepadsMissing == 1)
		{
			PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
			return 1;
		}
		else
		{
			PlayEffect(EFFECT_BADSELECT);
			MakeTwitch(gGatherPrompt, kTwitchPreset_PadlockWiggle);
		}
	}
	else if (GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		// User pressed [A] on gamepad
		if (gNumGamepadsMissing > 0)
		{
			PlayEffect(EFFECT_BADSELECT);
			MakeTwitch(gGatherPrompt, kTwitchPreset_PadlockWiggle);
		}
	}
	else if (IsCheatKeyComboDown())		// useful to test local multiplayer without having all gamepads plugged in
	{
		PlayEffect(EFFECT_ROMANCANDLE_LAUNCH);
		return 1;
	}

	return 0;
}


