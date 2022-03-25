/****************************/
/*   	MAINMENU SCREEN.C	*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "window.h"
#include "input.h"
#include "sound2.h"
#include	"file.h"
#include	"ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "skeletonobj.h"
#include "mainmenu.h"
#include "sobjtypes.h"
#include "sprites.h"
#include "miscscreens.h"
#include "network.h"
#include "selecttrack.h"
#include "player.h"
#include "selectvehicle.h"
#include "localization.h"
#include "atlas.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow;
extern	FSSpec		gDataSpec;
extern	Boolean		gNetGameInProgress,gSavedPlayerIsLoaded,gIsSelfRunningDemo;
extern	KeyMap gKeyMap,gNewKeys;
extern	short		gNumRealPlayers,gNumLocalPlayers, gMyNetworkPlayerNum;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds,gIsNetworkHost,gIsNetworkClient;
extern	PrefsType	gGamePrefs;
extern	OGLPoint3D	gCoord;
extern	MOPictureObject 	*gBackgoundPicture;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int			gGameMode,gTheAge;
extern	Byte		gActiveSplitScreenMode;
extern	SavePlayerType	gPlayerSaveData;
extern	const short gNumTracksUnlocked[];
extern	int		gVehicleParameters[MAX_CAR_TYPES+1][NUM_VEHICLE_PARAMETERS];
extern	float	gSteeringResponsiveness,gCarMaxTightTurn,gCarTurningRadius,gTireTractionConstant,gTireFrictionConstant,gCarGravity,gSlopeRatioAdjuster;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupMainMenuScreen(void);
static void FreeMainMenuArt(void);
static void DrawMainMenuCallback(OGLSetupOutputType *info);

static void BuildMenu(short menuNum);
static Boolean NavigateMenu(void);

static void DoPhysicsEditor(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAINMENU_ICON_SCALE	.55f

#define	MAX_LINES		5

#define	LINE_SPACING	.14f

enum
{
	MENU_ID_TITLE,
	MENU_ID_PLAY,
	MENU_ID_OPTIONS,
	MENU_ID_MULTIPLAYERGAMETYPE,
	MENU_ID_1PLAYERGAMETYPE,
	MENU_ID_TOURNAMENT,
	MENU_ID_NETGAME,
	NUM_MENU_IDS
};


#define	DEMO_DELAY	20.0f			// # seconds of idle until self-running demo kicks in


/*********************/
/*    VARIABLES      */
/*********************/


static short	gMainMenuSelection,gWhichMenu;

static ObjNode	*gIcons[MAX_LINES];

//static short	gNumMenuItems[] = {4,3,4,5 ,2,3,2};

#define MAX_ITEMS_PER_MENU 8

static int gNumMenuItems = 0;

static const LocStrID gMenuItemStrings[NUM_MENU_IDS][MAX_ITEMS_PER_MENU] =
{
	[MENU_ID_TITLE]               = { STR_NEW_GAME, STR_LOAD_GAME, STR_OPTIONS, STR_QUIT, 0 },
	[MENU_ID_PLAY]                = { STR_1PLAYER, STR_2PLAYER, STR_NET_GAME, 0 },
	[MENU_ID_OPTIONS]             = { STR_SETTINGS, STR_HELP, STR_CREDITS, STR_PHYSICS_EDITOR, 0 },
	[MENU_ID_MULTIPLAYERGAMETYPE] = { STR_RACE, STR_KEEP_AWAY_TAG, STR_STAMPEDE_TAG, STR_SURVIVAL, STR_QUEST_FOR_FIRE, 0 },
	[MENU_ID_1PLAYERGAMETYPE]     = { STR_PRACTICE, STR_TOURNAMENT, 0 },
	[MENU_ID_TOURNAMENT]          = { STR_STONE_AGE, STR_BRONZE_AGE, STR_IRON_AGE, 0 },
	[MENU_ID_NETGAME]             = { STR_HOST_NET_GAME, STR_JOIN_NET_GAME, 0 }
};

static const OGLColorRGBA gMainMenuHiliteColor = {.3,.5,.2,1};
static const OGLColorRGBA gMainMenuNormalColor = {1,1,1,1};

static float	gTimeUntilDemo;



/********************** DO MAINMENU SCREEN **************************/

void DoMainMenuScreen(void)
{
	gTimeUntilDemo = DEMO_DELAY;

do_again:
	SetupMainMenuScreen();



				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (NavigateMenu())
			break;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawMainMenuCallback);


		/* CHECK IF SELF-RUNNING DEMO */

		if (gNumLocalPlayers < 2)
		{
			gTimeUntilDemo -= gFramesPerSecondFrac;
			if (gTimeUntilDemo <= 0.0f)
			{
				gIsSelfRunningDemo = true;
				break;
			}
		}
	}

			/* CLEANUP */

	FreeMainMenuArt();

			/* PRIME SELF-RUNNING DEMO */

	if (gIsSelfRunningDemo)
	{
		gNetGameInProgress = false;
		gGameMode = GAME_MODE_PRACTICE;
	}


		/******************************/
		/* HANDLE GAME MODE SPECIFICS */
		/******************************/

	switch(gGameMode)
	{
			/* BATTLE MODE:  SELECT TRACK NOW */

		case	GAME_MODE_MULTIPLAYERRACE:
		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
		case	GAME_MODE_SURVIVAL:
		case	GAME_MODE_CAPTUREFLAG:
				if (!gIsNetworkClient)				// clients dont select tracks since they get the info when they join
				{
					GammaFadeOut();
					if (SelectSingleTrack())
						goto do_again;
				}
				break;

	}



			/********************/
			/* DO NETWORK SETUP */
			/********************/
			//
			// Let's do this all now so that we can easily abort out and so we can transfer the
			// necessary information about the game from the Host to the other players.
			//

	if (gNetGameInProgress)
	{
		if (gIsNetworkHost)
		{
			if (SetupNetworkHosting())						// do hosting dialog and check for abort
				goto do_again;
		}
		else
		{
			if (SetupNetworkJoin())							// get joining info and check for abort
				goto do_again;
		}
	}
	else
		gMyNetworkPlayerNum = 0;

	GammaFadeOut();
}


/********************* SETUP MAINMENU SCREEN **********************/

static void SetupMainMenuScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .1, .1, .1, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLColorRGBA		fillColor2 = { .3, .3, .3, 1 };
OGLVector3D			fillDirection1 = { .9, -.7, -1 };
OGLVector3D			fillDirection2 = { -1, -.2, -.5 };


//	if (gOSX)							//------- remove network option from menu
//		gNumMenuItems[MENU_ID_PLAY] = 2;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= 1.0;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;
	viewDef.view.pillarbox4x3		= true;

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

	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:MainMenuBackground.jpg", &spec))
		DoFatalAlert("SetupMainMenuScreen: background pict not found.");

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (uintptr_t) gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("SetupMainMenuScreen: MO_CreateNewObjectOfType failed");


			/* LOAD SPRITES */

	TextMesh_LoadFont(gGameViewInfoPtr, "wallfont");


			/* SETUP TITLE MENU */

	BuildMenu(MENU_ID_TITLE);

	MakeFadeEvent(true);
}


/***************** DRAW MAINMENU CALLBACK *******************/

static void DrawMainMenuCallback(OGLSetupOutputType *info)
{
	MO_DrawObject(gBackgoundPicture, info);
	DrawObjects(info);
}



/********************** FREE MAINMENU ART **********************/

static void FreeMainMenuArt(void)
{
	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();
	TextMesh_DisposeFont();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}


#pragma mark -


/**************************** BUILD MENU *********************************/

static void BuildMenu(short menuNum)
{
short	i;

	gMainMenuSelection = 0;
	gWhichMenu = menuNum;

	DeleteAllObjects();

			/* COUNT MENU ITEMS */

	gNumMenuItems = 0;
	while (gNumMenuItems < MAX_ITEMS_PER_MENU)
	{
		if (0 == gMenuItemStrings[menuNum][gNumMenuItems])
			break;
		
		gNumMenuItems++;
	}

			/* BUILD NEW MENU STRINGS */

	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= -.1 + (float)gNumMenuItems * LINE_SPACING/2;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = MAINMENU_ICON_SCALE;

	for (i = 0; i < gNumMenuItems; i++)
	{
		const char* s = Localize(gMenuItemStrings[menuNum][i]);

		gIcons[i] = TextMesh_New(s, kTextMeshAlignCenter, &gNewObjectDefinition);

		gNewObjectDefinition.coord.y 	-= LINE_SPACING;

		if (i != gMainMenuSelection)
		{
			gIcons[i]->ColorFilter = gMainMenuNormalColor;									// normal
		}
		else
		{
			gIcons[i]->ColorFilter = gMainMenuHiliteColor;										// hilite
		}

	}


			/* INIT SOME STUFF FOR EACH MENU TYPE */

	switch(menuNum)
	{
		case	MENU_ID_TITLE:
				gGameMode = -1;										// no game mode selected yet
				break;

		case	MENU_ID_PLAY:
				gIsNetworkHost = false;								// assume I'm not hosting
				gIsNetworkClient = false;							// assume I'm not joining either
				gNetGameInProgress = false;
				gNumLocalPlayers = 1;								// assume just 1 local player on this machine
				gNumRealPlayers = 1;
				break;

		case	MENU_ID_NETGAME:
				gIsNetworkHost = false;								// assume I'm not hosting
				gIsNetworkClient = false;							// assume I'm not joining either
				break;

		case	MENU_ID_TOURNAMENT:									// see if need to disable any of the ages
				if ((gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE) == 0)
				{
					gIcons[1]->ColorFilter.a = .5;
					gIcons[2]->ColorFilter.a = .5;
				}
				else
				if ((gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE) == 1)
				{
					gIcons[2]->ColorFilter.a = .5;
				}
				break;

	}

}


/*********************** NAVIGATE MENU **************************/

static Boolean NavigateMenu(void)
{
short	i;
Boolean	startGame = false;
short	max;

				/* GET # LINES IN THIS MENU */

	if (gWhichMenu == MENU_ID_TOURNAMENT)
		max = gNumTracksUnlocked[gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE]/3;
	else
		max = gNumMenuItems;



		/* SEE IF CHANGE PART SELECTION */

	if (GetNewNeedStateAnyP(kNeed_UIUp) && (gMainMenuSelection > 0))
	{
		gMainMenuSelection--;
		PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0xff));
		gTimeUntilDemo = DEMO_DELAY;
	}
	else
	if (GetNewNeedStateAnyP(kNeed_UIDown))
	{
		if (gMainMenuSelection < (max-1))
		{
			gMainMenuSelection++;
			PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0xff));
			gTimeUntilDemo = DEMO_DELAY;
		}
		else
			PlayEffect(EFFECT_BADSELECT);
	}

		/* SET APPROPRIATE FRAMES FOR ICONS */

	for (i = 0; i < gNumMenuItems; i++)
	{

		if (i != gMainMenuSelection)
		{
			gIcons[i]->ColorFilter	= gMainMenuNormalColor;									// normal
		}
		else
		{
			gIcons[i]->ColorFilter = gMainMenuHiliteColor;										// hilite
		}
	}


			/* SET SOME STUFF  */

	switch(gWhichMenu)
	{
		case	MENU_ID_TOURNAMENT:									// see if need to disable any of the ages
				if ((gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE) == 0)
				{
					gIcons[1]->ColorFilter.a = .5;
					gIcons[2]->ColorFilter.a = .5;
				}
				else
				if ((gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE) == 1)
				{
					gIcons[2]->ColorFilter.a = .5;
				}
				break;

	}


			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		gTimeUntilDemo = DEMO_DELAY;

		PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
		switch(gWhichMenu)
		{
			case	MENU_ID_TITLE:
					switch(gMainMenuSelection)
					{
						case	0:								// PLAY GAME
								BuildMenu(MENU_ID_PLAY);
								break;

						case	1:								// SAVED GAME
								DoSavedPlayerDialog();
								break;

						case	2:								// OPTIONS

								BuildMenu(MENU_ID_OPTIONS);
								break;

						case	3:								// QUIT
								CleanQuit();
								break;

					}
					break;

			case	MENU_ID_PLAY:
					switch(gMainMenuSelection)
					{
						case	0:								// 1 PLAYER
								gNumRealPlayers = 1;
								gNumLocalPlayers = 1;
								BuildMenu(MENU_ID_1PLAYERGAMETYPE);
								break;

						case	1:								// 2 PLAYER
								gNumRealPlayers = 2;
								gNumLocalPlayers = 2;
//								gNumRealPlayers = 1;
//								gNumLocalPlayers = 1;
								BuildMenu(MENU_ID_MULTIPLAYERGAMETYPE);
								break;

						case	2:								// NETGAME
								gNumRealPlayers = 1;
								gNetGameInProgress = true;
								BuildMenu(MENU_ID_NETGAME);
								break;
					}
					break;


			case	MENU_ID_OPTIONS:
					switch(gMainMenuSelection)
					{
						case	0:								// SETTINGS
								DoGameSettingsDialog();
								BuildMenu(gWhichMenu);			// rebuild menu in case language changed
								break;

						case	1:								// HELP
								FreeMainMenuArt();
								DoHelpScreen();
								SetupMainMenuScreen();
								BuildMenu(gWhichMenu);
								MakeFadeEvent(true);
								break;

						case	2:								// CREDITS
								FreeMainMenuArt();
								DoCreditsScreen();
								SetupMainMenuScreen();
								BuildMenu(gWhichMenu);
								break;

						case	3:								// PHYSICS EDITOR
								DoPhysicsEditor();
								BuildMenu(gWhichMenu);
								break;
					}
					break;


			case	MENU_ID_1PLAYERGAMETYPE:
					switch(gMainMenuSelection)
					{
						case	0:								// PRACTICE
								gGameMode = GAME_MODE_PRACTICE;
								startGame = true;
								break;

						case	1:								// TOURNAMENT
								gGameMode = GAME_MODE_TOURNAMENT;
								DoSavedPlayerDialog();

//								if (!gSavedPlayerIsLoaded)		// must have loaded a player to do this
//								{
//									DoSavedPlayerDialog();
//								}
//								else
								{
									BuildMenu(MENU_ID_TOURNAMENT);
								}
								break;
					}
					break;


			case	MENU_ID_MULTIPLAYERGAMETYPE:
					switch(gMainMenuSelection)
					{
						case	0:								// RACE
								gGameMode = GAME_MODE_MULTIPLAYERRACE;
								startGame = true;
								break;

						case	1:								// TAG 1
								gGameMode = GAME_MODE_TAG1;
								startGame = true;
								break;

						case	2:								// TAG 2
								gGameMode = GAME_MODE_TAG2;
								startGame = true;
								break;

						case	3:								// SURVIVAL
								gGameMode = GAME_MODE_SURVIVAL;
								startGame = true;
								break;

						case	4:								// CAPTURE FLAG
								gGameMode = GAME_MODE_CAPTUREFLAG;
								startGame = true;
								break;
					}
					break;




			case	MENU_ID_TOURNAMENT:
					switch(gMainMenuSelection)
					{
						case	0:								// STONE AGE
								gTheAge = STONE_AGE;
								startGame = true;
								break;

						case	1:								// BRONZE AGE
								gTheAge = BRONZE_AGE;
								startGame = true;
								break;

						case	2:								// IRON AGE
								gTheAge = IRON_AGE;
								startGame = true;
								break;
					}
					break;


			case	MENU_ID_NETGAME:
					switch(gMainMenuSelection)
					{
						case	0:								// HOST NET GAME
								gIsNetworkHost = true;
								BuildMenu(MENU_ID_MULTIPLAYERGAMETYPE);	// host gets to select the game type
								break;

						case	1:								// JOIN NET GAME
								gIsNetworkClient = true;
								startGame = true;
								break;
					}
					break;

		}
	}

			/*****************************/
			/* SEE IF CANCEL A SELECTION */
			/*****************************/
			//
			// If cancel on any menu, always go back to previous menu
			//
	else
	if (GetNewNeedStateAnyP(kNeed_UIBack))
	{
		gTimeUntilDemo = DEMO_DELAY;

		PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
		switch(gWhichMenu)
		{
			case	MENU_ID_PLAY:
			case	MENU_ID_OPTIONS:
					BuildMenu(MENU_ID_TITLE);
					break;

			case	MENU_ID_1PLAYERGAMETYPE:
			case	MENU_ID_NETGAME:
					BuildMenu(MENU_ID_PLAY);
					break;

			case	MENU_ID_MULTIPLAYERGAMETYPE:
					if (gNetGameInProgress)
						BuildMenu(MENU_ID_NETGAME);
					else
						BuildMenu(MENU_ID_PLAY);
					break;


			case	MENU_ID_TOURNAMENT:
					if (gNetGameInProgress)
						BuildMenu(MENU_ID_MULTIPLAYERGAMETYPE);
					else
						BuildMenu(MENU_ID_1PLAYERGAMETYPE);
					break;


		}
	}


	return(startGame);
}


#pragma mark -


/********************* DO GAME SETTINGS DIALOG *********************/

void DoGameSettingsDialog(void)
{
	IMPLEMENT_ME();
#if 0
DialogRef 		myDialog;
DialogItemType			itemType,itemHit;
Handle			itemHandle;
ControlRef		ctrlHandle;
Rect			itemRect;
Boolean			dialogDone;

	Enter2D(true);

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());

	myDialog = GetNewDialog(1000 + gGamePrefs.language,nil,MOVE_TO_FRONT);



				/* SET CONTROL VALUES */

	GetDialogItemAsControl(myDialog,2,&ctrlHandle);		// split screen mode
	SetControlValue(ctrlHandle,gGamePrefs.desiredSplitScreenMode == SPLITSCREEN_MODE_VERT);
	GetDialogItemAsControl(myDialog,3,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.desiredSplitScreenMode == SPLITSCREEN_MODE_HORIZ);

	GetDialogItemAsControl(myDialog,4,&ctrlHandle);		// difficulty
	SetControlValue(ctrlHandle,gGamePrefs.difficulty == 0);
	GetDialogItemAsControl(myDialog,5,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.difficulty == 1);
	GetDialogItemAsControl(myDialog,6,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.difficulty == 2);
	GetDialogItemAsControl(myDialog,7,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.difficulty == 3);

	GetDialogItemAsControl(myDialog,14,&ctrlHandle);		// language
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_ENGLISH);
	GetDialogItemAsControl(myDialog,15,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_FRENCH);
	GetDialogItemAsControl(myDialog,16,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_GERMAN);
	GetDialogItemAsControl(myDialog,17,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_SPANISH);
	GetDialogItemAsControl(myDialog,18,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_ITALIAN);
	GetDialogItemAsControl(myDialog,19,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_SWEDISH);

	GetDialogItemAsControl(myDialog,20,&ctrlHandle);		// tag duration
	SetControlValue(ctrlHandle,gGamePrefs.tagDuration == 2);
	GetDialogItemAsControl(myDialog,21,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.tagDuration == 3);
	GetDialogItemAsControl(myDialog,22,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.tagDuration == 4);


			/* SET OUTLINE FOR USERITEM */

	GetDialogItem(myDialog,1,&itemType,&itemHandle,&itemRect);
	SetDialogItem(myDialog, 10, userItem, (Handle)NewUserItemUPP(DoBold), &itemRect);



				/* DO IT */

	dialogDone = false;
	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case 	1:									// hit ok
					dialogDone = true;
					break;

			case	2:
					gGamePrefs.desiredSplitScreenMode = SPLITSCREEN_MODE_VERT;
					GetDialogItemAsControl(myDialog,2,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.desiredSplitScreenMode == SPLITSCREEN_MODE_VERT);
					GetDialogItemAsControl(myDialog,3,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.desiredSplitScreenMode == SPLITSCREEN_MODE_HORIZ);
					break;

			case	3:
					gGamePrefs.desiredSplitScreenMode = SPLITSCREEN_MODE_HORIZ;
					GetDialogItemAsControl(myDialog,2,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.desiredSplitScreenMode == SPLITSCREEN_MODE_VERT);
					GetDialogItemAsControl(myDialog,3,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.desiredSplitScreenMode == SPLITSCREEN_MODE_HORIZ);
					break;


			case	4:
			case	5:
			case	6:
			case	7:
					gGamePrefs.difficulty = itemHit - 4;
					GetDialogItemAsControl(myDialog,4,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.difficulty == 0);
					GetDialogItemAsControl(myDialog,5,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.difficulty == 1);
					GetDialogItemAsControl(myDialog,6,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.difficulty == 2);
					GetDialogItemAsControl(myDialog,7,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.difficulty == 3);
					break;


			case	14:
			case	15:
			case	16:
			case	17:
			case	18:
			case	19:
					gGamePrefs.language = LANGUAGE_ENGLISH + itemHit - 14;
					GetDialogItemAsControl(myDialog,14,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_ENGLISH);
					GetDialogItemAsControl(myDialog,15,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_FRENCH);
					GetDialogItemAsControl(myDialog,16,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_GERMAN);
					GetDialogItemAsControl(myDialog,17,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_SPANISH);
					GetDialogItemAsControl(myDialog,18,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_ITALIAN);
					GetDialogItemAsControl(myDialog,19,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_SWEDISH);
					break;

			case	11:
					DoKeyConfigDialog();
					break;

			case	20:
			case	21:
			case	22:
					gGamePrefs.tagDuration = 2 + (itemHit-20);
					GetDialogItemAsControl(myDialog,20,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.tagDuration == 2);
					GetDialogItemAsControl(myDialog,21,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.tagDuration == 3);
					GetDialogItemAsControl(myDialog,22,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.tagDuration == 4);
					break;

		}
	}


			/* CLEAN UP */

	DisposeDialog(myDialog);

	Exit2D();


		/* UPDATE ACTIVE SPLIT SCREEN MODE */

	if (gActiveSplitScreenMode != SPLITSCREEN_MODE_NONE)
	{
		gActiveSplitScreenMode = gGamePrefs.desiredSplitScreenMode;
	}


	CalcFramesPerSecond();				// reset this so things dont go crazy when we return
	CalcFramesPerSecond();
#endif
}




#pragma mark -

/********************** DO PHYSICS EDITOR ***********************/

static void DoPhysicsEditor(void)
{
	IMPLEMENT_ME();
#if 0
DialogRef 		myDialog;
DialogItemType	itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
Boolean			dialogDone;
int				i;
int				selectedCar = 0;
Str255			s;

	Enter2D(true);


	InitCursor();
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());


	myDialog = GetNewDialog(129,nil,MOVE_TO_FRONT);



				/* SET CONTROL VALUES */
reset:
	for (i = 0; i < 10; i++)
	{
		GetDialogItem(myDialog,3+i,&itemType,(Handle *)&itemHandle,&itemRect);
		SetControlValue((ControlHandle)itemHandle,selectedCar == i);
	}
	for (i = 0; i < 4; i++)
	{
		NumToString(gVehicleParameters[selectedCar][i],s);
		GetDialogItem(myDialog,13+i,&itemType,(Handle *)&itemHandle,&itemRect);
		SetDialogItemText((Handle)itemHandle,s);
	}

	GetDialogItem(myDialog,17,&itemType,(Handle *)&itemHandle,&itemRect);
	FloatToString(gSteeringResponsiveness,s);
	SetDialogItemText((Handle)itemHandle,s);

	GetDialogItem(myDialog,18,&itemType,(Handle *)&itemHandle,&itemRect);
	FloatToString(gCarMaxTightTurn,s);
	SetDialogItemText((Handle)itemHandle,s);

	GetDialogItem(myDialog,19,&itemType,(Handle *)&itemHandle,&itemRect);
	FloatToString(gCarTurningRadius,s);
	SetDialogItemText((Handle)itemHandle,s);

	FloatToString(gTireTractionConstant,s);
	GetDialogItem(myDialog,20,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItemText((Handle)itemHandle,s);

	FloatToString(gTireFrictionConstant,s);
	GetDialogItem(myDialog,21,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItemText((Handle)itemHandle,s);

	FloatToString(gCarGravity,s);
	GetDialogItem(myDialog,22,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItemText((Handle)itemHandle,s);

	FloatToString(gSlopeRatioAdjuster,s);
	GetDialogItem(myDialog,23,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItemText((Handle)itemHandle,s);


			/* SET OUTLINE FOR USERITEM */

	GetDialogItem(myDialog,38,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItem(myDialog,38, userItem, (Handle)NewUserItemUPP(DoOutline), &itemRect);


				/* DO IT */

	dialogDone = false;
	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case 	1:									// hit ok
					dialogDone = true;
					break;

			case	2:									// defaults
					SetDefaultPhysics();
					goto reset;

			case	3:									// Car Selection
			case	4:
			case	5:
			case	6:
			case	7:
			case	8:
			case	9:
			case	10:
			case	11:
			case	12:
					selectedCar = itemHit-3;
					for (i = 0; i < 10; i++)
					{
						GetDialogItem(myDialog,3+i,&itemType,(Handle *)&itemHandle,&itemRect);
						SetControlValue((ControlHandle)itemHandle,selectedCar == i);
					}
					for (i = 0; i < 4; i++)
					{
						NumToString(gVehicleParameters[selectedCar][i],s);
						GetDialogItem(myDialog,13+i,&itemType,(Handle *)&itemHandle,&itemRect);
						SetDialogItemText((Handle)itemHandle,s);
					}
					break;

			case	13:									// Car Parameters
			case	14:
			case	15:
			case	16:
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,s);
					StringToNum(s, (long *)&gVehicleParameters[selectedCar][itemHit-13]);
					break;

			case	17:
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,s);
					gSteeringResponsiveness = StringToFloat(s);
					break;

			case	18:
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,s);
					gCarMaxTightTurn = StringToFloat(s);
					break;

			case	19:
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,s);
					gCarTurningRadius = StringToFloat(s);
					break;

			case	20:
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,s);
					gTireTractionConstant = StringToFloat(s);
					break;

			case	21:
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,s);
					gTireFrictionConstant = StringToFloat(s);
					break;

			case	22:
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,s);
					gCarGravity = StringToFloat(s);
					break;

			case	23:
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,s);
					gSlopeRatioAdjuster = StringToFloat(s);
					break;

		}
	}
	DisposeDialog(myDialog);

	Exit2D();
#endif
}
