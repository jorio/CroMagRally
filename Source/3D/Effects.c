/****************************/
/*   	EFFECTS.C		    */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "mobjtypes.h"
#include "ogl_support.h"
#include "effects.h"
#include "3dmath.h"
#include "sound2.h"
#include "collision.h"
#include "player.h"
#include "bg3d.h"
#include "sobjtypes.h"
#include "metaobjects.h"
#include "sprites.h"
#include "terrain.h"
#include "triggers.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLPoint3D			gCoord;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLVector3D			gDelta;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	FSSpec		gDataSpec;
extern	int			gCurrentSplitScreenPane;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	OGLVector3D		gRecentTerrainNormal;
extern	PlayerInfoType	gPlayerInfo[];
extern	short			gNumTotalPlayers;
extern	OGLPoint3D		gEarCoords[];


/****************************/
/*    PROTOTYPES            */
/****************************/


static void MoveBlast(ObjNode *theNode);
static void MoveDustPuff(ObjNode *theNode);

static void MoveRipple(ObjNode *theNode);
static void DeleteParticleGroup(long groupNum);

static void MoveShockwave(ObjNode *theNode);
static void MoveConeBlast(ObjNode *theNode);

static void MakeSnowParticleGroup(void);
static void DrawParticleGroup(ObjNode *theNode, OGLSetupOutputType *setupInfo);
static void MoveSnowShockwave(ObjNode *theNode);

static void MoveLavaGenerator(ObjNode *theNode);
static void MakeLava(ObjNode *theNode, OGLPoint3D *coord);


static void MoveBubbleGenerator(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/



#define	BONEBOMB_BLAST_RADIUS		(TERRAIN_POLYGON_SIZE * 1.9f)
#define	FIRE_BLAST_RADIUS			(TERRAIN_POLYGON_SIZE * 1.5f)

#define	FIRE_TIMER	.05f
#define	SMOKE_TIMER	.07f

#define	BubbleTimer		SpecialF[0]

/*********************/
/*    VARIABLES      */
/*********************/

ParticleGroupType	*gParticleGroups[MAX_PARTICLE_GROUPS];

static float	gGravitoidDistBuffer[MAX_PARTICLES][MAX_PARTICLES];

NewParticleGroupDefType	gNewParticleGroupDef;

short			gNumActiveParticleGroups = 0;



/************************* INIT EFFECTS ***************************/

void InitEffects(OGLSetupOutputType *setupInfo)
{

	InitParticleSystem(setupInfo);

		/* SET TEXTURE WRAPPING & BLENDING MODES ON SOME MATERIALS */

	BG3D_SetContainerMaterialFlags(MODEL_GROUP_WEAPONS, WEAPONS_ObjType_ConeBlast, BG3D_MATERIALFLAG_CLAMP_V);

	BG3D_SetContainerMaterialFlags(MODEL_GROUP_WEAPONS, WEAPONS_ObjType_RomanCandleBullet, BG3D_MATERIALFLAG_ALWAYSBLEND);
	BG3D_SetContainerMaterialFlags(MODEL_GROUP_WEAPONS, WEAPONS_ObjType_FreezeBullet, BG3D_MATERIALFLAG_ALWAYSBLEND);


			/* SET SPRITE BLENDING FLAGS */

	BlendASprite(SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_Splash);



}


#pragma mark -


/************************* MAKE RIPPLE *********************************/

#if 0
ObjNode *MakeRipple(float x, float y, float z, float startScale)
{
ObjNode	*newObj;

	gNewObjectDefinition.group = GLOBAL1_MGroupNum_Ripple;
	gNewObjectDefinition.type = GLOBAL1_MObjType_Ripple;
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags 	= STATUS_BIT_NOZWRITES|STATUS_BIT_NOFOG|STATUS_BIT_GLOW|STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 	= SLOT_OF_DUMB+2;
	gNewObjectDefinition.moveCall = MoveRipple;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = startScale;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(nil);

	newObj->Health = .8;										// transparency value

	MakeObjectTransparent(newObj, newObj->Health);

	return(newObj);
}



/******************** MOVE RIPPLE ************************/

static void MoveRipple(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Health -= fps * .8f;
	if (theNode->Health < 0)
	{
		DeleteObject(theNode);
		return;
	}

	MakeObjectTransparent(theNode, theNode->Health);

	theNode->Scale.x += fps*6.0f;
	theNode->Scale.y += fps*6.0f;
	theNode->Scale.z += fps*6.0f;

	UpdateObjectTransforms(theNode);
}
#endif

//=============================================================================================================
//=============================================================================================================
//=============================================================================================================

#pragma mark -

/************************ INIT PARTICLE SYSTEM **************************/

void InitParticleSystem(OGLSetupOutputType *setupInfo)
{
short	i;
FSSpec	spec;
ObjNode	*obj;


			/* INIT GROUP ARRAY */

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
		gParticleGroups[i] = nil;

	gNumActiveParticleGroups = 0;



			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:particle.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTICLES, setupInfo);

	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);


		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE PARTICLE DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The particles need to be drawn after the fences object, but before any sprite or font objects.
		//

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= PARTICLE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 		= 1;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES;

	obj = MakeNewObject(&gNewObjectDefinition);
	obj->CustomDrawFunction = DrawParticleGroup;

}


/******************** DISPOSE PARTICLE SYSTEM **********************/

void DisposeParticleSystem(void)
{
	DisposeSpriteGroup(SPRITE_GROUP_PARTICLES);
}


/******************** DELETE ALL PARTICLE GROUPS *********************/

void DeleteAllParticleGroups(void)
{
long	i;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		DeleteParticleGroup(i);
	}
}


/******************* DELETE PARTICLE GROUP ***********************/

static void DeleteParticleGroup(long groupNum)
{
	if (gParticleGroups[groupNum])
	{
			/* NUKE GEOMETRY DATA */

		MO_DisposeObjectReference(gParticleGroups[groupNum]->geometryObj);


				/* NUKE GROUP ITSELF */

		SafeDisposePtr((Ptr)gParticleGroups[groupNum]);
		gParticleGroups[groupNum] = nil;

		gNumActiveParticleGroups--;
	}
}


#pragma mark -


/********************** NEW PARTICLE GROUP *************************/
//
// INPUT:	type 	=	group type to create
//
// OUTPUT:	group ID#
//

short NewParticleGroup(NewParticleGroupDefType *def)
{
short					p,i,j,k;
OGLTextureCoord			*uv;
MOVertexArrayData 		vertexArrayData;
MOTriangleIndecies		*t;


			/*************************/
			/* SCAN FOR A FREE GROUP */
			/*************************/

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (gParticleGroups[i] == nil)
		{
				/* ALLOCATE NEW GROUP */

			gParticleGroups[i] = (ParticleGroupType *)AllocPtr(sizeof(ParticleGroupType));
			if (gParticleGroups[i] == nil)
				return(-1);									// out of memory


				/* INITIALIZE THE GROUP */

			gParticleGroups[i]->type = def->type;						// set type
			for (p = 0; p < MAX_PARTICLES; p++)						// mark all unused
				gParticleGroups[i]->isUsed[p] = false;

			gParticleGroups[i]->flags 				= def->flags;
			gParticleGroups[i]->gravity 			= def->gravity;
			gParticleGroups[i]->magnetism 			= def->magnetism;
			gParticleGroups[i]->baseScale 			= def->baseScale;
			gParticleGroups[i]->decayRate 			= def->decayRate;
			gParticleGroups[i]->fadeRate 			= def->fadeRate;
			gParticleGroups[i]->magicNum 			= def->magicNum;
			gParticleGroups[i]->particleTextureNum 	= def->particleTextureNum;

			gParticleGroups[i]->srcBlend 			= def->srcBlend;
			gParticleGroups[i]->dstBlend 			= def->dstBlend;

				/*****************************/
				/* INIT THE GROUP'S GEOMETRY */
				/*****************************/

					/* SET THE DATA */

			vertexArrayData.numMaterials 	= 1;
			vertexArrayData.materials[0]	= gSpriteGroupList[SPRITE_GROUP_PARTICLES][def->particleTextureNum].materialObject;	// set illegal ref because it is made legit below

			vertexArrayData.numPoints 		= 0;
			vertexArrayData.numTriangles 	= 0;
			vertexArrayData.points 			= (OGLPoint3D *)AllocPtr(sizeof(OGLPoint3D) * MAX_PARTICLES * 4);
			vertexArrayData.normals 		= nil;
			vertexArrayData.uvs		 		= (OGLTextureCoord *)AllocPtr(sizeof(OGLTextureCoord) * MAX_PARTICLES * 4);
			vertexArrayData.colorsByte 		= (OGLColorRGBA_Byte *)AllocPtr(sizeof(OGLColorRGBA_Byte) * MAX_PARTICLES * 4);
			vertexArrayData.colorsFloat		= nil;
			vertexArrayData.triangles		= (MOTriangleIndecies *)AllocPtr(sizeof(MOTriangleIndecies) * MAX_PARTICLES * 2);


					/* INIT UV ARRAYS */

			uv = vertexArrayData.uvs;
			for (j=0; j < (MAX_PARTICLES*4); j+=4)
			{
				uv[j].u = 0;									// upper left
				uv[j].v = 1;
				uv[j+1].u = 0;									// lower left
				uv[j+1].v = 0;
				uv[j+2].u = 1;									// lower right
				uv[j+2].v = 0;
				uv[j+3].u = 1;									// upper right
				uv[j+3].v = 1;
			}

					/* INIT TRIANGLE ARRAYS */

			t = vertexArrayData.triangles;
			for (j = k = 0; j < (MAX_PARTICLES*2); j+=2, k+=4)
			{
				t[j].vertexIndices[0] = k;							// triangle A
				t[j].vertexIndices[1] = k+1;
				t[j].vertexIndices[2] = k+2;

				t[j+1].vertexIndices[0] = k;							// triangle B
				t[j+1].vertexIndices[1] = k+2;
				t[j+1].vertexIndices[2] = k+3;
			}


				/* CREATE NEW GEOMETRY OBJECT */

			gParticleGroups[i]->geometryObj = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &vertexArrayData);

			gNumActiveParticleGroups++;

			return(i);
		}
	}

			/* NOTHING FREE */

//	DoFatalAlert("NewParticleGroup: no free groups!");
	return(-1);
}


/******************** ADD PARTICLE TO GROUP **********************/
//
// Returns true if particle group was invalid or is full.
//

Boolean AddParticleToGroup(NewParticleDefType *def)
{
short	p,group;

	group = def->groupNum;

	if ((group < 0) || (group >= MAX_PARTICLE_GROUPS))
		DoFatalAlert("AddParticleToGroup: illegal group #");

	if (gParticleGroups[group] == nil)
	{
		return(true);
	}


			/* SCAN FOR FREE SLOT */

	for (p = 0; p < MAX_PARTICLES; p++)
	{
		if (!gParticleGroups[group]->isUsed[p])
			goto got_it;
	}

			/* NO FREE SLOTS */

	return(true);


			/* INIT PARAMETERS */
got_it:
	gParticleGroups[group]->alpha[p] = def->alpha;
	gParticleGroups[group]->scale[p] = def->scale;
	gParticleGroups[group]->coord[p] = *def->where;
	gParticleGroups[group]->delta[p] = *def->delta;
	gParticleGroups[group]->rotZ[p] = def->rotZ;
	gParticleGroups[group]->rotDZ[p] = def->rotDZ;
	gParticleGroups[group]->isUsed[p] = true;

	return(false);
}


/****************** MOVE PARTICLE GROUPS *********************/

void MoveParticleGroups(void)
{
Byte		flags;
long		i,n,p,j;
float		fps = gFramesPerSecondFrac;
float		y,baseScale,oneOverBaseScaleSquared,gravity;
float		decayRate,magnetism,fadeRate;
OGLPoint3D	*coord;
OGLVector3D	*delta;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (gParticleGroups[i])
		{
			baseScale 	= gParticleGroups[i]->baseScale;					// get base scale
			oneOverBaseScaleSquared = 1.0f/(baseScale*baseScale);
			gravity 	= gParticleGroups[i]->gravity;						// get gravity
			decayRate 	= gParticleGroups[i]->decayRate;					// get decay rate
			fadeRate 	= gParticleGroups[i]->fadeRate;						// get fade rate
			magnetism 	= gParticleGroups[i]->magnetism;					// get magnetism
			flags 		= gParticleGroups[i]->flags;

			n = 0;															// init counter
			for (p = 0; p < MAX_PARTICLES; p++)
			{
				if (!gParticleGroups[i]->isUsed[p])							// make sure this particle is used
					continue;

				n++;														// inc counter
				delta = &gParticleGroups[i]->delta[p];						// get ptr to deltas
				coord = &gParticleGroups[i]->coord[p];						// get ptr to coords

							/* ADD GRAVITY */

				delta->y -= gravity * fps;									// add gravity


						/* DO ROTATION */

				gParticleGroups[i]->rotZ[p] += gParticleGroups[i]->rotDZ[p] * fps;




				switch(gParticleGroups[i]->type)
				{
							/* FALLING SPARKS */

					case	PARTICLE_TYPE_FALLINGSPARKS:
							coord->x += delta->x * fps;						// move it
							coord->y += delta->y * fps;
							coord->z += delta->z * fps;
							break;


							/* GRAVITOIDS */
							//
							// Every particle has gravity pull on other particle
							//

					case	PARTICLE_TYPE_GRAVITOIDS:
							for (j = MAX_PARTICLES-1; j >= 0; j--)
							{
								float		dist,temp,x,z;
								OGLVector3D	v;

								if (p==j)									// dont check against self
									continue;
								if (!gParticleGroups[i]->isUsed[j])			// make sure this particle is used
									continue;

								x = gParticleGroups[i]->coord[j].x;
								y = gParticleGroups[i]->coord[j].y;
								z = gParticleGroups[i]->coord[j].z;

										/* calc 1/(dist2) */

								if (i < j)									// see if calc or get from buffer
								{
									temp = coord->x - x;					// dx squared
									dist = temp*temp;
									temp = coord->y - y;					// dy squared
									dist += temp*temp;
									temp = coord->z - z;					// dz squared
									dist += temp*temp;

									dist = sqrt(dist);						// 1/dist2
									if (dist != 0.0f)
										dist = 1.0f / (dist*dist);

									if (dist > oneOverBaseScaleSquared)		// adjust if closer than radius
										dist = oneOverBaseScaleSquared;

									gGravitoidDistBuffer[i][j] = dist;		// remember it
								}
								else
								{
									dist = gGravitoidDistBuffer[j][i];		// use from buffer
								}

											/* calc vector to particle */

								if (dist != 0.0f)
								{
									x = x - coord->x;
									y = y - coord->y;
									z = z - coord->z;
									FastNormalizeVector(x, y, z, &v);
								}
								else
								{
									v.x = v.y = v.z = 0;
								}

								delta->x += v.x * (dist * magnetism * fps);		// apply gravity to particle
								delta->y += v.y * (dist * magnetism * fps);
								delta->z += v.z * (dist * magnetism * fps);
							}

							coord->x += delta->x * fps;						// move it
							coord->y += delta->y * fps;
							coord->z += delta->z * fps;
							break;
				}

				/*****************/
				/* SEE IF BOUNCE */
				/*****************/

				if (!(flags & PARTICLE_FLAGS_DONTCHECKGROUND))
				{
					y = GetTerrainY(coord->x, coord->z)+10.0f;					// get terrain coord at particle x/z

					if (flags & PARTICLE_FLAGS_BOUNCE)
					{
						if (delta->y < 0.0f)									// if moving down, see if hit floor
						{
							if (coord->y < y)
							{
								coord->y = y;
								delta->y *= -.4f;

								delta->x += gRecentTerrainNormal.x * 300.0f;	// reflect off of surface
								delta->z += gRecentTerrainNormal.z * 300.0f;
							}
						}
					}

					/***************/
					/* SEE IF GONE */
					/***************/

					else
					{
						if (coord->y < y)									// if hit floor then nuke particle
						{
							gParticleGroups[i]->isUsed[p] = false;
						}
					}
				}


					/* DO SCALE */

				gParticleGroups[i]->scale[p] -= decayRate * fps;			// shrink it
				if (gParticleGroups[i]->scale[p] <= 0.0f)					// see if gone
					gParticleGroups[i]->isUsed[p] = false;

					/* DO FADE */

				gParticleGroups[i]->alpha[p] -= fadeRate * fps;				// fade it
				if (gParticleGroups[i]->alpha[p] <= 0.0f)					// see if gone
					gParticleGroups[i]->isUsed[p] = false;


			}

				/* SEE IF GROUP WAS EMPTY, THEN DELETE */

			if (n == 0)
			{
				DeleteParticleGroup(i);
			}
		}
	}
}


/**************** DRAW PARTICLE GROUPS *********************/

static void DrawParticleGroup(ObjNode *theNode, OGLSetupOutputType *setupInfo)
{
float				scale,baseScale;
long				g,p,n,i;
OGLColorRGBA_Byte	*vertexColors;
MOVertexArrayData	*geoData;
OGLPoint3D		v[4],*camCoords,*coord;
OGLMatrix4x4	m;
static const OGLVector3D up = {0,1,0};
OGLBoundingBox	bbox;

				/* SETUP ENVIRONTMENT */

	OGL_PushState();

	glEnable(GL_BLEND);
	glColor4f(1,1,1,1);										// full white & alpha to start with

	camCoords = &gGameViewInfoPtr->cameraPlacement[gCurrentSplitScreenPane].cameraLocation;

	for (g = 0; g < MAX_PARTICLE_GROUPS; g++)
	{
		float	minX,minY,minZ,maxX,maxY,maxZ;
		int		temp;

		if (gParticleGroups[g])
		{
			geoData = &gParticleGroups[g]->geometryObj->objectData;			// get pointer to geometry object data
			vertexColors = geoData->colorsByte;								// get pointer to vertex color array
			baseScale = gParticleGroups[g]->baseScale;						// get base scale

					/********************************/
					/* ADD ALL PARTICLES TO TRIMESH */
					/********************************/

			minX = minY = minZ = 100000000;									// init bbox
			maxX = maxY = maxZ = -minX;

			for (p = n = 0; p < MAX_PARTICLES; p++)
			{
				float			rot;

				if (!gParticleGroups[g]->isUsed[p])							// make sure this particle is used
					continue;

					/* TRANSFORM THIS PARTICLE'S VERTICES & ADD TO TRIMESH */

				coord = &gParticleGroups[g]->coord[p];
				SetLookAtMatrixAndTranslate(&m, &up, coord, camCoords);		// aim at camera & translate

				rot = gParticleGroups[g]->rotZ[p];							// get z rotation
				if (rot != 0.0f)											// see if need to apply rotation matrix
				{
					OGLMatrix4x4	rm;

					OGLMatrix4x4_SetRotate_Z(&rm, rot);
					OGLMatrix4x4_Multiply(&rm, &m, &m);
				}

						/* CREATE VERTEX DATA */

				scale = gParticleGroups[g]->scale[p];

				v[0].x = -scale*baseScale;
				v[0].y = scale*baseScale;
				v[0].z = 0;

				v[1].x = -scale*baseScale;
				v[1].y = -scale*baseScale;
				v[1].z = 0;

				v[2].x = scale*baseScale;
				v[2].y = -scale*baseScale;
				v[2].z = 0;

				v[3].x = scale*baseScale;
				v[3].y = scale*baseScale;
				v[3].z = 0;

						/* TRANSFORM VERTS TO FINAL POSITION */

				OGLPoint3D_TransformArray(&v[0], &m, &geoData->points[n*4], 4);


							/* UPDATE BBOX */

				for (i = 0; i < 4; i++)
				{
					if (geoData->points[n*4+i].x < minX)
						minX = geoData->points[n*4+i].x;
					if (geoData->points[n*4+i].x > maxX)
						maxX = geoData->points[n*4+i].x;
					if (geoData->points[n*4+i].y < minY)
						minY = geoData->points[n*4+i].y;
					if (geoData->points[n*4+i].y > maxY)
						maxY = geoData->points[n*4+i].y;
					if (geoData->points[n*4+i].z < minZ)
						minZ = geoData->points[n*4+i].z;
					if (geoData->points[n*4+i].z > maxZ)
						maxZ = geoData->points[n*4+i].z;
				}

					/* UPDATE COLOR/TRANSPARENCY */

				temp = n*4;
				for (i = temp; i < (temp+4); i++)
				{
					vertexColors[i].r =
					vertexColors[i].g =
					vertexColors[i].b = 0xff;
					vertexColors[i].a = gParticleGroups[g]->alpha[p] * 255.0f;		// set transparency alpha
				}

				n++;											// inc particle count
			}

			if (n == 0)											// if no particles, then skip
				continue;

				/* UPDATE FINAL VALUES */

			geoData->numTriangles = n*2;
			geoData->numPoints = n*4;

			bbox.min.x = minX;									// build bbox for culling test
			bbox.min.y = minY;
			bbox.min.z = minZ;
			bbox.max.x = maxX;
			bbox.max.y = maxY;
			bbox.max.z = maxZ;

			if (OGL_IsBBoxVisible(&bbox))						// do cull test on it
			{
					/* DRAW IT */

				glBlendFunc(gParticleGroups[g]->srcBlend, gParticleGroups[g]->dstBlend);		// set blending mode
				MO_DrawObject(gParticleGroups[g]->geometryObj, setupInfo);						// draw geometry
			}
		}
	}

			/* RESTORE MODES */

	glColor4f(1,1,1,1);										// reset this
	OGL_PopState();

}


/**************** VERIFY PARTICLE GROUP MAGIC NUM ******************/

Boolean VerifyParticleGroupMagicNum(short group, uint32_t magicNum)
{
	if (gParticleGroups[group] == nil)
		return(false);

	if (gParticleGroups[group]->magicNum != magicNum)
		return(false);

	return(true);
}


/************* PARTICLE HIT OBJECT *******************/
//
// INPUT:	inFlags = flags to check particle types against
//

Boolean ParticleHitObject(ObjNode *theNode, uint16_t inFlags)
{
int		i,p;
uint16_t	flags;
OGLPoint3D	*coord;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (!gParticleGroups[i])									// see if group active
			continue;

		if (inFlags)												// see if check flags
		{
			flags = gParticleGroups[i]->flags;
			if (!(inFlags & flags))
				continue;
		}

		for (p = 0; p < MAX_PARTICLES; p++)
		{
			if (!gParticleGroups[i]->isUsed[p])						// make sure this particle is used
				continue;

			if (gParticleGroups[i]->alpha[p] < .4f)				// if particle is too decayed, then skip
				continue;

			coord = &gParticleGroups[i]->coord[p];					// get ptr to coords
			if (DoSimpleBoxCollisionAgainstObject(coord->y+40.0f,coord->y-40.0f,
												coord->x-40.0f, coord->x+40.0f,
												coord->z+40.0f, coord->z-40.0f,
												theNode))
			{
				return(true);
			}
		}
	}
	return(false);
}

#pragma mark -

/********************* MAKE PUFF ***********************/

void MakePuff(float x, float y, float z)
{
long					pg,i,n;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;

	n = 15;

			/* white sparks */

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_GRAVITOIDS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= -80;
	gNewParticleGroupDef.magnetism				= 200000;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				=  -1.9;
	gNewParticleGroupDef.fadeRate				= .3;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < n; i++)
		{
			pt.x = x + (RandomFloat()-.5f) * 130.0f;
			pt.y = y + RandomFloat() * 40.0f;
			pt.z = z + (RandomFloat()-.5f) * 130.0f;

			delta.x = (RandomFloat()-.5f) * 300.0f;
			delta.y = RandomFloat() * 400.0f;
			delta.z = (RandomFloat()-.5f) * 300.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= RandomFloat() * PI2;
			newParticleDef.rotDZ		= (RandomFloat()-.5f) * 4.0f;
			newParticleDef.alpha		= FULL_ALPHA;
			AddParticleToGroup(&newParticleDef);
		}
	}
}


/********************* MAKE SPARK EXPLOSION ***********************/

void MakeSparkExplosion(float x, float y, float z, float force, short sparkTexture)
{
long					pg,i,n;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;

	n = force * .2f;

			/* white sparks */

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 200;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= .5;
	gNewParticleGroupDef.particleTextureNum		= sparkTexture;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < n; i++)
		{
			pt.x = x + RandomFloat2() * 130.0f;
			pt.y = y + RandomFloat() * 40.0f;
			pt.z = z + RandomFloat2() * 130.0f;

			delta.x = RandomFloat2() * force;
			delta.y = RandomFloat2() * force;
			delta.z = RandomFloat2() * force;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}
}


#pragma mark -


/************** MAKE BOMB EXPLOSION *********************/

void MakeBombExplosion(ObjNode *theBomb, float x, float z, OGLVector3D *delta)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					y;
float					dx,dz;
OGLPoint3D				where;
ObjNode					*whoThrew;
short					p;

	where.x = x;
	where.z = z;
	where.y = GetTerrainY(x,z);

		/*********************/
		/* FIRST MAKE SPARKS */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 30;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= .6;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = where.x;
		y = where.y;
		z = where.z;
		dx = delta->x * .2f;
		dz = delta->z * .2f;


		for (i = 0; i < 100; i++)
		{
			pt.x = x + (RandomFloat()-.5f) * 200.0f;
			pt.y = y + RandomFloat() * 60.0f;
			pt.z = z + (RandomFloat()-.5f) * 200.0f;

			d.y = RandomFloat() * 1000.0f;
			d.x = (RandomFloat()-.5f) * d.y * 3.0f + dx;
			d.z = (RandomFloat()-.5f) * d.y * 3.0f + dz;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}

		/*******************/
		/* MAKE SHOCKWAVES */
		/*******************/

	MakeShockwave(x,y+100.0f,z);
	MakeConeBlast(x, y, z);

	PlayEffect_Parms3D(EFFECT_BOOM, &where, NORMAL_CHANNEL_RATE, 6);


		/* SEE IF IT HIT ANY CARS */

	if (theBomb)
	{
		whoThrew = (ObjNode *)theBomb->WhoThrew;
		if (whoThrew == nil)
			p = -1;
		else
			p = whoThrew->PlayerNum;
	}
	else
		p = -1;

	BlastCars(p, x, y, z, BONEBOMB_BLAST_RADIUS);

}


/******************** MAKE SHOCKWAVE *************************/

void MakeShockwave(float x, float y, float z)
{
ObjNode					*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_Shockwave;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+30;
	gNewObjectDefinition.moveCall 	= MoveShockwave;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ColorFilter.a = 1.0;

	newObj->SpecialF[0] = RandomFloat() * 5.0f + 4.0f;		// random scaling
	newObj->SpecialF[1] = RandomFloat() * 2.0f + 1.0f;		// random fading
}


/***************** MOVE SHOCKWAVE ***********************/

static void MoveShockwave(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * theNode->SpecialF[0];

	theNode->ColorFilter.a -= fps * theNode->SpecialF[1];
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);
}



/******************** MAKE CONEBLAST *************************/

void MakeConeBlast(float x, float y, float z)
{
ObjNode					*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_ConeBlast;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+11;
	gNewObjectDefinition.moveCall 	= MoveConeBlast;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ColorFilter.a = 1.0;

}


/***************** MOVE CONEBLAST ***********************/

static void MoveConeBlast(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * 7.0f;

	theNode->ColorFilter.a -= fps * 2.0f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);
}


#pragma mark -

/********************* MAKE SPLASH ***********************/

void MakeSplash(float x, float y, float z)
{
long	pg,i;
OGLVector3D	delta;
OGLPoint3D	pt;
NewParticleDefType		newParticleDef;


	pt.y = y;

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= 0;
	gNewParticleGroupDef.gravity				= 800;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 40;
	gNewParticleGroupDef.decayRate				=  -.6;
	gNewParticleGroupDef.fadeRate				= .8;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Splash;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;


	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 30; i++)
		{
			pt.x = x + (RandomFloat()-.5f) * 130.0f;
			pt.z = z + (RandomFloat()-.5f) * 130.0f;

			delta.x = (RandomFloat()-.5f) * 400.0f;
			delta.y = 500.0f + RandomFloat() * 300.0f;
			delta.z = (RandomFloat()-.5f) * 400.0f;

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;
			AddParticleToGroup(&newParticleDef);
		}
	}

			/* PLAY SPLASH SOUND */

	pt.x = x;
	pt.z = z;
	PlayEffect_Parms3D(EFFECT_SPLASH, &pt, NORMAL_CHANNEL_RATE, 3);
}


#pragma mark -


/*********************** MAKE SNOW ******************************/

void MakeSnow(void)
{
long				i,n;
OGLVector3D			delta;
OGLPoint3D			pt;
NewParticleDefType	newParticleDef;
float				r,d;
float				dx,dz;
short				p;

	if (gFramesPerSecond < 12.0f)									// help us out if speed is really bad
		return;

	if (gNumActiveParticleGroups > (MAX_PARTICLE_GROUPS - 20))		// don't fill up all of the particle groups!
		return;

	for (p = 0; p < gNumTotalPlayers; p++)
	{

			/* CHECK IF SNOW NOW */

		gPlayerInfo[p].snowTimer -= gFramesPerSecondFrac;
		if (gPlayerInfo[p].snowTimer > 0.0f)
			return;
		gPlayerInfo[p].snowTimer += .05f;

				/* GET GROUP */

		if (gPlayerInfo[p].snowParticleGroup == -1)									// if no group, then try to make one
		{
			gNewParticleGroupDef.magicNum				= 0;
			gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			gNewParticleGroupDef.flags					= 0;
			gNewParticleGroupDef.gravity				= 0;
			gNewParticleGroupDef.magnetism				= 0;
			gNewParticleGroupDef.baseScale				= 30;
			gNewParticleGroupDef.decayRate				= 0;
			gNewParticleGroupDef.fadeRate				= 0;
			gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_SnowFlakes;
			gNewParticleGroupDef.srcBlend				= GL_ONE;
			gNewParticleGroupDef.dstBlend				= GL_ONE;

			gPlayerInfo[p].snowParticleGroup = NewParticleGroup(&gNewParticleGroupDef);
		}
		if (gPlayerInfo[p].snowParticleGroup == -1)									// still no group, so bail
			return;


				/******************/
				/* MAKE PARTICLES */
				/******************/

		dx = gPlayerInfo[p].objNode->Delta.x;
		dz = gPlayerInfo[p].objNode->Delta.z;

		n = gFramesPerSecond * .5f;										// snow density is proportional to frame rate (so looks really nice on fast systems!)
		if (n > 25)
			n = 25;
		if (n  < 7)
			n = 7;

		for (i = 0; i < n; i++)
		{
			r = RandomFloat() * PI2;									// pick random rotation around camera
			d = 500.0f + (RandomFloat() * 15000.0f);						// random diameter

			pt = gPlayerInfo[p].camera.cameraLocation;					// get coord of camera

			pt.x += sin(r) * d;											// project to point around camera
			pt.z += cos(r) * d;
			pt.y += 2500.0f;											// start up

			delta.x = dx + RandomFloat2() * 200.0f;
			delta.y = -700.0f - RandomFloat() * 200.0f;
			delta.z = dz + RandomFloat2() * 200.0f;

			newParticleDef.groupNum		= gPlayerInfo[p].snowParticleGroup;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 20.0f;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
			{
				gPlayerInfo[p].snowParticleGroup = -1;								// this group is full
				return;
			}
		}
	}
}

/************** MAKE SNOW EXPLOSION *********************/

void MakeSnowExplosion(OGLPoint3D *where)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;


		/*************/
		/* MAKE SNOW */
		/*************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 18;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= .6;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_SnowFlakes;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = where->x;
		y = where->y;
		z = where->z;

		for (i = 0; i < 240; i++)
		{
			pt.x = x + RandomFloat2() * 200.0f;
			pt.y = y + RandomFloat2() * 200.0f;
			pt.z = z + RandomFloat2() * 200.0f;

			d.y = RandomFloat2() * 700.0f;
			d.x = RandomFloat2() * 700.0f;
			d.z = RandomFloat2() * 700.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= 1.0;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 10.0f;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}

		/******************/
		/* MAKE SHOCKWAVE */
		/******************/

	MakeSnowShockwave(x,y,z);

	PlayEffect_Parms3D(EFFECT_BOOM, where, NORMAL_CHANNEL_RATE, 4);


}

/******************** MAKE SNOW SHOCKWAVE *************************/

void MakeSnowShockwave(float x, float y, float z)
{
ObjNode					*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_WEAPONS;
	gNewObjectDefinition.type 		= WEAPONS_ObjType_SnowShockwave;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1000;
	gNewObjectDefinition.moveCall 	= MoveSnowShockwave;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 50.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ColorFilter.a = 1.0;
}


/***************** MOVE SNOW SHOCKWAVE ***********************/

static void MoveSnowShockwave(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;
float	d,x2,y2,z2,x,y,z;
ObjNode	*obj;

			/* DO FADING AND SCALING */

	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += 800.0f * fps;

	theNode->ColorFilter.a -= 1.0f * fps;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

			/***********************/
			/* SEE IF HIT ANY CARS */
			/***********************/

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;

			/* CHECK EACH CAR */

	for (i = 0; i < gNumTotalPlayers; i++)
	{
		x2 = gPlayerInfo[i].coord.x;
		y2 = gPlayerInfo[i].coord.y;
		z2 = gPlayerInfo[i].coord.z;

		d = CalcDistance3D(x, y, z, x2, y2, z2);						// calc dist to see if in blast zone
		if (d <= theNode->Scale.x)
		{
				/* FREEZE THIS PLAYER */

			gPlayerInfo[i].greasedTiresTimer = .5f;					// skid out
			gPlayerInfo[i].isPlaning = true;
			gPlayerInfo[i].frozenTimer = 5.0;						// freeze for n time
			gPlayerInfo[i].gasPedalDown	= false;
			gPlayerInfo[i].braking	= false;

			obj = gPlayerInfo[i].objNode;

			if (fabs(obj->DeltaRot.y) < 15.0f)							// if not spinning fast, then make it spin faster
			{
				obj->DeltaRot.y *= 3.0f;								// spin amplifier
			}

			while(obj)
			{
				obj->ColorFilter.r = .3;									// tint the car ice-blue
				obj->ColorFilter.g = .3;
				obj->ColorFilter.b = 1.0;
				obj->ColorFilter.a = .8;
				obj = obj->ChainNode;
			}

		}
	}


	UpdateObjectTransforms(theNode);
}


#pragma mark -


/****************** BURN FIRE ************************/

void BurnFire(ObjNode *theNode, float x, float y, float z, Boolean doSmoke, short particleType, float scale)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;


		/**************/
		/* MAKE SMOKE */
		/**************/

	if (doSmoke && (gFramesPerSecond > 15.0f))										// only do smoke if running at good frame rate
	{
		theNode->SmokeTimer -= fps;													// see if add smoke
		if (theNode->SmokeTimer <= 0.0f)
		{
			theNode->SmokeTimer += SMOKE_TIMER;										// reset timer

			particleGroup 	= theNode->SmokeParticleGroup;
			magicNum 		= theNode->SmokeParticleMagic;

			if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
			{

				theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num

				groupDef.magicNum				= magicNum;
				groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
				groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
				groupDef.gravity				= 0;
				groupDef.magnetism				= 0;
				groupDef.baseScale				= 20 * scale;
				groupDef.decayRate				=  -.2f;
				groupDef.fadeRate				= .2;
				groupDef.particleTextureNum		= PARTICLE_SObjType_BlackSmoke;
				groupDef.srcBlend				= GL_SRC_ALPHA;
				groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
				theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&groupDef);
			}

			if (particleGroup != -1)
			{
				for (i = 0; i < 3; i++)
				{
					p.x = x + RandomFloat2() * (40.0f * scale);
					p.y = y + 200.0f + RandomFloat() * (50.0f * scale);
					p.z = z + RandomFloat2() * (40.0f * scale);

					d.x = RandomFloat2() * (20.0f * scale);
					d.y = 150.0f + RandomFloat() * (40.0f * scale);
					d.z = RandomFloat2() * (20.0f * scale);

					newParticleDef.groupNum		= particleGroup;
					newParticleDef.where		= &p;
					newParticleDef.delta		= &d;
					newParticleDef.scale		= RandomFloat() + 1.0f;
					newParticleDef.rotZ			= RandomFloat() * PI2;
					newParticleDef.rotDZ		= RandomFloat2();
					newParticleDef.alpha		= .7;
					if (AddParticleToGroup(&newParticleDef))
					{
						theNode->SmokeParticleGroup = -1;
						break;
					}
				}
			}
		}
	}

		/*************/
		/* MAKE FIRE */
		/*************/

	theNode->FireTimer -= fps;													// see if add fire
	if (theNode->FireTimer <= 0.0f)
	{
		theNode->FireTimer += FIRE_TIMER;										// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= -200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 30.0f * scale;
			groupDef.decayRate				=  0;
			groupDef.fadeRate				= .8;
			groupDef.particleTextureNum		= particleType;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * (30.0f * scale);
				p.y = y + RandomFloat() * (50.0f * scale);
				p.z = z + RandomFloat2() * (30.0f * scale);

				d.x = RandomFloat2() * (50.0f * scale);
				d.y = 50.0f + RandomFloat() * (60.0f * scale);
				d.z = RandomFloat2() * (50.0f * scale);

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= 1.0;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}








/****************** MAKE BUBBLES ************************/

void MakeBubbles(ObjNode *theNode, OGLPoint3D *coord, float fadeRate, float scale)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
float				x,y,z;

	x = coord->x;
	y = coord->y;
	z = coord->z;


	particleGroup 	= theNode->ParticleGroup;
	magicNum 		= theNode->ParticleMagicNum;

	if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
	{
		theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

		groupDef.magicNum				= magicNum;
		groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
		groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
		groupDef.gravity				= 0;
		groupDef.magnetism				= 0;
		groupDef.baseScale				= 8.0f * scale;
		groupDef.decayRate				=  0;
		groupDef.fadeRate				= fadeRate;
		groupDef.particleTextureNum		= PARTICLE_SObjType_Bubbles;
		groupDef.srcBlend				= GL_SRC_ALPHA;
		groupDef.dstBlend				= GL_ONE;
		theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
	}

	if (particleGroup != -1)
	{
		for (i = 0; i < 2; i++)
		{
			p.x = x + RandomFloat2() * (20.0f);
			p.y = y + RandomFloat() * (20.0f);
			p.z = z + RandomFloat2() * (20.0f);

			d.x = 0;
			d.y = 250.0f + RandomFloat() * (190.0f);
			d.z = 0;

			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &p;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= RandomFloat()*PI2;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= 1.0;
			if (AddParticleToGroup(&newParticleDef))
			{
				theNode->ParticleGroup = -1;
				break;
			}
		}
	}
}

#pragma mark -

/************** MAKE FIRE EXPLOSION *********************/

void MakeFireExplosion(float x, float z, OGLVector3D *delta)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					y;
float					dx,dz;
OGLPoint3D				where;

	where.x = x;
	where.z = z;
	where.y = GetTerrainY(x,z);

		/*********************/
		/* FIRST MAKE FLAMES */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= -500;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 30;
	gNewParticleGroupDef.decayRate				=  -.4;
	gNewParticleGroupDef.fadeRate				= .6;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = where.x;
		y = where.y;
		z = where.z;
		dx = delta->x * .2f;
		dz = delta->z * .2f;


		for (i = 0; i < 100; i++)
		{
			pt.x = x + (RandomFloat()-.5f) * 200.0f;
			pt.y = y + RandomFloat() * 60.0f;
			pt.z = z + (RandomFloat()-.5f) * 200.0f;

			d.y = RandomFloat() * 1000.0f;
			d.x = (RandomFloat()-.5f) * d.y * 3.0f + dx;
			d.z = (RandomFloat()-.5f) * d.y * 3.0f + dz;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 10.0f;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}

		/*******************/
		/* MAKE SHOCKWAVES */
		/*******************/

	MakeConeBlast(x, y, z);
	PlayEffect_Parms3D(EFFECT_BOOM, &where, NORMAL_CHANNEL_RATE * 3/2, 3);


		/* SEE IF IT HIT ANY CARS */

	BlastCars(-1, x, y, z, FIRE_BLAST_RADIUS);

}


#pragma mark -


/************************* ADD BUBBLE GENERATOR *********************************/

Boolean AddBubbleGenerator(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveBubbleGenerator;
	newObj = MakeNewObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Flag[0] = MyRandomLong()&1;								// does this one make a sound?

	return(true);													// item was added
}


/************************ MOVE BUBBLE GENERATOR **********************/

static void MoveBubbleGenerator(ObjNode *theNode)
{
OGLPoint3D	pt;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


			/************************/
			/* UPDATE SOUND EFFECT */
			/************************/

	if (theNode->Flag[0])									// does this one make sound?
	{
		if (theNode->EffectChannel != -1)
		{
			pt.x = theNode->Coord.x;
			pt.y = gEarCoords[0].y;							// set to same y as ear for max volume fakeout
			pt.z = theNode->Coord.z;

			Update3DSoundChannel(EFFECT_BUBBLES, &theNode->EffectChannel, &pt);
		}
		else
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BUBBLES, &gCoord, NORMAL_CHANNEL_RATE, 1.0);
	}

			/********************/
			/* UPDATE PARTICLES */
			/********************/

	if (gFramesPerSecond < 14.0f)										// no bubbles if going really slow
		return;

	if (gNumActiveParticleGroups > (MAX_PARTICLE_GROUPS - 15))			// don't fill up all of the particle groups!
		return;


			/* STREAM BUBBLES */

	theNode->BubbleTimer -= gFramesPerSecondFrac;									// check bubble timer
	if (theNode->BubbleTimer <= 0.0f)
	{
		theNode->BubbleTimer += .1f + (RandomFloat() * .2f);

		MakeBubbles(theNode, &theNode->Coord, .03, 3.5);
	}


}


#pragma mark -


/************************* ADD LAVA GENERATOR *********************************/

Boolean AddLavaGenerator(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveLavaGenerator;
	newObj = MakeNewObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_AVOID;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_LAVA;
	SetObjectCollisionBounds(newObj, 400.0, 0, -250, 250, 250, -250);


	return(true);													// item was added
}


/************************ MOVE LAVA GENERATOR **********************/

static void MoveLavaGenerator(ObjNode *theNode)
{

			/* STREAM BUBBLES */

	theNode->BubbleTimer -= gFramesPerSecondFrac;									// check bubble timer
	if (theNode->BubbleTimer <= 0.0f)
	{
		theNode->BubbleTimer += .08f;

		MakeLava(theNode, &theNode->Coord);
	}
}

/************** DO TRIGGER - LAVA ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_Lava(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	p = whoNode->PlayerNum;
ObjNode	*obj;

	sideBits;

	gPlayerInfo[p].flamingTimer = 5;						// set aflame

		obj = whoNode;
		while(obj)
		{
			obj->ColorFilter.r = .7;						// tint the car red
			obj->ColorFilter.g = 0;
			obj->ColorFilter.b = 0;
			obj->ColorFilter.a = 1.0;
			obj = obj->ChainNode;
		}

	return(false);
}



/****************** MAKE LAVA ************************/

static void MakeLava(ObjNode *theNode, OGLPoint3D *coord)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
float				x,y,z;

	x = coord->x;
	y = coord->y;
	z = coord->z;


	particleGroup 	= theNode->ParticleGroup;
	magicNum 		= theNode->ParticleMagicNum;

	if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
	{
		theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

		groupDef.magicNum				= magicNum;
		groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
		groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
		groupDef.gravity				= 3000;
		groupDef.magnetism				= 0;
		groupDef.baseScale				= 80.0f;
		groupDef.decayRate				=  -.9;
		groupDef.fadeRate				= 1.0;
		groupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
		groupDef.srcBlend				= GL_SRC_ALPHA;
		groupDef.dstBlend				= GL_ONE;
		theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
	}

	if (particleGroup != -1)
	{
		p.y = y;

		for (i = 0; i < 4; i++)
		{
			p.x = x + RandomFloat2() * 300.0f;
			p.z = z + RandomFloat2() * 300.0f;

			d.x = RandomFloat2() * 200.0f ;
			d.y = 1250.0f + RandomFloat() * 600.0f;
			d.z = RandomFloat2() * 200.0f;

			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &p;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= RandomFloat()*PI2;
			newParticleDef.rotDZ		= RandomFloat2() * 10.0f;
			newParticleDef.alpha		= .5;
			if (AddParticleToGroup(&newParticleDef))
			{
				theNode->ParticleGroup = -1;
				break;
			}
		}
	}
}






