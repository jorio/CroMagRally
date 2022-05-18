/****************************/
/*      FILE ROUTINES       */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include "bones.h"
#include "lzss.h"
#include <string.h>

/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType);
static void ReadDataFromPlayfieldFile(FSSpec *specPtr);
static void LoadTerrainSuperTileTextures(short fRefNum);
static void LoadTerrainSuperTileTexturesSeamless(short fRefNum);

static short InitSavedGamesListBox(Rect *r, WindowPtr myDialog);
static short UpdateSavedGamesList(void);
static short AddNewGameToFile(Str255 name);
static void DeleteSelectedGame(void);
static void LoadSelectedGame(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BASE_PATH_TILE		900					// tile # of 1st path tile

#define	PICT_HEADER_SIZE	512

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1

#define	SAVE_PLAYER_VERSION	0x0100		// 1.0



		/* PLAYFIELD HEADER */

typedef struct
{
	NumVersion	version;							// version of file
	int32_t		numItems;							// # items in map
	int32_t		mapWidth;							// width of map
	int32_t		mapHeight;							// height of map
	int32_t		numTilePages;						// # tile pages
	int32_t		numTilesInList;						// # extracted tiles in list
	float		tileSize;							// 3D unit size of a tile
	float		minY;								// min height value
	float 		maxY;								// max height value
	int32_t		numSplines;							// # splines
	int32_t		numFences;							// # fences
	int32_t		numUniqueSuperTiles;				// # unique supertile
	int32_t		numPaths;                           // # paths
	int32_t		numCheckpoints;						// # checkpoints
}PlayfieldHeaderType;


		/* FENCE STRUCTURE IN FILE */
		//
		// note: we copy this data into our own fence list
		//		since the game uses a slightly different
		//		data structure.
		//

typedef	struct
{
	int32_t		x,z;
}FencePointType;


typedef struct
{
	uint16_t		type;				// type of fence
	int16_t			numNubs;			// # nubs in fence
//	FencePointType	**nubList;			// handle to nub list
	uint32_t		_junkPtr;
	Rect			unusedbBox;				// bounding box of fence area
}FileFenceDefType;

/**********************/
/*     VARIABLES      */
/**********************/

float	g3DTileSize, g3DMinY, g3DMaxY;

MOMaterialObject* gCavemanSkins[2][NUM_CAVEMAN_SKINS];

PrefsType gDiskShadowPrefs;


/****************** SET DEFAULT DIRECTORY ********************/
//
// This function needs to be called for OS X because OS X doesnt automatically
// set the default directory to the application directory.
//

void SetDefaultDirectory(void)
{
}



/******************* LOAD SKELETON *******************/
//
// Loads a skeleton file & creates storage for it.
//
// NOTE: Skeleton types 0..NUM_CHARACTERS-1 are reserved for player character skeletons.
//		Skeleton types NUM_CHARACTERS and over are for other skeleton entities.
//
// OUTPUT:	Ptr to skeleton data
//

SkeletonDefType *LoadSkeletonFile(short skeletonType)
{
static const char* kSkeletonNames[MAX_SKELETON_TYPES] =
{
	[SKELETON_TYPE_PLAYER_MALE]		= "brog",
	[SKELETON_TYPE_PLAYER_FEMALE]	= "grag",
	[SKELETON_TYPE_BIRDBOMB]		= "birdbomb",
	[SKELETON_TYPE_YETI]			= "yeti",
	[SKELETON_TYPE_BEETLE]			= "beetle",
	[SKELETON_TYPE_CAMEL]			= "camel",
	[SKELETON_TYPE_CATAPULT]		= "catapult",
	[SKELETON_TYPE_BRONTONECK]		= "brontoneck",
	[SKELETON_TYPE_SHARK]			= "shark",
	[SKELETON_TYPE_FLAG]			= "flag",
	[SKELETON_TYPE_PTERADACTYL]		= "pteradactyl",
	[SKELETON_TYPE_MALESTANDING]	= "brogstanding",
	[SKELETON_TYPE_FEMALESTANDING]	= "gragstanding",
	[SKELETON_TYPE_DRAGON]			= "dragon",
	[SKELETON_TYPE_MUMMY]			= "mummy",
	[SKELETON_TYPE_TROLL]			= "troll",
	[SKELETON_TYPE_DRUID]			= "druid",
	[SKELETON_TYPE_POLARBEAR]		= "polarbear",
	[SKELETON_TYPE_FLOWER]			= "flower",
	[SKELETON_TYPE_VIKING]			= "viking",
};

OSErr		iErr;
short		fRefNum;
FSSpec		fsSpec;
SkeletonDefType	*skeleton;

				/* SET CORRECT FILENAME */

	GAME_ASSERT(skeletonType >= 0 && skeletonType < MAX_SKELETON_TYPES);

	char path[256];
	snprintf(path, sizeof(path), ":skeletons:%s.skeleton", kSkeletonNames[skeletonType]);
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &fsSpec);

			/* OPEN THE FILE'S REZ FORK */

	fRefNum = FSpOpenResFile(&fsSpec,fsRdPerm);
	if (fRefNum == -1)
	{
		iErr = ResError();
		DoAlert("Error opening Skel Rez file");
		ShowSystemErr(iErr);
	}

	UseResFile(fRefNum);
	if (ResError())
		DoFatalAlert("Error using Rez file!");


			/* ALLOC MEMORY FOR SKELETON INFO STRUCTURE */

	skeleton = (SkeletonDefType *)AllocPtr(sizeof(SkeletonDefType));
	if (skeleton == nil)
		DoFatalAlert("Cannot alloc SkeletonInfoType");


			/* READ SKELETON RESOURCES */

	ReadDataFromSkeletonFile(skeleton,&fsSpec,skeletonType);
	PrimeBoneData(skeleton);

			/* CLOSE REZ FILE */

	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);

	return(skeleton);
}


/************* READ DATA FROM SKELETON FILE *******************/
//
// Current rez file is set to the file.
//

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType)
{
Handle				hand;
int					i,k,j;
long				numJoints,numAnims,numKeyframes;
AnimEventType		*animEventPtr;
JointKeyframeType	*keyFramePtr;
SkeletonFile_Header_Type	*headerPtr;
short				version;
AliasHandle				alias;
OSErr					iErr;
FSSpec					target;
Boolean					wasChanged;
OGLPoint3D				*pointPtr;
SkeletonFile_AnimHeader_Type	*animHeaderPtr;


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	if (hand == nil)
		DoFatalAlert("ReadDataFromSkeletonFile: Error reading header resource!");
	BYTESWAP_HANDLE("4h", SkeletonFile_Header_Type, 1, hand);
	headerPtr = (SkeletonFile_Header_Type *)*hand;
	version = headerPtr->version;
	if (version != SKELETON_FILE_VERS_NUM)
		DoFatalAlert("Skeleton file has wrong version #");

	numAnims = skeleton->NumAnims = headerPtr->numAnims;			// get # anims in skeleton
	numJoints = skeleton->NumBones = headerPtr->numJoints;			// get # joints in skeleton
	ReleaseResource(hand);

	if (numJoints > MAX_JOINTS)										// check for overload
		DoFatalAlert("ReadDataFromSkeletonFile: numJoints > MAX_JOINTS");


				/*************************************/
				/* ALLOCATE MEMORY FOR SKELETON DATA */
				/*************************************/

	AllocSkeletonDefinitionMemory(skeleton);



		/********************************/
		/* 	LOAD THE REFERENCE GEOMETRY */
		/********************************/

	alias = (AliasHandle)GetResource(rAliasType,1001);				// alias to geometry BG3D file
	if (alias != nil)
	{
		iErr = ResolveAlias(fsSpec, alias, &target, &wasChanged);	// try to resolve alias
		if (!iErr)
			LoadBonesReferenceModel(&target,skeleton, skeletonType);
		else
			DoFatalAlert("ReadDataFromSkeletonFile: Cannot find Skeleton's 3DMF file!");
		ReleaseResource((Handle)alias);
	}


		/***********************************/
		/*  READ BONE DEFINITION RESOURCES */
		/***********************************/

	for (i=0; i < numJoints; i++)
	{
		File_BoneDefinitionType	*bonePtr;
		uint16_t				*indexPtr;

			/* READ BONE DATA */

		hand = GetResource('Bone',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading Bone resource!");
		HLock(hand);
		BYTESWAP_HANDLE("i32b3fHH8L", File_BoneDefinitionType, 1, hand);
		bonePtr = (File_BoneDefinitionType *)*hand;

			/* COPY BONE DATA INTO ARRAY */

		skeleton->Bones[i].parentBone = bonePtr->parentBone;								// index to previous bone
		skeleton->Bones[i].coord = bonePtr->coord;											// absolute coord (not relative to parent!)
		skeleton->Bones[i].numPointsAttachedToBone = bonePtr->numPointsAttachedToBone;		// # vertices/points that this bone has
		skeleton->Bones[i].numNormalsAttachedToBone = bonePtr->numNormalsAttachedToBone;	// # vertex normals this bone has
		ReleaseResource(hand);

			/* ALLOC THE POINT & NORMALS SUB-ARRAYS */

		skeleton->Bones[i].pointList = (uint16_t *)AllocPtr(sizeof(uint16_t) * (int)skeleton->Bones[i].numPointsAttachedToBone);
		if (skeleton->Bones[i].pointList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/pointList failed!");

		skeleton->Bones[i].normalList = (uint16_t *)AllocPtr(sizeof(uint16_t) * (int)skeleton->Bones[i].numNormalsAttachedToBone);
		if (skeleton->Bones[i].normalList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/normalList failed!");

			/* READ POINT INDEX ARRAY */

		hand = GetResource('BonP',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonP resource!");
		HLock(hand);
		indexPtr = (uint16_t *)(*hand);

			/* COPY POINT INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numPointsAttachedToBone; j++)
			skeleton->Bones[i].pointList[j] = Byteswap16(&indexPtr[j]);
		ReleaseResource(hand);


			/* READ NORMAL INDEX ARRAY */

		hand = GetResource('BonN',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonN resource!");
		HLock(hand);
		indexPtr = (uint16_t *)(*hand);

			/* COPY NORMAL INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numNormalsAttachedToBone; j++)
			skeleton->Bones[i].normalList[j] = Byteswap16(&indexPtr[j]);
		ReleaseResource(hand);

	}


		/*******************************/
		/* READ POINT RELATIVE OFFSETS */
		/*******************************/
		//
		// The "relative point offsets" are the only things
		// which do not get rebuilt in the ModelDecompose function.
		// We need to restore these manually.

	hand = GetResource('RelP', 1000);
	if (hand == nil)
		DoFatalAlert("Error reading RelP resource!");
	HLock(hand);
	pointPtr = (OGLPoint3D *)*hand;

	i = GetHandleSize(hand) / sizeof(OGLPoint3D);
	BYTESWAP_HANDLE("fff", OGLPoint3D, skeleton->numDecomposedPoints, hand);

	if (i != skeleton->numDecomposedPoints)
		DoFatalAlert("# of points in Reference Model has changed!");
	else
	{
		for (i = 0; i < skeleton->numDecomposedPoints; i++)
			skeleton->decomposedPointList[i].boneRelPoint = pointPtr[i];
	}

	ReleaseResource(hand);


			/*********************/
			/* READ ANIM INFO   */
			/*********************/

	for (i=0; i < numAnims; i++)
	{
				/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);
		if (hand == nil)
			DoFatalAlert("Error getting anim header resource");
		HLock(hand);
		BYTESWAP_HANDLE("b32bxh", SkeletonFile_AnimHeader_Type, 1, hand);
		animHeaderPtr = (SkeletonFile_AnimHeader_Type *)*hand;

		skeleton->NumAnimEvents[i] = animHeaderPtr->numAnimEvents;			// copy # anim events in anim
		ReleaseResource(hand);

			/* READ ANIM-EVENT DATA */

		hand = GetResource('Evnt',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading anim-event data resource!");
		BYTESWAP_HANDLE("hbb", AnimEventType, skeleton->NumAnimEvents[i], hand);
		animEventPtr = (AnimEventType *)*hand;
		for (j=0;  j < skeleton->NumAnimEvents[i]; j++)
			skeleton->AnimEventsList[i][j] = *animEventPtr++;
		ReleaseResource(hand);


			/* READ # KEYFRAMES PER JOINT IN EACH ANIM */

		hand = GetResource('NumK',1000+i);									// read array of #'s for this anim
		if (hand == nil)
			DoFatalAlert("Error reading # keyframes/joint resource!");
		for (j=0; j < numJoints; j++)
			skeleton->JointKeyframes[j].numKeyFrames[i] = (*hand)[j];
		ReleaseResource(hand);
	}


	for (j=0; j < numJoints; j++)
	{
				/* ALLOC 2D ARRAY FOR KEYFRAMES */

		Alloc_2d_array(JointKeyframeType,skeleton->JointKeyframes[j].keyFrames,	numAnims,MAX_KEYFRAMES);

		if ((skeleton->JointKeyframes[j].keyFrames == nil) || (skeleton->JointKeyframes[j].keyFrames[0] == nil))
			DoFatalAlert("ReadDataFromSkeletonFile: Error allocating Keyframe Array.");

					/* READ THIS JOINT'S KF'S FOR EACH ANIM */

		for (i=0; i < numAnims; i++)
		{
			numKeyframes = skeleton->JointKeyframes[j].numKeyFrames[i];					// get actual # of keyframes for this joint
			if (numKeyframes > MAX_KEYFRAMES)
				DoFatalAlert("Error: numKeyframes > MAX_KEYFRAMES");

					/* READ A JOINT KEYFRAME */

			hand = GetResource('KeyF',1000+(i*100)+j);
			if (hand == nil)
				DoFatalAlert("Error reading joint keyframes resource!");
			BYTESWAP_HANDLE("ii3f3f3f", JointKeyframeType, numKeyframes, hand);
			keyFramePtr = (JointKeyframeType *)*hand;
			for (k = 0; k < numKeyframes; k++)												// copy this joint's keyframes for this anim
				skeleton->JointKeyframes[j].keyFrames[i][k] = *keyFramePtr++;
			ReleaseResource(hand);
		}
	}

}

#pragma mark -

static OSErr MakeFSSpecForUserDataFile(const char* filename, FSSpec* spec)
{
	char path[256];
	snprintf(path, sizeof(path), ":%s:%s", PREFS_FOLDER_NAME, filename);

	return FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, path, spec);
}

/********* LOAD STRUCT FROM USER FILE IN PREFS FOLDER ***********/

OSErr LoadUserDataFile(const char* filename, const char* magic, long payloadLength, Ptr payloadPtr)
{
OSErr		iErr;
short		refNum;
FSSpec		file;
long		count;
long		eof = 0;
char		fileMagic[64];
long		magicLength = strlen(magic) + 1;		// including null-terminator

	GAME_ASSERT(magicLength < (long) sizeof(fileMagic));

	InitPrefsFolder(false);


				/*************/
				/* READ FILE */
				/*************/

	MakeFSSpecForUserDataFile(filename, &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr)
		return iErr;

				/* CHECK FILE LENGTH */

	GetEOF(refNum, &eof);

	if (eof != magicLength + payloadLength)
		goto fileIsCorrupt;

				/* READ HEADER */

	count = magicLength;
	iErr = FSRead(refNum, &count, fileMagic);
	if (iErr ||
		count != magicLength ||
		0 != strncmp(magic, fileMagic, magicLength-1))
	{
		goto fileIsCorrupt;
	}

				/* READ PAYLOAD */

	count = payloadLength;
	iErr = FSRead(refNum, &count, payloadPtr);
	if (iErr || count != payloadLength)
	{
		goto fileIsCorrupt;
	}

	FSClose(refNum);
	return noErr;

fileIsCorrupt:
	printf("File '%s' appears to be corrupt!\n", file.cName);
	FSClose(refNum);
	return badFileFormat;
}


/********* SAVE STRUCT TO USER FILE IN PREFS FOLDER ***********/

OSErr SaveUserDataFile(const char* filename, const char* magic, long payloadLength, Ptr payloadPtr)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

	InitPrefsFolder(true);

				/* CREATE BLANK FILE */

	MakeFSSpecForUserDataFile(filename, &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'CavM', 'Pref', smSystemScript);					// create blank file
	if (iErr)
	{
		return iErr;
	}

				/* OPEN FILE */

	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
		FSpDelete(&file);
		return iErr;
	}

				/* WRITE MAGIC */

	count = strlen(magic) + 1;
	iErr = FSWrite(refNum, &count, (Ptr) magic);
	if (iErr)
	{
		FSClose(refNum);
		return iErr;
	}

				/* WRITE DATA */

	count = payloadLength;
	iErr = FSWrite(refNum, &count, payloadPtr);
	FSClose(refNum);

	memcpy(&gDiskShadowPrefs, &gGamePrefs, sizeof(gGamePrefs));

	printf("Wrote %s\n", file.cName);

	return iErr;
}


#pragma mark -

/******************** LOAD PREFS **********************/
//
// Load in standard preferences
//

OSErr LoadPrefs(void)
{
	OSErr err = LoadUserDataFile("Prefs", PREFS_MAGIC, sizeof(PrefsType), (Ptr) &gGamePrefs);

	if (err != noErr)
	{
		InitDefaultPrefs();
	}

	memcpy(&gDiskShadowPrefs, &gGamePrefs, sizeof(gDiskShadowPrefs));

	return err;
}


/******************** SAVE PREFS **********************/

void SavePrefs(void)
{
#if _DEBUG
	// If prefs didn't change relative to what's on disk, don't bother rewriting them
	if (0 == memcmp(&gDiskShadowPrefs, &gGamePrefs, sizeof(gGamePrefs)))
	{
		return;
	}

#endif

	SaveUserDataFile("Prefs", PREFS_MAGIC, sizeof(PrefsType), (Ptr)&gGamePrefs);

	memcpy(&gDiskShadowPrefs, &gGamePrefs, sizeof(gGamePrefs));
}

#pragma mark -


/**************** SET DEFAULT PLAYER SAVE DATA **********************/
//
// Called at boot to initialize the currently "loaded" player since no saved
// player is loaded
//

void SetDefaultPlayerSaveData(void)
{
	memset(gGamePrefs.playerName, 0, sizeof(gGamePrefs.playerName));
	snprintf(gGamePrefs.playerName, sizeof(gGamePrefs.playerName), "PLAYER");

	SetPlayerProgression(0);
}


/*********************** SAVE PLAYER FILE *******************************/

void SavePlayerFile(void)
{
	SavePrefs();
}


/*********************** GET/SET PROGRESSION *******************************/


int GetNumAgesCompleted(void)
{
	return GAME_MIN(NUM_AGES, gGamePrefs.numTracksCompleted / TRACKS_PER_AGE);
}

int GetNumStagesCompletedInAge(void)
{
	return gGamePrefs.numTracksCompleted % TRACKS_PER_AGE;
}

int GetNumTracksCompletedTotal(void)
{
	return gGamePrefs.numTracksCompleted;
}

int GetTrackNumFromAgeStage(int age, int stage)
{
	return age * TRACKS_PER_AGE + stage;
}

void SetPlayerProgression(int numTracksCompleted)
{
	gGamePrefs.numTracksCompleted = numTracksCompleted;
}


#pragma mark -

/*********************** SCOREBOARD FILE *******************************/

OSErr SaveScoreboardFile(void)
{
	_Static_assert(sizeof(ScoreboardRecord) == 64, "Ideally, a ScoreboardRecord should be 64 bytes long");
	return SaveUserDataFile(
			"Scoreboard",
			SCOREBOARD_MAGIC,
			sizeof(gScoreboard),
			(Ptr) &gScoreboard);
}

OSErr LoadScoreboardFile(void)
{
	OSErr err = LoadUserDataFile(
			"Scoreboard",
			SCOREBOARD_MAGIC,
			sizeof(gScoreboard),
			(Ptr) &gScoreboard);

	if (err != noErr)
	{
		memset(&gScoreboard, 0, sizeof(gScoreboard));
	}

	return err;
}

#pragma mark -


void LoadCavemanSkins(void)
{
	char skinPath[256];

	for (int sex = 0; sex < 2; sex++)
	{
		const char* characterName = sex==0? "brog": "grag";

		for (int j = 0; j < NUM_CAVEMAN_SKINS; j++)
		{
			if (!gCavemanSkins[sex][j])
			{
				snprintf(skinPath, sizeof(skinPath), ":sprites:skins:%s%d.png", characterName, j);
				gCavemanSkins[sex][j] = MO_GetTextureFromFile(skinPath, GL_RGBA);
			}
		}
	}
}


void DisposeCavemanSkins(void)
{
	for (int sex = 0; sex < 2; sex++)
	{
		for (int i = 0; i < NUM_CAVEMAN_SKINS; i++)
		{
			if (gCavemanSkins[sex][i])
			{
				MO_DisposeObjectReference(gCavemanSkins[sex][i]);
				gCavemanSkins[sex][i] = NULL;
			}
		}
	}
}

#pragma mark -

void PreloadGameArt(void)
{
	LoadCavemanSkins();
	LoadSpriteGroup(SPRITE_GROUP_INFOBAR, "infobar", 0);
	LoadSpriteGroup(SPRITE_GROUP_EFFECTS, "effects", 0);
	LoadSoundBank(SOUNDBANK_MAIN);
	LoadSoundBank(SOUNDBANK_ANNOUNCER);
}

/************************** LOAD LEVEL ART ***************************/

void LoadLevelArt(void)
{
FSSpec	spec;

static const char*	terrainFiles[NUM_TRACKS] =
{
	":terrain:StoneAge_Desert.ter",
	":terrain:StoneAge_Jungle.ter",
	":terrain:StoneAge_Ice.ter",

	":terrain:BronzeAge_Crete.ter",
	":terrain:BronzeAge_China.ter",
	":terrain:BronzeAge_Egypt.ter",

	":terrain:IronAge_Europe.ter",
	":terrain:IronAge_Scandinavia.ter",
	":terrain:IronAge_Atlantis.ter",

	":terrain:Battle_StoneHenge.ter",
	":terrain:Battle_Aztec.ter",
	":terrain:Battle_Coliseum.ter",
	":terrain:Battle_Maze.ter",
	":terrain:Battle_Celtic.ter",
	":terrain:Battle_TarPits.ter",
	":terrain:Battle_Spiral.ter",
	":terrain:Battle_Ramps.ter",
};

static const char*	levelModelFiles[NUM_TRACKS] =
{
	":models:desert.bg3d",
	":models:jungle.bg3d",
	":models:ice.bg3d",

	":models:crete.bg3d",
	":models:china.bg3d",
	":models:egypt.bg3d",

	":models:europe.bg3d",
	":models:scandinavia.bg3d",
	":models:atlantis.bg3d",

	":models:stonehenge.bg3d",
	":models:aztec.bg3d",
	":models:coliseum.bg3d",
	":models:crete.bg3d",
	":models:crete.bg3d",
	":models:tarpits.bg3d",
	":models:crete.bg3d",
	":models:ramps.bg3d"
};


	GAME_ASSERT_MESSAGE((size_t)gTrackNum < (size_t)NUM_TRACKS, "illegal track#!");

				/* LOAD AUDIO */


	switch(gTrackNum)
	{
		case	TRACK_NUM_DESERT:
				LoadSoundEffect(EFFECT_DUSTDEVIL);
				break;

		case	TRACK_NUM_JUNGLE:
				LoadSoundEffect(EFFECT_BLOWDART);
				break;

		case	TRACK_NUM_ICE:
				LoadSoundEffect(EFFECT_HITSNOW);
				break;

		case	TRACK_NUM_CHINA:
				LoadSoundEffect(EFFECT_GONG);
				break;

		case	TRACK_NUM_EGYPT:
				LoadSoundEffect(EFFECT_SHATTER);
				break;

		case	TRACK_NUM_EUROPE:
				LoadSoundEffect(EFFECT_CATAPULT);
				break;

		case	TRACK_NUM_ATLANTIS:
				LoadSoundEffect(EFFECT_ZAP);
				LoadSoundEffect(EFFECT_TORPEDOFIRE);
				LoadSoundEffect(EFFECT_HUM);
				LoadSoundEffect(EFFECT_BUBBLES);
				break;

		case	TRACK_NUM_STONEHENGE:
				LoadSoundEffect(EFFECT_CHANT);
				break;
	}


			/* LOAD BG3D GEOMETRY */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:carparts.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_CARPARTS);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:weapons.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WEAPONS);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelModelFiles[gTrackNum], &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_PLAYER_MALE);
	LoadASkeleton(SKELETON_TYPE_PLAYER_FEMALE);

	LoadASkeleton(SKELETON_TYPE_BIRDBOMB);

	switch(gTrackNum)
	{
		case	TRACK_NUM_DESERT:
				LoadASkeleton(SKELETON_TYPE_BRONTONECK);
				break;

		case	TRACK_NUM_JUNGLE:
				LoadASkeleton(SKELETON_TYPE_PTERADACTYL);
				LoadASkeleton(SKELETON_TYPE_FLOWER);
				break;

		case	TRACK_NUM_ICE:
				LoadASkeleton(SKELETON_TYPE_YETI);
				LoadASkeleton(SKELETON_TYPE_POLARBEAR);
				break;

		case	TRACK_NUM_CHINA:
				LoadASkeleton(SKELETON_TYPE_DRAGON);
				break;

		case	TRACK_NUM_EGYPT:
				LoadASkeleton(SKELETON_TYPE_BEETLE);
				LoadASkeleton(SKELETON_TYPE_CAMEL);
				LoadASkeleton(SKELETON_TYPE_MUMMY);
				break;

		case	TRACK_NUM_EUROPE:
				LoadASkeleton(SKELETON_TYPE_CATAPULT);
				LoadASkeleton(SKELETON_TYPE_FLAG);
				break;

		case	TRACK_NUM_SCANDINAVIA:
				LoadASkeleton(SKELETON_TYPE_TROLL);
				LoadASkeleton(SKELETON_TYPE_VIKING);
				break;

		case	TRACK_NUM_ATLANTIS:
				LoadASkeleton(SKELETON_TYPE_SHARK);
				break;

		case	TRACK_NUM_STONEHENGE:
				LoadASkeleton(SKELETON_TYPE_DRUID);
				break;

		case	TRACK_NUM_CELTIC:
				LoadASkeleton(SKELETON_TYPE_FLAG);
				break;

	}


			/* DISPOSE MENU ASSETS THAT WE DON'T NEED IN-GAME */

	DisposeSpriteGroup(SPRITE_GROUP_MAINMENU);
	DisposePillarboxMaterial();

			/* LOAD SPRITES */


	// Ensure sprite groups are preloaded
PreloadGameArt();


			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, terrainFiles[gTrackNum], &spec);
	LoadPlayfield(&spec);
}


#pragma mark -

/******************* LOAD PLAYFIELD *******************/

void LoadPlayfield(FSSpec *specPtr)
{


			/* READ PLAYFIELD RESOURCES */

	ReadDataFromPlayfieldFile(specPtr);




				/***********************/
				/* DO ADDITIONAL SETUP */
				/***********************/

	CreateSuperTileMemoryList();				// allocate memory for the supertile geometry
	CalculateSplitModeMatrix();					// precalc the tile split mode matrix
	InitSuperTileGrid();						// init the supertile state grid

	BuildTerrainItemList();						// build list of items & find player start coords
	AssignPathVectorsToSuperTileGrid();         // put path vectors onto a supertile-based grid
}


/********************** READ DATA FROM PLAYFIELD FILE ************************/

static void ReadDataFromPlayfieldFile(FSSpec *specPtr)
{
Handle					hand;
PlayfieldHeaderType		**header;
long					row,col,j,i,size;
float					yScale;
short					fRefNum;
OSErr					iErr;
Ptr						tempBuffer16 = nil;

				/* OPEN THE REZ-FORK */

	fRefNum = FSpOpenResFile(specPtr,fsRdPerm);
	if (fRefNum == -1)
		DoFatalAlert("LoadPlayfield: FSpOpenResFile failed");
	UseResFile(fRefNum);


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	if (hand == nil)
	{
		DoAlert("ReadDataFromPlayfieldFile: Error reading header resource!");
		return;
	}

	header = (PlayfieldHeaderType **)hand;
	BYTESWAP_HANDLE("4b5i3f5i", PlayfieldHeaderType, 1, hand);
	gNumTerrainItems		= (**header).numItems;
	gTerrainTileWidth		= (**header).mapWidth;
	gTerrainTileDepth		= (**header).mapHeight;
	g3DTileSize				= (**header).tileSize;
	g3DMinY					= (**header).minY;
	g3DMaxY					= (**header).maxY;
	gNumSplines				= (**header).numSplines;
	gNumFences				= (**header).numFences;
	gNumUniqueSuperTiles	= (**header).numUniqueSuperTiles;
	gNumPaths              	= (**header).numPaths;
	gNumCheckpoints			= (**header).numCheckpoints;
	ReleaseResource(hand);

	if ((gTerrainTileWidth % SUPERTILE_SIZE) != 0)		// terrain must be non-fractional number of supertiles in w/h
		DoFatalAlert("ReadDataFromPlayfieldFile: terrain width not a supertile multiple");
	if ((gTerrainTileDepth % SUPERTILE_SIZE) != 0)
		DoFatalAlert("ReadDataFromPlayfieldFile: terrain depth not a supertile multiple");


				/* CALC SOME GLOBALS HERE */

	gTerrainTileWidth = (gTerrainTileWidth/SUPERTILE_SIZE)*SUPERTILE_SIZE;		// round size down to nearest supertile multiple
	gTerrainTileDepth = (gTerrainTileDepth/SUPERTILE_SIZE)*SUPERTILE_SIZE;
	gTerrainUnitWidth = gTerrainTileWidth*TERRAIN_POLYGON_SIZE;					// calc world unit dimensions of terrain
	gTerrainUnitDepth = gTerrainTileDepth*TERRAIN_POLYGON_SIZE;
	gNumSuperTilesDeep = gTerrainTileDepth/SUPERTILE_SIZE;						// calc size in supertiles
	gNumSuperTilesWide = gTerrainTileWidth/SUPERTILE_SIZE;


			/**************************/
			/* TILE RELATED RESOURCES */
			/**************************/


			/* READ TILE ATTRIBUTES */

	hand = GetResource('Atrb',1000);
	if (hand == nil)
		DoAlert("ReadDataFromPlayfieldFile: Error reading tile attrib resource!");
	else
	{
		DetachResource(hand);
		HLockHi(hand);
		gTileAttribList = (TileAttribType **)hand;

		Size numAttribs = GetHandleSize(hand) / sizeof(TileAttribType);
		BYTESWAP_HANDLE("Hbb", TileAttribType, numAttribs, hand);
	}


			/*******************************/
			/* SUPERTILE RELATED RESOURCES */
			/*******************************/

			/* READ SUPERTILE GRID MATRIX */

	if (gSuperTileTextureGrid)														// free old array
		Free_2d_array(gSuperTileTextureGrid);
	Alloc_2d_array(SuperTileGridType, gSuperTileTextureGrid, gNumSuperTilesDeep, gNumSuperTilesWide);

	hand = GetResource('STgd',1000);												// load grid from rez
	if (hand == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: Error reading supertile rez resource!");
	else																			// copy rez into 2D array
	{
		SuperTileGridType *src = (SuperTileGridType *)*hand;
		BYTESWAP_HANDLE("?xH", SuperTileGridType, gNumSuperTilesDeep*gNumSuperTilesWide, hand);

		for (row = 0; row < gNumSuperTilesDeep; row++)
			for (col = 0; col < gNumSuperTilesWide; col++)
			{
				gSuperTileTextureGrid[row][col] = *src++;
			}
		ReleaseResource(hand);
	}


			/*******************************/
			/* MAP LAYER RELATED RESOURCES */
			/*******************************/


			/* READ MAP DATA LAYER */
			//
			// The only thing we need this for is a quick way to
			// look up tile attributes
			//

	{
		uint16_t	*src;

		hand = GetResource('Layr',1000);
		if (hand == nil)
			DoAlert("ReadDataFromPlayfieldFile: Error reading map layer rez");
		else
		{
			if (gTileGrid)														// free old array
				Free_2d_array(gTileGrid);
			Alloc_2d_array(uint16_t, gTileGrid, gTerrainTileDepth, gTerrainTileWidth);

			src = (uint16_t *)*hand;
			BYTESWAP_HANDLE("H", uint16_t, gTerrainTileDepth*gTerrainTileWidth, hand);
			for (row = 0; row < gTerrainTileDepth; row++)
			{
				for (col = 0; col < gTerrainTileWidth; col++)
				{
					gTileGrid[row][col] = *src++;
				}
			}
			ReleaseResource(hand);
		}
	}


			/* READ HEIGHT DATA MATRIX */

	yScale = TERRAIN_POLYGON_SIZE / g3DTileSize;						// need to scale original geometry units to game units

	Alloc_2d_array(float, gMapYCoords, gTerrainTileDepth+1, gTerrainTileWidth+1);	// alloc 2D array for map

	hand = GetResource('YCrd',1000);
	if (hand == nil)
		DoAlert("ReadDataFromPlayfieldFile: Error reading height data resource!");
	else
	{
		float* src = (float *)*hand;
		BYTESWAP_HANDLE("f", float, (gTerrainTileDepth+1)*(gTerrainTileWidth+1), hand);
		for (row = 0; row <= gTerrainTileDepth; row++)
			for (col = 0; col <= gTerrainTileWidth; col++)
				gMapYCoords[row][col] = *src++ * yScale;
		ReleaseResource(hand);
	}

				/**************************/
				/* ITEM RELATED RESOURCES */
				/**************************/

				/* READ ITEM LIST */

	hand = GetResource('Itms',1000);
	if (hand == nil)
		DoAlert("ReadDataFromPlayfieldFile: Error reading itemlist resource!");
	else
	{
		DetachResource(hand);							// lets keep this data around
		HLockHi(hand);									// LOCK this one because we have the lookup table into this
		gMasterItemList = (TerrainItemEntryType **)hand;

		BYTESWAP_HANDLE("LLH4bH", TerrainItemEntryType, gNumTerrainItems, hand);
	}

				/* CONVERT COORDINATES */

	for (i = 0; i < gNumTerrainItems; i++)
	{
		(*gMasterItemList)[i].x *= MAP2UNIT_VALUE;
		(*gMasterItemList)[i].y *= MAP2UNIT_VALUE;
	}




			/****************************/
			/* SPLINE RELATED RESOURCES */
			/****************************/

			/* READ SPLINE LIST */

	hand = GetResource('Spln',1000);
	if (hand)
	{
		DetachResource(hand);
		HLockHi(hand);

		// SOURCE PORT NOTE: we have to convert this structure manually,
		// because the original contains 4-byte pointers
		BYTESWAP_HANDLE("hxxiiihxxi4h", File_SplineDefType, gNumSplines, hand);

		gSplineList = (SplineDefType **) NewHandleClear(gNumSplines * sizeof(SplineDefType));

		for (i = 0; i < gNumSplines; i++)
		{
			const File_SplineDefType*	srcSpline = &(*((File_SplineDefType **) hand))[i];
			SplineDefType*				dstSpline = &(*gSplineList)[i];

			dstSpline->numItems		= srcSpline->numItems;
//			dstSpline->numNubs		= srcSpline->numNubs;
			dstSpline->numPoints	= srcSpline->numPoints;
//			dstSpline->bBox			= srcSpline->bBox;
		}

		DisposeHandle(hand);
	}
	else
	{
		gNumSplines = 0;
		gSplineList = nil;
	}


			/* READ SPLINE POINT LIST */

	for (i = 0; i < gNumSplines; i++)
	{
		// If spline has 0 points, skip the byteswapping, but do alloc an empty handle, which the game expects.
		if ((*gSplineList)[i].numPoints == 0)
		{
#if _DEBUG
			DoAlert("WARNING: Spline #%ld has 0 points\n", i);
#endif
			(*gSplineList)[i].pointList = (SplinePointType**) AllocHandle(0);
			continue;
		}

		hand = GetResource('SpPt',1000+i);
		if (hand)
		{
			DetachResource(hand);
			HLockHi(hand);
			BYTESWAP_HANDLE("ff", SplinePointType, (*gSplineList)[i].numPoints, hand);
			(*gSplineList)[i].pointList = (SplinePointType **)hand;
		}
		else
			DoFatalAlert("ReadDataFromPlayfieldFile: cant get spline points rez");
	}


			/* READ SPLINE ITEM LIST */

	for (i = 0; i < gNumSplines; i++)
	{
		// If spline has 0 items, skip the byteswapping, but do alloc an empty handle, which the game expects.
		if ((*gSplineList)[i].numItems == 0)
		{
			DoAlert("WARNING: Spline #%ld has 0 items\n", i);
			(*gSplineList)[i].itemList = (SplineItemType**) AllocHandle(0);
			continue;
		}

		hand = GetResource('SpIt',1000+i);
		if (hand)
		{
			DetachResource(hand);
			HLockHi(hand);
			BYTESWAP_HANDLE("fH4bH", SplineItemType, (*gSplineList)[i].numItems, hand);
			(*gSplineList)[i].itemList = (SplineItemType **)hand;
		}
		else
			DoFatalAlert("ReadDataFromPlayfieldFile: cant get spline items rez");
	}

			/****************************/
			/* FENCE RELATED RESOURCES */
			/****************************/

			/* READ FENCE LIST */

	hand = GetResource('Fenc',1000);
	if (hand)
	{
		FileFenceDefType *inData;

		gFenceList = (FenceDefType *)AllocPtr(sizeof(FenceDefType) * gNumFences);	// alloc new ptr for fence data
		if (gFenceList == nil)
			DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed");

		BYTESWAP_HANDLE("Hhi4h", FileFenceDefType, gNumFences, hand);
		inData = (FileFenceDefType *)*hand;								// get ptr to input fence list

		for (i = 0; i < gNumFences; i++)								// copy data from rez to new list
		{
			gFenceList[i].type 		= inData[i].type;
			gFenceList[i].numNubs 	= inData[i].numNubs;
			gFenceList[i].nubList 	= nil;
			gFenceList[i].sectionVectors = nil;
		}
		ReleaseResource(hand);
	}
	else
	{
		gNumFences = 0;
		gFenceList = nil;
	}


			/* READ FENCE NUB LIST */

	for (i = 0; i < gNumFences; i++)
	{
		hand = GetResource('FnNb',1000+i);					// get rez
		HLock(hand);
		if (hand)
		{
			BYTESWAP_HANDLE("ii", FencePointType, gFenceList[i].numNubs, hand);
   			FencePointType *fileFencePoints = (FencePointType *)*hand;

			gFenceList[i].nubList = (OGLPoint3D *)AllocPtr(sizeof(FenceDefType) * gFenceList[i].numNubs);	// alloc new ptr for nub array
			if (gFenceList[i].nubList == nil)
				DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed");



			for (j = 0; j < gFenceList[i].numNubs; j++)		// convert x,z to x,y,z
			{
				gFenceList[i].nubList[j].x = fileFencePoints[j].x;
				gFenceList[i].nubList[j].z = fileFencePoints[j].z;
				gFenceList[i].nubList[j].y = 0;
			}
			ReleaseResource(hand);
		}
		else
			DoFatalAlert("ReadDataFromPlayfieldFile: cant get fence nub rez");
	}


			/****************************/
			/* PATH RELATED RESOURCES */
			/****************************/

			/* READ PATH LIST */

	hand = GetResource('Path',1000);
	if (hand)
	{
		DetachResource(hand);
		HLockHi(hand);

		BYTESWAP_HANDLE("bbhhxxiiihhhh", File_PathDefType, gNumPaths, hand);
		File_PathDefType* filePath = (File_PathDefType*) *hand;

		gPathList = (PathDefType**) NewHandleClear(gNumPaths * sizeof(PathDefType));

		for (i = 0; i < gNumPaths; i++)
		{
			(*gPathList)[i].flags = filePath[i].flags;
			(*gPathList)[i].parms[0] = filePath[i].parms[0];
			(*gPathList)[i].parms[1] = filePath[i].parms[1];
			(*gPathList)[i].parms[2] = filePath[i].parms[2];
//			(*gPathList)[i].numNubs = filePath[i].numNubs;
			(*gPathList)[i].numPoints = filePath[i].numPoints;
		}

		ReleaseResource(hand);
	}
	else
	{
		gNumPaths = 0;
		gPathList = nil;
	}


			/* READ PATH POINT LIST */

	for (i = 0; i < gNumPaths; i++)
   	{
		hand = GetResource('PaPt',1000+i);
		if (hand)
		{
			DetachResource(hand);
			HLockHi(hand);
			BYTESWAP_HANDLE("ff", PathPointType, (*gPathList)[i].numPoints, hand);
			(*gPathList)[i].pointList = (PathPointType **)hand;
		}
		else
			DoFatalAlert("ReadDataFromPlayfieldFile: cant get Path points rez");
	}


			/********************************/
			/* CHECKPOINT RELATED RESOURCES */
			/********************************/

	if (gNumCheckpoints > 0)
	{
		if (gNumCheckpoints > MAX_CHECKPOINTS)
			DoFatalAlert("ReadDataFromPlayfieldFile: gNumCheckpoints > MAX_CHECKPOINTS");

				/* READ CHECKPOINT LIST */

		hand = GetResource('CkPt',1000);
		if (hand)
		{
			DetachResource(hand);
			HLock(hand);
			BYTESWAP_HANDLE("HH2f2f", CheckpointDefType, gNumCheckpoints, hand);
			BlockMove(*hand, &gCheckpointList[0], GetHandleSize(hand));
			ReleaseResource(hand);

						/* CONVERT COORDINATES */

			for (i = 0; i < gNumCheckpoints; i++)
			{
				gCheckpointList[i].x[0] *= MAP2UNIT_VALUE;
				gCheckpointList[i].z[0] *= MAP2UNIT_VALUE;
				gCheckpointList[i].x[1] *= MAP2UNIT_VALUE;
				gCheckpointList[i].z[1] *= MAP2UNIT_VALUE;
			}
		}
		else
		{
			gNumCheckpoints = 0;
			//gCheckpointList = nil;
		}
	}


			/* CLOSE REZ FILE */

	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);




		/********************************************/
		/* READ SUPERTILE IMAGE DATA FROM DATA FORK */
		/********************************************/


				/* ALLOC BUFFERS */

	size = SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 2;						// calc size of supertile 16-bit texture
	tempBuffer16 = AllocPtr(size);
	if (tempBuffer16 == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: AllocHandle failed!");


				/* OPEN THE DATA FORK */

	iErr = FSpOpenDF(specPtr, fsRdPerm, &fRefNum);
	if (iErr)
		DoFatalAlert("ReadDataFromPlayfieldFile: FSpOpenDF failed!");

#if HQ_TERRAIN
	LoadTerrainSuperTileTexturesSeamless(fRefNum);
#else
	LoadTerrainSuperTileTextures(fRefNum);
#endif

			/* CLOSE THE FILE */

	FSClose(fRefNum);
	if (tempBuffer16)
		SafeDisposePtr(tempBuffer16);
}


static void LoadTerrainSuperTileTextures(short fRefNum)
{
			/* ALLOC BUFFERS */

	int size = SUPERTILE_TEXMAP_SIZE*SUPERTILE_TEXMAP_SIZE * sizeof(UInt16);		// calc size of supertile 16-bit texture

	Ptr	lzss = AllocPtr(size*2);									// temp buffer for compressed data
	Ptr pixels = AllocPtr(size);									// temp buffer for decompressed 16-bit pixel data

			/* LOAD EACH SUPERTILE */

	for (int i = 0; i < gNumUniqueSuperTiles; i++)
	{
		OSErr err;
		long sizeoflong = 4;
		UInt32 compressedSize;
		err = FSRead(fRefNum, &sizeoflong, (Ptr) &compressedSize);	// read size of compressed data
		compressedSize = Byteswap32Signed(&compressedSize);
		GAME_ASSERT(!err);
		GAME_ASSERT(sizeoflong == 4);
		GAME_ASSERT_MESSAGE(compressedSize <= (UInt32)size*2, "compressed data won't fit into buffer!");

		// read compressed data from file and decompress it into texture buffer
		long decompressedSize = LZSS_Decode(fRefNum, pixels, compressedSize);
		GAME_ASSERT(decompressedSize == size);

//		FlipImageVertically(pixels, SUPERTILE_TEXMAP_SIZE * sizeof(UInt16), SUPERTILE_TEXMAP_SIZE);

				/* BYTESWAP 16-BIT TEXTURE */

		ByteswapInts(2, decompressedSize/2, pixels);

				/* LOAD IT IN */

		gSuperTileTextureNames[i] = OGL_TextureMap_Load(pixels, SUPERTILE_TEXMAP_SIZE, SUPERTILE_TEXMAP_SIZE,
														GL_BGRA, GL_RGB, GL_UNSIGNED_SHORT_1_5_5_5_REV);

		OGL_Texture_SetOpenGLTexture(gSuperTileTextureNames[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);		// set clamp mode after each texture set since OGL just likes it that way
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	SafeDisposePtr(lzss);
	SafeDisposePtr(pixels);
}

#if HQ_TERRAIN

static void Blit16(
		const char*			src,
		int					srcWidth,
		int					srcHeight,
		int					srcRectX,
		int					srcRectY,
		int					srcRectWidth,
		int					srcRectHeight,
		char*				dst,
		int 				dstWidth,
		int 				dstHeight,
		int					dstRectX,
		int					dstRectY
		)
{
	const int bpp = 2;

	src += bpp * (srcRectX + srcWidth*srcRectY);
	dst += bpp * (dstRectX + dstWidth*dstRectY);

	for (int row = 0; row < srcRectHeight; row++)
	{
		memcpy(dst, src, bpp * srcRectWidth);
		src += bpp * srcWidth;
		dst += bpp * dstWidth;
	}
}

static void LoadTerrainSuperTileTexturesSeamless(short fRefNum)
{
	memset(gSuperTileTextureNames, 0, sizeof(gSuperTileTextureNames[0]) * gNumUniqueSuperTiles);

			/* ALLOC BUFFERS */

	const int tileSize = SUPERTILE_TEXMAP_SIZE;
	const int tileW = tileSize;
	const int tileH = tileSize;
	const int canvasW = tileW + 2;
	const int canvasH = tileH + 2;
	const int tileRowBytes = tileSize * sizeof(UInt16);
	const int tileBytes = tileRowBytes * tileSize;	// calc size of supertile 16-bit texture

	Ptr	lzss = AllocPtr(tileBytes * 2);									// temp buffer for compressed data
	Ptr canvas = AllocPtr((SUPERTILE_TEXMAP_SIZE + 2) * (SUPERTILE_TEXMAP_SIZE + 2) * sizeof(UInt16));
	Ptr allImages = AllocPtr(tileBytes * gNumUniqueSuperTiles);

			/* UNPACK SUPERTILE TEXTURES FROM FILE */

	for (int i = 0; i < gNumUniqueSuperTiles; i++)
	{
		Ptr image = allImages + i * tileBytes;

		OSErr err;
		long sizeoflong = 4;
		UInt32 compressedSize;
		err = FSRead(fRefNum, &sizeoflong, (Ptr) &compressedSize);	// read size of compressed data
		compressedSize = Byteswap32Signed(&compressedSize);
		GAME_ASSERT(!err);
		GAME_ASSERT(sizeoflong == 4);
		GAME_ASSERT_MESSAGE(compressedSize <= (UInt32)tileBytes*2, "compressed data won't fit into buffer!");

		// read compressed data from file and decompress it into texture buffer
		long decompressedSize = LZSS_Decode(fRefNum, image, compressedSize);
		GAME_ASSERT(decompressedSize == tileBytes);

//		FlipImageVertically(image, tileRowBytes, tileSize);

				/* BYTESWAP 16-BIT TEXTURE */

		ByteswapInts(2, decompressedSize/2, image);
	}

			/* ASSEMBLE SEAMLESS TEXTURES */

#define TILEIMAGE(col, row) (allImages + gSuperTileTextureGrid[(row)][(col)].superTileID * tileBytes)

	const int deep = gNumSuperTilesDeep;
	const int wide = gNumSuperTilesWide;

	for (int row = 0; row < deep; row++)
	for (int col = 0; col < wide; col++)
	{
		int unique = gSuperTileTextureGrid[row][col].superTileID;
		if (unique == 0)																// if 0 then its a blank
		{
			continue;
		}

		int tw = SUPERTILE_TEXMAP_SIZE;		// supertile width & height
		int th = SUPERTILE_TEXMAP_SIZE;
		int cw = tw + 2;
		int ch = th + 2;

		// Clear canvas to black
		memset(canvas, 0, canvasW * canvasH * sizeof(UInt16));

		// Blit supertile image to middle of canvas
		Blit16(TILEIMAGE(col, row), tw, th, 0, 0, tw, th, canvas, cw, ch, 1, 1);

		// Scan for neighboring supertiles
		bool hasN = row > 0;
		bool hasS = row < gNumSuperTilesDeep-1;
		bool hasW = col > 0;
		bool hasE = col < gNumSuperTilesWide-1;

		// Stitch edges from neighboring supertiles on each side and copy 1px corners from diagonal neighbors
		//                       srcBuf                   sW  sH    sX    sY  rW  rH  dstBuf  dW  dH    dX    dY
		if (hasN)         Blit16(TILEIMAGE(col  , row-1), tw, th,    0, th-1, tw,  1, canvas, cw, ch,    1,    0);
		if (hasS)         Blit16(TILEIMAGE(col  , row+1), tw, th,    0,    0, tw,  1, canvas, cw, ch,    1, ch-1);
		if (hasW)         Blit16(TILEIMAGE(col-1, row  ), tw, th, tw-1,    0,  1, th, canvas, cw, ch,    0,    1);
		if (hasE)         Blit16(TILEIMAGE(col+1, row  ), tw, th,    0,    0,  1, th, canvas, cw, ch, cw-1,    1);
		if (hasE && hasN) Blit16(TILEIMAGE(col+1, row-1), tw, th,    0, th-1,  1,  1, canvas, cw, ch, cw-1,    0);
		if (hasW && hasN) Blit16(TILEIMAGE(col-1, row-1), tw, th, tw-1, th-1,  1,  1, canvas, cw, ch,    0,    0);
		if (hasW && hasS) Blit16(TILEIMAGE(col-1, row+1), tw, th, tw-1,    0,  1,  1, canvas, cw, ch,    0, ch-1);
		if (hasE && hasS) Blit16(TILEIMAGE(col+1, row+1), tw, th,    0,    0,  1,  1, canvas, cw, ch, cw-1, ch-1);

		// Now load texture
		gSuperTileTextureNames[unique] = OGL_TextureMap_Load(canvas, canvasW, canvasH,
														GL_BGRA, GL_RGB, GL_UNSIGNED_SHORT_1_5_5_5_REV);

		// Clamp to edge
		OGL_Texture_SetOpenGLTexture(gSuperTileTextureNames[unique]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

#undef TILEIMAGE

			/* CLEAN UP */

	SafeDisposePtr(lzss);
	SafeDisposePtr(allImages);
	SafeDisposePtr(canvas);
}
#endif



/*********************** LOAD DATA FILE INTO MEMORY ***********************************/
//
// Use SafeDisposePtr when done.
//

Ptr LoadDataFile(const char* path, long* outLength)
{
	FSSpec spec;
	OSErr err;
	short refNum;
	long fileLength = 0;
	long readBytes = 0;

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
	GAME_ASSERT_MESSAGE(!err, path);

	err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	GAME_ASSERT_MESSAGE(!err, path);

	// Get number of bytes until EOF
	GetEOF(refNum, &fileLength);

	// Prep data buffer
	// Alloc 1 extra byte so LoadTextFile can return a null-terminated C string!
	Ptr data = AllocPtrClear(fileLength + 1);

	// Read file into data buffer
	readBytes = fileLength;
	err = FSRead(refNum, &readBytes, (Ptr) data);
	GAME_ASSERT_MESSAGE(err == noErr, path);
	FSClose(refNum);

	GAME_ASSERT_MESSAGE(fileLength == readBytes, path);

	if (outLength)
	{
		*outLength = fileLength;
	}

	return data;
}

/*********************** LOAD TEXT FILE INTO MEMORY ***********************************/
//
// Use SafeDisposePtr when done.
//

char* LoadTextFile(const char* spec, long* outLength)
{
	return LoadDataFile(spec, outLength);
}
