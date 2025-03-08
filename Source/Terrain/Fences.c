/**********************/
/*   	FENCES.C      */
/**********************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawFences(ObjNode *theNode);
static void SubmitFence(int f, float camX, float camZ);
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
	FENCE_TYPE_VIKING,

	NUM_FENCE_TYPES
};


const char* kFenceNames[NUM_FENCE_TYPES] =
{
	[FENCE_TYPE_DESERTSKIN]		= "desertskin",
	[FENCE_TYPE_INVISIBLE]		= "invisible",
	[FENCE_TYPE_CHINA1]			= "china1",
	[FENCE_TYPE_CHINA2]			= "china2",
	[FENCE_TYPE_CHINA3]			= "china3",
	[FENCE_TYPE_CHINA4]			= "china4",
	[FENCE_TYPE_HIEROGLYPHS]	= "hieroglyphs",
	[FENCE_TYPE_CAMEL]			= "camel",
	[FENCE_TYPE_FARM]			= "farm",
	[FENCE_TYPE_AZTEC]			= "aztec",
	[FENCE_TYPE_ROCKWALL]		= "rockwall",
	[FENCE_TYPE_TALLROCKWALL]	= "tallrockwall",
	[FENCE_TYPE_ROCKPILE]		= "rockpile",
	[FENCE_TYPE_TRIBAL]			= "tribal",
	[FENCE_TYPE_HORNS]			= "horns",
	[FENCE_TYPE_CRETE]			= "crete",
	[FENCE_TYPE_ROCKPILE2]		= "rockpile2",
	[FENCE_TYPE_ORANGEROCK]		= "orangerock",
	[FENCE_TYPE_SEAWEED]		= "seaweed1",
	[FENCE_TYPE_SEAWEED2]		= "seaweed2",
	[FENCE_TYPE_SEAWEED3]		= "seaweed3",
	[FENCE_TYPE_SEAWEED4]		= "seaweed4",
	[FENCE_TYPE_SEAWEED5]		= "seaweed5",
	[FENCE_TYPE_SEAWEED6]		= "seaweed6",
	[FENCE_TYPE_CHINACONCRETE]	= "chinaconcrete",
	[FENCE_TYPE_CHINADESIGN]	= "chinadesign",
	[FENCE_TYPE_VIKING]			= "viking",
};


/**********************/
/*     VARIABLES      */
/**********************/

long			gNumFences = 0;
short			gNumFencesDrawn;
FenceDefType	*gFenceList = nil;
Boolean			gDrawInvisiFences = true;

MOMaterialObject	*gFenceMaterials[NUM_FENCE_TYPES];

const float			gFenceHeight[NUM_FENCE_TYPES] =
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



/******************* LOAD FENCE TEXTURE *******************/

static void LoadFenceMaterial(int type)
{
	char fencePath[256];
	SDL_snprintf(fencePath, sizeof(fencePath), ":Sprites:Fences:%s.png", kFenceNames[type]);

	gFenceMaterials[type] = MO_GetTextureFromFile(fencePath, /*GL_RGB5_A1*/ GL_RGBA);


			/* WRAP U, CLAMP V */

	if (type != FENCE_TYPE_TRIBAL)		// except tribal, which can be wrapped on both dimensions
	{
		gFenceMaterials[type]->objectData.flags |= BG3D_MATERIALFLAG_CLAMP_V;
	}
}


/********************** DISPOSE FENCES *********************/

void DisposeFences(void)
{
	if (!gFenceList)
		return;

	for (int i = 0; i < gNumFences; i++)
	{
		if (gFenceList[i].sectionVectors)
			SafeDisposePtr((Ptr)gFenceList[i].sectionVectors);			// nuke section vectors
		gFenceList[i].sectionVectors = nil;

		if (gFenceList[i].sectionNormals)
			SafeDisposePtr((Ptr)gFenceList[i].sectionNormals);			// nuke normal vectors
		gFenceList[i].sectionNormals = nil;

		if (gFenceList[i].nubList)
			SafeDisposePtr((Ptr)gFenceList[i].nubList);
		gFenceList[i].nubList = nil;
	}

	SafeDisposePtr((Ptr)gFenceList);
	gFenceList = nil;
	gNumFences = 0;


	for (int i = 0; i < NUM_FENCE_TYPES; i++)
	{
		if (gFenceMaterials[i])
		{
			MO_DisposeObjectReference(gFenceMaterials[i]);
			gFenceMaterials[i] = NULL;
		}
	}
}



/********************* PRIME FENCES ***********************/
//
// Called during terrain prime function to initialize
//

ObjNode* PrimeFences(void)
{
long					numNubs;
FenceDefType			*fence;
OGLPoint3D				*nubs;

	gSeaweedFrame = 0;
	gSeaweedFrameTimer = 0;


	if (gNumFences > MAX_FENCES)
		DoFatalAlert("PrimeFences: gNumFences > MAX_FENCES");



			/***********************/
			/* LOAD FENCE TEXTURES */
			/***********************/

	SDL_memset(gFenceMaterials, 0, sizeof(gFenceMaterials));

	for (int f = 0; f < gNumFences; f++)
	{
		fence = &gFenceList[f];					// point to this fence

		GAME_ASSERT(fence->type < NUM_FENCE_TYPES);

		if (gFenceMaterials[fence->type] == NULL)
		{
			if (fence->type == FENCE_TYPE_SEAWEED)
			{
				LoadFenceMaterial(FENCE_TYPE_SEAWEED);
				LoadFenceMaterial(FENCE_TYPE_SEAWEED2);
				LoadFenceMaterial(FENCE_TYPE_SEAWEED3);
				LoadFenceMaterial(FENCE_TYPE_SEAWEED4);
				LoadFenceMaterial(FENCE_TYPE_SEAWEED5);
				LoadFenceMaterial(FENCE_TYPE_SEAWEED6);
			}
			else
			{
				LoadFenceMaterial(fence->type);
			}
		}
	}

			/******************************/
			/* ADJUST TO GAME COORDINATES */
			/******************************/

	for (int f = 0; f < gNumFences; f++)
	{
		fence 				= &gFenceList[f];					// point to this fence
		nubs 				= fence->nubList;					// point to nub list
		numNubs 			= fence->numNubs;					// get # nubs in fence

		if (numNubs == 1)
			DoFatalAlert("PrimeFences: numNubs == 1");

		if (numNubs > MAX_NUBS_IN_FENCE)
			DoFatalAlert("PrimeFences: numNubs > MAX_NUBS_IN_FENCE");

		for (int i = 0; i < numNubs; i++)							// adjust nubs
		{
			nubs[i].x *= MAP2UNIT_VALUE;
			nubs[i].z *= MAP2UNIT_VALUE;
			nubs[i].y = GetTerrainY(nubs[i].x,nubs[i].z);		// calc Y
		}

		/* CALCULATE VECTOR FOR EACH SECTION */

		fence->sectionVectors = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));
		if (fence->sectionVectors == nil)
			DoFatalAlert("PrimeFences: AllocPtr failed!");

		for (int i = 0; i < (numNubs-1); i++)
		{
			float	dx,dz;

			dx = nubs[i+1].x - nubs[i].x;
			dz = nubs[i+1].z - nubs[i].z;

			FastNormalizeVector2D(dx, dz, &fence->sectionVectors[i], false);
		}

		/* CALCULATE NORMALS FOR EACH SECTION */

		fence->sectionNormals = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));		// alloc array to hold vectors

		for (int i = 0; i < (numNubs-1); i++)
		{
			OGLVector3D	v;

			v.x = fence->sectionVectors[i].x;					// get section vector (as calculated above)
			v.z = fence->sectionVectors[i].y;

			fence->sectionNormals[i].x = -v.z;					//  reduced cross product to get perpendicular normal
			fence->sectionNormals[i].y = v.x;
			OGLVector2D_Normalize(&fence->sectionNormals[i], &fence->sectionNormals[i]);
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
		.scale		= 1,
		.drawCall	= DrawFences,
	};

	return MakeNewObject(&def);
}


/*************** MAKE FENCE GEOMETRY *********************/

static void MakeFenceGeometry(void)
{
uint16_t				type;
float					u,height,aspectRatio,textureUOff;
long					i,numNubs,j;
FenceDefType			*fence;
OGLPoint3D				*nubs;
MOMaterialObject		*material;
float					minX,minY,minZ,maxX,maxY,maxZ;

	for (int f = 0; f < gNumFences; f++)
	{
				/******************/
				/* GET FENCE INFO */
				/******************/

		fence = &gFenceList[f];								// point to this fence
		nubs = fence->nubList;								// point to nub list
		numNubs = fence->numNubs;							// get # nubs in fence
		type = fence->type;									// get fence type

		GAME_ASSERT_MESSAGE(type < NUM_FENCE_TYPES, "Illegal fence type!");
		GAME_ASSERT_MESSAGE(gFenceMaterials[type], "No material for fence type!");

		height = gFenceHeight[type];						// get fence height
		material = gFenceMaterials[type];					// get material

		aspectRatio = material->objectData.height / (float)material->objectData.width;
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

		gFenceTriMeshData[f].materials[0] = gFenceMaterials[type];		// set illegal temporary ref to material


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
			gFencePoints[f][j].y = y-height;							// sink an extra height's worth into ground to cover holes in terrain
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

			gFenceUVs[f][j].v 		= -1.0f;									// bottom (-1 due to sinking extra height)
			gFenceUVs[f][j+1].v 	= 1.0f;									// top
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

static void DrawFences(ObjNode *theNode)
{
long			f,type;
float			cameraX, cameraZ;

	(void) theNode;

			/* UPDATE SEAWEED ANIMATION */

	if (gCurrentSplitScreenPane == 0)		// update timer once per frame only
	{
		gSeaweedFrameTimer += gFramesPerSecondFrac;
		if (gSeaweedFrameTimer > .09f)
		{
			gSeaweedFrameTimer -= .09f;

			if (++gSeaweedFrame > 5)
				gSeaweedFrame = 0;
		}
	}


			/* GET CAMERA COORDS */

	cameraX = gGameView->cameraPlacement[gCurrentSplitScreenPane].cameraLocation.x;
	cameraZ = gGameView->cameraPlacement[gCurrentSplitScreenPane].cameraLocation.z;


			/* SET GLOBAL MATERIAL FLAGS */

//	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_ALWAYSBLEND;


			/*******************/
			/* DRAW EACH FENCE */
			/*******************/

	gNumFencesDrawn = 0;

	for (f = 0; f < gNumFences; f++)
	{
		type = gFenceList[f].type;									// get type
		if (type == FENCE_TYPE_INVISIBLE && gDebugMode != 2)		// see if skip invisible fence
			continue;

					/* DO BBOX CULLING */

		if (OGL_IsBBoxVisible(&gFenceList[f].bBox))
		{
				/* SUBMIT GEOMETRY */

			SubmitFence(f, cameraX, cameraZ);
			gNumFencesDrawn++;
		}
	}

	gGlobalMaterialFlags = 0;
}

/******************** SUBMIT FENCE **************************/
//
// Visibility checks have already been done, so there's a good chance the fence is visible
//

static void SubmitFence(int f, float camX, float camZ)
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
//		else
//			dist = 1.0f;

		gFenceColors[f][j].a =
		gFenceColors[f][j+1].a = 255.0f * alpha;
	}



		/*******************/
		/* SUBMIT GEOMETRY */
		/*******************/

	if (fence->type == FENCE_TYPE_SEAWEED)
	{
		gFenceTriMeshData[f].materials[0] = gFenceMaterials[FENCE_TYPE_SEAWEED + gSeaweedFrame];	// set illegal temporary ref to material
	}

	MO_DrawGeometry_VertexArray(&gFenceTriMeshData[f]);
}




#pragma mark -

/******************** DO FENCE COLLISION **************************/

void DoFenceCollision(ObjNode *theNode)
{
float			fromX,fromZ,toX,toZ;
long			f,numFenceSegments,i,numReScans;
float			segFromX,segFromZ,segToX,segToZ;
OGLPoint3D		*nubs;
Boolean			intersected;
float			intersectX,intersectZ;
OGLVector2D		lineNormal;
float			radius;
float			oldX,oldZ,newX,newZ;


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

					/* DO BOUNDING BOX CHECK FOR QUICK ELIMINATION */

			if (((oldX+radius) < segFromX) &&					// left of segX?
				((oldX+radius) < segToX) &&
				((newX+radius) < segFromX) &&
				((newX+radius) < segToX))
				continue;

			if (((oldX-radius) > segFromX) &&					// right of segX?
				((oldX-radius) > segToX) &&
				((newX-radius) > segFromX) &&
				((newX-radius) > segToX))
				continue;

			if (((oldZ+radius) < segFromZ) &&
				((oldZ+radius) < segToZ) &&
				((newZ+radius) < segFromZ) &&
				((newZ+radius) < segToZ))
				continue;

			if (((oldZ-radius) > segFromZ) &&
				((oldZ-radius) > segToZ) &&
				((newZ-radius) > segFromZ) &&
				((newZ-radius) > segToZ))
				continue;


					/* CALC NORMAL TO THE LINE */
					//
					// We need to find the point on the bounding sphere which is closest to the line
					// in order to do good collision checks
					//
			lineNormal.x = oldX - segFromX;								// calc normalized vector from ref pt. to section endpoint 0
			lineNormal.y = oldZ - segFromZ;
			OGLVector2D_Normalize(&lineNormal, &lineNormal);
			float cross = OGLVector2D_Cross(&gFenceList[f].sectionVectors[i], &lineNormal);	// calc cross product to determine which side we're on

			if (cross < 0.0f)
			{
				lineNormal.x = -gFenceList[f].sectionNormals[i].x;		// on the other side, so flip vector
				lineNormal.y = -gFenceList[f].sectionNormals[i].y;
			}
			else
			{
				lineNormal = gFenceList[f].sectionNormals[i];			// use pre-calculated vector
			}



					/* CALC FROM-TO POINTS OF MOTION */

			fromX = oldX; // - (lineNormal.x * radius);
			fromZ = oldZ; // - (lineNormal.y * radius);
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
					i = -1;							// reset segment index to scan all again (reset to -1 because for loop will auto-inc to 0 for us)
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



/******************** DOES LINE SEGMENT HIT FENCE **************************/

Boolean DoesLineSegmentHitFence(OGLPoint3D	*pt1, OGLPoint3D  *pt2, OGLPoint3D *outHitPt)
{
int			f,numFenceSegments,i;
float			segFromX,segFromZ,segToX,segToZ;
OGLPoint3D		*nubs;
Boolean			intersected;
float			intersectX,intersectZ;


			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/

	for (f = 0; f < gNumFences; f++)
	{

		/* QUICK CHECK TO SEE IF ENDPOINTS ARE OUTSIDE OF FENCE'S BBOX */

		if ((pt1->x < gFenceList[f].bBox.min.x) && (pt2->x < gFenceList[f].bBox.min.x))
			continue;
		if ((pt1->x > gFenceList[f].bBox.max.x) && (pt2->x > gFenceList[f].bBox.min.x))
			continue;

		if ((pt1->z < gFenceList[f].bBox.min.z) && (pt2->z < gFenceList[f].bBox.min.z))
			continue;
		if ((pt1->z > gFenceList[f].bBox.max.z) && (pt2->z > gFenceList[f].bBox.min.z))
			continue;


		nubs = gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence


				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/

		for (i = 0; i < numFenceSegments; i++)
		{
					/* GET LINE SEG ENDPOINTS */

			segFromX = nubs[i].x;
			segFromZ = nubs[i].z;
			segToX = nubs[i+1].x;
			segToZ = nubs[i+1].z;

					/* DO BOUNDING BOX CHECK FOR QUICK ELIMINATION */

			if ((pt1->x < segFromX) &&					// left of segX?
				(pt1->x < segToX) &&
				(pt2->x < segFromX) &&
				(pt2->x < segToX))
				continue;

			if ((pt1->x > segFromZ) &&					// right of segX?
				(pt1->x > segToZ) &&
				(pt2->x > segFromZ) &&
				(pt2->x > segToZ))
				continue;

			if ((pt1->z < segFromZ) &&
				(pt1->z < segToZ) &&
				(pt2->z < segFromZ) &&
				(pt2->z < segToZ))
				continue;

			if ((pt1->z > segFromZ) &&
				(pt1->z > segToZ) &&
				(pt2->z > segFromZ) &&
				(pt2->z > segToZ))
				continue;




					/* SEE IF THE LINES INTERSECT */

			intersected = IntersectLineSegments(pt1->x,  pt1->z, pt2->x, pt2->z,
						                     segFromX, segFromZ, segToX, segToZ,
				                             &intersectX, &intersectZ);

			if (intersected)
			{
				OGLVector3D v;
				float	intersectY, fenceTopY;

					/* SEE IF PASSED OVER THE FENCE */

				float lenOfLine			= CalcDistance(pt1->x, pt1->z, pt2->x, pt2->z);
				float distToIntersect	= CalcDistance(pt1->x, pt1->z, intersectX, intersectZ);
				float ratio				= distToIntersect / lenOfLine;

				OGLPoint3D_Subtract(pt2, pt1, &v);			// calc line ray vector
				OGLVector3D_Normalize(&v, &v);

				intersectY = v.y * ratio * lenOfLine;		// calc y coord at intersection

				fenceTopY = GetTerrainY(intersectX, intersectZ) + gFenceHeight[gFenceList[f].type];

				if (intersectY <= fenceTopY)				// is it below the top?
				{
					if (outHitPt)							// return hit pt
					{
						outHitPt->x = intersectX;
						outHitPt->y = fenceTopY;			// set Y coord to top of fence
						outHitPt->z = intersectZ;
					}
					return(true);
				}
			}
		} // for i
	}

	return(false);
}



