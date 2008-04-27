/*******************************/
/*   	ME BUG.C			   */
/* (c)1999 Pangea Software     */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <textutils.h>

#include "globals.h"
#include "3dmath.h"
#include "objects.h"
#include "misc.h"
#include "skeletonanim.h"
#include "collision.h"
#include "player_control.h"
#include "sound2.h"
#include "main.h"
#include "file.h"
#include "input.h"
#include "player.h"
#include "effects.h"
#include "mobjtypes.h"
#include "skeletonObj.h"
#include "skeletonJoints.h"
#include "camera.h"
#include "bones.h"
#include "triggers.h"
#include "enemy.h"
#include "items.h"
#include "mytraps.h"

extern	Byte					gCurrentLiquidType;
extern	ObjNode					*gFirstNodePtr,*gCurrentPlayer;
extern	float					gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLPoint3D				gCoord;
extern	OGLVector3D				gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	unsigned long 			gInfobarUpdateBits;
extern	short					gNumCollisions,gCurrentPlayerNum;
extern	CollisionRec			gCollisionList[];
extern	PrefsType				gGamePrefs;
extern	PlayerInfoType			gPlayerInfo[MAX_PLAYERS];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void UpdatePlayer_Bug(void);
static void MovePlayer_Bug(ObjNode *theNode);
static void MovePlayerBug_Stand(void);
static void MovePlayerBug_Walk(void);
static void DoPlayerControl_Bug(float slugFactor);
static void MovePlayerBug_RollUp(void);
static void MovePlayerBug_UnRoll(void);
static void MovePlayerBug_Kick(void);
static void MovePlayerBug_Jump(void);
static void MovePlayerBug_Fall(void);
static void MovePlayerBug_Land(void);

static void DoBugKick(void);
static void	AimAtClosestKickableObject(void);
static void	AimAtClosestBoppableObject(void);
static void PlayerLeaveRootSwing(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	PLAYER_BUG_SCALE			1.7f


#define	PLAYER_BUG_ACCEL			30.0f
#define	PLAYER_BUG_FRICTION_ACCEL	600.0f
#define PLAYER_BUG_SUPERFRICTION	2000.0f

#define	PLAYER_BUG_JUMPFORCE		2000.0f

#define LIMB_NUM_RIGHT_TOE_TIP		3					// joint # of right toe tip

#define	MY_KICK_ENEMY_DAMAGE		.4f


/*********************/
/*    VARIABLES      */
/*********************/

#define	KickNow			Flag[0]				// set during kick anim

#define RippleTimer		SpecialF[0]

/*************************** INIT PLAYER: BUG ****************************/
//
// Creates an ObjNode for the player in the Bug form.
//
// INPUT:
//			where = floor coord where to init the player.
//			rotY = rotation to assign to player if oldObj is nil.
//

ObjNode *InitPlayer_Bug(int playerNum, OGLPoint3D *where, float rotY, int animNum)
{
ObjNode	*newObj;

			/* CREATE MY CHARACTER */

	gNewObjectDefinition.type 		= SKELETON_TYPE_ME;
	gNewObjectDefinition.animNum 	= animNum;
	gNewObjectDefinition.coord.x 	= where->x;
	gNewObjectDefinition.coord.y 	= where->y;
	gNewObjectDefinition.coord.z 	= where->z;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MovePlayer_Bug;
	gNewObjectDefinition.rot 		= rotY;
	gNewObjectDefinition.scale 		= PLAYER_BUG_SCALE;
	newObj 							= MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->PlayerNum = playerNum;

				/* SET COLLISION INFO */

	newObj->CType = CTYPE_PLAYER;
	newObj->CBits = CBITS_TOUCHABLE;

		/* note: box must be same for both bug & ball to avoid collison fallthru against solids */

	CreateCollisionBoxFromBoundingBox(newObj);


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, 4.0, 4.0, true);


				/* SET GLOBALS */

	gPlayerInfo[playerNum].objNode 	= newObj;
	gPlayerInfo[playerNum].mode  	= PLAYER_MODE_BUG;

	return(newObj);
}



/******************** MOVE PLAYER: BUG ***********************/

static void MovePlayer_Bug(ObjNode *theNode)
{
	const static void(*myMoveTable[])(void) =
	{
		MovePlayerBug_Stand,
		MovePlayerBug_Walk,
		MovePlayerBug_RollUp,
		MovePlayerBug_UnRoll,
		MovePlayerBug_Kick,
		MovePlayerBug_Jump,
		MovePlayerBug_Fall,
		MovePlayerBug_Land
	};

	gCurrentPlayerNum = theNode->PlayerNum;			// get player #
	gCurrentPlayer = gPlayerInfo[gCurrentPlayerNum].objNode;

	GetObjectInfo(theNode);


			/* UPDATE INVINCIBILITY */

	if (theNode->InvincibleTimer > 0.0f)
	{
		theNode->InvincibleTimer -= gFramesPerSecondFrac;
		if (theNode->InvincibleTimer < 0.0f)
			theNode->InvincibleTimer = 0;
	}

	theNode->Rot.x = theNode->Rot.z = 0;		// hack to fix a problem when ball hits water and player gets stuck at strange rotation.


			/* JUMP TO HANDLER */

	myMoveTable[theNode->Skeleton->AnimNum]();
}


/******************** MOVE PLAYER BUG: STAND ***********************/

static void MovePlayerBug_Stand()
{

			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;


			/* SEE IF SHOULD WALK */

	if (gCurrentPlayer->Skeleton->AnimNum == PLAYER_ANIM_STAND)
	{
		if (gCurrentPlayer->Speed2D > 1.0f)
			MorphToSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_WALK, 9);
	}

			/* DO CONTROL */
			//
			// do this last since we want any jump command to work smoothly
			//

	DoPlayerControl_Bug(1.0);




			/* UPDATE IT */

update:
	UpdatePlayer_Bug();
}



/******************** MOVE PLAYER BUG: WALK ***********************/

static void MovePlayerBug_Walk(void)
{

			/* DO CONTROL */

	DoPlayerControl_Bug(1.0);


			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;


			/* SEE IF SHOULD STAND */

	if (gCurrentPlayer->Skeleton->AnimNum == PLAYER_ANIM_WALK)
	{
		if (gCurrentPlayer->Speed2D < 1.0f)
			MorphToSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_STAND, 6.0);
		else
			gCurrentPlayer->Skeleton->AnimSpeed = gCurrentPlayer->Speed2D * .006f;
	}



			/* UPDATE IT */

update:
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: ROLLUP ***********************/

static void MovePlayerBug_RollUp(void)
{
}


/******************** MOVE PLAYER BUG: UNROLL ***********************/

static void MovePlayerBug_UnRoll(void)
{
}



/******************** MOVE PLAYER BUG: KICK ***********************/

static void MovePlayerBug_Kick(void)
{

			/* AIM AT CLOSEST KICKABLE ITEM */

	AimAtClosestKickableObject();


			/* SEE IF PERFORM KICK */

	if (gCurrentPlayer->KickNow)
	{
		gCurrentPlayer->KickNow = false;
		DoBugKick();
	}

			/* SEE IF ANIM IS DONE */

	if (gCurrentPlayer->Skeleton->AnimHasStopped)
	{
		SetSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_STAND);
	}


			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_SUPERFRICTION*2);						// do super friction to stop
	if (DoPlayerMovementAndCollision(true))
		goto update;

			/* UPDATE IT */

update:
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: JUMP ***********************/

static void MovePlayerBug_Jump(void)
{

			/* AIM AT CLOSEST BOPPABLE ITEM */

	AimAtClosestBoppableObject();

			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;

			/* SEE IF HIT GROUND */

	if (gCurrentPlayer->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.y = 0;
		if (gCurrentPlayer->Skeleton->AnimNum == PLAYER_ANIM_JUMP)						// make sure still in jump anim
			MorphToSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_LAND, 7);
	}

			/* SEE IF FALLING YET */
	else
	if (gDelta.y < 900.0f)
	{
		if (gCurrentPlayer->Skeleton->AnimNum == PLAYER_ANIM_JUMP)						// make sure still in jump anim
			MorphToSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_FALL, 4);
	}

				/* LET PLAYER HAVE SOME CONTROL */

	DoPlayerControl_Bug(1.0);


			/* UPDATE IT */

update:
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: FALL ***********************/

static void MovePlayerBug_Fall(void)
{

			/* AIM AT CLOSEST BOPPABLE ITEM */

		AimAtClosestBoppableObject();


			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;

			/* SEE IF HIT GROUND */

	if (gCurrentPlayer->Skeleton->AnimNum == PLAYER_ANIM_FALL)			// make sure still in this anim
		if (gCurrentPlayer->StatusBits & STATUS_BIT_ONGROUND)
		{
			MorphToSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_LAND, 9);
			gDelta.x *= .5f;									// slow when landing like this
			gDelta.z *= .5f;
		}

				/* LET PLAYER HAVE SOME CONTROL */

	DoPlayerControl_Bug(1.0);

			/* UPDATE IT */

update:
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: LAND ***********************/

static void MovePlayerBug_Land(void)
{
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(true))
		goto update;


			/* SEE IF DONE */

	if (!gCurrentPlayer->Skeleton->IsMorphing)
	{
		MorphToSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_STAND, 6);
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Bug();
}




#pragma mark -

/************************ UPDATE PLAYER: BUG ***************************/

static void UpdatePlayer_Bug(void)
{

			/* UPDATE THE NODE */

	gPlayerInfo[gCurrentPlayerNum].coord = gCoord;
	UpdateObject(gCurrentPlayer);
}




/**************** DO PLAYER CONTROL: BUG ***************/
//
// Moves a player based on its control bit settings.
// These settings are already set either by keyboard interpretation or reading off the network.
//
// INPUT:	theNode = the node of the player
//			slugFactor = how much of acceleration to apply (varies if jumping et.al)
//


static void DoPlayerControl_Bug(float slugFactor)
{
float	fps = gFramesPerSecondFrac;
int		anim;
Boolean	isOnGround,isSwimming;

			/* GET SOME INFO */

	anim = gCurrentPlayer->Skeleton->AnimNum;				// get anim #

	if ((gCurrentPlayer->StatusBits & STATUS_BIT_ONGROUND) || (gPlayerInfo[gCurrentPlayerNum].distToFloor < 5.0f))
		isOnGround = true;
	else
		isOnGround = false;

	isSwimming = (anim == PLAYER_ANIM_SWIM);



//	gCurrentPlayer->AccelVector.y = mouseDY * PLAYER_BUG_ACCEL * slugFactor;
//	gCurrentPlayer->AccelVector.x = mouseDX * PLAYER_BUG_ACCEL * slugFactor;


			/***************/
			/* SEE IF JUMP */
			/***************/

	if (((!isOnGround) && isSwimming) ||									// if swimming...
		((anim == PLAYER_ANIM_STAND) || (anim == PLAYER_ANIM_WALK) ||	// or in one of these anims & on the ground
		(anim == PLAYER_ANIM_SWIM)))
	{
    	if (GetControlState(gCurrentPlayerNum, kControlBit_Jump))
		{
			MorphToSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_JUMP, 9);
				gDelta.y = PLAYER_BUG_JUMPFORCE;
		}
	}
}


/******************** DO BUG KICK **********************/

static void DoBugKick(void)
{
static const OGLPoint3D offsetCoord = {0,-10,-50};
OGLPoint3D	impactCoord;
int			i;


			/* GET WORLD-COORD TO TEST */

	FindCoordOnJoint(gCurrentPlayer, BUG_LIMB_NUM_PELVIS, &offsetCoord, &impactCoord);


		/* DO COLLISION DETECTION ON KICKABLE THINGS */

	if (DoSimpleBoxCollision(impactCoord.y + 40.0f, impactCoord.y - 40.0f,
							impactCoord.x - 40.0f, impactCoord.x + 40.0f,
							impactCoord.z + 40.0f, impactCoord.z - 40.0f,
							CTYPE_KICKABLE))
	{

		for (i = 0; i < gNumCollisions; i++)
		{
			ObjNode *kickedObj;

			kickedObj = gCollisionList[i].objectPtr;				// get objnode of kicked object


					/* HANDLE SPECIFICS */

			if (kickedObj->Genre == DISPLAY_GROUP_GENRE)
			{
				switch(kickedObj->Group)
				{
					case	MODEL_GROUP_GLOBAL:
							switch(kickedObj->Type)
							{
//								case	GLOBAL1_MObjType_Nut:			// NUT
//										KickNut(gPlayerObj, kickedObj);
//										break;

							}
							break;
				}
			}
			else
			if (kickedObj->Genre == SKELETON_GENRE)
			{
			}
		}
	} // if DoSimpleBox...
}


/******************** AIM AT CLOSEST KICKABLE OBJECT **********************/

static void	AimAtClosestKickableObject(void)
{
ObjNode	*closest,*thisNodePtr;
float	minDist,dist,myX,myZ;

			/************************/
			/* SCAN OBJ LINKED LIST */
			/************************/

	thisNodePtr = gFirstNodePtr;
	myX = gCoord.x;
	myZ = gCoord.z;
	minDist = 10000000;
	closest = nil;

	while(thisNodePtr)
	{
		if (thisNodePtr->CType & CTYPE_KICKABLE)
		{
			dist = CalcDistance(myX, myZ, thisNodePtr->Coord.x, thisNodePtr->Coord.z);	// calc dist to this obj
			if (dist < minDist)
			{
				minDist = dist;
				closest = thisNodePtr;
			}
		}
		thisNodePtr = thisNodePtr->NextNode;
	}

				/* SEE IF ANYTHING CLOSE ENOUGH */

	if (minDist < 300.0f)
	{
		TurnObjectTowardTarget(gCurrentPlayer, &gCoord, closest->Coord.x, closest->Coord.z,	9.0, false);
	}
}


/******************** AIM AT CLOSEST BOPPABLE OBJECT **********************/

static void	AimAtClosestBoppableObject(void)
{
ObjNode	*closest,*thisNodePtr;
u_long	closestCType;
float	minDist,dist,myX,myZ,myY;

			/************************/
			/* SCAN OBJ LINKED LIST */
			/************************/

	thisNodePtr = gFirstNodePtr;
	myX = gCoord.x;
	myY = gCoord.y + gCurrentPlayer->TopOff;
	myZ = gCoord.z;
	minDist = 10000000;
	closest = nil;
	closestCType = 0;

	while(thisNodePtr)
	{
		if (thisNodePtr->CType & CTYPE_AUTOTARGET)
		{
			if (myY > (thisNodePtr->Coord.y - 40))				// player must be above it
			{
				dist = CalcQuickDistance(myX, myZ, thisNodePtr->Coord.x, thisNodePtr->Coord.z);	// calc dist to this obj
				if (dist < minDist)
				{
					minDist = dist;
					closest = thisNodePtr;
					closestCType = thisNodePtr->CType;
				}
			}
		}
		thisNodePtr = thisNodePtr->NextNode;
	}

				/********************************/
				/* SEE IF ANYTHING CLOSE ENOUGH */
				/********************************/

	if (minDist < 290.0f)
	{
		OGLVector2D	dir;

		if (minDist > 15.0f)							// this avoids rotation & motion jitter
		{
			TurnObjectTowardTarget(gCurrentPlayer, &gCoord, closest->Coord.x, closest->Coord.z,	9.0, false);	// aim at target

					/* MOVE TOWARDS THE TARGET */

			if (closestCType & CTYPE_AUTOTARGETJUMP)
			{
				FastNormalizeVector2D(closest->Coord.x - gCoord.x, closest->Coord.z - gCoord.z, &dir);
				dir.x *= 500.0f;
				dir.y *= 500.0f;

				gCoord.x += dir.x * gFramesPerSecondFrac;
				gCoord.z += dir.y * gFramesPerSecondFrac;

				gDelta.x = gCoord.x - gCurrentPlayer->OldCoord.x;
				gDelta.z = gCoord.z - gCurrentPlayer->OldCoord.z;
			}
		}
	}
}




/******************* PLAYER GRAB ROOT SWING ********************/

void PlayerGrabRootSwing(ObjNode *root, int joint)
{
}


/*************** PLAYER LEAVE ROOT SWING *****************/

static void PlayerLeaveRootSwing(void)
{
float	r,dx,dz;

	r = gCurrentPlayer->Rot.y;
	dx = -sin(r) * 1200.0f;
	dz = -cos(r) * 1200.0f;

//	gDelta.x += dx;
//	gDelta.z += dz;
//	dy = gDelta.y;

	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &gDelta);

	gDelta.x *= 3000.0f;
	gDelta.z *= 3000.0f;
	gDelta.y = 200.0f; //(dy*.2f) +

	MorphToSkeletonAnim(gCurrentPlayer->Skeleton, PLAYER_ANIM_FALL, 9);
}


#pragma mark -





