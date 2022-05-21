#pragma once

enum
{
	kTwitchScaleIn = 1,
	kTwitchScaleOut,
	kTwitchLeft,
	kTwitchRight,
	kTwitchWiggle,
	kTwitchCOUNT,
};


ObjNode* MakeTwitch(ObjNode* puppet, int type);

void MoveUIArrow(ObjNode* theNode);
void TwitchUIArrow(ObjNode* theNode, float x, float y);
void MoveUIPadlock(ObjNode* theNode);
void WiggleUIPadlock(ObjNode* theNode);
ObjNode* MakeScrollingBackgroundPattern(void);
