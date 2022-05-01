#pragma once

#include "localization.h"

#define MAX_MENU_CYCLER_CHOICES		12

typedef enum
{
	kMenuItem_END_SENTINEL,
	kMenuItem_Pick,
	kMenuItem_Title,
	kMenuItem_Subtitle,
	kMenuItem_Label,
	kMenuItem_Spacer,
	kMenuItem_Cycler,
	kMenuItem_CMRCycler,
	kMenuItem_FloatRange,
	kMenuItem_KeyBinding,
	kMenuItem_PadBinding,
	kMenuItem_MouseBinding,
	kMenuItem_NUM_ITEM_TYPES
} MenuItemType;

enum
{
	kGotoMenu_ExitMenuTree = 0,
	kGotoMenu_GoBack = -1,
	kGotoMenu_NoOp = -2,
};

typedef struct
{
	Byte*			valuePtr;
	bool			callbackSetsValue;
	struct
	{
		LocStrID	text;
		uint8_t		value;
	} choices[MAX_MENU_CYCLER_CHOICES];
} MenuCyclerData;

typedef struct
{
	float*			valuePtr;
	float			rangeMin;
	float			rangeMax;
} MenuFloatRangeData;

typedef struct MenuItem
{
	MenuItemType			type;

	LocStrID				text;
	const char*				rawText;	// takes precedence over localizable text if non-NULL
	const char*				(*generateText)(void);

	void					(*callback)(const struct MenuItem*);
	bool					(*enableIf)(const struct MenuItem*);

	int						id;			// value returned by StartMenu if exiting menu
	int						gotoMenu;	// 0 exits menu tree

	union
	{
		int 				inputNeed;
		MenuCyclerData		cycler;
		MenuFloatRangeData	floatRange;
	};
} MenuItem;

typedef struct MenuStyle
{
	bool			darkenPane;
	float			fadeInSpeed;
	bool			asyncFadeOut;
	bool			fadeOutSceneOnExit;
	bool			centeredText;
	OGLColorRGBA	titleColor;
	OGLColorRGBA	highlightColor;
	OGLColorRGBA	inactiveColor;
	OGLColorRGBA	inactiveColor2;
	float			standardScale;
	float			rowHeight;
	float			uniformXExtent;
	bool			playMenuChangeSounds;
	float			darkenPaneScaleY;
	float			darkenPaneOpacity;
	bool			startButtonExits;
	bool			isInteractive;
	bool			canBackOutOfRootMenu;
	short			textSlot;
	float			yOffset;
} MenuStyle;

extern const MenuStyle kDefaultMenuStyle;

int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*updateRoutine)(void),
		void (*backgroundDrawRoutine)(OGLSetupOutputType *));

int StartMenuTree(
		const MenuItem** menus,
		const MenuStyle* style,
		void (*updateRoutine)(void),
		void (*backgroundDrawRoutine)(OGLSetupOutputType *));

void LayoutCurrentMenuAgain(void);
const MenuItem* GetCurrentMenu(void);
float GetMenuIdleTime(void);
void KillMenu(int returnCode);
