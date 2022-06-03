/****************************/
/*    RACE TIMES.C          */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include "game.h"
#include "menu.h"
#include <time.h>

#define MAX_RECORDS_PER_SCREEN 5
#define NODES_PER_RECORD 6
#define MAX_TODAYS_RECORDS 64

Scoreboard gScoreboard;

static Byte gScoreboardTrack = 0;
static ObjNode* gRecordChainHead = 0;

static struct
{
	int active : 1;
	int track : 7;
	int rank : 8;
} gTodaysRecords[MAX_TODAYS_RECORDS];

static void OnScoreboardTrackChanged(const MenuItem* mi);
static void LayOutScoreboardForTrack(void);

Boolean IsRaceMode(void)
{
	switch (gGameMode)
	{
	case GAME_MODE_PRACTICE:
	case GAME_MODE_TOURNAMENT:
	case GAME_MODE_MULTIPLAYERRACE:
		return true;

	default:
		return false;
	}
}

char* FormatRaceTime(float t)
{
	if (t <= 0)
	{
		return "0:00\v.00";
	}

	static char timeBuf[16];

	int min = (int) (t * (1.0f / 60.0f));
	int sec = ((int) t) % 60;
	int cent = ((int) (t * 100.0f)) % 100;

	snprintf(timeBuf, sizeof(timeBuf), "%d:%02d\v.%02d", min, sec, cent);

	return timeBuf;
}

void TickLapTimes(int playerNum)
{
	if (!IsRaceMode())
	{
		return;
	}

	PlayerInfoType* pi = &gPlayerInfo[playerNum];

	int lap = GAME_MAX(0, pi->lapNum);		// lap number is negative before we cross finish line at start of race

	if (!gNoCarControls						// start ticking when Brian says go
		&& lap < LAPS_PER_RACE				// lap number gets bumped to 3 when crossing finish line at end of race
		&& !pi->raceComplete)				// stop ticking when race is complete
	{
		pi->lapTimes[lap] += gFramesPerSecondFrac;
	}
}

float SumLapTimes(const float lapTimes[LAPS_PER_RACE])
{
	float total = 0;

	for (int lapNum = 0; lapNum < LAPS_PER_RACE; lapNum++)
	{
		total += lapTimes[lapNum];
	}

	return total;
}

float GetRaceTime(int playerNum)
{
	return SumLapTimes(gPlayerInfo[playerNum].lapTimes);
}

static int GetNumRecordsToday(void)
{
	int n;

	for (n = 0; n < MAX_TODAYS_RECORDS; n++)
	{
		if (gTodaysRecords[n].active == 0)
			break;
	}

	return n;
}

static Boolean IsNewRecord(int track, int rank)
{
	for (int i = 0; i < MAX_TODAYS_RECORDS; i++)
	{
		if (gTodaysRecords[i].active == 0)
		{
			return false;
		}
		else if (gTodaysRecords[i].track == track && gTodaysRecords[i].rank == rank)
		{
			return true;
		}
	}

	return false;
}

// Returns new rank, or negative value if not rankable
int SaveRaceTime(int playerNum)
{
	const PlayerInfoType* pi = &gPlayerInfo[playerNum];
	float raceTime = GetRaceTime(playerNum);
	ScoreboardRecord* trackRecords = gScoreboard.records[gTrackNum];

	// Don't save race time if...
	if (!IsRaceMode()						// we're not in a race
		|| pi->isComputer					// it's a CPU player
		|| gIsSelfRunningDemo				// it's a CPU player in SRD (they're considered "human" in SRD)
		|| gUserTamperedWithPhysics			// we're playing with non-stock physics
		|| pi->cheated						// we completed the race early by pressing B+R+I
		)
	{
		return -1;
	}

	// Bail if race time doesn't look legit
	for (int i = 0; i < gNumLapsThisRace; i++)
	{
		if (pi->lapTimes[i] < 0.1f)			// probably cheated with B-R-I to complete track early
			return -2;
	}

	// Work out our rank in the scoreboard
	int rank = 0;
	for (rank = 0; rank < MAX_RECORDS_PER_TRACK; rank++)
	{
		float recordTime = SumLapTimes(trackRecords[rank].lapTimes);

		if (recordTime <= 0					// zero or negative times implies that the slot is empty
			|| raceTime < recordTime)		// our time is better than all following times (records are sorted in descending order)
		{
			break;
		}
	}

	// Bail if all times in scoreboard are better than ours
	if (rank >= MAX_RECORDS_PER_TRACK)
	{
		return -3;
	}

	// Make room for our record so the scoreboard stays sorted
	int numRecordsToMove = MAX_RECORDS_PER_TRACK - rank - 1;
	if (numRecordsToMove > 0)
	{
		memmove(&trackRecords[rank + 1], &trackRecords[rank], numRecordsToMove * sizeof(trackRecords[0]));
	}

	// Fill record struct
	ScoreboardRecord* myRecord = &trackRecords[rank];
	memset(myRecord, 0, sizeof(*myRecord));
	memcpy(myRecord->lapTimes, pi->lapTimes, sizeof(myRecord->lapTimes));
	myRecord->timestamp		= time(NULL);
	myRecord->difficulty	= gGamePrefs.difficulty;
	myRecord->gameMode		= gGameMode;
	myRecord->trackNum		= gTrackNum;
	myRecord->vehicleType	= pi->vehicleType;
	myRecord->place			= pi->place;
	myRecord->sex			= pi->sex;
	myRecord->skin			= pi->skin;

	int todaysRecordNum = GetNumRecordsToday();
	if (todaysRecordNum >= MAX_TODAYS_RECORDS)
		todaysRecordNum = MAX_TODAYS_RECORDS-1;
	gTodaysRecords[todaysRecordNum].active = 1;
	gTodaysRecords[todaysRecordNum].track = gTrackNum;
	gTodaysRecords[todaysRecordNum].rank = rank;

	// Save the file
	SaveScoreboardFile();

	// Return our rank so infobar can tell user if they beat the record
	return rank;
}

#pragma mark -

static void SetupScoreboardScreen(void)
{
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .1, .1, .1, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLColorRGBA		fillColor2 = { .3, .3, .3, 1 };
OGLVector3D			fillDirection1 = { .9, -.7, -1 };
OGLVector3D			fillDirection2 = { -1, -.2, -.5 };


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 				= 1;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.camera.from[0].z		= 700;
	viewDef.camera.from[0].y		= 250;
	viewDef.view.clearColor 		= (OGLColorRGBA) {0,0,0, 1.0f};
	viewDef.styles.useFog			= false;
	viewDef.view.pillarboxRatio		= PILLARBOX_RATIO_4_3;
	viewDef.view.fontName			= "rockfont";

	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);
	OGLVector3D_Normalize(&fillDirection2, &fillDirection2);

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= fillColor1;
	viewDef.lights.fillColor[1] 	= fillColor2;

	OGL_SetupGameView(&viewDef);


				/************/
				/* LOAD ART */
				/************/

	LoadSpriteGroup(SPRITE_GROUP_SCOREBOARDSCREEN, "scoreboard", 0);

			/* BACKGROUND */

	MakeScrollingBackgroundPattern();

			/* FADE IN */

	MakeFadeEvent(true);

			/* SHOW INITIAL TRACK INFO */

	LayOutScoreboardForTrack();
}


static const MenuItem gScoreboardMenuTree[] =
{
	{.id='scbd'},
	{
		kMICycler1,
		STR_NULL,
		.callback = OnScoreboardTrackChanged,
		.cycler =
		{
			.valuePtr = &gScoreboardTrack,
			.choices =
			{
				{STR_LEVEL_1, 0},
				{STR_LEVEL_2, 1},
				{STR_LEVEL_3, 2},
				{STR_LEVEL_4, 3},
				{STR_LEVEL_5, 4},
				{STR_LEVEL_6, 5},
				{STR_LEVEL_7, 6},
				{STR_LEVEL_8, 7},
				{STR_LEVEL_9, 8},
			}
		},
		.customHeight = 1.3,
	},
	{0},
};

void DoScoreboardScreen(void)
{
	GAME_ASSERT(gRecordChainHead == NULL);

	int numRecordsToday = GetNumRecordsToday();
	if (numRecordsToday >= 1)
	{
		gScoreboardTrack = gTodaysRecords[numRecordsToday-1].track;
	}

	SetupScoreboardScreen();

	MenuStyle style = kDefaultMenuStyle;
	style.yOffset = -170;
	style.canBackOutOfRootMenu = true;

	StartMenu(gScoreboardMenuTree, &style, MoveObjects, DrawObjects);

	DeleteAllObjects();
	OGL_DisposeGameView();

	gRecordChainHead = NULL;

	DisposeSpriteGroup(SPRITE_GROUP_SCOREBOARDSCREEN);
}

static void LayOutScoreboardForTrack(void)
{
	if (gRecordChainHead)
	{
		DeleteObject(gRecordChainHead);
		gRecordChainHead = NULL;
	}


	static const OGLColorRGBA colors[]  =
	{
			{1.0,1,1,1},
			{.88,.88,1,1},
			{.76,.76,.9,1},
			{.64,.64,.8,1},
			{.52,.52,.7,1},
	};


	float x = -100;
	float y = -90;

	NewObjectDefinitionType defRank = {.coord = {x-130, y, 0}, .scale = 0.75,};
	NewObjectDefinitionType defCar  = {.coord = {x-50, y, 0}, .scale = 0.2, .group=SPRITE_GROUP_SCOREBOARDSCREEN};
	NewObjectDefinitionType defTime = {.coord = {x-20, y, 0}, .scale = 0.6,};
	NewObjectDefinitionType defLaps = {.coord = {x+150, y, 0}, .scale = 0.2,};
	NewObjectDefinitionType defDate = {.coord = {x+260, y, 0}, .scale = 0.2,};
	NewObjectDefinitionType defNew  = {.coord = {x-130, y, 0}, .scale = 0.75,};

	short slot = MENU_SLOT;
	const char* lapAbbrev = Localize(STR_LAP_ABBREV);
	char text[128];

	for (int row = 0; row < GAME_MIN(MAX_RECORDS_PER_SCREEN, MAX_RECORDS_PER_TRACK); row++)
	{
		int n = 0;
		ObjNode* nodes[NODES_PER_RECORD];
		memset(nodes, 0, sizeof(nodes));

		const ScoreboardRecord* record = &gScoreboard.records[gScoreboardTrack][row];

		float raceTime = SumLapTimes(record->lapTimes);

		if (IsNewRecord(gScoreboardTrack, row))
		{
			defNew.slot = slot++;
			nodes[n++] = TextMesh_New("NEW!", 0, &defNew);
			nodes[n-1]->Rot.z = -PI/8;
			nodes[n-1]->ColorFilter = (OGLColorRGBA) { .8, .2, .2, 1 };
		}
		else
		{
			defRank.slot = slot++;
			snprintf(text, sizeof(text), "#%d", row + 1);
			nodes[n++] = TextMesh_New(text, 0, &defRank);
		}

		if (raceTime <= 0)
		{
			defTime.slot = slot++;
			nodes[n++] = TextMesh_New("-------------------", kTextMeshAlignLeft, &defTime);
		}
		else
		{
			defTime.slot = slot++;
			nodes[n++] = TextMesh_New(FormatRaceTime(raceTime), kTextMeshAlignLeft, &defTime);

			text[0] = '\0';
			snprintfcat(text, sizeof(text), "%s1: %s", lapAbbrev, FormatRaceTime(record->lapTimes[0]));
			snprintfcat(text, sizeof(text), "\n%s2: %s", lapAbbrev, FormatRaceTime(record->lapTimes[1]));
			snprintfcat(text, sizeof(text), "\n%s3: %s", lapAbbrev, FormatRaceTime(record->lapTimes[2]));

			defLaps.slot = slot++;
			nodes[n++] = TextMesh_New(text, kTextMeshAlignLeft, &defLaps);

			time_t timestamp = (time_t) record->timestamp;
			struct tm *timeinfo = localtime(&timestamp);
			int day = timeinfo->tm_mday;
			const char* month = Localize(timeinfo->tm_mon + STR_MONTH_1);
			int year = 1900 + timeinfo->tm_year;

			snprintf(text, sizeof(text), "%s %s", Localize(STR_DIFFICULTY_1 + record->difficulty), Localize(STR_SIMPLISTIC + record->difficulty));
			if (gGamePrefs.language == LANGUAGE_ENGLISH)
				snprintfcat(text, sizeof(text), "\n%s %d, %d", month, day, year);
			else
				snprintfcat(text, sizeof(text), "\n%d %s %d", day, month, year);

			defDate.slot = slot++;
			nodes[n++] = TextMesh_New(text, kTextMeshAlignLeft, &defDate);

			if (record->vehicleType != CAR_TYPE_SUB)
			{
				defCar.slot = slot++;
				defCar.type = 1 + record->vehicleType;
				nodes[n++] = MakeSpriteObject(&defCar);
			}
		}



		for (n = 0; n < NODES_PER_RECORD; n++)
		{
			if (!nodes[n])
				continue;

			OGLColorRGBA oldColor = nodes[n]->ColorFilter;
			if (oldColor.r == 1 && oldColor.g == 1 && oldColor.b == 1)		// do gradient if the node doesn't set a custom color
			{
				nodes[n]->ColorFilter = colors[row];
			}

			Twitch* t = MakeTwitch(nodes[n], kTwitchPreset_ItemLoss);
			if (t)
			{
				t->delay = row * 0.05f;
				t->amplitude *= 3;

			}

			t = MakeTwitch(nodes[n], kTwitchPreset_ScoreboardFadeIn | kTwitchFlags_Chain);
			if (t)
			{
				t->delay = row * 0.05f;
			}

			if (!gRecordChainHead)
				gRecordChainHead = nodes[n];
			else
				AppendNodeToChain(gRecordChainHead, nodes[n]);
		}

		defTime.coord.y += 60;
		defLaps.coord.y += 60;
		defDate.coord.y += 60;
		defRank.coord.y += 60;
		defCar.coord.y += 60;
		defNew.coord.y += 60;
	}
}

static void OnScoreboardTrackChanged(const MenuItem* mi)
{
	LayOutScoreboardForTrack();
}
