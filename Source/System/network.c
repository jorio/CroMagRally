/****************************/
/*   	  NETWORK.C	   	    */
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/

typedef void* NSpPlayerLeftMessage;


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include "network.h"
#include "window.h"
#include "miscscreens.h"
#include <stdlib.h>

/**********************/
/*     PROTOTYPES     */
/**********************/

static void InitPlayerNamesListBox(Rect *r, WindowPtr myDialog);
static void ShowNamesOfJoinedPlayers(void);
static OSErr Client_WaitForGameConfigInfo(void);
static OSErr HostSendGameConfigInfo(void);
static void HandleGameConfigMessage(NetConfigMessageType *inMessage);
static Boolean HandleOtherNetMessage(NSpMessageHeader	*message);
static Boolean PlayerReceiveVehicleTypeFromOthers(short *playerNum, short *charType, short *sex);

/****************************/
/*    CONSTANTS             */
/****************************/

#define	DATA_TIMEOUT	2						// # seconds for data to timeout

#define	kNBPType		"CMR5"

/**********************/
/*     VARIABLES      */
/**********************/

static int	gNumGatheredPlayers = 0;			// this is only used during gathering, gNumRealPlayers should be used during game!

int			gNetSequenceState = kNetSequence_Offline;
int			gNetSequenceError = 0;

Boolean		gNetSprocketInitialized = false;

Boolean		gIsNetworkHost = false;
Boolean		gIsNetworkClient = false;
Boolean		gNetGameInProgress = false;

NSpGameReference	gNetGame = nil;

#if 0
static Str31				gameName;
static Str31				gNetPlayerName;
static Str31				password;
static Str31				kJoinDialogLabel = "Choose a Game:";
#endif

//ListHandle		gTheList;
//short			gNumRowsInList;
Str32			gPlayerNameStrings[MAX_PLAYERS];

uint32_t			gClientSendCounter[MAX_PLAYERS];
uint32_t			gHostSendCounter;
int				gTimeoutCounter;

//NetHostControlInfoMessageType	gHostOutMess;
//NetClientControlInfoMessageType	gClientOutMess;

Boolean		gHostNetworkGame = false;
Boolean		gJoinNetworkGame = false;


/******************* INIT NETWORK MANAGER *********************/
//
// Called once at boot
//

void InitNetworkManager(void)
{
#if _WIN32
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		printf("WSAStartup failed: %d\n", result);
		return;
	}
#endif

	gNetSprocketInitialized = true;

#if 0
OSStatus    iErr;

	if ((!gOSX) || OSX_PACKAGE)
	{
	            /*********************/
	            /* INIT NET SPROCKET */
	            /*********************/

		iErr = NSpInitialize(sizeof(NetHostControlInfoMessageType), kBufferSize, kQElements, kGameID, kTimeout);
	    if (iErr)
	        DoFatalAlert("InitNetworkManager: NSpInitialize failed!");

		gNetSprocketInitialized = true;
	}
#endif
}


/******************* SHuTDOWN NETWORK MANAGER *********************/

void ShutdownNetworkManager(void)
{
	if (gNetSprocketInitialized)
	{
#if _WIN32
		WSACleanup();
#endif
	}
}


/********************** END NETWORK GAME ******************************/
//
// Called from CleanupLevel() or when a player bails from game unexpectedly.
//

void EndNetworkGame(void)
{
//	IMPLEMENT_ME_SOFT();
#if 1
OSErr	iErr;

	if ((!gNetGameInProgress) || (!gNetGame))								// only if we're running a net game
		return;

		/* THE HOST MUST TERMINATE IT ENTIRELY */

	if (gIsNetworkHost)
	{
//		Wait(40);						// do this pause to let clients sync up so they don't get the terminate message prematurely
		iErr = NSpGame_Dispose(gNetGame, kNSpGameFlag_ForceTerminateGame);	// do not renegotiate a new host
		if (iErr)
			DoFatalAlert("EndNetworkGame: NSpGame_Dispose failed!");
	}

			/* CLIENTS CAN JUST BAIL NORMALLY */
	else
	{
		iErr = NSpGame_Dispose(gNetGame, 0);
		if (iErr)
			DoFatalAlert("EndNetworkGame: NSpGame_Dispose failed!");
	}
#endif


	gNetGameInProgress 	= false;
	gIsNetworkHost	= false;
	gIsNetworkClient	= false;
	gNetGame			= nil;
	gNumGatheredPlayers	= 0;
}


#pragma mark -


void UpdateNetSequence(void)
{
	switch (gNetSequenceState)
	{
		case kNetSequence_HostWaitingForAnyPlayersToJoinLobby:
		case kNetSequence_HostWaitingForMorePlayersToJoinLobby:
		{
			NSpGame_AcceptNewClient(gNetGame);

			NSpMessageHeader* message = NSpMessage_Get(gNetGame);

			if (message) switch (message->what)
			{
				case kNSpJoinRequest:
				{
					// Acknowledge the request
					NSpGame_AckJoinRequest(gNetGame, message);
					break;
				}
			}

			if (NSpGame_GetNumClients(gNetGame) > 0)
			{
				gNetSequenceState = kNetSequence_HostWaitingForMorePlayersToJoinLobby;
			}
			else if (NSpGame_GetNumClients(gNetGame) == 0)
			{
				gNetSequenceState = kNetSequence_HostWaitingForAnyPlayersToJoinLobby;
			}

			break;
		}

		case kNetSequence_HostReadyToStartGame:
			Net_CloseLobby();
			if (noErr == HostSendGameConfigInfo())
			{
				gNetSequenceState = kNetSequence_HostStartingGame;
			}
			else
			{
				gNetSequenceState = kNetSequence_Error;
			}
			break;

		case kNetSequence_ClientSearchingForGames:
			if (Net_GetNumLobbiesFound() > 0)
			{
				gNetSequenceState = kNetSequence_ClientFoundGames;
			}
			break;

		case kNetSequence_ClientFoundGames:
			if (Net_GetNumLobbiesFound() == 0)
			{
				gNetSequenceState = kNetSequence_ClientSearchingForGames;
			}
			else
			{
				Net_CloseLobbySearch();

				gNetGame = Net_JoinLobby(0);

				if (gNetGame)
				{
					gNetSequenceState = kNetSequence_ClientJoiningGame;
				}
				else
				{
					gNetSequenceState = kNetSequence_Error;
				}
			}
			break;

		case kNetSequence_ClientJoiningGame:
		{
			NSpMessageHeader* message = NSpMessage_Get(gNetGame);

			if (message) switch (message->what)
			{
				case	kNetConfigureMessage:										// GOT GAME START INFO
					HandleGameConfigMessage((NetConfigMessageType *)message);
					gNetSequenceState = kNetSequence_ClientJoinedGame;
					break;

				case 	kNSpGameTerminated:											// Host terminated the game :(
					break;

				case	kNSpJoinApproved:
					puts("Join approved!");
					// TODO: Store our client ID.
					break;

				case	kNSpPlayerLeft:												// see if someone decided to un-join
					ShowNamesOfJoinedPlayers();
					break;

				case	kNSpPlayerJoined:
					ShowNamesOfJoinedPlayers();
					break;

				case	kNSpError:
					DoFatalAlert("kNetSequence_ClientJoiningGame: message == kNSpError");
					break;

				default:
					HandleOtherNetMessage(message);
					break;
			}

			break;
		}

		case kNetSequence_WaitingForPlayerVehicles1:
		case kNetSequence_WaitingForPlayerVehicles2:
		case kNetSequence_WaitingForPlayerVehicles3:
		case kNetSequence_WaitingForPlayerVehicles4:
		case kNetSequence_WaitingForPlayerVehicles5:
		case kNetSequence_WaitingForPlayerVehicles6:
		case kNetSequence_WaitingForPlayerVehicles7:
		case kNetSequence_WaitingForPlayerVehicles8:
		case kNetSequence_WaitingForPlayerVehicles9:
		{
//			int count = 1;																// start count @ 1 since we have our own local info already
			int count = gNetSequenceState - kNetSequence_WaitingForPlayerVehicles1;

			if (count >= gNumRealPlayers)
			{
				gNetSequenceState = kNetSequence_GotAllPlayerVehicles;
			}
			else
			{
				short playerNum;
				short charType;
				short sex;

				if (PlayerReceiveVehicleTypeFromOthers(&playerNum, &charType, &sex))		// check for network message
				{
					gPlayerInfo[playerNum].vehicleType = charType;					// save this player's type
					gPlayerInfo[playerNum].sex = sex;								// save this player's sex
					gNetSequenceState++;											// inc count of received info
				}
			}

			break;
		}

	}
}



/****************** SETUP NETWORK HOSTING *********************/
//
// Called when this computer's user has selected to be a host for a net game.
//
// OUTPUT:  true == cancelled.
//
Boolean SetupNetworkHosting(void)
{
	gNetSequenceState = kNetSequence_HostOffline;


			/* GET SOME NAMES */

/*
	CopyPString(gPlayerSaveData.playerName, gNetPlayerName);
	// TODO: this is probably STR_RACE + gGameMode - GAME_MODE_MULTIPLAYERRACE
	GetIndStringC(gameName, 1000 + gGamePrefs.language, 16 + (gGameMode - GAME_MODE_MULTIPLAYERRACE));	// name of game is game mode string
*/

			/* NEW HOST GAME */

	//status = NSpGame_Host(&gNetGame, theList, MAX_PLAYERS, gameName, password, gNetPlayerName, 0, kNSpClientServer, 0);
	gNetGame = Net_CreateHostedGame();

	if (!gNetGame)
	{
		gNetSequenceState = kNetSequence_Error;
		// Don't goto failure; show the error to the player
		DoNetGatherScreen();
		return true;
	}

	gNetSequenceState = kNetSequence_HostWaitingForAnyPlayersToJoinLobby;


			/* LET USERS JOIN IN */

	if (DoNetGatherScreen())
		goto failure;



#if 0
		/*************************************/
		/* TELL ALL CLIENT PLAYERS SOME INFO */
		/*************************************/

	if (HostSendGameConfigInfo())
		goto failure;
#endif

	return false;


			/* SOMETHING WENT WRONG, SO BE GRACEFUL */

failure:
	Net_CloseLobby();
	NSpGame_Dispose(gNetGame, 0);

	return true;
}


/*************** SETUP NETWORK JOIN ************************/
//
// OUTPUT:	false == let's go!
//			true = cancel
//

Boolean SetupNetworkJoin(void)
{
OSStatus			status = noErr;

	gNetSequenceState = kNetSequence_ClientOffline;

	if (Net_CreateLobbySearch())
	{
		gNetSequenceState = kNetSequence_ClientSearchingForGames;
	}
	else
	{
		gNetSequenceState = kNetSequence_Error;
	}

	DoNetGatherScreen();
//	IMPLEMENT_ME_SOFT();
//	return true;
#if 0
NSpAddressReference	theAddress;
OSStatus			status;
int					i;


//	GameScreenToBlack();
	GammaOn();
	Enter2D(true);

	MyFlushEvents();
	InitCursor();

	gNetGame = nil;

	for (i = 0; i < MAX_PLAYERS; i++)
		gClientSendCounter[i] = 0;
	gTimeoutCounter = 0;

	CopyPString(gPlayerSaveData.playerName, gNetPlayerName);		// use loaded player's name
	password[0] = 0;

			/* DO UI FOR JOINING GAME */
			//
			//	passing an empty string (not nil) for the type causes NetSprocket to use the game id passed in to initialize
			//

	TurnOffISp();
	theAddress = NSpDoModalJoinDialog(kNBPType, kJoinDialogLabel, gNetPlayerName, password, NULL);
	TurnOnISp();

	if (theAddress == NULL)		// The user cancelled
	{
		HideCursor();
		return(true);
	}


				/* JOIN IN */

	status = NSpGame_Join(&gNetGame, theAddress, gNetPlayerName, password, 0, NULL, 0, 0);
	if (status)
	{
		HideCursor();
		return(true);												// an error will occur if user selects "blank" line in dialog above (sounds like an NSp bug to me!)
	}

	NSpReleaseAddressReference(theAddress);							// always dispose of this after _Join


			/* WAIT WHILE OTHERS JOIN ALSO */

	status = Client_WaitForGameConfigInfo();
	if (status)
	{
        if (gNetGame)
        {
            NSpGame_Dispose(gNetGame, 0);
            gNetGame = nil;
        }
	}

//	HideCursor();
//	Exit2D();
#endif
	return status;
}


#pragma mark -

/********************* DO MY CUSTOM GATHER GAME DIALOG **********************/
//
// Displays dialog which shows all currently gathered players.
//
// OUTPUT: OSErr = noErr if all's well.
//

static OSErr  Host_DoGatherPlayersDialog(void)
{
 	IMPLEMENT_ME_SOFT();
	return unimpErr;
#if 0
DialogRef 		myDialog;
DialogItemType			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
Boolean			dialogDone,cancelled = false;
ModalFilterUPP	myProc;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());

	gNumGatheredPlayers = 0;												// noone gathered yet

	myDialog = GetNewDialog(130,nil,MOVE_TO_FRONT);


			/* SET OUTLINE FOR USERITEM */

	GetDialogItem(myDialog,1,&itemType,(Handle *)&itemHandle,&itemRect);					// default button
	SetDialogItem(myDialog, 2, userItem, (Handle)NewUserItemUPP(DoBold), &itemRect);


			/* INIT LIST BOX */

	GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);					// player's box
	SetDialogItem(myDialog,4, userItem,(Handle)NewUserItemUPP(DoOutline), &itemRect);
	InitPlayerNamesListBox(&itemRect,GetDialogWindow(myDialog));													// create list manager list


				/*************/
				/* DO DIALOG */
				/*************/

	dialogDone = false;
	myProc = NewModalFilterUPP(GatherGameDialogCallback);
	while(dialogDone == false)
	{
		ModalDialog(myProc, &itemHit);
		switch (itemHit)
		{
			case 	3:
					cancelled = true;
					dialogDone = true;
					break;

			case	1:									// see if PLAY
					dialogDone = true;
					break;
		}
	}

		/* STOP ADVERTISING THIS GAME SINCE WE'RE ALL SET TO GO */

	NSpGame_EnableAdvertising(gNetGame, nil, false);


			/* CLEANUP */

	DisposeModalFilterUPP(myProc);
	DisposeDialog(myDialog);

	GameScreenToBlack();
	return(cancelled);
#endif
}


/******************** INIT GATHER LIST BOX *************************/
//
// Creates the List Manager list box which will contain a list of all the joiners in this game.
//

#if 0
static void InitPlayerNamesListBox(Rect *r, WindowPtr myDialog)
{
Rect	dataBounds;
Point	cSize;

	r->right -= 15;														// make room for scroll bars & outline
	r->top += 1;
	r->bottom -= 1;
	r->left += 1;

	gNumRowsInList = 0;
	SetRect(&dataBounds,0,0,1,gNumRowsInList);							// no entries yet
	cSize.h = cSize.v = 0;
	gTheList = LNew(r, &dataBounds, cSize, 0, myDialog, true, false, false, false);

	ShowNamesOfJoinedPlayers();
}
#endif



/**************** GATHER GAME DIALOG CALLBACK *************************/

#if 0
static Boolean GatherGameDialogCallback (DialogRef dp,EventRecord *event, short *item)
{
	IMPLEMENT_ME_SOFT();
	return true;
#if 0
char 			c;
Point			eventPoint;
static	long	tick = 0;
NSpMessageHeader	*message;

	dp; item;

				/* HANDLE DIALOG EVENTS */

	SetPort(GetDialogPort(dp));										// make sure we're drawing to this dialog

	switch(event->what)
	{
		case	keyDown:								// we have a key press
				c = event->message & 0x00FF;			// what character is it?
				break;

		case	mouseDown:								// mouse was clicked
				eventPoint = event->where;				// get the location of click
				GlobalToLocal (&eventPoint);			// got to make it local
				break;
	}

			/*******************************************/
			/* CHECK FOR OTHER PLAYERS WANTING TO JOIN */
			/*******************************************/

	while ((message = NSpMessage_Get(gNetGame)) != NULL)	// read Net message
	{
		switch(message->what)
		{
			case	kNSpPlayerLeft:						// see if someone decided to un-join
					ShowNamesOfJoinedPlayers();
					break;

			case 	kNSpPlayerJoined:					// see if we've got a new player joining
			case	kNSpJoinRequest:
					ShowNamesOfJoinedPlayers();
					break;

			default:
					HandleOtherNetMessage(message);
		}
		NSpMessage_Release(gNetGame, message);
	}

			/* KEEP MUSIC PLAYING */

	if (gSongPlayingFlag)
		MoviesTask(gSongMovie, 0);

	return(false);
#endif
}
#endif


/***************** SHOW NAMES OF JOINED PLAYERS ************************/
//
// For the Gather Game and Wait for Config dialogs, it displays list of joined players by updating
// the List Manager list for this dialog.
//

static void ShowNamesOfJoinedPlayers(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
short	i;
Cell	theCell;
NSpPlayerEnumerationPtr	players;
OSStatus	status;


	status = NSpPlayer_GetEnumeration(gNetGame, &players);
	if (status != noErr)
		return;

		/* COPY NBP NAMES INTO LIST BUFFER */

	gNumGatheredPlayers =  players->count;
	for (i=0; i < gNumGatheredPlayers; i++)
	{
		NSpPlayerInfoPtr	thePlayer;

		NSpPlayer_GetInfo(gNetGame, players->playerInfo[i]->id, &thePlayer);

		CopyPStr(thePlayer->name, gPlayerNameStrings[i]);
	}

	NSpPlayer_ReleaseEnumeration(gNetGame, players);


		/* DELETE ALL EXISTING ROWS IN LIST */

	LDelRow(0, 0, gTheList);
	gNumRowsInList = 0;


			/* ADD NAMES TO LIST */

	if (gNumGatheredPlayers > 0)
		LAddRow(gNumGatheredPlayers, 0, gTheList);		// create rows


	for (i=0; i < gNumGatheredPlayers; i++)
	{
		if (i == (gNumGatheredPlayers-1))				// reactivate draw on last cell
			LSetDrawingMode(true,gTheList);							// turn on updating
		theCell.h = 0;
		theCell.v = i;
		LSetCell(&gPlayerNameStrings[i][1], gPlayerNameStrings[i][0], theCell, gTheList);
		gNumRowsInList++;
	}
#endif
}




#pragma mark -

/*********************** SEND GAME CONFIGURATION INFO *******************************/
//
// Once everyone is in and we (the host) start things, then send this to all players to tell them we're on!
//

static OSErr HostSendGameConfigInfo(void)
{
OSStatus				status;
NetConfigMessageType	message;
#if 0
NSpPlayerEnumerationPtr	playerList;
NSpPlayerID				hostID,clientID;
short					i,p;
NSpPlayerInfoPtr		playerInfoPtr;
#endif

			/* GET PLAYER INFO */

#if 0
	hostID = NSpPlayer_GetMyID(gNetGame);							// get my/host ID
	status = NSpPlayer_GetEnumeration(gNetGame, &playerList);
	gNumRealPlayers = playerList->count;							// get # players (host + clients)
#else
	int numClients = NSpGame_GetNumClients(gNetGame);
	gNumRealPlayers = 1 + numClients;
#endif

	gMyNetworkPlayerNum = 0;										// the host is always player #0


			/***********************************************/
			/* SEND GAME CONFIGURATION INFO TO ALL PLAYERS */
			/***********************************************/
			//
			// Send one message at a time to each individual client player with
			// specific info for each client.
			//

	int p = 1;														// start assigning player nums at 1 since Host is always #0
#if 0
	for (i = 0; i < gNumRealPlayers; i++)
	{
		playerInfoPtr =  playerList->playerInfo[i];					// point to NSp's player info list

		gPlayerInfo[i].nspPlayerID = clientID = playerInfoPtr->id;	// get NSp's playerID (for use when player leaves game)

		if (clientID != hostID)										// don't send start info to myself/host

#else
	for (int i = 0; i < numClients; i++)
	{
		int clientID = kNSpClientID0 + i;

		gPlayerInfo[i].nspPlayerID = clientID;						// get NSp's playerID (for use when player leaves game)
#endif

		{
					/* MAKE NEW MESSAGE */

			NSpClearMessageHeader(&message.h);
			message.h.to 			= clientID;						// send to this client
			message.h.what 			= kNetConfigureMessage;			// set message type
			message.h.messageLen 	= sizeof(message);				// set size of message

			message.gameMode 		= gGameMode;					// set game Mode
			message.age		 		= gTheAge;						// set Age
			message.trackNum		= gTrackNum;					// set track #
			message.numPlayers 		= gNumRealPlayers;				// set # players
			message.playerNum 		= p++;							// set player #
			message.numTracksCompleted = gGamePrefs.tournamentProgression.numTracksCompleted;
			message.difficulty		= gGamePrefs.difficulty;		// set difficulty
			message.tagDuration		= gGamePrefs.tagDuration;		// set tag duration

			status = NSpMessage_Send(gNetGame, &message.h, kNSpSendFlag_Registered);	// send message
			if (status)
			{
				DoAlert("HostSendGameConfigInfo: NSpMessage_Send failed!");
				break;
			}
		}
	}

			/************/
			/* CLEAN UP */
			/************/

#if 0
	NSpPlayer_ReleaseEnumeration(gNetGame,playerList);					// dispose of player list
#endif

	return(status);
}




/******************** WAIT FOR GAME CONFIGURATION INFO *****************************/
//
// Waits for others to join and then Host to tell me which player # I am et.al.
//
// OUTPUT:	OSErr == noErr if all went well, otherwise aborted.
//

static OSErr Client_WaitForGameConfigInfo(void)
{
	IMPLEMENT_ME_SOFT();
	return unimpErr;
#if 0
DialogRef	myDialog;
Boolean		dialogDone,cancelled;
DialogItemType			itemType,itemHit,i;
ControlHandle	itemHandle;
Rect			itemRect;
ModalFilterUPP	myProc;
NSpPlayerEnumerationPtr	playerList;
NSpPlayerInfoPtr		playerInfoPtr;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	gNumGatheredPlayers = 0;												// noone gathered yet


			/* FIRST GET GAME PLAYER ID'S */

	NSpPlayer_GetEnumeration(gNetGame, &playerList);
	gNumRealPlayers = playerList->count;									// get # players (host + clients)
	for (i = 0; i < gNumRealPlayers; i++)
	{
		playerInfoPtr =  playerList->playerInfo[i];					// point to NSp's player info list
		gPlayerInfo[i].nspPlayerID = playerInfoPtr->id;					// get NSp's playerID (for use when player leaves game)
	}
	NSpPlayer_ReleaseEnumeration(gNetGame,playerList);					// dispose of player list


			/********************************************/
			/* MAKE "WAITING FOR OTHERS TO JOIN" DIALOG */
			/********************************************/

	myDialog = GetNewDialog(131,nil,MOVE_TO_FRONT);


			/* SET OUTLINE FOR USERITEM */

	GetDialogItem(myDialog,1,&itemType,(Handle *)&itemHandle,&itemRect);					// default button
	SetDialogItem(myDialog, 3, userItem, (Handle)NewUserItemUPP(DoBold), &itemRect);


			/* INIT LIST BOX */

	GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);					// player's box
	SetDialogItem(myDialog,4, userItem,(Handle)NewUserItemUPP(DoOutline), &itemRect);
	InitPlayerNamesListBox(&itemRect,GetDialogWindow(myDialog));													// create list manager list


	/* LET'S WAIT FOR HOST TO TELL US SOMETHING, OR WE CAN ALWAYS CANCEL */

	dialogDone = cancelled = false;
	myProc = NewModalFilterUPP(Client_WaitForGameConfigInfoDialogCallback);
	while(dialogDone == false)
	{
		ModalDialog(myProc, &itemHit);
		switch (itemHit)
		{
				/* PLAYER CANCELLED */

			case 	1:
					cancelled = true;
					dialogDone = true;
//					NSpGame_Dispose(gNetGame, 0);											// tell host that I'm gone
					EndNetworkGame();
					break;

			case	100:
					dialogDone = true;
					break;
			default:
				dialogDone = false;
			break;
		}
	}

	DisposeModalFilterUPP(myProc);
	DisposeDialog(myDialog);
	return(cancelled);
#endif
}


/********************* WAIT FOR GAME CONFIG INFO: DIALOG CALLBACK ***************************/
//
// Returns TRUE if game start info was received.  Upon return, "item" will be set to 100.
//

#if 0
static Boolean Client_WaitForGameConfigInfoDialogCallback (DialogRef dp,EventRecord *event, short *item)
{
	IMPLEMENT_ME_SOFT();
	return false;
#if 0
NSpMessageHeader *message;
Boolean handled = false;

	SetPort(GetDialogPort(dp));										// make sure we're drawing to this dialog


			/* HANDLE NET SPROCKET EVENTS */

	while ((message = NSpMessage_Get(gNetGame)) != nil)							// get message from Net
	{
		switch(message->what)													// handle message
		{
			case	kNetConfigureMessage:										// GOT GAME START INFO
					HandleGameConfigMessage((NetConfigMessageType *)message);
					*item = 100;
					handled = true;
					goto got_config;
					break;

			case 	kNSpGameTerminated:											// Host terminated the game :(
					*item = 1;
					handled = true;
					break;

			case	kNSpJoinApproved:
					break;

			case	kNSpPlayerLeft:												// see if someone decided to un-join
					ShowNamesOfJoinedPlayers();
					break;

			case	kNSpPlayerJoined:
					ShowNamesOfJoinedPlayers();
					break;

			case	kNSpError:
					DoFatalAlert("Client_WaitForGameConfigInfoDialogCallback: message == kNSpError");
					break;

			default:
					HandleOtherNetMessage(message);

		}
		NSpMessage_Release(gNetGame, message);										// dispose of message
	}

got_config:

			/* HANDLE DIALOG EVENTS */

	switch (event->what)
	{
		case keyDown:
			switch (event->message & charCodeMask)
			{
				case 	0x03:  					// Enter
				case 	0x0D: 					// Return
						*item = 1;
						handled = true;
						break;

				case 	0x1B:  					// Escape
						*item = 1;
						handled = true;
						break;

				case 	'.':  					// Command-period
						if (event->modifiers & cmdKey)
						{
							*item = 1;
							handled = true;
						}
						break;
			}
	}

			/* KEEP MUSIC PLAYING */

	if (gSongPlayingFlag)
		MoviesTask(gSongMovie, 0);


	return(handled);
#endif
}
#endif




/************************* HANDLE GAME CONFIGURATION MESSAGE *****************************/
//
// Called while polling in Client_WaitForGameConfigInfoDialogCallback.
//

static void HandleGameConfigMessage(NetConfigMessageType *inMessage)
{
	//TODO: fill in player info

	gGameMode 			= inMessage->gameMode;
	gTheAge 			= inMessage->age;
	gTrackNum 			= inMessage->trackNum;
	gNumRealPlayers 	= inMessage->numPlayers;
	gMyNetworkPlayerNum = inMessage->playerNum;

	// TODO: Make this stuff transient!
	puts("TODO: make net game settings transient!");
	gGamePrefs.difficulty = inMessage->difficulty;
	gGamePrefs.tagDuration = inMessage->tagDuration;
#if 0
	if ((inMessage->numAgesCompleted & AGE_MASK_AGE) > GetNumAgesCompleted())	// if better than our current game, then pseudo-logout that saved game
		gSavedPlayerIsLoaded = false;
	gPlayerSaveData.numAgesCompleted = inMessage->numAgesCompleted;
#else
	gGamePrefs.tournamentProgression.numTracksCompleted = inMessage->numTracksCompleted;
#endif
}


#pragma mark -


/********************* HOST WAIT FOR PLAYERS TO PREPARE LEVEL *******************************/
//
// Called right beofre PlayArea().  This waits for the sync message from the other client players
// indicating that they are ready to start playing.
//

void HostWaitForPlayersToPrepareLevel(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
OSStatus				status;
NetSyncMessageType		outMess;
NSpMessageHeader 		*inMess;
Boolean 				sync = false;
short					n = 1;					// start @ 1 because the host (us) is already ready

int						startTick = TickCount();

		/********************************/
		/* WAIT FOR ALL CLIENTS TO SYNC */
		/********************************/

	while(!sync)
	{
		inMess = NSpMessage_Get(gNetGame);					// get message
		if (inMess)
		{
			switch(inMess->what)
			{
				case	kNetSyncMessage:
						n++;								// we got another player
						if (n == gNumRealPlayers)				// see if that's all of them
							sync = true;
						break;

				case	kNSpError:
						DoFatalAlert("HostWaitForPlayersToPrepareLevel: message == kNSpError");
						break;

				default:
						HandleOtherNetMessage(inMess);
			}
			NSpMessage_Release(gNetGame, inMess);			// dispose of message
		}

		if (gSongPlayingFlag)												// keep music playing
			MoviesTask(gSongMovie, 0);

		if ((TickCount() - startTick) > (60 * 60 * 2))			// if no response for 2 minutes, then time out
		{
			DoFatalAlert("No Response from other player(s), something has gone wrong.");
		}
	}



		/*******************************/
		/* TELL ALL CIENTS WE'RE READY */
		/*******************************/

	NSpClearMessageHeader(&outMess.h);
	outMess.h.to 			= kNSpAllPlayers;						// send to all clients
	outMess.h.what 			= kNetSyncMessage;						// set message type
	outMess.h.messageLen 	= sizeof(outMess);						// set size of message
	outMess.playerNum 		= 0;									// (not used this time)
	status = NSpMessage_Send(gNetGame, &outMess.h, kNSpSendFlag_Registered);	// send message
	if (status)
		DoFatalAlert("HostWaitForPlayersToPrepareLevel: NSpMessage_Send failed!");
#endif
}



/********************* CLIENT TELL HOST LEVEL IS PREPARED *******************************/
//
// Called right beofre PlayArea().  This waits for the sync message from the other client players
// indicating that they are ready to start playing.
//

void ClientTellHostLevelIsPrepared(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
OSStatus				status;
NetSyncMessageType		outMess;
Boolean 				sync = false;
NSpMessageHeader 		*inMess;

		/***********************************/
		/* TELL THE HOST THAT WE ARE READY */
		/***********************************/

	NSpClearMessageHeader(&outMess.h);
	outMess.h.to 			= kNSpHostOnly;										// send to this host
	outMess.h.what 			= kNetSyncMessage;									// set message type
	outMess.h.messageLen 	= sizeof(outMess);									// set size of message
	outMess.playerNum 		= gMyNetworkPlayerNum;										// set player num
	status = NSpMessage_Send(gNetGame, &outMess.h, kNSpSendFlag_Registered);	// send message
	if (status)
		DoFatalAlert("ClientTellHostLevelIsPrepared: NSpMessage_Send failed!");


		/**************************/
		/* WAIT FOR HOST TO REPLY */
		/**************************/

	while(!sync)
	{
		inMess = NSpMessage_Get(gNetGame);											// get message
		if (inMess)
		{
			switch(inMess->what)
			{
				case	kNetSyncMessage:
						sync = true;
						break;

				case	kNSpError:
						DoFatalAlert("HostWaitForPlayersToPrepareLevel: message == kNSpError");
						break;

				default:
						HandleOtherNetMessage(inMess);
			}
			NSpMessage_Release(gNetGame, inMess);									// dispose of message
		}

		if (gSongPlayingFlag)												// keep music playing
			MoviesTask(gSongMovie, 0);

	}
#endif
}


#pragma mark -


/************** SEND HOST CONTROL INFO TO CLIENTS *********************/
//
// The host sends this at the beginning of each frame to all of the network clients.
// This data contains the gFramesPerSecond/Frac info plus the key controls state bitfields for each player.
//

void HostSend_ControlInfoToClients(void)
{
	IMPLEMENT_ME_SOFT();
#if 0

OSStatus						status;
short							i;


				/* BUILD MESSAGE */

	NSpClearMessageHeader(&gHostOutMess.h);

	gHostOutMess.h.to 			= kNSpAllPlayers;						// send to all clients
	gHostOutMess.h.what 		= kNetHostControlInfoMessage;			// set message type
	gHostOutMess.h.messageLen 	= sizeof(gHostOutMess);						// set size of message

	gHostOutMess.frameCounter	= gHostSendCounter++;					// send the frame counter & inc
	gHostOutMess.fps 			= gFramesPerSecond;						// fps
	gHostOutMess.fpsFrac		= gFramesPerSecondFrac;					// fps frac
	gHostOutMess.randomSeed		= MyRandomLong();						// send the host's current random value for sync verification

	for (i = 0; i < MAX_PLAYERS; i++)								// control bits
	{
		gHostOutMess.controlBits[i] = gPlayerInfo[i].controlBits;
		gHostOutMess.controlBitsNew[i] = gPlayerInfo[i].controlBits_New;
		gHostOutMess.analogSteering[i] = gPlayerInfo[i].analogSteering;
	}

			/* SEND IT */

	status = NSpMessage_Send(gNetGame, &gHostOutMess.h, kNSpSendFlag_Registered);
	if (status)
		DoFatalAlert("HostSend_ControlInfoToClients: NSpMessage_Send failed!");
#endif
}


/************** GET NETWORK CONTROL INFO FROM HOST *********************/
//
// The client reads this from the host at the beginning of each frame.
// This data will contain the fps and control bitfield info for each player.
//

void ClientReceive_ControlInfoFromHost(void)
{
	IMPLEMENT_ME_SOFT();
#if 0

NetHostControlInfoMessageType		*mess;
NSpMessageHeader 					*inMess;
uint32_t								tick,i;
Boolean								gotIt = false;


	tick = TickCount();														// init tick for timeout

	do
	{
		inMess = NSpMessage_Get(gNetGame);									// get message
		if (inMess)
		{
			tick = TickCount();												// reset tick for timeout
			switch(inMess->what)
			{
				case	kNetHostControlInfoMessage:
						mess = (NetHostControlInfoMessageType *)inMess;

						if (mess->frameCounter < gHostSendCounter)			// see if this is an old packet, possibly a duplicate.  If so, skip it
							break;
						if (mess->frameCounter > gHostSendCounter)			// see if we skipped a packet; one must have gotten lost
							DoFatalAlert("ClientReceive_ControlInfoFromHost: It seems Net Sprocket has lost a packet");
						gHostSendCounter++;									// inc host counter since the next packet we get will be +1

						gFramesPerSecond 		= mess->fps;
						gFramesPerSecondFrac 	= mess->fpsFrac;

						if (MyRandomLong() != mess->randomSeed)				// verify that host's random # is in sync with ours!
						{
							DoFatalAlert("ClientReceive_ControlInfoFromHost: Not in sync!  Net Sprocket must have lost some data.");
						}

						for (i = 0; i < MAX_PLAYERS; i++)					// control bits
						{
							gPlayerInfo[i].controlBits 		= mess->controlBits[i];
							gPlayerInfo[i].controlBits_New 	= mess->controlBitsNew[i];
							gPlayerInfo[i].analogSteering	= mess->analogSteering[i];
						}

						gotIt = true;
						break;

				case	kNSpError:
						DoFatalAlert("ClientReceive_ControlInfoFromHost: message == kNSpError");
						break;

				default:
						if (HandleOtherNetMessage(inMess))
							return;
			}
			NSpMessage_Release(gNetGame, inMess);			// dispose of message
		}

				/* SEE IF WE ARE NOT GETTING THE PACKET */
				//
				// If this happens, then it is possible that Net Sprocket lost a packet.  There is no way to know who's packet got lost
				// so go ahead and send our most recent packet again in case it was us.  The Host will throw out any dupes that it gets.
				//

		if ((TickCount() - tick) > (DATA_TIMEOUT*60))		// see if we've been waiting longer than n seconds
		{
			gTimeoutCounter++;								// keep track of how often this happens
			if (gTimeoutCounter > 3)
				DoFatalAlert("ClientReceive_ControlInfoFromHost: the network is losing too much data, must abort.");

			NSpMessage_Send(gNetGame, &gClientOutMess.h, kNSpSendFlag_Registered);	// resend the last message
			tick = TickCount();														// reset tick
		}

	}while(!gotIt);

	gTimeoutCounter = 0;
#endif
}



/************** CLIENT SEND CONTROL INFO TO HOST *********************/
//
// At the end of each frame, the client sends the new control state info to the host for
// the next frame.
//

void ClientSend_ControlInfoToHost(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
OSStatus						status;


				/* BUILD MESSAGE */

	NSpClearMessageHeader(&gClientOutMess.h);

	gClientOutMess.h.to 			= kNSpHostOnly;							// send to Host
	gClientOutMess.h.what 			= kNetClientControlInfoMessage;			// set message type
	gClientOutMess.h.messageLen 	= sizeof(gClientOutMess);				// set size of message

	gClientOutMess.frameCounter		= gClientSendCounter[gMyNetworkPlayerNum]++;	// send client frame counter & inc
	gClientOutMess.playerNum		= gMyNetworkPlayerNum;
	gClientOutMess.controlBits 		= gPlayerInfo[gMyNetworkPlayerNum].controlBits;
	gClientOutMess.controlBitsNew  	= gPlayerInfo[gMyNetworkPlayerNum].controlBits_New;
	gClientOutMess.analogSteering 	= gPlayerInfo[gMyNetworkPlayerNum].analogSteering;


			/* SEND IT */

	status = NSpMessage_Send(gNetGame, &gClientOutMess.h, kNSpSendFlag_Registered);
//	if (status)
//		DoFatalAlert("ClientSend_ControlInfoToHost: NSpMessage_Send failed!");
#endif
}


/*************** HOST GET CONTROL INFO FROM CLIENTS ***********************/

void HostReceive_ControlInfoFromClients(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
NetClientControlInfoMessageType		*mess;
NSpMessageHeader 					*inMess;
uint32_t								tick;
Boolean								gotIt = false;
short								n,i;


	n = 1;                                                  // start @ 1 because the host already has his own info
	tick = TickCount();										// start tick for timeout

	while(n < gNumRealPlayers)								// loop until I've got the message from all players
	{
		inMess = NSpMessage_Get(gNetGame);					// get message
		if (inMess)
		{
			tick = TickCount();								// reset tick for timeout
			switch(inMess->what)
			{
				case	kNetClientControlInfoMessage:
						mess = (NetClientControlInfoMessageType *)inMess;

						i = mess->playerNum;									// get player #

						if (mess->frameCounter < gClientSendCounter[i])			// see if this is an old packet, possibly a duplicate.  If so, skip it
							break;
						if (mess->frameCounter > gClientSendCounter[i])			// see if we skipped a packet; one must have gotten lost
							DoFatalAlert("HostReceive_ControlInfoFromClients: It seems Net Sprocket has lost a packet");
						gClientSendCounter[i]++;								// inc counter since the next packet we get will be +1


						gPlayerInfo[i].controlBits	= mess->controlBits;
						gPlayerInfo[i].controlBits_New = mess->controlBitsNew;
						gPlayerInfo[i].analogSteering = mess->analogSteering;

						n++;
						break;

				case	kNSpError:
						DoFatalAlert("HostReceive_ControlInfoFromClients: message == kNSpError");
						break;

				default:
						if (HandleOtherNetMessage(inMess))
							return;

			}
			NSpMessage_Release(gNetGame, inMess);			// dispose of message
		}

		if ((TickCount() - tick) > (DATA_TIMEOUT*60))		// see if we've been waiting longer than n seconds
		{
			gTimeoutCounter++;								// keep track of how often this happens
			if (gTimeoutCounter > 3)
				DoFatalAlert("HostReceive_ControlInfoFromClients: the network is losing too much data, must abort.");

			NSpMessage_Send(gNetGame, &gHostOutMess.h, kNSpSendFlag_Registered);
			tick = TickCount();														// reset tick
		}

	}
#endif
}


#pragma mark -


/********************* PLAYER BROADCAST VEHICLE TYPE *******************************/
//
// Tell all of the other net players what character type we want to be.
//

void PlayerBroadcastVehicleType(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
OSStatus					status;
NetPlayerCharTypeMessage	outMess;


				/* BUILD MESSAGE */

	NSpClearMessageHeader(&outMess.h);

	outMess.h.to 			= kNSpAllPlayers;						// send to all clients
	outMess.h.what 			= kNetPlayerCharTypeMessage;			// set message type
	outMess.h.messageLen 	= sizeof(outMess);						// set size of message

	outMess.playerNum		= gMyNetworkPlayerNum;					// player #
	outMess.vehicleType		= gPlayerInfo[gMyNetworkPlayerNum].vehicleType;
	outMess.sex				= gPlayerInfo[gMyNetworkPlayerNum].sex;

			/* SEND IT */

	status = NSpMessage_Send(gNetGame, &outMess.h, kNSpSendFlag_Registered);
	if (status)
		DoFatalAlert("PlayerBroadcastVehicleType: NSpMessage_Send failed!");
#endif
}

/***************** GET VEHICLE SELECTION FROM NET PLAYERS ***********************/

void GetVehicleSelectionFromNetPlayers(void)
{
	IMPLEMENT_ME_SOFT();
	gNetSequenceState = kNetSequence_WaitingForPlayerVehicles1;
	DoNetGatherScreen();
#if 0
short	playerNum, charType, count, sex;

	ShowLoadingPicture();													// show something while we wait

	count = 1;																// start count @ 1 since we have our own local info already

	do
	{
		if (PlayerReceiveVehicleTypeFromOthers(&playerNum, &charType, &sex))		// check for network message
		{
			gPlayerInfo[playerNum].vehicleType = charType;					// save this player's type
			gPlayerInfo[playerNum].sex = sex;								// save this player's sex
			count++;														// inc count of received info
		}

		if (gSongPlayingFlag)												// keep music playing
			MoviesTask(gSongMovie, 0);

	}while(count < gNumRealPlayers);
#endif
}



/*************** PLAYER RECEIVE CHARACTER TYPE FROM OTHERS ***********************/
//
// Receive above message from other players.
//
// OUTPUT: true if got a char type, playerNum/charType
//

static Boolean PlayerReceiveVehicleTypeFromOthers(short *playerNum, short *charType, short *sex)
{
	IMPLEMENT_ME_SOFT(); return false;
#if 0
NetPlayerCharTypeMessage		*mess;
NSpMessageHeader 				*inMess;
Boolean							gotType = false;

	inMess = NSpMessage_Get(gNetGame);					// get message
	if (inMess)
	{
		switch(inMess->what)
		{
			case	kNetPlayerCharTypeMessage:
					mess = (NetPlayerCharTypeMessage *)inMess;

					*playerNum	= mess->playerNum;					// get player #
					*charType	= mess->vehicleType;				// get character type
					*sex		= mess->sex;						// get character sex
					gotType 	= true;
					break;

			case	kNSpError:
					DoFatalAlert("PlayerReceiveVehicleTypeFromOthers: message == kNSpError");
					break;

			default:
					HandleOtherNetMessage(inMess);

		}
		NSpMessage_Release(gNetGame, inMess);			// dispose of message
	}

	return(gotType);
#endif
}


/******************* HANDLE OTHER NET MESSAGE ***********************/
//
// Called when other message handler's get a message that they don't expect to get.
//
// OUTPUT: returns TRUE if game terminated
//

static Boolean HandleOtherNetMessage(NSpMessageHeader	*message)
{
	printf("%s: %08x\n", __func__, message->what);
	IMPLEMENT_ME_SOFT(); return true;
#if 0

	switch(message->what)
	{

					/* AN ERROR MESSAGE */

		case	kNSpError:
				DoFatalAlert("HandleOtherNetMessage: kNSpError");


					/* A PLAYER UNEXPECTEDLY HAS LEFT THE GAME */

		case	kNSpPlayerLeft:
				PlayerUnexpectedlyLeavesGame((NSpPlayerLeftMessage *)message);
				break;

					/* THE HOST HAS UNEXPECTEDLY LEFT THE GAME */

		case	kNSpGameTerminated:
				DoAlert("Game Terminated: The Host has unexpectedly quit the game.");
				EndNetworkGame();
				gGameOver = true;
				break;

					/* NULL PACKET */

		case	kNetNullPacket:
				break;

		case	kNSpJoinRequest:
				DoFatalAlert("HandleOtherNetMessage: kNSpJoinRequest");

		case	kNSpJoinApproved:
				DoFatalAlert("HandleOtherNetMessage: kNSpJoinApproved");

		case	kNSpJoinDenied:
				DoFatalAlert("HandleOtherNetMessage: kNSpJoinDenied");

		case	kNSpPlayerJoined:
				DoFatalAlert("HandleOtherNetMessage: kNSpPlayerJoined");

		case	kNSpHostChanged:
				DoFatalAlert("HandleOtherNetMessage: kNSpHostChanged");

		case	kNSpGroupCreated:
				DoFatalAlert("HandleOtherNetMessage: kNSpGroupCreated");

		case	kNSpGroupDeleted:
				DoFatalAlert("HandleOtherNetMessage: kNSpGroupDeleted");

		case	kNSpPlayerAddedToGroup:
				DoFatalAlert("HandleOtherNetMessage: kNSpPlayerAddedToGroup");

		case	kNSpPlayerRemovedFromGroup:
				DoFatalAlert("HandleOtherNetMessage: kNSpPlayerAddedToGroup");

		case	kNSpPlayerTypeChanged:
				DoFatalAlert("HandleOtherNetMessage: kNSpPlayerAddedToGroup");

		default:
				DoFatalAlert("HandleOtherNetMessage: unknown");
	}

	return(gGameOver);
#endif
}


/***************** PLAYER UNEXPECTEDLY LEAVES GAME ***********************/
//
// Called when HandleOtherNetMessage() gets a kNSpPlayerLeft message from one of the other players.
//
// INPUT: playerID = ID# of player that sent message
//

static void PlayerUnexpectedlyLeavesGame(NSpPlayerLeftMessage *mess)
{
	IMPLEMENT_ME_SOFT();
#if 0
int			i;
NSpPlayerID	playerID = mess->playerID;

		/* FIND PLAYER NUM THAT MATCHES THE ID */

	for (i = 0; i < gNumTotalPlayers; i++)
	{
		if (!gPlayerInfo[i].isComputer)							// skip computer players
		{
			if (gPlayerInfo[i].nspPlayerID == playerID)			// see if ID matches
				 goto matched_id;
		}
	}
	DoFatalAlert("PlayerUnexpectedlyLeavesGame: cannot find matching player id#");


matched_id:
	gPlayerInfo[i].isComputer = true;							// turn it into a computer player.
	gPlayerInfo[i].isEliminated = true;							// also eliminate from battles
	gNumGatheredPlayers--;										// one less net player in the game
	gNumRealPlayers--;

	if (gNumRealPlayers <= 1)									// see if nobody to play with
		gGameOver = true;

			/* HANDLE SPECIFICS */

	switch(gGameMode)
	{
		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
				if (gPlayerInfo[i].isIt)
					ChooseTaggedPlayer();
				break;
	}
#endif
}


#pragma mark -

/********************* PLAYER BROADCAST NULL PACKET *******************************/
//
// Send a dummy packet to all other players to let them know we're still active, but we're
// probably paused for some reason.  The recipients will then just wait and not time out.
//

void PlayerBroadcastNullPacket(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
OSStatus					status;
NSpMessageHeader			outMess;


				/* BUILD MESSAGE */

	NSpClearMessageHeader(&outMess);

	outMess.to 			= kNSpAllPlayers;						// send to all clients
	outMess.what 		= kNetNullPacket;						// set message type
	outMess.messageLen 	= sizeof(outMess);						// set size of message


			/* SEND IT */

	status = NSpMessage_Send(gNetGame, &outMess, kNSpSendFlag_Registered);
	if (status)
		DoFatalAlert("PlayerBroadcastNullPacket: NSpMessage_Send failed!");
#endif
}

