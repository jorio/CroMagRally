/****************************/
/*   	PLAYER_WEAPONS.C    */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "objects.h"
#include "misc.h"
#include "collision.h"
#include "player.h"
#include "sound2.h"
#include "main.h"
#include "input.h"
#include "mobjtypes.h"
#include "skeletonanim.h"
#include "effects.h"
#include "3dmath.h"
#include "terrain.h"
#include "fences.h"
#include "skeletonjoints.h"
#include "triggers.h"
#include "infobar.h"
#include "skeletonobj.h"
#include "sobjtypes.h"

extern	ObjNode			*gCurrentPlayer;
extern	float			gFramesPerSecondFrac,gFramesPerSecond,gPlayerToCameraAngle;
extern	OGLPoint3D		gCoord;
extern	OGLVector3D		gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D		gRecentTerrainNormal;
extern	short			gCurrentPlayerNum,gPlayerMultiPassCount,gNumTotalPlayers;
extern	PlayerInfoType	gPlayerInfo[];
extern	uint32_t			gAutoFadeStatusBits;
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	int				gGameMode;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void VehicleActivatePOW(ObjNode *theVehicle, Boolean forwardThrow);
static void MoveOilBullet(ObjNode *theNode);
static void MakeOilSpill(float x, float z);
static void MoveOilSpill(ObjNode *theNode);
static void PlayerThrowsWeapon(short playerNum, Boolean forwardThrow);
static void MoveBoneWeapon(ObjNode *theNode);
static void SetThrowDeltas(ObjNode *theProjectile, ObjNode *theCar, float force, Boolean throwForward);
static void SetThrowDeltas2(ObjNode *theProjectile, ObjNode *theCar, float force, Boolean throwForward);
static void MoveBirdBomb(ObjNode *theNode);
static void PlayerLaunchRomanCandle(short playerNum);
static void MoveRomanCandle(ObjNode *theNode);
static void PlayerLaunchBottleRocket(short playerNum, Boolean forwardThrow);
static void MoveBottleRocket(ObjNode *theNode);
static void ExplodeRomanCandle(ObjNode *theBullet, const OGLPoint3D *where);
static void ExplodeBottleRocket(ObjNode *theBullet, const OGLPoint3D *where);
static void PlayerLaunchTorpedo(short playerNum);
static void MoveTorpedo(ObjNode *theNode);
static void ExplodeTorpedo(ObjNode *theBullet, const OGLPoint3D *where);
static void MoveFreezeWeapon(ObjNode *theNode);
static void MoveLandMine(ObjNode *theNode);
static void DropLandMine(short playerNum);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	NITRO_DURATION		5.0f

#define	BIRD_BOMB_MAX_ATTACK_DIST	15000.0f
#define	BIRD_FLY_SPEED		6000.0f

#define	BOTTLE_ROCKET_SPEED			5500.0f
#define	BOTTLEROCKET_BLAST_RADIUS	600.0f

#define TORPEDO_SPEED			(MAX_SUBMARINE_SPEED + 1900.0f)
#define	TORPEDO_BLAST_RADIUS	600.0f



/*********************/
/*    VARIABLES      */
/*********************/

#define	TargetPlayer	Special[1]

#define	ParticleTimer	SpecialF[0]
#define	RomanCandleTimer	SpecialF[1]

#define	BottleRocketTimer	SpecialF[1]

#define	TorpedoTimer		SpecialF[1]

#define	FreezeBombTimer		SpecialF[1]

static const OGLPoint3D	gGunNozzelOff = {0,0,0};

#define	MineArmingTimer		SpecialF[0]


/******************* CHECK POWERUP CONTROLS ************************/
//
//

void CheckPOWControls(ObjNode *theNode)
{
short	playerNum = theNode->PlayerNum;

	if (gPlayerMultiPassCount > 0)							// if doing multipass and on secondary passes, then dont check "New" keys.
		return;

				/***************/
				/* SEE IF FIRE */
				/***************/

	if (GetControlStateNew(playerNum, kControlBit_ThrowForward))
		VehicleActivatePOW(theNode, true);
	else
	if (GetControlStateNew(playerNum, kControlBit_ThrowBackward))
		VehicleActivatePOW(theNode, false);

}


/*************************** VEHICLE ACTIVATE POWERUP **********************************/
//
// Called whenever a player presses the fire button.
//

static void VehicleActivatePOW(ObjNode *theVehicle, Boolean forwardThrow)
{
uint16_t		playerNum = theVehicle->PlayerNum;
short		powType;

			/* FIRST CHECK IF JUST DROP A FLAG */

	if (gGameMode == GAME_MODE_CAPTUREFLAG)
	{
		ObjNode	*theTorch = (ObjNode *)theVehicle->CapturedFlag;				// get torch object that player is carrying

		if (theTorch != nil)										// if have one, then drop it
		{
			PlayerDropFlag(theVehicle);
			return;
		}
	}


			/* PROCESS POWERUP TYPE */

	powType = gPlayerInfo[playerNum].powType;			// get current weapon type

	switch(powType)
	{
		case	POW_TYPE_NONE:
				break;

		case	POW_TYPE_BONE:
		case	POW_TYPE_OIL:
		case	POW_TYPE_BIRDBOMB:
		case	POW_TYPE_FREEZE:
				PlayerThrowsWeapon(playerNum, forwardThrow);
				break;

		case	POW_TYPE_ROMANCANDLE:
				PlayerLaunchRomanCandle(playerNum);
				break;

		case	POW_TYPE_BOTTLEROCKET:
				PlayerLaunchBottleRocket(playerNum, forwardThrow);
				break;

		case	POW_TYPE_TORPEDO:
				PlayerLaunchTorpedo(playerNum);
				break;


		case	POW_TYPE_NITRO:
				ActivateNitroPOW(playerNum);
				break;

		case	POW_TYPE_MINE:
				DropLandMine(playerNum);
				break;

		default:
				DoFatalAlert("VehicleActivatePOW: unknown powType");
	}
}


/******************** ACTIVATE NITRO POW *******************/

void ActivateNitroPOW(short playerNum)
{
	DecCurrentPOWQuantity(playerNum);
	gPlayerInfo[playerNum].nitroTimer += NITRO_DURATION;		// give player more nitro

	PlayEffect_Parms3D(EFFECT_NITRO, &gPlayerInfo[playerNum].coord, NORMAL_CHANNEL_RATE, 1.5);

}


/*********************** PLAYER THROWS WEAPON **************************/
//
// Called to start the throwing animation
//

static void PlayerThrowsWeapon(short playerNum, Boolean forwardThrow)
{
ObjNode	*head;

	head = gPlayerInfo[playerNum].headObj;

	head->HEAD_THROW_READY_FLAG = false;

	if ((head->Skeleton->AnimNum != PLAYER_ANIM_THROWFORWARD) && (head->Skeleton->AnimNum != PLAYER_ANIM_THROWBACKWARD))
	{
		if (forwardThrow)
	   		MorphToSkeletonAnim(head->Skeleton, PLAYER_ANIM_THROWFORWARD, 8.0);
	   	else
	   		MorphToSkeletonAnim(head->Skeleton, PLAYER_ANIM_THROWBACKWARD, 8.0);

	   	if (gPlayerInfo[playerNum].sex)
			PlayEffect_Parms3D(EFFECT_THROW1 + RandomRange(0,2), &gCoord, NORMAL_CHANNEL_RATE + 0x5000, 1.5);
		else
			PlayEffect_Parms3D(EFFECT_THROW1 + RandomRange(0,2), &gCoord, NORMAL_CHANNEL_RATE, 1.5);
	}
}


/*********************** SET THROW DELTAS ************************/

static void SetThrowDeltas(ObjNode *theProjectile, ObjNode *theCar, float force, Boolean throwForward)
{
static const OGLVector3D	throwVector = {0,.4,-1};
static const OGLVector3D	throwVectorBack = {0,.3,1};

	if (throwForward)																	// rotate vector based on car orientation
	{
		OGLVector3D_Transform(&throwVector,	&theCar->BaseTransformMatrix, &theProjectile->Delta);
		theProjectile->Delta.x *= 3300.0f * force;															// give it speed
		theProjectile->Delta.y *= 1400.0f * force;
		theProjectile->Delta.z *= 3300.0f * force;
	}
	else
	{
		OGLVector3D_Transform(&throwVectorBack,	&theCar->BaseTransformMatrix, &theProjectile->Delta);
		theProjectile->Delta.x *= 1800.0f * force;															// give it speed
		theProjectile->Delta.y *= 1000.0f * force;
		theProjectile->Delta.z *= 1800.0f * force;
	}

	theProjectile->Delta.x += theCar->Delta.x;
	theProjectile->Delta.y += theCar->Delta.y;
	theProjectile->Delta.z += theCar->Delta.z;

}


/*********************** SET THROW DELTAS 2 ************************/

static void SetThrowDeltas2(ObjNode *theProjectile, ObjNode *theCar, float force, Boolean throwForward)
{
static const OGLVector3D	throwVector = {0,.6,-1};
static const OGLVector3D	throwVectorBack = {0,.6,1};

	if (throwForward)																	// rotate vector based on car orientation
	{
		OGLVector3D_Transform(&throwVector,	&theCar->BaseTransformMatrix, &theProjectile->Delta);
		theProjectile->Delta.x *= 3300.0f * force;															// give it speed
		theProjectile->Delta.y *= 3300.0f * force;
		theProjectile->Delta.z *= 3300.0f * force;
	}
	else
	{
		OGLVector3D_Transform(&throwVectorBack,	&theCar->BaseTransformMatrix, &theProjectile->Delta);
		theProjectile->Delta.x *= 1800.0f * force;															// give it speed
		theProjectile->Delta.y *= 1000.0f * force;
		theProjectile->Delta.z *= 1800.0f * force;
	}

	theProjectile->Delta.x += theCar->Delta.x;
	theProjectile->Delta.y += theCar->Delta.y;
	theProjectile->Delta.z += theCar->Delta.z;

}


#pragma mark -

/************************* THROW BONE *********************************/

void ThrowBone(short playerNum, Boolean throwForward)
{
ObjNode						*newObj,*head,*car;

	car = gPlayerInfo[playerNum].objNode;
	head = gPlayerInfo[playerNum].headObj;


		/********************/
		/* MAKE BONE OBJECT */
		/********************/

	FindCoordOnJointAtFlagEvent(head, 5, &gGunNozzelOff, &gNewObjectDefinition.coord);

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_BoneBullet;
	gNewObjectDefinition.flags 		= STATUS_BIT_AIMATCAMERA|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_ROTXZY|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+2;
	gNewObjectDefinition.moveCall 	= MoveBoneWeapon;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 3.0, 3.0, true);


		/*****************/
		/* SET IN MOTION */
		/*****************/

	SetThrowDeltas(newObj, car, 1.0, throwForward);

	newObj->WhoThrew = (Ptr)car;														// remember who threw it
}


/********************** MOVE BONE WEAPON **************************/

static void MoveBoneWeapon(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	ApplyFrictionToDeltas(fps * 500.0f, &gDelta);			// air friction

	gDelta.y -= 800.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;								// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.z += fps * 10.0f;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) ||
		DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, (ObjNode *)theNode->WhoThrew))
	{
		MakeBombExplosion(theNode, gCoord.x, gCoord.z, &gDelta);
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);
}


#pragma mark -

/************************* THROW FREEZE *********************************/

void ThrowFreeze(short playerNum, Boolean throwForward)
{
ObjNode						*newObj,*head,*car;

	car = gPlayerInfo[playerNum].objNode;
	head = gPlayerInfo[playerNum].headObj;


		/********************/
		/* MAKE BONE OBJECT */
		/********************/

	FindCoordOnJointAtFlagEvent(head, 5, &gGunNozzelOff, &gNewObjectDefinition.coord);

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_FreezeBullet;
	gNewObjectDefinition.flags 		= STATUS_BIT_AIMATCAMERA|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_GLOW|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= PARTICLE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveFreezeWeapon;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	newObj->WhoThrew = (Ptr)car;														// remember who threw it


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4.0, 4.0, true);


		/*****************/
		/* SET IN MOTION */
		/*****************/

	SetThrowDeltas2(newObj, car, 1.0, throwForward);

	newObj->FreezeBombTimer = 1.7;


	PlayEffect_Parms3D(EFFECT_SNOWBALL, &car->Coord, NORMAL_CHANNEL_RATE-0x4000, 4.0);

}


/********************** MOVE FREEZE WEAPON **************************/

static void MoveFreezeWeapon(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	terrainY;


	GetObjectInfo(theNode);

	ApplyFrictionToDeltas(fps * 500.0f, &gDelta);			// air friction

	gDelta.y -= 2000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;								// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/* SEE IF TIME TO BOOM */

	theNode->FreezeBombTimer -= fps;
	if (theNode->FreezeBombTimer <= 0.0f)
	{
		goto boom;
	}

		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	terrainY = GetTerrainY(gCoord.x, gCoord.z);

	if ((gCoord.y <= terrainY) || DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, (ObjNode *)theNode->WhoThrew))	// see if hit something
	{
boom:
		MakeSnowExplosion(&gCoord);
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);


			/***************/
			/* MAKE SPARKS */
			/***************/

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer < 0.0f)
	{
		int		particleGroup,magicNum;
		NewParticleGroupDefType	groupDef;
		NewParticleDefType	newParticleDef;

		theNode->ParticleTimer += .03f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_BOUNCE;
			groupDef.gravity				= 200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 20.0f;
			groupDef.decayRate				=  1.0;
			groupDef.fadeRate				= 1.0;
			groupDef.particleTextureNum		= PARTICLE_SObjType_SnowFlakes;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float	x,y,z;
			int		i;
			OGLPoint3D	p;
			OGLVector3D	d;

			x = gCoord.x;
			y = gCoord.y;
			z = gCoord.z;

			for (i = 0; i < 8; i++)
			{
				p.x = x + RandomFloat2() * 20.0;
				p.y = y + RandomFloat() * 60.0f;
				p.z = z + RandomFloat2() * 20.0f;

				d.x = RandomFloat2() * 20.0f;
				d.y = RandomFloat2() * 10.0f;
				d.z = RandomFloat2() * 20.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * 10.0f;
				newParticleDef.rotDZ		= RandomFloat2();
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




#pragma mark -

/************************* THROW OIL *********************************/

void ThrowOil(short playerNum, Boolean throwForward)
{
static const OGLVector3D	throwVector = {0,.5,-1};
ObjNode						*newObj,*head,*car;

	car = gPlayerInfo[playerNum].objNode;
	head = gPlayerInfo[playerNum].headObj;


		/***********************/
		/* MAKE OIL PROJECTILE */
		/***********************/

	FindCoordOnJointAtFlagEvent(head, 5, &gGunNozzelOff, &gNewObjectDefinition.coord);

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_OilBullet;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveOilBullet;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2.0, 2.0, true);


		/* SET IN MOTION */

	SetThrowDeltas(newObj, car, .6, throwForward);
	newObj->WhoThrew = (Ptr)car;														// remember who threw it

}


/********************** MOVE OIL BULLET **************************/

static void MoveOilBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	ApplyFrictionToDeltas(fps * 500.0f, &gDelta);			// air friction

	gDelta.y -= 1000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;								// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.z += fps * 10.0f;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) ||
		DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, (ObjNode *)theNode->WhoThrew))
	{
		MakeOilSpill(gCoord.x, gCoord.z);
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);
}



/************************ MAKE OIL SPILL **********************/

static void MakeOilSpill(float x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_OilPatch;
	gNewObjectDefinition.coord.x	= x;
	gNewObjectDefinition.coord.y	= GetTerrainY(x, z);
	gNewObjectDefinition.coord.z	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+9;
	gNewObjectDefinition.moveCall 	= MoveOilSpill;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	gNewObjectDefinition.scale 	    = 3.0f + RandomFloat() * 2.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CType = CTYPE_AVOID;

	RotateOnTerrain(newObj, 5.0, nil);							// set transform matrix
	SetObjectTransformMatrix(newObj);
}


/********************* MOVE OIL SPILL ************************/

static void MoveOilSpill(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;
float	s,left,right,front,back;


			/* SCALE OUT */

	if (theNode->Scale.x < 10.0f)
		theNode->Scale.x = theNode->Scale.z += fps * 1.5f;
	s = theNode->Scale.x;
	RotateOnTerrain(theNode, 5.0, nil);							// set transform matrix
	SetObjectTransformMatrix(theNode);


		/**************************/
		/* SEE IF PLAYER IS ON IT */
		/**************************/

	left = theNode->Coord.x - (30.0f * s);
	right = theNode->Coord.x + (30.0f * s);
	front = theNode->Coord.z + (30.0f * s);
	back = theNode->Coord.z - (30.0f * s);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		ObjNode				*player;
		CollisionBoxType	*collisionBox;

		if (gPlayerInfo[i].distToFloor > 5.0f)						// must be on ground
			continue;

		player = gPlayerInfo[i].objNode;
		if (!player)
			continue;

		if (player->Speed2D < 1600.0f)								// if going too slow, then dont bother
			continue;

		collisionBox = player->CollisionBoxes;						// point to player's collision box

		if (!collisionBox)
			continue;

		if (collisionBox[0].right < left)							// see if rectangles overlap
			continue;
		if (collisionBox[0].left > right)
			continue;
		if (collisionBox[0].front < back)
			continue;
		if (collisionBox[0].back > front)
			continue;

			/* PLAYER HIT THE SPILL */

		gPlayerInfo[i].greasedTiresTimer = 1.3;
		gPlayerInfo[i].isPlaning = true;
	}

}




#pragma mark -

/************************* THROW BIRD BOMB *********************************/

void ThrowBirdBomb(short playerNum, Boolean throwForward)
{
ObjNode		*newObj,*head, *car;
OGLVector2D	aimVec,targetVec;
float		bestAngle,r;
short		p,bestP;

	car = gPlayerInfo[playerNum].objNode;
	head = gPlayerInfo[playerNum].headObj;


		/********************/
		/* MAKE OBJECT */
		/********************/

	FindCoordOnJointAtFlagEvent(head, 5, &gGunNozzelOff, &gNewObjectDefinition.coord);

	gNewObjectDefinition.moveCall 	= MoveBirdBomb;
	gNewObjectDefinition.type 		= SKELETON_TYPE_BIRDBOMB;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+3;

	if (throwForward)
		gNewObjectDefinition.rot 	= car->Rot.y;
	else
		gNewObjectDefinition.rot 	= car->Rot.y + PI;

	gNewObjectDefinition.scale 	    = 1.0;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		DoFatalAlert("ThrowBirdBomb: MakeNewSkeletonObject failed!");

	newObj->Skeleton->AnimSpeed = 2.0f;
	newObj->Delta.y = car->Delta.y + 600.0f;

			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 6.0, 6.0, true);


	newObj->WhoThrew = (Ptr)car;														// remember who threw it


				/**************************/
				/* FIND BEST TARGET AHEAD */
				/**************************/

	r = newObj->Rot.y;														// calc aim vector
	aimVec.x = -sin(r);
	aimVec.y = -cos(r);


	bestAngle = -1.0;
	bestP = -1;

	for (p = 0; p < gNumTotalPlayers; p++)									// check against all players
	{
		float	dot;

		if (p == playerNum)													// dont attack the thrower!
			continue;


			/* SEE IF AHEAD */

		targetVec.x = gPlayerInfo[p].coord.x - gPlayerInfo[playerNum].coord.x;		// calc vec from car to target
		targetVec.y = gPlayerInfo[p].coord.z - gPlayerInfo[playerNum].coord.z;
		FastNormalizeVector2D(targetVec.x, targetVec.y, &targetVec, false);

		dot = OGLVector2D_Dot(&aimVec, &targetVec);							// calc angle between
		if (dot < 0.0f)														// see if not aiming at it enough
			continue;
		if (dot < bestAngle)												// see if best aim so far
			continue;

			/* THIS IS THE BEST PICK SO FAR */

		bestAngle = dot;
		bestP = p;
	}

	newObj->TargetPlayer = bestP;

}


/********************** MOVE BIRD BOMB **************************/

static void MoveBirdBomb(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
float		r,dx,dz;
short		p = theNode->TargetPlayer;
float		y;
ObjNode		*who = (ObjNode *)theNode->WhoThrew;

	GetObjectInfo(theNode);




			/*************************/
			/* MOVE AFTER THE TARGET */
			/*************************/

				/* AIM AT TARGET */

	if (p != -1)
		TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo[p].coord.x, gPlayerInfo[p].coord.z, PI2, false);

	r = theNode->Rot.y;														// calc new aim vector
	dx = -sin(r);
	dz = -cos(r);

	gDelta.x = dx * BIRD_FLY_SPEED;
	gDelta.z = dz * BIRD_FLY_SPEED;

	y = GetTerrainY(gCoord.x, gCoord.z) + 250.0f;						// get target y

	gDelta.y += (y - gCoord.y) * 2.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF HIT GROUND */

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		MakeBombExplosion((ObjNode *)theNode->WhoThrew, gCoord.x, gCoord.z, &gDelta);
		DeleteObject(theNode);
		return;
	}


			/* IF CLOSE TO ANY PLAYER, THEN BOOM */

	for (p = 0; p < gNumTotalPlayers; p++)									// check against all players
	{
		if (p == who->PlayerNum)											// dont attack the thrower!
			continue;

		if (CalcDistance3D(gCoord.x, gCoord.y, gCoord.z, gPlayerInfo[p].coord.x,  gPlayerInfo[p].coord.y, gPlayerInfo[p].coord.z) < 220.0f)
		{
			MakeBombExplosion((ObjNode *)theNode->WhoThrew, gCoord.x, gCoord.z, &gDelta);
			DeleteObject(theNode);
			return;
		}
	}

	UpdateObject(theNode);


			/************************/
			/* UPDATE SOUND EFFECT */
			/************************/

	if (theNode->EffectChannel != -1)
		Update3DSoundChannel(EFFECT_BIRDCAW, &theNode->EffectChannel, &gCoord);
	else
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BIRDCAW, &gCoord, NORMAL_CHANNEL_RATE, 1.0);

}


#pragma mark -

/**************** PLAYER LAUNCH ROMAN CANDLE ************************/

static void PlayerLaunchRomanCandle(short playerNum)
{
ObjNode		*newObj, *car;

	DecCurrentPOWQuantity(playerNum);

	car = gPlayerInfo[playerNum].objNode;


		/********************/
		/* MAKE OBJECT 		*/
		/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_RomanCandleBullet;
	gNewObjectDefinition.coord.x	= car->Coord.x;
	gNewObjectDefinition.coord.y	= car->Coord.y + 100.0f;
	gNewObjectDefinition.coord.z	= car->Coord.z;
	gNewObjectDefinition.flags 		= STATUS_BIT_AIMATCAMERA|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_GLOW|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= PARTICLE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveRomanCandle;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	newObj->WhoThrew = (Ptr)car;														// remember who threw it

	newObj->Delta.y = 3000.0f;

	newObj->Mode = 0;																	// shoot up

	PlayEffect3D(EFFECT_ROMANCANDLE_LAUNCH, &gNewObjectDefinition.coord);
}



/******************* MOVE ROMAN CANDLE ****************************/

static void MoveRomanCandle(ObjNode *theNode)
{
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
float fps = gFramesPerSecondFrac;
ObjNode		*car;
Boolean		makeSparks = false;
short		targetP;
float		dist;

	car = (ObjNode *)theNode->WhoThrew;			// point to car who shot it


	switch(theNode->Mode)
	{
				/***************/
				/* SHOOTING UP */
				/***************/

		case	0:
				GetObjectInfo(theNode);
				gCoord.y += gDelta.y * fps;

				if (gCoord.y > (car->Coord.y + 5000.0f))	// see if nice and high now
				{
					theNode->Mode = 1;						// go into coast mode
					theNode->RomanCandleTimer = 1.0;
					theNode->StatusBits |= STATUS_BIT_HIDDEN;	// hide while coasting
				}
				else
					makeSparks = true;
				UpdateObject(theNode);
				break;

					/*********/
					/* COAST */
					/*********/

		case	1:
				theNode->RomanCandleTimer -= fps;
				if (theNode->RomanCandleTimer <= 0.0f)
				{
					theNode->Mode = 2;							// fall
					theNode->StatusBits &= ~STATUS_BIT_HIDDEN;	// hide while coasting

					targetP = FindClosestPlayer(car, car->Coord.x, car->Coord.z, ROMAN_CANDLE_RANGE, true, &dist);	// see who's closest
					if (targetP == -1)							// if nobody else to hit, then hit ourselves
						targetP = car->PlayerNum;

					theNode->Coord.x = gPlayerInfo[targetP].objNode->Coord.x;
					theNode->Coord.y = gPlayerInfo[targetP].objNode->Coord.y + 4000.0f;
					theNode->Coord.z = gPlayerInfo[targetP].objNode->Coord.z;

					theNode->Delta.x = gPlayerInfo[targetP].objNode->Delta.x;	// match target's dx,dz
					theNode->Delta.z = gPlayerInfo[targetP].objNode->Delta.z;
					theNode->Delta.y = -4000;

					theNode->EffectChannel = PlayEffect3D(EFFECT_ROMANCANDLE_FALL, &gNewObjectDefinition.coord);

				}
				break;


				/****************/
				/* FALLING DOWN */
				/****************/

		case	2:
				makeSparks = true;
				GetObjectInfo(theNode);
				gCoord.x += gDelta.x * fps;
				gCoord.y += gDelta.y * fps;
				gCoord.z += gDelta.z * fps;

				if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) || DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, car))
				{
					ExplodeRomanCandle(theNode, &gCoord);
					DeleteObject(theNode);
					return;
				}

				UpdateObject(theNode);

				if (theNode->EffectChannel != -1)
					Update3DSoundChannel(EFFECT_ROMANCANDLE_FALL, &theNode->EffectChannel, &theNode->Coord);

				break;


	}


			/***************/
			/* MAKE SPARKS */
			/***************/

	if (makeSparks)
	{
		theNode->ParticleTimer -= fps;
		if (theNode->ParticleTimer < 0.0f)
		{
			theNode->ParticleTimer += .03f;

			particleGroup 	= theNode->ParticleGroup;
			magicNum 		= theNode->ParticleMagicNum;

			if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
			{
				theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

				groupDef.magicNum				= magicNum;
				groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
				groupDef.flags					= PARTICLE_FLAGS_BOUNCE;
				groupDef.gravity				= 200;
				groupDef.magnetism				= 0;
				groupDef.baseScale				= 20.0f;
				groupDef.decayRate				=  1.0;
				groupDef.fadeRate				= 1.0;
				groupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
				groupDef.srcBlend				= GL_SRC_ALPHA;
				groupDef.dstBlend				= GL_ONE;
				theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
			}

			if (particleGroup != -1)
			{
				float	x,y,z;
				int		i;
				OGLPoint3D	p;
				OGLVector3D	d;

				x = gCoord.x;
				y = gCoord.y;
				z = gCoord.z;

				for (i = 0; i < 8; i++)
				{
					p.x = x + RandomFloat2() * 30.0;
					p.y = y + RandomFloat2() * 80.0f;
					p.z = z + RandomFloat2() * 30.0f;

					d.x = RandomFloat2() * 30.0f;
					d.y = RandomFloat2() * 20.0f;
					d.z = RandomFloat2() * 30.0f;

					newParticleDef.groupNum		= particleGroup;
					newParticleDef.where		= &p;
					newParticleDef.delta		= &d;
					newParticleDef.scale		= RandomFloat() + 1.0f;
					newParticleDef.rotZ			= RandomFloat() * 10.0f;
					newParticleDef.rotDZ		= RandomFloat2();
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


/******************** EXPLODE ROMAN CANDLE ****************************/

static void ExplodeRomanCandle(ObjNode *theBullet, const OGLPoint3D *where)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
ObjNode					*whoThrew;

		/*********************/
		/* MAKE WHITE SPARKS */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				= 1.0;
	gNewParticleGroupDef.fadeRate				= 0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 200; i++)
		{
			pt.x = where->x + RandomFloat2() * 60.0f;
			pt.y = where->y + 60.0 + RandomFloat2() * 60.0f;
			pt.z = where->z + RandomFloat2() * 60.0f;

			d.y = RandomFloat2() * 800.0f;
			d.x = RandomFloat2() * 800.0f;
			d.z = RandomFloat2() * 800.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}


		/*********************/
		/* MAKE RED SPARKS */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				= 1.0;
	gNewParticleGroupDef.fadeRate				= 0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_RedSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 200; i++)
		{
			pt.x = where->x + RandomFloat2() * 60.0f;
			pt.y = where->y + 60.0 + RandomFloat2() * 60.0f;
			pt.z = where->z + RandomFloat2() * 60.0f;

			d.y = RandomFloat2() * 1000.0f;
			d.x = RandomFloat2() * 1000.0f;
			d.z = RandomFloat2() * 1000.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 30.0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}




	PlayEffect_Parms3D(EFFECT_BOOM, where, NORMAL_CHANNEL_RATE*3/2, 4);


		/* SEE IF IT HIT ANY CARS */

	whoThrew = (ObjNode *)theBullet->WhoThrew;
	BlastCars(whoThrew->PlayerNum, where->x, where->y, where->z, BOTTLEROCKET_BLAST_RADIUS);

}



#pragma mark -

/**************** PLAYER LAUNCH BOTTLE ROCKET ************************/

static void PlayerLaunchBottleRocket(short playerNum, Boolean forwardThrow)
{
ObjNode		*newObj, *car;
float		r,dist;
short		targetP;

	DecCurrentPOWQuantity(playerNum);

	car = gPlayerInfo[playerNum].objNode;


		/********************/
		/* MAKE OBJECT 		*/
		/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_BottleRocketBullet;
	gNewObjectDefinition.coord.x	= car->Coord.x;
	gNewObjectDefinition.coord.y	= car->Coord.y + 120.0f;
	gNewObjectDefinition.coord.z	= car->Coord.z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveBottleRocket;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	newObj->WhoThrew = (Ptr)car;														// remember who threw it
	newObj->BottleRocketTimer = 4.0;

	newObj->Rot.x = -PI/2.7f;
	newObj->DeltaRot.x = 1.0f;

		/* FIND TARGET TO SHOOT AT */

	if (forwardThrow)
		targetP = FindClosestPlayerInFront(car, BOTTLE_ROCKET_RANGE, true, &dist, 0);	// see who's closest
	else
		targetP = FindClosestPlayerInBack(car, BOTTLE_ROCKET_RANGE, true, &dist, 0);	// see who's closest

	newObj->TargetPlayer = targetP;
	if (targetP == -1)										// if nobody else to hit then just shoot straight
	{
		r = newObj->Rot.y = car->Rot.y;
		if (!forwardThrow)
			r += PI;

		newObj->Delta.x = gDelta.x - sin(r) * BOTTLE_ROCKET_SPEED;
		newObj->Delta.z = gDelta.z - cos(r) * BOTTLE_ROCKET_SPEED;
		newObj->Rot.x = -PI/3;
	}
	else
	{
		r = CalcYAngleFromPointToPoint(car->Rot.y, car->Coord.x, car->Coord.z, gPlayerInfo[targetP].coord.x, gPlayerInfo[targetP].coord.z);
		newObj->Rot.y = r;

		newObj->Delta.x = gDelta.x - sin(r) * BOTTLE_ROCKET_SPEED;
		newObj->Delta.z = gDelta.z - cos(r) * BOTTLE_ROCKET_SPEED;
	}

	newObj->Delta.y = gDelta.y + 2000.0f;


	PlayEffect3D(EFFECT_ROMANCANDLE_LAUNCH, &gNewObjectDefinition.coord);
}


/******************* MOVE BOTTLE ROCKET ****************************/

static void MoveBottleRocket(ObjNode *theNode)
{
int				particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
float 			fps = gFramesPerSecondFrac;
ObjNode			*car;
static const OGLVector3D up = {0,1,0};
OGLMatrix4x4	m;
short			targetP;

	GetObjectInfo(theNode);
	car = (ObjNode *)theNode->WhoThrew;			// point to car who shot it

	theNode->BottleRocketTimer -= fps;						// see if time to explode
	if (theNode->BottleRocketTimer <= 0.0f)
		goto boom;


		/* TURN TOWARD TARGET */

	targetP = theNode->TargetPlayer;
	if (targetP != -1)
		TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo[targetP].coord.x, gPlayerInfo[targetP].coord.z, PI, false);


		/* TILT DOWN */

	theNode->DeltaRot.x -= fps * 1.0f;
	if (theNode->DeltaRot.x < 0.0f)
		theNode->DeltaRot.x = 0;

	theNode->Rot.x -= theNode->DeltaRot.x * fps;
	if (theNode->Rot.x < -PI)
		theNode->Rot.x = -PI;



		/* DO PHYSICS */

	OGLMatrix4x4_SetRotate_XYZ(&m, theNode->Rot.x, theNode->Rot.y, theNode->Rot.z);
	OGLVector3D_Transform(&up, &m, &gDelta);

	gDelta.x *= BOTTLE_ROCKET_SPEED;
	gDelta.y *= BOTTLE_ROCKET_SPEED;
	gDelta.z *= BOTTLE_ROCKET_SPEED;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

		/* SEE IF HIT */

	if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) || DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, car))
	{
boom:
		ExplodeBottleRocket(theNode, &gCoord);
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);



			/**************/
			/* MAKE SMOKE */
			/**************/

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer < 0.0f)
	{
		theNode->ParticleTimer += .03f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_BOUNCE;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 20.0f;
			groupDef.decayRate				=  -.4;
			groupDef.fadeRate				= 1.5;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float	x,y,z;
			int		i;
			OGLPoint3D	p;
			OGLVector3D	d;

			x = gCoord.x;
			y = gCoord.y;
			z = gCoord.z;

			for (i = 0; i < 4; i++)
			{
				p.x = x + RandomFloat2() * 20.0;
				p.y = y + RandomFloat2() * 20.0f;
				p.z = z + RandomFloat2() * 20.0f;

				d.x = RandomFloat2() * 20.0f;
				d.y = RandomFloat2() * 20.0f;
				d.z = RandomFloat2() * 20.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * 4.0f;
				newParticleDef.rotDZ		= RandomFloat2();
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


/******************** EXPLODE BOTTLE ROCKET ****************************/

static void ExplodeBottleRocket(ObjNode *theBullet, const OGLPoint3D *where)
{
long					pg;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
ObjNode					*whoThrew;

		/*********************/
		/* MAKE RED SPARKS */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				= 1.0;
	gNewParticleGroupDef.fadeRate				= 0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_RedSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (int i = 0; i < 230; i++)
		{
			pt.x = where->x + RandomFloat2() * 60.0f;
			pt.y = where->y + 60.0f + RandomFloat2() * 60.0f;
			pt.z = where->z + RandomFloat2() * 60.0f;

			d.y = RandomFloat2() * 800.0f;
			d.x = RandomFloat2() * 800.0f;
			d.z = RandomFloat2() * 800.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}


		/*********************/
		/* MAKE BLUE SPARKS */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				=  1.0;
	gNewParticleGroupDef.fadeRate				= 0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_BlueSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (int i = 0; i < 230; i++)
		{
			pt.x = where->x + RandomFloat2() * 60.0f;
			pt.y = where->y + 60.0f + RandomFloat2() * 60.0f;
			pt.z = where->z + RandomFloat2() * 60.0f;

			d.y = RandomFloat2() * 1000.0f;
			d.x = RandomFloat2() * 1000.0f;
			d.z = RandomFloat2() * 1000.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 30.0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}




	PlayEffect_Parms3D(EFFECT_BOOM, where, NORMAL_CHANNEL_RATE*3/2, 4);


		/* SEE IF IT HIT ANY CARS */

	whoThrew = (ObjNode *)theBullet->WhoThrew;
	BlastCars(whoThrew->PlayerNum, where->x, where->y, where->z, BOTTLEROCKET_BLAST_RADIUS);

}


#pragma mark -

/**************** PLAYER LAUNCH TORPEDO ************************/

static void PlayerLaunchTorpedo(short playerNum)
{
ObjNode		*newObj, *car;
static const OGLPoint3D	nose = {0,0,-100};

	DecCurrentPOWQuantity(playerNum);

	car = gPlayerInfo[playerNum].objNode;


		/********************/
		/* MAKE OBJECT 		*/
		/********************/

	OGLPoint3D_Transform(&nose, &car->BaseTransformMatrix, &gNewObjectDefinition.coord);

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_Torpedo;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveTorpedo;
	gNewObjectDefinition.rot 		= car->Rot.y;
	gNewObjectDefinition.scale 	    = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	newObj->WhoThrew = (Ptr)car;														// remember who threw it
	newObj->TorpedoTimer = 10.0;

	newObj->Rot = car->Rot;																// match rotation



	PlayEffect_Parms3D(EFFECT_TORPEDOFIRE, &gNewObjectDefinition.coord, NORMAL_CHANNEL_RATE, 3);

}


/******************* MOVE TORPEDO ****************************/

static void MoveTorpedo(ObjNode *theNode)
{
float 			fps = gFramesPerSecondFrac;
ObjNode			*car;
static const OGLVector3D go = {0,0,-1};
OGLMatrix4x4	m;
short			targetP;
float			ydiff,targetRotX,dist;
OGLVector3D		aimVec;
OGLPoint3D		bubblePt;


	GetObjectInfo(theNode);
	car = (ObjNode *)theNode->WhoThrew;						// point to car who shot it



	theNode->TorpedoTimer -= fps;						// see if time to explode
	if (theNode->TorpedoTimer <= 0.0f)
		goto boom;


		/* FIND CLOSES TARGET TO AIM AT */

	targetP = theNode->TargetPlayer = FindClosestPlayerInFront(theNode, TORPEDO_RANGE, true, &dist, .1);					// see who's closest


		/* TURN TOWARD TARGET */

	if (targetP != -1)
	{
		TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo[targetP].coord.x, gPlayerInfo[targetP].coord.z, PI, false);

		ydiff = gPlayerInfo[targetP].coord.y - gCoord.y;				// see if @ same y
		if (fabs(ydiff) < 50.0f)
		{
			targetRotX = 0;
		}
		else
		if (ydiff > 0.0f)
		{
			targetRotX = .5;
		}
		else
			targetRotX = -.5;

		if (theNode->Rot.x < targetRotX)
		{
			theNode->Rot.x += fps * 1.0f;
			if (theNode->Rot.x > targetRotX)
				theNode->Rot.x = targetRotX;
		}
		else
		if (theNode->Rot.x > targetRotX)
		{
			theNode->Rot.x -= fps * 1.0f;
			if (theNode->Rot.x < targetRotX)
				theNode->Rot.x = targetRotX;
		}

	}


		/* DO MOTION */

	OGLMatrix4x4_SetRotate_XYZ(&m, theNode->Rot.x, theNode->Rot.y, theNode->Rot.z);
	OGLVector3D_Transform(&go, &m, &aimVec);

	gDelta.x = aimVec.x * TORPEDO_SPEED;
	gDelta.y = aimVec.y * TORPEDO_SPEED;
	gDelta.z = aimVec.z * TORPEDO_SPEED;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/* SEE IF HIT */

	if ((gCoord.y <= GetTerrainY(gCoord.x, gCoord.z)) || DoSimplePointCollision(&gCoord, CTYPE_MISC|CTYPE_PLAYER, car))
	{
boom:
		ExplodeTorpedo(theNode, &gCoord);
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);



			/***************/
			/* MAKE BUBBLES */
			/***************/

	bubblePt.x = gCoord.x - aimVec.x * 200.0f;
	bubblePt.y = gCoord.y - aimVec.y * 200.0f;
	bubblePt.z = gCoord.z - aimVec.z * 200.0f;

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer < 0.0f)
	{
		theNode->ParticleTimer += .03f;

		MakeBubbles(theNode, &bubblePt, .15, 2.5);
	}

			/************************/
			/* UPDATE TORPEDO EFFECT */
			/************************/

	if (theNode->EffectChannel != -1)
		Update3DSoundChannel(EFFECT_HUM, &theNode->EffectChannel, &gCoord);
	else
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_HUM, &gCoord, NORMAL_CHANNEL_RATE + 0x2000, 1.0);

}


/******************** EXPLODE TORPEDO ****************************/

static void ExplodeTorpedo(ObjNode *theBullet, const OGLPoint3D *where)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
ObjNode					*whoThrew;

		/****************/
		/* MAKE BUBBLES */
		/****************/

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
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 100; i++)
		{
			pt.x = where->x + RandomFloat2() * 160.0f;
			pt.y = where->y + RandomFloat2() * 160.0f;
			pt.z = where->z + RandomFloat2() * 160.0f;

			d.y = 800;
			d.x = 0;
			d.z = 0;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}


		/*********************/
		/* MAKE WHITE SPARKS */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 0;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				= 0;
	gNewParticleGroupDef.fadeRate				= 2.0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 200; i++)
		{
			pt.x = where->x + RandomFloat2() * 60.0f;
			pt.y = where->y + RandomFloat2() * 60.0f;
			pt.z = where->z + RandomFloat2() * 60.0f;

			d.y = RandomFloat2() * 500.0f;
			d.x = RandomFloat2() * 500.0f;
			d.z = RandomFloat2() * 500.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}




	PlayEffect_Parms3D(EFFECT_BOOM, where, NORMAL_CHANNEL_RATE/2, 4);


		/* SEE IF IT HIT ANY CARS */

	whoThrew = (ObjNode *)theBullet->WhoThrew;
	BlastCars(whoThrew->PlayerNum, where->x, where->y, where->z, TORPEDO_BLAST_RADIUS);

}


#pragma mark -


/************************* DROP LAND MINE *********************************/

static void DropLandMine(short playerNum)
{
ObjNode						*newObj;

	DecCurrentPOWQuantity(playerNum);

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_LandMine;
	gNewObjectDefinition.coord.x	= gCoord.x;
	gNewObjectDefinition.coord.y	= GetTerrainY(gCoord.x, gCoord.z);
	gNewObjectDefinition.coord.z	= gCoord.z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveLandMine;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_AVOID;
	newObj->Kind		 	= TRIGTYPE_LANDMINE;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	RotateOnTerrain(newObj, 0, nil);							// set transform matrix
	SetObjectTransformMatrix(newObj);

	newObj->MineArmingTimer = 1.0;							// arming timer

	PlayEffect_Parms3D(EFFECT_MINE, &newObj->Coord, NORMAL_CHANNEL_RATE, 2);

}


/********************* MOVE LAND MINE ************************/

static void MoveLandMine(ObjNode *theNode)
{


			/***************/
			/* SEE IF GONE */
			/***************/

	if (TrackTerrainItem(theNode) && (theNode->MineArmingTimer < -30.0f))		// see if out of range and timed out
	{
		DeleteObject(theNode);
		return;
	}

	theNode->MineArmingTimer -= gFramesPerSecondFrac;		// dec arming timer

}


/************** DO TRIGGER - LAND MINE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_LandMine(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{

	sideBits;

	if (theNode->MineArmingTimer > 0.0f)					// do nothing if not armed yet
		return(false);

		/* MAKE CAR SPIN WILDLY */

	whoNode->DeltaRot.y = RandomFloat2() * 20.0f;
	whoNode->DeltaRot.z = RandomFloat2() * 10.0f;


		/* MAKE EXPLOSION */

	MakeSparkExplosion(theNode->Coord.x, theNode->Coord.y+50.0f, theNode->Coord.z,
						whoNode->Speed2D * .1f, PARTICLE_SObjType_Fire);

	MakeConeBlast(theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);

	PlayEffect_Parms3D(EFFECT_BOOM, &theNode->Coord, NORMAL_CHANNEL_RATE + 0x1000, 6);


			/* DELETE THE MINE */

	DeleteObject(theNode);

	gDelta.y += 1400.0f;				// pop up the guy who hit the it
	gDelta.x *= .3f;					// slow
	gDelta.z *= .3f;

	return(false);
}










