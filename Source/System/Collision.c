/****************************/
/*   	COLLISION.c		    */
/* (c)1997-2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void AllocateCollisionTriangleMemory(ObjNode *theNode, long numTriangles);

static Boolean RayIntersectTriangle(OGLPoint3D *origin, OGLVector3D *dir,
                   			OGLPoint3D *v0, OGLPoint3D *v1, OGLPoint3D *v2,
                   			Boolean	cullTest, float *t, float *u, float *v);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_COLLISIONS				60
#define	MAX_TEMP_COLL_TRIANGLES		300

enum
{
	WH_HEAD	=	1,
	WH_FOOT =	1<<1
};

/****************************/
/*    VARIABLES             */
/****************************/


CollisionRec	gCollisionList[MAX_COLLISIONS];
short			gNumCollisions = 0;
Byte			gTotalSides;


/******************* COLLISION DETECT *********************/
//
// INPUT: startNumCollisions = value to start gNumCollisions at should we need to keep existing data in collision list
//

void CollisionDetect(ObjNode *baseNode, uint32_t CType, short startNumCollisions)
{
ObjNode 	*thisNode;
long		sideBits,cBits,cType;
float		relDX,relDY,relDZ;						// relative deltas
float		realDX,realDY,realDZ;					// real deltas
float		x,y,z,oldX,oldZ,oldY;
short		numBaseBoxes,targetNumBoxes,target;
CollisionBoxType *baseBoxList;
CollisionBoxType *targetBoxList;
long		leftSide,rightSide,frontSide,backSide,bottomSide,topSide;

	gNumCollisions = startNumCollisions;								// clear list

			/* GET BASE BOX INFO */

	numBaseBoxes = baseNode->NumCollisionBoxes;
	if (numBaseBoxes == 0)
		return;
	baseBoxList = baseNode->CollisionBoxes;

	leftSide 		= baseBoxList->left;
	rightSide 		= baseBoxList->right;
	frontSide 		= baseBoxList->front;
	backSide 		= baseBoxList->back;
	bottomSide 		= baseBoxList->bottom;
	topSide 		= baseBoxList->top;

	x = gCoord.x;									// copy coords into registers
	y = gCoord.y;
	z = gCoord.z;


	oldY = baseNode->OldCoord.y;
	oldX = baseNode->OldCoord.x;
	oldZ = baseNode->OldCoord.z;

			/* CALC REAL DELTA */

	realDX = gCoord.x - oldX;
	realDY = gCoord.y - oldY;
	realDZ = gCoord.z - oldZ;



			/****************************/
			/* SCAN AGAINST ALL OBJECTS */
			/****************************/

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		cType = thisNode->CType;
		if (cType == INVALID_NODE_FLAG)							// see if something went wrong
			break;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (!(cType & CType))									// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;

		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;

		if (thisNode == baseNode)								// dont collide against itself
			goto next;

		if (baseNode->ChainNode == thisNode)					// don't collide against its own chained object
			goto next;

				/******************************/
				/* NOW DO COLLISION BOX CHECK */
				/******************************/

		targetNumBoxes = thisNode->NumCollisionBoxes;			// see if target has any boxes
		if (targetNumBoxes)
		{
			targetBoxList = thisNode->CollisionBoxes;


				/******************************************/
				/* CHECK BASE BOX AGAINST EACH TARGET BOX */
				/*******************************************/

			for (target = 0; target < targetNumBoxes; target++)
			{
						/* DO RECTANGLE INTERSECTION */

				if (rightSide < targetBoxList[target].left)
					continue;

				if (leftSide > targetBoxList[target].right)
					continue;

				if (frontSide < targetBoxList[target].back)
					continue;

				if (backSide > targetBoxList[target].front)
					continue;

				if (bottomSide > targetBoxList[target].top)
					continue;

				if (topSide < targetBoxList[target].bottom)
					continue;


						/* THERE HAS BEEN A COLLISION SO CHECK WHICH SIDE PASSED THRU */

				sideBits = 0;
				cBits = thisNode->CBits;					// get collision info bits

				if (cBits & CBITS_TOUCHABLE)				// if it's generically touchable, then add it without side info
					goto got_sides;

				relDX = realDX - thisNode->Delta.x;			// calc relative deltas
				relDY = realDY - thisNode->Delta.y;
				relDZ = realDZ - thisNode->Delta.z;

								/* CHECK FRONT COLLISION */

				if ((cBits & SIDE_BITS_BACK) && (relDZ > 0.0f))						// see if target has solid back & we are going relatively +Z
				{
					if (baseBoxList->oldFront < targetBoxList[target].oldBack)		// get old & see if already was in target (if so, skip)
					{
						if ((baseBoxList->front >= targetBoxList[target].back) &&	// see if currently in target
							(baseBoxList->front <= targetBoxList[target].front))
						{
							sideBits = SIDE_BITS_FRONT;
						}
					}
				}
				else
								/* CHECK BACK COLLISION */

				if ((cBits & SIDE_BITS_FRONT) && (relDZ < 0.0f))					// see if target has solid front & we are going relatively -Z
				{
					if (baseBoxList->oldBack > targetBoxList[target].oldFront)		// get old & see if already was in target
					{
						if ((baseBoxList->back <= targetBoxList[target].front) &&	// see if currently in target
							(baseBoxList->back >= targetBoxList[target].back))
						{
							sideBits = SIDE_BITS_BACK;
						}
					}
				}


								/* CHECK RIGHT COLLISION */


				if ((cBits & SIDE_BITS_LEFT) && (relDX > 0.0f))						// see if target has solid left & we are going relatively right
				{
					if (baseBoxList->oldRight < targetBoxList[target].oldLeft)		// get old & see if already was in target
					{
						if ((baseBoxList->right >= targetBoxList[target].left) &&	// see if currently in target
							(baseBoxList->right <= targetBoxList[target].right))
						{
							sideBits |= SIDE_BITS_RIGHT;
						}
					}
				}
				else

							/* CHECK COLLISION ON LEFT */

				if ((cBits & SIDE_BITS_RIGHT) && (relDX < 0.0f))					// see if target has solid right & we are going relatively left
				{
					if (baseBoxList->oldLeft > targetBoxList[target].oldRight)		// get old & see if already was in target
					{
						if ((baseBoxList->left <= targetBoxList[target].right) &&	// see if currently in target
							(baseBoxList->left >= targetBoxList[target].left))
						{
							sideBits |= SIDE_BITS_LEFT;
						}
					}
				}

								/* CHECK TOP COLLISION */

				if ((cBits & SIDE_BITS_BOTTOM) && (relDY > 0.0f))					// see if target has solid bottom & we are going relatively up
				{
					if (baseBoxList->oldTop < targetBoxList[target].oldBottom)		// get old & see if already was in target
						if ((baseBoxList->top >= targetBoxList[target].bottom) &&	// see if currently in target
							(baseBoxList->top <= targetBoxList[target].top))
							sideBits |= SIDE_BITS_TOP;
				}

							/* CHECK COLLISION ON BOTTOM */

				else
				if ((cBits & SIDE_BITS_TOP) && (relDY < 0.0f))						// see if target has solid top & we are going relatively down
				{
					if (baseBoxList->oldBottom >= targetBoxList[target].oldTop)		// get old & see if already was in target
					{
						if ((baseBoxList->bottom <= targetBoxList[target].top) &&	// see if currently in target
							(baseBoxList->bottom >= targetBoxList[target].bottom))
						{
							sideBits |= SIDE_BITS_BOTTOM;
						}
					}
				}

								 /* SEE IF ANYTHING TO ADD */

				if (cType & CTYPE_IMPENETRABLE)										// if its impenetrable, add to list regardless of sides
					goto got_sides;

				if (!sideBits)														// see if anything actually happened
				{
					continue;
				}

						/* ADD TO COLLISION LIST */
got_sides:
				gCollisionList[gNumCollisions].baseBox = 0;
				gCollisionList[gNumCollisions].targetBox = target;
				gCollisionList[gNumCollisions].sides = sideBits;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
				gCollisionList[gNumCollisions].objectPtr = thisNode;
				gNumCollisions++;
				gTotalSides |= sideBits;											// remember total of this
			}
		}
next:
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);

	if (gNumCollisions > MAX_COLLISIONS)											// see if overflowed (memory corruption ensued)
		DoFatalAlert("CollisionDetect: gNumCollisions > MAX_COLLISIONS");
}


/***************** HANDLE COLLISIONS ********************/
//
// This is a generic collision handler.  Takes care of
// all processing.
//
// INPUT:  cType = CType bit mask for collision matching
//
// OUTPUT: totalSides
//

Byte HandleCollisions(ObjNode *theNode, uint32_t cType, float deltaBounce)
{
Byte		totalSides;
short		i;
float		originalX,originalY,originalZ;
long		offset,maxOffsetX,maxOffsetZ,maxOffsetY;
long		offXSign,offZSign,offYSign;
Byte		base,target;
ObjNode		*targetObj;
CollisionBoxType *baseBoxPtr,*targetBoxPtr;
long		leftSide,rightSide,frontSide,backSide,bottomSide;
CollisionBoxType *boxList;
short		numSolidHits, numPasses = 0;
Boolean		hitImpenetrable = false;
short		oldNumCollisions;

	gNumCollisions = oldNumCollisions = 0;
	gTotalSides = 0;

	originalX = gCoord.x;									// remember starting coords
	originalY = gCoord.y;
	originalZ = gCoord.z;

again:
	numSolidHits = 0;

	CalcObjectBoxFromGlobal(theNode);						// calc current collision box

	CollisionDetect(theNode,cType, gNumCollisions);			// get collision info

	totalSides = 0;
	maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
	offXSign = offYSign = offZSign = 0;

			/* GET BASE BOX INFO */

	if (theNode->NumCollisionBoxes == 0)					// it's gotta have a collision box
		return(0);
	boxList = theNode->CollisionBoxes;
	leftSide = boxList->left;
	rightSide = boxList->right;
	frontSide = boxList->front;
	backSide = boxList->back;
	bottomSide = boxList->bottom;


			/* SCAN THRU ALL RETURNED COLLISIONS */

	for (i=oldNumCollisions; i < gNumCollisions; i++)		// handle all collisions
	{
		totalSides |= gCollisionList[i].sides;				// keep sides info
		base = gCollisionList[i].baseBox;					// get collision box index for base & target
		target = gCollisionList[i].targetBox;
		targetObj = gCollisionList[i].objectPtr;			// get ptr to target objnode

		baseBoxPtr = boxList + base;						// calc ptrs to base & target collision boxes
		if (targetObj)
		{
			targetBoxPtr = targetObj->CollisionBoxes;
			targetBoxPtr += target;
		}

				/*********************************************/
				/* HANDLE ANY SPECIAL OBJECT COLLISION TYPES */
				/*********************************************/

		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
				/* SEE IF THIS OBJECT HAS SINCE BECOME INVALID */

			if (targetObj->CType == INVALID_NODE_FLAG)
				continue;

				/* HANDLE TRIGGERS */

			if ((targetObj->CType & CTYPE_TRIGGER) && (cType & CTYPE_TRIGGER))	// target must be trigger and we must have been looking for them as well
			{
				if (!HandleTrigger(targetObj,theNode,gCollisionList[i].sides))	// returns false if handle as non-solid trigger
					gCollisionList[i].sides = 0;

				maxOffsetX = gCoord.x - originalX;								// see if trigger caused a move
				maxOffsetZ = gCoord.z - originalZ;

			}
		}

					/********************************/
					/* HANDLE OBJECT COLLISIONS 	*/
					/********************************/

		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			if (gCollisionList[i].sides & ALL_SOLID_SIDES)						// see if object with any solidness
			{
				numSolidHits++;

				if (targetObj->CType & CTYPE_IMPENETRABLE)							// if this object is impenetrable, then throw out any other collision offsets
				{
					hitImpenetrable = true;
					maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
					offXSign = offYSign = offZSign = 0;
				}

				if (gCollisionList[i].sides & SIDE_BITS_BACK)						// SEE IF BACK HIT
				{
					offset = (targetBoxPtr->front - baseBoxPtr->back)+1;			// see how far over it went
					if (offset > maxOffsetZ)
					{
						maxOffsetZ = offset;
						offZSign = 1;
					}
					gDelta.z *= deltaBounce;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_FRONT)						// SEE IF FRONT HIT
				{
					offset = (baseBoxPtr->front - targetBoxPtr->back)+1;			// see how far over it went
					if (offset > maxOffsetZ)
					{
						maxOffsetZ = offset;
						offZSign = -1;
					}
					gDelta.z *= deltaBounce;
				}

				if (gCollisionList[i].sides & SIDE_BITS_LEFT)						// SEE IF HIT LEFT
				{
					offset = (targetBoxPtr->right - baseBoxPtr->left)+1;			// see how far over it went
					if (offset > maxOffsetX)
					{
						maxOffsetX = offset;
						offXSign = 1;
					}
					gDelta.x *= deltaBounce;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_RIGHT)						// SEE IF HIT RIGHT
				{
					offset = (baseBoxPtr->right - targetBoxPtr->left)+1;			// see how far over it went
					if (offset > maxOffsetX)
					{
						maxOffsetX = offset;
						offXSign = -1;
					}
					gDelta.x *= deltaBounce;
				}

				if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)						// SEE IF HIT BOTTOM
				{
					offset = (targetBoxPtr->top - baseBoxPtr->bottom)+1;			// see how far over it went
					if (offset > maxOffsetY)
					{
						maxOffsetY = offset;
						offYSign = 1;
					}
					gDelta.y = 0;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_TOP)						// SEE IF HIT TOP
				{
					offset = (baseBoxPtr->top - targetBoxPtr->bottom)+1;			// see how far over it went
					if (offset > maxOffsetY)
					{
						maxOffsetY = offset;
						offYSign = -1;
					}
					gDelta.y =0;
				}
			}
		}

		if (hitImpenetrable)						// if that was impenetrable, then we dont need to check other collisions
		{
			break;
		}
	}

		/* IF THERE WAS A SOLID HIT, THEN WE NEED TO UPDATE AND TRY AGAIN */

	if (numSolidHits > 0)
	{
				/* ADJUST MAX AMOUNTS */

		gCoord.x = originalX + (maxOffsetX * offXSign);
		gCoord.z = originalZ + (maxOffsetZ * offZSign);
		gCoord.y = originalY + (maxOffsetY * offYSign);			// y is special - we do some additional rouding to avoid the jitter problem


				/* SEE IF NEED TO SET GROUND FLAG */

		if (totalSides & SIDE_BITS_BOTTOM)
			theNode->StatusBits |= STATUS_BIT_ONGROUND;


				/* SEE IF DO ANOTHER PASS */

		numPasses++;
		if ((numPasses < 3) && (!hitImpenetrable))				// see if can do another pass and havnt hit anything impenetrable
		{
			goto again;
		}
	}


			/******************************************/
			/* SEE IF DO AUTOMATIC TERRAIN GROUND HIT */
			/******************************************/

	if (cType & CTYPE_TERRAIN)
	{
		float	y = GetTerrainY(gCoord.x, gCoord.z);

		if (bottomSide < y)										// see if bottom is under ground
		{
			gCoord.y += y-bottomSide;
			gDelta.y *= deltaBounce;
		}
	}

	return(totalSides);
}



#pragma mark ------------ POINT COLLISION -----------------

/****************** IS POINT IN POLY ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInPoly2D(float pt_x, float pt_y, Byte numVerts, OGLPoint2D *polypts)
{
Byte 		oldquad,newquad;
float 		thispt_x,thispt_y,lastpt_x,lastpt_y;
signed char	wind;										// current winding number
Byte		i;

			/************************/
			/* INIT STARTING VALUES */
			/************************/

	wind = 0;
    lastpt_x = polypts[numVerts-1].x;  					// get last point's coords
    lastpt_y = polypts[numVerts-1].y;

	if (lastpt_x < pt_x)								// calc quadrant of the last point
	{
    	if (lastpt_y < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (lastpt_y < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}


			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/

    for (i=0; i<numVerts; i++)
    {
   			/* GET THIS POINT INFO */

		thispt_x = polypts[i].x;						// get this point's coords
		thispt_y = polypts[i].y;

		if (thispt_x < pt_x)							// calc quadrant of this point
		{
	    	if (thispt_y < pt_y)
	    		newquad = 2;
	 		else
	 			newquad = 1;
	 	}
	 	else
	    {
	    	if (thispt_y < pt_y)
	    		newquad = 3;
	 		else
	 			newquad = 0;
		}

				/* SEE IF QUADRANT CHANGED */

        if (oldquad != newquad)
        {
			if (((oldquad+1)&3) == newquad)				// see if advanced
            	wind++;
			else
        	if (((newquad+1)&3) == oldquad)				// see if backed up
				wind--;
    		else
			{
				float	a,b;

             		/* upper left to lower right, or upper right to lower left.
             		   Determine direction of winding  by intersection with x==0. */

    			a = (lastpt_y - thispt_y) * (pt_x - lastpt_x);
                b = lastpt_x - thispt_x;
                a += lastpt_y * b;
                b *= pt_y;

				if (a > b)
                	wind += 2;
 				else
                	wind -= 2;
    		}
  		}

  				/* MOVE TO NEXT POINT */

   		lastpt_x = thispt_x;
   		lastpt_y = thispt_y;
   		oldquad = newquad;
	}


	return(wind); 										// non zero means point in poly
}





/****************** IS POINT IN TRIANGLE ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2)
{
Byte 		oldquad,newquad;
float		m;
signed char	wind;										// current winding number

			/*********************/
			/* DO TRIVIAL REJECT */
			/*********************/

	m = x0;												// see if to left of triangle
	if (x1 < m)
		m = x1;
	if (x2 < m)
		m = x2;
	if (pt_x < m)
		return(false);

	m = x0;												// see if to right of triangle
	if (x1 > m)
		m = x1;
	if (x2 > m)
		m = x2;
	if (pt_x > m)
		return(false);

	m = y0;												// see if to back of triangle
	if (y1 < m)
		m = y1;
	if (y2 < m)
		m = y2;
	if (pt_y < m)
		return(false);

	m = y0;												// see if to front of triangle
	if (y1 > m)
		m = y1;
	if (y2 > m)
		m = y2;
	if (pt_y > m)
		return(false);


			/*******************/
			/* DO WINDING TEST */
			/*******************/

		/* INIT STARTING VALUES */


	if (x2 < pt_x)								// calc quadrant of the last point
	{
    	if (y2 < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}


			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/

	wind = 0;


//=============================================

	if (x0 < pt_x)									// calc quadrant of this point
	{
    	if (y0 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y0 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */

    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;

         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */

			a = (y2 - y0) * (pt_x - x2);
            b = x2 - x0;
            a += y2 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}

	oldquad = newquad;

//=============================================

	if (x1 < pt_x)							// calc quadrant of this point
	{
    	if (y1 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y1 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */

    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;

         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */

			a = (y0 - y1) * (pt_x - x0);
            b = x0 - x1;
            a += y0 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}

	oldquad = newquad;

//=============================================

	if (x2 < pt_x)							// calc quadrant of this point
	{
    	if (y2 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */

    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;

         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */

			a = (y1 - y2) * (pt_x - x1);
            b = x1 - x2;
            a += y1 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}

	return(wind); 										// non zero means point in poly
}






/******************** DO SIMPLE POINT COLLISION *********************************/
//
// INPUT:  except == objNode to skip
//
// OUTPUT: # collisions detected
//

short DoSimplePointCollision(OGLPoint3D *thePoint, uint32_t cType, ObjNode *except)
{
ObjNode	*thisNode;
short	targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	gNumCollisions = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode == except)									// see if skip this one
			goto next;

		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)	// don't collide against these
			goto next;

		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;


				/* GET BOX INFO FOR THIS NODE */

		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;


			/***************************************/
			/* CHECK POINT AGAINST EACH TARGET BOX */
			/***************************************/

		for (target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */

			if (thePoint->x < targetBoxList[target].left)
				continue;

			if (thePoint->x > targetBoxList[target].right)
				continue;

			if (thePoint->z < targetBoxList[target].back)
				continue;

			if (thePoint->z > targetBoxList[target].front)
				continue;

			if (thePoint->y > targetBoxList[target].top)
				continue;

			if (thePoint->y < targetBoxList[target].bottom)
				continue;


					/* THERE HAS BEEN A COLLISION */

			gCollisionList[gNumCollisions].targetBox = target;
			gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
			gCollisionList[gNumCollisions].objectPtr = thisNode;
			gNumCollisions++;
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}


/******************** DO SIMPLE BOX COLLISION *********************************/
//
// OUTPUT: # collisions detected
//

short DoSimpleBoxCollision(float top, float bottom, float left, float right,
						float front, float back, uint32_t cType)
{
ObjNode			*thisNode;
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	gNumCollisions = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)	// don't collide against these
			goto next;

		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;


				/* GET BOX INFO FOR THIS NODE */

		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;


			/*********************************/
			/* CHECK AGAINST EACH TARGET BOX */
			/*********************************/

		for (target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */

			if (right < targetBoxList[target].left)
				continue;

			if (left > targetBoxList[target].right)
				continue;

			if (front < targetBoxList[target].back)
				continue;

			if (back > targetBoxList[target].front)
				continue;

			if (bottom > targetBoxList[target].top)
				continue;

			if (top < targetBoxList[target].bottom)
				continue;


					/* THERE HAS BEEN A COLLISION */

			gCollisionList[gNumCollisions].targetBox = target;
			gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
			gCollisionList[gNumCollisions].objectPtr = thisNode;
			gNumCollisions++;
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}


/******************** DO SIMPLE BOX COLLISION AGAINST PLAYER *********************************/

Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right,
										float front, float back, int playerNum)
{
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;


			/* GET BOX INFO FOR THIS NODE */

	targetNumBoxes = gPlayerInfo[playerNum].objNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = gPlayerInfo[playerNum].objNode->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/

	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */

		if (right < targetBoxList[target].left)
			continue;

		if (left > targetBoxList[target].right)
			continue;

		if (front < targetBoxList[target].back)
			continue;

		if (back > targetBoxList[target].front)
			continue;

		if (bottom > targetBoxList[target].top)
			continue;

		if (top < targetBoxList[target].bottom)
			continue;

		return(true);
	}

	return(false);
}



/******************** DO SIMPLE POINT COLLISION AGAINST PLAYER *********************************/

Boolean DoSimplePointCollisionAgainstPlayer(OGLPoint3D *thePoint, int playerNum)
{
short	targetNumBoxes,target;
CollisionBoxType *targetBoxList;



			/* GET BOX INFO FOR THIS NODE */

	targetNumBoxes = gPlayerInfo[playerNum].objNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = gPlayerInfo[playerNum].objNode->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/

	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */

		if (thePoint->x < targetBoxList[target].left)
			continue;

		if (thePoint->x > targetBoxList[target].right)
			continue;

		if (thePoint->z < targetBoxList[target].back)
			continue;

		if (thePoint->z > targetBoxList[target].front)
			continue;

		if (thePoint->y > targetBoxList[target].top)
			continue;

		if (thePoint->y < targetBoxList[target].bottom)
			continue;

		return(true);
	}

	return(false);
}

/******************** DO SIMPLE BOX COLLISION AGAINST OBJECT *********************************/

Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right,
										float front, float back, ObjNode *targetNode)
{
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;


			/* GET BOX INFO FOR THIS NODE */

	targetNumBoxes = targetNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = targetNode->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/

	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */

		if (right < targetBoxList[target].left)
			continue;

		if (left > targetBoxList[target].right)
			continue;

		if (front < targetBoxList[target].back)
			continue;

		if (back > targetBoxList[target].front)
			continue;

		if (bottom > targetBoxList[target].top)
			continue;

		if (top < targetBoxList[target].bottom)
			continue;

		return(true);
	}

	return(false);
}


#if 0
/*************************** DO TRIANGLE COLLISION **********************************/

void DoTriangleCollision(ObjNode *theNode, unsigned long CType)
{
ObjNode	*thisNode;
float	x,y,z,oldX,oldY,oldZ;

	x = gCoord.x;	y = gCoord.y; 	z = gCoord.z;
	oldX = theNode->OldCoord.x;
	oldY = theNode->OldCoord.y;
	oldZ = theNode->OldCoord.z;

	gNumCollisions = 0;											// clear list
	gTotalSides = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->CollisionTriangles == nil)				// must be triangles here
			goto next;

		if (!(thisNode->CType & CType))							// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;

		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;

		if (thisNode == theNode)								// dont collide against itself
			goto next;

		if (theNode->ChainNode == thisNode)						// don't collide against its own chained object
			goto next;

				/* SINCE NOTHING HIT, ADD TRIANGLE COLLISIONS */

		AddTriangleCollision(thisNode, x, y, z, oldX, oldZ, oldY);
next:
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);

}


/******************* ADD TRIANGLE COLLISION ********************************/

static void AddTriangleCollision(ObjNode *thisNode, float x, float y, float z, float oldX, float oldZ, float oldY)
{
OGLPlaneEquation 		*planeEQ;
TriangleCollisionList	*collisionRec = thisNode->CollisionTriangles;
CollisionTriangleType	*triangleList;
short					numTriangles,i;
OGLPoint3D 				intersectPt;
OGLVector3D				myVector,myNewVector;
float					normalX,normalY,normalZ;
float					dotProduct,reflectedX,reflectedZ,reflectedY;
OGLVector3D				normalizedDelta,myNormalizedVector;
float					oldMag,newMag,oldSpeed,impactFriction;
short					count;
float					offsetOldX,offsetOldY,offsetOldZ;

	if (!collisionRec)
		return;

	count = 0;													// init iteration count

try_again:
			/* FIRST CHECK IF INSIDE BOUNDING BOX */
			//
			// note: the bbox must be larger than the true bounds of the geometry
			//		for this to work correctly.  Since it's possible that in 1 frame of
			//		anim animation an object has gone thru a polygon and out of the bbox
			//		we enlarge the bbox (earlier) and cross our fingers that this will be sufficient.
			//

	if (x < collisionRec->bBox.min.x)
		return;
	if (x > collisionRec->bBox.max.x)
		return;
	if (y < collisionRec->bBox.min.y)
		return;
	if (y > collisionRec->bBox.max.y)
		return;
	if (z < collisionRec->bBox.min.z)
		return;
	if (z > collisionRec->bBox.max.z)
		return;

	myVector.x = x - oldX;														// calc my directional vector
	myVector.y = y - oldY;
	myVector.z = z - oldZ;

	if ((myVector.x == 0.0f) && (myVector.y == 0.0f) && (myVector.z == 0.0f))	// see if not moving
		return;

	FastNormalizeVector(myVector.x,myVector.y,myVector.z, &myNormalizedVector);	// normalize my delta vector

	offsetOldX = oldX - myNormalizedVector.x * .5f;								// back up the starting point by a small amount to fix for exact overlap
	offsetOldY = oldY - myNormalizedVector.y * .5f;
	offsetOldZ = oldZ - myNormalizedVector.z * .5f;


		/**************************************************************/
		/* WE'RE IN THE BBOX, SO NOW SEE IF WE INTERSECTED A TRIANGLE */
		/**************************************************************/

	numTriangles = collisionRec->numTriangles;									// get # triangles to check
	triangleList = collisionRec->triangles;										// get pointer to triangles

	for (i = 0; i < numTriangles; i++)
	{
		planeEQ = &triangleList[i].planeEQ;										// get pointer to plane equation

				/* IGNORE BACKFACES */

		dotProduct = Q3Vector3D_Dot(&planeEQ->normal, &myNormalizedVector);		// calc dot product between triangle normal & my motion vector
		if (dotProduct > 0.0f)													// if angle > 90 then skip this
			continue;


					/* SEE WHERE LINE INTERSECTS PLANE */

		if (!IntersectionOfLineSegAndPlane(planeEQ, x, y, z, offsetOldX, offsetOldY, offsetOldZ, &intersectPt))
			continue;


					/* SEE IF INTERSECT PT IS INSIDE THE TRIANGLE */

		if (IsPointInTriangle3D(&intersectPt, &triangleList[i].verts[0], &planeEQ->normal))
			goto got_it;
		else
			continue;
	}

				/* DIDNT ENCOUNTER ANY TRIANGLE COLLISIONS */

	return;


			/***********************************/
			/*       HANDLE THE COLLISION      */
			/***********************************/
got_it:

			/* ADJUST COORD TO INTERSECT POINT */

	gCoord = intersectPt;														// set new coord to intersect point
	myNewVector.x = gCoord.x - oldX;											// vector to new coord (intersect point)
	myNewVector.y = gCoord.y - oldY;
	myNewVector.z = gCoord.z - oldZ;


			/* CALC MAGNITUDE OF NEW AND OLD VECTORS */

	oldMag = sqrt((myVector.x*myVector.x) + (myVector.y*myVector.y) + (myVector.z*myVector.z));

	if (count == 0)
		oldSpeed = oldMag * gFramesPerSecond;									// calc speed value

	newMag = sqrt((myNewVector.x*myNewVector.x) + (myNewVector.y*myNewVector.y) + (myNewVector.z*myNewVector.z));
	newMag = oldMag-newMag;														// calc diff between mags


	impactFriction = (400.0f - oldSpeed) / 100.0f;
	if (impactFriction <= 0.5f)
		impactFriction = 0.5f;
	if (impactFriction > 1.0f)
		impactFriction = 1.0f;


			/************************************/
			/* CALC REFLECTION VECTOR TO BOUNCE */
			/************************************/

	normalX = planeEQ->normal.x;												// get triangle normal
	normalY = planeEQ->normal.y;
	normalZ = planeEQ->normal.z;
	dotProduct = normalX * myNormalizedVector.x;
	dotProduct += normalY * myNormalizedVector.y;
	dotProduct += normalZ * myNormalizedVector.z;
	dotProduct += dotProduct;
	reflectedX = normalX * dotProduct - myNormalizedVector.x;
	reflectedY = normalY * dotProduct - myNormalizedVector.y;
	reflectedZ = normalZ * dotProduct - myNormalizedVector.z;
	FastNormalizeVector(reflectedX,reflectedY,reflectedZ, &normalizedDelta);	// normalize reflection vector


			/*********************/
			/* BOUNCE THE DELTAS */
			/*********************/

	gDelta.x = normalizedDelta.x * -oldSpeed * impactFriction;					// convert reflected vector into new deltas
	gDelta.y = normalizedDelta.y * -oldSpeed * impactFriction;
	gDelta.z = normalizedDelta.z * -oldSpeed * impactFriction;


		/**************************************************/
		/* MAKE UP FOR THE LOST MAGNITUDE BY MOVING COORD */
		/**************************************************/

	if ((gDelta.x != 0.0f) || ((gDelta.y != 0.0f) || (gDelta.z == 0.0f)))		// special check for no delta
		FastNormalizeVector(gDelta.x,gDelta.y,gDelta.z, &normalizedDelta);		// normalize my delta vector
	else
		normalizedDelta.x = normalizedDelta.y = normalizedDelta.z = 0;

	oldX = gCoord.x;															// for next pass, the intersect point is "old"
	oldY = gCoord.y;
	oldZ = gCoord.z;

	gCoord.x += normalizedDelta.x * newMag * 1.0f;								// move it
	gCoord.y += normalizedDelta.y * newMag * 1.0f;
	gCoord.z += normalizedDelta.z * newMag * 1.0f;


			/***********************************************/
			/* NOW WE NEED TO PERFORM COLLISION TEST AGAIN */
			/***********************************************/

	if (++count < 5)											// make sure this doesnt get stuck
	{
		x = gCoord.x;											// new coords = gCoord to try again
		y = gCoord.y;
		z = gCoord.z;
		goto try_again;
	}
}
#endif


#pragma mark -


/******************* HANDLE FLOOR COLLISION *********************/
//
// OUTPUT: 	newCoord = adjusted coord if collision occurred
//			newDelta = adjusted/reflected delta if collision occurred
//			Boolean = true if is on ground.
//
// INPUT:	newCoord = current coord
//			oldCoord = coord before the move
//			newDelta = current deltas
//			footYOff = y dist from coord to foot
//

Boolean HandleFloorCollision(OGLPoint3D *newCoord, float footYOff)
{
Boolean				hitFoot;
float				floorY;
float				distToFloor;
float				fps;

	fps = gFramesPerSecondFrac;

			/*************************************/
			/* CALC DISTANCES TO FLOOR & CEILING */
			/*************************************/

	floorY 		= GetTerrainY(newCoord->x, newCoord->z);					// get y coord at this x/z
	distToFloor = (newCoord->y - footYOff) - floorY;						// calc dist off of ground
	hitFoot 	= distToFloor <= 0.0f;										// see if hit ground


			/********************/
			/* SEE IF ON GROUND */
			/********************/

	if (hitFoot)
	{
		newCoord->y = floorY+footYOff;										// move so its on top of ground
	}


	return(hitFoot);
}


/***************** RAY INTERSECT TRIANGLE *******************/

static Boolean RayIntersectTriangle(OGLPoint3D *origin, OGLVector3D *dir,
                   			OGLPoint3D *v0, OGLPoint3D *v1, OGLPoint3D *v2,
                   			Boolean	cullTest, float *t, float *u, float *v)
{
   OGLVector3D 	edge1, edge2;
   OGLVector3D 	tvec, pvec, qvec;
   float 		det,inv_det;

   /* find vectors for two edges sharing vert0 */

   edge1.x = v1->x - v0->x;
   edge1.y = v1->y - v0->y;
   edge1.z = v1->z - v0->z;

   edge2.x = v2->x - v0->x;
   edge2.y = v2->y - v0->y;
   edge2.z = v2->z - v0->z;


	/* begin calculating determinant - also used to calculate U parameter */

	OGLVector3D_Cross(dir, &edge2, &pvec);


	/* if determinant is near zero, ray lies in plane of triangle */

	det = (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);		// calc dot product

	if (cullTest)
	{
	   if (det < EPS)
	      return(false);

		/* calculate distance from vert0 to ray origin */

	   tvec.x = origin->x - v0->x;
	   tvec.y = origin->y - v0->y;
	   tvec.z = origin->z - v0->z;


		/* calculate U parameter and test bounds */

		*u = (tvec.x * pvec.x) + (tvec.y * pvec.y) + (tvec.z * pvec.z);		// calc dot product
		if ((*u < 0.0f) || (*u > det))
	  		return (false);


		/* prepare to test V parameter */

		OGLVector3D_Cross(&tvec, &edge1, &qvec);


		/* calculate V parameter and test bounds */

		*v = (dir->x * qvec.x) + (dir->y * qvec.y) + (dir->z * qvec.z);		// calc dot product
		if ((*v < 0.0f) || ((*u + *v) > det))
			return 0;


		/* calculate t, scale parameters, ray intersects triangle */

		*t = (edge2.x * qvec.x) + (edge2.y * qvec.y) + (edge2.z * qvec.z);		// calc dot product

		inv_det = 1.0f / det;
		*t *= inv_det;
		*u *= inv_det;
		*v *= inv_det;
	}
				/* NO CULLING */
	else
	{
		if ((det > -EPS) && (det < EPS))
			return(false);

		inv_det = 1.0f / det;

		/* calculate distance from vert0 to ray origin */

	   	tvec.x = origin->x - v0->x;
		tvec.y = origin->y - v0->y;
	 	tvec.z = origin->z - v0->z;

   		/* calculate U parameter and test bounds */

		*u = (tvec.x * pvec.x) + (tvec.y * pvec.y) + (tvec.z * pvec.z);		// calc dot product
   		*u *= inv_det;

		if ((*u < 0.0f) || (*u > 1.0f))
			return(false);

		/* prepare to test V parameter */

		OGLVector3D_Cross(&tvec, &edge1, &qvec);


		/* calculate V parameter and test bounds */

		*v = (dir->x * qvec.x) + (dir->y * qvec.y) + (dir->z * qvec.z);		// calc dot product
	   	*v *= inv_det;

		if ((*v < 0.0f) || (*u + *v > 1.0f))
			return(false);

		/* calculate t, ray intersects triangle */

		*t = (edge2.x * qvec.x) + (edge2.y * qvec.y) + (edge2.z * qvec.z);		// calc dot product
		*t *= inv_det;
	}
	return(true);
}














