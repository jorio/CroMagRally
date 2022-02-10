/****************************/
/*     TRIANGULATE.C        */
/* By Brian Greenstone      */
/****************************/

/***************/
/* EXTERNALS   */
/***************/

#include "globals.h"
#include "misc.h"
#include "ogl_support.h"
#include "metaobjects.h"
#include "triangulate.h"
#include "input.h"
#include <textutils.h>


/****************************/
/*  PROTOTYPES             */
/****************************/

inline static Boolean circumcircle(float xp,float zp,float x1,float z1,float x2,float z2,float x3,float z3);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DEBUG_TRIANGULATE	0

typedef struct EDGE
{
	long p1,p2;
}EDGE;


#define	EMAX	50


/**********************/
/*     VARIABLES      */
/**********************/

EDGE 	edges[EMAX];


/*
	Triangulate subroutine

   Notes
      - First element of point array not used, this made it easier to translate
        TRIANGULATE() from my original version in FORTRAN-77
      - Takes as input NV vertices in array vertexList
      - Returned is a list of numTris triangular faces in the array v
      - These triangles are arranged in clockwise order.

      - vertexList must be numPoints+1 elements long!!

*/

void    Triangulate(int  numPoints, OGLVertex *vertexList, MOTriangleIndecies *tri, int *numTris, int triangeListSize)
{
long 	numEdges;
short 	inside;
long 	i,j,k;
float 	xp,zp,x1,z1,x2,z2,x3,z3;
float 	xmin,xmax,zmin,zmax,xmid,zmid;
float 	dx,dz,dmax;

int		vi0,vi1,vi2;

#if DEBUG_TRIANGULATE
Rect	 	rect = {0,0,600,700};
WindowPtr 	window = NewCWindow(nil, &rect, "", true, plainDBox, MOVE_TO_FRONT, false, 0);

		SetPort(window);
		ForeColor(yellowColor);
		BackColor(blackColor);
#endif


		/* FIND THE MIN/MAX BOUNDS TO ALLOW CALC OF BOUNDING TRIANGLE */

	xmin = vertexList[0].point.x;
	zmin = vertexList[0].point.z;
	xmax = xmin;
	zmax = zmin;

	for (i = 1; i < numPoints; i++)
   	{
		if (vertexList[i].point.x < xmin)
    	  	xmin = vertexList[i].point.x;
    	if (vertexList[i].point.x > xmax)
      		xmax = vertexList[i].point.x;
      	if (vertexList[i].point.z < zmin)
      		zmin = vertexList[i].point.z;
      	if (vertexList[i].point.z > zmax)
      		zmax = vertexList[i].point.z;
 	}

   	dx 		= xmax - xmin;
   	dz 		= zmax - zmin;
   	dmax 	= (dx > dz) ? dx : dz;
   	xmid 	= (xmax + xmin) / 2.0f;
   	zmid 	= (zmax + zmin) / 2.0f;


	/* SETUP THE SUPERTRIANGLE */
  	//
  	//   This is a triangle which encompasses all the sample points.
  	//   The supertriangle coordinates are added to the end of the vertex list.
  	//   The supertriangle is the first triangle in the triangle list.
	//

   	vertexList[numPoints].point.x 		= xmid - (2.0f * dmax);				// set 3 points
   	vertexList[numPoints].point.z 		= zmid - dmax;
   	vertexList[numPoints].point.y 		= 0.0;
   	vertexList[numPoints+1].point.x 	= xmid;
   	vertexList[numPoints+1].point.z 	= zmid + dmax;
   	vertexList[numPoints+1].point.y 	= 0.0;
   	vertexList[numPoints+2].point.x 	= xmid + (2.0f * dmax);
   	vertexList[numPoints+2].point.z 	= zmid - dmax;
   	vertexList[numPoints+2].point.y 	= 0.0;

   	tri[0].vertexIndices[0] = numPoints;							// set triangle indecies
   	tri[0].vertexIndices[1] = numPoints+1;
   	tri[0].vertexIndices[2] = numPoints+2;

   	*numTris 		= 1;


   	/***********************************************************/
   	/* INCLUDE EACH POINT ONE AT A TIME INTO THE EXISTING MESH */
   	/***********************************************************/

 	for (i = 0; i < numPoints; i++)
 	{
#if DEBUG_TRIANGULATE
		int		tt;
		Rect	r2;
		RGBColor	c;
		Str255		s;

		EraseRect(&rect);

		SetRect(&r2, 200,300-dz/35, 200 + dx / 35, 300);
		ForeColor(redColor);
		FrameRect(&r2);

		ForeColor(greenColor);

		r2.left = 200 + (vertexList[i].x-xmin) / 35 - 5;
		r2.right = r2.left + 10;
		r2.top =  300 - (vertexList[i].z-zmin) / 35 - 5;
		r2.bottom = r2.top + 10;
		PaintOval(&r2);
		ForeColor(whiteColor);

		for (tt = 0; tt < *numTris; tt++)
		{
			int		xx1,xx2,xx3,yy1,yy2,yy3;

			xx1 = 200 + (vertexList[tri[tt].vertexIndices[0]].x - xmin) / 35;
			yy1 = 300 - (vertexList[tri[tt].vertexIndices[0]].z - zmin) / 35;
			xx2 = 200 + (vertexList[tri[tt].vertexIndices[1]].x - xmin) / 35;
			yy2 = 300 - (vertexList[tri[tt].vertexIndices[1]].z - zmin) / 35;
			xx3 = 200 + (vertexList[tri[tt].vertexIndices[2]].x - xmin) / 35;
			yy3 = 300 - (vertexList[tri[tt].vertexIndices[2]].z - zmin) / 35;

			MoveTo(xx1,yy1);
			LineTo(xx2,yy2);
			MoveTo(xx2,yy2);
			LineTo(xx3,yy3);
			MoveTo(xx3,yy3);
			LineTo(xx1,yy1);
		}

		MoveTo(20,20);
		NumToString(*numTris, s);
		DrawString(s);

//		while(!Button())
//		{
//			ReadKeyboard();
//		}
//		while(Button());
#endif

		xp 			= vertexList[i].point.x;												// get point x/z
    	zp 			= vertexList[i].point.z;


      	/* SETUP THE EDGE BUFFER */
      	//
        // If the point (xp,zp) lies inside the circumcircle then the
        // three edges of that triangle are added to the edge buffer
        // and that triangle is removed.
 		//

		numEdges 	= 0;
		j 			= 0;														// start w/ triangle #0

		do
	    {
         	vi0 = tri[j].vertexIndices[0];										// get triangle j's indecies
         	vi1 = tri[j].vertexIndices[1];
         	vi2 = tri[j].vertexIndices[2];

            x1 = vertexList[vi0].point.x;										// get x/z's of all 3 triangle j's points
            z1 = vertexList[vi0].point.z;
            x2 = vertexList[vi1].point.x;
            z2 = vertexList[vi1].point.z;
            x3 = vertexList[vi2].point.x;
            z3 = vertexList[vi2].point.z;

            inside = circumcircle(xp,zp,x1,z1,x2,z2,x3,z3);				// see if point is inside this triangle's circumcircle

        	if (inside)													// if new point is inside the circumcircle, then add edges of triangle
            {
          		if ((numEdges+3) > EMAX)								// Check that we haven't exceeded the edge list size
            		DoFatalAlert("Triangulate: Internal edge list exceeded");

               	edges[numEdges].p1 		= tri[j].vertexIndices[0];		// add 3 edges to edge list
               	edges[numEdges++].p2 	= tri[j].vertexIndices[1];
               	edges[numEdges].p1 		= tri[j].vertexIndices[1];
               	edges[numEdges++].p2 	= tri[j].vertexIndices[2];
               	edges[numEdges].p1 		= tri[j].vertexIndices[2];
               	edges[numEdges++].p2 	= tri[j].vertexIndices[0];

               	(*numTris)--;
               	tri[j] 		= tri[*numTris];							// delete this triangle by copying last triangle in list into this location & dec tri count
               	j--;													// dec this to cancel out the ++ below (we need to process this triangle again since its now a different tri)
        	}
			j++;
		}while(j < *numTris);

		if (numEdges < 1)
			DoFatalAlert("Triangulate:  numEdges < 1");	//----------

    	//   Tag multiple edges
     	//   Note: if all triangles are specified anticlockwise then all
     	//         interior edges are opposite pointing in direction.


		for (j=0; j < (numEdges-1); j++)
		{
        	for (k = j+1; k < numEdges; k++)
        	{
            	if ((edges[j].p1 == edges[k].p2) && (edges[j].p2 == edges[k].p1))
            	{
               		edges[j].p1 = -1;
               		edges[j].p2 = -1;
               		edges[k].p1 = -1;
               		edges[k].p2 = -1;
            	}

            /* Shouldn't need the following, see note above */

            	if ((edges[j].p1 == edges[k].p1) && (edges[j].p2 == edges[k].p2))
            	{
               		edges[j].p1 = -1;
               		edges[j].p2 = -1;
               		edges[k].p1 = -1;
               		edges[k].p2 = -1;
            	}
         	}
      	}

		/* FORM NEW TRIANGLES FOR THE CURRENT POINT */
		//
        // Skip over any tagged edges.
        // All edges are arranged in clockwise order.
      	//

      	for (j = 0; j < numEdges; j++)
      	{
        	if ((edges[j].p1 != -1) && (edges[j].p2 != -1))
        	{
            	if (*numTris > triangeListSize)
	           		DoFatalAlert("Triangulate: triangles exceeds maximum");

            	tri[*numTris].vertexIndices[0] 	= edges[j].p1;					// use edge points
            	tri[*numTris].vertexIndices[1] 	= edges[j].p2;
            	tri[*numTris].vertexIndices[2] 	= i;							// this is the new point
            	(*numTris)++;
         	}
      	}
	}

   	// Remove triangles with supertriangle vertices
   	// These are triangles which have a vertex number greater than numPoints

  	i = 0;
   	do
   	{
      	if ((tri[i].vertexIndices[0] >= numPoints) || (tri[i].vertexIndices[1] >= numPoints) || (tri[i].vertexIndices[2] >= numPoints))
      	{
         	(*numTris)--;
         	tri[i] = tri[*numTris];								// delete triangle by moving last in list to this slot
         	i--;
      	}
    	i++;
 	}while (i < *numTris);

#if DEBUG_TRIANGULATE
	{
		int		tt;
		Rect	r2;

		EraseRect(&rect);

		SetRect(&r2, 0,300-dz/35, dx / 35, 300);
		ForeColor(redColor);
		FrameRect(&r2);
		ForeColor(whiteColor);

		for (tt = 0; tt < *numTris; tt++)
		{
			int		xx1,xx2,xx3,yy1,yy2,yy3;

			xx1 = 200 + (vertexList[tri[tt].vertexIndices[0]].x - xmin) / 35;
			yy1 = 300 - (vertexList[tri[tt].vertexIndices[0]].z - zmin) / 35;
			xx2 = 200 + (vertexList[tri[tt].vertexIndices[1]].x - xmin) / 35;
			yy2 = 300 - (vertexList[tri[tt].vertexIndices[1]].z - zmin) / 35;
			xx3 = 200 + (vertexList[tri[tt].vertexIndices[2]].x - xmin) / 35;
			yy3 = 300 - (vertexList[tri[tt].vertexIndices[2]].z - zmin) / 35;

			MoveTo(xx1,yy1);
			LineTo(xx2,yy2);
			MoveTo(xx2,yy2);
			LineTo(xx3,yy3);
			MoveTo(xx3,yy3);
			LineTo(xx1,yy1);
		}

		while(!Button())
		{
			ReadKeyboard();
		}
		while(Button());
	}
#endif

#if DEBUG_TRIANGULATE
	DisposeWindow(window);
#endif
}




/*****************************************************************************
        Return TRUE if a point (xp,yp) is inside the circumcircle made up
        of the points (x1,y1), (x2,y2), (x3,y3)
        The circumcircle centre is returned in (xc,yc) and the radius r
        NOTE: A point on the edge is inside the circumcircle
*/

inline static Boolean circumcircle(float xp,float zp,float x1,float z1,float x2,float z2,float x3,float z3)
{
float 	m1,m2,mx1,mx2,mz1,mz2,xc,zc;
float 	dx,dz,rsqr,drsqr;

	if ((fabs(z1-z2) < EPS) && (fabs(z2-z3) < EPS))
 		DoFatalAlert("circumcircle - Coincident points");

	if (fabs(z2-z1) < EPS)
	{
    	m2 	= - (x3-x2) / (z3-z2);
        mx2 = (x2 + x3) * .5f;
        mz2 = (z2 + z3) * .5f;
        xc = (x2 + x1) * .5f;
        zc = m2 * (xc - mx2) + mz2;
    }
    else
    if (fabs(z3-z2) < EPS)
    {
        m1 = - (x2-x1) / (z2-z1);
        mx1 = (x1 + x2) * .5f;
        mz1 = (z1 + z2) * .5f;
        xc = (x3 + x2) * .5f;
        zc = m1 * (xc - mx1) + mz1;
    }
    else
    {
        m1 = -(x2-x1) / (z2-z1);
        m2 = -(x3-x2) / (z3-z2);
        mx1 = (x1 + x2) * .5f;
        mx2 = (x2 + x3) * .5f;
        mz1 = (z1 + z2) * .5f;
        mz2 = (z2 + z3) * .5f;
        xc = (m1 * mx1 - m2 * mx2 + mz2 - mz1) / (m1 - m2);
        zc = m1 * (xc - mx1) + mz1;
    }

 	dx = x2 - xc;
    dz = z2 - zc;
    rsqr = dx*dx + dz*dz;

    dx = xp - xc;
    dz = zp - zc;
    drsqr = dx*dx + dz*dz;

    return((drsqr <= rsqr) ? true : false);
}






