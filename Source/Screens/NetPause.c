/****************************/
/* NET PAUSE SCREEN         */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include "game.h"

static ObjNode* gNetPauseText = NULL;

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
	};

	char text[128];
	snprintf(text, sizeof(text), "NET PAUSE");

	gNetPauseText = TextMesh_New(text, 0, &netPauseDef);
}

void RemoveNetPauseScreen(void)
{
	if (!gNetPauseText)
	{
		return;
	}

	gHideInfobar = false;

	DeleteObject(gNetPauseText);
	gNetPauseText = NULL;
}
