/****************************/
/*    RACE TIMES.C          */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include "game.h"
#include "uielements.h"
#include "menu.h"
#include <time.h>

Scoreboard gScoreboard;

static Byte gScoreboardTrack = 0;
static ObjNode* gRecordChainHead = 0;

static void OnScoreboardTrackChanged(const MenuItem* mi);

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
		return "-'--''---";
	}

	static char timeBuf[16];

	int min = (int) (t * (1.0f / 60.0f));
	int sec = ((int) t) % 60;
	int cent = ((int) (t * 100.0f)) % 100;

	snprintf(timeBuf, sizeof(timeBuf), "%d'%02d\v.%02d", min, sec, cent);

	return timeBuf;
}

void TickLapTimes(int playerNum)
{
	if (!IsRaceMode())
	{
		return;
	}

	PlayerInfoType* pi = &gPlayerInfo[playerNum];

	if (pi->lapNum >= 0						// lap number is negative before we cross finish line at start of race
		&& pi->lapNum < LAPS_PER_RACE)		// lap number gets bumped to 3 when crossing finish line at end of race
	{
		pi->lapTimes[pi->lapNum] += gFramesPerSecondFrac;
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
	viewDef.view.pillarboxRatio	= PILLARBOX_RATIO_4_3;
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


			/* BACKGROUND */

	MakeScrollingBackgroundPattern();

			/* FADE IN */

	MakeFadeEvent(true);

//	UpdateShadowCarStats();
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
		.customHeight = 1.5,
	},
	{0},
};

void DoScoreboardScreen(void)
{
	GAME_ASSERT(gRecordChainHead == NULL);

	SetupScoreboardScreen();

	MenuStyle style = kDefaultMenuStyle;
	style.yOffset = -200;
	style.canBackOutOfRootMenu = true;

	int outcome = StartMenu(gScoreboardMenuTree, &style, MoveObjects, DrawObjects);

	DeleteAllObjects();
	OGL_DisposeGameView();

	gRecordChainHead = NULL;
}

static void LayOutScoreboardForTrack(void)
{
	if (gRecordChainHead)
	{
		DeleteObject(gRecordChainHead);
		gRecordChainHead = NULL;
	}

	NewObjectDefinitionType def1 =
	{
		.coord = {-200, -110, 0},
		.scale = 0.5,
	};

	NewObjectDefinitionType def2 =
	{
		.coord = {-23, -105, 0},
		.scale = 0.2,
	};
	NewObjectDefinitionType def3 =
	{
		.coord = {-200, -110+20, 0},
		.scale = 0.2,
	};

	short slot = MENU_SLOT;
	const char* lapStr = Localize(STR_LAP);

	for (int i = 0; i < GAME_MIN(10, MAX_RECORDS_PER_TRACK); i++)
	{
		const ScoreboardRecord* record = &gScoreboard.records[gScoreboardTrack][i];

		def1.slot = slot++;

		float raceTime = SumLapTimes(record->lapTimes);
		char* raceTimeStr = FormatRaceTime(raceTime);
		ObjNode* textNode = TextMesh_New(raceTimeStr, kTextMeshAlignLeft, &def1);

		if (gRecordChainHead)
			AppendNodeToChain(gRecordChainHead, textNode);
		else
			gRecordChainHead = textNode;

		char lapsBuf[128];
		lapsBuf[0] = '\0';
		snprintfcat(lapsBuf, sizeof(lapsBuf), "%s 1: %s", lapStr, FormatRaceTime(record->lapTimes[0]));
		snprintfcat(lapsBuf, sizeof(lapsBuf), "\n%s 2: %s", lapStr, FormatRaceTime(record->lapTimes[1]));
		snprintfcat(lapsBuf, sizeof(lapsBuf), "\n%s 3: %s", lapStr, FormatRaceTime(record->lapTimes[2]));


		def2.slot = slot++;
		ObjNode* textNode2 = TextMesh_New(lapsBuf, kTextMeshAlignLeft, &def2);
		AppendNodeToChain(gRecordChainHead, textNode2);

		time_t timestamp = (time_t) record->timestamp;
		struct tm *timeinfo = localtime(&timestamp);
		int day = timeinfo->tm_mday;
		const char* month = Localize(timeinfo->tm_mon + STR_MONTH_1);
		int year = 1900 + timeinfo->tm_year;

		if (gGamePrefs.language == LANGUAGE_ENGLISH)
		{
			snprintf(lapsBuf, sizeof(lapsBuf), "%s %d, %d", month, day, year);
		}
		else if (gGamePrefs.language == LANGUAGE_GERMAN)
		{
			snprintf(lapsBuf, sizeof(lapsBuf), "%d. %s %d", day, month, year);
		}
		else
		{
			snprintf(lapsBuf, sizeof(lapsBuf), "%d %s %d", day, month, year);
		}

		def3.slot = slot++;
		ObjNode* textNode3 = TextMesh_New(lapsBuf, kTextMeshAlignLeft, &def3);
		AppendNodeToChain(gRecordChainHead, textNode3);

		def1.coord.y += 80;
		def2.coord.y += 80;
		def3.coord.y += 80;
	}
}

static void OnScoreboardTrackChanged(const MenuItem* mi)
{
	LayOutScoreboardForTrack();
}
