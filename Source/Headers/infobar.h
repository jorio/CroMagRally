//
// infobar.h
//

#define	MAX_TOKENS		8

#define	TAG_TIME_LIMIT	((float)(gTagDuration) * 60.0)			// n minutes


void InitInfobar(void);
void DisposeInfobar(void);
void ShowLapNum(short playerNum);
void ShowFinalPlace(short playerNum, int rankInScoreboard);
void DecCurrentPOWQuantity(short playerNum);
void ShowWinLose(short playerNum, Byte mode, short winner);
void MakeIntroTrackName(void);
