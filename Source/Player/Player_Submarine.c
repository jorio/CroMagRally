/*******************************/
/*   	PLAYER_SUBMARINE.C	   */
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

static void MovePlayer_Submarine(ObjNode *theNode);
static void MovePlayer_Sub_Multipass(ObjNode *theNode);
static void DoSubmarineMotion(ObjNode *theNode);
static void UpdatePlayer_Submarine(ObjNode *theNode);
static void RotateSubmarine(ObjNode *theNode);
static Boolean DoSubCollisionDetect(ObjNode *vehicle);
static void SubHitSub(ObjNode *car1, ObjNode *car2);
static void DoCPUControl_Sub(ObjNode *theNode);
static void CreateSubmarinePropeller(ObjNode *theCar, short playerNum);
static void DoPlayerControl_Submarine(ObjNode *theNode);
static void AlignPropellerOnSubmarine(ObjNode *theCar);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	SUB_TURNSPEED			(PI*.8)

#define	DELTA_SUBDIV			40.0f

#define	SUB_MAX_Y				6000.0f

#define	SUBMARINE_FRICTION		3000.0f


/*********************/
/*    VARIABLES      */
/*********************/

#define	BubbleTimer		SpecialF[0]


/*************************** INIT PLAYER: SUBMARINE ****************************/
//
// Creates an ObjNode for the player in the Submarine form.
//
// INPUT:
//			where = floor coord where to init the player.
//			rotY = rotation to assign to player if oldObj is nil.
//

ObjNode *InitPlayer_Submarine(int playerNum, OGLPoint3D *where, float rotY)
{
ObjNode			*newObj;


		/* CREATE SUBMARINE BODY AS MAIN OBJECT FOR PLAYER */

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_CARPARTS,
		.type		= CARPARTS_ObjType_Submarine,
		.coord.x	= where->x,
		.coord.z	= where->z,
		.coord.y	= GetTerrainY(where->x,where->z) + 500,
		.flags		= STATUS_BIT_ROTXZY,
		.slot		= PLAYER_SLOT,
		.moveCall	= MovePlayer_Submarine,
		.scale		= 1.0,
	};

	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);


	newObj->PlayerNum = playerNum;
	newObj->Rot.y = rotY;
	newObj->CType = CTYPE_MISC|CTYPE_PLAYER;


				/* SET COLLISION INFO */

	newObj->CType = CTYPE_PLAYER|CTYPE_MISC|CTYPE_AVOID;
	newObj->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);


			/* CREATE PROPELLER */

	CreateSubmarinePropeller(newObj, playerNum);


				/* SET GLOBALS */

	gPlayerInfo[playerNum].objNode 	= newObj;



	AttachShadowToObject(newObj, SHADOW_TYPE_CAR_SUBMARINE, 13, 13, false);


	return(newObj);
}



#pragma mark -


/******************** MOVE PLAYER: SUBMARINE ***********************/

static void MovePlayer_Submarine(ObjNode *theNode)
{
int					numPasses;
float				oldFPS,oldFPSFrac;



	/* SEE IF THE SUB IS BEING DRIVEN BY A PLAYER */

	gCurrentPlayerNum = theNode->PlayerNum;								// get player #
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


//	if (GetNewKeyState_Real(KEY_F11))	//---------- hack to make player a CPU
//	{
//		gAutoPilot = true;
//	}


			/* DO IT IN PASSES */

	for (gPlayerMultiPassCount = 0; gPlayerMultiPassCount < numPasses; gPlayerMultiPassCount++)
	{
		GetObjectInfo(theNode);

		if (gPlayerMultiPassCount > 0)
			KeepOldCollisionBoxes(theNode);								// keep old boxes & other stuff secondary passes

		MovePlayer_Sub_Multipass(theNode);

		UpdatePlayer_Submarine(theNode);
	}


			/* UPDATE */

	gFramesPerSecond = oldFPS;											// restore real FPS values
	gFramesPerSecondFrac = oldFPSFrac;
}


/******************** MOVE PLAYER: SUBMARINE : MULTIPASS ***********************/

static void MovePlayer_Sub_Multipass(ObjNode *theNode)
{
            /***********/
			/* CONTROL */
            /***********/

            /* CPU DRIVEN */

    if (gPlayerInfo[gCurrentPlayerNum].isComputer || gAutoPilot)
    {
    	DoCPUControl_Sub(theNode);
    }
            /* PLAYER DRIVEN */
    else
		DoPlayerControl_Submarine(theNode);


	        /***********/
			/* MOVE IT */
	        /***********/

	DoSubmarineMotion(theNode);


		/* DO COLLISION CHECK */

	DoSubCollisionDetect(theNode);
	DoFenceCollision(theNode);
	UpdatePlayerCheckpoints(gCurrentPlayerNum);
}



/******************** DO SUBMARINE MOTION ********************/

static void DoSubmarineMotion(ObjNode *theNode)
{
float		y;
float		fps = gFramesPerSecondFrac;
short		playerNum;
OGLMatrix4x4	mx,my,mxy;
static const OGLVector3D	v = {0,0,-1};
OGLVector3D		aimVec;

	playerNum 	= theNode->PlayerNum;


			/*****************/
			/* DO CAR MOTION */
			/*****************/

		/* GET AIM VECTOR */

	OGLMatrix4x4_SetRotate_X(&mx, theNode->Rot.x);
	OGLMatrix4x4_SetRotate_Y(&my, theNode->Rot.y);
	OGLMatrix4x4_Multiply(&mx,&my, &mxy);
	OGLVector3D_Transform(&v, &mxy, &aimVec);
	FastNormalizeVector(aimVec.x, aimVec.y, aimVec.z, &aimVec);


			/* APPLY THRUST TO SUBMARINE */

	if (gNoCarControls || gTrackCompleted || (gPlayerInfo[playerNum].submarineImmobilized > 0.0f))
	{
		ApplyFrictionToDeltas(SUBMARINE_FRICTION * fps, &gDelta);
		gPlayerInfo[playerNum].currentRPM = CalcVectorLength(&gDelta);
	}
	else
	{
		float	maxSpeed = gPlayerInfo[playerNum].carStats.maxSpeed + ((float)gPlayerInfo[playerNum].place * 100.0f);

		if (gPlayerInfo[playerNum].nitroTimer > 0.0f)			// see if give nitro boost
			maxSpeed *= 1.4f;

		gPlayerInfo[playerNum].currentRPM += fps * gPlayerInfo[playerNum].carStats.acceleration;		// store sub speed in RPM variable
		if (gPlayerInfo[playerNum].currentRPM > maxSpeed)
			gPlayerInfo[playerNum].currentRPM = maxSpeed;

		gDelta.x = aimVec.x * gPlayerInfo[playerNum].currentRPM;
		gDelta.y = aimVec.y * gPlayerInfo[playerNum].currentRPM;
		gDelta.z = aimVec.z * gPlayerInfo[playerNum].currentRPM;
	}


			/****************/
			/* MOVE THE SUB */
			/****************/

	gCoord.x += gDelta.x * fps;									// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	y = GetTerrainY(gCoord.x, gCoord.z) + 200.0f;

	if (gCoord.y < y)
	{
		gCoord.y = y;
		gDelta.y = 0;
	}
	else
	if (gCoord.y > (y + SUB_MAX_Y))
	{
		gCoord.y = y + SUB_MAX_Y;
		gDelta.y = 0;
	}



}


/************************ UPDATE PLAYER: SUBMARINE ***************************/

static void UpdatePlayer_Submarine(ObjNode *theNode)
{
short	p = theNode->PlayerNum;

	UpdateObject(theNode);

	gPlayerInfo[p].coord = gCoord;				// update player coord


			/* UPDATE FINAL SPEED VALUES */

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);
	theNode->Speed3D = CalcVectorLength(&gDelta);


	if (gPlayerInfo[p].submarineImmobilized  > 0.0f)
	{
		gPlayerInfo[p].submarineImmobilized -= gFramesPerSecondFrac;
		if (gPlayerInfo[p].submarineImmobilized < 0.0f)
			gPlayerInfo[p].submarineImmobilized  = 0.0;

	}

			/* UPDATE ALL OF THE ATTACHMENTS */

	AlignPropellerOnSubmarine(theNode);


			/* UPDATE POWERUP TIMERS */

	UpdateNitro(p);


#if 0
			/************************/
			/* UPDATE ENGINE EFFECT */
			/************************/

	if (gPlayerInfo[p].engineChannel != -1)
		Update3DSoundChannel(EFFECT_HUM, &gPlayerInfo[p].engineChannel, &gCoord);
	else
		gPlayerInfo[p].engineChannel = PlayEffect_Parms3D(EFFECT_HUM, &gCoord, NORMAL_CHANNEL_RATE, .5);


			/* UPDATE FREQ */

	if (gPlayerInfo[p].engineChannel != -1)
	{
		int	freq;

		freq = NORMAL_CHANNEL_RATE - 0x2000 + (theNode->Speed3D);
		ChangeChannelRate(gPlayerInfo[p].engineChannel, freq);
	}
#endif
}



#pragma mark -



/******************** DO SUB COLLISION DETECT **************************/
//
// Standard collision handler for player
//
// OUTPUT: true = disabled/killed
//

static Boolean DoSubCollisionDetect(ObjNode *vehicle)
{
short		i,playerNum;
ObjNode		*hitObj;
unsigned long	ctype;
uint8_t		sides;


			/* DETERMINE CTYPE BITS TO CHECK FOR */

	ctype = PLAYER_COLLISION_CTYPE;
	playerNum = gCurrentPlayer->PlayerNum;


	gPreCollisionDelta = gDelta;									// remember delta before collision handler messes with it


			/***************************************/
			/* AUTOMATICALLY HANDLE THE GOOD STUFF */
			/***************************************/
			//
			// this also sets the ONGROUND status bit if on a solid object.
			//

	sides = HandleCollisions(vehicle, ctype, 0);
	(void) sides;


			/******************************/
			/* SCAN FOR INTERESTING STUFF */
			/******************************/

	gPlayerInfo[playerNum].onWater = false;							// assume not on water

	for (i=0; i < gNumCollisions; i++)
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision

			if (hitObj->CType == INVALID_NODE_FLAG)				// see if has since become invalid
				continue;

			ctype = hitObj->CType;								// get collision ctype from hit obj


			/**************************/
			/* CHECK FOR IMPENETRABLE */
			/**************************/

			if ((ctype & CTYPE_IMPENETRABLE) && (!(ctype & CTYPE_IMPENETRABLE2)))
			{
				if (!(gCollisionList[i].sides & SIDE_BITS_BOTTOM))	// dont do this if we landed on top of it
				{
					gCoord.x = gCurrentPlayer->OldCoord.x;					// dont take any chances, just move back to original safe place
					gCoord.z = gCurrentPlayer->OldCoord.z;
				}
			}

				/********************/
				/* CHECK FOR PLAYER */
				/********************/

			if (ctype & CTYPE_PLAYER)
			{
				SubHitSub(vehicle, hitObj);
			}


				/******************/
				/* SEE IF VISCOUS */
				/******************/

			if (ctype & CTYPE_VISCOUS)
			{
				ApplyFrictionToDeltas(120, &gDelta);
			}
		}

	}

	gCurrentPlayer = gPlayerInfo[playerNum].objNode;

	return(false);
}



/*********************** SUB HIT SUB *****************************/
//
// Called from above when two vehicles collide.
//
// INPUT:  car1 == current ObjNode doing collision test.
//			car2 == other car that got hit
//

static void SubHitSub(ObjNode *car1, ObjNode *car2)
{
float		relSpeed;
OGLVector3D	v1,v2;
OGLVector3D	b1,b2;
OGLVector3D	relD1,relD2;


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
}




#pragma mark -


/**************** DO CPU CONTROL: SUB ***************/
//
// INPUT:	theNode = the node of the player
//

static void DoCPUControl_Sub(ObjNode *theNode)
{
short           player;
float			fps = gFramesPerSecondFrac;
float			dot;
OGLVector2D		pathVec;
short			avoidTurn;

    player = theNode->PlayerNum;
    gPlayerInfo[player].controlBits = 0;                                     // assume nothing
	gPlayerInfo[player].controlBits_New = 0;

	gPlayerInfo[player].attackTimer -= fps;									// dec this timer


		/*********************************/
		/* SEE IF NOT MOVING OR IS STUCK */
		/*********************************/

	if (gStartingLightTimer <= 1.0f)										// dont check if waiting for green light
	{
		gPlayerInfo[player].oldPositionTimer -= fps;
		if (gPlayerInfo[player].oldPositionTimer <= 0.0f)					// see if time to do the check
		{
			gPlayerInfo[player].oldPositionTimer += POSITION_TIMER;			// reset timer

			if (CalcDistance3D(gPlayerInfo[player].oldPosition.x, gPlayerInfo[player].oldPosition.y, gPlayerInfo[player].oldPosition.z,
								gCoord.x, gCoord.y, gCoord.z) < 100.0f)		// see if player isnt moving
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




			/*************************************/
			/* DO STEERING VIA PATH OR AVOIDANCE */
			/*************************************/

		/* SEE IF THERE ARE ANY OBJECTS AHEAD THAT WE SHOULD AVOID */

	avoidTurn = DoCPU_AvoidObjects(theNode);
	if (avoidTurn != 0)
	{
		if (avoidTurn == -1)
			gPlayerInfo[player].analogSteering = -1.0f;
		else
			gPlayerInfo[player].analogSteering = 1.0f;
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
    		r = acos(dot);                     				// convert dot to angle

    		if (r > (PI/14))								// see if outside of tolerance
    		{
			    if (cross > 0.0f)
                    gPlayerInfo[player].analogSteering = -1.0f;
		        else
		            gPlayerInfo[player].analogSteering = 1.0f;
            }
		}
		else
		{
			pathVec.x = 1;													// no path found, so set default values
			pathVec.y = 0;
		}
		gPlayerInfo[player].pathVec = pathVec;								// keep a copy


	}

			/* SEE IF NEED TO GO UP */

	if (gPlayerInfo[player].reverseTimer > 0.0f)						// see if going in reverse
	{
	    gPlayerInfo[player].controlBits |= (1L << kControlBit_Backward);
		if ((gPlayerInfo[player].reverseTimer -= fps) < 0.0f)			// dec reverse timer
			gPlayerInfo[player].reverseTimer = 0;
	}
//	else
//	if ((gCoord.y - GetTerrainY(gCoord.x, gCoord.z)) > 4000.0f)			// keep from being too high
//	    gPlayerInfo[player].controlBits |= (1L << kControlBit_Forward);	// go forward


				/* SEE IF CPU PLAYER SHOULD ATTACK */

	DoCPUPowerupLogic(theNode, player);



            /* HANDLE LIKE A REAL PLAYER */

    DoPlayerControl_Submarine(theNode);
}




#pragma mark -


/********************** CREATE PROPELLER ****************************/

static void CreateSubmarinePropeller(ObjNode *theCar, short playerNum)
{
ObjNode			*prop;


			/* ATTACH PROPELLER */

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_CARPARTS,
		.type		= CARPARTS_ObjType_Propeller,
		.coord		= theCar->Coord,
		.flags		= STATUS_BIT_KEEPBACKFACES,
		.slot		= PLAYER_SLOT,
		.scale		= 1,
	};
	prop = MakeNewDisplayGroupObject(&def);


	prop->PlayerNum = playerNum;						// set playernum in this obj

	theCar->ChainNode = prop;							// add to chain link

	gPlayerInfo[playerNum].headObj = prop;

	AlignPropellerOnSubmarine(theCar);
}




/**************** DO PLAYER CONTROL: SUBMARINE ***************/
//
// Moves a player based on its control bit settings.  NOTE:  Also called by CPU player!
//
// INPUT:	theNode = the node of the player
//

static void DoPlayerControl_Submarine(ObjNode *theNode)
{
uint16_t			playerNum = theNode->PlayerNum;
float			fps = gFramesPerSecondFrac;


	if (gNoCarControls || gTrackCompleted || (gPlayerInfo[playerNum].submarineImmobilized > 0.0f))
		return;

			/*****************/
			/* CHECK UP/DOWN */
			/*****************/

	if (GetControlState(playerNum, kControlBit_Backward))		// up
	{
		theNode->Rot.x += SUB_TURNSPEED *fps;
		if (theNode->Rot.x > (PI/3))
			theNode->Rot.x = PI/3;

	}
	else
	if (GetControlState(playerNum, kControlBit_Forward))		// down
	{
		theNode->Rot.x -= SUB_TURNSPEED *fps;
		if (theNode->Rot.x < (-PI/3))
			theNode->Rot.x = -PI/3;
	}

		/* AUTO-CENTER */
	else
	{
		if (theNode->Rot.x < 0.0f)
		{
			theNode->Rot.x += SUB_TURNSPEED * .5f * fps;
			if (theNode->Rot.x > 0.0f)
				theNode->Rot.x = 0.0f;
		}
		else
		if (theNode->Rot.x > 0.0f)
		{
			theNode->Rot.x -= SUB_TURNSPEED * .5f * fps;
			if (theNode->Rot.x < 0.0f)
				theNode->Rot.x = 0.0f;
		}

	}


			/**************************/
			/* SEE IF TURN LEFT/RIGHT */
			/**************************/

	if (gPlayerInfo[playerNum].analogSteering < -.3f)
	{
		theNode->Rot.y += SUB_TURNSPEED * fps;
	}
	else
	if (gPlayerInfo[playerNum].analogSteering > .3f)
	{
		theNode->Rot.y -= SUB_TURNSPEED * fps;
	}


		/******************/
		/* HANDLE WEAPONS */
		/******************/

	CheckPOWControls(theNode);

}





#pragma mark -


/************************ ALIGN PROPELLER ON SUBMARINE *****************************/

static void AlignPropellerOnSubmarine(ObjNode *theCar)
{
float			fps = gFramesPerSecondFrac;
ObjNode			*prop;
OGLMatrix4x4	*carMatrix;
OGLMatrix4x4	m1,rotM,m2;
short			playerNum = theCar->PlayerNum;
static const OGLPoint3D	propOffset = {0,0,140};
OGLPoint3D		coord;

	carMatrix = &theCar->BaseTransformMatrix;					// point to car's transform matrix

	prop = theCar->ChainNode;								// get prop
	if (!prop)
		return;

	if (gPlayerInfo[playerNum].submarineImmobilized == 0.0f)
		prop->Rot.z += fps * 20.0f;						// spin prop

	OGLMatrix4x4_SetRotate_Z(&rotM, prop->Rot.z);
	OGLMatrix4x4_SetTranslate(&m1, propOffset.x, propOffset.y, propOffset.z);
	OGLMatrix4x4_Multiply(&rotM, &m1, &m2);
	OGLMatrix4x4_Multiply(&m2, carMatrix, &prop->BaseTransformMatrix);
	SetObjectTransformMatrix(prop);


			/* STREAM BUBBLES */

	prop->BubbleTimer -= fps;									// check bubble timer
	if (prop->BubbleTimer <= 0.0f)
	{
		prop->BubbleTimer += .03f;

		coord.x = prop->BaseTransformMatrix.value[M03];			// get coord from prop matrix
		coord.y = prop->BaseTransformMatrix.value[M13];
		coord.z = prop->BaseTransformMatrix.value[M23];
		MakeBubbles(theCar, &coord, .5, 1.0);
	}
}

















