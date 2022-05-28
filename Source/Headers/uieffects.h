#pragma once

#define MAX_TWITCHES 256

enum
{
	kTwitchPreset_NULL = 0,
	kTwitchPreset_MenuSelect,
	kTwitchPreset_MenuDeselect,
	kTwitchPreset_MenuWiggle,
	kTwitchPreset_MenuFadeIn,
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
	kTwitchPreset_FadeOutLapMessage,
	kTwitchPreset_FadeOutTrackName,
	kTwitchPresetCOUNT,

	kTwitchClass_Scale = kTwitchPresetCOUNT,		// deliberately starting above last preset # so presets and classes cannot be mixed up
	kTwitchClass_SpinX,
	kTwitchClass_DisplaceX,
	kTwitchClass_DisplaceY,
	kTwitchClass_WiggleX,
	kTwitchClass_Shrink,
	kTwitchClass_MultAlphaFadeIn,
	kTwitchClass_MultAlphaFadeOut,
	kTwitchClass_OverrideAlphaFadeIn,
	kTwitchClass_OverrideAlphaFadeOut,

	kEaseLinear,
	kEaseInQuad,
	kEaseOutQuad,
	kEaseOutCubic,
	kEaseInBack,
	kEaseOutBack,
	kEaseBounce0To0,
};

enum
{
	kTwitchFlags_Chain			= 0x100,
	kTwitchFlags_KillPuppet		= 0x200,
};

typedef struct
{
	Byte fxClass;
	Byte easing;
	float duration;
	float amplitude;
	float period;
	float phase;
	float delay;
	float seed;
} Twitch;

void InitTwitchSystem(void);
Twitch* MakeTwitch(ObjNode* puppet, int presetAndFlags);

ObjNode* MakeScrollingBackgroundPattern(void);
