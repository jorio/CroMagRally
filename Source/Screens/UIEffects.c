// UI EFFECTS.C
// (c)2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

#include "game.h"

static const TwitchDef kTwitchPresets[kTwitchPresetCOUNT] =
{
	[kTwitchScaleHard] =
	{
		.fxClass = kTwitchClassScale,
		.duration = .20f,
		.amplitude = 1.1f,
		.easing = kEaseOutCubic
	},

	[kTwitchScaleSoft] =
	{
		.fxClass = kTwitchClassScale,
		.duration = .14f,
		.amplitude = 1.1f,
		.easing = kEaseOutQuad
	},

	[kTwitchDisplaceLTR] =
	{
		.fxClass = kTwitchClassDisplaceX,
		.duration = .20f,
		.amplitude = -32,
		.easing = kEaseOutQuad,
	},

	[kTwitchDisplaceRTL] =
	{
		.fxClass = kTwitchClassDisplaceX,
		.duration = .20f,
		.amplitude = +32,
		.easing = kEaseOutQuad,
	},

	[kTwitchLightDisplaceLTR] =
	{
		.fxClass=kTwitchClassDisplaceX,
		.duration = .10f,
		.amplitude = -8,
		.easing = kEaseOutQuad,
	},

	[kTwitchLightDisplaceRTL] =
	{
		.fxClass = kTwitchClassDisplaceX,
		.duration = .10f,
		.amplitude = +8,
		.easing = kEaseOutQuad,
	},

	[kTwitchLightDisplaceDTU] =
	{
		.fxClass = kTwitchClassDisplaceY,
		.duration = .20f,
		.amplitude = +8,
		.easing = kEaseOutQuad,
	},

	[kTwitchLightDisplaceUTD] =
	{
		.fxClass = kTwitchClassDisplaceY,
		.duration = .20f,
		.amplitude = -8,
		.easing = kEaseOutQuad,
	},

	[kTwitchYBounce] =
	{
		.fxClass = kTwitchClassDisplaceY,
		.duration = .20f,
		.amplitude = -16	,
		.easing = kEaseBounce0To0,
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

	[kTwitchSubtleWiggle] =
	{
		.fxClass = kTwitchClassWiggleX,
		.duration = .25f,
		.amplitude = 12,
		.period = PI * 2,
		.easing = kEaseLerp,
	},

	[kTwitchScaleUp] =
	{
		.fxClass = kTwitchClassScale,
		.duration = .20f,
		.amplitude = .8f,
		.easing = kEaseInQuad,
	},

	[kTwitchArrowheadSpin] =
	{
		.fxClass = kTwitchClassSpinX,
		.duration = 1.5f,
		.amplitude = 1.0f,
		.period = PI * 8,
		.easing = kEaseOutQuad,
	},

	[kTwitchCoinSpin] =
	{
		.fxClass = kTwitchClassSpinX,
		.duration = 0.35f,
		.amplitude = 1.0f,
		.period = PI,
		.phase = -PI,
		.easing = kEaseOutQuad,
	},

	[kTwitchSweepFromLeft] =
	{
		.fxClass = kTwitchClassDisplaceX,
		.duration = 0.25f,
		.amplitude = -64.0f,
		.easing = kEaseOutBack, //kEaseOutQuad,
	},

	[kTwitchSweepFromTop] =
	{
		.fxClass = kTwitchClassDisplaceY,
		.duration = 0.25f,
		.amplitude = -64.0f,
		.easing = kEaseOutBack, //kEaseOutQuad,
	},

	[kTwitchScaleVanish] =
	{
		.fxClass = kTwitchClassScaleVanish,
		.duration = 0.25f,
		.amplitude = 1,
		.easing = kEaseInBack,
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
		case kEaseLerp:
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
		case kTwitchClassScale:
			puppet->Scale.x = realS.x * Lerp(def->amplitude, 1, p);
			puppet->Scale.y = realS.y * Lerp(def->amplitude, 1, p);
			break;

		case kTwitchClassScaleVanish:
			puppet->Scale.x = realS.x * Lerp(def->amplitude, 0, p);
			puppet->Scale.y = realS.y * Lerp(def->amplitude, 0, p);
			break;

		case kTwitchClassDisplaceX:
			puppet->Coord.x += realS.x * Lerp(def->amplitude, 0, p);
			break;

		case kTwitchClassDisplaceY:
			puppet->Coord.y += realS.y * Lerp(def->amplitude, 0, p);
			break;

		case kTwitchClassWiggleX:
		{
			float dampen = 1 - p;
			puppet->Coord.x += realS.x * def->amplitude * dampen * sinf(dampen * def->period + def->phase);
			break;
		}

		case kTwitchClassSpinX:
		{
			float wave = cosf(p * def->period + def->phase);

			if (fabsf(wave) < 0.2f)
				wave = 0.2f * (wave < 0? -1: 1);

			puppet->Scale.x = realS.x * def->amplitude * wave;

			break;
		}
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
		.def = kTwitchPresets[preset],
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
