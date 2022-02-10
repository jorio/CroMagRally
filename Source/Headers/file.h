//
// file.h
//

#include "input.h"

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

#define	MAX_HTTP_NOTES	1000

typedef struct
{
	Byte	difficulty;
	Byte	desiredSplitScreenMode;
	Boolean	showScreenModeDialog;
	short	depth;
	int		screenWidth;
	int		screenHeight;
	float	screenCrop;
	Byte	language;
	Byte	tagDuration;
	double	videoHz;
	Byte	monitorNum;
	u_short	keySettings[NUM_CONTROL_NEEDS];
//	DateTimeRec	lastVersCheckDate;
//	Byte	didThisNote[MAX_HTTP_NOTES];
	Byte	reserved[8];
}PrefsType;



		/* SAVE PLAYER */

typedef struct
{
	Byte		numAgesCompleted;		// encode # ages in lower 4 bits, and stage in upper 4 bits
	Str255		playerName;
}SavePlayerType;





//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo);
extern	void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr);
void LoadLevelArt(OGLSetupOutputType *setupInfo);
OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth);
void GetDemoTimer(void);
void SaveDemoTimer(void);
void SetDefaultDirectory(void);

void SetDefaultPlayerSaveData(void);
void DoSavedPlayerDialog(void);
void SavePlayerFile(void);

Ptr LoadFileData(const FSSpec* spec, long* outLength);
