/****************************/
/*   	FAILED.C			*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void BuildFailedMenu(const char* s);
static Boolean NavigateFailedMenu(void);
static void FreeFailedMenu(void);
static void MoveFailedObject(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	FAILED_ICON_SCALE	.4f

#define	LINE_SPACING	.12f



/*********************/
/*    VARIABLES      */
/*********************/

static short	gFailedMenuSelection;

static ObjNode	*gFailedIcons[2];

static Boolean	gTryAgain;

static const OGLColorRGBA gFailedMenuHiliteColor = {.3,.5,.2,1};

/********************** DO FAILED **************************/
//
// Called at end of race when draw context is still visible and active
//
// return true if try again
//

Boolean DoFailedMenu(const char* headerString)
{
	BuildFailedMenu(headerString);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();
	gNoCarControls = true;									// nobody has control during this

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (NavigateFailedMenu())
			break;

			/* MOVE STUFF */

		MoveEverything();


			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		DoPlayerTerrainUpdate();							// need to call this to keep supertiles active
		OGL_DrawScene(gGameViewInfoPtr, DrawTerrain);
	}

	FreeFailedMenu();

	return(gTryAgain);
}


/**************************** BUILD FAILED MENU *********************************/

static void BuildFailedMenu(const char* title)
{
ObjNode	*obj;

	gFailedMenuSelection = 0;

	gTryAgain = true;

			/* BUILD NEW TEXT STRINGS */

	NewObjectDefinitionType def =
	{
		.coord 		= {0, -.6f, 0},
		.moveCall 	= MoveFailedObject,
		.scale		= FAILED_ICON_SCALE * .8f,
		.slot 		= SPRITE_SLOT,
	};
	obj = TextMesh_New(title, kTextMeshAlignCenter, &def);		// title
	obj->ColorFilter.a = 0;

	def.coord.y	-= LINE_SPACING * 1.5f;
	def.scale	= FAILED_ICON_SCALE;

	for (int i = 0; i < 2; i++)
	{
		const char* s = Localize(STR_TRY_AGAIN + i);

		gFailedIcons[i] = TextMesh_New(s, kTextMeshAlignCenter, &def);
		gFailedIcons[i]->ColorFilter.a = 0;
		def.coord.y -= LINE_SPACING;
	}
}

/****************** MOVE FAILED OBJECT ************************/

static void MoveFailedObject(ObjNode *theNode)
{
	theNode->ColorFilter.a += gFramesPerSecondFrac * 2.0f;			// fade in
	if (theNode->ColorFilter.a > 1.0f)
		theNode->ColorFilter.a = 1.0f;
}



/******************** FREE FAILED MENU **********************/
//
// Free objects of current menu
//

static void FreeFailedMenu(void)
{
int		i;

	for (i = 0; i < 2; i++)
	{
		DeleteObject(gFailedIcons[i]);
	}
}


/*********************** NAVIGATE FAILED MENU **************************/

static Boolean NavigateFailedMenu(void)
{
short	i;


		/* SEE IF CHANGE SELECTION */

	if (GetNewNeedStateAnyP(kNeed_UIUp) && (gFailedMenuSelection > 0))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gFailedMenuSelection--;
	}
	else
	if (GetNewNeedStateAnyP(kNeed_UIDown) && (gFailedMenuSelection < 1))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gFailedMenuSelection++;
	}

		/* SET APPROPRIATE FRAMES FOR ICONS */

	for (i = 0; i < 2; i++)
	{
		if (i != gFailedMenuSelection)
		{
			gFailedIcons[i]->ColorFilter.r = 										// normal
			gFailedIcons[i]->ColorFilter.g =
			gFailedIcons[i]->ColorFilter.b =
			gFailedIcons[i]->ColorFilter.a = 1.0;
		}
		else
		{
			gFailedIcons[i]->ColorFilter = gFailedMenuHiliteColor;										// hilite
		}

	}


			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		switch(gFailedMenuSelection)
		{
			case	0:								// try again
					gTryAgain = true;
					gGameOver = false;

					break;

			case	1:								// retire
					gTryAgain = false;
					break;

		}
		return(true);
	}


	return(false);
}

