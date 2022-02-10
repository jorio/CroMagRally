/****************************/
/*        WINDOWS           */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include 	<Folders.h>
#include 	<DrawSprocket.h>
#include 	<stdlib.h>
#include 	<CodeFragments.h>
#include	"globals.h"
#include	"windows.h"
#include	"misc.h"
#include "objects.h"
#include "file.h"
#include "input.h"
#include <TextUtils.h>
#include <FixMath.h>
#include <ToolUtils.h>
#include <Movies.h>
#include <gl.h>
#include <agl.h>
#include "sound2.h"
#include <appearance.h>


extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode	*gCurrentNode,*gFirstNodePtr;
extern	float	gFramesPerSecondFrac;
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;
extern	Boolean				gOSX, gISpActive;
extern	PrefsType			gGamePrefs;
extern	Boolean			gSongPlayingFlag,gMuteMusicFlag;
extern	Movie				gSongMovie;
extern	AGLContext		gAGLContext;
extern	AGLDrawable		gAGLWin;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void PrepDrawSprockets(void);
static void MoveFadeEvent(ObjNode *theNode);
static Boolean SetupEventProc(EventRecord *event);
static pascal Boolean VideoModeTimeOut (DialogRef dp,EventRecord *event, short *item);
static void DoVideoConfirmDialog(void);

static void CreateDisplayModeList(void);
pascal void ModeListCallback(void *userData, DMListIndexType, DMDisplayModeListEntryPtr displaymodeInfo);
static void DoVideoModeSelectPopUpMenu(void);
pascal void ShowVideoMenuSelection (DialogRef dlogPtr, short item);

/****************************/
/*    CONSTANTS             */
/****************************/



#if 0
#define MONITORS_TO_FADE	gDisplayContext
#else
#define MONITORS_TO_FADE	nil
#endif


typedef struct
{
	int		rezH,rezV;
	double	lowestHz;
}VideoModeType;


#define	MAX_VIDEO_MODES		30


/**********************/
/*     VARIABLES      */
/**********************/

static	Boolean	gOldISpFlag;

static	Boolean			gOldMuteMusic;

static GDHandle				gOurDevice = nil;
DisplayIDType		gOurDisplayID;


static short				gNumVideoModes = 0;
static VideoModeType		gVideoModeList[MAX_VIDEO_MODES];

long					gScreenXOffset,gScreenYOffset;
WindowPtr				gCoverWindow = nil;
DSpContextReference 	gDisplayContext = nil;
Boolean					gLoadedDrawSprocket = false;

static unsigned long	gVideoModeTimoutCounter;
static Boolean			gVideoModeTimedOut;


float		gGammaFadePercent;

int				gGameWindowWidth, gGameWindowHeight;

WindowPtr			g1PixelWindow = nil;

CGrafPtr				gDisplayContextGrafPtr = nil;

Boolean				gHasFixedDSpForX = false;

short			g2DStackDepth = 0;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
GDHandle 		phGD;
DisplayIDType displayID;
OSErr		iErr;
Rect			r;
float		w,h;



				/****************************/
				/* INIT WITH DRAW SPROCKETS */
				/****************************/

#if USE_DSP
	PrepDrawSprockets();

	w = gGamePrefs.screenWidth;
	h = gGamePrefs.screenHeight;

	DSpContext_GetDisplayID(gDisplayContext, &displayID);				// get context display ID
	iErr = DMGetGDeviceByDisplayID(displayID, &phGD, true);				// get GDHandle for ID'd device
	if (iErr != noErr)
	{
		DoFatalAlert("InitWindowStuff: DMGetGDeviceByDisplayID failed!");
	}

	r.top  		= (short) ((**phGD).gdRect.top + ((**phGD).gdRect.bottom - (**phGD).gdRect.top) / 2);	  	// h center
	r.top  		-= (short) (h / 2);
	r.left  	= (short) ((**phGD).gdRect.left + ((**phGD).gdRect.right - (**phGD).gdRect.left) / 2);		// v center
	r.left  	-= (short) (w / 2);
	r.right 	= (short) (r.left + w);
	r.bottom 	= (short) (r.top + h);


			/* GET SOME INFO ABOUT THIS GDEVICE */

//	if (OGL_CheckRenderer(phGD, &totalVRAM) == false)
//		DoAlert("No 3D hardware acceleration was found.  Check that the drivers for your 3D accelerator are installed properly");


#else
				/********************/
				/* INIT SOME WINDOW */
				/********************/
	{
		WindowPtr	window;

		SetRect(&r, 100,100,1024,768);

		window = NewCWindow(nil, &r, "", true, plainDBox, (void *)-1, false, nil);
		gDisplayContextGrafPtr = GetWindowPort(window);
	}

#endif


	gGameWindowWidth = r.right - r.left;
	gGameWindowHeight = r.bottom - r.top;
}


/************************** CHANGE WINDOW SCALE ***********************/
//
// Called whenever gGameWindowShrink is changed.  This updates
// the view and everything else to accomodate new window size.
//

void ChangeWindowScale(void)
{
Rect			r;
GDHandle 		phGD;
DisplayIDType displayID;
float		w,h;


	if ((!gLoadedDrawSprocket) || (gCoverWindow == nil))
		return;

	w = gGamePrefs.screenWidth;
	h = gGamePrefs.screenHeight;

			/* CALC NEW INFO */

	DSpContext_GetDisplayID(gDisplayContext, &displayID);			// get context display ID
	DMGetGDeviceByDisplayID(displayID, &phGD, false);				// get GDHandle for ID'd device

	r.top  		= (short) ((**phGD).gdRect.top + ((**phGD).gdRect.bottom - (**phGD).gdRect.top) / 2);	  	// h center
	r.top  		-= (short) (h / 2);
	r.left  	= (short) ((**phGD).gdRect.left + ((**phGD).gdRect.right - (**phGD).gdRect.left) / 2);		// v center
	r.left  	-= (short) (w / 2);
	r.right 	= (short) (r.left + w);
	r.bottom 	= (short) (r.top + h);


	gGameWindowWidth = r.right - r.left;
	gGameWindowHeight = r.bottom - r.top;


			/* MODIFY THE WINODW TO THE NEW PARMS */

	HideWindow(gCoverWindow);
	MoveWindow(gCoverWindow, r.left, r.top, true);
	SizeWindow(gCoverWindow, gGameWindowWidth, gGameWindowHeight, false);
	ShowWindow(gCoverWindow);
}



/*==============================================================================
* Dobold ()
* this is the user item procedure to make the thick outline around the default
* button (assumed to be item 1)
*=============================================================================*/

pascal void DoBold (DialogRef dlogPtr, short item)
{
short		itype;
Handle		ihandle;
Rect		irect;

	item;

	if (!gOSX)
	{

		GetDialogItem (dlogPtr, 1, &itype, &ihandle, &irect);	/* get the buttons rect */
		PenSize (3, 3);											/* make thick lines	*/
		InsetRect (&irect, -4, -4);							/* grow rect a little   */
		FrameRoundRect (&irect, 16, 16);						/* frame the button now */
		PenNormal ();
	}
}

/*==============================================================================
* DoOutline ()
* this is the user item procedure to make the thin outline around the given useritem
*=============================================================================*/

pascal void DoOutline (DialogRef dlogPtr, short item)
{
short		itype;
Handle		ihandle;
Rect		irect;

	GetDialogItem (dlogPtr, item, &itype, (Handle *)&ihandle, &irect);	// get the user item's rect
	FrameRect (&irect);						// frame the button now
	PenNormal();
}


#pragma mark -

/****************** PREP DRAW SPROCKETS *********************/

static void PrepDrawSprockets(void)
{
DSpContextAttributes 	displayConfig;
OSStatus 				theError;
Boolean					confirmIt = false;
const RGBColor			rgbBlack	= { 0x0000, 0x0000, 0x0000 };


		/* startup DrawSprocket */

	theError = DSpStartup();
	if( theError )
	{
		DoFatalAlert("DSpStartup failed!");
	}
	gLoadedDrawSprocket = true;


				/*************************/
				/* SETUP A REQUEST BLOCK */
				/*************************/

try_again:

	displayConfig.frequency					= 0;
	displayConfig.displayWidth				= gGamePrefs.screenWidth;
	displayConfig.displayHeight				= gGamePrefs.screenHeight;
	displayConfig.reserved1					= 0;
	displayConfig.reserved2					= 0;
	displayConfig.colorNeeds				= kDSpColorNeeds_Request;
	displayConfig.colorTable				= NULL;
	displayConfig.contextOptions			= 0;
	displayConfig.backBufferDepthMask		= kDSpDepthMask_1;

	if (gGamePrefs.depth == 32)
		displayConfig.displayDepthMask			= kDSpDepthMask_32;
	else
		displayConfig.displayDepthMask			= kDSpDepthMask_16;

	displayConfig.backBufferBestDepth		= 1;
	displayConfig.displayBestDepth			= gGamePrefs.depth;
	displayConfig.pageCount					= 1;
	displayConfig.filler[0]                 = 0;
	displayConfig.filler[1]                 = 0;
	displayConfig.filler[2]                 = 0;
	displayConfig.gameMustConfirmSwitch		= false;
	displayConfig.reserved3[0]				= 0;
	displayConfig.reserved3[1]				= 0;
	displayConfig.reserved3[2]				= 0;
	displayConfig.reserved3[3]				= 0;



			/* FIND A MATCH */

	theError = DSpFindBestContextOnDisplayID( &displayConfig, &gDisplayContext, gOurDisplayID);
	if (theError)
	{
		gGamePrefs.showScreenModeDialog = true;		// show the settings dialog next time
		SavePrefs();
		DoFatalAlert("Draw Sprocket could not set monitor to desired resolution and/or depth.  Please try again and select a different video mode.");
	}

				/* RESERVE IT */

	theError = DSpContext_Reserve( gDisplayContext, &displayConfig );
	if( theError )
		DoFatalAlert("PrepDrawSprockets: DSpContext_Reserve failed");

	DSpSetBlankingColor(&rgbBlack);


			/* MAKE STATE ACTIVE */

	theError = DSpContext_SetState( gDisplayContext, kDSpContextState_Active );
	if (theError == kDSpConfirmSwitchWarning)
	{
		confirmIt = true;
	}
	else
	if (theError)
	{
		DSpContext_Release( gDisplayContext );
		gDisplayContext = nil;
		DoFatalAlert("PrepDrawSprockets: DSpContext_SetState failed");
		return;
	}

				/* GET GRAFPTR */

	theError = DSpContext_GetFrontBuffer(gDisplayContext, &gDisplayContextGrafPtr);
	if (theError || (gDisplayContextGrafPtr == nil))
		DoFatalAlert("PrepDrawSprockets: DSpContext_GetFrontBuffer failed");

	gGammaFadePercent = 100.0f;
#if ALLOW_FADE
	DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);
#else
	DSpContext_FadeGamma(gDisplayContext,100,nil);
#endif
}



/**************** GAMMA FADE IN *************************/

void GammaFadeIn(void)
{
	if (gDisplayContext)
	{
		while(gGammaFadePercent < 100.0f)
		{
			gGammaFadePercent += 7.0f;
			if (gGammaFadePercent > 100.0f)
				gGammaFadePercent = 100.0f;

#if ALLOW_FADE
			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
#endif

			Wait(1);

		}
	}
}

/**************** GAMMA FADE OUT *************************/

void GammaFadeOut(void)
{
	if (gDisplayContext)
	{
		while(gGammaFadePercent > 0.0f)
		{
			gGammaFadePercent -= 7.0f;
			if (gGammaFadePercent < 0.0f)
				gGammaFadePercent = 0.0f;

#if ALLOW_FADE
			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
#endif

			Wait(1);

		}
	}

}

/********************** GAMMA ON *********************/

void GammaOn(void)
{

	if (gDisplayContext)
	{
		if (gGammaFadePercent != 100.0f)
		{
#if ALLOW_FADE
			DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);
#endif
			gGammaFadePercent = 100.0f;
		}
	}
}


/***************** SETUP EVENT PROC ******************/

static Boolean SetupEventProc(EventRecord *event)
{
	event;
	return(false);
}


/****************** CLEANUP DISPLAY *************************/

void CleanupDisplay(void)
{
OSStatus 		theError;

	if(gDisplayContext != nil)
	{
#if ALLOW_FADE
		GammaFadeOut();
#endif


		DSpContext_SetState( gDisplayContext, kDSpContextState_Inactive );	// deactivate


#if ALLOW_FADE
		DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);						// gamme on all
#endif
		DSpContext_Release( gDisplayContext );								// release

		gDisplayContext = nil;
	}


	/* shutdown draw sprocket */

	if (gLoadedDrawSprocket)
	{
		theError = DSpShutdown();
		gLoadedDrawSprocket = false;
	}

	gDisplayContextGrafPtr = nil;
}

/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean	fadeIn)
{
ObjNode	*newObj;
ObjNode		*thisNodePtr;

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

	gNewObjectDefinition.genre = EVENT_GENRE;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB + 1000;
	gNewObjectDefinition.moveCall = MoveFadeEvent;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Flag[0] = fadeIn;
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF FADE IN */

	if (theNode->Flag[0])
	{
		gGammaFadePercent += 400.0f*fps;
		if (gGammaFadePercent >= 100.0f)										// see if @ 100%
		{
			gGammaFadePercent = 100.0f;
			DeleteObject(theNode);
		}
#if ALLOW_FADE
		if (gDisplayContext)
			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
#endif
	}

			/* FADE OUT */
	else
	{
		gGammaFadePercent -= 400.0f*fps;
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
		{
			gGammaFadePercent = 0;
			DeleteObject(theNode);
		}
#if ALLOW_FADE
		if (gDisplayContext)
			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
#endif
	}
}


/************************ GAME SCREEN TO BLACK ************************/

void GameScreenToBlack(void)
{
Rect	r;

	if (!gDisplayContextGrafPtr)
		return;

	SetPort(gDisplayContextGrafPtr);
	BackColor(blackColor);

	GetPortBounds(gDisplayContextGrafPtr, &r);
	EraseRect(&r);
}


/************************** ENTER 2D *************************/
//
// For OS X - turn off DSp when showing 2D
//

void Enter2D(Boolean pauseDSp)
{
	InitCursor();
	MyFlushEvents();

	g2DStackDepth++;
	if (g2DStackDepth > 1)						// see if already in 2D
		return;

	gOldISpFlag = gISpActive;					// remember if ISp was on
	TurnOffISp();

	if (!gLoadedDrawSprocket)
		return;
	if (!gDisplayContext)
		return;

	if (gAGLContext)
	{
		glFlush();
		glFinish();
		aglSetDrawable(gAGLContext, nil);		// diable gl for dialog
		glFlush();
		glFinish();
	}



	if (pauseDSp)
	{
		DSpContext_SetState(gDisplayContext, kDSpContextState_Paused);
		gDisplayContextGrafPtr = nil;
	}

}


/************************** EXIT 2D *************************/
//
// For OS X - turn ON DSp when NOT 2D
//

void Exit2D(void)
{
AGLContext agl_ctx = gAGLContext;

	g2DStackDepth--;
	if (g2DStackDepth > 0)			// don't exit unless on final exit
		return;

	if (gOldISpFlag)
		TurnOnISp();									// resume input sprockets if needed
	HideCursor();


	if (!gLoadedDrawSprocket)
		return;
	if (!gDisplayContext)
		return;


	ReadKeyboard_Real();			// do this to flush the keymap


	if (gDisplayContextGrafPtr == nil)
	{
		DSpContext_SetState(gDisplayContext, kDSpContextState_Active);
		DSpContext_GetFrontBuffer(gDisplayContext, &gDisplayContextGrafPtr);		// get this again since may have changed
	}

	if (gAGLContext)
	{
		gAGLWin = (AGLDrawable)gDisplayContextGrafPtr;
		if (gAGLWin)
		{
			GLboolean      ok = aglSetDrawable(gAGLContext, gAGLWin);					// reenable gl
			if ((!ok) || (aglGetError() != AGL_NO_ERROR))
				DoFatalAlert("Exit2D: aglSetDrawable failed!");

		}
	}

}


#pragma mark -

/*********************** DUMP GWORLD 2 **********************/
//
//    copies to a destination RECT
//

void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect)
{
PixMapHandle pm;
GDHandle		oldGD;
GWorldPtr		oldGW;
Rect			r;

	DoLockPixels(thisWorld);

	GetGWorld (&oldGW,&oldGD);
	pm = GetGWorldPixMap(thisWorld);
	if ((pm == nil) | (*pm == nil) )
		DoAlert("PixMap Handle or Ptr = Null?!");

	SetPort(GetWindowPort(thisWindow));

	ForeColor(blackColor);
	BackColor(whiteColor);

	GetPortBounds(thisWorld, &r);

	CopyBits((BitMap *)*pm, GetPortBitMapForCopyBits(GetWindowPort(thisWindow)),
			 &r,
			 destRect,
			 srcCopy, 0);

	SetGWorld(oldGW,oldGD);								// restore gworld
}


/******************* DO LOCK PIXELS **************/

void DoLockPixels(GWorldPtr world)
{
PixMapHandle pm;

	pm = GetGWorldPixMap(world);
	if (LockPixels(pm) == false)
		DoFatalAlert("PixMap Went Bye,Bye?!");
}


/********************** WAIT **********************/

void Wait(long ticks)
{
long	start;

	start = TickCount();

	while (TickCount()-start < ticks);

}


#pragma mark -

/********************* DO SCREEN MODE DIALOG *************************/


void DoScreenModeDialog(void)
{
DialogRef 		myDialog;
DialogItemType			itemType,itemHit;
int				i;
ControlHandle	itemHandle;
Rect			itemRect;
Boolean			dialogDone;



try_again:
				/* GET LIST OF VIDEO MODES FOR USER TO CHOOSE FROM */

	CreateDisplayModeList();

#if OEM
	return;	//--------
#endif


	if (gNumVideoModes == 0)						// if none found, then just set default rez and bail
	{
		gVideoModeList[0].rezH = 640;
		gVideoModeList[0].rezV = 480;
		gVideoModeList[0].lowestHz = 0;
		gNumVideoModes = 1;

		gGamePrefs.screenWidth = 800;
		gGamePrefs.screenHeight = 600;
		gGamePrefs.videoHz = 0;
		gGamePrefs.depth = 32;
		return;
	}



				/**********************************************/
				/* DETERMINE IF NEED TO DO THIS DIALOG OR NOT */
				/**********************************************/


	ReadKeyboard_Real();
	ReadKeyboard_Real();
	ReadKeyboard_Real();
	if (GetKeyState_Real(KEY_OPTION) || gGamePrefs.showScreenModeDialog)			// see if force it
		goto do_it;

					/* VERIFY THAT PREFERENCE MODE IS ALLOWED */

	for (i=0; i < gNumVideoModes; i++)
	{
		if ((gVideoModeList[i].rezH == gGamePrefs.screenWidth) &&					// if its a match then bail
			(gVideoModeList[i].rezV == gGamePrefs.screenHeight) &&
			(gVideoModeList[i].lowestHz == gGamePrefs.videoHz))
				return;
	}

				/* BAD MODE, SO RESET TO SOMETHING LEGAL AS DEFAULT */

	gGamePrefs.screenWidth = gVideoModeList[0].rezH;
	gGamePrefs.screenHeight = gVideoModeList[0].rezV;
	gGamePrefs.videoHz = gVideoModeList[0].lowestHz;


					/*****************/
					/* DO THE DIALOG */
					/*****************/
do_it:
	InitCursor();
	myDialog = GetNewDialog(2000+gGamePrefs.language,nil,MOVE_TO_FRONT);


				/* SET CONTROL VALUES */

	GetDialogItem(myDialog,3,&itemType,(Handle *)&itemHandle,&itemRect);		// dont show again
	SetControlValue((ControlHandle)itemHandle,!gGamePrefs.showScreenModeDialog);


	GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);		// depth
	SetControlValue((ControlHandle)itemHandle,gGamePrefs.depth == 16);
	GetDialogItem(myDialog,5,&itemType,(Handle *)&itemHandle,&itemRect);
	SetControlValue((ControlHandle)itemHandle,gGamePrefs.depth == 32);

	GetDialogItem(myDialog,10,&itemType,(Handle *)&itemHandle,&itemRect);		// monitor #
	SetControlValue((ControlHandle)itemHandle,gGamePrefs.monitorNum == 0);
	GetDialogItem(myDialog,11,&itemType,(Handle *)&itemHandle,&itemRect);
	SetControlValue((ControlHandle)itemHandle,gGamePrefs.monitorNum == 1);


			/* SET OUTLINE FOR USERITEMS */

	GetDialogItem(myDialog,1,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItem(myDialog, 2, userItem, (Handle)NewUserItemUPP(DoBold), &itemRect);
	GetDialogItem(myDialog,8,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItem(myDialog, 8, userItem, (Handle)NewUserItemUPP(ShowVideoMenuSelection), &itemRect);

	AutoSizeDialog(myDialog);

				/* DO IT */

	dialogDone = false;
	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case 	1:									// hit ok
					dialogDone = true;
					break;

			case	3:									// dont show again
					gGamePrefs.showScreenModeDialog = !gGamePrefs.showScreenModeDialog;
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,!gGamePrefs.showScreenModeDialog);
					break;

			case	4:
					gGamePrefs.depth = 16;
					GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.depth == 16);
					GetDialogItem(myDialog,5,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.depth == 32);
					break;

			case	5:
					gGamePrefs.depth = 32;
					GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.depth == 16);
					GetDialogItem(myDialog,5,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.depth == 32);
					break;

			case	8:
					DoVideoModeSelectPopUpMenu();
					ShowVideoMenuSelection (myDialog, itemHit);
					break;

			case	10:
					gGamePrefs.monitorNum = 0;
					GetDialogItem(myDialog,10,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.monitorNum == 0);
					GetDialogItem(myDialog,11,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.monitorNum == 1);
					CreateDisplayModeList();
					gGamePrefs.screenWidth = gVideoModeList[0].rezH;
					gGamePrefs.screenHeight = gVideoModeList[0].rezV;
					gGamePrefs.videoHz = gVideoModeList[0].lowestHz;
					ShowVideoMenuSelection (myDialog, 8);
					break;

			case	11:
					gGamePrefs.monitorNum = 1;
					GetDialogItem(myDialog,10,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.monitorNum == 0);
					GetDialogItem(myDialog,11,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.monitorNum == 1);
					CreateDisplayModeList();
					gGamePrefs.screenWidth = gVideoModeList[0].rezH;
					gGamePrefs.screenHeight = gVideoModeList[0].rezV;
					gGamePrefs.videoHz = gVideoModeList[0].lowestHz;
					ShowVideoMenuSelection (myDialog, 8);
					break;

		}
	}
	DisposeDialog(myDialog);


	SavePrefs();

}


Str255	themeS;

/********************* MY THEME BUTTON DRAW PROC *********************/

static pascal void MyThemeButtonDrawProc(const Rect *bounds, ThemeButtonKind kind, ThemeButtonDrawInfo *info,
										UInt32 userData, SInt16 depth, Boolean isColorDev)
{
#pragma unused (kind,info,userData,depth,isColorDev)

int			w,y;
FontInfo	finfo;

	GetFontInfo(&finfo);

	if (gOSX)												// center vertically (for some reason gotta tweak this for 9 or X
		y =  bounds->top + (finfo.ascent + finfo.leading);
	else
		y =  bounds->bottom -2;

	w = StringWidth(themeS) / 2;
	MoveTo((bounds->right + bounds->left) /2 - w, y);		// center it in the rect

	DrawString(themeS);

}

/****************** SHOW VIDEO MODE SELECTION ********************/

pascal void ShowVideoMenuSelection (DialogRef dlogPtr, short item)
{
short		itype;
Handle		ihandle;
Rect		irect;
Str255		t;
GDHandle		oldGD;
GWorldPtr		oldGW;

ThemeButtonDrawInfo	inNewInfo = {kThemeStateActive, kThemeButtonOff,kThemeAdornmentDefault};
ThemeButtonDrawInfo	inPrevInfo = {kThemeStateActive, kThemeButtonOff,kThemeAdornmentDefault};
ThemeButtonDrawUPP	drawUPP;

			/* BUILD MENU ITEM STRING */

	NumToString(gGamePrefs.screenWidth, themeS);
	themeS[themeS[0]+1] = 'x';
	themeS[0]++;
	NumToString(gGamePrefs.screenHeight, t);
	BlockMove(&t[1], &themeS[themeS[0]+1], t[0]);
	themeS[0] += t[0];


			/* DRAW THEME BUTTON POP-UP THEME */

	drawUPP = NewThemeButtonDrawUPP((void *)MyThemeButtonDrawProc);

	GetGWorld (&oldGW,&oldGD);
	SetPort(GetDialogPort(dlogPtr));

	GetDialogItem (dlogPtr, item, &itype, (Handle *)&ihandle, &irect);	// get the rect

//	GetThemeButtonBackgroundBounds (&irect,kThemePopupButton,&inNewInfo,&irect);
	DrawThemeButton(&irect,kThemePopupButton,&inNewInfo,&inPrevInfo,nil,drawUPP,0);

	SetGWorld(oldGW,oldGD);								// restore gworld

	DisposeThemeButtonDrawUPP(drawUPP);
}


/****************** DO VIDEO MODE SELECT POPUP MENU *************************/

static void DoVideoModeSelectPopUpMenu(void)
{
long	menuChoice;
MenuHandle	aMenu;
short	i,theItem;
Point	pt;
Str255	s,t;

	GetMouse(&pt);

	aMenu = NewMenu(100, "Video Modes");

	for (i=0; i < gNumVideoModes; i++)
	{
				/* BUILD MENU ITEM STRING */

		NumToString(gVideoModeList[i].rezH, s);
		s[s[0]+1] = 'x';
		s[0]++;
		NumToString(gVideoModeList[i].rezV, t);
		BlockMove(&t[1], &s[s[0]+1], t[0]);
		s[0] += t[0];
		AppendMenu(aMenu, s);
	}

	InsertMenu(aMenu,hierMenu);

	menuChoice = PopUpMenuSelect(aMenu, pt.v, pt.h, 1);
	theItem	= LoWord(menuChoice);
	if (theItem > 0)
	{
		gGamePrefs.screenWidth = gVideoModeList[theItem-1].rezH;
		gGamePrefs.screenHeight = gVideoModeList[theItem-1].rezV;
		gGamePrefs.videoHz = gVideoModeList[theItem-1].lowestHz;
	}

	DeleteMenu(100);
	DisposeMenu(aMenu);

}



/********************* CREATE DISPLAY MODE LIST *************************/

static void CreateDisplayModeList(void)
{
DMDisplayModeListIteratorUPP	myModeIteratorProc = nil;
DMListIndexType					numModesInList;
DMListType						theDisplayModeList;
int								i;

	gOurDevice = nil;
	gNumVideoModes = 0;

					/* CREATE A CALLBACK PROC */

	myModeIteratorProc = NewDMDisplayModeListIteratorUPP(ModeListCallback);


					/* GET SCREEN DEVICE */

	gOurDevice = DMGetFirstScreenDevice (dmOnlyActiveDisplays);			// get 1st screen device
	for (i = 0; i < gGamePrefs.monitorNum; i++)							// scan for Nth device to use
	{
		GDHandle	nextDevice;

		nextDevice = DMGetNextScreenDevice(gOurDevice, true);
		if (nextDevice)
			gOurDevice = nextDevice;
		else
		{
			gGamePrefs.monitorNum = 0;
			SavePrefs();
			break;
		}
	}

					/*****************************/
					/* EXTRACT INFO ABOUT DEVICE */
					/*****************************/

	if (gOurDevice && myModeIteratorProc)
	{
		if (DMGetDisplayIDByGDevice(gOurDevice, &gOurDisplayID, false) == noErr)								// get Display ID
		{
						/* GET MODE LIST */

			numModesInList = 0;
			if (DMNewDisplayModeList(gOurDisplayID, 0, 0, &numModesInList, &theDisplayModeList) == noErr)		// get mode list for the Display
			{
						/* EXTRACT INFO FOR EACH MODE */

				for (i = 0; i < numModesInList; i++)
					DMGetIndexedDisplayModeFromList(theDisplayModeList, i, 0, myModeIteratorProc, nil);

				DMDisposeList(theDisplayModeList);																// dispose of mode list
			}
		}
	}

			/* CLEANUP */

	if (myModeIteratorProc)
		DisposeDMDisplayModeListIteratorUPP(myModeIteratorProc);


}

/****************** MODE LIST CALLBACK ***********************/

pascal void ModeListCallback(void *userData, DMListIndexType a, DMDisplayModeListEntryPtr displaymodeInfo)
{
double					refreshRate;
u_short					i,depth,sizeH,sizeV,j;
//Boolean				modeOk;
Boolean					skip;
//OSErr					error;
//u_long					switchFlags;
u_long					timingFlags;

#pragma unused (userData, a)

	timingFlags = displaymodeInfo->displayModeTimingInfo->csTimingFlags;
	if	((timingFlags & (1 << kModeShowNow)) || (timingFlags & (1<<kModeSafe)) || (timingFlags & (1<<kModeValid)))
	{
				/* GET REFRESH RATE (Hz) */

		refreshRate = Fix2X(displaymodeInfo->displayModeResolutionInfo->csRefreshRate);


#if 0
				/* VALIDATE THIS MODE */

		error = DMCheckDisplayMode(gOurDevice, displaymodeInfo->displayModeSwitchInfo->csData,
												displaymodeInfo->displayModeSwitchInfo->csMode,
												&switchFlags, 0, &modeOk);
		if ((error == noErr) && modeOk)
		{
			if (!(switchFlags & (1<<kShowModeBit)))			// see if should show it
				return;
		}
		else
			return;
#endif

				/****************************************/
				/* GET RESOLUTIONS & DEPTHS FOR THIS Hz */
				/****************************************/

		for (i = 0; i < displaymodeInfo->displayModeDepthBlockInfo->depthBlockCount; i++)
		{
			depth = displaymodeInfo->displayModeDepthBlockInfo->depthVPBlock[i].depthVPBlock->vpPixelSize;
			sizeH = displaymodeInfo->displayModeDepthBlockInfo->depthVPBlock[i].depthVPBlock->vpBounds.right;
			sizeV = displaymodeInfo->displayModeDepthBlockInfo->depthVPBlock[i].depthVPBlock->vpBounds.bottom;

			if (							//(depth == 32) &&								// skip any modes which are not 32-bit
				(sizeV <= sizeH) &&															// skip vertical oriented modes
				(sizeH <= 1940))															// skip any crazy modes
			{

							/* SEE IF ADD TO MODE LIST */

				skip = false;
				for (j = 0; j < gNumVideoModes; j++)
				{
					if ((gVideoModeList[j].rezH == sizeH) && (gVideoModeList[j].rezV == sizeV))	// see if this rez already in list
					{
						skip = true;
						if (refreshRate < gVideoModeList[j].lowestHz)							// see if this Hz is lower
							gVideoModeList[j].lowestHz = refreshRate;							// update Hz
					}
				}

						/* THIS REZ NOT IN LIST YET, SO ADD */

				if (!skip)
				{
					if (gNumVideoModes < MAX_VIDEO_MODES)
					{
						gVideoModeList[gNumVideoModes].rezH = sizeH;
						gVideoModeList[gNumVideoModes].rezV = sizeV;
						gVideoModeList[gNumVideoModes].lowestHz = refreshRate;
						gNumVideoModes++;
					}
				}
			}
		}
	}
}





