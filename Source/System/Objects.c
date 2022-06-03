/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include "bones.h"
#include <stddef.h>

extern int gMyState_ProjectionType;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void FlushObjectDeleteQueue(void);
static void DrawCollisionBoxes(ObjNode *theNode, Boolean old);
static void DrawPathVec(short p);


/****************************/
/*    CONSTANTS             */
/****************************/

#define ALLOW_GL_CLIP_HINTS		0
#define	DO_EDGE_ALPHA_CLIPPING	1

#define	OBJ_DEL_Q_SIZE	100


/**********************/
/*     VARIABLES      */
/**********************/

											// OBJECT LIST
ObjNode		*gFirstNodePtr = nil;

ObjNode		*gCurrentNode,*gMostRecentlyAddedNode,*gNextNode;

OGLPoint3D	gCoord;
OGLVector3D	gDelta;

long		gNumObjsInDeleteQueue = 0;
ObjNode		*gObjectDeleteQueue[OBJ_DEL_Q_SIZE];

float		gAutoFadeStartDist,gAutoFadeEndDist,gAutoFadeRange_Frac;

int			gNumObjectNodes;


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CREATION ------

/************************ INIT OBJECT MANAGER **********************/

void InitObjectManager(void)
{

				/* INIT LINKED LIST */


	gCurrentNode = nil;

					/* CLEAR ENTIRE OBJECT LIST */

	gFirstNodePtr = nil;									// no node yet

	gNumObjectNodes = 0;
}


/*********************** MAKE NEW OBJECT ******************/
//
// MAKE NEW OBJECT & RETURN PTR TO IT
//
// The linked list is sorted from smallest to largest!
//

ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newNodePtr;
long	slot;
unsigned long flags = newObjDef->flags;

	_Static_assert(0 == (offsetof(struct ObjNode, SpecialPadding) & 7), "SpecialPadding should be aligned to 8");


				/* ALLOCATE NEW NODE(CLEARED TO 0'S) */

	newNodePtr = (ObjNode *)AllocPtrClear(sizeof(ObjNode));
	if (newNodePtr == nil)
		DoFatalAlert("MakeNewObject: Alloc Ptr failed!");

	if (newObjDef->slot <= 0)
	{
		printf("%s: Slot is %d, are you sure?\n", __func__, newObjDef->slot);
	}

	if (newObjDef->scale == 0.0f)
	{
		printf("%s: Scale is 0, are you sure?\n", __func__);
	}


			/* INITIALIZE ALL OF THE FIELDS */

	slot = newObjDef->slot;

	newNodePtr->Slot = slot;
	newNodePtr->Type = newObjDef->type;
	newNodePtr->Group = newObjDef->group;

	if (flags & STATUS_BIT_ONSPLINE)
	{
		newNodePtr->MoveCall = nil;
		newNodePtr->SplineMoveCall = newObjDef->moveCall;				// save spline move routine
	}
	else
	{
		newNodePtr->MoveCall = newObjDef->moveCall;						// save move routine
		newNodePtr->SplineMoveCall = nil;
	}

	newNodePtr->Genre = newObjDef->genre;
	newNodePtr->Coord = newNodePtr->InitCoord = newNodePtr->OldCoord = newObjDef->coord;		// save coords
	newNodePtr->StatusBits = flags;
	newNodePtr->Projection = newObjDef->projection;
	newNodePtr->CustomDrawFunction = newObjDef->drawCall;
	newNodePtr->PlayerNum = newObjDef->player;

/*	newNodePtr->Flag[0] =
	newNodePtr->Flag[1] =
	newNodePtr->Flag[2] =
	newNodePtr->Flag[3] =
	newNodePtr->Flag[4] =
	newNodePtr->Flag[5] =
	newNodePtr->Special[0] =
	newNodePtr->Special[1] =
	newNodePtr->Special[2] =
	newNodePtr->Special[3] =
	newNodePtr->Special[4] =
	newNodePtr->Special[5] =
	newNodePtr->SpecialF[0] =
	newNodePtr->SpecialF[1] =
	newNodePtr->SpecialF[2] =
	newNodePtr->SpecialF[3] =
	newNodePtr->SpecialF[4] =
	newNodePtr->SpecialF[5] =
	newNodePtr->CType =							// must init ctype to something ( INVALID_NODE_FLAG might be set from last delete)
	newNodePtr->CBits =
	newNodePtr->Delta.x =
	newNodePtr->Delta.y =
	newNodePtr->Delta.z =
	newNodePtr->Rot.x =
	newNodePtr->Rot.z = 0;
*/

	newNodePtr->Rot.y =  newObjDef->rot;
	newNodePtr->Scale.x =
	newNodePtr->Scale.y =
	newNodePtr->Scale.z = newObjDef->scale;


	newNodePtr->BoundingSphereRadius = 100;

/*
	newNodePtr->SpriteMO = nil;

	newNodePtr->DeltaRot.x =
	newNodePtr->DeltaRot.y =
	newNodePtr->DeltaRot.z = 0;

	newNodePtr->RealMotion.x =
	newNodePtr->RealMotion.y =
	newNodePtr->RealMotion.z = 0;

	newNodePtr->TopOff =
	newNodePtr->BottomOff =
	newNodePtr->LeftOff =
	newNodePtr->RightOff =
	newNodePtr->FrontOff =
	newNodePtr->BackOff = 0;

	newNodePtr->AccelVector.x =
	newNodePtr->AccelVector.y = 0;

	newNodePtr->Damage = 0;
	newNodePtr->Health = 0;
	newNodePtr->Mode = 0;

	newNodePtr->OldSpeed2D =
	newNodePtr->Speed2D =
	newNodePtr->Speed3D = 0;

	newNodePtr->ChainNode = nil;
	newNodePtr->ChainHead = nil;
	newNodePtr->BaseGroup = nil;						// nothing attached as QD3D object yet
	newNodePtr->ShadowNode = nil;
	newNodePtr->BaseTransformObject = nil;
	newNodePtr->NumCollisionBoxes = 0;
	newNodePtr->CollisionBoxes = nil;					// no collision boxes yet
*/


	newNodePtr->EffectChannel = -1;						// no streaming sound effect
	newNodePtr->ParticleGroup = -1;						// no particle group
	newNodePtr->ParticleMagicNum = 0;

	newNodePtr->TerrainItemPtr = nil;					// assume not a terrain item
	newNodePtr->SplineItemPtr = nil;					// assume not a spline item
	newNodePtr->SplineNum = 0;
	newNodePtr->SplineObjectIndex = -1;					// no index yet
	newNodePtr->SplinePlacement = 0;

	newNodePtr->Skeleton = nil;

	newNodePtr->ColorFilter.r =
	newNodePtr->ColorFilter.g =
	newNodePtr->ColorFilter.b =
	newNodePtr->ColorFilter.a = 1.0;

			/* MAKE SURE SCALE != 0 */

	if (newNodePtr->Scale.x == 0.0f)
		newNodePtr->Scale.x = 0.0001f;
	if (newNodePtr->Scale.y == 0.0f)
		newNodePtr->Scale.y = 0.0001f;
	if (newNodePtr->Scale.z == 0.0f)
		newNodePtr->Scale.z = 0.0001f;


				/* INSERT NODE INTO LINKED LIST */

	newNodePtr->StatusBits |= STATUS_BIT_DETACHED;		// its not attached to linked list yet
	AttachObject(newNodePtr);

	gNumObjectNodes++;


				/* CLEANUP */

	gMostRecentlyAddedNode = newNodePtr;					// remember this
	return(newNodePtr);
}

/************* MAKE NEW DISPLAY GROUP OBJECT *************/
//
// This is an ObjNode who's BaseGroup is a group, therefore these objects
// can be transformed (scale,rot,trans).
//

ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newObj;
Byte	group,type;


	newObjDef->genre = DISPLAY_GROUP_GENRE;

	newObj = MakeNewObject(newObjDef);
	if (newObj == nil)
		return(nil);

			/* MAKE BASE GROUP & ADD GEOMETRY TO IT */

	CreateBaseGroup(newObj);											// create group object
	group = newObjDef->group;											// get group #
	type = newObjDef->type;												// get type #

	if (type >= gNumObjectsInBG3DGroupList[group])							// see if illegal
	{
		DoFatalAlert("MakeNewDisplayGroupObject: type > gNumObjectsInGroupList[]! group=%d, type=%d", group, type);
	}

	AttachGeometryToDisplayGroupObject(newObj,gBG3DGroupList[group][type]);

	return(newObj);
}


/******************* RESET DISPLAY GROUP OBJECT *********************/
//
// If the ObjNode's "Type" field has changed, call this to dispose of
// the old BaseGroup and create a new one with the correct model attached.
//

void ResetDisplayGroupObject(ObjNode *theNode)
{
	DisposeObjectBaseGroup(theNode);									// dispose of old group
	CreateBaseGroup(theNode);											// create new group object

	if (theNode->Type >= gNumObjectsInBG3DGroupList[theNode->Group])							// see if illegal
		DoFatalAlert("ResetDisplayGroupObject: type > gNumObjectsInGroupList[]!");

	AttachGeometryToDisplayGroupObject(theNode,gBG3DGroupList[theNode->Group][theNode->Type]);	// attach geometry to group

}



/************************* ADD GEOMETRY TO DISPLAY GROUP OBJECT ***********************/
//
// Attaches a geometry object to the BaseGroup object. MakeNewDisplayGroupObject must have already been
// called which made the group & transforms.
//

void AttachGeometryToDisplayGroupObject(ObjNode *theNode, MetaObjectPtr geometry)
{
	MO_AppendToGroup(theNode->BaseGroup, geometry);
}



/***************** CREATE BASE GROUP **********************/
//
// The base group contains the base transform matrix plus any other objects you want to add into it.
// This routine creates the new group and then adds a transform matrix.
//
// The base is composed of BaseGroup & BaseTransformObject.
//

void CreateBaseGroup(ObjNode *theNode)
{
OGLMatrix4x4			transMatrix,scaleMatrix,rotMatrix;
MOMatrixObject			*transObject;


				/* CREATE THE BASE GROUP OBJECT */

	theNode->BaseGroup = MO_CreateNewObjectOfType(MO_TYPE_GROUP, 0, nil);
	if (theNode->BaseGroup == nil)
		DoFatalAlert("CreateBaseGroup: MO_CreateNewObjectOfType failed!");


					/* SETUP BASE MATRIX */

	if ((theNode->Scale.x == 0) || (theNode->Scale.y == 0) || (theNode->Scale.z == 0))
		DoFatalAlert("CreateBaseGroup: A scale component == 0");


	OGLMatrix4x4_SetScale(&scaleMatrix, theNode->Scale.x, theNode->Scale.y,		// make scale matrix
							theNode->Scale.z);

	OGLMatrix4x4_SetRotate_XYZ(&rotMatrix, theNode->Rot.x, theNode->Rot.y,		// make rotation matrix
								 theNode->Rot.z);

	OGLMatrix4x4_SetTranslate(&transMatrix, theNode->Coord.x, theNode->Coord.y,	// make translate matrix
							 theNode->Coord.z);

	OGLMatrix4x4_Multiply(&scaleMatrix,											// mult scale & rot matrices
						 &rotMatrix,
						 &theNode->BaseTransformMatrix);

	OGLMatrix4x4_Multiply(&theNode->BaseTransformMatrix,						// mult by trans matrix
						 &transMatrix,
						 &theNode->BaseTransformMatrix);


					/* CREATE A MATRIX XFORM */

	transObject = MO_CreateNewObjectOfType(MO_TYPE_MATRIX, 0, &theNode->BaseTransformMatrix);	// make matrix xform object
	if (transObject == nil)
		DoFatalAlert("CreateBaseGroup: MO_CreateNewObjectOfType/Matrix Failed!");

	MO_AttachToGroupStart(theNode->BaseGroup, transObject);						// add to base group
	theNode->BaseTransformObject = transObject;									// keep extra LEGAL ref (remember to dispose later)
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT PROCESSING ------


/*******************************  MOVE OBJECTS **************************/

void MoveObjects(void)
{
ObjNode		*thisNodePtr;

	if (gFirstNodePtr == nil)								// see if there are any objects
		return;

	thisNodePtr = gFirstNodePtr;

	do
	{
		gCurrentNode = thisNodePtr;							// set current object node
		gNextNode	 = thisNodePtr->NextNode;				// get next node now (cuz current node might get deleted)


		if (gGamePaused && !(thisNodePtr->StatusBits & STATUS_BIT_MOVEINPAUSE))
			goto next;



		KeepOldCollisionBoxes(thisNodePtr);					// keep old boxes & other stuff


				/* UPDATE ANIMATION */

		if (thisNodePtr->Skeleton != nil)
			UpdateSkeletonAnimation(thisNodePtr);


					/* NEXT TRY TO MOVE IT */

		if ((!(thisNodePtr->StatusBits & STATUS_BIT_NOMOVE)) &&	(thisNodePtr->MoveCall != nil))
		{
			thisNodePtr->MoveCall(thisNodePtr);				// call object's move routine
		}

next:
		thisNodePtr = gNextNode;							// next node
	}
	while (thisNodePtr != nil);



			/* CALL SOUND MAINTENANCE HERE FOR CONVENIENCE */

	DoSoundMaintenance();

			/* FLUSH THE DELETE QUEUE */

	FlushObjectDeleteQueue();
}



/**************************** DRAW OBJECTS ***************************/

void DrawObjects(void)
{
ObjNode		*theNode;
short		i,numTriMeshes;
unsigned long	statusBits;
Boolean			noLighting = false;
Boolean			noZBuffer = false;
Boolean			noZWrites = false;
//Boolean			noFog = false;
Boolean			glow = false;
#if ALLOW_GL_CLIP_HINTS
Boolean			noClipTest = false;
#endif
Boolean			noCullFaces = false;
Boolean			texWrap = false;
#if DO_EDGE_ALPHA_CLIPPING
Boolean			clipAlpha= false;
#endif
float			cameraX, cameraZ;
short			skelType, playerNum;

	if (gFirstNodePtr == nil)									// see if there are any objects
		return;

	playerNum = GetPlayerNum(gCurrentSplitScreenPane);			// get the player # who's draw context is being drawn


				/* FIRST DO OUR CULLING */

	CullTestAllObjects();

	theNode = gFirstNodePtr;


			/* GET CAMERA COORDS */

	cameraX = gGameView->cameraPlacement[gCurrentSplitScreenPane].cameraLocation.x;
	cameraZ = gGameView->cameraPlacement[gCurrentSplitScreenPane].cameraLocation.z;

			/***********************/
			/* MAIN NODE TASK LOOP */
			/***********************/
	do
	{
		statusBits = theNode->StatusBits;						// get obj's status bits

		if (statusBits & (STATUS_BIT_ISCULLED|STATUS_BIT_HIDDEN))	// see if is culled or hidden
			goto next;

		if (statusBits & STATUS_BIT_ONLYSHOWTHISPLAYER)			// see if only show for current player's draw context
		{
			if (theNode->PlayerNum != playerNum)
				goto next;
		}
		else
		if (statusBits & STATUS_BIT_NOSHOWTHISPLAYER)			// see if dont show for current player's draw context
		{
			if (theNode->PlayerNum == playerNum)
				goto next;
		}

		if (!gDrawingOverlayPane)
		{
			if (statusBits & STATUS_BIT_OVERLAYPANE)
				goto next;
		}
		else
		{
			if (!(statusBits & STATUS_BIT_OVERLAYPANE))
				goto next;
		}

		if (gGamePaused || gHideInfobar)
		{
			if (statusBits & STATUS_BIT_HIDEINPAUSE)
				goto next;
		}

		if (theNode->CType == INVALID_NODE_FLAG)				// see if already deleted
			goto next;

		gGlobalTransparency = theNode->ColorFilter.a;			// get global transparency
		if (gGlobalTransparency <= 0.0f)						// see if invisible
			goto next;

		gGlobalColorFilter.r = theNode->ColorFilter.r;			// set color filter
		gGlobalColorFilter.g = theNode->ColorFilter.g;
		gGlobalColorFilter.b = theNode->ColorFilter.b;




			/******************/
			/* CHECK AUTOFADE */
			/******************/

		if (gAutoFadeStartDist != 0.0f)							// see if this level has autofade
		{
			if (statusBits & STATUS_BIT_AUTOFADE)
			{
				float		dist;

				dist = CalcQuickDistance(cameraX, cameraZ, theNode->Coord.x, theNode->Coord.z);			// see if in fade zone
				if (dist >= gAutoFadeStartDist)
				{
					dist -= gAutoFadeStartDist;							// calc xparency %
					dist *= gAutoFadeRange_Frac;
					if (dist < 0.0f)
						goto next;

					gGlobalTransparency -= dist;
					if (gGlobalTransparency <= 0.0f)
						goto next;
				}
			}
		}


			/*******************/
			/* CHECK BACKFACES */
			/*******************/

		if (statusBits & STATUS_BIT_KEEPBACKFACES)
		{
			if (!noCullFaces)
			{
				glDisable(GL_CULL_FACE);
				noCullFaces = true;
			}
		}
		else
		if (noCullFaces)
		{
			noCullFaces = false;
			glEnable(GL_CULL_FACE);
		}




			/*********************/
			/* CHECK NULL SHADER */
			/*********************/

		if (statusBits & STATUS_BIT_NOLIGHTING)
		{
			if (!noLighting)
			{
				OGL_DisableLighting();
				noLighting = true;
			}
		}
		else
		if (noLighting)
		{
			noLighting = false;
			OGL_EnableLighting();
		}

 #if 0
 			/****************/
			/* CHECK NO FOG */
			/****************/

		if (setupInfo->useFog)
		{
			if (statusBits & STATUS_BIT_NOFOG)
			{
				if (!noFog)
				{
					glDisable(GL_FOG);
					noFog = true;
				}
			}
			else
			if (noFog)
			{
				noFog = false;
				glEnable(GL_FOG);
			}
		}
#endif
			/********************/
			/* CHECK GLOW BLEND */
			/********************/

		if (statusBits & STATUS_BIT_GLOW)
		{
			if (!glow)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glow = true;
			}
		}
		else
		if (glow)
		{
			glow = false;
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}


			/**********************/
			/* CHECK TEXTURE WRAP */
			/**********************/

		if (statusBits & STATUS_BIT_NOTEXTUREWRAP)
		{
			if (!texWrap)
			{
				gGlobalMaterialFlags |= BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V;
				texWrap = true;
			}
		}
		else
		if (texWrap)
		{
			texWrap = false;
		    gGlobalMaterialFlags &= ~(BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);
		}




			/****************/
			/* CHECK ZWRITE */
			/****************/

		if (statusBits & STATUS_BIT_NOZWRITES)
		{
			if (!noZWrites)
			{
				glDepthMask(GL_FALSE);
				noZWrites = true;
			}
		}
		else
		if (noZWrites)
		{
			glDepthMask(GL_TRUE);
			noZWrites = false;
		}


			/*****************/
			/* CHECK ZBUFFER */
			/*****************/

		if (statusBits & STATUS_BIT_NOZBUFFER)
		{
			if (!noZBuffer)
			{
				glDisable(GL_DEPTH_TEST);
				noZBuffer = true;
			}
		}
		else
		if (noZBuffer)
		{
			noZBuffer = false;
			glEnable(GL_DEPTH_TEST);
		}


#if DO_EDGE_ALPHA_CLIPPING
			/*****************************/
			/* CHECK EDGE ALPHA CLIPPING */
			/*****************************/

		if ((statusBits & STATUS_BIT_CLIPALPHA) && (gGlobalTransparency == 1.0f))
		{
			if (!clipAlpha)
			{
				glAlphaFunc(GL_EQUAL, 1);	// draw any pixel who's Alpha == 1, skip semi-transparent pixels
				clipAlpha = true;
			}
		}
		else
		if (clipAlpha)
		{
			clipAlpha = false;
			glAlphaFunc(GL_NOTEQUAL, 0);	// draw any pixel who's Alpha != 0
		}
#endif


#if ALLOW_GL_CLIP_HINTS
			/******************/
			/* CHECK CLIPTEST */
			/******************/

		if (statusBits & STATUS_BIT_NOCLIPTEST)
		{
			if (!noClipTest)
			{
				glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_FASTEST);
				noClipTest = true;
			}
		}
		else
		if (noClipTest)
		{
			noClipTest = false;
			glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT,  GL_NICEST);
		}
#endif

			/* AIM AT CAMERA */

		if (statusBits & STATUS_BIT_AIMATCAMERA)
		{
			theNode->Rot.y = PI+CalcYAngleFromPointToPoint(theNode->Rot.y,
														theNode->Coord.x, theNode->Coord.z,
														cameraX, cameraZ);

			UpdateObjectTransforms(theNode);

		}



			/************************/
			/* SHOW COLLISION BOXES */
			/************************/

		if (gDebugMode == 2)
		{
			DrawCollisionBoxes(theNode,false);
			if (theNode->CType & CTYPE_PLAYER)			// also draw path vectors
			{
				DrawPathVec(theNode->PlayerNum);
			}
		}

			/***************************/
			/* SET 2D OR 3D PROJECTION */
			/***************************/

		if (theNode->Projection != gMyState_ProjectionType)
		{
			OGL_SetProjection(theNode->Projection);
			GAME_ASSERT(theNode->Projection == gMyState_ProjectionType);
		}


			/***********************/
			/* SUBMIT THE GEOMETRY */
			/***********************/


 		if (noLighting || (theNode->Scale.y == 1.0f))				// if scale == 1 or no lighting, then dont need to normalize vectors
 			glDisable(GL_NORMALIZE);
 		else
 			glEnable(GL_NORMALIZE);

		switch(theNode->Genre)
		{
			MOMaterialObject	*overrideTexture;

			case	SKELETON_GENRE:
					UpdateSkinnedGeometry(theNode);													// update skeleton geometry
					numTriMeshes = theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
					skelType = theNode->Type;

					overrideTexture = theNode->Skeleton->overrideTexture;							// get any override texture ref (illegal ref)

					for (i = 0; i < numTriMeshes; i++)												// submit each trimesh of it
					{
						if (overrideTexture)														// set override texture
						{
							if (gLocalTriMeshesOfSkelType[skelType][i].numMaterials > 0)
							{
								MO_DisposeObjectReference(gLocalTriMeshesOfSkelType[skelType][i].materials[0]);				// de-reference the existing texture
								gLocalTriMeshesOfSkelType[skelType][i].materials[0] = MO_GetNewReference(overrideTexture);	// put new ref to override in there
							}
						}

						MO_DrawGeometry_VertexArray(&gLocalTriMeshesOfSkelType[skelType][i]);
					}
					break;

			case	DISPLAY_GROUP_GENRE:
			case	QUADMESH_GENRE:
					if (theNode->BaseGroup)
					{
						MO_DrawObject(theNode->BaseGroup);
					}
					break;

			case	TEXTMESH_GENRE:
					if (theNode->BaseGroup)
					{
						MO_DrawObject(theNode->BaseGroup);

						if (gDebugMode >= 2)
						{
							TextMesh_DrawExtents(theNode);
						}
					}
					break;

			case	SPRITE_GENRE:
			{
					GAME_ASSERT(theNode->Type > 0 && theNode->Type <= 255);

					char text[2] = { (char) theNode->Type, 0 };
					int spriteFlags = kTextMeshAlignCenter | kTextMeshAlignMiddle | kTextMeshKeepCurrentProjection;

					OGL_PushState();
					glMultMatrixf(theNode->BaseTransformMatrix.value);
					Atlas_ImmediateDraw(theNode->Group, text, spriteFlags);
					OGL_PopState();
					break;
			}

			case	CUSTOM_GENRE:
					if (theNode->CustomDrawFunction)
					{
						theNode->CustomDrawFunction(theNode);
					}
					break;
		}


			/* NEXT NODE */
next:
		theNode = (ObjNode *)theNode->NextNode;
	}while (theNode != nil);


				/*****************************/
				/* RESET SETTINGS TO DEFAULT */
				/*****************************/

	if (noLighting)
		OGL_EnableLighting();

#if 0
	if (setupInfo->useFog)
	{
		if (noFog)
			glEnable(GL_FOG);
	}
#endif

	if (glow)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (noZBuffer)
	{
		glEnable(GL_DEPTH_TEST);
//		glDepthMask(GL_TRUE);
	}

	if (noZWrites)
		glDepthMask(GL_TRUE);

	if (noCullFaces)
		glEnable(GL_CULL_FACE);

	if (texWrap)
	    gGlobalMaterialFlags &= ~(BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);

#if DO_EDGE_ALPHA_CLIPPING
	if (clipAlpha)
		glAlphaFunc(GL_NOTEQUAL, 0);
#endif


#if ALLOW_GL_CLIP_HINTS
	if (noClipTest)
		glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT,  GL_NICEST);
#endif


	gGlobalTransparency = 			// reset this in case it has changed
	gGlobalColorFilter.r =
	gGlobalColorFilter.g =
	gGlobalColorFilter.b = 1.0;

	glEnable(GL_NORMALIZE);
}


/************************ DRAW COLLISION BOXES ****************************/

static void DrawCollisionBoxes(ObjNode *theNode, Boolean old)
{
int					n,i;
CollisionBoxType	*c;
float				left,right,top,bottom,front,back;

			/* SET LINE MATERIAL */

	glDisable(GL_TEXTURE_2D);


		/* SCAN EACH COLLISION BOX */

	n = theNode->NumCollisionBoxes;							// get # collision boxes
	c = theNode->CollisionBoxes;							// pt to array
	if (c == nil)
		return;

	for (i = 0; i < n; i++)
	{
			/* GET BOX PARAMS */

		if (old)
		{
			left 	= c[i].oldLeft;
			right 	= c[i].oldRight;
			top 	= c[i].oldTop;
			bottom 	= c[i].oldBottom;
			front 	= c[i].oldFront;
			back 	= c[i].oldBack;
		}
		else
		{
			left 	= c[i].left;
			right 	= c[i].right;
			top 	= c[i].top;
			bottom 	= c[i].bottom;
			front 	= c[i].front;
			back 	= c[i].back;
		}

			/* DRAW TOP */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, top, back);
		glColor3f(1,1,0);
		glVertex3f(left, top, front);
		glVertex3f(right, top, front);
		glColor3f(1,0,0);
		glVertex3f(right, top, back);
		glEnd();

			/* DRAW BOTTOM */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(left, bottom, front);
		glVertex3f(right, bottom, front);
		glColor3f(1,0,0);
		glVertex3f(right, bottom, back);
		glEnd();


			/* DRAW LEFT */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, top, back);
		glColor3f(1,0,0);
		glVertex3f(left, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(left, bottom, front);
		glVertex3f(left, top, front);
		glEnd();


			/* DRAW RIGHT */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(right, top, back);
		glVertex3f(right, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(right, bottom, front);
		glVertex3f(right, top, front);
		glEnd();

	}
}


/********************* DRAW PATH VEC *************************/

static void DrawPathVec(short p)
{
float	x,y,z;

			/* SET LINE MATERIAL */

	glDisable(GL_TEXTURE_2D);
	glColor3f(1,1,0);


	x = gPlayerInfo[p].coord.x;
	y = gPlayerInfo[p].coord.y + 200.0f;
	z = gPlayerInfo[p].coord.z;

	glBegin(GL_LINES);
	glVertex3f(x, y, z);

	x += gPlayerInfo[p].pathVec.x * 200.0f;
	z += gPlayerInfo[p].pathVec.y * 200.0f;
	glVertex3f(x, y, z);
	glEnd();


}



/********************* MOVE STATIC OBJECT **********************/

void MoveStaticObject(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	UpdateShadow(theNode);										// prime it
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT DELETING ------


/******************** DELETE ALL OBJECTS ********************/

void DeleteAllObjects(void)
{
	while (gFirstNodePtr != nil)
		DeleteObject(gFirstNodePtr);

	FlushObjectDeleteQueue();
}


/************************ DELETE OBJECT ****************/

void DeleteObject(ObjNode	*theNode)
{
	if (theNode == nil)								// see if passed a bogus node
		return;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
	{
		DoFatalAlert("Attempted to Double Delete an Object.  Object was already deleted! genre=%d group=%d type=%d", theNode->Genre, theNode->Group, theNode->Type);
	}

			/* CALL CUSTOM DESTRUCTOR */

	if (theNode->Destructor)
		theNode->Destructor(theNode);

			/* RECURSIVE DELETE OF CHAIN NODE & SHADOW NODE */
			//
			// should do these first so that base node will have appropriate nextnode ptr
			// since it's going to be used next pass thru the moveobjects loop.  This
			// assumes that all chained nodes are later in list.
			//

	if (theNode->ChainNode)
		DeleteObject(theNode->ChainNode);

	if (theNode->ShadowNode)
		DeleteObject(theNode->ShadowNode);

	if (theNode->TwitchNode)
	{
		ObjNode* twitchNode = theNode->TwitchNode;
		theNode->TwitchNode = NULL;					// detach from TwitchNode
		DeleteObject(twitchNode);
	}


			/* SEE IF NEED TO FREE UP SPECIAL MEMORY */

	switch(theNode->Genre)
	{
		case	SKELETON_GENRE:
				FreeSkeletonBaseData(theNode->Skeleton);		// purge all alloced memory for skeleton data
				theNode->Skeleton = nil;
				break;
	}

	if (theNode->CollisionBoxes != nil)				// free collision box memory
	{
		SafeDisposePtr((Ptr)theNode->CollisionBoxes);
		theNode->CollisionBoxes = nil;
	}


			/* SEE IF STOP SOUND CHANNEL */

	StopObjectStreamEffect(theNode);


		/* SEE IF NEED TO DEREFERENCE A QD3D OBJECT */

	DisposeObjectBaseGroup(theNode);


			/* REMOVE NODE FROM LINKED LIST */

	DetachObject(theNode);


			/* SEE IF MARK AS NOT-IN-USE IN ITEM LIST */

	if (theNode->TerrainItemPtr)
	{
		theNode->TerrainItemPtr->flags &= ~ITEM_FLAGS_INUSE;		// clear the "in use" flag
	}

		/* OR, IF ITS A SPLINE ITEM, THEN UPDATE SPLINE OBJECT LIST */

	if (theNode->StatusBits & STATUS_BIT_ONSPLINE)
	{
		RemoveFromSplineObjectList(theNode);
	}


			/* DELETE THE NODE BY ADDING TO DELETE QUEUE */

	theNode->CType = INVALID_NODE_FLAG;							// INVALID_NODE_FLAG indicates its deleted


	gObjectDeleteQueue[gNumObjsInDeleteQueue++] = theNode;

	if (gNumObjsInDeleteQueue >= OBJ_DEL_Q_SIZE)				// object delete queue full -- OK inbetween scenes but shouldn't happen in-game!
		FlushObjectDeleteQueue();

}


/****************** DETACH OBJECT ***************************/
//
// Simply detaches the objnode from the linked list, patches the links
// and keeps the original objnode intact.
//

void DetachObject(ObjNode *theNode)
{
	if (theNode == nil)
		return;

	if (theNode->StatusBits & STATUS_BIT_DETACHED)	// make sure not already detached
		return;

	if (theNode == gNextNode)						// if its the next node to be moved, then fix things
		gNextNode = theNode->NextNode;

	if (theNode->PrevNode == nil)					// special case 1st node
	{
		gFirstNodePtr = theNode->NextNode;
		if (gFirstNodePtr)
			gFirstNodePtr->PrevNode = nil;
	}
	else
	if (theNode->NextNode == nil)					// special case last node
	{
		theNode->PrevNode->NextNode = nil;
	}
	else											// generic middle deletion
	{
		theNode->PrevNode->NextNode = theNode->NextNode;
		theNode->NextNode->PrevNode = theNode->PrevNode;
	}

	theNode->PrevNode = nil;						// seal links on original node
	theNode->NextNode = nil;

	theNode->StatusBits |= STATUS_BIT_DETACHED;




			/* UNCHAIN NODE */

	UnchainNode(theNode);
}


/****************** ATTACH OBJECT ***************************/

void AttachObject(ObjNode *theNode)
{
short	slot;

	if (theNode == nil)
		return;

	if (!(theNode->StatusBits & STATUS_BIT_DETACHED))		// see if already attached
		return;

	slot = theNode->Slot;

	if (gFirstNodePtr == nil)						// special case only entry
	{
		gFirstNodePtr = theNode;
		theNode->PrevNode = nil;
		theNode->NextNode = nil;
	}

			/* INSERT AS FIRST NODE */
	else
	if (slot < gFirstNodePtr->Slot)
	{
		theNode->PrevNode = nil;					// no prev
		theNode->NextNode = gFirstNodePtr; 			// next pts to old 1st
		gFirstNodePtr->PrevNode = theNode; 			// old pts to new 1st
		gFirstNodePtr = theNode;
	}
		/* SCAN FOR INSERTION PLACE */
	else
	{
		ObjNode	 *reNodePtr, *scanNodePtr;

		reNodePtr = gFirstNodePtr;
		scanNodePtr = gFirstNodePtr->NextNode;		// start scanning for insertion slot on 2nd node

		while (scanNodePtr != nil)
		{
			if (slot < scanNodePtr->Slot)					// INSERT IN MIDDLE HERE
			{
				theNode->NextNode = scanNodePtr;
				theNode->PrevNode = reNodePtr;
				reNodePtr->NextNode = theNode;
				scanNodePtr->PrevNode = theNode;
				goto out;
			}
			reNodePtr = scanNodePtr;
			scanNodePtr = scanNodePtr->NextNode;			// try next node
		}

		theNode->NextNode = nil;							// TAG TO END
		theNode->PrevNode = reNodePtr;
		reNodePtr->NextNode = theNode;
	}
out:


	theNode->StatusBits &= ~STATUS_BIT_DETACHED;
}


/***************** IS OBJECT BEING DELETED ****************/

Boolean IsObjectBeingDeleted(ObjNode* theNode)
{
	return theNode->CType == INVALID_NODE_FLAG;
}


/***************** FLUSH OBJECT DELETE QUEUE ****************/

static void FlushObjectDeleteQueue(void)
{
long	i,num;

	num = gNumObjsInDeleteQueue;

	gNumObjectNodes -= num;


	for (i = 0; i < num; i++)
		SafeDisposePtr((Ptr)gObjectDeleteQueue[i]);

	gNumObjsInDeleteQueue = 0;
}


/****************** DISPOSE OBJECT BASE GROUP **********************/

void DisposeObjectBaseGroup(ObjNode *theNode)
{
	if (theNode->BaseGroup != nil)
	{
		MO_DisposeObjectReference(theNode->BaseGroup);

		theNode->BaseGroup = nil;
	}

	if (theNode->BaseTransformObject != nil)							// also nuke extra ref to transform object
	{
		MO_DisposeObjectReference(theNode->BaseTransformObject);
		theNode->BaseTransformObject = nil;
	}
}




//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT INFORMATION ------


/********************** GET OBJECT INFO *********************/

void GetObjectInfo(ObjNode *theNode)
{
	gCoord = theNode->Coord;
	gDelta = theNode->Delta;
}


/********************** UPDATE OBJECT *********************/

void UpdateObject(ObjNode *theNode)
{
	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;

	theNode->Coord = gCoord;
	theNode->Delta = gDelta;
	UpdateObjectTransforms(theNode);
	if (theNode->CollisionBoxes)
		CalcObjectBoxFromNode(theNode);


		/* UPDATE ANY SHADOWS */

	UpdateShadow(theNode);
}



/****************** UPDATE OBJECT TRANSFORMS *****************/
//
// This updates the skeleton object's base translate & rotate transforms
//

void UpdateObjectTransforms(ObjNode *theNode)
{
OGLMatrix4x4	m,m2;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;

				/********************/
				/* SET SCALE MATRIX */
				/********************/

	OGLMatrix4x4_SetScale(&m, theNode->Scale.x,	theNode->Scale.y, theNode->Scale.z);


			/*****************************/
			/* NOW ROTATE & TRANSLATE IT */
			/*****************************/

				/* DO XZY ROTATION */

	if (theNode->StatusBits & STATUS_BIT_ROTXZY)
	{
		OGLMatrix4x4	mx,my,mz,mxz;

		OGLMatrix4x4_SetRotate_X(&mx, theNode->Rot.x);
		OGLMatrix4x4_SetRotate_Y(&my, theNode->Rot.y);
		OGLMatrix4x4_SetRotate_Z(&mz, theNode->Rot.z);

		OGLMatrix4x4_Multiply(&mx,&mz, &mxz);
		OGLMatrix4x4_Multiply(&mxz,&my, &m2);
	}
				/* STANDARD XYZ ROTATION */
	else
	{
		OGLMatrix4x4_SetRotate_XYZ(&m2, theNode->Rot.x, theNode->Rot.y, theNode->Rot.z);
	}



	m2.value[M03] = theNode->Coord.x;
	m2.value[M13] = theNode->Coord.y;
	m2.value[M23] = theNode->Coord.z;

	OGLMatrix4x4_Multiply(&m,&m2, &theNode->BaseTransformMatrix);


				/* UPDATE TRANSFORM OBJECT */

	SetObjectTransformMatrix(theNode);
}


/***************** SET OBJECT TRANSFORM MATRIX *******************/
//
// This call simply resets the base transform object so that it uses the latest
// base transform matrix
//

void SetObjectTransformMatrix(ObjNode *theNode)
{
MOMatrixObject	*mo = theNode->BaseTransformObject;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if invalid
		return;

	if (mo)				// see if this has a trans obj
	{
		mo->matrix =  theNode->BaseTransformMatrix;
	}
}


/***************** TOGGLE OBJECT VISIBILITY *******************/

Boolean SetObjectVisible(ObjNode* theNode, bool visible)
{
	if (visible)
	{
		theNode->StatusBits &= ~STATUS_BIT_HIDDEN;
	}
	else
	{
		theNode->StatusBits |= STATUS_BIT_HIDDEN;
	}

	return visible;
}

int GetNodeChainLength(ObjNode* node)
{
	for (int length = 0; length <= 0x7FFF; length++)
	{
		if (!node)
			return length;

		node = node->ChainNode;
	}

	DoFatalAlert("Circular chain?");
}

ObjNode* GetNthChainedNode(ObjNode* start, int targetIndex, ObjNode** outPrevNode)
{
	ObjNode* pNode = NULL;
	ObjNode* node = start;

	if (start != NULL)
	{
		int currentIndex;
		for (currentIndex = 0; (node != NULL) && (currentIndex < targetIndex); currentIndex++)
		{
			pNode = node;
			node = pNode->ChainNode;

			if (pNode && node)
			{
				GAME_ASSERT_MESSAGE(pNode->Slot <= node->Slot, "the game assumes that chained nodes are sorted after their parent");
			}
		}

		if (currentIndex != targetIndex)
		{
			node = NULL;
		}
	}

	if (outPrevNode)
	{
		*outPrevNode = pNode;
	}

	return node;
}

ObjNode* GetChainTailNode(ObjNode* start)
{
	ObjNode* pNode = NULL;
	ObjNode* node = start;

	while (node != NULL)
	{
		pNode = node;
		node = pNode->ChainNode;

		if (node)
			GAME_ASSERT(node->Slot > pNode->Slot);
	}

	return pNode;
}

void AppendNodeToChain(ObjNode* first, ObjNode* newTail)
{
	ObjNode* oldTail = GetChainTailNode(first);
	oldTail->ChainNode = newTail;

	newTail->ChainHead = first->ChainHead;
	if (!newTail->ChainHead)
		newTail->ChainHead = first;

	GAME_ASSERT(newTail->Slot > oldTail->Slot);
}

void UnchainNode(ObjNode* theNode)
{
	if (theNode == NULL)
		return;

	ObjNode* chainHead = theNode->ChainHead;
	ObjNode* chainNext = theNode->ChainNode;

	for (ObjNode* node = chainHead; node != NULL; node = node->ChainNode)
	{
		if (node == theNode)
			continue;

		if (node->ChainNode == theNode)
		{
			node->ChainNode = chainNext;
		}

		if (node->ChainHead == theNode)		// rewrite ChainHead
		{
			node->ChainHead = chainNext;
		}
	}
}
