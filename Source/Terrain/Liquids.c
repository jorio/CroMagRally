/****************************/
/*   	LIQUIDS.C		    */
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/



static void MoveWaterPatch(ObjNode *theNode);
static void UpdateWaterTextureAnimation(void);



/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

const float gWaterHeights[NUM_TRACKS][6] =
{
	[TRACK_NUM_DESERT]      = {0,0,0,0,0,0},
	[TRACK_NUM_JUNGLE]      = {700,0,0,0,0,0},
	[TRACK_NUM_ICE]         = {0,0,0,0,0,0},
	[TRACK_NUM_CRETE]       = {600,0,0,0,0,0},
	[TRACK_NUM_CHINA]       = {0,0,0,0,0,0},
	[TRACK_NUM_EGYPT]       = {250,0,0,0,0,0},
	[TRACK_NUM_EUROPE]      = {0,0,0,0,0,0},
	[TRACK_NUM_SCANDINAVIA] = {0,0,0,0,0,0},
	[TRACK_NUM_ATLANTIS]    = {0,0,0,0,0,0},
};



/*************** UPDATE LIQUID ANIMATION ****************/

void UpdateLiquidAnimation(void)
{
	UpdateWaterTextureAnimation();
}


/***************** FIND LIQUID Y **********************/

float FindLiquidY(float x, float z)
{
ObjNode *thisNodePtr;
float		y;

	thisNodePtr = gFirstNodePtr;
	do
	{
		if (thisNodePtr->CType & CTYPE_LIQUID)
		{
			if (thisNodePtr->CollisionBoxes)
			{
				if (x < thisNodePtr->CollisionBoxes[0].left)
					goto next;
				if (x > thisNodePtr->CollisionBoxes[0].right)
					goto next;
				if (z > thisNodePtr->CollisionBoxes[0].front)
					goto next;
				if (z < thisNodePtr->CollisionBoxes[0].back)
					goto next;

				y = thisNodePtr->CollisionBoxes[0].top;

				return(y);
			}
		}
next:
		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return(0);
}


#pragma mark -

/************************* ADD WATER PATCH *********************************/
//
// 4x4 tiles
//
// parm[3] bit 0 = fixed ht, parm[0] = ht.
//

Boolean AddWaterPatch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode				*newObj;
float				y;


	x -= (int)x % (int)TERRAIN_POLYGON_SIZE;				// round down to nearest tile & center
	x += TERRAIN_POLYGON_SIZE/2;
	z -= (int)z % (int)TERRAIN_POLYGON_SIZE;
	z += TERRAIN_POLYGON_SIZE/2;

	if (itemPtr->parm[3] & 1)						// see if use fixed height
	{
		y = gWaterHeights[gTrackNum][itemPtr->parm[0]];
	}
	else
		y = GetTerrainY(x,z) + 100.0f;				// get y coord of patch

	if (fabsf(gUserPhysics.terrainHeight) < EPS)
		y *= EPS;									// prevent z-fighting if user set terrain height to 0
	else
		y *= gUserPhysics.terrainHeight;


			/***************/
			/* MAKE OBJECT */
			/***************/

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_GLOBAL,
		.type 		= GLOBAL_ObjType_WaterPatch,
		.coord		= {x, y, z},
		.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING | STATUS_BIT_KEEPBACKFACES,
		.slot 		= SLOT_OF_DUMB-2,
		.moveCall 	= MoveWaterPatch,
		.scale 		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);


			/* SET OBJECT INFO */

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = LIQUID_WATER;

//	newObj->Transparency = .8;

			/* SET COLLISION */
			//
			// Water patches have a water volume collision area.
			// The collision volume starts a little under the top of the water so that
			// no collision is detected in shallow water, but if player sinks deep
			// then a collision will be detected.
			//

	newObj->CType = CTYPE_LIQUID|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,-50,-4000,
							-2 * TERRAIN_POLYGON_SIZE, 2 * TERRAIN_POLYGON_SIZE,
							2 * TERRAIN_POLYGON_SIZE,-2 * TERRAIN_POLYGON_SIZE);


	return(true);													// item was added
}


/********************* MOVE WATER PATCH **********************/

static void MoveWaterPatch(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
}


/**************** UPDATE WATER TEXTURE ANIMATION ********************/

static void UpdateWaterTextureAnimation(void)
{
float	du,dv;
MetaObjectPtr	waterObj;

			/* SEE IF DO ANIM ON THIS TRACK */

	switch(gTrackNum)
	{
		case	TRACK_NUM_JUNGLE:
		case	TRACK_NUM_CRETE:
				du = gFramesPerSecondFrac * .05f;
				dv = gFramesPerSecondFrac * .1f;
				break;

		case	TRACK_NUM_EGYPT:
				du = 0;
				dv = gFramesPerSecondFrac * .1f;
				break;

		default:
				return;
	}

		/* DO IT */

	waterObj = gBG3DGroupList[MODEL_GROUP_GLOBAL][GLOBAL_ObjType_WaterPatch];


	MO_VertexArray_OffsetUVs(waterObj, du, dv);
}


#pragma mark -


/************************* ADD WATERFALL *********************************/
//
// parm[0] = rot 0..15
// parm[1] = scale 0..n
//

Boolean AddWaterfall(TerrainItemEntryType *itemPtr, long  x, long z)
{
#if 0
ObjNode				*newObj;
float				scale,rot;


	rot = PI2 * ((float)itemPtr->parm[0] * (1.0f/16.0f));		// get rotation angle
	scale = 2.0f + ((float)itemPtr->parm[1] * .3f);


			/***************/
			/* MAKE OBJECT */
			/***************/

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type 		= JUNGLE_ObjType_Waterfall,
		.coord		= {x, GetTerrainY(x, z), z},
		.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING | STATUS_BIT_KEEPBACKFACES,
		.slot 		= SLOT_OF_DUMB+20,
		.moveCall 	= MoveWaterfall,
		.rot 		= rot,
		.scale 		= scale,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);


			/* SET OBJECT INFO */

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->ColorFilter.a = .8;
#endif

	return(true);													// item was added
}


#if 0  // Waterfall was removed by Pangea

/********************* MOVE WATERFALL **********************/

static void MoveWaterfall(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
}


/**************** UPDATE WATER TEXTURE ANIMATION ********************/

static void UpdateWaterfallTextureAnimation(void)
{
#if 0
MetaObjectPtr	waterObj;

			/* SEE IF DO ANIM ON THIS TRACK */

	switch(gTrackNum)
	{
		case	TRACK_NUM_JUNGLE:
				break;

		default:
				return;
	}


	waterObj = gBG3DGroupList[MODEL_GROUP_LEVELSPECIFIC][JUNGLE_ObjType_Waterfall];

	MO_VertexArray_OffsetUVs(waterObj, 0, -gFramesPerSecondFrac * .9f);
#endif
}

#endif

#pragma mark -

/************************* ADD TAR PATCH *********************************/
//
// 4x4 tiles
//

Boolean AddTarPatch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode				*newObj;
float				y;

	y = GetTerrainY(x,z) + 150.0f;				// get y coord of patch



			/***************/
			/* MAKE OBJECT */
			/***************/

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type 		= TAR_ObjType_TarPatch,
		.coord		= {x, y, z},
		.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING | STATUS_BIT_KEEPBACKFACES,
		.slot 		= 77,
		.moveCall 	= MoveStaticObject,
		.scale 		= 1.0 + (float)(itemPtr->parm[0]) * .5f
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);


			/* SET OBJECT INFO */

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list
	newObj->Kind = LIQUID_TAR;


			/* SET COLLISION */
			//
			// Water patches have a water volume collision area.
			// The collision volume starts a little under the top of the water so that
			// no collision is detected in shallow water, but if player sinks deep
			// then a collision will be detected.
			//

	newObj->CType = CTYPE_LIQUID|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_TOUCHABLE;

	CreateCollisionBoxFromBoundingBox(newObj,1,1);

//	SetObjectCollisionBounds(newObj,-50,-4000,
//							-2 * TERRAIN_POLYGON_SIZE, 2 * TERRAIN_POLYGON_SIZE,
//							2 * TERRAIN_POLYGON_SIZE,-2 * TERRAIN_POLYGON_SIZE);


	return(true);													// item was added
}





