#pragma once

enum
{
	kTwitchPreset_NULL = 0,
	kTwitchPreset_MenuSelect,
	kTwitchPreset_MenuDeselect,
	kTwitchPreset_MenuWiggle,
	kTwitchPreset_PadlockWiggle,
	kTwitchPreset_DisplaceLTR,
	kTwitchPreset_DisplaceRTL,
	kTwitchPreset_SlideshowLTR,
	kTwitchPreset_SlideshowRTL,
	kTwitchPreset_ItemGain,
	kTwitchPreset_ItemLoss,
	kTwitchPreset_RankGain,
	kTwitchPreset_RankLoss,
	kTwitchPreset_ArrowheadSpin,
	kTwitchPreset_WeaponFlip,
	kTwitchPreset_NewBuff,
	kTwitchPreset_NewWeapon,
	kTwitchPreset_POWExpired,
	kTwitchPresetCOUNT,

	kTwitchClass_Scale = kTwitchPresetCOUNT,		// deliberately starting above last preset # so presets and classes cannot be mixed up
	kTwitchClass_SpinX,
	kTwitchClass_DisplaceX,
	kTwitchClass_DisplaceY,
	kTwitchClass_WiggleX,
	kTwitchClass_Shrink,

	kEaseLinear,
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
