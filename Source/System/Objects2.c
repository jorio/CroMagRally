/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)1993-1999 Pangea Software  */
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "globals.h"
#include "objects.h"
#include "misc.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "ogl_support.h"
#include "3dmath.h"
#include "camera.h"
#include "mobjtypes.h"
#include "bones.h"
#include "collision.h"
#include "player.h"
#include "bg3d.h"
#include "terrain.h"
#include "sprites.h"
#include "sobjtypes.h"
#include "file.h"
#include "sound2.h"

extern	OGLBoundingBox	gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	float		gFramesPerSecondFrac;
extern	ObjNode		*gFirstNodePtr;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLPoint3D	gCoord;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	uint32_t		gAutoFadeStatusBits;
extern	PrefsType	gGamePrefs;
extern	FSSpec		gDataSpec;
extern	OGLMatrix4x4			gViewToFrustumMatrix,gLocalToViewMatrix,gLocalToFrustumMatrix;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void FlushObjectDeleteQueue(void);
static void DrawShadow(ObjNode *theNode, OGLSetupOutputType *setupInfo);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SHADOW_Y_OFF	6.0f

/**********************/
/*     VARIABLES      */
/**********************/

#define	CheckForBlockers	Flag[0]


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT COLLISION ------

/***************** CREATE COLLISION BOX FROM BOUNDING BOX ********************/

void CreateCollisionBoxFromBoundingBox(ObjNode *theNode, float tweakXZ, float tweakY)
{
OGLBoundingBox	*bBox;
float			sx,sy,sz;

	AllocateCollisionBoxMemory(theNode, 1);

			/* POINT TO BOUNDING BOX */

	if (theNode->Genre == SKELETON_GENRE)
		bBox =  &gObjectGroupBBoxList[MODEL_GROUP_SKELETONBASE+theNode->Type][0];	// (skeleton type is actually the Group #, and only 1 model in there, so 0)
	else
		bBox =  &gObjectGroupBBoxList[theNode->Group][theNode->Type];				// get ptr to this model's local bounding box


			/* CONVERT TO COLLISON BOX */

	sx = theNode->Scale.x * tweakXZ;
	sy = theNode->Scale.y;
	sz = theNode->Scale.z * tweakXZ;

	theNode->LeftOff 	= bBox->min.x * sx;
	theNode->RightOff 	= bBox->max.x * sx;
	theNode->FrontOff 	= bBox->max.z * sz;
	theNode->BackOff 	= bBox->min.z * sz;
	theNode->TopOff 	= bBox->max.y * sy * tweakY;
	theNode->BottomOff 	= bBox->min.y * sy;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}


/***************** CREATE COLLISION BOX FROM BOUNDING BOX MAXIMIZED ********************/
//
// Same as above except it expands the x/z box to the max of x or z so object can rotate without problems.
//

void CreateCollisionBoxFromBoundingBox_Maximized(ObjNode *theNode)
{
OGLBoundingBox	*bBox;
float			sx,sy,sz;
float			maxSide,off;

	AllocateCollisionBoxMemory(theNode, 1);

			/* POINT TO BOUNDING BOX */

	if (theNode->Genre == SKELETON_GENRE)
		bBox =  &gObjectGroupBBoxList[MODEL_GROUP_SKELETONBASE+theNode->Type][0];	// (skeleton type is actually the Group #, and only 1 model in there, so 0)
	else
		bBox =  &gObjectGroupBBoxList[theNode->Group][theNode->Type];				// get ptr to this model's local bounding box

			/* DETERMINE LARGEST SIDE */

	sx = theNode->Scale.x;
	sy = theNode->Scale.y;
	sz = theNode->Scale.z;

	maxSide = fabs(bBox->min.x);
	off = fabs(bBox->max.x);
	if (off > maxSide)
		maxSide = off;
	off = fabs(bBox->max.z);
	if (off > maxSide)
		maxSide = off;
	off = fabs(bBox->min.z);
	if (off > maxSide)
		maxSide = off;


			/* CONVERT TO COLLISON BOX */

	theNode->LeftOff 	= -(maxSide * sx);
	theNode->RightOff 	= maxSide * sx;
	theNode->FrontOff 	= maxSide * sz;
	theNode->BackOff 	= -(maxSide * sz);
	theNode->TopOff 	= bBox->max.y * sy;
	theNode->BottomOff 	= bBox->min.y * sy;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}



/***************** CREATE COLLISION BOX FROM BOUNDING BOX ROTATED ********************/

void CreateCollisionBoxFromBoundingBox_Rotated(ObjNode *theNode, float tweakXZ, float tweakY)
{
OGLBoundingBox	*bBox;
float			s;
OGLMatrix3x3	m;
OGLPoint2D		p,p1,p2,p3,p4;
float			minX,minZ,maxX,maxZ;

	AllocateCollisionBoxMemory(theNode, 1);

			/* POINT TO BOUNDING BOX */

	if (theNode->Genre == SKELETON_GENRE)
		bBox =  &gObjectGroupBBoxList[MODEL_GROUP_SKELETONBASE+theNode->Type][0];	// (skeleton type is actually the Group #, and only 1 model in there, so 0)
	else
		bBox =  &gObjectGroupBBoxList[theNode->Group][theNode->Type];				// get ptr to this model's local bounding box


			/* ROTATE BOUNDING BOX */

	OGLMatrix3x3_SetRotate(&m, theNode->Rot.y);


	p.x = bBox->min.x;								// far left corner
	p.y = bBox->min.z;
	OGLPoint2D_Transform(&p, &m, &p1);
	p.x = bBox->max.x;								// far right corner
	p.y = bBox->min.z;
	OGLPoint2D_Transform(&p, &m, &p2);
	p.x = bBox->max.x;								// near right corner
	p.y = bBox->max.z;
	OGLPoint2D_Transform(&p, &m, &p3);
	p.x = bBox->min.x;								// near left corner
	p.y = bBox->max.z;
	OGLPoint2D_Transform(&p, &m, &p4);


			/* FIND MIN/MAX */

	minX = p1.x;
	if (p2.x < minX)
		minX = p2.x;
	if (p3.x < minX)
		minX = p3.x;
	if (p4.x < minX)
		minX = p4.x;

	maxX = p1.x;
	if (p2.x > maxX)
		maxX = p2.x;
	if (p3.x > maxX)
		maxX = p3.x;
	if (p4.x > maxX)
		maxX = p4.x;

	minZ = p1.y;
	if (p2.y < minZ)
		minZ = p2.y;
	if (p3.y < minZ)
		minZ = p3.y;
	if (p4.y < minZ)
		minZ = p4.y;

	maxZ = p1.y;
	if (p2.y > maxZ)
		maxZ = p2.y;
	if (p3.y > maxZ)
		maxZ = p3.y;
	if (p4.y > maxZ)
		maxZ = p4.y;


			/* CONVERT TO COLLISON BOX */

	s = theNode->Scale.x;

	theNode->LeftOff 	= minX * (s * tweakXZ);
	theNode->RightOff 	= maxX * (s * tweakXZ);
	theNode->FrontOff 	= maxZ * (s * tweakXZ);
	theNode->BackOff 	= minZ * (s * tweakXZ);
	theNode->TopOff 	= bBox->max.y * s * tweakY;
	theNode->BottomOff 	= bBox->min.y * s;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}




/**************** ALLOCATE COLLISION BOX MEMORY *******************/

void AllocateCollisionBoxMemory(ObjNode *theNode, short numBoxes)
{
Ptr	mem;

			/* FREE OLD STUFF */

	if (theNode->CollisionBoxes)
		SafeDisposePtr((Ptr)theNode->CollisionBoxes);

				/* SET # */

	theNode->NumCollisionBoxes = numBoxes;
	if (numBoxes == 0)
		DoFatalAlert("AllocateCollisionBoxMemory with 0 boxes?");


				/* CURRENT LIST */

	mem = AllocPtr(sizeof(CollisionBoxType)*numBoxes);
	if (mem == nil)
		DoFatalAlert("Couldnt alloc collision box memory");
	theNode->CollisionBoxes = (CollisionBoxType *)mem;
}


/*******************************  KEEP OLD COLLISION BOXES **************************/
//
// Also keeps old coordinate and stuff
//

void KeepOldCollisionBoxes(ObjNode *theNode)
{
long	i;

	for (i = 0; i < theNode->NumCollisionBoxes; i++)
	{
		theNode->CollisionBoxes[i].oldTop = theNode->CollisionBoxes[i].top;
		theNode->CollisionBoxes[i].oldBottom = theNode->CollisionBoxes[i].bottom;
		theNode->CollisionBoxes[i].oldLeft = theNode->CollisionBoxes[i].left;
		theNode->CollisionBoxes[i].oldRight = theNode->CollisionBoxes[i].right;
		theNode->CollisionBoxes[i].oldFront = theNode->CollisionBoxes[i].front;
		theNode->CollisionBoxes[i].oldBack = theNode->CollisionBoxes[i].back;
	}

			/* CALC ACTUAL MOTION VECTOR */

	theNode->RealMotion.x = theNode->Coord.x - theNode->OldCoord.x;
	theNode->RealMotion.y = theNode->Coord.y - theNode->OldCoord.y;
	theNode->RealMotion.z = theNode->Coord.z - theNode->OldCoord.z;

	theNode->OldCoord = theNode->Coord;			// remember coord also
	theNode->OldSpeed2D = theNode->OldSpeed2D;	// remember speed
}


/**************** CALC OBJECT BOX FROM NODE ******************/
//
// This does a simple 1 box calculation for basic objects.
//
// Box is calculated based on theNode's coords.
//

void CalcObjectBoxFromNode(ObjNode *theNode)
{
CollisionBoxType *boxPtr;

	if (theNode->CollisionBoxes == nil)
		DoFatalAlert("CalcObjectBox on objnode with no CollisionBoxType");

	boxPtr = theNode->CollisionBoxes;					// get ptr to 1st box (presumed only box)

	boxPtr->left 	= theNode->Coord.x + (float)theNode->LeftOff;
	boxPtr->right 	= theNode->Coord.x + (float)theNode->RightOff;
	boxPtr->top 	= theNode->Coord.y + (float)theNode->TopOff;
	boxPtr->bottom 	= theNode->Coord.y + (float)theNode->BottomOff;
	boxPtr->back 	= theNode->Coord.z + (float)theNode->BackOff;
	boxPtr->front 	= theNode->Coord.z + (float)theNode->FrontOff;

}


/**************** CALC OBJECT BOX FROM GLOBAL ******************/
//
// This does a simple 1 box calculation for basic objects.
//
// Box is calculated based on gCoord
//

void CalcObjectBoxFromGlobal(ObjNode *theNode)
{
CollisionBoxType *boxPtr;

	if (theNode == nil)
		return;

	if (theNode->CollisionBoxes == nil)
		return;

	boxPtr = theNode->CollisionBoxes;					// get ptr to 1st box (presumed only box)

	boxPtr->left 	= gCoord.x  + (float)theNode->LeftOff;
	boxPtr->right 	= gCoord.x  + (float)theNode->RightOff;
	boxPtr->back 	= gCoord.z  + (float)theNode->BackOff;
	boxPtr->front 	= gCoord.z  + (float)theNode->FrontOff;
	boxPtr->top 	= gCoord.y  + (float)theNode->TopOff;
	boxPtr->bottom 	= gCoord.y  + (float)theNode->BottomOff;
}


/******************* SET OBJECT COLLISION BOUNDS **********************/
//
// Sets an object's collision offset/bounds.  Adjust accordingly for input rotation 0..3 (clockwise)
//

void SetObjectCollisionBounds(ObjNode *theNode, short top, short bottom, short left,
							 short right, short front, short back)
{

	AllocateCollisionBoxMemory(theNode, 1);					// alloc 1 collision box
	theNode->TopOff 		= top;
	theNode->BottomOff 	= bottom;
	theNode->LeftOff 	= left;
	theNode->RightOff 	= right;
	theNode->FrontOff 	= front;
	theNode->BackOff 	= back;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT SHADOWS ------




/******************* ATTACH SHADOW TO OBJECT ************************/

ObjNode	*AttachShadowToObject(ObjNode *theNode, int shadowType, float scaleX, float scaleZ, Boolean checkBlockers)
{
ObjNode	*shadowObj;

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.coord 		= theNode->Coord;
	gNewObjectDefinition.coord.y 	+= SHADOW_Y_OFF;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|gAutoFadeStatusBits;

	if (theNode->Slot >= SLOT_OF_DUMB+1)					// shadow *must* be after parent!
		gNewObjectDefinition.slot 	= theNode->Slot+1;
	else
		gNewObjectDefinition.slot 	= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scaleX;

	shadowObj = MakeNewObject(&gNewObjectDefinition);
	if (shadowObj == nil)
		return(nil);

	theNode->ShadowNode = shadowObj;

	shadowObj->SpecialF[0] = scaleX;							// need to remeber scales for update
	shadowObj->SpecialF[1] = scaleZ;

	shadowObj->CheckForBlockers = checkBlockers;

	shadowObj->CustomDrawFunction = DrawShadow;

	shadowObj->Kind = shadowType;							// remember the shadow type

	return(shadowObj);
}


/************************ UPDATE SHADOW *************************/

void UpdateShadow(ObjNode *theNode)
{
ObjNode *shadowNode;
long	x,y,z;
float	dist;

	if (theNode == nil)
		return;

	shadowNode = theNode->ShadowNode;
	if (shadowNode == nil)
		return;


	x = theNode->Coord.x;
	y = theNode->Coord.y + theNode->BottomOff;
	z = theNode->Coord.z;

	shadowNode->Coord = theNode->Coord;
	shadowNode->Rot.y = theNode->Rot.y;

	RotateOnTerrain(shadowNode, SHADOW_Y_OFF, nil);							// set transform matrix

			/* CALC SCALE OF SHADOW */

	dist = (y - shadowNode->Coord.y) * (1.0f/800.0f);					// as we go higher, shadow gets smaller
	if (dist > .5f)
		dist = .5f;
	else
	if (dist < 0.0f)
		dist = 0;

	dist = 1.0f - dist;

	shadowNode->Scale.x = dist * shadowNode->SpecialF[0];				// this scale wont get updated until next frame (RotateOnTerrain).
	shadowNode->Scale.z = dist * shadowNode->SpecialF[1];
}


/******************* DRAW SHADOW ******************/

static void DrawShadow(ObjNode *theNode, OGLSetupOutputType *setupInfo)
{
int	shadowType = theNode->Kind;


	OGL_PushState();

			/* SUBMIT THE MATRIX */

	glMultMatrixf((GLfloat *)&theNode->BaseTransformMatrix);


			/* SUBMIT SHADOW TEXTURE */


	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_Shadow_Circular+shadowType].materialObject, setupInfo);


			/* DRAW THE SHADOW */

	glDisable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex3f(-20, 0, 20);
	glTexCoord2f(1,0);	glVertex3f(20, 0, 20);
	glTexCoord2f(1,1);	glVertex3f(20, 0, -20);
	glTexCoord2f(0,1);	glVertex3f(-20, 0, -20);
	glEnd();
	glEnable(GL_CULL_FACE);

	OGL_PopState();
}


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CULLING ------


/**************** CULL TEST ALL OBJECTS *******************/

void CullTestAllObjects(void)
{
int			i;
ObjNode		*theNode;
float		m00,m01,m02,m03;
float		m10,m11,m12,m13;
float		m20,m21,m22,m23;
float		m30,m31,m32,m33;
float		minX,minY,minZ,maxX,maxY,maxZ;
OGLMatrix4x4		m;
OGLBoundingBox		*bBox;
float		minusHW;				// -hW
uint32_t		clipFlags;				// Clip in/out tests for point
uint32_t		clipCodeAND,clipCodeOR;	// Clip test for entire object
float		lX, lY, lZ;				// Local space co-ordinates
float		hX, hY, hZ, hW;			// Homogeneous co-ordinates


	theNode = gFirstNodePtr;														// get & verify 1st node
	if (theNode == nil)
		return;

					/* PROCESS EACH OBJECT */

	do
	{
		theNode->StatusBits &= ~STATUS_BIT_NOCLIPTEST;			// assume we'll need clip testing

		if (theNode->StatusBits & STATUS_BIT_ALWAYSCULL)
			goto try_cull;

		if (theNode->StatusBits & STATUS_BIT_HIDDEN)			// if hidden then skip
			goto next;

		if (theNode->BaseGroup == nil)							// quick check if any geometry at all
			if (theNode->Genre != SKELETON_GENRE)
				goto next;

		if (theNode->StatusBits & STATUS_BIT_DONTCULL)			// see if dont want to use our culling
			goto draw_on;

try_cull:

			/* CALCULATE THE LOCAL->FRUSTUM MATRIX FOR THIS OBJECT */

	OGLMatrix4x4_Multiply(&theNode->BaseTransformMatrix, &gLocalToFrustumMatrix, &m);

	m00 = m.value[M00];
	m01 = m.value[M01];
	m02 = m.value[M02];
	m03 = m.value[M03];
	m10 = m.value[M10];
	m11 = m.value[M11];
	m12 = m.value[M12];
	m13 = m.value[M13];
	m20 = m.value[M20];
	m21 = m.value[M21];
	m22 = m.value[M22];
	m23 = m.value[M23];
	m30 = m.value[M30];
	m31 = m.value[M31];
	m32 = m.value[M32];
	m33 = m.value[M33];


				/* TRANSFORM THE BOUNDING BOX */

	if (theNode->Genre == SKELETON_GENRE)
		bBox =  &gObjectGroupBBoxList[MODEL_GROUP_SKELETONBASE+theNode->Type][0];	// (skeleton type is actually the Group #, and only 1 model in there, so 0)
	else
		bBox =  &gObjectGroupBBoxList[theNode->Group][theNode->Type];				// get ptr to this model's local bounding box

	minX = bBox->min.x;													// load bbox into registers
	minY = bBox->min.y;
	minZ = bBox->min.z;
	maxX = bBox->max.x;
	maxY = bBox->max.y;
	maxZ = bBox->max.z;

	clipCodeAND = ~0;
	clipCodeOR 	= 0;

	for (i = 0; i < 8; i++)
	{
		switch (i)														// load current bbox corner in IX,IY,IZ
		{
			case	0:	lX = minX;	lY = minY;	lZ = minZ;	break;
			case	1:	lX = minX;	lY = minY;	lZ = maxZ;	break;
			case	2:	lX = minX;	lY = maxY;	lZ = minZ;	break;
			case	3:	lX = minX;	lY = maxY;	lZ = maxZ;	break;
			case	4:	lX = maxX;	lY = minY;	lZ = minZ;	break;
			case	5:	lX = maxX;	lY = minY;	lZ = maxZ;	break;
			case	6:	lX = maxX;	lY = maxY;	lZ = minZ;	break;
			default:	lX = maxX;	lY = maxY;	lZ = maxZ;
		}

		hW = lX * m30 + lY * m31 + lZ * m32 + m33;
		hY = lX * m10 + lY * m11 + lZ * m12 + m13;
		hZ = lX * m20 + lY * m21 + lZ * m22 + m23;
		hX = lX * m00 + lY * m01 + lZ * m02 + m03;

		minusHW = -hW;

				/* CHECK Y */

		if (hY < minusHW)
			clipFlags = 0x8;
		else
		if (hY > hW)
			clipFlags = 0x4;
		else
			clipFlags = 0;


				/* CHECK Z */

		if (hZ > hW)
			clipFlags |= 0x20;
		else
		if (hZ < 0.0f)
			clipFlags |= 0x10;


				/* CHECK X */

		if (hX < minusHW)
			clipFlags |= 0x2;
		else
		if (hX > hW)
			clipFlags |= 0x1;

		clipCodeAND &= clipFlags;
		clipCodeOR |= clipFlags;
	}

	/****************************/
	/* SEE IF WAS CULLED OR NOT */
	/****************************/
	//
	//  We have one of 3 cases:
	//
	//	1. completely in bounds - no clipping
	//	2. completely out of bounds - no need to render
	//	3. some clipping is needed
	//

	if (clipCodeOR == 0)														// check for case #1
	{
		theNode->StatusBits |= STATUS_BIT_NOCLIPTEST;
	}


	if (clipCodeAND)															// check for case #2
	{
				/* NOT DRAWN */
		theNode->StatusBits |= STATUS_BIT_ISCULLED;								// set cull bit
	}
	else
	{
draw_on:
		theNode->StatusBits &= ~STATUS_BIT_ISCULLED;							// clear cull bit
	}



				/* NEXT NODE */
next:
		theNode = theNode->NextNode;		// next node
	}
	while (theNode != nil);
}


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- MISC OBJECT FUNCTIONS ------


/************************ STOP OBJECT STREAM EFFECT *****************************/

void StopObjectStreamEffect(ObjNode *theNode)
{
	if (theNode->EffectChannel != -1)
	{
		StopAChannel(&theNode->EffectChannel);
	}
}

