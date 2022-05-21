// UI EFFECTS.C
// (c)2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

#include "game.h"
#include "uieffects.h"

static const float kEffectDurations[kTwitchCOUNT] =
{
	[kTwitchScaleIn]		= .14f,
	[kTwitchScaleOut]		= .14f,
	[kTwitchDisplaceLeft]	= .2f,
	[kTwitchDisplaceRight]	= .2f,
	[kTwitchLightDisplaceLeft]	= .1f,
	[kTwitchLightDisplaceRight]	= .1f,
	[kTwitchQuickWiggle]	= .35f,
	[kTwitchBigWiggle]		= .45f,
};

typedef struct
{
	ObjNode*	puppet;
	float		duration;
	float		timer;
	float		amplitude;
	float		seed;
	uint32_t	cookie;
} UifxData;
CheckSpecialDataStruct(UifxData);

float EaseInQuad(float p) {	return p * p; }
float EaseOutQuad(float p) { return 1 - (1 - p) * (1 - p); }
float EaseOutCubic(float p) { return 1 - powf(1 - p, 3); }

static float Lerp(float a, float b, float p)
{
	return (1-p) * a + p*b;
}

static void MoveUIEffectDriver(ObjNode* driver)
{
	UifxData* effect = GetSpecialData(driver, UifxData);
	GAME_ASSERT(effect->cookie == 'UIFX');

	Byte type = driver->Type;

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

	effect->timer += gFramesPerSecondFrac;

	float duration = kEffectDurations[type];
	if (duration == 0)
	{
#if _DEBUG
		printf("Twitch type %d is missing duration\n", type);
#endif
		duration = 1;
	}

	float p = effect->timer / duration;

	if (p > 1)	// we'll nuke the driver at the end of this function
	{
		p = 1;
	}

	switch (type)
	{
		case kTwitchScaleIn:
		{
			float overdrive = 1.1f;

			p = EaseOutCubic(p);
			puppet->Scale.x = realS.x * Lerp(overdrive, 1, p);
			puppet->Scale.y = realS.y * Lerp(overdrive, 1, p);

			break;
		}

		case kTwitchScaleOut:
		{
			float overdrive = 1.1f;

			p = EaseOutQuad(p);
			puppet->Scale.x = realS.x * Lerp(overdrive, 1, p);
			puppet->Scale.y = realS.y * Lerp(overdrive, 1, p);

			break;
		}

		case kTwitchDisplaceLeft:
		case kTwitchDisplaceRight:
		{
			float peakDisplacement = 32.0f;

			float way = type==kTwitchDisplaceLeft? -1.0f: 1.0f;

			p = EaseOutQuad(p);

			puppet->Coord.x += way * realS.x * Lerp(peakDisplacement, 0, p);

			break;
		}

		case kTwitchLightDisplaceLeft:
		case kTwitchLightDisplaceRight:
		{
			float peakDisplacement = 8.0f;

			float way = type == kTwitchLightDisplaceLeft ? -1.0f : 1.0f;

			p = EaseOutQuad(p);

			puppet->Coord.x += way * realS.x * Lerp(peakDisplacement, 0, p);

			break;
		}

		case kTwitchQuickWiggle:
		case kTwitchBigWiggle:
		{
			float period = PI * (type==kTwitchQuickWiggle ? 3.0f : 4.0f);
			float amp = 25 * realS.x;

			float invp = 1 - p;
			float dampen = invp;

			puppet->Coord.x += amp * dampen * sinf(invp * period);

			break;
		}
	}

	UpdateObjectTransforms(puppet);

	puppet->Coord = realC;
	puppet->Scale = realS;
	puppet->Rot = realR;

	if (p == 1)
	{
		DeleteObject(driver);
	}
}

ObjNode* MakeTwitch(ObjNode* puppet, int type)
{
	if (puppet == NULL)
	{
		return NULL;
	}

	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.type = type,
		.slot = puppet->Slot + 1,
		.scale = 1,
		.moveCall = MoveUIEffectDriver,
		.flags = STATUS_BIT_MOVEINPAUSE,
	};

	ObjNode* driver = MakeNewObject(&def);

	*GetSpecialData(driver, UifxData) = (UifxData)
	{
		.cookie = 'UIFX',
		.puppet = puppet,
		.duration = 0.14f,
		.amplitude = 10,
		.timer = 0,
		.seed = RandomFloat(),
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
