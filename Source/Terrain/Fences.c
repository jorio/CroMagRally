/**********************/
/*   	FENCES.C      */
/**********************/


#include "globals.h"
#include "terrain.h"
#include "misc.h"
#include "main.h"
#include "objects.h"
#include "3dmath.h"
#include "fences.h"
#include "bg3d.h"
#include "sprites.h"
#include "sobjtypes.h"

/***************/
/* EXTERNALS   */
/***************/

extern	OGLPoint3D	gCoord;
extern	OGLVector3D	gDelta;
extern	float		gAutoFadeStartDist,gAutoFadeRange_Frac,gAutoFadeEndDist,gFramesPerSecondFrac;
extern	short		gCurrentPlayerNum;
extern	FSSpec		gDataSpec;
extern	int			gCurrentSplitScreenPane;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	long		gNumSpritesInGroupList[MAX_SPRITE_GROUPS];
extern	uint32_t		gGlobalMaterialFlags;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawFences(ObjNode *theNode, OGLSetupOutputType *setupInfo);
static void SubmitFence(int f, OGLSetupOutputType *setupInfo, float camX, float camZ);
static void MakeFenceGeometry(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_FENCES			60
#define	MAX_NUBS_IN_FENCE	80


#define	FENCE_SINK_FACTOR	200.0f

enum
{
	FENCE_TYPE_DESERTSKIN,
	FENCE_TYPE_INVISIBLE,
	FENCE_TYPE_CHINA1,
	FENCE_TYPE_CHINA2,
	FENCE_TYPE_CHINA3,
	FENCE_TYPE_CHINA4,
	FENCE_TYPE_HIEROGLYPHS,
	FENCE_TYPE_CAMEL,
	FENCE_TYPE_FARM,
	FENCE_TYPE_AZTEC,
	FENCE_TYPE_ROCKWALL,
	FENCE_TYPE_TALLROCKWALL,
	FENCE_TYPE_ROCKPILE,
	FENCE_TYPE_TRIBAL,
	FENCE_TYPE_HORNS,
	FENCE_TYPE_CRETE,
	FENCE_TYPE_ROCKPILE2,
	FENCE_TYPE_ORANGEROCK,

	FENCE_TYPE_SEAWEED,
	FENCE_TYPE_SEAWEED2,
	FENCE_TYPE_SEAWEED3,
	FENCE_TYPE_SEAWEED4,
	FENCE_TYPE_SEAWEED5,
	FENCE_TYPE_SEAWEED6,

	FENCE_TYPE_CHINACONCRETE,
	FENCE_TYPE_CHINADESIGN,
	FENCE_TYPE_VIKING
};


/**********************/
/*     VARIABLES      */
/**********************/

long			gNumFences = 0;
short			gNumFencesDrawn;
FenceDefType	*gFenceList = nil;


float			gFenceHeight[] =
{
	2000,					// desert skin
	3200,					// INVISIBLE
	1400,					// China - Multicolored
	1400,					// China - Red
	1400,					// China - White
	1400,					// China - Yellow
	1800,					// Hieroglyphs
	1000,					// Camel xing
	700,					// farm
	1500,					// Aztec
	1000,					// Rock Wall
	5000,					// Tall Rock Wall
	1500,					// Rock Pile
	1300,					// Tribal
	1300,					// horns
	1300,					// crete
	1300,					// rock pile 2
	1300,					// orange rock

	7000,					// 18: Sea Weed 1
	7000,					// Sea Weed 2
	7000,					// Sea Weed 3
	7000,					// Sea Weed 4
	7000,					// Sea Weed 5
	7000,					// Sea Weed 6

	1800,					// China - Concrete
	1900,					// China -Design
	2000,					// Viking
};


static MOVertexArrayData		gFenceTriMeshData[MAX_FENCES];
static MOTriangleIndecies		gFenceTriangles[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLPoint3D				gFencePoints[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLTextureCoord			gFenceUVs[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLColorRGBA_Byte			gFenceColors[MAX_FENCES][MAX_NUBS_IN_FENCE*2];

static short	gSeaweedFrame;
static float	gSeaweedFrameTimer;


/********************** DISPOSE FENCES *********************/

void DisposeFences(void)
{
int		i;

	if (!gFenceList)
		return;

	for (i = 0; i < gNumFences; i++)
	{
		if (gFenceList[i].sectionVectors)
			SafeDisposePtr((Ptr)gFenceList[i].sectionVectors);			// nuke section vectors
		gFenceList[i].sectionVectors = nil;

		if (gFenceList[i].nubList)
			SafeDisposePtr((Ptr)gFenceList[i].nubList);
		gFenceList[i].nubList = nil;
	}

	SafeDisposePtr((Ptr)gFenceList);
	gFenceList = nil;
	gNumFences = 0;
}



/********************* PRIME FENCES ***********************/
//
// Called during terrain prime function to initialize
//

void PrimeFences(void)
{
long					f,i,numNubs;
FenceDefType			*fence;
OGLPoint3D				*nubs;
ObjNode					*obj;

	gSeaweedFrame = 0;
	gSeaweedFrameTimer = 0;


	if (gNumFences > MAX_FENCES)
		DoFatalAlert("PrimeFences: gNumFences > MAX_FENCES");

		/* SET TEXTURE WARPPING MODE ON ALL FENCE SPRITE TEXTURES */

	for (i = 0; i < gNumSpritesInGroupList[SPRITE_GROUP_FENCES]; i++)
	{
		MOMaterialObject	*mat;

		mat = gSpriteGroupList[SPRITE_GROUP_FENCES][i].materialObject;
		mat->objectData.flags |= BG3D_MATERIALFLAG_CLAMP_V;				// don't wrap v, only u
	}


			/******************************/
			/* ADJUST TO GAME COORDINATES */
			/******************************/

	for (f = 0; f < gNumFences; f++)
	{
		fence 				= &gFenceList[f];					// point to this fence
		nubs 				= fence->nubList;					// point to nub list
		numNubs 			= fence->numNubs;					// get # nubs in fence

		if (numNubs == 1)
			DoFatalAlert("PrimeFences: numNubs == 1");

		if (numNubs > MAX_NUBS_IN_FENCE)
			DoFatalAlert("PrimeFences: numNubs > MAX_NUBS_IN_FENCE");

		for (i = 0; i < numNubs; i++)							// adjust nubs
		{
			nubs[i].x *= MAP2UNIT_VALUE;
			nubs[i].z *= MAP2UNIT_VALUE;
			nubs[i].y = GetTerrainY(nubs[i].x,nubs[i].z);		// calc Y
		}

		/* CALCULATE VECTOR FOR EACH SECTION */

		fence->sectionVectors = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));
		if (fence->sectionVectors == nil)
			DoFatalAlert("PrimeFences: AllocPtr failed!");

		for (i = 0; i < (numNubs-1); i++)
		{
			float	dx,dz;

			dx = nubs[i+1].x - nubs[i].x;
			dz = nubs[i+1].z - nubs[i].z;

			FastNormalizeVector2D(dx, dz, &fence->sectionVectors[i], false);
		}

	}

			/*****************************/
			/* MAKE FENCE GEOMETRY */
			/*****************************/

	MakeFenceGeometry();

		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE FENCE DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The fences need to be drawn after the Cyc object, but before any sprite or font objects.
		//

	NewObjectDefinitionType def =
	{
		.genre		= CUSTOM_GENRE,
		.slot		= FENCE_SLOT,
		.flags		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING,
	};

	obj = MakeNewObject(&def);
	obj->CustomDrawFunction = DrawFences;
}


/*************** MAKE FENCE GEOMETRY *********************/

static void MakeFenceGeometry(void)
{
int						f;
uint16_t					type;
float					u,height,aspectRatio,textureUOff;
long					i,numNubs,j;
FenceDefType			*fence;
OGLPoint3D				*nubs;
float					minX,minY,minZ,maxX,maxY,maxZ;

	for (f = 0; f < gNumFences; f++)
	{
				/******************/
				/* GET FENCE INFO */
				/******************/

		fence = &gFenceList[f];								// point to this fence
		nubs = fence->nubList;								// point to nub list
		numNubs = fence->numNubs;							// get # nubs in fence
		type = fence->type;									// get fence type
		if (type > gNumSpritesInGroupList[SPRITE_GROUP_FENCES])
			DoFatalAlert("MakeFenceGeometry: illegal fence type");
		height = gFenceHeight[type];						// get fence height

		aspectRatio = gSpriteGroupList[SPRITE_GROUP_FENCES][type].aspectRatio;	// get aspect ratio

		textureUOff = 1.0f / height * aspectRatio;			// calc UV offset


					/***************************/
					/* SET VERTEX ARRAY HEADER */
					/***************************/

		gFenceTriMeshData[f].numMaterials		 		= 1;
		gFenceTriMeshData[f].materials[0]				= nil;
		gFenceTriMeshData[f].points 					= &gFencePoints[f][0];
		gFenceTriMeshData[f].triangles					= &gFenceTriangles[f][0];
		gFenceTriMeshData[f].uvs						= &gFenceUVs[f][0];
		gFenceTriMeshData[f].normals					= nil;
		gFenceTriMeshData[f].colorsByte					= &gFenceColors[f][0];
		gFenceTriMeshData[f].colorsFloat				= nil;
		gFenceTriMeshData[f].numPoints = numNubs * 2;					// 2 vertices per nub
		gFenceTriMeshData[f].numTriangles = (numNubs-1) * 2;			// 2 faces per nub (minus 1st)


				/* BUILD TRIANGLE INFO */

		for (i = j = 0; i < MAX_NUBS_IN_FENCE; i++, j+=2)
		{
			gFenceTriangles[f][j].vertexIndices[0] = 1 + j;
			gFenceTriangles[f][j].vertexIndices[1] = 0 + j;
			gFenceTriangles[f][j].vertexIndices[2] = 3 + j;

			gFenceTriangles[f][j+1].vertexIndices[0] = 3 + j;
			gFenceTriangles[f][j+1].vertexIndices[1] = 0 + j;
			gFenceTriangles[f][j+1].vertexIndices[2] = 2 + j;

		}

				/* INIT VERTEX COLORS */

		for (i = 0; i < (MAX_NUBS_IN_FENCE*2); i++)
			gFenceColors[f][i].r = gFenceColors[f][i].g = gFenceColors[f][i].b = 0xff;


				/* SET TEXTURE */

		gFenceTriMeshData[f].materials[0] = gSpriteGroupList[SPRITE_GROUP_FENCES][type].materialObject;	// set illegal temporary ref to material


				/**********************/
				/* BUILD POINTS, UV's */
				/**********************/

		maxX = maxY = maxZ = -1000000;									// build new bboxes while we do this
		minX = minY = minZ = -maxX;

		u = 0;
		for (i = j = 0; i < numNubs; i++, j+=2)
		{
			float		x,y,z,y2;

					/* GET COORDS */

			x = nubs[i].x;
			z = nubs[i].z;

			y = nubs[i].y;
			y -= FENCE_SINK_FACTOR;										// sink into ground a little bit
			y2 = y + height;

					/* CHECK BBOX */

			if (x < minX)	minX = x;									// find min/max bounds for bbox
			if (x > maxX)	maxX = x;
			if (z < minZ)	minZ = z;
			if (z > maxZ)	maxZ = z;
			if (y < minY)	minY = y;
			if (y > maxY)	maxY = y;


				/* SET COORDS */

			gFencePoints[f][j].x = x;
			gFencePoints[f][j].y = y;
			gFencePoints[f][j].z = z;

			gFencePoints[f][j+1].x = x;
			gFencePoints[f][j+1].y = y2;
			gFencePoints[f][j+1].z = z;



				/* CALC UV COORDS */

			if (i > 0)
			{
				u += CalcDistance3D(gFencePoints[f][j].x, gFencePoints[f][j].y, gFencePoints[f][j].z,
									gFencePoints[f][j-2].x, gFencePoints[f][j-2].y, gFencePoints[f][j-2].z) * textureUOff;
			}

			gFenceUVs[f][j].v 		= 0;									// bottom
			gFenceUVs[f][j+1].v 	= 1.0;									// top
			gFenceUVs[f][j].u 		= gFenceUVs[f][j+1].u = u;
		}

				/* SET CALCULATED BBOX */

		fence->bBox.min.x = minX;
		fence->bBox.max.x = maxX;
		fence->bBox.min.y = minY;
		fence->bBox.max.y = maxY;
		fence->bBox.min.z = minZ;
		fence->bBox.max.z = maxZ;
		fence->bBox.isEmpty = false;
	}
}


#pragma mark -

/********************* DRAW FENCES ***********************/

static void DrawFences(ObjNode *theNode, OGLSetupOutputType *setupInfo)
{
long			f,type;
float			cameraX, cameraZ;

	theNode;

			/* UPDATE SEAWEED ANIMATION */

	gSeaweedFrameTimer += gFramesPerSecondFrac;
	if (gSeaweedFrameTimer > .09f)
	{
		gSeaweedFrameTimer -= .09f;

		if (++gSeaweedFrame > 5)
			gSeaweedFrame = 0;
	}


			/* GET CAMERA COORDS */

	cameraX = setupInfo->cameraPlacement[gCurrentSplitScreenPane].cameraLocation.x;
	cameraZ = setupInfo->cameraPlacement[gCurrentSplitScreenPane].cameraLocation.z;


			/* SET GLOBAL MATERIAL FLAGS */

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_ALWAYSBLEND;


			/*******************/
			/* DRAW EACH FENCE */
			/*******************/

	gNumFencesDrawn = 0;

	for (f = 0; f < gNumFences; f++)
	{
		type = gFenceList[f].type;							// get type
		if (type == FENCE_TYPE_INVISIBLE)					// see if skip invisible fence
			continue;

					/* DO BBOX CULLING */

		if (OGL_IsBBoxVisible(&gFenceList[f].bBox))
		{
				/* SUBMIT GEOMETRY */

			SubmitFence(f, setupInfo, cameraX, cameraZ);
			gNumFencesDrawn++;
		}
	}

	gGlobalMaterialFlags = 0;
}

/******************** SUBMIT FENCE **************************/
//
// Visibility checks have already been done, so there's a good chance the fence is visible
//

static void SubmitFence(int f, OGLSetupOutputType *setupInfo, float camX, float camZ)
{
float					dist,alpha = 1.0f;
long					i,numNubs,j;
FenceDefType			*fence;
OGLPoint3D				*nubs;

			/* GET FENCE INFO */

	fence = &gFenceList[f];								// point to this fence
	nubs = fence->nubList;								// point to nub list
	numNubs = fence->numNubs;							// get # nubs in fence


				/* CALC & SET TRANSPARENCY */

	for (i = j = 0; i < numNubs; i++, j+=2)
	{
		float		x,z;

		x = nubs[i].x;
		z = nubs[i].z;

				/* CALC & SET TRANSPARENCY */

		if (gAutoFadeStartDist != 0.0f)								// see if this level has xparency
		{
			dist = CalcQuickDistance(camX, camZ, x, z);				// see if in fade zone
			if (dist < gAutoFadeStartDist)
				alpha = 1.0;
			else
			if (dist >= gAutoFadeEndDist)
				alpha = 0.0;
			else
			{
				dist -= gAutoFadeStartDist;							// calc xparency %
				dist *= gAutoFadeRange_Frac;
				if (dist < 0.0f)
					alpha = 0;
				else
					alpha = 1.0f - dist;
			}
		}

		gFenceColors[f][j].a =
		gFenceColors[f][j+1].a = 255.0f * alpha;
	}



		/*******************/
		/* SUBMIT GEOMETRY */
		/*******************/

	if (fence->type == FENCE_TYPE_SEAWEED)
	{
		gFenceTriMeshData[f].materials[0] = gSpriteGroupList[SPRITE_GROUP_FENCES][FENCE_TYPE_SEAWEED + gSeaweedFrame].materialObject;	// set illegal temporary ref to material
	}

	MO_DrawGeometry_VertexArray(&gFenceTriMeshData[f], setupInfo);
}




#pragma mark -

/******************** DO FENCE COLLISION **************************/

void DoFenceCollision(ObjNode *theNode)
{
float			fromX,fromZ,toX,toZ;
long			f,numFenceSegments,i,numReScans;
float			segFromX,segFromZ,segToX,segToZ;
OGLPoint3D		*nubs;
Boolean			intersected,letGoOver;
float			intersectX,intersectZ;
OGLVector2D		lineNormal;
float			radius;
float			oldX,oldZ,newX,newZ;

	letGoOver = false;

			/* CALC MY MOTION LINE SEGMENT */

	oldX = theNode->OldCoord.x;						// from old coord
	oldZ = theNode->OldCoord.z;
	newX = gCoord.x;								// to new coord
	newZ = gCoord.z;
	radius = theNode->BoundingSphereRadius;



			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/

	for (f = 0; f < gNumFences; f++)
	{
		float	temp;
		int		type;

				/* SPECIAL STUFF */

		type = gFenceList[f].type;

#if 0
		switch(type)
		{
			case	FENCE_TYPE_WHEAT:					// things can go over this
			case	FENCE_TYPE_FOREST2:
					letGoOver = true;
					break;

			case	FENCE_TYPE_MOSS:					// this isnt solid
					goto next_fence;
		}
#endif

		/* QUICK CHECK TO SEE IF OLD & NEW COORDS (PLUS RADIUS) ARE OUTSIDE OF FENCE'S BBOX */

		temp = gFenceList[f].bBox.min.x - radius;
		if ((oldX < temp) && (newX < temp))
			continue;
		temp = gFenceList[f].bBox.max.x + radius;
		if ((oldX > temp) && (newX > temp))
			continue;

		temp = gFenceList[f].bBox.min.z - radius;
		if ((oldZ < temp) && (newZ < temp))
			continue;
		temp = gFenceList[f].bBox.max.z + radius;
		if ((oldZ > temp) && (newZ > temp))
			continue;

		nubs = gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence


				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/

		numReScans = 0;
		for (i = 0; i < numFenceSegments; i++)
		{
					/* GET LINE SEG ENDPOINTS */

			segFromX = nubs[i].x;
			segFromZ = nubs[i].z;
			segToX = nubs[i+1].x;
			segToZ = nubs[i+1].z;


					/* SEE IF ROUGHLY CAN GO OVER */

			if (letGoOver)
			{
				float y = nubs[i].y + gFenceHeight[type];
				if ((gCoord.y + theNode->BottomOff) >= y)
					continue;
			}


					/* CALC NORMAL TO THE LINE */
					//
					// We need to find the point on the bounding sphere which is closest to the line
					// in order to do good collision checks
					//

			CalcRayNormal2D(&gFenceList[f].sectionVectors[i], segFromX, segFromZ,
							 oldX, oldZ, &lineNormal);


					/* CALC FROM-TO POINTS OF MOTION */

			fromX = oldX - (lineNormal.x * radius);
			fromZ = oldZ - (lineNormal.y * radius);
			toX = newX - (lineNormal.x * radius);
			toZ = newZ - (lineNormal.y * radius);


					/* SEE IF THE LINES INTERSECT */

			intersected = IntersectLineSegments(fromX,  fromZ, toX, toZ,
						                     segFromX, segFromZ, segToX, segToZ,
				                             &intersectX, &intersectZ);

			if (intersected)
			{
						/***************************/
						/* HANDLE THE INTERSECTION */
						/***************************/
						//
						// Move so edge of sphere would be tangent, but also a bit
						// farther so it isnt tangent.
						//

				gCoord.x = intersectX + lineNormal.x*radius + (lineNormal.x*5.0f);
				gCoord.z = intersectZ + lineNormal.y*radius + (lineNormal.y*5.0f);


						/* BOUNCE OFF WALL */

				{
					OGLVector2D deltaV;

					deltaV.x = gDelta.x;
					deltaV.y = gDelta.z;
					ReflectVector2D(&deltaV, &lineNormal);
					gDelta.x = deltaV.x * .3f;
					gDelta.z = deltaV.y * .3f;
				}

						/* UPDATE COORD & SCAN AGAIN */

				newX = gCoord.x;
				newZ = gCoord.z;
				if (++numReScans < 5)
					i = 0;							// reset segment index to scan all again
			}

			/**********************************************/
			/* NO INTERSECT, DO SAFETY CHECK FOR /\ CASES */
			/**********************************************/
			//
			// The above check may fail when the sphere is going thru
			// the tip of a tee pee /\ intersection, so this is a hack
			// to get around it.
			//

			else
			{
					/* SEE IF EITHER ENDPOINT IS IN SPHERE */

				if ((CalcQuickDistance(segFromX, segFromZ, newX, newZ) <= radius) ||
					(CalcQuickDistance(segToX, segToZ, newX, newZ) <= radius))
				{
					OGLVector2D deltaV;

					gCoord.x = oldX;
					gCoord.z = oldZ;

						/* BOUNCE OFF WALL */

					deltaV.x = gDelta.x;
					deltaV.y = gDelta.z;
					ReflectVector2D(&deltaV, &lineNormal);
					gDelta.x = deltaV.x * .5f;
					gDelta.z = deltaV.y * .5f;
					return;
				}
				else
					continue;
			}
		} // for i
	}
}





