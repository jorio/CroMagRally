/****************************/
/*   	PATH ITEMS.C      */
/****************************/


#include "globals.h"
#include "misc.h"
#include "main.h"
#include "3dmath.h"
#include "paths.h"
#include "terrain.h"

/***************/
/* EXTERNALS   */
/***************/


extern  long gNumSuperTilesDeep,gNumSuperTilesWide;


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/



#define	PATH_SCAN_RANGE		(TERRAIN_SUPERTILE_UNIT_SIZE * 1.0f)

/**********************/
/*     VARIABLES      */
/**********************/

long			gNumPaths = 0;
PathDefType	    **gPathList = nil;

SuperTilePathGridType	**gSuperTilePathGrid = nil;


/********************** ASSIGN PATH VECTORS TO SUPERTILE GRID **************************/
//
// The path line segments are assigned to a list maintained by each supertile grid slot.
// That way it is very easy to determine which path vectors are associated for
// any given supertile.
//
// Once we have created these new lists, we can dispose of the terrain file's path
// data since it isnt needed anymore.
//
// The supertiles dont need to have been created yet since this allocates a separate array.
//

void AssignPathVectorsToSuperTileGrid(void)
{
int     		i,row,col,n;
PathPointType   *pointList;
int             j,p;

			/* ALLOC MEMORY FOR PATH GRID */

	Alloc_2d_array(SuperTilePathGridType, gSuperTilePathGrid, gNumSuperTilesDeep, gNumSuperTilesWide);


        /* INITIALIZE EACH GRID LOCATION - NO PATHS */

    for (row = 0; row < gNumSuperTilesDeep; row++)
    {
        for (col = 0; col < gNumSuperTilesWide; col++)
        {
            gSuperTilePathGrid[row][col].numVectors = 0;
            gSuperTilePathGrid[row][col].vectors    = nil;
        }
    }


			/**************************************/
			/* CONVERT PATH POINTS TO UNIT COORDS */
			/**************************************/

    for (i = 0; i < gNumPaths; i++)
    {
        n = (*gPathList)[i].numPoints;                          // get # points in the list
        pointList = *(*gPathList)[i].pointList;                 // dereference the handle (its locked, so a ptr is okay)
        for (p = 0; p < n; p++)
        {
			pointList[p].x *= MAP2UNIT_VALUE;
			pointList[p].z *= MAP2UNIT_VALUE;
		}
	}



        /****************************************************/
        /* SCAN THE PATH LISTS AND ADD PATH VECTORS TO GRID */
        /****************************************************/

    for (i = 0; i < gNumPaths; i++)
    {

            /* POINT TO THIS PATH'S POINT LIST */

        n = (*gPathList)[i].numPoints;                          // get # points in the list
        if (n == 1)												// skip 1 point paths since they are garbage
        	continue;

        pointList = *(*gPathList)[i].pointList;                 // dereference the handle (its locked, so a ptr is okay)


			/***********************************************************/
	 		/* CREATE VECTORS FROM POINTS AND ASSIGN TO SUPERTILE GRID */
			/***********************************************************/

        for (p = 0; p < n; p++)
        {
			OGLVector2D		v = {0, 0};
			float			x,z;

        	x 	= pointList[p].x;									// get point coord
        	z 	= pointList[p].z;

        	if (p < (n-1))											// for last point, keep previous vector (since theres no next point to create a new vector)
        	{
        		v.x	= pointList[p+1].x - x;							// calc vector
        		v.y	= pointList[p+1].z - z;
		        FastNormalizeVector2D(v.x, v.y, &v, true);			// normalize the vector
		 	}

			col = x / TERRAIN_SUPERTILE_UNIT_SIZE;					// convert to supertile row,col
			row = z / TERRAIN_SUPERTILE_UNIT_SIZE;

			if ((col < 0) || (col >= gNumSuperTilesWide))			// see if out of bounds
				DoFatalAlert("AssignPathVectorsToSuperTileGrid: col out of bounds");
			if ((row < 0) || (row >= gNumSuperTilesDeep))
				DoFatalAlert("AssignPathVectorsToSuperTileGrid: row out of bounds");

					/* SEE IF NEED TO ALLOC MEMORY FOR THE VECTOR LIST */

			if (gSuperTilePathGrid[row][col].vectors == nil)
			{
				gSuperTilePathGrid[row][col].vectors = (PathVectorType *)AllocPtr(sizeof(PathVectorType) * MAX_VECTORS_PER_SUPERTILE);

				if (gSuperTilePathGrid[row][col].vectors == nil)
					DoFatalAlert("AssignPathVectorsToSuperTileGrid: AllocPtr failed");
			}

					/* ADD THIS VECTOR TO THE LIST */

			j = gSuperTilePathGrid[row][col].numVectors;		// get # already in list
			if (j >= MAX_VECTORS_PER_SUPERTILE)					// see if too many
				DoFatalAlert("AssignPathVectorsToSuperTileGrid: numVectors >= MAX_VECTORS_PER_SUPERTILE");

			gSuperTilePathGrid[row][col].vectors[j].vx = v.x;	// copy coord info into list (normalized vector & origin coord)
			gSuperTilePathGrid[row][col].vectors[j].vz = v.y;
			gSuperTilePathGrid[row][col].vectors[j].ox = x;
			gSuperTilePathGrid[row][col].vectors[j].oz = z;

			gSuperTilePathGrid[row][col].flags[j] = (*gPathList)[i].flags;

			gSuperTilePathGrid[row][col].numVectors++;			// inc count
		}
	}


			/***************************/
			/* NUKE ORIGINAL PATH DATA */
			/***************************/

	for (i = 0; i < gNumPaths; i++)
	{
		DisposeHandle((Handle)(*gPathList)[i].pointList);		// nuke point list
	}
	DisposeHandle((Handle)gPathList);
}



/********************** DISPOSE PATH GRID ***********************/

void DisposePathGrid(void)
{
int row,col;

    if (gSuperTilePathGrid == nil)
    	return;

			/* DELETE VECTOR LIST IN EACH GRID LOCATION */

    for (row = 0; row < gNumSuperTilesDeep; row++)
    {
        for (col = 0; col < gNumSuperTilesWide; col++)
        {
            if (gSuperTilePathGrid[row][col].vectors)           // see if there is a list to nuke
                SafeDisposePtr((Ptr)gSuperTilePathGrid[row][col].vectors);
        }
    }

			/* NUKE THE GRID ITSELF */

    Free_2d_array(gSuperTilePathGrid);
    gSuperTilePathGrid = nil;
}


#pragma mark -


/************************** CALC PATH VECTOR FROM COORD ***************************/
//
// Given a world x,z coordinate, calculate a path vector based on all of the path information
// available for the nearby supertiles.
//
// Returns FALSE if no paths are found.
//

Boolean CalcPathVectorFromCoord(float x, float y, float z, OGLVector2D *outVec)
{
float			minX,maxX,minZ,maxZ,dist;
int				startCol,endCol,startRow,endRow;
int				row,col,i;
PathVectorType	*vecData;
Boolean			gotVector = false;
Byte			flags;
Byte			*flagData;

	minX = x - PATH_SCAN_RANGE;
	maxX = x + PATH_SCAN_RANGE;
	minZ = z - PATH_SCAN_RANGE;
	maxZ = z + PATH_SCAN_RANGE;

	startCol 	= minX * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;
	endCol 		= maxX * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;
	startRow 	= minZ * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;
	endRow 		= maxZ * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;

	if (startCol < 0)										// check bounds
		startCol = 0;
	else
	if (startCol >= gNumSuperTilesWide)
		startCol = gNumSuperTilesWide-1;

	if (endCol < 0)
		endCol = 0;
	else
	if (endCol >= gNumSuperTilesWide)
		endCol = gNumSuperTilesWide-1;

	if (startRow < 0)
		startRow = 0;
	else
	if (startRow >= gNumSuperTilesDeep)
		startRow = gNumSuperTilesDeep-1;

	if (endRow < 0)
		endRow = 0;
	else
	if (endRow >= gNumSuperTilesDeep)
		endRow = gNumSuperTilesDeep-1;



	outVec->x = outVec->y = 0;													// initialize the vector

	for (row = startRow; row <= endRow; row++)
	{
		for (col = startCol; col <= endCol; col++)
		{
			vecData = gSuperTilePathGrid[row][col].vectors;						// get ptr to vector list
			flagData = gSuperTilePathGrid[row][col].flags;
			if (vecData)
			{
						/**************************************/
						/* SCAN EACH VECTOR ON THIS SUPERTILE */
						/**************************************/

				for (i = 0; i < gSuperTilePathGrid[row][col].numVectors; i++)
				{
						/* SEE IF VECTOR ORIGIN IN WITHIN OUR RANGE */

					if ((vecData[i].ox >= minX) && (vecData[i].ox <= maxX) &&
						(vecData[i].oz >= minZ) && (vecData[i].oz <= maxZ))
					{
						flags = flagData[i];									// get path flags

						if (flags & PATH_FLAGS_YCLOSE)							// see if must be close in y to use this
						{
							if (fabs(GetTerrainY(x,z) - y) > 50.0f)
								continue;
						}

						dist = CalcDistance(vecData[i].ox, vecData[i].oz, x, z);	// calc dist to this vec
					    dist = 1.0f / dist; 										// calc inverse dist to vector origin

						outVec->x += vecData[i].vx * dist;							// add in this vector
						outVec->y += vecData[i].vz * dist;

						gotVector = true;
					}
				}
			}
		}
	}

		/* NORMALIZE THE RESULTING VECTOR TO "AVERAGE" IT */

	if (gotVector)
	{
		FastNormalizeVector2D(outVec->x, outVec->y, outVec, true);
	}

	return(gotVector);
}












