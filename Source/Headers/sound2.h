//
// Sound2.h
//


#define		BASE_EFFECT_RESOURCE	10000

#define		FULL_CHANNEL_VOLUME		kFullVolume
#define		NORMAL_CHANNEL_RATE		0x10000



enum
{
	SONG_WIN,
	SONG_DESERT,
	SONG_JUNGLE,
	SONG_THEME,
	SONG_ATLANTIS,
	SONG_CHINA,
	SONG_CRETE,
	SONG_EUROPE,
	SONG_ICE,
	SONG_VIKING,
	SONG_EGYPT,
	MAX_SONGS
};



/***************** EFFECTS *************************/
// This table must match gEffectsTable
//

enum
{
		/* DEFAULT */

	EFFECT_BADSELECT = 0,
	EFFECT_SKID3,
	EFFECT_ENGINE,
	EFFECT_SKID,
	EFFECT_CRASH,
	EFFECT_BOOM,
	EFFECT_NITRO,
	EFFECT_ROMANCANDLE_LAUNCH,
	EFFECT_ROMANCANDLE_FALL,
	EFFECT_SKID2,
	EFFECT_SELECTCLICK,
	EFFECT_GETPOW,
	EFFECT_CANNON,
	EFFECT_CRASH2,
	EFFECT_SPLASH,
	EFFECT_BIRDCAW,
	EFFECT_SNOWBALL,
	EFFECT_MINE,
	EFFECT_THROW1,
	EFFECT_THROW2,
	EFFECT_THROW3,

			/* DESERT */

	EFFECT_DUSTDEVIL,

			/* JUNGLE */

	EFFECT_BLOWDART,

			/* ICE */

	EFFECT_HITSNOW,


			/* EUROPE */

	EFFECT_CATAPULT,

			/* CHINA */

	EFFECT_GONG,

			/* EGYPT */

	EFFECT_SHATTER,

			/* ATLANTIS */

	EFFECT_ZAP,
	EFFECT_TORPEDOFIRE,
	EFFECT_HUM,
	EFFECT_BUBBLES,

			/* STONEHENGE */

	EFFECT_CHANT,

			/* ANNOUNCER */

	EFFECT_READY,
	EFFECT_SET,
	EFFECT_GO,
	EFFECT_THATSALL,
	EFFECT_GOODJOB,
	EFFECT_REDTEAMWINS,
	EFFECT_GREENTEAMWINS,
	EFFECT_LAP2,
	EFFECT_FINALLAP,
	EFFECT_NICESHOT,
	EFFECT_GOTTAHURT,
	EFFECT_1st,
	EFFECT_2nd,
	EFFECT_3rd,
	EFFECT_4th,
	EFFECT_5th,
	EFFECT_6th,
	EFFECT_OHYEAH,
	EFFECT_NICEDRIVING,
	EFFECT_WOAH,
	EFFECT_YOUWIN,
	EFFECT_YOULOSE,
	EFFECT_POW_BONEBOMB,
	EFFECT_POW_OIL,
	EFFECT_POW_NITRO,
	EFFECT_POW_PIGEON,
	EFFECT_POW_CANDLE,
	EFFECT_POW_ROCKET,
	EFFECT_POW_TORPEDO,
	EFFECT_POW_FREEZE,
	EFFECT_POW_MINE,
	EFFECT_POW_SUSPENSION,
	EFFECT_POW_INVISIBILITY,
	EFFECT_POW_STICKYTIRES,
	EFFECT_ARROWHEAD,
	EFFECT_COMPLETED,
	EFFECT_INCOMPLETE,
	EFFECT_COSTYA,
	EFFECT_WATCHIT,
	NUM_EFFECTS
};



/**********************/
/* SOUND BANK TABLES  */
/**********************/

enum
{
	SOUNDBANK_MAIN,
	SOUNDBANK_LEVELSPECIFIC,
	SOUNDBANK_ANNOUNCER,
	NUM_SOUNDBANKS
};

//===================== PROTOTYPES ===================================


void	InitSoundTools(void);
void ShutdownSound(void);

void LoadSoundEffect(int effectNum);
void DisposeSoundEffect(int effectNum);
void LoadSoundBank(int bankNum);
void DisposeSoundBank(int bankNum);
void DisposeAllSoundBanks(void);

void StopAChannel(short *channelNum);
extern	void StopAllEffectChannels(void);
void PlaySong(short songNum, Boolean loopFlag);
extern void	KillSong(void);
extern	short PlayEffect(short effectNum);
short PlayEffect_Parms3D(short effectNum, const OGLPoint3D *where, uint32_t rateMultiplier, float volumeAdjust);
void	ToggleMusic(void);
extern void	DoSoundMaintenance(void);
extern	void WaitEffectsSilent(void);
short PlayEffect_Parms(short effectNum, uint32_t leftVolume, uint32_t rightVolume, unsigned long rateMultiplier);
void ChangeChannelVolume(short channel, uint32_t leftVol, uint32_t rightVol);
short PlayEffect3D(short effectNum, const OGLPoint3D *where);
void Update3DSoundChannel(short effectNum, short *channel, const OGLPoint3D *where);
Boolean IsEffectChannelPlaying(short chanNum);
void UpdateListenerLocation(OGLSetupOutputType *setupInfo);
void ChangeChannelRate(short channel, long rateMult);
void StopAChannelIfEffectNum(short *channelNum, short effectNum);
void UpdateGlobalVolume(void);

void PlayAnnouncerSound(short effectNum, Boolean override, float delay);





