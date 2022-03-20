/****************************/
/*   	  INPUT.C	   	    */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include <SDL.h>
#include <stdlib.h>
#include "globals.h"
#include "misc.h"
#include "input.h"
#include "window.h"
#include "ogl_support.h"
#include "miscscreens.h"
#include "main.h"
#include "player.h"
#include "file.h"

extern	short				gMainAppRezFile,gMyNetworkPlayerNum,gNumLocalPlayers;
extern	Byte				gActiveSplitScreenMode;
extern	PlayerInfoType		gPlayerInfo[];
extern	Boolean				gOSX,gQuitting;
extern	float			gFramesPerSecondFrac;
extern	PrefsType			gGamePrefs;

/**********************/
/*     PROTOTYPES     */
/**********************/

static void MyGetKeys(KeyMap *keyMap);
static void GetLocalKeyStateForPlayer(short playerNum, Boolean secondaryControls);

static void DoMyKeyboardEdit(void);
static void SetKeyControl(short item, unsigned short keyCode);
static void SetMyKeyEditDefaults(void);
static short GetFirstRealKeyPressed(void);
static void KeyCodeToChar(UInt16 code, Str32 s);


/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/


float	gAnalogSteeringTimer[MAX_PLAYERS] = {0,0,0,0,0,0};


		/* CONTORL NEEDS */


#if 0
static const u_short	gNeedToKey[NUM_CONTROL_NEEDS] =				// table to convert need # into key equate value
{
	(kKey_Left_P1<<8) | kKey_Right_P1,	// steer p1
	kKey_Forward_P1,
	kKey_Backward_P1,
	kKey_Brakes_P1,
	kKey_ThrowForward_P1,
	kKey_ThrowBackward_P1,
	kKey_CameraMode_P1,

	(kKey_Left_P2<<8) | kKey_Right_P2,	// steer p2
	kKey_Forward_P2,
	kKey_Backward_P2,
	kKey_Brakes_P2,
	kKey_ThrowForward_P2,
	kKey_ThrowBackward_P2,
	kKey_CameraMode_P2
};

static u_short	gUserKeySettings[NUM_CONTROL_NEEDS];

const u_short	gUserKeySettings_Defaults[NUM_CONTROL_NEEDS] =
{
	(kKey_Left_P1<<8) | kKey_Right_P1,	// steer p1
	kKey_Forward_P1,
	kKey_Backward_P1,
	kKey_Brakes_P1,
	kKey_ThrowForward_P1,
	kKey_ThrowBackward_P1,
	kKey_CameraMode_P1,

	(kKey_Left_P2<<8) | kKey_Right_P2,	// steer p2
	kKey_Forward_P2,
	kKey_Backward_P2,
	kKey_Brakes_P2,
	kKey_ThrowForward_P2,
	kKey_ThrowBackward_P2,
	kKey_CameraMode_P2
};



/**********************************************************************/


static const int gControlBitToKey[NUM_CONTROL_BITS][2] = {						// remember to update kControlBit_XXX list!!!
														kKey_ThrowForward_P1, 	kKey_ThrowForward_P2,
														kKey_ThrowBackward_P1, 	kKey_ThrowBackward_P2,
														kKey_Brakes_P1, 		kKey_Brakes_P2,
														kKey_CameraMode_P1, 	kKey_CameraMode_P2,
														kKey_Forward_P1,		kKey_Forward_P2,
														kKey_Backward_P1,		kKey_Backward_P2
													};

#endif



/************************* INIT INPUT *********************************/

void InitInput(void)
{
#if 0
OSErr				iErr;
UInt32				count = 0;
int			i;

			/********************************************/
			/* SEE IF USE INPUT SPROCKET OR HID MANAGER */
			/********************************************/

				/* OS X */

		for (i = 0; i < NUM_CONTROL_NEEDS; i++)			// copy key settings from prefs
			gUserKeySettings[i] = gGamePrefs.keySettings[i];
#endif
}

/**************** READ KEYBOARD *************/
//
//
//

void ReadKeyboard(void)
{


			/* READ REAL KEYBOARD & INPUT SPROCKET KEYMAP */

	DoSDLMaintenance();



#if 0
				/* SEE IF QUIT GAME */

	if (!gQuitting)
	{
		if (GetKeyState_Real(KEY_Q) && GetKeyState_Real(KEY_APPLE))			// see if real key quit
			CleanQuit();
	}



			/* SEE IF DO SAFE PAUSE FOR SCREEN SHOTS */

	if (GetNewKeyState_Real(KEY_F12))
	{
		IMPLEMENT_ME_SOFT();
		Boolean o = gISpActive;
		TurnOffISp();

		if (gAGLContext)
			aglSetDrawable(gAGLContext, nil);			// diable gl

		do
		{
			EventRecord	e;
			WaitNextEvent(everyEvent,&e, 0, 0);
			ReadKeyboard_Real();
		}while(!GetNewKeyState_Real(KEY_F12));
		if (o)
			TurnOnISp();

		if (gAGLContext)
			aglSetDrawable(gAGLContext, gAGLWin);		// reenable gl

		CalcFramesPerSecond();
		CalcFramesPerSecond();
	}
#endif
}

#if 0
/********************** MY GET KEYS ******************************/
//
// Depending on mode, will either read key map from GetKeys or
// will "fake" a keymap using Input Sprockets.
//

static void MyGetKeys(KeyMap *keyMap)
{
short	i,key,j,q,p;
UInt32	keyState,axisState;
unsigned char *keyBytes;

	ReadKeyboard_Real();												// always read real keyboard anyway

	keyBytes = (unsigned char *)keyMap;
	(*keyMap)[0] = (*keyMap)[1] = (*keyMap)[2] = (*keyMap)[3] = 0;		// clear out keymap

				/********************/
				/* DO HACK FOR OS X */
				/********************/

	//if (!gISpActive)
	{
		for (i = 0; i < NUM_CONTROL_NEEDS; i++)
		{
			switch(i)
			{
								/* EMULATE THE ANALOG INPUT FOR STEERING */
				case	0:

						if (GetKeyState_Real((gUserKeySettings[0] & 0xff00) >> 8))
							gPlayerInfo[0].analogSteering = -1.0f;
						else
						if (GetKeyState_Real(gUserKeySettings[0] & 0xff))
							gPlayerInfo[0].analogSteering = 1.0f;
						else
							gPlayerInfo[0].analogSteering = 0.0f;
						break;

				case	7:
						if (GetKeyState_Real((gUserKeySettings[7] & 0xff00) >> 8))
							gPlayerInfo[1].analogSteering = -1.0f;
						else
						if (GetKeyState_Real(gUserKeySettings[7] & 0xff))
							gPlayerInfo[1].analogSteering = 1.0f;
						else
							gPlayerInfo[1].analogSteering = 0.0f;
						break;


							/* HANDLE OTHER KEYS */

				default:
						key = gUserKeySettings[i];
						if (GetKeyState_Real(key))
						{
							j = key>>3;
							q = (1<<(key&7));
							keyBytes[j] |= q;											// set correct bit in keymap
						}
			}
		}
	}
	else
	{

			/***********************************/
			/* POLL NEEDS FROM INPUT SPROCKETS */
			/***********************************/
			//
			// Convert input sprocket info into keymap bits to simulate a real key being pressed.
			//

		for (i = 0; i < NUM_CONTROL_NEEDS; i++)
		{
			switch(gControlNeeds[i].theKind)
			{
					/* AXIS  */

				case	kISpElementKind_Axis:
						ISpElement_GetSimpleState(gVirtualElements[i],&axisState);		// get state of this one

						if (gNumLocalPlayers > 1)										// if 2-player split screen then extract player # from control
							p = gControlNeeds[i].playerNum - 1;							// get p1 or p2
						else															// otherwise single or net play
						{
							if (gControlNeeds[i].playerNum > 1)							// ignore any p2 input data form the device since there is only 1 player on this machine
								break;
							else
								p = gMyNetworkPlayerNum;
						}

						switch (axisState)												// check for specific states
						{
							case	kISpAxisMiddle:
									gPlayerInfo[p].analogSteering = 0.0f;
									gAnalogSteeringTimer[p] -= gFramesPerSecondFrac;
									break;

							case	kISpAxisMinimum:
									gPlayerInfo[p].analogSteering = -1.0f;
									gAnalogSteeringTimer[p] -= gFramesPerSecondFrac;
									break;

							case	kISpAxisMaximum:
									gPlayerInfo[p].analogSteering = 1.0f;
									gAnalogSteeringTimer[p] -= gFramesPerSecondFrac;
									break;

							default:
									gPlayerInfo[p].analogSteering = ((double)axisState - kISpAxisMiddle) / (double)kISpAxisMiddle;
									gAnalogSteeringTimer[p] = 5.0f;

						}
						break;


					/* SIMPLE KEY BUTTON */

				case	kISpElementKind_Button:
						ISpElement_GetSimpleState(gVirtualElements[i],&keyState);		// get state of this one
						if (keyState == kISpButtonDown)
						{
							key = gNeedToKey[i];										// get keymap value for this "need"
							j = key>>3;
							q = (1<<(key&7));
							keyBytes[j] |= q;											// set correct bit in keymap
						}
						break;

			}

		}
	}
}
#endif



#pragma mark -

/******************** TURN ON ISP *********************/

void TurnOnISp(void)
{

}

/******************** TURN OFF ISP *********************/

void TurnOffISp(void)
{
}



/************************ DO KEY CONFIG DIALOG ***************************/
//
// NOTE:  ISp must be turned ON when this is called!
//

void DoKeyConfigDialog(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
	Enter2D(true);

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());

	if (gOSX)
	{
		DoMyKeyboardEdit();
	}

				/* DO ISP CONFIG DIALOG */
	else
	{
		Boolean	ispWasOn = gISpActive;

		TurnOnISp();
		ISpConfigure(nil);
		if (!ispWasOn)
			TurnOffISp();

	}

	Exit2D();
#endif
}


#pragma mark -

/******************* INIT CONTROL BITS *********************/
//
// Called when each level is initialized.
//

void InitControlBits(void)
{
int	i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		gPlayerInfo[i].controlBits = gPlayerInfo[i].controlBits_New = 0;
	}
}



/********************* GET LOCAL KEY STATE *************************/
//
// Called at the end of the Main Loop, after rendering.  Gets the key state of *this* computer
// for the next frame.
//

void GetLocalKeyState(void)
{
		/* SEE IF JUST 1 PLAYER ON THIS COMPUTER */

	if (gActiveSplitScreenMode == SPLITSCREEN_MODE_NONE)
	{
		GetLocalKeyStateForPlayer(gMyNetworkPlayerNum, 0);
	}

		/* GET KEY STATES FOR BOTH PLAYERS */

	else
	{
		GetLocalKeyStateForPlayer(0, 0);
		GetLocalKeyStateForPlayer(1, 1);
	}
}



/*********************** GET LOCAL KEY STATE FOR PLAYER ****************************/
//
// Creates a bitfield of information for the player's key state.
// This bitfield is use throughout the game and over the network for all player controls.
//
// INPUT:  	playerNum 			=	player #0..n
// 			secondaryControsl	=	if doing split-screen player, true to check secondary local controls
//


static void GetLocalKeyStateForPlayer(short playerNum, Boolean secondaryControls)
{
uint32_t	mask,old;
short	i;

	old = gPlayerInfo[playerNum].controlBits;						// remember old bits
	gPlayerInfo[playerNum].controlBits = 0;							// initialize new bits
	mask = 0x1;


			/* BUILD CURRENT BITFIELD */

	for (i = 0; i < NUM_CONTROL_BITS; i++)
	{
		if (secondaryControls)
			puts("TODO: Secondary Controls");
		
		if (GetNeedState(i, playerNum))								// see if key is down
			gPlayerInfo[playerNum].controlBits |= mask;				// set bit in bitfield

		mask <<= 1;													// shift bit to next position
	}


			/* BUILD "NEW" BITFIELD */

	gPlayerInfo[playerNum].controlBits_New = (gPlayerInfo[playerNum].controlBits ^ old) & gPlayerInfo[playerNum].controlBits;
}


/********************** GET CONTROL STATE ***********************/
//
// INPUT: control = one of kControlBit_XXXX
//

Boolean GetControlState(short player, uint32_t control)
{
	if (gPlayerInfo[player].controlBits & (1L << control))
		return(true);
	else
		return(false);
}


/********************** GET CONTROL STATE NEW ***********************/
//
// INPUT: control = one of kControlBit_XXXX
//

Boolean GetControlStateNew(short player, uint32_t control)
{
	if (gPlayerInfo[player].controlBits_New & (1L << control))
		return(true);
	else
		return(false);
}


#pragma mark -

/************************ PUSH KEYS ******************************/

void PushKeys(void)
{
#if 0
int	i;

	for (i = 0; i < 4; i++)
	{
		gKeyMapP[i] = gKeyMap[i];
		gNewKeysP[i] = gNewKeys[i];
		gOldKeysP[i] = gOldKeys[i];
	}
#endif
}

/************************ POP KEYS ******************************/

void PopKeys(void)
{
#if 0
int	i;

	for (i = 0; i < 4; i++)
	{
		gKeyMap[i] = gKeyMapP[i];
		gNewKeys[i] = gNewKeysP[i];
		gOldKeys[i] = gOldKeysP[i];
	}
#endif
}


#pragma mark -


#if 0
/********************* SET MY KEY EDIT DEFAULTS ****************************/

static void SetMyKeyEditDefaults(void)
{
int		i;

	for (i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		gUserKeySettings[i] = gGamePrefs.keySettings[i] = gUserKeySettings_Defaults[i];		// copy from defaults
	}



}


/********************** GET FIRST REAL KEY PRESSED ************/
//
// returns the char code of the first key found in the keymap
//

static short GetFirstRealKeyPressed(void)
{
short	i;

	for (i = 0; i < 256; i++)
	{
		if(GetNewKeyState_Real(i))
			return(i);
	}
	return(0);
}


/*********************** SET KEY CONTROL ********************************/

static void SetKeyControl(short item, u_short keyCode)
{
	switch(item)
	{
		case	2:							// turn left p1
				gUserKeySettings[0] = (gUserKeySettings[0] & 0x00ff) | (keyCode << 8);
				break;

		case	3:							// turn right p1
				gUserKeySettings[0] = (gUserKeySettings[0] & 0xff00) | keyCode;
				break;

		case	10:							// turn left p2
				gUserKeySettings[7] = (gUserKeySettings[7] & 0x00ff) | (keyCode << 8);
				break;

		case	11:							// turn right p2
				gUserKeySettings[7] = (gUserKeySettings[7] & 0xff00) | keyCode;
				break;

				/* P1 */
		case	4:
		case	5:
		case	6:
		case	7:
		case	8:
		case	9:
				gUserKeySettings[item-3] = keyCode;
				break;

				/* P2 */
		case	12:
		case	13:
		case	14:
		case	15:
		case	16:
		case	17:
				gUserKeySettings[item-11+7] = keyCode;
				break;
	}
}

#endif
