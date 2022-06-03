#pragma once

#define MAX_TWITCHES 256

enum
{
	kTwitchPreset_NULL = 0,
	kTwitchPreset_MenuSelect,
	kTwitchPreset_MenuDeselect,
	kTwitchPreset_MenuWiggle,
	kTwitchPreset_MenuFadeIn,
	kTwitchPreset_MenuFadeOut,
	kTwitchPreset_MenuSweep,
	kTwitchPreset_MenuDarkenPaneResize,
	kTwitchPreset_MenuDarkenPaneVanish,
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
	kTwitchPreset_GiantArrowheadSpin,
	kTwitchPreset_WeaponFlip,
	kTwitchPreset_NewBuff,
	kTwitchPreset_NewWeapon,
	kTwitchPreset_POWExpired,
	kTwitchPreset_LapMessageAppear,
	kTwitchPreset_LapMessageFadeOut,
	kTwitchPreset_LapDiffFadeOut,
	kTwitchPreset_TrackNameFadeOut,
	kTwitchPreset_FinalPlaceReveal,
	kTwitchPreset_YourTimeReveal,
	kTwitchPreset_NewRecordReveal,
	kTwitchPreset_ScoreboardFadeIn,
	kTwitchPreset_PressKeyPrompt,
	kTwitchPresetCOUNT,

	kTwitchClass_Heartbeat = kTwitchPresetCOUNT,		// deliberately starting above last preset # so presets and classes cannot be mixed up
	kTwitchClass_Shrink,
	kTwitchClass_Spin,
	kTwitchClass_Displace,
	kTwitchClass_Wiggle,
	kTwitchClass_AlphaFadeIn,
	kTwitchClass_AlphaFadeOut,
	kTwitchClass_MultAlphaFadeIn,
	kTwitchClass_MultAlphaFadeOut,

	kEaseLinear,
	kEaseInQuad,
	kEaseOutQuad,
	kEaseOutCubic,
	kEaseInExpo,
	kEaseOutExpo,
	kEaseInBack,
	kEaseOutBack,
	kEaseBounce0To0,
};

enum
{
	kTwitchFlags_Chain				= 0x100,
	kTwitchFlags_KillPuppet			= 0x200,
	kTwitchFlags_HideDuringDelay	= 0x400,
};

typedef struct
{
	Byte fxClass;
	Byte easing;
	bool x;
	bool y;
	bool z;
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
