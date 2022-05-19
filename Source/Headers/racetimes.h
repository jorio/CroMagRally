#pragma once

#include "main.h"

Boolean IsRaceMode(void);
char* FormatRaceTime(float t);
void TickLapTimes(int playerNum);
float SumLapTimes(const float lapTimes[LAPS_PER_RACE]);
float GetRaceTime(int playerNum);
int SaveRaceTime(int playerNum);

void DoScoreboardScreen(void);
