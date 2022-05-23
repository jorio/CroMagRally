// UI EFFECTS.C
// (c)2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

#include "game.h"
#include "uieffects.h"

static const TwitchDef kTwitchPresets[kTwitchPresetCOUNT] =
{
	[kTwitchScaleIn] =
	{
		.fxClass = kTwitchClassScale,
		.duration = .20f,
		.amplitude = 1.1f,
		.easing = kEaseOutCubic
	},

	[kTwitchScaleOut] =
	{
		.fxClass = kTwitchClassScale,
		.duration = .14f,
		.amplitude = 1.1f,
		.easing = kEaseOutQuad
	},

	[kTwitchDisplaceLeft] =
	{
		.fxClass = kTwitchClassDisplaceX,
		.duration = .20f,
		.amplitude = -32,
		.easing = kEaseOutQuad,
	},

	[kTwitchDisplaceRight] =
	{
		.fxClass = kTwitchClassDisplaceX,
		.duration = .20f,
		.amplitude = +32,
		.easing = kEaseOutQuad,
	},

	[kTwitchLightDisplaceLeft] =
	{
		.fxClass=kTwitchClassDisplaceX,
		.duration = .10f,
		.amplitude = -8,
		.easing = kEaseOutQuad,
	},

	[kTwitchLightDisplaceRight] =
	{
		.fxClass = kTwitchClassDisplaceX,
		.duration = .10f,
		.amplitude = +8,
		.easing = kEaseOutQuad,
	},

	[kTwitchQuickWiggle] =
	{
		.fxClass = kTwitchClassWiggleX,
		.duration = .35f,
		.amplitude = 25,
		.period = PI * 3,
		.easing = kEaseLerp,
	},

	[kTwitchBigWiggle] =
	{
		.fxClass = kTwitchClassWiggleX,
		.duration = .45f,
		.amplitude = 25,
		.period = PI * 4,
		.easing = kEaseLerp,
	},
};

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
		case kEaseInQuad:		return p*p;
		case kEaseOutQuad:		return 1 - (1 - p) * (1 - p);
		case kEaseOutCubic:		return 1 - powf(1 - p, 3);

		case kEaseLerp:
		default:
			return p;
	}
}

static void MoveUIEffectDriver(ObjNode* driver)
{
	TwitchDriverData* effect = GetSpecialData(driver, TwitchDriverData);
	GAME_ASSERT(effect->cookie == 'UIFX');

	const TwitchDef* def = &effect->def;
	ObjNode* puppet = effect->puppet;

	if (IsObjectBeingDeleted(effect->puppet))
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

	float p = effect->timer / duration;

	if (p > 1)	// we'll nuke the driver at the end of this function
	{
		p = 1;
	}
	else
	{
		p = Ease(p, def->easing);
	}

	switch (effect->def.fxClass)
	{
		case kTwitchClassScale:
			puppet->Scale.x = realS.x * Lerp(def->amplitude, 1, p);
			puppet->Scale.y = realS.y * Lerp(def->amplitude, 1, p);
			break;

		case kTwitchClassDisplaceX:
			puppet->Coord.x += realS.x * Lerp(def->amplitude, 0, p);
			break;

		case kTwitchClassWiggleX:
		{
			float dampen = 1 - p;
			puppet->Coord.x += realS.x * def->amplitude * dampen * sinf(dampen * def->period);
			break;
		}
	}

	UpdateObjectTransforms(puppet);

	puppet->Coord = realC;
	puppet->Scale = realS;
	puppet->Rot = realR;

	effect->timer += gFramesPerSecondFrac;

	if (p >= 1)
	{
		DeleteObject(driver);
	}
}

ObjNode* MakeTwitch(ObjNode* puppet, int preset)
{
	if (puppet == NULL)
	{
		return NULL;
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
		.def = kTwitchPresets[preset],
		.timer = 0,
	};

	return driver;
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
	float sx = s * gCurrentAspectRatio;
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
