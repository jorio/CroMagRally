/****************************/
/*   	TRAPS.C			    */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "mobjtypes.h"
#include "mytraps.h"
#include "collision.h"
#include "3dmath.h"
#include "sound2.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "terrain.h"
#include "player.h"
#include "main.h"
#include "effects.h"
#include "sobjtypes.h"
#include "bg3d.h"
#include "splineitems.h"
#include "file.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLPoint3D			gCoord;
extern	PlayerInfoType	gPlayerInfo[];
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLVector3D			gRecentTerrainNormal;
extern	SplineDefType		**gSplineList;
extern	uint32_t		gAutoFadeStatusBits;
extern	short				gNumTotalPlayers,gNumCollisions;
extern	SuperTileStatus	**gSuperTileStatusGrid;
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	PrefsType			gGamePrefs;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveDustDevil(ObjNode *);
static void MoveCampFire(ObjNode *);
static void MoveYeti(ObjNode *theNode);
static void MoveBeetle(ObjNode *theNode);
static void MoveCamel(ObjNode *theNode);
static void MoveCatapult(ObjNode *theNode);
static void CatapultThrowRock(ObjNode *theNode);
static void MoveCatapultRock(ObjNode *theNode);
static void MoveCannon(ObjNode *theNode);
static void ShootCannon(ObjNode *theNode, float dist);
static void MoveCannonBall(ObjNode *theNode);
static void MoveShark(ObjNode *theNode);
static void MoveGoddess(ObjNode *theNode);
static void GoddessAttack(ObjNode *goddess, short playerNum, float dist);
static void MoveCapsule(ObjNode *theNode);
static void CapsuleAttack(ObjNode *goddess, short playerNum, float dist);
static void MovePteradactyl(ObjNode *theNode);
static void PteradactylAttack(ObjNode *theNode);
static void MovePteradactylBomb(ObjNode *theNode);
static void MoveMummy(ObjNode *theNode);
static void MoveTotemPole(ObjNode *theNode);
static void ShootTotemPole(ObjNode *theNode, short playerNum);
static void MoveDart(ObjNode *theNode);
static void MoveDragon(ObjNode *theNode);
static void MoveTroll(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DUST_TIMER	.04f

#define	YETI_SCALE	7.0f
#define	YETI_YOFF	(50.0f * YETI_SCALE)

#define	BEETLE_SCALE	1.0f
#define	BEETLE_YOFF		(40.0f * BEETLE_SCALE)

#define	CAMEL_SCALE		3.0f

#define	SHARK_YOFF		5000.0f
#define	SHARK_SCALE		25.0f

#define	GODDESS_SPARK_TIMER	.02f
#define	MAX_BOLT_ENDPOINTS	20

#define	CAPSULE_SPARK_TIMER	.02f

#define	PTERADACTYL_YOFF	4000.0f
#define	PTERADACTYL_SCALE	15.0f

#define	MUMMY_SCALE			3.0f

#define	TOTEM_SHOOT_Y		2000.0f

#define	TROLL_SCALE			4.5f;

/*********************/
/*    VARIABLES      */
/*********************/

#define	DustTimer		SpecialF[0]
#define	BendFactor		SpecialF[1]

#define	ThrowRock		Flag[0]

#define	ShootTimer		SpecialF[0]

#define	SparkAngle		SpecialF[2]
#define	TimeSinceBolt	SpecialF[3]
#define	CapsuleBob		SpecialF[4]


/************************* ADD DUST DEVIL *********************************/

Boolean AddDustDevil(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*baseObj;
int		i;


			/* CREATE BASE */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= DESERT_ObjType_DustDevilTop;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= FENCE_SLOT+2;
	gNewObjectDefinition.moveCall 	= MoveDustDevil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1.0;
	baseObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (baseObj == nil)
		return(false);

	baseObj->TerrainItemPtr = itemPtr;								// keep ptr to item list
	baseObj->BendFactor = 0;										// init bending factor
	baseObj->ParticleGroup = -1;										// no particle group yet
	baseObj->DustTimer = DUST_TIMER;								// reset dust timer
	baseObj->EffectChannel = -1;									// no sound yet

	baseObj->CType = CTYPE_AVOID;									// CPU cars should avoid this

			/* CREATE EXTRA SEGMENTS */

	for (i = 1; i < 5; i++)
	{
		gNewObjectDefinition.type 		= DESERT_ObjType_DustDevilTop + i;
		gNewObjectDefinition.moveCall 	= nil;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return(false);

		baseObj->ChainNode = newObj;								// chain to previous segment

		baseObj = newObj;
	}

	return(true);													// item was added
}


/********************* MOVE DUST DEVIL *************************/

static void MoveDustDevil(ObjNode *baseObj)
{
ObjNode	*segObj = baseObj;
int		i;
float	dr;
float	fps = gFramesPerSecondFrac;
float	baseX,baseZ,x,z;
float	bendIndex;
static const float	segOffsets[] = {100,350,250,150,150};
int				particleGroup,magicNum;

			/***************/
			/* SEE IF GONE */
			/***************/

	if (TrackTerrainItem(baseObj))
	{
		DeleteObject(baseObj);
		return;
	}


		/********************/
		/* UPDATE ANIMATION */
		/********************/

	dr = 5.0f;

	baseX = x = baseObj->Coord.x;							// get base/top coords
	baseZ = z = baseObj->Coord.z;

	bendIndex = baseObj->BendFactor += PI * fps;

	i = 0;
	do
	{

		segObj->Coord.x = x;
		segObj->Coord.z = z;

		segObj->Rot.y += dr * fps;
		dr += 4.0f;

		UpdateObjectTransforms(segObj);

		x = baseX + sin(bendIndex) * segOffsets[i];

		bendIndex += .05f;

		segObj = segObj->ChainNode;					// next segment in chain
		i++;
	}while(segObj);


		/***********************/
		/* UPDATE SOUND EFFECT */
		/***********************/

	if (baseObj->EffectChannel != -1)
		Update3DSoundChannel(EFFECT_DUSTDEVIL, &baseObj->EffectChannel, &baseObj->Coord);
	else
		baseObj->EffectChannel = PlayEffect_Parms3D(EFFECT_DUSTDEVIL, &baseObj->Coord, NORMAL_CHANNEL_RATE, 1.5);



		/***********************/
		/* CHECK IF HIT PLAYER */
		/***********************/

	for (i = 0; i < gNumTotalPlayers; i++)
	{
		ObjNode	*playerObj;

		if (gPlayerInfo[i].distToFloor > 1000.0f)							// see if player already high up
			continue;

		if (CalcQuickDistance(x,z, gPlayerInfo[i].coord.x, gPlayerInfo[i].coord.z) > 700.0f)		// see if in range
			continue;

		playerObj = gPlayerInfo[i].objNode;									// get player ObjNode

		playerObj->Delta.y += 12000.0f * fps;

		playerObj->DeltaRot.y += 20.0f * fps;

	}

		/***********************/
		/* MAKE DUST PARTICLES */
		/***********************/

	if (gFramesPerSecond < 15.0f)									// help us out if speed is really bad
		return;


	baseObj->DustTimer -= fps;					// see if add dust
	if (baseObj->DustTimer <= 0.0f)
	{
		baseObj->DustTimer += DUST_TIMER;			// reset timer

		particleGroup 	= baseObj->ParticleGroup;
		magicNum 		= baseObj->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			NewParticleGroupDefType	groupDef;

			baseObj->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= 0;
			groupDef.gravity				= -200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 60;
			groupDef.decayRate				=  0;
			groupDef.fadeRate				= .5;
			groupDef.particleTextureNum		= PARTICLE_SObjType_Dirt;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			baseObj->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			NewParticleDefType	newParticleDef;
			OGLVector3D			d;
			OGLPoint3D			p;

			for (i = 0; i < 5; i++)
			{
				p.x = x + RandomFloat2() * 150.0f;
				p.y = baseObj->Coord.y + RandomFloat() * 500.0f;
				p.z = z + RandomFloat2() * 150.0f;

				d.x = RandomFloat2() * 500.0f;
				d.y = RandomFloat() * 300.0f;
				d.z = RandomFloat2() * 500.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= (RandomFloat()-.5f) * 2.0f;
				newParticleDef.alpha		= 1.0;
				if (AddParticleToGroup(&newParticleDef))
				{
					baseObj->ParticleGroup = -1;
					break;
				}
			}
		}
	}

}


#pragma mark -

/************************ PRIME YETI *************************/

Boolean PrimeYeti(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_YETI;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + YETI_YOFF;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 144;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= YETI_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	DetachObject(newObj);									// detach this object from the linked list


				/* SET MORE INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveYeti;						// set move call
	newObj->CType			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 20, 20, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj);
	return(true);
}


/******************** MOVE YETI ON SPLINE ***************************/

static void MoveYeti(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 25);

	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box
		theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc deltas
		theNode->Delta.y = (theNode->Coord.y - theNode->OldCoord.y) * gFramesPerSecond;
		theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
		UpdateShadow(theNode);
	}
}


#pragma mark -

/************************ PRIME BEETLE *************************/

Boolean PrimeBeetle(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_BEETLE;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + BEETLE_YOFF;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 144;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= BEETLE_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	DetachObject(newObj);									// detach this object from the linked list


				/* SET MORE INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveBeetle;						// set move call
	newObj->CType			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 10, 15, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj);
	return(true);
}


/******************** MOVE BEETLE ON SPLINE ***************************/

static void MoveBeetle(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 25);

	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) + BEETLE_YOFF;	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box
		theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc deltas
		theNode->Delta.y = (theNode->Coord.y - theNode->OldCoord.y) * gFramesPerSecond;
		theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
		UpdateShadow(theNode);
	}
}



#pragma mark -

/************************ PRIME CAMEL *************************/

Boolean PrimeCamel(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_CAMEL;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 144;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= CAMEL_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	DetachObject(newObj);									// detach this object from the linked list


				/* SET MORE INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveCamel;						// set move call
	newObj->CType			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;

	newObj->Skeleton->AnimSpeed = 1.6;

	SetObjectCollisionBounds(newObj, 700, -10, -300, 300, 300, -300);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 15, 30, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj);
	return(true);
}


/******************** MOVE CAMEL ON SPLINE ***************************/

static void MoveCamel(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 32);

	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z);	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box
		theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc deltas
		theNode->Delta.y = (theNode->Coord.y - theNode->OldCoord.y) * gFramesPerSecond;
		theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
		UpdateShadow(theNode);
	}
}


#pragma mark -


/************************* ADD CATAPULT *********************************/

Boolean AddCatapult(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_CATAPULT;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, MODEL_GROUP_SKELETONBASE+SKELETON_TYPE_CATAPULT, 0, 1.0);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 208;
	gNewObjectDefinition.rot 		= PI2 * ((float)itemPtr->parm[0] * (1.0f/8.0f));
	gNewObjectDefinition.scale 		= 4.5;
	gNewObjectDefinition.moveCall 	= MoveCatapult;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	newObj->ThrowRock		= false;

	return(true);													// item was added
}


/********************** MOVE CATAPULT ************************/

static void MoveCatapult(ObjNode *theNode)
{
SkeletonObjDataType	*skeleton = theNode->Skeleton;
short		p;
float		dist;

			/***************/
			/* SEE IF GONE */
			/***************/

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}


	switch(skeleton->AnimNum)
	{
			/* WAITING */

		case	0:
				p = FindClosestPlayerInFront(theNode, 15000, true, &dist, 0);		// see if anyone to shoot at
				if (p != -1)
				{
					SetSkeletonAnim(skeleton, 1);
					PlayEffect_Parms3D(EFFECT_CATAPULT, &theNode->Coord, NORMAL_CHANNEL_RATE - 0x100, 2.0);
				}
				break;


			/* THROWING */

		case	1:
				if (skeleton->AnimHasStopped)					// see if done
				{
					SetSkeletonAnim(skeleton, 0);
				}
				else
				if (theNode->ThrowRock)							// see if throw a rock
				{
					theNode->ThrowRock = false;
					CatapultThrowRock(theNode);
				}
				break;
	}
}


/****************** CATAPULT THROW ROCK ************************/

static void CatapultThrowRock(ObjNode *theNode)
{
static const OGLPoint3D		tipOff = {0,0,0};
static const OGLVector3D	throwVector = {0,.3,-1};
ObjNode		*newObj;
OGLMatrix4x4	m;
int				i;
float			speed;

	FindCoordOnJointAtFlagEvent(theNode, 7, &tipOff, &gNewObjectDefinition.coord);

	for (i = 0; i < 3; i++)				// throw 3 rocks
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
		gNewObjectDefinition.type 		= GLOBAL_ObjType_GreyRock;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB+2;
		gNewObjectDefinition.moveCall 	= MoveCatapultRock;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = .13;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return;


				/* MAKE SHADOW */

		AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4.0, 4.0, true);


			/*****************/
			/* SET IN MOTION */
			/*****************/

		speed = 8000.0f + RandomFloat() * 1000.0f;

		OGLMatrix4x4_SetRotate_Y(&m, theNode->Rot.y + (RandomFloat2() * .3f));
		OGLVector3D_Transform(&throwVector,	&m, &newObj->Delta);

		newObj->Delta.y += RandomFloat() * .1f;

		newObj->Delta.x *= speed;															// give it speed
		newObj->Delta.y *= speed;
		newObj->Delta.z *= speed;
	}
}


/******************* MOVE CATAPULT ROCK *********************/

static void MoveCatapultRock(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

//	ApplyFrictionToDeltas(fps * 500.0f, &gDelta);			// air friction

	gDelta.y -= 6000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;								// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.z += fps * 10.0f;
	theNode->Rot.x += fps * 8.0f;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) || DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, nil))
	{
		MakeBombExplosion(nil, gCoord.x, gCoord.z, &gDelta);

		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);
}

#pragma mark -

/********************** ADD GODDESS **************************/

Boolean AddGoddess(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CRETE_ObjType_Goddess;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_CLIPALPHA;
	gNewObjectDefinition.slot 		= 300;
	gNewObjectDefinition.moveCall 	= MoveGoddess;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 2000, -10, -250, 250, 250, -250);

	newObj->TimeSinceBolt= 0;

	return(true);													// item was added
}

/**************** MOVE GODDESS **********************/

static void MoveGoddess(ObjNode *theNode)
{
float	dist;
short	p;

			/* SEE IF GONE */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

		/**************/
		/* MAKE SPARK */
		/**************/

	theNode->DustTimer -= gFramesPerSecondFrac;						// see if add sparks
	if (theNode->DustTimer <= 0.0f)
	{
		int	particleGroup,magicNum, i;

		theNode->DustTimer += GODDESS_SPARK_TIMER;			// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			NewParticleGroupDefType	groupDef;

			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= 0;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 20;
			groupDef.decayRate				=  .5;
			groupDef.fadeRate				= 1.0;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreenFire;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			NewParticleDefType	newParticleDef;
			OGLVector3D			d;
			OGLPoint3D			pt;
			float				r,x,y,z;

			r = theNode->SparkAngle += gFramesPerSecondFrac * 2.0f;
			x = theNode->Coord.x;
			y = theNode->Coord.y + 1000.0f;
			z = theNode->Coord.z;

			for (i = 0; i < 3; i++)
			{
				pt.x = x + RandomFloat2() * 100.0f;
				pt.y = y + RandomFloat2() * 100.0f;
				pt.z = z + RandomFloat2() * 100.0f;

				d.x = sin(r) * 700.0f;
				d.y = RandomFloat2() * 100.0f;
				d.z = cos(r) * 700.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &pt;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= 0;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= 1.0;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}


	/************************/
	/* SEE IF ATTACK ANYONE */
	/************************/

	theNode->TimeSinceBolt += gFramesPerSecondFrac;
	if (theNode->TimeSinceBolt > 2.0f)
	{
		p = FindClosestPlayer(nil, theNode->Coord.x, theNode->Coord.z, 5000, true, &dist);
		if (p != -1)
		{
			GoddessAttack(theNode, p, dist);
			theNode->TimeSinceBolt = 0;
		}
	}
}


/*************** GODDESS ATTACK ***********************/

static void GoddessAttack(ObjNode *goddess, short playerNum, float dist)
{
int		numSegments,particleGroup, i,p, particlesPerSegment;
float	x,y,z;
OGLPoint3D	endPoints[MAX_BOLT_ENDPOINTS];
OGLVector3D	boltVector;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			pt;

	x = goddess->Coord.x;
	y = goddess->Coord.y + 1000.0f;
	z = goddess->Coord.z;

	numSegments = dist * .005;									// calc # bolt segments based on distance to player
	if (numSegments > MAX_BOLT_ENDPOINTS)
		numSegments = MAX_BOLT_ENDPOINTS;
	if (numSegments < 2)
		numSegments = 2;

		/***********************/
		/* CALCULATE ENDPOINTS */
		/***********************/

		/* CALC VECTOR FROM GODDESS TO PLAYER */

	boltVector.x = (gPlayerInfo[playerNum].coord.x + RandomFloat2() * 300.0f) - x;
	boltVector.y = gPlayerInfo[playerNum].coord.y - y;
	boltVector.z = (gPlayerInfo[playerNum].coord.z + RandomFloat2() * 300.0f) - z;

	boltVector.x /= (float)(numSegments-1);							// divide vector length for multiple segments
	boltVector.y /= (float)(numSegments-1);
	boltVector.z /= (float)(numSegments-1);


		/* SET STARTING POINT */

	endPoints[0].x = goddess->Coord.x;
	endPoints[0].y = goddess->Coord.y + 1000.0f;
	endPoints[0].z = goddess->Coord.x;


		/* CALC INTERMEDIATE & END POINTS */

	for (i = 1; i < numSegments; i++)
	{
		x += boltVector.x;										// calc linear endpoint value
		y += boltVector.y;
		z += boltVector.z;

		endPoints[i].x = x + RandomFloat2() * 100.0f;			// randomize it
		endPoints[i].y = y + RandomFloat2() * 100.0f;
		endPoints[i].z = z + RandomFloat2() * 100.0f;
	}


		/****************************************/
		/* CREATE PARTICLES ALONG BOLT SEGMENTS */
		/****************************************/

				/* MAKE NEW PARTICLE GROUP */

	groupDef.magicNum				= 0;
	groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	groupDef.flags					= 0;
	groupDef.gravity				= 0;
	groupDef.magnetism				= 0;
	groupDef.baseScale				= 25;
	groupDef.decayRate				=  .6;
	groupDef.fadeRate				= 1.0;
	groupDef.particleTextureNum		= PARTICLE_SObjType_GreenFire;
	groupDef.srcBlend				= GL_SRC_ALPHA;
	groupDef.dstBlend				= GL_ONE;
	particleGroup 					= NewParticleGroup(&groupDef);
	if (particleGroup == -1)
		return;

				/* BUILD PARTICLES */

	particlesPerSegment = MAX_PARTICLES / numSegments;

	for (i = 0; i < (numSegments-1); i++)											// do each segment
	{
		pt.x = endPoints[i].x;															// get start coord of segment
		pt.y = endPoints[i].y;
		pt.z = endPoints[i].z;

		boltVector.x = (endPoints[i+1].x - pt.x) / particlesPerSegment;				// calc vector to next endpoint
		boltVector.y = (endPoints[i+1].y - pt.y) / particlesPerSegment;
		boltVector.z = (endPoints[i+1].z - pt.z) / particlesPerSegment;

		for (p = 0; p < particlesPerSegment; p++)
		{
			d.x = RandomFloat2() * 40.0f;
			d.y = RandomFloat2() * 40.0f;
			d.z = RandomFloat2() * 40.0f;


			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= 1.0;
			AddParticleToGroup(&newParticleDef);

			pt.x += boltVector.x;
			pt.y += boltVector.y;
			pt.z += boltVector.z;
		}
	}

		/*****************************/
		/* PUT EXPLOSION AT ENDPOINT */
		/*****************************/

	MakeSparkExplosion(pt.x, pt.y, pt.z, 500.0, PARTICLE_SObjType_WhiteSpark);
	BlastCars(-1, pt.x, pt.y, pt.z, 300.0);
	PlayEffect_Parms3D(EFFECT_CANNON, &goddess->Coord, NORMAL_CHANNEL_RATE * 3/4, 6);


}



#pragma mark -

/********************** ADD CANNON **************************/

Boolean AddCannon(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= EUROPE_ObjType_Cannon;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 150;
	gNewObjectDefinition.moveCall 	= MoveCannon;
	gNewObjectDefinition.rot 		= PI2 * ((float)itemPtr->parm[0] * (1.0f/8.0f));
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);


	newObj->ShootTimer = 0;

	return(true);													// item was added
}



/**************** MOVE CANNON **********************/

static void MoveCannon(ObjNode *theNode)
{
short	p;
float	dist;

	if (TrackTerrainItem(theNode))													// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (gGamePrefs.difficulty > DIFFICULTY_EASY)		// dont shoot if easy
	{
		theNode->ShootTimer -= gFramesPerSecondFrac;									// see if able to shoot now
		if (theNode->ShootTimer <= 0.0f)
		{
				/* SEE IF SHOT ANYONE */

			p = FindClosestPlayerInFront(theNode, 16000, true, &dist, .7);
			if (p != -1)
			{
				theNode->ShootTimer = 2.0;												// delay until can shoot again
				ShootCannon(theNode, dist);
			}
		}
	}
}



/***************** SHOOT CANNON **********************/

static void ShootCannon(ObjNode *theNode, float dist)
{
static const OGLPoint2D	nozzle = {0,-800};
OGLMatrix3x3	m;
OGLMatrix4x4	m2;
OGLPoint2D		nozPt;
float			speed,dx,dy,dz,y;
static const OGLVector3D	throwVector = {0,.15,-.9};
ObjNode			*newObj;
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;

	PlayEffect_Parms3D(EFFECT_CANNON, &theNode->Coord, NORMAL_CHANNEL_RATE, 4.0);

	OGLMatrix3x3_SetRotate(&m, -theNode->Rot.y);									// calc coords of nozzle
	OGLPoint2D_Transform(&nozzle, &m, &nozPt);
	nozPt.x += theNode->Coord.x;
	nozPt.y += theNode->Coord.z;



			/********************/
			/* MAKE CANNON BALL */
			/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= EUROPE_ObjType_CannonBall;
	gNewObjectDefinition.coord.x 	= nozPt.x;
	gNewObjectDefinition.coord.z 	= nozPt.y;
	gNewObjectDefinition.coord.y 	= y = theNode->Coord.y + 500.0f;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+5;
	gNewObjectDefinition.moveCall 	= MoveCannonBall;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 6.0, 6.0, true);


		/*****************/
		/* SET IN MOTION */
		/*****************/

	if (dist < 5000.0f)
		dist = 5000.0;

	speed = dist * .9f;

	OGLMatrix4x4_SetRotate_Y(&m2, theNode->Rot.y + (RandomFloat2() * .1f));
	OGLVector3D_Transform(&throwVector,	&m2, &delta);

	newObj->Delta.y += RandomFloat() * .1f;

	newObj->Delta.x = delta.x * speed;															// give it speed
	newObj->Delta.y = delta.y * speed;
	newObj->Delta.z = delta.z * speed;


			/***************************/
			/* MAKE SPARKS FROM NOZZLE */
			/***************************/

	dx = newObj->Delta.x * .1f;
	dy = newObj->Delta.y * .1f;
	dz = newObj->Delta.z * .1f;

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= 0;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 60;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= 1.0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 50; i++)
		{
			pt.x = nozPt.x + RandomFloat2() * 130.0f;
			pt.y = y + RandomFloat() * 40.0f;
			pt.z = nozPt.y + RandomFloat2() * 130.0f;

			delta.x = dx + RandomFloat2() * 200.0f;
			delta.y = dy + RandomFloat2() * 200.0f;
			delta.z = dz + RandomFloat2() * 200.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 15.0;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}

}


/******************* MOVE CANNON BALL *******************/

static void MoveCannonBall(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 6000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;								// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.z += fps * 10.0f;
	theNode->Rot.x += fps * 8.0f;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) || DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, nil))
	{
		MakeBombExplosion(nil, gCoord.x, gCoord.z, &gDelta);

		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);

}






#pragma mark -

/************************ PRIME SHARK *************************/

Boolean PrimeShark(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_SHARK;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + SHARK_YOFF;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 144;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= SHARK_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	DetachObject(newObj);									// detach this object from the linked list


				/* SET MORE INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveShark;						// set move call
	newObj->CType			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 15, 25, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj);
	return(true);
}


/******************** MOVE SHARK ON SPLINE ***************************/

static void MoveShark(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 200);

	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) + SHARK_YOFF;	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box
		theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc deltas
		theNode->Delta.y = (theNode->Coord.y - theNode->OldCoord.y) * gFramesPerSecond;
		theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
		UpdateShadow(theNode);
	}
}



#pragma mark -

/********************** ADD CAPSULE **************************/

Boolean AddCapsule(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= ATLANTIS_ObjType_Capsule;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 2000.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= 170;
	gNewObjectDefinition.moveCall 	= MoveCapsule;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 200, -200, -150, 150, 150, -150);

	newObj->TimeSinceBolt = 0;
	newObj->CapsuleBob = RandomFloat();

	return(true);													// item was added
}

/**************** MOVE CAPSULE **********************/

static void MoveCapsule(ObjNode *theNode)
{
float	dist;
short	p;

			/* SEE IF GONE */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Rot.y += gFramesPerSecondFrac * 2.0f;			// spin

	theNode->CapsuleBob += gFramesPerSecondFrac * 1.0f;
	theNode->Coord.y = theNode->InitCoord.y + sin(theNode->CapsuleBob) * 400.0f;

	UpdateObjectTransforms(theNode);


	/************************/
	/* SEE IF ATTACK ANYONE */
	/************************/

	theNode->TimeSinceBolt += gFramesPerSecondFrac;
	if (theNode->TimeSinceBolt > 2.0f)
	{
		p = FindClosestPlayer(nil, theNode->Coord.x, theNode->Coord.z, 3000, true, &dist);
		if (p != -1)
		{
			CapsuleAttack(theNode, p, dist);
			theNode->TimeSinceBolt = 0;
		}
	}
}


/*************** CAPSULE ATTACK ***********************/

static void CapsuleAttack(ObjNode *goddess, short playerNum, float dist)
{
int		numSegments,particleGroup, i,p, particlesPerSegment;
float	x,y,z;
OGLPoint3D	endPoints[MAX_BOLT_ENDPOINTS];
OGLVector3D	boltVector;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			pt;

	x = goddess->Coord.x;
	y = goddess->Coord.y;
	z = goddess->Coord.z;

	numSegments = dist * .005;									// calc # bolt segments based on distance to player
	if (numSegments > MAX_BOLT_ENDPOINTS)
		numSegments = MAX_BOLT_ENDPOINTS;
	if (numSegments < 2)
		numSegments = 2;

		/***********************/
		/* CALCULATE ENDPOINTS */
		/***********************/

		/* CALC VECTOR FROM CAPSULE TO PLAYER */

	boltVector.x = (gPlayerInfo[playerNum].coord.x + RandomFloat2() * 300.0f) - x;
	boltVector.y = gPlayerInfo[playerNum].coord.y - y;
	boltVector.z = (gPlayerInfo[playerNum].coord.z + RandomFloat2() * 300.0f) - z;

	boltVector.x /= (float)(numSegments-1);							// divide vector length for multiple segments
	boltVector.y /= (float)(numSegments-1);
	boltVector.z /= (float)(numSegments-1);


		/* SET STARTING POINT */

	endPoints[0].x = goddess->Coord.x;
	endPoints[0].y = goddess->Coord.y;
	endPoints[0].z = goddess->Coord.x;


		/* CALC INTERMEDIATE & END POINTS */

	for (i = 1; i < numSegments; i++)
	{
		x += boltVector.x;										// calc linear endpoint value
		y += boltVector.y;
		z += boltVector.z;

		endPoints[i].x = x + RandomFloat2() * 100.0f;			// randomize it
		endPoints[i].y = y + RandomFloat2() * 100.0f;
		endPoints[i].z = z + RandomFloat2() * 100.0f;
	}


		/****************************************/
		/* CREATE PARTICLES ALONG BOLT SEGMENTS */
		/****************************************/

				/* MAKE NEW PARTICLE GROUP */

	groupDef.magicNum				= 0;
	groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	groupDef.flags					= 0;
	groupDef.gravity				= 0;
	groupDef.magnetism				= 0;
	groupDef.baseScale				= 35;
	groupDef.decayRate				=  1.0;
	groupDef.fadeRate				= 1.0;
	groupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark;
	groupDef.srcBlend				= GL_SRC_ALPHA;
	groupDef.dstBlend				= GL_ONE;
	particleGroup 					= NewParticleGroup(&groupDef);
	if (particleGroup == -1)
		return;

				/* BUILD PARTICLES */

	particlesPerSegment = MAX_PARTICLES / numSegments;

	for (i = 0; i < (numSegments-1); i++)											// do each segment
	{
		pt.x = endPoints[i].x;															// get start coord of segment
		pt.y = endPoints[i].y;
		pt.z = endPoints[i].z;

		boltVector.x = (endPoints[i+1].x - pt.x) / particlesPerSegment;				// calc vector to next endpoint
		boltVector.y = (endPoints[i+1].y - pt.y) / particlesPerSegment;
		boltVector.z = (endPoints[i+1].z - pt.z) / particlesPerSegment;

		for (p = 0; p < particlesPerSegment; p++)
		{
			d.x = RandomFloat2() * 20.0f;
			d.y = RandomFloat2() * 20.0f;
			d.z = RandomFloat2() * 20.0f;


			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= 1.0;
			AddParticleToGroup(&newParticleDef);

			pt.x += boltVector.x;
			pt.y += boltVector.y;
			pt.z += boltVector.z;
		}
	}

		/*****************************/
		/* PUT EXPLOSION AT ENDPOINT */
		/*****************************/

	PlayEffect_Parms3D(EFFECT_ZAP, &goddess->Coord, NORMAL_CHANNEL_RATE, 6);


		/* MAKE BUBBLES */

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= -40;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 5;
	gNewParticleGroupDef.decayRate				= 0;
	gNewParticleGroupDef.fadeRate				= .2;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Bubbles;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	particleGroup = NewParticleGroup(&gNewParticleGroupDef);
	if (particleGroup != -1)
	{
		x = pt.x;
		y = pt.y;
		z = pt.z;


		for (i = 0; i < 100; i++)
		{
			pt.x = x + RandomFloat2() * 160.0f;
			pt.y = y + RandomFloat2() * 160.0f;
			pt.z = z + RandomFloat2() * 160.0f;

			d.y = 500;
			d.x = 0;
			d.z = 0;


			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}

		/* GIVE SUBMARINE A NITRO BOOST */

	gPlayerInfo[playerNum].nitroTimer = 5.0;

	PlayEffect_Parms3D(EFFECT_NITRO, &gPlayerInfo[playerNum].coord, NORMAL_CHANNEL_RATE, 1.5);
}


#pragma mark -

/************************ PRIME PTERADACTYL *************************/

Boolean PrimePteradactyl(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_PTERADACTYL;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + PTERADACTYL_YOFF;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= PTERADACTYL_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	DetachObject(newObj);									// detach this object from the linked list


				/* SET MORE INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MovePteradactyl;						// set move call

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 30, 20, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj);
	return(true);
}


/******************** MOVE PTERADACTYL ON SPLINE ***************************/

static void MovePteradactyl(ObjNode *theNode)
{
Boolean isVisible;
float	fps = gFramesPerSecondFrac;
short	p;
float	dist;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 200);

	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) + PTERADACTYL_YOFF;	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc deltas
		theNode->Delta.y = (theNode->Coord.y - theNode->OldCoord.y) * gFramesPerSecond;
		theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
		UpdateShadow(theNode);

				/* SEE IF DROP BOMB */

		if (gGamePrefs.difficulty > DIFFICULTY_EASY)		// dont shoot if easy
		{
			theNode->ShootTimer -= fps;
			if (theNode->ShootTimer <= 0.0f)
			{
				p = FindClosestPlayer(nil, theNode->Coord.x, theNode->Coord.z, 3500, true, &dist);
				if (p != -1)
				{
					PteradactylAttack(theNode);
					theNode->ShootTimer = 3.0f;					// reset timer
				}

			}
		}

	}

			/* NOT VISIBLE */
	else
	{
	}
}


/******************* PTERADACTYLE ATTACK ************************/

static void PteradactylAttack(ObjNode *theNode)
{
ObjNode	*newObj;

			/********************/
			/* MAKE CANNON BALL */
			/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_GreyRock;
	gNewObjectDefinition.coord.x 	= theNode->Coord.x;
	gNewObjectDefinition.coord.y 	= theNode->Coord.y - 100.0f;
	gNewObjectDefinition.coord.z 	= theNode->Coord.z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+10;
	gNewObjectDefinition.moveCall 	= MovePteradactylBomb;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .3;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	newObj->Delta.x = theNode->Delta.x * .2f;			// give some momentum
	newObj->Delta.z = theNode->Delta.z * .2f;
	newObj->Delta.y = 0;


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 6.0, 6.0, true);

}


/******************* MOVE PTERADACTYL BOMB *******************/

static void MovePteradactylBomb(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 3000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;								// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.z += fps * 4.0f;
	theNode->Rot.x += fps * 2.0f;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) || DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, nil))
	{
		MakeBombExplosion(nil, gCoord.x, gCoord.z, &gDelta);

		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);

}


#pragma mark -

/************************* ADD DRAGON *********************************/

Boolean AddDragon(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.type 		= SKELETON_TYPE_DRAGON;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 5000.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_CLIPALPHA | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+2;
	gNewObjectDefinition.moveCall 	= MoveDragon;
	gNewObjectDefinition.rot 		= PI2 * ((float)itemPtr->parm[0] * (1.0f/8.0f));
	gNewObjectDefinition.scale 		= 20.0;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Flag[0] = false;										// not breathing fire


	return(true);													// item was added
}



/************************ MOVE DRAGON *****************************/

static void MoveDragon(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))													// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

		/* SEE IF BREATH FIRE */

	if (theNode->Flag[0])
	{
		theNode->DustTimer -= gFramesPerSecondFrac;					// see if add dust
		if (theNode->DustTimer <= 0.0f)
		{
			OGLVector3D			d;
			OGLPoint3D			p;
			static const OGLPoint3D		fireStart = {0,5,-10};
			static const OGLVector3D	refVec = {0,-200,-800};
			OGLMatrix4x4		m;
			int				particleGroup,magicNum,i;

			theNode->DustTimer += .05f;				// reset timer




					/* CALC BREATH VECTOR */

			FindCoordOnJoint(theNode, 5, &fireStart, &p);							// calc start point
			FindJointFullMatrix(theNode, 4, &m);									// calc vector
			OGLVector3D_Transform(&refVec, &m, &d);
			d.x *= 4000.0f;
			d.y *= 4000.0f;
			d.z *= 4000.0f;

					/* MAKE PARTICLES */

			particleGroup 	= theNode->ParticleGroup;
			magicNum 		= theNode->ParticleMagicNum;

			if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
			{
				NewParticleGroupDefType	groupDef;

				theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

				groupDef.magicNum				= magicNum;
				groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
				groupDef.flags					= 0;
				groupDef.gravity				= 0;
				groupDef.magnetism				= 0;
				groupDef.baseScale				= 70;
				groupDef.decayRate				=  0;
				groupDef.fadeRate				= .5;
				groupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
				groupDef.srcBlend				= GL_SRC_ALPHA;
				groupDef.dstBlend				= GL_ONE;
				theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
			}

			if (particleGroup != -1)
			{
				NewParticleDefType	newParticleDef;

				for (i = 0; i < 8; i++)
				{
					d.x += RandomFloat2() * 300.0f;
					d.y += RandomFloat2() * 300.0f;
					d.z += RandomFloat2() * 300.0f;

					newParticleDef.groupNum		= particleGroup;
					newParticleDef.where		= &p;
					newParticleDef.delta		= &d;
					newParticleDef.scale		= RandomFloat() + 1.0f;
					newParticleDef.rotZ			= RandomFloat() * PI2;
					newParticleDef.rotDZ		= RandomFloat2() * 2.0f;
					newParticleDef.alpha		= 1.0;
					if (AddParticleToGroup(&newParticleDef))
					{
						theNode->ParticleGroup = -1;
						break;
					}
				}
			}
		}
	}
}


#pragma mark -

/************************ PRIME MUMMY *************************/

Boolean PrimeMummy(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_MUMMY;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 205;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= MUMMY_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	DetachObject(newObj);									// detach this object from the linked list


				/* SET MORE INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveMummy;						// set move call
	newObj->CType			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;

	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 20, 20, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj);
	return(true);
}


/******************** MOVE MUMMY ON SPLINE ***************************/

static void MoveMummy(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 25);

	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box
		theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc deltas
		theNode->Delta.y = (theNode->Coord.y - theNode->OldCoord.y) * gFramesPerSecond;
		theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
		UpdateShadow(theNode);
	}
}

#pragma mark -

/********************** ADD TOTEMPOLE **************************/

Boolean AddTotemPole(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_TotemPole;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 460;
	gNewObjectDefinition.moveCall 	= MoveTotemPole;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	newObj->ShootTimer = 0;

	return(true);													// item was added
}



/**************** MOVE TOTEMPOLE **********************/

static void MoveTotemPole(ObjNode *theNode)
{
short	p;
float	dist;

	if (TrackTerrainItem(theNode))													// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	theNode->ShootTimer -= gFramesPerSecondFrac;									// see if able to shoot now
	if (theNode->ShootTimer <= 0.0f)
	{
			/* SEE IF SHOOT ANYONE */

		if (gGamePrefs.difficulty > DIFFICULTY_EASY)		// dont shoot if easy
		{
			p = FindClosestPlayer(nil, theNode->Coord.x, theNode->Coord.z, 3000, true, &dist);
			if (p != -1)
			{
				theNode->ShootTimer = 1.8;												// delay until can shoot again
				ShootTotemPole(theNode, p);
			}
		}
	}
}



/***************** SHOOT TOTEMPOLE **********************/

static void ShootTotemPole(ObjNode *theNode, short playerNum)
{
ObjNode			*newObj;
OGLVector3D				aimVec;
ObjNode		*playerObj = gPlayerInfo[playerNum].objNode;
float		targetX,targetY,targetZ;

			/*************************/
			/* CALC VECTOR TO TARGET */
			/*************************/

			/* PROJECT PLAYERS FUTURE COORD */

	targetX = playerObj->Coord.x + (playerObj->Delta.x * 1.5f);		// project n seconds into future
	targetY = playerObj->Coord.y + (playerObj->Delta.y * 1.5f);
	targetZ = playerObj->Coord.z + (playerObj->Delta.z * 1.5f);

			/* CALC VECTOR */

	aimVec.x = targetX - theNode->Coord.x;
	aimVec.y = targetY - (theNode->Coord.y + TOTEM_SHOOT_Y);
	aimVec.z = targetZ - theNode->Coord.z;
	OGLVector3D_Normalize(&aimVec, &aimVec);


			/*************/
			/* MAKE DART */
			/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_Dart;
	gNewObjectDefinition.coord.x 	= theNode->Coord.x;
	gNewObjectDefinition.coord.y 	= theNode->Coord.y + TOTEM_SHOOT_Y;
	gNewObjectDefinition.coord.z 	= theNode->Coord.z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+5;
	gNewObjectDefinition.moveCall 	= MoveDart;
	gNewObjectDefinition.rot 		=  CalcYAngleFromPointToPoint(0, 0, 0, aimVec.x, aimVec.z);
	gNewObjectDefinition.scale 	    = .4;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2.0, 6.0, true);


		/*****************/
		/* SET IN MOTION */
		/*****************/

	newObj->Delta.x = aimVec.x * 3000.0f;
	newObj->Delta.y = aimVec.y * 3000.0f;
	newObj->Delta.z = aimVec.z * 3000.0f;

	newObj->Rot.x = -PI/3;


	PlayEffect_Parms3D(EFFECT_BLOWDART, &newObj->Coord, NORMAL_CHANNEL_RATE, 1.0);
}


/******************* MOVE DART *******************/

static void MoveDart(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 1000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;								// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) || DoSimplePointCollision(&gCoord, CTYPE_PLAYER, nil))
	{
		MakeSparkExplosion(gCoord.x, gCoord.y, gCoord.z, 900.0f, PARTICLE_SObjType_WhiteSpark);
		BlastCars(-1, gCoord.x, gCoord.y, gCoord.z, 500.0f);
		PlayEffect_Parms3D(EFFECT_BOOM, &gCoord, NORMAL_CHANNEL_RATE + 0x4000, 3);

		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);

}


#pragma mark -

/************************ PRIME TROLL *************************/

Boolean PrimeTroll(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_TROLL;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 205;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= TROLL_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	DetachObject(newObj);									// detach this object from the linked list


	newObj->Skeleton->AnimSpeed = 1.5;


				/* SET MORE INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveTroll;						// set move call
	newObj->CType			= CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;

	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 20, 20, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj);
	return(true);
}


/******************** MOVE TROLL ON SPLINE ***************************/

static void MoveTroll(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 55);

	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box
		theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc deltas
		theNode->Delta.y = (theNode->Coord.y - theNode->OldCoord.y) * gFramesPerSecond;
		theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
		UpdateShadow(theNode);
	}
}









