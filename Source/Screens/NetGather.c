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
extern NSpSearchReference gNetSearch;


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
		&& NSpGame_IsAdvertising(gNetGame)
		&& NSpGame_GetNumClients(gNetGame) > 0
		;
}


static void UpdateNetGatherPrompt(void)
{
	static char buf[256];

	switch (gNetSequenceState)
	{
		case kNetSequence_Error:
		{
#if _WIN32
			char errorChar = 'W';
#else
			char errorChar = 'U';
#endif
			snprintf(buf, sizeof(buf), "NETWORK ERROR %c-%d", errorChar, GetLastSocketError());
			break;
		}

		case kNetSequence_HostWaitingForAnyPlayersToJoinLobby:
			snprintf(buf, sizeof(buf), "WAITING FOR PLAYERS\nON LOCAL NETWORK...");
			break;

		case kNetSequence_HostWaitingForMorePlayersToJoinLobby:
		{
			int numClientsConnectedToHost = NSpGame_GetNumClients(gNetGame);
			if (numClientsConnectedToHost == 1)
			{
				snprintf(buf, sizeof(buf), "1 PLAYER CONNECTED\n\nPRESS ENTER TO BEGIN");
			}
			else
			{
				snprintf(buf, sizeof(buf), "%d PLAYERS CONNECTED\n\nPRESS ENTER TO BEGIN", numClientsConnectedToHost);
			}
			break;
		}

		case kNetSequence_ClientSearchingForGames:
			snprintf(buf, sizeof(buf), "SEARCHING FOR GAMES\nON LOCAL NETWORK...");
			break;

		case kNetSequence_ClientFoundGames:
		{
			int numLobbiesFound = NSpSearch_GetNumGamesFound(gNetSearch);
			if (numLobbiesFound == 1)
			{
				snprintf(buf, sizeof(buf), "FOUND A GAME AT\n%s", NSpSearch_GetHostAddress(gNetSearch, 0));
			}
			else
			{
				snprintf(buf, sizeof(buf), "FOUND %d GAMES\nON LOCAL NETWORK.", numLobbiesFound);
			}
			break;
		}

		case kNetSequence_ClientJoiningGame:
		{
			snprintf(buf, sizeof(buf), "JOINED THE GAME.\nWAITING FOR HOST...");
			break;
		}

		case kNetSequence_WaitingForPlayerVehicles:
			snprintf(buf, sizeof(buf), "THE OTHER PLAYERS\nARE READYING UP...\n");
			break;

		case kNetSequence_GotAllPlayerVehicles:
		case kNetSequence_ClientJoinedGame:
		case kNetSequence_HostStartingGame:
			snprintf(buf, sizeof(buf), "LET'S GO!");
			break;

		default:
			snprintf(buf, sizeof(buf), "SEQ %d", gNetSequenceState);
			break;
	}


	TextMesh_Update(buf, 0, gGatherPrompt);
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

		UpdateNetSequence();

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
		EndNetworkGame();
		return -1;
	}

	switch (gNetSequenceState)
	{
		case kNetSequence_HostWaitingForMorePlayersToJoinLobby:
			if (GetNewNeedStateAnyP(kNeed_UIConfirm))
			{
				gNetSequenceState = kNetSequence_HostReadyToStartGame;
			}
			break;

		case kNetSequence_HostStartingGame:
		case kNetSequence_ClientJoinedGame:
		case kNetSequence_GotAllPlayerVehicles:
		case kNetSequence_GameLoop:
			return 1;
	}

	return 0;
}


