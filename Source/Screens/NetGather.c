/****************************/
/* NET GATHER.C             */
/* (C) 2022 Iliyas Jorio    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "network.h"
#include "miscscreens.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupNetGatherScreen(void);
static int DoNetGatherControls(void);



/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode* gGatherPrompt = NULL;




static void UpdateNetGatherPrompt(void)
{
	static char buf[256];

	if (gIsNetworkHost)
	{
		if (Net_IsLobbyBroadcastOpen())
		{
			TextMesh_Update("WAITING FOR PLAYERS\nON LOCAL NETWORK...", 0, gGatherPrompt);
		}
		else
		{
			TextMesh_Update("COULDN'T CREATE THE GAME.", 0, gGatherPrompt);
		}
	}
	else if (gIsNetworkClient)
	{
		switch (gNumLobbiesFound)
		{
			case 0:
				snprintf(buf, sizeof(buf), "LOOKING FOR GAMES\nON LOCAL NETWORK...");
				break;

			case 1:
				snprintf(buf, sizeof(buf), "FOUND A GAME\nON LOCAL NETWORK.");
				break;

			default:
				snprintf(buf, sizeof(buf), "FOUND %d GAMES\nON LOCAL NETWORK.", gNumLobbiesFound);
				break;
		}

		TextMesh_Update(buf, 0, gGatherPrompt);
	}
}




/********************** DO GATHER SCREEN **************************/
//
// Return true if user aborts.
//

Boolean DoNetGatherScreen(void)
{
	SetupNetGatherScreen();
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

		outcome = DoNetGatherControls();

		UpdateNetGatherPrompt();

			/**************/
			/* DRAW STUFF */
			/**************/

		CalcFramesPerSecond();
		ReadKeyboard();
		Net_Tick();
		MoveObjects();
		OGL_DrawScene(DrawObjects);
	}

			/* SHOW 'OK!' */

	if (outcome >= 0)
	{
		UpdateNetGatherPrompt();
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


/********************* SETUP NET GATHER SCREEN **********************/

static void SetupNetGatherScreen(void)
{
	SetupGenericMenuScreen(true);


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
}



/***************** DO CHARACTERSELECT CONTROLS *******************/

static int DoNetGatherControls(void)
{
	if (GetNewNeedStateAnyP(kNeed_UIBack))
	{
		if (gIsNetworkHost)
		{
			Net_CloseLobby();
		}
		else if (gIsNetworkClient)
		{
			Net_CloseLobbySearch();
		}

		return -1;
	}

	return 0;
}


