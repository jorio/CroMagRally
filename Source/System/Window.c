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

#define FaderMode				Flag[0]
#define FaderDone				Flag[1]
#define FaderSpeed				SpecialF[0]
#define FaderFrameCounter		Special[0]

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
			thisNodePtr->FaderMode = fadeIn;							// set new mode
			return thisNodePtr;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */

	NewObjectDefinitionType newObjDef =
	{
		.genre = QUADMESH_GENRE,
		.slot = FADE_SLOT,
		.scale = 1,
		.flags = STATUS_BITS_FOR_2D | STATUS_BIT_OVERLAYPANE | STATUS_BIT_MOVEINPAUSE,
		.projection = kProjectionType2DNDC,
		.moveCall = MoveFadeEvent,
	};

	newObj = MakeQuadMeshObject(&newObjDef, 1, NULL);

	float fadeDuration = fadeIn? gGameView->fadeInDuration: gGameView->fadeOutDuration;
	if (fabsf(fadeDuration) < .01f)	// don't div/0
		fadeDuration = .01f;

	newObj->FaderMode = fadeIn;
	newObj->FaderDone = false;
	newObj->FaderSpeed = 1.0f / fadeDuration;
	newObj->FaderFrameCounter = 0;

	MOVertexArrayData* mesh = GetQuadMeshWithin(newObj);
	mesh->numPoints = 4;
	mesh->numTriangles = 2;
	mesh->points[0] = (OGLPoint3D) { -1, -1, 0 };
	mesh->points[1] = (OGLPoint3D) { +1, -1, 0 };
	mesh->points[2] = (OGLPoint3D) { +1, +1, 0 };
	mesh->points[3] = (OGLPoint3D) { -1, +1, 0 };

	newObj->ColorFilter = (OGLColorRGBA) {0,0,0, fadeIn? 1: 0};

	return newObj;
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->FaderFrameCounter++;

			/* SEE IF FADE IN */

	if (theNode->FaderMode)
	{
		gGammaFadePercent += fps * theNode->FaderSpeed;
		if (gGammaFadePercent >= 1.0f)										// see if @ 100%
		{
			gGammaFadePercent = 1.0f;
			theNode->FaderDone = true;
		}
	}

			/* FADE OUT */
	else
	{
		gGammaFadePercent -= fps * theNode->FaderSpeed;
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
		{
			gGammaFadePercent = 0;
			theNode->FaderDone = true;
		}
	}


	theNode->ColorFilter = (OGLColorRGBA) {0, 0, 0, 1.0f - gGammaFadePercent};

	if (gGameView->fadeSound)
	{
		FadeSound(gGammaFadePercent);
	}

	if (theNode->FaderDone)
	{
		if (theNode->FaderMode)			// nuke it if fading in, hold it if fading out
			DeleteObject(theNode);

		theNode->MoveCall = NULL;		// don't run this again
	}
}


/***************** FREEZE-FRAME FADE OUT ********************/

void OGL_FadeOutScene(void (*drawCall)(void), void (*moveCall)(void))
{
	if (gDebugMode)
	{
		gGammaFadePercent = 0;
		return;
	}

	ObjNode* fader = MakeFadeEvent(false);
	long pFaderFrameCount = fader->FaderFrameCounter;

	while (!fader->FaderDone)
	{
		CalcFramesPerSecond();
		DoSDLMaintenance();

		if (moveCall)
		{
			moveCall();
		}

		// Force fader object to move even if MoveObjects was skipped
		if (fader->FaderFrameCounter == pFaderFrameCount)	// fader wasn't moved by moveCall
		{
			MoveFadeEvent(fader);
			pFaderFrameCount = fader->FaderFrameCounter;
		}

		OGL_DrawScene(drawCall);
	}

	// Draw one more blank frame
	gGammaFadePercent = 0;
	CalcFramesPerSecond();
	DoSDLMaintenance();
	OGL_DrawScene(drawCall);

	if (gGameView->fadeSound)
	{
		FadeSound(0);
		KillSong();
		StopAllEffectChannels();
		FadeSound(1);		// restore sound volume for new playback
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
