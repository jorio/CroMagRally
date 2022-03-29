//
// window.h
//

#define	ALLOW_FADE		(1)

extern void	InitWindowStuff(void);
extern void	DumpGWorld2(GWorldPtr, WindowPtr, Rect *);
extern	void MakeFadeEvent(Boolean	fadeIn);
void Enter2D(Boolean pauseDSp);
void Exit2D(void);
void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect);

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaFadeIn(void);
extern	void GammaOn(void);

extern	void GameScreenToBlack(void);

void DoLockPixels(GWorldPtr world);

void ChangeWindowScale(void);
void Wait(long ticks);

void SetFullscreenMode(bool enforceDisplayPref);
