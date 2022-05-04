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
	char	playerName[64];
	Byte	numTracksCompleted;

	Byte	difficulty;
	Byte	desiredSplitScreenMode;
	Byte	language;
	Byte	tagDuration;		// legal values: 2,3,4
	Byte	antialiasingLevel;
	Boolean	fullscreen;
	Boolean	music;
	Byte	monitorNum;

	KeyBinding keys[NUM_CONTROL_NEEDS];
	Boolean	gamepadRumble;
}PrefsType;

#define PREFS_MAGIC "CMR Prefs v0"
#define PREFS_FOLDER_NAME "CroMagRally"
#define PREFS_FILE_PATH (":" PREFS_FOLDER_NAME ":Prefs")



		/* COMMAND-LINE OPTIONS */

typedef struct
{
	int		msaa;
	int		vsync;
	int		bootToTrack;
} CommandLineOptions;

//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo);
extern	void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
void SavePrefs(void);
void LoadPlayfield(FSSpec *specPtr);
void PreloadGameArt(OGLSetupOutputType *setupInfo);
void LoadLevelArt(OGLSetupOutputType *setupInfo);
void LoadCavemanSkins(OGLSetupOutputType *setupInfo);
void DisposeCavemanSkins(void);
void SetDefaultDirectory(void);

void SetDefaultPlayerSaveData(void);
void SavePlayerFile(void);
int GetNumAgesCompleted(void);
int GetNumStagesCompletedInAge(void);
int GetNumTracksCompletedTotal(void);
int GetTrackNumFromAgeStage(int age, int stage);
void SetPlayerProgression(int numTracksCompleted);

Ptr LoadDataFile(const char* path, long* outLength);
char* LoadTextFile(const char* path, long* outLength);

#define BYTESWAP_HANDLE(format, type, n, handle)                                  \
{                                                                                 \
	if ((n) * sizeof(type) > (unsigned long) GetHandleSize((Handle) (handle)))   \
		DoFatalAlert("BYTESWAP_HANDLE: size mismatch!\nHdl=%ld Exp=%ld\nWhen swapping %dx %s in %s:%d", \
			GetHandleSize((Handle) (handle)), \
			(n) * sizeof(type), \
			n, #type, __func__, __LINE__); \
	ByteswapStructs((format), sizeof(type), (n), *(handle));                      \
}
