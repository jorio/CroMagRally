// UI EFFECTS.C
// (c)2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

#include <stdlib.h>
#include "game.h"

#define REGISTER_ENUM_NAME(x) [(x)] = (#x)

static const char* kEnumNames[] =
{
	REGISTER_ENUM_NAME(kTwitchPreset_MenuSelect),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuDeselect),
	REGISTER_ENUM_NAME(kTwitchPreset_MenuWiggle),
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
	REGISTER_ENUM_NAME(kTwitchPreset_WeaponFlip),
	REGISTER_ENUM_NAME(kTwitchPreset_NewBuff),
	REGISTER_ENUM_NAME(kTwitchPreset_NewWeapon),
	REGISTER_ENUM_NAME(kTwitchPreset_POWExpired),

	REGISTER_ENUM_NAME(kTwitchClass_Scale),
	REGISTER_ENUM_NAME(kTwitchClass_SpinX),
	REGISTER_ENUM_NAME(kTwitchClass_DisplaceX),
	REGISTER_ENUM_NAME(kTwitchClass_DisplaceY),
	REGISTER_ENUM_NAME(kTwitchClass_WiggleX),
	REGISTER_ENUM_NAME(kTwitchClass_Shrink),

	REGISTER_ENUM_NAME(kEaseLinear),
	REGISTER_ENUM_NAME(kEaseInQuad),
	REGISTER_ENUM_NAME(kEaseOutQuad),
	REGISTER_ENUM_NAME(kEaseOutCubic),
	REGISTER_ENUM_NAME(kEaseInBack),
	REGISTER_ENUM_NAME(kEaseOutBack),
	REGISTER_ENUM_NAME(kEaseBounce0To0),
};

static TwitchDef gTwitchPresets[kTwitchPresetCOUNT];
static bool gTwitchPresetsLoaded = false;

static int FindEnumString(const char* prefix, const char* column)
{
	if (column == NULL)
	{
		return -1;
	}

	size_t prefixLength = (prefix==NULL) ? 0 : strlen(prefix);

	const int numNames = sizeof(kEnumNames) / sizeof(kEnumNames[0]);

	for (int i = 0; i < numNames; i++)
	{
		const char* enumName = kEnumNames[i];

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
	char* csv = LoadTextFile(":system:twitch.csv", NULL);

	bool eol = false;

	char* csvReader = csv;
	int state = 0;
	TwitchDef* preset = NULL;

	while (csvReader)
	{
		char* column = CSVIterator(&csvReader, &eol);

		int n = -1;

		if (!column)
			goto nextColumn;

		if (state != 0 && preset == NULL)
			goto nextColumn;

		switch (state)
		{
			case 0:
				preset = NULL;
				n = FindEnumString("kTwitchPreset_", column);
				if (n >= 0 && n < kTwitchPresetCOUNT)
				{
					preset = &gTwitchPresets[n];
				}
				else if (0 != strcmp("PRESET", column))		// skip header row
				{
					DoFatalAlert("Unknown twitch preset name '%s'", column);
				}
				break;

			case 1:
				n = FindEnumString("kTwitchClass_", column);
				if (n < 0)
				{
					DoFatalAlert("Unknown effect class '%s'", column);
				}
				preset->fxClass = n;
				break;

			case 2:
				n = FindEnumString("kEase", column);
				if (n < 0)
				{
					DoFatalAlert("Unknown easing type '%s'", column);
				}
				preset->easing = n;
				break;

			case 3:
				preset->duration = atof(column);
				break;

			case 4:
				preset->amplitude = atof(column);
				break;

			case 5:
				preset->period = PI * atof(column);
				break;

			case 6:
				preset->phase = PI * atof(column);
				GAME_ASSERT(eol);
				break;
		}

nextColumn:
		if (eol)
			state = 0;
		else
			state++;
	}

	SafeDisposePtr(csv);

	gTwitchPresetsLoaded = true;
}

typedef struct
{
	TwitchDef	def;
	ObjNode*	puppet;
	float		timer;
	uint32_t	cookie;
} TwitchDriverData;
CheckSpecialDataStruct(TwitchDriverData);

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

static void MoveUIEffectDriver(ObjNode* driver)
{
	TwitchDriverData* effect = GetSpecialData(driver, TwitchDriverData);
	GAME_ASSERT(effect->cookie == 'UIFX');

	const TwitchDef* def = &effect->def;
	ObjNode* puppet = effect->puppet;

	if (IsObjectBeingDeleted(effect->puppet)		// puppet is being destroyed
		|| effect->puppet->TwitchNode != driver)	// twitch effect was replaced
	{
		DeleteObject(driver);
		return;
	}

	// Backup home coord/scale/rotation
	OGLPoint3D realC = puppet->Coord;
	OGLVector3D realS = puppet->Scale;
	OGLVector3D realR = puppet->Rot;

	float duration = def->duration;
	if (duration == 0)
	{
#if _DEBUG
		printf("Twitch is missing duration\n");
#endif
		duration = 1;
	}

	float p = (effect->timer - effect->def.delay) / duration;
	bool done = false;

	if (p >= 1)			// twitch complete -- we'll nuke the driver at the end of this function
	{
		p = 1;
		done = true;
	}
	else if (p <= 0)	// delayed
	{
		p = 0;
	}

	p = Ease(p, def->easing);

	switch (effect->def.fxClass)
	{
		case kTwitchClass_Scale:
			puppet->Scale.x = realS.x * Lerp(def->amplitude, 1, p);
			puppet->Scale.y = realS.y * Lerp(def->amplitude, 1, p);
			break;

		case kTwitchClass_Shrink:
			puppet->Scale.x = realS.x * Lerp(def->amplitude, 0, p);
			puppet->Scale.y = realS.y * Lerp(def->amplitude, 0, p);
			break;

		case kTwitchClass_DisplaceX:
			puppet->Coord.x += realS.x * Lerp(def->amplitude, 0, p);
			break;

		case kTwitchClass_DisplaceY:
			puppet->Coord.y += realS.y * Lerp(def->amplitude, 0, p);
			break;

		case kTwitchClass_WiggleX:
		{
			float dampen = 1 - p;
			puppet->Coord.x += realS.x * def->amplitude * dampen * sinf(dampen * def->period + def->phase);
			break;
		}

		case kTwitchClass_SpinX:
		{
			float wave = cosf(p * def->period + def->phase);

			if (fabsf(wave) < 0.2f)
				wave = 0.2f * (wave < 0? -1: 1);

			puppet->Scale.x = realS.x * def->amplitude * wave;

			break;
		}

		default:
			printf("Unknown effect class %d\n", effect->def.fxClass);
			break;
	}

	UpdateObjectTransforms(puppet);

	puppet->Coord = realC;
	puppet->Scale = realS;
	puppet->Rot = realR;

	effect->timer += gFramesPerSecondFrac;

	if (done)
	{
		DeleteObject(driver);
		puppet->TwitchNode = NULL;
	}
}

ObjNode* MakeTwitch(ObjNode* puppet, uint8_t preset)
{
	GAME_ASSERT_MESSAGE(preset <= kTwitchPresetCOUNT, "Not a legal twitch preset");

	if (puppet == NULL)
	{
		return NULL;
	}

	if (!gTwitchPresetsLoaded)
	{
		LoadTwitchPresets();
	}

	if (puppet->TwitchNode)
	{
		DeleteObject(puppet->TwitchNode);
		puppet->TwitchNode = NULL;
	}

	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.slot = puppet->Slot + 1,		// puppet's move call must set coord/scale BEFORE twitch's move call
		.scale = 1,
		.moveCall = MoveUIEffectDriver,
		.flags = STATUS_BIT_MOVEINPAUSE,
	};

	ObjNode* driver = MakeNewObject(&def);

	*GetSpecialData(driver, TwitchDriverData) = (TwitchDriverData)
	{
		.cookie = 'UIFX',
		.puppet = puppet,
		.def = gTwitchPresets[preset],
		.timer = 0,
	};

	puppet->TwitchNode = driver;

	return driver;
}

TwitchDef* GetTwitchParameters(ObjNode* driver)
{
	if (driver == NULL)
	{
		return NULL;
	}

	TwitchDriverData* driverData = GetSpecialData(driver, TwitchDriverData);
	GAME_ASSERT(driverData->cookie == 'UIFX');

	return &driverData->def;
}

#pragma mark -

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

	ObjNode* vignette = MakeBackgroundPictureObject(":images:Vignette.png");
	vignette->ColorFilter = (OGLColorRGBA) {0,0,0,1};

			/* PATTERN TEXTURE */

	MOMaterialObject* patternMaterial = MO_GetTextureFromFile(":images:BoneCollage.png", GL_RGB);

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
