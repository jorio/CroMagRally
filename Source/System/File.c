/****************************/
/*      FILE ROUTINES       */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include	<string.h>
#include	"globals.h"
#include 	"objects.h"
#include	"misc.h"
#include	"skeletonanim.h"
#include	"skeletonobj.h"
#include	"skeletonjoints.h"
#include 	"mobjtypes.h"
#include	"file.h"
#include 	"window.h"
#include 	"main.h"
#include 	"bones.h"
#include 	"sound2.h"
#include 	"terrain.h"
#include 	"bg3d.h"
#include 	"fences.h"
#include 	"paths.h"
#include 	"checkpoints.h"
#include 	"sprites.h"
#include 	"sobjtypes.h"
#include 	"lzss.h"
#include 	"input.h"
#include 	"miscscreens.h"
#include 	"main.h"

extern	short			gMainAppRezFile;
extern	short			gNumTerrainItems;
extern	short			gPrefsFolderVRefNum;
extern	long			gPrefsFolderDirID,gNumPaths;
extern	long			gTerrainTileWidth,gTerrainTileDepth,gTerrainUnitWidth,gTerrainUnitDepth,gNumUniqueSuperTiles;
extern	long			gNumSuperTilesDeep,gNumSuperTilesWide;
extern	FSSpec			gDataSpec;
extern	uint32_t			gScore;
extern	float			gDemoVersionTimer;
extern  uint16_t			**gTileDataHandle;
extern	float			**gMapYCoords;
extern	Byte			**gMapSplitMode;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	SuperTileGridType **gSuperTileTextureGrid;
extern	FenceDefType	*gFenceList;
extern	long			gNumFences,gNumSplines,gNumCheckpoints;
extern	SplineDefType	**gSplineList;
extern  PathDefType	    **gPathList;
extern	CheckpointDefType	   gCheckpointList[MAX_CHECKPOINTS];
extern	int				gTrackNum;
extern	TileAttribType	**gTileAttribList;
extern	uint16_t			**gTileGrid;
extern	GLuint			gSuperTileTextureNames[MAX_SUPERTILE_TEXTURES];
extern	PrefsType			gGamePrefs;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType, OGLSetupOutputType *setupInfo);
static void ReadDataFromPlayfieldFile(FSSpec *specPtr);
static void	ConvertTexture16To24(uint16_t *textureBuffer2, uint8_t *textureBuffer3, int width, int height);
static void	ConvertTexture16To16(uint16_t *textureBuffer, int width, int height);

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

static	Str255		gBasePathName = "NewGame";

float	g3DTileSize, g3DMinY, g3DMaxY;

SavePlayerType	gPlayerSaveData;
Boolean			gSavedPlayerIsLoaded = false;
short			gSavedPlayerRezNum = 0;

/****************** SET DEFAULT DIRECTORY ********************/
//
// This function needs to be called for OS X because OS X doesnt automatically
// set the default directory to the application directory.
//

void SetDefaultDirectory(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
ProcessSerialNumber serial;
ProcessInfoRec info;
FSSpec	app_spec;
WDPBRec wpb;
OSErr	iErr;

	serial.highLongOfPSN = 0;
	serial.lowLongOfPSN = kCurrentProcess;


	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processName = NULL;
	info.processAppSpec = &app_spec;

	iErr = GetProcessInformation(&serial, & info);

	wpb.ioVRefNum = app_spec.vRefNum;
	wpb.ioWDDirID = app_spec.parID;
	wpb.ioNamePtr = NULL;

	iErr = PBHSetVolSync(&wpb);
#endif
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

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo)
{
QDErr		iErr;
short		fRefNum;
FSSpec		fsSpec;
SkeletonDefType	*skeleton;

				/* SET CORRECT FILENAME */

	switch(skeletonType)
	{
		case	SKELETON_TYPE_PLAYER_MALE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Brog.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_PLAYER_FEMALE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Grag.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_YETI:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Yeti.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_BIRDBOMB:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:BirdBomb.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_BEETLE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Beetle.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_CAMEL:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Camel.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_CATAPULT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Catapult.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_BRONTONECK:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:BrontoNeck.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_SHARK:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Shark.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_FLAG:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Flag.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_PTERADACTYL:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Pteradactyl.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_MALESTANDING:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:BrogStanding.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_FEMALESTANDING:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:GragStanding.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_DRAGON:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Dragon.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_MUMMY:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Mummy.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_TROLL:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Troll.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_DRUID:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Druid.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_POLARBEAR:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:PolarBear.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_FLOWER:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Flower.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_VIKING:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Viking.skeleton", &fsSpec);
				break;

		default:
				DoFatalAlert("LoadSkeleton: Unknown skeletonType!");
	}


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

	ReadDataFromSkeletonFile(skeleton,&fsSpec,skeletonType,setupInfo);
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

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType, OGLSetupOutputType *setupInfo)
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
			LoadBonesReferenceModel(&target,skeleton, skeletonType, setupInfo);
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



/******************** LOAD PREFS **********************/
//
// Load in standard preferences
//

OSErr LoadPrefs(PrefsType *prefBlock)
{
OSErr		iErr;
short		refNum;
FSSpec		file;
long		count;

				/*************/
				/* READ FILE */
				/*************/

#if DEMO
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:DemoPreferences4", &file);
#else
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:Preferences4", &file);
#endif
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr)
		return(iErr);

	count = sizeof(PrefsType);
	iErr = FSRead(refNum, &count,  (Ptr)prefBlock);		// read data from file
	if (iErr)
	{
		FSClose(refNum);
		return(iErr);
	}

	FSClose(refNum);

			/****************/
			/* VERIFY PREFS */
			/****************/

	if ((gGamePrefs.depth != 16) && (gGamePrefs.depth != 32))
		goto err;

//	if ((gGamePrefs.screenWidth != 1024) && (gGamePrefs.screenWidth != 800) && (gGamePrefs.screenWidth != 640))
//		goto err;

//	if ((gGamePrefs.screenHeight != 768) && (gGamePrefs.screenHeight != 600) && (gGamePrefs.screenHeight != 480))
//		goto err;

	if ((gGamePrefs.screenCrop < 0.0f) || (gGamePrefs.screenCrop > 1.0))
		goto err;


	return(noErr);

err:
	InitDefaultPrefs();
	return(noErr);
}



/******************** SAVE PREFS **********************/

void SavePrefs(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

				/* CREATE BLANK FILE */

#if DEMO
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:DemoPreferences4", &file);
#else
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:Preferences4", &file);
#endif
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'CavM', 'Pref', smSystemScript);					// create blank file
	if (iErr)
		return;

				/* OPEN FILE */

	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
		FSpDelete(&file);
		return;
	}

				/* WRITE DATA */

	count = sizeof(PrefsType);
	FSWrite(refNum, &count, (Ptr) &gGamePrefs);
	FSClose(refNum);
}

#pragma mark -


/**************** SET DEFAULT PLAYER SAVE DATA **********************/
//
// Called at boot to initialize the currently "loaded" player since no saved
// player is loaded
//

void SetDefaultPlayerSaveData(void)
{
Str255	chooserName;

	gSavedPlayerIsLoaded = false;							// user has not selected a saved player yet

	gPlayerSaveData.numAgesCompleted = 0;					// no ages completed yet


//	GetChooserName(chooserName);							// assign machine name to player name
	strcpy(chooserName, "localhost"); //---TODO: use real hostname

	snprintf(gPlayerSaveData.playerName, sizeof(gPlayerSaveData.playerName), "%s", chooserName);
}


/***************************** CREATE SAVED PLAYER ********************************/
//
// Called when player wants to create a new saved game player
//

void DoSavedPlayerDialog(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
DialogRef 		myDialog;
DialogItemType			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
Boolean			dialogDone;
Str255			gameName;
ModalFilterUPP	myProc;
short			numGamesInList;

	SavePlayerFile();												// first save off any existing player that is active

 	Enter2D(true);
	TurnOffISp();
	InitCursor();

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());

	myDialog = GetNewDialog(3000+ gGamePrefs.language,nil,MOVE_TO_FRONT);
	if (myDialog == nil)
		DoFatalAlert("DoSavedPlayerDialog: no dialog?");

			/*****************/
			/* INIT LIST BOX */
			/*****************/

	GetDialogItem(myDialog,3,&itemType,(Handle *)&itemHandle,&itemRect);					// player's box
	SetDialogItem(myDialog,3, userItem,(Handle)NewUserItemUPP(DoOutline), &itemRect);
	numGamesInList = InitSavedGamesListBox(&itemRect, GetDialogWindow(myDialog));			// create list manager list


				/* SET CONTROL VALUES */

	GetDialogItem(myDialog,3,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItemText((Handle)itemHandle,gPlayerSaveData.playerName);

	BlockMove("New Game", gameName, 9);
	GetDialogItem(myDialog,6,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItemText((Handle)itemHandle,gameName);


			/* SET OUTLINE FOR USERITEM */

	GetDialogItem(myDialog,1,&itemType,(Handle *)&itemHandle,&itemRect);
	SetDialogItem(myDialog, 2, userItem, (Handle)NewUserItemUPP(DoBold), &itemRect);


				/* DO IT */

	dialogDone = false;
	myProc = NewModalFilterUPP(DialogCallback);

	while(dialogDone == false)
	{
		ModalDialog(myProc, &itemHit);
		switch (itemHit)
		{
			case 	1:									// hit ok
					if (numGamesInList <= 0)			// make sure there's a game
					{
						DoAlert("You must first create a new game.");
						InitCursor();
					}
					else
						dialogDone = true;
					break;

			case	6:									// game name text box
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,gameName);
					break;

			case	7:									// delete selected game
					DeleteSelectedGame();
					break;

			case	8:									// add Game
					if (gameName[0] == 0)
					{
						SysBeep(0);
						break;
					}

					numGamesInList = AddNewGameToFile(gameName);
					break;

		}
	}

	LoadSelectedGame();

			/* CLEAN UP */

	DisposeModalFilterUPP(myProc);
	DisposeDialog(myDialog);
	HideCursor();
	Exit2D();
	TurnOnISp();
#endif
}

/*********************** ADD NEW GAME TO FILE ***************************/

static short AddNewGameToFile(Str255 name)
{
	IMPLEMENT_ME(); return 0;
#if 0
int		i;
SavePlayerType	**rez;
FSSpec		spec;
short	fRefNum,numGames;
Cell	cell;

	SetDefaultPlayerSaveData();			// set info to the defaults

				/* OPEN THE REZ-FORK */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:SavedGames", &spec);

	fRefNum = FSpOpenResFile(&spec,fsCurPerm);
	if (fRefNum == -1)
			DoFatalAlert("AddNewGameToFile: FSpOpenResFile failed!");
	UseResFile(fRefNum);

	i = Count1Resources('GAME');						// count # games in there


			/* ADD GAME TO REZ FILE */

	rez = (SavePlayerType **)AllocHandle(sizeof(gPlayerSaveData));	// make handle
	**rez = gPlayerSaveData;										// copy data into handle
	AddResource((Handle)rez, 'GAME', i, name);						// save rez
	ReleaseResource((Handle)rez);									// free it

				/* CLEAN UP */

	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);

	numGames = UpdateSavedGamesList();										// update the list

	cell.v = i;
	cell.h = 0;
	LSetSelect(true, cell, gTheList);								// highlite it in the list
	LAutoScroll(gTheList);										// scroll to it

	return(numGames);
#endif
}



/********************* LOAD SELECTED GAME *****************************/

static void LoadSelectedGame(void)
{
	IMPLEMENT_ME();
#if 0
Cell	cell = {0,0};
SavePlayerType	**rez;
FSSpec	spec;
short	fRefNum;

	if (LGetSelect(true, &cell, gTheList))						// get first selected cell, if any
	{

					/* OPEN THE REZ-FORK */

		FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:SavedGames", &spec);

		fRefNum = FSpOpenResFile(&spec,fsCurPerm);
		if (fRefNum == -1)
				DoFatalAlert("DeleteSelectedGame: FSpOpenResFile failed!");
		UseResFile(fRefNum);


				/* LOAD THE SAVED DATA */

		rez = (SavePlayerType **)GetResource('GAME',cell.v);	// get the resource
		gPlayerSaveData = **rez;								// copy to struct
		ReleaseResource((Handle)rez);
		gSavedPlayerIsLoaded = true;
		gSavedPlayerRezNum = cell.v;							// remember rez # for saving later

					/* CLEAN UP */

		CloseResFile(fRefNum);
		UseResFile(gMainAppRezFile);
	}
	else
		gSavedPlayerIsLoaded = true;								// nothing was selected
#endif
}


/****************** DELETE SELECTED GAME ************************/

static void DeleteSelectedGame(void)
{
	IMPLEMENT_ME();
#if 0
Cell	cell = {0,0};
int		num,i;
Handle	rez;
FSSpec	spec;
short	fRefNum;

	if (LGetSelect(true, &cell, gTheList))						// get first selected cell, if any
	{

					/* OPEN THE REZ-FORK */

		FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:SavedGames", &spec);

		fRefNum = FSpOpenResFile(&spec,fsCurPerm);
		if (fRefNum == -1)
				DoFatalAlert("DeleteSelectedGame: FSpOpenResFile failed!");
		UseResFile(fRefNum);

		num = Count1Resources('GAME');						// count # games in there

					/* DELETE THE TARGET */

		rez = GetResource('GAME',cell.v);					// get the resource
		RemoveResource(rez);									// nuke it

				/* SHIFT THE OTHERS DOWN */

		for (i = cell.v + 1; i < num; i++)
		{
			rez = GetResource('GAME',i);					// get the resource
			SetResInfo(rez, i-1, nil);						// shift it down
			ReleaseResource(rez);
		}


					/* CLEAN UP */

		CloseResFile(fRefNum);
		UseResFile(gMainAppRezFile);

		UpdateSavedGamesList();
	}
#endif
}



/******************** INIT SAVED GAMES LIST BOX *************************/

static short InitSavedGamesListBox(Rect *r, WindowPtr myDialog)
{
	IMPLEMENT_ME(); return 0;
#if 0
Rect	dataBounds;
Point	cSize;

	r->right -= 15;														// make room for scroll bars & outline
	r->top += 1;
	r->bottom -= 1;
	r->left += 1;

	gNumRowsInList = 0;
	SetRect(&dataBounds,0,0,1,gNumRowsInList);							// no entries yet
	cSize.h = cSize.v = 0;
	gTheList = LNew(r, &dataBounds, cSize, 0, myDialog, true, false, false, true);

	(**gTheList).selFlags |= lOnlyOne;

	return (UpdateSavedGamesList());
#endif
}



/***************** UPDATE SAVED GAMES LIST ************************/

static short UpdateSavedGamesList(void)
{
	IMPLEMENT_ME(); return 0;
#if 0
short	i,numGames;
Cell	theCell;
short		fRefNum,theID;
ResType		theType;
FSSpec		spec;
Handle		rez;
Str255		name;

		/* DELETE ALL EXISTING ROWS IN LIST */

	LDelRow(0, 0, gTheList);
	gNumRowsInList = 0;


				/* OPEN THE REZ-FORK */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:SavedGames", &spec);

	fRefNum = FSpOpenResFile(&spec,fsCurPerm);
	if (fRefNum == -1)
	{
				/* CREATE A NEW SAVED GAMES FILE */

		FSpCreateResFile(&spec,kGameID,'PSav',nil);
		if (ResError())
			DoFatalAlert("UpdateSavedGamesList: FSpCreateResFile failed!");
		fRefNum = FSpOpenResFile(&spec,fsCurPerm);
	}
	UseResFile(fRefNum);


			/* GET # SAVED GAMES IN THERE */

	numGames = Count1Resources('GAME');
	LAddRow(numGames, 0, gTheList);									// create rows in list


			/* ADD EACH GAME'S NAME TO THE LIST */

	for (i = 0; i < numGames; i++)
	{
		if (i == (numGames-1))										// reactivate draw on last cell
			LSetDrawingMode(true,gTheList);							// turn on updating
		theCell.h = 0;
		theCell.v = i;

		rez = GetResource('GAME',i);								// get the resource
		GetResInfo(rez, &theID, &theType, name);					// extract the name of it
		ReleaseResource(rez);										// free it

		LSetCell(&name[1], name[0], theCell, gTheList);				// add the name to the list
		gNumRowsInList++;
	}


	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);


		/* HIGHLIGHT THE BEST ONE */

	if ((numGames > 0) && (gSavedPlayerRezNum < numGames))
	{
		theCell.h = 0;
		theCell.v = gSavedPlayerRezNum;
		LSetSelect(true, theCell, gTheList);								// highlite it in the list
		LAutoScroll(gTheList);										// scroll to it
	}

	return(numGames);
#endif
}




/*********************** SAVE PLAYER FILE *******************************/

void SavePlayerFile(void)
{
SavePlayerType	**rez;
FSSpec	spec;
short	fRefNum;

	if (!gSavedPlayerIsLoaded)				// see if no player
		return;

				/* OPEN THE REZ-FORK */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":CroMag:SavedGames", &spec);

	fRefNum = FSpOpenResFile(&spec,fsCurPerm);
	if (fRefNum == -1)
			DoFatalAlert("SavePlayerFile: FSpOpenResFile failed!");
	UseResFile(fRefNum);


			/* LOAD THE REZ */

	rez = (SavePlayerType **)GetResource('GAME',gSavedPlayerRezNum);	// get the resource
	**rez = gPlayerSaveData;											// copy from struct
	ChangedResource((Handle)rez);
	WriteResource((Handle)rez);
	ReleaseResource((Handle)rez);

				/* CLEAN UP */

	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);
}






#pragma mark -


/**************** DRAW PICTURE INTO GWORLD ***********************/
//
// Uses Quicktime to load any kind of picture format file and draws
// it into the GWorld
//
//
// INPUT: myFSSpec = spec of image file
//
// OUTPUT:	theGWorld = gworld contining the drawn image.
//

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth)
{
	IMPLEMENT_ME(); return unimpErr;
#if 0
OSErr						iErr;
GraphicsImportComponent		gi;
Rect						r;
ComponentResult				result;
PixMapHandle 				hPixMap;
FInfo						fndrInfo;

			/* VERIFY FILE - DEBUG */

	iErr = FSpGetFInfo(myFSSpec, &fndrInfo);
	if (iErr)
	{
		switch(iErr)
		{
			case	fnfErr:
					DoFatalAlert("DrawPictureIntoGWorld:  file is missing");
					break;

			default:
					DoFatalAlert("DrawPictureIntoGWorld:  file is invalid");
		}
	}


			/* PREP IMPORTER COMPONENT */

	result = GetGraphicsImporterForFile(myFSSpec, &gi);		// load importer for this image file
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GetGraphicsImporterForFile failed!  You do not have Quicktime properly installed, reinstall Quicktime and do a FULL install.");
		return(result);
	}
	if (GraphicsImportGetBoundsRect(gi, &r) != noErr)		// get dimensions of image
		DoFatalAlert("DrawPictureIntoGWorld: GraphicsImportGetBoundsRect failed!");


			/* KEEP MUSIC PLAYING */

	if (gSongPlayingFlag)
		MoviesTask(gSongMovie, 0);


			/* MAKE GWORLD */

	iErr = NewGWorld(theGWorld, depth, &r, nil, nil, 0);					// try app mem
	if (iErr)
	{
		DoAlert("DrawPictureIntoGWorld: using temp mem");
		iErr = NewGWorld(theGWorld, depth, &r, nil, nil, useTempMem);		// try sys mem
		if (iErr)
		{
			DoAlert("DrawPictureIntoGWorld: MakeMyGWorld failed");
			return(1);
		}
	}

	if (depth == 32)
	{
		hPixMap = GetGWorldPixMap(*theGWorld);				// get gworld's pixmap
		(**hPixMap).cmpCount = 4;							// we want full 4-component argb (defaults to only rgb)
	}


			/* DRAW INTO THE GWORLD */

	DoLockPixels(*theGWorld);
	GraphicsImportSetGWorld(gi, *theGWorld, nil);			// set the gworld to draw image into
	GraphicsImportSetQuality(gi,codecLosslessQuality);		// set import quality

	{
//		RGBColor	cc;
//		GraphicsImportSetGraphicsMode(gi, srcCopy, &cc);	// set copy mode (no dither!)
	}


	result = GraphicsImportDraw(gi);						// draw into gworld
	CloseComponent(gi);										// cleanup
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GraphicsImportDraw failed!");
		ShowSystemErr(result);
		DisposeGWorld (*theGWorld);
		*theGWorld= nil;
		return(result);
	}
	return(noErr);
#endif
}


#pragma mark -

/************************** LOAD LEVEL ART ***************************/

void LoadLevelArt(OGLSetupOutputType *setupInfo)
{
FSSpec	spec;

static const Str255	terrainFiles[NUM_TRACKS] =
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

static const Str255	levelModelFiles[NUM_TRACKS] =
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



				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Caveman.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_CAVEMAN);


	switch(gTrackNum)
	{
		case	TRACK_NUM_DESERT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Desert.sounds", &spec);
				LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
				break;

		case	TRACK_NUM_JUNGLE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Jungle.sounds", &spec);
				LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
				break;

		case	TRACK_NUM_ICE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Ice.sounds", &spec);
				LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
				break;

		case	TRACK_NUM_CHINA:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:China.sounds", &spec);
				LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
				break;

		case	TRACK_NUM_EGYPT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Egypt.sounds", &spec);
				LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
				break;

		case	TRACK_NUM_EUROPE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Europe.sounds", &spec);
				LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
				break;

		case	TRACK_NUM_ATLANTIS:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Atlantis.sounds", &spec);
				LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
				break;

		case	TRACK_NUM_STONEHENGE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Stonehenge.sounds", &spec);
				LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
				break;

	}


			/* LOAD BG3D GEOMETRY */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:carparts.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_CARPARTS, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:weapons.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WEAPONS, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelModelFiles[gTrackNum], &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_PLAYER_MALE, setupInfo);
	LoadASkeleton(SKELETON_TYPE_PLAYER_FEMALE, setupInfo);

	LoadASkeleton(SKELETON_TYPE_BIRDBOMB, setupInfo);

	switch(gTrackNum)
	{
		case	TRACK_NUM_DESERT:
				LoadASkeleton(SKELETON_TYPE_BRONTONECK, setupInfo);
				break;

		case	TRACK_NUM_JUNGLE:
				LoadASkeleton(SKELETON_TYPE_PTERADACTYL, setupInfo);
				LoadASkeleton(SKELETON_TYPE_FLOWER, setupInfo);
				break;

		case	TRACK_NUM_ICE:
				LoadASkeleton(SKELETON_TYPE_YETI, setupInfo);
				LoadASkeleton(SKELETON_TYPE_POLARBEAR, setupInfo);
				break;

		case	TRACK_NUM_CHINA:
				LoadASkeleton(SKELETON_TYPE_DRAGON, setupInfo);
				break;

		case	TRACK_NUM_EGYPT:
				LoadASkeleton(SKELETON_TYPE_BEETLE, setupInfo);
				LoadASkeleton(SKELETON_TYPE_CAMEL, setupInfo);
				LoadASkeleton(SKELETON_TYPE_MUMMY, setupInfo);
				break;

		case	TRACK_NUM_EUROPE:
				LoadASkeleton(SKELETON_TYPE_CATAPULT, setupInfo);
				LoadASkeleton(SKELETON_TYPE_FLAG, setupInfo);
				break;

		case	TRACK_NUM_SCANDINAVIA:
				LoadASkeleton(SKELETON_TYPE_TROLL, setupInfo);
				LoadASkeleton(SKELETON_TYPE_VIKING, setupInfo);
				break;

		case	TRACK_NUM_ATLANTIS:
				LoadASkeleton(SKELETON_TYPE_SHARK, setupInfo);
				break;

		case	TRACK_NUM_STONEHENGE:
				LoadASkeleton(SKELETON_TYPE_DRUID, setupInfo);
				break;

		case	TRACK_NUM_CELTIC:
				LoadASkeleton(SKELETON_TYPE_FLAG, setupInfo);
				break;

	}



			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:fence.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FENCES, setupInfo);


	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:rockfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, setupInfo);


			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, terrainFiles[gTrackNum], &spec);
	LoadPlayfield(&spec);

			/* CAST SHADOWS */

	DoItemShadowCasting();

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
Ptr						tempBuffer16 = nil,tempBuffer24 = nil;

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
		BYTESWAP_HANDLE("f", float, gTerrainTileDepth*gTerrainTileWidth, hand);
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

	tempBuffer24 = AllocPtr(SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 3);		// alloc for 24bit pixels
	if (tempBuffer24 == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: AllocHandle failed!");


				/* OPEN THE DATA FORK */

	iErr = FSpOpenDF(specPtr, fsRdPerm, &fRefNum);
	if (iErr)
		DoFatalAlert("ReadDataFromPlayfieldFile: FSpOpenDF failed!");



	for (i = 0; i < gNumUniqueSuperTiles; i++)
	{
		static long	sizeoflong = 4;
		int32_t	compressedSize;
		int32_t	decompressedSize;
		long	width,height;

				/* READ THE SIZE OF THE NEXT COMPRESSED SUPERTILE TEXTURE */

		iErr = FSRead(fRefNum, &sizeoflong, (Ptr) &compressedSize);
		compressedSize = Byteswap32Signed(&compressedSize);
		if (iErr)
			DoFatalAlert("ReadDataFromPlayfieldFile: FSRead failed!");


				/* READ & DECOMPRESS IT */

		decompressedSize = LZSS_Decode(fRefNum, tempBuffer16, compressedSize);
		if (decompressedSize != size)
      			DoFatalAlert("ReadDataFromPlayfieldFile: LZSS_Decode size is wrong!");
		

				/* BYTESWAP 16-BIT TEXTURE */

		ByteswapInts(2, decompressedSize/2, tempBuffer16);


		width = SUPERTILE_TEXMAP_SIZE;
		height = SUPERTILE_TEXMAP_SIZE;


				/* USE PACKED PIXEL TYPE */
				// TODO: skip ConvertTexture16To16 and just flip in UVs

		ConvertTexture16To16((uint16_t *)tempBuffer16, width, height);
		gSuperTileTextureNames[i] = OGL_TextureMap_Load(tempBuffer16, width, height, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);



				/* KEEP MUSIC PLAYING */

//		if (gSongPlayingFlag)
//			MoviesTask(gSongMovie, 0);
	}

			/* CLOSE THE FILE */

	FSClose(fRefNum);
	if (tempBuffer16)
		SafeDisposePtr(tempBuffer16);
	if (tempBuffer24)
		SafeDisposePtr(tempBuffer24);
}


/*********************** CONVERT TEXTURE; 16 TO 16 ***********************************/
//
// Simply flips Y since OGL Textures are screwey
//

static void	ConvertTexture16To16(uint16_t *textureBuffer, int width, int height)
{
int		x,y;
uint16_t	pixel,*bottom;
uint16_t	*dest;

	bottom = textureBuffer + ((height - 1) * width);

	for (y = 0; y < height / 2; y++)
	{
		dest = bottom;

		for (x = 0; x < width; x++)
		{
			pixel = textureBuffer[x] | 0x8000;						// get 16bit pixel
			textureBuffer[x] = bottom[x] | 0x8000;
			bottom[x] = pixel;
		}

		textureBuffer += width;
		bottom -= width;
	}
}


/*********************** LOAD FILE INTO MEMORY ***********************************/

Ptr LoadFileData(const FSSpec* spec, long* outLength)
{
	OSErr err;
	short refNum;
	long fileLength = 0;
	long readBytes = 0;

	puts(spec->cName);
	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	GAME_ASSERT(!err);

	// Get number of bytes until EOF
	GetEOF(refNum, &fileLength);

	// Prep data buffer
	Ptr data = AllocPtrClear(fileLength);

	// Read file into data buffer
	readBytes = fileLength;
	err = FSRead(refNum, &readBytes, (Ptr) data);
	GAME_ASSERT(err == noErr);
	FSClose(refNum);

	GAME_ASSERT(fileLength == readBytes);

	if (outLength)
	{
		*outLength = fileLength;
	}

	return data;
}
