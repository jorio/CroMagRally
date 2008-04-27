/****************************/
/*      MISC ROUTINES       */
/* (c)1996-2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include <Gestalt.h>
#include <TextUtils.h>
#include <math.h>
#include <sound.h>
#include <folders.h>
#include <palettes.h>
#include <osutils.h>
#include <timer.h>
#include 	<DrawSprocket.h>
#include <InputSprocket.h>
#include <DriverServices.h>

#include	"globals.h"
#include	"misc.h"
#include	"windows.h"
#include "file.h"
#include "objects.h"
#include "input.h"
#include "skeletonobj.h"
#include "terrain.h"
#include "bg3d.h"
#include "sprites.h"
#include "network.h"
#include "sound2.h"
#include "miscscreens.h"

extern	unsigned long gOriginalSystemVolume;
extern	short		gMainAppRezFile;
extern	Boolean		gISPInitialized,gOSX;
extern	Boolean		gISpActive,gHasFixedDSpForX;
extern	DSpContextReference 	gDisplayContext;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int			gPolysThisFrame;
extern	AGLContext		gAGLContext;
extern	AGLDrawable		gAGLWin;
extern	float			gDemoVersionTimer;


/****************************/
/*    CONSTANTS             */
/****************************/





#define		ERROR_ALERT_ID		401

#define	DEFAULT_FPS			9

/**********************/
/*     VARIABLES      */
/**********************/

Boolean	gQuitting = false;

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

u_long 	seed0 = 0, seed1 = 0, seed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;

int		gNumPointers = 0;

Boolean	gLowMemMode = false;

Boolean     gGameIsRegistered = false;

unsigned char	gRegInfo[64];

Str255  gRegFileName = "\p:CroMag:Info";


/**********************/
/*     PROTOTYPES     */
/**********************/

static void DoRegistrationDialog(unsigned char *out);
static Boolean ValidateRegistrationNumber(unsigned char *regInfo);



/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;

	Enter2D(true);

	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	TurnOffISp();										// MUST TURN OFF INPUT SPROK TO GET KEYBOARD EVENTS!!!
	UseResFile(gMainAppRezFile);
	NumToString(err, numStr);
	DoAlert (numStr);

//	DebugStr("\pShowSystemErr has been called");

	CleanQuit();
}

/****************** DO SYSTEM ERROR : NONFATAL ***************/
//
// nonfatal
//
void ShowSystemErr_NonFatal(long err)
{
Str255		numStr;

	Enter2D(true);

	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	NumToString(err, numStr);
	DoAlert (numStr);

	Exit2D();

}


/*********************** DO ALERT *******************/

void DoAlert(Str255 s)
{
Boolean	oldISpFlag = gISpActive;

	if (gDisplayContext)
		GammaOn();

	Enter2D(true);

	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	TurnOffISp();										// MUST TURN OFF INPUT SPROK TO GET KEYBOARD EVENTS!!!
	UseResFile(gMainAppRezFile);
	InitCursor();
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	NoteAlert(ERROR_ALERT_ID,nil);

//	DebugStr("\pDoAlert has been called");

	if (oldISpFlag)
		TurnOnISp();									// resume input sprockets if needed

	Exit2D();


	HideCursor();
}


/*********************** DO ALERT NUM *******************/

void DoAlertNum(int n)
{
Boolean	oldISpFlag = gISpActive;

	if (gDisplayContext)
		GammaOn();

	Enter2D(true);

	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	TurnOffISp();										// MUST TURN OFF INPUT SPROK TO GET KEYBOARD EVENTS!!!
	InitCursor();
	NoteAlert(n,nil);

	if (oldISpFlag)
		TurnOnISp();									// resume input sprockets if needed

	Exit2D();

}



/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(Str255 s)
{
OSErr	iErr;

	if (gDisplayContext)
		GammaOn();

	Enter2D(true);

	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
	UseResFile(gMainAppRezFile);
	TurnOffISp();										// MUST TURN OFF INPUT SPROK TO GET KEYBOARD EVENTS!!!

	InitCursor();
	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	iErr = NoteAlert(ERROR_ALERT_ID,nil);

//	DebugStr("\pDoFatalAlert has been called");

	CleanQuit();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static Boolean	beenHere = false;

	gQuitting = true;

	if (!beenHere)
	{

#if DEMO || SHAREWARE
		DeleteAllObjects();
#endif

		beenHere = true;

		SavePlayerFile();								// save player if any

		EndNetworkGame();								// remove me from any active network game

		DisposeTerrain();								// dispose of any memory allocated by terrain manager
		DisposeAllBG3DContainers();						// nuke all models
		DisposeAllSpriteGroups();						// nuke all sprites


		if (gGameViewInfoPtr)							// see if need to dispose this
			OGL_DisposeWindowSetup(&gGameViewInfoPtr);


#if DEMO
		ShowDemoQuitScreen();
#elif SHAREWARE
		if (!gGameIsRegistered)
			DoSharewareExitScreen();
#endif

		ShutdownSound();								// cleanup sound stuff
	}


	GameScreenToBlack();
	CleanupDisplay();								// unloads Draw Sprocket
    TurnOnHighQualityRateConverter ();

	if (gISPInitialized)							// unload ISp
		ISpShutdown();

	UseResFile(gMainAppRezFile);

	InitCursor();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());


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

u_short	RandomRange(unsigned short min, unsigned short max)
{
u_short		qdRdm;											// treat return value as 0-65536
u_long		range, t;

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
// returns a random float between 0 and 1
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


#pragma mark -


/******************* FLOAT TO STRING *******************/

void FloatToString(float num, Str255 string)
{
Str255	sf;
long	i,f;

	i = num;						// get integer part


	f = (fabs(num)-fabs((float)i)) * 10000;		// reduce num to fraction only & move decimal --> 5 places

	if ((i==0) && (num < 0))		// special case if (-), but integer is 0
	{
		string[0] = 2;
		string[1] = '-';
		string[2] = '0';
	}
	else
		NumToString(i,string);		// make integer into string

	NumToString(f,sf);				// make fraction into string

	string[++string[0]] = '.';		// add "." into string

	if (f >= 1)
	{
		if (f < 1000)
			string[++string[0]] = '0';	// add 1000's zero
		if (f < 100)
			string[++string[0]] = '0';	// add 100's zero
		if (f < 10)
			string[++string[0]] = '0';	// add 10's zero
	}

	for (i = 0; i < sf[0]; i++)
	{
		string[++string[0]] = sf[i+1];	// copy fraction into string
	}
}

/*********************** STRING TO FLOAT *************************/

float StringToFloat(Str255 textStr)
{
short	i;
short	length;
Byte	mode = 0;
long	integer = 0;
long	mantissa = 0,mantissaSize = 0;
float	f;
float	tens[8] = {1,10,100,1000,10000,100000,1000000,10000000};
char	c;
float	sign = 1;												// assume positive

	length = textStr[0];										// get string length

	if (length== 0)												// quick check for empty
		return(0);


			/* SCAN THE NUMBER */

	for (i = 1; i <= length; i++)
	{
		c  = textStr[i];										// get this char

		if (c == '-')											// see if negative
		{
			sign = -1;
			continue;
		}
		else
		if (c == '.')											// see if hit the decimal
		{
			mode = 1;
			continue;
		}
		else
		if ((c < '0') || (c > '9'))								// skip all but #'s
			continue;


		if (mode == 0)
			integer = (integer * 10) + (c - '0');
		else
		{
			mantissa = (mantissa * 10) + (c - '0');
			mantissaSize++;
		}
	}

			/* BUILT A FLOAT FROM IT */

	f = (float)integer + ((float)mantissa/tens[mantissaSize]);
	f *= sign;

	return(f);
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
		DoAlert("\pAllocHandle: using temp mem");
		hand = TempNewHandle(size,&err);			// try TEMP mem
		if (hand == nil)
		{
			DoAlert("\pAllocHandle: failed!");
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
u_long	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

	pr = NewPtr(size);						// alloc in Application
	if (pr == nil)
		DoFatalAlert("\pAllocPtr: NewPtr failed");

	cookiePtr = (u_long *)pr;

	*cookiePtr = 'FACE';

	pr += 16;

	gNumPointers++;

	return(pr);
}


/****************** ALLOC PTR CLEAR ********************/

void *AllocPtrClear(long size)
{
Ptr	pr;
u_long	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

	pr = NewPtrClear(size);						// alloc in Application
	if (pr == nil)
		DoFatalAlert("\pAllocPtr: NewPtr failed");

	cookiePtr = (u_long *)pr;

	*cookiePtr = 'FACE';

	pr += 16;

	gNumPointers++;

	return(pr);
}


/***************** SAFE DISPOSE PTR ***********************/

void SafeDisposePtr(Ptr ptr)
{
u_long	*cookiePtr;

	ptr -= 16;					// back up to pt to cookie

	cookiePtr = (u_long *)ptr;

	if (*cookiePtr != 'FACE')
		DoFatalAlert("\pSafeSafeDisposePtr: invalid cookie!");

	*cookiePtr = 0;

	DisposePtr(ptr);

	gNumPointers--;
}



#pragma mark -

/******************* COPY P STRING ********************/

void CopyPString(Str255 from, Str255 to)
{
short	i,n;

	n = from[0];			// get length

	for (i = 0; i <= n; i++)
		to[i] = from[i];

}


/***************** P STRING TO C ************************/

void PStringToC(char *pString, char *cString)
{
Byte	pLength,i;

	pLength = pString[0];

	for (i=0; i < pLength; i++)					// copy string
		cString[i] = pString[i+1];

	cString[pLength] = 0x00;					// add null character to end of c string
}


/***************** DRAW C STRING ********************/

void DrawCString(char *string)
{
	while(*string != 0x00)
		DrawChar(*string++);
}


/******************* VERIFY SYSTEM ******************/

void VerifySystem(void)
{
OSErr	iErr;
NumVersion				;
long		createdDirID;
NumVersion	vers;


		/* DETERMINE IF RUNNING ON OS X */

	iErr = Gestalt(gestaltSystemVersion,(long *)&vers);
	if (vers.stage >= 0x10)
	{
		gOSX = true;
	}
	else
	{
		gOSX = false;
	}


		/* REQUIRE CARBONLIB 1.0.4 */

	iErr = Gestalt(gestaltCarbonVersion,(long *)&vers);
	if (vers.stage == 1)
	{
		if ((vers.minorAndBugRev == 0) && (vers.nonRelRev < 4))
		{
			DoFatalAlert("\pThis application requires CarbonLib 1.0.4 or newer.  Run Software Update, or download the latest CarbonLib from www.apple.com");
		}
	}



#if 0
			/* CHECK TIME-BOMB */
	{
		unsigned long secs;
		DateTimeRec	d;

		GetDateTime(&secs);
		SecondsToDate(secs, &d);

		if ((d.year > 2000) ||
			((d.year == 2000) && (d.month > 10)))
		{
			DoFatalAlert("\pSorry, but this beta has expired");
		}
	}
#endif


			/* CHECK PREFERENCES FOLDER */

	iErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,		// locate the folder
					&gPrefsFolderVRefNum,&gPrefsFolderDirID);
	if (iErr != noErr)
		DoAlert("\pWarning: Cannot locate the Preferences folder.");

	iErr = DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"\pCroMag",&createdDirID);		// make folder in there


			/* CHECK OPENGL */

	if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) aglChoosePixelFormat) // check for existance of OpenGL
		DoFatalAlert("\pThis application needs OpenGL to function.  Please install OpenGL and try again.");


		/* CHECK SPROCKETS */

	if (!gOSX)
	{
		if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) ISpStartup) 							// check for existance of Input Sprocket
			DoFatalAlert("\pThis application needs Input Sprocket to function.  Please install Game Sprockets and try again.");

		if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) DSpStartup) 							// check for existance of Draw Sprocket
			DoFatalAlert("\pThis application needs Draw Sprocket to function.  Please install Game Sprockets and try again.");

		if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) NSpInitialize) 						// check for existance of Net Sprocket
			DoFatalAlert("\pThis application needs Net Sprocket to function.  Please install Game Sprockets and try again.");
	}
	else
	{
		if ((Ptr)kUnresolvedCFragSymbolAddress == (Ptr)DSpSetWindowToFront) 		// check for special OS X DSp hacks
			gHasFixedDSpForX = false;
		else
			gHasFixedDSpForX = true;

		if (OSX_PACKAGE)
		{
			if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) NSpInitialize) 						// check for existance of Net Sprocket
				DoFatalAlert("\pThis application needs Net Sprocket to function.  Please install the Net Sprocket libraries in your Cro-Mag Rally folder.");
		}
	}


		/* CHECK MEMORY */
	{
		u_long	mem;
		iErr = Gestalt(gestaltPhysicalRAMSize,(long *)&mem);
		if (iErr == noErr)
		{
					/* CHECK FOR LOW-MEMORY SITUATIONS */

			mem /= 1024;
			mem /= 1024;
			if (mem <= 64)						// see if have only 64 MB of real RAM or less installed
			{
				u_long	vmAttr;

						/* MUST HAVE VM ON */

				Gestalt(gestaltVMAttr,(long *)&vmAttr);	// get VM attribs to see if its ON
				if (!(vmAttr & (1 << gestaltVMPresent)))
				{
					DoFatalAlert("\pPlease turn on Virtual Memory, reboot your computer, and try again");
				}
				gLowMemMode = true;
			}
		}
	}
}


/******************** REGULATE SPEED ***************/

void RegulateSpeed(short fps)
{
short	n;
static oldTick = 0;

	n = 60 / fps;
	while ((TickCount() - oldTick) < n) {}			// wait for n ticks
	oldTick = TickCount();							// remember current time
}


/************* COPY PSTR **********************/

void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr)
{
short	dataLen = inSourceStr[0] + 1;

	BlockMoveData(inSourceStr, outDestStr, dataLen);
	outDestStr[0] = dataLen - 1;
}





#pragma mark -



/************** CALC FRAMES PER SECOND *****************/
//
// This version uses UpTime() which is only available on PCI Macs.
//

void CalcFramesPerSecond(void)
{
AbsoluteTime currTime,deltaTime;
static AbsoluteTime time = {0,0};
Nanoseconds	nano;

	currTime = UpTime();

	deltaTime = SubAbsoluteFromAbsolute(currTime, time);
	nano = AbsoluteToNanoseconds(deltaTime);

	gFramesPerSecond = 1000000.0f / (float)nano.lo;
	gFramesPerSecond *= 1000.0f;

	if (gFramesPerSecond < DEFAULT_FPS)			// (avoid divide by 0's later)
		gFramesPerSecond = DEFAULT_FPS;
	gFramesPerSecondFrac = 1.0f/gFramesPerSecond;		// calc fractional for multiplication


	time = currTime;	// reset for next time interval
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
EventRecord 	theEvent;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());

			/* POLL EVENT QUEUE TO BE SURE THINGS ARE FLUSHED OUT */

	while (GetNextEvent(mDownMask|mUpMask|keyDownMask|keyUpMask|autoKeyMask, &theEvent));


	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
}


#pragma mark -


/********************** CHECK GAME REGISTRATION *************************/

void CheckGameRegistration(void)
{
OSErr   iErr;
FSSpec  spec;
short		fRefNum;
long        	numBytes = REG_LENGTH;

            /* GET SPEC TO REG FILE */

	iErr = FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gRegFileName, &spec);
    if (iErr)
        goto game_not_registered;


            /*************************/
            /* VALIDATE THE REG FILE */
            /*************************/

            /* READ REG DATA */

    if (FSpOpenDF(&spec,fsCurPerm,&fRefNum) != noErr)
        goto game_not_registered;

	FSRead(fRefNum,&numBytes,gRegInfo);

    FSClose(fRefNum);

            /* VALIDATE IT */

    if (!ValidateRegistrationNumber(&gRegInfo[0]))
        goto game_not_registered;

    gGameIsRegistered = true;
    return;

        /* GAME IS NOT REGISTERED YET, SO DO DIALOG */

game_not_registered:

    DoRegistrationDialog(&gRegInfo[0]);

    if (gGameIsRegistered)                                  // see if write out reg file
    {
	    FSpDelete(&spec);	                                // delete existing file if any
	    iErr = FSpCreate(&spec,kGameID,'xxxx',-1);
        if (iErr == noErr)
        {
        	numBytes = REG_LENGTH;
			FSpOpenDF(&spec,fsCurPerm,&fRefNum);
			FSWrite(fRefNum,&numBytes,gRegInfo);
		    FSClose(fRefNum);
     	}
    }

            /* DEMO MODE */
    else
    {
		/* SEE IF TIMER HAS EXPIRED */

        GetDemoTimer();
    	if (gDemoVersionTimer > (60 * 90))		// let play for n minutes
    	{
			DoAlertNum(136);
    		ExitToShell();
    	}
    }

}


/********************* VALIDATE REGISTRATION NUMBER ******************/

static Boolean ValidateRegistrationNumber(unsigned char *regInfo)
{
Handle	hand;
int	   i,j, c;
unsigned char pirateNumbers[17][REG_LENGTH] =
{
	"AAAAAAMMMMMM",
	"CENDRMHCQGPR",
	"AJADGEIGJMDM",
	"AKAJHMAFDMCM",
	"MMMMMMAAAAAA",

	"MMMMMMHHHHHH",
	"HHHHHHMMMMMM",
	"MHMHMHMHMHMH",
	"HMHMHMHMHMHM",
	"AMAMAMAMAMAM",

	"MAMAMAMAMAMA",
	"ALADGIEGJMBM",
	"AAMMAAMMAAMM",
	"MMAAMMAAMMAA",
	"AHBIAIEMELFM",

	"AJBLAIEMBLDM",
	"ADBFAHFMHLJM",
};

			/*************************/
            /* VALIDATE ENTERED CODE */
            /*************************/

    for (i = 0, j = REG_LENGTH-1; i < REG_LENGTH; i += 2, j -= 2)     // get checksum
    {
        Byte    value,c,d;

		if ((regInfo[i] >= 'a') && (regInfo[i] <= 'z'))	// convert to upper case
			regInfo[i] = 'A' + (regInfo[i] - 'a');

		if ((regInfo[j] >= 'a') && (regInfo[j] <= 'z'))	// convert to upper case
			regInfo[j] = 'A' + (regInfo[j] - 'a');

        value = regInfo[i] - 'A';           // convert letter to digit 0..9
        c = ('M' - regInfo[j]);             // convert character to number

        d = c - value;                      // the difference should be == i

        if (d != 0)
            return(false);
    }

			/**********************************/
			/* CHECK FOR KNOWN PIRATE NUMBERS */
			/**********************************/

	for (j = 0; j < 17; j++)
	{
		for (i = 0; i < REG_LENGTH; i++)
		{
			if (regInfo[i] != pirateNumbers[j][i])					// see if doesn't match
				goto next_code;
		}

				/* THIS CODE IS PIRATED */

		return(false);

next_code:;
	}


			/*******************************/
			/* SECONDARY CHECK IN REZ FILE */
			/*******************************/

	c = CountResources('BseR');						// count how many we've got stored
	for (j = 0; j < c; j++)
	{
		hand = GetResource('BseR',128+j);			// read the #

		for (i = 0; i < REG_LENGTH; i++)
		{
			if (regInfo[i] != (*hand)[i])			// see if doesn't match
				goto next2;
		}

				/* THIS CODE IS PIRATED */

		return(false);

next2:
		ReleaseResource(hand);
	}


    return(true);
}



/****************** DO REGISTRATION DIALOG *************************/

static void DoRegistrationDialog(unsigned char *out)
{
DialogPtr 		myDialog;
Boolean			dialogDone = false, isValid;
short			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;

	TurnOffISp();
	InitCursor();

	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	myDialog = GetNewDialog(128,nil,MOVE_TO_FRONT);

				/* DO IT */

	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case	1:									        // Register
					GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,gRegInfo);
                    BlockMove(&gRegInfo[1], &gRegInfo[0], REG_LENGTH);         // shift out length byte

                    isValid = ValidateRegistrationNumber(gRegInfo);    // validate the number

                    if (isValid == true)
                    {
                        gGameIsRegistered = true;
                        dialogDone = true;
                        BlockMove(gRegInfo, out, REG_LENGTH);		// copy to output
                    }
                    else
                    {
                        DoAlert("\pSorry, that registration code is not valid.  Please try again.");
						InitCursor();
                    }
					break;

            case    2:                                  // Demo
                    dialogDone = true;
                    break;

			case 	3:									// QUIT
                    ExitToShell();
					break;
		}
	}
	DisposeDialog(myDialog);
	HideCursor();
	TurnOnISp();
}







