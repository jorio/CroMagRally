//
// file.h
//

#pragma once

#include "input.h"
#include "main.h"
#include "structs.h"

		/***********************/
		/* RESOURCE STURCTURES */
		/***********************/

			/* Hedr */

typedef struct
{
	int16_t	version;			// 0xaa.bb
	int16_t	numAnims;			// gNumAnims
	int16_t	numJoints;			// gNumJoints
	int16_t	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.

typedef struct
{
	int32_t				parentBone;			 		// index to previous bone
	uint8_t				name[32];					// text string name for bone
	OGLPoint3D			coord;						// absolute coord (not relative to parent!)
	uint16_t			numPointsAttachedToBone;	// # vertices/points that this bone has
	uint16_t			numNormalsAttachedToBone;	// # vertex normals this bone has
	uint32_t			reserved[8];				// reserved for future use
}File_BoneDefinitionType;



			/* AnHd */

typedef struct
{
	Str32	animName;
	int16_t	numAnimEvents;
}SkeletonFile_AnimHeader_Type;



		/* PREFERENCES */

typedef struct
{
	Byte	numTracksCompleted;
	float	tournamentLapTimes[NUM_RACE_TRACKS][LAPS_PER_RACE];
} TournamentProgression;

typedef struct
{
	Byte	difficulty;
	Byte	splitScreenMode2P;
	Byte	splitScreenMode3P;
	Byte	language;
	Byte	tagDuration;		// legal values: 2,3,4
	Byte	antialiasingLevel;
	Boolean	fullscreen;
	Byte	monitorNum;
	Byte	musicVolumePercent;
	Byte	sfxVolumePercent;

	InputBinding bindings[NUM_CONTROL_NEEDS];
	Boolean	gamepadRumble;

	TournamentProgression tournamentProgression;
	char	playerName[32];
}PrefsType;

#define PREFS_FOLDER_NAME "CroMagRally"

#define PREFS_MAGIC "CMR Prefs v0   "
#define PREFS_FILENAME "Prefs"


// Probably storing more info than necessary, but it's there if we ever want to make a super-detailed scoreboard
typedef struct
{
	int64_t		timestamp;
	float		lapTimes[LAPS_PER_RACE];
	Byte		trackNum;
	Byte		difficulty;
	Byte		gameMode;
	Byte		vehicleType;
	Byte		place;
	Byte		sex;
	Byte		skin;
	char		reserved[32+5];		// pad to 64 bytes & make room for player name (if we want to add this later)
} ScoreboardRecord;

typedef struct
{
	ScoreboardRecord records[NUM_RACE_TRACKS][MAX_RECORDS_PER_TRACK];
} Scoreboard;

#define SCOREBOARD_MAGIC "CMR Scores v0  "


		/* COMMAND-LINE OPTIONS */

typedef struct
{
	int		vsync;
	int		bootToTrack;
} CommandLineOptions;

//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType);

OSErr LoadUserDataFile(const char* path, const char* magic, long payloadLength, Ptr payloadPtr);
OSErr SaveUserDataFile(const char* path, const char* magic, long payloadLength, Ptr payloadPtr);

OSErr LoadPrefs(void);
void SavePrefs(void);
void LoadPlayfield(FSSpec *specPtr);
void PreloadGameArt(void);
void LoadLevelArt(void);
void LoadCavemanSkins(void);
void DisposeCavemanSkins(void);
void SetDefaultDirectory(void);

void SetDefaultPlayerSaveData(void);
void SavePlayerFile(void);
int GetNumAgesCompleted(void);
int GetNumStagesCompletedInAge(void);
int GetNumTracksCompletedTotal(void);
int GetTrackNumFromAgeStage(int age, int stage);
float GetTotalTournamentTime(void);
void SetPlayerProgression(int numTracksCompleted);

Ptr LoadDataFile(const char* path, long* outLength);
char* LoadTextFile(const char* path, long* outLength);

OSErr SaveScoreboardFile(void);
OSErr LoadScoreboardFile(void);

#define BYTESWAP_HANDLE(format, type, n, handle)                                  \
{                                                                                 \
	if ((n) * sizeof(type) > (unsigned long) GetHandleSize((Handle) (handle)))   \
		DoFatalAlert("BYTESWAP_HANDLE: size mismatch!\nHdl=%ld Exp=%ld\nWhen swapping %dx %s in %s:%d", \
			GetHandleSize((Handle) (handle)), \
			(n) * sizeof(type), \
			n, #type, __func__, __LINE__); \
	ByteswapStructs((format), sizeof(type), (n), *(handle));                      \
}
