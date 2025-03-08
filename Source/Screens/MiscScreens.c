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

static void SetupConqueredScreen(void);
static void FreeConqueredScreen(void);
static void MoveCup(ObjNode *theNode);

static void SetupWinScreen(void);
static void MoveWinCups(ObjNode *theNode);
static void FreeWinScreen(void);
static void MoveWinGuy(ObjNode *theNode);

static void SetupCreditsScreen(void);
static void MoveCredit(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/





#pragma mark -


/********************** DISPLAY PICTURE **************************/
//
// Displays a picture using OpenGL until the user clicks the mouse or presses any key.
// If showAndBail == true, then show it and bail out
//

void DisplayPicture(const char* picturePath, float timeout)
{
OGLSetupInputType	viewDef;
bool				showAndBail = timeout <= 0;
bool				doKeyText = timeout > 9;


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;
	viewDef.view.pillarboxRatio		= PILLARBOX_RATIO_4_3;
	viewDef.view.fontName			= "rockfont";

	OGL_SetupGameView(&viewDef);


			/* CREATE BACKGROUND OBJECT */

	MakeBackgroundPictureObject(picturePath);



			/* CREATE TEXT */

	if (doKeyText)
	{
		NewObjectDefinitionType def =
		{
			.coord		= {0, 225, 0},
			.scale		= .3,
			.slot		= SPRITE_SLOT,
		};

		int strKey = gUserPrefersGamepad ? STR_PRESS_START_TO_CONTINUE : STR_PRESS_SPACE_TO_CONTINUE;
		ObjNode* keyText = TextMesh_New(Localize(strKey), kTextMeshAlignCenter, &def);
		MakeTwitch(keyText, kTwitchPreset_PressKeyPrompt);
	}




		/***********/
		/* SHOW IT */
		/***********/

	if (showAndBail)
	{
		int	i;

		for (i = 0; i < 10; i++)
			OGL_DrawScene(DrawObjects);

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
			OGL_DrawScene(DrawObjects);

			ReadKeyboard();
			if (UserWantsOut())
				break;

			timeout -= gFramesPerSecondFrac;
		}

				/* FADE OUT */

		OGL_FadeOutScene(DrawObjects, NULL);
	}

			/* CLEANUP */

	DeleteAllObjects();

	OGL_DisposeGameView();
}

#pragma mark -


/********************* SHOW AGE PICTURE **********************/

void ShowAgePicture(int age)
{
static const char*	names[NUM_AGES] =
{
	":Images:Ages:StoneAgeIntro.jpg",
	":Images:Ages:BronzeAgeIntro.jpg",
	":Images:Ages:IronAgeIntro.jpg"
};

	DisplayPicture(names[age], 20);
}


/********************* SHOW LOADING PICTURE **********************/

void ShowLoadingPicture(void)
{
#if ENABLE_LOADING_SCREEN
FSSpec	spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:Loading1.jpg", &spec);
	DisplayPicture(&spec, -1);
#endif
}



#pragma mark -


/********************** DO TITLE SCREEN ***********************/

void DoTitleScreen(void)
{
	DisplayPicture(":Images:PangeaLogo.jpg", 4.0f);
	DisplayPicture(":Images:TitleScreen.jpg", 5.0f);
}


#pragma mark -


/******* DO PROGRAM WARM-UP SCREEN AS WE PRELOAD ASSETS **********/

void DoWarmUpScreen(void)
{
	OGLSetupInputType	viewDef;

			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef);
	viewDef.view.pillarboxRatio		= PILLARBOX_RATIO_FULLSCREEN;
	viewDef.view.fontName			= "rockfont";

	OGL_SetupGameView(&viewDef);

			/* SHOW IT */

	for (int i = 0; i < 8; i++)
	{
		OGL_DrawScene(DrawObjects);
		DoSDLMaintenance();
	}

			/* CLEANUP */

	DeleteAllObjects();

	OGL_DisposeGameView();
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
		OGL_DrawScene(DrawObjects);

		if (UserWantsOut())
			break;
	}

			/* FADE OUT */

	OGL_FadeOutScene(DrawObjects, MoveObjects);


			/* CLEANUP */

	FreeConqueredScreen();
}



/********************* SETUP CONQUERED SCREEN **********************/

static void SetupConqueredScreen(void)
{
ObjNode				*newObj;
FSSpec				spec;
OGLSetupInputType	viewDef;
static const char* names[] =
{
	":Images:Conquered:StoneAgeConquered.png",
	":Images:Conquered:BronzeAgeConquered.png",
	":Images:Conquered:IronAgeConquered.png"
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
	viewDef.view.pillarboxRatio	= PILLARBOX_RATIO_4_3;

	OGL_SetupGameView(&viewDef);


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */


	MakeBackgroundPictureObject(names[gTheAge]);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:winners.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINNERS);


			/* LOAD SPRITES */

	InitParticleSystem();


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
			.flags		= STATUS_BIT_KEEPBACKFACES,	// normals are inside out on black part of cup handle
		};
		newObj = MakeNewDisplayGroupObject(&def);

		newObj->SmokeParticleGroup = -1;
		newObj->SmokeParticleMagic = 0;
	}

			/* TEXT */

	{
		NewObjectDefinitionType def =
		{
			.coord		= {0, -192, 0},
			.scale		= .9,
			.slot		= PARTICLE_SLOT-1,		// in this rare case we want to draw text before particles
		};
		TextMesh_New(Localize(STR_STONE_AGE + gTheAge), 0, &def);

		def.coord.y = -144;
		TextMesh_New(Localize(STR_AGE_COMPLETE), 0, &def);
	}
}


/********************** FREE CONQURERED ART **********************/

static void FreeConqueredScreen(void)
{
	DeleteAllObjects();
	DeleteAllParticleGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeGameView();
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




/***************** MOVE TOURNAMENT TOTAL TIME **********************/

static void MoveTotalTime(ObjNode* theNode)
{
	theNode->ColorFilter.a = GAME_MIN(theNode->ColorFilter.a + gFramesPerSecondFrac, 1);
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
		OGL_DrawScene(DrawObjects);

		timer += gFramesPerSecondFrac;
		if (timer > 13.0f && UserWantsOut())
		{
			break;
		}
	}


			/* CLEANUP */

	OGL_FadeOutScene(DrawObjects, MoveObjects);
	FreeWinScreen();
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
	viewDef.view.pillarboxRatio	= PILLARBOX_RATIO_4_3;

	OGL_SetupGameView(&viewDef);


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */

	MakeBackgroundPictureObject(":Images:Conquered:GameCompleted.png");


	LoadASkeleton(SKELETON_TYPE_MALESTANDING);
	LoadASkeleton(SKELETON_TYPE_FEMALESTANDING);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:winners.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINNERS);


			/* LOAD SPRITES */

	InitParticleSystem();


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
		.scale		= .5,
		.flags		= STATUS_BIT_KEEPBACKFACES,		// normals are inside out on black part of cup handle
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

			/* TOTAL TIME */

	{
		NewObjectDefinitionType def =
		{
			.coord = {0,208,0},
			.scale = .4f,
			.slot = SPRITE_SLOT,
			.moveCall = MoveTotalTime,
		};

		char totalTimeStr[64];
		SDL_snprintf(totalTimeStr, sizeof(totalTimeStr), "%s: %s", Localize(STR_TOTAL_TIME), FormatRaceTime(GetTotalTournamentTime()));
		ObjNode* totalTimeObj = TextMesh_New(totalTimeStr, 0, &def);
		totalTimeObj->ColorFilter.a = -11;
	}
}


/********************** FREE WIN ART **********************/

static void FreeWinScreen(void)
{
	DeleteAllObjects();
	DeleteAllParticleGroups();
	FreeAllSkeletonFiles(-1);
	DisposeAllBG3DContainers();
	OGL_DisposeGameView();
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
	BurnFire(theNode, theNode->Coord.x, theNode->Coord.y + 20.0f, theNode->Coord.z, false, PARTICLE_SObjType_Fire, .6);
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
float	timer = 51.5f;

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
		OGL_DrawScene(DrawObjects);

		if (UserWantsOut())
			break;

		timer -= gFramesPerSecondFrac;
	}

	gGameView->fadeOutDuration = .5f;
	OGL_FadeOutScene(DrawObjects, MoveObjects);


			/* CLEANUP */

	DeleteAllObjects();
	DisposeAllBG3DContainers();
	OGL_DisposeGameView();
}



/********************* SETUP CREDITS SCREEN **********************/


typedef struct
{
	signed char	color;
	signed char	size;
	int			loca;
	const char*	text;
}CreditLine;

static void SetupCreditsScreen(void)
{
ObjNode				*newObj;
OGLSetupInputType	viewDef;

static const CreditLine lines[] =
{
	{0, 2, .loca=STR_CREDITS_PROGRAMMING},
	{1, 4, .text=""},
	{1, 0, .text="BRIAN GREENSTONE"},
	{1, 3, .text=""},
	{0, 2, .loca=STR_CREDITS_ART},
	{1, 4, .text=""},
	{1, 0, .text="MARCUS CONGE"},
	{1, 4, .text=""},
	{1, 0, .text="DEE BROWN"},
	{1, 4, .text=""},
	{1, 0, .text="JOSHUA MARUSKA"},
	{1, 4, .text=""},
	{1, 0, .text="DANIEL MARCOUX"},
	{1, 4, .text=""},
	{1, 0, .text="CARL LOISELLE"},
	{1, 3, .text=""},
	{0, 2, .loca=STR_CREDITS_MUSIC},
	{1, 4, .text=""},
	{1, 0, .text="MIKE BECKETT"},
	{0, 3, .text=""},
	{0, 2, .loca=STR_CREDITS_PORT},
	{1, 4, .text=""},
	{1, 0, .text="ILIYAS JORIO"},
	{0, 3, .text=""},
	{0, 2, .loca=STR_CREDITS_SPECIAL_THANKS},
	{1, 4, .text=""},
	{1, 0, .text="PASCAL BRULOTTE"},
	{1, 0, .text="TUNCER DENIZ"},
	{1, 0, .text="ZOE BENTLEY"},
	{1, 0, .text="CHRIS BENTLEY"},
	{1, 0, .text="GEOFF STAHL"},
	{1, 0, .text="JOHN STAUFFER"},
	{1, 0, .text="MIGUEL CORNEJO"},
	{1, 0, .text="FELIX SEGEBRECHT"},
	{1, 3, .text=""},
	{1, 4, .text=""},
	{1, 1, .text="[C] 2000 PANGEA SOFTWARE, INC."},
	{1, 1, .loca=STR_CREDITS_ALL_RIGHTS_RESERVED},
	{1, 4, .text=""},
	{1, 1, .text="''CRO-MAG RALLY''"},
	{1, 1, .text="IS A REGISTERED TRADEMARK"},
	{1, 1, .text="OF PANGEA SOFTWARE, INC."},
	{0, 3, .text=""},
	{2, 0, .text="WWW.PANGEASOFT.NET"},
	{0, 2, .text=""},
	{2, 0, .text="GITHUB.COM/JORIO"},
	{-1,0, .text=""},
};


static const OGLColorRGBA	colors[] =
{
	{.2,.8,.2,1},
	{1,1,1,1},
	{.8,.8,1,1},
};

static const float sizes[] =
{
	.3,		// names
	.2,		// small print
	.4,		// headings
	1.5,	// large spacing
	.2,		// small spacing
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
	viewDef.view.clearColor 		= (OGLColorRGBA) { 0, 0, 0, 1 };
	viewDef.view.pillarboxRatio		= PILLARBOX_RATIO_16_9;
	viewDef.view.fontName			= "wallfont";

	OGL_SetupGameView(&viewDef);

	gGameView->fadePillarbox = true;


				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */


	ObjNode* artwork = MakeBackgroundPictureObject(":Images:Credits.jpg");

	artwork->Scale.x = (4.0/3.0) / viewDef.view.pillarboxRatio;  // hack - preserve aspect ratio of 4:3 picture in 16:9 viewport
	artwork->Coord.x = -.25f;
	UpdateObjectTransforms(artwork);



			/*****************/
			/* BUILD CREDITS */
			/*****************/

	float y = 1.2 * 240.0f;
	for (int i = 0; lines[i].color != -1; i++)
	{
		NewObjectDefinitionType def =
		{
			.coord		= {310, y, 0},
			.moveCall	= MoveCredit,
			.scale		= .8 * sizes[lines[i].size],
			.slot 		= PARTICLE_SLOT-1,		// in this rare case we want to draw text before particles
		};

		const char* text;
		if (lines[i].loca != STR_NULL)
		{
			text = Localize(lines[i].loca);
		}
		else
		{
			text = lines[i].text;
		}

		if (text && text[0] != '\0')
		{
			newObj = TextMesh_New(text, kTextMeshAlignCenter, &def);
			newObj->ColorFilter = colors[lines[i].color];
		}

		y += def.scale * .275f * 240.0f;
	}



	NewObjectDefinitionType def =
	{
		.slot = PARTICLE_SLOT-2,
		.scale = 1,
		.group = SPRITE_GROUP_INFOBAR,
		.type = INFOBAR_SObjType_OverlayBackgroundH,
		.coord = {255,0,0},
	};

	ObjNode* pane = MakeSpriteObject(&def);
	pane->ColorFilter = (OGLColorRGBA) {0, 0, 0, 1};
	pane->Scale.x = 1.0f;
	pane->Scale.y = 250;
	UpdateObjectTransforms(pane);
}


/******************** MOVE CREDIT **************************/

static void MoveCredit(ObjNode *theNode)
{
	theNode->Coord.y -= .13f * gFramesPerSecondFrac * 240.0f;

	bool withinBounds = theNode->Coord.y > -280 && theNode->Coord.y < 280;

	if (SetObjectVisible(theNode, withinBounds))
		UpdateObjectTransforms(theNode);
}


#pragma mark -


/************** DO HELP SCREEN *********************/

void DoHelpScreen(void)
{
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .5, .5, .5, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLVector3D			fillDirection1 = { .9, -.3, -1 };

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= .3;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.camera.from[0].z		= 700;

	viewDef.view.clearColor 		= (OGLColorRGBA) { 0, 0, 0, 1 };
	viewDef.styles.useFog			= false;
	viewDef.view.pillarboxRatio		= PILLARBOX_RATIO_4_3;

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 1;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0] 	= fillColor1;

	viewDef.view.fontName			= "rockfont";

	OGL_SetupGameView(&viewDef);


	MakeFadeEvent(true);



				/************/
				/* LOAD ART */
				/************/

	MakeScrollingBackgroundPattern();

	int loadTexFlagsBackup = gLoadTextureFlags;
	gLoadTextureFlags |= kLoadTextureFlags_NearestNeighbor;
	LoadSpriteGroup(SPRITE_GROUP_MAINMENU, "qrcodes", 0);
	gLoadTextureFlags = loadTexFlagsBackup;


			/*****************/
			/* BUILD OBJECTS */
			/*****************/

	float startY = -92;

	NewObjectDefinitionType textDef =
	{
			.scale = 0.32f,
			.coord = {-130, startY, 0},
			.slot = SPRITE_SLOT,
	};

	TextMesh_New("PANGEA'S CRO-MAG HOMEPAGE\nWITH HINTS, TIPS AND CHEATS:", kTextMeshAlignBottom | kTextMeshAlignLeft, &textDef);

	textDef.coord.y += 0.4 * 64;
	ObjNode* url1 = TextMesh_New("PANGEASOFT.NET/CROMAG", kTextMeshAlignLeft, &textDef);

	textDef.coord.y += 0.4 * 64 * 6;
	TextMesh_New("HOMEPAGE OF THE MODERN VERSION\nOF CRO-MAG RALLY:", kTextMeshAlignBottom | kTextMeshAlignLeft, &textDef);

	textDef.coord.y += 0.4 * 64;
	ObjNode* url2 = TextMesh_New("JORIO.ITCH.IO/CROMAGRALLY", kTextMeshAlignLeft, &textDef);

	url1->ColorFilter = url2->ColorFilter = (OGLColorRGBA) { .8, .2, .2, 1 };

	textDef.coord.x = 0;
	textDef.coord.y = 220;
	textDef.scale = 0.27f;
	ObjNode* pressEsc = TextMesh_New(Localize(STR_PRESS_ESC_TO_GO_BACK), 0, &textDef);
	pressEsc->ColorFilter = (OGLColorRGBA) {.5, .5, .5, 1};
	MakeTwitch(pressEsc, kTwitchPreset_PressKeyPrompt);

	NewObjectDefinitionType qrCodeDef =
	{
		.scale = 3.0f,
		.coord = {-200, startY-3, 0},
		.group = SPRITE_GROUP_MAINMENU,
		.type = 1,
		.slot = SPRITE_SLOT,
	};
	MakeSpriteObject(&qrCodeDef);

	qrCodeDef.coord.y += 0.4*64*7;
	qrCodeDef.type = 2;
	MakeSpriteObject(&qrCodeDef);


	char systemInfo[512];
	SDL_snprintf(systemInfo, sizeof(systemInfo), "CRO-MAG RALLY V%s / SDL %s [%s]\nOPENGL %s [%s]",
				 GAME_VERSION,
				 SDL_GetRevision(),
				 SDL_GetCurrentVideoDriver(),
				 (const char*)glGetString(GL_VERSION),
				 (const char*)glGetString(GL_RENDERER));
	textDef.coord.y = 200;
	textDef.scale = .15f;
	ObjNode* techInfo = TextMesh_New(systemInfo, kTextMeshAllCaps, &textDef);
	techInfo->ColorFilter = (OGLColorRGBA) {.25f, .25f, .25f, 1};



			/*************/
			/* MAIN LOOP */
			/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while (!UserWantsOut())
	{
		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(DrawObjects);
	}


			/***********/
			/* CLEANUP */
			/***********/

	OGL_FadeOutScene(DrawObjects, MoveObjects);

	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllBG3DContainers();
	DisposeSpriteGroup(SPRITE_GROUP_MAINMENU);
	OGL_DisposeGameView();

}

