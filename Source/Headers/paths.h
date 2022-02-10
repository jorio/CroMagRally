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

//	short			numNubs;			// # nubs in path -- unused in-game
//	PathPointType	**nubList;			// handle to nub list -- unused in-game
	long			numPoints;			// # points in path
	PathPointType	**pointList;		// handle to calculated path points

//	Rect			bBox;				// bounding box of path area -- unused in-game
}PathDefType;

typedef struct
{
	Byte			flags;
	Byte			parms[3];

	int16_t			numNubs;			// # nubs in path
	int16_t			_pad1;				// # nubs in path
	int32_t 		_junkptr1;			// handle to nub list
	int32_t			numPoints;			// # points in path
	int32_t			_junkptr2;			// handle to calculated path points

	Rect			bBox;				// bounding box of path area
}File_PathDefType;


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