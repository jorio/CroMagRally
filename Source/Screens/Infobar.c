/****************************/
/*   	INFOBAR.C		    */
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <stddef.h>

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MakeInfobar(void);
static void DrawInfobar(ObjNode* theNode);

static void Infobar_DrawMap(Byte whichPane);
static void Infobar_MovePlace(ObjNode* objNode);
static void Infobar_MoveInventoryPOW(ObjNode* objNode);
static void Infobar_MoveWrongWay(ObjNode* objNode);
static void Infobar_DrawStartingLight(Byte whichPane);
static void Infobar_MoveLap(ObjNode* objNode);
static void Infobar_MoveToken(ObjNode* objNode);
static void Infobar_MovePOWTimer(ObjNode* objNode);
static void Infobar_DrawTagTimer(Byte whichPane);
static void Infobar_MoveFlag(ObjNode* objNode);
static void Infobar_DrawHealth(Byte whichPane);

static void MoveFinalPlace(ObjNode *theNode);
static void MovePressAnyKey(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define PLAYER_NAME_SAFE_CHARSET " .0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

#define OVERHEAD_MAP_REFERENCE_SIZE 256.0f

#define INFOBAR_SPRITE_FLAGS (kTextMeshAlignCenter | kTextMeshAlignMiddle | kTextMeshKeepCurrentProjection)

#define MAX_PANEDIVIDER_QUADS		4

#define MAX_SUBICONS				12

enum
{
	ICON_PLACE	 = 0,		// 1st, 2nd, 3rd etc.
	ICON_MAP,
	ICON_STARTLIGHT,
	ICON_LAP,
	ICON_WRONGWAY,			// Big red X
	ICON_TOKEN,				// Tournament mode arrowheads
	ICON_WEAPON_RACE,		// Weapon/nitro inventory in race modes
	ICON_WEAPON_BATTLE,		// Weapon/nitro inventory in battle modes
	ICON_TIMER,				// Health background
	ICON_TIMERINDEX,		// Health indicator
	ICON_POWTIMER,			// Buff timers
	ICON_FLAG,				// CTF torches collected
	NUM_INFOBAR_ICONTYPES
};

enum
{
	kAnchorCenter		= 0,
	kAnchorTop			= 1 << 0,
	kAnchorBottom		= 1 << 1,
	kAnchorLeft			= 1 << 2,
	kAnchorRight		= 1 << 3,
	kAnchorTopLeft		= kAnchorTop | kAnchorLeft,
	kAnchorTopRight		= kAnchorTop | kAnchorRight,
	kAnchorBottomLeft	= kAnchorBottom | kAnchorLeft,
	kAnchorBottomRight	= kAnchorBottom | kAnchorRight
};

typedef struct
{
	int type;
	int sub;
	int displayedValue;
	int state;
} InfobarIconData;
CheckSpecialDataStruct(InfobarIconData);
#define GetInfobarIconData(node) GetSpecialData(node, InfobarIconData)

typedef struct
{
	int anchor;
	float xFromAnchor;
	float yFromAnchor;
	float scale;
	float xSpacing;
} IconPlacement;

static const IconPlacement gIconInfo[NUM_INFOBAR_ICONTYPES] =
{
	[ICON_PLACE]		= { kAnchorTopLeft,		  64,  48, 0.9f,   0 },
	[ICON_MAP]			= { kAnchorBottomRight,	 -80, -84, 0.5f,   0 },
	[ICON_STARTLIGHT]	= { kAnchorCenter,		   0, -72, 1.0f,   0 },
	[ICON_LAP]			= { kAnchorBottomLeft,	  51, -48, 0.7f,   0 },
	[ICON_WRONGWAY]		= { kAnchorCenter,		   0, -72, 1.5f,   0 },
	[ICON_TOKEN]		= { kAnchorTopRight,	-196,  24, 0.4f,  26 },
	[ICON_WEAPON_RACE]	= { kAnchorTop,			 -16,  36, 0.9f,  42 },
	[ICON_WEAPON_BATTLE]= { kAnchorTopLeft,		  32,  36, 0.9f,  42 },
	[ICON_TIMER]		= { kAnchorTopRight,	-118,  36, 1.0f, 125 },
	[ICON_TIMERINDEX]	= { kAnchorTopRight,	-166,  36, 0.6f, 106 },
	[ICON_POWTIMER]		= { kAnchorTopLeft,		  29, 144, 0.8f,   0 },
	[ICON_FLAG]			= { kAnchorTopRight,	  19-6*36,  36, 0.5f,  32 },
};

#define MAX_POWTIMERS 6

static const struct
{
	size_t timerFloatOffset;
	short sprite;
} kPOWTimerDefs[MAX_POWTIMERS] =
{
	{ offsetof(PlayerInfoType, stickyTiresTimer),		INFOBAR_SObjType_StickyTires },
	{ offsetof(PlayerInfoType, nitroTimer),				INFOBAR_SObjType_Weapon_Nitro },
	{ offsetof(PlayerInfoType, superSuspensionTimer),	INFOBAR_SObjType_Suspension },
	{ offsetof(PlayerInfoType, invisibilityTimer),		INFOBAR_SObjType_Invisibility },
	{ offsetof(PlayerInfoType, frozenTimer),			INFOBAR_SObjType_Weapon_Freeze },
	{ offsetof(PlayerInfoType, flamingTimer),			INFOBAR_SObjType_RedTorch },
};

static int8_t gPOWTimersByRow[MAX_LOCAL_PLAYERS][MAX_POWTIMERS];

const OGLColorRGB kCavemanSkinColors[NUM_CAVEMAN_SKINS] =
{
	[CAVEMAN_SKIN_BROWN]	=	{.8,.5,.3},
	[CAVEMAN_SKIN_GREEN]	=	{ 0, 1, 0},
	[CAVEMAN_SKIN_BLUE]		=	{ 0, 0, 1},
	[CAVEMAN_SKIN_GRAY]		=	{.5,.5,.5},
	[CAVEMAN_SKIN_RED]		=	{ 1, 0, 0},
	[CAVEMAN_SKIN_WHITE]	=	{ 1, 1, 1},
};

/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode	*gInfobarMasterObj = nil;

static float	gMapFit = 1;

float			gStartingLightTimer;

ObjNode			*gFinalPlaceObj = nil;

Boolean			gHideInfobar = false;

ObjNode			*gWinLoseString[MAX_PLAYERS];

ObjNode			*gInfobarIconObjs[MAX_SPLITSCREENS][NUM_INFOBAR_ICONTYPES][MAX_SUBICONS];


#pragma mark -

/********************* INFOBAR ICON POSITIONING ****************************/

#define GetIconScale(iconID) (gIconInfo[iconID].scale)
#define GetIconXSpacing(iconID) (gIconInfo[iconID].xSpacing)

static float GetIconX(Byte whichPane, int iconID)
{
	int anchor = gIconInfo[iconID].anchor;
	float dx = gIconInfo[iconID].xFromAnchor;
	float lw = gGameView->panes[whichPane].logicalWidth;

	switch (anchor & (kAnchorLeft | kAnchorRight))
	{
		case kAnchorLeft:
			return dx;

		case kAnchorRight:
			return dx + lw;

		default:
			return dx + lw * 0.5f;
	}
}

static float GetIconY(Byte whichPane, int iconID)
{
	int anchor = gIconInfo[iconID].anchor;
	float dy = gIconInfo[iconID].yFromAnchor;
	float lh = gGameView->panes[whichPane].logicalHeight;

	switch (anchor & (kAnchorTop | kAnchorBottom))
	{
		case kAnchorTop:
			return dy;

		case kAnchorBottom:
			return dy + lh;

		default:
			return dy + lh * 0.5f;
	}
}

#pragma mark -

/********************* PANE DIVIDER MESH **************************/

typedef struct
{
	int splitScreenMode;
	float logicalWidth;
	float logicalHeight;
} PaneDividerState;
CheckSpecialDataStruct(PaneDividerState);

static void MovePaneDivider(ObjNode* theNode)
{
	PaneDividerState* state = GetSpecialData(theNode, PaneDividerState);

	float overlayLogicalWidth = gGameView->panes[GetOverlayPaneNumber()].logicalWidth;
	float overlayLogicalHeight = gGameView->panes[GetOverlayPaneNumber()].logicalHeight;

	if (state->splitScreenMode == gActiveSplitScreenMode
		&& state->logicalWidth == overlayLogicalWidth
		&& state->logicalHeight == overlayLogicalHeight)
	{
		// Layout didn't change since last time
		return;
	}

	// Save params for which we are laying out the pane dividers
	state->splitScreenMode = gActiveSplitScreenMode;
	state->logicalWidth = overlayLogicalWidth;
	state->logicalHeight = overlayLogicalHeight;

	bool hideMe = (gActiveSplitScreenMode == SPLITSCREEN_MODE_NONE);
	SetObjectVisible(theNode, !hideMe);
	if (hideMe)
	{
		return;
	}

	MOVertexArrayData* mesh = GetQuadMeshWithin(theNode);
	GAME_ASSERT(mesh->pointCapacity == 4 * MAX_PANEDIVIDER_QUADS);
	GAME_ASSERT(mesh->triangleCapacity == 2 * MAX_PANEDIVIDER_QUADS);

	int p = 0;
	float halfThickness = (SPLITSCREEN_DIVIDER_THICKNESS + 1.0f) / 2.0f;
	float halfLW = overlayLogicalWidth * 0.5f + 10;
	float halfLH = overlayLogicalHeight * 0.5f + 10;

	// Divider quad types
	enum { EmptyDivider = 0, HAcross, VAcross, HLeftHalf, VUpperHalf };

	static const Byte kPaneDividerQuadDefs[NUM_SPLITSCREEN_MODES][MAX_PANEDIVIDER_QUADS] =
	{
		[SPLITSCREEN_MODE_NONE] = {EmptyDivider},
		[SPLITSCREEN_MODE_2P_WIDE] = {HAcross},
		[SPLITSCREEN_MODE_2P_TALL] = {VAcross},
		[SPLITSCREEN_MODE_3P_WIDE] = {HAcross, VUpperHalf},
		[SPLITSCREEN_MODE_3P_TALL] = {VAcross, HLeftHalf},
		[SPLITSCREEN_MODE_4P_GRID] = {HAcross, VAcross},
	};

	for (int i = 0; i < MAX_PANEDIVIDER_QUADS; i++)
	{
		switch (kPaneDividerQuadDefs[gActiveSplitScreenMode][i])
		{
			case	HAcross:
				mesh->points[p++] = (OGLPoint3D) { -halfLW, -halfThickness, 0 };
				mesh->points[p++] = (OGLPoint3D) { -halfLW, +halfThickness, 0 };
				mesh->points[p++] = (OGLPoint3D) { +halfLW, +halfThickness, 0 };
				mesh->points[p++] = (OGLPoint3D) { +halfLW, -halfThickness, 0 };
				break;

			case	VAcross:
				mesh->points[p++] = (OGLPoint3D) { -halfThickness, -halfLH, 0 };
				mesh->points[p++] = (OGLPoint3D) { -halfThickness, +halfLH, 0 };
				mesh->points[p++] = (OGLPoint3D) { +halfThickness, +halfLH, 0 };
				mesh->points[p++] = (OGLPoint3D) { +halfThickness, -halfLH, 0 };
				break;

			case	VUpperHalf:
				mesh->points[p++] = (OGLPoint3D) { -halfThickness, -halfLH, 0  };
				mesh->points[p++] = (OGLPoint3D) { -halfThickness, 0, 0  };
				mesh->points[p++] = (OGLPoint3D) { +halfThickness, 0, 0  };
				mesh->points[p++] = (OGLPoint3D) { +halfThickness, -halfLH, 0  };
				break;

			case	HLeftHalf:
				mesh->points[p++] = (OGLPoint3D) { -halfLW, -halfThickness, 0  };
				mesh->points[p++] = (OGLPoint3D) { -halfLW, +halfThickness, 0  };
				mesh->points[p++] = (OGLPoint3D) { 0, +halfThickness, 0  };
				mesh->points[p++] = (OGLPoint3D) { 0, -halfThickness, 0  };
				break;
		}
	}

	mesh->numPoints = p;
	mesh->numTriangles = p/2;
}

static ObjNode* MakePaneDivider(void)
{
	NewObjectDefinitionType def =
	{
		.scale		= 1,
		.slot		= PANEDIVIDER_SLOT,
		.projection	= kProjectionType2DOrthoCentered,
		.flags		= STATUS_BITS_FOR_2D | STATUS_BIT_OVERLAYPANE | STATUS_BIT_MOVEINPAUSE,
		.moveCall	= MovePaneDivider,
	};
	ObjNode* paneDivider = MakeQuadMeshObject(&def, MAX_PANEDIVIDER_QUADS, NULL);
	paneDivider->ColorFilter = (OGLColorRGBA) {0,0,0,1};
	return paneDivider;
}


#pragma mark -

/********************* INIT INFOBAR ****************************/
//
// Called at beginning of level
//

void InitInfobar(void)
{
static const char*	maps[] =
{
	"maps:DesertMap",
	"maps:JungleMap",
	"maps:IceMap",
	"maps:CreteMap",
	"maps:ChinaMap",
	"maps:EgyptMap",
	"maps:EuropeMap",
	"maps:ScandinaviaMap",
	"maps:AtlantisMap",
	"maps:StoneHengeMap",
	"maps:AztecMap",
	"maps:ColiseumMap",
	"maps:MazeMap",
	"maps:CelticMap",
	"maps:TarPitsMap",
	"maps:SpiralMap",
	"maps:RampsMap",
};


			/* RESET HIDE FLAG AT START OF AREA */

	gHideInfobar = false;

			/* RESET ROW ALLOCATIONS FO RPOW TIMERS */

	for (int p = 0; p < MAX_LOCAL_PLAYERS; p++)
	{
		for (int row = 0; row < MAX_POWTIMERS; row++)
		{
			gPOWTimersByRow[p][row] = -1;
		}
	}

			/* SETUP MASTER OBJECT */

	GAME_ASSERT(!gInfobarMasterObj);

	NewObjectDefinitionType masterObjDef =
	{
		.scale = 1,
		.slot = INFOBAR_SLOT,
		.genre = CUSTOM_GENRE,
		.flags = STATUS_BITS_FOR_2D,
		.projection = kProjectionType2DOrthoFullRect,
		.drawCall = DrawInfobar,
	};
	gInfobarMasterObj = MakeNewObject(&masterObjDef);


	MakeInfobar();

	

		/* SETUP STARTING LIGHT */

	gStartingLightTimer = 3.0;											// init starting light
	gFinalPlaceObj = nil;

	for (int i = 0; i < MAX_PLAYERS; i++)
		gWinLoseString[i] = nil;


			/* LOAD MAP SPRITE */

	LoadSpriteGroup(SPRITE_GROUP_OVERHEADMAP, maps[gTrackNum], kAtlasLoadAsSingleSprite);

	float mapImageHeight = GetSpriteInfo(SPRITE_GROUP_OVERHEADMAP, 1)->yadv;
	gMapFit = OVERHEAD_MAP_REFERENCE_SIZE / mapImageHeight;




	float bottomTextY = 225;

			/* PUT SELF-RUNNING DEMO MESSAGE UP */

	if (gIsSelfRunningDemo)
	{
		NewObjectDefinitionType def =
		{
			.coord		= {0, bottomTextY, 0},
			.scale		= .3,
			.slot		= PANEDIVIDER_SLOT+1,
			.moveCall	= MovePressAnyKey,
			.flags		= STATUS_BIT_OVERLAYPANE,
		};

		int strKey = gUserPrefersGamepad ? STR_PRESS_START : STR_PRESS_SPACE;
		TextMesh_New(Localize(strKey), 0, &def);

		bottomTextY -= 32;
	}


			/* TAMPER WARNING MESSAGE */

	if (gUserTamperedWithPhysics)
	{
		NewObjectDefinitionType def =
		{
			.coord		= {0, bottomTextY, 0},
			.scale		= .4,
			.slot		= PANEDIVIDER_SLOT+1,
			.moveCall	= MovePressAnyKey,
			.flags		= STATUS_BIT_OVERLAYPANE,
		};
		ObjNode* tamperedPhysicsText = TextMesh_New(Localize(STR_PHYSICS_EDITOR), 0, &def);
		tamperedPhysicsText->ColorFilter = (OGLColorRGBA) {1,0.4,0,1};

		bottomTextY -= 32;
	}


			/* PANE DIVIDER */

	MakePaneDivider();
}


/****************** DISPOSE INFOBAR **********************/

void DisposeInfobar(void)
{
	if (gInfobarMasterObj)
	{
		DeleteObject(gInfobarMasterObj);
		gInfobarMasterObj = nil;
	}

	memset(gInfobarIconObjs, 0, sizeof(gInfobarIconObjs));
}

static void Infobar_MakeIcon(uint8_t type, uint8_t flags)
{
	static void (*moveCallbacks[NUM_INFOBAR_ICONTYPES])(ObjNode*) =
	{
		[ICON_PLACE]			= Infobar_MovePlace,
		[ICON_LAP]				= Infobar_MoveLap,
		[ICON_WRONGWAY]			= Infobar_MoveWrongWay,
		[ICON_TOKEN]			= Infobar_MoveToken,
		[ICON_WEAPON_RACE]		= Infobar_MoveInventoryPOW,
		[ICON_WEAPON_BATTLE]	= Infobar_MoveInventoryPOW,
		[ICON_POWTIMER]			= Infobar_MovePOWTimer,
		[ICON_FLAG]				= Infobar_MoveFlag,
	};

	uint8_t sub = flags & 0x7f;
	bool isText = !!(flags & 0x80);

	GAME_ASSERT(type < NUM_INFOBAR_ICONTYPES);
	GAME_ASSERT(sub < MAX_SUBICONS);

	NewObjectDefinitionType def =
	{
		.scale = 1,
		.coord = {64 + type*32, 64, 0},
		.slot = INFOBAR_SLOT,
		.moveCall = moveCallbacks[type],
		.flags = STATUS_BITS_FOR_2D | STATUS_BIT_ONLYSHOWTHISPLAYER | STATUS_BIT_MOVEINPAUSE,
		.projection = kProjectionType2DOrthoFullRect,
	};

	if (!isText)
	{
		def.group = SPRITE_GROUP_INFOBAR;
		def.type = INFOBAR_SObjType_WrongWay;
	}

	for (int pane = 0; pane < gNumSplitScreenPanes; pane++)
	{
		ObjNode* obj = NULL;

		if (isText)
			obj = TextMesh_New("?", kTextMeshAlignLeft, &def);
		else
			obj = MakeSpriteObject(&def);

		obj->PlayerNum = GetPlayerNum(pane);		// TODO: maybe revise this for net games

		InfobarIconData* special = GetInfobarIconData(obj);
		special->type = type;
		special->sub = sub;
		special->displayedValue = -1;
//		special->allocatedRow = -1;

		gInfobarIconObjs[pane][type][sub] = obj;
	}
}

static void Infobar_RepositionIconTemp(ObjNode* theNode)
{
	const InfobarIconData* special = GetInfobarIconData(theNode);
	int pane = theNode->PlayerNum;
	int id = special->type;
	int sub = special->sub;
	theNode->Coord.x = GetIconX(pane, id) + sub * GetIconXSpacing(id);
	theNode->Coord.y = GetIconY(pane, id);
	theNode->Scale.x = GetIconScale(id);
	theNode->Scale.y = GetIconScale(id);
}

static void Infobar_RepositionIcon(ObjNode* theNode)
{
	Infobar_RepositionIconTemp(theNode);
	UpdateObjectTransforms(theNode);
}

static void MakeInfobar(void)
{
	int weaponIconType;
	switch (gGameMode)
	{
		case GAME_MODE_TAG1:
		case GAME_MODE_TAG2:
		case GAME_MODE_SURVIVAL:
		case GAME_MODE_CAPTUREFLAG:
			weaponIconType = ICON_WEAPON_BATTLE;
			break;
		default:
			weaponIconType = ICON_WEAPON_RACE;
			break;
	}

	// Inventory
	Infobar_MakeIcon(weaponIconType, 0x00);
	Infobar_MakeIcon(weaponIconType, 0x01);
	Infobar_MakeIcon(weaponIconType, 0x82);		// text

	// Buff timers
	for (int i = 0; i < MAX_POWTIMERS; i++)
	{
		Infobar_MakeIcon(ICON_POWTIMER, 0x00 | (i*2 + 0));	// icon
		Infobar_MakeIcon(ICON_POWTIMER, 0x80 | (i*2 + 1));	// text
	}

	//Infobar_DrawMap();
	//Infobar_DrawStartingLight();

	/* DRAW GAME MODE SPECIFICS */

	if (IsRaceMode())
	{
		Infobar_MakeIcon(ICON_PLACE, 0);		// big number
		Infobar_MakeIcon(ICON_PLACE, 1);		// ordinal
		Infobar_MakeIcon(ICON_WRONGWAY, 0);
		Infobar_MakeIcon(ICON_LAP, 0);
	}

	switch (gGameMode)
	{
		case	GAME_MODE_PRACTICE:
		case	GAME_MODE_MULTIPLAYERRACE:
			break;

		case	GAME_MODE_TOURNAMENT:
			for (int i = 0; i < MAX_TOKENS; i++)
			{
				Infobar_MakeIcon(ICON_TOKEN, i);
			}
			break;

		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
			//Infobar_DrawTagTimer();
			break;

		case	GAME_MODE_SURVIVAL:
			//Infobar_DrawHealth();
			break;

		case	GAME_MODE_CAPTUREFLAG:
			for (int i = 0; i < NUM_FLAGS_TO_GET; i++)
			{
				Infobar_MakeIcon(ICON_FLAG, i);
			}
			break;
	}
}


/********************** DRAW INFOBAR ****************************/

static void DrawInfobar(ObjNode* theNode)
{
	if (gHideInfobar)
		return;

		/***************/
		/* DRAW THINGS */
		/***************/
		// STATUS_BITS_FOR_2D has already set the tedious OpenGL 2D state for us.


		/* DRAW THE STANDARD THINGS */

	Infobar_DrawMap(gCurrentSplitScreenPane);
	Infobar_DrawStartingLight(gCurrentSplitScreenPane);


		/* DRAW GAME MODE SPECIFICS */

	switch(gGameMode)
	{
		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
				Infobar_DrawTagTimer(gCurrentSplitScreenPane);
				break;

		case	GAME_MODE_SURVIVAL:
				Infobar_DrawHealth(gCurrentSplitScreenPane);
				break;
	}
}



/********************** DRAW MAP *******************************/

static void GetPointOnOverheadMap(Byte whichPane, float* px, float* pz)
{
	float x = *px;
	float z = *pz;

	float scale = GetIconScale(ICON_MAP);

	x /= gTerrainUnitWidth;		// get 0..1 coordinate values
	z /= gTerrainUnitDepth;

	x = x * 2.0f - 1.0f;								// convert to -1..1
	z = z * 2.0f - 1.0f;

	x *= scale * OVERHEAD_MAP_REFERENCE_SIZE * .5f;		// shrink to size of underlay map
	z *= scale * OVERHEAD_MAP_REFERENCE_SIZE * .5f;

	x += GetIconX(whichPane, ICON_MAP);							// position it
	z += GetIconY(whichPane, ICON_MAP);

	*px = x;
	*pz = z;
}

static void Infobar_DrawMap(Byte whichPane)
{
	short	p = GetPlayerNum(whichPane);


	if (gGameMode == GAME_MODE_TAG1)						// if in TAG 1 mode, tagged player is only one with a map
	{
		if (!gPlayerInfo[p].isIt)
			return;
	}
	else
	if (gGameMode == GAME_MODE_TAG2)						// if in TAG 2 mode, tagged player is only one without a map
	{
		if (gPlayerInfo[p].isIt)
			return;
	}


	float scale = GetIconScale(ICON_MAP);


			/* DRAW THE MAP UNDERLAY */

	DrawSprite(SPRITE_GROUP_OVERHEADMAP, 1,
			GetIconX(whichPane, ICON_MAP),
			GetIconY(whichPane, ICON_MAP),
			scale * gMapFit,
			INFOBAR_SPRITE_FLAGS);


			/***********************/
			/* DRAW PLAYER MARKERS */
			/***********************/

	gGlobalTransparency = .9f;

	for (int i = gNumTotalPlayers-1; i >= 0; i--)			// draw from last to first so that player 1 is always on "top"
	{
		if (p != i											// if this isnt me then see if hidden
			&& gPlayerInfo[i].invisibilityTimer > 0.0f)
		{
			continue;
		}

			/* ORIENT IT */

		float x = gPlayerInfo[i].coord.x;
		float z = gPlayerInfo[i].coord.z;
		GetPointOnOverheadMap(whichPane, &x, &z);

		float scaleBasis = ((i == p) ? 10 : 7);				// draw my marker bigger
		scaleBasis *= scale;
		scaleBasis *= .1f;

		float rot = PI - gPlayerInfo[i].objNode->Rot.y;		// "pi minus angle" because Y points down for us

			/* SET COLOR */

		switch(gGameMode)
		{
			case	GAME_MODE_TAG1:
			case	GAME_MODE_TAG2:
					if (gPlayerInfo[i].isIt)						// in tag mode, all players are white except for the tagged guy
						gGlobalColorFilter = gTagColor;
					else
						gGlobalColorFilter = (OGLColorRGB) {1,1,1};
					break;

			default:
					gGlobalColorFilter = kCavemanSkinColors[gPlayerInfo[i].skin];
		}

		
		DrawSprite2(
			SPRITE_GROUP_INFOBAR,
			INFOBAR_SObjType_PlayerBlip,
			x,
			z,
			scaleBasis,
			scaleBasis,
			rot,
			INFOBAR_SPRITE_FLAGS);
	}

	gGlobalColorFilter = (OGLColorRGB) {1,1,1};
	gGlobalTransparency = 1.0f;

			/**********************/
			/* DRAW TORCH MARKERS */
			/**********************/

	for (int i = 0; i < gNumTorches; i++)
	{
		if (gTorchObjs[i]->Mode == 2) 		// dont draw if this torch is at base
			continue;

			/* ORIENT IT */

		float x = gTorchObjs[i]->Coord.x;
		float z = gTorchObjs[i]->Coord.z;
		GetPointOnOverheadMap(whichPane, &x, &z);

		float scaleBasis = scale * 7.0f;					// calculate a scale basis to keep things scaled relative to texture size
		scaleBasis *= .05f;

			/* SET COLOR */

		int sprite = (gTorchObjs[i]->TorchTeam & 1) == 0
			? INFOBAR_SObjType_RedTorchBlip
			: INFOBAR_SObjType_GreenTorchBlip;

				/* DRAW IT */

		DrawSprite(SPRITE_GROUP_INFOBAR, sprite,
			x, z, scaleBasis,
			INFOBAR_SPRITE_FLAGS);
	}
}


/********************** DRAW PLACE *************************/

static int LocalizeOrdinalSprite(int place, int sex)
{
	switch (gGamePrefs.language)
	{
		case LANGUAGE_ENGLISH:
		default:
			switch (place)
			{
				case 0: return INFOBAR_SObjType_PlaceST;
				case 1: return INFOBAR_SObjType_PlaceND;
				case 2: return INFOBAR_SObjType_PlaceRD;
				default: return INFOBAR_SObjType_PlaceTH;
			}
			break;

		case LANGUAGE_FRENCH:
			if (place == 0)
				return sex==1? INFOBAR_SObjType_PlaceRE: INFOBAR_SObjType_PlaceER;
			else
				return INFOBAR_SObjType_PlaceE;
	}
}

static void Infobar_MovePlace(ObjNode* node)
{
	int playerNum = node->PlayerNum;
	int place = gPlayerInfo[playerNum].place;
	int sex = gPlayerInfo[playerNum].sex;

	InfobarIconData* special = GetInfobarIconData(node);

	if (special->displayedValue != place)
	{
		if (gCameraStartupTimer <= 0)					// only twitch once the race has started
		{
			bool gain = place < special->displayedValue;	// gain if rank--
			MakeTwitch(node, gain ? kTwitchPreset_RankGain : kTwitchPreset_RankLoss);
		}

		special->displayedValue = place;
	}

	switch (GetInfobarIconData(node)->sub)
	{
		case 0:
			ModifySpriteObjectFrame(node, INFOBAR_SObjType_Place1+place);
			break;

		case 1:
			ModifySpriteObjectFrame(node, LocalizeOrdinalSprite(place, sex));
			break;

		default:
			ModifySpriteObjectFrame(node, INFOBAR_SObjType_WrongWay);
	}

	Infobar_RepositionIcon(node);
}

/********************** DRAW WEAPON TYPE *************************/

static void Infobar_MoveInventoryPOW(ObjNode* node)
{
PlayerInfoType* pi = &gPlayerInfo[node->PlayerNum];

enum
{
	kDormant,
	kActive,
	kVanishing,
};

	InfobarIconData* special = GetInfobarIconData(node);

	bool hasAnyPow = pi->powType != POW_TYPE_NONE;

	switch (special->state)
	{
		case kDormant:
			if (!hasAnyPow)
			{
				SetObjectVisible(node, false);
			}
			else
			{
				// earned POW, go active
				SetObjectVisible(node, true);
				special->state = kActive;
				MakeTwitch(node, kTwitchPreset_NewWeapon);
			}
			break;

		case kActive:
			if (!hasAnyPow)
			{
				// went inactive
				MakeTwitch(node, kTwitchPreset_POWExpired);
				special->state = kVanishing;
			}
			break;

		case kVanishing:
			if (hasAnyPow)
			{
				// got a new POW as we were vanishing
				special->state = kActive;
			}
			else if (node->TwitchNode == NULL)
			{
				special->state = kDormant;
				special->displayedValue = -1;
				SetObjectVisible(node, false);
			}
			break;
	}

	if (node->StatusBits & STATUS_BIT_HIDDEN)
	{
		return;
	}

	Infobar_RepositionIconTemp(node);

	switch (GetInfobarIconData(node)->sub)
	{
		case 0:		// DRAW WEAPON ICON
			if (hasAnyPow)	// if we lost the POW, retain previous sprite
			{
				ModifySpriteObjectFrame(node, INFOBAR_SObjType_Weapon_Bone + pi->powType);

				if (special->displayedValue != pi->powType &&
					special->displayedValue != -1)
				{
					node->StatusBits |= STATUS_BIT_KEEPBACKFACES;
					MakeTwitch(node, kTwitchPreset_WeaponFlip);
				}

				special->displayedValue = pi->powType;
			}
			break;

		case 1:		// DRAW X-QUANTITY ICON
			ModifySpriteObjectFrame(node, INFOBAR_SObjType_WeaponX);
			node->Scale.x = node->Scale.y *= .8f;
			break;

		case 2:		// DRAW QUANTITY NUMBER
		{
			if (special->displayedValue != pi->powQuantity)
			{
				char s[8];
				snprintf(s, sizeof(s), "%d", pi->powQuantity);
				TextMesh_Update(s, kTextMeshAlignLeft, node);

				if (!node->TwitchNode)
				{
					bool gain = pi->powQuantity > special->displayedValue;		// gain if qty++
					MakeTwitch(node, gain ? kTwitchPreset_ItemGain : kTwitchPreset_ItemLoss);
				}

				special->displayedValue = pi->powQuantity;
			}

			node->Coord.x -= 22;
			node->Scale.x = node->Scale.y *= .7f;
			node->ColorFilter = (OGLColorRGBA) {.4, 1.0, .3, 1.0};
			break;
		}
	}

	UpdateObjectTransforms(node);
}


/********************** DRAW WRONG WAY *************************/

static void Infobar_MoveWrongWay(ObjNode* objNode)
{
Boolean	wrongWay;
short	p = objNode->PlayerNum;

	wrongWay = gPlayerInfo[p].wrongWay;

	SetObjectVisible(objNode, wrongWay);

	if (wrongWay)
	{
		ModifySpriteObjectFrame(objNode, INFOBAR_SObjType_WrongWay);
		Infobar_RepositionIcon(objNode);
	}
}


/********************** DRAW STARTING LIGHT *************************/

static void Infobar_DrawStartingLight(Byte whichPane)
{
int		oldTimer;

	if (gGamePaused)
		return;

			/* CHECK TIMER */

	if (gStartingLightTimer <= 0.0f)
		return;

	oldTimer = gStartingLightTimer;

	if (gCameraStartupTimer < .2f)									// dont tick down until camera intro is about done
		gStartingLightTimer -= gFramesPerSecondFrac / (float)gNumSplitScreenPanes;
	else
		return;


			/* DRAW THE NEEDED SPRITE */

	int spriteNo;

	if (gStartingLightTimer <= 1.0f)								// green
	{
		gNoCarControls = false;										// once green we have control
		spriteNo = INFOBAR_SObjType_Go;
	}
	else
	if (gStartingLightTimer <= 2.0f)								// yellow
	{
		spriteNo = INFOBAR_SObjType_Set;
	}
	else															// red
	{
		spriteNo = INFOBAR_SObjType_Ready;
	}

	DrawSprite(SPRITE_GROUP_INFOBAR, spriteNo,
			GetIconX(whichPane, ICON_STARTLIGHT),
			GetIconY(whichPane, ICON_STARTLIGHT),
			GetIconScale(ICON_STARTLIGHT),
			INFOBAR_SPRITE_FLAGS);



			/* IF CHANGED, THEN BEEP */

	if (oldTimer != (int)gStartingLightTimer)
	{
		static const short voice[] = {EFFECT_GO, EFFECT_SET, EFFECT_READY};

		PlayAnnouncerSound(voice[(int)gStartingLightTimer], true, 0);
	}
}





/********************** DRAW LAP *************************/

static void Infobar_MoveLap(ObjNode* node)
{
int	lap,playerNum;

			/*******************************/
			/* DRAW THE CURRENT LAP NUMBER */
			/*******************************/

	playerNum = node->PlayerNum;

	lap = gPlayerInfo[playerNum].lapNum;
	lap = GAME_CLAMP(lap, 0, 2);

	ModifySpriteObjectFrame(node, INFOBAR_SObjType_Lap1of3+lap);
	Infobar_RepositionIcon(node);
}

/********************** DRAW TOKENS *************************/

static void Infobar_MoveToken(ObjNode* node)
{
	int playerNum = node->PlayerNum;
	int numTokens = gPlayerInfo[playerNum].numTokens;
	int nthToken = GetSpecialData(node, InfobarIconData)->sub;

	bool hasToken = nthToken < numTokens;

	InfobarIconData* data = GetInfobarIconData(node);
	if (hasToken && data->displayedValue != hasToken)
	{
		MakeTwitch(node, kTwitchPreset_ArrowheadSpin);
		node->StatusBits |= STATUS_BIT_KEEPBACKFACES;
		data->displayedValue = hasToken;
	}

	ModifySpriteObjectFrame(node, hasToken ? INFOBAR_SObjType_Token_Arrowhead : INFOBAR_SObjType_Token_ArrowheadDim);
	Infobar_RepositionIcon(node);
}


/********************* DRAW TIMER POWERUPS **************************/

static int GetRowForPOWTimer(int playerNum, int powTimerID)
{
	for (int row = 0; row < MAX_POWTIMERS; row++)
	{
		if (gPOWTimersByRow[playerNum][row] == powTimerID)
		{
			return row;
		}
	}

	return -1;
}

static int AllocRowForNewPOWTimer(int playerNum, int powTimerID)
{
	for (int row = 0; row < MAX_POWTIMERS; row++)
	{
		if (gPOWTimersByRow[playerNum][row] < 0)
		{
			gPOWTimersByRow[playerNum][row] = powTimerID;
			return row;
		}
	}

	return -1;
}

static void FreeRowTakenByPOWTimer(int playerNum, int powTimerID)
{
	int8_t* rowAllocation = &gPOWTimersByRow[playerNum][0];

	int row = GetRowForPOWTimer(playerNum, powTimerID);

	if (row >= 0)
	{
#if 0
		int numToMove = MAX_POWTIMERS - row - 1;

		if (numToMove != 0)
		{
			memmove(&rowAllocation[row],
					&rowAllocation[row+1],
					numToMove * sizeof(rowAllocation[0]));

			rowAllocation[MAX_POWTIMERS-1] = -1;
		}
#else
		rowAllocation[row] = -1;
#endif
	}
}


static void Infobar_MovePOWTimer(ObjNode* objNode)
{
static const OGLColorRGBA tint = {1,.7,.5,1};
enum
{
	kDormant		= 0,
	kActive,
	kVanishing,
};

	short p = objNode->PlayerNum;
	InfobarIconData* special = GetInfobarIconData(objNode);

	int timerID = special->sub >> 1;
	int subInRow = special->sub & 1;

	size_t timerOffset = kPOWTimerDefs[timerID].timerFloatOffset;
	float timer = *(float*) (((char*) &gPlayerInfo[p]) + timerOffset);
	bool isTicking = timer > 0;

	int currentRow = GetRowForPOWTimer(p, timerID);
	bool hasRow = currentRow >= 0;

	switch (special->state)
	{
		case kDormant:
		default:
			if (!isTicking)
			{
				SetObjectVisible(objNode, false);
			}
			else
			{
				// Started ticking, reserve row
				if (!hasRow)
				{
					currentRow = AllocRowForNewPOWTimer(p, timerID);
					hasRow = currentRow >= 0;
				}

				// If we have a row, appear
				if (hasRow)
				{
					special->state = kActive;
					MakeTwitch(objNode, kTwitchPreset_NewBuff);
					SetObjectVisible(objNode, true);
				}
				else
				{
					SetObjectVisible(objNode, false);
				}
			}
			break;

		case kActive:
			if (!isTicking)		// Wait for ticking to end to change states
			{
				MakeTwitch(objNode, kTwitchPreset_POWExpired);
				special->state = kVanishing;
			}
			break;

		case kVanishing:
			if (!isTicking)
			{
				// Wait for vanish effect to finish
				if (objNode->TwitchNode == NULL)
				{
					FreeRowTakenByPOWTimer(p, timerID);
					special->state = kDormant;
					SetObjectVisible(objNode, false);
				}
			}
			else
			{
				// Started ticking again, go back to active state
				special->state = kActive;
			}
			break;
	}

	if (currentRow < 0									// no room!
		|| objNode->StatusBits & STATUS_BIT_HIDDEN)
	{
		return;
	}

	Infobar_RepositionIconTemp(objNode);

	objNode->Coord.x += subInRow * 37.0f;
	objNode->Coord.y += currentRow * 46.0f;

	switch (subInRow)
	{
		case 0:		// DRAW ICON
			ModifySpriteObjectFrame(objNode, kPOWTimerDefs[timerID].sprite);
			break;

		case 1:		// DRAW TIME
		{
			int q = (int) (timer+.5f);

			if (special->displayedValue != q)
			{
				char s[8];
				snprintf(s, sizeof(s), "%d", q);
				TextMesh_Update(s, kTextMeshAlignLeft, objNode);
				special->displayedValue = q;

//				if (!objNode->TwitchNode)
//					MakeTwitch(objNode, kTwitchPreset_MenuSelect);
			}

			objNode->Scale.x = objNode->Scale.y *= .6f;
			objNode->ColorFilter = tint;
			break;
		}
	}

	UpdateObjectTransforms(objNode);
}

/********************* DRAW TAG TIMER **************************/

static void Infobar_DrawTagTimer(Byte whichPane)
{
short	p,p2;
float	timer,x,y, scale, spacing;

	x			= GetIconX(whichPane, ICON_TIMER);
	y			= GetIconY(whichPane, ICON_TIMER);
	scale		= GetIconScale(ICON_TIMER);
	spacing		= GetIconXSpacing(ICON_TIMER);


			/********************/
			/* DRAW THE TIMEBAR */
			/********************/

				/* DRAW BAR */

	DrawSprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_TimeBar,	x, y, scale, INFOBAR_SPRITE_FLAGS);



		/* DRAW THE TIME MARKER */

	x			= GetIconX(whichPane, ICON_TIMERINDEX);
	y			= GetIconY(whichPane, ICON_TIMERINDEX);
	scale		= GetIconScale(ICON_TIMERINDEX);
	spacing		= GetIconXSpacing(ICON_TIMERINDEX);


	p = GetPlayerNum(whichPane);
	timer = (gPlayerInfo[p].tagTimer / TAG_TIME_LIMIT);							// get timer value 0..1
	x += timer * spacing;

	DrawSprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_Marker, x, y, scale, INFOBAR_SPRITE_FLAGS);


		/**********************************************/
		/* ALSO DRAW TIME MARKER FOR TAGGED OPPONENT */
		/**********************************************/

	if (gWhoIsIt != p)							// only if it isn't us
	{
		p2 = gWhoIsIt;							// in tag, show timer of tagged player

		x = GetIconX(whichPane, ICON_TIMERINDEX);
		timer = (gPlayerInfo[p2].tagTimer / TAG_TIME_LIMIT);							// get timer value 0..1
		x += timer * spacing;

		gGlobalColorFilter = gTagColor;							// tint
		gGlobalTransparency = .35;

		DrawSprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_Marker, x, y, scale, INFOBAR_SPRITE_FLAGS);

		gGlobalColorFilter.r =
		gGlobalColorFilter.g =
		gGlobalColorFilter.b =
		gGlobalTransparency = 1;
	}


}


/********************* DRAW HEALTH **************************/

static void Infobar_DrawHealth(Byte whichPane)
{
short	p,p2;
float	timer,x,y, scale, spacing, dist;

	x			= GetIconX(whichPane, ICON_TIMER);
	y			= GetIconY(whichPane, ICON_TIMER);
	scale		= GetIconScale(ICON_TIMER);
	spacing		= GetIconXSpacing(ICON_TIMER);


			/********************/
			/* DRAW THE TIMEBAR */
			/********************/

				/* DRAW BAR */

	DrawSprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_TimeBar,	x, y, scale, INFOBAR_SPRITE_FLAGS);



		/* DRAW THE TIME MARKER */

	x			= GetIconX(whichPane, ICON_TIMERINDEX);
	y			= GetIconY(whichPane, ICON_TIMERINDEX);
	scale		= GetIconScale(ICON_TIMERINDEX);
	spacing		= GetIconXSpacing(ICON_TIMERINDEX);


	p = GetPlayerNum(whichPane);
	timer = gPlayerInfo[p].health;							// get timer value 0..1
	x += timer * spacing;

	DrawSprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_Marker, x, y, scale, INFOBAR_SPRITE_FLAGS);


		/**********************************************/
		/* ALSO DRAW TIME MARKER FOR CLOSEST OPPONENT */
		/**********************************************/

	p2 = FindClosestPlayerInFront(gPlayerInfo[p].objNode, 10000, false, &dist, .5);			// check directly ahead
	if (p2 == -1)
		p2 = FindClosestPlayer(gPlayerInfo[p].objNode, gPlayerInfo[p].coord.x, gPlayerInfo[p].coord.z, 20000, false, &dist);

	if (p2 != -1)
	{
		x = GetIconX(whichPane, ICON_TIMERINDEX);
		timer = gPlayerInfo[p2].health;
		x += timer * spacing;

		gGlobalColorFilter.r = 1;							// tint
		gGlobalColorFilter.g = 0;
		gGlobalColorFilter.b = 0;
		gGlobalTransparency = .35;

		DrawSprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_Marker, x, y, scale, INFOBAR_SPRITE_FLAGS);

		gGlobalColorFilter.r =
		gGlobalColorFilter.g =
		gGlobalColorFilter.b =
		gGlobalTransparency = 1;
	}

}


/********************* DRAW FLAGS **************************/

static void Infobar_MoveFlag(ObjNode* theNode)
{
	int p = theNode->PlayerNum;
	int team = gPlayerInfo[p].team;
	int numCapturedFlags = gCapturedFlagCount[team];
	InfobarIconData* data = GetInfobarIconData(theNode);

	int flagNum = NUM_FLAGS_TO_GET - data->sub - 1;		// invert order so torches appear right-to-left

	if (SetObjectVisible(theNode, flagNum < numCapturedFlags))
	{
		ModifySpriteObjectFrame(theNode, INFOBAR_SObjType_RedTorch + team);
		Infobar_RepositionIcon(theNode);
	}
}




#pragma mark -



/****************** SHOW LAPNUM *********************/
//
// Called from Checkpoints.c whenever the player completes a new lap (except for winning lap).
//

void ShowLapNum(short playerNum)
{
short	lapNum;

	if ((!gPlayerInfo[playerNum].onThisMachine) || gPlayerInfo[playerNum].isComputer)
		return;

	lapNum = gPlayerInfo[playerNum].lapNum;


			/* SEE IF TELL LAP */

	if (lapNum > 0)
	{
		PlayAnnouncerSound(EFFECT_LAP2 + lapNum - 1,false, 0);

		NewObjectDefinitionType def =
		{
			.coord		= {0,0,0},
			.scale		= .7f,
			.slot		= SPRITE_SLOT,
            .flags      = STATUS_BIT_ONLYSHOWTHISPLAYER
		};

		const char* s = Localize(lapNum == 1 ? STR_LAP_2 : STR_LAP_3);
		ObjNode* text = TextMesh_New(s, kTextMeshAlignCenter, &def);
        text->PlayerNum = playerNum;

		MakeTwitch(text, kTwitchPreset_MenuSelect);
		MakeTwitch(text, kTwitchPreset_FadeOutLapMessage | kTwitchFlags_Chain | kTwitchFlags_KillPuppet);
	}
}


/****************** SHOW FINAL PLACE *********************/
//
// Called from Checkpoints.c whenever the player completes a race.
//

void ShowFinalPlace(short playerNum, int rankInScoreboard)
{
short	place;
short	sex;

	if ((!gPlayerInfo[playerNum].onThisMachine) || gPlayerInfo[playerNum].isComputer)
		return;

	place = gPlayerInfo[playerNum].place;
	sex = gPlayerInfo[playerNum].sex;

			/* ANNOUNCE PLACE */

	PlayAnnouncerSound(EFFECT_1st + place, true, 1.0);

			/* MAKE NUMBER SPRITE */

	NewObjectDefinitionType spriteDef =
	{
		.group 		= SPRITE_GROUP_INFOBAR,
		.type		= INFOBAR_SObjType_Place1+place,
		.coord		= {0,0,0},
		.flags		= STATUS_BIT_ONLYSHOWTHISPLAYER,
		.slot		= SPRITE_SLOT,
		.moveCall	= MoveFinalPlace,
		.scale		= 1.5,
	};

	if (gGamePrefs.language == LANGUAGE_FRENCH)					// french has shorter ordinals, keep sprite centered
	{
		spriteDef.coord.x += 18;
	}

	gFinalPlaceObj = MakeSpriteObject(&spriteDef);

	gFinalPlaceObj->PlayerNum = playerNum;						// only show for this player

			/* MAKE ORDINAL SPRITE ON TOP OF NUMBER */

	spriteDef.slot++;
	spriteDef.type = LocalizeOrdinalSprite(place, sex);
	ObjNode* ordinalObj = MakeSpriteObject(&spriteDef);
	ordinalObj->PlayerNum = playerNum;

	AppendNodeToChain(gFinalPlaceObj, ordinalObj);

			/* MAKE RACE TIME TEXT */

	if (IsRaceMode())
	{
		char timeStr[64];
		snprintf(timeStr, sizeof(timeStr), "%s: %s", Localize(STR_YOUR_TIME), FormatRaceTime(GetRaceTime(playerNum)));

		NewObjectDefinitionType textDef =
		{
			.slot = GetChainTailNode(gFinalPlaceObj)->Slot + 1,
			.coord = {0,100,0},
			.scale = 0.4,
		};
		ObjNode* yourTimeObj = TextMesh_New(timeStr, 0, &textDef);

		AppendNodeToChain(gFinalPlaceObj, yourTimeObj);


			/* MAKE NEW RECORD TEXT IF RANKED 1ST & SCOREBOARD WASN'T EMPTY */

		textDef.slot++;
		if (rankInScoreboard == 0
			&& SumLapTimes(gScoreboard.records[gTrackNum][1].lapTimes) > 0)
		{
			textDef.coord.y += 32;
			ObjNode* newRecordObj = TextMesh_New(Localize(STR_NEW_RECORD), 0, &textDef);
			AppendNodeToChain(gFinalPlaceObj, newRecordObj);
		}
	}
}


/**************** MOVE FINAL PLACE ***************************/

static void MoveFinalPlace(ObjNode *theNode)
{
//	theNode->Rot.z = sin(theNode->SpecialF[0]) * .2f;
	theNode->SpecialF[0] += gFramesPerSecondFrac * 5.0f;
//	UpdateObjectTransforms(theNode);
}

#pragma mark -


/********************** DEC CURRENT POW QUANTITY *********************/

void DecCurrentPOWQuantity(short playerNum)
{
	gPlayerInfo[playerNum].powQuantity -= 1;
	if (gPlayerInfo[playerNum].powQuantity <= 0)
		gPlayerInfo[playerNum].powType = POW_TYPE_NONE;
}


#pragma mark -


/****************** SHOW WIN LOSE *********************/
//
// Used for battle modes to show player if they win or lose.
//
// INPUT: 	mode 0 : elimminated
//			mode 1 : won
//			mode 2 : lost
//

void ShowWinLose(short playerNum, Byte mode, short winner)
{
			/* ONLY DO THIS FOR HUMANS ON THIS MACHINE */

	if ((!gPlayerInfo[playerNum].onThisMachine) || gPlayerInfo[playerNum].isComputer)
		return;


		/* SAY THINGS ONLY IF NETWORK */

	if (gNetGameInProgress)
	{
		switch(mode)
		{
			case	0:
			case	2:
					PlayAnnouncerSound(EFFECT_YOULOSE, true, .5);
					break;

			default:
					PlayAnnouncerSound(EFFECT_YOUWIN, true, .5);
		}
	}


	if (gWinLoseString[playerNum])							// see if delete existing message (usually to change ELIMINATED to xxx WINS
	{
		DeleteObject(gWinLoseString[playerNum]);
		gWinLoseString[playerNum] = nil;
	}


			/* MAKE SPRITE OBJECT */

	NewObjectDefinitionType def =
	{
		.coord		= { 0,0,0 },
		.flags		= STATUS_BIT_ONLYSHOWTHISPLAYER,
		.slot		= SPRITE_SLOT,
		.scale		= .8f,
	};

	switch(mode)
	{
		case	0:
				gWinLoseString[playerNum] = TextMesh_New(Localize(STR_ELIMINATED), 0, &def);
				break;

		case	1:
				gWinLoseString[playerNum] = TextMesh_New(Localize(STR_YOU_WIN), 0, &def);
				break;

		case	2:
				if (gNetGameInProgress && (gGameMode != GAME_MODE_CAPTUREFLAG) && (winner != -1))		// if net game & not CTF, then show name of winner
				{
					char s[64];
					char safePlayerName[32];

					memset(s, 0, sizeof(s));
					memset(safePlayerName, 0, sizeof(safePlayerName));

					int j = 0;
					for (int i = 0; i < 20; i++)									// copy name
					{
						char c = gPlayerNameStrings[winner][i];

						if ((c >= 'a') && (c <= 'z'))								// convert to UPPER CASE
							c = 'A' + (c - 'a');

						if (strchr(PLAYER_NAME_SAFE_CHARSET, c))					// safe characters
							safePlayerName[j++] = c;
					}

					snprintf(s, sizeof(s), "%s %s", safePlayerName, Localize(STR_3RDPERSON_WINS));	// insert "WINS"

					gWinLoseString[playerNum] = TextMesh_New(s, 0, &def);
				}
				else
				{
					gWinLoseString[playerNum] = TextMesh_New(Localize(STR_YOU_LOSE), 0, &def);
				}
				break;

	}


	gWinLoseString[playerNum]->PlayerNum = playerNum;						// only show for this player

}


#pragma mark -

/******************** MAKE TRACK NAME ***************************/

void MakeIntroTrackName(void)
{
ObjNode	*newObj;

	NewObjectDefinitionType def =
	{
		.coord		= {0,0,0},
		.slot 		= SPRITE_SLOT,
		.scale 	    = .8f,
		.flags		= STATUS_BIT_MOVEINPAUSE,
	};

	newObj = TextMesh_New(Localize(STR_LEVEL_1 + gTrackNum), kTextMeshAlignCenter, &def);

	MakeTwitch(newObj, kTwitchPreset_FadeOutTrackName | kTwitchFlags_KillPuppet);
}


/******************** MOVE PRESS ANY KEY TEXT *************************/

static void MovePressAnyKey(ObjNode* theNode)
{
	theNode->SpecialF[0] += gFramesPerSecondFrac * 4.0f;
	theNode->ColorFilter.a = 0.66f + sinf(theNode->SpecialF[0]) * 0.33f;
}

