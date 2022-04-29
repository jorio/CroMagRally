/****************************/
/*   	SELECT TRACK.C 	    */
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupTrackSelectScreen(void);
static Boolean DoTrackSelectControls(void);
static void MakeTrackName(void);
static void UpdateSelectedTrack(void);


/****************************/
/*    CONSTANTS             */
/****************************/


enum
{
	TRACKSELECT_SObjType__NULL = 0,

	TRACKSELECT_SObjType__Level0,			// stone age
	TRACKSELECT_SObjType__Level1,
	TRACKSELECT_SObjType__Level2,

	TRACKSELECT_SObjType__Level3,			// bronze age
	TRACKSELECT_SObjType__Level4,
	TRACKSELECT_SObjType__Level5,

	TRACKSELECT_SObjType__Level6,			// iron age
	TRACKSELECT_SObjType__Level7,
	TRACKSELECT_SObjType__Level8,

	TRACKSELECT_SObjType__Arena0,			// stone henge
	TRACKSELECT_SObjType__Arena1,
	TRACKSELECT_SObjType__Arena2,			// aztec
	TRACKSELECT_SObjType__Arena3,			// maze
	TRACKSELECT_SObjType__Arena4,
	TRACKSELECT_SObjType__Arena5,
	TRACKSELECT_SObjType__Arena6,
	TRACKSELECT_SObjType__Arena7
};


#define	LEFT_ARROW_X	-208
#define RIGHT_ARROW_X	208
#define	ARROW_Y			0
#define	ARROW_SCALE		.5

#define	LEVEL_IMAGE_X		0
#define	LEVEL_IMAGE_Y		-5
#define	LEVEL_IMAGE_SCALE	.58


#define	NUM_PRACTICE_TRACKS		9
#define	NUM_BATTLE_TRACKS		8

/*********************/
/*    VARIABLES      */
/*********************/

static int		gSelectedTrackIndex;
static ObjNode	*gTrackImageIcon = nil;
static ObjNode	*gTrackName = nil;
static ObjNode	*gTrackLeftArrow = nil;
static ObjNode	*gTrackRightArrow = nil;
static ObjNode	*gTrackPadlock = nil;

static	short	gNumTracksInSelection;
static 	short	gBaseTrack;

const short gNumTracksUnlocked[] = {3,6,9,9};



/********************** MOVE LEFT/RIGHT ARROWS **************************/

static void MoveArrow(ObjNode* theNode)
{
	if (!theNode->Flag[0])
	{
		// Not initialized: save home position
		theNode->SpecialF[0] = theNode->Coord.x;
		theNode->SpecialF[1] = theNode->Coord.y;
		theNode->Flag[0] = true;
	}

	float homeX = theNode->SpecialF[0];
	float homeY = theNode->SpecialF[1];

	float dx = theNode->Coord.x - homeX;
	float dy = theNode->Coord.y - homeY;

	if (fabsf(dx) < 0.2f) dx = 0;
	if (fabsf(dy) < 0.2f) dy = 0;

	theNode->Coord.x -= 12.0f * gFramesPerSecondFrac * dx;
	theNode->Coord.y -= 12.0f * gFramesPerSecondFrac * dy;
	UpdateObjectTransforms(theNode);
}



/********************** SELECT PRACTICE TRACK **************************/
//
// Return true if user wants to back up a menu
//

Boolean SelectSingleTrack(void)
{
	SetupTrackSelectScreen();
	MakeFadeEvent(true);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (DoTrackSelectControls())
			break;


			/**************/
			/* DRAW STUFF */
			/**************/

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawObjects);
	}


			/***********/
			/* CLEANUP */
			/***********/

	OGL_FadeOutScene(gGameViewInfoPtr, DrawObjects, NULL);

	DeleteAllObjects();
	DisposeAllSpriteGroups();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);

	if (gSelectedTrackIndex == -1)								// see if bail
		return(true);

		/* CONVERT THE INDEX INTO A GLOGAL TRACK NUMBER */

	gTrackNum = gBaseTrack + gSelectedTrackIndex;				// set global track # to play
	return(false);
}


/********************* SETUP TRACKSELECT SCREEN **********************/

static void SetupTrackSelectScreen(void)
{
OGLSetupInputType	viewDef;


	gSelectedTrackIndex = 0;
	gTrackName = nil;


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);
	viewDef.view.pillarbox4x3		= true;
	viewDef.view.clearColor 		= (OGLColorRGBA) {.76f, .61f, .45f, 1.0f};
	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);



				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */

	MakeBackgroundPictureObject(":images:TrackSelectScreen.png");

			/* LOAD SPRITES */

	switch(gGameMode)
	{
		case	GAME_MODE_PRACTICE:
		case	GAME_MODE_MULTIPLAYERRACE:
				gNumTracksInSelection = NUM_PRACTICE_TRACKS;
				gBaseTrack = 0;
				LoadSpriteGroup(SPRITE_GROUP_TRACKSELECTSCREEN, "trackselectsp", 0, gGameViewInfoPtr);
				break;

		default:
				gNumTracksInSelection = NUM_BATTLE_TRACKS;
				gBaseTrack = 9;
				LoadSpriteGroup(SPRITE_GROUP_TRACKSELECTSCREEN, "trackselectmp", 0, gGameViewInfoPtr);
	}

	LoadSpriteGroup(SPRITE_GROUP_MAINMENU, "menus", 0, gGameViewInfoPtr);



			/*****************/
			/* BUILD OBJECTS */
			/*****************/

			/* TRACK IMAGE */

	{
		NewObjectDefinitionType def =
		{
			.group = SPRITE_GROUP_TRACKSELECTSCREEN,
			.type = TRACKSELECT_SObjType__Level0 + gBaseTrack + gSelectedTrackIndex,
			.coord = { LEVEL_IMAGE_X, LEVEL_IMAGE_Y, 0 },
			.slot = BGPIC_SLOT-1,
			.moveCall = nil,
			.scale = LEVEL_IMAGE_SCALE,
		};

		gTrackImageIcon = MakeSpriteObject(&def, gGameViewInfoPtr);
	}

	{
		NewObjectDefinitionType def =
		{
			.group = SPRITE_GROUP_MAINMENU,
			.type = MENUS_SObjType_LeftArrow,
			.coord = { LEFT_ARROW_X, ARROW_Y, 0 },
			.slot = SPRITE_SLOT,
			.moveCall = MoveArrow,
			.scale = ARROW_SCALE,
		};
		gTrackLeftArrow = MakeSpriteObject(&def, gGameViewInfoPtr);

		def.type = MENUS_SObjType_RightArrow;
		def.coord.x = RIGHT_ARROW_X;
		gTrackRightArrow = MakeSpriteObject(&def, gGameViewInfoPtr);
	}

	{
		NewObjectDefinitionType def =
		{
			.group = SPRITE_GROUP_MAINMENU,
			.type = MENUS_SObjType_Padlock,
			.coord = { 0, ARROW_Y, 0 },
			.slot = SPRITE_SLOT,
			.moveCall = MoveArrow,
			.scale = ARROW_SCALE,
		};
		gTrackPadlock = MakeSpriteObject(&def, gGameViewInfoPtr);
	}



			/* TRACK NAME */

	UpdateSelectedTrack();
}

/******************** MAKE TRACK NAME ************************/

static void MakeTrackName(void)
{
	if (gTrackName)
	{
		DeleteObject(gTrackName);
		gTrackName = nil;
	}

	NewObjectDefinitionType def =
	{
		.coord		= {0, -168, 0},
		.scale		= .7,
		.slot		= SPRITE_SLOT,
	};

	switch(gGameMode)
	{
		case	GAME_MODE_PRACTICE:
		case	GAME_MODE_MULTIPLAYERRACE:
				gTrackName = TextMesh_New(Localize(STR_LEVEL_1 + gSelectedTrackIndex), kTextMeshAlignCenter, &def);
				break;

		default:
				gTrackName = TextMesh_New(Localize(STR_MPLEVEL_1 + gSelectedTrackIndex), kTextMeshAlignCenter, &def);
	}
}



static void UpdateSelectedTrack(void)
{
		/* SET TRACK SCREENSHOT */

	ModifySpriteObjectFrame(gTrackImageIcon, TRACKSELECT_SObjType__Level0 + gBaseTrack + gSelectedTrackIndex, gGameViewInfoPtr);

		/* MAKE TRACK NAME */

	MakeTrackName();

		/* SHOW/HIDE ARROWS */

	SetObjectVisible(gTrackLeftArrow, gSelectedTrackIndex > 0);
	SetObjectVisible(gTrackRightArrow, gSelectedTrackIndex < (gNumTracksInSelection - 1));

		/**********************/
		/* SEE IF DRAW LOCKED */
		/**********************/

	short highestUnlocked;

	switch (gGameMode)
	{
	case	GAME_MODE_PRACTICE:
		highestUnlocked = gNumTracksUnlocked[GetNumAgesCompleted()] - 1;
		break;

	default:
		highestUnlocked = 1000;
	}

	SetObjectVisible(gTrackPadlock, gSelectedTrackIndex > highestUnlocked);
}




/***************** DO TRACKSELECT CONTROLS *******************/

static Boolean DoTrackSelectControls(void)
{
short				highestUnlocked;

	if (gGameMode ==	GAME_MODE_PRACTICE)
		highestUnlocked = gNumTracksUnlocked[GetNumAgesCompleted()] - 1;
	else
		highestUnlocked = 1000;



		/* SEE IF ABORT */

	if (GetNewNeedStateAnyP(kNeed_UIBack))
	{
		gSelectedTrackIndex = -1;
		return(true);
	}

		/* SEE IF SELECT THIS ONE */

	if (GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		if (gSelectedTrackIndex > highestUnlocked)
		{
			PlayEffect(EFFECT_BADSELECT);
		}
		else
		{
			PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
			return(true);
		}
	}
	else
	if (IsCheatKeyComboDown())
	{
		PlayEffect(EFFECT_ROMANCANDLE_LAUNCH);
		return true;
	}


		/* SEE IF CHANGE SELECTION */

	if (GetNewNeedStateAnyP(kNeed_UILeft) && (gSelectedTrackIndex > 0))
	{
		gSelectedTrackIndex--;
		PlayEffect(EFFECT_SELECTCLICK);
		UpdateSelectedTrack();
		gTrackLeftArrow->Coord.x = LEFT_ARROW_X - 16;
	}
	else
	if (GetNewNeedStateAnyP(kNeed_UIRight) && (gSelectedTrackIndex < (gNumTracksInSelection-1)))
	{
		gSelectedTrackIndex++;
		PlayEffect(EFFECT_SELECTCLICK);
		UpdateSelectedTrack();
		gTrackRightArrow->Coord.x = RIGHT_ARROW_X + 16;
	}

	return(false);
}
