#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "globals.h"
#include "3dmath.h"
#include "objects.h"
#include "misc.h"
#include "collision.h"
#include "sound2.h"
#include "main.h"
#include "file.h"
#include "input.h"
#include "player.h"
#include "effects.h"
#include "mobjtypes.h"
#include "skeletonanim.h"
#include "skeletonobj.h"
#include "skeletonjoints.h"
#include "terrain.h"
#include "camera.h"
#include "triggers.h"
#include "items.h"
#include "bg3d.h"
#include "fences.h"
#include "paths.h"
#include "sobjtypes.h"
#include "metaobjects.h"
#include "sprites.h"
#include "triggers.h"
#include "infobar.h"
#include "liquids.h"
#include "splineitems.h"
#include "localization.h"
#include "quadmesh.h"
#include "atlas.h"
#include "window.h"
#include "pillarbox.h"
#include "racetimes.h"

extern Atlas*					gAtlases[MAX_SPRITE_GROUPS];
extern BG3DFileContainer		*gBG3DContainerList[MAX_BG3D_GROUPS];
extern Boolean					gAnnouncedPOW[MAX_POW_TYPES];
extern Boolean					gAutoPilot;
extern Boolean					gDisableAnimSounds;
extern Boolean					gDisableHiccupTimer;
extern Boolean					gDrawLensFlare;
extern Boolean					gDrawingOverlayPane;
extern Boolean					gGameOver;
extern Boolean					gGamePaused;
extern Boolean					gHideInfobar;
extern Boolean					gIsNetworkClient;
extern Boolean					gIsNetworkHost;
extern Boolean					gIsSelfRunningDemo;
extern Boolean					gMouseMotionNow;
extern Boolean					gNetGameInProgress;
extern Boolean					gNoCarControls;
extern Boolean					gSongPlayingFlag;
extern Boolean					gTrackCompleted;
extern Boolean					gUserPrefersGamepad;
extern Boolean					gUserTamperedWithPhysics;
extern Byte						gActiveSplitScreenMode;
extern Byte						gDebugMode;
extern CheckpointDefType		gCheckpointList[MAX_CHECKPOINTS];
extern CollisionRec				gCollisionList[];
extern CommandLineOptions		gCommandLine;
extern const float				gWaterHeights[NUM_TRACKS][6];
extern const InputBinding		kDefaultInputBindings[NUM_CONTROL_NEEDS];
extern const OGLColorRGB		kCavemanSkinColors[NUM_CAVEMAN_SKINS];
extern const UserPhysics		kDefaultPhysics;
extern FenceDefType				*gFenceList;
extern float					**gMapYCoords;
extern float					g2DLogicalHeight;
extern float					g2DLogicalWidth;
extern float					gAutoFadeEndDist;
extern float					gAutoFadeRange_Frac;
extern float					gAutoFadeStartDist;
extern float					gCameraStartupTimer;
extern float					gCurrentAspectRatio;
extern float					gFramesPerSecond;
extern float					gFramesPerSecondFrac;
extern float					gGammaFadePercent;
extern float					gGlobalTransparency;
extern float					gReTagTimer;
extern float					gStartingLightTimer;
extern float					gTrackCompletedCoolDownTimer;
extern FSSpec					gDataSpec;
extern GLuint					gSuperTileTextureNames[MAX_SUPERTILE_TEXTURES];
extern int						gCurrentAntialiasingLevel;
extern int						gCurrentSplitScreenPane;
extern int						gLoadTextureFlags;
extern int						gGameMode;
extern int						gGameWindowHeight;
extern int						gGameWindowWidth;
extern int						gNumObjectNodes;
extern int						gNumObjectsInBG3DGroupList[MAX_BG3D_GROUPS];
extern int						gNumPointers;
extern int						gNumSplitScreenPanes;
extern int						gPolysThisFrame;
extern int						gTheAge;
extern int						gTrackNum;
extern int						gVRAMUsedThisFrame;
extern long						gNumCheckpoints;
extern long						gNumFences;
extern long						gNumPaths;
extern long						gNumSplines;
extern long						gNumSuperTilesDeep;
extern long						gNumSuperTilesWide;
extern long						gNumUniqueSuperTiles;
extern long						gPrefsFolderDirID;
extern long						gTerrainTileDepth;
extern long						gTerrainTileWidth;
extern long						gTerrainUnitDepth;
extern long						gTerrainUnitWidth;
extern MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern MOMaterialObject			*gMostRecentMaterial;
extern MOMaterialObject         *gCavemanSkins[2][NUM_CAVEMAN_SKINS];
extern MOVertexArrayData		**gLocalTriMeshesOfSkelType;
extern NewParticleGroupDefType	gNewParticleGroupDef;
extern ObjNode					*gCurrentPlayer;
extern ObjNode					*gCycloramaObj;
extern ObjNode					*gFinalPlaceObj;
extern ObjNode					*gFirstNodePtr;
extern ObjNode					*gTorchObjs[];
extern OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern OGLColorRGB				gGlobalColorFilter;
extern OGLColorRGB				gTagColor;
extern OGLMatrix4x4				gLocalToFrustumMatrix;
extern OGLMatrix4x4				gLocalToViewMatrix;
extern OGLMatrix4x4				gViewToFrustumMatrix;
extern OGLMatrix4x4				gWorldToWindowMatrix[MAX_VIEWPORTS];
extern OGLPoint3D				gCoord;
extern OGLPoint3D				gEarCoords[MAX_LOCAL_PLAYERS];
extern OGLSetupOutputType		*gGameView;
extern OGLVector3D				gDelta;
extern OGLVector3D				gPreCollisionDelta;
extern OGLVector3D				gRecentTerrainNormal;
extern OGLVector3D				gWorldSunDirection;
extern ParticleGroupType		*gParticleGroups[MAX_PARTICLE_GROUPS];
extern PathDefType				**gPathList;
extern PlayerInfoType			gPlayerInfo[MAX_PLAYERS];
extern PrefsType				gGamePrefs;
extern Scoreboard				gScoreboard;
extern short					gCapturedFlagCount[2];
extern short					gCurrentPlayerNum;
extern short					gMainAppRezFile;
extern short					gMyNetworkPlayerNum;
extern short					gNumActiveParticleGroups;
extern short					gNumCollisions;
extern short					gNumFencesDrawn;
extern short					gNumLapsThisRace;
extern short					gNumLocalPlayers;
extern short					gNumPlayersEliminated;
extern short					gNumRealPlayers;
extern short					gNumTerrainItems;
extern short					gNumTorches;
extern short					gNumTotalPlayers;
extern short					gPlayerMultiPassCount;
extern short					gPrefsFolderVRefNum;
extern short					gTotalTokens;
extern short					gWhoIsIt;
extern short					gWhoWasIt;
extern short					gWorstHumanPlace;
extern SplineDefType			**gSplineList;
extern SplineDefType			**gSplineList;
extern Str32					gPlayerNameStrings[MAX_PLAYERS];
extern SuperTileGridType		**gSuperTileTextureGrid;
extern SuperTileStatus			**gSuperTileStatusGrid;
extern TerrainItemEntryType		**gMasterItemList;
extern TileAttribType			**gTileAttribList;
extern UserPhysics				gUserPhysics;
extern uint16_t					**gTileGrid;
extern uint32_t					gAutoFadeStatusBits;
extern uint32_t					gGlobalMaterialFlags;
extern uint32_t					gInfobarUpdateBits;
