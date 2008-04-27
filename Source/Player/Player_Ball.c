/*******************************/
/*   	ME BALL.C			   */
/* (c)1997-98 Pangea Software  */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

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
#include "myguy.h"
#include "effects.h"
#include "mobjtypes.h"
#include "skeletonJoints.h"
#include "camera.h"
#include "bones.h"
#include "skeletonObj.h"

extern	ObjNode					*gCurrentNode,*gPlayerObj;
extern	float					gFramesPerSecondFrac,gFramesPerSecond,gPlayerToCameraAngle;
extern	OGLPoint3D				gCoord,gMyCoord;
extern	OGLVector3D				gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	u_short					gMyLatestPathTileNum,gMyLatestTileAttribs;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	float					gPlayerMaxSpeed,gMyDistToFloor,gBallTimer;
extern	unsigned long 			gInfobarUpdateBits;
extern	Boolean					gPlayerCanMove,gPlayerUsingKeyControl;
extern	PrefsType	gGamePrefs;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void UpdatePlayer_Ball(ObjNode *theNode);
static void MovePlayer_Ball(ObjNode *theNode);
static void DoPlayerControl_Ball(ObjNode *theNode, float slugFactor);
static void SpinBall(ObjNode *theNode);
static void LeaveNitroTrail(void);
static void StartNitroTrail(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	PLAYER_BALL_SCALE			1.0



#define	PLAYER_BALL_ACCEL			66.0f
#define	PLAYER_BALL_FRICTION_ACCEL	400.0f
#define	PLAYER_MAX_SPEED_BALL		2800.0f;

#define	PLAYER_BALL_JUMPFORCE		1400.0f

#define	NITRO_RING_SIZE				16			// #particles in a nitro ring

/*********************/
/*    VARIABLES      */
/*********************/

static float	gNitroTimer,gNitroTrailTick;
int				gNitroParticleGroup;

OGLMatrix4x4	gBallRotationMatrix;

Boolean			gPlayerKnockOnButt = false;
OGLVector3D		gPlayerKnockOnButtDelta;



/*************************** INIT PLAYER: BALL ****************************/
//
// Creates an ObjNode for the player in the Ball form.
//
//
// INPUT:	oldObj = old objNode to base some parameters on.  nil = no old player obj
//			where = floor coord where to init the player.
//

void InitPlayer_Ball(ObjNode *oldObj, OGLPoint3D *where)
{


}




/******************** MOVE ME: BALL ***********************/

static void MovePlayer_Ball(ObjNode *theNode)
{
	gPlayerCanMove = true;

	GetObjectInfo(theNode);


			/* UPDATE INVINCIBILITY */

	if (theNode->InvincibleTimer > 0.0f)
	{
		theNode->InvincibleTimer -= gFramesPerSecondFrac;
		if (theNode->InvincibleTimer < 0.0f)
			theNode->InvincibleTimer = 0;
	}


			/* DO CONTROL */

	DoPlayerControl_Ball(theNode,1.0);


			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BALL_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;


		/* SEE IF LEAVE NITRO TRAIL */

	if (gNitroTimer > 0.0f)
	{
		LeaveNitroTrail();
		gNitroTimer -= gFramesPerSecondFrac;
		if (gNitroTimer <= 0.0f)
			gNitroParticleGroup = -1;
	}

			/* UPDATE IT */

update:
	UpdatePlayer_Ball(gPlayerObj);
}


/************************ UPDATE PLAYER: BALL ***************************/

static void UpdatePlayer_Ball(ObjNode *theNode)
{
	SpinBall(theNode);

	gMyCoord = gCoord;
	UpdateObject(theNode);


		/* FINAL CHECK: SEE IF NEED TO KNOCK ON BUTT */

	if (gPlayerKnockOnButt)
	{
		gPlayerKnockOnButt = false;
		KnockPlayerBugOnButt(&gPlayerKnockOnButtDelta, true, true);
	}

		/* UPDATE SHIELD */

	UpdatePlayerShield();
}


/**************** DO PLAYER CONTROL: BALL ***************/
//
// Moves a player based on its control bit settings.
// These settings are already set either by keyboard interpretation or reading off the network.
//
// INPUT:	theNode = the node of the player
//			slugFactor = how much of acceleration to apply (varies if jumping et.al)
//


static void DoPlayerControl_Ball(ObjNode *theNode, float slugFactor)
{
float	mouseDX, mouseDY;
float	fps = gFramesPerSecondFrac;
float	dx, dy;

			/********************/
			/* GET MOUSE DELTAS */
			/********************/
			//
			// NOTE: can only call this once per frame since
			//		this resets the mouse coord.
			//

	GetMouseDelta(&dx, &dy);


	if (gPlayerUsingKeyControl && gGamePrefs.playerRelativeKeys)
	{
		gPlayerObj->AccelVector.y =
		gPlayerObj->AccelVector.x = 0;
	}
	else
	{
		mouseDX = dx * .05f;
		mouseDY = dy * .05f;
		theNode->AccelVector.y = mouseDY * PLAYER_BALL_ACCEL * slugFactor;
		theNode->AccelVector.x = mouseDX * PLAYER_BALL_ACCEL * slugFactor;
	}

		/* SEE IF DO NITRO */

	if (gNitroTimer <= 0.0f)
	{
		if (GetNewKeyState(kKey_KickBoost))
		{
			theNode->Speed = gPlayerMaxSpeed;		// boost to full speed
			gDelta.z *= 100.0f;						// boost this up really high (will get tweaked later)
			gDelta.x *= 100.0f;
			StartNitroTrail();
			gBallTimer -= .05f;						// lose a bit more ball time
		}
	}
}


/******************** SPIN BALL ************************/

static void SpinBall(ObjNode *theNode)
{
float	d,fps = gFramesPerSecondFrac;

			/* REGULATE SPIN IF ON GROUND */

//	if (gMyDistToFloor < 4.0f)
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		d = theNode->RotDeltaX = -theNode->Speed * .01f;
	}

			/* OTHERWISE DECAY SPIN IN AIR */
	else
	{
		if (theNode->RotDeltaX < 0.0f)
		{
			theNode->RotDeltaX += fps*12.0f;
			if (theNode->RotDeltaX > 0.0f)
				theNode->RotDeltaX = 0;
		}

		d = theNode->RotDeltaX;
	}

	theNode->Rot.x += d * fps;					// rotate on x

			/* AIM IN DIRECTION OF MOTION */

	if ((!gPlayerUsingKeyControl) || (!gGamePrefs.playerRelativeKeys))
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x + gDelta.x, gCoord.z + gDelta.z, 8.0, false);

}


#pragma mark -

/****************** START NITRO TRAIL *********************/

static void StartNitroTrail(void)
{
	gNitroTimer = .6;
	gNitroTrailTick = 0;
}



/***************** LEAVE NITRO TRAIL *********************/

static void LeaveNitroTrail(void)
{
int				i;
OGLMatrix4x4	m;
static const OGLVector3D up = {0,1,0};

	gNitroTrailTick += gFramesPerSecondFrac;
	if (gNitroTrailTick < .05f)
		return;

	gNitroTrailTick = 0;

			/* CALC MATRIX TO AIM & PLACE A NITRO RING */

	SetLookAtMatrixAndTranslate(&m, &up, &gPlayerObj->OldCoord, &gCoord);


				/* MAKE PARTICLE GROUP */

	if (gNitroParticleGroup == -1)
	{
new_pgroup:
		gNitroParticleGroup = NewParticleGroup(0,							// magic num
												PARTICLE_TYPE_FALLINGSPARKS,	// type
												0,		// flags
												600,							// gravity
												10000,						// magnetism
												25,							// base scale
												-1.9,						// decay rate
												1.0,							// fade rate
												PARTICLE_TEXTURE_GREENRING);	// texture
	}

	if (gNitroParticleGroup == -1)
		return;


			/* ADD PARTICLES TO GROUP */

	for (i = 0; i < NITRO_RING_SIZE; i++)
	{
		OGLVector3D	delta;
		OGLPoint3D	pt;
		float		a;


		a = PI2 * ((float)i * (1.0f/(float)NITRO_RING_SIZE));
		pt.x = sin(a) * 50.0f;
		pt.y = cos(a) * 50.0f;
		pt.z = 0;

		OGLPoint3D_Transform(&pt, &m, &pt);

		delta.x = (RandomFloat()-.5f) * 200.0f;
		delta.y = 200.0f + (RandomFloat()-.5f) * 200.0f;
		delta.z = (RandomFloat()-.5f) * 200.0f;

		if (AddParticleToGroup(gNitroParticleGroup, &pt, &delta, 1.0 + RandomFloat(), FULL_ALPHA))
			goto new_pgroup;
	}
}








