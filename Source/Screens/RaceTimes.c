#include "game.h"

Boolean IsRaceMode()
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
	if (t == 0)
	{
		return "-'--''--";
	}

	static char timeBuf[16];

	int min = (int) (t * (1.0f / 60.0f));
	int sec = ((int) t) % 60;
	int cent = ((int) (t * 100.0f)) % 100;

	snprintf(timeBuf, sizeof(timeBuf), "%d'%02d''%02d", min, sec, cent);

	return timeBuf;
}

void TickLapTimes(int playerNum)
{
	PlayerInfoType* pi = &gPlayerInfo[playerNum];

	if (pi->lapNum < 0)		// lap number is negative before we cross finish line at start of race
	{
		return;
	}

	if (IsRaceMode())
	{
		GAME_ASSERT(pi->lapNum < LAPS_PER_RACE);

		pi->lapTimes[pi->lapNum] += gFramesPerSecondFrac;
	}
}

float GetRaceTime(int playerNum)
{
	PlayerInfoType* pi = &gPlayerInfo[playerNum];
	float total = 0;

	for (int lapNum = 0; lapNum < LAPS_PER_RACE; lapNum++)
	{
		total += pi->lapTimes[lapNum];
	}

	return total;
}
