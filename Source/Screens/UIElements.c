#include "game.h"

#define ARROW_TWITCH_DISTANCE		16.0f
#define ARROW_TWITCH_SPEED			12.0f

#define ArrowInitialized	Flag[0]
#define ArrowHomeX			SpecialF[0]
#define ArrowHomeY			SpecialF[1]

#define	PADLOCK_WIGGLE_DURATION		0.55f
#define	PADLOCK_WIGGLE_AMPLITUDE	8.0f
#define	PADLOCK_WIGGLE_SPEED		25.0f

#define PadlockInitialized	Flag[0]
#define PadlockHomeX		SpecialF[0]
#define PadlockHomeY		SpecialF[1]
#define PadlockWiggleTimer	SpecialF[2]
#define PadlockWiggleSign	SpecialF[3]

#pragma mark -

static void PrimeUIArrow(ObjNode* theNode)
{
	if (!theNode->ArrowInitialized)
	{
		// Not initialized: save home position
		theNode->ArrowHomeX = theNode->Coord.x;
		theNode->ArrowHomeY = theNode->Coord.y;
		theNode->ArrowInitialized = true;
	}
}

void MoveUIArrow(ObjNode* theNode)
{
	PrimeUIArrow(theNode);

	float dx = theNode->Coord.x - theNode->ArrowHomeX;
	float dy = theNode->Coord.y - theNode->ArrowHomeY;

	if (fabsf(dx) < 0.2f) dx = 0;	// if close enough, kill movement
	if (fabsf(dy) < 0.2f) dy = 0;

	if (dx != 0 || dy != 0)
	{
		theNode->Coord.x -= ARROW_TWITCH_SPEED * gFramesPerSecondFrac * dx;
		theNode->Coord.y -= ARROW_TWITCH_SPEED * gFramesPerSecondFrac * dy;
	}
	else
	{
		theNode->Coord.x = theNode->ArrowHomeX;	// pin to home position
		theNode->Coord.y = theNode->ArrowHomeY;
	}
	
	UpdateObjectTransforms(theNode);
}

void TwitchUIArrow(ObjNode* theNode, float x, float y)
{
	PrimeUIArrow(theNode);

	theNode->Coord.x = theNode->ArrowHomeX + ARROW_TWITCH_DISTANCE * x;
	theNode->Coord.y = theNode->ArrowHomeY + ARROW_TWITCH_DISTANCE * y;
}

#pragma mark -

static void PrimeUIPadlock(ObjNode* theNode)
{
	if (!theNode->PadlockInitialized)
	{
		// Not initialized: save home position
		theNode->PadlockHomeX = theNode->Coord.x;
		theNode->PadlockHomeY = theNode->Coord.y;
		theNode->PadlockWiggleSign = 1;
		theNode->PadlockWiggleTimer = 0;
		theNode->PadlockInitialized = true;
	}
}

void MoveUIPadlock(ObjNode* theNode)
{
	PrimeUIPadlock(theNode);

	if (theNode->PadlockWiggleTimer <= 0)
	{
		// Wiggle time elapsed, pin to home position
		theNode->Coord.x = theNode->PadlockHomeX;
		theNode->Coord.y = theNode->PadlockHomeY;
		UpdateObjectTransforms(theNode);
		return;
	}

	theNode->PadlockWiggleTimer -= gFramesPerSecondFrac;

	float dampening = theNode->PadlockWiggleTimer * (1.0f / PADLOCK_WIGGLE_DURATION);

	float x = sinf((theNode->PadlockWiggleTimer - PADLOCK_WIGGLE_DURATION) * PADLOCK_WIGGLE_SPEED);
	x *= dampening;
	x *= PADLOCK_WIGGLE_AMPLITUDE;
	x *= theNode->PadlockWiggleSign;

	theNode->Coord.x = x;

	UpdateObjectTransforms(theNode);
}

void WiggleUIPadlock(ObjNode* theNode)
{
	PrimeUIPadlock(theNode);
	theNode->PadlockWiggleTimer = PADLOCK_WIGGLE_DURATION;
	theNode->PadlockWiggleSign = -theNode->PadlockWiggleSign;	// alternate sign everytime we wiggle anew
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

	float dx = theNode->SpecialF[0] -= gFramesPerSecondFrac * 0.05f;
	float dy = theNode->SpecialF[1] -= gFramesPerSecondFrac * 0.05f;

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
