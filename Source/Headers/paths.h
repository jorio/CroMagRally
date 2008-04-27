//
// paths.h
//

#ifndef __PATHS__
#define __PATHS__


#define	MAX_VECTORS_PER_SUPERTILE	16

typedef	struct
{
	float	x,z;
}PathPointType;


typedef struct
{
	Byte			flags;
	Byte			parms[3];

	short			numNubs;			// # nubs in path
	PathPointType	**nubList;			// handle to nub list
	long			numPoints;			// # points in path
	PathPointType	**pointList;		// handle to calculated path points

	Rect			unusedbBox;				// bounding box of path area
}PathDefType;


        /* SUPERTILE GRID STUFF */

typedef struct
{
	float	ox,oz;						// origin coords
    float   vx,vz;						// normalized vector
}PathVectorType;


typedef struct
{
    short           numVectors;         // # path vectors on this supertile grid location
    PathVectorType  *vectors;           // pointer to list of vectors (or nil if none)
    Byte			flags[MAX_VECTORS_PER_SUPERTILE];
}SuperTilePathGridType;


enum
{
	PATH_FLAGS_PRIMARY 	=	(1),
	PATH_FLAGS_YCLOSE	=	(1<<1)

};


//================================================================

void AssignPathVectorsToSuperTileGrid(void);
void DisposePathGrid(void);
Boolean CalcPathVectorFromCoord(float x, float y, float z, OGLVector2D *outVec);




#endif