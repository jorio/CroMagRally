/*******************************/
/*     		3D MATH.C		   */
/* (c)1996-99 Pangea Software  */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <math.h>

#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "3dmath.h"
#include "collision.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLMatrix4x4		gLocalToFrustumMatrix;

/****************************/
/*    PROTOTYPES            */
/****************************/

#define Math_Min(x,y)             ((x) <= (y) ? (x) : (y))
#define Math_Max(x,y)             ((x) >= (y) ? (x) : (y))


#define CROSS(dest,v1,v2){                 \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];}

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2){       \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2];}


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	VectorComponent_X,
	VectorComponent_Y,
	VectorComponent_Z
};


/*********************/
/*    VARIABLES      */
/*********************/



/********************* CALC X ANGLE FROM POINT TO POINT **********************/

float CalcXAngleFromPointToPoint(float fromY, float fromZ, float toY, float toZ)
{
float	xangle,zdiff;

	zdiff = toZ-fromZ;									// calc diff in Z
	zdiff = fabs(zdiff);								// get abs value

	xangle = atan2(-zdiff,toY-fromY) + (PI/2.0f);

			/* KEEP BETWEEN 0 & 2*PI */

	return(MaskAngle(xangle));
}


/********************* CALC Y ANGLE FROM POINT TO POINT **********************/
//
// Returns an unsigned result from 0 -> 6.28
//

float	CalcYAngleFromPointToPoint(float oldRot, float fromX, float fromZ, float toX, float toZ)
{
float	yangle;
float	dx, dz;

	dx = fromX-toX;
	dz = fromZ-toZ;

	if ((fabs(dx) < EPS) && (fabs(dz) < EPS))			// see if delta to small to be accurate
		return(oldRot);

	yangle = PI2 - MaskAngle(atan2(dz,dx) - (PI/2.0f));

	if (yangle >= (PI2-.0001f))
		return(PI2);

	return(yangle);
}



#pragma mark -

/************ TURN OBJECT TOWARD TARGET ****************/
//
// INPUT:	theNode = object
//			from = from pt or uses theNode->Coord
//			x/z = target coords to aim towards
//			turnSpeed = turn speed, 0 == just return angle between
//
// OUTPUT:  remaining angle to target rot
//

float TurnObjectTowardTarget(ObjNode *theNode, const OGLPoint3D *from, float toX, float toZ, float turnSpeed, Boolean useOffsets)
{
float	fromX,fromZ;
float	r,angle,cross;
OGLVector2D	aimVec,targetVec;

			/* CALC FROM/TO COORDS */

//	if (useOffsets)
//	{
//		toX += theNode->TargetOff.x;										// offset coord
//		toZ += theNode->TargetOff.y;
//	}

	if (from)
	{
		fromX = from->x;
		fromZ = from->z;
	}
	else
	{
		fromX = theNode->Coord.x;
		fromZ = theNode->Coord.z;
	}


			/* CALC ANGLE BETWEEN AIM AND TARGET */

	r = theNode->Rot.y;												// get normalized vector in direction of current aim
	aimVec.x = -sin(r);
	aimVec.y = -cos(r);

 	targetVec.x = toX - fromX;										// get normalized vector to target object
 	targetVec.y = toZ - fromZ;
	FastNormalizeVector2D(targetVec.x, targetVec.y, &targetVec, true);
	if ((targetVec.x == 0.0f) && (targetVec.y == 0.0f))
		return(0);

	angle = acos(OGLVector2D_Dot(&aimVec, &targetVec));				// dot == angle to turn
	cross = OGLVector2D_Cross(&aimVec, &targetVec);					// sign of cross tells us which way to turn


			/* DO THE TURN */

	turnSpeed *= gFramesPerSecondFrac;								// calc amount to turn this frame

	if (turnSpeed > angle)											// make sure we dont exceed how far we want to go
		turnSpeed = angle;

	if (cross > 0.0f)												// see which direction
		turnSpeed = -turnSpeed;

	theNode->Rot.y += turnSpeed;									// turn it!

	return(angle - fabs(turnSpeed));								// return angular distance to target
}


/************ TURN OBJECT TOWARD TARGET ON X ****************/
//
// same as above except rotates on X axis to pitch object up/down toward target
//

float TurnObjectTowardTargetOnX(ObjNode *theNode, const OGLPoint3D *from, float toX, float toY, float turnSpeed)
{
float	fromX,fromY;
float	r,angle,cross;
OGLVector2D	aimVec,targetVec;

			/* CALC FROM/TO COORDS */

	if (from)
	{
		fromX = from->x;
		fromY = from->y;
	}
	else
	{
		fromX = theNode->Coord.x;
		fromY = theNode->Coord.y;
	}


			/* CALC ANGLE BETWEEN AIM AND TARGET */

	r = theNode->Rot.x;												// get normalized vector in direction of current aim
	aimVec.x = -sin(r);
	aimVec.y = -cos(r);

 	targetVec.x = toX - fromX;										// get normalized vector to target object
 	targetVec.y = toY - fromY;
	FastNormalizeVector2D(targetVec.x, targetVec.y, &targetVec, true);
	if ((targetVec.x == 0.0f) && (targetVec.y == 0.0f))
		return(0);

	angle = acos(OGLVector2D_Dot(&aimVec, &targetVec));				// dot == angle to turn
	cross = OGLVector2D_Cross(&aimVec, &targetVec);					// sign of cross tells us which way to turn


			/* DO THE TURN */

	turnSpeed *= gFramesPerSecondFrac;								// calc amount to turn this frame

	if (turnSpeed > angle)											// make sure we dont exceed how far we want to go
		turnSpeed = angle;

	if (cross > 0.0f)												// see which direction
		turnSpeed = -turnSpeed;

	theNode->Rot.x += turnSpeed;									// turn it!

	return(angle - fabs(turnSpeed));								// return angular distance to target
}




#pragma mark -


/******************** CALC POINT ON OBJECT *************************/
//
// Uses the input ObjNode's BaseTransformMatrix to transform the input point.
//

void CalcPointOnObject(const ObjNode *theNode, const OGLPoint3D *inPt, OGLPoint3D *outPt)
{
	OGLPoint3D_Transform(inPt, &theNode->BaseTransformMatrix, outPt);
}


/************************** VECTORS ARE CLOSE ENOUGH ****************************/

Boolean VectorsAreCloseEnough(const OGLVector3D *v1, const OGLVector3D *v2)
{
	if (fabs(v1->x - v2->x) < 0.02f)			// WARNING: If change, must change in BioOreoPro also!!
		if (fabs(v1->y - v2->y) < 0.02f)
			if (fabs(v1->z - v2->z) < 0.02f)
				return(true);

	return(false);
}

/************************** POINTS ARE CLOSE ENOUGH ****************************/

Boolean PointsAreCloseEnough(const OGLPoint3D *v1, const OGLPoint3D *v2)
{
	if (fabs(v1->x - v2->x) < 0.001f)		// WARNING: If change, must change in BioOreoPro also!!
		if (fabs(v1->y - v2->y) < 0.001f)
			if (fabs(v1->z - v2->z) < 0.001f)
				return(true);

	return(false);
}




#pragma mark -

/******************** INTERSECT PLANE & LINE SEGMENT ***********************/
//
// Returns true if the input line segment intersects the plane.
//

Boolean IntersectionOfLineSegAndPlane(const OGLPlaneEquation *plane, float v1x, float v1y, float v1z,
								 float v2x, float v2y, float v2z, OGLPoint3D *outPoint)
{
int		a,b;
float	r;
float	nx,ny,nz,planeConst;
float	vBAx, vBAy, vBAz, dot, lam;

	nx = plane->normal.x;
	ny = plane->normal.y;
	nz = plane->normal.z;
	planeConst = plane->constant;


		/* DETERMINE SIDENESS OF VERT1 */

	r = -planeConst;
	r += (nx * v1x) + (ny * v1y) + (nz * v1z);
	a = (r < 0.0f) ? 1 : 0;

		/* DETERMINE SIDENESS OF VERT2 */

	r = -planeConst;
	r += (nx * v2x) + (ny * v2y) + (nz * v2z);
	b = (r < 0.0f) ? 1 : 0;

		/* SEE IF LINE CROSSES PLANE (INTERSECTS) */

	if (a == b)
	{
		return(false);
	}


		/****************************************************/
		/* LINE INTERSECTS, SO CALCULATE INTERSECTION POINT */
		/****************************************************/

				/* CALC LINE SEGMENT VECTOR BA */

	vBAx = v2x - v1x;
	vBAy = v2y - v1y;
	vBAz = v2z - v1z;

			/* DOT OF PLANE NORMAL & LINE SEGMENT VECTOR */

	dot = (nx * vBAx) + (ny * vBAy) + (nz * vBAz);

			/* IF VALID, CALC INTERSECTION POINT */

	if (dot)
	{
		lam = planeConst;
		lam -= (nx * v1x) + (ny * v1y) + (nz * v1z);		// calc dot product of plane normal & 1st vertex
		lam /= dot;											// div by previous dot for scaling factor

		outPoint->x = v1x + (lam * vBAx);					// calc intersect point
		outPoint->y = v1y + (lam * vBAy);
		outPoint->z = v1z + (lam * vBAz);
		return(true);
	}

		/* IF DOT == 0, THEN LINE IS PARALLEL TO PLANE THUS NO INTERSECTION */

	else
		return(false);
}




/*********** INTERSECTION OF Y AND PLANE FUNCTION ********************/
//
// INPUT:
//			x/z		:	xz coords of point
//			p		:	ptr to the plane
//
//
// *** IMPORTANT-->  This function does not check for divides by 0!! As such, there should be no
//					"vertical" polygons (polys with normal->y == 0).
//

float IntersectionOfYAndPlane_Func(float x, float z, const OGLPlaneEquation *p)
{
	return	((p->constant - ((p->normal.x * x) + (p->normal.z * z))) / p->normal.y);
}


/********************* SET LOOKAT MATRIX ********************/

void SetLookAtMatrix(OGLMatrix4x4 *m, const OGLVector3D *upVector, const OGLPoint3D *from, const OGLPoint3D *to)
{
OGLVector3D	lookAt,theXAxis;

			/* SET UP VECTOR */

	m->value[M01] = upVector->x;
	m->value[M11] = upVector->y;
	m->value[M21] = upVector->z;


		/* CALC THE X-AXIS VECTOR */

	FastNormalizeVector(from->x - to->x, from->y - to->y, from->z - to->z, &lookAt);	// calc temporary look-at vector

	theXAxis.x = 	upVector->y * lookAt.z - lookAt.y * upVector->z;					// calc cross product
	theXAxis.y =  -(upVector->x	* lookAt.z - lookAt.x * upVector->z);
	theXAxis.z = 	upVector->x * lookAt.y - lookAt.x * upVector->y;
	FastNormalizeVector(theXAxis.x, theXAxis.y, theXAxis.z, &theXAxis);		// (this normalize shouldnt be needed, but do it to be safe)

	m->value[M00] = theXAxis.x;
	m->value[M10] = theXAxis.y;
	m->value[M20] = theXAxis.z;


		/* CALC LOOK-AT VECTOR */
		//
		// We totally recompute this since the input "to" is not probably orthonormal to other axes.
		//

	lookAt.x = 	-(upVector->y    * theXAxis.z - theXAxis.y * upVector->z);		// calc reversed cross product
	lookAt.y =  (upVector->x  * theXAxis.z - theXAxis.x * upVector->z);
	lookAt.z = 	-(upVector->x    * theXAxis.y - theXAxis.x * upVector->y);

	m->value[M02] = lookAt.x;
	m->value[M12] = lookAt.y;
	m->value[M22] = lookAt.z;


			/* SET OTHER THINGS */
	m->value[M30] =
	m->value[M31] =
	m->value[M32] =
	m->value[M03] = m->value[M13] = m->value[M23] = 0;
	m->value[M33] = 1;
}


/********************* SET LOOKAT MATRIX AND TRANSLATE ********************/

void SetLookAtMatrixAndTranslate(OGLMatrix4x4 *m, const OGLVector3D *upVector, const OGLPoint3D *from, const OGLPoint3D *to)
{
OGLVector3D	lookAt;

			/* CALC LOOK-AT VECTOR */

	FastNormalizeVector(from->x - to->x, from->y - to->y, from->z - to->z, &lookAt);

	m->value[M02] = lookAt.x;
	m->value[M12] = lookAt.y;
	m->value[M22] = lookAt.z;


			/* CALC UP VECTOR */

	m->value[M01] = upVector->x;
	m->value[M11] = upVector->y;
	m->value[M21] = upVector->z;


		/* CALC THE X-AXIS VECTOR */

	m->value[M00] = 	upVector->y * lookAt.z - lookAt.y * upVector->z;		// calc cross product
	m->value[M10] =  -(upVector->x	* lookAt.z - lookAt.x * upVector->z);
	m->value[M20] = 	upVector->x * lookAt.y - lookAt.x * upVector->y;


			/* SET OTHER THINGS */

	m->value[M30] =
	m->value[M31] =
	m->value[M32] = 0;

	m->value[M03] = from->x;					// set translate
	m->value[M13] = from->y;
	m->value[M23] = from->z;
	m->value[M33] = 1;
}


/******************* CALC VECTOR LENGTH ********************/

float CalcVectorLength(OGLVector3D *v)
{
float	d;

	d = (v->x * v->x) + (v->y * v->y) + (v->z * v->z);

	return(sqrt(d));
}

/******************* CALC VECTOR LENGTH 2D ********************/

float CalcVectorLength2D(const OGLVector2D *v)
{
float	d;

	d = (v->x * v->x) + (v->y * v->y);

	return(sqrt(d));
}



/***************** APPLY FRICTION TO DELTAS ********************/
//
// INPUT: speed = precalculated length of d vector.
//

void ApplyFrictionToDeltas(float f,OGLVector3D *d)
{
float	ratio;
float	newSpeed,speed;

	VectorLength3D(speed, d->x, d->y, d->z);	// calc length of vector

	if (speed <= EPS)							// avoid divides by 0
	{
		d->x = d->y = d->z = 0;
		return;
	}

	newSpeed = speed - f;						// calc target speed by reducing vector length value
	if (newSpeed < 0.0f)
	{
		d->x = d->y = d->z = 0;
		return;
	}

	ratio = newSpeed / speed;					// calc ratio of new to old

	d->x *= ratio;								// reduce deltas by ratio
	d->y *= ratio;
	d->z *= ratio;
}



#pragma mark -

/********************** IS POINT IN TRIANGLE 3D **************************/
//
// INPUT:	point3D			= the point to test
//			trianglePoints	= triangle's 3 points
//			normal			= triangle's normal
//

Boolean IsPointInTriangle3D(const OGLPoint3D *point3D,	const OGLPoint3D *trianglePoints, OGLVector3D *normal)
{
Boolean					intersects = false;
Byte					maximalComponent;
unsigned long			skip;
float					*tmp;
float 					xComp, yComp, zComp;
float					pX,pY;
float					verts0x,verts0y;
float					verts1x,verts1y;
float					verts2x,verts2y;

	tmp = (float *)trianglePoints;
	skip = sizeof(OGLPoint3D) / sizeof(float);


			/*****************************************/
			/* DETERMINE LONGEST COMPONENT OF NORMAL */
			/*****************************************/

//	EiVector3D_MaximalComponent(normal,&maximalComponent,&maximalComponentNegative);

	xComp = fabs(normal->x);
	yComp = fabs(normal->y);
	zComp = fabs(normal->z);

	if (xComp > yComp)
	{
		if (xComp > zComp)
			maximalComponent = VectorComponent_X;
		else
			maximalComponent = VectorComponent_Z;
	}
	else
	{
		if (yComp > zComp)
			maximalComponent = VectorComponent_Y;
		else
			maximalComponent = VectorComponent_Z;
	}



				/* PROJECT 3D POINTS TO 2D */

	switch(maximalComponent)
	{
		OGLPoint3D	*point;

		case	VectorComponent_X:
				pX = point3D->y;
				pY = point3D->z;

				point = (OGLPoint3D *)tmp;
				verts0x = point->y;
				verts0y = point->z;

				point = (OGLPoint3D *)(tmp + skip);
				verts1x = point->y;
				verts1y = point->z;

				point = (OGLPoint3D *)(tmp + skip + skip);
				verts2x = point->y;
				verts2y = point->z;
				break;

		case	VectorComponent_Y:
				pX = point3D->z;
				pY = point3D->x;

				point = (OGLPoint3D *)tmp;
				verts0x = point->z;
				verts0y = point->x;

				point = (OGLPoint3D *)(tmp + skip);
				verts1x = point->z;
				verts1y = point->x;

				point = (OGLPoint3D *)(tmp + skip + skip);
				verts2x = point->z;
				verts2y = point->x;
				break;

		case	VectorComponent_Z:
				pX = point3D->x;
				pY = point3D->y;

				point = (OGLPoint3D *)tmp;
				verts0x = point->x;
				verts0y = point->y;

				point = (OGLPoint3D *)(tmp + skip);
				verts1x = point->x;
				verts1y = point->y;

				point = (OGLPoint3D *)(tmp + skip + skip);
				verts2x = point->x;
				verts2y = point->y;
				break;
	}


			/* NOW DO 2D POINT-IN-TRIANGLE CHECK */

	return ((Boolean)IsPointInTriangle(pX, pY, verts0x, verts0y, verts1x, verts1y, verts2x, verts2y));
}

#pragma mark -


/******************** INTERSECT LINE SEGMENTS ************************/
//
// OUTPUT: x,y = coords of intersection
//			true = yes, intersection occured
//

Boolean IntersectLineSegments(float x1, float y1, float x2, float y2,
		                     float x3, float y3, float x4, float y4,
                             float *x, float *y)
{
float 	a1, a2, b1, b2, c1, c2; 				// Coefficients of line eqns.
long 	r1, r2, r3, r4;         				// 'Sign' values
float 	denom, offset, num;     				// Intermediate values
float	max1,min1,max2,min2;

			/***********************************************/
			/* DO BOUNDING BOX CHECK FOR QUICK ELIMINATION */
			/***********************************************/

			/* SEE IF HORIZ OUT OF BOUNDS */

	if (x1 > x2)
	{
		max1 = x1;
		min1 = x2;
	}
	else
	{
		max1 = x2;
		min1 = x1;
	}

	if (x3 < x4)
	{
		min2 = x3;
		max2 = x4;
	}
	else
	{
		min2 = x4;
		max2 = x3;
	}

	if (max1 < min2)
		return(false);
	if (min1 > max2)
		return(false);


			/* SEE IF VERT OUT OF BOUNDS */

	if (y1 > y2)
	{
		max1 = y1;
		min1 = y2;
	}
	else
	{
		max1 = y2;
		min1 = y1;
	}

	if (y3 < y4)
	{
		min2 = y3;
		max2 = y4;
	}
	else
	{
		min2 = y4;
		max2 = y3;
	}

	if (max1 < min2)
		return(false);
	if (min1 > max2)
		return(false);





     /* Compute a1, b1, c1, where line joining points 1 and 2
      * is "a1 x  +  b1 y  +  c1  =  0".
      */

	a1 = y2 - y1;
	b1 = x1 - x2;
	c1 = (x2 * y1) - (x1 * y2);


     /* Compute r3 and r4. */

	r3 = (a1 * x3) + (b1 * y3) + c1;
	r4 = (a1 * x4) + (b1 * y4) + c1;


     /* Check signs of r3 and r4.  If both point 3 and point 4 lie on
      * same side of line 1, the line segments do not intersect.
      */

	if ((r3 != 0) && (r4 != 0) && (!((r3^r4)&0x80000000)))
         return(false);

     /* Compute a2, b2, c2 */

	a2 = y4 - y3;
	b2 = x3 - x4;
	c2 = x4 * y3 - x3 * y4;

     /* Compute r1 and r2 */

	r1 = a2 * x1 + b2 * y1 + c2;
	r2 = a2 * x2 + b2 * y2 + c2;

     /* Check signs of r1 and r2.  If both point 1 and point 2 lie
      * on same side of second line segment, the line segments do
      * not intersect.
      */

	if ((r1 != 0) && (r2 != 0) && (!((r1^r2)&0x80000000)))
		return (false);


     /* Line segments intersect: compute intersection point. */

	denom = (a1 * b2) - (a2 * b1);
	if (denom == 0.0f)
	{
		*x = x1;
		*y = y1;
		return (true);									// collinear
	}

	offset = denom < 0.0f ? - denom * .5f : denom * .5f;

     /* The denom/2 is to get rounding instead of truncating.  It
      * is added or subtracted to the numerator, depending upon the
      * sign of the numerator.
      */

	if (denom != 0.0f)
		denom = 1.0f/denom;

 	num = b1 * c2 - b2 * c1;
     *x = ( num < 0 ? num - offset : num + offset ) * denom;

	num = a2 * c1 - a1 * c2;
	*y = ( num < 0 ? num - offset : num + offset ) * denom;

	return (true);
}




/********************* CALC LINE NORMAL 2D *************************/
//
// INPUT: 	p0, p1 = 2 points on the line
//			px,py = point used to determine which way we want the normal aiming
//
// OUTPUT:	normal = normal to the line
//

void CalcLineNormal2D(float p0x, float p0y, float p1x, float p1y,
					 float	px, float py, OGLVector2D *normal)
{
OGLVector2D		normalA, normalB;
float			temp,x,y;


		/* CALC NORMALIZED VECTOR FROM ENDPOINT TO ENDPOINT */

	FastNormalizeVector2D(p0x - p1x, p0y - p1y, &normalA, false);


		/* CALC NORMALIZED VECTOR FROM REF POINT TO ENDPOINT 0 */

	FastNormalizeVector2D(px - p0x, py - p0y, &normalB, false);


	temp = -((normalB.x * normalA.y) - (normalA.x * normalB.y));
	x =   -(temp * normalA.y);
	y =   normalA.x * temp;

	FastNormalizeVector2D(x, y, normal, false);					// normalize the result
}

/********************* CALC RAY NORMAL 2D *************************/
//
// INPUT: 	vec = normalized vector of ray
//			p0 = ray origin
//			px,py = point used to determine which way we want the normal aiming
//
// OUTPUT:	normal = normal to the line
//

void CalcRayNormal2D(const OGLVector2D *vec, float p0x, float p0y,
					 float	px, float py, OGLVector2D *normal)
{
OGLVector2D		normalB;
float			temp,x,y;


		/* CALC NORMALIZED VECTOR FROM REF POINT TO ENDPOINT 0 */

	FastNormalizeVector2D(px - p0x, py - p0y, &normalB, false);


	temp = -((normalB.x * vec->y) - (vec->x * normalB.y));
	x =   -(temp * vec->y);
	y =   vec->x * temp;

	FastNormalizeVector2D(x, y, normal, false);					// normalize the result
}





/*********************** REFLECT VECTOR 2D *************************/
//
// NOTE:	This preserves the magnitude of the input vector.
//
// INPUT:	theVector = vector to reflect (not normalized)
//			N 		  = normal to reflect around (normalized)
//
// OUTPUT:	theVector = reflected vector, scaled to magnitude of original
//

void ReflectVector2D(OGLVector2D *theVector, const OGLVector2D *N)
{
float	x,y;
float	normalX,normalY;
float	dotProduct,reflectedX,reflectedY;
float	mag,oneOverM;

	x = theVector->x;
	y = theVector->y;

			/* CALC LENGTH AND NORMALIZE INPUT VECTOR */

	mag = sqrt(x*x + y*y);							// calc magnitude of input vector;
	if (mag != 0.0f)
		oneOverM = 1.0f / mag;
	else
		oneOverM = 0;
	x *= oneOverM;									// normalize
	y *= oneOverM;


	normalX = N->x;
	normalY = N->y;

	/* compute NxV */
	dotProduct = normalX * x;
	dotProduct += normalY * y;

	/* compute 2(NxV) */
	dotProduct += dotProduct;

	/* compute final vector */
	reflectedX = normalX * dotProduct - x;
	reflectedY = normalY * dotProduct - y;

	/* Normalize the result */

	FastNormalizeVector2D(reflectedX,reflectedY, theVector, false);

			/* SCALE TO ORIGINAL MAGNITUDE */

	theVector->x *= -mag;
	theVector->y *= -mag;
}


/*********************** REFLECT VECTOR 3D *************************/
//
// compute reflection vector
// which is N(2(N¥V)) - V
// N - Surface Normal
// vec = vector aiming at the normal.
//
//

void ReflectVector3D(const OGLVector3D *vec, OGLVector3D *N, OGLVector3D *out)
{
float	normalX,normalY,normalZ;
float	dotProduct,reflectedX,reflectedZ,reflectedY;
float	vx,vy,vz;

	normalX = N->x;
	normalY = N->y;
	normalZ = N->z;

	vx = -vec->x;				// we need the vector to be from normal away, so we invert it
	vy = -vec->y;
	vz = -vec->z;


	/* compute NxV */
	dotProduct = normalX * vx;
	dotProduct += normalY * vy;
	dotProduct += normalZ * vz;

	/* compute 2(NxV) */
	dotProduct += dotProduct;

	/* compute final vector */
	reflectedX = normalX * dotProduct - vx;
	reflectedY = normalY * dotProduct - vy;
	reflectedZ = normalZ * dotProduct - vz;

	/* Normalize the result */

	FastNormalizeVector(reflectedX,reflectedY,reflectedZ,out);
}



/***************** MATRIX4X4 TRANSPOSE *********************/

void OGLMatrix4x4_Transpose(const OGLMatrix4x4 *matrix4x4, OGLMatrix4x4 *result)
{
OGLMatrix4x4 		source;
const OGLMatrix4x4	*sourcePtr;
uint32_t 				row, column;

	if (result == matrix4x4)
	{
		source = *matrix4x4;
		sourcePtr = &source;
	}
	else
		sourcePtr = matrix4x4;

	for (row = 0; row < 4; row++)
	{
		for (column = 0; column < 4; column++)
		{
			int	a,b;

			a = column*4 + row;
			b = row*4 + column;
			result->value[a] = sourcePtr->value[b];
		}
	}
}

#pragma mark -


/********************* OGL POINT 3D: TRANSFORM ************************/

void OGLPoint3D_Transform(const OGLPoint3D *point3D, const OGLMatrix4x4 *matrix4x4,
								OGLPoint3D *result)
{
OGLPoint3D					s;
register const OGLPoint3D	*sPtr;
float						w;
register float				inverseW;

	if (point3D == result)
	{
		s = *point3D;
		sPtr = &s;
	}
	else
		sPtr = point3D;

	result->x = sPtr->x * matrix4x4->value[M00] +
				sPtr->y * matrix4x4->value[M01] +
				sPtr->z * matrix4x4->value[M02] +
				          matrix4x4->value[M03];

	result->y = sPtr->x * matrix4x4->value[M10] +
				sPtr->y * matrix4x4->value[M11] +
				sPtr->z * matrix4x4->value[M12] +
				          matrix4x4->value[M13];

	result->z = sPtr->x * matrix4x4->value[M20] +
				sPtr->y * matrix4x4->value[M21] +
				sPtr->z * matrix4x4->value[M22] +
				          matrix4x4->value[M23];

	w 		  = sPtr->x * matrix4x4->value[M30] +
				sPtr->y * matrix4x4->value[M31] +
				sPtr->z * matrix4x4->value[M32] +
						  matrix4x4->value[M33];

	inverseW = 1.0f / w;

	result->x *= inverseW;
	result->y *= inverseW;
	result->z *= inverseW;
}



/**************** GLMATRIX4X4: SETSCALE ********************/

void OGLMatrix4x4_SetScale(OGLMatrix4x4 *m, float x, float y, float z)
{
	m->value[M00] = x;
	m->value[M11] = y;
	m->value[M22] = z;
	m->value[M33] = 1;

	m->value[M01] = 0;
	m->value[M02] = 0;
	m->value[M03] = 0;

	m->value[M10] = 0;
	m->value[M12] = 0;
	m->value[M13] = 0;

	m->value[M20] = 0;
	m->value[M21] = 0;
	m->value[M23] = 0;

	m->value[M30] = 0;
	m->value[M31] = 0;
	m->value[M32] = 0;
}


/**************** GLMATRIX4X4: SET ROTATE X ******************/

void OGLMatrix4x4_SetRotate_X(OGLMatrix4x4	*m, float angle)
{
float s = sin(angle);
float c = cos(angle);

	OGLMatrix4x4_SetIdentity(m);

	m->value[M11] = c;
	m->value[M12] = -s;
	m->value[M21] = s;
	m->value[M22] = c;
}


/**************** GLMATRIX4X4: SET ROTATE Y ******************/

void OGLMatrix4x4_SetRotate_Y(OGLMatrix4x4	*m, float angle)
{
float s = sin(angle);
float c = cos(angle);

	OGLMatrix4x4_SetIdentity(m);

	m->value[M00] = c;
	m->value[M02] = s;
	m->value[M20] = -s;
	m->value[M22] = c;
}


/**************** GLMATRIX4X4: SET ROTATE Z ******************/

void OGLMatrix4x4_SetRotate_Z(OGLMatrix4x4	*m, float angle)
{
float s = sin(angle);
float c = cos(angle);

	OGLMatrix4x4_SetIdentity(m);

	m->value[M00] = c;
	m->value[M01] = -s;
	m->value[M10] = s;
	m->value[M11] = c;
}


/*************** GL MATRIX 4X4: SET ROTATE ABOUT POINT ******************/

void OGLMatrix4x4_SetRotateAboutPoint(OGLMatrix4x4 *matrix4x4, const OGLPoint3D	*origin,
									float xAngle, float yAngle, float zAngle)
{
OGLMatrix4x4		negTransM, rotM;

	OGLMatrix4x4_SetTranslate(&negTransM, -origin->x, -origin->y, -origin->z);
	OGLMatrix4x4_SetTranslate(matrix4x4, origin->x, origin->y, origin->z);

	OGLMatrix4x4_SetRotate_XYZ(&rotM, xAngle, yAngle, zAngle);

	OGLMatrix4x4_Multiply(&rotM, matrix4x4, matrix4x4);
	OGLMatrix4x4_Multiply(&negTransM, matrix4x4, matrix4x4);
}


/****************** OGL MATRIX 4X4 MULTIPLY ********************/

void OGLMatrix4x4_Multiply(const OGLMatrix4x4	*mA, const OGLMatrix4x4 *mB, OGLMatrix4x4	*result)
{
float	b00, b01, b02, b03;
float	b10, b11, b12, b13;
float	b20, b21, b22, b23;
float	b30, b31, b32, b33;
float	ax0, ax1, ax2, ax3;

	b00 = mB->value[M00];	b01 = mB->value[M10];	b02 = mB->value[M20];	b03 = mB->value[M30];
	b10 = mB->value[M01];	b11 = mB->value[M11];	b12 = mB->value[M21];	b13 = mB->value[M31];
	b20 = mB->value[M02];	b21 = mB->value[M12];	b22 = mB->value[M22];	b23 = mB->value[M32];
	b30 = mB->value[M03];	b31 = mB->value[M13];	b32 = mB->value[M23];	b33 = mB->value[M33];

	ax0 = mA->value[M00];	ax1 = mA->value[M10];
	ax2 = mA->value[M20];	ax3 = mA->value[M30];

	result->value[M00] = ax0*b00 + ax1*b10 + ax2*b20 + ax3*b30;
	result->value[M10] = ax0*b01 + ax1*b11 + ax2*b21 + ax3*b31;
	result->value[M20] = ax0*b02 + ax1*b12 + ax2*b22 + ax3*b32;
	result->value[M30] = ax0*b03 + ax1*b13 + ax2*b23 + ax3*b33;

	ax0 = mA->value[M01];	ax1 = mA->value[M11];
	ax2 = mA->value[M21];	ax3 = mA->value[M31];

	result->value[M01] = ax0*b00 + ax1*b10 + ax2*b20 + ax3*b30;
	result->value[M11] = ax0*b01 + ax1*b11 + ax2*b21 + ax3*b31;
	result->value[M21] = ax0*b02 + ax1*b12 + ax2*b22 + ax3*b32;
	result->value[M31] = ax0*b03 + ax1*b13 + ax2*b23 + ax3*b33;

	ax0 = mA->value[M02];	ax1 = mA->value[M12];
	ax2 = mA->value[M22];	ax3 = mA->value[M32];

	result->value[M02] = ax0*b00 + ax1*b10 + ax2*b20 + ax3*b30;
	result->value[M12] = ax0*b01 + ax1*b11 + ax2*b21 + ax3*b31;
	result->value[M22] = ax0*b02 + ax1*b12 + ax2*b22 + ax3*b32;
	result->value[M32] = ax0*b03 + ax1*b13 + ax2*b23 + ax3*b33;

	ax0 = mA->value[M03];	ax1 = mA->value[M13];
	ax2 = mA->value[M23];	ax3 = mA->value[M33];

	result->value[M03] = ax0*b00 + ax1*b10 + ax2*b20 + ax3*b30;
	result->value[M13] = ax0*b01 + ax1*b11 + ax2*b21 + ax3*b31;
	result->value[M23] = ax0*b02 + ax1*b12 + ax2*b22 + ax3*b32;
	result->value[M33] = ax0*b03 + ax1*b13 + ax2*b23 + ax3*b33;

}


/************** MATRIX4X4 SET TRANSLATE ******************/

void OGLMatrix4x4_SetTranslate(OGLMatrix4x4 *m, float x, float y, float z)
{
	m->value[M03] = x;
	m->value[M13] = y;
	m->value[M23] = z;

	m->value[M00] = 1;
	m->value[M11] = 1;
	m->value[M22] = 1;
	m->value[M33] = 1;

	m->value[M01] = 0;
	m->value[M02] = 0;
	m->value[M10] = 0;
	m->value[M12] = 0;
	m->value[M20] = 0;
	m->value[M21] = 0;
	m->value[M30] = 0;
	m->value[M31] = 0;
	m->value[M32] = 0;
}


/***************** MATRIX 4X4: GET FRUSTUM TO WINDOW *****************/

void OGLMatrix4x4_GetFrustumToWindow(const OGLSetupOutputType *setupInfo, OGLMatrix4x4 *m, int pane)
{
int		x,y,w,h;
float	width, height;

	OGL_GetCurrentViewport(setupInfo, &x, &y, &w, &h, pane);

	width = w;
	height = h;

	OGLMatrix4x4_SetIdentity(m);

	m->value[M00] =	width  * 0.5f;
	m->value[M11] = -height * 0.5f;
	m->value[M03] = width  * 0.5f;
	m->value[M13] = height  * 0.5f;
}



/************** MATRIX3X3 SET TRANSLATE ******************/

void OGLMatrix3x3_SetTranslate(OGLMatrix3x3 *m, float x, float y)
{
	m->value[N02] = x;
	m->value[N12] = y;

	m->value[N00] = 1;
	m->value[N11] = 1;
	m->value[N22] = 1;

	m->value[N01] = 0;
	m->value[N02] = 0;
	m->value[N10] = 0;
	m->value[N12] = 0;
	m->value[N20] = 0;
	m->value[N21] = 0;
}


/************* MATRIX 4X4 SET IDENTITY ******************/

void OGLMatrix4x4_SetIdentity(OGLMatrix4x4 *m)
{
	m->value[M00] = 1;
	m->value[M11] = 1;
	m->value[M22] = 1;
	m->value[M33] = 1;

	m->value[M01] = 0;
	m->value[M02] = 0;
	m->value[M03] = 0;
	m->value[M10] = 0;
	m->value[M12] = 0;
	m->value[M13] = 0;
	m->value[M20] = 0;
	m->value[M21] = 0;
	m->value[M23] = 0;
	m->value[M30] = 0;
	m->value[M31] = 0;
	m->value[M32] = 0;
}


/******************* SET QUICK XYZ-ROTATION MATRIX ************************/
//
// Does a quick precomputation to calculate an XYZ rotation matrix
//

void OGLMatrix4x4_SetRotate_XYZ(OGLMatrix4x4 *m, float rx, float ry, float rz)
{
float	sx,cx,sy,sz,cy,cz,sxsy,cxsy;

	sx = sin(rx);
	sy = sin(ry);
	sz = sin(rz);
	cx = cos(rx);
	cy = cos(ry);
	cz = cos(rz);

	sxsy = sx*sy;
	cxsy = cx*sy;

	m->value[M00] = cy*cz;					m->value[M10] = cy*sz; 				m->value[M20] = -sy; 	m->value[M30] = 0;
	m->value[M01] = (sxsy*cz)+(cx*-sz);		m->value[M11] = (sxsy*sz)+(cx*cz);	m->value[M21] = sx*cy;	m->value[M31] = 0;
	m->value[M02] = (cxsy*cz)+(-sx*-sz);	m->value[M12] = (cxsy*sz)+(-sx*cz);	m->value[M22] = cx*cy;	m->value[M32] = 0;
	m->value[M03] = 0;						m->value[M13] = 0;					m->value[M23] = 0;		m->value[M33] = 1;
}


/********************* SET ROTATE ABOUT AXIS ***************************/

void OGLMatrix4x4_SetRotateAboutAxis(OGLMatrix4x4	*m, const OGLVector3D	*axis, float angle)
{
float			sine,cosine,t,axy,axz,ayz;
float	ax	= axis->x;
float	ay	= axis->y;
float	az	= axis->z;
float	ax2 = ax * ax;
float	ay2 = ay * ay;
float	az2	= az * az;

	axy = ax * ay;
	axz = ax * az;
	ayz = ay * az;

	sine   = sin(angle);
	cosine = cos(angle);
	t = 1.0f - cosine;

	OGLMatrix4x4_SetIdentity(m);

	m->value[M00] = t * ax2 + cosine;
	m->value[M10] = t * axy + sine * az;
	m->value[M20] = t * axz - sine * ay;

	m->value[M01] = t * axy - sine * az;
	m->value[M11] = t * ay2 + cosine;
	m->value[M21] = t * ayz + sine * ax;

	m->value[M02] = t * axz + sine * ay;
	m->value[M12] = t * ayz - sine * ax;
	m->value[M22] = t * az2 + cosine;
}



/***************** OGL MATRIX3X3 SET ROTATE ABOUT POINT *******************/

void OGLMatrix3x3_SetRotateAboutPoint(OGLMatrix3x3 *m, const OGLPoint2D *origin, float angle)
{
#if 0
float 			sine 	= sin(angle);
float		 	cosine 	= cos(angle);
OGLMatrix3x3	tmp1, tmp2;

	OGLMatrix3x3_SetTranslate(&tmp1, -origin->x, -origin->y);
	OGLMatrix3x3_SetTranslate(m, origin->x, origin->y);

	OGLMatrix3x3_SetIdentity(&tmp2);

	tmp2.value[N00] =  cosine;
	tmp2.value[N10] =  sine;
	tmp2.value[N01] = -sine;
	tmp2.value[N11] =  cosine;

	OGLMatrix3x3_Multiply(&tmp2, m, m);
	OGLMatrix3x3_Multiply(&tmp1, m, m);
#else

	float			sina = sin(angle);
	float			cosa = cos(angle);

	OGLMatrix3x3_SetIdentity(m);

	m->value[N00] = cosa;
	m->value[N10] = sina;
	m->value[N01] = -sina;
	m->value[N11] = cosa;
	m->value[N02] = -(origin->x * cosa) + (origin->y * sina) + origin->x;
	m->value[N12] = -(origin->x * sina) + (origin->y * cosa) + origin->y;

#endif
}

/***************** OGL MATRIX3X3 SET ROTATE *******************/

void OGLMatrix3x3_SetRotate(OGLMatrix3x3 *m, float angle)
{
float 			sine 	= sin(angle);
float		 	cosine 	= cos(angle);

	OGLMatrix3x3_SetIdentity(m);

	m->value[N00] =  cosine;
	m->value[N10] =  sine;
	m->value[N01] = -sine;
	m->value[N11] =  cosine;
}



/******************* OGL MATRIX 3X3 SET IDENTITY ****************/

void OGLMatrix3x3_SetIdentity(OGLMatrix3x3 *m)
{
	m->value[N00] = 1;
	m->value[N11] = 1;
	m->value[N22] = 1;

	m->value[N01] = 0;
	m->value[N02] = 0;
	m->value[N10] = 0;
	m->value[N12] = 0;
	m->value[N20] = 0;
	m->value[N21] = 0;
}


/***************** OGL MATRIX 3X3 MULTIPLY **********************/

void OGLMatrix3x3_Multiply(const OGLMatrix3x3	*mA,
							const OGLMatrix3x3	*mB,
							OGLMatrix3x3		*result)
{
float	b00, b01, b02;
float	b10, b11, b12;
float	b20, b21, b22;
float	ax0, ax1, ax2;

	b00 = mB->value[N00];	b01 = mB->value[N10];	b02 = mB->value[N20];
	b10 = mB->value[N01];	b11 = mB->value[N11];	b12 = mB->value[N21];
	b20 = mB->value[N02];	b21 = mB->value[N12];	b22 = mB->value[N22];

	ax0 = mA->value[N00];	ax1 = mA->value[N10];
	ax2 = mA->value[N20];

	result->value[N00] = ax0*b00 + ax1*b10 + ax2*b20;
	result->value[N10] = ax0*b01 + ax1*b11 + ax2*b21;
	result->value[N20] = ax0*b02 + ax1*b12 + ax2*b22;

	ax0 = mA->value[N01];	ax1 = mA->value[N11];
	ax2 = mA->value[N21];

	result->value[N01] = ax0*b00 + ax1*b10 + ax2*b20;
	result->value[N11] = ax0*b01 + ax1*b11 + ax2*b21;
	result->value[N21] = ax0*b02 + ax1*b12 + ax2*b22;

	ax0 = mA->value[N02];	ax1 = mA->value[N12];
	ax2 = mA->value[N22];

	result->value[N02] = ax0*b00 + ax1*b10 + ax2*b20;
	result->value[N12] = ax0*b01 + ax1*b11 + ax2*b21;
	result->value[N22] = ax0*b02 + ax1*b12 + ax2*b22;
}

/************** OGL:  POINT 2D TRANSFORM ******************/

void OGLPoint2D_Transform(const OGLPoint2D *p, const OGLMatrix3x3 *m, OGLPoint2D *result)
{
float newx = (p->x * m->value[N00]) + (p->y * m->value[N01]) + m->value[N02];
float newy = (p->x * m->value[N10]) + (p->y * m->value[N11]) + m->value[N12];
float neww = (p->x * m->value[N20]) + (p->y * m->value[N21]) + m->value[N22];

	if (neww == 1.0f)
	{
		result->x = newx;
		result->y = newy;
	}
	else
	{
		float invw = 1.0f / neww;
		result->x = newx * invw;
		result->y = newy * invw;
	}
}

/************* OGL VECTOR 2D TRANSFORM ****************/

void OGLVector2D_Transform(const OGLVector2D *vector2D,
							const OGLMatrix3x3	*matrix3x3,
							OGLVector2D			*result)
{
OGLVector2D			s;
const OGLVector2D	*sPtr;

	if (vector2D == result)
	{
		s = *vector2D;
		sPtr = &s;
	}
	else
		sPtr = vector2D;

	result->x = sPtr->x * matrix3x3->value[N00] +
				sPtr->y * matrix3x3->value[N01];

	result->y = sPtr->x * matrix3x3->value[N10] +
				sPtr->y * matrix3x3->value[N11];

}


/****************** OGLVECTOR3D DOT ******************/

float OGLVector3D_Dot(const OGLVector3D	*v1, const OGLVector3D	*v2)
{
float	dot;

	dot = ((v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z));		// calc dot

			/* CHECK FOR FLOATING POINT PRECISION PROBLEMS */
			//
			// Since the acos of anything >1.0 is a NaN, lets be careful
			// that we return something valid!
			//

	if (dot > 1.0f)
		dot = 1.0f;
	else
	if (dot < -1.0f)
		dot = -1.0f;

	return(dot);
}


/************* VECTOR 2D DOT ****************/
//
// 0.0 == perpendicular, 1.0 = parallel
//

float OGLVector2D_Dot(const OGLVector2D	*v1,  const OGLVector2D	*v2)
{
float	dot;

	dot = v1->x * v2->x + v1->y * v2->y;

			/* CHECK FOR FLOATING POINT PRECISION PROBLEMS */
			//
			// Since the acos of anything >1.0 is a NaN, lets be careful
			// that we return something valid!
			//

	if (dot > 1.0f)
		dot = 1.0f;
	else
	if (dot < -1.0f)
		dot = -1.0f;

	return(dot);
}


/************** VECTOR 3D NORMALIZE *****************/

void OGLVector3D_Normalize(const OGLVector3D *vector3D, OGLVector3D	*result)
{
float length, oneOverLength;

	length = (vector3D->x * vector3D->x) +
			 (vector3D->y * vector3D->y) +
			 (vector3D->z * vector3D->z);

	length = sqrt(length);

     //  Check for zero-length vector

    if (length <= EPS)
    {
       	result->x =
       	result->y =
       	result->z = 0;
    }
    else
    {
    	oneOverLength = 1.0f/length;

		result->x = vector3D->x * oneOverLength;
		result->y = vector3D->y * oneOverLength;
		result->z = vector3D->z * oneOverLength;
	}
}


/************** VECTOR 2D NORMALIZE *****************/

void OGLVector2D_Normalize(const OGLVector2D *vector2D, OGLVector2D	*result)
{
float length, oneOverLength;

	length = (vector2D->x * vector2D->x) + (vector2D->y * vector2D->y);

	length = sqrt(length);

     //  Check for zero-length vector

    if (length <= EPS)
    {
       	*result = *vector2D;
    }
    else
    {
    	oneOverLength = 1.0f/length;

		result->x = vector2D->x * oneOverLength;
		result->y = vector2D->y * oneOverLength;
	}
}


/****************** VECTOR 3D CROSS *********************/

void OGLVector3D_Cross(const OGLVector3D *v1, const OGLVector3D	*v2, OGLVector3D *result)
{
OGLVector3D			s1, s2, temp;
const OGLVector3D	*s1Ptr, *s2Ptr;

	if (v1 == result)
	{
		s1 = *v1;
		s1Ptr = &s1;
	}
	else
	{
		s1Ptr = v1;
	}

	if (v2 == result)
	{
		s2 = *v2;
		s2Ptr = &s2;
	}
	else
	{
		s2Ptr = v2;
	}

	temp.x = s1Ptr->y * s2Ptr->z - s2Ptr->y * s1Ptr->z;
	temp.y = s2Ptr->x * s1Ptr->z - s1Ptr->x * s2Ptr->z;
	temp.z = s1Ptr->x * s2Ptr->y - s2Ptr->x * s1Ptr->y;

	OGLVector3D_Normalize(&temp, result);
}


/*********************** VECTOR 3D TRANSFORM ****************************/

void OGLVector3D_Transform(const OGLVector3D *vector3D,	const OGLMatrix4x4	*matrix4x4, OGLVector3D *result)
{
OGLVector3D			s;
const OGLVector3D	*sPtr;

	/* SEE IF VECTOR BASHING */

	if (vector3D == result)
	{
		s = *vector3D;
		sPtr = &s;
	}
	else
		sPtr = vector3D;


		/* TRANSFORM IT */

	result->x = sPtr->x * matrix4x4->value[M00] +
				sPtr->y * matrix4x4->value[M01] +
				sPtr->z * matrix4x4->value[M02];

	result->y = sPtr->x * matrix4x4->value[M10] +
				sPtr->y * matrix4x4->value[M11] +
				sPtr->z * matrix4x4->value[M12];

	result->z = sPtr->x * matrix4x4->value[M20] +
				sPtr->y * matrix4x4->value[M21] +
				sPtr->z * matrix4x4->value[M22];

		/* NORMALIZE IT */

	FastNormalizeVector(result->x, result->y, result->z, result);
}


/****************** VECTOR 2D CROSS *********************/

float OGLVector2D_Cross(const OGLVector2D *v1, const OGLVector2D *v2)
{
	return ((v1->x * v2->y) - (v1->y * v2->x));
}


/****************** POINT 3D DISTANCE ***********************/

float OGLPoint3D_Distance(const OGLPoint3D *p1, const OGLPoint3D *p2)
{
float dx,dy,dz;

	dx = p1->x - p2->x;
	dy = p1->y - p2->y;
	dz = p1->z - p2->z;

	return(sqrt(dx*dx + dy*dy + dz*dz));
}


/****************** POINT 2D DISTANCE ***********************/

float OGLPoint2D_Distance(const OGLPoint2D *p1, const OGLPoint2D *p2)
{
float dx,dy;

	dx = p1->x - p2->x;
	dy = p1->y - p2->y;

	return(sqrt(dx*dx + dy*dy));
}


#pragma mark -

/******************* OGL: POINT 3D TO 4D TRANSFORM ARRAY ************************/

void OGLPoint3D_To4DTransformArray(const OGLPoint3D *inVertex, const OGLMatrix4x4  *matrix,
									OGLPoint4D *outVertex,  long numVertices)
{
float 	accum;
long	i;
float	m00,m01,m02,m03;
float	m10,m11,m12,m13;
float	m20,m21,m22,m23;
float	m30,m31,m32,m33;

	m00 = matrix->value[M00];	m01 = matrix->value[M01];	m02 = matrix->value[M02];	m03 = matrix->value[M03];
	m10 = matrix->value[M10];	m11 = matrix->value[M11];	m12 = matrix->value[M12];	m13 = matrix->value[M13];
	m20 = matrix->value[M20];	m21 = matrix->value[M21];	m22 = matrix->value[M22];	m23 = matrix->value[M23];
	m30 = matrix->value[M30];	m31 = matrix->value[M31];	m32 = matrix->value[M32];	m33 = matrix->value[M33];

	for (i = 0; i < numVertices; i++)
	{
		accum = inVertex[i].x * m00;
		accum += inVertex[i].y * m01;
		accum += inVertex[i].z * m02;
		outVertex[i].x = accum + m03;

		accum = inVertex[i].x * m10;
		accum += inVertex[i].y * m11;
		accum += inVertex[i].z * m12;
		outVertex[i].y = accum + m13;

		accum = inVertex[i].x * m20;
		accum += inVertex[i].y * m21;
		accum += inVertex[i].z * m22;
		outVertex[i].z = accum + m23;

		accum = inVertex[i].x * m30;
		accum += inVertex[i].y * m31;
		accum += inVertex[i].z * m32;
		outVertex[i].w = accum + m33;
	}
}



/******************* OGL: POINT 3D TRANSFORM ARRAY ************************/

void OGLPoint3D_TransformArray(const OGLPoint3D *inVertex, const OGLMatrix4x4  *matrix,
									OGLPoint3D *outVertex,  long numVertices)
{
float 	accum;
long	i;
float	m00,m01,m02,m03;
float	m10,m11,m12,m13;
float	m20,m21,m22,m23;
float	x,y,z;

	m00 = matrix->value[M00];	m01 = matrix->value[M01];	m02 = matrix->value[M02];	m03 = matrix->value[M03];
	m10 = matrix->value[M10];	m11 = matrix->value[M11];	m12 = matrix->value[M12];	m13 = matrix->value[M13];
	m20 = matrix->value[M20];	m21 = matrix->value[M21];	m22 = matrix->value[M22];	m23 = matrix->value[M23];

	for (i = 0; i < numVertices; i++)
	{
		x = inVertex[i].x;
		y = inVertex[i].y;
		z = inVertex[i].z;

		accum = x * m00;
		accum += y * m01;
		accum += z * m02;
		outVertex[i].x = accum + m03;

		accum = x * m10;
		accum += y * m11;
		accum += z * m12;
		outVertex[i].y = accum + m13;

		accum = x * m20;
		accum += y * m21;
		accum += z * m22;
		outVertex[i].z = accum + m23;
	}
}




#pragma mark -


/******************** OGL MATRIX 4X4: INVERT **********************/

void OGLMatrix4x4_Invert(const OGLMatrix4x4 *inMatrix, OGLMatrix4x4 *result)
{
register float 	val, val2, val_inv;
register GLint 	i, i4, i8, i12, j, ind;
OGLMatrix4x4 	mt1, mt2;


	OGLMatrix4x4_SetIdentity(&mt2);
	mt1 = *inMatrix;									// copy in matrix

	for(i = 0; i != 4; i++)
	{
		val = mt1.value[(i << 2) + i];
		ind = i;

		i4  = i + 4;
		i8  = i + 8;
		i12 = i + 12;

		for (j = i + 1; j != 4; j++)
		{
			if (fabs(mt1.value[(i << 2) + j]) > fabs(val))
			{
				ind = j;
				val = mt1.value[(i << 2) + j];
			}
		}

		if (ind != i)
		{
			val2      = mt2.value[i];
			mt2.value[i]    = mt2.value[ind];
			mt2.value[ind]  = val2;

			val2      = mt1.value[i];
			mt1.value[i]    = mt1.value[ind];
			mt1.value[ind]  = val2;

			ind += 4;

			val2      = mt2.value[i4];
			mt2.value[i4]   = mt2.value[ind];
			mt2.value[ind]  = val2;

			val2      = mt1.value[i4];
			mt1.value[i4]   = mt1.value[ind];
			mt1.value[ind]  = val2;

			ind += 4;

			val2      = mt2.value[i8];
			mt2.value[i8]   = mt2.value[ind];
			mt2.value[ind]  = val2;

			val2      = mt1.value[i8];
			mt1.value[i8]   = mt1.value[ind];
			mt1.value[ind]  = val2;

			ind += 4;

			val2      		= mt2.value[i12];
			mt2.value[i12]  = mt2.value[ind];
			mt2.value[ind]  = val2;

			val2     		= mt1.value[i12];
			mt1.value[i12]  = mt1.value[ind];
			mt1.value[ind]  = val2;
		}

		if (val == 0.0f)
			goto err;

		val_inv = 1.0f / val;

		mt1.value[i]   *= val_inv;
		mt2.value[i]   *= val_inv;

		mt1.value[i4]  *= val_inv;
		mt2.value[i4]  *= val_inv;

		mt1.value[i8]  *= val_inv;
		mt2.value[i8]  *= val_inv;

		mt1.value[i12] *= val_inv;
		mt2.value[i12] *= val_inv;

		if (i != 0)
		{
			val = mt1.value[i << 2];

			mt1.value[0]  -= mt1.value[i]   * val;
			mt2.value[0]  -= mt2.value[i]   * val;

			mt1.value[4]  -= mt1.value[i4]  * val;
			mt2.value[4]  -= mt2.value[i4]  * val;

			mt1.value[8]  -= mt1.value[i8]  * val;
			mt2.value[8]  -= mt2.value[i8]  * val;

			mt1.value[12] -= mt1.value[i12] * val;
			mt2.value[12] -= mt2.value[i12] * val;
		}

		if (i != 1)
		{
			val = mt1.value[(i << 2) + 1];

			mt1.value[1]  -= mt1.value[i]   * val;
			mt2.value[1]  -= mt2.value[i]   * val;

			mt1.value[5]  -= mt1.value[i4]  * val;
			mt2.value[5]  -= mt2.value[i4]  * val;

			mt1.value[9]  -= mt1.value[i8]  * val;
			mt2.value[9]  -= mt2.value[i8]  * val;

			mt1.value[13] -= mt1.value[i12] * val;
			mt2.value[13] -= mt2.value[i12] * val;
		}

		if (i != 2)
		{
			val = mt1.value[(i << 2) + 2];

			mt1.value[2]  -= mt1.value[i]   * val;
			mt2.value[2]  -= mt2.value[i]   * val;

			mt1.value[6]  -= mt1.value[i4]  * val;
			mt2.value[6]  -= mt2.value[i4]  * val;

			mt1.value[10] -= mt1.value[i8]  * val;
			mt2.value[10] -= mt2.value[i8]  * val;

			mt1.value[14] -= mt1.value[i12] * val;
			mt2.value[14] -= mt2.value[i12] * val;
		}

		if (i != 3)
		{
			val = mt1.value[(i << 2) + 3];

			mt1.value[3]  -= mt1.value[i]   * val;
			mt2.value[3]  -= mt2.value[i]   * val;

			mt1.value[7]  -= mt1.value[i4]  * val;
			mt2.value[7]  -= mt2.value[i4]  * val;

			mt1.value[11] -= mt1.value[i8]  * val;
			mt2.value[11] -= mt2.value[i8]  * val;

			mt1.value[15] -= mt1.value[i12] * val;
			mt2.value[15] -= mt2.value[i12] * val;
		}
	}

	*result = mt2;								// copy to result
	return;

err:
	OGLMatrix4x4_SetIdentity(result);			// error, so set result to identity
}


/******************** OGL:  IS BBOX VISIBLE ****************************/
//
// Transform all the vertices into homogenous co-ordinates, and do
// clip tests. Return true if visible, false if fully clipped.
//

Boolean OGL_IsBBoxVisible(const OGLBoundingBox *bBox)
{
float			m00, m01, m02, m03;		// Transform matrix 4x4
float			m10, m11, m12, m13;
float			m20, m21, m22, m23;
float			m30, m31, m32, m33;

float			lX, lY, lZ;				// Local space co-ordinates
float			hX, hY, hZ, hW;			// Homogeneous co-ordinates
float			minusHW;				// -hW

uint32_t		clipFlags;				// Clip in/out tests for point
uint32_t		clipCodeAND;			// Clip test for entire object

long			i;
float			minX,minY,minZ,maxX,maxY,maxZ;



		/* GET LOCAL->FRUSTUM MATRIX */

	m00 = gLocalToFrustumMatrix.value[M00];
	m01 = gLocalToFrustumMatrix.value[M01];
	m02 = gLocalToFrustumMatrix.value[M02];
	m03 = gLocalToFrustumMatrix.value[M03];
	m10 = gLocalToFrustumMatrix.value[M10];
	m11 = gLocalToFrustumMatrix.value[M11];
	m12 = gLocalToFrustumMatrix.value[M12];
	m13 = gLocalToFrustumMatrix.value[M13];
	m20 = gLocalToFrustumMatrix.value[M20];
	m21 = gLocalToFrustumMatrix.value[M21];
	m22 = gLocalToFrustumMatrix.value[M22];
	m23 = gLocalToFrustumMatrix.value[M23];
	m30 = gLocalToFrustumMatrix.value[M30];
	m31 = gLocalToFrustumMatrix.value[M31];
	m32 = gLocalToFrustumMatrix.value[M32];
	m33 = gLocalToFrustumMatrix.value[M33];


		/* TRANSFORM THE BOUNDING BOX */

	minX = bBox->min.x;								// load bbox into registers
	minY = bBox->min.y;
	minZ = bBox->min.z;
	maxX = bBox->max.x;
	maxY = bBox->max.y;
	maxZ = bBox->max.z;

	clipCodeAND = ~0;

	for (i = 0; i < 8; i++)
	{
		switch (i)									// load current bbox corner in IX,IY,IZ
		{
			case	0:	lX = minX;	lY = minY;	lZ = minZ;	break;
			case	1:							lZ = maxZ;	break;
			case	2:				lY = maxY;	lZ = minZ;	break;
			case	3:							lZ = maxZ;	break;
			case	4:	lX = maxX;	lY = minY;	lZ = minZ;	break;
			case	5:							lZ = maxZ;	break;
			case	6:				lY = maxY;	lZ = minZ;	break;
			default:							lZ = maxZ;
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
	}

	if (clipCodeAND)
		return(false);
	else
		return(true);
}


#pragma mark -

/***************** CREATE FROM TO ROTATION MATRIX ********************/
//
// A function for creating a rotation matrix that rotates a vector called
// "from" into another vector called "to".
// Author: Tomas Möller, 1999
//

void OGLCreateFromToRotationMatrix(const OGLVector3D *from, const OGLVector3D *to, OGLMatrix4x4 *m)
{
OGLVector3D v;
float 		e,h;

	OGLVector3D_Cross(from, to, &v);

	e = OGLVector3D_Dot(from, to);


  	if (e > (1.0-EPS))     				// "from" almost or equal to "to"-vector?
	{
		OGLMatrix4x4_SetIdentity(m);
  	}
  	else
  	if (e < (-1.0+EPS)) 				// "from" almost or equal to negated "to"?
 	{
    	OGLVector3D up,left;
    	float 	fxx,fyy,fzz,fxy,fxz,fyz;
    	float 	uxx,uyy,uzz,uxy,uxz,uyz;
    	float 	lxx,lyy,lzz,lxy,lxz,lyz;

	    /* left=CROSS(from, (1,0,0)) */

		left.x = 0.0;
		left.y = from->z;
		left.z = -from->y;

    	if (OGLVector3D_Dot(&left,&left) < EPS) 	// was left=CROSS(from,(1,0,0)) a good choice?
    	{
      		/* here we know that left = CROSS(from, (1,0,0)) will be a good choice */

			left.x = -from->z;
			left.y = 0.0;
			left.z = from->x;
    	}

		    /* normalize "left" */

		OGLVector3D_Normalize(&left, &left);

	    OGLVector3D_Cross(&left, from, &up);


	    // now we have a coordinate system, i.e., a basis;
	    // M=(from, up, left), and we want to rotate to:
	    // N=(-from, up, -left). This is done with the matrix:
	    // N*M^T where M^T is the transpose of M

	    fxx = -from->x * from->x; fyy = -from->y * from->y; fzz = -from->z * from->z;
	    fxy = -from->x * from->y; fxz = -from->x * from->z; fyz = -from->y * from->z;

	    uxx = up.x * up.x; uyy = up.y * up.y; uzz = up.z * up.z;
	    uxy = up.x * up.y; uxz = up.x * up.z; uyz = up.y * up.z;

	    lxx = -left.x * left.x; lyy = -left.y * left.y; lzz = -left.z * left.z;
	    lxy = -left.x * left.y; lxz = -left.x * left.z; lyz = -left.y * left.z;


	    		/* symmetric matrix */

	    m->value[M00] = fxx+uxx+lxx;
	    m->value[M01] = m->value[M10] = fxy+uxy+lxy;
	    m->value[M02] = m->value[M20] = fxz+uxz+lxz;
	    m->value[M11] = fyy+uyy+lyy;
	    m->value[M12] = m->value[M21] = fyz+uyz+lyz;
	    m->value[M22] = fzz+uzz+lzz;
	}

			/*************************************************************/
			/* the most common case, unless "from"="to", or "from"=-"to" */
			/*************************************************************/
  	else
  	{
		float  hvx,hvz,hvxy,hvxz,hvyz;

		h 		= (1.0f - e) / (v.x*v.x + v.y*v.y + v.z*v.z);
    	hvx 	= h * v.x;
    	hvz		= h * v.z;
    	hvxy 	= hvx * v.y;
    	hvxz 	= hvx * v.z;
    	hvyz 	= hvz * v.y;

 		m->value[M00] = e + hvx * v.x;
 		m->value[M10] = hvxy - v.z;
 		m->value[M20] = hvxz + v.y;
    	m->value[M01] = hvxy + v.z;
    	m->value[M11] = e + h * (v.y * v.y);
    	m->value[M21] = hvyz - v.x;
    	m->value[M02] = hvxz - v.y;
    	m->value[M12] = hvyz + v.x;
    	m->value[M22] = e + hvz * v.z;
	}
}


#pragma mark -

/**************** OGL: POINT 3D DISTANCE TO PLANE *****************/
//
// Returns a SIGNED distance!
//

float OGLPoint3D_DistanceToPlane(const OGLPoint3D *point,const OGLPlaneEquation	*plane)
{
float	d;

	d = OGLVector3D_Dot(&plane->normal, (OGLVector3D *) point);
	d += plane->constant;

	return(d);
}

/***************** OGL:  POINT 2D LINE DISTANCE ****************/

float OGLPoint2D_LineDistance(OGLPoint2D *point, float p1x, float p1y, float p2x, float p2y, float *t)
{
float	XJ, YJ;
float	p1xJ, YKJ,
		XLK, YLK;
float	DENOM;
float	XFAC, YFAC;
float	t2;

	XJ = point->x;
	YJ = point->y;

	p1xJ 	= p1x - XJ;
	YKJ 	= p1y - YJ;
	XLK 	= p2x - p1x;
	YLK 	= p2y - p1y;
	DENOM 	= XLK * XLK + YLK * YLK;

	if (DENOM < EPS)
	{
		*t = 0;
		return (sqrt(p1xJ*p1xJ + YKJ*YKJ));
	}
	else
	{
		t2 = -(p1xJ*XLK + YKJ*YLK)/DENOM;
		t2 = Math_Min(Math_Max(t2, 0.0f), 1.0f);
		XFAC = p1xJ + t2 * XLK;
		YFAC = YKJ + t2 * YLK;

		*t = t2;
		return (sqrt(XFAC*XFAC + YFAC*YFAC));
	}
}



#pragma mark -

/********************** FILL PROJECTION MATRIX ************************/
//
// Result equivalent to matrix produced by gluPerspective
//

void OGL_SetGluPerspectiveMatrix(
		OGLMatrix4x4* m,
		float fov,
		float aspect,
		float hither,
		float yon)
{
	float cotan = 1.0f / tanf(fov/2.0f);
	float depth = hither - yon;

#define M m->value
	M[M00] = cotan/aspect;	M[M01] = 0;			M[M02] = 0;						M[M03] = 0;
	M[M10] = 0;				M[M11] = cotan;		M[M12] = 0;						M[M13] = 0;
	M[M20] = 0;				M[M21] = 0;			M[M22] = (yon+hither)/depth;	M[M23] = 2*yon*hither/depth;
	M[M30] = 0;				M[M31] = 0;			M[M32] = -1;					M[M33] = 0;
#undef M
}

/********************** FILL LOOKAT MATRIX ************************/
//
// Result equivalent to matrix produced by gluLookAt
//
void OGL_SetGluLookAtMatrix(
		OGLMatrix4x4* m,
		const OGLPoint3D* eye,
		const OGLPoint3D* target,
		const OGLVector3D* upDir)
{
	// Forward = target - eye
	OGLVector3D fwd;
	fwd.x = target->x - eye->x;
	fwd.y = target->y - eye->y;
	fwd.z = target->z - eye->z;
	OGLVector3D_Normalize(&fwd, &fwd);

	// Side = forward x up
	OGLVector3D side;
	OGLVector3D_RawCross(&fwd, upDir, &side);
	OGLVector3D_Normalize(&side, &side);

	// Recompute up as: up = side x forward
	OGLVector3D up;
	OGLVector3D_RawCross(&side, &fwd, &up);

	// Premultiply by translation to eye position
	float tx = OGLVector3D_RawDot(&side,	(OGLVector3D*)eye);
	float ty = OGLVector3D_RawDot(&up,		(OGLVector3D*)eye);
	float tz = OGLVector3D_RawDot(&fwd,		(OGLVector3D*)eye);

#define M m->value
	M[M00] = side.x;	M[M01] = side.y;	M[M02] = side.z;	M[M03] = -tx;
	M[M10] = up.x;		M[M11] = up.y;		M[M12] = up.z;		M[M13] = -ty;
	M[M20] = -fwd.x;	M[M21] = -fwd.y;	M[M22] = -fwd.z;	M[M23] = tz;
	M[M30] = 0;			M[M31] = 0;			M[M32] = 0;			M[M33] = 1;
#undef M
}

