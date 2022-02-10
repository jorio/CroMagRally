#if 0 // Unused in-game

/****************************/
/*   	PARTSHOP.C 	    */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "miscscreens.h"
#include "objects.h"
#include "window.h"
#include "input.h"
#include "sound2.h"
#include	"file.h"
#include "ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "sprites.h"
#include "player.h"
#include "sobjtypes.h"
#include "partshop.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow;
extern	FSSpec		gDataSpec;
extern	Boolean		gLowVRAM;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds,gSongPlayingFlag;
extern	PrefsType	gGamePrefs;
extern	OGLPoint3D	gCoord;
extern  MOPictureObject 	*gBackgoundPicture;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	PlayerInfoType	gPlayerInfo[MAX_PLAYERS];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawPartShop(OGLSetupOutputType *info);
static void SetupPartShop(short playerNum);
static void UpdatePartShopSelectionIcons(short playerNum);
static void DoPartShopControls(short playerNum);
static void MoveCursor(ObjNode *theNode);
static void ShowPartOptions(short playerNum);
static Boolean SelectOption(short playerNum);
static Byte GetPartCurrentType(Byte part, short playerNum);
static void SetPartCurrentType(Byte part, Byte type, short playerNum);
static void MakeCostNumber(u_long num, float x, float y);
static void DisposeCostDigits(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define ICON_SCALE		.5f

#define	PART_SPACING	(.5f * ICON_SCALE)
#define	OPTION_SPACING	(.8f * ICON_SCALE)

#define	COST_DIGIT_SCALE	.5f
#define	COST_DIGIT_SPACING	(.08f * COST_DIGIT_SCALE)


#define NUM_PART_TYPES	6							// includes EXit
#define	NUM_SUB_TYPES	3

enum
{
	PART_TYPE_BODY = 0,
	PART_TYPE_ENGINE,
	PART_TYPE_WHEELS,
	PART_TYPE_SUSPENSION,
	PART_TYPE_WEAPON,
	PART_TYPE_EXIT
};


/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode	*gPartIcons[NUM_PART_TYPES], *gCursorObj, *gExitIcon;
static ObjNode	*gOptions[NUM_SUB_TYPES];
static ObjNode	*gPurchase[NUM_PART_TYPES];

static int		gNumCostNumberDigits;
static ObjNode 	*gCostNumber[NUM_SUB_TYPES*10];			// enough for all of the digits for all sub-options

static short	gPurchaseTypes[NUM_PART_TYPES];

static Byte		gSelectedPart,gNumOptions,gSelectedOption;

static const Byte gPartBaseModel[] =
{
	PARTSHOP_SObjType_Body_Log,
	PARTSHOP_SObjType_Engine_Foot,
	PARTSHOP_SObjType_Wheel_Stone,
	PARTSHOP_SObjType_Suspension_None,
	PARTSHOP_SObjType_Weapon_Stone,
	PARTSHOP_SObjType_Exit
};


/********************** DO PART SHOP **************************/

void DoPartShop(short playerNum)
{
	SetupPartShop(playerNum);

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
		OGL_DrawScene(gGameViewInfoPtr, DrawPartShop);


			/* SEE IF MAKE SELECTION */

		DoPartShopControls(playerNum);

		if (GetNewKeyState(kKey_MakeSelection_P1))
		{
			if (SelectOption(playerNum))
				break;
		}
	}

			/* CLEANUP */

    DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeSpriteGroup(SPRITE_GROUP_PARTSHOP);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);


		/* UPDATE CAR PHYSICS PARAMETERS */

	SetCarParametersFromTypes(playerNum);

}


/****************** SETUP PART SHOP *************************/

static void SetupPartShop(short playerNum)
{
OGLSetupInputType	viewDef;
FSSpec				spec;
OGLColorRGBA		ambientColor = { .1, .1, .1, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLColorRGBA		fillColor2 = { .3, .3, .3, 1 };
OGLVector3D			fillDirection1 = { .9, -.7, -1 };
OGLVector3D			fillDirection2 = { -1, -.2, -.5 };
short				i;


	for (i = 0; i < NUM_SUB_TYPES; i++)								// no option icons yet
		gOptions[i] = nil;

	for (i = 0; i < NUM_PART_TYPES; i++)							// no part or purchse icons yet
	{
		gPartIcons[i] = nil;
		gPurchase[i] = nil;
		gPurchaseTypes[i] = -1;
	}

	gSelectedPart = 0;
	gSelectedOption = 0;
	gNumCostNumberDigits = 0;


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.fov 				= OGLMath_RadiansToDegrees(.8);
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 1;
	viewDef.styles.useFog			= false;

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

			/* CREATE BACKGROUND OBJECT */

	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:PartShopScreen", &spec))
		DoFatalAlert("DoPartShop: background pict not found.");
	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("DoPartShop: MO_CreateNewObjectOfType failed");


			/* LOAD SPRITES */

	FSMakeFSSpec(0, 0, ":data:sprites:partshop.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTSHOP, gGameViewInfoPtr);


			/***************/
			/* BUILD ICONS */
			/***************/

			/* SET PARTS MENU */

	UpdatePartShopSelectionIcons(playerNum);


			/* CURSOR */

	gNewObjectDefinition.group 		= SPRITE_GROUP_PARTSHOP;
	gNewObjectDefinition.type 		= PARTSHOP_SObjType_SelectionFrame;
	gNewObjectDefinition.coord		= gPartIcons[gSelectedPart]->Coord;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveCursor;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	gCursorObj = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);
}



/************* UPDATE PART SHOP SELECTION ICONS *****************/
//
// Sets the icon for each of the parts to correct value
//

static void UpdatePartShopSelectionIcons(short playerNum)
{
CarStatsType	*info= &gPlayerInfo[playerNum].carStats;
short			i;

			/* NUKE OLD ICONS */

	for (i = 0; i < NUM_PART_TYPES; i++)
	{
		if (gPartIcons[i])
		{
			DeleteObject(gPartIcons[i]);
			gPartIcons[i] = nil;
		}
	}


			/* BODY ICON */

	gNewObjectDefinition.group 		= SPRITE_GROUP_PARTSHOP;
	gNewObjectDefinition.type 		= PARTSHOP_SObjType_Body_Log + info->bodyType;
	gNewObjectDefinition.coord.x 	= -(PART_SPACING * (6-1)) / 2;
	gNewObjectDefinition.coord.y 	= .5;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = ICON_SCALE;
	gPartIcons[PART_TYPE_BODY] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);


			/* ENGINE ICON */

	gNewObjectDefinition.type 		= PARTSHOP_SObjType_Engine_Foot + info->engineType;
	gNewObjectDefinition.coord.x 	+= PART_SPACING;
	gPartIcons[PART_TYPE_ENGINE] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

			/* WHEEL ICON */

	gNewObjectDefinition.type 		= PARTSHOP_SObjType_Wheel_Stone + info->wheelType;
	gNewObjectDefinition.coord.x 	+= PART_SPACING;
	gPartIcons[PART_TYPE_WHEELS] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);


			/* SUSPENSION ICON */

	gNewObjectDefinition.type 		= PARTSHOP_SObjType_Suspension_None + info->suspensionType;
	gNewObjectDefinition.coord.x 	+= PART_SPACING;
	gPartIcons[PART_TYPE_SUSPENSION] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

			/* WEAPON ICON */

	gNewObjectDefinition.type 		= PARTSHOP_SObjType_Weapon_Stone + info->weaponClass;
	gNewObjectDefinition.coord.x 	+= PART_SPACING;
	gPartIcons[PART_TYPE_WEAPON] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);


			/* EXIT ICON */

	gNewObjectDefinition.type 		= PARTSHOP_SObjType_Exit;
	gNewObjectDefinition.coord.x 	+= PART_SPACING;
	gPartIcons[PART_TYPE_EXIT] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);


		/* SHOW OPTIONS FOR INITIAL SELECTION */

	ShowPartOptions(playerNum);
}


/***************** DRAW  PART SHOP *******************/

static void DrawPartShop(OGLSetupOutputType *info)
{
	MO_DrawObject(gBackgoundPicture, info);
	DrawObjects(info);
}



/***************** DO PART SHOP CONTROLS *******************/

static void DoPartShopControls(short playerNum)
{

		/* SEE IF CHANGE PART SELECTION */

	if (GetNewKeyState(kKey_Left_P1) && (gSelectedPart > 0))
	{
		gSelectedPart--;
		gSelectedOption = 0;
		gCursorObj->Coord.x -= PART_SPACING;
		gCursorObj->Coord.y = gPartIcons[gSelectedPart]->Coord.y;
		ShowPartOptions(playerNum);
	}
	else
	if (GetNewKeyState(kKey_Right_P1) && (gSelectedPart < 5))
	{
		gSelectedPart++;
		gSelectedOption = 0;
		gCursorObj->Coord.x += PART_SPACING;
		gCursorObj->Coord.y = gPartIcons[gSelectedPart]->Coord.y;
		ShowPartOptions(playerNum);
	}


		/* SEE IF CHANGE OPTION SELECTION */

	if (gSelectedPart != PART_TYPE_EXIT)			// exit doesnt have any subs
	{
		if (GetNewKeyState(kKey_Forward_P1) && (gSelectedOption > 0))
		{
			gSelectedOption--;
			gCursorObj->Coord.y += OPTION_SPACING;
		}
		else
		if (GetNewKeyState(kKey_Backward_P1) && (gSelectedOption < gNumOptions))
		{
			gSelectedOption++;
			gCursorObj->Coord.y -= OPTION_SPACING;
		}
	}
}


/******************** MOVE CURSOR ************************/

static void MoveCursor(ObjNode *theNode)
{
float	s;

	theNode->SpecialF[0] += gFramesPerSecondFrac * 8.0f;

	s = (sin(theNode->SpecialF[0])+1.0f) * .04f;

	theNode->Scale.x =
	theNode->Scale.y = ICON_SCALE + s;
}


/***************** SHOW PART OPTIONS **************************/

static void ShowPartOptions(short playerNum)
{
Byte			type;
short			i,numSubs;


			/* NUKE OLD OPTION ICONS */

	for (i = 0; i < NUM_SUB_TYPES; i++)
	{
		if (gOptions[i])
		{
			DeleteObject(gOptions[i]);
			gOptions[i] = nil;
		}
	}
	DisposeCostDigits();


			/* GET CURRENT TYPE OF OPTION */

	type = GetPartCurrentType(gSelectedPart, playerNum);
	if (type >= (NUM_SUB_TYPES-1))							// if we've already got the best, then bail
		return;

		/************************/
		/* BUILD OPTIONAL ICONS */
		/************************/

	gNumOptions = 0;

	if (gSelectedPart == PART_TYPE_EXIT)			// exit doesnt have any subs
		numSubs = 1;
	else
		numSubs = NUM_SUB_TYPES;

	for (i = type+1; i < numSubs; i++)
	{
		float	y = gPartIcons[gSelectedPart]->Coord.y - (i * OPTION_SPACING);

			/* BUILD THE PART ICON */

		gNewObjectDefinition.group 		= SPRITE_GROUP_PARTSHOP;
		gNewObjectDefinition.type 		= gPartBaseModel[gSelectedPart] + i;
		gNewObjectDefinition.coord.x 	= gPartIcons[gSelectedPart]->Coord.x;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.coord.z 	= gPartIcons[gSelectedPart]->Coord.z;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = ICON_SCALE;
		gOptions[i] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

			/* BUILD THE PRICE NUMBER */

		MakeCostNumber(1230, gPartIcons[gSelectedPart]->Coord.x, y-.2f);

		gNumOptions++;
	}
}


/************** GET PART CURRENT TYPE ********************/

static Byte GetPartCurrentType(Byte part, short playerNum)
{
CarStatsType	*info= &gPlayerInfo[playerNum].carStats;
Byte			type;

	switch(part)
	{
		case	PART_TYPE_BODY:
				type = info->bodyType;
				break;

		case	PART_TYPE_ENGINE:
				type = info->engineType;
				break;

		case	PART_TYPE_WHEELS:
				type = info->wheelType;
				break;

		case	PART_TYPE_SUSPENSION:
				type = info->suspensionType;
				break;

		case	PART_TYPE_WEAPON:
				type = info->weaponClass;
				break;

		case	PART_TYPE_EXIT:
				return(0);
	}

	return(type);
}


/************** SET PART CURRENT TYPE ********************/

static void SetPartCurrentType(Byte part, Byte type, short playerNum)
{
CarStatsType	*info= &gPlayerInfo[playerNum].carStats;

	switch(part)
	{
		case	PART_TYPE_BODY:
				info->bodyType = type;
				break;

		case	PART_TYPE_ENGINE:
				info->engineType = type;
				break;

		case	PART_TYPE_WHEELS:
				info->wheelType = type;
				break;

		case	PART_TYPE_SUSPENSION:
				info->suspensionType = type;
				break;

		case	PART_TYPE_WEAPON:
				info->weaponClass = type;
				break;
	}
}



/*************************** SELECT OPTION **********************************/
//
// OUTPUT:  true = selected Exit
//

static Boolean SelectOption(short playerNum)
{
short	type,i;

			/* SEE IF SELECTED EXIT */

	if (gSelectedPart == PART_TYPE_EXIT)
	{
		for (i = 0; i < (NUM_PART_TYPES-1); i++)		// make all purchases final (skip exit)
		{
			if (gPurchaseTypes[i] != -1)
				SetPartCurrentType(i, gPurchaseTypes[i], playerNum);
		}
		return(true);
	}


	type = GetPartCurrentType(gSelectedPart, playerNum);			// get type of part we already own


			/* DELETE ANY EXISTING OPTION ICON */

	if (gPurchase[gSelectedPart])			// see if anything to refund
	{
		DeleteObject(gPurchase[gSelectedPart]);
		gPurchase[gSelectedPart] = nil;
	}

		/* IF SELECTED ORIGINAL, THEN REMOVE ANY PURCHASE */

	if (gSelectedOption == 0)
	{
		gPurchaseTypes[gSelectedPart] = -1;
		return(false);
	}


		/* SELECTED A NEW OPTION UPGRADE */

	gPurchaseTypes[gSelectedPart] 	= type + gSelectedOption;

	gNewObjectDefinition.group 		= SPRITE_GROUP_PARTSHOP;
	gNewObjectDefinition.type 		= gPartBaseModel[gSelectedPart] + gPurchaseTypes[gSelectedPart];
	gNewObjectDefinition.coord.x 	= gPartIcons[gSelectedPart]->Coord.x;
	gNewObjectDefinition.coord.y 	= -.5;
	gNewObjectDefinition.coord.z 	= gPartIcons[gSelectedPart]->Coord.z;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = ICON_SCALE;
	gPurchase[gSelectedPart] 		= MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);


	return(false);
}


#pragma mark -

/*********************** MAKE COST NUMBER ********************/
//
// NOTE: we do not init gNumCostNumberDigits at the beginning because
//		we want to attach these digits to the master list.
//

static void MakeCostNumber(u_long num, float x, float y)
{
u_long	digit,i,j = gNumCostNumberDigits;
float	w;
float	x2;


			/* BUILD THE NUMBER DIGITS */

	while((num > 0) || (gNumCostNumberDigits < 1))
	{
		digit = num % 10;								// get digit @ end of #
		num /= 10;										// shift down a digit

		gNewObjectDefinition.group 		= SPRITE_GROUP_PARTSHOP;
		gNewObjectDefinition.type 		= PARTSHOP_SObjType_Digit0 + digit;
		gNewObjectDefinition.coord.x 	= 0;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
		gNewObjectDefinition.slot 		= 1000;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = COST_DIGIT_SCALE;
		gCostNumber[gNumCostNumberDigits] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

		gNumCostNumberDigits++;
	}

			/* CENTER THE NUMBER */

	w = (gNumCostNumberDigits-j) * COST_DIGIT_SPACING;			// calc width of new number (digits from j to end)
	w *= .5f;													// offset is half
	w -= COST_DIGIT_SPACING/2;									// offset by half a digit's width
	x2 = x + w;													// calc rightmost coord

	for (i = j; i < gNumCostNumberDigits; i++)					// only center the new digits, so start @ j
	{
		gCostNumber[i]->Coord.x = x2;
		x2 -= COST_DIGIT_SPACING;
	}
}


/********************* DISPOSE COST DIGITS ***************************/
//
// Nukes all of the digits for all of the cost numbers being displayed
//

static void DisposeCostDigits(void)
{
int	i;

	for (i = 0; i < gNumCostNumberDigits; i++)
	{
		DeleteObject(gCostNumber[i]);
		gCostNumber[i] = nil;
	}

	gNumCostNumberDigits = 0;
}

#endif
