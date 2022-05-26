#pragma once

enum
{
	kTwitchScaleHard = 1,
	kTwitchScaleSoft,
	kTwitchDisplaceLeft,
	kTwitchDisplaceRight,
	kTwitchLightDisplaceLeft,
	kTwitchLightDisplaceRight,
	kTwitchQuickWiggle,
	kTwitchBigWiggle,
	kTwitchPresetCOUNT,
};

enum
{
	kTwitchClassScale = 1,
	kTwitchClassDisplaceX,
	kTwitchClassWiggleX,
	kTwitchClassCOUNT,
};

enum
{
	kEaseLerp = 0,
	kEaseInQuad,
	kEaseOutQuad,
	kEaseOutCubic,
};

typedef struct
{
	Byte fxClass;
	Byte easing;
	float duration;
	float period;
	float amplitude;
	float seed;
} TwitchDef;

ObjNode* MakeTwitch(ObjNode* puppet, int preset);
TwitchDef* GetTwitchParameters(ObjNode* driver);

ObjNode* MakeScrollingBackgroundPattern(void);
