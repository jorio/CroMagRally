/****************************/
/*     SOUND ROUTINES       */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/
#include <Pomme.h>
#include "globals.h"
#include "misc.h"
#include "sound2.h"
#include "file.h"
#include "input.h"
#include "skeletonobj.h"
#include "3dmath.h"
#include "ogl_support.h"
#include "main.h"
#include "window.h"
#include "mainmenu.h"

extern	short		gMainAppRezFile;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int			gNumSplitScreenPanes;
extern	FSSpec				gDataSpec;
extern	float		gFramesPerSecondFrac,gGameWindowShrink;
extern	Boolean		gNetGameInProgress,gIsSelfRunningDemo,gOSX;
extern	PrefsType			gGamePrefs;

/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static short EmergencyFreeChannel(void);
static void Calc3DEffectVolume(short effectNum, OGLPoint3D *where, float volAdjust, uint32_t *leftVolOut, uint32_t *rightVolOut);
static void UpdateGlobalVolume(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ANNOUNCER_VOLUME	(FULL_CHANNEL_VOLUME * 4)
#define	SONG_VOLUME		1.6f

#define FloatToFixed16(a)      ((Fixed)((float)(a) * 0x000100L))		// convert float to 16bit fixed pt


#define		MAX_CHANNELS			20

#define		MAX_EFFECTS				50

#define     kNumBogusConverters     200000

typedef struct
{
	Byte	bank,sound;
	long	refDistance;
}EffectType;


typedef struct
{
	uint16_t	effectNum;
	float	volumeAdjust;
	uint32_t	leftVolume, rightVolume;
}ChannelInfoType;

static CGrafPtr		gQTDummyPort = nil;


#define	VOLUME_DISTANCE_FACTOR	.001f		// bigger == sound decays FASTER with dist, smaller = louder far away

/**********************/
/*     VARIABLES      */
/**********************/

short						gRecentAnnouncerEffect = -1, gDelayedAnnouncerEffect;
short						gRecentAnnouncerChannel = -1;
float						gAnnouncerDelay = -1;

float						gMoviesTaskTimer = 0;

float						gGlobalVolume = .4;

OGLPoint3D					gEarCoords[MAX_LOCAL_PLAYERS];			// coord of camera plus a tad to get pt in front of camera
static	OGLVector3D			gEyeVector[MAX_LOCAL_PLAYERS];


static	SndListHandle		gSndHandles[MAX_SOUND_BANKS][MAX_EFFECTS];		// handles to ALL sounds
static  long				gSndOffsets[MAX_SOUND_BANKS][MAX_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
static	ChannelInfoType		gChannelInfo[MAX_CHANNELS];

static short				gMaxChannels = 0;


static short				gNumSndsInBank[MAX_SOUND_BANKS] = {0,0,0,0};


Boolean						gSongPlayingFlag = false;
Boolean						gLoopSongFlag = true;


static short				gMusicFileRefNum = 0x0ded;
Boolean				gMuteMusicFlag = false;
static short				gCurrentSong = -1;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/

static EffectType	gEffectsTable[] =
{
	SOUND_BANK_MAIN,SOUND_DEFAULT_BADSELECT,2000,			// EFFECT_BADSELECT
	SOUND_BANK_MAIN,SOUND_DEFAULT_SKID3,2000,				// EFFECT_SKID3
	SOUND_BANK_MAIN,SOUND_DEFAULT_ENGINE,1000,				// EFFECT_ENGINE
	SOUND_BANK_MAIN,SOUND_DEFAULT_SKID,2000,				// EFFECT_SKID
	SOUND_BANK_MAIN,SOUND_DEFAULT_CRASH,1000,				// EFFECT_CRASH
	SOUND_BANK_MAIN,SOUND_DEFAULT_BOOM,1500,				// EFFECT_BOOM
	SOUND_BANK_MAIN,SOUND_DEFAULT_NITRO,5000,				// EFFECT_NITRO
	SOUND_BANK_MAIN,SOUND_DEFAULT_ROMANCANDLELAUNCH,2000,	// EFFECT_ROMANCANDLE_LAUNCH
	SOUND_BANK_MAIN,SOUND_DEFAULT_ROMANCANDLEFALL,4000,		// EFFECT_ROMANCANDLE_FALL
	SOUND_BANK_MAIN,SOUND_DEFAULT_SKID2,2000,				// EFFECT_SKID2
	SOUND_BANK_MAIN,SOUND_DEFAULT_SELECTCLICK,2000,			// EFFECT_SELECTCLICK
	SOUND_BANK_MAIN,SOUND_DEFAULT_GETPOW,3000,				// EFFECT_GETPOW
	SOUND_BANK_MAIN,SOUND_DEFAULT_CANNON,2000,				// EFFECT_CANNON
	SOUND_BANK_MAIN,SOUND_DEFAULT_CRASH2,3000,				// EFFECT_CRASH2
	SOUND_BANK_MAIN,SOUND_DEFAULT_SPLASH,3000,				// EFFECT_SPLASH
	SOUND_BANK_MAIN,SOUND_DEFAULT_BIRDCAW,1000,				// EFFECT_BIRDCAW
	SOUND_BANK_MAIN,SOUND_DEFAULT_SNOWBALL,3000,			// EFFECT_SNOWBALL
	SOUND_BANK_MAIN,SOUND_DEFAULT_MINE,1000,				// EFFECT_MINE

	SOUND_BANK_LEVELSPECIFIC,SOUND_DESERT_DUSTDEVIL,2000,	// EFFECT_DUSTDEVIL

	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_BLOWDART,2000,	// EFFECT_BLOWDART

	SOUND_BANK_LEVELSPECIFIC,SOUND_ICE_HITSNOW,2000,		// EFFECT_HITSNOW


	SOUND_BANK_LEVELSPECIFIC,SOUND_EUROPE_CATAPULT,5000,	// EFFECT_CATAPULT

	SOUND_BANK_LEVELSPECIFIC,SOUND_CHINA_GONG,4000,			// EFFECT_GONG

	SOUND_BANK_LEVELSPECIFIC,SOUND_EGYPT_SHATTER,2000,		// EFFECT_SHATTER

	SOUND_BANK_LEVELSPECIFIC,SOUND_ATLANTIS_ZAP,4000,		// EFFECT_ZAP
	SOUND_BANK_LEVELSPECIFIC,SOUND_ATLANTIS_TORPEDOFIRE,3000,	// EFFECT_TORPEDOFIRE
	SOUND_BANK_LEVELSPECIFIC,SOUND_ATLANTIS_HUM,2000,		// EFFECT_HUM
	SOUND_BANK_LEVELSPECIFIC,SOUND_ATLANTIS_BUBBLES,2500,	// EFFECT_BUBBLES

	SOUND_BANK_LEVELSPECIFIC,SOUND_STONEHENGE_CHANT,3000,	// EFFECT_CHANT


	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_READY,2000,			// EFFECT_READY
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_SET,2000,				// EFFECT_SET
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_GO,1000,				// EFFECT_GO
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_THROW1,2000,			// EFFECT_THROW1
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_THROW2,2000,			// EFFECT_THROW2
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_THROW3,2000,			// EFFECT_THROW3
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_THATSALL,2000,			// EFFECT_THATSALL
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_GOODJOB,2000,			// EFFECT_GOODJOB
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_REDTEAMWINS,4000,		// EFFECT_REDTEAMWINS
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_GREENTEAMWINS,4000,	// EFFECT_GREENTEAMWINS
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_LAP2,1000,				// EFFECT_LAP2
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_FINALLAP,1000,			// EFFECT_FINALLAP
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_NICESHOT,1000,			// EFFECT_NICESHOT
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_GOTTAHURT,1000,		// EFFECT_GOTTAHURT
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_1st,1000,				// EFFECT_1st
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_2nd,1000,				// EFFECT_2nd
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_3rd,1000,				// EFFECT_3rd
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_4th,1000,				// EFFECT_4th
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_5th,1000,				// EFFECT_5th
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_6th,1000,				// EFFECT_6th
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_OHYEAH,1000,			// EFFECT_OHYEAH
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_NICEDRIVING,1000,		// EFFECT_NICEDRIVING
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_WOAH,1000,				// EFFECT_WOAH
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_YOUWIN,1000,			// EFFECT_YOUWIN
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_YOULOSE,1000,			// EFFECT_YOULOSE
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_BONEBOMB,1000,			// EFFECT_POW_BONEBOMB
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_OIL,1000,			// EFFECT_POW_OIL
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_NITRO,1000,			// EFFECT_POW_NITRO
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_PIGEON,1000,			// EFFECT_POW_PIGEON
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_CANDLE,1000,			// EFFECT_POW_CANDLE
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_ROCKET,1000,			// EFFECT_POW_ROCKET
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_TORPEDO,1000,			// EFFECT_POW_TORPEDO
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_FREEZE,1000,			// EFFECT_POW_FREEZE
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_MINE,1000,			// EFFECT_POW_MINE
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_SUSPENSION,1000,			// EFFECT_POW_SUSPENSION
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_INVISIBILITY,1000,			// EFFECT_POW_INVISIBILITY
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_TIRES,1000,			// EFFECT_POW_STICKYTIRES
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_ARROWHEAD,1000,			// EFFECT_ARROWHEAD
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_COMPLETE,1000,			// EFFECT_COMPLETED
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_INCOMPLETE,1000,			// EFFECT_INCOMPLETE
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_COSTYA,1000,			// EFFECT_COSTYA
	SOUND_BANK_CAVEMAN,SOUND_CAVEMAN_WATCHIT,1000,			// EFFECT_WATCHIT

};


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
OSErr			iErr;
short			i;
FSSpec			spec;

	gRecentAnnouncerEffect = -1;
	gRecentAnnouncerChannel = -1;
	gAnnouncerDelay = 0;

	gMaxChannels = 0;

			/* INIT BANK INFO */

	for (i = 0; i < MAX_SOUND_BANKS; i++)
		gNumSndsInBank[i] = 0;

			/******************/
			/* ALLOC CHANNELS */
			/******************/

			/* ALL OTHER CHANNELS */

	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */

		iErr = SndNewChannel(&gSndChannel[gMaxChannels],sampledSynth,0,nil);
		if (iErr)												// if err, stop allocating channels
			break;
	}


		/* LOAD DEFAULT SOUNDS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_MAIN);
}


/******************** SHUTDOWN SOUND ***************************/
//
// Called at Quit time
//

void ShutdownSound(void)
{
int	i;

			/* STOP ANY PLAYING AUDIO */

	StopAllEffectChannels();
	KillSong();


		/* DISPOSE OF CHANNELS */

	for (i = 0; i < gMaxChannels; i++)
		SndDisposeChannel(gSndChannel[i], true);
	gMaxChannels = 0;


}

#pragma mark -

/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(FSSpec *spec, long bankNum)
{
short			srcFile1,numSoundsInBank,i;
Str255			error = "Couldnt Open Sound Resource File.";
OSErr			iErr;

	StopAllEffectChannels();

	if (bankNum >= MAX_SOUND_BANKS)
		DoFatalAlert("LoadSoundBank: bankNum >= MAX_SOUND_BANKS");

			/* DISPOSE OF EXISTING BANK */

	DisposeSoundBank(bankNum);


			/* OPEN APPROPRIATE REZ FILE */

	srcFile1 = FSpOpenResFile(spec, fsRdPerm);
	if (srcFile1 == -1)
		DoFatalAlert("LoadSoundBank: OpenResFile failed!");

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	UseResFile( srcFile1 );												// open sound resource fork
	numSoundsInBank = Count1Resources('snd ');							// count # snd's in this bank
	if (numSoundsInBank > MAX_EFFECTS)
		DoFatalAlert("LoadSoundBank: numSoundsInBank > MAX_EFFECTS");

	for (i=0; i < numSoundsInBank; i++)
	{
				/* LOAD SND REZ */

		gSndHandles[bankNum][i] = (SndListResource **)GetResource('snd ',BASE_EFFECT_RESOURCE+i);
		if (gSndHandles[bankNum][i] == nil)
		{
			iErr = ResError();
			DoAlert("LoadSoundBank: GetResource failed!");
			if (iErr == memFullErr)
				DoFatalAlert("LoadSoundBank: Out of Memory");
			else
				ShowSystemErr(iErr);
		}
		DetachResource((Handle)gSndHandles[bankNum][i]);				// detach resource from rez file & make a normal Handle

		HNoPurge((Handle)gSndHandles[bankNum][i]);						// make non-purgeable
		HLockHi((Handle)gSndHandles[bankNum][i]);

				/* GET OFFSET INTO IT */

		GetSoundHeaderOffset(gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);
	}

	UseResFile(gMainAppRezFile );								// go back to normal res file
	CloseResFile(srcFile1);

	gNumSndsInBank[bankNum] = numSoundsInBank;					// remember how many sounds we've got
}


/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(short bankNum)
{
short	i;

	gAnnouncerDelay = 0;										// set this to avoid announcer talking after the sound bank is gone

	if (bankNum > MAX_SOUND_BANKS)
		return;

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */

	for (i=0; i < gNumSndsInBank[bankNum]; i++)
		DisposeHandle((Handle)gSndHandles[bankNum][i]);


	gNumSndsInBank[bankNum] = 0;
}



/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
OSErr 		myErr;
SCStatus	theStatus;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))		// make sure its a legal #
		return;

	myErr = SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
	if (theStatus.scChannelBusy)					// if channel busy, then stop it
	{

		mySndCmd.cmd = flushCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);
	}

	*channelNum = -1;
}


/********************* STOP A CHANNEL IF EFFECT NUM **********************/
//
// Stops the indicated sound channel from playing if it is still this effect #
//

void StopAChannelIfEffectNum(short *channelNum, short effectNum)
{
SndCommand 	mySndCmd;
OSErr 		myErr;
SCStatus	theStatus;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))				// make sure its a legal #
		return;

	if (gChannelInfo[c].effectNum != effectNum)		// make sure its the right effect
		return;


	myErr = SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
	if (theStatus.scChannelBusy)					// if channel busy, then stop it
	{

		mySndCmd.cmd = flushCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);
	}

	*channelNum = -1;
}



/********************* STOP ALL EFFECT CHANNELS **********************/

void StopAllEffectChannels(void)
{
short		i;

	for (i=0; i < gMaxChannels; i++)
	{
		short	c;

		c = i;
		StopAChannel(&c);
	}
}


/****************** WAIT EFFECTS SILENT *********************/

void WaitEffectsSilent(void)
{
short	i;
Boolean	isBusy;
SCStatus				theStatus;

	do
	{
		isBusy = 0;
		for (i=0; i < gMaxChannels; i++)
		{
			SndChannelStatus(gSndChannel[i],sizeof(SCStatus),&theStatus);	// get channel info
			isBusy |= theStatus.scChannelBusy;
		}
	}while(isBusy);
}

#pragma mark -

/******************** PLAY SONG ***********************/
//
// if songNum == -1, then play existing open song
//
// INPUT: loopFlag = true if want song to loop
//

void PlaySong(short songNum, Boolean loopFlag)
{
OSErr 	iErr;
Str255	errStr = "PlaySong: Couldnt Open Music AIFF File.";
static	SndCommand 		mySndCmd;
FSSpec	spec;
short	myRefNum;
float	volumeTweak;
GrafPtr	oldPort;

	if (songNum == gCurrentSong)					// see if this is already playing
		return;


		/* ZAP ANY EXISTING SONG */

	gCurrentSong 	= songNum;
	gLoopSongFlag 	= loopFlag;
	KillSong();
	DoSoundMaintenance();

			/******************************/
			/* OPEN APPROPRIATE AIFF FILE */
			/******************************/

	switch(songNum)
	{
		case	SONG_WIN:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:WinSong.aiff", &spec);
				volumeTweak = 1.9;
				break;

		case	SONG_DESERT:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:DesertSong.aiff", &spec);
				volumeTweak = 1.0;
				break;

		case	SONG_JUNGLE:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:JungleSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		case	SONG_THEME:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:ThemeSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		case	SONG_ATLANTIS:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:AtlantisSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		case	SONG_CHINA:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:ChinaSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		case	SONG_CRETE:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:CreteSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		case	SONG_EUROPE:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:EuroSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		case	SONG_ICE:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:IceSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		case	SONG_EGYPT:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:EgyptSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		case	SONG_VIKING:
				iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:VikingSong.aiff", &spec);
				volumeTweak = 1.5;
				break;

		default:
				DoFatalAlert("PlaySong: unknown song #");
	}


	if (iErr)
	{
		iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:ThemeSong.mp3", &spec);		// if song not found, try MP3 version of theme
		if (iErr)
			DoFatalAlert("PlaySong: song file not found");
	}

	gCurrentSong = songNum;


				/*****************/
				/* START PLAYING */
				/*****************/

	IMPLEMENT_ME_SOFT();
#if 0
			/* GOT TO SET A DUMMY PORT OR QT MAY FREAK */

	if (gQTDummyPort == nil)						// create a blank graf port
		gQTDummyPort = CreateNewPort();

	GetPort(&oldPort);								// set as active before NewMovieFromFile()
	SetPort(gQTDummyPort);


	iErr = OpenMovieFile(&spec, &myRefNum, fsRdPerm);
	if (myRefNum && (iErr == noErr))
	{
		iErr = NewMovieFromFile(&gSongMovie, myRefNum, 0, nil, newMovieActive, nil);
		CloseMovieFile(myRefNum);

		if (iErr == noErr)
		{
//			if (gOSX)
//				LoadMovieIntoRam(gSongMovie, 0, GetMovieDuration(gSongMovie), keepInRam);

			SetMoviePlayHints(gSongMovie, 0, hintsUseSoundInterp|hintsHighQuality);

			SetMovieVolume(gSongMovie, FloatToFixed16(gGlobalVolume * SONG_VOLUME * volumeTweak));						// set volume
			StartMovie(gSongMovie);

			gSongPlayingFlag = true;
		}
	}

	SetPort(oldPort);
#endif



			/* SEE IF WANT TO MUTE THE MUSIC */

	if (gMuteMusicFlag)
	{
		IMPLEMENT_ME_SOFT();
#if 0
		if (gSongMovie)
			StopMovie(gSongMovie);
#endif
	}
}



/*********************** KILL SONG *********************/

void KillSong(void)
{

	gCurrentSong = -1;

	if (!gSongPlayingFlag)
		return;

	gSongPlayingFlag = false;											// tell callback to do nothing

	IMPLEMENT_ME_SOFT();
#if 0
	StopMovie(gSongMovie);
	DisposeMovie(gSongMovie);
#endif

	gMusicFileRefNum = 0x0ded;
}

/******************** TOGGLE MUSIC *********************/

void ToggleMusic(void)
{
	gMuteMusicFlag = !gMuteMusicFlag;

	IMPLEMENT_ME_SOFT();
#if 0
	if (gSongMovie)
	{
		if (gMuteMusicFlag)
			StopMovie(gSongMovie);
		else
			StartMovie(gSongMovie);
	}
#endif
}



#pragma mark -

/***************************** PLAY EFFECT 3D ***************************/
//
// NO SSP
//
// OUTPUT: channel # used to play sound
//

short PlayEffect3D(short effectNum, OGLPoint3D *where)
{
short					theChan;
Byte					bankNum,soundNum;
uint32_t					leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);
	}

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, 1.0, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, NORMAL_CHANNEL_RATE);

	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = 1.0;			// full volume adjust

	return(theChan);									// return channel #
}



/***************************** PLAY EFFECT PARMS 3D ***************************/
//
// Plays an effect with parameters in 3D
//
// OUTPUT: channel # used to play sound
//

short PlayEffect_Parms3D(short effectNum, OGLPoint3D *where, uint32_t rateMultiplier, float volumeAdjust)
{
short			theChan;
Byte			bankNum,soundNum;
uint32_t			leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);
	}

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, volumeAdjust, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


				/* PLAY EFFECT */

	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, rateMultiplier);

	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = volumeAdjust;	// remember volume adjuster

	return(theChan);									// return channel #
}


/************************* UPDATE 3D SOUND CHANNEL ***********************/

void Update3DSoundChannel(short effectNum, short *channel, OGLPoint3D *where)
{
SCStatus		theStatus;
uint32_t			leftVol,rightVol;
short			c;

	c = *channel;

	if (c == -1)
		return;


			/* SEE IF SOUND HAS COMPLETED */

	SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
	if (!theStatus.scChannelBusy)									// see if channel not busy
	{
gone:
		*channel = -1;												// this channel is now invalid
		return;
	}

			/* MAKE SURE THE SAME SOUND IS STILL ON THIS CHANNEL */

	if (effectNum != gChannelInfo[c].effectNum)
		goto gone;


			/* UPDATE THE THING */

	Calc3DEffectVolume(gChannelInfo[c].effectNum, where, gChannelInfo[c].volumeAdjust, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)										// if volume goes to 0, then kill channel
	{
		StopAChannel(channel);
		return;
	}

	ChangeChannelVolume(c, leftVol, rightVol);
}

/******************** CALC 3D EFFECT VOLUME *********************/

static void Calc3DEffectVolume(short effectNum, OGLPoint3D *where, float volAdjust, uint32_t *leftVolOut, uint32_t *rightVolOut)
{
float	dist;
float	refDist,volumeFactor;
uint32_t	volume,left,right;
uint32_t	maxLeft,maxRight;

	dist 	= OGLPoint3D_Distance(where, &gEarCoords[0]);		// calc dist to sound for pane 0
	if (gNumSplitScreenPanes > 1)								// see if other pane is closer (thus louder)
	{
		float	dist2 = OGLPoint3D_Distance(where, &gEarCoords[1]);

		if (dist2 < dist)
			dist = dist2;
	}

			/* DO VOLUME CALCS */

	refDist = gEffectsTable[effectNum].refDistance;			// get ref dist

	dist -= refDist;
	if (dist <= 0.0f)
		volumeFactor = 1.0f;
	else
	{
		volumeFactor = 1.0f / (dist * VOLUME_DISTANCE_FACTOR);
		if (volumeFactor > 1.0f)
			volumeFactor = 1.0f;
	}

	volume = (float)FULL_CHANNEL_VOLUME * volumeFactor * volAdjust;


	if (volume < 6)							// if really quiet, then just turn it off
	{
		*leftVolOut = *rightVolOut = 0;
		return;
	}

			/************************/
			/* DO STEREO SEPARATION */
			/************************/

	else
	{
		float		volF = (float)volume;
		OGLVector2D	earToSound,lookVec;
		int			i;
		float		dot,cross;

		maxLeft = maxRight = 0;

		for (i = 0; i < gNumSplitScreenPanes; i++)										// calc for each camera and use max of left & right
		{

				/* CALC VECTOR TO SOUND */

			earToSound.x = where->x - gEarCoords[0].x;
			earToSound.y = where->z - gEarCoords[0].z;
			FastNormalizeVector2D(earToSound.x, earToSound.y, &earToSound, true);


				/* CALC EYE LOOK VECTOR */

			FastNormalizeVector2D(gEyeVector[0].x, gEyeVector[0].z, &lookVec, true);


				/* DOT PRODUCT  TELLS US HOW MUCH STEREO SHIFT */

			dot = 1.0f - fabs(OGLVector2D_Dot(&earToSound,  &lookVec));
			if (dot < 0.0f)
				dot = 0.0f;
			else
			if (dot > 1.0f)
				dot = 1.0f;


				/* CROSS PRODUCT TELLS US WHICH SIDE */

			cross = OGLVector2D_Cross(&earToSound,  &lookVec);


					/* DO LEFT/RIGHT CALC */

			if (cross > 0.0f)
			{
				left 	= volF + (volF * dot);
				right 	= volF - (volF * dot);
			}
			else
			{
				right 	= volF + (volF * dot);
				left 	= volF - (volF * dot);
			}


					/* KEEP MAX */

			if (left > maxLeft)
				maxLeft = left;
			if (right > maxRight)
				maxRight = right;

		}


	}

	*leftVolOut = maxLeft;
	*rightVolOut = maxRight;
}



#pragma mark -

/******************* UPDATE LISTENER LOCATION ******************/
//
// Get ear coord for all local players
//

void UpdateListenerLocation(OGLSetupOutputType *setupInfo)
{
OGLVector3D	v;
int			i;

	for (i = 0; i < gNumSplitScreenPanes; i++)
	{
		v.x = setupInfo->cameraPlacement[i].pointOfInterest.x - setupInfo->cameraPlacement[i].cameraLocation.x;	// calc line of sight vector
		v.y = setupInfo->cameraPlacement[i].pointOfInterest.y - setupInfo->cameraPlacement[i].cameraLocation.y;
		v.z = setupInfo->cameraPlacement[i].pointOfInterest.z - setupInfo->cameraPlacement[i].cameraLocation.z;
		FastNormalizeVector(v.x, v.y, v.z, &v);

		gEarCoords[i].x = setupInfo->cameraPlacement[i].cameraLocation.x + (v.x * 300.0f);			// put ear coord in front of camera
		gEarCoords[i].y = setupInfo->cameraPlacement[i].cameraLocation.y + (v.y * 300.0f);
		gEarCoords[i].z = setupInfo->cameraPlacement[i].cameraLocation.z + (v.z * 300.0f);

		gEyeVector[i] = v;
	}
}


/***************************** PLAY EFFECT ***************************/
//
// OUTPUT: channel # used to play sound
//

short PlayEffect(short effectNum)
{
	return(PlayEffect_Parms(effectNum,FULL_CHANNEL_VOLUME,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE));

}

/***************************** PLAY EFFECT PARMS ***************************/
//
// Plays an effect with parameters
//
// OUTPUT: channel # used to play sound
//

short  PlayEffect_Parms(short effectNum, uint32_t leftVolume, uint32_t rightVolume, unsigned long rateMultiplier)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
short			theChan;
Byte			bankNum,soundNum;
OSErr			myErr;
uint32_t			lv2,rv2;
static UInt32          loopStart, loopEnd;


			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum = gEffectsTable[effectNum].bank;
	soundNum = gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);
	}

			/* LOOK FOR FREE CHANNEL */

	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

	lv2 = (float)leftVolume * gGlobalVolume;							// amplify by global volume
	rv2 = (float)rightVolume * gGlobalVolume;


					/* GET IT GOING */

	chanPtr = gSndChannel[theChan];

	mySndCmd.cmd = flushCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);


	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.ptr = ((Ptr) *gSndHandles[bankNum][soundNum]) + gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
    SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMultiplier;
	SndDoImmediate(chanPtr, &mySndCmd);

    // If the loop start point is before the loop end, then there is a loop
	/*
    sndPtr = (SoundHeaderPtr)(((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum]);
    loopStart = sndPtr->loopStart;
    loopEnd = sndPtr->loopEnd;
    if (loopStart + 1 < loopEnd)
    {
    	mySndCmd.cmd = callBackCmd;										// let us know when the buffer is done playing
    	mySndCmd.param1 = 0;
    	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
    	SndDoCommand(chanPtr, &mySndCmd, true);
	}
	*/


			/* SET MY INFO */

	gChannelInfo[theChan].effectNum 	= effectNum;		// remember what effect is playing on this channel
	gChannelInfo[theChan].leftVolume 	= leftVolume;		// remember requested volume (not the adjusted volume!)
	gChannelInfo[theChan].rightVolume 	= rightVolume;
	return(theChan);										// return channel #
}


#pragma mark -


/****************** UPDATE GLOBAL VOLUME ************************/
//
// Call this whenever gGlobalVolume is changed.  This will update
// all of the sounds with the correct volume.
//

static void UpdateGlobalVolume(void)
{
int		c;

			/* ADJUST VOLUMES OF ALL CHANNELS REGARDLESS IF THEY ARE PLAYING OR NOT */

	for (c = 0; c < gMaxChannels; c++)
	{
		ChangeChannelVolume(c, gChannelInfo[c].leftVolume, gChannelInfo[c].rightVolume);
	}


			/* UPDATE SONG VOLUME */

	IMPLEMENT_ME_SOFT();
#if 0
	if (gSongPlayingFlag)
		SetMovieVolume(gSongMovie, FloatToFixed16(gGlobalVolume) * SONG_VOLUME);
#endif

}

/*************** CHANGE CHANNEL VOLUME **************/
//
// Modifies the volume of a currently playing channel
//

void ChangeChannelVolume(short channel, uint32_t leftVol, uint32_t rightVol)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
uint32_t			lv2,rv2;

	if (channel < 0)									// make sure it's valid
		return;

	lv2 = (float)leftVol * gGlobalVolume;				// amplify by global volume
	rv2 = (float)rightVol * gGlobalVolume;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr

	mySndCmd.cmd = volumeCmd;							// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;			// set volume left & right
	SndDoImmediate(chanPtr, &mySndCmd);

	gChannelInfo[channel].leftVolume = leftVol;				// remember requested volume (not the adjusted volume!)
	gChannelInfo[channel].rightVolume = rightVol;
}



/*************** CHANGE CHANNEL RATE **************/
//
// Modifies the frequency of a currently playing channel
//
// The Input Freq is a fixed-point multiplier, not the static rate via rateCmd.
// This function uses rateMultiplierCmd, so a value of 0x00020000 is x2.0
//

void ChangeChannelRate(short channel, long rateMult)
{
static	SndCommand 		mySndCmd;
static 	OSErr			iErr;
static	SndChannelPtr	chanPtr;

	if (channel < 0)									// make sure it's valid
		return;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMult;
	SndDoImmediate(chanPtr, &mySndCmd);
}




#pragma mark -


/******************** DO SOUND MAINTENANCE *************/
//
// 		ReadKeyboard() must have already been called
//

void DoSoundMaintenance(void)
{
				/* SEE IF TOGGLE MUSIC */

	if (GetNewKeyState_Real(kKey_ToggleMusic))
	{
		ToggleMusic();
	}


			/* SEE IF CHANGE VOLUME */

	if (GetNewKeyState_Real(kKey_RaiseVolume))
	{
		gGlobalVolume += .5f * gFramesPerSecondFrac;
		UpdateGlobalVolume();
	}
	else
	if (GetNewKeyState_Real(kKey_LowerVolume))
	{
		gGlobalVolume -= .5f * gFramesPerSecondFrac;
		if (gGlobalVolume < 0.0f)
			gGlobalVolume = 0.0f;
		UpdateGlobalVolume();
	}


				/* UPDATE SONG */

	if (gSongPlayingFlag)
	{
		IMPLEMENT_ME_SOFT();
#if 0
		if (IsMovieDone(gSongMovie))				// see if the song has completed
		{
			if (gLoopSongFlag)						// see if repeat it
			{
				GoToBeginningOfMovie(gSongMovie);
				StartMovie(gSongMovie);
			}
			else									// otherwise kill the song
				KillSong();
		}
		else
		{
			gMoviesTaskTimer -= gFramesPerSecondFrac;
			if (gMoviesTaskTimer <= 0.0f)
			{
				MoviesTask(gSongMovie, 0);
				gMoviesTaskTimer += .15f;
			}
		}
#endif
	}


		/* WHILE WE'RE HERE, SEE IF SCALE SCREEN */

	if (GetNewKeyState_Real(KEY_F10))
	{
		gGamePrefs.screenCrop -= .1f;
		if (gGamePrefs.screenCrop < 0.0f)
			gGamePrefs.screenCrop = 0;
		ChangeWindowScale();
	}
	else
	if (GetNewKeyState_Real(KEY_F9))
	{
		gGamePrefs.screenCrop += .1f;
		if (gGamePrefs.screenCrop > 1.0f)
			gGamePrefs.screenCrop = 1.0;
		ChangeWindowScale();
	}


		/* ALSO CHECK OPTIONS */


	if (!gNetGameInProgress)
	{
		if (GetNewKeyState_Real(KEY_F1) && (!gIsSelfRunningDemo))
		{
			DoGameSettingsDialog();
		}
	}


		/* CHECK FOR ANNOUNCER */

	if (gAnnouncerDelay > 0.0f)
	{
		gAnnouncerDelay -= gFramesPerSecondFrac;
		if (gAnnouncerDelay <= 0.0f)								// see if ready to go
		{
			gAnnouncerDelay = 0;
			PlayAnnouncerSound(gDelayedAnnouncerEffect, false, 0);	// play the delayed effect now
		}
	}

}



/******************** FIND SILENT CHANNEL *************************/

static short FindSilentChannel(void)
{
short		theChan;
OSErr		myErr;
SCStatus	theStatus;

	for (theChan=0; theChan < gMaxChannels; theChan++)
	{
		myErr = SndChannelStatus(gSndChannel[theChan],sizeof(SCStatus),&theStatus);	// get channel info
		if (myErr)
			continue;
		if (!theStatus.scChannelBusy)					// see if channel not busy
		{
			return(theChan);
		}
	}

			/* NO FREE CHANNELS */

	return(-1);
}


/********************** IS EFFECT CHANNEL PLAYING ********************/

Boolean IsEffectChannelPlaying(short chanNum)
{
SCStatus	theStatus;

	SndChannelStatus(gSndChannel[chanNum],sizeof(SCStatus),&theStatus);	// get channel info
	return (theStatus.scChannelBusy);
}

/***************** EMERGENCY FREE CHANNEL **************************/
//
// This is used by the music streamer when all channels are used.
// Since we must have a channel to play music, we arbitrarily kill some other channel.
//

static short EmergencyFreeChannel(void)
{
short	i,c;

	for (i = 0; i < gMaxChannels; i++)
	{
		c = i;
		StopAChannel(&c);
		return(i);
	}

		/* TOO BAD, GOTTA NUKE ONE OF THE STREAMING CHANNELS IT SEEMS */

	StopAChannel(0);
	return(0);
}


#pragma mark -

/********************** PLAY ANNOUNCER SOUND ***************************/

void PlayAnnouncerSound(short effectNum, Boolean override, float delay)
{
	gAnnouncerDelay = delay;


				/*************************************/
				/* IF NO DELAY, THEN TRY TO PLAY NOW */
				/*************************************/

	if (delay == 0.0f)
	{

				/* SEE IF ANNOUNCER IS STILL TALKING */

		if ((!override) && (gRecentAnnouncerChannel != -1))										// see if announcer has spoken yet
		{
			if (IsEffectChannelPlaying(gRecentAnnouncerChannel))								// see if that channel is playing
			{
				if (gChannelInfo[gRecentAnnouncerChannel].effectNum == gRecentAnnouncerEffect)	// is it playing the announcer from last time?
					return;
			}
		}


				/* PLAY THE NEW EFFECT */

		gRecentAnnouncerChannel = PlayEffect_Parms(effectNum, ANNOUNCER_VOLUME, ANNOUNCER_VOLUME, NORMAL_CHANNEL_RATE);
		gRecentAnnouncerEffect = effectNum;
	}

				/***************************/
				/* DELAY, SO DONT PLAY NOW */
				/***************************/
	else
	{
		gDelayedAnnouncerEffect = effectNum;

	}

}
