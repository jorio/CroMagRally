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

extern NSpGameReference gNetGame;


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


static bool HostReadyToStartGame(void)
{
	return gIsNetworkHost
		&& Net_IsLobbyBroadcastOpen()
		&& NSpGame_GetNumPlayersConnectedToHost(gNetGame)
		;
}


static void UpdateNetGatherPrompt(void)
{
	static char buf[256];

	if (gIsNetworkHost)
	{
		if (Net_IsLobbyBroadcastOpen())
		{
			int numClientsConnectedToHost = NSpGame_GetNumPlayersConnectedToHost(gNetGame);
			switch (numClientsConnectedToHost)
			{
				case 0:
					snprintf(buf, sizeof(buf), "WAITING FOR PLAYERS\nON LOCAL NETWORK...");
					break;

				case 1:
					snprintf(buf, sizeof(buf), "1 PLAYER CONNECTED\n\nPRESS ENTER TO BEGIN");
					break;

				default:
					snprintf(buf, sizeof(buf), "%d PLAYERS CONNECTED\n\nPRESS ENTER TO BEGIN", numClientsConnectedToHost);
					break;
			}

			TextMesh_Update(buf, 0, gGatherPrompt);
		}
		else
		{
			TextMesh_Update("FAILED TO HOST GAME\nON LOCAL NETWORK.", 0, gGatherPrompt);
		}
	}
	else if (gIsNetworkClient)
	{
		if (Net_IsLobbyBroadcastOpen())
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
		else
		{
			TextMesh_Update("FAILED TO SEARCH FOR GAMES\nON LOCAL NETWORK.", 0, gGatherPrompt);
		}

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


	if (GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		if (HostReadyToStartGame())
		{
			return 1;
		}
	}

	return 0;
}


