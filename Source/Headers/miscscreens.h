//
// miscscreens.h
//

#include "menu.h"

void DoWarmUpScreen(void);
void DisplayPicture(const char* picturePath, float timeout);
void DoPaused(void);
void ShowAgePicture(int age);
void ShowLoadingPicture(void);
void SetupGenericMenuScreen(bool withEscPrompt);

void DoMainMenuScreen(void);

Boolean DoFailedMenu(const char* headerString);
void DoTitleScreen(void);
void DoAgeConqueredScreen(void);

Boolean DoCharacterSelectScreen(short whichPlayer, Boolean allowAborting);
void DoCreditsScreen(void);
void DoTournamentWinScreen(void);

void DoHelpScreen(void);
Boolean SelectSingleTrack(void);

Boolean DoMultiPlayerVehicleSelections(void);
Boolean DoVehicleSelectScreen(short whichPlayer, Boolean allowAborting);

Boolean DoLocalGatherScreen(void);

Boolean DoNetGatherScreen(void);
void ShowPostGameNetErrorScreen(void);

void DoPhysicsEditor(void);

void RegisterSettingsMenu(const MenuItem* junk);

void SetupNetPauseScreen(void);
void RemoveNetPauseScreen(void);
