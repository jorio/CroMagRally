#pragma once

#include "localization.h"

#define MAX_MENU_CYCLER_CHOICES		12

typedef enum
{
	kMISENTINEL,
	kMIPick,
	kMITitle,
	kMISubtitle,
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
	const char*				rawText;	// takes precedence over localizable text if non-NULL
	const char*				(*generateText)(void);

	void					(*callback)(const struct MenuItem*);
	bool					(*enableIf)(const struct MenuItem*);
	bool					(*displayIf)(const struct MenuItem*);

	int						id;			// value returned by StartMenu if exiting menu
	int						next;		// next menu, or one of 'EXIT', 'BACK' or 0 (no-op)

	union
	{
		int 				inputNeed;
		MenuCyclerData		cycler;
		MenuFloatRangeData	floatRange;
	};

	float					customHeight;
} MenuItem;

typedef struct MenuStyle
{
	float			darkenPaneOpacity;
	float			fadeInSpeed;		// Menu will ignore input during 1.0/fadeInSpeed seconds after booting
	float			fadeOutSpeed;
	float			sweepInSpeed;
	OGLColorRGBA	titleColor;
	OGLColorRGBA	highlightColor;
	OGLColorRGBA	inactiveColor;
	OGLColorRGBA	inactiveColor2;
	float			standardScale;
	float			rowHeight;
	float			uniformXExtent;
	short			textSlot;
	float			yOffset;

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
		void (*backgroundDrawRoutine)(OGLSetupOutputType *));

void LayoutCurrentMenuAgain(void);
int GetCurrentMenu(void);
float GetMenuIdleTime(void);
void KillMenu(int returnCode);
