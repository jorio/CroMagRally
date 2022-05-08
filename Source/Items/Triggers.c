/****************************/
/*    	TRIGGERS	        */
/*  (c)2000 Pangea Software */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/*******************/
/*   PROTOTYPES    */
/*******************/

static Boolean DoTrig_POW(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MovePOW(ObjNode *theNode);
static void MoveToken(ObjNode *theNode);
static Boolean DoTrig_Token(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static Boolean DoTrig_StickyTiresPOW(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static Boolean DoTrig_SuspensionPOW(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static Boolean DoTrig_Cactus(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveCactus(ObjNode *theNode);

static Boolean DoTrig_SnoMan(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void ExplodeSnoMan(ObjNode *theNode);

static void MoveCampFire(ObjNode *theNode);
static Boolean DoTrig_CampFire(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

static void MoveTeamTorch(ObjNode *theNode);
static Boolean DoTrig_TeamTorch(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static Boolean DoTrig_TeamBase(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

static Boolean DoTrig_InvisibilityPOW(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

static Boolean DoTrig_Vase(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void ExplodeVase(ObjNode *theNode);

static void MoveCauldron(ObjNode *theNode);
static Boolean DoTrig_Cauldron(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

static void MoveGong(ObjNode *theNode);
static Boolean DoTrig_Gong(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

static Boolean DoTrig_SeaMine(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveSeaMine(ObjNode *theNode);

static Boolean DoTrig_Druid(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_TORCHES				(6*2)

#define	GONG_PYLON_RADIUS			50.0f

enum
{
	GONE_MODE_READY,
	GONE_MODE_SWING

};



/**********************/
/*     VARIABLES      */
/**********************/

#define	POWType			Special[0]			// powerup type
#define	POWHiddenTimer	SpecialF[0]
#define	POWHidden		Flag[0]
												// TRIGGER HANDLER TABLE
												//========================

Boolean	(*gTriggerTable[])(ObjNode *, ObjNode *, Byte) =
{
	DoTrig_POW,
	DoTrig_InvisibilityPOW,
	DoTrig_Token,
	DoTrig_StickyTiresPOW,
	DoTrig_SuspensionPOW,
	DoTrig_Cactus,
	DoTrig_SnoMan,
	DoTrig_CampFire,
	DoTrig_TeamTorch,
	DoTrig_TeamBase,
	DoTrig_Vase,
	DoTrig_Cauldron,
	DoTrig_Gong,
	DoTrig_LandMine,
	DoTrig_SeaMine,
	DoTrig_Lava,
	DoTrig_Druid,
};




#define	GongSwingIndex		SpecialF[0]
#define	GongSwingSpan		SpecialF[1]

#define	MineWobble			SpecialF[0]


Boolean gAnnouncedPOW[MAX_POW_TYPES];


short	gNumTorches;
ObjNode	*gTorchObjs[MAX_TORCHES];



/******************** HANDLE TRIGGER ***************************/
//
// INPUT: triggerNode = ptr to trigger's node
//		  whoNode = who touched it?
//		  side = side bits from collision.  Which side (of hitting object) hit the trigger
//
// OUTPUT: true if we want to handle the trigger as a solid object
//
// NOTE:  Triggers cannot self-delete in their DoTrig_ calls!!!  Bad things will happen in the hander collision function!
//

Boolean HandleTrigger(ObjNode *triggerNode, ObjNode *whoNode, Byte side)
{
	if (triggerNode->CBits & CBITS_TOUCHABLE)					// see if a non-solid trigger
	{
		return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
	}

			/* CHECK SIDES */
	else
	if (side & SIDE_BITS_BACK)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_FRONT)		// if my back hit, then must be front-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_FRONT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_BACK)			// if my front hit, then must be back-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_LEFT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_RIGHT)		// if my left hit, then must be right-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_RIGHT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_LEFT)			// if my right hit, then must be left-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_TOP)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_BOTTOM)		// if my top hit, then must be bottom-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_BOTTOM)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_TOP)			// if my bottom hit, then must be top-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
		return(true);											// assume it can be solid since didnt trigger
}


#pragma mark -

/************************* ADD POW *********************************/


Boolean AddPOW(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*newObj;
OGLPoint3D		where;
short			powType;
float			heightOff;

	powType = itemPtr->parm[0];								// get POW type
	heightOff = (float)itemPtr->parm[1] * 400.0f;			// get height off ground

				/* CREATE OBJECT */

	where.y = GetTerrainY(x,z) + heightOff;
	where.x = x;
	where.z = z;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_GLOBAL,
		.type		= GLOBAL_ObjType_BonePOW + powType,
		.coord		= where,
		.flags		= gAutoFadeStatusBits | STATUS_BIT_AIMATCAMERA | STATUS_BIT_KEEPBACKFACES
						| STATUS_BIT_NOLIGHTING | STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_CLIPALPHA,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MovePOW,
		.scale 		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return false;

	newObj->TerrainItemPtr = itemPtr;						// keep ptr to item list

	newObj->POWType = powType;								// remember powerup type


			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_POW;

	newObj->POWHidden = false;

	SetObjectCollisionBounds(newObj, 300, 0, -150, 150, 150, -150);		// make collision box
	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 9, 4, false);


	return(true);							// item was added
}


/*********************** MOVE POW ************************/

static void MovePOW(ObjNode *theNode)
{
		/* SEE IF GONE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


		/* SEE IF HIDDEN */

	if (theNode->POWHidden)
	{
		theNode->POWHiddenTimer -= gFramesPerSecondFrac;
		if (theNode->POWHiddenTimer <= 0.0f)
		{
			theNode->POWHidden = false;
			theNode->Scale.x =
			theNode->Scale.y =
			theNode->Scale.z = 1.0;

			theNode->CType = CTYPE_TRIGGER;
			theNode->StatusBits &= ~STATUS_BIT_HIDDEN;
			if (theNode->ShadowNode)						// also unhide shadow
				(theNode->ShadowNode)->StatusBits &= ~STATUS_BIT_HIDDEN;
		}
		else
		if (theNode->POWHiddenTimer <= .5f)			// scale back in
		{
			theNode->CType = CTYPE_TRIGGER;
			theNode->StatusBits &= ~STATUS_BIT_HIDDEN;

			theNode->Scale.x =
			theNode->Scale.y =
			theNode->Scale.z = (.5f - theNode->POWHiddenTimer) * 2.0f;

			if (theNode->ShadowNode)						// also unhide shadow
				(theNode->ShadowNode)->StatusBits &= ~STATUS_BIT_HIDDEN;

		}
	}

	UpdateShadow(theNode);
}


/************** DO TRIGGER - POW ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_POW(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	powType,playerNum;
Boolean	thud = false;

	(void) sideBits;

	playerNum = whoNode->PlayerNum;
	powType = theNode->POWType;

	if (gPlayerInfo[playerNum].powType == powType)		// see if we already have this
	{
		gPlayerInfo[playerNum].powQuantity += 1;
		thud = true;
	}
	else												// reset to this weapon
	{
		gPlayerInfo[playerNum].powType = powType;
		gPlayerInfo[playerNum].powQuantity = 1;
		gPlayerInfo[playerNum].attackTimer = 1.0;		// CPU cars dont use this for at least 1 second

		if (gPlayerInfo[playerNum].onThisMachine && (!gPlayerInfo[playerNum].isComputer) && (!gAnnouncedPOW[powType]))
		{
			PlayAnnouncerSound(EFFECT_POW_BONEBOMB + powType,false, .4);
			gAnnouncedPOW[powType] = true;
		}
		else
			thud = true;
	}

	if (thud)
		PlayEffect_Parms3D(EFFECT_GETPOW, &theNode->Coord, NORMAL_CHANNEL_RATE, 2.0);



			/* PUT INTO HIDE MODE */

	theNode->POWHidden = true;
	theNode->POWHiddenTimer = 5.0;					// n seconds until it reappears
	theNode->StatusBits |= STATUS_BIT_HIDDEN;
	theNode->CType = 0;

	if (theNode->ShadowNode)						// also hide shadow
	{
		theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;
	}


	return(false);								// not solid
}




#pragma mark -

/************************* ADD TOKEN *********************************/


Boolean AddToken(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*newObj;
OGLPoint3D		where;

	if (gGameMode != GAME_MODE_TOURNAMENT)				// only do these in this mode
	{
		return(true);
	}

				/* CREATE OBJECT */

	where.y = GetTerrainY(x,z);
	where.x = x;
	where.z = z;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_GLOBAL,
		.type		= GLOBAL_ObjType_Token_ArrowHead,
		.coord		= where,
		.flags		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MoveToken,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return false;

	newObj->TerrainItemPtr = itemPtr;						// keep ptr to item list


			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_TOKEN;

	SetObjectCollisionBounds(newObj, 300, 0, -150, 150, 150, -150);		// make collision box



			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 12, 5, false);


	return(true);							// item was added
}


/*********************** MOVE TOKEN ************************/

static void MoveToken(ObjNode *theNode)
{
		/* SEE IF GONE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Rot.y += gFramesPerSecondFrac;
	UpdateObjectTransforms(theNode);
	UpdateShadow(theNode);
}


/************** DO TRIGGER - TOKEN ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Token(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	playerNum;

	(void) sideBits;

	playerNum = whoNode->PlayerNum;
	if (gPlayerInfo[playerNum].isComputer)		// CPU players cannot collect these, only real players can
		return(false);

	gPlayerInfo[playerNum].numTokens++;			// inc token counter
	gTotalTokens++;

			/* AUDIO */

	PlayEffect_Parms3D(EFFECT_GETPOW, &theNode->Coord, NORMAL_CHANNEL_RATE, 2.0);

	if (gTotalTokens == MAX_TOKENS)
	{
		PlayAnnouncerSound(EFFECT_THATSALL, false, .4);
	}
	else
		PlayAnnouncerSound(EFFECT_ARROWHEAD, false, .4);


			/* FREE THE POW */

	theNode->TerrainItemPtr	= nil;				// dont ever come back
	DeleteObject(theNode);


	return(false);								// not solid
}



#pragma mark -

/************************* ADD STICKY TIRES POW *********************************/

Boolean AddStickyTiresPOW(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*newObj;
OGLPoint3D		where;

				/* CREATE OBJECT */

	where.y = GetTerrainY(x,z);
	where.x = x;
	where.z = z;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_GLOBAL,
		.type		= GLOBAL_ObjType_StickyTiresPOW,
		.coord		= where,
		.flags		= gAutoFadeStatusBits | STATUS_BIT_AIMATCAMERA | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOLIGHTING,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MovePOW,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return false;

	newObj->TerrainItemPtr = itemPtr;						// keep ptr to item list


			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_TRACTION;

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);



			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 12, 8, false);


	return(true);							// item was added
}



/************** DO TRIGGER - STICKY TIRES POW ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_StickyTiresPOW(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	playerNum;

	(void) sideBits;

	playerNum = whoNode->PlayerNum;


			/* ANNOUNCE IT */

	if (gPlayerInfo[playerNum].stickyTiresTimer <= 0.0f)
		if (gPlayerInfo[playerNum].onThisMachine && (!gPlayerInfo[playerNum].isComputer))
			PlayAnnouncerSound(EFFECT_POW_STICKYTIRES,false, .5);


				/* SET THE TICKY TIRES */

	SetStickyTires(playerNum);

	PlayEffect_Parms3D(EFFECT_GETPOW, &theNode->Coord, NORMAL_CHANNEL_RATE, 2.0);

			/* PUT INTO HIDE MODE */

	theNode->POWHidden = true;
	theNode->POWHiddenTimer = 5.0;					// n seconds until it reappears
	theNode->StatusBits |= STATUS_BIT_HIDDEN;
	theNode->CType = 0;

	if (theNode->ShadowNode)						// also hide shadow
	{
		theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;
	}

	return(false);								// not solid
}



#pragma mark -

/************************* ADD SUSPENSION POW *********************************/

Boolean AddSuspensionPOW(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*newObj;
OGLPoint3D		where;

				/* CREATE OBJECT */

	where.y = GetTerrainY(x,z);
	where.x = x;
	where.z = z;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_GLOBAL,
		.type		= GLOBAL_ObjType_SuspensionPOW,
		.coord		= where,
		.flags		= gAutoFadeStatusBits | STATUS_BIT_AIMATCAMERA | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOLIGHTING,
		.slot		= TRIGGER_SLOT,
		.moveCall 	= MovePOW,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return false;

	newObj->TerrainItemPtr = itemPtr;						// keep ptr to item list


			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_SUSPENSION;

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);



			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 13, 13, false);


	return(true);							// item was added
}



/************** DO TRIGGER - SUSPENSION POW ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_SuspensionPOW(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	playerNum;

	(void) sideBits;

	playerNum = whoNode->PlayerNum;

			/* ANNOUNCE IT */

	if (gPlayerInfo[playerNum].superSuspensionTimer <= 0.0f)
		if (gPlayerInfo[playerNum].onThisMachine && (!gPlayerInfo[playerNum].isComputer))
			PlayAnnouncerSound(EFFECT_POW_SUSPENSION,false, .5);

				/* SET THE SUSPENSION */

	SetSuspensionPOW(playerNum);

	PlayEffect_Parms3D(EFFECT_GETPOW, &theNode->Coord, NORMAL_CHANNEL_RATE, 2.0);

			/* PUT INTO HIDE MODE */

	theNode->POWHidden = true;
	theNode->POWHiddenTimer = 5.0;					// n seconds until it reappears
	theNode->StatusBits |= STATUS_BIT_HIDDEN;
	theNode->CType = 0;

	if (theNode->ShadowNode)						// also hide shadow
	{
		theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;
	}


	return(false);								// not solid
}

#pragma mark -

/************************* ADD INVISIBILITY POW *********************************/


Boolean AddInvisibilityPOW(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*newObj;
OGLPoint3D		where;

				/* CREATE OBJECT */

	where.y = GetTerrainY(x,z);
	where.x = x;
	where.z = z;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_GLOBAL,
		.type		= GLOBAL_ObjType_InvisibilityPOW,
		.coord		= where,
		.flags		= gAutoFadeStatusBits | STATUS_BIT_AIMATCAMERA | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOTEXTUREWRAP,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MovePOW,
		.rot		= 0,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return false;

	newObj->TerrainItemPtr = itemPtr;						// keep ptr to item list


			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_INVISIBILITY;

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);



			/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 13, 13, false);


	return(true);							// item was added
}



/************** DO TRIGGER - INVISIBILITY POW ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_InvisibilityPOW(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	playerNum;

	(void) sideBits;

	playerNum = whoNode->PlayerNum;


			/* ANNOUNCE IT */

	if (gPlayerInfo[playerNum].invisibilityTimer <= 0.0f)
		if (gPlayerInfo[playerNum].onThisMachine && (!gPlayerInfo[playerNum].isComputer))
			PlayAnnouncerSound(EFFECT_POW_INVISIBILITY,false, .5);


	gPlayerInfo[playerNum].invisibilityTimer = 15.0;				// set duration


	PlayEffect_Parms3D(EFFECT_GETPOW, &theNode->Coord, NORMAL_CHANNEL_RATE, 2.0);


			/* PUT INTO HIDE MODE */

	theNode->POWHidden = true;
	theNode->POWHiddenTimer = 5.0;					// n seconds until it reappears
	theNode->StatusBits |= STATUS_BIT_HIDDEN;
	theNode->CType = 0;

	if (theNode->ShadowNode)						// also hide shadow
	{
		theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;
	}


	return(false);								// not solid
}





#pragma mark -

/************************* ADD CACTUS *********************************/

Boolean AddCactus(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
Boolean	notSolid = itemPtr->parm[3];
short	cactusType = itemPtr->parm[0];			// get cactus type

	if (cactusType > 1)
		cactusType = 1;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= DESERT_ObjType_Cactus + cactusType,
		.coord		= {x,0,z},  // y filled in below
		.slot		= TRIGGER_SLOT,
		.flags		= gAutoFadeStatusBits,
		.moveCall	= MoveCactus,
		.scale		= 1.0,
	};
	def.coord.y = GetMinTerrainY(x,z, def.group, def.type, 1.0) - gObjectGroupBBoxList[def.group][def.type].min.y,
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	if (!notSolid)
	{
		if (cactusType == 1)
			newObj->CType 			= CTYPE_MISC|CTYPE_AVOID;
		else
			newObj->CType 			= CTYPE_TRIGGER|CTYPE_AVOID;
		newObj->Kind		 	= TRIGTYPE_CACTUS;
		newObj->CBits			= CBITS_ALLSOLID;
		newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it

		CreateCollisionBoxFromBoundingBox(newObj,.9,1);
	}

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list
	newObj->Flag[0] = false;										// not hit yet



	return(true);													// item was added
}


/****************** MOVE CACTUS ********************/

static void MoveCactus(ObjNode *theNode)
{
float	fps;

		/* SEE IF GONE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* SEE IF IT HAS BEEN HIT */

	if (!(theNode->Flag[0]))
		return;



			/* MOVE THE FLYING CACTUS */

	GetObjectInfo(theNode);

	fps = gFramesPerSecondFrac;

	gDelta.y -= DEFAULT_GRAVITY * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

	if (GetTerrainY(gCoord.x, gCoord.y) >= gCoord.y)		// see if hit ground
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;


	UpdateObject(theNode);

}


/************** DO TRIGGER - CACTUS ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Cactus(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
float	speed;

	(void) sideBits;

		/* SEE IF GOING FAST ENOUGH TO SMASH IT */

	if ((speed = whoNode->Speed3D) > 2000.0f)
	{
		theNode->TerrainItemPtr	= nil;				// dont ever come back
		theNode->Flag[0] = true;					// set the got-hit flag
		theNode->CType = 0;							// not a trigger anymore

		theNode->Delta.y = speed * .5f;
		theNode->Delta.x = gDelta.x * .3f;
		theNode->Delta.z = gDelta.z * .3f;

		speed *= .0015f;
		theNode->DeltaRot.x = RandomFloat2() * speed;		// set spinning
		theNode->DeltaRot.z = RandomFloat2() * speed;
		theNode->DeltaRot.y = RandomFloat2() * speed * .5f;

		gDelta.y += 1000.0f;				// pop up the guy who hit the cactus
		gDelta.x *= .5f;					// slow
		gDelta.z *= .5f;
		return(false);
	}

	return(true);

}





#pragma mark -

/************************* ADD SNOMAN *********************************/

Boolean AddSnoMan(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	NewObjectDefinitionType def =
	{
		.group 		= MODEL_GROUP_LEVELSPECIFIC,
		.type 		= ICE_ObjType_SnoMan,
		.coord.x 	= x,
		.coord.z 	= z,
		.coord.y 	= GetTerrainY(x,z),
		.slot 		= TRIGGER_SLOT,
		.flags 		= gAutoFadeStatusBits,
		.moveCall 	= MoveStaticObject,
		.rot 		= (float)(itemPtr->parm[0]) / 8.0f * PI2,
		.scale 		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_AVOID;
	newObj->Kind		 	= TRIGTYPE_SNOMAN;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it

	SetObjectCollisionBounds(newObj, 1000, -10, -300, 300, 300, -300);		// make collision box


	return(true);													// item was added
}


/************** DO TRIGGER - SNOMAN ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_SnoMan(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{

	(void) sideBits;

		/* SEE IF GOING FAST ENOUGH TO SMASH IT */

	if (whoNode->Speed3D > 2000.0f)
	{
		ExplodeSnoMan(theNode);
		theNode->TerrainItemPtr	= nil;				// dont ever come back
		DeleteObject(theNode);

		gDelta.y += 1000.0f;				// pop up the guy who hit the it
		gDelta.x *= .2f;					// slow
		gDelta.z *= .2f;

		return(false);
	}

	return(true);
}


/************** EXPLODE SNOMAN  *********************/

static void ExplodeSnoMan(ObjNode *theNode)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;


	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 30;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= .6;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_SnowDust;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
	pg = NewParticleGroup(&gNewParticleGroupDef);

	if (pg != -1)
	{
		x = theNode->Coord.x;
		y = theNode->Coord.y;
		z = theNode->Coord.z;

		for (i = 0; i < 220; i++)
		{
			pt.x = x + RandomFloat2() * 140.0f;
			pt.y = y + RandomFloat() * 700.0f;
			pt.z = z + RandomFloat2() * 140.0f;

			d.y = RandomFloat() * 800.0f;
			d.x = RandomFloat2() * 600.0f;
			d.z = RandomFloat2() * 600.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}


	PlayEffect_Parms3D(EFFECT_HITSNOW, &theNode->Coord, NORMAL_CHANNEL_RATE, 4);

}

#pragma mark -

/************************* ADD CAMP FIRE *********************************/

Boolean AddCampFire(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= gTrackNum == TRACK_NUM_ICE ? ICE_ObjType_CampFire : SCANDINAVIA_ObjType_Campfire,
		.coord.x	= x,
		.coord.z	= z,
		.coord.y	= GetTerrainY(x,z),
		.flags		= gAutoFadeStatusBits,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MoveCampFire,
		.rot		= 0,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_AVOID;
	newObj->Kind		 	= TRIGTYPE_CAMPFIRE;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	CreateCollisionBoxFromBoundingBox(newObj,.9,1);


	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;
	newObj->SmokeTimer = 0;

	return(true);													// item was added
}


/********************* MOVE CAMP FIRE *************************/

static void MoveCampFire(ObjNode *theNode)
{

			/***************/
			/* SEE IF GONE */
			/***************/

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

	BurnFire(theNode, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, true, PARTICLE_SObjType_Fire, 1.0);
}


/************** DO TRIGGER - CAMPFIRE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_CampFire(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{

	(void) sideBits;


	if (whoNode->Speed3D > 2000.0f)
	{

			/* MAKE CAR SPIN WILDLY */

		whoNode->DeltaRot.y = RandomFloat2() * 20.0f;
		whoNode->DeltaRot.z = RandomFloat2() * 10.0f;


			/* MAKE EXPLOSION */

		MakeSparkExplosion(theNode->Coord.x, theNode->Coord.y+50.0f, theNode->Coord.z,
							whoNode->Speed2D * .1f, PARTICLE_SObjType_Fire);

		MakeConeBlast(theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);

		PlayEffect_Parms3D(EFFECT_BOOM, &theNode->Coord, NORMAL_CHANNEL_RATE, 4);


				/* DELETE THE FIRE */

		theNode->TerrainItemPtr = nil;							// dont come back
		DeleteObject(theNode);

		gDelta.y += 1300.0f;				// pop up the guy who hit the it
		gDelta.x *= .3f;					// slow
		gDelta.z *= .3f;

		return(false);
	}

	return(true);
}




#pragma mark -

/************************* ADD TEAM TORCH *********************************/

Boolean AddTeamTorch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	if (gGameMode != GAME_MODE_CAPTUREFLAG)					// only in capture flag mode
		return(true);

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_GLOBAL,
		.type		= GLOBAL_ObjType_TeamTorch,
		.coord.x	= x,
		.coord.z	= z,
		.coord.y	= GetTerrainY(x,z),
		.flags		= gAutoFadeStatusBits,
		.slot		= PLAYER_SLOT + 5,	// we need this trigger to be *after* the player so it looks correct when carried!
		.moveCall	= MoveTeamTorch,
		.rot		= RandomFloat() * PI2,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType 			= CTYPE_TRIGGER;
	newObj->Kind		 	= TRIGTYPE_TEAMTORCH;
	newObj->CBits			= CBITS_TOUCHABLE;				// non-solid trigger
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it

	SetObjectCollisionBounds(newObj, 1000, -10, -100, 100, 100, -100);		// make collision box


	newObj->Mode			= 0;									// it can be picked up

	newObj->TorchTeam		= itemPtr->parm[0];						// which team?


			/* INIT FIRE & SMOKE */

	newObj->SmokeParticleGroup = -1;
	newObj->SmokeParticleMagic = 0;


	if (gNumTorches < MAX_TORCHES)
	{
		gTorchObjs[gNumTorches] = newObj;
		gNumTorches++;
	}

	return(true);													// item was added
}


/********************* MOVE TEAM TORCH *************************/

static void MoveTeamTorch(ObjNode *theNode)
{
float			fps = gFramesPerSecondFrac;
float			y,tipX,tipY,tipZ;
short			p;
ObjNode			*car;
OGLMatrix4x4	*carMatrix;
OGLMatrix4x4	m1,m2;

	switch(theNode->Mode)
	{
			/* NOT BEING CARRIED, READY TO TRIGGER */

		case	0:
				GetObjectInfo(theNode);
				gDelta.y -= 2000.0f * fps;							// gravity
				gCoord.y += gDelta.y * fps;
				y = GetTerrainY(gCoord.x, gCoord.z);				// get terrain y
				if (gCoord.y < y)
				{
					theNode->CType = CTYPE_TRIGGER;					// if on ground, then make sure its a trigger
					gCoord.y = y;
					gDelta.y = 0;
				}
				UpdateObject(theNode);

				tipX = gCoord.x;									// set coord for fire
				tipY = gCoord.y + 110.0f;
				tipZ = gCoord.z;
				break;


			/* BEING CARRIED */

		case	1:
				p = theNode->PlayerNum;								// get player # who's carrying me
				car = gPlayerInfo[p].objNode;						// get car obj
				carMatrix = &car->BaseTransformMatrix;				// point to car's transform matrix

				OGLMatrix4x4_SetTranslate(&m1, 0, 40, 80);			// calc torch coord
				OGLMatrix4x4_Multiply(&m1, carMatrix, &theNode->BaseTransformMatrix);
				SetObjectTransformMatrix(theNode);
				theNode->Coord = car->Coord;						// set the actual coord so the autofade will work

				OGLMatrix4x4_SetTranslate(&m1, 0, 170, 80);			// calc torch coord
				OGLMatrix4x4_Multiply(&m1, carMatrix, &m2);
				tipX = m2.value[M03];								// extract xyz from matrix
				tipY = m2.value[M13];
				tipZ = m2.value[M23];
				break;

			/* RECOVERED - NOT MOVING AGAIN */

		case	2:
				tipX = theNode->Coord.x;									// set coord for fire
				tipY = theNode->Coord.y + 110.0f;
				tipZ = theNode->Coord.z;
				break;

		default:
				printf("Incorrect TeamTorch mode: %d\n", theNode->Mode);
				return;

	}

		/* ITS ALWAYS BURNING */

	if (theNode->TorchTeam == 0)
		BurnFire(theNode, tipX, tipY, tipZ, false, PARTICLE_SObjType_Fire, .5);
	else
		BurnFire(theNode, tipX, tipY, tipZ, false, PARTICLE_SObjType_GreenFire, .5);


}


/************** DO TRIGGER - TEAM TORCH ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_TeamTorch(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) sideBits;

	if (whoNode->CapturedFlag)					// see if player already has a flag
		return(true);

	theNode->CType = 0;							// its not a trigger while being carried
	theNode->Mode = 1;							// set carried mode
	theNode->PlayerNum = whoNode->PlayerNum;	// remember which player is carrying it

	whoNode->CapturedFlag = (Ptr)theNode;		// player remembers the flag


	return(false);
}


/*********************** PLAYER DROP FLAG ***********************/

void PlayerDropFlag(ObjNode *theCar)
{
ObjNode	*theTorch;

	theTorch = (ObjNode *)theCar->CapturedFlag;				// get torch object that player is carrying
	if (theTorch == nil)									// bail if none
		return;

	theCar->CapturedFlag = nil;								// release it

	theTorch->Mode = 0;										// make normal again

	theTorch->Delta.y = 1200;

	GetTerrainY(theTorch->Coord.x, theTorch->Coord.z);	// call this to calc terrain normal
	if (gRecentTerrainNormal.y < .5f)					// if dropped on a steep slope, then reset to home
		theTorch->Coord = theTorch->InitCoord;
}



#pragma mark -

/************************* ADD TEAM BASE *********************************/

Boolean AddTeamBase(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	if (gGameMode != GAME_MODE_CAPTUREFLAG)					// only in capture flag mode
		return(true);

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_GLOBAL,
		.type		= GLOBAL_ObjType_TeamBaseRed + itemPtr->parm[0],
		.coord.x	= x,
		.coord.z	= z,
		.coord.y	= GetTerrainY(x,z),
		.slot		= TRIGGER_SLOT,
		.flags		= gAutoFadeStatusBits,
		.moveCall	= MoveStaticObject,
		.rot		= 0,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER;
	newObj->Kind		 	= TRIGTYPE_TEAMBASE;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	newObj->TorchTeam		= itemPtr->parm[0];						// which team?


	return(true);													// item was added
}


/************** DO TRIGGER - TEAMBASE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_TeamBase(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
ObjNode	*theTorch;
short	teamNum;

	(void) sideBits;

	theTorch = (ObjNode *)whoNode->CapturedFlag;				// get torch object that player is carrying
	if (theTorch == nil)										// bail if none
		return(true);

	teamNum = theTorch->TorchTeam;								// get team # that this torch belongs to
	if (teamNum != theNode->TorchTeam)							// if this color torch doesnt belong on this base then bail
		return(true);


		/* DROP THE TORCH AT THE BASE */

	whoNode->CapturedFlag = nil;								// player no longer has the torch

	gCapturedFlagCount[teamNum]++;								// we got ourselves another flag


	if (gCapturedFlagCount[teamNum] >= NUM_FLAGS_TO_GET)		// see if that was the last one
	{
		if (teamNum == 0)
			PlayEffect_Parms(EFFECT_REDTEAMWINS, FULL_CHANNEL_VOLUME * 3, FULL_CHANNEL_VOLUME*2, NORMAL_CHANNEL_RATE);
		else
			PlayEffect_Parms(EFFECT_GREENTEAMWINS, FULL_CHANNEL_VOLUME * 3, FULL_CHANNEL_VOLUME*2, NORMAL_CHANNEL_RATE);
	}
	else														// say "Score!"
	{
		if (gPlayerInfo[whoNode->PlayerNum].onThisMachine)
			PlayAnnouncerSound(EFFECT_GOODJOB, false, 0);
	}



			/* PUT TORCH ON BASE */

	theTorch->Coord.x = theNode->Coord.x + RandomFloat2() * 100.0f;
	theTorch->Coord.z = theNode->Coord.z + RandomFloat2() * 100.0f;
	theTorch->Coord.y = theNode->Coord.y + gObjectGroupBBoxList[MODEL_GROUP_GLOBAL][GLOBAL_ObjType_TeamBaseRed].max.y;

	UpdateObjectTransforms(theTorch);

	theTorch->Mode = 2;											// never move again



	return(true);
}


#pragma mark -


/************************* ADD VASE *********************************/

Boolean AddVase(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	NewObjectDefinitionType def =
	{
		.group 		= MODEL_GROUP_LEVELSPECIFIC,
		.type 		= EGYPT_ObjType_Vase,
		.coord.x 	= x,
		.coord.z 	= z,
		.coord.y 	= GetTerrainY(x,z),
		.slot 		= TRIGGER_SLOT,
		.flags 		= gAutoFadeStatusBits | STATUS_BIT_KEEPBACKFACES,
		.moveCall 	= MoveStaticObject,
		.rot 		= (float)(itemPtr->parm[0]) / 8.0f * PI2,
		.scale 		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_AVOID;
	newObj->Kind		 	= TRIGTYPE_VASE;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);


	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 10, 10, false);


	return(true);													// item was added
}


/************** DO TRIGGER - VASE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Vase(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{

	(void) sideBits;

		/* SEE IF GOING FAST ENOUGH TO SMASH IT */

	if (whoNode->Speed3D > 2000.0f)
	{
		ExplodeVase(theNode);
		theNode->TerrainItemPtr	= nil;				// dont ever come back
		DeleteObject(theNode);

		gDelta.y += 1000.0f;				// pop up the guy who hit the it
		gDelta.x *= .2f;					// slow
		gDelta.z *= .2f;

		PlayEffect_Parms3D(EFFECT_SHATTER, &theNode->Coord, NORMAL_CHANNEL_RATE, 3);

		return(false);
	}

	return(true);
}


/************** EXPLODE VASE  *********************/

static void ExplodeVase(ObjNode *theNode)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;


	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= 0;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 30;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= .6;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Dirt;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
	pg = NewParticleGroup(&gNewParticleGroupDef);

	if (pg != -1)
	{
		x = theNode->Coord.x;
		y = theNode->Coord.y;
		z = theNode->Coord.z;

		for (i = 0; i < 100; i++)
		{
			pt.x = x + RandomFloat2() * 100.0f;
			pt.y = y + RandomFloat() * 300.0f;
			pt.z = z + RandomFloat2() * 100.0f;

			d.y = RandomFloat() * 400.0f;
			d.x = RandomFloat2() * 600.0f;
			d.z = RandomFloat2() * 600.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}

}


#pragma mark -

/******************** ADD CAULDRON *********************/

Boolean AddCauldron(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= EUROPE_ObjType_Cauldron,
		.coord.x	= x,
		.coord.z	= z,
		.coord.y	= GetTerrainY(x,z),
		.flags		= gAutoFadeStatusBits|STATUS_BIT_NOTEXTUREWRAP,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MoveCauldron,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_AVOID;
	newObj->Kind		 	= TRIGTYPE_CAULDRON;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);


	return(true);													// item was added
}


/********************* MOVE CAULDRON *************************/

static void MoveCauldron(ObjNode *theNode)
{

			/***************/
			/* SEE IF GONE */
			/***************/

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

	BurnFire(theNode, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, true, PARTICLE_SObjType_Fire, 1.0);
}



/************** DO TRIGGER - CAULDRON ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Cauldron(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) sideBits;

		/* SEE IF GOING FAST ENOUGH TO SMASH IT */

	if (whoNode->Speed3D > 1500.0f)
	{
		MakeFireExplosion(theNode->Coord.x, theNode->Coord.z, &gDelta);
		PlayEffect_Parms3D(EFFECT_SELECTCLICK, &theNode->Coord, NORMAL_CHANNEL_RATE / 3, 3);

		DeleteObject(theNode);
		return(false);
	}

	return(true);
}



#pragma mark -

/******************** ADD GONG *********************/

Boolean AddGong(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*frame,*gong;
CollisionBoxType *boxPtr;
OGLPoint2D		p,p1,p2;
OGLMatrix3x3	m;

					/***************/
					/* BUILD FRAME */
					/***************/
	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= CHINA_ObjType_GongFrame,
		.coord		= {x,0,z},  // y filled in below
		.flags		= gAutoFadeStatusBits,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MoveGong,
		.rot		= PI2 * ((float)itemPtr->parm[0] * (1.0f/4.0f)),
		.scale 		= 1.0,
	};
	def.coord.y = GetMinTerrainY(x, z, def.group, def.type, 1.0);
	frame = MakeNewDisplayGroupObject(&def);
	if (frame == nil)
		return(false);

	frame->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	frame->CType 			= CTYPE_MISC; //|CTYPE_AVOID;
	frame->CBits			= CBITS_ALLSOLID;


		/* CALC COORDS OF EACH SUPPORT BEAM */

	OGLMatrix3x3_SetRotate(&m, -frame->Rot.y);
	p.x = -395.0f;
	p.y = 0;
	OGLPoint2D_Transform(&p, &m, &p1);				// calc left
	p.x = -p.x;
	OGLPoint2D_Transform(&p, &m, &p2);				// calc right

			/* BUILD 2 COLLISION BOXES */

	AllocateCollisionBoxMemory(frame, 2);					// alloc 2 collision boxes
	boxPtr = frame->CollisionBoxes;						// get ptr to boxes
	boxPtr[0].left 		= frame->Coord.x + p1.x - GONG_PYLON_RADIUS;
	boxPtr[0].right 	= frame->Coord.x + p1.x + GONG_PYLON_RADIUS;
	boxPtr[0].top 		= frame->Coord.y + 1000.0;
	boxPtr[0].bottom 	= frame->Coord.y - 10.0f;
	boxPtr[0].back 		= frame->Coord.z + p1.y - GONG_PYLON_RADIUS;
	boxPtr[0].front 	= frame->Coord.z + p1.y + GONG_PYLON_RADIUS;
	boxPtr[1].left 		= frame->Coord.x + p2.x - GONG_PYLON_RADIUS;
	boxPtr[1].right 	= frame->Coord.x + p2.x + GONG_PYLON_RADIUS;
	boxPtr[1].top 		= frame->Coord.y + 1000.0;
	boxPtr[1].bottom 	= frame->Coord.y - 10.0f;
	boxPtr[1].back 		= frame->Coord.z + p2.y - GONG_PYLON_RADIUS;
	boxPtr[1].front 	= frame->Coord.z + p2.y + GONG_PYLON_RADIUS;
	KeepOldCollisionBoxes(frame);


			/*************************/
			/* MAKE THE GONG TRIGGER */
			/*************************/

	def.type		= CHINA_ObjType_Gong;
	def.coord.y		+= 886.0;
	def.flags		= gAutoFadeStatusBits | STATUS_BIT_KEEPBACKFACES;
	def.slot++;
	def.moveCall	= nil;
	gong = MakeNewDisplayGroupObject(&def);
	if (gong == nil)
		return(false);


			/* SET COLLISION STUFF */

	gong->CType 		= CTYPE_TRIGGER;
	gong->Kind		 	= TRIGTYPE_GONG;
	gong->CBits			= CBITS_ALLSOLID;
	gong->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it

	CreateCollisionBoxFromBoundingBox_Rotated(gong,1,1);

	gong->Mode = GONE_MODE_READY;


	frame->ChainNode = gong;

	return(true);													// item was added
}


/********************* MOVE GONG *************************/

static void MoveGong(ObjNode *theNode)
{
ObjNode	*gongObj;

			/***************/
			/* SEE IF GONE */
			/***************/

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

		/***************/
		/* UPDATE GONG */
		/***************/

	gongObj = theNode->ChainNode;							// get gong obj

	switch(gongObj->Mode)
	{
		case	GONE_MODE_READY:
				break;


		case	GONE_MODE_SWING:
				gongObj->GongSwingSpan -= gFramesPerSecondFrac * .4f;
				if (gongObj->GongSwingSpan <= 0.0f)
				{
					gongObj->GongSwingSpan = 0.0f;
					gongObj->Mode = GONE_MODE_READY;
				}

				gongObj->GongSwingIndex += gFramesPerSecondFrac * 5.0f;
				gongObj->Rot.x = sin(gongObj->GongSwingIndex) * gongObj->GongSwingSpan;

				break;
	}

	UpdateObjectTransforms(gongObj);
}



/************** DO TRIGGER - GONG ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Gong(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) sideBits;

	if (theNode->Mode != GONE_MODE_READY)
		return(false);

		/* SEE IF GOING FAST ENOUGH TO GONG IT */

	if (whoNode->Speed3D > 1000.0f)
	{
		short	p = whoNode->PlayerNum;
		theNode->Mode = GONE_MODE_SWING;
		theNode->GongSwingSpan = PI/2;

				/* DOUBLE STUFF */

		gPlayerInfo[p].powQuantity *= 2;							// double weapons
		gPlayerInfo[p].stickyTiresTimer *= 2.0f;			// double stick tires timer
		gPlayerInfo[p].superSuspensionTimer *= 2.0f;			// double suspension timer


		PlayEffect_Parms3D(EFFECT_GONG, &theNode->Coord, NORMAL_CHANNEL_RATE, 3.0);

		return(false);
	}

	return(true);
}


#pragma mark -


/******************** ADD SEA MINE *********************/

Boolean AddSeaMine(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= ATLANTIS_ObjType_SeaMine,
		.coord.x	= x,
		.coord.z	= z,
		.coord.y	= GetTerrainY(x,z) + 300.0f + (float)itemPtr->parm[0] * 15.0f,
		.flags		= gAutoFadeStatusBits,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MoveSeaMine,
		.scale		= 1.0,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_AVOID;
	newObj->Kind		 	= TRIGTYPE_SEAMINE;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;
	SetObjectCollisionBounds(newObj, 300, -300, -300, 300, 300, -300);		// make collision box

	newObj->MineWobble = 0;

	return(true);													// item was added
}


/********************* MOVE SEA MINE **********************/

static void MoveSeaMine(ObjNode *theNode)
{
		/* SEE IF GONE */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}


	theNode->MineWobble += gFramesPerSecondFrac * .9f;
	theNode->Rot.x = sin(theNode->MineWobble) * .09f;
	UpdateObjectTransforms(theNode);
}



/************** DO TRIGGER - SEA MINE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_SeaMine(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
float	x,y,z;
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;

	(void) sideBits;
	(void) whoNode;

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;

	MakeShockwave(x, y, z);




		/*************/
		/* MAKE BUBBLES */
		/*************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= 0;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 25;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= .6;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Bubbles;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 150; i++)
		{
			pt.x = x + RandomFloat2() * 100.0f;
			pt.y = y + RandomFloat2() * 100.0f;
			pt.z = z + RandomFloat2() * 100.0f;

			d.y = RandomFloat2() * 700.0f;
			d.x = RandomFloat2() * 700.0f;
			d.z = RandomFloat2() * 700.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= 1.0;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}


	PlayEffect_Parms3D(EFFECT_BOOM, &pt, NORMAL_CHANNEL_RATE, 4);
	BlastCars(-1, x, y, z, 900);
	DeleteObject(theNode);

	return(true);
}


#pragma mark -

/************************* ADD DRUID *********************************/

Boolean AddDruid(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	NewObjectDefinitionType def =
	{
		.type		= SKELETON_TYPE_DRUID,
		.animNum	= 0,
		.coord.x	= x,
		.coord.z	= z,
		.coord.y	= GetTerrainY(x,z),
		.flags		= gAutoFadeStatusBits,
		.slot		= TRIGGER_SLOT,
		.moveCall	= MoveStaticObject,
		.rot		= 0,
		.scale		= 5.0,
	};
	newObj = MakeNewSkeletonObject(&def);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_DRUID;

	CreateCollisionBoxFromBoundingBox(newObj,1,1);


	return(true);													// item was added
}


/************** DO TRIGGER - DRUID ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Druid(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	p;

	(void) sideBits;

	p = whoNode->PlayerNum;

			/* PLAY SOUND */

	if ((int)gPlayerInfo[p].stickyTiresTimer < 19)		// only if not recently
	{
		PlayEffect_Parms3D(EFFECT_CHANT, &theNode->Coord, NORMAL_CHANNEL_RATE-0x5000, 2.0);
	}

			/* DOUBLE STUFF */

	gPlayerInfo[p].powQuantity = 20;
	gPlayerInfo[p].stickyTiresTimer = 20;
	gPlayerInfo[p].superSuspensionTimer = 20;

	return(true);
}






