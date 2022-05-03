/****************************/
/*   	TERRAIN2.C 	        */
/****************************/

/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include "mytraps.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void PreloadAllTiles(void);
static void RestoreItemList(void);
static Boolean NilAdd(TerrainItemEntryType *itemPtr,long x, long z);
static void	ShrinkSuperTileTextureMap(const uint16_t *srcPtr,uint16_t *destPtr);


/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/

short	  				gNumTerrainItems;
TerrainItemEntryType 	**gMasterItemList = nil;

float					**gMapYCoords = nil;		// 2D array of map vertex y coords
Byte					**gMapSplitMode = nil;

SuperTileItemIndexType	**gSuperTileItemIndexGrid = nil;


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_ITEM_NUM	66					// for error checking!

static Boolean (*gTerrainItemAddRoutines[MAX_ITEM_NUM+1])(TerrainItemEntryType *itemPtr, long x, long z) =
{
		NilAdd,								// My Start Coords
		AddCactus,							// 1
		AddWaterPatch,						// 2
		AddSign,							// 3 Signs
		AddTree,							// 4
		AddPOW,								// 5
		AddFinishLine,						// 6
		AddVase,							// 7
		AddRickshaw,						// 8
		AddFlagPole,						// 9 Flagpole
		AddWaterfall,						// 10 waterfall
		AddToken,							// 11 token (arrowhead, etc.)
		AddStickyTiresPOW,					// 12
		AddSuspensionPOW,					// 13
		AddEasterHead,						// 14
		AddDustDevil,						// 15
		AddSnoMan,							// 16
		AddCampFire,						// 17
		NilAdd,								// 18 Yeti
		AddLavaGenerator,					// 19 Lava Generator
		AddPillar,							// 20 pillar
		AddPylon,							// 21 pylon
		AddBoat,							// 22 solar boat
		NilAdd,								// 23 camel - SPLINE
		AddStatue,							// 24 statue
		AddSphinx,							// 25 sphinx head
		AddTeamTorch,						// 26 team torch
		AddTeamBase,						// 27 team base
		AddBubbleGenerator,					// 28
		AddInvisibilityPOW,					// 29
		AddRock,							// 30 rock
		AddBrontoNeck,						// 31 brontosaur neck
		AddRockOverhang,					// 32 rock overhang
		AddVine,							// 33 Vine
		AddAztecHead,						// 34 Aztec Head
		NilAdd,								// 35 beetle- spline
		AddCastleTower,						// 36 castle tower
		AddCatapult,						// 37 catapult
		AddGong,							// 38 Gong
		AddHouse,							// 39 Houses
		AddCauldron,						// 40 Cauldron
		AddWell,							// 41 Well
		AddVolcano,							// 42 volcano
		AddClock,							// 43 crete clock
		AddGoddess,							// 44 Goddess
		AddStoneHenge,						// 45 stone henge
		AddColiseum,						// 46 Coliseum
		AddStump,							// 47 Stump
		AddBaracade,						// 48 Baracade
		AddVikingFlag,						// 49 Viking Flag
		AddTorchPot,						// 50 Torchpot
		AddCannon,							// 51 Cannon
		AddClam,							// 52 Clam
		NilAdd,								// 53 Shark - spline
		NilAdd,								// 54 troll - spline
		AddWeaponsRack,						// 55 weapons rack
		AddCapsule,							// 56 Capsule
		AddSeaMine,							// 57 Sea Mine
		NilAdd,								// 58 Pteradactyl - spline
		AddDragon,							// 59 Chinese Dragon
		AddTarPatch,						// 60 Tar Patch
		NilAdd,								// 61 Mummy -spline
		AddTotemPole,						// 62 Totem Pole
		AddDruid,							// 63 Druid
		NilAdd,								// 64 Polar Bear - spline
		AddFlower,							// 65 flower
		NilAdd,								// 66 Viking - spline
};



/********************* BUILD TERRAIN ITEM LIST ***********************/
//
// This takes the input item list and resorts it according to supertile grid number
// such that the items on any supertile are all sequential in the list instead of scattered.
//

void BuildTerrainItemList(void)
{
TerrainItemEntryType	**tempItemList;
TerrainItemEntryType	*srcList,*newList;
int						row,col,i;
int						itemX, itemZ;
int						total;

			/* ALLOC MEMORY FOR SUPERTILE ITEM INDEX GRID */


	Alloc_2d_array(SuperTileItemIndexType, gSuperTileItemIndexGrid, gNumSuperTilesDeep, gNumSuperTilesWide);

	if (gNumTerrainItems == 0)
		DoFatalAlert("BuildTerrainItemList: there must be at least 1 terrain item!");


			/* ALLOC MEMORY FOR NEW LIST */

	tempItemList = (TerrainItemEntryType **)AllocHandle(sizeof(TerrainItemEntryType) * gNumTerrainItems);
	if (tempItemList == nil)
		DoFatalAlert("BuildTerrainItemList: AllocPtr failed!");

	HLock((Handle)gMasterItemList);
	HLock((Handle)tempItemList);

	srcList = *gMasterItemList;
	newList = *tempItemList;


			/************************/
			/* SCAN ALL SUPERTILES  */
			/************************/

	total = 0;

	for (row = 0; row < gNumSuperTilesDeep; row++)
	{
		for (col = 0; col < gNumSuperTilesWide; col++)
		{
			gSuperTileItemIndexGrid[row][col].numItems = 0;			// no items on this supertile yet


			/* FIND ALL ITEMS ON THIS SUPERTILE */

			for (i = 0; i < gNumTerrainItems; i++)
			{
				itemX = srcList[i].x;								// get pixel coords of item
				itemZ = srcList[i].y;

				itemX /= TERRAIN_SUPERTILE_UNIT_SIZE;				// convert to supertile row
				itemZ /= TERRAIN_SUPERTILE_UNIT_SIZE;				// convert to supertile column

				if ((itemX == col) && (itemZ == row))				// see if its on this supertile
				{
					if (gSuperTileItemIndexGrid[row][col].numItems == 0)		// see if this is the 1st item
						gSuperTileItemIndexGrid[row][col].itemIndex = total;	// set starting index

					newList[total] = srcList[i];					// copy into new list
					total++;										// inc counter
					gSuperTileItemIndexGrid[row][col].numItems++;	// inc # items on this supertile

				}
				else
				if (itemX > col)									// since original list is sorted, we can know when we are past the usable edge
					break;
			}
		}
	}


		/* NUKE THE ORIGINAL ITEM LIST AND REASSIGN TO THE NEW SORTED LIST */

	DisposeHandle((Handle)gMasterItemList);						// nuke old list
	gMasterItemList = tempItemList;								// reassign



			/* FIGURE OUT WHERE THE STARTING POINT IS */

	FindPlayerStartCoordItems();										// look thru items for my start coords
}



/******************** FIND PLAYER START COORD ITEM *******************/
//
// Scans thru item list for item type #14 which is a teleport reciever / start coord,
//

void FindPlayerStartCoordItems(void)
{
long					i;
TerrainItemEntryType	*itemPtr;
Boolean                 flags[MAX_PLAYERS];

	for (i = 0; i < MAX_PLAYERS; i++)
		flags[i] = false;


	itemPtr = *gMasterItemList; 												// get pointer to data inside the LOCKED handle

				/* SCAN FOR "START COORD" ITEM */

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_MYSTARTCOORD)						// see if it's a MyStartCoord item
		{
			short	p;

					/* CHECK FOR BIT INFO */

			if (gGameMode ==GAME_MODE_CAPTUREFLAG)
			{
				if (!(itemPtr[i].parm[3] & 1))								// only check ones with bit set
					continue;
			}
			else
			{
				if (itemPtr[i].parm[3] & 1)									// skip those with bit set
					continue;
			}


			p = itemPtr[i].parm[0];											// player # is in parm 0

			if (p >= MAX_PLAYERS)											// skip illegal player #'s
				continue;

			gPlayerInfo[p].coord.x = gPlayerInfo[p].startX = itemPtr[i].x;
			gPlayerInfo[p].coord.z = gPlayerInfo[p].startZ = itemPtr[i].y;
			gPlayerInfo[p].startRotY = PI2 * ((float)itemPtr[i].parm[1] * (1.0f/16.0f));	// calc starting rotation aim

			if (flags[p])                      								// if we already got a coord for this player then err
                DoFatalAlert("FindPlayerStartCoordItems:  duplicate start item for player #n");
	        flags[p] = true;
		}
	}
}


/****************** ADD TERRAIN ITEMS ON SUPERTILE *******************/
//
// Called by DoPlayerTerrainUpdate() per each supertile needed.
// This scans all of the items on this supertile and attempts to add them.
//

void AddTerrainItemsOnSuperTile(long row, long col, short playerNum)
{
TerrainItemEntryType *itemPtr;
long			type,numItems,startIndex,i;
Boolean			flag;


	numItems = gSuperTileItemIndexGrid[row][col].numItems;		// see how many items are on this supertile
	if (numItems == 0)
		return;

	startIndex = gSuperTileItemIndexGrid[row][col].itemIndex;	// get starting index into item list
	itemPtr = &(*gMasterItemList)[startIndex];					// get pointer to 1st item on this supertile


			/*****************************/
			/* SCAN ALL ITEMS UNDER HERE */
			/*****************************/

	for (i = 0; i < numItems; i++)
	{
		float	x,z;

		if (itemPtr[i].flags & ITEM_FLAGS_INUSE)				// see if item available
			continue;

		x = itemPtr[i].x;										// get item coords
		z = itemPtr[i].y;

		if (!SeeIfCoordsOutOfRange(x,z,playerNum))				// if is in range of another player, then cannot add since this means item got reinstated while still in range of others
			continue;

		type = itemPtr[i].type;									// get item #
		if (type > MAX_ITEM_NUM)								// error check!
		{
			DoAlert("Illegal Map Item Type!");
			ShowSystemErr(type);
		}

		flag = gTerrainItemAddRoutines[type](&itemPtr[i],itemPtr[i].x, itemPtr[i].y); // call item's ADD routine
		if (flag)
			itemPtr[i].flags |= ITEM_FLAGS_INUSE;				// set in-use flag
	}
}



/******************** NIL ADD ***********************/
//
// nothing add
//

static Boolean NilAdd(TerrainItemEntryType *itemPtr,long x, long z)
{
	(void) itemPtr;
	(void) x;
	(void) z;
	return(false);
}


/***************** TRACK TERRAIN ITEM ******************/
//
// Returns true if theNode is out of range
//

Boolean TrackTerrainItem(ObjNode *theNode)
{
	return(SeeIfCoordsOutOfRange(theNode->Coord.x,theNode->Coord.z, -1));		// see if out of range of all players
}


/********************* SEE IF COORDS OUT OF RANGE RANGE *********************/
//
// Returns true if the given x/z coords are outside the item delete
// window of any of the players.
//

Boolean SeeIfCoordsOutOfRange(float x, float z, short playerToSkip)
{
int			row,col;
uint16_t		playerFlags, mask;

			/* SEE IF OUT OF RANGE */

	if ((x < 0) || (z < 0))
		return(true);
	if ((x >= gTerrainUnitWidth) || (z >= gTerrainUnitDepth))
		return(true);


		/* SEE IF A PLAYER USES THIS SUPERTILE */

	col = x * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;							// calc supertile relative row/col that the coord lies on
	row = z * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;

	playerFlags = gSuperTileStatusGrid[row][col].playerHereFlags;		// get player flags

	mask = 0xffff;
	if (playerToSkip != -1)												// see if dont check for a player
		mask &= ~(1 << playerToSkip);

	if (playerFlags & mask)												// if a player is using this supertile, then coords are in range
		return(false);
	else
		return(true);													// otherwise, out of range since no players can see this supertile
}


/*************************** ROTATE ON TERRAIN ***************************/
//
// Rotates an object's x & z such that it's lying on the terrain.
//
// INPUT: surfaceNormal == optional input normal to use.
//

void RotateOnTerrain(ObjNode *theNode, float yOffset, OGLVector3D *surfaceNormal)
{
OGLVector3D		up;
float			r,x,z,y;
OGLPoint3D		to;
OGLMatrix4x4	*m,m2;

			/* GET CENTER Y COORD & TERRAIN NORMAL */

	x = theNode->Coord.x;
	z = theNode->Coord.z;
	y = theNode->Coord.y = GetTerrainY(x, z) + yOffset;

	if (surfaceNormal)
		up = *surfaceNormal;
	else
		up = gRecentTerrainNormal;


			/* CALC "TO" COORD */

	r = theNode->Rot.y;
	to.x = x + sin(r) * -30.0f;
	to.z = z + cos(r) * -30.0f;
	to.y = GetTerrainY(to.x, to.z) + yOffset;


			/* CREATE THE MATRIX */

	m = &theNode->BaseTransformMatrix;
	SetLookAtMatrix(m, &up, &theNode->Coord, &to);


		/* POP IN THE TRANSLATE INTO THE MATRIX */

	m->value[M03] = x;
	m->value[M13] = y;
	m->value[M23] = z;


			/* SET SCALE */

	OGLMatrix4x4_SetScale(&m2, theNode->Scale.x,				// make scale matrix
							 	1.0f,
							 	theNode->Scale.z);
	OGLMatrix4x4_Multiply(&m2, m, m);
}



#pragma mark ======= TERRAIN PRE-CONSTRUCTION =========




/********************** CALC TILE NORMALS *****************************/
//
// Given a row, col coord, calculate the face normals for the 2 triangles.
//

void CalcTileNormals(long row, long col, OGLVector3D *n1, OGLVector3D *n2)
{
static OGLPoint3D	p1 = {0,0,0};
static OGLPoint3D	p2 = {0,0,TERRAIN_POLYGON_SIZE};
static OGLPoint3D	p3 = {TERRAIN_POLYGON_SIZE,0,TERRAIN_POLYGON_SIZE};
static OGLPoint3D	p4 = {TERRAIN_POLYGON_SIZE, 0, 0};


		/* MAKE SURE ROW/COL IS IN RANGE */

	if ((row >= gTerrainTileDepth) || (row < 0) ||
		(col >= gTerrainTileWidth) || (col < 0))
	{
		n1->x = n2->x = 0;						// pass back up vector by default since our of range
		n1->y = n2->y = 1;
		n1->z = n2->z = 0;
		return;
	}

	p1.y = gMapYCoords[row][col];		// far left
	p2.y = gMapYCoords[row+1][col];		// near left
	p3.y = gMapYCoords[row+1][col+1];	// near right
	p4.y = gMapYCoords[row][col+1];		// far right


		/* CALC NORMALS BASED ON SPLIT */

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)
	{
		CalcFaceNormal(&p1, &p2, &p3, n1);		// fl, nl, nr
		CalcFaceNormal(&p1, &p3, &p4, n2);		// fl, nr, fr
	}
	else
	{
		CalcFaceNormal(&p1, &p2, &p4, n1);		// fl, nl, fr
		CalcFaceNormal(&p4, &p2, &p3, n2);		// fr, nl, nr
	}
}
