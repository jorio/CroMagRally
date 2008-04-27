/****************************/
/*   	LIQUIDS.C		    */
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "terrain.h"
#include "liquids.h"
#include "collision.h"
#include "3dmath.h"
#include "mobjtypes.h"
#include "metaobjects.h"
#include "bg3d.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLPoint3D			gCoord,gMyCoord;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	ObjNode				*gPlayerObj;
extern	Byte				gPlayerMode;
extern	u_long				gAutoFadeStatusBits;
extern	ObjNode				*gFirstNodePtr;
extern	MetaObjectPtr		gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	int					gTrackNum;

/****************************/
/*    PROTOTYPES            */
/****************************/



static void MoveWaterPatch(ObjNode *theNode);
static void UpdateWaterTextureAnimation(void);

static void MoveWaterfall(ObjNode *theNode);
static void UpdateWaterfallTextureAnimation(void);



/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

const float gWaterHeights[NUM_TRACKS][6] =
{
	0,0,0,0,0,0,			// desert
	700,0,0,0,0,0,			// forest
	0,0,0,0,0,0,			// glacier

	600,0,0,0,0,0,			// crete
	0,0,0,0,0,0,			// china
	250,0,0,0,0,0,			// egypt

	0,0,0,0,0,0,			// europe
	0,0,0,0,0,0,			// scandinavia
	0,0,0,0,0,0,			// atlantis

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



			/***************/
			/* MAKE OBJECT */
			/***************/

	gNewObjectDefinition.group		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_WaterPatch;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-2;
	gNewObjectDefinition.moveCall 	= MoveWaterPatch;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
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

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_Waterfall;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+20;
	gNewObjectDefinition.moveCall 	= MoveWaterfall;
	gNewObjectDefinition.rot 		= rot;
	gNewObjectDefinition.scale 		= scale;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);


			/* SET OBJECT INFO */

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->ColorFilter.a = .8;
#endif

	return(true);													// item was added
}


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

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= TAR_ObjType_TarPatch;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= 77;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0 + (float)(itemPtr->parm[0]) * .5f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
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





