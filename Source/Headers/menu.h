#pragma once

#include "localization.h"

#define MAX_MENU_CYCLER_CHOICES		8

typedef enum
{
	kMenuItem_END_SENTINEL,
	kMenuItem_Pick,
	kMenuItem_Title,
	kMenuItem_Subtitle,
	kMenuItem_Label,
//	kMenuItem_Action,
//	kMenuItem_Submenu,
	kMenuItem_Spacer,
	kMenuItem_Cycler,
	kMenuItem_CMRCycler,
	kMenuItem_KeyBinding,
	kMenuItem_PadBinding,
	kMenuItem_MouseBinding,
	kMenuItem_NUM_ITEM_TYPES
} MenuItemType;

typedef struct MenuItem
{
	MenuItemType			type;

	LocStrID				text;
	const char*				rawText;
	const char*				(*generateText)(void);

	bool					(*enableIf)(const struct MenuItem*);

	int						gotoMenu;  // 0 exits menu tree
	void					(*callback)(const struct MenuItem*);

	int						id;

	struct
	{
		Byte*			valuePtr;
		bool			callbackSetsValue;
		struct
		{
			LocStrID	text;
			uint8_t		value;
		} choices[MAX_MENU_CYCLER_CHOICES];
	} cycler;

	int 				kb;  // keybinding
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
} MenuStyle;

extern const MenuStyle kDefaultMenuStyle;

int StartMenuTree(
		const MenuItem** menus,
		const MenuStyle* style,
		void (*updateRoutine)(void),
		void (*backgroundDrawRoutine)(OGLSetupOutputType *));
void LayoutCurrentMenuAgain(void);
const MenuItem* GetCurrentMenu(void);
float GetMenuIdleTime(void);
void KillMenu(int returnCode);
void MenuCallback_Back(const MenuItem* mi);
