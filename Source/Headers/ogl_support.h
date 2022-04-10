//
// ogl_support.h
//

#pragma once

#include <SDL_opengl.h>

#define MAX_SPLITSCREENS	2


#define	MAX_FILL_LIGHTS		4
#define	MAX_TEXTURES		300

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
	Boolean					pillarbox4x3;
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
	OGLPoint3D				from[MAX_SPLITSCREENS];			// 2 cameras, one for each viewport/player
	OGLPoint3D				to[MAX_SPLITSCREENS];
	OGLVector3D				up[MAX_SPLITSCREENS];
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
	Boolean					isActive;
//	AGLContext				drawContext;
	WindowPtr				window;
	Rect					clip;				// not pane size, but clip:  left = amount to clip off left
	OGLLightDefType			lightList;
	OGLCameraPlacement		cameraPlacement[MAX_SPLITSCREENS];	// 2 cameras, one for each viewport/player
	float					fov[MAX_SPLITSCREENS],hither,yon;
	Boolean					useFog;
	Boolean					clearBackBuffer;
	Boolean					pillarbox4x3;
	Boolean					fadePillarbox;		// if true, pillarbox border brightness tracks global gamma fade
}OGLSetupOutputType;



enum
{
	SPLITSCREEN_MODE_NONE = 0,
	SPLITSCREEN_MODE_HORIZ,			// 2 horizontal panes
	SPLITSCREEN_MODE_VERT,			// 2 vertical panes

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


//=====================================================================

void OGL_Boot(void);
void OGL_NewViewDef(OGLSetupInputType *viewDef);
void OGL_SetupWindow(OGLSetupInputType *setupDefPtr, OGLSetupOutputType **outputHandle);
void OGL_DisposeWindowSetup(OGLSetupOutputType **dataHandle);
void OGL_DrawScene(OGLSetupOutputType *setupInfo, void (*drawRoutine)(OGLSetupOutputType *));
void OGL_ChangeDrawSize(OGLSetupOutputType *setupInfo);
void OGL_Camera_SetPlacementAndUpdateMatrices(OGLSetupOutputType *setupInfo, int camNum);
void OGL_MoveCameraFromTo(OGLSetupOutputType *setupInfo, float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ, int camNum);
void OGL_UpdateCameraFromToUp(OGLSetupOutputType *setupInfo, OGLPoint3D *from, OGLPoint3D *to, OGLVector3D *up, int camNum);
void OGL_UpdateCameraFromTo(OGLSetupOutputType *setupInfo, OGLPoint3D *from, OGLPoint3D *to, int camNum);
void OGL_Texture_SetOpenGLTexture(GLuint textureName);
GLuint OGL_TextureMap_Load(void *imageMemory, int width, int height,
							GLint srcFormat,  GLint destFormat, GLint dataType);
GLuint OGL_TextureMap_LoadImageFile(const char* path, int* width, int* height);
GLenum _OGL_CheckError(const char* file, int line);
#define OGL_CheckError() _OGL_CheckError(__FILE__, __LINE__)

void OGL_GetCurrentViewport(const OGLSetupOutputType *setupInfo, int *x, int *y, int *w, int *h, Byte whichPane);

void OGL_PushState(void);
void OGL_PopState(void);

void OGL_EnableLighting(void);
void OGL_DisableLighting(void);

void OGL_Update2DLogicalSize(void);
void OGL_SetProjection(int projectionType);

