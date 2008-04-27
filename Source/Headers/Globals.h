/****************************/
/*         MY GLOBALS       */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

#ifndef MYGLOBALS_H
#define MYGLOBALS_H



#include <math.h>
#include <gl.h>
#include <textutils.h>
#include <qdoffscreen.h>

#define	kGameID 		'CavM'

			/* SOME FLOATING POINT HELPERS */

#define INFINITE	1e20
#define EPS .0001					// a very small number which is useful for FP compares close to 0
#define	IS_ZERO(_x)  (fabs(_x) < EPS)


#define	MOVE_TO_FRONT		(WindowPtr)-1L
#define	NIL_STRING			"\p"
#define	PICT_HEADER_SIZE	512
#define REMOVE_ALL_EVENTS	 0



		/* CLOSE ENOUGH TO ZERO */
		//
		// If float value is close enough to 0, then make it 0
		//

#define	CLOSE_ENOUGH_TO_ZERO(theFloat)	if (fabs(theFloat) < EPS) theFloat = 0;


		/*******************/
		/* 2D ARRAY MACROS */
		/*******************/

#define Alloc_2d_array(type, array, n, m)								\
{																		\
int _i;																	\
																		\
	array = (type **) AllocPtr((long)(n) * sizeof(type *));				\
	if (array == nil)													\
		DoFatalAlert("\pAlloc_2d_array failed!");						\
	array[0] = (type *) AllocPtr((long)(n) * (long)(m) * sizeof(type));	\
	if (array[0] == nil)												\
		DoFatalAlert("\pAlloc_2d_array failed!");						\
	for (_i = 1; _i < (n); _i++)										\
		array[_i] = array[_i-1] + (m);									\
}


#define Free_2d_array(array)				\
{											\
		SafeDisposePtr((Ptr)array[0]);		\
		SafeDisposePtr((Ptr)array);			\
		array = nil;						\
}

/* UNIVERSAL PTR TYPE WHICH CAN READ/WRITE ANYTHING */

typedef union {
	long 	*L;
	short 	*S;
	Ptr 	B;
} UniversalPtr;

typedef union {
	long 	**L;
	short 	**S;
	Handle 	B;
} UniversalHandle;

typedef	unsigned char u_char;
typedef	unsigned long u_long;
typedef	unsigned short u_short;


#define	PI					((float)3.1415926535898)
#define PI2					(2.0f*PI)


#define	CHAR_RETURN			0x0d	/* ASCII code for Return key */
#define CHAR_UP				0x1e
#define CHAR_DOWN			0x1f
#define	CHAR_LEFT			0x1c
#define	CHAR_RIGHT			0x1d
#define	CHAR_DELETE			0x08
#define	CHAR_SPACE			0x20
#define	CHAR_ESC			0x1b
#define	CHAR_TAB			0x09






							// COLLISION SIDE INFO
							//=================================

#define	SIDE_BITS_TOP		(1)							// %000001	(r/l/b/t)
#define	SIDE_BITS_BOTTOM	(1<<1)						// %000010
#define	SIDE_BITS_LEFT		(1<<2)						// %000100
#define	SIDE_BITS_RIGHT		(1<<3)						// %001000
#define	SIDE_BITS_FRONT		(1<<4)						// %010000
#define	SIDE_BITS_BACK		(1<<5)						// %100000
#define	ALL_SOLID_SIDES		(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT|\
							SIDE_BITS_FRONT|SIDE_BITS_BACK)


							// CBITS (32 BIT VALUES)
							//==================================

enum
{
	CBITS_TOP 			= SIDE_BITS_TOP,
	CBITS_BOTTOM 		= SIDE_BITS_BOTTOM,
	CBITS_LEFT 			= SIDE_BITS_LEFT,
	CBITS_RIGHT 		= SIDE_BITS_RIGHT,
	CBITS_FRONT 		= SIDE_BITS_FRONT,
	CBITS_BACK 			= SIDE_BITS_BACK,
	CBITS_ALLSOLID		= ALL_SOLID_SIDES,
	CBITS_NOTTOP		= SIDE_BITS_LEFT|SIDE_BITS_RIGHT|SIDE_BITS_FRONT|SIDE_BITS_BACK,
	CBITS_TOUCHABLE		= (1<<6)
};


							// CTYPES (32 BIT VALUES)
							//==================================

enum
{
	CTYPE_PLAYER	=	1,			// Me
	CTYPE_AVOID		=	(1<<1),		// set if CPU cars should avoid hitting this
	CTYPE_VISCOUS	=	(1<<2),		// Viscous object
	CTYPE_BONUS		=	(1<<3),		// Bonus item
	CTYPE_TRIGGER	=	(1<<4),		// Trigger
	CTYPE_SKELETON	=	(1<<5),		// Skeleton
	CTYPE_MISC		=	(1<<6),		// Misc
	CTYPE_BLOCKSHADOW =	(1<<7),		// Shadows go over it
	CTYPE_xxxxx		=	(1<<8),
	CTYPE_BGROUND2 	=	(1<<9),		// Collide against Terrain BGround 2 path tiles
	CTYPE_aaaa		= 	(1<<10),
	CTYPE_cccc		= 	(1<<11),
	CTYPE_BGROUND 	=	(1<<12),	// Terrain BGround path tiles
	CTYPE_TERRAIN	=	(1<<13),	// not an attribute, but just a flag passed to HandleCollisions()
	CTYPE_SPIKED	=	(1<<14),	// Ball player cannot hit this
	CTYPE_xxxxxx 	=	(1<<15),
	CTYPE_AUTOTARGET =	(1<<16),	// if can be auto-targeted by player
	CTYPE_LIQUID	=	(1<<17),	// is a liquid patch
	CTYPE_BOPPABLE	=	(1<<18),	// enemy that can be bopped on top
	CTYPE_BLOCKCAMERA =	(1<<19),	// camera goes over this
	CTYPE_DRAINBALLTIME = (1<<20),	// drain ball time
	CTYPE_xxxxxxx	= (1<<21),
	CTYPE_IMPENETRABLE	= (1<<22),	// set if object must have high collision priority and cannot be pushed thru this
	CTYPE_IMPENETRABLE2	= (1<<23),	// set with CTYPE_IMPENETRABLE if dont want player to do coord=oldCoord when touched
	CTYPE_AUTOTARGETJUMP = (1<<24)	// if auto target when jumping
};



							// OBJNODE STATUS BITS
							//==================================

enum
{
	STATUS_BIT_ONGROUND		=	1,			// Player is on anything solid (terrain or objnode)
	STATUS_BIT_NOTEXTUREWRAP =	(1<<1),		// if pin textures so they dont wrap
	STATUS_BIT_DONTCULL		=	(1<<2),		// set if don't want to perform custom culling on this object
	STATUS_BIT_NOCOLLISION  = 	(1<<3),		// set if want collision code to skip testing against this object
	STATUS_BIT_NOMOVE  		= 	(1<<4),		// dont call object's move function
	STATUS_BIT_ANIM  		= 	(1<<5),		// set if can animate
	STATUS_BIT_HIDDEN		=	(1<<6),		// dont draw object
	STATUS_BIT_KEEPBACKFACES	=	(1<<7),	// keep both sides
	STATUS_BIT_ONLYSHOWTHISPLAYER =	(1<<8),	// set if only draw this object for this PlayerNum
	STATUS_BIT_ROTXZY		 = 	(1<<9),		// set if want to do x->z->y ordered rotation
	STATUS_BIT_ISCULLED		 = 	(1<<10),	// set if culling function deemed it culled
	STATUS_BIT_GLOW			=	(1<<11),	// use additive blending for glow effect
	STATUS_BIT_ONTERRAIN	=  (1<<12),		// also set with STATUS_BIT_ONGROUND if specifically on the terrain
	STATUS_BIT_NOLIGHTING	 =  (1<<13),	// used when want to render object will NULL shading (no lighting)
	STATUS_BIT_ALWAYSCULL	 =  (1<<14),	// to force a cull-check
	STATUS_BIT_CLIPALPHA 	 =  (1<<15), 	// set if want to modify alphafunc
	STATUS_BIT_DELETE		=	(1<<16),	// set by a function which cannot actually delete an object but we want to tell system to delete it at its convenience
	STATUS_BIT_NOZBUFFER	=	(1<<17),	// set when want to turn off z buffer
	STATUS_BIT_NOZWRITES	=	(1<<18),	// set when want to turn off z buffer writes
	STATUS_BIT_NOFOG		=	(1<<19),
	STATUS_BIT_AUTOFADE		=	(1<<20),	// calculate fade xparency value for object when rendering
	STATUS_BIT_DETACHED		=	(1<<21),	// set if objnode is free-floating and not attached to linked list
	STATUS_BIT_ONSPLINE		=	(1<<22),	// if objnode is attached to spline
	STATUS_BIT_REVERSESPLINE =	(1<<23),	// if going reverse direction on spline
	STATUS_BIT_NOCLIPTEST 	=	(1<<24),	// if dont need to do clip testing
	STATUS_BIT_AIMATCAMERA 	=	(1<<25),		// if need to aim at current player's camera (for sprite billboards)
	STATUS_BIT_NOSHOWTHISPLAYER = (1<<26)	// opposite of STATUS_BIT_ONLYSHOWTHISPLAYER
};


#include "structs.h"


#define	GetPlayerNum(noNet)	(gNetGameInProgress ? gMyNetworkPlayerNum : (noNet))

#endif










