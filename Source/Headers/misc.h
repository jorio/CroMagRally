//
// misc.h
//

#pragma once

#include <stdio.h>

#define REG_LENGTH      12


void	DoAlert(const char* format, ...);
void	DoFatalAlert(const char* format, ...);
extern void	Wait(long);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	void FloatToString(float num, Str255 string);
extern	Handle	AllocHandle(long size);
extern	void *AllocPtr(long size);
void *AllocPtrClear(long size);
extern	void PStringToC(char *pString, char *cString);
float StringToFloat(Str255 textStr);
extern	void DrawCString(char *string);
extern	void InitMyRandomSeed(void);
extern	void VerifySystem(void);
extern	float RandomFloat(void);
u_short	RandomRange(unsigned short min, unsigned short max);
extern	void RegulateSpeed(short fps);
extern	void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr);
extern	void ShowSystemErr_NonFatal(long err);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);

void CopyPString(Str255 from, Str255 to);

void SafeDisposePtr(Ptr ptr);

void MyFlushEvents(void);

void CheckGameRegistration(void);



#define ShowSystemErr(err) DoFatalAlert("Fatal system error: %d", err)
#define ShowSystemErr_NonFatal(err) DoFatalAlert("System error: %d", err)

#define IMPLEMENT_ME() DoFatalAlert("IMPLEMENT ME! %s:%d", __func__, __LINE__)
#define IMPLEMENT_ME_SOFT() printf("IMPLEMENT ME (soft): %s:%d\n", __func__, __LINE__)

#define GAME_ASSERT(condition) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, #condition); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, message); } while(0)
