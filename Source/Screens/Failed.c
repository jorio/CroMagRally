/****************************/
/*   	FAILED.C			*/
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "menu.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void BuildFailedMenu(const char* s);
static void MoveFailedObject(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	FAILED_ICON_SCALE	.4f



/*********************/
/*    VARIABLES      */
/*********************/



/********************** UPDATE FAILED MENU **************************/

static void UpdateFailedMenu(void)
{
	MoveEverything();
	DoPlayerTerrainUpdate();							// need to call this to keep supertiles active
}

/********************** DO FAILED **************************/
//
// Called at end of race when draw context is still visible and active
//
// return true if try again
//

Boolean DoFailedMenu(const char* headerString)
{
	{
		NewObjectDefinitionType def =
		{
			.coord 		= {0, 144, 0},
			.moveCall 	= MoveFailedObject,
			.scale		= FAILED_ICON_SCALE * .8f,
			.slot 		= SPRITE_SLOT,
		};
		ObjNode* header = TextMesh_New(headerString, kTextMeshAlignCenter, &def);
		header->ColorFilter.a = 0;
	}


	static const MenuItem failedMenu[] =
	{
		{ .id='fail' },
		{kMIPick, STR_TRY_AGAIN, .id=1, .next='EXIT'},
		{kMIPick, STR_RETIRE, .id=0, .next='EXIT'},
		{0 },
	};

	MenuStyle failedMenuStyle = kDefaultMenuStyle;
	failedMenuStyle.standardScale = FAILED_ICON_SCALE;
	failedMenuStyle.rowHeight = 28;
	failedMenuStyle.yOffset = 215;
	failedMenuStyle.fadeOutSceneOnExit = false;


				/*************/
				/* MAIN LOOP */
				/*************/


	gNoCarControls = true;									// nobody has control during this

	int outcome = StartMenu(failedMenu, &failedMenuStyle, UpdateFailedMenu, DrawTerrain);


			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (outcome == 0)								// retire
	{
		return false;
	}
	else											// try again
	{
		gGameOver = false;
		return true;
	}
}

/****************** MOVE FAILED OBJECT ************************/

static void MoveFailedObject(ObjNode *theNode)
{
	theNode->ColorFilter.a += gFramesPerSecondFrac * 2.0f;			// fade in
	if (theNode->ColorFilter.a > 1.0f)
		theNode->ColorFilter.a = 1.0f;
}
