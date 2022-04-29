#include "game.h"

#define ARROW_TWITCH_DISTANCE		16.0f
#define ARROW_TWITCH_SPEED			12.0f

#define	PADLOCK_WIGGLE_DURATION		0.55f
#define	PADLOCK_WIGGLE_AMPLITUDE	8.0f
#define	PADLOCK_WIGGLE_SPEED		25.0f

void MoveUIArrow(ObjNode* theNode)
{
	if (!theNode->Flag[0])
	{
		// Not initialized: save home position
		theNode->SpecialF[0] = theNode->Coord.x;
		theNode->SpecialF[1] = theNode->Coord.y;
		theNode->Flag[0] = true;
	}

	float homeX = theNode->SpecialF[0];
	float homeY = theNode->SpecialF[1];

	float dx = theNode->Coord.x - homeX;
	float dy = theNode->Coord.y - homeY;

	if (fabsf(dx) < 0.2f) dx = 0;	// if close enough, kill movement
	if (fabsf(dy) < 0.2f) dy = 0;

	if (dx != 0 || dy != 0)
	{
		theNode->Coord.x -= ARROW_TWITCH_SPEED * gFramesPerSecondFrac * dx;
		theNode->Coord.y -= ARROW_TWITCH_SPEED * gFramesPerSecondFrac * dy;
	}
	else
	{
		theNode->Coord.x = homeX;	// pin to home position
		theNode->Coord.y = homeY;
	}
	
	UpdateObjectTransforms(theNode);
}

void TwitchUIArrow(ObjNode* theNode, float x, float y)
{
	float homeX = theNode->SpecialF[0];
	float homeY = theNode->SpecialF[1];
	theNode->Coord.x = homeX + ARROW_TWITCH_DISTANCE * x;
	theNode->Coord.y = homeY + ARROW_TWITCH_DISTANCE * y;
}

void MoveUIPadlock(ObjNode* theNode)
{
	if (!theNode->Flag[0])
	{
		// Not initialized: save home position
		theNode->SpecialF[0] = theNode->Coord.x;
		theNode->SpecialF[1] = theNode->Coord.y;
		theNode->Flag[0] = true;
	}

	if (theNode->SpecialF[2] <= 0)
	{
		// Wiggle time elapsed, pin to home position
		theNode->Coord.x = theNode->SpecialF[0];
		theNode->Coord.y = theNode->SpecialF[1];
		UpdateObjectTransforms(theNode);
		return;
	}

	theNode->SpecialF[2] -= gFramesPerSecondFrac;

	float t = theNode->SpecialF[2];

	float dampening = t * (1.0f / PADLOCK_WIGGLE_DURATION);

	float x = sinf((t - PADLOCK_WIGGLE_DURATION) * PADLOCK_WIGGLE_SPEED);
	x *= dampening;
	x *= PADLOCK_WIGGLE_AMPLITUDE;

	theNode->Coord.x = x;

	UpdateObjectTransforms(theNode);
}

void WiggleUIPadlock(ObjNode* theNode)
{
	theNode->SpecialF[2] = PADLOCK_WIGGLE_DURATION;
}
