//
// collision.h
//

enum
{
	COLLISION_TYPE_OBJ,						// box
	COLLISION_TYPE_TILE
};




					/* COLLISION STRUCTURES */
struct CollisionRec
{
	Byte			baseBox,targetBox;
	unsigned short	sides;
	Byte			type;
	ObjNode			*objectPtr;			// object that collides with (if object type)
	float			planeIntersectY;	// where intersected triangle
};
typedef struct CollisionRec CollisionRec;



//=================================

void CollisionDetect(ObjNode *baseNode, uint32_t CType, short startNumCollisions);

Byte HandleCollisions(ObjNode *theNode, uint32_t cType, float deltaBounce);
extern	Boolean IsPointInPoly2D( float,  float ,  Byte ,  OGLPoint2D *);
extern	Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2);
short DoSimplePointCollision(OGLPoint3D *thePoint, uint32_t cType, ObjNode *except);
short DoSimpleBoxCollision(float top, float bottom, float left, float right,
						float front, float back, uint32_t cType);


Boolean HandleFloorCollision(OGLPoint3D *newCoord, float footYOff);

Boolean DoSimplePointCollisionAgainstPlayer(OGLPoint3D *thePoint, int playerNum);
Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right,
										float front, float back, int playerNum);
Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right,
										float front, float back, ObjNode *targetNode);

