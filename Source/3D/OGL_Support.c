/****************************/
/*   OPENGL SUPPORT.C	    */
/* (c)2001 Pangea Software  */
/*   By Brian Greenstone    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>

#include "globals.h"
#include "misc.h"
#include "window.h"
#include "ogl_support.h"
#include "3dmath.h"
#include "main.h"
#include "player.h"
#include "input.h"
#include "file.h"
#include "sound2.h"
#include "stb_image.h"
#include <string.h>

extern SDL_Window*		gSDLWindow;
extern int				gNumObjectNodes,gNumPointers;
extern	MOMaterialObject	*gMostRecentMaterial;
extern	GWorldPtr		gTerrainDebugGWorld;
extern	short			gNumSuperTilesDrawn,gNumActiveParticleGroups,gNumFencesDrawn,gNumTotalPlayers;
extern	PlayerInfoType	gPlayerInfo[];
extern	float			gFramesPerSecond,gCameraStartupTimer;
extern	Byte			gDebugMode;
extern	Boolean			gAutoPilot,gNetGameInProgress,gOSX;
extern	uint16_t			gPlayer0TileAttribs;
extern	uint32_t			gGlobalMaterialFlags;
extern	PrefsType			gGamePrefs;
extern	int				gGameWindowWidth,gGameWindowHeight;
extern	CGrafPtr				gDisplayContextGrafPtr;
//extern	DisplayIDType		gOurDisplayID;
extern	float 			gGammaFadePercent;
extern	FSSpec			gDataSpec;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void OGL_InitFont(void);

static void OGL_CreateDrawContext(OGLViewDefType *viewDefPtr);
static void OGL_SetStyles(OGLSetupInputType *setupDefPtr);
static void OGL_CreateLights(OGLLightDefType *lightDefPtr);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	STATE_STACK_SIZE	20



/*********************/
/*    VARIABLES      */
/*********************/

SDL_GLContext	gAGLContext = nil;


//OGLMatrix4x4	gViewToFrustumMatrix,gWorldToViewMatrix,gWorldToFrustumMatrix;
OGLMatrix4x4	gViewToFrustumMatrix,gLocalToViewMatrix,gLocalToFrustumMatrix;
OGLMatrix4x4	gWorldToWindowMatrix[MAX_SPLITSCREENS],gFrustumToWindowMatrix[MAX_SPLITSCREENS];

int		gCurrentSplitScreenPane = 0;
int		gNumSplitScreenPanes = 1;
Byte	gActiveSplitScreenMode 	= SPLITSCREEN_MODE_NONE;		// currently active split mode

float	gCurrentAspectRatio = 1;
float	g2DLogicalWidth		= 640.0f;
float	g2DLogicalHeight	= 480.0f;


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

Boolean		gMyState_Lighting;

static ObjNode* gDebugText;


/******************** OGL BOOT *****************/
//
// Initialize my OpenGL stuff.
//

void OGL_Boot(void)
{
}


/*********************** OGL: NEW VIEW DEF **********************/
//
// fills a view def structure with default values.
//

void OGL_NewViewDef(OGLSetupInputType *viewDef)
{
const OGLColorRGBA		clearColor = {0,0,0,1};
const OGLPoint3D			cameraFrom = { 0, 0, 0.0 };
const OGLPoint3D			cameraTo = { 0, 0, -1 };
const OGLVector3D			cameraUp = { 0.0, 1.0, 0.0 };
const OGLColorRGBA			ambientColor = { .3, .3, .3, 1 };
const OGLColorRGBA			fillColor = { 1.0, 1.0, 1.0, 1 };
static OGLVector3D			fillDirection1 = { 1, 0, -1 };
static OGLVector3D			fillDirection2 = { -1, -.3, -.3 };


	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);
	OGLVector3D_Normalize(&fillDirection2, &fillDirection2);

	viewDef->view.clearColor 		= clearColor;
	viewDef->view.clip.left 	= 0;
	viewDef->view.clip.right 	= 0;
	viewDef->view.clip.top 		= 0;
	viewDef->view.clip.bottom 	= 0;
	viewDef->view.numPanes	 	= 1;				// assume only 1 pane
	viewDef->view.clearBackBuffer = true;

	for (int i = 0; i < MAX_SPLITSCREENS; i++)
	{
		viewDef->camera.from[i]		= cameraFrom;
		viewDef->camera.to[i]		= cameraTo;
		viewDef->camera.up[i]		= cameraUp;
	}
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
	viewDef->lights.numFillLights 	= 1;
	viewDef->lights.fillDirection[0] = fillDirection1;
	viewDef->lights.fillDirection[1] = fillDirection2;
	viewDef->lights.fillColor[0] 	= fillColor;
	viewDef->lights.fillColor[1] 	= fillColor;
}


/************** SETUP OGL WINDOW *******************/

void OGL_SetupWindow(OGLSetupInputType *setupDefPtr, OGLSetupOutputType **outputHandle)
{
OGLSetupOutputType	*outputPtr;

	SDL_ShowCursor(0);	// do this just as a safety precaution to make sure no cursor lingering around

			/* ALLOC MEMORY FOR OUTPUT DATA */

	outputPtr = (OGLSetupOutputType *)AllocPtr(sizeof(OGLSetupOutputType));
	if (outputPtr == nil)
		DoFatalAlert("OGL_SetupWindow: AllocPtr failed");


				/* SETUP */

	OGL_CreateDrawContext(&setupDefPtr->view);
	OGL_CheckError();

	OGL_SetStyles(setupDefPtr);
	OGL_CheckError();

	OGL_CreateLights(&setupDefPtr->lights);
	OGL_CheckError();

	SDL_GL_SetSwapInterval(1);//gCommandLine.vsync);



				/* PASS BACK INFO */

//	outputPtr->drawContext 		= gAGLContext;
	outputPtr->clip 			= setupDefPtr->view.clip;
	outputPtr->hither 			= setupDefPtr->camera.hither;			// remember hither/yon
	outputPtr->yon 				= setupDefPtr->camera.yon;
	outputPtr->useFog 			= setupDefPtr->styles.useFog;
	outputPtr->clearBackBuffer 	= setupDefPtr->view.clearBackBuffer;

	outputPtr->isActive = true;											// it's now an active structure

	outputPtr->lightList = setupDefPtr->lights;							// copy lights

	for (int i = 0; i < MAX_SPLITSCREENS; i++)
	{
		outputPtr->fov[i] = setupDefPtr->camera.fov;					// each camera will have its own fov so we can change it for special effects
		OGL_UpdateCameraFromTo(outputPtr, &setupDefPtr->camera.from[i], &setupDefPtr->camera.to[i], i);
	}

	*outputHandle = outputPtr;											// return value to caller


//	TextMesh_InitMaterial(outputPtr, setupDefPtr->styles.redFont);
	OGL_InitFont();
}


/***************** OGL_DisposeWindowSetup ***********************/
//
// Disposes of all data created by OGL_SetupWindow
//

void OGL_DisposeWindowSetup(OGLSetupOutputType **dataHandle)
{
OGLSetupOutputType	*data;

	data = *dataHandle;
	GAME_ASSERT(data);										// see if this setup exists

			/* KILL FONT MATERIAL */

//	TextMesh_DisposeMaterial();

			/* KILL GL CONTEXT */

	SDL_GL_MakeCurrent(gSDLWindow, NULL);					// make context not current
	SDL_GL_DeleteContext(gAGLContext);						// nuke the AGL context


		/* FREE MEMORY & NIL POINTER */

	data->isActive = false;									// now inactive
	SafeDisposePtr((Ptr)data);
	*dataHandle = nil;

	gAGLContext = nil;
}




/**************** OGL: CREATE DRAW CONTEXT *********************/

static void OGL_CreateDrawContext(OGLViewDefType *viewDefPtr)
{
GLint			maxTexSize;

			/* CREATE AGL CONTEXT & ATTACH TO WINDOW */

	gAGLContext = SDL_GL_CreateContext(gSDLWindow);

	if (!gAGLContext)
		DoFatalAlert(SDL_GetError());

	GAME_ASSERT(glGetError() == GL_NO_ERROR);


			/* ACTIVATE CONTEXT */

	int mkc = SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);
	GAME_ASSERT_MESSAGE(mkc == 0, SDL_GetError());


#if 0
			/* GET OPENGL EXTENSIONS */
			//
			// On Mac/Linux, we only need to do this once.
			// But on Windows, we must do it whenever we create a draw context.
			//

	OGL_InitFunctions();
#endif


				/* SET VARIOUS STATE INFO */


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

//	char* s = (char *)glGetString(GL_EXTENSIONS);					// get extensions list



			/* SEE IF SUPPORT 1024x1024 TEXTURES */

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	if (maxTexSize < 1024)
		DoFatalAlert("Your video card cannot do 1024x1024 textures, so it is below the game's minimum system requirements.");


				/* CLEAR BACK BUFFER ENTIRELY */

	glClearColor(0,0,0, 1.0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(gSDLWindow);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(viewDefPtr->clearColor.r, viewDefPtr->clearColor.g, viewDefPtr->clearColor.b, 1.0);

}



/**************** OGL: SET STYLES ****************/

static void OGL_SetStyles(OGLSetupInputType *setupDefPtr)
{
OGLStyleDefType *styleDefPtr = &setupDefPtr->styles;


	glEnable(GL_CULL_FACE);									// activate culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);									// CCW is front face

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// set default blend func
	glDisable(GL_BLEND);									// but turn it off by default

	glDisable(GL_RESCALE_NORMAL);

    glHint(GL_FOG_HINT, GL_NICEST);		// pixel accurate fog?

	OGL_CheckError();

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

	OGL_CheckError();
}




/********************* OGL: CREATE LIGHTS ************************/
//
// NOTE:  The Projection matrix must be the identity or lights will be transformed.
//

static void OGL_CreateLights(OGLLightDefType *lightDefPtr)
{
int		i;
GLfloat	ambient[4];

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
	if (setupInfo == nil)										// make sure it's legit
		DoFatalAlert("OGL_DrawScene setupInfo == nil");
	if (!setupInfo->isActive)
		DoFatalAlert("OGL_DrawScene isActive == false");

	int makeCurrentRC = SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);		// make context active
	if (makeCurrentRC != 0)
		DoFatalAlert(SDL_GetError());


	if (gGammaFadePercent <= 0)							// if we just finished fading out and haven't started fading in yet, just show black
	{
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(gSDLWindow);					// end render loop
		return;
	}


			/* INIT SOME STUFF */

	if (gDebugMode)
	{
		int depth = 32;
		gVRAMUsedThisFrame = gGameWindowWidth * gGameWindowHeight * (depth / 8);	// backbuffer size
		gVRAMUsedThisFrame += gGameWindowWidth * gGameWindowHeight * 2;				// z-buffer size
		gVRAMUsedThisFrame += gGameWindowWidth * gGameWindowHeight * (depth / 8);	// display size
	}


	gPolysThisFrame 	= 0;										// init poly counter
	gMostRecentMaterial = nil;
	gGlobalMaterialFlags = 0;
	glColor4f(1,1,1,1);
	glEnable(GL_COLOR_MATERIAL); //---

				/*****************/
				/* CLEAR BUFFERS */
				/*****************/

	if (setupInfo->clearBackBuffer || (gDebugMode == 3))
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
	else
		glClear(GL_DEPTH_BUFFER_BIT);


	glColor4f(1,1,1,1);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);		// this lets us hot-switch between anaglyph and non-anaglyph in the settings


				/**************************************/
				/* DRAW EACH SPLIT-SCREEN PANE IF ANY */
				/**************************************/

	for (gCurrentSplitScreenPane = 0; gCurrentSplitScreenPane < gNumSplitScreenPanes; gCurrentSplitScreenPane++)
	{
		int x, y, w, h;
		OGL_GetCurrentViewport(setupInfo, &x, &y, &w, &h, gCurrentSplitScreenPane);
		glViewport(x, y, w, h);
		gCurrentAspectRatio = (float) w / (float) (h == 0? 1: h);

		// Compute logical width & height for 2D elements
		g2DLogicalHeight = 480.0f;
		if (gCurrentAspectRatio < 4.0f/3.0f)
			g2DLogicalWidth = 640.0f;
		else
			g2DLogicalWidth = 480.0f * gCurrentAspectRatio;


				/* GET UPDATED GLOBAL COPIES OF THE VARIOUS MATRICES */

		OGL_Camera_SetPlacementAndUpdateMatrices(setupInfo, gCurrentSplitScreenPane);


				/* CALL INPUT DRAW FUNCTION */

		if (drawRoutine != nil)
			drawRoutine(setupInfo);
	}


		/**************************/
		/* SEE IF SHOW DEBUG INFO */
		/**************************/

	if (GetNewKeyState(SDL_SCANCODE_F8))
	{
		if (++gDebugMode > 3)
			gDebugMode = 0;

		if (gDebugMode == 3)								// see if show wireframe
			glPolygonMode(GL_FRONT_AND_BACK ,GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK ,GL_FILL);
	}

				/* SHOW BASIC DEBUG INFO */

	if (gDebugMode > 0)
	{
		IMPLEMENT_ME_SOFT();
#if 0
		int		y = 200;
		int		mem = FreeMem();

		if (mem < gMinRAM)		// poll for lowest RAM free
			gMinRAM = mem;

		OGL_DrawString("fps:", 20,y);
		OGL_DrawInt(gFramesPerSecond+.5f, 100,y);
		y += 15;

		OGL_DrawString("#tri:", 20,y);
		OGL_DrawInt(gPolysThisFrame, 100,y);
		y += 15;

		OGL_DrawString("tri/sec:", 20,y);
		OGL_DrawInt((float)gPolysThisFrame * gFramesPerSecond, 100,y);
		y += 15;

		OGL_DrawString("#pgroups:", 20,y);
		OGL_DrawInt(gNumActiveParticleGroups, 100,y);
		y += 15;

		OGL_DrawString("#objNodes:", 20,y);
		OGL_DrawInt(gNumObjectNodes, 100,y);
		y += 15;

//		OGL_DrawString("#fences:", 20,y);
//		OGL_DrawInt(gNumFencesDrawn, 100,y);
//		y += 15;

		OGL_DrawString("#free RAM:", 20,y);
		OGL_DrawInt(mem, 100,y);
		y += 15;

		OGL_DrawString("min RAM:", 20,y);
		OGL_DrawInt(gMinRAM, 100,y);
		y += 15;

		OGL_DrawString("used VRAM:", 20,y);
		OGL_DrawInt(gVRAMUsedThisFrame, 100,y);
		y += 15;

		OGL_DrawString("OGL Mem:", 20,y);
		OGL_DrawInt(glmGetInteger(GLM_CURRENT_MEMORY), 100,y);
		y += 15;

//		OGL_DrawString("# pointers:", 20,y);
//		OGL_DrawInt(gNumPointers, 100,y);
//		y += 15;


#endif
	}



            /**************/
			/* END RENDER */
			/**************/

           /* SWAP THE BUFFS */

	SDL_GL_SwapWindow(gSDLWindow);					// end render loop
}


/********************** OGL: GET CURRENT VIEWPORT ********************/
//
// Remember that with OpenGL, the bottom of the screen is y==0, so some of this code
// may look upside down.
//

void OGL_GetCurrentViewport(const OGLSetupOutputType *setupInfo, int *x, int *y, int *w, int *h, Byte whichPane)
{
int	t,b,l,r;

	SDL_GetWindowSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);

	t = setupInfo->clip.top;
	b = setupInfo->clip.bottom;
	l = setupInfo->clip.left;
	r = setupInfo->clip.right;

#if 0
	*x = l;
	*y = t;
	*w = gGameWindowWidth-l-r;
	*h = gGameWindowHeight-t-b;

	if (whichPane != 0)
		IMPLEMENT_ME_SOFT();
#endif

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
					
					default:
							DoFatalAlert("Unsupported pane # (horiz split)");
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
						
					default:
							DoFatalAlert("Unsupported pane # (vert split)");
				}
				break;
		
		default:
				DoFatalAlert("Unsupported split-screen mode %d", gActiveSplitScreenMode);
	}
}


#pragma mark -


/***************** OGL TEXTUREMAP LOAD **************************/

GLuint OGL_TextureMap_Load(void *imageMemory, int width, int height,
							GLint srcFormat,  GLint destFormat, GLint dataType)
{
GLuint	textureName;


			/* GET A UNIQUE TEXTURE NAME & INITIALIZE IT */

	glGenTextures(1, &textureName);
	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glGenTextures failed!");

	glBindTexture(GL_TEXTURE_2D, textureName);				// this is now the currently active texture
	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glBindTexture failed!");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D,
				0,										// mipmap level
				destFormat,								// format in OpenGL
				width,									// width in pixels
				height,									// height in pixels
				0,										// border
				srcFormat,								// what my format is
				dataType,								// size of each r,g,b
				imageMemory);							// pointer to the actual texture pixels


			/* SEE IF RAN OUT OF MEMORY WHILE COPYING TO OPENGL */

	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glTexImage2D failed!");


				/* SET THIS TEXTURE AS CURRENTLY ACTIVE FOR DRAWING */

	OGL_Texture_SetOpenGLTexture(textureName);

	return(textureName);
}

/***************** OGL TEXTUREMAP LOAD FROM PNG/JPG **********************/

GLuint OGL_TextureMap_LoadImageFile(const char* path, int* outWidth, int* outHeight)
{
FSSpec					spec;
uint8_t*				pixelData = nil;
OSErr					err;
int						width;
int						height;
long					imageFileLength = 0;
Ptr						imageFileData = nil;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);

			/* LOAD RAW ARGB DATA FROM TGA FILE */

				/* LOAD PICTURE FILE */

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
	GAME_ASSERT(!err);

	imageFileData = LoadFileData(&spec, &imageFileLength);
	GAME_ASSERT(imageFileData);

	pixelData = (uint8_t*) stbi_load_from_memory((const stbi_uc*) imageFileData, imageFileLength, &width, &height, NULL, 4);
	GAME_ASSERT(pixelData);

	SafeDisposePtr(imageFileData);
	imageFileData = NULL;

			/* PRE-PROCESS IMAGE */

	int internalFormat = GL_RGBA;

#if 0
	if (flags & kLoadTextureFlags_KeepOriginalAlpha)
	{
		internalFormat = GL_RGBA;
	}
	else
	{
		internalFormat = GL_RGB;
	}
#endif

			/* LOAD TEXTURE */

	GLuint glTextureName = OGL_TextureMap_Load(
			pixelData,
			width,
			height,
			GL_RGBA,
			internalFormat,
			GL_UNSIGNED_BYTE);
	GAME_ASSERT(!OGL_CheckError());

			/* CLEAN UP */

	//DisposePtr((Ptr) pixelData);
	free(pixelData);  // TODO: define STBI_MALLOC/STBI_REALLOC/STBI_FREE in stb_image.c?

	if (outWidth)
		*outWidth = width;
	if (outHeight)
		*outHeight = height;

	return glTextureName;
}


/****************** OGL: TEXTURE SET OPENGL TEXTURE **************************/
//
// Sets the current OpenGL texture using glBindTexture et.al. so any textured triangles will use it.
//

void OGL_Texture_SetOpenGLTexture(GLuint textureName)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (OGL_CheckError())
		DoFatalAlert("OGL_Texture_SetOpenGLTexture: glPixelStorei failed!");

	glBindTexture(GL_TEXTURE_2D, textureName);
	if (OGL_CheckError())
		DoFatalAlert("OGL_Texture_SetOpenGLTexture: glBindTexture failed!");


	glGetError();

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
		DoFatalAlert("OGL_UpdateCameraFromTo: illegal camNum");

	setupInfo->cameraPlacement[camNum].upVector 		= up;
	setupInfo->cameraPlacement[camNum].cameraLocation 	= *from;
	setupInfo->cameraPlacement[camNum].pointOfInterest 	= *to;

	UpdateListenerLocation(setupInfo);
}

/*************** OGL_UpdateCameraFromToUp ***************/

void OGL_UpdateCameraFromToUp(OGLSetupOutputType *setupInfo, OGLPoint3D *from, OGLPoint3D *to, OGLVector3D *up, int camNum)
{
	if ((camNum < 0) || (camNum >= MAX_SPLITSCREENS))
		DoFatalAlert("OGL_UpdateCameraFromToUp: illegal camNum");

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
int		temp, w, h, i;
OGLLightDefType	*lights;

	OGL_GetCurrentViewport(setupInfo, &temp, &temp, &w, &h, 0);
	aspect = (float)w/(float)h;

			/* INIT PROJECTION MATRIX */

	glMatrixMode(GL_PROJECTION);

			/* SETUP STANDARD PERSPECTIVE CAMERA */

	OGL_SetGluPerspectiveMatrix(
			&gViewToFrustumMatrix,
			setupInfo->fov[camNum],
			aspect,
			setupInfo->hither,
			setupInfo->yon);
	glLoadMatrixf((const GLfloat*) &gViewToFrustumMatrix.value[0]);



			/* INIT MODELVIEW MATRIX */

	glMatrixMode(GL_MODELVIEW);
	OGL_SetGluLookAtMatrix(
			&gLocalToViewMatrix, //&gWorldToViewMatrix,
			&setupInfo->cameraPlacement[camNum].cameraLocation,
			&setupInfo->cameraPlacement[camNum].pointOfInterest,
			&setupInfo->cameraPlacement[camNum].upVector);
	//glLoadMatrixf((const GLfloat*) &gWorldToViewMatrix.value[0]);
	glLoadMatrixf((const GLfloat*) &gLocalToViewMatrix.value[0]);



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

	//OGLMatrix4x4_Multiply(&gWorldToViewMatrix, &gViewToFrustumMatrix, &gWorldToFrustumMatrix);
	OGLMatrix4x4_Multiply(&gLocalToViewMatrix, &gViewToFrustumMatrix, &gLocalToFrustumMatrix);

	OGLMatrix4x4_GetFrustumToWindow(setupInfo, &gFrustumToWindowMatrix[camNum], camNum);
	//OGLMatrix4x4_Multiply(&gWorldToFrustumMatrix, &gFrustumToWindowMatrix[camNum], &gWorldToWindowMatrix);
	OGLMatrix4x4_Multiply(&gLocalToFrustumMatrix, &gFrustumToWindowMatrix[camNum], &gWorldToWindowMatrix[camNum]);

	UpdateListenerLocation(setupInfo);
}



#pragma mark -


/******************** OGL: CHECK ERROR ********************/

GLenum _OGL_CheckError(const char* file, const int line)
{
	GLenum error = glGetError();
	if (error != 0)
	{
		DoFatalAlert("OpenGL error 0x%x in %s:%d", error, file, line);
	}
	return error;
}



#pragma mark -


/********************* PUSH STATE **************************/

void OGL_PushState(void)
{
int	i;

		/* PUSH MATRIES WITH OPENGL */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glMatrixMode(GL_MODELVIEW);										// in my code, I keep modelview matrix as the currently active one all the time.


		/* SAVE OTHER INFO */

	i = gStateStackIndex++;											// get stack index and increment

	if (i >= STATE_STACK_SIZE)
		DoFatalAlert("OGL_PushState: stack overflow");

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

		/* RETREIVE OPENGL MATRICES */

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

		/* GET OTHER INFO */

	i = --gStateStackIndex;												// dec stack index

	if (i < 0)
		DoFatalAlert("OGL_PopState: stack underflow!");

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
	gMyState_Lighting = true;
	glEnable(GL_LIGHTING);
}

/******************* OGL DISABLE LIGHTING ****************************/

void OGL_DisableLighting(void)
{
	gMyState_Lighting = false;
	glDisable(GL_LIGHTING);
}


#pragma mark -

/************************** OGL_INIT FONT **************************/

static void OGL_InitFont(void)
{
IMPLEMENT_ME_SOFT();
#if 0
	NewObjectDefinitionType newObjDef;
	memset(&newObjDef, 0, sizeof(newObjDef));
	newObjDef.flags = STATUS_BIT_HIDDEN;
	newObjDef.slot = DEBUGOVERLAY_SLOT;
	newObjDef.scale = 0.45f;
	newObjDef.coord = (OGLPoint3D) { -320, -100, 0 };
	gDebugText = TextMesh_NewEmpty(2048, &newObjDef);
#endif
}


