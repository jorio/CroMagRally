/****************************/
/*   OPENGL SUPPORT.C	    */
/* (c)2000 Pangea Software  */
/*   By Brian Greenstone    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <gl.h>
#include <glu.h>
#include <glm.h>
#include <agl.h>
#include <aglRenderers.h>
#include <math.h>
#include <aglmacro.h>

#include "globals.h"
#include "misc.h"
#include "windows.h"
#include "ogl_support.h"
#include "3dmath.h"
#include "main.h"
#include "player.h"
#include "input.h"
#include "file.h"
#include "sound2.h"
#include <string.h>
#include <displays.h>

extern int				gNumObjectNodes,gNumPointers;
extern	MOMaterialObject	*gMostRecentMaterial;
extern	GWorldPtr		gTerrainDebugGWorld;
extern	short			gNumSuperTilesDrawn,gNumActiveParticleGroups,gNumFencesDrawn,gNumTotalPlayers;
extern	PlayerInfoType	gPlayerInfo[];
extern	float			gFramesPerSecond,gCameraStartupTimer;
extern	Byte			gDebugMode;
extern	Boolean			gAutoPilot,gNetGameInProgress,gOSX;
extern	u_short			gPlayer0TileAttribs;
extern	u_long			gGlobalMaterialFlags;
extern	PrefsType			gGamePrefs;
extern	int				gGameWindowWidth,gGameWindowHeight;
extern	CGrafPtr				gDisplayContextGrafPtr;
extern	DisplayIDType		gOurDisplayID;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void OGL_CreateDrawContext(OGLViewDefType *viewDefPtr);
static void OGL_SetStyles(OGLSetupInputType *setupDefPtr);
static void OGL_CreateLights(OGLLightDefType *lightDefPtr);
static void OGL_InitFont(void);
static void OGL_FreeFont(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	STATE_STACK_SIZE	20

/*********************/
/*    VARIABLES      */
/*********************/

Boolean			gSupportsPackedPixels = false;
Boolean			gOpenGL112 = false;
Boolean			gCanDo512 = true;

AGLDrawable		gAGLWin;
AGLContext		gAGLContext = nil;

static GLuint 			gFontList;


OGLMatrix4x4	gViewToFrustumMatrix,gLocalToViewMatrix,gLocalToFrustumMatrix;
OGLMatrix4x4	gWorldToWindowMatrix[MAX_SPLITSCREENS],gFrustumToWindowMatrix[MAX_SPLITSCREENS];

int		gCurrentSplitScreenPane = 0, gNumSplitScreenPanes= 0;
Byte	gActiveSplitScreenMode 	= SPLITSCREEN_MODE_NONE;		// currently active split mode

float	gCurrentAspectRatio = 1;


Boolean		gStateStack_Lighting[STATE_STACK_SIZE];
Boolean		gStateStack_CullFace[STATE_STACK_SIZE];
Boolean		gStateStack_DepthTest[STATE_STACK_SIZE];
Boolean		gStateStack_Normalize[STATE_STACK_SIZE];
Boolean		gStateStack_Texture2D[STATE_STACK_SIZE];
Boolean		gStateStack_Blend[STATE_STACK_SIZE];
Boolean		gStateStack_Fog[STATE_STACK_SIZE];
GLboolean	gStateStack_DepthMask[STATE_STACK_SIZE];
GLint		gStateStack_BlendDst[STATE_STACK_SIZE];
GLint		gStateStack_BlendSrc[STATE_STACK_SIZE];
GLfloat		gStateStack_Color[STATE_STACK_SIZE][4];

int			gStateStackIndex = 0;

int			gPolysThisFrame;
int			gVRAMUsedThisFrame = 0;
static int			gMinRAM = 10000000000000;

Boolean		gMyState_Lighting;


/******************** OGL BOOT *****************/
//
// Initialize my OpenGL stuff.
//

void OGL_Boot(void)
{
NumVersion	version;


			/* SEE IF RUNNING THE DREDED OPENGL 1.1.2 */

	gOpenGL112 = false;
	if (!gOSX)
	{
		GetLibVersion("\pOpenGLEngine", &version);
		if (version.majorRev == 1)
		{
			if (version.minorAndBugRev == 0x12)
				gOpenGL112 = true;
		}
	}
}


/*********************** OGL: NEW VIEW DEF **********************/
//
// fills a view def structure with default values.
//

void OGL_NewViewDef(OGLSetupInputType *viewDef, WindowPtr theWindow)
{
OGLColorRGBA		clearColor = {0,0,0,1};
OGLPoint3D			cameraFrom = { 0, 0, 100.0 };
OGLPoint3D			cameraTo = { 0, 0, 0 };
OGLVector3D			cameraUp = { 0.0, 1.0, 0.0 };
OGLColorRGBA			ambientColor = { .3, .3, .3, 1 };
OGLColorRGBA			fillColor = { 1.0, 1.0, 1.0, 1 };
OGLVector3D			fillDirection1 = { 0, 0, -1 };
OGLVector3D			fillDirection2 = { -1, -.3, -.3 };


	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);
	OGLVector3D_Normalize(&fillDirection2, &fillDirection2);

	if (gOSX)
		viewDef->view.displayWindow 	= nil;
	else
		viewDef->view.displayWindow 	= theWindow;
	viewDef->view.clearColor 		= clearColor;
	viewDef->view.clip.left 	= 0;
	viewDef->view.clip.right 	= 0;
	viewDef->view.clip.top 		= 0;
	viewDef->view.clip.bottom 	= 0;
	viewDef->view.numPanes	 	= 1;				// assume only 1 pane
	viewDef->view.clearBackBuffer = true;

	viewDef->camera.from[0]			= cameraFrom;
	viewDef->camera.from[1] 		= cameraFrom;
	viewDef->camera.to[0] 			= cameraTo;
	viewDef->camera.to[1] 			= cameraTo;
	viewDef->camera.up[0] 			= cameraUp;
	viewDef->camera.up[1] 			= cameraUp;
	viewDef->camera.hither 			= 10;
	viewDef->camera.yon 			= 4000;
	viewDef->camera.fov 			= 1.1;

	viewDef->styles.usePhong 		= false;

	viewDef->styles.useFog			= false;
	viewDef->styles.fogStart		= viewDef->camera.yon * .5f;
	viewDef->styles.fogEnd			= viewDef->camera.yon;
	viewDef->styles.fogDensity		= 1.0;
	viewDef->styles.fogMode			= GL_LINEAR;

	viewDef->lights.ambientColor 	= ambientColor;
	viewDef->lights.numFillLights 	= 2;
	viewDef->lights.fillDirection[0] = fillDirection1;
	viewDef->lights.fillDirection[1] = fillDirection2;
	viewDef->lights.fillColor[0] 	= fillColor;
	viewDef->lights.fillColor[1] 	= fillColor;
}


/************** SETUP OGL WINDOW *******************/

void OGL_SetupWindow(OGLSetupInputType *setupDefPtr, OGLSetupOutputType **outputHandle)
{
static OGLVector3D	v = {0,0,0};
OGLSetupOutputType	*outputPtr;
int					i;

			/* ALLOC MEMORY FOR OUTPUT DATA */

	outputPtr = (OGLSetupOutputType *)AllocPtr(sizeof(OGLSetupOutputType));
	if (outputPtr == nil)
		DoFatalAlert("\pOGL_SetupWindow: AllocPtr failed");

			/* SET SOME PANE INFO */

	gCurrentSplitScreenPane = 0;
	gNumSplitScreenPanes = setupDefPtr->view.numPanes;
	switch(setupDefPtr->view.numPanes)
	{
		case	1:
				gActiveSplitScreenMode = SPLITSCREEN_MODE_NONE;
				break;

		case	2:
				gActiveSplitScreenMode = gGamePrefs.desiredSplitScreenMode;
				break;

		default:
				DoFatalAlert("\pOGL_SetupWindow: # panes not implemented");
	}


				/* SETUP */

	OGL_CreateDrawContext(&setupDefPtr->view);
	OGL_SetStyles(setupDefPtr);
	OGL_CreateLights(&setupDefPtr->lights);


				/* PASS BACK INFO */

	outputPtr->drawContext 		= gAGLContext;
	outputPtr->window 			= setupDefPtr->view.displayWindow;		// remember which window
	outputPtr->clip 			= setupDefPtr->view.clip;
	outputPtr->hither 			= setupDefPtr->camera.hither;			// remember hither/yon
	outputPtr->yon 				= setupDefPtr->camera.yon;
	outputPtr->useFog 			= setupDefPtr->styles.useFog;
	outputPtr->clearBackBuffer 	= setupDefPtr->view.clearBackBuffer;

	outputPtr->isActive = true;											// it's now an active structure

	outputPtr->lightList = setupDefPtr->lights;							// copy lights

	for (i = 0; i < MAX_SPLITSCREENS; i++)
	{
		outputPtr->fov[i] = setupDefPtr->camera.fov;					// each camera will have its own fov so we can change it for special effects
		OGL_UpdateCameraFromTo(outputPtr, &setupDefPtr->camera.from[i], &setupDefPtr->camera.to[i], i);
	}

	*outputHandle = outputPtr;											// return value to caller
}


/***************** OGL_DisposeWindowSetup ***********************/
//
// Disposes of all data created by OGL_SetupWindow
//

void OGL_DisposeWindowSetup(OGLSetupOutputType **dataHandle)
{
OGLSetupOutputType	*data;
AGLContext agl_ctx = gAGLContext;

	data = *dataHandle;
	if (data == nil)												// see if this setup exists
		return;

	glFlush();
	glFinish();

			/* KILL DEBUG FONT */

	OGL_FreeFont();

  	aglSetCurrentContext(nil);								// make context not current
   	aglSetDrawable(data->drawContext, nil);
	aglDestroyContext(data->drawContext);					// nuke the AGL context


		/* FREE MEMORY & NIL POINTER */

	data->isActive = false;									// now inactive
	SafeDisposePtr((Ptr)data);
	*dataHandle = nil;

	gAGLContext = nil;
}




/**************** OGL: CREATE DRAW CONTEXT *********************/

static void OGL_CreateDrawContext(OGLViewDefType *viewDefPtr)
{
AGLPixelFormat 	fmt;
GLboolean      mkc,ok;
GLint          attrib[] 		= {AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 16, AGL_ALL_RENDERERS, AGL_ACCELERATED, AGL_NO_RECOVERY, AGL_NONE};
GLint          attribDeepZ[] 	= {AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 32, AGL_ALL_RENDERERS, AGL_ACCELERATED, AGL_NO_RECOVERY, AGL_NONE};
GLint          attrib2[] 		= {AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 16, AGL_ALL_RENDERERS, AGL_NONE};
AGLContext agl_ctx;
GLint			maxTexSize;
static char			*s;

	gAGLWin = (AGLDrawable)gDisplayContextGrafPtr;


			/* CHOOSE PIXEL FORMAT */

	if (0)		//gGamePrefs.deepZ)
		fmt = aglChoosePixelFormat(nil, 0, attribDeepZ);
	else
		fmt = aglChoosePixelFormat(nil, 0, attrib);

	if ((fmt == NULL) || (aglGetError() != AGL_NO_ERROR))
	{
		fmt = aglChoosePixelFormat(nil, 0, attrib2);							// try being less stringent
		if ((fmt == NULL) || (aglGetError() != AGL_NO_ERROR))
		{
			DoFatalAlert("\paglChoosePixelFormat failed!  Check that your 3D accelerator is OpenGL compliant, installed properly, and that you have the latest drivers.");
		}
	}


			/* CREATE AGL CONTEXT & ATTACH TO WINDOW */

	gAGLContext = aglCreateContext(fmt, nil);
	if ((gAGLContext == nil) || (aglGetError() != AGL_NO_ERROR))
		DoFatalAlert("\pOGL_CreateDrawContext: aglCreateContext failed!");

	agl_ctx = gAGLContext;

	ok = aglSetDrawable(gAGLContext, gAGLWin);
	if ((!ok) || (aglGetError() != AGL_NO_ERROR))
	{
		if (aglGetError() == AGL_BAD_ALLOC)
		{
			gGamePrefs.showScreenModeDialog	= true;
			SavePrefs();
			DoFatalAlert("\pNot enough VRAM for the selected video mode.  Please try again and select a different mode.");
		}
		else
			DoFatalAlert("\pOGL_CreateDrawContext: aglSetDrawable failed!");
	}


			/* ACTIVATE CONTEXT */

	mkc = aglSetCurrentContext(gAGLContext);
	if ((mkc == NULL) || (aglGetError() != AGL_NO_ERROR))
		return;


			/* NO LONGER NEED PIXEL FORMAT */

	aglDestroyPixelFormat(fmt);


			/* CLEAR ALL BUFFERS TO BLACK */

	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);		// clear buffer
	aglSwapBuffers(gAGLContext);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);		// clear buffer
	aglSwapBuffers(gAGLContext);


				/* SET VARIOUS STATE INFO */


	glClearColor(viewDefPtr->clearColor.r, viewDefPtr->clearColor.g, viewDefPtr->clearColor.b, 1.0);
	glEnable(GL_DEPTH_TEST);								// use z-buffer

	{
		GLfloat	color[] = {1,1,1,1};									// set global material color to white
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
	}

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
  	glEnable(GL_NORMALIZE);




 		/***************************/
		/* GET OPENGL CAPABILITIES */
 		/***************************/




			/* INIT DEBUG FONT */

	OGL_InitFont();


			/* SEE IF SUPPORT 512X512 */

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	if (maxTexSize < 512)
		gCanDo512 = false;
	else
		gCanDo512 = true;
}



/**************** OGL: SET STYLES ****************/

static void OGL_SetStyles(OGLSetupInputType *setupDefPtr)
{
OGLStyleDefType *styleDefPtr = &setupDefPtr->styles;
AGLContext agl_ctx = gAGLContext;

//	glPolygonMode(GL_FRONT_AND_BACK ,GL_LINE);		//---------------

	glEnable(GL_CULL_FACE);									// activate culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);									// CCW is front face
	glEnable(GL_DITHER);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// set default blend func
	glDisable(GL_BLEND);									// but turn it off by default

	glHint(GL_TRANSFORM_HINT_APPLE, GL_FASTEST);



			/* ENABLE ALPHA CHANNELS */

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0);	// draw any pixel who's Alpha != 0


		/* SET FOG */

	glHint(GL_FOG_HINT, GL_FASTEST);

	if (styleDefPtr->useFog)
	{
		glFogi(GL_FOG_MODE, styleDefPtr->fogMode);
		glFogf(GL_FOG_DENSITY, styleDefPtr->fogDensity);
		glFogf(GL_FOG_START, styleDefPtr->fogStart);
		glFogf(GL_FOG_END, styleDefPtr->fogEnd);
		glFogfv(GL_FOG_COLOR, (float *)&setupDefPtr->view.clearColor);
		glEnable(GL_FOG);
	}
	else
		glDisable(GL_FOG);
}




/********************* OGL: CREATE LIGHTS ************************/
//
// NOTE:  The Projection matrix must be the identity or lights will be transformed.
//

static void OGL_CreateLights(OGLLightDefType *lightDefPtr)
{
int		i;
GLfloat	ambient[4];
AGLContext agl_ctx = gAGLContext;

	OGL_EnableLighting();


			/************************/
			/* CREATE AMBIENT LIGHT */
			/************************/

	ambient[0] = lightDefPtr->ambientColor.r;
	ambient[1] = lightDefPtr->ambientColor.g;
	ambient[2] = lightDefPtr->ambientColor.b;
	ambient[3] = 1;
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);			// set scene ambient light


			/**********************/
			/* CREATE FILL LIGHTS */
			/**********************/

	for (i=0; i < lightDefPtr->numFillLights; i++)
	{
		static GLfloat lightamb[4] = { 0.0, 0.0, 0.0, 1.0 };
		GLfloat lightVec[4];
		GLfloat	diffuse[4];

					/* SET FILL DIRECTION */

		OGLVector3D_Normalize(&lightDefPtr->fillDirection[i], &lightDefPtr->fillDirection[i]);
		lightVec[0] = -lightDefPtr->fillDirection[i].x;		// negate vector because OGL is stupid
		lightVec[1] = -lightDefPtr->fillDirection[i].y;
		lightVec[2] = -lightDefPtr->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);


					/* SET COLOR */

		glLightfv(GL_LIGHT0+i, GL_AMBIENT, lightamb);

		diffuse[0] = lightDefPtr->fillColor[i].r;
		diffuse[1] = lightDefPtr->fillColor[i].g;
		diffuse[2] = lightDefPtr->fillColor[i].b;
		diffuse[3] = 1;

		glLightfv(GL_LIGHT0+i, GL_DIFFUSE, diffuse);


		glEnable(GL_LIGHT0+i);								// enable the light
	}

}

#pragma mark -

/******************* OGL DRAW SCENE *********************/

void OGL_DrawScene(OGLSetupOutputType *setupInfo, void (*drawRoutine)(OGLSetupOutputType *))
{
int	x,y,w,h;
AGLContext agl_ctx = setupInfo->drawContext;


	if (setupInfo == nil)										// make sure it's legit
		DoFatalAlert("\pOGL_DrawScene setupInfo == nil");
	if (!setupInfo->isActive)
		DoFatalAlert("\pOGL_DrawScene isActive == false");

			/* INIT SOME STUFF */

	if (gDebugMode)
	{
		gVRAMUsedThisFrame = gGameWindowWidth * gGameWindowHeight * (gGamePrefs.depth / 8);				// backbuffer size
		gVRAMUsedThisFrame += gGameWindowWidth * gGameWindowHeight * 2;										// z-buffer size
		gVRAMUsedThisFrame += gGamePrefs.screenWidth * gGamePrefs.screenHeight * (gGamePrefs.depth / 8);	// display size
	}


	gPolysThisFrame 	= 0;										// init poly counter
	gMostRecentMaterial = nil;
	gGlobalMaterialFlags = 0;

				/* CLEAR BUFFER */

	if (setupInfo->clearBackBuffer)
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	else
		glClear(GL_DEPTH_BUFFER_BIT);

	glColor4f(1,1,1,1);

				/**************************************/
				/* DRAW EACH SPLIT-SCREEN PANE IF ANY */
				/**************************************/

	for (gCurrentSplitScreenPane = 0; gCurrentSplitScreenPane < gNumSplitScreenPanes; gCurrentSplitScreenPane++)
	{
					/* SET SPLIT-SCREEN VIEWPORT */

		OGL_GetCurrentViewport(setupInfo, &x, &y, &w, &h, gCurrentSplitScreenPane);
		glViewport(x,y, w, h);
		gCurrentAspectRatio = (float)w/(float)h;


				/* GET UPDATED GLOBAL COPIES OF THE VARIOUS MATRICES */

		OGL_Camera_SetPlacementAndUpdateMatrices(setupInfo, gCurrentSplitScreenPane);


				/* CALL INPUT DRAW FUNCTION */

		if (drawRoutine != nil)
			drawRoutine(setupInfo);
	}


		/**************************/
		/* SEE IF SHOW DEBUG INFO */
		/**************************/

	if (GetNewKeyState_Real(KEY_F8))
	{
		if (++gDebugMode > 2)
			gDebugMode = 0;
	}


				/* SHOW BASIC DEBUG INFO */

	if (gDebugMode > 0)
	{
		int		y = 200;
		int		mem = FreeMem();

		if (mem < gMinRAM)		// poll for lowest RAM free
			gMinRAM = mem;

		OGL_DrawString("\pfps:", 20,y);
		OGL_DrawInt(gFramesPerSecond+.5f, 100,y);
		y += 15;

		OGL_DrawString("\p#tri:", 20,y);
		OGL_DrawInt(gPolysThisFrame, 100,y);
		y += 15;

		OGL_DrawString("\ptri/sec:", 20,y);
		OGL_DrawInt((float)gPolysThisFrame * gFramesPerSecond, 100,y);
		y += 15;

		OGL_DrawString("\p#pgroups:", 20,y);
		OGL_DrawInt(gNumActiveParticleGroups, 100,y);
		y += 15;

		OGL_DrawString("\p#objNodes:", 20,y);
		OGL_DrawInt(gNumObjectNodes, 100,y);
		y += 15;

//		OGL_DrawString("\p#fences:", 20,y);
//		OGL_DrawInt(gNumFencesDrawn, 100,y);
//		y += 15;

		OGL_DrawString("\p#free RAM:", 20,y);
		OGL_DrawInt(mem, 100,y);
		y += 15;

		OGL_DrawString("\pmin RAM:", 20,y);
		OGL_DrawInt(gMinRAM, 100,y);
		y += 15;

		OGL_DrawString("\pused VRAM:", 20,y);
		OGL_DrawInt(gVRAMUsedThisFrame, 100,y);
		y += 15;

		OGL_DrawString("\pOGL Mem:", 20,y);
		OGL_DrawInt(glmGetInteger(GLM_CURRENT_MEMORY), 100,y);
		y += 15;

//		OGL_DrawString("\p# pointers:", 20,y);
//		OGL_DrawInt(gNumPointers, 100,y);
//		y += 15;



	}



            /**************/
			/* END RENDER */
			/**************/

           /* SWAP THE BUFFS */

	aglSwapBuffers(setupInfo->drawContext);					// end render loop

}


/********************** OGL: GET CURRENT VIEWPORT ********************/
//
// Remember that with OpenGL, the bottom of the screen is y==0, so some of this code
// may look upside down.
//

void OGL_GetCurrentViewport(const OGLSetupOutputType *setupInfo, int *x, int *y, int *w, int *h, Byte whichPane)
{
int	t,b,l,r;

	t = setupInfo->clip.top;
	b = setupInfo->clip.bottom;
	l = setupInfo->clip.left;
	r = setupInfo->clip.right;

	switch(gActiveSplitScreenMode)
	{
		case	SPLITSCREEN_MODE_NONE:
				*x = l;
				*y = t;
				*w = gGameWindowWidth-l-r;
				*h = gGameWindowHeight-t-b;
				break;

		case	SPLITSCREEN_MODE_HORIZ:
				*x = l;
				*w = gGameWindowWidth-l-r;
				*h = (gGameWindowHeight-l-r)/2;
				switch(whichPane)
				{
					case	0:
							*y = t + (gGameWindowHeight-l-r)/2;
							break;

					case	1:
							*y = t;
							break;
				}
				break;

		case	SPLITSCREEN_MODE_VERT:
				*w = (gGameWindowWidth-l-r)/2;
				*h = gGameWindowHeight-t-b;
				*y = t;
				switch(whichPane)
				{
					case	0:
							*x = l;
							break;

					case	1:
							*x = l + (gGameWindowWidth-l-r)/2;
							break;
				}
				break;
	}
}


#pragma mark -


/***************** OGL TEXTUREMAP LOAD **************************/

GLuint OGL_TextureMap_Load(void *imageMemory, int width, int height,
							GLint srcFormat,  GLint destFormat, GLint dataType)
{
GLuint	textureName;
AGLContext agl_ctx = gAGLContext;

			/* HACK TO FIX BUG IN OPENGL 1.1.2 */
			//
			// Tell OGL that 64 wide texture are half tall to trick it into working!
			//

	if (gOpenGL112)
	{
		if ((width == 64) || (width == 32))
			height /= 2;
	}

			/* GET A UNIQUE TEXTURE NAME & INITIALIZE IT */

	glGenTextures(1, &textureName);
	if (OGL_CheckError())
		DoFatalAlert("\pOGL_TextureMap_Load: glGenTextures failed!");

	glBindTexture(GL_TEXTURE_2D, textureName);				// this is now the currently active texture
	if (OGL_CheckError())
		DoFatalAlert("\pOGL_TextureMap_Load: glBindTexture failed!");

	glTexImage2D(GL_TEXTURE_2D,
				0,										// mipmap level
				destFormat,								// format in OpenGL
				width,									// width in pixels
				height,									// height in pixels
				0,										// border
				srcFormat,								// what my format is
				dataType,		//GL_UNSIGNED_BYTE,						// size of each r,g,b
				imageMemory);							// pointer to the actual texture pixels


			/* SEE IF RAN OUT OF MEMORY WHILE COPYING TO OPENGL */

	if (OGL_CheckError())
		DoFatalAlert("\pOGL_TextureMap_Load: glTexImage2D failed!");


				/* SET THIS TEXTURE AS CURRENTLY ACTIVE FOR DRAWING */

	OGL_Texture_SetOpenGLTexture(textureName);

	return(textureName);
}


/****************** OGL: TEXTURE SET OPENGL TEXTURE **************************/
//
// Sets the current OpenGL texture using glBindTexture et.al. so any textured triangles will use it.
//

void OGL_Texture_SetOpenGLTexture(GLuint textureName)
{
AGLContext agl_ctx = gAGLContext;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (OGL_CheckError())
		DoFatalAlert("\pOGL_Texture_SetOpenGLTexture: glPixelStorei failed!");

	glBindTexture(GL_TEXTURE_2D, textureName);
	if (OGL_CheckError())
		DoFatalAlert("\pOGL_Texture_SetOpenGLTexture: glBindTexture failed!");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// disable mipmaps & turn on filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	glEnable(GL_TEXTURE_2D);
}



#pragma mark -

/*************** OGL_MoveCameraFromTo ***************/

void OGL_MoveCameraFromTo(OGLSetupOutputType *setupInfo, float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ, int camNum)
{

			/* SET CAMERA COORDS */

	setupInfo->cameraPlacement[camNum].cameraLocation.x += fromDX;
	setupInfo->cameraPlacement[camNum].cameraLocation.y += fromDY;
	setupInfo->cameraPlacement[camNum].cameraLocation.z += fromDZ;

	setupInfo->cameraPlacement[camNum].pointOfInterest.x += toDX;
	setupInfo->cameraPlacement[camNum].pointOfInterest.y += toDY;
	setupInfo->cameraPlacement[camNum].pointOfInterest.z += toDZ;

	UpdateListenerLocation(setupInfo);
}


/*************** OGL_UpdateCameraFromTo ***************/

void OGL_UpdateCameraFromTo(OGLSetupOutputType *setupInfo, OGLPoint3D *from, OGLPoint3D *to, int camNum)
{
static const OGLVector3D up = {0,1,0};

	if ((camNum < 0) || (camNum >= MAX_SPLITSCREENS))
		DoFatalAlert("\pOGL_UpdateCameraFromTo: illegal camNum");

	setupInfo->cameraPlacement[camNum].upVector 		= up;
	setupInfo->cameraPlacement[camNum].cameraLocation 	= *from;
	setupInfo->cameraPlacement[camNum].pointOfInterest 	= *to;

	UpdateListenerLocation(setupInfo);
}

/*************** OGL_UpdateCameraFromToUp ***************/

void OGL_UpdateCameraFromToUp(OGLSetupOutputType *setupInfo, OGLPoint3D *from, OGLPoint3D *to, OGLVector3D *up, int camNum)
{
	if ((camNum < 0) || (camNum >= MAX_SPLITSCREENS))
		DoFatalAlert("\pOGL_UpdateCameraFromToUp: illegal camNum");

	setupInfo->cameraPlacement[camNum].upVector 		= *up;
	setupInfo->cameraPlacement[camNum].cameraLocation 	= *from;
	setupInfo->cameraPlacement[camNum].pointOfInterest 	= *to;

	UpdateListenerLocation(setupInfo);
}



/************** OGL: CAMERA SET PLACEMENT & UPDATE MATRICES **********************/
//
// This is called by OGL_DrawScene to initialize all of the view matrices,
// and to extract the current view matrices used for culling et.al.
//

void OGL_Camera_SetPlacementAndUpdateMatrices(OGLSetupOutputType *setupInfo, int camNum)
{
float	aspect;
OGLCameraPlacement	*placement;
int		temp, w, h, i;
OGLLightDefType	*lights;
AGLContext agl_ctx = gAGLContext;

	OGL_GetCurrentViewport(setupInfo, &temp, &temp, &w, &h, 0);
	aspect = (float)w/(float)h;

			/* INIT PROJECTION MATRIX */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (OGLMath_RadiansToDegrees(setupInfo->fov[camNum]),	// fov
					aspect,					// aspect
					setupInfo->hither,		// hither
					setupInfo->yon);		// yon


			/* INIT MODELVIEW MATRIX */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	placement = &setupInfo->cameraPlacement[camNum];
	gluLookAt(placement->cameraLocation.x, placement->cameraLocation.y, placement->cameraLocation.z,
			placement->pointOfInterest.x, placement->pointOfInterest.y, placement->pointOfInterest.z,
			placement->upVector.x, placement->upVector.y, placement->upVector.z);


		/* UPDATE LIGHT POSITIONS */

	lights =  &setupInfo->lightList;						// point to light list
	for (i=0; i < lights->numFillLights; i++)
	{
		GLfloat lightVec[4];

		lightVec[0] = -lights->fillDirection[i].x;			// negate vector because OGL is stupid
		lightVec[1] = -lights->fillDirection[i].y;
		lightVec[2] = -lights->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);
	}


			/* GET VARIOUS CAMERA MATRICES */

	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&gLocalToViewMatrix);
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)&gViewToFrustumMatrix);
	OGLMatrix4x4_Multiply(&gLocalToViewMatrix, &gViewToFrustumMatrix, &gLocalToFrustumMatrix);

	OGLMatrix4x4_GetFrustumToWindow(setupInfo, &gFrustumToWindowMatrix[camNum],camNum);
	OGLMatrix4x4_Multiply(&gLocalToFrustumMatrix, &gFrustumToWindowMatrix[camNum], &gWorldToWindowMatrix[camNum]);

	UpdateListenerLocation(setupInfo);
}



#pragma mark -


/**************** OGL BUFFER TO GWORLD ***********************/

GWorldPtr OGL_BufferToGWorld(Ptr buffer, int width, int height, int bytesPerPixel)
{
Rect			r;
GWorldPtr		gworld;
PixMapHandle	gworldPixmap;
long			gworldRowBytes,x,y,pixmapRowbytes;
Ptr				gworldPixelPtr;
unsigned long	*pix32Src,*pix32Dest;
unsigned short	*pix16Src,*pix16Dest;
OSErr			iErr;
long			pixelSize;

			/* CREATE GWORLD TO DRAW INTO */

	switch(bytesPerPixel)
	{
		case	2:
				pixelSize = 16;
				break;

		case	4:
				pixelSize = 32;
				break;
	}

	SetRect(&r,0,0,width,height);
	iErr = NewGWorld(&gworld,pixelSize, &r, nil, nil, 0);
	if (iErr)
		DoFatalAlert("\pOGL_BufferToGWorld: NewGWorld failed!");

	DoLockPixels(gworld);

	gworldPixmap = GetGWorldPixMap(gworld);
	LockPixels(gworldPixmap);

	gworldRowBytes = (**gworldPixmap).rowBytes & 0x3fff;					// get GWorld's rowbytes
	gworldPixelPtr = GetPixBaseAddr(gworldPixmap);							// get ptr to pixels

	pixmapRowbytes = width * bytesPerPixel;


			/* WRITE DATA INTO GWORLD */

	switch(pixelSize)
	{
		case	32:
				pix32Src = (unsigned long *)buffer;							// get 32bit pointers
				pix32Dest = (unsigned long *)gworldPixelPtr;
				for (y = 0; y <  height; y++)
				{
					for (x = 0; x < width; x++)
						pix32Dest[x] = pix32Src[x];

					pix32Dest += gworldRowBytes/4;							// next dest row
					pix32Src += pixmapRowbytes/4;
				}
				break;

		case	16:
				pix16Src = (unsigned short *)buffer;						// get 16bit pointers
				pix16Dest = (unsigned short *)gworldPixelPtr;
				for (y = 0; y <  height; y++)
				{
					for (x = 0; x < width; x++)
						pix16Dest[x] = pix16Src[x];

					pix16Dest += gworldRowBytes/2;							// next dest row
					pix16Src += pixmapRowbytes/2;
				}
				break;


		default:
				DoFatalAlert("\pOGL_BufferToGWorld: Only 32/16 bit textures supported right now.");

	}

	return(gworld);
}


/******************** OGL: CHECK ERROR ********************/

GLenum OGL_CheckError(void)
{
GLenum	err;
AGLContext agl_ctx = gAGLContext;


	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		switch(err)
		{
			case	GL_INVALID_ENUM:
					DoAlert("\pOGL_CheckError: GL_INVALID_ENUM");
					break;

			case	GL_INVALID_VALUE:
					DoAlert("\pOGL_CheckError: GL_INVALID_VALUE");
					break;

			case	GL_INVALID_OPERATION:
					DoAlert("\pOGL_CheckError: GL_INVALID_OPERATION");
					break;

			case	GL_STACK_OVERFLOW:
					DoAlert("\pOGL_CheckError: GL_STACK_OVERFLOW");
					break;

			case	GL_STACK_UNDERFLOW:
					DoAlert("\pOGL_CheckError: GL_STACK_UNDERFLOW");
					break;

			case	GL_OUT_OF_MEMORY:
					DoAlert("\pOGL_CheckError: GL_OUT_OF_MEMORY  (increase your Virtual Memory setting!)");
					break;

			default:
					DoAlert("\pOGL_CheckError: some other error");
					ShowSystemErr_NonFatal(err);
		}
	}

	return(err);
}


#pragma mark -


/********************* PUSH STATE **************************/

void OGL_PushState(void)
{
int	i;
AGLContext agl_ctx = gAGLContext;

		/* PUSH MATRIES WITH OPENGL */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glMatrixMode(GL_MODELVIEW);										// in my code, I keep modelview matrix as the currently active one all the time.


		/* SAVE OTHER INFO */

	i = gStateStackIndex++;											// get stack index and increment

	if (i >= STATE_STACK_SIZE)
		DoFatalAlert("\pOGL_PushState: stack overflow");

	gStateStack_Lighting[i] = gMyState_Lighting;
	gStateStack_CullFace[i] = glIsEnabled(GL_CULL_FACE);
	gStateStack_DepthTest[i] = glIsEnabled(GL_DEPTH_TEST);
	gStateStack_Normalize[i] = glIsEnabled(GL_NORMALIZE);
	gStateStack_Texture2D[i] = glIsEnabled(GL_TEXTURE_2D);
	gStateStack_Fog[i] 		= glIsEnabled(GL_FOG);
	gStateStack_Blend[i] 	= glIsEnabled(GL_BLEND);

	glGetFloatv(GL_CURRENT_COLOR, &gStateStack_Color[i][0]);

	glGetIntegerv(GL_BLEND_SRC, &gStateStack_BlendSrc[i]);
	glGetIntegerv(GL_BLEND_DST, &gStateStack_BlendDst[i]);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &gStateStack_DepthMask[i]);
}


/********************* POP STATE **************************/

void OGL_PopState(void)
{
int		i;
AGLContext agl_ctx = gAGLContext;

		/* RETREIVE OPENGL MATRICES */

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

		/* GET OTHER INFO */

	i = --gStateStackIndex;												// dec stack index

	if (i < 0)
		DoFatalAlert("\pOGL_PopState: stack underflow!");

	if (gStateStack_Lighting[i])
		OGL_EnableLighting();
	else
		OGL_DisableLighting();


	if (gStateStack_CullFace[i])
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);


	if (gStateStack_DepthTest[i])
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	if (gStateStack_Normalize[i])
		glEnable(GL_NORMALIZE);
	else
		glDisable(GL_NORMALIZE);

	if (gStateStack_Texture2D[i])
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);

	if (gStateStack_Blend[i])
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	if (gStateStack_Fog[i])
		glEnable(GL_FOG);
	else
		glDisable(GL_FOG);

	glDepthMask(gStateStack_DepthMask[i]);
	glBlendFunc(gStateStack_BlendSrc[i], gStateStack_BlendDst[i]);

	glColor4fv(&gStateStack_Color[i][0]);

}


/******************* OGL ENABLE LIGHTING ****************************/

void OGL_EnableLighting(void)
{
AGLContext agl_ctx = gAGLContext;

	gMyState_Lighting = true;
	glEnable(GL_LIGHTING);
}

/******************* OGL DISABLE LIGHTING ****************************/

void OGL_DisableLighting(void)
{
AGLContext agl_ctx = gAGLContext;

	gMyState_Lighting = false;
	glDisable(GL_LIGHTING);
}


#pragma mark -

/************************** OGL_INIT FONT **************************/

static void OGL_InitFont(void)
{
#if !OEM
AGLContext agl_ctx = gAGLContext;

	gFontList = glGenLists(256);

    if (!aglUseFont(gAGLContext, kFontIDMonaco, bold, 9, 0, 256, gFontList))
		DoFatalAlert("\pOGL_InitFont: aglUseFont failed");
#endif
}


/******************* OGL_FREE FONT ***********************/

static void OGL_FreeFont(void)
{
#if !OEM

AGLContext agl_ctx = gAGLContext;
	glDeleteLists(gFontList, 256);

#endif
}

/**************** OGL_DRAW STRING ********************/

void OGL_DrawString(Str255 s, GLint x, GLint y)
{
#if !OEM

AGLContext agl_ctx = gAGLContext;

	OGL_PushState();

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 0, 480, -10.0, 10.0);

	glDisable(GL_LIGHTING);

	glRasterPos2i(x, 480-y);

	glListBase(gFontList);
	glCallLists(s[0], GL_UNSIGNED_BYTE, &s[1]);

	OGL_PopState();

#endif
}

/**************** OGL_DRAW FLOAT ********************/

void OGL_DrawFloat(float f, GLint x, GLint y)
{
#if !OEM

Str255	s;

	FloatToString(f,s);
	OGL_DrawString(s,x,y);

#endif
}



/**************** OGL_DRAW INT ********************/

void OGL_DrawInt(int f, GLint x, GLint y)
{
#if !OEM

Str255	s;

	NumToString(f,s);
	OGL_DrawString(s,x,y);

#endif
}

#pragma mark -


/********************* OGL:  CHECK RENDERER **********************/
//
// Returns: true if renderer for the requested device complies, false otherwise
//

Boolean OGL_CheckRenderer (GDHandle hGD, long* vram)
{
AGLRendererInfo info, head_info;
GLint 			dAccel = 0;
Boolean			gotit = false;

			/**********************/
			/* GET FIRST RENDERER */
			/**********************/

	head_info = aglQueryRendererInfo(&hGD, 1);
	if(!head_info)
	{
		DoAlert("\pCheckRenderer: aglQueryRendererInfo failed");
		DoFatalAlert("\pThis is usually a result of installing OS 9.2.1 which has a faulty installer.  Delete all Nvidia extensions, reboot, and then the game will run.");
	}


		/*******************************************/
		/* SEE IF THERE IS AN ACCELERATED RENDERER */
		/*******************************************/

	info = head_info;

	while (info)
	{
		aglDescribeRenderer(info, AGL_ACCELERATED, &dAccel);

				/* GOT THE ACCELERATED RENDERER */

		if (dAccel)
		{
			gotit = true;

					/* GET VRAM */

			aglDescribeRenderer (info, AGL_TEXTURE_MEMORY, vram);



			break;
		}


				/* TRY NEXT ONE */

		info = aglNextRendererInfo(info);
	}



			/***********/
			/* CLEANUP */
			/***********/

	aglDestroyRendererInfo(head_info);

	return(gotit);
}






