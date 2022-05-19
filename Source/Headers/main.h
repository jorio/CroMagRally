//
// main.h
//

#pragma once

#define	MAX_PLAYERS			6
#define	MAX_LOCAL_PLAYERS	4

#define	GAME_FOV		1.1f

#define	TRACK_COMPLETE_COOLDOWN_TIME	5.0				// n seconds

#define	NUM_FLAGS_TO_GET	6		// # flags to get in Capture Flag mode

enum
{
	GAME_MODE_PRACTICE = 0,
	GAME_MODE_TOURNAMENT,
	GAME_MODE_MULTIPLAYERRACE,
	GAME_MODE_TAG1,				// player does NOT want to be it
	GAME_MODE_TAG2,				// player DOES want to be it
	GAME_MODE_SURVIVAL,
	GAME_MODE_CAPTUREFLAG,

	NUM_GAME_MODES
};


enum
{
	STONE_AGE,
	BRONZE_AGE,
	IRON_AGE,

	NUM_AGES
};

#define	AGE_MASK_AGE	0x0f			// lower 4 bits are age
#define	AGE_MASK_STAGE	0xf0			// upper 4 bits are stage within age


#define	TRACKS_PER_AGE	3			// # tracks per age in tournament mode

#define LAPS_PER_RACE	3
#define MAX_RECORDS_PER_TRACK	10

enum
{
	TRACK_NUM_DESERT = 0,
	TRACK_NUM_JUNGLE,
	TRACK_NUM_ICE,

	TRACK_NUM_CRETE,
	TRACK_NUM_CHINA,
	TRACK_NUM_EGYPT,

	TRACK_NUM_EUROPE,
	TRACK_NUM_SCANDINAVIA,
	TRACK_NUM_ATLANTIS,

	NUM_RACE_TRACKS,

	TRACK_NUM_STONEHENGE = NUM_RACE_TRACKS,
	TRACK_NUM_AZTEC,
	TRACK_NUM_COLISEUM,
	TRACK_NUM_MAZE,
	TRACK_NUM_CELTIC,
	TRACK_NUM_TARPITS,
	TRACK_NUM_SPIRAL,
	TRACK_NUM_RAMPS,

	NUM_TRACKS
};


enum
{
	DIFFICULTY_SIMPLISTIC,
	DIFFICULTY_EASY,
	DIFFICULTY_MEDIUM,
	DIFFICULTY_HARD,

	NUM_DIFFICULTIES
};

enum
{
	RED_TEAM,
	GREEN_TEAM,
};

enum
{
	CAVEMAN_SKIN_BROWN,
	CAVEMAN_SKIN_GREEN,
	CAVEMAN_SKIN_BLUE,
	CAVEMAN_SKIN_GRAY,
	CAVEMAN_SKIN_RED,
	CAVEMAN_SKIN_WHITE,
	NUM_CAVEMAN_SKINS
};


//=================================================


void ToolBoxInit(void);
void FadeOutArea(void);
void MoveEverything(void);
void InitDefaultPrefs(void);
