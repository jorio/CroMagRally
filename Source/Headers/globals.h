/****************************/
/*         MY GLOBALS       */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

#pragma once

#include <Pomme.h>
#include <math.h>

#if _MSC_VER
	#define _Static_assert static_assert
#endif

#define	kGameID 		'CavM'

			/* SOME FLOATING POINT HELPERS */

#define EPS .0001					// a very small number which is useful for FP compares close to 0

#define GAME_CLAMP(x, lo, hi) ( (x) < (lo) ? (lo) : ( (x) > (hi) ? (hi) : (x) ) )
#define GAME_MIN(a, b) ( (a) < (b) ? (a) : (b) )
#define GAME_MAX(a, b) ( (a) > (b) ? (a) : (b) )
#define	SQUARED(x)					((x)*(x))


		/*******************/
		/* 2D ARRAY MACROS */
		/*******************/

#define Alloc_2d_array(type, array, n, m)								\
{																		\
int _i;																	\
																		\
	array = (type **) AllocPtr((long)(n) * sizeof(type *));				\
	if (array == nil)													\
		DoFatalAlert("Alloc_2d_array failed!");						\
	array[0] = (type *) AllocPtr((long)(n) * (long)(m) * sizeof(type));	\
	if (array[0] == nil)												\
		DoFatalAlert("Alloc_2d_array failed!");						\
	for (_i = 1; _i < (n); _i++)										\
		array[_i] = array[_i-1] + (m);									\
}


#define Free_2d_array(array)				\
{											\
		SafeDisposePtr((Ptr)array[0]);		\
		SafeDisposePtr((Ptr)array);			\
		array = nil;						\
}



#define	PI					((float)3.1415926535898)
#define PI2					(2.0f*PI)


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
	CTYPE_PLAYER		= (1<<0),	// Me
	CTYPE_AVOID			= (1<<1),	// set if CPU cars should avoid hitting this
	CTYPE_VISCOUS		= (1<<2),	// Viscous object
	CTYPE_TRIGGER		= (1<<3),	// Trigger
	CTYPE_MISC			= (1<<4),	// Misc
	CTYPE_TERRAIN		= (1<<5),	// not an attribute, but just a flag passed to HandleCollisions()
	CTYPE_AUTOTARGET 	= (1<<6),	// if can be auto-targeted by player
	CTYPE_LIQUID		= (1<<7),	// is a liquid patch
	CTYPE_BLOCKCAMERA	= (1<<8),	// camera goes over this
	CTYPE_IMPENETRABLE	= (1<<9),	// set if object must have high collision priority and cannot be pushed thru this
	CTYPE_IMPENETRABLE2	= (1<<10),	// set with CTYPE_IMPENETRABLE if dont want player to do coord=oldCoord when touched
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
	STATUS_BIT_MOVEINPAUSE	= 	(1<<5),		// set if can animate
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
	STATUS_BIT_OVERLAYPANE	=	(1<<16),	// set to show node on top of all panes in split-screen mode
	STATUS_BIT_NOZBUFFER	=	(1<<17),	// set when want to turn off z buffer
	STATUS_BIT_NOZWRITES	=	(1<<18),	// set when want to turn off z buffer writes
	STATUS_BIT_NOFOG		=	(1<<19),
	STATUS_BIT_AUTOFADE		=	(1<<20),	// calculate fade xparency value for object when rendering
	STATUS_BIT_DETACHED		=	(1<<21),	// set if objnode is free-floating and not attached to linked list
	STATUS_BIT_ONSPLINE		=	(1<<22),	// if objnode is attached to spline
	STATUS_BIT_REVERSESPLINE =	(1<<23),	// if going reverse direction on spline
	STATUS_BIT_NOCLIPTEST 	=	(1<<24),	// if dont need to do clip testing
	STATUS_BIT_AIMATCAMERA 	=	(1<<25),		// if need to aim at current player's camera (for sprite billboards)
	STATUS_BIT_NOSHOWTHISPLAYER = (1<<26),	// opposite of STATUS_BIT_ONLYSHOWTHISPLAYER
	STATUS_BIT_HIDEINPAUSE	=	(1<<27),
};


#define STATUS_BITS_FOR_2D \
	(STATUS_BIT_NOTEXTUREWRAP \
	| STATUS_BIT_DONTCULL \
	| STATUS_BIT_NOZBUFFER \
	| STATUS_BIT_NOZWRITES \
	| STATUS_BIT_NOFOG \
	| STATUS_BIT_NOLIGHTING)


#include "structs.h"


#define	GetPlayerNum(noNet)	(gNetGameInProgress ? gMyNetworkPlayerNum : (noNet))

