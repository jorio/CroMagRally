// UI EFFECTS.C
// (c)2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

#include <stdlib.h>
#include "game.h"

#define MAX_TWITCH_CHAIN_LENGTH 4
#define REGISTER_ENUM_NAME(x) [(x)] = (#x)

typedef struct
{
	ObjNode*			puppet;
	Twitch*				fxChain[MAX_TWITCH_CHAIN_LENGTH];
	float				timers[MAX_TWITCH_CHAIN_LENGTH];
	uint32_t			cookie;
	uint8_t				chainLength;
	bool				deletePuppetOnCompletion;
	bool				hideDuringDelay;
} TwitchDriverData;
CheckSpecialDataStruct(TwitchDriverData);

int						gFreeTwitches = MAX_TWITCHES;
static bool				gTwitchPtrPoolInitialized = false;
static Twitch*			gTwitchPtrPool[MAX_TWITCHES];
static Twitch			gTwitchBackingMemory[MAX_TWITCHES];

static Twitch			gTwitchPresets[kTwitchPresetCOUNT];
static bool				gTwitchPresetsLoaded = false;

static const char* kTwitchEnumNames[] =
{
	REGISTER_ENUM_NAME(kTwitchPreset_MenuSelect),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuDeselect),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuWiggle),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuFadeIn),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuFadeOut),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuDarkenPaneResize),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuDarkenPaneVanish),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuSweep),
	REGISTER_ENUM_NAME(kTwitchPreset_PadlockWiggle),
	REGISTER_ENUM_NAME(kTwitchPreset_DisplaceLTR),
	REGISTER_ENUM_NAME(kTwitchPreset_DisplaceRTL),
	REGISTER_ENUM_NAME(kTwitchPreset_SlideshowLTR),
	REGISTER_ENUM_NAME(kTwitchPreset_SlideshowRTL),
	REGISTER_ENUM_NAME(kTwitchPreset_ItemGain),
	REGISTER_ENUM_NAME(kTwitchPreset_ItemLoss),
	REGISTER_ENUM_NAME(kTwitchPreset_RankGain),
	REGISTER_ENUM_NAME(kTwitchPreset_RankLoss),
	REGISTER_ENUM_NAME(kTwitchPreset_ArrowheadSpin),
	REGISTER_ENUM_NAME(kTwitchPreset_GiantArrowheadSpin),
	REGISTER_ENUM_NAME(kTwitchPreset_WeaponFlip),
	REGISTER_ENUM_NAME(kTwitchPreset_NewBuff),
	REGISTER_ENUM_NAME(kTwitchPreset_NewWeapon),
	REGISTER_ENUM_NAME(kTwitchPreset_POWExpired),
	REGISTER_ENUM_NAME(kTwitchPreset_LapMessageAppear),
	REGISTER_ENUM_NAME(kTwitchPreset_LapMessageFadeOut),
	REGISTER_ENUM_NAME(kTwitchPreset_LapDiffFadeOut),
	REGISTER_ENUM_NAME(kTwitchPreset_TrackNameFadeOut),
	REGISTER_ENUM_NAME(kTwitchPreset_FinalPlaceReveal),
	REGISTER_ENUM_NAME(kTwitchPreset_YourTimeReveal),
	REGISTER_ENUM_NAME(kTwitchPreset_NewRecordReveal),
	REGISTER_ENUM_NAME(kTwitchPreset_ScoreboardFadeIn),
	REGISTER_ENUM_NAME(kTwitchPreset_PressKeyPrompt),

	REGISTER_ENUM_NAME(kTwitchClass_Heartbeat),
	REGISTER_ENUM_NAME(kTwitchClass_Shrink),
	REGISTER_ENUM_NAME(kTwitchClass_Spin),
	REGISTER_ENUM_NAME(kTwitchClass_Displace),
	REGISTER_ENUM_NAME(kTwitchClass_Wiggle),
	REGISTER_ENUM_NAME(kTwitchClass_AlphaFadeIn),
	REGISTER_ENUM_NAME(kTwitchClass_AlphaFadeOut),
	REGISTER_ENUM_NAME(kTwitchClass_MultAlphaFadeIn),
	REGISTER_ENUM_NAME(kTwitchClass_MultAlphaFadeOut),

	REGISTER_ENUM_NAME(kEaseLinear),
	REGISTER_ENUM_NAME(kEaseInQuad),
	REGISTER_ENUM_NAME(kEaseOutQuad),
	REGISTER_ENUM_NAME(kEaseOutCubic),
	REGISTER_ENUM_NAME(kEaseInExpo),
	REGISTER_ENUM_NAME(kEaseOutExpo),
	REGISTER_ENUM_NAME(kEaseInBack),
	REGISTER_ENUM_NAME(kEaseOutBack),
	REGISTER_ENUM_NAME(kEaseBounce0To0),
};

#pragma mark - Twitch pool management

static void InitTwitchPool(void)
{
	for (int i = 0; i < MAX_TWITCHES; i++)
	{
		gTwitchPtrPool[i] = &gTwitchBackingMemory[i];
	}

	gFreeTwitches = MAX_TWITCHES;
	gTwitchPtrPoolInitialized = true;
}

#if _DEBUG
static bool IsPooledTwitchFree(Twitch* e)
{
	for (int i = 0; i < MAX_TWITCHES; i++)
	{
		if (gTwitchPtrPool[i] == e)
			return true;
	}
	return false;
}
#endif

static Twitch* AllocTwitch(void)
{
	if (gFreeTwitches <= 0)
		return NULL;

	// Grab free slot
	Twitch* e = gTwitchPtrPool[gFreeTwitches - 1];
	gTwitchPtrPool[gFreeTwitches - 1] = NULL;

	gFreeTwitches--;

	return e;
}

static void DisposeTwitch(Twitch* e)
{
	if (e == NULL)
		return;

#if _DEBUG
	static bool detectedDoubleFree = false;
	if (!detectedDoubleFree && IsPooledTwitchFree(e))
	{
		detectedDoubleFree = true;		// avoid endless double-free messages caused by final cleanup in DoFatalAlert
		GAME_ASSERT_MESSAGE(false, "double-freeing pooled ptr");
	}
#endif

	int i = gFreeTwitches;
	GAME_ASSERT(i < MAX_TWITCHES);

	gTwitchPtrPool[i] = e;
	gFreeTwitches++;
}

#pragma mark - Parse presets from CSV file

static int FindEnumString(const char* prefix, const char* column)
{
	if (column == NULL)
	{
		return -1;
	}

	size_t prefixLength = (prefix==NULL) ? 0 : strlen(prefix);

	const int numNames = sizeof(kTwitchEnumNames) / sizeof(kTwitchEnumNames[0]);

	for (int i = 0; i < numNames; i++)
	{
		const char* enumName = kTwitchEnumNames[i];

		if (NULL != enumName
			&& ((0 == prefixLength) || (0 == strncmp(enumName, prefix, prefixLength)))
			&& 0 == strcmp(enumName + prefixLength, column))
		{
			return i;
		}
	}

	return -1;
}

static void LoadTwitchPresets(void)
{
	enum
	{
		kCSVColumn_Preset,
		kCSVColumn_Class,
		kCSVColumn_AxisConstraint,
		kCSVColumn_Easing,
		kCSVColumn_Duration,
		kCSVColumn_Amplitude,
		kCSVColumn_Period,
		kCSVColumn_Phase,
		kCSVColumn_Delay,
	};

	memset(gTwitchPresets, 0, sizeof(gTwitchPresets));

	char* csv = LoadTextFile(":System:twitch.csv", NULL);

	bool eol = false;

	char* csvReader = csv;
	int column = 0;
	Twitch* preset = NULL;

	while (csvReader)
	{
		char* cell = CSVIterator(&csvReader, &eol);

		int n = -1;

		if (!cell)
			goto nextColumn;

		if (column != 0 && preset == NULL)
			goto nextColumn;

		switch (column)
		{
			case kCSVColumn_Preset:
				preset = NULL;
				n = FindEnumString("kTwitchPreset_", cell);
				if (n >= 0 && n < kTwitchPresetCOUNT)
				{
					preset = &gTwitchPresets[n];
				}
				else if (0 != strcmp("PRESET", cell))		// skip header row
				{
					DoFatalAlert("Unknown twitch preset name '%s'", cell);
				}
				break;

			case kCSVColumn_Class:
				n = FindEnumString("kTwitchClass_", cell);
				if (n < 0)
				{
					DoFatalAlert("Unknown effect class '%s'", cell);
				}
				preset->fxClass = n;
				break;

			case kCSVColumn_AxisConstraint:
			{
				for (; cell && *cell; cell++)
				{
					switch (*cell)
					{
						case 'x': case 'X': preset->x = true; break;
						case 'y': case 'Y': preset->y = true; break;
						case 'z': case 'Z': preset->z = true; break;
					}
				}

				if (!(preset->x || preset->y || preset->z))		// if the cell was empty, default to XY
				{
					preset->x = true;
					preset->y = true;
					preset->z = false;
				}
				break;
			}

			case kCSVColumn_Easing:
				n = FindEnumString("kEase", cell);
				if (n < 0)
				{
					DoFatalAlert("Unknown easing type '%s'", cell);
				}
				preset->easing = n;
				break;

			case kCSVColumn_Duration:
				preset->duration = atof(cell);
				break;

			case kCSVColumn_Amplitude:
				preset->amplitude = atof(cell);
				break;

			case kCSVColumn_Period:
				preset->period = PI * atof(cell);
				break;

			case kCSVColumn_Phase:
				preset->phase = PI * atof(cell);
				break;

			case kCSVColumn_Delay:
				preset->delay = atof(cell);
				break;

			default:
				// ignore any other columns
				break;
		}

nextColumn:
		if (eol)
			column = 0;
		else
			column++;
	}

	SafeDisposePtr(csv);

	gTwitchPresetsLoaded = true;
}

#pragma mark - Easing

static float Lerp(float a, float b, float p)
{
	return (1-p) * a + p*b;
}

static float Ease(float p, int easingType)
{
	switch (easingType)
	{
		case kEaseLinear:
		default:
			return p;

		case kEaseInQuad:
			return p*p;

		case kEaseOutQuad:
		{
			float q = 1-p;
			return 1 - q*q;
		}

		case kEaseOutCubic:
		{
			float q = 1-p;
			return 1 - q*q*q;
		}

		case kEaseInExpo:
			return p == 0 ? 0 : powf(2, 10*p - 10);

		case kEaseOutExpo:
			return p == 1 ? 1 : 1 - powf(2, -10*p);

		case kEaseInBack:
		{
			float k = 1.7f;
			return (k+1)*p*p*p - k*p*p;
		}

		case kEaseOutBack:
		{
			float q = 1-p;
			float k = 1.7f;
			return 1 - ((k+1)*q*q*q - k*q*q);
		}

		case kEaseBounce0To0:
		{
			float q = 2*p - 1;
			return q*q;
			//return 1 - sqrtf(1 - (1-2*p)*(1-2*p));
		}
	}
}

#pragma mark - Twitch runtime

static void MultScale(ObjNode* puppet, const Twitch* fx, float f)
{
	if (fx->x) puppet->Scale.x *= f;
	if (fx->y) puppet->Scale.y *= f;
	if (fx->z) puppet->Scale.z *= f;
}

static void Displace(ObjNode* puppet, const Twitch* fx, float f)
{
	if (fx->x) puppet->Coord.x += puppet->Scale.x * f;
	if (fx->y) puppet->Coord.y += puppet->Scale.y * f;
	if (fx->z) puppet->Coord.z += puppet->Scale.z * f;
}

static bool ApplyTwitch(ObjNode* puppet, const Twitch* fx, float timer)
{
	float duration = fx->duration;
	if (duration == 0)
	{
#if _DEBUG
		printf("Twitch effect is missing duration\n");
#endif
		duration = 1;
	}

	float p = (timer - fx->delay) / duration;
	bool done = false;

	if (p >= 1)			// twitch complete
	{
		p = 1;
		done = true;
	}
	else if (p < 0)	// delayed
	{
		p = 0;
	}

	p = Ease(p, fx->easing);

	float f;

	switch (fx->fxClass)
	{
		case kTwitchClass_Heartbeat:
			f = Lerp(fx->amplitude, 1, p);
			MultScale(puppet, fx, f);
			break;

		case kTwitchClass_Shrink:
			f = Lerp(fx->amplitude, EPS, p);
			MultScale(puppet, fx, f);
			break;

		case kTwitchClass_Displace:
			f = Lerp(fx->amplitude, 0, p);
			Displace(puppet, fx, f);
			break;

		case kTwitchClass_Wiggle:
			f = 1 - p;		// dampening
			f = fx->amplitude * f * sinf(f * fx->period + fx->phase);
			Displace(puppet, fx, f);
			break;

		case kTwitchClass_Spin:
			f = cosf(p * fx->period + fx->phase);

			if (fabsf(f) < 0.2f)
				f = 0.2f * (f < 0? -1: 1);

			f *= fx->amplitude;
			MultScale(puppet, fx, f);
			break;

		case kTwitchClass_MultAlphaFadeIn:
			puppet->ColorFilter.a *= Lerp(0, 1, p);
			break;

		case kTwitchClass_MultAlphaFadeOut:
			puppet->ColorFilter.a *= Lerp(1, 0, p);
			break;

		case kTwitchClass_AlphaFadeIn:
			puppet->ColorFilter.a = Lerp(0, fx->phase, p);
			break;

		case kTwitchClass_AlphaFadeOut:
			puppet->ColorFilter.a = Lerp(fx->phase, 0, p);
			break;

		default:
			printf("Unknown effect class %d\n", fx->fxClass);
			break;
	}

	// Don't rely on p>=1 for completion because some easing functions may overshoot on purpose!
	return done;
}

static void MoveTwitchDriver(ObjNode* driverNode)
{
	TwitchDriverData* driverData = GetSpecialData(driverNode, TwitchDriverData);
	GAME_ASSERT(driverData->cookie == 'UIFX');

	ObjNode* puppet = driverData->puppet;

	if (IsObjectBeingDeleted(driverData->puppet)			// puppet is being destroyed
		|| driverData->puppet->TwitchNode != driverNode)	// twitch driver was replaced
	{
		DeleteObject(driverNode);
		return;
	}

	// Backup home coord/scale/rotation
	OGLPoint3D realC = puppet->Coord;
	OGLVector3D realS = puppet->Scale;
	OGLVector3D realR = puppet->Rot;

	bool anyEffectActive = false;

	// Work through all effects in chain
	int fxNum = 0;
	while (fxNum < driverData->chainLength)
	{
		bool done = ApplyTwitch(puppet, driverData->fxChain[fxNum], driverData->timers[fxNum]);

		if (done)
		{
			// Remove this effect from the chain
			DisposeTwitch(driverData->fxChain[fxNum]);
			driverData->fxChain[fxNum] = NULL;

			driverData->chainLength--;
			int lastFx = driverData->chainLength;

			// Swap
			if (fxNum != lastFx)
			{
				driverData->timers[fxNum]	= driverData->timers[lastFx];
				driverData->fxChain[fxNum]	= driverData->fxChain[lastFx];
			}
		}
		else
		{
			driverData->timers[fxNum] += gFramesPerSecondFrac;	// tick effect timer
			anyEffectActive |= driverData->timers[fxNum] >= driverData->fxChain[fxNum]->delay;

			fxNum++;											// onto next effect in chain
		}
	}

	// Commit matrix
	UpdateObjectTransforms(puppet);

	// Restore home coord/scale/rotation
	puppet->Coord = realC;
	puppet->Scale = realS;
	puppet->Rot = realR;

	// If we're done, nuke driverNode
	if (driverData->chainLength == 0)
	{
		bool killPuppet = driverData->deletePuppetOnCompletion;

		puppet->TwitchNode = NULL;
		DeleteObject(driverNode);

		if (killPuppet)
		{
			DeleteObject(puppet);
		}
	}
	else
	{
		if (driverData->hideDuringDelay)
		{
			SetObjectVisible(puppet, anyEffectActive);
		}
	}
}

#pragma mark - Twitch construction/destruction

static void DisposeEffectChain(TwitchDriverData* driverData)
{
	for (int i = 0; i < driverData->chainLength; i++)
	{
		DisposeTwitch(driverData->fxChain[i]);
		driverData->fxChain[i] = NULL;
	}

	driverData->chainLength = 0;
}

static void DestroyTwitchDriver(ObjNode* driver)
{
	TwitchDriverData* driverData = GetSpecialData(driver, TwitchDriverData);

	if (driverData->puppet
		&& !IsObjectBeingDeleted(driverData->puppet)
		&& driverData->puppet->TwitchNode == driver)
	{
		driverData->puppet->TwitchNode = NULL;
	}
	driverData->puppet = NULL;

	DisposeEffectChain(driverData);
}

Twitch* MakeTwitch(ObjNode* puppet, int presetAndFlags)
{
	if (puppet == NULL)
	{
		return NULL;
	}

	GAME_ASSERT(gTwitchPresetsLoaded);
	GAME_ASSERT(gTwitchPtrPoolInitialized);

	ObjNode* driverNode = NULL;
	TwitchDriverData* driverData = NULL;

	if (puppet->TwitchNode)
	{
		driverNode = puppet->TwitchNode;
		driverData = GetSpecialData(driverNode, TwitchDriverData);

		GAME_ASSERT(puppet->TwitchNode == driverNode);
		GAME_ASSERT(driverData->cookie == 'UIFX');
		GAME_ASSERT(driverData->puppet == puppet);
		GAME_ASSERT(driverNode->Slot > puppet->Slot);

		if (presetAndFlags & kTwitchFlags_Chain)
		{
			if (driverData->chainLength >= MAX_TWITCH_CHAIN_LENGTH)
			{
				return NULL;
			}
		}
		else
		{
			// Interrupt ongoing effect chain
			driverData->deletePuppetOnCompletion = false;
			DisposeEffectChain(driverData);
		}
	}
	else
	{
		// Create a new twitch driver node
		NewObjectDefinitionType def =
		{
			.genre		= EVENT_GENRE,
			.slot		= puppet->Slot + 1,
			.scale		= 1,
			.moveCall	= MoveTwitchDriver,
			.flags		= puppet->StatusBits & STATUS_BIT_MOVEINPAUSE,
		};

		driverNode = MakeNewObject(&def);
		driverNode->Destructor = DestroyTwitchDriver;
		driverData = GetSpecialData(driverNode, TwitchDriverData);

		driverData->cookie = 'UIFX';
		driverData->puppet = puppet;
		puppet->TwitchNode = driverNode;
	}

	driverData->deletePuppetOnCompletion |= (presetAndFlags & kTwitchFlags_KillPuppet);
	driverData->hideDuringDelay |= (presetAndFlags & kTwitchFlags_HideDuringDelay);

	Twitch* twitch = AllocTwitch();

	if (!twitch)
		return NULL;

	int effectNum = driverData->chainLength;
	driverData->chainLength++;
	GAME_ASSERT(driverData->chainLength <= MAX_TWITCH_CHAIN_LENGTH);

	driverData->timers[effectNum] = 0;
	driverData->fxChain[effectNum] = twitch;

	uint8_t presetNum = presetAndFlags & 0xFF;
	GAME_ASSERT_MESSAGE(presetNum <= kTwitchPresetCOUNT, "Not a legal twitch preset");
	*twitch = gTwitchPresets[presetNum];

	// Save current alpha as "phase" for alpha fade effects
	switch (twitch->fxClass)
	{
	case kTwitchClass_AlphaFadeIn:
	case kTwitchClass_AlphaFadeOut:
		twitch->phase = puppet->ColorFilter.a;
		break;
	}

	return twitch;
}

#pragma mark - Init

void InitTwitchSystem(void)
{
	LoadTwitchPresets();
	InitTwitchPool();
}

#pragma mark - Scrolling background pattern

/************** SCROLLING BACKGROUND PATTERN *********************/

static void DestroyScrollingBackgroundPattern(ObjNode* theNode)
{
	if (theNode->SpecialPtr[0])
	{
		MO_DisposeObjectReference(theNode->SpecialPtr[0]);
		theNode->SpecialPtr[0] = NULL;
	}
}

static void DrawScrollingBackgroundPattern(ObjNode* theNode)
{
	float s = 4;
	float sx = s * gGameView->panes[gCurrentSplitScreenPane].aspectRatio;
	float sy = s;

	MOMaterialObject* patternMaterial = (MOMaterialObject*) theNode->SpecialPtr[0];

	float dx = theNode->SpecialF[0] -= gFramesPerSecondFrac * 0.08f;
	float dy = theNode->SpecialF[1] -= gFramesPerSecondFrac * 0.08f;

	OGL_PushState();
	MO_DrawMaterial(patternMaterial);
	glBegin(GL_QUADS);
	glTexCoord2f(sx + dx, sy + dy); glVertex3f( 1.0f,  1.0f, 0.0f);
	glTexCoord2f(     dx, sy + dy); glVertex3f(-1.0f,  1.0f, 0.0f);
	glTexCoord2f(     dx,      dy); glVertex3f(-1.0f, -1.0f, 0.0f);
	glTexCoord2f(sx + dx,      dy); glVertex3f( 1.0f, -1.0f, 0.0f);
	glEnd();
	OGL_PopState();
}

ObjNode* MakeScrollingBackgroundPattern(void)
{
			/* VIGNETTE EFFECT */

	ObjNode* vignette = MakeBackgroundPictureObject(":Images:Vignette.png");
	vignette->ColorFilter = (OGLColorRGBA) {0,0,0,1};

			/* PATTERN TEXTURE */

	MOMaterialObject* patternMaterial = MO_GetTextureFromFile(":Images:BoneCollage.png", GL_RGB);

			/* PATTERN OBJECT */

	NewObjectDefinitionType collageDef =
	{
		.scale		= 1,
		.slot		= vignette->Slot-1,
		.drawCall	= DrawScrollingBackgroundPattern,
		.genre		= CUSTOM_GENRE,
		.flags		= STATUS_BITS_FOR_2D & ~STATUS_BIT_NOTEXTUREWRAP,
		.projection	= kProjectionType2DNDC,
	};

	ObjNode* collageObj = MakeNewObject(&collageDef);

	collageObj->SpecialPtr[0] = (Ptr) patternMaterial;
	collageObj->Destructor = DestroyScrollingBackgroundPattern;

	return collageObj;
}
