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
#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

extern	SDL_Window* 	gSDLWindow;


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DEFAULT_FPS			9

/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

uint32_t 	seed0 = 0, seed1 = 0, seed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;

int		gNumPointers = 0;


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
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	printf("CMR Alert: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Cro-Mag Rally", message, gSDLWindow);

	Exit2D();
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* format, ...)
{
	Enter2D(true);

	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	printf("CMR Fatal Alert: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Cro-Mag Rally", message, NULL);//gSDLWindow);

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


	UseResFile(gMainAppRezFile);

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
Handle	hand;
OSErr	err;

	hand = NewHandle(size);							// alloc in APPL
	if (hand == nil)
	{
		DoAlert("AllocHandle: using temp mem");
		hand = TempNewHandle(size,&err);			// try TEMP mem
		if (hand == nil)
		{
			DoAlert("AllocHandle: failed!");
			return(nil);
		}
		else
			return(hand);
	}
	return(hand);

}



/****************** ALLOC PTR ********************/

void *AllocPtr(long size)
{
Ptr	pr;
uint32_t* cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

	pr = NewPtr(size);						// alloc in Application
	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

	cookiePtr = (uint32_t*) pr;

	*cookiePtr = 'FACE';

	pr += 16;

	gNumPointers++;

	return(pr);
}


/****************** ALLOC PTR CLEAR ********************/

void *AllocPtrClear(long size)
{
Ptr	pr;
uint32_t* cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

	pr = NewPtrClear(size);						// alloc in Application
	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

	cookiePtr = (uint32_t*) pr;

	*cookiePtr = 'FACE';

	pr += 16;

	gNumPointers++;

	return(pr);
}


/***************** SAFE DISPOSE PTR ***********************/

void SafeDisposePtr(Ptr ptr)
{
uint32_t* cookiePtr;

	ptr -= 16;					// back up to pt to cookie

	cookiePtr = (uint32_t*) ptr;

	if (*cookiePtr != 'FACE')
		DoFatalAlert("SafeSafeDisposePtr: invalid cookie!");

	*cookiePtr = 0;

	DisposePtr(ptr);

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
static UnsignedWide time;
UnsignedWide currTime;
unsigned long deltaTime;

	Microseconds(&currTime);
	deltaTime = currTime.lo - time.lo;

	gFramesPerSecond = 1000000.0f / deltaTime;

#if 0
AbsoluteTime currTime,deltaTime;
static AbsoluteTime time = {0,0};
Nanoseconds	nano;

	currTime = UpTime();

	deltaTime = SubAbsoluteFromAbsolute(currTime, time);
	nano = AbsoluteToNanoseconds(deltaTime);

	gFramesPerSecond = 1000000.0f / (float)nano.lo;
	gFramesPerSecond *= 1000.0f;
#endif

	if (gFramesPerSecond < DEFAULT_FPS)			// (avoid divide by 0's later)
		gFramesPerSecond = DEFAULT_FPS;

	if (GetKeyState(SDL_SCANCODE_GRAVE) && GetKeyState(SDL_SCANCODE_KP_PLUS))		// debug speed-up with `+KP_PLUS
		gFramesPerSecond = 10;

	gFramesPerSecondFrac = 1.0f/gFramesPerSecond;		// calc fractional for multiplication

	time = currTime;	// reset for next time interval

	//printf("FPS: %f\n", gFramesPerSecond);
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


/************* SNPRINTF THAT APPENDS TO EXISTING STRING ****************/

size_t snprintfcat(char* buf, size_t bufSize, char const* fmt, ...)
{
	size_t len = strnlen(buf, bufSize);
	int result;
	va_list args;

	va_start(args, fmt);
	result = vsnprintf(buf + len, bufSize - len, fmt, args);
	va_end(args);

	return result;
}
