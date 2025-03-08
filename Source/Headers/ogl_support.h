//
// ogl_support.h
//

#pragma once

#include "main.h"
#include <SDL3/SDL_opengl.h>

#define MAX_SPLITSCREENS	MAX_LOCAL_PLAYERS
#define MAX_VIEWPORTS		(1+MAX_SPLITSCREENS)

#define	MAX_FILL_LIGHTS		4

		/* 4x4 MATRIX INDECIES */
enum
{
	M00	= 0,
	M10,
	M20,
	M30,
	M01,
	M11,
	M21,
	M31,
	M02,
	M12,
	M22,
	M32,
	M03,
	M13,
	M23,
	M33
};

		/* 3x3 MATRIX INDECIES */
enum
{
	N00	= 0,
	N10,
	N20,
	N01,
	N11,
	N21,
	N02,
	N12,
	N22
};


		/* 3D STRUCTURES */

typedef struct
{
	float 	x,y,z,w;
}OGLPoint4D;

typedef struct
{
	GLfloat	x,y,z;
}OGLPoint3D;

typedef struct
{
	GLfloat	x,y;
}OGLPoint2D;

typedef struct
{
	GLfloat	x,y,z;
}OGLVector3D;

typedef struct
{
	GLfloat	x,y;
}OGLVector2D;

typedef struct
{
	GLfloat	u,v;
}OGLTextureCoord;

typedef struct
{
	GLfloat	r,g,b;
}OGLColorRGB;

typedef struct
{
	GLfloat	r,g,b,a;
}OGLColorRGBA;

typedef struct
{
	GLubyte	r,g,b,a;
}OGLColorRGBA_Byte;

typedef struct
{
	uint16_t b : 5;
	uint16_t g : 5;
	uint16_t r : 5;
	uint16_t a : 1;
} OGLColorBGRA16;

typedef struct
{
	GLfloat	value[16];
}OGLMatrix4x4;

typedef struct
{
	GLfloat	value[9];
}OGLMatrix3x3;

typedef struct
{
	OGLVector3D 					normal;
	float 							constant;
}OGLPlaneEquation;

typedef struct
{
	OGLPoint3D			point;
	OGLTextureCoord		uv;
	OGLColorRGBA		color;
}OGLVertex;

typedef struct
{
	OGLPoint3D 						cameraLocation;				/*  Location point of the camera 	*/
	OGLPoint3D 						pointOfInterest;			/*  Point of interest 				*/
	OGLVector3D 					upVector;					/*  "up" vector 					*/
}OGLCameraPlacement;

typedef struct
{
	OGLPoint3D 			min;
	OGLPoint3D 			max;
	Boolean 			isEmpty;
}OGLBoundingBox;


typedef struct
{
	OGLPoint3D 			origin;
	float 				radius;
	Boolean 			isEmpty;
}OGLBoundingSphere;


typedef struct
{
	float	top,bottom,left,right;
}OGLRect;

//========================

typedef	struct
{
	Boolean					clearBackBuffer;
	WindowPtr				displayWindow;
	OGLColorRGBA			clearColor;
	Rect					clip;			// left = amount to clip off left, etc.
	int						numPanes;
	float					pillarboxRatio;
	const char*				fontName;
}OGLViewDefType;


typedef	struct
{
	Boolean			usePhong;
	Boolean			useFog;
	float			fogStart;
	float			fogEnd;
	float			fogDensity;
	short			fogMode;

}OGLStyleDefType;


typedef struct
{
	OGLPoint3D				from[MAX_VIEWPORTS];			// 2 cameras, one for each viewport/player
	OGLPoint3D				to[MAX_VIEWPORTS];
	OGLVector3D				up[MAX_VIEWPORTS];
	float					hither;
	float					yon;
	float					fov;
}OGLCameraDefType;

typedef	struct
{
	OGLColorRGBA		ambientColor;
	long				numFillLights;
	OGLVector3D			fillDirection[MAX_FILL_LIGHTS];
	OGLColorRGBA		fillColor[MAX_FILL_LIGHTS];
}OGLLightDefType;


		/* OGLSetupInputType */

typedef struct
{
	OGLViewDefType		view;
	OGLStyleDefType		styles;
	OGLCameraDefType	camera;
	OGLLightDefType		lights;
}OGLSetupInputType;


		/* OGLSetupOutputType */

typedef struct
{
	Rect					clip;				// not pane size, but clip:  left = amount to clip off left
	OGLLightDefType			lightList;
	OGLCameraPlacement		cameraPlacement[MAX_VIEWPORTS];	// 2 cameras, one for each viewport/player
	float					fov[MAX_VIEWPORTS],hither,yon;
	Boolean					useFog;
	Boolean					clearBackBuffer;
	float					pillarboxRatio;
	Boolean					fadePillarbox;		// if true, pillarbox border brightness tracks global gamma fade
	Boolean					fadeSound;			// if true, global sound volume will track global gamma fade
	float					fadeInDuration;
	float					fadeOutDuration;

	struct
	{
		int					vpx;
		int					vpy;
		int					vpw;
		int					vph;
		float				aspectRatio;
		float				logicalWidth;
		float				logicalHeight;
	} panes[MAX_VIEWPORTS];
}OGLSetupOutputType;



#define PILLARBOX_RATIO_FULLSCREEN	(0.0)
#define PILLARBOX_RATIO_4_3			(4.0/3.0)
#define PILLARBOX_RATIO_16_9		(16.0/9.0)


#define SPLITSCREEN_DIVIDER_THICKNESS 2			// relative to 640x480

enum
{
	//  +---------+
	//  |         |
	//  |         |
	//  |         |
	//  +---------+
	SPLITSCREEN_MODE_NONE = 0,

	//  +----+----+
	//  |    |    |
	//  |    |    |
	//  |    |    |
	//  +----+----+
	SPLITSCREEN_MODE_2P_TALL,

	//  +---------+
	//  |         |
	//  +---------+
	//  |         |
	//  +---------+
	SPLITSCREEN_MODE_2P_WIDE,

	//  +----+----+
	//  |    |    |
	//  +----+----+
	//  |         |
	//  +---------+
	SPLITSCREEN_MODE_3P_WIDE,

	//  +----+----+
	//  |    |    |
	//  +----+    |
	//  |    |    |
	//  +----+----+
	SPLITSCREEN_MODE_3P_TALL,

	//  +----+----+
	//  |    |    |
	//  +----+----+
	//  |    |    |
	//  +----+----+
	SPLITSCREEN_MODE_4P_GRID,

	//  +--+--+--+
	//  |  |  |  |
	//  +--+--+  |
	//  |  |  |  |
	//  +--+--+--+
	// Not implemented
	SPLITSCREEN_MODE_5P_TALL,

	//  +--+--+--+
	//  |  |  |  |
	//  +--+--+--+
	//  |  |     |
	//  +--+--+--+
	// Not implemented
	SPLITSCREEN_MODE_5P_WIDE,

	//  +--+--+--+
	//  |  |  |  |
	//  +--+--+--+
	//  |  |  |  |
	//  +--+--+--+
	SPLITSCREEN_MODE_6P_GRID,

	NUM_SPLITSCREEN_MODES
};


enum
{
	kProjectionType3D = 0,
	kProjectionType2DNDC,
	kProjectionType2DOrthoFullRect,
	kProjectionType2DOrthoCentered,
	kProjectionTypeUnspecified,
};

enum
{
	kLoadTextureFlags_NoGammaFix = 1 << 1,
	kLoadTextureFlags_NearestNeighbor = 1 << 2,
};

//=====================================================================

void OGL_Boot(void);
void OGL_Shutdown(void);
void OGL_NewViewDef(OGLSetupInputType *viewDef);
void OGL_SetupGameView(OGLSetupInputType *setupDefPtr);
void OGL_DisposeGameView(void);
void OGL_DrawScene(void (*drawRoutine)(void));
void OGL_Camera_SetPlacementAndUpdateMatrices(int camNum);
void OGL_MoveCameraFromTo(float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ, int camNum);
void OGL_UpdateCameraFromToUp(OGLPoint3D *from, OGLPoint3D *to, OGLVector3D *up, int camNum);
void OGL_UpdateCameraFromTo(OGLPoint3D *from, OGLPoint3D *to, int camNum);
void OGL_Texture_SetOpenGLTexture(GLuint textureName);
GLuint OGL_TextureMap_Load(void *imageMemory, int width, int height,
							GLint srcFormat,  GLint destFormat, GLint dataType);
GLuint OGL_TextureMap_LoadImageFile(const char* path, int* width, int* height);
void OGL_FixColorGamma(OGLColorRGBA* color);
GLenum _OGL_CheckError(const char* file, int line);
#define OGL_CheckError() _OGL_CheckError(__FILE__, __LINE__)

void OGL_PushState(void);
void OGL_PopState(void);

void OGL_EnableLighting(void);
void OGL_DisableLighting(void);

void OGL_SetProjection(int projectionType);

#define GetOverlayPaneNumber() (gNumSplitScreenPanes)

