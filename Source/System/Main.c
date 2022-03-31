/****************************/
/*    NEW GAME - MAIN 		*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <SDL.h>
#include "globals.h"
#include "mobjtypes.h"
#include "objects.h"
#include "window.h"
#include "main.h"
#include "misc.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "camera.h"
#include "sound2.h"
#include "bg3d.h"
#include "file.h"
#include 	"input.h"
#include "player.h"
#include "3dmath.h"
#include "items.h"
#include "effects.h"
#include "miscscreens.h"
#include "ogl_support.h"
#include "terrain.h"
#include "fences.h"
#include "splineitems.h"
#include "network.h"
#include "selectvehicle.h"
#include "mainmenu.h"
#include "checkpoints.h"
#include "infobar.h"
#include "sprites.h"
#include "liquids.h"
#include "selecttrack.h"
#include "sobjtypes.h"
#include "localization.h"
#include "atlas.h"

extern	Boolean			gSongPlayingFlag,gDrawLensFlare,gIsNetworkHost,gIsNetworkClient,gNetGameInProgress,gDisableHiccupTimer,gHideInfobar;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gAutoFadeStartDist,gAutoFadeEndDist,gAutoFadeRange_Frac,gStartingLightTimer;
extern	float			gAnalogSteeringTimer[];
extern	WindowPtr	gCoverWindow;
extern	OGLPoint3D	gCoord;
extern	unsigned long 		gScore;
extern	ObjNode				*gFirstNodePtr, *gFinalPlaceObj;
extern	short		gNumSuperTilesDrawn,gNumLocalPlayers,gMyNetworkPlayerNum,gNumRealPlayers,gCurrentPlayerNum, gNumTotalPlayers;
extern	float		gGlobalTransparency;
extern	signed char	gNumEnemyOfKind[];
extern	int			gMaxItemsAllocatedInAPass,gNumObjectNodes;
extern	PlayerInfoType	gPlayerInfo[];
extern	PrefsType	gGamePrefs;
extern	Boolean		gAutoPilot;
extern	Byte		gActiveSplitScreenMode;
//extern	const uint16_t	gUserKeySettings_Defaults[];
extern  FSSpec		gDataSpec;

extern const KeyBinding kDefaultKeyBindings[NUM_CONTROL_NEEDS];
extern const KeyBinding kDefaultKeyBindings_P2[NUM_CONTROL_NEEDS];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitArea(void);
static void CleanupLevel(void);
static void PlayArea(void);
static Boolean PlayGame(void);

static void PlayGame_MultiplayerRace(void);
static Boolean PlayGame_Practice(void);
static Boolean PlayGame_Tournament(void);
static void PlayGame_Tag(void);
static void PlayGame_Survival(void);
static void PlayGame_CaptureTheFlag(void);
static void UpdateGameModeSpecifics(void);

static void TallyTokens(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_RETRIES		3

/****************************/
/*    VARIABLES             */
/****************************/

Byte				gDebugMode = 0;				// 0 == none, 1 = fps, 2 = all

uint32_t				gAutoFadeStatusBits;
short				gMainAppRezFile;

OGLSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType			gGamePrefs;



OGLVector3D			gWorldSunDirection = { 1, -.2, 1};		// also serves as lense flare vector


OGLColorRGBA		gFillColor1 = { 1.0, 1.0, 1.0, 1.0 };
OGLColorRGBA		gFillColor2 = { .5, .5, .5, 1.0 };

uint32_t				gGameFrameNum = 0;

Boolean				gNoCarControls = false;						// set if player has no control (for starting light et al)
Boolean				gGameOver = false;
Boolean				gTrackCompleted = false;
float				gTrackCompletedCoolDownTimer = 0;

int					gGameMode,gTheAge,gTournamentStage,gTrackNum;

static const int 	gTournamentTrackTable[NUM_AGES][TRACKS_PER_AGE] =
{
	[STONE_AGE]  = {TRACK_NUM_DESERT,	TRACK_NUM_JUNGLE,		TRACK_NUM_ICE},
	[BRONZE_AGE] = {TRACK_NUM_CRETE,	TRACK_NUM_CHINA,		TRACK_NUM_EGYPT},
	[IRON_AGE]   = {TRACK_NUM_EUROPE,	TRACK_NUM_SCANDINAVIA,	TRACK_NUM_ATLANTIS},
};


			/* BATTLE MODE VARS */

short				gWhoIsIt, gWhoWasIt;
float				gReTagTimer;							// timer to keep players from spaztic tagging each other

short				gCapturedFlagCount[2];					// # flags captured for both teams
short				gNumPlayersEliminated;

short				gTotalTokens;
short				gNumRetriesRemaining;


Boolean				gIsSelfRunningDemo = false;
float				gSelfRunningDemoTimer;


//======================================================================================
//======================================================================================
//======================================================================================


/****************** TOOLBOX INIT  *****************/

void ToolBoxInit(void)
{
OSErr		iErr;

	gMainAppRezFile = CurResFile();

		/* FIRST VERIFY SYSTEM BEFORE GOING TOO FAR */

	VerifySystem();


			/* BOOT OGL */

	OGL_Boot();


			/* MAKE FSSPEC FOR DATA FOLDER */

	SetDefaultDirectory();							// be sure to get the default directory

	iErr = FSMakeFSSpec(0, 0, ":Data:Images", &gDataSpec);
	if (iErr)
	{
		DoFatalAlert("Cannot find Data folder.");
		CleanQuit();
	}

			/********************/
			/* INIT PREFERENCES */
			/********************/

	InitDefaultPrefs();
	LoadPrefs(&gGamePrefs);							// attempt to read from prefs file

	LoadLocalizedStrings(gGamePrefs.language);

			/* DO BOOT CHECK FOR SCREEN MODE */

	SetFullscreenMode(true);

	SetDefaultPhysics();								// set all physics to defaults
}


/********** INIT DEFAULT PREFS ******************/

void InitDefaultPrefs(void)
{
	memset(&gGamePrefs, 0, sizeof(gGamePrefs));

	SetDefaultPlayerSaveData();

	gGamePrefs.language				= GetBestLanguageIDFromSystemLocale();
	gGamePrefs.difficulty			= DIFFICULTY_MEDIUM;
	gGamePrefs.desiredSplitScreenMode	= SPLITSCREEN_MODE_VERT;
	gGamePrefs.monitorNum			= 0;			// main monitor by default
	gGamePrefs.tagDuration 			= 3;

//	for (i = 0; i < NUM_CONTROL_NEEDS; i++)			// set OS X keyboard defaults
//		gGamePrefs.keySettings[i][0] = gUserKeySettings_Defaults[i];
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		gGamePrefs.keys[i][0] = kDefaultKeyBindings[i];
		gGamePrefs.keys[i][1] = kDefaultKeyBindings_P2[i];
	}

	
}





#pragma mark -

/******************** PLAY GAME ************************/
//
// Return true if user bailed before game started
//
// NOTE:  	at this point the network connections have already been established between Host and Clients for
//			network games.
//

static Boolean PlayGame(void)
{
	if (gNetGameInProgress)
		SetDefaultPhysics();								// set all physics to defaults for net game


	switch(gGameMode)
	{
		case	GAME_MODE_PRACTICE:
				if (PlayGame_Practice())
					return(true);
				break;

		case	GAME_MODE_TOURNAMENT:
				if (PlayGame_Tournament())
					return(true);
				break;

		case	GAME_MODE_MULTIPLAYERRACE:
				PlayGame_MultiplayerRace();
				break;

		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
				PlayGame_Tag();
				break;

		case	GAME_MODE_SURVIVAL:
				PlayGame_Survival();
				break;

		case	GAME_MODE_CAPTUREFLAG:
				PlayGame_CaptureTheFlag();
				break;

		default:
				DoFatalAlert("PlayGame: unknown game mode");

	}


		/* UPDATE ANY SAVED PLAYER FILE THAT'S ACTIVE */

	if (!gIsSelfRunningDemo)
		SavePlayerFile();

	return(false);
}


/******************** PLAY GAME: PRACTICE ************************/
//
// return true if user aborted before game started
//

static Boolean PlayGame_Practice(void)
{
	gMyNetworkPlayerNum = 0;								// no networking in practice mode



			/***********************/
			/* GAME INITIALIZATION */
			/***********************/

			/* SELECT THE PRACTICE TRACK */

select_track:

	if (gIsSelfRunningDemo)									// auto-pick if SRD
	{
		switch (GetNumAgesCompleted())
		{
			case	0:
					gTrackNum = RandomRange(0,2);			// randomly pick tracks 0..2
					break;

			case	1:
					gTrackNum = RandomRange(0,5);			// randomly pick tracks 0..5
					break;

			default:
					gTrackNum = RandomRange(0,7);			// randomly pick tracks 0..7 (no atlantis!)
					break;
		}
	}
	else
	{
		if (SelectSingleTrack())
			return(true);
	}


			/* SET PLAYER INFO */

	InitPlayerInfo_Game();									// init player info for entire game


			/* LET PLAYER(S) SELECT CHARACTERS & VECHICLES */
			//
			// Must do this after InitPlayer above!
			//

select_sex:
	if (gIsSelfRunningDemo)									// auto-pick if SRD
	{
		gPlayerInfo[0].sex = MyRandomLong() & 1;

		switch (GetNumAgesCompleted())
		{
			case	0:
					gPlayerInfo[0].vehicleType = RandomRange(0, 5);
					break;

			case	1:
					gPlayerInfo[0].vehicleType = RandomRange(0, 7);
					break;

			default:
					gPlayerInfo[0].vehicleType = RandomRange(0, 9);
					break;
		}
		gAutoPilot = true;									// set auto-pilot so computer will play for us in SRD mode
		gSelfRunningDemoTimer = 50.0f;						// set duration of SRD
	}
	else
	{
		if (DoCharacterSelectScreen(0, true))					// determine sex
			goto select_track;
		if (DoVehicleSelectScreen(0, true))						// get vehicle for only player, player 0
			goto select_sex;
	}



        /* LOAD ALL OF THE ART & STUFF */

	if (!gNetGameInProgress)									// dont show intro if Net since we're already showing it
		ShowLoadingPicture();									// show track intro screen

	InitArea();


		/***********/
        /* PLAY IT */
        /***********/

	PlayArea();

		/* CLEANUP LEVEL */

	FadeOutArea();
	CleanupLevel();

	gAutoPilot = false;										// make sure we turn this off!!
	gIsSelfRunningDemo = false;								// and disble SRD

	return(false);
}


/******************** PLAY GAME: TOURNAMENT ************************/
//
// Game loop for 1-player tournament mode.
//
//
// return true if user aborted before game started
//

static Boolean PlayGame_Tournament(void)
{
Boolean	canAbort = true;									// player can abort only at beginning
short	placeToWin,startStage;

	if (gGamePrefs.difficulty <= DIFFICULTY_EASY)			// in easy mode, 3rd place will do
		placeToWin = 2;
	else
		placeToWin = 0;


			/* SET PLAYER INFO */

	gMyNetworkPlayerNum = 0;								// this tournament mode is 1-player only
	InitPlayerInfo_Game();									// init player info for entire game


	if (DoCharacterSelectScreen(0, true))					// determine sex
		return(true);

			/*************************************************************************/
			/* GO THRU EACH AGE, STARTING WITH THE ONE THE USER SELECTED IN THE MENU */
			/*************************************************************************/

	for (;gTheAge <= IRON_AGE; gTheAge++)
	{
		gNumRetriesRemaining = MAX_RETRIES;								// # retries for this age

				/* SHOW THE AGE PICTURE WHILE LOADING LEVEL */

		ShowAgePicture(gTheAge);


				/* LET PLAYER SELECT CHARACTER AT START OF EACH AGE */

		if (DoVehicleSelectScreen(0, canAbort))								// select vehicle for player #0
			return(true);



					/********************************/
					/* PLAY EACH  LEVEL OF THIS AGE */
					/********************************/

		if (GetNumAgesCompleted() >= NUM_AGES)					// if won game, then start stage @ 0
			startStage = 0;
		else if (gGamePrefs.difficulty >= DIFFICULTY_HARD)		// always start @ stage 0 in hard modes
			startStage = 0;
		else if (gTheAge == GetNumAgesCompleted())				// if it's the age we're working on completing, start where we last were
			startStage = GetNumStagesCompleted();
		else													// picked an age we've already completed, start @ stage 0
			startStage = 0;

		for (gTournamentStage = startStage; gTournamentStage < 3; gTournamentStage++)
		{
			gTrackNum = gTournamentTrackTable[gTheAge][gTournamentStage];	// get global track # to play

			ShowLoadingPicture();									// show track intro screen

			InitArea();
			PlayArea();

			if (gPlayerInfo[0].place <= placeToWin)									// if came in 1st place then tally tokens
				TallyTokens();


					/* SEE IF DIDNT COME IN 1ST PLACE OR DIDNT GET ALL THE TOKENS */

			if ((gPlayerInfo[0].place > placeToWin) || (gTotalTokens < MAX_TOKENS))
			{
				if (gNumRetriesRemaining > 0)										// see if there are any retries remaining
				{
					const char* s = Localize(STR_1_TRY_REMAINING + gNumRetriesRemaining - 1);

					if (DoFailedMenu(s))												// returns true if want to retry
					{
						gTournamentStage--;											// back up, so "next" will put us back here again
						gNumRetriesRemaining--;
					}
					else															// otherwise, user wants to retire
						gGameOver = true;
				}
				else
					gGameOver = true;												// no retries, so bail
			}


				/* IF JUST COMPLETED SOMETHING NEW THEN INC THE STAGE COUNTER */

			if (!gGameOver															// dont do anything if we failed or bailed
				&& gTheAge == GetNumAgesCompleted()									// only if this is the age we're working on winning
				&& gTournamentStage < 2												// completing third stage bumps the age
				&& gTournamentStage > GetNumStagesCompleted()						// only if it's better than current progress
			   )
			{
				SetPlayerProgression(gTheAge, gTournamentStage + 1);
				SavePlayerFile();
			}

				/* CLEANUP TRACK */

			FadeOutArea();
			CleanupLevel();

			if (gGameOver)													// see if its over
				return(false);
		}

			/* NEXT AGE */

		if (gTheAge >= GetNumAgesCompleted())								// inc player's age completion value
		{
			SetPlayerProgression(gTheAge + 1, 0);
			SavePlayerFile();
		}


		DoAgeConqueredScreen();


		canAbort = false;
	}																		// next age


			/* WE WON!! */

	DoTournamentWinScreen();


	return(false);
}

/******************** PLAY GAME: MULTIPLAYER RACE ************************/
//
// Multiplayer racing can be a 2-player splitscreen or network.
//
// gTrackNum has already been set by the Host
//

static void PlayGame_MultiplayerRace(void)
{
			/* SET PLAYER INFO */

	InitPlayerInfo_Game();											// init player info for entire game


			/***********************/
			/* GAME INITIALIZATION */
			/***********************/

			/* LET PLAYER(S) SELECT CHARACTERS */
			//
			// Must do this after InitPlayerInfo_Game above!
			//

	DoMultiPlayerVehicleSelections();


			/* LOAD TRACK */

	ShowLoadingPicture();									// show track intro screen
	InitArea();

			/* PLAY TRACK */

	PlayArea();


			/* CLEANUP LEVEL */

	FadeOutArea();
	CleanupLevel();
}


/******************** PLAY GAME: TAG ************************/
//
// Multiplayer tag can be a 2-player splitscreen or network.
//

static void PlayGame_Tag(void)
{

			/* SET PLAYER INFO */

	InitPlayerInfo_Game();											// init player info for entire game


			/***********************/
			/* GAME INITIALIZATION */
			/***********************/

			/* LET PLAYER(S) SELECT CHARACTERS */
			//
			// Must do this after InitPlayerInfo_Game above!
			//

	DoMultiPlayerVehicleSelections();


			/* INIT TRACK */

	ShowLoadingPicture();
	InitArea();


		/* PICK RANDOM PLAYER TO BE "IT" */

	gNumPlayersEliminated = 0;
	ChooseTaggedPlayer();											// MUST do this AFTER InitArea() since that's where the random seed gets reset
	gWhoWasIt = gWhoIsIt;

		/***********/
        /* PLAY IT */
        /***********/

	PlayArea();


		/* CLEANUP LEVEL */

	FadeOutArea();
	CleanupLevel();
}


/******************** PLAY GAME: SURVIVAL ************************/

static void PlayGame_Survival(void)
{

			/* SET PLAYER INFO */

	InitPlayerInfo_Game();											// init player info for entire game


			/***********************/
			/* GAME INITIALIZATION */
			/***********************/

			/* LET PLAYER(S) SELECT CHARACTERS */
			//
			// Must do this after InitPlayerInfo_Game above!
			//

	DoMultiPlayerVehicleSelections();


			/* INIT TRACK */

	ShowLoadingPicture();									// show intro screen
	InitArea();
	gNumPlayersEliminated = 0;


		/***********/
        /* PLAY IT */
        /***********/

	PlayArea();


		/* CLEANUP LEVEL */

	FadeOutArea();
	CleanupLevel();
}


/******************** PLAY GAME: CAPTURE THE FLAG ************************/

static void PlayGame_CaptureTheFlag(void)
{

			/* SET PLAYER INFO */

	InitPlayerInfo_Game();											// init player info for entire game

	gCapturedFlagCount[0] = 0;									// no flags captured yet
	gCapturedFlagCount[1] = 0;

			/***********************/
			/* GAME INITIALIZATION */
			/***********************/

			/* LET PLAYER(S) SELECT CHARACTERS */
			//
			// Must do this after InitPlayerInfo_Game above!
			//

	DoMultiPlayerVehicleSelections();


			/* INIT TRACK */

	ShowLoadingPicture();									// show intro screen
	InitArea();


		/***********/
        /* PLAY IT */
        /***********/

	PlayArea();


		/* CLEANUP LEVEL */

	FadeOutArea();
	CleanupLevel();
}

#pragma mark -


/********************** TALLY TOKENS **************************/
//
// Called @ end of tournament game to do token tally effect
//

static void TallyTokens(void)
{
Byte	stage = 0;
float	stageTimer;
ObjNode	*newObj;
char	s[32];
Boolean	done = false;

	if (gFinalPlaceObj)
	{
		DeleteObject(gFinalPlaceObj);
		gFinalPlaceObj = nil;
	}

	stageTimer = 0;

	while(!done)
	{
			/************************/
			/* SEE IF DO NEXT STAGE */
			/************************/

		stageTimer -= gFramesPerSecondFrac;
		if (stageTimer <= 0.0f)
		{

			switch(stage++)
			{
					/* SHOW BIG ARROWHEAD ICON */

				case	0:
				{
						stageTimer = 1.5;										// reset stage timer

						NewObjectDefinitionType def =
						{
							.group		= SPRITE_GROUP_INFOBAR,
							.type		= INFOBAR_SObjType_Token_Arrowhead,
							.coord		= {-.2, 0, 0},
							.slot 		= SPRITE_SLOT,
							.scale		= 1.5,
						};
						newObj = MakeSpriteObject(&def, gGameViewInfoPtr);
						break;
				}


					/* START COUNTER */

				case	1:
				{
						stageTimer = 1.5;										// reset stage timer

								/* MAKE X */

						NewObjectDefinitionType def =
						{
							.group		= SPRITE_GROUP_INFOBAR,
							.type		= INFOBAR_SObjType_WeaponX,
							.coord		= {0, 0, 0},
							.slot		= SPRITE_SLOT,
							.scale		= 1.5,
						};
						newObj = MakeSpriteObject(&def, gGameViewInfoPtr);
						break;
				}

					/* SHOW COUNTER */

				case	2:
				{
						stageTimer = 4.0;										// reset stage timer

								/* MAKE NUMBER */

						NewObjectDefinitionType def =
						{
							.coord 		= {.2, 0, 0},
							.scale		= 1.5,
							.slot 		= SPRITE_SLOT,
						};

						snprintf(s, sizeof(s), "%d", gTotalTokens);
						TextMesh_New(s, kTextMeshAlignCenter, &def);

						if (gTotalTokens == MAX_TOKENS)
							PlayAnnouncerSound(EFFECT_COMPLETED, true, .3);
						else
							PlayAnnouncerSound(EFFECT_INCOMPLETE, true, .3);

						break;
				}

				case	3:
						done = true;
						break;

			}
		}


			/* MOVE STUFF */

		MoveEverything();

			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		DoPlayerTerrainUpdate();							// need to call this to keep supertiles active
		OGL_DrawScene(gGameViewInfoPtr, DrawTerrain);


	}


}


#pragma mark -

/**************** PLAY AREA ************************/

static void PlayArea(void)
{
	/* IF DOING NET GAME THEN WAIT FOR SYNC */
	//
	// Need to wait for other players to finish loading art and when
	// everyone is ready, then we can start all at once.
	//


	if (gIsNetworkHost)
		HostWaitForPlayersToPrepareLevel();
	else
	if (gIsNetworkClient)
		ClientTellHostLevelIsPrepared();


			/* PREP STUFF */

	ReadKeyboard();
	CalcFramesPerSecond();
	CalcFramesPerSecond();
	gNoCarControls = true;									// no control when starting light is going
	gDisableHiccupTimer = true;

		/******************/
		/* MAIN GAME LOOP */
		/******************/

	MakeFadeEvent(true);

	while(true)
	{
				/******************************************/
				/* GET CONTROL INFORMATION FOR THIS FRAME */
				/******************************************/
				//
				// Also gathers frame rate info for the net clients.
				//

				/* NETWORK CLIENT */

		if (gIsNetworkClient)
		{
			ClientReceive_ControlInfoFromHost();			// read all player's control info back from the Host once he's gathered it all
		}

				/* HOST OR NON-NET */
		else
		{
			ReadKeyboard();									// read local keys
			GetLocalKeyState();								// build a control state bitfield

				/* NETWORK HOST*/

			if (gIsNetworkHost)
			{
				HostSend_ControlInfoToClients();			// now send everyone's key states to all clients
			}
		}


				/****************/
				/* MOVE OBJECTS */
				/****************/

		MoveEverything();

				/* DO GAME MODE SPECIFICS */

		UpdateGameModeSpecifics();


			/* UPDATE THE TERRAIN */

		DoPlayerTerrainUpdate();




			/****************************/
			/* SEND NET CLIENT KEY INFO */
			/****************************/
			//
			// We can do this anytime AFTER this frame's key control info is no longer needed.
			// Since this will change the control bits, we MUST BE SURE that the bits are not
			// used again until the next frame!
			//
			// For best performance, we do this before the render function.  That way there is
			// time for this send to get to the host while we're still waiting for the render to
			// complete - we essentially get this send for free!
			//

		if (gIsNetworkClient)
		{
			ReadKeyboard();									// read local client keys
			GetLocalKeyState();								// build a control state bitfield
			ClientSend_ControlInfoToHost();					// send this info to the host to be used the next frame
		}



			/***************/
			/* DRAW IT ALL */
			/***************/

		OGL_DrawScene(gGameViewInfoPtr,DrawTerrain);




			/************************************/
			/* NET HOST RECEIVE CLIENT KEY INFO */
			/************************************/
			//
			// Odds are that the clients have all sent their control into to the Host by now,
			// so the Host can read all of the client key info for use on the next frame.
			//

		if (gIsNetworkHost)
		{
			HostReceive_ControlInfoFromClients();		// get client info
		}


			/**************/
			/* MISC STUFF */
			/**************/

			/* CHECK CHEATS */

		if (GetKeyState(SDL_SCANCODE_B))
		{
			if (GetKeyState(SDL_SCANCODE_R) && GetKeyState(SDL_SCANCODE_I))		// win race cheat
			{
				if (!gPlayerInfo[0].raceComplete)
				{
					PlayerCompletedRace(0);
					gPlayerInfo[0].place = 0;
					gPlayerInfo[0].raceComplete = true;
					gPlayerInfo[0].numTokens = gTotalTokens = MAX_TOKENS;
				}
			}

			if (GetKeyState(SDL_SCANCODE_F13))		// hide/show infobar
				gHideInfobar = !gHideInfobar;


		}

			/* SEE IF PAUSED */

		if (!gIsSelfRunningDemo)
		{
			if (GetNewNeedStateAnyP(kNeed_UIPause))
				DoPaused();
		}

		if (!gIsNetworkClient)						// clients dont need to calc frame rate since its passed to them from host.
			CalcFramesPerSecond();

		gGameFrameNum++;


				/* SEE IF TRACK IS COMPLETED */

		if (gGameOver)													// if we need immediate abort, then bail now
			break;

		if (gTrackCompleted)
		{
			gTrackCompletedCoolDownTimer -= gFramesPerSecondFrac;		// game is done, but wait for cool-down timer before bailing
			if (gTrackCompletedCoolDownTimer <= 0.0f)
				break;
		}

		gDisableHiccupTimer = false;									// reenable this after the 1st frame

			/* UPDATE SELF-RUNNING DEMO */

		if (gIsSelfRunningDemo)
		{
			if (AreAnyNewKeysPressed())									// stop SRD if any key is pressed
				break;

			gSelfRunningDemoTimer -= gFramesPerSecondFrac;
			if (gSelfRunningDemoTimer <= 0.0f)							// or stop of timer runs out
			{
				break;
			}
		}
	}
}


void FadeOutArea(void)
{
			/* FADE OUT */

	OGL_FadeOutScene(gGameViewInfoPtr, DrawTerrain, DoPlayerTerrainUpdate);		// need to keep supertiles alive
}

/******************** MOVE EVERYTHING ************************/

void MoveEverything(void)
{

	MoveObjects();
	MoveSplineObjects();
	MoveParticleGroups();
	UpdateCameras(false);							// update cameras for ALL players
	CalcPlayerPlaces();							// determinw who is in what place
	UpdateSkidMarks();							// update skid marks
	UpdateLiquidAnimation();					// update liquid animation

			/* DO TRACK SPECIFICS */

	switch(gTrackNum)
	{
		case	TRACK_NUM_ICE:
		case	TRACK_NUM_RAMPS:
				MakeSnow();
				break;

	}
}


/***************** INIT AREA ************************/

static void InitArea(void)
{
OGLSetupInputType	viewDef;
short				numPanes;


	switch(gTrackNum)
	{
		case	TRACK_NUM_DESERT:
				PlaySong(SONG_DESERT, true);
				break;

		case	TRACK_NUM_JUNGLE:
		case	TRACK_NUM_AZTEC:
				PlaySong(SONG_JUNGLE, true);
				break;

		case	TRACK_NUM_ATLANTIS:
		case	TRACK_NUM_TARPITS:
				PlaySong(SONG_ATLANTIS, true);
				break;

		case	TRACK_NUM_CHINA:
		case	TRACK_NUM_SPIRAL:
				PlaySong(SONG_CHINA, true);
				break;

		case	TRACK_NUM_EGYPT:
				PlaySong(SONG_EGYPT, true);
				break;

		case	TRACK_NUM_CRETE:
		case	TRACK_NUM_CELTIC:
		case	TRACK_NUM_COLISEUM:
				PlaySong(SONG_CRETE, true);
				break;

		case	TRACK_NUM_ICE:
		case	TRACK_NUM_RAMPS:
				PlaySong(SONG_ICE, true);
				break;

		case	TRACK_NUM_EUROPE:
		case	TRACK_NUM_STONEHENGE:
				PlaySong(SONG_EUROPE, true);
				break;

		case	TRACK_NUM_SCANDINAVIA:
				PlaySong(SONG_VIKING, true);
				break;

		default:
				PlaySong(SONG_DESERT, true);
	}



			/* INIT SOME PRELIM STUFF */

	if ((!gIsSelfRunningDemo) && (gGameMode != GAME_MODE_PRACTICE))				// dont reset random seed for SRD - we want variety!
		InitMyRandomSeed();
	InitControlBits();

	gGameFrameNum 		= 0;
	gGameOver 			= false;
	gTrackCompleted 	= false;
	gTotalTokens 		= 0;
	gReTagTimer 		= 0;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		gPlayerInfo[i].objNode = nil;
		gAnalogSteeringTimer[i] = 0;
	}


			/*************/
			/* MAKE VIEW */
			/*************/

	if (gNetGameInProgress)
		numPanes = 1;
	else
		numPanes = gNumRealPlayers;


			/* SETUP VIEW DEF */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 50;
	viewDef.camera.yon 				= (SUPERTILE_ACTIVE_RANGE * SUPERTILE_SIZE * TERRAIN_POLYGON_SIZE);
	viewDef.camera.fov 				= GAME_FOV;

	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.view.clearBackBuffer	= true;
	viewDef.view.numPanes			= numPanes;
	viewDef.view.pillarbox4x3		= false;
	viewDef.view.fontName			= "rockfont";

	viewDef.styles.useFog			= false;
	viewDef.styles.fogStart			= viewDef.camera.yon * .1f;
	viewDef.styles.fogEnd			= viewDef.camera.yon;


			/* SET LIGHTS */

	switch(gTrackNum)
	{
		case	TRACK_NUM_ICE:
				viewDef.lights.numFillLights 	= 1;

				viewDef.lights.ambientColor.r 		= .7;
				viewDef.lights.ambientColor.g 		= .7;
				viewDef.lights.ambientColor.b 		= .7;
				viewDef.lights.fillDirection[0].x 	= 1.0;
				viewDef.lights.fillDirection[0].y 	= -.1;
				viewDef.lights.fillDirection[0].z 	= 1.0;
				viewDef.lights.fillColor[0].r 		= 1.0;
				viewDef.lights.fillColor[0].g 		= 1.0;
				viewDef.lights.fillColor[0].b 		= 1.0;
				break;

		case	TRACK_NUM_ATLANTIS:
				viewDef.lights.numFillLights 		= 1;

				viewDef.lights.ambientColor.r 		= .5;
				viewDef.lights.ambientColor.g 		= .5;
				viewDef.lights.ambientColor.b 		= .7;
				viewDef.lights.fillDirection[0].x 	= 0;
				viewDef.lights.fillDirection[0].y 	= -1.0;
				viewDef.lights.fillDirection[0].z 	= 0;
				viewDef.lights.fillColor[0].r 		= .9;
				viewDef.lights.fillColor[0].g 		= .9;
				viewDef.lights.fillColor[0].b 		= 1.0;
				break;

		default:
				viewDef.lights.numFillLights 		= 1;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.ambientColor.r 		= .6;
				viewDef.lights.ambientColor.g 		= .6;
				viewDef.lights.ambientColor.b 		= .6;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				viewDef.lights.fillDirection[1].x 	= -gWorldSunDirection.x;
				viewDef.lights.fillDirection[1].y 	= gWorldSunDirection.y;
				viewDef.lights.fillDirection[1].z 	= -gWorldSunDirection.z;
				viewDef.lights.fillColor[1]			= gFillColor2;
	}

			/* SET CLEAR COLOR */
			//
			// Our crappy sky dome is a cylinder with a big hole, so pick a color to kinda blend it in
			//

	switch(gTrackNum)
	{

		case		TRACK_NUM_DESERT:
					viewDef.view.clearColor.r = 153.0/255.0;
					viewDef.view.clearColor.g = 171.0/255.0;
					viewDef.view.clearColor.b = 237.0/255.0;
					break;

		case		TRACK_NUM_JUNGLE:
					viewDef.view.clearColor.r = 82.0/255.0;
					viewDef.view.clearColor.g = 148.0/255.0;
					viewDef.view.clearColor.b = 198.0/255.0;
					break;

		case		TRACK_NUM_ICE:
		case		TRACK_NUM_RAMPS:
					viewDef.view.clearColor.r = 115.0/255.0;
					viewDef.view.clearColor.g = 198.0/255.0;
					viewDef.view.clearColor.b = 255.0/255.0;
					break;

		case		TRACK_NUM_SPIRAL:
		case		TRACK_NUM_CRETE:
		case		TRACK_NUM_CELTIC:
		case		TRACK_NUM_MAZE:
					viewDef.view.clearColor.r = 44.0/255.0;
					viewDef.view.clearColor.g = 73.0/255.0;
					viewDef.view.clearColor.b = 195.0/255.0;
					break;

		case		TRACK_NUM_CHINA:
					viewDef.view.clearColor.r = 179.0/255.0;
					viewDef.view.clearColor.g = 153.0/255.0;
					viewDef.view.clearColor.b = 91.0/255.0;
					break;

		case		TRACK_NUM_EGYPT:
		case		TRACK_NUM_TARPITS:
					viewDef.view.clearColor.r = 222.0/255.0;
					viewDef.view.clearColor.g = 181.0/255.0;
					viewDef.view.clearColor.b = 99.0/255.0;
					break;

		case		TRACK_NUM_EUROPE:
					viewDef.view.clearColor.r = 16.0/255.0;
					viewDef.view.clearColor.g = 16.0/255.0;
					viewDef.view.clearColor.b = 74.0/255.0;
					break;

		case		TRACK_NUM_SCANDINAVIA:
		case		TRACK_NUM_STONEHENGE:
					viewDef.view.clearColor.r = 74.0/255.0;
					viewDef.view.clearColor.g = 90.0/255.0;
					viewDef.view.clearColor.b = 148.0/255.0;
					break;

		case		TRACK_NUM_ATLANTIS:
					viewDef.view.clearColor.r = 5.0/255.0;
					viewDef.view.clearColor.g = 160.0/255.0;
					viewDef.view.clearColor.b = 190.0/255.0;
					break;


		case		TRACK_NUM_AZTEC:
					viewDef.view.clearColor.r = 82.0/255.0;
					viewDef.view.clearColor.g = 148.0/255.0;
					viewDef.view.clearColor.b = 198.0/255.0;
					break;

		case		TRACK_NUM_COLISEUM:
					viewDef.view.clearColor.r = 61.0/255.0;
					viewDef.view.clearColor.g = 87.0/255.0;
					
					break;

	}


	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


			/* SET AUTO-FADE INFO */

	gAutoFadeStartDist	= gGameViewInfoPtr->yon * .80f;
	gAutoFadeEndDist	= gGameViewInfoPtr->yon * .92f;
	gAutoFadeRange_Frac	= 1.0 / (gAutoFadeEndDist - gAutoFadeStartDist);

	if (gAutoFadeStartDist != 0.0f)
		gAutoFadeStatusBits = STATUS_BIT_AUTOFADE;
	else
		gAutoFadeStatusBits = 0;





			/**********************/
			/* LOAD ART & TERRAIN */
			/**********************/
			//
			// NOTE: only call this *after* draw context is created!
			//

	LoadLevelArt(gGameViewInfoPtr);
	InitInfobar(gGameViewInfoPtr);


			/* INIT OTHER MANAGERS */

	InitEffects(gGameViewInfoPtr);
	InitItemsManager();
	InitSkidMarks();


		/* INIT THE PLAYERS & RELATED STUFF */

	InitPlayersAtStartOfLevel();
	InitCurrentScrollSettings();

	PrimeSplines();
	PrimeFences();

			/* PRINT TRACK NAME */

	MakeTrackName();

			/* INIT CAMERAS */

	InitCameras();
 }


/**************** CLEANUP LEVEL **********************/

static void CleanupLevel(void)
{
	EndNetworkGame();
	StopAllEffectChannels();
 	EmptySplineObjectList();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSuperTileMemoryList();
	DisposeTerrain();
	DeleteAllParticleGroups();
	DisposeInfobar();
	DisposeParticleSystem();
	DisposeAllSpriteGroups();
	DisposeFences();

	DisposeAllBG3DContainers();

	DisposeSoundBank(SOUND_BANK_LEVELSPECIFIC);
	DisposeSoundBank(SOUND_BANK_CAVEMAN);

	OGL_DisposeWindowSetup(&gGameViewInfoPtr);	// do this last!

	gNumRealPlayers = 1;					// reset at end of level to be safe
	gNumLocalPlayers = 1;
	gActiveSplitScreenMode = SPLITSCREEN_MODE_NONE;
}


/*************** UPDATE GAME MODE SPECIFICS **********************/

static void UpdateGameModeSpecifics(void)
{
short	i,t,winner;

	if (gTrackCompleted)										// if track is done then dont do anything
		return;

	switch(gGameMode)
	{
				/****************************/
				/* TAG - DONT WANT TO BE IT */
				/****************************/

		case	GAME_MODE_TAG1:
				gReTagTimer -= gFramesPerSecondFrac;						// dec this timer

				if (gStartingLightTimer <= 1.0f)
					gPlayerInfo[gWhoIsIt].tagTimer -= gFramesPerSecondFrac;		// dec the tagged player's timer

				if (gPlayerInfo[gWhoIsIt].tagTimer <= 0.0f)					// if done, then eliminate this player from the battle
				{
					gPlayerInfo[gWhoIsIt].tagTimer = 0;
					gPlayerInfo[gWhoIsIt].isEliminated = true;

					gNumPlayersEliminated++;
					if (gNumPlayersEliminated == (gNumTotalPlayers-1))		// if all players eliminated except 1 then we're done!
					{
							/* GAME IS DONE */

						gTrackCompleted = true;
						gTrackCompletedCoolDownTimer = TRACK_COMPLETE_COOLDOWN_TIME;

						for (winner = 0; winner < gNumTotalPlayers; winner++)
							if (!gPlayerInfo[winner].isEliminated)			// scan for the winner
								break;

						for (i = 0; i < gNumTotalPlayers; i++)				// see which player Won (was not eliminated)
						{
							if (gPlayerInfo[i].isEliminated)
							{
								ShowWinLose(i, 2, winner);					// lost
							}
							else
								ShowWinLose(i, 1, winner);					// won!
						}
					}

							/* ELIMINATE THIS PLAYER AND CHOOSE ANOTHER */
					else
					{
						ShowWinLose(gWhoIsIt, 0, 0);						// this player is eliminated
						ChooseTaggedPlayer();								// tagged guy is gone, so choose a new one
					}

				}
				UpdateTagMarker();
				break;


				/**************************/
				/* TAG - DO WANT TO BE IT */
				/**************************/

		case	GAME_MODE_TAG2:
				gReTagTimer -= gFramesPerSecondFrac;						// dec this timer

				if (gStartingLightTimer <= 1.0f)
					gPlayerInfo[gWhoIsIt].tagTimer -= gFramesPerSecondFrac;		// update the "it" player for tagged mode

				if (gPlayerInfo[gWhoIsIt].tagTimer <= 0.0f)					// if this player's timer is done then this player has won
				{
					gPlayerInfo[gWhoIsIt].tagTimer = 0;
					gTrackCompleted = true;
					gTrackCompletedCoolDownTimer = TRACK_COMPLETE_COOLDOWN_TIME;

					for (i = 0; i < gNumTotalPlayers; i++)					// see which player Won (was not eliminated)
					{
						if (i == gWhoIsIt)
							ShowWinLose(i, 1, gWhoIsIt);
						else
							ShowWinLose(i, 2, gWhoIsIt);
					}

				}
				UpdateTagMarker();
				break;


				/************/
				/* SURVIVAL */
				/************/

		case	GAME_MODE_SURVIVAL:
				for (i = 0; i < gNumTotalPlayers; i++)					// dec impact timers
				{
					gPlayerInfo[i].impactResetTimer -= gFramesPerSecondFrac;
				}

				if (gNumPlayersEliminated == (gNumTotalPlayers-1))		// if all players eliminated except 1 then we're done!
				{
						/* GAME IS DONE */

					gTrackCompleted = true;
					gTrackCompletedCoolDownTimer = TRACK_COMPLETE_COOLDOWN_TIME;

					for (winner = 0; winner < gNumTotalPlayers; winner++)
						if (!gPlayerInfo[winner].isEliminated)			// scan for the winner
							break;


								/* NOBODY WON */

					if (winner >= gNumTotalPlayers)
					{
						for (i = 0; i < gNumTotalPlayers; i++)			// all were eliminated
							ShowWinLose(i, 2, -1);						// lost
					}

							/* SHOW WINNER/LOSERS */
					else
					{
						for (i = 0; i < gNumTotalPlayers; i++)				// see which player Won (was not eliminated)
						{
							if (gPlayerInfo[i].isEliminated)
							{
								ShowWinLose(i, 2, winner);					// lost
							}
							else
								ShowWinLose(i, 1, winner);					// won!
						}
					}
				}
				break;


				/********************/
				/* CAPTURE THE FLAG */
				/********************/

		case	GAME_MODE_CAPTUREFLAG:
				for (t = 0; t < 2; t++)											// check both teams
				{
					if (gCapturedFlagCount[t] >= NUM_FLAGS_TO_GET) 				// see if team t got all of their flags
					{
						gTrackCompleted = true;
						gTrackCompletedCoolDownTimer = TRACK_COMPLETE_COOLDOWN_TIME;

						for (i = 0; i < gNumTotalPlayers; i++)					// see which players Won (was not eliminated)
						{
							if (gPlayerInfo[i].team == t)
								ShowWinLose(i, 1, 0);								// won!
							else
								ShowWinLose(i, 2, 0);								// lost
						}
						break;
					}
				}
				break;

	}
}


/************************************************************/
/******************** PROGRAM MAIN ENTRY  *******************/
/************************************************************/


void GameMain(void)
{
unsigned long	someLong;
Boolean			userAbortedBeforeGameStarted;


				/**************/
				/* BOOT STUFF */
				/**************/

	ToolBoxInit();



			/* INIT SOME OF MY STUFF */

	InitSpriteManager();
	InitBG3DManager();
    InitNetworkManager();
 	InitInput();
	InitWindowStuff();
	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();


			/* INIT MORE MY STUFF */

	InitObjectManager();

	GetDateTime ((unsigned long *)(&someLong));		// init random seed
	SetMyRandomSeed(someLong);
//	HideCursor();

	PlaySong(SONG_THEME, true);


		/* SHOW TITLE SCREEN */

	DoTitleScreen();


		/* MAIN LOOP */

	while(true)
	{
		PlaySong(SONG_THEME, true);
		DoMainMenuScreen();

		userAbortedBeforeGameStarted = PlayGame();
	}

}




