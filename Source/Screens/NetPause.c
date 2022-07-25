/****************************/
/* NET PAUSE SCREEN         */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include "game.h"

static bool gNetPauseActive = false;
static ObjNode* gNetPauseText = NULL;


static void SetupNetPauseScreen(void)
{
	if (gNetPauseActive)
	{
		return;
	}

	gNetPauseActive = true;
	gGamePaused = true;
	gHideInfobar = true;

	NewObjectDefinitionType netPauseDef =
	{
		.scale = 0.7f,
		.slot = MENU_SLOT,
	};

	char text[128];
	snprintf(text, sizeof(text), "GAME PAUSED BY\n%s", gNetPausedBy);

	gNetPauseText = TextMesh_New(text, 0, &netPauseDef);
}

void DoNetPauseScreenTick(void)
{
	if (!gNetPauseActive)
	{
		SetupNetPauseScreen();
	}

	DoSDLMaintenance();
	MoveObjects();
	DoPlayerTerrainUpdate();
	CalcFramesPerSecond();
	OGL_DrawScene(DrawTerrain);
}

void EndNetPauseScreen(void)
{
	if (!gNetPauseActive)
	{
		return;
	}

	gNetPauseActive = false;
	gGamePaused = false;
	gHideInfobar = false;

	DeleteObject(gNetPauseText);
}
