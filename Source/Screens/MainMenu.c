/****************************/
/*   	MAINMENU SCREEN.C	*/
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
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
#include "menu.h"


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
static void DoPhysicsEditor(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAINMENU_ICON_SCALE	.55f

#define	MAX_LINES		5

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

static void OnConfirmPlayMenu(const MenuItem* mi)
{
	switch (mi->id)
	{
		case 1:
			gIsNetworkHost = false;								// assume I'm not hosting
			gIsNetworkClient = false;							// assume I'm not joining either
			gNetGameInProgress = false;
			gNumLocalPlayers = 1;								// assume just 1 local player on this machine
			gNumRealPlayers = 1;
			break;
		
		case 2:
			gIsNetworkHost = false;								// assume I'm not hosting
			gIsNetworkClient = false;							// assume I'm not joining either
			gNetGameInProgress = false;
			gNumLocalPlayers = 2;								// assume just 1 local player on this machine
			gNumRealPlayers = 2;
			break;

		case 3:
			gIsNetworkHost = false;								// assume I'm not hosting
			gIsNetworkClient = false;							// assume I'm not joining either
			gNetGameInProgress = true;
			gNumRealPlayers = 1;
			gNumLocalPlayers = 1;
			break;
	}
}

static void OnPickGameMode(const MenuItem* mi)
{
	gGameMode = GAME_CLAMP(mi->id, 0, NUM_GAME_MODES);
}

static void OnPickTournamentAge(const MenuItem* mi)
{
	gTheAge = GAME_CLAMP(mi->id, 0, NUM_AGES-1);
}

static void OnPickHostOrJoin(const MenuItem* mi)
{
	switch (mi->text)
	{
		case STR_HOST_NET_GAME:
			gIsNetworkHost = true;
			gIsNetworkClient = false;
			break;
		
		case STR_JOIN_NET_GAME:
			gIsNetworkHost = false;
			gIsNetworkClient = true;
			break;

		default:
			DoAlert("Unsupported host/join mode: %d", mi->text);
			break;
	}
}


//static short	gMainMenuSelection,gWhichMenu;

static const OGLColorRGBA gMainMenuHiliteColor = {.3,.5,.2,1};
static const OGLColorRGBA gMainMenuNormalColor = {1,1,1,1};

static float	gTimeUntilDemo;

static bool IsTournamentAgeAvailable(const MenuItem* mi)
{
	return mi->id < (gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE);
}

enum
{
	kPick_QuitGame,
	kPick_MPRace,
	kPick_MPKeepAwayTag,
	kPick_MPStampedeTag,
};

static void OnPickQuit(const MenuItem* mi)
{
	CleanQuit();
}

static const MenuItem
	gMenuTitle[] =
	{
		{ kMenuItem_Pick, STR_NEW_GAME, .gotoMenu=MENU_ID_PLAY, },
//		{ kMenuItem_Pick, STR_LOAD_GAME },  // DoSavedPlayerDialog
		{ kMenuItem_Pick, STR_OPTIONS, .gotoMenu=MENU_ID_OPTIONS, },
		{ kMenuItem_Pick, STR_QUIT, .callback=OnPickQuit, .gotoMenu=-1 },  // Quit
		{ kMenuItem_END_SENTINEL },
	},

	gMenuPlay[] =
	{
		{ kMenuItem_Pick, STR_1PLAYER,	.id=1, .callback=OnConfirmPlayMenu, .gotoMenu=MENU_ID_1PLAYERGAMETYPE },
		{ kMenuItem_Pick, STR_2PLAYER,	.id=2, .callback=OnConfirmPlayMenu, .gotoMenu=MENU_ID_MULTIPLAYERGAMETYPE },
		{ kMenuItem_Pick, STR_NET_GAME,	.id=3, .callback=OnConfirmPlayMenu, .gotoMenu=MENU_ID_NETGAME },
		{ kMenuItem_END_SENTINEL },
	},

	gMenuOptions[] =
	{
		{ kMenuItem_Pick, STR_SETTINGS, .gotoMenu=0 },
		{ kMenuItem_Pick, STR_HELP, .gotoMenu=0 },		// FreeMainMenuArt, DoHelpScreen, SetupMainMenuScreen, BuildMenu, MakeFadeEvent
		{ kMenuItem_Pick, STR_CREDITS, .gotoMenu=0 },	// FreeMainMenuArt, DoCreditsScreen, SetupMainMenuScreen, BuildMenu, MakeFadeEvent
		{ kMenuItem_Pick, STR_PHYSICS_EDITOR, .gotoMenu=0 },	// DoPhysicsEditor
		{ kMenuItem_END_SENTINEL },
	},

	gMenuMPGameModes[] =
	{
		{ kMenuItem_Pick,	STR_RACE,			.callback=OnPickGameMode, .id=GAME_MODE_MULTIPLAYERRACE,	.gotoMenu=-1, },
		{ kMenuItem_Pick,	STR_KEEP_AWAY_TAG,	.callback=OnPickGameMode, .id=GAME_MODE_TAG1,				.gotoMenu=-1, },
		{ kMenuItem_Pick,	STR_STAMPEDE_TAG,	.callback=OnPickGameMode, .id=GAME_MODE_TAG2,				.gotoMenu=-1, },
		{ kMenuItem_Pick,	STR_SURVIVAL,		.callback=OnPickGameMode, .id=GAME_MODE_SURVIVAL,			.gotoMenu=-1, },
		{ kMenuItem_Pick,	STR_QUEST_FOR_FIRE,	.callback=OnPickGameMode, .id=GAME_MODE_CAPTUREFLAG,		.gotoMenu=-1, },
		{ kMenuItem_END_SENTINEL },
	},

	gMenu1PGameModes[] =
	{
		{ kMenuItem_Pick,	STR_PRACTICE,		.callback=OnPickGameMode, .id=GAME_MODE_PRACTICE,			.gotoMenu=-1 },
		{ kMenuItem_Pick,	STR_TOURNAMENT,		.callback=OnPickGameMode, .id=GAME_MODE_TOURNAMENT,			.gotoMenu=MENU_ID_TOURNAMENT, },
		// ^^^ TODO: if picking tournament, pick saved game file?
		{ kMenuItem_END_SENTINEL },
	},

	gMenuTournament[] =
	{
		{ kMenuItem_Pick, STR_STONE_AGE,	.callback=OnPickTournamentAge, .id=STONE_AGE,	.gotoMenu=-1 },
		{ kMenuItem_Pick, STR_BRONZE_AGE,	.callback=OnPickTournamentAge, .id=BRONZE_AGE,	.gotoMenu=-1, .enableIf=IsTournamentAgeAvailable },
		{ kMenuItem_Pick, STR_IRON_AGE,		.callback=OnPickTournamentAge, .id=IRON_AGE,	.gotoMenu=-1, .enableIf=IsTournamentAgeAvailable },
		{ kMenuItem_END_SENTINEL },
	},

	gMenuNetGame[] =
	{
		{ kMenuItem_Pick, STR_HOST_NET_GAME, .callback=OnPickHostOrJoin, .id=0, .gotoMenu=MENU_ID_MULTIPLAYERGAMETYPE }, // host gets to select game type
		{ kMenuItem_Pick, STR_JOIN_NET_GAME, .callback=OnPickHostOrJoin, .id=1, .gotoMenu=-1 },
		{ kMenuItem_END_SENTINEL },
	};



static const MenuItem* gMainMenuTree[NUM_MENU_IDS] =
{
	[MENU_ID_TITLE] = gMenuTitle,
	[MENU_ID_PLAY] = gMenuPlay,
	[MENU_ID_OPTIONS] = gMenuOptions,
	[MENU_ID_MULTIPLAYERGAMETYPE] = gMenuMPGameModes,
	[MENU_ID_1PLAYERGAMETYPE] = gMenu1PGameModes,
	[MENU_ID_TOURNAMENT] = gMenuTournament,
	[MENU_ID_NETGAME] = gMenuNetGame,
};







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

	int outcome = StartMenuTree(
		gMainMenuTree,
		NULL,
		NULL,
		DrawMainMenuCallback
	);

#if 0
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
#endif

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


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= 1.0;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor 		= (OGLColorRGBA) {.76f, .61f, .45f, 1.0f};
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

//	BuildMenu(MENU_ID_TITLE);

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
