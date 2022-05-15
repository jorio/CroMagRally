/*******************************/
/*   	PLAYER_CAR.C		   */
/* (c)2000 Pangea Software     */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MovePlayer_Car(ObjNode *theNode);
static void MovePlayer_Car_Multipass(ObjNode *theNode);
static void UpdatePlayer_Car(ObjNode *theNode);
static void DoCarGroundCollision(ObjNode *theNode);
static void RotateCar(ObjNode *theNode);
static void DoCarMotion(ObjNode *theNode);
static Boolean DoVehicleCollisionDetect(ObjNode *vehicle);
static void DoCPUControl_Car(ObjNode *theNode);
static void DoCPUWeaponLogic_Standard(ObjNode *carObj, short playerNum, short powType);
static void DoCPUPOWLogic_Nitro(ObjNode *carObj, short playerNum);
static void DoPlayerControl_Car(ObjNode *theNode);
static void DoCPUPOWLogic_RomanCandle(short playerNum);
static void DoCPUPOWLogic_BottleRocket(short playerNum);
static void DoCPUPOWLogic_Mine(short playerNum);
static void UpdateTireSkidMarks(ObjNode *car);
static void VehicleHitVehicle(ObjNode *car1, ObjNode *car2);
static void AddToTireSkidMarks(ObjNode *car, short playerNum);
static void MovePlayer_HeadSkeleton(ObjNode *head);
static void SpewDebrisFromWheel(short p, ObjNode *carObj, ObjNode *wheelObj);
static void ResetTractionFromCopy(short p);
static void UpdateStickyTires(short p);
static void UpdateSuperSuspension(short p);
static void UpdateInvisibility(short p);
static void UpdateFrozenTimer(short p);
static void UpdateFlaming(short p);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	IMPACT_DAMAGE			25000.0f		// smaller == MORE damage
#define	BLAST_DAMAGE			10000.0f		// smaller == MORE damage

#define	TAG_SPAZ_TIMER			3.0f			// car cannot be re-tagged for n seconds

#define	SKID_SMOKE_TIMER				.05f;

#define	PLACE_SPEED_TWEAK				170.0f
#define	PLACE_SPEED_TWEAK_CPU			200.0f


#define	BRAKE_EFFECTIVENESS		4000.0f //3300.0f			// Bigger == more braking power

#define	AMBIENT_AIR_FRICTION	1100.0f


#define	PLAYER_CAR_SCALE	1.0f
#define	BROG_SCALE			1.0f

#define	MAX_CAR_ROT			(PI/2.1)		// max x & z rotation to keep from turning over.  Must not go beyone or equal to 45 degrees!

#define	CAR_MAX_VERTICAL_SPEED	(gPlayerInfo[playerNum].carStats.maxSpeed * 1.1f)



#define CAR_X_DIP_FACTOR	0.0013f
#define CAR_Z_DIP_FACTOR	0.0005f

#define	TERRAIN_SLOPE_ACCELFACTOR	2500.0f




#define	CAR_TO_CAR_BOUNCE		-.9f;			// reflection value when 2 cars collide


#define	NITRO_ACCELERATION		7000.0f
#define	NITRO_MAXSPEED			7000.0f

#define	WATER_MAXSPEED			2000.0f

#define	DELTA_SUBDIV			40.0f				// smaller == more subdivisions per frame


#define	WATER_FRICTION			1600.0f


#define	ENGINE_VOLUME			0.4f




		/* USER EDITABLE */

const UserPhysics kDefaultPhysics =
{
	.steeringResponsiveness = 7.0,
	.carMaxTightTurn 		= 2000.0f,
	.carTurningRadius 		= 0.0016f,
	.tireTraction			= .028f,
	.tireFriction			= 10000.0,
	.carGravity				= 6500.0f,
	.slopeRatioAdjuster		= .7f,

	.carStats = // parameter values 0..7
	{
		//						  Sp,Ac,Tr,Su
		[CAR_TYPE_MAMMOTH]		= {{4, 2, 4, 7}},
		[CAR_TYPE_BONEBUGGY]	= {{5, 4, 4, 2}},
		[CAR_TYPE_GEODE]		= {{3, 5, 6, 7}},
		[CAR_TYPE_LOG]			= {{6, 5, 2, 3}},
		[CAR_TYPE_TURTLE]		= {{3, 6, 4, 3}},
		[CAR_TYPE_ROCK]			= {{4, 3, 5, 2}},
		[CAR_TYPE_TROJANHORSE]	= {{4, 5, 6, 4}},
		[CAR_TYPE_OBELISK]		= {{5, 3, 6, 7}},
		[CAR_TYPE_CATAPULT]		= {{6, 7, 5, 4}},
		[CAR_TYPE_CHARIOT]		= {{7, 7, 4, 3}},
		[CAR_TYPE_SUB]			= {{0, 7, 0, 0}},
	},
};

#if 0	// from iOS version
const UserPhysics kiOSPhysicsConsts =
{
	./*CPU*/steeringResponsiveness = 8.0,		// for CPU cars only!
	.carMaxTightTurn		= 1200.0,			// bigger == more responsive steering at high speeds
	.carTurningRadius		= 0.0017,			// bigger == tighter turns, smaller = wider turns
	.tireTraction			= .027,				// amount of grip the tires have when turning
	.tireFriction			= 5000.0,			// when travelling perpendicular to motion
	.carGravity				= 9000.0,
	.slopeRatioAdjuster		= .7,				// bigger == able to climb steep hills easier, smaller == bounce off walls more.
	.carStats =
	{
		[CAR_TYPE_MAMMOTH]		= {{5, 3, 5, 8}},
		[CAR_TYPE_BONEBUGGY]	= {{5, 7, 4, 2}},
		[CAR_TYPE_GEODE]		= {{3, 5, 6, 7}},
		[CAR_TYPE_LOG]			= {{6, 4, 3, 2}},
		[CAR_TYPE_TURTLE]		= {{3, 6, 7, 5}},
		[CAR_TYPE_ROCK]			= {{4, 6, 5, 2}},
		[CAR_TYPE_TROJANHORSE]	= {{4, 4, 6, 4}},
		[CAR_TYPE_OBELISK]		= {{6, 2, 7, 5}},
		[CAR_TYPE_CATAPULT]		= {{6, 3, 4, 2}},
		[CAR_TYPE_CHARIOT]		= {{6, 4, 3, 1}},
		[CAR_TYPE_SUB]			= {{0, 1, 0, 0}},
	},
};
#endif


/*********************/
/*    VARIABLES      */
/*********************/

uint16_t			gPlayer0TileAttribs;

Boolean			gAutoPilot = false;

#define WheelSpinRot		SpecialF[0]

OGLVector3D				gPreCollisionDelta;

short	gPlayerMultiPassCount = 0;

UserPhysics		gUserPhysics;
Boolean			gUserTamperedWithPhysics = false;


/********************** SET DEFAULT PHYSICS ***********************/

void SetDefaultPhysics(void)
{
	memcpy(&gUserPhysics, &kDefaultPhysics, sizeof(UserPhysics));
	gUserTamperedWithPhysics = false;
}



/*************************** INIT PLAYER: CAR ****************************/
//
// Creates an ObjNode for the player in the Car form.
//
// INPUT:
//			where = floor coord where to init the player.
//			rotY = rotation to assign to player if oldObj is nil.
//

ObjNode *InitPlayer_Car(int playerNum, OGLPoint3D *where, float rotY)
{
short			carType;
ObjNode			*newObj;
static const float bottomOffsets[NUM_LAND_CAR_TYPES] =
{
	-57,				// mammoth
	-78,				// bone buggy
	-94,				// geode
	-99,				// log
	-63,				// turtle
	-89,				// rock

	-127,				// trojan
	-107,				// obelisk

	-77,				// catapult
	-100,				// chariot

};

static const float shadowScale[NUM_LAND_CAR_TYPES] =
{
	11.0,				// mammoth
	9.0,				// bone buggy
	12.0,				// geode
	11.5,				// log
	10.5,				// turtle
	9.0,				// rock

	12,					// trojan
	11,					// obelisk

	9,					// catapult
	10,					// chariot

};

	carType = gPlayerInfo[playerNum].vehicleType;


		/* CREATE CAR BODY AS MAIN OBJECT FOR PLAYER */

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_CARPARTS,
		.type		= CARPARTS_ObjType_Body_Mammoth + carType,
		.coord.x	= where->x,
		.coord.z	= where->z,
		.coord.y	= GetTerrainY(where->x,where->z)+100,
		.flags		= STATUS_BIT_ROTXZY,
		.slot		= PLAYER_SLOT,
		.moveCall	= MovePlayer_Car,
		.scale		= PLAYER_CAR_SCALE,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	gPlayerInfo[playerNum].objNode 	= newObj;

	newObj->PlayerNum = playerNum;
	newObj->Rot.y = rotY;
	newObj->CType = CTYPE_MISC|CTYPE_PLAYER;


				/* SET COLLISION INFO */

	newObj->CType = CTYPE_PLAYER|CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 180, bottomOffsets[carType], -120, 120, 120, -120);


			/* CREATE WHEELS & HEAD */

	CreateCarWheelsAndHead(newObj, playerNum);


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CAR_MAMMOTH + carType, shadowScale[carType], shadowScale[carType], true);


	return(newObj);
}


/*************** SET PHYSICS FOR VEHICLE TYPE ***********************/
//
// Sets the physics variables based on car part types.
//

void SetPhysicsForVehicleType(short playerNum)
{
	CarStatsType *info = &gPlayerInfo[playerNum].carStats;

	const int	t = gPlayerInfo[playerNum].vehicleType;								// get vehicle type
	const float	userCarStatMultiplier = 1.0f / 8.0f;

	float speed			= userCarStatMultiplier * gUserPhysics.carStats[t].speed;
	float acceleration	= userCarStatMultiplier * gUserPhysics.carStats[t].acceleration;
	float traction		= userCarStatMultiplier * gUserPhysics.carStats[t].traction;
	float suspension	= userCarStatMultiplier * gUserPhysics.carStats[t].suspension;

				/* SPEED  */

	if (gTrackNum == TRACK_NUM_ATLANTIS)				// rigged speed in subs
		info->maxSpeed = MAX_SUBMARINE_SPEED + RandomFloat()*200.0f;
	else
		info->maxSpeed = 3000.0f + speed * 3000.0f;

	if ((gPlayerInfo[playerNum].isComputer) && (gGamePrefs.difficulty == DIFFICULTY_SIMPLISTIC))	// make CPU cars slower
	{
		info->maxSpeed *= .7f;
	}

				/* ACCELERATION  */

	if (gPlayerInfo[playerNum].isComputer && (gTheAge > STONE_AGE))				// tweak for CPU Cars
	{
		if (gGamePrefs.difficulty == DIFFICULTY_HARD)
			acceleration += .2f;
		else
		if (gGamePrefs.difficulty == DIFFICULTY_MEDIUM)
			acceleration += .05f;
	}

	info->acceleration = 2000.0f + acceleration * 4000.0f;

				/* TRACTION  */


	if (gPlayerInfo[playerNum].isComputer && (gTheAge > STONE_AGE))				// tweak for CPU Cars
	{
		if (gGamePrefs.difficulty == DIFFICULTY_HARD)
			traction += .2f;
		else
		if (gGamePrefs.difficulty == DIFFICULTY_MEDIUM)
			traction += .05f;
	}

	SetTractionPhysics(info, traction);

				/* SUSPENSION */

	SetSuspensionPhysics(info, suspension);



			/* SET EXTRA STUFF */

	info->airFriction		=	8.0;


			/* KEEP A MASTER COPY SO THAT POWERUPS CAN CHANGE THE WORKING ONE */

	gPlayerInfo[playerNum].carStatsCopy = gPlayerInfo[playerNum].carStats;
}

/******************* SET SUSPENSION PHYSICS **********************/

void SetSuspensionPhysics(CarStatsType *carStats, float n)
{
	if (gGamePrefs.difficulty <= DIFFICULTY_EASY)		// easy mode has easy suspension
		n += .4f;

	carStats->suspension 	=	.4f + n * .6f;
}



/****************** SET TRACTION PHYSICS **********************/

void SetTractionPhysics(CarStatsType *carStats, float p)
{
	if (gGamePrefs.difficulty == DIFFICULTY_SIMPLISTIC)
	{
		p += .8f;
	}
	else
	if (gGamePrefs.difficulty == DIFFICULTY_EASY)		// easy mode has easy traction
		p += .5f;
	else
	if (gGamePrefs.difficulty == DIFFICULTY_MEDIUM)
		p += .3f;


	carStats->tireTraction		= .4f + p * .5f;
	carStats->minPlaningAngle 	= .6f - (p * .3f);
	carStats->minPlaningSpeed 	= 3000.0f + (p * 500.0f);
}


/******************** RESET TRACTION FROM COPY ***********************/
//
// Resets the traction related values from the master copy
//

static void ResetTractionFromCopy(short p)
{
	gPlayerInfo[p].carStats.tireTraction	= gPlayerInfo[p].carStatsCopy.tireTraction;
	gPlayerInfo[p].carStats.minPlaningAngle = gPlayerInfo[p].carStatsCopy.minPlaningAngle;
	gPlayerInfo[p].carStats.minPlaningSpeed = gPlayerInfo[p].carStatsCopy.minPlaningSpeed;
}


#pragma mark -


/******************** MOVE PLAYER: CAR ***********************/

static void MovePlayer_Car(ObjNode *theNode)
{
int					numPasses;
float				oldFPS,oldFPSFrac;
long	oldLeft,oldRight,oldFront,oldBack,oldTop,oldBottom;


	/* SEE IF THE CAR IS BEING DRIVEN BY A PLAYER */

	gCurrentPlayerNum = theNode->PlayerNum;			// get player #
	gCurrentPlayer = theNode;



		/* SUB-DIVIDE DELTA INTO MANAGABLE LENGTHS */
		//
		// Do the player motion in multiple passes depending on how fast
		// it is moving.  This prevents things from going thru other things.
		//

	oldFPS = gFramesPerSecond;											// remember what fps really is
	oldFPSFrac = gFramesPerSecondFrac;

	numPasses = (theNode->Speed2D*oldFPSFrac) * (1.0f / DELTA_SUBDIV);	// calc how many subdivisions to create
	if (numPasses == 0)
		numPasses++;													// always at least 1 pass
	if (numPasses > 8)													// keep it reasonable
		numPasses = 8;


	gFramesPerSecondFrac *= 1.0f / (float)numPasses;					// adjust frame rate during motion and collision
	gFramesPerSecond *= 1.0f / (float)numPasses;


			/*******************/
			/* DO IT IN PASSES */
			/*******************/

			/* KEEP OLD INFO */

//	oldCoord = theNode->OldCoord;
	oldTop = theNode->CollisionBoxes[0].oldTop;
	oldBottom = theNode->CollisionBoxes[0].oldBottom;
	oldLeft = theNode->CollisionBoxes[0].oldLeft;
	oldRight = theNode->CollisionBoxes[0].oldRight;
	oldFront = theNode->CollisionBoxes[0].oldFront;
	oldBack = theNode->CollisionBoxes[0].oldBack;



			/* DO THE PASSES */

	for (gPlayerMultiPassCount = 0; gPlayerMultiPassCount < numPasses; gPlayerMultiPassCount++)
	{
		GetObjectInfo(theNode);

		if (gPlayerMultiPassCount > 0)
			KeepOldCollisionBoxes(theNode);								// keep old boxes & other stuff secondary passes

		MovePlayer_Car_Multipass(theNode);

		UpdatePlayer_Car(theNode);
	}


		/* RESTORE THE OLD INFO */

	theNode->CollisionBoxes[0].oldTop = oldTop;
	theNode->CollisionBoxes[0].oldBottom = oldBottom;
	theNode->CollisionBoxes[0].oldLeft = oldLeft;
	theNode->CollisionBoxes[0].oldRight = oldRight;
	theNode->CollisionBoxes[0].oldFront = oldFront;
	theNode->CollisionBoxes[0].oldBack = oldBack;


			/* UPDATE */

	gFramesPerSecond = oldFPS;											// restore real FPS values
	gFramesPerSecondFrac = oldFPSFrac;


}


/******************** MOVE PLAYER: CAR : MULTIPASS ***********************/

static void MovePlayer_Car_Multipass(ObjNode *theNode)
{
            /***********/
			/* CONTROL */
            /***********/

            /* CPU DRIVEN */

    if (gPlayerInfo[gCurrentPlayerNum].isComputer || gAutoPilot)
    {
    	DoCPUControl_Car(theNode);
    }
            /* PLAYER DRIVEN */
    else
		DoPlayerControl_Car(theNode);


	        /***********/
			/* MOVE IT */
	        /***********/

	DoCarMotion(theNode);


		/* DO COLLISION CHECK */

	DoCarGroundCollision(theNode);
	DoVehicleCollisionDetect(theNode);
	DoFenceCollision(theNode);
	UpdatePlayerCheckpoints(gCurrentPlayerNum);
}



/******************** DO CAR MOTION ********************/

static void DoCarMotion(ObjNode *theNode)
{
float		speed2D,terrainY,maxSpeed;
float		r,r2,dot,traction,friction;
float		fps = gFramesPerSecondFrac;
OGLVector2D	aimVector,skidVector;
float		carBottomY, oldDeltaY;
short		playerNum;
ObjNode		*head;
short		desiredAnim;
CarStatsType	*info;
uint16_t		tileAttribs;
float		thrust,dx,dz;
PlayerInfoType	*pinfo;
float		cpuTweakFactor;
Boolean		onWater;

	playerNum = theNode->PlayerNum;
	info = &gPlayerInfo[playerNum].carStats;
	pinfo = &gPlayerInfo[playerNum];
	onWater = pinfo->onWater;

			/* GET CPU TWEAK FACTOR */

	cpuTweakFactor = 1.0f;													// assume no tweak

	if (gGamePrefs.difficulty == DIFFICULTY_HARD)
	{
		if (pinfo->isComputer)												// give cars in back a slight edge
		{
			if (pinfo->place > gWorstHumanPlace)							// only give CPU an edge if its behind the worst human
				cpuTweakFactor = 1.0f + (float)(pinfo->place - gWorstHumanPlace) * 0.3f;
		}
	}

			/****************************/
			/* GET TILE ATTRIBUTES HERE */
			/****************************/

	tileAttribs = GetTileAttribsAtRowCol(gCoord.x, gCoord.z);			// get tile attributes
	SetPlayerParmsFromTileAttributes(playerNum,tileAttribs);			// set some player info based on the settings

	if (playerNum == 0)													// save a global for debug
		gPlayer0TileAttribs = tileAttribs;


			/*****************/
			/* DO CAR MOTION */
			/*****************/

		/* DO GRAVITY & ROTATION */

	oldDeltaY = gDelta.y;
	gDelta.y -= gUserPhysics.carGravity * fps;				// gravity
	RotateCar(theNode);										// rotate it
	r = r2 = theNode->Rot.y;


			/* APPLY THRUST TO CAR */


	thrust = pinfo->currentThrust;								// get thrust calculated earlier based on user controls
	thrust *= gPlayerInfo[playerNum].groundAcceleration;		// use ground accel to affect the thrust/acceleration

	dx = theNode->AccelVector.x = -sin(r) * thrust;				// calculate acceleration deltas
	dz = theNode->AccelVector.y = -cos(r) * thrust;

	gDelta.x += dx * fps;										// and apply to deltas
	gDelta.z += dz * fps;




			/*************************************/
			/* HANDLE TERRAIN SLOPE ACCELERATION */
			/*************************************/

	terrainY = GetTerrainY(gCoord.x, gCoord.z);						// get some terrain info (coord and normal)
	carBottomY = gCoord.y + theNode->BottomOff;						// calc y coord of bottom of car
	pinfo->distToFloor = carBottomY - terrainY;						// calc dist to floor

			/* DO A SCREAM IF JUST STARTED TO FALL AND WE'RE HIGH OFF GROUND */

	if ((oldDeltaY >= 0.0f) && (gDelta.y < 0.0f))
	{
		if (pinfo->distToFloor > 500.0f)							// must be high for it to register
		{
			if (pinfo->onThisMachine && (!pinfo->isComputer) && (!onWater))		// only humans on this computer make sound
			{
				PlayAnnouncerSound(EFFECT_WOAH, false, 0);
			}
		}
	}



	if (!onWater)
	{
		if ((theNode->StatusBits & STATUS_BIT_ONTERRAIN) || (pinfo->distToFloor < 40.0f))
		{
			float	acc;

			/* ACCEL FACTOR IS STRONGER AS TERRAIN GETS STEEPER */

			acc = cos(gRecentTerrainNormal.y) * TERRAIN_SLOPE_ACCELFACTOR;

				/* IF BRAKING THEN DONT SLIDE MUCH */

			if (pinfo->braking)
			{
				if (gRecentTerrainNormal.y > .95f)				// if almost flat, then dont slide at all
					acc = 0;
				else
					acc *= .6f;									// otherwise, grip a little better
			}


					/* APPLY SLOPE ACC */

			gDelta.x += gRecentTerrainNormal.x * (fps * acc);
			gDelta.z += gRecentTerrainNormal.z * (fps * acc);
		}
	}

		/****************************/
		/* HANDLE CAR TIRE TRACTION */
		/****************************/

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
				/* CALC 2D X/Z SPEED WE'RE TRAVELLING BASED ON DELTAS */

		VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);
		speed2D = theNode->Speed2D;

			/* GET AIM VECTOR */

		if (pinfo->accelBackwards)											// see if accelerating backwards
   			r  += PI;

		aimVector.x = -sin(r);												// get vector in direction of acceleration
		aimVector.y = -cos(r);
		FastNormalizeVector2D(gDelta.x, gDelta.z, &skidVector, true);		// get vector in direction of true motion
		dot = OGLVector2D_Dot(&aimVector, &skidVector);						// when dot is 1.0 there's no skid, when 0 its perpendiculr skid


				/* CALC TRACTION & FRICTION */

		traction = gPlayerInfo[playerNum].groundTraction * info->tireTraction;	// use both ground and tire values
		traction *= dot * gUserPhysics.tireTraction;							// better traction as dot == 1.0 (when not skidding)
		traction *= cpuTweakFactor;


		friction = gPlayerInfo[playerNum].groundFriction * info->tireTraction;	// friction based on tire traction and ground friction
		friction *= (1.0f - dot) * gUserPhysics.tireFriction;					// skid friction gets greater as dot == 0 (when doing more skidding)
		friction *= cpuTweakFactor;


			/* IF BRAKING, THEN APPLY BRAKING FRICTION ALSO */

		if (pinfo->braking)
			friction += (fabs(dot) * BRAKE_EFFECTIVENESS) * gPlayerInfo[playerNum].groundTraction;


				/* SEE IF PLANING */

		if (pinfo->isPlaning)										// if planing, then loosen things up
		{
			if (pinfo->greasedTiresTimer > 0.0f)					// if greased then no friction at all
			{
				traction = 0;
				friction = 0;
			}
			else																	// standard planing, so have some friction
			{
				traction *= .1f;
				friction *= .1f;
			}
		}

			/* SEE IF AIMING BACKWARDS TO ACCELERATION */

		if (dot < 0.0f)
		{
			friction *= (dot + 1.0f) * 2.0f;
			traction = -traction;
		}


			/* ADD IN AMBIENT/GENERAL AIR FRICTION */

		if (pinfo->greasedTiresTimer <= 0.0f)						// for better effect, no air friction with greased tires
			friction += AMBIENT_AIR_FRICTION;


			/* AVERAGE THE VECTORS, BUT WEIGHT THEM DIFFERENTLY */

		skidVector.x += aimVector.x * traction;
		skidVector.y += aimVector.y * traction;
		FastNormalizeVector2D(skidVector.x, skidVector.y, &skidVector, true);


		gDelta.x = skidVector.x * speed2D;
		gDelta.z = skidVector.y * speed2D;

	}
	else
	{
		if (onWater)
			friction = WATER_FRICTION;					// water friction
		else
			friction = info->airFriction;
	}


			/* DO FRICTION ON DELTAS */

	ApplyFrictionToDeltas(friction * fps, &gDelta);



		/************************/
		/* UPDATE 2D SPEED VALUE*/
		/************************/

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);

		/* CALC MAX SPEED */

	if (pinfo->onWater)
	{
		maxSpeed = WATER_MAXSPEED;
	}
	else
	if (pinfo->nitroTimer > 0.0f)											// see if doing Nitro speed
	{
		maxSpeed = NITRO_MAXSPEED;
	}
	else																	// not doing nitro
	{
		maxSpeed = info->maxSpeed;											// get preferred max speed of car

		if (pinfo->isComputer)
		{
			if (pinfo->place > gWorstHumanPlace)							// only give CPU an edge if its behind the worst human
				maxSpeed += (float)(pinfo->place - gWorstHumanPlace) * PLACE_SPEED_TWEAK_CPU;
		}
		else
			maxSpeed += (float)(pinfo->place) * PLACE_SPEED_TWEAK;				// give human cars in back a slight edge
	}

	if (pinfo->flamingTimer > 0.0f)											// half speed if flaming
		maxSpeed *= .5f;

	if (theNode->Speed2D > maxSpeed)											// see if going too fast
	{
		float	scale;

		scale = maxSpeed/theNode->Speed2D;										// get 100-% over
		gDelta.x *= scale;														// adjust back to max
		gDelta.z *= scale;
		theNode->Speed2D = maxSpeed;
	}

		/************************************/
		/* SEE IF GOING FORWARD OR BACKWARD */
		/************************************/

	FastNormalizeVector2D(gDelta.x, gDelta.z, &skidVector, true);						// get new in direction of true motion
	if ((skidVector.x == 0.0f) && (skidVector.y == 0.0f))						// if not moving, then fake to aim forward
		dot = 1.0f;
	else
	{
		aimVector.x = -sin(r2);													// get vector in direction of aim
		aimVector.y = -cos(r2);
		dot = OGLVector2D_Dot(&aimVector, &skidVector);							// calc angle between motion and aim vectors
	}

	pinfo->skidDot	= dot;										// remember this value

	if (dot < 0.0f)																// see if travelling backwards relative to the aim (dot is - )
		pinfo->movingBackwards = true;
	else
		pinfo->movingBackwards = false;


			/*****************************/
			/* ALSO SEE IF WE'RE PLANING */
			/*****************************/

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)							// only update this stuff if we're on the ground
	{
		float	minPlaningSpeed = info->minPlaningSpeed * cpuTweakFactor;	// get planing speed

				/* SEE IF WE'RE STILL PLANING FROM EARLIER */

		if (!pinfo->isPlaning)
		{
			float	minPlaningAngle = info->minPlaningAngle * cpuTweakFactor;	// get planing angle

			pinfo->isPlaning = false;										// assume not planing

					/* SEE IF START PLANING NOW */



			if (fabs(dot) < minPlaningAngle)								// first, we must be skidding fairly perpendicular to motion
			{
				if (theNode->Speed2D > minPlaningSpeed)						// second, we must be going fast enough to cause the planing
				{
					pinfo->isPlaning	= true;								// plane for 1 second before re-testing it.
				}
			}
		}

				/* SEE IF END A PREVIOUS PLANING */
		else
		{
			if (pinfo->greasedTiresTimer == 0.0f)							// if tires are still greased, then keep planing
			{
				if (theNode->Speed2D < (minPlaningSpeed * .8f))				// not greased, so see if we've slowed enough to stop planing
					pinfo->isPlaning = false;
			}
			else															// tires greased, so see if grease wore off
			{
				pinfo->greasedTiresTimer -= fps;
				if (pinfo->greasedTiresTimer <= 0.0f)
				{
					pinfo->greasedTiresTimer = 0;
					pinfo->isPlaning = false;
				}
			}
		}
	}

			/****************/
			/* MOVE THE CAR */
			/****************/

	gCoord.x += gDelta.x * fps;									// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;



		/************************/
		/* SET APPROPRIATE ANIM */
		/************************/


	head = pinfo->headObj;							// get ObjNode of skeleton

	if ((head->Skeleton->AnimNum != PLAYER_ANIM_THROWFORWARD) && (head->Skeleton->AnimNum != PLAYER_ANIM_THROWBACKWARD))		// dont interrupt a throw for fluff anims
	{
		desiredAnim = PLAYER_ANIM_SIT;								// assume the default


		if ((pinfo->isPlaning) && (!pinfo->onWater))
			desiredAnim = PLAYER_ANIM_FREAKOUT;
		else
		if ((pinfo->distToFloor > 250.0f) && (!pinfo->onWater))		// see if high off ground
		{
			desiredAnim = PLAYER_ANIM_HANGTIME;
		}
		else														// close or on ground
		{
			if (pinfo->steering < -.1f)
				desiredAnim = PLAYER_ANIM_LEFTTURN;
			else
			if (pinfo->steering > .1f)
				desiredAnim = PLAYER_ANIM_RIGHTTURN;
		}

		if (head->Skeleton->AnimNum != desiredAnim)
		{
			MorphToSkeletonAnim(head->Skeleton, desiredAnim, 4.0);
		}
	}
}


/********************* FLATTEN CAR ON GROUND ********************/

static void DoCarGroundCollision(ObjNode *theNode)
{
OGLPoint3D		corner[4];
OGLVector3D		oldXAxis, oldZAxis, newXAxis, newZAxis;
float			groundY,carBottomY;
float			dzRot,dxRot,dot;
float			terrainY[4];
Boolean			onGround = false;
Boolean			backSideDown, frontSideDown;
Boolean			leftSideDown, rightSideDown;
OGLVector3D		t;
float			oldDY = gDelta.y;
float			diff;
short			playerNum = theNode->PlayerNum;

		/**********************************/
		/* SEE IF CENTER POINT HIT GROUND */
		/**********************************/

	groundY = GetTerrainY(gCoord.x, gCoord.z);
	carBottomY = gCoord.y + theNode->BottomOff;				// calc coord on bottom of car

	if (carBottomY < groundY)
	{
				/* KEEP CAR ABOVE SURFACE */

		diff = groundY - carBottomY;						// how far under did we go?
		gCoord.y += diff;									// move us up so we're not down there
		onGround = true;


			/* CALC DELTA UPWARDS FROM GROUND HIT */

		if ((gRecentTerrainNormal.y < .95f) || (theNode->Speed2D > 400.0f))			// only do this if on a slope and moving fast enough
		{
			OGLVector3D	motion,ref,upHill,final;
			float	speed = CalcVectorLength(&gDelta);
			float	ratio,oneMinusRatio;

			OGLVector3D_Normalize(&gDelta, &motion);								// get a normalized motion vector.

			if (motion.x || motion.z)
			{
							/* CALC UP-HILL VECTOR */

				OGLVector3D_Cross(&gRecentTerrainNormal, &motion, &upHill);			// get perpendicular
				upHill.x = -upHill.x;												// invert so next cross prod will be facing the desired direction
				upHill.y = -upHill.y;
				upHill.z = -upHill.z;
				OGLVector3D_Cross(&gRecentTerrainNormal, &upHill, &upHill);			// get uphill vector (vec flush with terrain, but pointing in uphill direction)


							/* CALC BOUNCE VECTOR */

				ReflectVector3D(&motion, &gRecentTerrainNormal, &ref);


					/* CALC FINAL VECTOR AS RATIO OF UPHILL AND BOUNCE */

				ratio = gRecentTerrainNormal.y + gUserPhysics.slopeRatioAdjuster;	// base it on the normal's y.  The steep ther the terrain, the more the bounce is used
				ratio *= gPlayerInfo[playerNum].carStats.suspension;				// adjust for suspension

				if (ratio > 1.0f)
					ratio = 1.0f;
				oneMinusRatio 	= 1.0f - ratio;

				final.x 	= (upHill.x * ratio) + (ref.x * oneMinusRatio);
				final.y 	= (upHill.y * ratio) + (ref.y * oneMinusRatio);
				final.z 	= (upHill.z * ratio) + (ref.z * oneMinusRatio);
				OGLVector3D_Normalize(&final, &final);


							/* SCALE BOUNCE BY ORIGINAL SPEED */

				final.x *= speed;
				final.y *= speed;
				final.z *= speed;

				gDelta.x = final.x;
				gDelta.y = final.y;
				gDelta.z = final.z;

				if (gDelta.y > CAR_MAX_VERTICAL_SPEED)		// see if popped up too fast
					gDelta.y = CAR_MAX_VERTICAL_SPEED;
			}
			else
			{
				gDelta.y = 0;
			}
		}
		else
			gDelta.y = 0;				// not going fast and on fairly flat ground, so dont do the sliding - keep us planted on ground
	}



		/**********************/
		/* CALC CORNER COORDS */
		/**********************/

		/* GET LOCAL-SPACE CORNERS */

	corner[0].x = theNode->LeftOff;			// far left
	corner[0].y = theNode->BottomOff;
	corner[0].z = theNode->BackOff;

	corner[1].x = theNode->LeftOff;			// near left
	corner[1].y = theNode->BottomOff;
	corner[1].z = theNode->FrontOff;

	corner[2].x = theNode->RightOff;		// near right
	corner[2].y = theNode->BottomOff;
	corner[2].z = theNode->FrontOff;

	corner[3].x = theNode->RightOff;		// far right
	corner[3].y = theNode->BottomOff;
	corner[3].z = theNode->BackOff;


	/* TRANSFORM CORNERS TO WORLD-SPACE */

	theNode->Coord = gCoord;
	UpdateObjectTransforms(theNode);	// make sure matrix is updated for this

	OGLPoint3D_TransformArray(&corner[0], &theNode->BaseTransformMatrix,&corner[0],4);


		/* GET TERRAIN-Y COORDS */

	terrainY[0] = GetTerrainY(corner[0].x, corner[0].z);
	terrainY[1] = GetTerrainY(corner[1].x, corner[1].z);
	terrainY[2] = GetTerrainY(corner[2].x, corner[2].z);
	terrainY[3] = GetTerrainY(corner[3].x, corner[3].z);



		/* SEE IF ANY WHEELS ARE HITTING GROUND */

	backSideDown = frontSideDown = leftSideDown = rightSideDown = false;


	if (corner[0].y < terrainY[0])						// front left tire
	{
		corner[0].y 	= terrainY[0];
		onGround 		= true;
		frontSideDown 	= true;
		leftSideDown 	= true;
	}


	if (corner[1].y < terrainY[1])						// back left tire
	{
		corner[1].y 	= terrainY[1];
		onGround 		= true;
		backSideDown 	= true;
		leftSideDown 	= true;
	}

	if (corner[2].y < terrainY[2])						// back right tire
	{
		corner[2].y 	= terrainY[2];
		onGround 		= true;
		backSideDown 	= true;
		rightSideDown 	= true;
	}

	if (corner[3].y < terrainY[3])						// front right tire
	{
		corner[3].y 	= terrainY[3];
		onGround 		= true;
		frontSideDown 	= true;
		rightSideDown	= true;
	}


			/***********************************************/
			/* IF ANY WHEEL HIT, THEN NEED TO DO SOMETHING */
			/***********************************************/

	if (onGround)
	{
		float	rotY = theNode->Rot.y;
		float	c = cos(rotY), s = sin(rotY);

			/* GET Y==0 AXIS VECTORS */

		oldZAxis.x = -s;
		oldZAxis.y = 0;
		oldZAxis.z = -c;

		oldXAxis.x = -c;
		oldXAxis.y = 0;
		oldXAxis.z = s;


			/* GET NEW AXIS VECTORS */

		t.x = corner[0].x - corner[1].x;				// calc average vector of left and right sides along length of car
		t.y = corner[0].y - corner[1].y;
		t.z = corner[0].z - corner[1].z;
		t.x += corner[3].x - corner[2].x;
		t.y += corner[3].y - corner[2].y;
		t.z += corner[3].z - corner[2].z;
		OGLVector3D_Normalize(&t,&newZAxis);

		t.x = corner[0].x - corner[3].x;				// calc average vector of front and back sides along width of car
		t.y = corner[0].y - corner[3].y;
		t.z = corner[0].z - corner[3].z;
		t.x += corner[1].x - corner[2].x;
		t.y += corner[1].y - corner[2].y;
		t.z += corner[1].z - corner[2].z;
		OGLVector3D_Normalize(&t,&newXAxis);


			/* SEE HOW MUCH WE ARE ROTATED ON THE X AXIS */

		dot = OGLVector3D_Dot(&oldZAxis, &newZAxis);
		dxRot = acos(dot);								// rot on X-axis is angle of Z vector
		if (corner[0].y < corner[1].y)					// if right side is higher, then reverse rot
			dxRot = -dxRot;

		theNode->DeltaRot.x = (dxRot - theNode->Rot.x) * 15.0f;


			/* SEE HOW MUCH WE ROTATED ON THE Z AXIS */

		dot = OGLVector3D_Dot(&oldXAxis, &newXAxis);
		dzRot = acos(dot);								// rot on Z-axis is angle of shift in X vector
		if (corner[0].y > corner[3].y)					// set z-rot
			dzRot = -dzRot;

		theNode->DeltaRot.z = (dzRot - theNode->Rot.z) * 15.0f;


				/* DO ADDITIONAL ROTATION FOR SIDE DIPPING */

		if (oldDY < 0.0f)									// dips only can happen if we're going down
		{
			if (backSideDown && (!frontSideDown))			// if back hit, but not front then add some spin
				theNode->DeltaRot.x += oldDY * CAR_X_DIP_FACTOR;
			else
			if (frontSideDown && (!backSideDown))			// if front hit, but not back then add some spin
				theNode->DeltaRot.x -= oldDY * CAR_X_DIP_FACTOR;

			if (leftSideDown && (!rightSideDown))			// if left hit, but not right then add some spin
				theNode->DeltaRot.z += oldDY * CAR_Z_DIP_FACTOR;
			else
			if (rightSideDown && (!leftSideDown))			// if right hit, but not left then add some spin
				theNode->DeltaRot.z -= oldDY * CAR_Z_DIP_FACTOR;
		}


				/* UPDATE */

		theNode->StatusBits |= STATUS_BIT_ONTERRAIN|STATUS_BIT_ONGROUND;

		RotateCarXZ(theNode);								// call this to update xz rot correctly
	}

			/* NOT ON GROUND */
	else
	{
		if (theNode->StatusBits & (STATUS_BIT_ONGROUND|STATUS_BIT_ONTERRAIN))		// if just now left ground, reduce y spin slightly
		{
			float	suspension = gPlayerInfo[playerNum].carStats.suspension * .4f;

			if (suspension > 1.0f)
				suspension = 1.0f;

			theNode->DeltaRot.y *= 1.0f - suspension;	// reduction is based on suspension
		}

		theNode->StatusBits &= ~(STATUS_BIT_ONGROUND|STATUS_BIT_ONTERRAIN);
	}


}


/************************ UPDATE PLAYER: CAR ***************************/

static void UpdatePlayer_Car(ObjNode *theNode)
{
short	p = theNode->PlayerNum;
float	fps = gFramesPerSecondFrac;

	UpdateObject(theNode);

	gPlayerInfo[p].coord = gCoord;				// update player coord


			/* UPDATE FINAL SPEED VALUES */

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);
	theNode->Speed3D = CalcVectorLength(&gDelta);


			/* UPDATE POWERUP TIMERS */

	UpdateNitro(p);
	UpdateStickyTires(p);
	UpdateSuperSuspension(p);
	UpdateInvisibility(p);
	UpdateFrozenTimer(p);
	UpdateFlaming(p);

	gPlayerInfo[p].bumpSoundTimer -=  fps;

			/*********************************/
			/* UPDATE ALL OF THE ATTACHMENTS */
			/*********************************/

	AlignWheelsAndHeadOnCar(theNode);


			/* UPDATE TIRE SKIDMARKS */

	UpdateTireSkidMarks(theNode);


		/********************/
		/* UPDATE RPM VALUE */
		/********************/

		/* CALC DESIRED RPM VALUE */

	if (gPlayerInfo[p].gasPedalDown)
	{
		float	max = gPlayerInfo[p].carStats.maxSpeed * (1.0f / 5000.0f);	// max rpm is based on max speed

		gPlayerInfo[p].currentRPM += gPlayerInfo[p].carStats.acceleration * .0003 * fps;

		if (!(theNode->StatusBits & STATUS_BIT_ONGROUND))					// rev higher if not on ground
			max *= 1.3f;

		if (gPlayerInfo[p].currentRPM > max)
			gPlayerInfo[p].currentRPM = max;
	}
	else
	{
		gPlayerInfo[p].currentRPM -= gPlayerInfo[p].carStats.acceleration * .0003 * fps;
		if (gPlayerInfo[p].currentRPM < 0.0f)
			gPlayerInfo[p].currentRPM = 0.0f;
	}

			/************************/
			/* UPDATE ENGINE EFFECT */
			/************************/

	if (gPlayerInfo[p].engineChannel != -1)
		Update3DSoundChannel(EFFECT_ENGINE, &gPlayerInfo[p].engineChannel, &gCoord);
	else
		gPlayerInfo[p].engineChannel = PlayEffect_Parms3D(EFFECT_ENGINE, &gCoord, NORMAL_CHANNEL_RATE, ENGINE_VOLUME);


			/* UPDATE FREQ */
	if (gPlayerInfo[p].engineChannel != -1)
	{
		int	freq;

		freq = NORMAL_CHANNEL_RATE + (gPlayerInfo[p].currentRPM * 150000.0);
		ChangeChannelRate(gPlayerInfo[p].engineChannel, freq);
	}
}


/**************** UPDATE NITRO ***********************/
//
// Called from UpdatePlayer.
//

void UpdateNitro(short p)
{
float	targetFOV,fov;
int		pane;

	if (gPlayerInfo[p].nitroTimer <= 0.0f)
	{
		targetFOV = GAME_FOV;
	}
	else
	{
		gPlayerInfo[p].nitroTimer -= gFramesPerSecondFrac;
		if (gPlayerInfo[p].nitroTimer < 0.0f)
			gPlayerInfo[p].nitroTimer = 0;

		targetFOV = 2.0;
	}


		/**************/
		/* UPDATE FOV */
		/**************/
		//
		// Do a tunnel-vision effect when using nitro.
		//

	if (gNetGameInProgress)
	{
		if (p != gMyNetworkPlayerNum)									// bail if this isnt the local player
			return;
		else
			pane = 0;
	}
	else
	{
		if (p >= gNumSplitScreenPanes)							// bail if player doesnt have its own pane
			return;
		pane = p;
	}



	fov = gGameView->fov[pane];				// get current fov

	if (targetFOV > fov)
	{
		fov += gFramesPerSecondFrac * 0.5f;
		if (fov > targetFOV)
			fov = targetFOV;
	}
	else
	if (targetFOV < fov)
	{
		fov -= gFramesPerSecondFrac * 0.5f;
		if (fov < targetFOV)
			fov = targetFOV;
	}

	gGameView->fov[pane] = fov;
}


/**************** UPDATE STICKY TIRES ***********************/
//
// Called from UpdatePlayer.
//

static void UpdateStickyTires(short p)
{
	if (gPlayerInfo[p].stickyTiresTimer <= 0.0f)
		return;

	gPlayerInfo[p].stickyTiresTimer -= gFramesPerSecondFrac;
	if (gPlayerInfo[p].stickyTiresTimer < 0.0f)							// see if just now ran out of time
	{
		gPlayerInfo[p].stickyTiresTimer = 0;
		ResetTractionFromCopy(p);
	}

}


/**************** UPDATE SUPER SUSPENSION ***********************/
//
// Called from UpdatePlayer.
//

static void UpdateSuperSuspension(short p)
{
	if (gPlayerInfo[p].superSuspensionTimer <= 0.0f)
		return;

	gPlayerInfo[p].superSuspensionTimer -= gFramesPerSecondFrac;
	if (gPlayerInfo[p].superSuspensionTimer < 0.0f)							// see if just now ran out of time
	{
		gPlayerInfo[p].superSuspensionTimer = 0;
		gPlayerInfo[p].carStats.suspension	= gPlayerInfo[p].carStatsCopy.suspension;
	}

}


/**************** UPDATE INVISIBILITY ***********************/
//
// Called from UpdatePlayer.
//

static void UpdateInvisibility(short p)
{
float	t;
ObjNode	*theNode;

	if (gPlayerInfo[p].invisibilityTimer <= 0.0f)
		return;

	gPlayerInfo[p].invisibilityTimer -= gFramesPerSecondFrac;
	if (gPlayerInfo[p].invisibilityTimer <= 0.0f)							// see if just now ran out of time
	{
		gPlayerInfo[p].invisibilityTimer = 0;
		t = 1.0f;
	}
	else
	if (gPlayerInfo[p].invisibilityTimer > 1.0f)
		t = .1f;
	else
		t = 1.0f - gPlayerInfo[p].invisibilityTimer;


			/* SET TRANSPARENCY FOR ALL CHAIN LINKS */

    	theNode = gPlayerInfo[p].objNode;

	 while(theNode)
	{
		theNode->ColorFilter.a = t;
		theNode = theNode->ChainNode;

	}

}


/************************ UPDATE FROZEN TIMER **************************/

static void UpdateFrozenTimer(short p)
{

	if (gPlayerInfo[p].frozenTimer == 0.0f)
		return;

	gPlayerInfo[p].frozenTimer -= gFramesPerSecondFrac;
	if (gPlayerInfo[p].frozenTimer <= 0.0f)						// see if all done
	{
		ObjNode	*obj;

		gPlayerInfo[p].frozenTimer = 0;


		obj = gPlayerInfo[p].objNode;
		while(obj)
		{
			obj->ColorFilter.r = 									// untint the car ice-blue
			obj->ColorFilter.g =
			obj->ColorFilter.b =
			obj->ColorFilter.a = 1.0;
			obj = obj->ChainNode;
		}

	}
}


/************************ UPDATE FLAMING TIMER **************************/

static void UpdateFlaming(short p)
{

	if (gPlayerInfo[p].flamingTimer == 0.0f)
		return;

	gPlayerInfo[p].flamingTimer -= gFramesPerSecondFrac;
	if (gPlayerInfo[p].flamingTimer <= 0.0f)						// see if all done
	{
		ObjNode	*obj;

		gPlayerInfo[p].flamingTimer = 0;

		obj = gPlayerInfo[p].objNode;
		while(obj)
		{
			obj->ColorFilter.r = 									// untint the car red
			obj->ColorFilter.g =
			obj->ColorFilter.b =
			obj->ColorFilter.a = 1.0;
			obj = obj->ChainNode;
		}

	}
}




/***************** ROTATE CAR *************************/
//
// INPUT:	groundTraction = 0.0..2.0 from ground tile attributes, 1.0 = normal
//

static void RotateCar(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
uint16_t		playerNum = theNode->PlayerNum;
float		steering,speed,turnSpeed;
float		targetDeltaY;
Boolean		onWater = gPlayerInfo[playerNum].onWater;
float		tireTraction,groundSteeringFactor;
float		friction;


			/***************************/
			/* HANDLE TURNING ROTATION */
			/***************************/

	steering = gPlayerInfo[playerNum].steering;							// get -1.0 -- +1.0 steering value
	if (gPlayerInfo[playerNum].movingBackwards)							// if moving backwards, then fake reversed steering
		steering = -steering;


				/* IF ON WATER */

	if (onWater)
	{
		turnSpeed = 1000.0f;
		tireTraction = .3f;

		targetDeltaY = turnSpeed * -gUserPhysics.carTurningRadius * steering;	// calc delta Y if we had perfect traction

		if (theNode->DeltaRot.y < targetDeltaY)
		{
			theNode->DeltaRot.y += tireTraction * fps * 15.0f;
			if (theNode->DeltaRot.y > targetDeltaY)
				theNode->DeltaRot.y = targetDeltaY;
		}
		else
		if (theNode->DeltaRot.y > targetDeltaY)
		{
			theNode->DeltaRot.y -= tireTraction * fps * 15.0f;
			if (theNode->DeltaRot.y < targetDeltaY)
				theNode->DeltaRot.y = targetDeltaY;
		}
	}

			/* IF ON GROUND */

	else
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		groundSteeringFactor = gPlayerInfo[playerNum].groundSteering;			// get steering slippage factor

		if (!gPlayerInfo[playerNum].isPlaning)									// if we're planing then we have no steering control
		{
			speed = theNode->Speed2D;											// get current speed of vehicle
			if (speed < gUserPhysics.carMaxTightTurn)							// pin the max value
				turnSpeed = speed;
			else
				turnSpeed = gUserPhysics.carMaxTightTurn;

			steering *= fabs(gPlayerInfo[playerNum].skidDot);					// steering has less effect as skid gets worse (max skid when dot == 0.0)

			targetDeltaY = turnSpeed * -gUserPhysics.carTurningRadius * steering;	// calc delta Y if we had perfect traction


					/* ACCEL THE DELTA INTO POSITION */

			tireTraction = gPlayerInfo[playerNum].carStats.tireTraction;

			if (theNode->DeltaRot.y < targetDeltaY)
			{
				theNode->DeltaRot.y += tireTraction * fps * 15.0f * groundSteeringFactor;
				if (theNode->DeltaRot.y > targetDeltaY)
					theNode->DeltaRot.y = targetDeltaY;
			}
			else
			if (theNode->DeltaRot.y > targetDeltaY)
			{
				theNode->DeltaRot.y -= tireTraction * fps * 15.0f * groundSteeringFactor;
				if (theNode->DeltaRot.y < targetDeltaY)
					theNode->DeltaRot.y = targetDeltaY;
			}
		}

				/* APPLY FRICTION TO DELTA Y */

		friction = gPlayerInfo[playerNum].groundFriction * gPlayerInfo[playerNum].carStats.tireTraction;		// get ground steering

		if (theNode->DeltaRot.y > 0.0f)
		{
			theNode->DeltaRot.y -= friction * fps;
			if (theNode->DeltaRot.y < 0.0f)
				theNode->DeltaRot.y = 0.0f;
		}
		else
		{
			theNode->DeltaRot.y += friction * fps;
			if (theNode->DeltaRot.y > 0.0f)
				theNode->DeltaRot.y = 0.0f;
		}


	}



		/* DO THE Y SPIN */

	theNode->Rot.y += theNode->DeltaRot.y * fps;


		/***********************/
		/* HANDLE X/Z SPINNING */
		/***********************/

	RotateCarXZ(theNode);
}


/******************** ROTATE CAR XZ *********************/

void RotateCarXZ(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	if (theNode->Rot.x > (MAX_CAR_ROT))
		theNode->Rot.x = MAX_CAR_ROT;
	else
	if (theNode->Rot.x < (-MAX_CAR_ROT))
		theNode->Rot.x = -MAX_CAR_ROT;


	theNode->Rot.z += theNode->DeltaRot.z * fps;
	if (theNode->Rot.z > (MAX_CAR_ROT))
		theNode->Rot.z = MAX_CAR_ROT;
	else
	if (theNode->Rot.z < (-MAX_CAR_ROT))
		theNode->Rot.z = -MAX_CAR_ROT;


}



#pragma mark -



/******************** DO VEHICLE COLLISION DETECT **************************/
//
// Standard collision handler for player
//
// OUTPUT: true = disabled/killed
//

static Boolean DoVehicleCollisionDetect(ObjNode *vehicle)
{
short		i,playerNum;
ObjNode		*hitObj;
unsigned long	ctype;
uint8_t		sides;
Boolean		wasInWater;


			/* DETERMINE CTYPE BITS TO CHECK FOR */

	ctype = PLAYER_COLLISION_CTYPE;
	playerNum = gCurrentPlayer->PlayerNum;

	wasInWater = gPlayerInfo[playerNum].onWater;

	gPreCollisionDelta = gDelta;									// remember delta before collision handler messes with it


			/***************************************/
			/* AUTOMATICALLY HANDLE THE GOOD STUFF */
			/***************************************/
			//
			// this also sets the ONGROUND status bit if on a solid object.
			//

	sides = HandleCollisions(vehicle, ctype, 0);


			/******************************/
			/* SCAN FOR INTERESTING STUFF */
			/******************************/

	gPlayerInfo[playerNum].onWater = false;							// assume not on water

	for (i=0; i < gNumCollisions; i++)
	{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision

			if (hitObj->CType == INVALID_NODE_FLAG)				// see if has since become invalid
				continue;

			ctype = hitObj->CType;								// get collision ctype from hit obj


			/**************************/
			/* CHECK FOR IMPENETRABLE */
			/**************************/
#if 0
			if ((ctype & CTYPE_IMPENETRABLE) && (!(ctype & CTYPE_IMPENETRABLE2)))
			{
				if (!(gCollisionList[i].sides & SIDE_BITS_BOTTOM))	// dont do this if we landed on top of it
				{
					gCoord.x = gCurrentPlayer->OldCoord.x;					// dont take any chances, just move back to original safe place
					gCoord.z = gCurrentPlayer->OldCoord.z;
				}
			}
#endif

				/********************/
				/* CHECK FOR PLAYER */
				/********************/

			if (ctype & CTYPE_PLAYER)
			{
				VehicleHitVehicle(vehicle, hitObj);
			}

				/**********************/
				/* SEE IF LIQUID PATCH */
				/**********************/
				//
				// Give other solid items priority over the water by checking
				// if the bottom collision bit is set.  This way jumping on lily pads
				// will be more reliable.
				//

			else
			if ((ctype & CTYPE_LIQUID) && (!(sides&CBITS_BOTTOM)))	// only check for water if no bottom collision
			{
				if (GetTerrainY(gCoord.x, gCoord.z) < hitObj->Coord.y)		// make sure didnt hit liquid thru solid floor
				{
					gPlayerInfo[playerNum].onWater = true;
					gPlayerInfo[playerNum].waterY = hitObj->CollisionBoxes[0].top;

					vehicle->Rot.z = vehicle->Rot.x = 0;				// flatt on water
					vehicle->DeltaRot.x = vehicle->DeltaRot.z = 0;

					gDelta.y = 0;
					gCoord.y = hitObj->CollisionBoxes[0].top;

					if (hitObj->Kind == LIQUID_WATER)					// splash if water
					{
						if (!wasInWater)
							MakeSplash(gCoord.x, gPlayerInfo[playerNum].waterY, gCoord.z);
					}
				}
			}

			/*************************************/
			/* MAKE CRASH-THUD IF HIT SOLID HARD */
			/*************************************/

			else
			if ((hitObj->CType & CTYPE_MISC) && (vehicle->Speed2D > 2000.0f))
			{
				float		volume;

				volume = vehicle->Speed2D / 3000.0f;
				if (volume > 2.0f)
					volume = 2.0;
				else
				if (volume < .5f)
					volume = .5;


				if (gPlayerInfo[playerNum].bumpSoundTimer <= 0.0f)
				{
					PlayEffect_Parms3D(EFFECT_CRASH2, &gCoord, NORMAL_CHANNEL_RATE, volume);
					gPlayerInfo[playerNum].bumpSoundTimer = .5f;
				}
			}


#if 0
				/******************/
				/* SEE IF VISCOUS */
				/******************/

			if (ctype & CTYPE_VISCOUS)
			{
				ApplyFrictionToDeltas(120, &gDelta);
			}
#endif
	}

	gCurrentPlayer = gPlayerInfo[playerNum].objNode;

	return(false);
}



/*********************** VEHICLE HIT VEHICLE *****************************/
//
// Called from above when two vehicles collide.
//
// INPUT:  car1 == current ObjNode doing collision test.
//			car2 == other car that got hit
//

static void VehicleHitVehicle(ObjNode *car1, ObjNode *car2)
{
float		relSpeed;
OGLVector3D	v1,v2;
OGLVector3D	b1,b2;
OGLVector3D	relD1,relD2;
float		dot;
short		p1 = car1->PlayerNum;
short		p2 = car2->PlayerNum;


	switch(gGameMode)
	{
		/**********************/
		/* SEE IF PLAYING TAG */
		/**********************/

		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
				if (!gTrackCompleted)
				{

						/* CAR 1 WAS IT */

					if (gPlayerInfo[p1].isIt)
					{
						if ((p2 != gWhoWasIt) || (gReTagTimer <= 0.0f))			// see if P2 was "it" last time and we have not timed out, then dont tag
						{
							gWhoWasIt = p1;
							gWhoIsIt = p2;
							gPlayerInfo[p1].isIt = false;
							gPlayerInfo[p2].isIt = true;
							gReTagTimer = TAG_SPAZ_TIMER;


						}
					}

						/* CAR 2 WAS IT */
					else
					if (gPlayerInfo[p2].isIt)
					{
						if ((p1 != gWhoWasIt) || (gReTagTimer <= 0.0f))			// see if P1 was "it" last time and we have not timed out, then dont tag
						{
							gWhoWasIt = p2;
							gWhoIsIt = p1;
							gPlayerInfo[p1].isIt = true;
							gPlayerInfo[p2].isIt = false;
							gReTagTimer = TAG_SPAZ_TIMER;

						}
					}
				}
				break;


		/***************************/
		/* SEE IF PLAYING SURVIVAL */
		/***************************/

		case	GAME_MODE_SURVIVAL:

				{
					OGLVector3D	impactVector;
					float		impactSpeed,ratio,dam1,dam2;

							/* DETERMINE IMPACT FORCE */

					impactVector.x = car1->Delta.x - car2->Delta.x;
					impactVector.y = car1->Delta.y - car2->Delta.y;
					impactVector.z = car1->Delta.z - car2->Delta.z;
					impactSpeed = CalcVectorLength(&impactVector);
					if (impactSpeed > 1400.0f)									// taps dont hurt
					{
						impactSpeed /= IMPACT_DAMAGE;								// convert to a damage value


							/* DAMAGE IS PROPORTIONAL TO SPEED RATIO OF CARS */
							//
							// slower car gets more damage, so this code may look wrong, but its correct!
							//

						if (car1->Speed3D > car2->Speed3D)						// if car #1 is faster
						{
							ratio = car2->Speed3D / (car1->Speed3D + EPS);
							dam1 = impactSpeed * ratio;
							dam2 = impactSpeed;
						}
						else													// car #2 if faster
						{
							ratio = car1->Speed3D / (car2->Speed3D + EPS);
							dam1 = impactSpeed;
							dam2 = impactSpeed * ratio;
						}

						if (gPlayerInfo[p1].impactResetTimer <= 0.0f)			// only do it if the impact timer says its okay
						{
							PlayerLoseHealth(p1, dam1);
							gPlayerInfo[p1].impactResetTimer = 1.5;
						}
						if (gPlayerInfo[p2].impactResetTimer <= 0.0f)
						{
							PlayerLoseHealth(p2, dam2);
							gPlayerInfo[p2].impactResetTimer = 1.5;
						}
					}
				}
				break;



		/***********************************/
		/* SEE IF PLAYING CAPTURE THE FLAG */
		/***********************************/

		case	GAME_MODE_CAPTUREFLAG:
				PlayerDropFlag(car1);
				PlayerDropFlag(car2);
				break;
	}


			/* SINCE THE COLLISION FUNCTION PROBABLY NEUTRALIZED THE DELTAS, LETS RESET THEM */

	gDelta.x = car1->Delta.x;
	gDelta.y = car1->Delta.y;
	gDelta.z = car1->Delta.z;


		/*********************************/
		/* BOUNCE CARS OFF OF EACH OTHER */
		/*********************************/

				/* CALC RELATIVE DELTAS */

	relD1.x = gDelta.x - car2->Delta.x;
	relD1.y = gDelta.y - car2->Delta.y;
	relD1.z = gDelta.z - car2->Delta.z;
	VectorLength3D(relSpeed, relD1.x, relD1.y, relD1.z);					// relative speed of car 1 (same for both cars, so just calc it once)

	relD2.x = car2->Delta.x - gDelta.x;
	relD2.y = car2->Delta.y - gDelta.y;
	relD2.z = car2->Delta.z - gDelta.z;


			/* CALC RELATIVE MOTION VECTORS OF EACH CAR */

	FastNormalizeVector(relD1.x, relD1.y, relD1.z, &v1);
	FastNormalizeVector(relD2.x, relD2.y, relD2.z, &v2);


				/* CALC BOUNCE VECTORS */

	ReflectVector3D(&v1, &v2, &b1);
	ReflectVector3D(&v2, &v1, &b2);

	b1.x *= relSpeed;
	b1.y *= relSpeed;
	b1.z *= relSpeed;

	b2.x *= relSpeed;
	b2.y *= relSpeed;
	b2.z *= relSpeed;


			/* APPLY TO DELTAS */

	gDelta.x += b1.x;
	gDelta.y += b1.y;
	gDelta.z += b1.z;

	car2->Delta.x += b2.x;
	car2->Delta.y += b2.y;
	car2->Delta.z += b2.z;


		/*******************************/
		/* SEE IF IT WAS A MAJOR WRECK */
		/*******************************/

	if ((!gPlayerInfo[p1].onWater) && (!gPlayerInfo[p2].onWater))	// only if nobody is on water
	{

		dot = OGLVector3D_Dot(&v1, &v2);								// determine the angle between the car vectors (-1 = head on, 0 = perp, +1 parallel)


				/* SEND INTO AIR IF T-BONE OR HEAD-ON COLLISION*/

		if (dot < 0.2f)
		{
			if (gDelta.y >= 0.0f)
				gDelta.y += relSpeed * .002f;

			if (car2->Delta.y >= 0.0f)
				car2->Delta.y += relSpeed * .002f;
		}

				/* SEE IF HIT HARD */

		if (relSpeed > 1200.0f)
		{
			car1->DeltaRot.y = PI2 * (1.0f- dot) * .1f;
			car2->DeltaRot.y = -PI2 * (1.0f - dot) * .1f;

			car1->DeltaRot.x = (RandomFloat()-.5f) * 3.0f;				// send other axes into wild spin
			car1->DeltaRot.z = (RandomFloat()-.5f) * 3.0f;
			car2->DeltaRot.x = (RandomFloat()-.5f) * 3.0f;
			car2->DeltaRot.z = (RandomFloat()-.5f) * 3.0f;


					/* SET SKID INFO */

			gPlayerInfo[car1->PlayerNum].greasedTiresTimer = .5f;
			gPlayerInfo[car1->PlayerNum].isPlaning = true;
			gPlayerInfo[car2->PlayerNum].greasedTiresTimer = .5f;
			gPlayerInfo[car2->PlayerNum].isPlaning = true;


					/* MAKE IMPACT EXPLOSION */

			MakeSparkExplosion((gCoord.x + car2->Coord.x)/2, (gCoord.y + car2->Coord.y)/2,
								(gCoord.z + car2->Coord.z)/2, 300.0f, PARTICLE_SObjType_WhiteSpark);

					/* ANNOUNCE REALLY BIG ONES */

			if (relSpeed > 3000.0f)
			{
				if ((gGameMode != GAME_MODE_TAG1) && (gGameMode != GAME_MODE_TAG2))		// Dont announce in Tag modes
				{
					if (gPlayerInfo[p1].onThisMachine || gPlayerInfo[p2].onThisMachine)			// one of them must be on this machine
					{
						if ((!gPlayerInfo[p1].isComputer) || (!gPlayerInfo[p2].isComputer))		// and one of them must be human
						{
							switch(TickCount() & 0x3)										// use tick count as a random # generator since we cannot get a real Random # b/c that would hoze the seed
							{
								case	0:
										PlayAnnouncerSound(EFFECT_GOTTAHURT, false, .3);
										break;

								case	1:
										PlayAnnouncerSound(EFFECT_NICEDRIVING, false, .4);
										break;

								case	2:
										PlayAnnouncerSound(EFFECT_COSTYA, false, .4);
										break;

								case	3:
										PlayAnnouncerSound(EFFECT_WATCHIT, false, .4);
										break;
							}
						}
					}
				}
			}

		}
	}
				/* MAKE SOUND */

	if (relSpeed > 500.0f)
		PlayEffect3D(EFFECT_CRASH, &gCoord);
}




#pragma mark -


/**************** DO CPU CONTROL: CAR ***************/
//
// INPUT:	theNode = the node of the player
//

static void DoCPUControl_Car(ObjNode *theNode)
{
short           player;
float			fps = gFramesPerSecondFrac;
Boolean			onWater;

    player = theNode->PlayerNum;
    gPlayerInfo[player].controlBits = 0;                                     // assume nothing
	gPlayerInfo[player].controlBits_New = 0;

	gPlayerInfo[player].attackTimer -= fps;									// dec this timer


			/* SEE IF FROZEN */

	if (gPlayerInfo[player].frozenTimer > 0.0f)
		return;

	onWater = gPlayerInfo[player].onWater;

		/*********************************/
		/* SEE IF NOT MOVING OR IS STUCK */
		/*********************************/

	if (!gNoCarControls)													// see if control is allowed
	{
		gPlayerInfo[player].oldPositionTimer -= fps;
		if (gPlayerInfo[player].oldPositionTimer <= 0.0f)					// see if time to do the check
		{
			float	stuckDist;

			gPlayerInfo[player].oldPositionTimer += POSITION_TIMER;			// reset timer

			if (onWater)
				stuckDist = 40.0f;
			else
				stuckDist = 80.0f;

			if (CalcDistance3D(gPlayerInfo[player].oldPosition.x, gPlayerInfo[player].oldPosition.y, gPlayerInfo[player].oldPosition.z,
								gCoord.x, gCoord.y, gCoord.z) < stuckDist)		// see if player isnt moving
			{
				if (gPlayerInfo[player].reverseTimer > 0.0f)					// if was reversing then go forward again
					gPlayerInfo[player].reverseTimer = 0;
				else
					gPlayerInfo[player].reverseTimer = 4.0f;					// try moving backwards to get unstuck
			}
			else
			{
				gPlayerInfo[player].reverseTimer = 0;						// player is NOT stuck, so go forward
			}

			gPlayerInfo[player].oldPosition = gCoord;						// remember position
		}
	}


	if ((theNode->StatusBits & STATUS_BIT_ONGROUND) || onWater)
	{
		Boolean			giveGas, brake;
		float			pathVarianceAngle, dot;
		float			x,z;
		OGLVector2D		pathVec;
		short			avoidTurn;

		brake = false;														// assume not braking
		giveGas = true;														// assume giving gas


				/*************************************/
				/* DO STEERING VIA PATH OR AVOIDANCE */
				/*************************************/

			/* SEE IF THERE ARE ANY OBJECTS AHEAD THAT WE SHOULD AVOID */

		avoidTurn = DoCPU_AvoidObjects(theNode);
		if (avoidTurn != 0)
		{
			if (avoidTurn == -1)
				gPlayerInfo[player].analogSteering.x = -1.0f;
			else
				gPlayerInfo[player].analogSteering.x = 1.0f;
		}

				/********************/
				/* DO PATH STEERING */
				/********************/

		else
		{
			if (CalcPathVectorFromCoord(gCoord.x, gCoord.y, gCoord.z, &pathVec))				// get path vector
			{
			    float       cross,r;
			    OGLVector2D aimVec;

			    r = theNode->Rot.y;												// get aim vector of car
			    aimVec.x = -sin(r);
			    aimVec.y = -cos(r);
	            cross = OGLVector2D_Cross(&pathVec, &aimVec);       			// the sign of the cross product will tell us which way to turn
	            dot = OGLVector2D_Dot(&pathVec, &aimVec);          				// also get dot product
	    		pathVarianceAngle = acos(dot);                     				// convert dot to angle

	    		if (gPlayerInfo[player].reverseTimer > 0.0f)					// if reversing then change steering
	    			cross = -cross;

	    		if (pathVarianceAngle > (PI/14))								// see if outside of tolerance
				{
					if (cross > 0.0f)
						gPlayerInfo[player].analogSteering.x = -1.0f;
					else
						gPlayerInfo[player].analogSteering.x = 1.0f;
				}
			}
			else
			{
				pathVec.x = 1;													// no path found, so set default values
				pathVec.y = 0;
				pathVarianceAngle = 0;
			}
			gPlayerInfo[player].pathVec = pathVec;								// keep a copy

				/******************************************/
				/* SEE IF NEED TO ACCEL, COAST, OR BRAKE  */
				/******************************************/

			if (gPlayerInfo[player].isPlaning || gPlayerInfo[player].greasedTiresTimer || (fabs(theNode->DeltaRot.y) > PI))	// if sliding or spinning then brake!
				brake = true;
			else
			if ((theNode->Speed2D > 2500.0f) && (gGamePrefs.difficulty > DIFFICULTY_EASY))			// if we're going fast then see if we need to slow
			{
				OGLVector2D	futurePathVec;

				if (pathVarianceAngle > (PI/3))									// if we're aiming extremely the wrong way, then brake
				{
					brake = true;
				}
				else
				{
					/* GET PATH VECTOR FAR AHEAD OF US TO SEE IF WE'RE ABOUT TO TURN SHARP */

					x = gCoord.x + (gDelta.x * .9f);									// project n seconds into the future
					z = gCoord.z + (gDelta.z * .9f);
					if (CalcPathVectorFromCoord(x, gCoord.y, z, &futurePathVec))		// get path vector
					{
						/* SEE HOW MUCH WE'RE GOING TO NEED TO TURN */

						dot = OGLVector2D_Dot(&pathVec, &futurePathVec);
						if (dot < 0.0f) 												// if (-) then we'll be doing a 180, so brake!
						{
							brake = true;
				  		}
				  		else
				  		if (dot < .3f)													// if its a tight turn, then just let off gas
				  		{
				  			giveGas = false;
				  		}
					}
				}
			}
		}

			/************************************/
			/* SET BRAKE, FORWARD/BACKWARD KEYS */
			/************************************/

		if (brake)
		{
			gPlayerInfo[player].controlBits |= (1L << kControlBit_Brakes);
		}
		else
		if (giveGas)
		{
			if (gPlayerInfo[player].reverseTimer > 0.0f)						// see if going in reverse
			{
			    gPlayerInfo[player].controlBits |= (1L << kControlBit_Backward);
				if ((gPlayerInfo[player].reverseTimer -= fps) < 0.0f)			// dec reverse timer
					gPlayerInfo[player].reverseTimer = 0;
			}
			else
			    gPlayerInfo[player].controlBits |= (1L << kControlBit_Forward);	// go forward
		}
	}


				/* SEE IF CPU PLAYER SHOULD ATTACK */

	DoCPUPowerupLogic(theNode, player);



            /* HANDLE LIKE A REAL PLAYER */

    DoPlayerControl_Car(theNode);
}


/********************* DO CPU:  AVOID OBJECTS *****************************/
//
// OUTPUT:  -1 = turn left, 0 = nothing to avoid, +1 = turn right
//

short DoCPU_AvoidObjects(ObjNode *theCar)
{
ObjNode 		*targetNode;
OGLVector2D		motionVec,vecToTarget;
float			tx,tz, angle, cross;

			/* GET CAR'S MOTION VECTOR */

	motionVec.x = -sin(theCar->Rot.y);
	motionVec.y = -cos(theCar->Rot.y);


			/*****************************/
			/* SCAN FOR OBJECTS TO AVOID */
			/*****************************/

	targetNode = gFirstNodePtr;

	do
	{
		if (targetNode->CType & CTYPE_AVOID)												// see if this object is an avoidance object
		{
			tx = targetNode->Coord.x;														// get target coords
			tz = targetNode->Coord.z;

					/* SEE IF ITS CLOSE */

			if (CalcQuickDistance(tx, tz, gCoord.x, gCoord.z) < (theCar->Speed2D * 1.0f))	// see if within n seconds impact range
			{
						/* CALC VECTOR TO OBJECT */

				vecToTarget.x = tx - gCoord.x;
				vecToTarget.y = tz - gCoord.z;
				FastNormalizeVector2D(vecToTarget.x, vecToTarget.y, &vecToTarget, true);

					/* SEE IF WE'RE HEADED RIGHT FOR IT */

				angle = acos(OGLVector2D_Dot(&motionVec, &vecToTarget));				// angle to target
				if (angle < (PI/10.0f))													// if we're aiming almost directly at it, then avoid
				{
					cross = OGLVector2D_Cross(&motionVec, &vecToTarget);			// cross product tells us which way to turn
					if (cross > 0.0f)
						return(-1);
					else
						return(1);
				}
			}
		}

				/* NEXT NODE */

		targetNode = targetNode->NextNode;
	}
	while (targetNode != nil);

	return(0);
}


/****************** DO CPU POWERUP LOGIC ************************/

void DoCPUPowerupLogic(ObjNode *carObj, short playerNum)
{
short	powType;

	if (gGamePrefs.difficulty <= DIFFICULTY_EASY)			// CPU doesnt shoot in easy mode
		return;

	if (gPlayerInfo[playerNum].attackTimer > 0.0f)				// see if allowed to do this yet
		return;

	if (gPlayerInfo[playerNum].powQuantity == 0)			// if I'm empty then dont do anything
		return;

	powType = gPlayerInfo[playerNum].powType;				// get pow type
	switch(powType)
	{
		case	POW_TYPE_NONE:
				break;

		case	POW_TYPE_NITRO:
				DoCPUPOWLogic_Nitro(carObj, playerNum);
				break;

		case	POW_TYPE_ROMANCANDLE:
				DoCPUPOWLogic_RomanCandle(playerNum);
				break;

		case	POW_TYPE_BIRDBOMB:
		case	POW_TYPE_BOTTLEROCKET:
		case	POW_TYPE_TORPEDO:
				DoCPUPOWLogic_BottleRocket(playerNum);
				break;

		case	POW_TYPE_MINE:
		case	POW_TYPE_OIL:
				DoCPUPOWLogic_Mine(playerNum);
				break;

		default:
				DoCPUWeaponLogic_Standard(carObj, playerNum, powType);
	}
}


/********************* DO CPU POWERUP LOGIC:  NITRO *********************/
//
// Called when CPU car has a Nitro POW.
//

static void DoCPUPOWLogic_Nitro(ObjNode *carObj, short playerNum)
{
	(void) carObj;

	if (gPlayerInfo[playerNum].nitroTimer > 0.0f)						// if already doing a nitro then dont do another one
		return;

	if (gPlayerInfo[playerNum].isPlaning)								// if planing then dont do it
		return;

	if (fabs(gPlayerInfo[playerNum].steering) > .4f)					// if doing heavy steer then dont nitro
		return;

	if (gPlayerInfo[playerNum].distToFloor > 10.0f)						// if off ground, then dont
		return;

	ActivateNitroPOW(playerNum);
}


/****************** DO CPU WEAPONS LOGIC:  STANDARD ********************/
//
// Standard objects are those that can be thrown easily.
//

static void DoCPUWeaponLogic_Standard(ObjNode *carObj, short playerNum, short powType)
{
float	dist,dist2;
Boolean	attackCPUCars,behind;
short	bestFront,bestBack,bestShot;

	if (!(carObj->StatusBits & STATUS_BIT_ONGROUND))				// must be on the ground to fire
		goto no_target;

	if (gPlayerInfo[playerNum].isPlaning)						// cant fire while planing
		goto no_target;


		/* CPU CARS ONLY ATTACK OTHER CPU CARS IF THEY'RE IN FRONT OF PLAYER */

	if (gIsSelfRunningDemo)							// lots of action in SRD mode
	{
		attackCPUCars = true;
	}
	else
	{
		if (gGamePrefs.difficulty == DIFFICULTY_HARD)
		{
			if (gPlayerInfo[playerNum].place < gWorstHumanPlace)
				attackCPUCars = true;
			else
				attackCPUCars = false;
		}
		else
			attackCPUCars = true;
	}


			/***************************************************/
			/* GATHER SOME INFORMATION ABOUT THE OTHER PLAYERS */
			/****************************************************/

	bestFront = FindClosestPlayerInFront(carObj, 11000, attackCPUCars, &dist, .6);
	bestBack = FindClosestPlayerInBack(carObj, 8000, attackCPUCars, &dist2, -.6);

			/* SEE IF NOTHING */

	if ((bestFront == -1) && (bestBack == -1))
	{
		gPlayerInfo[playerNum].targetedPlayer = -1;				// nobody is targeted
		return;
	}

		/* SEE WHICH IS BEST */

	if ((bestFront != -1) && (bestBack != -1))					// if targets in front & back
	{
		if (dist < 2000.0f)										// if forward is too close, then do back
		{
			behind = true;
			bestShot = bestBack;
		}
		else
		{
			if (dist < dist2)									// choose the closest
			{
				behind = false;
				bestShot = bestFront;
			}
			else
			{
				behind = true;
				bestShot = bestBack;
			}
		}
	}

	else
	if (bestFront != -1)
	{
		behind = false;
		bestShot = bestFront;
	}
	else
	{
		behind = true;
		bestShot = bestBack;
	}


			/********************************/
			/* IF WE GOT ANYONE, THEN FIRE! */
			/********************************/

	if (bestShot != -1)
	{
		if (gPlayerInfo[playerNum].targetedPlayer == bestShot)						// if this is the guy we've been targeting, then see if we can shoot
		{
			gPlayerInfo[playerNum].targetingTimer += gFramesPerSecondFrac;			// inc the target timer
			if (gPlayerInfo[playerNum].targetingTimer > .8f)						// see if we can fire!
			{
				if (fabs(gPlayerInfo[playerNum].steering) < .4f)					// one final check:  if doing heavy steer, then dont fire yet
				{
					if (behind)														// see if throw forward or backward
						gPlayerInfo[playerNum].controlBits_New |= (1L << kControlBit_ThrowBackward);
					else
						gPlayerInfo[playerNum].controlBits_New |= (1L << kControlBit_ThrowForward);
					gPlayerInfo[playerNum].attackTimer = RandomFloat() * 1.1f;
				}
			}
		}
		else
		{
			gPlayerInfo[playerNum].targetedPlayer = bestShot;						// start targeting this guy
			gPlayerInfo[playerNum].targetingTimer = 0;
		}
	}
	else
	{
no_target:
		gPlayerInfo[playerNum].targetedPlayer = -1;									// nobody is targeted
		return;
	}
}



/****************** DO CPU WEAPONS LOGIC:  ROMAN CANDLE ********************/

static void DoCPUPOWLogic_RomanCandle(short playerNum)
{
float	dist;
Boolean	attackCPUCars;
short	targetP;


		/* CPU CARS ONLY ATTACK OTHER CPU CARS IF THEY'RE IN FRONT OF PLAYER */

	if (gPlayerInfo[playerNum].place < gWorstHumanPlace)
		attackCPUCars = true;
	else
		attackCPUCars = false;



			/****************************************/
			/* SEE IF ANYONE IS IN RANGE TO FIRE AT */
			/****************************************/

	targetP = FindClosestPlayer(gPlayerInfo[playerNum].objNode, gPlayerInfo[playerNum].coord.x, gPlayerInfo[playerNum].coord.z,
								 BOTTLE_ROCKET_RANGE, attackCPUCars, &dist);


			/********************************/
			/* IF WE GOT ANYONE, THEN FIRE! */
			/********************************/

	if (targetP != -1)
	{
		if (gPlayerInfo[playerNum].targetedPlayer == targetP)						// if this is the guy we've been targeting, then see if we can shoot
		{
			gPlayerInfo[playerNum].targetingTimer += gFramesPerSecondFrac;			// inc the target timer
			if (gPlayerInfo[playerNum].targetingTimer > 1.0f)						// see if we can fire!
			{
				gPlayerInfo[playerNum].controlBits_New |= (1L << kControlBit_ThrowForward);
				gPlayerInfo[playerNum].attackTimer = RandomFloat() * .7f;
			}
		}
		else
		{
			gPlayerInfo[playerNum].targetedPlayer = targetP;						// start targeting this guy
			gPlayerInfo[playerNum].targetingTimer = 0;
		}
	}
	else
	{
		gPlayerInfo[playerNum].targetedPlayer = -1;									// nobody is targeted
		return;
	}
}


/****************** DO CPU WEAPONS LOGIC:  MINE ********************/

static void DoCPUPOWLogic_Mine(short playerNum)
{
	gPlayerInfo[playerNum].controlBits_New |= (1L << kControlBit_ThrowBackward);
	gPlayerInfo[playerNum].attackTimer = RandomFloat() * 2.0f;
}




/****************** DO CPU WEAPONS LOGIC:  BOTTLE ROCKET ********************/

static void DoCPUPOWLogic_BottleRocket(short playerNum)
{
float	dist;
int		targetP;
Boolean	attackCPUCars, inFront;


		/* CPU CARS ONLY ATTACK OTHER CPU CARS IF THEY'RE IN FRONT OF PLAYER */

	if (gPlayerInfo[playerNum].place < gWorstHumanPlace)
		attackCPUCars = true;
	else
		attackCPUCars = false;



			/****************************************/
			/* SEE IF ANYONE IS IN RANGE TO FIRE AT */
			/****************************************/

	targetP = FindClosestPlayerInFront(gPlayerInfo[playerNum].objNode, BOTTLE_ROCKET_RANGE, attackCPUCars, &dist, 0);		// look for a target in front
	if (targetP == -1)
	{
		inFront = false;
		targetP = FindClosestPlayerInBack(gPlayerInfo[playerNum].objNode, BOTTLE_ROCKET_RANGE, attackCPUCars, &dist, 0);	// look for target in back
	}
	else
		inFront = true;


			/********************************/
			/* IF WE GOT ANYONE, THEN FIRE! */
			/********************************/

	if (targetP != -1)
	{
		if (gPlayerInfo[playerNum].targetedPlayer == targetP)						// if this is the guy we've been targeting, then see if we can shoot
		{
			gPlayerInfo[playerNum].targetingTimer += gFramesPerSecondFrac;			// inc the target timer
			if (gPlayerInfo[playerNum].targetingTimer > 1.0f)						// see if we can fire!
			{
				if (inFront)
					gPlayerInfo[playerNum].controlBits_New |= (1L << kControlBit_ThrowForward);
				else
					gPlayerInfo[playerNum].controlBits_New |= (1L << kControlBit_ThrowBackward);

				gPlayerInfo[playerNum].attackTimer = RandomFloat() * .7f;
			}
		}
		else
		{
			gPlayerInfo[playerNum].targetedPlayer = targetP;						// start targeting this guy
			gPlayerInfo[playerNum].targetingTimer = 0;
		}
	}
	else
	{
		gPlayerInfo[playerNum].targetedPlayer = -1;									// nobody is targeted
		return;
	}
}


#pragma mark -


/********************** CREATE CAR WHEELS AND HEAD ****************************/

void CreateCarWheelsAndHead(ObjNode *theCar, short playerNum)
{
short			i, carType, sex;
ObjNode			*wheel,*link;

	if (playerNum >= MAX_PLAYERS)
		DoFatalAlert("CreateCarWheelsAndHead: playerNum >= MAX_PLAYERS");

	sex = gPlayerInfo[playerNum].sex;							// get player sex
	if (sex > 1)
		DoFatalAlert("CreateCarWheelsAndHead: sx > 1");

	carType = gPlayerInfo[playerNum].vehicleType;				// get car type
	GAME_ASSERT(carType < NUM_LAND_CAR_TYPES);

	link = theCar;


			/* MAKE WHEELS */

	for (i = 0; i < 4; i++)
	{
		NewObjectDefinitionType def =
		{
			.group		= MODEL_GROUP_CARPARTS,
			.type		= (carType * 4) + CARPARTS_ObjType_Wheel_MammothFL + i,
			.coord		= {0,0,0},
			.slot		= theCar->Slot+1,
			.scale		= theCar->Scale.x,
		};
		if (carType == CAR_TYPE_CHARIOT)
			def.flags |= STATUS_BIT_CLIPALPHA;

		wheel = MakeNewDisplayGroupObject(&def);
		if (wheel == nil)
			DoFatalAlert("CreateCarWheelsAndHead: MakeNewDisplayGroupObject failed!");

		wheel->WheelSpinRot = 0;
		wheel->PlayerNum = playerNum;						// set playernum in this obj

		link->ChainNode = wheel;							// add to chain link
		link = wheel;

		gPlayerInfo[playerNum].wheelObj[i] = wheel;
	}


			/*****************************/
			/* MAKE HEAD SKELETON OBJECT */
			/*****************************/

	NewObjectDefinitionType def =
	{
		.moveCall	= MovePlayer_HeadSkeleton,
		.type		= SKELETON_TYPE_PLAYER_MALE + sex,
		.coord		= {0,0,0},
		.animNum	= PLAYER_ANIM_SIT,
		.slot		= theCar->Slot+1,
		.scale		= BROG_SCALE,
	};

	wheel = MakeNewSkeletonObject(&def);
	if (wheel == nil)
		DoFatalAlert("CreateCarWheelsAndHead: MakeNewSkeletonObject failed!");

	wheel->PlayerNum = playerNum;						// set playernum in this obj

	link->ChainNode = wheel;							// add to chain link

	gPlayerInfo[playerNum].headObj = wheel;


			/************************/
			/* SET OVERRIDE TEXTURE */
			/************************/

	int skinID = gPlayerInfo[playerNum].skin;
	wheel->Skeleton->overrideTexture = gCavemanSkins[sex][skinID];


			/* ALIGN THEM */

	AlignWheelsAndHeadOnCar(theCar);
}


/******************* MOVE PLAYER_HEAD SKELETON ***********************/

static void MovePlayer_HeadSkeleton(ObjNode *head)
{
short		powType;
short		playerNum = head->PlayerNum;

	switch(head->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_THROWBACKWARD:
		case	PLAYER_ANIM_THROWFORWARD:

					/* SEE IF THROW NOW */

				if (head->HEAD_THROW_READY_FLAG)
				{
					Boolean	throwForward = head->Skeleton->AnimNum == PLAYER_ANIM_THROWFORWARD;			// see if throwing forward or backward

					head->HEAD_THROW_READY_FLAG = false;

					powType = gPlayerInfo[playerNum].powType;			// get current weapon type


					/* THROW THE APPROPRIATE WEAPON */

					switch(powType)
					{
						case	POW_TYPE_BONE:
								ThrowBone(playerNum, throwForward);
								break;

						case	POW_TYPE_OIL:
								ThrowOil(playerNum, throwForward);
								break;

						case	POW_TYPE_BIRDBOMB:
								ThrowBirdBomb(playerNum, throwForward);
								break;

						case	POW_TYPE_FREEZE:
								ThrowFreeze(playerNum, throwForward);
								break;
					}

							/* DEC THE INVENTORY */

					DecCurrentPOWQuantity(playerNum);
				}



					/* SEE IF DONE WITH THROW */

				if (head->Skeleton->AnimHasStopped)
				{
					MorphToSkeletonAnim(head->Skeleton, PLAYER_ANIM_SIT, 4.0);
				}
				break;
	}
}




/**************** DO PLAYER CONTROL: CAR ***************/
//
// Moves a player based on its control bit settings.  NOTE:  Also called by CPU player!
//
// INPUT:	theNode = the node of the player
//

static void DoPlayerControl_Car(ObjNode *theNode)
{
uint16_t			playerNum = theNode->PlayerNum;
float			thrust,speed;
float			fps = gFramesPerSecondFrac;
float			steering,analogSteering;

	if (gNoCarControls || (gPlayerInfo[playerNum].frozenTimer > 0.0f) || (gPlayerInfo[playerNum].isEliminated))		// see if no control
	{
		gPlayerInfo[playerNum].braking	 		= false;
		gPlayerInfo[playerNum].gasPedalDown 	= false;
		gPlayerInfo[playerNum].accelBackwards 	= false;
		gPlayerInfo[playerNum].steering 		= 0;
		gPlayerInfo[playerNum].currentThrust 	= 0;
		return;
	}

	speed = theNode->Speed2D;									// get current speed of vehicle
	thrust = 0;													// assume not gassing it


			/****************************/
			/* CHECK GAS PEDAL & BRAKES */
			/****************************/
			//
			// We need to check this even if we're not on the ground
			// since we use the pedal info for engine RPM effects.
			//

				/* SEE IF DOING NITRO */

	if (gPlayerInfo[playerNum].nitroTimer > 0.0f)
	{
		gPlayerInfo[playerNum].braking = false;
		gPlayerInfo[playerNum].gasPedalDown = true;
		thrust = NITRO_ACCELERATION;
	}

					/* NO NITRO */

	else
	if (GetControlState(playerNum, kControlBit_Brakes) || gPlayerInfo[playerNum].raceComplete)
	{
		gPlayerInfo[playerNum].braking = true;
		gPlayerInfo[playerNum].gasPedalDown = false;
	}
	else
	{
		gPlayerInfo[playerNum].braking = false;

		if (GetControlState(playerNum, kControlBit_Forward))
		{
			thrust = gPlayerInfo[playerNum].carStats.acceleration;
			gPlayerInfo[playerNum].accelBackwards = false;
			gPlayerInfo[playerNum].gasPedalDown = true;
		}
		else
		if (GetControlState(playerNum, kControlBit_Backward))
		{
			thrust = -gPlayerInfo[playerNum].carStats.acceleration;
			gPlayerInfo[playerNum].accelBackwards = true;
			gPlayerInfo[playerNum].gasPedalDown = true;
		}
		else
			gPlayerInfo[playerNum].gasPedalDown = false;
	}


			/**************************/
			/* SEE IF TURN LEFT/RIGHT */
			/**************************/

	steering = gPlayerInfo[playerNum].steering;
	analogSteering = gPlayerInfo[playerNum].analogSteering.x;

			/* CHECK FOR DIGITAL STEERING */
			//
			// If user is using keys, then the steering values will be at their maximums.
			// In this case, do automated incremental steering instead of analog manual.
			//

	if ((gAnalogSteeringTimer[playerNum] <= 0.0f) || gNetGameInProgress)				// see if not doing analog (ignore timer in net mode cuz it caused sync problems)
	{
		if (analogSteering == -1.0f)
		{
			steering -= gUserPhysics.steeringResponsiveness * fps;
			if (steering < -1.0f)
				steering = -1.0f;
		}
		else
		if (analogSteering == 1.0f)
		{
			steering += gUserPhysics.steeringResponsiveness * fps;
			if (steering > 1.0f)
				steering = 1.0f;
		}
				/* DIGITAL AUTO-CENTER STEERING */
		else
		if (analogSteering == 0.0f)
		{
			if (steering < 0.0f)
			{
				steering += gUserPhysics.steeringResponsiveness * fps;
				if (steering > 0.0f)
					steering = 0.0f;
			}
			else
			if (steering > 0.0f)
			{
				steering -= gUserPhysics.steeringResponsiveness * fps;
				if (steering < 0.0f)
					steering = 0.0f;
			}
		}
		else
			steering = analogSteering;									// analog (should only get here in a net game)
	}

			/* WE ARE DOING ANALOG STEERING */
	else
	{
		steering = analogSteering;
	}

	gPlayerInfo[playerNum].steering = steering;							// update value


	/***************************************/
	/* IF ON GROUND, THEN LET'S ACCELERATE */
	/***************************************/


	if ((theNode->StatusBits & STATUS_BIT_ONGROUND)	&& (!gNoCarControls))	// must be on the ground for most controls to have an effect
	{
		if ((speed < 400.0f) && (thrust != 0.0f))				// give a little boost if going too slow
		{
			if (thrust < 0.0f)
				thrust = -5000;
			else
				thrust = 5000;
		}

		if (gPlayerInfo[playerNum].isPlaning)					// if we're planing then thrust has no effect
			thrust = 0;

	}
	else
	if (!gPlayerInfo[playerNum].onWater)
		thrust = 0;												// no thrust unless we're on water


	gPlayerInfo[playerNum].currentThrust = thrust;				// remember the thrust

		/******************/
		/* HANDLE WEAPONS */
		/******************/

	CheckPOWControls(theNode);
}


/********************* UPDATE TIRE SKID MARKS *****************************/

static void UpdateTireSkidMarks(ObjNode *car)
{
short	p = car->PlayerNum;
Boolean	stopSkid = false;
static const short skidEffect[] = {EFFECT_SKID, EFFECT_SKID2, EFFECT_SKID3};

	gPlayerInfo[p].skidSoundTimer -= gFramesPerSecondFrac;			// always dec this so we know if we can play skid sound again when the time comes

	if (gPlayerInfo[p].noSkids)										// see if tile attribute says not skidding
		goto stopSkidding;

			/*********************************/
			/* SEE IF SHOULD NOT BE SKIDDING */
			/*********************************/

	if (!(car->StatusBits & STATUS_BIT_ONGROUND))						// if not on ground then make sure current skid is terminated
		stopSkid = true;

	if ((!gPlayerInfo[p].isPlaning) && (!gPlayerInfo[p].braking))	// must be planing or braking for forced skidmarks
		stopSkid = true;

	if (gPlayerInfo[p].braking && (car->Speed2D < 300.0f))				// if not going fast enough to brake-skid, then bail
		stopSkid = true;

	if (stopSkid)
	{
stopSkidding:
		DetachOwnerFromSkid(car);
		gPlayerInfo[p].makingSkid = false;
	}
	else
	{
		AddToTireSkidMarks(car, p);

		if (!gPlayerInfo[p].makingSkid)								// if new skid, then make sound
		{
			if (gPlayerInfo[p].skidSoundTimer <= 0.0f)				// make sure its okay to do it now
			{
				float vol = 1.0f;

				if (gPlayerInfo[p].isComputer)						// preserve my hearing from excessive CPU skidding
					vol = 0.5f;

				int effect = EFFECT_SKID;

				if (gPlayerInfo[p].greasedTiresTimer > 0.0f)		// always make same sound if greased tires
					effect = EFFECT_SKID2;
				else if (gPlayerInfo[p].braking)					// always make same sound if braking
					effect = EFFECT_SKID;
				else												// otherwise random
					effect = skidEffect[RandomRange(0,2)];

				gPlayerInfo[p].skidChannel = PlayEffect_Parms3D(effect, &gCoord, NORMAL_CHANNEL_RATE, vol);
				gPlayerInfo[p].skidSoundTimer = gPlayerInfo[p].isComputer ? 1.3f : 0.8f;
			}
		}
		gPlayerInfo[p].makingSkid = true;
	}


}



/********************* ADD TO TIRE SKID MARKS *****************************/

static void AddToTireSkidMarks(ObjNode *car, short p)
{
ObjNode		*wheels[4];
int			i;
float		dist,dot;
Boolean		makeNewSegment,skidSmoke;
OGLVector2D	myDir;
OGLPoint3D	coord;

static const OGLPoint3D	wheelOffset[4] =
{
	{-20,0,0},				// f/l
	{20,0,0},				// f/r
	{20,0,0},				// b/r
	{-20,0,0},				// b/l
};

			/* SEE IF NEED TO MAKE NEW SEGMENT OR JUST STRETCH LAST ONE */

	dist = CalcQuickDistance(gPlayerInfo[p].lastSkidSegCoord.x, gPlayerInfo[p].lastSkidSegCoord.y, gCoord.x, gCoord.z);

	FastNormalizeVector2D(gDelta.x, gDelta.z, &myDir, true);
	dot = OGLVector2D_Dot(&gPlayerInfo[p].lastSkidVector, &myDir);

	makeNewSegment = (dist > 200.0f) || (fabs(dot) < .9f);
	if (makeNewSegment)
	{
		gPlayerInfo[p].lastSkidSegCoord.x = gCoord.x;
		gPlayerInfo[p].lastSkidSegCoord.y = gCoord.z;
		gPlayerInfo[p].lastSkidVector = myDir;
	}

	if ((fabs(gDelta.x) < EPS) && (fabs(gDelta.z) < EPS))		// we've gotta be moving or else we need to bail
		return;



			/* GET OBJ NODES OF ALL OF THE WHEELS */

	wheels[0] = car->ChainNode;									// get front left
	if (!wheels[0])
		return;

	wheels[1] = wheels[0]->ChainNode;							// get front right
	if (!wheels[1])
		return;

	wheels[2] = wheels[1]->ChainNode;							// get back right
	if (!wheels[2])
		return;

	wheels[3] = wheels[2]->ChainNode;							// get back left
	if (!wheels[3])
		return;


		/****************************/
		/* DO SKID FOR ALL 4 WHEELS */
		/****************************/

	skidSmoke = false;											// assume not smoking

	if (gPlayerInfo[p].greasedTiresTimer <= 0.0f)				// no smoke if sliding on oil
	{
		gPlayerInfo[p].skidSmokeTimer -= gFramesPerSecondFrac;	// check skid timer
		if (gPlayerInfo[p].skidSmokeTimer <= 0.0f)
		{
			gPlayerInfo[p].skidSmokeTimer += SKID_SMOKE_TIMER;
			skidSmoke = true;
		}
	}

	for (i = 0; i < 4; i++)
	{
		OGLMatrix4x4	*m;

		m = &wheels[i]->BaseTransformMatrix;					// point to wheel's transform matrix
		OGLPoint3D_Transform(&wheelOffset[i], m, &coord);		// get wheel world coord

			/* UPDATE SKID MARK */

		if (makeNewSegment)
			DoSkidMark(car, i, coord.x, coord.z, gDelta.x, gDelta.z);
		else
			StretchSkidMark(car, i, coord.x, coord.z, gDelta.x, gDelta.z);


			/* SEE IF MAKE SMOKE */

		if (skidSmoke)
		{
			MakeSkidSmoke(p, &coord);
		}
	}
}


#pragma mark -


/******************* BLAST CARS **************************/
//
// Called when an explosion occurs to see if any cars were hit
//

void BlastCars(short whoThrew, float x, float y, float z, float radius)
{
short	i;
float	d,x2,y2,z2,dx,dy,dz,d2;
ObjNode	*obj;

			/* CHECK EACH CAR */

	for (i = 0; i < gNumTotalPlayers; i++)
	{
		x2 = gPlayerInfo[i].coord.x;
		y2 = gPlayerInfo[i].coord.y;
		z2 = gPlayerInfo[i].coord.z;


		d = CalcDistance3D(x, y, z, x2, y2, z2);						// calc dist to see if in blast zone
		if (d <= radius)
		{
			d2 = radius - d;												// determine blast force

			obj = gPlayerInfo[i].objNode;								// get the car object


				/* CAUSE HEALTH DAMAGE */

			if (gGameMode == GAME_MODE_SURVIVAL)
			{
				PlayerLoseHealth(i, d2 / BLAST_DAMAGE);
			}

				/* CAUSE DROP TORCH */

			else
			if (gGameMode == GAME_MODE_SURVIVAL)
				PlayerDropFlag(obj);


				/* SEE IF SUBMARINES */

			if (gTrackNum == TRACK_NUM_ATLANTIS)
			{
				gPlayerInfo[i].submarineImmobilized = 2.0f;					// immobilize the sub for a few seconds
			}
			else
			{

					/* CALC BLAST VECTOR */

				dx = x2 - x;
				dy = y2 - y;
				dz = z2 - z;


				obj->DeltaRot.y = d2 * .006f;
				obj->DeltaRot.z = d2 * .002f;

				obj->Delta.x += dx * 1.5f;										// add blast motion to deltas
				obj->Delta.y += dy + 1600.0f;
				obj->Delta.z += dz * 1.5f;

				obj->StatusBits &= ~(STATUS_BIT_ONGROUND|STATUS_BIT_ONTERRAIN);	// make it think we were already in the air so the DeltaRot.y depricator will not kick in


				gPlayerInfo[i].greasedTiresTimer = .5f;					// skid out
				gPlayerInfo[i].isPlaning = true;
			}

				/* ANNOUNCE IT IF REALLY CLOSE */

			if (whoThrew != -1)
			{
				if (whoThrew != i)									// it isnt a nice shot if we shot ourselves!
				{
					if (d <= (radius * .7f))
					{
						if ((gPlayerInfo[whoThrew].onThisMachine) && (!gPlayerInfo[whoThrew].isComputer))
						{
							if (TickCount() & 1)										// use tick count as a random # generator since we cannot get a real Random # b/c that would hoze the seed
								PlayAnnouncerSound(EFFECT_NICESHOT, false, .9);
							else
								PlayAnnouncerSound(EFFECT_OHYEAH, false, .9);
						}
					}
				}
			}
		}
	}
}



#pragma mark -


/************************ ALIGN WHEELS AND HEAD ON CAR *****************************/

void AlignWheelsAndHeadOnCar(ObjNode *theCar)
{
short			carType;
Boolean			debris,onWater;
float			dr2,dr,drDiff;
float			fps = gFramesPerSecondFrac;
ObjNode			*wheels[4],*head;
OGLMatrix4x4	*carMatrix;
OGLMatrix4x4	m1,m2,m3;
short			i,playerNum = theCar->PlayerNum;
static const OGLPoint3D	wheelOffsets[NUM_LAND_CAR_TYPES][4] =
{
	//                         Front Left		Front Right		Back Right		Back Left
	[CAR_TYPE_MAMMOTH]     = { {-109,-19,-92},	{109,-19,-92},	{129,4,91},		{-129,4,91} },
	[CAR_TYPE_BONEBUGGY]   = { {-76,-35,-101},	{76,-35,-101},	{84,-9,91},		{-84,-9,91} },
	[CAR_TYPE_GEODE]       = { {-126,-27,-85},	{126,-27,-85},	{126,-27,82},	{-126,-27,82} },
	[CAR_TYPE_LOG]         = { {-112,-49,-122},	{112,-49,-122},	{123,-27,37},	{-123,-27,37} },
	[CAR_TYPE_TURTLE]      = { {-99,-31,-102},	{99,-31,-102},	{109,-23,73},	{-109,-23,73} },
	[CAR_TYPE_ROCK]        = { {-70,-45,-84},	{70,-45,-84},	{96,-38,77},	{-96,-38,77} },
	[CAR_TYPE_TROJANHORSE] = { {-103,-57,-64},	{103,-57,-64},	{103,-57,94},	{-103,-57,94} },
	[CAR_TYPE_OBELISK]     = { {-111,-58,-89},	{111,-58,-89},	{112,-38,79},	{-112,-38,79} },
	[CAR_TYPE_CATAPULT]    = { {-108,-38,-109},	{108,-38,-109},	{112,-10,105},	{-112,-10,105} },
	[CAR_TYPE_CHARIOT]     = { {-122,-28,-80},	{122,-28,-80},	{122,-2,85},	{-122,-2,85} },
};


static const OGLPoint3D	headOffsets[NUM_LAND_CAR_TYPES] =
{
	[CAR_TYPE_MAMMOTH]     = { 0, 58, -10 },
	[CAR_TYPE_BONEBUGGY]   = { 0, 65, -20 },
	[CAR_TYPE_GEODE]       = { 0, 70, 20 },
	[CAR_TYPE_LOG]         = { 0, 85, 0 },
	[CAR_TYPE_TURTLE]      = { 0, 70, 40 },
	[CAR_TYPE_ROCK]        = { 0, 50, 10 },
	[CAR_TYPE_TROJANHORSE] = { 0, 140, 35 },
	[CAR_TYPE_OBELISK]     = { 0, 85, 22 },
	[CAR_TYPE_CATAPULT]    = { 0, 85, 10 },
	[CAR_TYPE_CHARIOT]     = { 0, 85, 10 },
};


	if (playerNum >= gNumTotalPlayers)
		DoFatalAlert("AlignWheelsAndHeadOnCar: playerNum >= gNumTotalPlayers");

	carType = gPlayerInfo[playerNum].vehicleType;				// get car type
	onWater = gPlayerInfo[playerNum].onWater;

	carMatrix = &theCar->BaseTransformMatrix;					// point to car's transform matrix

	GAME_ASSERT(carType < NUM_LAND_CAR_TYPES);


			/* GET OBJ NODES OF ALL OF THE WHEELS */

	wheels[0] = theCar->ChainNode;								// get front left
	if (!wheels[0])
		goto err;

	wheels[1] = wheels[0]->ChainNode;							// get front right
	if (!wheels[1])
		goto err;

	wheels[2] = wheels[1]->ChainNode;							// get back right
	if (!wheels[2])
		goto err;

	wheels[3] = wheels[2]->ChainNode;							// get back left
	if (!wheels[3])
		goto err;

	head = wheels[3]->ChainNode;								// get head
	if (!head)
		goto err;


			/*********************************/
			/* CALCULATE WHEEL TURNING SPEED */
			/*********************************/

			/* CALC WHEEL ROT BASED ON RPM */

	dr =  gPlayerInfo[playerNum].currentRPM * fps * 30.0f;

	if (gPlayerInfo[playerNum].accelBackwards)												// see if going backward
		dr = -dr;

			/* CALC WHEEL ROT BASED ON SPEED */

	dr2 = theCar->Speed2D * 0.007f * fps;
	dr2 *= fabs(gPlayerInfo[playerNum].skidDot);											// less speed as skid more perpendicular


			/* DETERMINE WHICH TO USE */

	drDiff = dr2 - fabs(dr);																// calc diff between rpm and speed based rotation deltas

	if (drDiff > 0.0f)																		// see if speed rot is > rpm rot
	{
		dr = dr2;
		if (gPlayerInfo[playerNum].movingBackwards)
			dr = -dr;
	}

		/************************************************************/
		/* IF THERE'S A BIG DIFFERENCE, THEN THE TIRES ARE DRAGGING */
		/************************************************************/

	debris = gPlayerInfo[playerNum].tiresAreDragging = false;								// assume not dragging

	if ((theCar->StatusBits & STATUS_BIT_ONGROUND) || onWater)								// must be on ground or water to normal drag
	{
		if (gPlayerInfo[playerNum].alwaysDoDrag)												// check to see if we're doing an easy drag effect
		{
			if (gPlayerInfo[playerNum].objNode->Speed2D > 200.0f)
				debris = true;
		}
		else
		{
			drDiff *= gFramesPerSecond;															// adjust to a frame rate independant value

			if (drDiff <  -4.0f)																	// if big difference, then we're dragging our tires / peeling out
			{
				gPlayerInfo[playerNum].tiresAreDragging = true;

				gPlayerInfo[playerNum].dragDebrisTimer -= fps;									// check timer to see if can spew debris from wheels
				if (gPlayerInfo[playerNum].dragDebrisTimer <= 0.0f)
				{
					gPlayerInfo[playerNum].dragDebrisTimer += .03f;
					debris = true;
				}
			}
		}
	}



			/***************************************/
			/* SET TRANSFORM MATRIX FOR EACH WHEEL */
			/***************************************/

	for (i = 0; i < 4; i++)
	{
			/* DO WHEEL ROTATION */

		if (!gPlayerInfo[playerNum].braking)														// wheels do not spin during braking
			wheels[i]->WheelSpinRot -= dr;

		OGLMatrix4x4_SetRotate_X(&m1, wheels[i]->WheelSpinRot);										// set rotation matrix


		if (i < 2)																					// set y rot for turning on front 2 wheels
		{
			wheels[i]->Rot.y = gPlayerInfo[playerNum].steering * -.6f;
			OGLMatrix4x4_SetRotate_Y(&m2, wheels[i]->Rot.y);
			OGLMatrix4x4_Multiply(&m1, &m2, &m3);
			m1 = m3;
		}

		OGLMatrix4x4_SetTranslate(&m2, wheelOffsets[carType][i].x, wheelOffsets[carType][i].y, wheelOffsets[carType][i].z);	// locate tire on axel
		OGLMatrix4x4_Multiply(&m1, &m2, &m3);
		OGLMatrix4x4_Multiply(&m3, carMatrix, &wheels[i]->BaseTransformMatrix);

		SetObjectTransformMatrix(wheels[i]);														// make sure to update it internally


				/* SEE IF SHOOT DEBRIS FROM WHEEL */

		if (debris)
		{
			SpewDebrisFromWheel(playerNum, theCar, wheels[i]);
		}

	}


			/* SET FOR HEAD */

	OGLMatrix4x4_SetScale(&m2, head->Scale.x, head->Scale.x, head->Scale.x);						// use head's scale value
	OGLMatrix4x4_SetTranslate(&m1, headOffsets[carType].x, headOffsets[carType].y, headOffsets[carType].z);
	OGLMatrix4x4_Multiply(&m2, &m1, &m3);
	OGLMatrix4x4_Multiply(&m3, carMatrix, &head->BaseTransformMatrix);
	SetObjectTransformMatrix(head);
	return;

err:
	DoFatalAlert("AlignWheelsAndHeadOnCar:  chain node is nil");
}



/********************** SPEW DEBRIS FROM WHEEL ***********************/
//
// Called from above when tire is dragging on ground
//

static void SpewDebrisFromWheel(short p, ObjNode *carObj, ObjNode *wheelObj)
{
OGLMatrix4x4	*m;
OGLPoint3D		coord;
int				particleGroup,magicNum;
short			textureNum;

	if (gFramesPerSecond < 13.0f)									// help us out if speed is really bad
		return;


			/* GET COORD TO MAKE DEBRIS AT */

	m = &wheelObj->BaseTransformMatrix;								// point to wheel's transform matrix

	coord.x = m->value[M03];										// extract translation from matrix
	coord.y = m->value[M13];
	coord.z = m->value[M23];

	if (gPlayerInfo[p].onWater)										// if on water, then move water spray up a little
	{
		if (gTrackNum == TRACK_NUM_TARPITS)							// water is tar, so don't splash
			return;

		coord.y += 50.0f;
	}

			/******************/
			/* MAKE PARTICLES */
			/******************/

	particleGroup 	= gPlayerInfo[p].dragDebrisParticleGroup;
	magicNum 		= gPlayerInfo[p].dragDebrisMagicNum;
	textureNum		= gPlayerInfo[p].dragDebrisTexture;

	if (textureNum == -1)											// see if no debris here
		return;

			/* SEE IF MAKE NEW PARTICLE GROUP */

	if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
	{
		NewParticleGroupDefType	groupDef;
new_group:
		gPlayerInfo[p].dragDebrisMagicNum = magicNum = MyRandomLong();			// generate a random magic num

		groupDef.magicNum				= magicNum;
		groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
		groupDef.flags					= PARTICLE_FLAGS_BOUNCE;
		groupDef.gravity				= 3000;
		groupDef.magnetism				= 0;
		groupDef.baseScale				= 30;
		groupDef.decayRate				=  1.2;
		groupDef.fadeRate				= 1.4;
		groupDef.particleTextureNum		= textureNum;

		switch(textureNum)														// set appropriate blending mode
		{
			case	PARTICLE_SObjType_Splash:
					groupDef.srcBlend				= GL_ONE;
					groupDef.dstBlend				= GL_ONE;
					break;

			default:
					groupDef.srcBlend				= GL_SRC_ALPHA;
					groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;

		}
		gPlayerInfo[p].dragDebrisParticleGroup = particleGroup = NewParticleGroup(&groupDef);
	}


	if (particleGroup != -1)
	{
		NewParticleDefType	newParticleDef;
		OGLVector3D			smokeDelta;
		float				r,speed;

			/* IF TEXTURE CHANGED, THEN MAKE NEW GROUP */

		if (textureNum != gParticleGroups[particleGroup]->particleTextureNum)
			goto new_group;


			/* ADD PARTICLES TO GROUP */

		r = wheelObj->Rot.y + carObj->Rot.y;								// calc rot of wheel in world
		r += (RandomFloat() - .5f) * .3f;									// offset randomly
		speed = gPlayerInfo[p].currentRPM * 1200.0f;


		smokeDelta.x = sin(r) * speed;
		smokeDelta.y = speed * (.1f + RandomFloat() * .1f);
		smokeDelta.z = cos(r) * speed;

		if (gPlayerInfo[p].accelBackwards)
		{
			smokeDelta.x = -smokeDelta.x;
			smokeDelta.z = -smokeDelta.z;
		}


		newParticleDef.groupNum		= particleGroup;
		newParticleDef.where		= &coord;
		newParticleDef.delta		= &smokeDelta;
		newParticleDef.scale		= RandomFloat() + 1.0f;
		newParticleDef.rotZ			= RandomFloat() * PI2;
		newParticleDef.rotDZ		= (RandomFloat()-.5f) * 2.0f;
		newParticleDef.alpha		= .8;
		AddParticleToGroup(&newParticleDef);
	}


}

















