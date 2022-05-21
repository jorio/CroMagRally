#pragma once

enum
{
	kTwitchScaleIn = 1,
	kTwitchScaleOut,
	kTwitchLeft,
	kTwitchRight,
	kTwitchQuickWiggle,
	kTwitchBigWiggle,
	kTwitchCOUNT,
};


ObjNode* MakeTwitch(ObjNode* puppet, int type);

void MoveUIArrow(ObjNode* theNode);
void TwitchUIArrow(ObjNode* theNode, float x, float y);
ObjNode* MakeScrollingBackgroundPattern(void);
