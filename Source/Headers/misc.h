//
// misc.h
//

#pragma once

#include <stdio.h>

void	DoAlert(const char* format, ...);
POMME_NORETURN void DoFatalAlert(const char* format, ...);
POMME_NORETURN void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	Handle	AllocHandle(long size);
extern	void *AllocPtr(long size);
void *AllocPtrClear(long size);
extern	void InitMyRandomSeed(void);
int PositiveModulo(int value, unsigned int m);
void InitPrefsFolder(bool createIt);
extern	float RandomFloat(void);
uint16_t	RandomRange(unsigned short min, unsigned short max);
extern	void ShowSystemErr_NonFatal(long err);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);
void SafeDisposePtr(Ptr ptr);
void MyFlushEvents(void);
size_t snprintfcat(char* buf, size_t bufSize, char const* fmt, ...);
void AdvanceTextCursor(int snprintfReturnCode, char** cursor, size_t* remainingSize);

#define ShowSystemErr(err) DoFatalAlert("Fatal system error: %d", err)
#define ShowSystemErr_NonFatal(err) DoFatalAlert("System error: %d", err)

#if _DEBUG
	#define IMPLEMENT_ME_SOFT() printf("IMPLEMENT ME: %s:%d\n", __func__, __LINE__)
#else
	#define IMPLEMENT_ME_SOFT()
#endif

#define GAME_ASSERT(condition) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, #condition); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, message); } while(0)
