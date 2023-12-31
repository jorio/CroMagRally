/****************************/
/*     SOUND ROUTINES       */
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include <string.h>

/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static short EmergencyFreeChannel(void);
static void Calc3DEffectVolume(short effectNum, const OGLPoint3D *where, float volAdjust, uint32_t *leftVolOut, uint32_t *rightVolOut);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ANNOUNCER_VOLUME	(FULL_CHANNEL_VOLUME * 4)
#define	FULL_SONG_VOLUME	0.4f
#define FULL_EFFECTS_VOLUME	0.3f

#define FloatToFixed16(a)      ((Fixed)((float)(a) * 0x000100L))		// convert float to 16bit fixed pt


typedef struct
{
	Byte	bank;
	const char* filename;
	long	refDistance;
} EffectDef;

typedef struct
{
	SndListHandle	sndHandle;
	long			sndOffset;
	short			lastPlayedOnChannel;
	uint32_t		lastLoudness;
} LoadedEffect;

typedef struct
{
	uint16_t	effectNum;
	float	volumeAdjust;
	uint32_t	leftVolume, rightVolume;
}ChannelInfoType;

#define	VOLUME_DISTANCE_FACTOR	.001f		// bigger == sound decays FASTER with dist, smaller = louder far away

/**********************/
/*     VARIABLES      */
/**********************/

short						gRecentAnnouncerEffect = -1, gDelayedAnnouncerEffect;
short						gRecentAnnouncerChannel = -1;
float						gAnnouncerDelay = -1;

static float				gGlobalVolumeFade = 1.0f;				// multiplier applied to sfx/song volumes
static float				gEffectsVolume = FULL_EFFECTS_VOLUME;
static float				gMusicVolume = FULL_SONG_VOLUME;
static float				gMusicVolumeTweak = 1.0f;

OGLPoint3D					gEarCoords[MAX_LOCAL_PLAYERS];			// coord of camera plus a tad to get pt in front of camera
static	OGLVector3D			gEyeVector[MAX_LOCAL_PLAYERS];

static	LoadedEffect		gLoadedEffects[NUM_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
static	ChannelInfoType		gChannelInfo[MAX_CHANNELS];
static	SndChannelPtr		gMusicChannel;

static short				gMaxChannels = 0;

Boolean						gSongPlayingFlag = false;
Boolean						gLoopSongFlag = true;

static short				gCurrentSong = -1;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/


static const char* kSoundBankNames[NUM_SOUNDBANKS] =
{
	[SOUNDBANK_MAIN]			= "Main",
	[SOUNDBANK_LEVELSPECIFIC]	= "LevelSpecific",
	[SOUNDBANK_ANNOUNCER]		= "Announcer",
};

static const EffectDef kEffectsTable[NUM_EFFECTS] =
{
	[EFFECT_BADSELECT]          = {SOUNDBANK_MAIN,				"BadSelect",			2000},
	[EFFECT_SKID3]              = {SOUNDBANK_MAIN,				"Skid3",				2000},
	[EFFECT_ENGINE]             = {SOUNDBANK_MAIN,				"Engine",				1000},
	[EFFECT_SKID]               = {SOUNDBANK_MAIN,				"Skid",					2000},
	[EFFECT_CRASH]              = {SOUNDBANK_MAIN,				"Crash",				1000},
	[EFFECT_BOOM]               = {SOUNDBANK_MAIN,				"Boom",					1500},
	[EFFECT_NITRO]              = {SOUNDBANK_MAIN,				"NitroBurst",			5000},
	[EFFECT_ROMANCANDLE_LAUNCH] = {SOUNDBANK_MAIN,				"RomanCandleLaunch",	2000},
	[EFFECT_ROMANCANDLE_FALL]   = {SOUNDBANK_MAIN,				"RomanCandleFall",		4000},
	[EFFECT_SKID2]              = {SOUNDBANK_MAIN,				"Skid2",				2000},
	[EFFECT_SELECTCLICK]        = {SOUNDBANK_MAIN,				"SelectClick",			2000},
	[EFFECT_GETPOW]             = {SOUNDBANK_MAIN,				"GetPOW",				3000},
	[EFFECT_CANNON]             = {SOUNDBANK_MAIN,				"Cannon",				2000},
	[EFFECT_CRASH2]             = {SOUNDBANK_MAIN,				"Crash2",				3000},
	[EFFECT_SPLASH]             = {SOUNDBANK_MAIN,				"Splash",				3000},
	[EFFECT_BIRDCAW]            = {SOUNDBANK_MAIN,				"BirdCaw",				1000},
	[EFFECT_SNOWBALL]           = {SOUNDBANK_MAIN,				"Snowball",				3000},
	[EFFECT_MINE]               = {SOUNDBANK_MAIN,				"DropMine",				1000},
	[EFFECT_THROW1]             = {SOUNDBANK_MAIN,				"Throw1",				2000},
	[EFFECT_THROW2]             = {SOUNDBANK_MAIN,				"Throw2",				2000},
	[EFFECT_THROW3]             = {SOUNDBANK_MAIN,				"Throw3",				2000},
	[EFFECT_DUSTDEVIL]          = {SOUNDBANK_LEVELSPECIFIC,		"DustDevil",			2000},
	[EFFECT_BLOWDART]           = {SOUNDBANK_LEVELSPECIFIC,		"BlowDart",				2000},
	[EFFECT_HITSNOW]            = {SOUNDBANK_LEVELSPECIFIC,		"HitSnow",				2000},
	[EFFECT_CATAPULT]           = {SOUNDBANK_LEVELSPECIFIC,		"Catapult",				5000},
	[EFFECT_GONG]               = {SOUNDBANK_LEVELSPECIFIC,		"Gong",					4000},
	[EFFECT_SHATTER]            = {SOUNDBANK_LEVELSPECIFIC,		"VaseShatter",			2000},
	[EFFECT_ZAP]                = {SOUNDBANK_LEVELSPECIFIC,		"Zap",					4000},
	[EFFECT_TORPEDOFIRE]        = {SOUNDBANK_LEVELSPECIFIC,		"TorpedoFire",			3000},
	[EFFECT_HUM]                = {SOUNDBANK_LEVELSPECIFIC,		"Hum",					2000},
	[EFFECT_BUBBLES]            = {SOUNDBANK_LEVELSPECIFIC,		"Bubbles",				2500},
	[EFFECT_CHANT]              = {SOUNDBANK_LEVELSPECIFIC,		"Chant",				3000},
	[EFFECT_READY]              = {SOUNDBANK_ANNOUNCER,			"Ready",				2000},
	[EFFECT_SET]                = {SOUNDBANK_ANNOUNCER,			"Set",					2000},
	[EFFECT_GO]                 = {SOUNDBANK_ANNOUNCER,			"Go",					1000},
	[EFFECT_THATSALL]           = {SOUNDBANK_ANNOUNCER,			"ThatsAll",				2000},
	[EFFECT_GOODJOB]            = {SOUNDBANK_ANNOUNCER,			"GoodJob",				2000},
	[EFFECT_REDTEAMWINS]        = {SOUNDBANK_ANNOUNCER,			"RedTeamWins",			4000},
	[EFFECT_GREENTEAMWINS]      = {SOUNDBANK_ANNOUNCER,			"GreenTeamWins",		4000},
	[EFFECT_LAP2]               = {SOUNDBANK_ANNOUNCER,			"Lap2",					1000},
	[EFFECT_FINALLAP]           = {SOUNDBANK_ANNOUNCER,			"FinalLap",				1000},
	[EFFECT_NICESHOT]           = {SOUNDBANK_ANNOUNCER,			"NiceShot",				1000},
	[EFFECT_GOTTAHURT]          = {SOUNDBANK_ANNOUNCER,			"GottaHurt",			1000},
	[EFFECT_1st]                = {SOUNDBANK_ANNOUNCER,			"1st",					1000},
	[EFFECT_2nd]                = {SOUNDBANK_ANNOUNCER,			"2nd",					1000},
	[EFFECT_3rd]                = {SOUNDBANK_ANNOUNCER,			"3rd",					1000},
	[EFFECT_4th]                = {SOUNDBANK_ANNOUNCER,			"4th",					1000},
	[EFFECT_5th]                = {SOUNDBANK_ANNOUNCER,			"5th",					1000},
	[EFFECT_6th]                = {SOUNDBANK_ANNOUNCER,			"6th",					1000},
	[EFFECT_OHYEAH]             = {SOUNDBANK_ANNOUNCER,			"OhYeah",				1000},
	[EFFECT_NICEDRIVING]        = {SOUNDBANK_ANNOUNCER,			"NiceDrivin",			1000},
	[EFFECT_WOAH]               = {SOUNDBANK_ANNOUNCER,			"Woah",					1000},
	[EFFECT_YOUWIN]             = {SOUNDBANK_ANNOUNCER,			"YouWin",				1000},
	[EFFECT_YOULOSE]            = {SOUNDBANK_ANNOUNCER,			"YouLose",				1000},
	[EFFECT_POW_BONEBOMB]       = {SOUNDBANK_ANNOUNCER,			"BoneBomb",				1000},
	[EFFECT_POW_OIL]            = {SOUNDBANK_ANNOUNCER,			"Oil",					1000},
	[EFFECT_POW_NITRO]          = {SOUNDBANK_ANNOUNCER,			"Nitro",				1000},
	[EFFECT_POW_PIGEON]         = {SOUNDBANK_ANNOUNCER,			"Pigeon",				1000},
	[EFFECT_POW_CANDLE]         = {SOUNDBANK_ANNOUNCER,			"Candle",				1000},
	[EFFECT_POW_ROCKET]         = {SOUNDBANK_ANNOUNCER,			"Rocket",				1000},
	[EFFECT_POW_TORPEDO]        = {SOUNDBANK_ANNOUNCER,			"Torpedo",				1000},
	[EFFECT_POW_FREEZE]         = {SOUNDBANK_ANNOUNCER,			"Freeze",				1000},
	[EFFECT_POW_MINE]           = {SOUNDBANK_ANNOUNCER,			"Mine",					1000},
	[EFFECT_POW_SUSPENSION]     = {SOUNDBANK_ANNOUNCER,			"Suspension",			1000},
	[EFFECT_POW_INVISIBILITY]   = {SOUNDBANK_ANNOUNCER,			"Invisibility",			1000},
	[EFFECT_POW_STICKYTIRES]    = {SOUNDBANK_ANNOUNCER,			"StickyTires",			1000},
	[EFFECT_ARROWHEAD]          = {SOUNDBANK_ANNOUNCER,			"Arrowhead",			1000},
	[EFFECT_COMPLETED]          = {SOUNDBANK_ANNOUNCER,			"Completed",			1000},
	[EFFECT_INCOMPLETE]         = {SOUNDBANK_ANNOUNCER,			"Incomplete",			1000},
	[EFFECT_COSTYA]             = {SOUNDBANK_ANNOUNCER,			"CostYa",				1000},
	[EFFECT_WATCHIT]            = {SOUNDBANK_ANNOUNCER,			"WatchIt",				1000},
};


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
OSErr			iErr;

	gRecentAnnouncerEffect = -1;
	gRecentAnnouncerChannel = -1;
	gAnnouncerDelay = 0;

	gMaxChannels = 0;

			/* INIT BANK INFO */

	memset(gLoadedEffects, 0, sizeof(gLoadedEffects));

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


	iErr = SndNewChannel(&gMusicChannel, sampledSynth, 0, nil);
	GAME_ASSERT_MESSAGE(!iErr, "Couldn't allocate music channel");


		/* LOAD DEFAULT SOUNDS */

	LoadSoundBank(SOUNDBANK_MAIN);

	UpdateGlobalVolume();
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

	DisposeAllSoundBanks();


		/* DISPOSE OF CHANNELS */

	for (i = 0; i < gMaxChannels; i++)
		SndDisposeChannel(gSndChannel[i], true);
	gMaxChannels = 0;


}

#pragma mark -


/******************* LOAD A SOUND EFFECT ************************/

void LoadSoundEffect(int effectNum)
{
char path[256];
FSSpec spec;
short refNum;
OSErr err;

	LoadedEffect* loadedSound = &gLoadedEffects[effectNum];
	const EffectDef* effectDef = &kEffectsTable[effectNum];

	if (loadedSound->sndHandle)
	{
		// already loaded
		return;
	}

	snprintf(path, sizeof(path), ":Audio:%s:%s.aiff", kSoundBankNames[effectDef->bank], effectDef->filename);

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
	if (err != noErr)
	{
		DoAlert(path);
		return;
	}

	err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	GAME_ASSERT_MESSAGE(err == noErr, path);

	loadedSound->sndHandle = Pomme_SndLoadFileAsResource(refNum);
	GAME_ASSERT_MESSAGE(loadedSound->sndHandle, path);

	FSClose(refNum);

			/* GET OFFSET INTO IT */

	GetSoundHeaderOffset(loadedSound->sndHandle, &loadedSound->sndOffset);

			/* PRE-DECOMPRESS IT */

	Pomme_DecompressSoundResource(&loadedSound->sndHandle, &loadedSound->sndOffset);
}

/******************* DISPOSE OF A SOUND EFFECT ************************/

void DisposeSoundEffect(int effectNum)
{
	LoadedEffect* loadedSound = &gLoadedEffects[effectNum];

	if (loadedSound->sndHandle)
	{
		DisposeHandle((Handle) loadedSound->sndHandle);
		memset(loadedSound, 0, sizeof(LoadedEffect));
	}
}

/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(int bankNum)
{
	StopAllEffectChannels();

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
		if (kEffectsTable[i].bank == bankNum)
		{
			LoadSoundEffect(i);
		}
	}
}

/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(int bankNum)
{
	gAnnouncerDelay = 0;										// set this to avoid announcer talking after the sound bank is gone

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
		if (kEffectsTable[i].bank == bankNum)
		{
			DisposeSoundEffect(i);
		}
	}
}


/******************* DISPOSE ALL SOUND BANKS *****************/

void DisposeAllSoundBanks(void)
{
	for (int i = 0; i < NUM_SOUNDBANKS; i++)
	{
		DisposeSoundBank(i);
	}
}


/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
SCStatus	theStatus = {0};
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))		// make sure its a legal #
		return;

	SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
	if (theStatus.scChannelBusy)					// if channel busy, then stop it
	{

		mySndCmd.cmd = flushCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[c], &mySndCmd);
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
SCStatus	theStatus = {0};
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))				// make sure its a legal #
		return;

	if (gChannelInfo[c].effectNum != effectNum)		// make sure its the right effect
		return;


	SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
	if (theStatus.scChannelBusy)					// if channel busy, then stop it
	{
		mySndCmd.cmd = flushCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[c], &mySndCmd);
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
static	SndCommand 		mySndCmd;
FSSpec	spec;
short	musicFileRefNum;

	if (songNum == gCurrentSong)					// see if this is already playing
		return;


		/* ZAP ANY EXISTING SONG */

	gCurrentSong 	= songNum;
	gLoopSongFlag 	= loopFlag;
	KillSong();
	DoSoundMaintenance();

		/* ENFORCE MUTE PREF */

//	gMuteMusicFlag = !gGamePrefs.music;

			/******************************/
			/* OPEN APPROPRIATE AIFF FILE */
			/******************************/

	static const struct
	{
		const char* path;
		float volumeTweak;
	} songs[MAX_SONGS] =
	{
		[SONG_WIN]		= {":Audio:WinSong.aiff",		1.25f},
		[SONG_DESERT]	= {":Audio:DesertSong.aiff",	1.0f},
		[SONG_JUNGLE]	= {":Audio:JungleSong.aiff",	1.25f},
		[SONG_THEME]	= {":Audio:ThemeSong.aiff",		1.25f},
		[SONG_ATLANTIS]	= {":Audio:AtlantisSong.aiff",	1.25f},
		[SONG_CHINA]	= {":Audio:ChinaSong.aiff",		1.25f},
		[SONG_CRETE]	= {":Audio:CreteSong.aiff",		1.25f},
		[SONG_EUROPE]	= {":Audio:EuroSong.aiff",		1.25f},
		[SONG_ICE]		= {":Audio:IceSong.aiff",		1.25f},
		[SONG_EGYPT]	= {":Audio:EgyptSong.aiff",		1.25f},
		[SONG_VIKING]	= {":Audio:VikingSong.aiff",	1.25f},
	};

	if (songNum < 0 || songNum >= MAX_SONGS)
		DoFatalAlert("PlaySong: unknown song #");

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, songs[songNum].path, &spec);
	GAME_ASSERT(!iErr);

	iErr = FSpOpenDF(&spec, fsRdPerm, &musicFileRefNum);
	GAME_ASSERT(!iErr);

	gMusicVolumeTweak = songs[songNum].volumeTweak;

	gCurrentSong = songNum;


				/*****************/
				/* START PLAYING */
				/*****************/

			/* START PLAYING FROM FILE */

	iErr = SndStartFilePlay(
		gMusicChannel,
		musicFileRefNum,
		0,
		0, //STREAM_BUFFER_SIZE
		0, //gMusicBuffer
		nil,
		nil, //SongCompletionProc
		true);

	FSClose(musicFileRefNum);		// close the file (Pomme decompresses entire song into memory)

	if (iErr)
	{
		DoAlert("PlaySong: SndStartFilePlay failed!");
		ShowSystemErr(iErr);
	}
	gSongPlayingFlag = true;


			/* SET LOOP FLAG ON STREAM (SOURCE PORT ADDITION) */
			/* So we don't need to re-read the file over and over. */

	mySndCmd.cmd = pommeSetLoopCmd;
	mySndCmd.param1 = loopFlag ? 1 : 0;
	mySndCmd.param2 = 0;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	if (iErr)
		DoFatalAlert("PlaySong: SndDoImmediate (pomme loop extension) failed!");

	uint32_t lv2 = kFullVolume * gMusicVolumeTweak * gMusicVolume;
	uint32_t rv2 = kFullVolume * gMusicVolumeTweak * gMusicVolume;
	mySndCmd.cmd = volumeCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	if (iErr)
		DoFatalAlert("PlaySong: SndDoImmediate (volumeCmd) failed!");
}



/*********************** KILL SONG *********************/

void KillSong(void)
{

	gCurrentSong = -1;

	if (!gSongPlayingFlag)
		return;

	gSongPlayingFlag = false;											// tell callback to do nothing

	SndStopFilePlay(gMusicChannel, true);								// stop it
}



#pragma mark -

/***************************** PLAY EFFECT 3D ***************************/
//
// NO SSP
//
// OUTPUT: channel # used to play sound
//

short PlayEffect3D(short effectNum, const OGLPoint3D *where)
{
short					theChan;
uint32_t				leftVol, rightVol;

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

short PlayEffect_Parms3D(short effectNum, const OGLPoint3D *where, uint32_t rateMultiplier, float volumeAdjust)
{
short			theChan;
uint32_t		leftVol, rightVol;

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

void Update3DSoundChannel(short effectNum, short *channel, const OGLPoint3D *where)
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

static void Calc3DEffectVolume(short effectNum, const OGLPoint3D *where, float volAdjust, uint32_t *leftVolOut, uint32_t *rightVolOut)
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

	refDist = kEffectsTable[effectNum].refDistance;			// get ref dist

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

void UpdateListenerLocation(void)
{
OGLVector3D	v;
int			i;

	for (i = 0; i < gNumSplitScreenPanes; i++)
	{
		const OGLCameraPlacement* cameraPlacement = &gGameView->cameraPlacement[i];

		v.x = cameraPlacement->pointOfInterest.x - cameraPlacement->cameraLocation.x;	// calc line of sight vector
		v.y = cameraPlacement->pointOfInterest.y - cameraPlacement->cameraLocation.y;
		v.z = cameraPlacement->pointOfInterest.z - cameraPlacement->cameraLocation.z;
		FastNormalizeVector(v.x, v.y, v.z, &v);

		gEarCoords[i].x = cameraPlacement->cameraLocation.x + (v.x * 300.0f);			// put ear coord in front of camera
		gEarCoords[i].y = cameraPlacement->cameraLocation.y + (v.y * 300.0f);
		gEarCoords[i].z = cameraPlacement->cameraLocation.z + (v.z * 300.0f);

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
OSErr			myErr;
uint32_t		lv2,rv2;
//static UInt32          loopStart, loopEnd;


			/* GET BANK & SOUND #'S FROM TABLE */

	LoadedEffect* sound = &gLoadedEffects[effectNum];

	GAME_ASSERT_MESSAGE(effectNum >= 0 && effectNum < NUM_EFFECTS, "illegal effect number");
	GAME_ASSERT_MESSAGE(sound->sndHandle, "effect wasn't loaded!");

			/* LOOK FOR FREE CHANNEL */

	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

	lv2 = (float)leftVolume * gEffectsVolume;							// amplify by global volume
	rv2 = (float)rightVolume * gEffectsVolume;


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
	mySndCmd.ptr = ((Ptr) *sound->sndHandle) + sound->sndOffset;	// pointer to SoundHeader
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
// Call this whenever gEffectsVolume is changed.  This will update
// all of the sounds with the correct volume.
//

void UpdateGlobalVolume(void)
{
	gEffectsVolume = FULL_EFFECTS_VOLUME * (0.01f * gGamePrefs.sfxVolumePercent) * gGlobalVolumeFade;

			/* ADJUST VOLUMES OF ALL CHANNELS REGARDLESS IF THEY ARE PLAYING OR NOT */

	for (int c = 0; c < gMaxChannels; c++)
	{
		ChangeChannelVolume(c, gChannelInfo[c].leftVolume, gChannelInfo[c].rightVolume);
	}


			/* UPDATE SONG VOLUME */

	// First, resume song playback if it was paused --
	// e.g. when we're adjusting the volume via pause menu
	SndCommand cmd1 = { .cmd = pommeResumePlaybackCmd };
	SndDoImmediate(gMusicChannel, &cmd1);

	// Now update song channel volume
	gMusicVolume = FULL_SONG_VOLUME * (0.01f * gGamePrefs.musicVolumePercent) * gGlobalVolumeFade;
	uint32_t lv2 = kFullVolume * gMusicVolumeTweak * gMusicVolume;
	uint32_t rv2 = kFullVolume * gMusicVolumeTweak * gMusicVolume;
	SndCommand cmd2 =
	{
		.cmd = volumeCmd,
		.param1 = 0,
		.param2 = (rv2 << 16) | lv2,
	};
	SndDoImmediate(gMusicChannel, &cmd2);
}


/*************** PAUSE ALL SOUND CHANNELS **************/

void PauseAllChannels(Boolean pause)
{
	SndCommand cmd = { .cmd = pause ? pommePausePlaybackCmd : pommeResumePlaybackCmd };

	for (int c = 0; c < gMaxChannels; c++)
	{
		SndDoImmediate(gSndChannel[c], &cmd);
	}

	SndDoImmediate(gMusicChannel, &cmd);
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

	lv2 = (float)leftVol * gEffectsVolume;				// amplify by global volume
	rv2 = (float)rightVol * gEffectsVolume;

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
#if 0

				/* SEE IF TOGGLE MUSIC */

	if (GetNewNeedStateAnyP(kNeed_ToggleMusic))
	{
		ToggleMusic();
	}


			/* SEE IF CHANGE VOLUME */

	if (GetNewKeyState_Real(kKey_RaiseVolume))
	{
		gEffectsVolume += .5f * gFramesPerSecondFrac;
		UpdateGlobalVolume();
	}
	else
	if (GetNewKeyState_Real(kKey_LowerVolume))
	{
		gEffectsVolume -= .5f * gFramesPerSecondFrac;
		if (gEffectsVolume < 0.0f)
			gEffectsVolume = 0.0f;
		UpdateGlobalVolume();
	}


				/* UPDATE SONG */

	if (gSongPlayingFlag)
	{
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
	}
#endif


#if 0
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
#endif


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
OSErr		err; 
SCStatus	theStatus;

	err = SndChannelStatus(gSndChannel[chanNum], sizeof(SCStatus), &theStatus);	// get channel info
	GAME_ASSERT(!err);

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


/****************** GET # OF EFFECT CHANNELS PLAYING *****************/

int GetNumBusyEffectChannels(void)
{
OSErr		err;
SCStatus	theStatus;
int			count = 0;

	for (int i = 0; i < gMaxChannels; i++)
	{
		err = SndChannelStatus(gSndChannel[i], sizeof(SCStatus), &theStatus);
		if (err == noErr && theStatus.scChannelBusy)
		{
			count++;
		}
	}

	return count;
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



#pragma mark -

/********************** GLOBAL VOLUME FADE ***************************/


void FadeSound(float loudness)
{
	gGlobalVolumeFade = loudness;
	UpdateGlobalVolume();
}
