/****************************/
/*   OPENGL SUPPORT.C	    */
/*   By Brian Greenstone    */
/* (c)2001 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "stb_image.h"
#include "pillarbox.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>

extern SDL_Window*		gSDLWindow;
//extern	GWorldPtr		gTerrainDebugGWorld;

_Static_assert(sizeof(OGLColorBGRA16) == 2, "OGLColorBGRA16 must fit on 2 bytes");


/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitDebugText(void);

static void OGL_CreateDrawContext(void);
static void OGL_DisposeDrawContext(void);
static void OGL_InitDrawContext(void);
static void OGL_SetStyles(OGLSetupInputType *setupDefPtr);
static void OGL_CreateLights(OGLLightDefType *lightDefPtr);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	STATE_STACK_SIZE	20

#define GAMMA_CORRECTION	1.5



/*********************/
/*    VARIABLES      */
/*********************/

SDL_GLContext	gAGLContext = nil;


//OGLMatrix4x4	gViewToFrustumMatrix,gWorldToViewMatrix,gWorldToFrustumMatrix;
OGLMatrix4x4	gViewToFrustumMatrix,gLocalToViewMatrix,gLocalToFrustumMatrix;
OGLMatrix4x4	gWorldToWindowMatrix[MAX_VIEWPORTS],gFrustumToWindowMatrix[MAX_VIEWPORTS];

int		gCurrentSplitScreenPane = 0;
int		gNumSplitScreenPanes = 1;								// does NOT account for global overlay viewport
Byte	gActiveSplitScreenMode 	= SPLITSCREEN_MODE_NONE;		// currently active split mode
Boolean	gDrawingOverlayPane = false;

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
int			gStateStack_ProjectionType[STATE_STACK_SIZE];

int			gStateStackIndex = 0;

int			gPolysThisFrame;
int			gVRAMUsedThisFrame = 0;

Boolean		gMyState_Lighting;
int			gMyState_ProjectionType = kProjectionType3D;

int			gLoadTextureFlags = 0;
static uint8_t		gGammaRamp8[256];
static uint16_t		gGammaRamp5[32];
static float		gGammaRampF[256];



/******************** OGL BOOT *****************/
//
// Initialize my OpenGL stuff.
//

void OGL_Boot(void)
{
	OGL_CreateDrawContext();
	OGL_CheckError();

	OGL_InitDrawContext();
	OGL_CheckError();



		/* INITIALIZE GAMMA RAMP */

	for (int i = 0; i < 256; i++)
	{
		double v = (double)i / 255.0;
		double corrected = pow(v, 1.0 / GAMMA_CORRECTION);
		gGammaRampF[i] = corrected;
		gGammaRamp8[i] = (uint8_t) (corrected * 255.0);
	}

	for (int i = 0; i < 32; i++)	// 5-bit gamma ramp (for 16-bit packed BGRA)
	{
		double v = (double)i / 31.0;
		double corrected = pow(v, 1.0 / GAMMA_CORRECTION);
		gGammaRamp5[i] = (uint16_t) (corrected * 31.0);
	}


		/***************************/
		/* GET OPENGL CAPABILITIES */
		/***************************/

		/* SEE IF SUPPORT 1024x1024 TEXTURES */

	GLint maxTexSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	if (maxTexSize < 1024)
		DoFatalAlert("Your video card cannot do 1024x1024 textures, so it is below the game's minimum system requirements.");
}


/******************** OGL SHUTDOWN *****************/

void OGL_Shutdown(void)
{
	OGL_DisposeDrawContext();
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

	memset(viewDef, 0, sizeof(*viewDef));

	viewDef->view.clearColor 		= clearColor;
	viewDef->view.clip.left 	= 0;
	viewDef->view.clip.right 	= 0;
	viewDef->view.clip.top 		= 0;
	viewDef->view.clip.bottom 	= 0;
	viewDef->view.numPanes	 	= 1;				// assume only 1 pane
	viewDef->view.clearBackBuffer = true;
	viewDef->view.pillarboxRatio= PILLARBOX_RATIO_4_3;
	viewDef->view.fontName		= "wallfont";

	for (int i = 0; i < MAX_VIEWPORTS; i++)
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

void OGL_SetupGameView(OGLSetupInputType *setupDefPtr)
{
	SDL_ShowCursor(0);	// do this just as a safety precaution to make sure no cursor lingering around

	GAME_ASSERT_MESSAGE(gGameView == NULL, "gGameView overwritten");

			/* ALLOC MEMORY FOR OUTPUT DATA */

	gGameView = (OGLSetupOutputType *)AllocPtrClear(sizeof(OGLSetupOutputType));
	GAME_ASSERT(gGameView);


			/* SET SOME PANE INFO */

	gCurrentSplitScreenPane = 0;
	gNumSplitScreenPanes = setupDefPtr->view.numPanes;
	switch (setupDefPtr->view.numPanes)
	{
		case	1:
				gActiveSplitScreenMode = SPLITSCREEN_MODE_NONE;
				break;

		case	2:
				gActiveSplitScreenMode = gGamePrefs.splitScreenMode2P;
				break;

		case	3:
				gActiveSplitScreenMode = gGamePrefs.splitScreenMode3P;
				break;

		case	4:
				gActiveSplitScreenMode = SPLITSCREEN_MODE_4P_GRID;
				break;

		case	5:	// For debugging
		case	6:	// For debugging
				gActiveSplitScreenMode = SPLITSCREEN_MODE_6P_GRID;
				break;

		default:
			DoFatalAlert("OGL_SetupGameView: # panes not implemented");
	}


				/* UPDATE WINDOW SIZE */

	SDL_GetWindowSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);


				/* SETUP */

	OGL_InitDrawContext();
	OGL_CheckError();

	OGLColorRGBA clearColor = setupDefPtr->view.clearColor;
	OGL_FixColorGamma(&clearColor);

	glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0);
	OGL_CheckError();

	OGL_SetStyles(setupDefPtr);
	OGL_CheckError();

	OGL_CreateLights(&setupDefPtr->lights);
	OGL_CheckError();

	SDL_GL_SetSwapInterval(1);//gCommandLine.vsync);



				/* PASS BACK INFO */

//	gGameView->drawContext 		= gAGLContext;
	gGameView->clip 			= setupDefPtr->view.clip;
	gGameView->hither 			= setupDefPtr->camera.hither;			// remember hither/yon
	gGameView->yon 				= setupDefPtr->camera.yon;
	gGameView->useFog 			= setupDefPtr->styles.useFog;
	gGameView->clearBackBuffer 	= setupDefPtr->view.clearBackBuffer;
	gGameView->pillarboxRatio	= setupDefPtr->view.pillarboxRatio;
	gGameView->fadePillarbox	= false;
	gGameView->fadeInDuration	= .25f;
	gGameView->fadeOutDuration	= .15f;

//	gGameView->isActive = true;											// it's now an active structure

	gGameView->lightList = setupDefPtr->lights;							// copy lights

	for (int i = 0; i < MAX_VIEWPORTS; i++)
	{
		gGameView->fov[i] = setupDefPtr->camera.fov;					// each camera will have its own fov so we can change it for special effects
		OGL_UpdateCameraFromTo(&setupDefPtr->camera.from[i], &setupDefPtr->camera.to[i], i);
	}


			/* LOAD FONT */

	LoadSpriteGroup(SPRITE_GROUP_FONT, setupDefPtr->view.fontName, kAtlasLoadFont);
	InitDebugText();


			/* PRIME PILLARBOX */

	InitPillarbox();


			/* PRIME 2D LOGICAL SIZE */
			//
			// The 2D logical size is updated on each frame, but computing it now
			// lets us create 2D objects in fixed-AR screens before the 1st frame is shown.
			//

	{
		int x, y, w, h;
		OGL_GetCurrentViewport(&x, &y, &w, &h, 0);

		gCurrentAspectRatio = (float) w / (float) (h == 0? 1: h);
		OGL_Update2DLogicalSize();
	}
}


/***************** OGL_DisposeGameView ***********************/
//
// Disposes of all data created by OGL_SetupGameView
//
// Source port note: The original used to kill the draw context here,
// but we keep one DC throughout the lifetime of the program.
//

void OGL_DisposeGameView(void)
{
	GAME_ASSERT(gGameView);									// see if this setup exists

		/* FREE MEMORY & NIL POINTER */

	SafeDisposePtr((Ptr) gGameView);
	gGameView = nil;
}




/**************** OGL: CREATE DRAW CONTEXT *********************/
//
// Source port note: The original used to create a new DC for each scene,
// but we keep one DC throughout the lifetime of the program.
//

static void OGL_CreateDrawContext(void)
{
	GAME_ASSERT_MESSAGE(gSDLWindow, "Window must be created before the DC!");

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
}


/**************** OGL: DISPOSE DRAW CONTEXT *********************/

static void OGL_DisposeDrawContext(void)
{
	if (!gAGLContext)
		return;

	SDL_GL_MakeCurrent(gSDLWindow, NULL);		// make context not current
	SDL_GL_DeleteContext(gAGLContext);			// nuke context

	gAGLContext = nil;
}

/**************** OGL: INIT DRAW CONTEXT *********************/
//
// Call this at the beginning of each scene.
//

static void OGL_InitDrawContext(void)
{
	GAME_ASSERT(gStateStackIndex == 0);

	glEnable(GL_DEPTH_TEST);								// use z-buffer

	{
		GLfloat	color[] = {1,1,1,1};									// set global material color to white
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
	}

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

  	glEnable(GL_NORMALIZE);

	OGL_DisableLighting();

	glClearColor(0, .25f, .5f, 1.0f);
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

	GAME_ASSERT(lightDefPtr->numFillLights <= MAX_FILL_LIGHTS);

	for (int i = 0; i < lightDefPtr->numFillLights; i++)
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


			/* DISABLE OTHER LIGHTS THAT MIGHT STILL BE ACTIVE FROM PREVIOUS SCENE */

	for (int i = lightDefPtr->numFillLights; i < MAX_FILL_LIGHTS; i++)
	{
		glDisable(GL_LIGHT0+i);
	}

}

/******************* OGL DRAW SCENE *********************/

void OGL_DrawScene(void (*drawRoutine)(void))
{
	GAME_ASSERT(gGameView);										// make sure it's legit

	int makeCurrentRC = SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);		// make context active
	if (makeCurrentRC != 0)
		DoFatalAlert(SDL_GetError());


#if 0
	if (gGammaFadePercent <= 0)							// if we just finished fading out and haven't started fading in yet, just show black
	{
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(gSDLWindow);					// end render loop
		return;
	}
#endif


			/* UPDATE WINDOW SIZE ONCE PER FRAME */

	SDL_GetWindowSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);


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

	if (gGameView->clearBackBuffer || (gDebugMode == 3))
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
	else
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}


//	glColor4f(1,1,1,1);
//	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);		// this lets us hot-switch between anaglyph and non-anaglyph in the settings


				/**************************************/
				/* DRAW EACH SPLIT-SCREEN PANE IF ANY */
				/**************************************/

	int numPasses = gNumSplitScreenPanes + 1;
	gDrawingOverlayPane = false;

	for (gCurrentSplitScreenPane = 0; gCurrentSplitScreenPane < numPasses; gCurrentSplitScreenPane++)
	{
		gDrawingOverlayPane = gCurrentSplitScreenPane == numPasses-1;

		int x, y, w, h;
		OGL_GetCurrentViewport(&x, &y, &w, &h, gCurrentSplitScreenPane);

		glViewport(x, y, w, h);
		gCurrentAspectRatio = (float) w / (float) (h == 0? 1: h);

		OGL_Update2DLogicalSize();


				/* GET UPDATED GLOBAL COPIES OF THE VARIOUS MATRICES */

		OGL_Camera_SetPlacementAndUpdateMatrices(gCurrentSplitScreenPane);


				/* CALL INPUT DRAW FUNCTION */

		if (drawRoutine != nil)
			drawRoutine();
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

void OGL_GetCurrentViewport(int *x, int *y, int *w, int *h, Byte whichPane)
{
int	t,b,l,r;


	t = gGameView->clip.top;
	b = gGameView->clip.bottom;
	l = gGameView->clip.left;
	r = gGameView->clip.right;

	int clippedWidth = gGameWindowWidth - l - r;
	int clippedHeight = gGameWindowHeight - t - b;

	// Split-screen divider thickness
	float div;
	if (gGameWindowWidth >= gGameWindowHeight)
		div = SPLITSCREEN_DIVIDER_THICKNESS * (gGameWindowHeight*(1.0f/480.0f));
	else
		div = SPLITSCREEN_DIVIDER_THICKNESS * (gGameWindowWidth*(1.0f/640.0f));
	div = GAME_MAX(1, div);

	if (whichPane >= gNumSplitScreenPanes) //gDrawingOverlayPane)
	{
		// Any pane IDs beyond gNumSplitScreenPanes is interpreted as
		// a global viewport overlaying the rest.
		*x = l;
		*y = t;
		*w = clippedWidth;
		*h = clippedHeight;
	}
	else
	if (gGameView->pillarboxRatio > 0)
	{
		int widescreenAdjustedWidth = gGameWindowHeight * gGameView->pillarboxRatio;

		if (widescreenAdjustedWidth <= gGameWindowWidth)
		{
			// keep height
			*w = widescreenAdjustedWidth;
			*h = gGameWindowHeight;
			*x = (gGameWindowWidth - *w) / 2;
			*y = 0;
		}
		else
		{
			// keep width
			*w = gGameWindowWidth;
			*h = gGameWindowWidth / gGameView->pillarboxRatio;
			*x = 0;
			*y = (gGameWindowHeight - *h) / 2;
		}
	}
	else
	switch(gActiveSplitScreenMode)
	{
		case	SPLITSCREEN_MODE_NONE:
				*x = l;
				*y = t;
				*w = clippedWidth;
				*h = clippedHeight;
				break;

		case	SPLITSCREEN_MODE_2P_WIDE:
		_2pwide:
				*x = l;
				*w = clippedWidth;
				*h = 0.5f * (clippedHeight - div) + 0.5f;
				switch(whichPane)
				{
					case	0:		// top pane (y points up!)
							*y = t + 0.5f * (clippedHeight + div) + 0.5f;
							break;

					case	1:		// bottom pane (y points up!)
							*y = t;
							break;

					default:
							DoFatalAlert("Unsupported pane %d in 2P_WIDE split", whichPane);
				}
				break;

		case	SPLITSCREEN_MODE_2P_TALL:
		_2ptall:
				*w = 0.5f * (clippedWidth - div) + 0.5f;
				*h = clippedHeight;
				*y = t;
				switch(whichPane)
				{
					case	0:
							*x = l;
							break;

					case	1:
							*x = l + 0.5f * (clippedWidth + div) + 0.5f;
							break;

					default:
							DoFatalAlert("Unsupported pane %d in 2P_TALL split", whichPane);
				}
				break;

		case	SPLITSCREEN_MODE_3P_WIDE:
				switch (whichPane)
				{
					case	0:				// Player 1 has top-left pane in 2x2 grid
					case	1:				// Player 2 has top-right pane in 2x2 grid
							goto _4pgrid;

					case	2:				// Player 3 has wide pane spanning bottom row
							whichPane = 1;
							goto _2pwide;

					default:
							DoFatalAlert("Unsupported pane %d in 3P_WIDE split", whichPane);
				}

		case	SPLITSCREEN_MODE_3P_TALL:
				switch (whichPane)
				{
					case	0:				// Player 1 has top-left pane in 2x2 grid
							goto _4pgrid;

					case	1:				// Player 2 has bottom-left pane in 2x2 grid
							whichPane = 2;
							goto _4pgrid;

					case	2:				// Player 3 has tall pane spanning right column
							whichPane = 1;
							goto _2ptall;

					default:
							DoFatalAlert("Unsupported pane %d in 3P_TALL split", whichPane);
				}
				break;

		case	SPLITSCREEN_MODE_4P_GRID:
		_4pgrid:
		{
				int column = whichPane & 1;
				int row    = whichPane < 2 ? 1 : 0;

				*w = 0.5f * (clippedWidth - div) + 0.5f;
				*h = 0.5f * (clippedHeight - div) + 0.5f;

				*x = l + (0.5f*column) * (clippedWidth  + div) + 0.5f;
				*y = t + (0.5f*row   ) * (clippedHeight + div) + 0.5f;

				break;
		}

		case	SPLITSCREEN_MODE_6P_GRID:
		{
				int column = whichPane % 3;
				int row    = whichPane < 3 ? 1 : 0;

				*w = (1.0f/3.0f) * (clippedWidth - div) + 0.5f;
				*h = (1.0f/2.0f) * (clippedHeight - div) + 0.5f;

				*x = l + (column*(1.0f/3.0f)) * (clippedWidth  + div) + 0.5f;
				*y = t + (row   *(1.0f/2.0f)) * (clippedHeight + div) + 0.5f;
				break;
		}

		default:
				DoFatalAlert("Unsupported split-screen mode %d", gActiveSplitScreenMode);
	}
}


#pragma mark -

void OGL_FixColorGamma(OGLColorRGBA* color)
{
	color->r = gGammaRampF[(uint8_t)(color->r * 255.0f)];
	color->g = gGammaRampF[(uint8_t)(color->g * 255.0f)];
	color->b = gGammaRampF[(uint8_t)(color->b * 255.0f)];
}

static void OGL_FixTextureGamma(uint8_t* imageMemory, int width, int height, GLint srcFormat, GLint dataType)
{
	GAME_ASSERT_MESSAGE(gGammaRamp8[255] != 0, "Gamma ramp wasn't initialized");

	switch (srcFormat)
	{
		case GL_RGB:
			if (dataType == GL_UNSIGNED_BYTE)
			{
				for (int i = 0; i < 3 * width * height; i++)
				{
					imageMemory[i] = gGammaRamp8[imageMemory[i]];
				}
				return;
			}
			break;

		case GL_RGBA:
			if (dataType == GL_UNSIGNED_BYTE)
			{
				for (int i = 0; i < 4 * width * height; i += 4)
				{
					imageMemory[i + 0] = gGammaRamp8[imageMemory[i + 0]];
					imageMemory[i + 1] = gGammaRamp8[imageMemory[i + 1]];
					imageMemory[i + 2] = gGammaRamp8[imageMemory[i + 2]];
				}
				return;
			}
			break;

		case GL_BGRA:
			if (dataType == GL_UNSIGNED_BYTE)
			{
				for (int i = 0; i < 4 * width * height; i += 4)
				{
					imageMemory[i+1] = gGammaRamp8[imageMemory[i+1]];
					imageMemory[i+2] = gGammaRamp8[imageMemory[i+2]];
					imageMemory[i+3] = gGammaRamp8[imageMemory[i+3]];
				}
				return;
			}
			else if (dataType == GL_UNSIGNED_SHORT_1_5_5_5_REV)
			{
				OGLColorBGRA16* colorPtr = ((OGLColorBGRA16*) imageMemory);
				for (int i = 0; i < width * height; i++)
				{
					colorPtr->b = gGammaRamp5[colorPtr->b];
					colorPtr->g = gGammaRamp5[colorPtr->g];
					colorPtr->r = gGammaRamp5[colorPtr->r];
					colorPtr++;
				}
				return;
			}
			break;
	}

	printf("Gamma correction not supported for srcFormat $%x / dataType $%x\n", srcFormat, dataType);
}


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

	GLint filter = gLoadTextureFlags & kLoadTextureFlags_NearestNeighbor
			? GL_NEAREST
			: GL_LINEAR;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	if (!(gLoadTextureFlags & kLoadTextureFlags_NoGammaFix))
	{
		OGL_FixTextureGamma(imageMemory, width, height, srcFormat, dataType);
	}

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
uint8_t*				pixelData = nil;
int						width;
int						height;
long					imageFileLength = 0;
Ptr						imageFileData = nil;

				/* LOAD PICTURE FILE */

	imageFileData = LoadDataFile(path, &imageFileLength);
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

void OGL_MoveCameraFromTo(float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ, int camNum)
{

			/* SET CAMERA COORDS */

	gGameView->cameraPlacement[camNum].cameraLocation.x += fromDX;
	gGameView->cameraPlacement[camNum].cameraLocation.y += fromDY;
	gGameView->cameraPlacement[camNum].cameraLocation.z += fromDZ;

	gGameView->cameraPlacement[camNum].pointOfInterest.x += toDX;
	gGameView->cameraPlacement[camNum].pointOfInterest.y += toDY;
	gGameView->cameraPlacement[camNum].pointOfInterest.z += toDZ;

	UpdateListenerLocation();
}


/*************** OGL_UpdateCameraFromTo ***************/

void OGL_UpdateCameraFromTo(OGLPoint3D *from, OGLPoint3D *to, int camNum)
{
static const OGLVector3D up = {0,1,0};

	if ((camNum < 0) || (camNum >= MAX_VIEWPORTS))
		DoFatalAlert("OGL_UpdateCameraFromTo: illegal camNum");

	gGameView->cameraPlacement[camNum].upVector 		= up;
	gGameView->cameraPlacement[camNum].cameraLocation 	= *from;
	gGameView->cameraPlacement[camNum].pointOfInterest 	= *to;

	UpdateListenerLocation();
}

/*************** OGL_UpdateCameraFromToUp ***************/

void OGL_UpdateCameraFromToUp(OGLPoint3D *from, OGLPoint3D *to, OGLVector3D *up, int camNum)
{
	if ((camNum < 0) || (camNum >= MAX_VIEWPORTS))
		DoFatalAlert("OGL_UpdateCameraFromToUp: illegal camNum");

	gGameView->cameraPlacement[camNum].upVector 		= *up;
	gGameView->cameraPlacement[camNum].cameraLocation 	= *from;
	gGameView->cameraPlacement[camNum].pointOfInterest 	= *to;

	UpdateListenerLocation();
}



/************** OGL: CAMERA SET PLACEMENT & UPDATE MATRICES **********************/
//
// This is called by OGL_DrawScene to initialize all of the view matrices,
// and to extract the current view matrices used for culling et.al.
//
// Assumes gCurrentAspectRatio is set!
//

void OGL_Camera_SetPlacementAndUpdateMatrices(int camNum)
{
OGLLightDefType	*lights;


			/* INIT PROJECTION MATRIX -- STANDARD PERSPECTIVE CAMERA */

	OGL_SetGluPerspectiveMatrix(
			&gViewToFrustumMatrix,
			gGameView->fov[camNum],
			gCurrentAspectRatio,
			gGameView->hither,
			gGameView->yon);



			/* INIT MODELVIEW MATRIX */

	OGL_SetGluLookAtMatrix(
			&gLocalToViewMatrix,
			&gGameView->cameraPlacement[camNum].cameraLocation,
			&gGameView->cameraPlacement[camNum].pointOfInterest,
			&gGameView->cameraPlacement[camNum].upVector);


			/* ENABLE THOSE MATRICES */

	OGL_SetProjection(kProjectionType3D);



		/* UPDATE LIGHT POSITIONS */

	lights =  &gGameView->lightList;						// point to light list
	for (int i = 0; i < lights->numFillLights; i++)
	{
		GLfloat lightVec[4];

		lightVec[0] = -lights->fillDirection[i].x;			// negate vector because OGL is stupid
		lightVec[1] = -lights->fillDirection[i].y;
		lightVec[2] = -lights->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);
	}


			/* GET VARIOUS CAMERA MATRICES */

	OGLMatrix4x4_Multiply(&gLocalToViewMatrix, &gViewToFrustumMatrix, &gLocalToFrustumMatrix);

	OGLMatrix4x4_GetFrustumToWindow(&gFrustumToWindowMatrix[camNum], camNum);
	OGLMatrix4x4_Multiply(&gLocalToFrustumMatrix, &gFrustumToWindowMatrix[camNum], &gWorldToWindowMatrix[camNum]);

	UpdateListenerLocation();
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
	gStateStack_ProjectionType[i] = gMyState_ProjectionType;

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

	gMyState_ProjectionType = gStateStack_ProjectionType[i];

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

/************************** DEBUG TEXT OBJECT **************************/

static char* UpdateDebugText(void)
{
	static char debugTextBuffer[256];
	extern short gNumFreeSupertiles;

	snprintf(debugTextBuffer, sizeof(debugTextBuffer),
		"FPS:\t%d"
		"\nTRIS:\t%d"
		"\nOBJS:\t%d"
		"\nVRAM:\t%d\vK"
		"\nPTRS:\t%d"
		"\nTILES:\t%d\v/%d"
		"\nCHAN:\t%d\v/%d"
		"\nRES:\t%d\vX\r%d\v/%d"
		"\nRACE:\t%s"
		"\nAS\vTEER\r:\t%+.02f"
		"\nSTEER:\t%+.02f%s"
		,
		(int)(gFramesPerSecond + .5f),
		gPolysThisFrame,
		gNumObjectNodes,
		gVRAMUsedThisFrame / 1024,
		gNumPointers,
		MAX_SUPERTILES - gNumFreeSupertiles,
		MAX_SUPERTILES,
		GetNumBusyEffectChannels(),
		MAX_CHANNELS,
		gGameWindowWidth,
		gGameWindowHeight,
		gNumSplitScreenPanes,
		FormatRaceTime(GetRaceTime(0)),
		gPlayerInfo[0].analogSteering.x,
		gPlayerInfo[0].steering,
		gPlayerInfo[0].steering == gPlayerInfo[0].analogSteering.x? "": "*"
	);

	return debugTextBuffer;
}

static void MoveDebugText(ObjNode* theNode)
{
	if (SetObjectVisible(theNode, gDebugMode != 0))
	{
		TextMesh_Update(UpdateDebugText(), kTextMeshAlignLeft, theNode);
	}
}

static void InitDebugText(void)
{
	NewObjectDefinitionType newObjDef =
	{
		.flags = STATUS_BIT_HIDDEN | STATUS_BIT_OVERLAYPANE | STATUS_BIT_MOVEINPAUSE,
		.slot = DEBUGTEXT_SLOT,
		.scale = .25f,
		.coord = (OGLPoint3D) { 0, 480.0f/2.0f, 0 },
		.moveCall = MoveDebugText,
		.projection = kProjectionType2DOrthoFullRect
	};
	TextMesh_NewEmpty(256, &newObjDef);
}


#pragma mark -

/***************** UPDATE 2D LOGICAL SIZE *******************/
//
// Compute logical width & height for UI elements.
//
// This allows positioning UI elements as if we were working with a 640x480 screen,
// regardless of the actual dimensions of the viewport.
// If the window is too narrow, the UI will be squashed to fit horizontally.
//
// gCurrentAspectRatio must be set prior to calling this function!
//

void OGL_Update2DLogicalSize(void)
{
	float referenceHeight;
	float referenceWidth;
	float invUIScale = 1;

	if (gCurrentSplitScreenPane >= gNumSplitScreenPanes)		// TODO: IsDrawingOverlayPane()
	{
		referenceWidth = 640;
		referenceHeight = 480;
	}
	else
	switch (gActiveSplitScreenMode)
	{
		case SPLITSCREEN_MODE_2P_TALL:
		_2ptall:
			referenceWidth = 320;
			referenceHeight = 480;
			break;

		case SPLITSCREEN_MODE_2P_WIDE:
		_2pwide:
			referenceWidth = 640;
			referenceHeight = 240;
			invUIScale = (1.0f / 0.75f);
			break;

		case SPLITSCREEN_MODE_4P_GRID:
		_4pgrid:
			referenceWidth = 640;
			referenceHeight = 480;
			invUIScale = (1.0f / 1.5f);		// scale up a little in grid mode
			break;

		case SPLITSCREEN_MODE_3P_WIDE:
			if (gCurrentSplitScreenPane == 2)		// Player 3 has wide pane (like 1x2 grid)
				goto _2pwide;
			else									// Players 1 & 2 have standard pane in 2x2 grid
				goto _4pgrid;

		case SPLITSCREEN_MODE_3P_TALL:
			if (gCurrentSplitScreenPane == 2)		// Player 3 has tall pane (like 2x1 grid)
				goto _2ptall;
			else									// Players 1 & 2 have standard pane in 2x2 grid
				goto _4pgrid;

		default:
			referenceWidth = 640;
			referenceHeight = 480;
			break;
	}

	referenceHeight *= invUIScale;
	referenceWidth *= invUIScale;

	float minAspectRatio = referenceWidth / referenceHeight;

	g2DLogicalHeight = referenceHeight;

	if (gCurrentAspectRatio < minAspectRatio)
		g2DLogicalWidth = referenceHeight * minAspectRatio;
	else
		g2DLogicalWidth = referenceHeight * gCurrentAspectRatio;
}



/***************** SET 2D/3D PROJECTION *******************/

void OGL_SetProjection(int projectionType)
{
	switch (projectionType)
	{
		case kProjectionType3D:
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf((const GLfloat*) &gViewToFrustumMatrix.value[0]);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf((const GLfloat*) &gLocalToViewMatrix.value[0]);
			break;

		case kProjectionType2DNDC:
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			break;

		case kProjectionType2DOrthoFullRect:
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, g2DLogicalWidth, g2DLogicalHeight, 0, 0, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			break;

		case kProjectionType2DOrthoCentered:
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(-g2DLogicalWidth*.5f, g2DLogicalWidth*.5f, g2DLogicalHeight*.5f, -g2DLogicalHeight*.5f, 0, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			break;

		default:
			GAME_ASSERT_MESSAGE("illegal projection type #%d", projectionType);
			break;
	}

	gMyState_ProjectionType = projectionType;
}

