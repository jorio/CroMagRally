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


static void UpdateNetGatherPrompt(void)
{
	static char buf[256];

	char* cursor = buf;
	size_t bufSize = sizeof(buf);
	int rc = 0;

	const char* output = buf;

	switch (gNetSequenceState)
	{
		case kNetSequence_Error:
		{
			// Since winsock error numbers are different from posix,
			// add a W or U tag so we know the user's OS if they report an issue.
#if _WIN32
			char errorChar = 'W';
#else
			char errorChar = 'U';
#endif
			snprintf(buf, sizeof(buf), "%s %c-%d", Localize(STR_NETWORK_ERROR), errorChar, GetLastSocketError());
			break;
		}

		case kNetSequence_SeedDesync:
			output = "ERROR: SEED DESYNC.";
			break;

		case kNetSequence_PositionDesync:
			output = "ERROR: POSITION DESYNC.";
			break;

		case kNetSequence_ErrorNoResponseFromClients:
			output = "ERROR: NO RESPONSE FROM CLIENTS.";
			break;

		case kNetSequence_ErrorNoResponseFromHost:
			output = "ERROR: NO RESPONSE FROM HOST.";
			break;

		case kNetSequence_ErrorLostPacket:
			output = "ERROR: LOST PACKET.";
			break;

		case kNetSequence_ErrorSendFailed:
			output = "ERROR: SEND FAILED.";
			break;

		case kNetSequence_ClientOfflineBecauseHostBailed:
			output = Localize(STR_HOST_ENDED_GAME);
			break;

		case kNetSequence_ClientOfflineBecauseHostUnreachable:
			output = Localize(STR_HOST_UNREACHABLE);
			break;

		case kNetSequence_ClientOfflineBecauseKicked:
			output = Localize(STR_YOU_WERE_KICKED);
			break;

		case kNetSequence_OfflineEverybodyLeft:
			output = Localize(STR_EVERYBODY_LEFT);
			break;

		case kNetSequence_HostLobbyOpen:
		{
			int numPlayers = NSpGame_GetNumActivePlayers(gNetGame);
			int numClients = numPlayers - 1;
			if (numClients <= 0)
			{
				output = Localize(STR_WAITING_FOR_PLAYERS_ON_LAN);
			}
			else
			{
				if (numClients == 1)
					rc = snprintf(cursor, bufSize, "%s", Localize(STR_1_PLAYER_CONNECTED));
				else
					rc = LocalizeWithPlaceholder(STR_N_PLAYERS_CONNECTED, cursor, bufSize, "%d", numClients);
				AdvanceTextCursor(rc, &cursor, &bufSize);
				rc = snprintf(cursor, bufSize, "\n \n");
				AdvanceTextCursor(rc, &cursor, &bufSize);
				LocalizeWithPlaceholder(STR_PRESS_XXX_TO_BEGIN, cursor, bufSize, "[%s]", "ENTER");
			}
			break;
		}

		case kNetSequence_ClientSearchingForGames:
			output = Localize(STR_SEARCHING_FOR_GAMES_ON_LAN);
			break;

		case kNetSequence_ClientFoundGames:
		{
			int numLobbiesFound = NSpSearch_GetNumGamesFound(gNetSearch);
			if (numLobbiesFound == 1)
			{
				snprintf(buf, sizeof(buf), "%s\n%s",
					Localize(STR_FOUND_1_GAME_AT),
					NSpSearch_GetHostAddress(gNetSearch, 0));
			}
			else
			{
				LocalizeWithPlaceholder(STR_FOUND_N_GAMES_ON_LAN, buf, bufSize, "%d", numLobbiesFound);
			}
			break;
		}

		case kNetSequence_ClientJoiningGame:
			snprintf(buf, sizeof(buf), "%s\n%s", Localize(STR_JOINED_GAME), Localize(STR_WAITING_FOR_HOST));
			break;

		case kNetSequence_WaitingForPlayerVehicles:
			output = Localize(STR_OTHER_PLAYERS_READYING_UP);
			break;

		case kNetSequence_GotAllPlayerVehicles:
		case kNetSequence_ClientJoinedGame:
		case kNetSequence_HostStartingGame:
			output = Localize(STR_LETS_GO);
			break;

		default:
			snprintf(buf, sizeof(buf), "SEQ %d", gNetSequenceState);
			break;
	}


	TextMesh_Update(output, 0, gGatherPrompt);
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


/********** SHOW NET GATHER SCREEN IF AN ERROR OCCURRED ***********/

void ShowPostGameNetErrorScreen(void)
{
	if (gNetSequenceState >= kNetSequence_Error)
	{
		DoNetGatherScreen();
		gNetSequenceState = kNetSequence_Offline;
	}
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
		case kNetSequence_HostLobbyOpen:
			if (GetNewNeedStateAnyP(kNeed_UIConfirm)
				&& NSpGame_GetNumActivePlayers(gNetGame) >= 2)
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


