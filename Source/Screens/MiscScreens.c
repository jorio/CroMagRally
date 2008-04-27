/****************************/
/*   	MISCSCREENS.C	    */
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "miscscreens.h"
#include "objects.h"
#include "windows.h"
#include "input.h"
#include	"file.h"
#include	"ogl_support.h"
#include "ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "bg3d.h"
#include "skeletonobj.h"
#include "terrain.h"
#include "camera.h"
#include "mobjtypes.h"
#include "sprites.h"
#include "sobjtypes.h"
#include "effects.h"
#include "skeletonanim.h"
#include "skeletonJoints.h"
#include "sound2.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow,g1PixelWindow;
extern	FSSpec		gDataSpec;
extern	KeyMap gKeyMap,gNewKeys;
extern	short		gNumRealPlayers;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gOSX,gDisableAnimSounds,gIsNetworkHost,gIsNetworkClient;
extern	PrefsType	gGamePrefs;
extern	OGLPoint3D	gCoord;
extern	int			gGameMode,gTheAge;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D	gDelta;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DisplayPicture_Draw(OGLSetupOutputType *info);
static void SetupConqueredScreen(void);
static void FreeConqueredScreen(void);
static void MoveCup(ObjNode *theNode);
static void DrawConqueredCallback(OGLSetupOutputType *info);

static void SetupWinScreen(void);
static void MoveWinCups(ObjNode *theNode);
static void FreeWinScreen(void);
static void DrawWinCallback(OGLSetupOutputType *info);
static void MoveWinGuy(ObjNode *theNode);

static void SetupCreditsScreen(void);
static void FreeCreditsScreen(void);
static void DrawCreditsCallback(OGLSetupOutputType *info);
static void MoveCredit(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

MOPictureObject 	*gBackgoundPicture = nil;

OGLSetupOutputType	*gScreenViewInfoPtr = nil;





#pragma mark -


/********************** DISPLAY PICTURE **************************/
//
// Displays a picture using OpenGL until the user clicks the mouse or presses any key.
// If showAndBail == true, then show it and bail out
//

void DisplayPicture(FSSpec *spec, Boolean showAndBail, Boolean doKeyText)
{
OGLSetupInputType	viewDef;
OGLSetupOutputType	*pictureViewInfoPtr = nil;


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;

	OGL_SetupWindow(&viewDef, &pictureViewInfoPtr);


			/* CREATE BACKGROUND OBJECT */

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)pictureViewInfoPtr, spec);
	if (!gBackgoundPicture)
		DoFatalAlert("\pDisplayPicture: MO_CreateNewObjectOfType failed");


			/* CREATE TEXT */

	if (doKeyText)
	{
		FSSpec	spec2;
		Str255	s;

		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:rockfont.sprites", &spec2);
		LoadSpriteFile(&spec2, SPRITE_GROUP_FONT, pictureViewInfoPtr);

		gNewObjectDefinition.coord.x 	= 0;
		gNewObjectDefinition.coord.y 	= -.94;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = .3;
		gNewObjectDefinition.slot 		= SPRITE_SLOT;

		GetIndString(s, 4000 + gGamePrefs.language, 1);				// get "PRESS ANY KEY" string
		MakeFontStringObject(s, &gNewObjectDefinition, pictureViewInfoPtr, true);
	}




		/***********/
		/* SHOW IT */
		/***********/

	if (showAndBail)
	{
		int	i;

		g1PixelWindow = GetNewCWindow(129, nil, MOVE_TO_FRONT);			// create 1 pixel dummy window to cover the corner of the screen to prevent full-screen mode from kicking in.

		for (i = 0; i < 10; i++)
			OGL_DrawScene(pictureViewInfoPtr, DisplayPicture_Draw);
		GammaFadeIn();
	}
	else
	{
		float	timeout = 20.0f;

		MakeFadeEvent(true);
		ReadKeyboard();
		CalcFramesPerSecond();


					/* MAIN LOOP */

		while(!Button())
		{
			if (gOSX)						// hack to fix OS 10.1 audio bug
				MyFlushEvents();

			CalcFramesPerSecond();
			MoveObjects();
			OGL_DrawScene(pictureViewInfoPtr, DisplayPicture_Draw);

			ReadKeyboard();
			if (AreAnyNewKeysPressed())
				break;

			timeout -= gFramesPerSecondFrac;
			if (timeout < 0.0f)
				break;
		}

	}

			/* CLEANUP */

	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();


			/* FADE OUT */

	if (!showAndBail)
		GammaFadeOut();


	OGL_DisposeWindowSetup(&pictureViewInfoPtr);


}


/***************** DISPLAY PICTURE: DRAW *******************/

static void DisplayPicture_Draw(OGLSetupOutputType *info)
{
	MO_DrawObject(gBackgoundPicture, info);
	DrawObjects(info);
}


#pragma mark -


/********************* SHOW AGE PICTURE **********************/

void ShowAgePicture(int age)
{
FSSpec	spec;
Str31	names[NUM_AGES] =
{
	"\p:images:Ages:StoneAgeIntro",
	"\p:images:Ages:BronzeAgeIntro",
	"\p:images:Ages:IronAgeIntro"
};


	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, names[age], &spec);

	DisplayPicture(&spec, false, true);
//	GammaFadeOut();
}


/********************* SHOW LOADING PICTURE **********************/

void ShowLoadingPicture(void)
{
FSSpec	spec;

	if (gOSX)
	{
		GameScreenToBlack();
	}
	else
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:Loading1", &spec);
		DisplayPicture(&spec, true, false);
	}
}



#pragma mark -


/********************** DO TITLE SCREEN ***********************/

void DoTitleScreen(void)
{
FSSpec	spec;

			/* DO PANGEA LOGO */

#if SHAREWARE
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:PSLogo", &spec);
#else
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:PangeaLogo", &spec);
#endif


	DisplayPicture(&spec, false, false);
//	GammaFadeOut();


			/* DO TITLE SCREEN */

	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:TitleScreen", &spec))
		DoFatalAlert("\pDoTitleScreen: TitleScreen pict not found.");

	DisplayPicture(&spec, false, false);
//	GammaFadeOut();





}



#pragma mark -


/********************* DO AGE CONQUERED SCREEN **********************/

void DoAgeConqueredScreen(void)
{
			/* SETUP */

	SetupConqueredScreen();
	MakeFadeEvent(true);

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		MoveParticleGroups();
		OGL_DrawScene(gGameViewInfoPtr, DrawConqueredCallback);

		if (AreAnyNewKeysPressed())
			break;
	}


			/* CLEANUP */

	GammaFadeOut();
	FreeConqueredScreen();

}

/***************** DRAW CONQUERED CALLBACK *******************/

static void DrawConqueredCallback(OGLSetupOutputType *info)
{

			/* DRAW BACKGROUND */

	MO_DrawObject(gBackgoundPicture, info);


	DrawObjects(info);
}



/********************* SETUP CONQUERED SCREEN **********************/

static void SetupConqueredScreen(void)
{
ObjNode				*newObj;
FSSpec				spec;
OGLSetupInputType	viewDef;
static const Str255	names[] =
{
	"\p:images:Conquered:StoneAge_Conquered",
	"\p:images:Conquered:BronzeAge_Conquered",
	"\p:images:Conquered:IronAge_Conquered"
};

static Str31 age[3] = {"\pSTONE AGE", "\pBRONZE AGE", "\pIRON AGE"};


	PlaySong(SONG_THEME, true);


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.fov 				= .7;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.from[0].z		= 1200;
	viewDef.camera.from[0].y		= 0;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */


	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID,  names[gTheAge], &spec))
		DoFatalAlert("\pSetupConqueredScreen: background pict not found.");

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("\pSetupTrackSelectScreen: MO_CreateNewObjectOfType failed");


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:models:winners.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINNERS, gGameViewInfoPtr);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:wallfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);

	InitParticleSystem(gGameViewInfoPtr);


			/* CUP MODEL */

	gNewObjectDefinition.group 		= MODEL_GROUP_WINNERS;
	gNewObjectDefinition.type 		= gTheAge;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 900;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= PARTICLE_SLOT+1;			// draw cup last
	gNewObjectDefinition.moveCall 	= MoveCup;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;


			/* TEXT */


	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= .8;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .9;
	gNewObjectDefinition.slot 		= PARTICLE_SLOT-1;		// in this rare case we want to draw text before particles
	MakeFontStringObject(age[gTheAge], &gNewObjectDefinition, gGameViewInfoPtr, true);

	gNewObjectDefinition.coord.y 	= .6;
	MakeFontStringObject("\pCOMPLETE", &gNewObjectDefinition, gGameViewInfoPtr, true);

}


/********************** FREE CONQURERED ART **********************/

static void FreeConqueredScreen(void)
{
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	DeleteAllObjects();
	DisposeParticleSystem();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}


/******************** MOVE CUP *************************/

static void MoveCup(ObjNode *theNode)
{
	GetObjectInfo(theNode);
	theNode->Rot.y += gFramesPerSecondFrac;							// spin

	gDelta.y -= 400.0f * gFramesPerSecondFrac;						// drop it
	gCoord.y += gDelta.y * gFramesPerSecondFrac;
	if (gCoord.y < 20.0f)
	{
		gCoord.y = 20.0f;
		gDelta.y *= -.4f;
	}


	UpdateObject(theNode);
	BurnFire(theNode, theNode->Coord.x, theNode->Coord.y + 20.0f, theNode->Coord.z, true, PARTICLE_SObjType_Fire, 1.2);
}




#pragma mark -


/********************* DO TOURNEMENT WIN SCREEN **********************/

void DoTournamentWinScreen(void)
{
float	timer = 0;

			/* SETUP */

	SetupWinScreen();
	MakeFadeEvent(true);

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		MoveParticleGroups();
		OGL_DrawScene(gGameViewInfoPtr, DrawWinCallback);

		timer += gFramesPerSecondFrac;
		if (timer > 10.0f)
			if (AreAnyNewKeysPressed())
				break;
	}


			/* CLEANUP */

	GammaFadeOut();
	FreeWinScreen();

}


/***************** DRAW WIN CALLBACK *******************/

static void DrawWinCallback(OGLSetupOutputType *info)
{

			/* DRAW BACKGROUND */

	MO_DrawObject(gBackgoundPicture, info);
	DrawObjects(info);
}



/********************* SETUP WIN SCREEN **********************/

static void SetupWinScreen(void)
{
ObjNode				*newObj,*signObj;
FSSpec				spec;
OGLSetupInputType	viewDef;

static Str31 age[3] = {"\pSTONE AGE", "\pBRONZE AGE", "\pIRON AGE"};


	PlaySong(SONG_WIN, false);


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.fov 				= .7;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.from[0].z		= 1200;
	viewDef.camera.from[0].y		= 0;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */

	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:Conquered:GameCompleted",&spec))
		DoFatalAlert("\pSetupWinScreen: background pict not found.");

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("\pSetupTrackSelectScreen: MO_CreateNewObjectOfType failed");


	LoadASkeleton(SKELETON_TYPE_MALESTANDING, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_FEMALESTANDING, gGameViewInfoPtr);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:models:winners.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINNERS, gGameViewInfoPtr);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:wallfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);

	InitParticleSystem(gGameViewInfoPtr);


			/**************/
			/* CUP MODELS */
			/**************/

	gNewObjectDefinition.group 		= MODEL_GROUP_WINNERS;
	gNewObjectDefinition.type 		= 0;
	gNewObjectDefinition.coord.x 	= -200;
	gNewObjectDefinition.coord.y 	= 900;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveWinCups;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;
	newObj->SpecialF[3] = 4;

	gNewObjectDefinition.type 		= 1;
	gNewObjectDefinition.coord.x 	= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;
	newObj->SpecialF[3] = 6;

	gNewObjectDefinition.type 		= 2;
	gNewObjectDefinition.coord.x 	= 200;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;
	newObj->SpecialF[3] = 8;


			/* CHARACTER */

	gNewObjectDefinition.type 		= SKELETON_TYPE_MALESTANDING;
	gNewObjectDefinition.animNum	= 2;
	gNewObjectDefinition.coord.x 	= -900;
	gNewObjectDefinition.coord.z 	= -200;
	gNewObjectDefinition.coord.y 	= 120;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveWinGuy;
	gNewObjectDefinition.rot 		= -PI/2;
	gNewObjectDefinition.scale 		= 1.8;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	newObj->Skeleton->AnimSpeed 	= 1.7f;


			/* WIN SIGN */

	gNewObjectDefinition.group 		= MODEL_GROUP_WINNERS;
	gNewObjectDefinition.type 		= 3;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .5;
	signObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->ChainNode = signObj;

}


/********************** FREE WIN ART **********************/

static void FreeWinScreen(void)
{
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	DeleteAllObjects();
	DisposeParticleSystem();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();
	FreeAllSkeletonFiles(-1);
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}


/******************** MOVE CUPS *************************/

static void MoveWinCups(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->SpecialF[3] -= fps;					// see if activate
	if (theNode->SpecialF[3] > 0.0f)
		return;

	GetObjectInfo(theNode);
	theNode->Rot.y += fps;							// spin

	gDelta.y -= 400.0f * fps;						// drop it
	gCoord.y += gDelta.y * fps;
	if (gCoord.y < -280.0f)
	{
		gCoord.y = -280.0f;
		gDelta.y *= -.3f;
	}


	UpdateObject(theNode);
	BurnFire(theNode, theNode->Coord.x, theNode->Coord.y + 20.0f, theNode->Coord.z, true, PARTICLE_SObjType_Fire, .6);
}


/********************* MOVE WIN GUY *********************/

static void MoveWinGuy(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode		*signObj;
OGLMatrix4x4			m,m3,m2;

	GetObjectInfo(theNode);

	switch(theNode->Skeleton->AnimNum)
	{
			/* WALKING */

		case	2:
				gCoord.x += 300.0f * fps;
				if (gCoord.x > 40.0f)
				{
					gCoord.x = 40;
					MorphToSkeletonAnim(theNode->Skeleton, 3, 4.0);
				}
				break;


			/* TURN/SHOW */

		case	3:
				break;
	}

	UpdateObject(theNode);


		/***************/
		/* UPDATE SIGN */
		/***************/

	signObj = theNode->ChainNode;

	OGLMatrix4x4_SetIdentity(&m3);
	m3.value[M03] = -90;				// set coords
	m3.value[M13] = 0;
	m3.value[M23] = -20;
	m3.value[M00] = 					// set scale
	m3.value[M11] =
	m3.value[M22] = .45f;

	OGLMatrix4x4_SetRotate_X(&m2, -PI/2);
	OGLMatrix4x4_Multiply(&m3, &m2, &m3);

	FindJointFullMatrix(theNode, 6, &m);
	OGLMatrix4x4_Multiply(&m3, &m, &signObj->BaseTransformMatrix);
	SetObjectTransformMatrix(signObj);
}


#pragma mark -

/********************** DO CREDITS SCREEN ***********************/

void DoCreditsScreen(void)
{
float	timer = 63.0f;

			/* SETUP */

	SetupCreditsScreen();
	MakeFadeEvent(true);

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		MoveParticleGroups();
		OGL_DrawScene(gGameViewInfoPtr, DrawCreditsCallback);

		if (AreAnyNewKeysPressed())
			break;

		if ((timer -= gFramesPerSecondFrac) < 0.0f)
			break;

	}


			/* CLEANUP */

	FreeCreditsScreen();


}



/***************** DRAW CREDITS CALLBACK *******************/

static void DrawCreditsCallback(OGLSetupOutputType *info)
{

			/* DRAW BACKGROUND */

	MO_DrawObject(gBackgoundPicture, info);
	DrawObjects(info);
}



/********************* SETUP CREDITS SCREEN **********************/


typedef struct
{
	signed char	color;
	signed char	size;
	Str32	text;
}CreditLine;

static void SetupCreditsScreen(void)
{
int					i;
ObjNode				*newObj;
FSSpec				spec;
OGLSetupInputType	viewDef;
float				y,scale;

static CreditLine lines[] =
{
	0,2,"\pPROGRAMMING AND CONCEPT",
	1,0,"\pBRIAN GREENSTONE",
#if OEM
	2,1,"\pBRIAN@BRIANGREENSTONE.COM",
#else
	2,1,"\pBRIAN@BRIANGREENSTONE.COM",
	2,1,"\pWWW.BRIANGREENSTONE.COM",
#endif
	1,0,"\p ",
	1,0,"\p ",

	0,2,"\pART",
	1,0,"\pJOSH MARUSKA",
	2,1,"\pMARUSKAJ@MAC.COM",
	1,0,"\p ",
	1,0,"\pMARCUS CONGE",
	2,1,"\pMCONGE@DIGITALMANIPULATION.COM",
	2,1,"\pWWW.DIGITALMANIPULATION.COM",
	1,0,"\p ",
	1,0,"\pDANIEL MARCOUX",
	2,1,"\pDAN@BEENOX.COM",
	1,0,"\p ",
	1,0,"\pCARL LOISELLE",
	2,1,"\pCLOISELLE@BEENOX.COM",
	1,0,"\p ",
	1,0,"\pBRIAN GREENSTONE",
	2,1,"\pBRIAN@BRIANGREENSTONE.COM",
	1,0,"\p ",
	1,0,"\p ",

	0,2,"\pMUSIC",
	1,0,"\pMIKE BECKETT",
	2,1,"\pINFO@NUCLEARKANGAROOMUSIC.COM",
	2,1,"\pWWW.NUCLEARKANGAROOMUSIC.COM",
	1,0,"\p ",
	1,0,"\p ",

	0,2,"\pADDITIONAL WORK",
	1,0,"\pDEE BROWN",
	2,1,"\pDEEBROWN@BEENOX.COM",
	2,1,"\pWWW.BEENOX.COM",
	1,0,"\p ",
	1,0,"\pPASCAL BRULOTTE",
	1,0,"\p ",
	1,0,"\p ",

	0,2,"\pSPECIAL THANKS",
	1,0,"\pTUNCER DENIZ",
	1,0,"\pZOE BENTLEY",
	1,0,"\pCHRIS BENTLEY",
	1,0,"\pGEOFF STAHL",
	1,0,"\pJOHN STAUFFER",
	1,0,"\pMIGUEL CORNEJO",
	1,0,"\pFELIX SEGEBRECHT",
	1,0,"\p ",
	1,0,"\p ",

	1,1,"\pCOPYRIGHT 2000",
	1,1,"\pPANGEA SOFTWARE INC.",
	1,1,"\pALL RIGHTS RESERVED",
	1,1,"\pWWW.PANGEASOFT.NET",
	-1,0,"\p ",
};


static const OGLColorRGBA	colors[] =
{
	.2,.8,.2,1,
	1,1,1,1,
	.8,.8,1,1
};

static const float sizes[] =
{
	.4,
	.3,
	.5,
};





			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.fov 				= .7;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.from[0].z		= 1200;
	viewDef.camera.from[0].y		= 0;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */


	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID,  	"\p:images:Credits",&spec))
		DoFatalAlert("\pSetupCreditsScreen: background pict not found.");

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("\pSetupCreditsScreen: MO_CreateNewObjectOfType failed");


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:rockfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);


			/*****************/
			/* BUILD CREDITS */
			/*****************/

	y = -1.1;
	i = 0;
	do
	{
		gNewObjectDefinition.coord.x 	= 0;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= MoveCredit;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = scale = sizes[lines[i].size];
		gNewObjectDefinition.slot 		= PARTICLE_SLOT-1;		// in this rare case we want to draw text before particles
		newObj = MakeFontStringObject(lines[i].text, &gNewObjectDefinition, gGameViewInfoPtr, true);


		newObj->ColorFilter = colors[lines[i].color];

		y -= scale * .27f;

		i++;
	}while(lines[i].color != -1);
}


/******************** MOVE CREDIT **************************/

static void MoveCredit(ObjNode *theNode)
{
short	i;
MOSpriteObject		*spriteMO;

	for (i = 0; i < theNode->NumStringSprites; i++)
	{
		spriteMO = theNode->StringCharacters[i];
		spriteMO->objectData.coord.y += .12f * gFramesPerSecondFrac;
	}
}







/********************** FREE CREDITS ART **********************/

static void FreeCreditsScreen(void)
{
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}


#pragma mark -


/************** DO HELP SCREEN *********************/

void DoHelpScreen(void)
{
FSSpec	spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:Help", &spec);

	DisplayPicture(&spec, false, false);

}


#pragma mark -

#if DEMO

/****************** DO DEMO EXPIRED SCREEN **************************/

void DoDemoExpiredScreen(void)
{
FSSpec	spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:DemoExpired", &spec);
	DisplayPicture(&spec, false, false);
	CleanQuit();
}


/*************** SHOW DEMO QUIT SCREEN **********************/

void ShowDemoQuitScreen(void)
{
FSSpec	spec;

	SaveDemoTimer();						// make sure to save this before we bail

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:DemoQuit", &spec);
	DisplayPicture(&spec, false, false);

}


#endif


#if SHAREWARE


/***************** DO DEMO EXIT SCREEN ********************/

void DoSharewareExitScreen(void)
{
FSSpec	spec;

	SaveDemoTimer();						// make sure to save this before we bail

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:swexit", &spec);
	DisplayPicture(&spec, false, false);
}
#endif









