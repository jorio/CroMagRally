/****************************/
/*   	MISCSCREENS.C	    */
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "miscscreens.h"


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

void DisplayPicture(const char* picturePath, float timeout)
{
OGLSetupInputType	viewDef;
ObjNode				*keyText = NULL;
bool				showAndBail = timeout <= 0;
bool				doKeyText = timeout > 9;
float				keyTextFadeIn = -2.0f;		// fade in after a small delay


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;
	viewDef.view.pillarbox4x3		= true;
	viewDef.view.fontName			= "rockfont";

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


			/* CREATE BACKGROUND OBJECT */

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (uintptr_t) gGameViewInfoPtr, (void*) picturePath);
	if (!gBackgoundPicture)
		DoFatalAlert("DisplayPicture: MO_CreateNewObjectOfType failed");



			/* CREATE TEXT */

	if (doKeyText)
	{
		NewObjectDefinitionType def =
		{
			.coord		= {0, 225, 0},
			.scale		= .3,
			.slot		= SPRITE_SLOT,
		};

		keyText = TextMesh_New(Localize(STR_PRESS_ANY_KEY), kTextMeshAlignCenter, &def);
		keyText->ColorFilter.a = 0;
	}




		/***********/
		/* SHOW IT */
		/***********/

	if (showAndBail)
	{
		int	i;

		for (i = 0; i < 10; i++)
			OGL_DrawScene(gGameViewInfoPtr, DisplayPicture_Draw);

		// TODO: Fade in, I guess?
	}
	else
	{
		MakeFadeEvent(true);
		ReadKeyboard();
		CalcFramesPerSecond();


					/* MAIN LOOP */

		while (timeout > 0)
		{
			CalcFramesPerSecond();
			MoveObjects();
			OGL_DrawScene(gGameViewInfoPtr, DisplayPicture_Draw);

			ReadKeyboard();
			if (AreAnyNewKeysPressed())
				break;

			timeout -= gFramesPerSecondFrac;

			
			if (keyText)
			{
				keyTextFadeIn += gFramesPerSecondFrac;
				keyText->ColorFilter.a = GAME_CLAMP(keyTextFadeIn * 3, 0, 1);
			}
		}

				/* FADE OUT */

		OGL_FadeOutScene(gGameViewInfoPtr, DisplayPicture_Draw, NULL);
	}

			/* CLEANUP */

	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();


	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
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
static const char*	names[NUM_AGES] =
{
	":images:Ages:StoneAgeIntro.jpg",
	":images:Ages:BronzeAgeIntro.jpg",
	":images:Ages:IronAgeIntro.jpg"
};

	DisplayPicture(names[age], 20);
}


/********************* SHOW LOADING PICTURE **********************/

void ShowLoadingPicture(void)
{
#if ENABLE_LOADING_SCREEN
FSSpec	spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:Loading1.jpg", &spec);
	DisplayPicture(&spec, -1);
#endif
}



#pragma mark -


/********************** DO TITLE SCREEN ***********************/

void DoTitleScreen(void)
{
	DisplayPicture(":images:PangeaLogo.jpg", 4.0f);
	DisplayPicture(":images:TitleScreen.jpg", 5.0f);
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

			/* FADE OUT */

	OGL_FadeOutScene(gGameViewInfoPtr, DrawConqueredCallback, MoveObjects);


			/* CLEANUP */

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
static const char* names[] =
{
	":images:Conquered:StoneAgeConquered.png",
	":images:Conquered:BronzeAgeConquered.png",
	":images:Conquered:IronAgeConquered.png"
};


	PlaySong(SONG_THEME, true);


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= .7;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.from[0].z		= 1200;
	viewDef.camera.from[0].y		= 0;
	viewDef.view.pillarbox4x3		= true;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */


	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (uintptr_t) gGameViewInfoPtr, (void*) names[gTheAge]);
	if (!gBackgoundPicture)
		DoFatalAlert("SetupTrackSelectScreen: MO_CreateNewObjectOfType failed");


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:winners.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINNERS, gGameViewInfoPtr);


			/* LOAD SPRITES */

	InitParticleSystem(gGameViewInfoPtr);


			/* CUP MODEL */

	{
		NewObjectDefinitionType def =
		{
			.group		= MODEL_GROUP_WINNERS,
			.type		= gTheAge,
			.coord		= {0, 900, 0},
			.slot		= PARTICLE_SLOT+1,			// draw cup last
			.moveCall	= MoveCup,
			.scale		= 1,
		};
		newObj = MakeNewDisplayGroupObject(&def);

		newObj->SmokeParticleGroup = -1;
		newObj->SmokeParticleMagic = 0;
	}

			/* TEXT */

	{
		NewObjectDefinitionType def =
		{
			.coord		= {0, .8, 0},
			.scale		= .9,
			.slot		= PARTICLE_SLOT-1,		// in this rare case we want to draw text before particles
		};
		TextMesh_New(Localize(STR_STONE_AGE + gTheAge), 0, &def);

		def.coord.y = .6;
		TextMesh_New(Localize(STR_AGE_COMPLETE), 0, &def);
	}
}


/********************** FREE CONQURERED ART **********************/

static void FreeConqueredScreen(void)
{
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
		if (timer > 10.0f && AreAnyNewKeysPressed())
		{
			break;
		}
	}


			/* CLEANUP */

	OGL_FadeOutScene(gGameViewInfoPtr, DrawWinCallback, MoveObjects);
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

	PlaySong(SONG_WIN, false);


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= .7;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.from[0].z		= 1200;
	viewDef.camera.from[0].y		= 0;
	viewDef.view.pillarbox4x3		= true;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (uintptr_t) gGameViewInfoPtr, ":images:Conquered:GameCompleted.png");
	if (!gBackgoundPicture)
		DoFatalAlert("SetupTrackSelectScreen: MO_CreateNewObjectOfType failed");


	LoadASkeleton(SKELETON_TYPE_MALESTANDING, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_FEMALESTANDING, gGameViewInfoPtr);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:winners.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINNERS, gGameViewInfoPtr);


			/* LOAD SPRITES */

	InitParticleSystem(gGameViewInfoPtr);


			/**************/
			/* CUP MODELS */
			/**************/

	NewObjectDefinitionType cupDef =
	{
		.group		= MODEL_GROUP_WINNERS,
		.type		= 0,
		.coord		= {-200,900,0},
		.slot 		= 100,
		.moveCall 	= MoveWinCups,
		.scale		= .5
	};
	newObj = MakeNewDisplayGroupObject(&cupDef);
	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;
	newObj->SpecialF[3] = 4;

	cupDef.type			= 1,
	cupDef.coord.x		= 0;
	newObj = MakeNewDisplayGroupObject(&cupDef);
	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;
	newObj->SpecialF[3] = 6;

	cupDef.type			= 2,
	cupDef.coord.x		= 200;
	newObj = MakeNewDisplayGroupObject(&cupDef);
	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;
	newObj->SpecialF[3] = 8;


			/* CHARACTER */

	{
		NewObjectDefinitionType def =
		{
			.type		= SKELETON_TYPE_MALESTANDING,
			.animNum	= 2,
			.coord		= {-900, 120, -200},
			.flags		= 0,
			.slot		= 100,
			.moveCall	= MoveWinGuy,
			.rot		= -PI/2,
			.scale		= 1.8
		};
		newObj = MakeNewSkeletonObject(&def);
		newObj->Skeleton->AnimSpeed 	= 1.7f;
	}

			/* WIN SIGN */

	{
		NewObjectDefinitionType def =
		{
			.group		= MODEL_GROUP_WINNERS,
			.type		= 3,
			.coord		= {0,0,0},
			.flags		= 0,
			.slot		= 100,
			.moveCall	= nil,
			.scale		= .5,
		};
		signObj = MakeNewDisplayGroupObject(&def);
		newObj->ChainNode = signObj;
	}
}


/********************** FREE WIN ART **********************/

static void FreeWinScreen(void)
{
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

	while (timer > 0)
	{
			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		MoveParticleGroups();
		OGL_DrawScene(gGameViewInfoPtr, DrawCreditsCallback);

		if (AreAnyNewKeysPressed())
			break;

		timer -= gFramesPerSecondFrac;
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
	const char*	text;
}CreditLine;

static void SetupCreditsScreen(void)
{
ObjNode				*newObj;
OGLSetupInputType	viewDef;

static CreditLine lines[] =
{
	{0,2,"PROGRAMMING AND CONCEPT"},
	{1,0,"BRIAN GREENSTONE"},
	{2,1,"BRIAN@BRIANGREENSTONE.COM"},
	{2,1,"WWW.BRIANGREENSTONE.COM"},
	{1,0," "},
	{1,0," "},
	{0,2,"ART"},
	{1,0,"JOSH MARUSKA"},
	{2,1,"MARUSKAJ@MAC.COM"},
	{1,0," "},
	{1,0,"MARCUS CONGE"},
	{2,1,"MCONGE@DIGITALMANIPULATION.COM"},
	{2,1,"WWW.DIGITALMANIPULATION.COM"},
	{1,0," "},
	{1,0,"DANIEL MARCOUX"},
	{2,1,"DAN@BEENOX.COM"},
	{1,0," "},
	{1,0,"CARL LOISELLE"},
	{2,1,"CLOISELLE@BEENOX.COM"},
	{1,0," "},
	{1,0,"BRIAN GREENSTONE"},
	{2,1,"BRIAN@BRIANGREENSTONE.COM"},
	{1,0," "},
	{1,0," "},
	{0,2,"MUSIC"},
	{1,0,"MIKE BECKETT"},
	{2,1,"INFO@NUCLEARKANGAROOMUSIC.COM"},
	{2,1,"WWW.NUCLEARKANGAROOMUSIC.COM"},
	{1,0," "},
	{1,0," "},
	{0,2,"ADDITIONAL WORK"},
	{1,0,"DEE BROWN"},
	{2,1,"DEEBROWN@BEENOX.COM"},
	{2,1,"WWW.BEENOX.COM"},
	{1,0," "},
	{1,0,"PASCAL BRULOTTE"},
	{1,0," "},
	{1,0," "},
	{0,2,"SPECIAL THANKS"},
	{1,0,"TUNCER DENIZ"},
	{1,0,"ZOE BENTLEY"},
	{1,0,"CHRIS BENTLEY"},
	{1,0,"GEOFF STAHL"},
	{1,0,"JOHN STAUFFER"},
	{1,0,"MIGUEL CORNEJO"},
	{1,0,"FELIX SEGEBRECHT"},
	{1,0," "},
	{1,0," "},
	{1,1,"COPYRIGHT 2000"},
	{1,1,"PANGEA SOFTWARE INC."},
	{1,1,"ALL RIGHTS RESERVED"},
	{1,1,"WWW.PANGEASOFT.NET"},
	{-1,0," "},
};


static const OGLColorRGBA	colors[] =
{
	{.2,.8,.2,1},
	{1,1,1,1},
	{.8,.8,1,1},
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

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= .7;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.from[0].z		= 1200;
	viewDef.camera.from[0].y		= 0;
	viewDef.view.clearColor 		= (OGLColorRGBA) { .49f, .39f, .29f, 1 };
	viewDef.view.pillarbox4x3		= true;
	viewDef.view.fontName			= "rockfont";

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */


	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (uintptr_t) gGameViewInfoPtr, ":images:Credits.jpg");
	if (!gBackgoundPicture)
		DoFatalAlert("SetupCreditsScreen: MO_CreateNewObjectOfType failed");



			/*****************/
			/* BUILD CREDITS */
			/*****************/

	float y = -1.1;
	for (int i = 0; lines[i].color != -1; i++)
	{
		NewObjectDefinitionType def =
		{
			.coord		= {0,y,0},
			.moveCall	= MoveCredit,
			.scale		= sizes[lines[i].size],
			.slot 		= PARTICLE_SLOT-1,		// in this rare case we want to draw text before particles
		};
		newObj = TextMesh_New(lines[i].text, kTextMeshAlignCenter, &def);


		newObj->ColorFilter = colors[lines[i].color];

		y -= def.scale * .27f;
	}
}


/******************** MOVE CREDIT **************************/

static void MoveCredit(ObjNode *theNode)
{
#if 0
short	i;
MOSpriteObject		*spriteMO;

	for (i = 0; i < theNode->NumStringSprites; i++)
	{
		spriteMO = theNode->StringCharacters[i];
		spriteMO->objectData.coord.y += .12f * gFramesPerSecondFrac;
	}
#endif
	theNode->Coord.y += .12f * gFramesPerSecondFrac;
}







/********************** FREE CREDITS ART **********************/

static void FreeCreditsScreen(void)
{
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
	DisplayPicture(":images:Help.jpg", 20);
}

