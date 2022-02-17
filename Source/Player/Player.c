/****************************/
/*   	PLAYER.C   			*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "objects.h"
#include "misc.h"
#include "player.h"
#include "mobjtypes.h"
#include "skeletonobj.h"
#include 	"input.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "collision.h"
#include "effects.h"
#include "camera.h"
#include "sound2.h"
#include "3dmath.h"
#include "terrain.h"
#include "triggers.h"
#include "sobjtypes.h"
#include "infobar.h"
#include "file.h"

extern	OGLPoint3D				gCoord;
extern	OGLVector3D				gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float					gFramesPerSecondFrac;
extern	short					gNumCollisions,gWorstHumanPlace,gWhoIsIt,gNumPlayersEliminated;
extern	CollisionRec			gCollisionList[];
extern	unsigned long 			gScore;
extern	float					gMyDistToFloor;
extern	float 					gCameraDistFromMe;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	uint32_t 					gInfobarUpdateBits,gAutoFadeStatusBits;
extern	Boolean					gIsNetworkHost,gIsNetworkClient,gNetGameInProgress;
extern	long					gNumCheckpoints;
extern	int						gGameMode,gTrackNum,gNumSplitScreenPanes;
extern	Boolean				gTrackCompleted;
extern	float				gTrackCompletedCoolDownTimer;
extern	SavePlayerType		gPlayerSaveData;
extern	PrefsType			gGamePrefs;

/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/


short		gNumRealPlayers = 1;                // # of actual human players (on computer and/or network)
short       gNumTotalPlayers = 0;               // # of total player characters in game (real + CPU)
short		gNumLocalPlayers = 1;				// 2 if split-screen, otherwise 1

ObjNode		*gCurrentPlayer;
short		gCurrentPlayerNum = 0;				// current player number of player being processed (via MovePlayer et. al.)
short		gMyNetworkPlayerNum = 0;			// not the same as the ClientID, just a player number that the host assigns.  The host is always player #0

PlayerInfoType	gPlayerInfo[MAX_PLAYERS];

OGLColorRGB	gTagColor;

/******************** INIT PLAYER INFO ***************************/
//
// Called once at beginning of game (after network has been setup if needed).
//

void InitPlayerInfo_Game(void)
{
short	i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		gPlayerInfo[i].objNode			= nil;

		gPlayerInfo[i].sex 				= i&1;			// altername male/female

		gPlayerInfo[i].startX 			= 0;
		gPlayerInfo[i].startZ 			= 0;
		gPlayerInfo[i].coord.x 			= 0;
		gPlayerInfo[i].coord.y 			= 0;
		gPlayerInfo[i].coord.z 			= 0;
		gPlayerInfo[i].onThisMachine 	= false;

		gPlayerInfo[i].wheelObj[0] 		=
		gPlayerInfo[i].wheelObj[1] 		=
		gPlayerInfo[i].wheelObj[2] 		=
		gPlayerInfo[i].wheelObj[3] 		=
		gPlayerInfo[i].headObj = nil;



		gPlayerInfo[i].splitPaneNum		= -1;


		gPlayerInfo[i].team = i % 2;			// set team for capture the flag mode

				/* CAR SPECS INFO */

		gPlayerInfo[i].steering					= 0;




			/* AI */

		gPlayerInfo[i].isComputer 	    = true;


	}

			/* NETWORK GAME */

	if (gNetGameInProgress)
	{
		gPlayerInfo[gMyNetworkPlayerNum].onThisMachine = true;			// set the local player
		gPlayerInfo[gMyNetworkPlayerNum].splitPaneNum = 0;
	}

			/* LOCAL GAME */
	else
	{
		for (i = 0; i < gNumLocalPlayers; i++)
		{
			gPlayerInfo[i].onThisMachine = true;
			gPlayerInfo[i].splitPaneNum  = i;
		}
	}


	    /* REAL PLAYERS ARE NOT CONTROLLED BY CPU */

	for (i = 0; i < gNumRealPlayers; i++)
	{
		gPlayerInfo[i].isComputer 	    = false;        // these are real players, not computer players
    }


			/* SEE HOW MANY PLAYERS IN GAME */

	switch(gGameMode)
	{
		case	GAME_MODE_PRACTICE:
		case	GAME_MODE_TOURNAMENT:
//		case	GAME_MODE_MULTIPLAYERRACE:
				gNumTotalPlayers = MAX_PLAYERS;                 // use them all
				break;

		default:
				gNumTotalPlayers = gNumRealPlayers;				// no CPU players in battle modes
				break;


	}
}


/******************* INIT PLAYERS AT START OF LEVEL ********************/
//
// Initializes player stuff at the beginning of each track.
//

void InitPlayersAtStartOfLevel(void)
{
int		i,j,type;
Boolean	taken[MAX_CAR_TYPES];

	gWorstHumanPlace = 0;


		/* FIRST MARK WHICH CAR TYPES THE HUMANS HAVE */

	for (i = 0; i < MAX_CAR_TYPES; i++)								// first mark all unused
		taken[i] = false;

	for (i = 0; i < gNumTotalPlayers; i++)
	{
		if (!gPlayerInfo[i].isComputer)								// check for human player
		{
			taken[gPlayerInfo[i].vehicleType] = true;				// mark this used
		}
	}


	i = gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE;
	if (i > 2)														// dont get extra cars after winning, so pin @ 2
		i = 2;
	type = 6 + (i * 2)-1;											// start @ end of usable cars so it will pick best cars


			/* SET SOME GLOBALS */

	for (i = 0; i < gNumTotalPlayers; i++)
	{
		if (gPlayerInfo[i].isComputer)									// set CPU vehicle type
		{
			if (gGamePrefs.difficulty == DIFFICULTY_HARD)				// in hard mode, the CPU can have duplicate cars
				gPlayerInfo[i].vehicleType = RandomRange(0, type);
			else														// in other difficulty modes, only choose unique cars
			{
				while(taken[type])										// skip over vehicles already used by Humans
					type--;

				gPlayerInfo[i].vehicleType = type--;
			}
		}

		gPlayerInfo[i].coord.y = GetTerrainY(gPlayerInfo[i].startX,gPlayerInfo[i].startX);

			/* CREATE THE CAR MODEL */

		if (gTrackNum == TRACK_NUM_ATLANTIS)
			InitPlayer_Submarine(i, &gPlayerInfo[i].coord, gPlayerInfo[i].startRotY);
		else
			InitPlayer_Car(i, &gPlayerInfo[i].coord, gPlayerInfo[i].startRotY);

		gPlayerInfo[i].objNode->Damage 			= .5;
		gPlayerInfo[i].objNode->InvincibleTimer = 0;

		gPlayerInfo[i].controlBits		= 0;
		gPlayerInfo[i].controlBits_New	= 0;
		gPlayerInfo[i].analogSteering	= 0;

		gPlayerInfo[i].distToFloor				= 0;
		gPlayerInfo[i].skidDot					= 0;
		gPlayerInfo[i].mostRecentFloorY 		= 0;

		gPlayerInfo[i].onWater			 		= false;
		gPlayerInfo[i].waterY 					= 0;

		gPlayerInfo[i].lapNum					= -1;			// start @ -1 since we cross the finish line @ start
		gPlayerInfo[i].checkpointNum			= gNumCheckpoints-1;
		gPlayerInfo[i].place					= i;
		gPlayerInfo[i].distToNextCheckpoint		= 0;
		gPlayerInfo[i].raceComplete				= false;

		gPlayerInfo[i].snowParticleGroup		= -1;
		gPlayerInfo[i].snowTimer				= 0;
		gPlayerInfo[i].frozenTimer				= 0;


		for (j = 0; j < MAX_CHECKPOINTS; j++)				// start with all checkpoints tagged to trick lapNum @ start of race
			gPlayerInfo[i].checkpointTagged[j] 	= true;

		gPlayerInfo[i].currentThrust	= 0;
		gPlayerInfo[i].gasPedalDown		= false;
		gPlayerInfo[i].accelBackwards	= false;
		gPlayerInfo[i].movingBackwards	= false;
		gPlayerInfo[i].braking			= false;
		gPlayerInfo[i].isPlaning		= false;
		gPlayerInfo[i].greasedTiresTimer = 0;
		gPlayerInfo[i].wrongWay			= false;
		gPlayerInfo[i].steering			= 0;
		gPlayerInfo[i].currentRPM		= 0;
		gPlayerInfo[i].submarineImmobilized = 0;
		gPlayerInfo[i].bumpSoundTimer	= 0;

			/* DRAG DEBRIS */

		gPlayerInfo[i].tiresAreDragging		= false;
		gPlayerInfo[i].alwaysDoDrag			= false;
		gPlayerInfo[i].dragDebrisTimer		= 0;
		gPlayerInfo[i].dragDebrisParticleGroup = -1;
		gPlayerInfo[i].dragDebrisMagicNum 	= 0;
		gPlayerInfo[i].dragDebrisTexture	= 0;

		gPlayerInfo[i].lastSkidSegCoord.x 	=
		gPlayerInfo[i].lastSkidSegCoord.y 	= 0;
		gPlayerInfo[i].lastSkidVector.x 	=
		gPlayerInfo[i].lastSkidVector.y 	= 0;
		gPlayerInfo[i].skidSmokeParticleGroup = -1;
		gPlayerInfo[i].skidSmokeMagicNum 	= 0;
		gPlayerInfo[i].skidSmokeTimer		= 0;
		gPlayerInfo[i].makingSkid			= false;
		gPlayerInfo[i].skidChannel			= -1;
		gPlayerInfo[i].skidSoundTimer		= 0;

		gPlayerInfo[i].skidColor.r 			=
		gPlayerInfo[i].skidColor.g 			=
		gPlayerInfo[i].skidColor.b 			=
		gPlayerInfo[i].skidColor.a 			= 0;

				/* POWERUP */

		gPlayerInfo[i].nitroTimer			= 0;
		gPlayerInfo[i].stickyTiresTimer		= 0;
		gPlayerInfo[i].superSuspensionTimer	= 0;
		gPlayerInfo[i].numTokens			= 0;
		gPlayerInfo[i].invisibilityTimer	= 0;
		gPlayerInfo[i].flamingTimer			= 0;

				/* BATTLE MODES */

		gPlayerInfo[i].tagTimer				= TAG_TIME_LIMIT;
		gPlayerInfo[i].tagOccilation		= 0;
		gPlayerInfo[i].isIt					= false;
		gPlayerInfo[i].isEliminated			= false;
		gPlayerInfo[i].health				= 1.0;
		gPlayerInfo[i].impactResetTimer		= 0;

			/* GROUND TILE INFO */

		gPlayerInfo[i].groundTraction		= 1.0;
		gPlayerInfo[i].groundFriction		= 1.0;
		gPlayerInfo[i].groundSteering		= 1.0;
		gPlayerInfo[i].groundAcceleration	= 1.0;
		gPlayerInfo[i].noSkids				= false;


				/* WEAPON INFO */

		gPlayerInfo[i].powType 		= POW_TYPE_NONE;
		gPlayerInfo[i].powQuantity	= 0;

				/* CAMERA */

		gPlayerInfo[i].cameraRingRot = 0;
		gPlayerInfo[i].cameraUserRot = 0;

		gPlayerInfo[i].camera.cameraLocation.x = 0;
		gPlayerInfo[i].camera.cameraLocation.y = 0;
		gPlayerInfo[i].camera.cameraLocation.z = 0;
		gPlayerInfo[i].camera.pointOfInterest.x = 0;
		gPlayerInfo[i].camera.pointOfInterest.y = 0;
		gPlayerInfo[i].camera.pointOfInterest.z = 0;
		gPlayerInfo[i].camera.upVector.x = 0;
		gPlayerInfo[i].camera.upVector.y = 1;
		gPlayerInfo[i].camera.upVector.z = 0;

		gPlayerInfo[i].cameraMode = 0;


			/* AI */

		gPlayerInfo[i].oldPositionTimer	= 0;
		gPlayerInfo[i].oldPosition.x	= 0;
		gPlayerInfo[i].oldPosition.y	= 0;
		gPlayerInfo[i].oldPosition.z	= 0;
		gPlayerInfo[i].reverseTimer		= 0;
		gPlayerInfo[i].attackTimer		= 2;					// dont attack for the first few seconds
		gPlayerInfo[i].targetedPlayer	= -1;					// no players targeted yet
		gPlayerInfo[i].targetingTimer	= 0;
		gPlayerInfo[i].pathVec.x	= 0;
		gPlayerInfo[i].pathVec.y	= 0;



			/* SOUND */

		gPlayerInfo[i].engineChannel = -1;


			/* RESET CAR PHYSICS INFO */

		SetPhysicsForVehicleType(i);
	}



}

#pragma mark -

/********* SET PLAYER PARMS FROM TILE ATTRIBUTES *****************/
//
// INPUT: 	flags = tile attribute flags
//			gTileAttribParm[3] = byte values
//

void SetPlayerParmsFromTileAttributes(short playerNum, uint16_t flags)
{
			/* IF ON WATER */

	if (gPlayerInfo[playerNum].onWater)
	{
		gPlayerInfo[playerNum].groundTraction = .3;
		gPlayerInfo[playerNum].groundFriction = .3;
		gPlayerInfo[playerNum].groundSteering = .3;
		gPlayerInfo[playerNum].groundAcceleration = .5;
		gPlayerInfo[playerNum].noSkids			= true;
		gPlayerInfo[playerNum].dragDebrisTexture = PARTICLE_SObjType_Splash;
		gPlayerInfo[playerNum].alwaysDoDrag 	= true;
		return;
	}

			/* SEE IF ON ICE */

	if (flags & TILE_ATTRIB_ICE)
	{
		gPlayerInfo[playerNum].groundTraction = .05f;
		gPlayerInfo[playerNum].groundFriction = .05f;
		gPlayerInfo[playerNum].groundSteering = .2f;
		gPlayerInfo[playerNum].groundAcceleration = .3;
		gPlayerInfo[playerNum].noSkids			= true;
		gPlayerInfo[playerNum].dragDebrisTexture = -1;
		gPlayerInfo[playerNum].alwaysDoDrag 	= false;
	}

			/* SEE IF ON SNOW */

	else
	if (flags & TILE_ATTRIB_SNOW)
	{
		gPlayerInfo[playerNum].groundTraction = .3f;
		gPlayerInfo[playerNum].groundFriction = .5f;
		gPlayerInfo[playerNum].groundSteering = .6f;
		gPlayerInfo[playerNum].groundAcceleration = .7;
		gPlayerInfo[playerNum].noSkids			= false;
		gPlayerInfo[playerNum].skidColor.r		= 1.0f;
		gPlayerInfo[playerNum].skidColor.g		= 1.0f;
		gPlayerInfo[playerNum].skidColor.b		= 1.0f;
		gPlayerInfo[playerNum].dragDebrisTexture = PARTICLE_SObjType_SnowDust;
		gPlayerInfo[playerNum].alwaysDoDrag 	= true;
	}

		/* SET DEFAULTS */

	else
	{
		gPlayerInfo[playerNum].groundTraction = 1.0;
		gPlayerInfo[playerNum].groundFriction = 1.0;
		gPlayerInfo[playerNum].groundSteering = 1.0;
		gPlayerInfo[playerNum].groundAcceleration = 1.0;
		gPlayerInfo[playerNum].noSkids			= false;
		gPlayerInfo[playerNum].skidColor.r		= .0f;
		gPlayerInfo[playerNum].skidColor.g		= .0f;
		gPlayerInfo[playerNum].skidColor.b		= .0f;
		gPlayerInfo[playerNum].dragDebrisTexture = PARTICLE_SObjType_Dirt;
		gPlayerInfo[playerNum].alwaysDoDrag 	= false;
	}


}




#pragma mark -

/******************** FIND CLOSEST PLAYER ****************************/
//
// Returns -1 if no other players in range
//
// Ignore thePlayer if != nil
//

short FindClosestPlayer(ObjNode *thePlayer, float x, float z, float range, Boolean allowCPUCars, float *dist)
{
short	p,bestP = -1;
ObjNode	*target;
float	bestDist = 10000000000;
float	d;

	for (p = 0; p < gNumTotalPlayers; p++)
	{
		target = gPlayerInfo[p].objNode;

		if (target == thePlayer)
			continue;

		if (!allowCPUCars)
		{
			if (gPlayerInfo[p].isComputer)
				continue;
		}

		d = CalcDistance(x,z, target->Coord.x, target->Coord.z);
		if (d > range)
			continue;

		if (d < bestDist)
		{
			bestDist = d;
			bestP = p;
		}
	}

	*dist = bestDist;
	return(bestP);
}



/******************** FIND CLOSEST PLAYER IN FRONT ****************************/
//
// Returns -1 if no other players in range
//
// INPUT:  angle, 0 = full 180 degrees in front, 1.0 = none
//

short FindClosestPlayerInFront(ObjNode *theNode, float range, Boolean allowCPUCars, float *dist, float angle)
{
short	p,bestP = -1;
ObjNode	*target;
float	bestDist = 10000000000;
float	x,z,d, r, dot;
OGLVector2D	aimVec, toVec;

	x = theNode->Coord.x;
	z = theNode->Coord.z;

	r = theNode->Rot.y;							// calc aim vector
	aimVec.x = -sin(r);
	aimVec.y = -cos(r);


	for (p = 0; p < gNumTotalPlayers; p++)
	{
		target = gPlayerInfo[p].objNode;

		if (target == theNode)
			continue;

		if (!allowCPUCars)
		{
			if (gPlayerInfo[p].isComputer)
				continue;
		}

		d = CalcDistance(x,z, target->Coord.x, target->Coord.z);		// calc dist & check range
		if (d > range)
			continue;

		toVec.x = target->Coord.x - x;
		toVec.y = target->Coord.z - z;
		FastNormalizeVector2D(toVec.x, toVec.y, &toVec, true);			// calc normal to target

		dot = OGLVector2D_Dot(&aimVec, &toVec);							// dot = angle
		if (dot < angle)													// if in back, then skip
			continue;

		if (d < bestDist)
		{
			bestDist = d;
			bestP = p;
		}
	}

	*dist = bestDist;
	return(bestP);
}


/******************** FIND CLOSEST PLAYER IN BACK ****************************/
//
// Returns -1 if no other players in range
//
// INPUT:  angle, 0 = full 180 degrees, -1.0 = none
//

short FindClosestPlayerInBack(ObjNode *theNode, float range, Boolean allowCPUCars, float *dist, float angle)
{
short	p,bestP = -1;
ObjNode	*target;
float	bestDist = 10000000000;
float	x,z,d, r, dot;
OGLVector2D	aimVec, toVec;

	x = theNode->Coord.x;
	z = theNode->Coord.z;

	r = theNode->Rot.y;							// calc aim vector
	aimVec.x = -sin(r);
	aimVec.y = -cos(r);


	for (p = 0; p < gNumTotalPlayers; p++)
	{
		target = gPlayerInfo[p].objNode;

		if (target == theNode)
			continue;

		if (!allowCPUCars)
		{
			if (gPlayerInfo[p].isComputer)
				continue;
		}

		d = CalcDistance(x,z, target->Coord.x, target->Coord.z);		// calc dist & check range
		if (d > range)
			continue;

		toVec.x = target->Coord.x - x;
		toVec.y = target->Coord.z - z;
		FastNormalizeVector2D(toVec.x, toVec.y, &toVec, true);			// calc normal to target

		dot = OGLVector2D_Dot(&aimVec, &toVec);							// dot = angle
		if (dot > angle)												// if in front, then skip
			continue;

		if (d < bestDist)
		{
			bestDist = d;
			bestP = p;
		}
	}

	*dist = bestDist;
	return(bestP);
}


#pragma mark -


/******************* CHOOSE TAGGED PLAYER *********************/

void ChooseTaggedPlayer(void)
{
short	i,j;

	i = j = RandomRange(0, gNumTotalPlayers-1);

	while(gPlayerInfo[i].isEliminated)
	{
		i++;
		if (i >= gNumTotalPlayers)			// wrap around
			i = 0;
		if (i == j)							// error check
			DoFatalAlert("ChooseTaggedPlayer: all players have been eliminated");
	}

	gPlayerInfo[i].isIt = true;
	gWhoIsIt = i;

}

/*********************** UPDATE TAG MARKER **********************/
//
// Occilatets the filter color on the tagged player
//

void UpdateTagMarker(void)
{
short	p;
float	o,r,g,b;
ObjNode	*obj;

	for (p = 0; p < gNumTotalPlayers; p++)					// scan all players to update their colors
	{
		if (p == gWhoIsIt)
		{
			o = gPlayerInfo[p].tagOccilation += gFramesPerSecondFrac * 2.0f;

			gTagColor.r = r = fabs(sin(o));
			gTagColor.g = g = fabs(cos(o));
			gTagColor.b = b = fabs(tan(o));

			obj = gPlayerInfo[p].objNode;
			while(obj)
			{
				obj->ColorFilter.r = r;
				obj->ColorFilter.g = g;
				obj->ColorFilter.b = b;
				obj = obj->ChainNode;
			}
		}
		else
		{
			obj = gPlayerInfo[p].objNode;
			while(obj)
			{
				obj->ColorFilter.r =
				obj->ColorFilter.g =
				obj->ColorFilter.b = 1.0;
				obj = obj->ChainNode;
			}
		}
	}
}



/***************** PLAYER LOSE HEALTH ************************/

void PlayerLoseHealth(short p, float damage)
{

	if (gTrackCompleted)
		return;

	if (gPlayerInfo[p].isEliminated)
		return;

	gPlayerInfo[p].health -= damage;

			/* SEE IF DEAD */

	if (gPlayerInfo[p].health <= 0.0f)
	{
		gPlayerInfo[p].health = 0;
		gPlayerInfo[p].isEliminated = true;
		gNumPlayersEliminated++;

		if (gNumPlayersEliminated < (gNumTotalPlayers-1))		// if more than 1 player remaining, then post ELIMINATED message
		{
			ShowWinLose(p, 0, 0);								// this player is eliminated
		}
	}
}

#pragma mark -

/******************** SET STICKY TIRES *************************/

void SetStickyTires(short playerNum)
{
	SetTractionPhysics(&gPlayerInfo[playerNum].carStats, 3.0);
	gPlayerInfo[playerNum].stickyTiresTimer = 20.0;				// set duration of sticky tires


}

/******************** SET SUSPENSION POW *************************/

void SetSuspensionPOW(short playerNum)
{
	SetSuspensionPhysics(&gPlayerInfo[playerNum].carStats, 3.0);
	gPlayerInfo[playerNum].superSuspensionTimer = 20.0;				// set duration


}


/****************** SET CAR STATUS BITS *******************/

void SetCarStatusBits(short	playerNum, uint32_t bits)
{
ObjNode *obj;

	obj = gPlayerInfo[playerNum].objNode;
	while(obj)
	{
		obj->StatusBits |= bits;
		obj = obj->ChainNode;
	}
}

/****************** CLEAR CAR STATUS BITS *******************/

void ClearCarStatusBits(short	playerNum, uint32_t bits)
{
ObjNode *obj;

	obj = gPlayerInfo[playerNum].objNode;
	while(obj)
	{
		obj->StatusBits &= ~bits;
		obj = obj->ChainNode;
	}
}










