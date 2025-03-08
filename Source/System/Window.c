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

extern SDL_Window* gSDLWindow;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFadeEvent(ObjNode *theNode);

typedef struct
{
	Byte		mode;
	Boolean		done;
	float		speed;
	long		frameCounter;
} FaderData;
CheckSpecialDataStruct(FaderData);

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
			FaderData* faderData = GetSpecialData(thisNodePtr, FaderData);
			faderData->mode = fadeIn;									// set new mode
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

	FaderData* faderData = GetSpecialData(newObj, FaderData);
	faderData->mode = fadeIn;
	faderData->done = false;
	faderData->speed = 1.0f / fadeDuration;
	faderData->frameCounter = 0;

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

	FaderData* faderData = GetSpecialData(theNode, FaderData);

	faderData->frameCounter++;

			/* SEE IF FADE IN */

	if (faderData->mode)
	{
		gGammaFadePercent += fps * faderData->speed;
		if (gGammaFadePercent >= 1.0f)										// see if @ 100%
		{
			gGammaFadePercent = 1.0f;
			faderData->done = true;
		}
	}

			/* FADE OUT */
	else
	{
		gGammaFadePercent -= fps * faderData->speed;
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
		{
			gGammaFadePercent = 0;
			faderData->done = true;
		}
	}


	theNode->ColorFilter = (OGLColorRGBA) {0, 0, 0, 1.0f - gGammaFadePercent};

	if (gGameView->fadeSound)
	{
		FadeSound(gGammaFadePercent);
	}

	if (faderData->done)
	{
		if (faderData->mode)			// nuke it if fading in, hold it if fading out
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
	FaderData* faderData = GetSpecialData(fader, FaderData);

	long pFaderFrameCount = faderData->frameCounter;

	while (!faderData->done)
	{
		CalcFramesPerSecond();
		DoSDLMaintenance();

		if (moveCall)
		{
			moveCall();
		}

		// Force fader object to move even if MoveObjects was skipped
		if (faderData->frameCounter == pFaderFrameCount)	// fader wasn't moved by moveCall
		{
			MoveFadeEvent(fader);
			pFaderFrameCount = faderData->frameCounter;
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
	(void) pauseDSp;
}


/************************** EXIT 2D *************************/
//
// For OS X - turn ON DSp when NOT 2D
//

void Exit2D(void)
{
}


/******************** GET DEFAULT WINDOW SIZE *******************/

void GetDefaultWindowSize(SDL_DisplayID display, int* width, int* height)
{
	const float aspectRatio = 16.0 / 9.0f;
	const float screenCoverage = .8f;

	SDL_Rect displayBounds = { .x = 0, .y = 0, .w = 640, .h = 480 };
	SDL_GetDisplayUsableBounds(display, &displayBounds);

	if (displayBounds.w > displayBounds.h)
	{
		*width = displayBounds.h * screenCoverage * aspectRatio;
		*height = displayBounds.h * screenCoverage;
	}
	else
	{
		*width = displayBounds.w * screenCoverage;
		*height = displayBounds.w * screenCoverage / aspectRatio;
	}
}

/********************** GET NUM DISPLAYS **********************/

int GetNumDisplays(void)
{
	int numDisplays = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
	SDL_free(displays);
	return numDisplays;
}

/******************** MOVE WINDOW TO PREFERRED DISPLAY *******************/
//
// This works best in windowed mode.
// Turn off fullscreen before calling this!
//

void MoveToPreferredDisplay(void)
{
	if (gGamePrefs.displayNumMinus1 >= GetNumDisplays())
	{
		gGamePrefs.displayNumMinus1 = 0;
	}

	SDL_DisplayID display = gGamePrefs.displayNumMinus1 + 1;

	int w = 640;
	int h = 480;
	GetDefaultWindowSize(display, &w, &h);
	SDL_SetWindowSize(gSDLWindow, w, h);
	SDL_SyncWindow(gSDLWindow);

	int centered = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
	SDL_SetWindowPosition(gSDLWindow, centered, centered);
	SDL_SyncWindow(gSDLWindow);
}

/*********************** SET FULLSCREEN MODE **********************/

void SetFullscreenMode(bool enforceDisplayPref)
{
	if (!gGamePrefs.fullscreen)
	{
		SDL_SetWindowFullscreen(gSDLWindow, 0);
		SDL_SyncWindow(gSDLWindow);

		if (enforceDisplayPref)
		{
			MoveToPreferredDisplay();
		}
	}
	else
	{
		if (enforceDisplayPref)
		{
			SDL_DisplayID currentDisplay = SDL_GetDisplayForWindow(gSDLWindow);
			SDL_DisplayID desiredDisplay = gGamePrefs.displayNumMinus1 + 1;

			if (currentDisplay != desiredDisplay)
			{
				// We must switch back to windowed mode for the preferred monitor to take effect
				SDL_SetWindowFullscreen(gSDLWindow, false);
				SDL_SyncWindow(gSDLWindow);
				MoveToPreferredDisplay();
			}
		}

		// Enter fullscreen mode
		SDL_SetWindowFullscreen(gSDLWindow, true);
		SDL_SyncWindow(gSDLWindow);
	}

	if (gGamePrefs.fullscreen)
		SDL_HideCursor();
	else
		SDL_ShowCursor();
	//SDL_GL_SetSwapInterval(gGamePrefs.vsync);
}
