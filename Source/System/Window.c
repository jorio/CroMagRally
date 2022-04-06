/****************************/
/*        WINDOWS           */
/* By Brian Greenstone      */
/* (c)2001 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include <SDL.h>
#include <stdlib.h>

extern SDL_Window* gSDLWindow;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFadeEvent(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

float			gGammaFadePercent;
int				gGameWindowWidth, gGameWindowHeight;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
	// This is filled in from gSDLWindow in-game
	gGameWindowWidth = 640;
	gGameWindowHeight = 480;
}



#pragma mark -


/****************  DRAW FADE OVERLAY *******************/

static void DrawFadePane(ObjNode* theNode, OGLSetupOutputType* setupInfo)
{
	OGL_PushState();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor4f(0, 0, 0, 1.0f - gGammaFadePercent);
	glBegin(GL_QUADS);
	glVertex3f(-1, +1, 0);
	glVertex3f(+1, +1, 0);
	glVertex3f(+1, -1, 0);
	glVertex3f(-1, -1, 0);
	glEnd();
	glDisable(GL_BLEND);

	OGL_PopState();
}




/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean	fadeIn)
{
ObjNode	*newObj;
ObjNode		*thisNodePtr;

	if (gDebugMode)
	{
		gGammaFadePercent = fadeIn? 1: 0;
		return;
	}

		/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	thisNodePtr = gFirstNodePtr;

	while (thisNodePtr)
	{
		if (thisNodePtr->MoveCall == MoveFadeEvent)
		{
			thisNodePtr->Flag[0] = fadeIn;								// set new mode
			return;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */

	NewObjectDefinitionType newObjDef =
	{
		.genre = CUSTOM_GENRE,
		.slot = SLOT_OF_DUMB + 1000,
		.scale = 1,
		.moveCall = MoveFadeEvent
	};
	newObj = MakeNewObject(&newObjDef);
	newObj->CustomDrawFunction = DrawFadePane;
	newObj->Flag[0] = fadeIn;
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF FADE IN */

	if (theNode->Flag[0])
	{
		gGammaFadePercent += 4.0f*fps;
		if (gGammaFadePercent >= 1.0f)										// see if @ 100%
		{
			gGammaFadePercent = 1.0f;
			DeleteObject(theNode);
		}
	}

			/* FADE OUT */
	else
	{
		gGammaFadePercent -= 4.0f*fps;
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
		{
			gGammaFadePercent = 0;
			DeleteObject(theNode);
		}
	}
}


/***************** FREEZE-FRAME FADE OUT ********************/

void OGL_FadeOutScene(
	OGLSetupOutputType* setupInfo,
	void (*drawRoutine)(OGLSetupOutputType*),
	void (*updateRoutine)(void))
{
	if (gDebugMode)
	{
		gGammaFadePercent = 0;
		return;
	}

	NewObjectDefinitionType newObjDef =
	{
		.genre = CUSTOM_GENRE,
		.slot = SLOT_OF_DUMB + 1000,
		.scale = 1,
	};
	ObjNode* newObj = MakeNewObject(&newObjDef);
	newObj->CustomDrawFunction = DrawFadePane;

	const float duration = 0.15f;
	float timer = duration;
	while (timer >= 0)
	{
		gGammaFadePercent = 1.0f * timer / duration;

		CalcFramesPerSecond();
		DoSDLMaintenance();

		if (updateRoutine)
			updateRoutine();

		OGL_DrawScene(setupInfo, drawRoutine);

		timer -= gFramesPerSecondFrac;
	}

	gGammaFadePercent = 0;

	CalcFramesPerSecond();
	DoSDLMaintenance();
	OGL_DrawScene(setupInfo, drawRoutine);
}


/************************** ENTER 2D *************************/
//
// For OS X - turn off DSp when showing 2D
//

void Enter2D(Boolean pauseDSp)
{
}


/************************** EXIT 2D *************************/
//
// For OS X - turn ON DSp when NOT 2D
//

void Exit2D(void)
{
}


/******************** MOVE WINDOW TO PREFERRED DISPLAY *******************/
//
// This works best in windowed mode.
// Turn off fullscreen before calling this!
//

static void MoveToPreferredDisplay(void)
{
#if !(__APPLE__)
	int currentDisplay = SDL_GetWindowDisplayIndex(gSDLWindow);

	if (currentDisplay != gGamePrefs.monitorNum)
	{
		SDL_SetWindowPosition(
			gSDLWindow,
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.monitorNum),
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.monitorNum));
	}
#endif
}

/*********************** SET FULLSCREEN MODE **********************/

void SetFullscreenMode(bool enforceDisplayPref)
{
	if (!gGamePrefs.fullscreen)
	{
		SDL_SetWindowFullscreen(gSDLWindow, 0);

		if (enforceDisplayPref)
		{
			MoveToPreferredDisplay();
		}
	}
	else
	{
#if !(__APPLE__)
		if (enforceDisplayPref)
		{
			int currentDisplay = SDL_GetWindowDisplayIndex(gSDLWindow);

			if (currentDisplay != gGamePrefs.monitorNum)
			{
				// We must switch back to windowed mode for the preferred monitor to take effect
				SDL_SetWindowFullscreen(gSDLWindow, 0);
				MoveToPreferredDisplay();
			}
		}
#endif

		// Enter fullscreen mode
		SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

	// Ensure the clipping pane gets resized properly after switching in or out of fullscreen mode
//	int width, height;
//	SDL_GetWindowSize(gSDLWindow, &width, &height);
//	QD3D_OnWindowResized(width, height);

	SDL_ShowCursor(gGamePrefs.fullscreen ? 0 : 1);
}
