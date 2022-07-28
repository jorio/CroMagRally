/****************************/
/* NET PAUSE SCREEN         */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include "game.h"

static ObjNode* gNetPauseText = NULL;

static void MovePauseText(ObjNode* theNode)
{
	theNode->SpecialF[0] += gFramesPerSecondFrac;
	theNode->ColorFilter.a = 0.85f + sinf(3.0f * theNode->SpecialF[0]) * 0.15f;
}

void SetupNetPauseScreen(void)
{
	if (gNetPauseText)
	{
		return;
	}

	gHideInfobar = true;

	NewObjectDefinitionType netPauseDef =
	{
		.scale = 0.7f,
		.slot = MENU_SLOT,
		.flags = STATUS_BIT_MOVEINPAUSE,
		.moveCall = MovePauseText
	};

	char text[128];
	snprintf(text, sizeof(text), "NET PAUSE");

	gNetPauseText = TextMesh_New(Localize(STR_NET_PAUSE), 0, &netPauseDef);

	MakeTwitch(gNetPauseText, kTwitchPreset_NetPauseAppear);
}

void RemoveNetPauseScreen(void)
{
	if (!gNetPauseText)
	{
		return;
	}

	gHideInfobar = false;

	MakeTwitch(gNetPauseText, kTwitchPreset_NetPauseVanish | kTwitchFlags_KillPuppet);

	// The twitch will delete the puppet for us
	gNetPauseText = NULL;
}
