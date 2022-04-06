//
// miscscreens.h
//

void DisplayPicture(const char* picturePath, float timeout);
void DoPaused(void);
void ShowAgePicture(int age);
void ShowLoadingPicture(void);

void DoMainMenuScreen(void);
void DoGameSettingsDialog(void);

Boolean DoFailedMenu(const char* headerString);
void DoTitleScreen(void);
void DoAgeConqueredScreen(void);

Boolean DoCharacterSelectScreen(short whichPlayer, Boolean allowAborting);
void DoCreditsScreen(void);
void DoTournamentWinScreen(void);

void DoHelpScreen(void);
Boolean SelectSingleTrack(void);

void DoMultiPlayerVehicleSelections(void);
Boolean DoVehicleSelectScreen(short whichPlayer, Boolean allowAborting);

Boolean DoLocalGatherScreen(void);

