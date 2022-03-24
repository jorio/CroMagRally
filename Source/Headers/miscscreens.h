//
// miscscreens.h
//

void DisplayPicture(FSSpec *spec, Boolean showAndBail, Boolean doKeyText);
void DoPaused(void);
void ShowAgePicture(int age);
void ShowLoadingPicture(void);

Boolean DoFailedMenu(const char* headerString);
void DoTitleScreen(void);
void DoAgeConqueredScreen(void);

Boolean DoCharacterSelectScreen(short whichPlayer, Boolean allowAborting);
void DoCreditsScreen(void);
void DoTournamentWinScreen(void);

void DoHelpScreen(void);