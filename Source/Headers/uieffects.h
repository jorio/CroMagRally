#pragma once

enum
{
	kTwitchScaleHard = 1,
	kTwitchScaleSoft,
	kTwitchDisplaceLTR,
	kTwitchDisplaceRTL,
	kTwitchLightDisplaceLTR,
	kTwitchLightDisplaceRTL,
	kTwitchLightDisplaceUTD,
	kTwitchLightDisplaceDTU,
	kTwitchYBounce,
	kTwitchQuickWiggle,
	kTwitchBigWiggle,
	kTwitchSubtleWiggle,
	kTwitchScaleUp,
	kTwitchArrowheadSpin,
	kTwitchCoinSpin,
	kTwitchSweepFromLeft,
	kTwitchSweepFromTop,
	kTwitchScaleVanish,
	kTwitchPresetCOUNT,
};

enum
{
	kTwitchClassScale = kTwitchPresetCOUNT,		// deliberately starting above last preset # so presets and classes cannot be mixed up
	kTwitchClassSpinX,
	kTwitchClassDisplaceX,
	kTwitchClassDisplaceY,
	kTwitchClassWiggleX,
	kTwitchClassScaleVanish,
};

enum
{
	kEaseLerp = 0,
	kEaseInQuad,
	kEaseOutQuad,
	kEaseOutCubic,
	kEaseInBack,
	kEaseOutBack,
	kEaseBounce0To0,
};

typedef struct
{
	Byte fxClass;
	Byte easing;
	float duration;
	float delay;
	float period;
	float amplitude;
	float phase;
	float seed;
} TwitchDef;

ObjNode* MakeTwitch(ObjNode* puppet, uint8_t preset);
TwitchDef* GetTwitchParameters(ObjNode* driver);

ObjNode* MakeScrollingBackgroundPattern(void);
