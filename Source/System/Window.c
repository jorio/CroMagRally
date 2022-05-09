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

	// 2D state should have been set for us by STATUS_BITS_FOR_2D and ObjNode::Projection.
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	glColor4f(0, 0, 0, 1.0f - gGammaFadePercent);
	glBegin(GL_QUADS);
	glVertex3f(-1, -1, 0);
	glVertex3f(+1, -1, 0);
	glVertex3f(+1, +1, 0);
	glVertex3f(-1, +1, 0);
	glEnd();
	glDisable(GL_BLEND);

	OGL_PopState();
}




/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

ObjNode* MakeFadeEvent(Boolean fadeIn)
{
ObjNode	*newObj;
ObjNode		*thisNodePtr;

	if (gDebugMode)
	{
		gGammaFadePercent = fadeIn? 1: 0;
		return NULL;
	}

		/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	thisNodePtr = gFirstNodePtr;

	while (thisNodePtr)
	{
		if (thisNodePtr->MoveCall == MoveFadeEvent)
		{
			thisNodePtr->Flag[0] = fadeIn;								// set new mode
			return thisNodePtr;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */

	NewObjectDefinitionType newObjDef =
	{
		.genre = CUSTOM_GENRE,
		.slot = FADE_SLOT,
		.scale = 1,
		.flags = STATUS_BITS_FOR_2D | STATUS_BIT_OVERLAYPANE,
		.projection = kProjectionType2DNDC,
		.moveCall = MoveFadeEvent,
		.drawCall = DrawFadePane,
	};
	newObj = MakeNewObject(&newObjDef);
	newObj->Flag[0] = fadeIn;
	return newObj;
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
		.slot = FADE_SLOT,
		.scale = 1,
		.flags = STATUS_BIT_OVERLAYPANE | STATUS_BITS_FOR_2D,
		.drawCall = DrawFadePane,
		.projection = kProjectionType2DNDC,
	};
	MakeNewObject(&newObjDef);

	float timer = setupInfo->fadeDuration;
	while (timer >= 0)
	{
		gGammaFadePercent = 1.0f * timer / (setupInfo->fadeDuration + .01f);

		CalcFramesPerSecond();
		DoSDLMaintenance();

		if (updateRoutine)
			updateRoutine();

		OGL_DrawScene(setupInfo, drawRoutine);

		if (setupInfo->fadeSound)
		{
			FadeSound(gGammaFadePercent);
		}

		timer -= gFramesPerSecondFrac;
	}

	gGammaFadePercent = 0;

	CalcFramesPerSecond();
	DoSDLMaintenance();
	OGL_DrawScene(setupInfo, drawRoutine);

	if (setupInfo->fadeSound)
	{
		FadeSound(0);
		KillSong();
		StopAllEffectChannels();
		FadeSound(1);
	}
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
