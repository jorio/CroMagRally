#pragma once

enum
{
	kTwitchScaleIn = 1,
	kTwitchScaleOut,
	kTwitchDisplaceLeft,
	kTwitchDisplaceRight,
	kTwitchQuickWiggle,
	kTwitchBigWiggle,
	kTwitchCOUNT,
};


ObjNode* MakeTwitch(ObjNode* puppet, int type);

ObjNode* MakeScrollingBackgroundPattern(void);
