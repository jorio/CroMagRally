//
// window.h
//

#pragma once

void InitWindowStuff(void);
ObjNode* MakeFadeEvent(Boolean fadeIn);
void OGL_FadeOutScene(OGLSetupOutputType* setupInfo, void (*drawRoutine)(OGLSetupOutputType*), void (*updateRoutine)(void));
void Enter2D(Boolean pauseDSp);
void Exit2D(void);
void SetFullscreenMode(bool enforceDisplayPref);
