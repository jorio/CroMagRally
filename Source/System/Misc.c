/****************************/
/*      MISC ROUTINES       */
/* (c)1996-2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include "network.h"

extern	SDL_Window* 	gSDLWindow;


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DEFAULT_FPS			9
#define	PTRCOOKIE_SIZE		16

/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

uint32_t 	seed0 = 0, seed1 = 0, seed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;

int		gNumPointers = 0;
long	gRAMAlloced = 0;


/**********************/
/*     PROTOTYPES     */
/**********************/



/*********************** DO ALERT *******************/

void DoAlert(const char* format, ...)
{
	Enter2D(true);

	char message[1024];
	va_list args;
	va_start(args, format);
	SDL_vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Game Alert: %s", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_FULL_NAME, message, NULL);

	Exit2D();
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* format, ...)
{
	Enter2D(true);

	char message[1024];
	va_list args;
	va_start(args, format);
	SDL_vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Game Fatal Alert: %s", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_FULL_NAME, message, NULL);//gSDLWindow);

	Exit2D();
	CleanQuit();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static Boolean	beenHere = false;

	if (!beenHere)
	{
		DeleteAllObjects();

		beenHere = true;

		SavePlayerFile();								// save player if any

		EndNetworkGame();								// remove me from any active network game

		DisposeTerrain();								// dispose of any memory allocated by terrain manager
		DisposeAllBG3DContainers();						// nuke all models
		DisposeCavemanSkins();
		DisposeAllSpriteGroups();						// nuke all sprites
		DisposePillarboxMaterial();

		ShutdownSkeletonManager();

		if (gGameView)							// see if need to dispose this
			OGL_DisposeGameView();

		OGL_Shutdown();									// nuke draw context

		ShutdownSound();								// cleanup sound stuff

		DisposeLocalizedStrings();
	}


	SavePrefs();							// save prefs before bailing

	ExitToShell();
}



#pragma mark -


/******************** MY RANDOM LONG **********************/
//
// My own random number generator that returns a LONG
//
// NOTE: call this instead of MyRandomShort if the value is going to be
//		masked or if it just doesnt matter since this version is quicker
//		without the 0xffff at the end.
//

unsigned long MyRandomLong(void)
{
  return seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+
                   (seed0 = (seed0+1)*3141592621UL))*2435386481UL;
}


/************************* RANDOM RANGE *************************/
//
// THE RANGE *IS* INCLUSIVE OF MIN AND MAX
//

uint16_t	RandomRange(unsigned short min, unsigned short max)
{
uint16_t		qdRdm;											// treat return value as 0-65536
uint32_t		range, t;

	qdRdm = MyRandomLong();
	range = max+1 - min;
	t = (qdRdm * range)>>16;	 							// now 0 <= t <= range

	return( t+min );
}



/************** RANDOM FLOAT ********************/
//
// returns a random float between 0 and 1
//

float RandomFloat(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (1.0f/(float)0xfff);			// get # between 0..1
	return(f);
}


/************** RANDOM FLOAT 2 ********************/
//
// returns a random float between -1 and 1
//

float RandomFloat2(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (2.0f/(float)0xfff);			// get # between 0..2
	f -= 1.0f;								// get -1..+1
	return(f);
}



/**************** SET MY RANDOM SEED *******************/

void SetMyRandomSeed(unsigned long seed)
{
	seed0 = seed;
	seed1 = 0;
	seed2 = 0;

}

/**************** INIT MY RANDOM SEED *******************/

void InitMyRandomSeed(void)
{
	seed0 = 0x2a80ce30;
	seed1 = 0;
	seed2 = 0;
}


/**************** POSITIVE MODULO *******************/

int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int) m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}

#pragma mark -

/****************** ALLOC HANDLE ********************/

Handle	AllocHandle(long size)
{
	Handle hand = NewHandle(size);							// alloc in APPL
	GAME_ASSERT(hand);
	return(hand);
}


/****************** ALLOC PTR ********************/

void *AllocPtr(long size)
{
	GAME_ASSERT(size >= 0);
	GAME_ASSERT(size <= 0x7FFFFFFF);

	size += PTRCOOKIE_SIZE;						// make room for our cookie & whatever else (also keep to 16-byte alignment!)
	Ptr p = SDL_malloc(size);
	GAME_ASSERT(p);

	uint32_t* cookiePtr = (uint32_t *)p;
	cookiePtr[0] = 'FACE';
	cookiePtr[1] = (uint32_t) size;
	cookiePtr[2] = 'PTR3';
	cookiePtr[3] = 'PTR4';

	gNumPointers++;
	gRAMAlloced += size;

	return p + PTRCOOKIE_SIZE;
}


/****************** ALLOC PTR CLEAR ********************/

void *AllocPtrClear(long size)
{
	GAME_ASSERT(size >= 0);
	GAME_ASSERT(size <= 0x7FFFFFFF);

	size += PTRCOOKIE_SIZE;						// make room for our cookie & whatever else (also keep to 16-byte alignment!)
	Ptr p = SDL_calloc(1, size);
	GAME_ASSERT(p);

	uint32_t* cookiePtr = (uint32_t *)p;
	cookiePtr[0] = 'FACE';
	cookiePtr[1] = (uint32_t) size;
	cookiePtr[2] = 'PTC3';
	cookiePtr[3] = 'PTC4';

	gNumPointers++;
	gRAMAlloced += size;

	return p + PTRCOOKIE_SIZE;
}


/****************** REALLOC PTR ********************/

void* ReallocPtr(void* initialPtr, long newSize)
{
	GAME_ASSERT(newSize >= 0);
	GAME_ASSERT(newSize <= 0x7FFFFFFF);

	if (initialPtr == NULL)
	{
		return AllocPtr(newSize);
	}

	Ptr p = ((Ptr)initialPtr) - PTRCOOKIE_SIZE;	// back up pointer to cookie
	newSize += PTRCOOKIE_SIZE;					// make room for our cookie & whatever else (also keep to 16-byte alignment!)

	p = SDL_realloc(p, newSize);				// reallocate it
	GAME_ASSERT(p);

	uint32_t* cookiePtr = (uint32_t *)p;
	GAME_ASSERT(cookiePtr[0] == 'FACE');		// realloc shouldn't have touched our cookie

	uint32_t initialSize = cookiePtr[1];		// update heap size metric
	gRAMAlloced += newSize - initialSize;

	cookiePtr[0] = 'FACE';						// rewrite cookie
	cookiePtr[1] = (uint32_t) newSize;
	cookiePtr[2] = 'REA3';
	cookiePtr[3] = 'REA4';

	return p + PTRCOOKIE_SIZE;
}


/***************** SAFE DISPOSE PTR ***********************/

void SafeDisposePtr(void *ptr)
{
	if (ptr == NULL)
	{
		return;
	}

	Ptr p = ((Ptr)ptr) - PTRCOOKIE_SIZE;			// back up to pt to cookie

	uint32_t* cookiePtr = (uint32_t *)p;
	GAME_ASSERT(cookiePtr[0] == 'FACE');
	gRAMAlloced -= cookiePtr[1];					// deduct ptr size from heap size

	cookiePtr[0] = 'DEAD';							// zap cookie

	SDL_free(p);

	gNumPointers--;
}


#pragma mark -

/******************* VERIFY SYSTEM ******************/

void InitPrefsFolder(bool createIt)
{
OSErr iErr;
long createdDirID;

			/* CHECK PREFERENCES FOLDER */

	iErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,		// locate the folder
					&gPrefsFolderVRefNum,&gPrefsFolderDirID);
	if (iErr != noErr)
		DoAlert("Warning: Cannot locate the Preferences folder.");

	if (createIt)
	{
		iErr = DirCreate(gPrefsFolderVRefNum, gPrefsFolderDirID, PREFS_FOLDER_NAME, &createdDirID);		// make folder in there
	}
}



#pragma mark -



/************** CALC FRAMES PER SECOND *****************/
//
// This version uses UpTime() which is only available on PCI Macs.
//

void CalcFramesPerSecond(void)
{
	static uint64_t performanceFrequency = 0;
	static uint64_t prevTime = 0;
	uint64_t currTime;

	static float minFps = 10;
	static float maxFps = 60;



	if (performanceFrequency == 0)
	{
		performanceFrequency = SDL_GetPerformanceFrequency();
	}

slow_down:
	currTime = SDL_GetPerformanceCounter();
	uint64_t deltaTime = currTime - prevTime;

	if (deltaTime <= 0)
	{
		gFramesPerSecond = minFps;						// avoid divide by 0
	}
	else
	{
		gFramesPerSecond = performanceFrequency / (float)(deltaTime);

		if (gFramesPerSecond > maxFps)					// keep from going over 100fps (there were problems in 2.0 of frame rate precision loss)
		{
			if (gFramesPerSecond - maxFps > 1000)		// try to sneak in some sleep if we have 1 ms to spare
			{
				SDL_Delay(1);
			}
			goto slow_down;
		}

		if (gFramesPerSecond < minFps)					// (avoid divide by 0's later)
		{
			gFramesPerSecond = minFps;
		}
	}

#if _DEBUG
	// Fast-forward with grave key
	if (GetKeyState(SDL_SCANCODE_GRAVE))
	{
		gFramesPerSecond = minFps;
	}

	// Toggle between 60 and 300 fps with tab key
	if (GetNewKeyState(SDL_SCANCODE_TAB))
	{
		maxFps = maxFps > 60 ? 60 : 300;
	}
#endif

	gFramesPerSecondFrac = 1.0f / gFramesPerSecond;		// calc fractional for multiplication

	prevTime = currTime;								// reset for next time interval
}


/********************* IS POWER OF 2 ****************************/

Boolean IsPowerOf2(int num)
{
int		i;

	i = 2;
	do
	{
		if (i == num)				// see if this power of 2 matches
			return(true);
		i *= 2;						// next power of 2
	}while(i <= num);				// search until power is > number

	return(false);
}


#pragma mark -

/******************* MY FLUSH EVENTS **********************/
//
// This call makes damed sure that there are no events anywhere in any event queue.
//

void MyFlushEvents(void)
{
#if 0 //IJ
EventRecord 	theEvent;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());

			/* POLL EVENT QUEUE TO BE SURE THINGS ARE FLUSHED OUT */

	while (GetNextEvent(mDownMask|mUpMask|keyDownMask|keyUpMask|autoKeyMask, &theEvent));


	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
#endif
}


/************* SDL_snprintf THAT APPENDS TO EXISTING STRING ****************/

size_t snprintfcat(char* buf, size_t bufSize, char const* fmt, ...)
{
	size_t len = SDL_strnlen(buf, bufSize);
	int result;
	va_list args;

	va_start(args, fmt);
	result = SDL_vsnprintf(buf + len, bufSize - len, fmt, args);
	va_end(args);

	return result;
}
