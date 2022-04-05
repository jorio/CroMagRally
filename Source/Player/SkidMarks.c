/****************************/
/*   	SKIDMARKS.C    		*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void CalcSkidSegmentPoints(float x, float z, float dx, float dz, OGLPoint3D *p1, OGLPoint3D *p2);
static int StartNewSkidMark(ObjNode *owner, short subID);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SKIDMARKS		60
#define	MAX_SKID_SEGMENTS	40								// max segments in skid

#define	SKID_START_ALPHA	.7f

#define	SKID_WIDTH			17.0f
#define	SKID_YOFF			6.0f


typedef struct
{
	Boolean		isActive;								// true if this one is active
	Boolean		isContinuance;							// continued from other skid

	ObjNode		*owner;									// who owns this skid
	short		subID;									// subID so owner can have multiple skids (ie. for each tire)

	int				numSegments;						// # segments in skid
	OGLPoint3D		points[MAX_SKID_SEGMENTS][2];		// each segment has 2 points
	OGLColorRGBA	color[MAX_SKID_SEGMENTS];			// each segment has a color+alpha value to fade

}SkidMarkType;


/*********************/
/*    VARIABLES      */
/*********************/


SkidMarkType	gSkidMarks[MAX_SKIDMARKS];


/*************************** INIT SKID MARKS **********************************/

void InitSkidMarks(void)
{
int		i;

	for (i = 0; i < MAX_SKIDMARKS; i++)
	{
		gSkidMarks[i].isActive = false;
	}
}


/*************************** START NEW SKIDMARK ******************************/
//
// INPUT:	x/z = coord of skid
//			dx/dz = direction of motion (not necessarily normalized)
//			owner = objNode to assign to skid
//
// OUTPUT:  skid #, or -1 if none
//

static int StartNewSkidMark(ObjNode *owner, short subID)
{
int		i;

			/* FIND A FREE SLOT */

	for (i = 0; i < MAX_SKIDMARKS; i++)
	{
		if (!gSkidMarks[i].isActive)
			goto got_it;

	}
	return(-1);

got_it:

		/**********************/
		/* CREATE 1ST SEGMENT */
		/**********************/

	gSkidMarks[i].isActive  = true;
	gSkidMarks[i].owner		= owner;
	gSkidMarks[i].subID		= subID;
	gSkidMarks[i].numSegments = 0;
	return(i);
}


/********************** DO SKIDMARK **************************/

void DoSkidMark(ObjNode *owner, short subID, float x, float z, float dx, float dz)
{
int		i,n,j;
short	p = owner->PlayerNum;

		/* SCAN FOR A SKIDMARK ALREADY IN PROGRESS */

	for (i = 0; i < MAX_SKIDMARKS; i++)
	{
		if (gSkidMarks[i].isActive)														// must be active
		{
			if ((gSkidMarks[i].owner == owner) && (gSkidMarks[i].subID == subID))		// owner must match
			{
				goto got_it;
			}
		}
	}

		/* NOTHING IN PROGRESS, SO START A NEW SKID */

	i = StartNewSkidMark(owner, subID);
	if (i == -1)
		return;

	gSkidMarks[i].isContinuance = false;


		/*******************************/
		/* ADD NEW SEGMENT TO THE SKID */
		/*******************************/

got_it:

			/* SEE IF THIS SKID IS FULL */

	if (gSkidMarks[i].numSegments >= MAX_SKID_SEGMENTS)
	{
			/* CREATE NEW SKID TO CONTINUE WITH */

		gSkidMarks[i].owner = nil;								// free the owner from this skid
		j = i;													// remember old skid
		i = StartNewSkidMark(owner, subID);						// make new skid
		if (i == -1)
			return;

		gSkidMarks[i].isContinuance = true;

		gSkidMarks[i].numSegments = 1;							// copy last seg from old to 1st seg of new

		gSkidMarks[i].points[0][0] = gSkidMarks[j].points[MAX_SKID_SEGMENTS-1][0];
		gSkidMarks[i].points[0][1] = gSkidMarks[j].points[MAX_SKID_SEGMENTS-1][1];
		gSkidMarks[i].color[0] = gSkidMarks[j].color[MAX_SKID_SEGMENTS-1];
	}


			/* ADD NEW SKID SEGMENT */

	n = gSkidMarks[i].numSegments++;							// one more segment added

	gSkidMarks[i].color[n].a = SKID_START_ALPHA;				// set alpha

	gSkidMarks[i].color[n].r = gPlayerInfo[p].skidColor.r;		// set rgb color
	gSkidMarks[i].color[n].g = gPlayerInfo[p].skidColor.g;
	gSkidMarks[i].color[n].b = gPlayerInfo[p].skidColor.b;

	CalcSkidSegmentPoints(x, z, dx, dz, &gSkidMarks[i].points[n][0], &gSkidMarks[i].points[n][1]);


}


/********************** STRETCH SKIDMARK **************************/
//
// Simply replaces the last segment in the skid with new values
//

void StretchSkidMark(ObjNode *owner, short subID, float x, float z, float dx, float dz)
{
int		i,n;

		/* FIND MATCHING SKID FOR OWNER */

	for (i = 0; i < MAX_SKIDMARKS; i++)
	{
		if (gSkidMarks[i].isActive)														// must be active
		{
			if ((gSkidMarks[i].owner == owner) && (gSkidMarks[i].subID == subID))		// owner must match
			{
				goto got_it;
			}
		}
	}
	return;


		/*********************/
		/* EDIT LAST SEGMENT */
		/*********************/

got_it:

	n = gSkidMarks[i].numSegments-1;
	CalcSkidSegmentPoints(x, z, dx, dz, &gSkidMarks[i].points[n][0], &gSkidMarks[i].points[n][1]);
}



/**************** CALC SKID SEGMENT POINTS **********************/

static void CalcSkidSegmentPoints(float x, float z, float dx, float dz, OGLPoint3D *p1, OGLPoint3D *p2)
{
OGLVector3D	v,side;
static const OGLVector3D up = {0,1,0};

		/* CALC SIDE VECTOR */

	FastNormalizeVector(dx, 0, dz, &v);								// calc normalized direction vector
	OGLVector3D_Cross(&v, &up, &side);								// cross product gives us perpendicular vector for side offset

	dx = side.x * SKID_WIDTH;
	dz = side.z * SKID_WIDTH;

	if ((side.x == 0.0f) && (side.z == 0.0f))
		SysBeep(0);	//----------

		/* CALC COORDS OF POINTS */

	p1->x = x + dx;
	p1->z = z + dz;
	p1->y = GetTerrainY(p1->x, p1->z) + SKID_YOFF;

	p2->x = x - dx;
	p2->z = z - dz;
	p2->y = GetTerrainY(p2->x, p2->z) + SKID_YOFF;
}


#pragma mark -

/*************************** DRAW SKID MARKS ******************************/

void DrawSkidMarks(void)
{
int		i,j,n;

	OGL_PushState();

	OGL_DisableLighting();									// deactivate lighting
	glDisable(GL_NORMALIZE);								// disable vector normalizing since scale == 1
	glDisable(GL_CULL_FACE);								// deactivate culling
	glDepthMask(GL_FALSE);									// no z-writes
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

			/******************/
			/* DRAW EACH SKID */
			/******************/

	for (i = 0; i < MAX_SKIDMARKS; i++)
	{
		if (!gSkidMarks[i].isActive)						// must be active
			continue;
		if (gSkidMarks[i].numSegments < 2)					// takes at least 2 segments to draw something
			continue;

		glBegin(GL_QUADS);

			/* DRAW EACH SEGMENT */

		n = gSkidMarks[i].numSegments-1;
		for (j = 0; j < n; j++)
		{
			OGLPoint3D	*p1, *p2;
			float		a1,a2,r1,g1,b1,r2,g2,b2;

			a1 = gSkidMarks[i].color[j].a;					// get alpha fade here
			a2 = gSkidMarks[i].color[j+1].a;

			r1 = gSkidMarks[i].color[j].r;					// get rgb color
			r2 = gSkidMarks[i].color[j+1].r;
			g1 = gSkidMarks[i].color[j].g;
			g2 = gSkidMarks[i].color[j+1].g;
			b1 = gSkidMarks[i].color[j].b;
			b2 = gSkidMarks[i].color[j+1].b;

			if ((a1 <= 0.0f) && (a2 <= 0.0f))				// if faded out, then skip segment
				continue;

			p1 = &gSkidMarks[i].points[j][0];
			p2 = &gSkidMarks[i].points[j+1][1];

			glColor4f(r1,g1,b1,a1);	glVertex3f(p1->x, p1->y, p1->z);
			p1++;
			glColor4f(r1,g1,b1,a1);	glVertex3f(p1->x, p1->y, p1->z);

			glColor4f(r2,g2,b2,a2);	glVertex3f(p2->x, p2->y, p2->z);
			p2--;
			glColor4f(r2,g2,b2,a2);	glVertex3f(p2->x, p2->y, p2->z);
		}

		glEnd();

	}


	OGL_PopState();
}



/****************************** UPDATE SKID MARKS *****************************/

void UpdateSkidMarks(void)
{
int		i,j,numActive,n;
float	fadeRate = gFramesPerSecondFrac * .14f;


	for (i = 0; i < MAX_SKIDMARKS; i++)
	{
		if (!gSkidMarks[i].isActive)
			continue;

		n = gSkidMarks[i].numSegments;

		if (!gSkidMarks[i].isContinuance)								// if it isnt a continued skid, then always set 1st alpha to 0
			if (n >= 2)
				gSkidMarks[i].color[0].a = 0;

			/* CHECK EACH VERTED FOR FADING */

		numActive = 0;

		for (j = 0; j < n; j++)
		{
			gSkidMarks[i].color[j].a -= fadeRate;							// fade out
			if (gSkidMarks[i].color[j].a > 0.0f)
				numActive++;											// keep track of how many segments are still visible
			else
				gSkidMarks[i].color[j].a = 0.0f;
		}

			/* SEE IF ALL ARE FADED AWAY */

		if (numActive == 0)
		{
			gSkidMarks[i].isActive = false;
		}
	}
}

/***************** DETACH OWNER FROM SKID ******************/

void DetachOwnerFromSkid(ObjNode *owner)
{
int	i,n,e;
OGLVector2D vecA;
float	x1,z1,x2,z2,dx,dz;

	for (i = 0; i < MAX_SKIDMARKS; i++)
	{
		if (!gSkidMarks[i].isActive)
			continue;

		if (gSkidMarks[i].owner == owner) 							// look for matching owner, ignore subID
		{
			gSkidMarks[i].owner = nil;


				/* APPEND AN EXTENSION FOR FADE-OUT */

			n = gSkidMarks[i].numSegments;
			if ((n < MAX_SKID_SEGMENTS) && (n > 1))					// we need the right # of segments
			{
				for (e = 0; e < 2; e++)								// do both edges
				{
					x1 = gSkidMarks[i].points[n-2][e].x;
					z1 = gSkidMarks[i].points[n-2][e].z;
					x2 = gSkidMarks[i].points[n-1][e].x;
					z2 = gSkidMarks[i].points[n-1][e].z;
					dx = x2-x1;
					dz = z2-z1;
					FastNormalizeVector2D(dx, dz, &vecA, true);

					x2 += vecA.x * 40.0f;
					z2 += vecA.y * 40.0f;

					gSkidMarks[i].points[n][e].x = x2;					// add new points
					gSkidMarks[i].points[n][e].z = z2;
					gSkidMarks[i].points[n][e].y = GetTerrainY(x2,z2) + SKID_YOFF;
				}
				gSkidMarks[i].color[n].a = 0;

				gSkidMarks[i].numSegments++;
			}
		}
	}
}


/************************* MAKE SKID SMOKE ******************************/

void MakeSkidSmoke(short p, OGLPoint3D *coord)
{
int		particleGroup,magicNum;

	particleGroup 	= gPlayerInfo[p].skidSmokeParticleGroup;
	magicNum 		= gPlayerInfo[p].skidSmokeMagicNum;

	if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
	{
		NewParticleGroupDefType	groupDef;

		gPlayerInfo[p].skidSmokeMagicNum = magicNum = MyRandomLong();			// generate a random magic num

		groupDef.magicNum				= magicNum;
		groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
		groupDef.flags					= 0;
		groupDef.gravity				= -60;
		groupDef.magnetism				= 0;
		groupDef.baseScale				= 30;
		groupDef.decayRate				=  -2.0;
		groupDef.fadeRate				= .7;
		groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
		groupDef.srcBlend				= GL_SRC_ALPHA;
		groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
		gPlayerInfo[p].skidSmokeParticleGroup = particleGroup = NewParticleGroup(&groupDef);
	}

	if (particleGroup != -1)
	{
		NewParticleDefType	newParticleDef;
		OGLVector3D			smokeDelta;


		smokeDelta.x = gDelta.x * RandomFloat() * .3f;
		smokeDelta.y = 100.0f + RandomFloat() * 200.0f;
		smokeDelta.z = gDelta.z * RandomFloat() * .3f;

		newParticleDef.groupNum		= particleGroup;
		newParticleDef.where		= coord;
		newParticleDef.delta		= &smokeDelta;
		newParticleDef.scale		= RandomFloat() + 1.0f;
		newParticleDef.rotZ			= RandomFloat() * PI2;
		newParticleDef.rotDZ		= (RandomFloat()-.5f) * 4.0f;
		newParticleDef.alpha		= FULL_ALPHA;
		AddParticleToGroup(&newParticleDef);
	}
}








