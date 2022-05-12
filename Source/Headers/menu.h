#pragma once

#include "localization.h"

#define MAX_MENU_CYCLER_CHOICES		12

typedef enum
{
	kMISENTINEL,
	kMIPick,
	kMILabel,
	kMISpacer,
	kMICycler1,
	kMICycler2,
	kMIFloatRange,
	kMIKeyBinding,
	kMIPadBinding,
	kMIMouseBinding,
	kMI_COUNT
} MenuItemType;

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
	int32_t					id;			// value returned by StartMenu if exiting menu
	int32_t					next;		// next menu, or one of 'EXIT', 'BACK' or 0 (no-op)

	void					(*callback)(const struct MenuItem*);
	int						(*getLayoutFlags)(const struct MenuItem*);

	union
	{
		int 				inputNeed;
		MenuCyclerData		cycler;
		MenuFloatRangeData	floatRange;
	};

	float					customHeight;
} MenuItem;

enum	// Returned by getLayoutFlags
{
	kMILayoutFlagDisabled = 1 << 0,
	kMILayoutFlagHidden = 1 << 1,
};

typedef struct MenuStyle
{
	float			darkenPaneOpacity;
	float			fadeInSpeed;		// Menu will ignore input during 1.0/fadeInSpeed seconds after booting
	float			fadeOutSpeed;
	float			sweepInSpeed;
	float			standardScale;
	float			rowHeight;
	float			uniformXExtent;
	short			textSlot;
	float			yOffset;

	OGLColorRGBA	labelColor;
	OGLColorRGBA	idleColor;
	OGLColorRGBA	highlightColor;
	struct
	{
		bool			asyncFadeOut : 1;
		bool			fadeOutSceneOnExit : 1;
		bool			startButtonExits : 1;
		bool			isInteractive : 1;
		bool			canBackOutOfRootMenu : 1;
	};
} MenuStyle;

extern const MenuStyle kDefaultMenuStyle;

void RegisterMenu(const MenuItem* menus);

int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*updateRoutine)(void),
		void (*backgroundDrawRoutine)(void));

void LayoutCurrentMenuAgain(void);
int GetCurrentMenu(void);
ObjNode* GetCurrentMenuItemObject(void);
float GetMenuIdleTime(void);
void KillMenu(int returnCode);
