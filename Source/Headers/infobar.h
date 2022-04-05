//
// infobar.h
//

#define	MAX_TOKENS		8

//#define	TAG_TIME_LIMIT	(2.0 * 60.0)			// n minutes
#define	TAG_TIME_LIMIT	((float)(gGamePrefs.tagDuration) * 60.0)			// n minutes


void InitInfobar(OGLSetupOutputType *setupInfo);
void DrawInfobar(OGLSetupOutputType *setupInfo);
void DisposeInfobar(void);
void ShowLapNum(short playerNum);
void ShowFinalPlace(short playerNum);
void DecCurrentPOWQuantity(short playerNum);
void ShowWinLose(short playerNum, Byte mode, short winner);
void MakeIntroTrackName(void);
