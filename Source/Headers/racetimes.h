#pragma once

char* FormatRaceTime(float t);
void TickLapTimes(int playerNum);
float SumLapTimes(const float lapTimes[LAPS_PER_RACE]);
float GetRaceTime(int playerNum);
Boolean IsRaceMode();
int SaveRaceTime(int playerNum);
