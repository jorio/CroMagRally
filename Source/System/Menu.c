// MENU.C
// (c)2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "menu.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>
#include <string.h>

#define DECLARE_WORKBUF(buf, bufSize) char (buf)[256]; const int (bufSize) = 256
#define DECLARE_STATIC_WORKBUF(buf, bufSize) static char (buf)[256]; static const int (bufSize) = 256



/****************************/
/*    PROTOTYPES            */
/****************************/

static ObjNode* MakeText(const char* text, int row, int col, int flags);

static const MenuItem* LookUpMenu(int menuID);
static void LayOutMenu(int menuID);

static ObjNode* LayOutCyclerValueText(int row);
static ObjNode* LayOutFloatRangeValueText(int row);

static ObjNode* LayOutCMRCycler(int row, float sweepFactor);
static ObjNode* LayOutCycler(int row, float sweepFactor);
static ObjNode* LayOutPick(int row, float sweepFactor);
static ObjNode* LayOutLabel(int row, float sweepFactor);
static ObjNode* LayOutSubtitle(int row, float sweepFactor);
static ObjNode* LayOutTitle(int row, float sweepFactor);
static ObjNode* LayOutKeyBinding(int row, float sweepFactor);
static ObjNode* LayOutPadBinding(int row, float sweepFactor);
static ObjNode* LayOutMouseBinding(int row, float sweepFactor);
static ObjNode* LayOutFloatRange(int row, float sweepFactor);

static void NavigateCycler(const MenuItem* entry);
static void NavigatePick(const MenuItem* entry);
static void NavigateKeyBinding(const MenuItem* entry);
static void NavigatePadBinding(const MenuItem* entry);
static void NavigateMouseBinding(const MenuItem* entry);
static void NavigateFloatRange(const MenuItem* entry);

#define SpecialRow					Special[0]
#define SpecialCol					Special[1]
#define SpecialMuted				Special[2]
#define SpecialPulsateTimer			SpecialF[0]
#define SpecialSweepTimer			SpecialF[1]
#define SpecialAsyncFadeOutSpeed	SpecialF[2]

/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_MENU_ROWS	25
#define MAX_MENU_COLS	5
#define MAX_STACK_LENGTH 16

#define kSfxNavigate	EFFECT_SELECTCLICK
#define kSfxMenuChange	EFFECT_SELECTCLICK
#define kSfxBack		EFFECT_GETPOW
#define kSfxCycle		EFFECT_SELECTCLICK
#define kSfxError		EFFECT_BADSELECT
#define kSfxDelete		EFFECT_BOOM

#define PlayConfirmEffect() PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3)

const int16_t kJoystickDeadZone_BindingThreshold = (75 * 32767 / 100);

enum
{
	kMenuStateOff,
	kMenuStateFadeIn,
	kMenuStateReady,
	kMenuStateFadeOut,
	kMenuStateAwaitingKeyPress,
	kMenuStateAwaitingPadPress,
	kMenuStateAwaitingMouseClick,
};

const MenuStyle kDefaultMenuStyle =
{
	.darkenPane			= true,
	.darkenPaneScaleY	= 480,
	.darkenPaneOpacity	= .8f,
	.fadeInSpeed		= (1.0f / 0.3f),
	.fadeOutSpeed		= (1.0f / 0.2f),
	.asyncFadeOut		= true,
	.fadeOutSceneOnExit	= true,
	.sweepInSpeed		= (1.0f / 0.2f),
	.titleColor			= {1.0f, 1.0f, 0.7f, 1.0f},
	.highlightColor		= {0.3f, 0.5f, 0.2f, 1.0f},
	.inactiveColor		= {1.0f, 1.0f, 1.0f, 1.0f},
	.inactiveColor2		= {0.5f, 0.5f, 0.5f, 0.5f},
	.standardScale		= .5f,
	.rowHeight			= 40,
	.uniformXExtent		= 0,
	.playMenuChangeSounds	= true,
	.startButtonExits	= false,
	.isInteractive		= true,
	.canBackOutOfRootMenu	= false,
	.textSlot			= SLOT_OF_DUMB + 100,
	.yOffset			= 32,
};

typedef struct
{
	float height;
	ObjNode* (*layOutCallback)(int row, float sweepFactor);
	void (*navigateCallback)(const MenuItem* menuItem);
} MenuItemClass;

static const MenuItemClass kMenuItemClasses[kMI_COUNT] =
{
	[kMISENTINEL]		= {0.0f, NULL, NULL },
	[kMITitle]			= {1.4f, LayOutTitle, NULL },
	[kMISubtitle]		= {0.6f, LayOutSubtitle, NULL },
	[kMILabel]			= {1.0f, LayOutLabel, NULL },
	[kMISpacer]			= {0.5f, NULL, NULL },
	[kMICycler2]			= {1.0f, LayOutCycler, NavigateCycler },
	[kMICycler1]		= {1.0f, LayOutCMRCycler, NavigateCycler },
	[kMIFloatRange]		= {1.0f, LayOutFloatRange, NavigateFloatRange },
	[kMIPick]			= {1.0f, LayOutPick, NavigatePick },
	[kMIKeyBinding]		= {0.5f, LayOutKeyBinding, NavigateKeyBinding },
	[kMIPadBinding]		= {0.5f, LayOutPadBinding, NavigatePadBinding },
	[kMIMouseBinding]	= {0.5f, LayOutMouseBinding, NavigateMouseBinding },
};

/*********************/
/*    VARIABLES      */
/*********************/

typedef struct
{
	int					menuID;
	const MenuItem*		menu;
	MenuStyle			style;

	int					numMenuEntries;
	int					menuRow;
	int					keyColumn;
	int					padColumn;
	float				menuRowYs[MAX_MENU_ROWS];
	float				menuFadeAlpha;
	int					menuState;
	int					menuPick;
	ObjNode*			menuObjects[MAX_MENU_ROWS][MAX_MENU_COLS];

	struct
	{
		int menuID;
		int row;
	} history[MAX_STACK_LENGTH];
	int					historyPos;

	bool				mouseHoverValid;
	int					mouseHoverColumn;
	SDL_Cursor*			handCursor;
	SDL_Cursor*			standardCursor;

	float				idleTime;

	ObjNode*			arrowObjects[2];
} MenuNavigation;

static MenuNavigation* gNav = NULL;

/***********************************/
/*    MENU NAVIGATION STRUCT       */
/***********************************/
#pragma mark - Navigation struct

static void InitMenuNavigation(void)
{
	MenuNavigation* nav = NULL;

	GAME_ASSERT(gNav == NULL);
	nav = (MenuNavigation*) AllocPtrClear(sizeof(MenuNavigation));
	gNav = nav;

	memcpy(&nav->style, &kDefaultMenuStyle, sizeof(MenuStyle));
	nav->menuPick = -1;
	nav->menuState = kMenuStateOff;
	nav->mouseHoverColumn = -1;

	nav->standardCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	nav->handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

	NewObjectDefinitionType arrowDef =
	{
		.scale = 1,
		.slot = SPRITE_SLOT,
	};
	nav->arrowObjects[0] = TextMesh_New("<", 0, &arrowDef);
	nav->arrowObjects[1] = TextMesh_New(">", 0, &arrowDef);
}

static void DisposeMenuNavigation(void)
{
	MenuNavigation* nav = gNav;

	for (int i = 0; i < 2; i++)
	{
		if (nav->arrowObjects[i] != NULL)
		{
			DeleteObject(nav->arrowObjects[i]);
			nav->arrowObjects[i] = NULL;
		}
	}

	if (nav->standardCursor != NULL)
	{
		SDL_FreeCursor(nav->standardCursor);
		nav->standardCursor = NULL;
	}

	if (nav->handCursor != NULL)
	{
		SDL_FreeCursor(nav->handCursor);
		nav->handCursor = NULL;
	}

	SafeDisposePtr((Ptr) nav);

	gNav = nil;
}

int GetCurrentMenu(void)
{
	return gNav->menuID;
}

float GetMenuIdleTime(void)
{
	return gNav->idleTime;
}

void KillMenu(int returnCode)
{
	if (gNav->menuState == kMenuStateReady)
	{
		gNav->menuPick = returnCode;
		gNav->menuState = kMenuStateFadeOut;
	}
}

static void SetStandardMouseCursor(void)
{
	if (gNav->standardCursor != NULL &&
		gNav->standardCursor != SDL_GetCursor())
	{
		SDL_SetCursor(gNav->standardCursor);
	}
}

static void SetHandMouseCursor(void)
{
	if (gNav->handCursor != NULL &&
		gNav->handCursor != SDL_GetCursor())
	{
		SDL_SetCursor(gNav->handCursor);
	}
}

/****************************/
/*    MENU UTILITIES        */
/****************************/
#pragma mark - Utilities

static const char* FourccToString(int fourCC)
{
	static char string[5];
	string[0] = (fourCC >> 24) & 0xFF;
	string[1] = (fourCC >> 16) & 0xFF;
	string[2] = (fourCC >> 8) & 0xFF;
	string[3] = (fourCC) & 0xFF;
	string[4] = 0;
	return string;
}

static OGLColorRGBA PulsateColor(float* time)
{
	*time += gFramesPerSecondFrac;
	float intensity = (1.0f + sinf(*time * 10.0f)) * 0.5f;
	return (OGLColorRGBA) {1,1,1,intensity};
}

static KeyBinding* GetBindingAtRow(int row)
{
	return &gGamePrefs.keys[gNav->menu[row].inputNeed];
}

static const char* GetKeyBindingName(int row, int col)
{
	int16_t scancode = GetBindingAtRow(row)->key[col];

	switch (scancode)
	{
		case 0:								return Localize(STR_UNBOUND_PLACEHOLDER);
		case SDL_SCANCODE_APOSTROPHE:		return "Apostrophe";
		case SDL_SCANCODE_BACKSLASH:		return "Backslash";
		case SDL_SCANCODE_GRAVE:			return "Backtick";
		case SDL_SCANCODE_SEMICOLON:		return "Semicolon";
		default:							return SDL_GetScancodeName(scancode);
	}
}

static const char* GetPadBindingName(int row, int col)
{
	KeyBinding* kb = GetBindingAtRow(row);

	switch (kb->gamepad[col].type)
	{
		case kInputTypeUnbound:
			return Localize(STR_UNBOUND_PLACEHOLDER);

		case kInputTypeButton:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_BUTTON_INVALID:			return Localize(STR_UNBOUND_PLACEHOLDER);
				case SDL_CONTROLLER_BUTTON_A:				return Localize(STR_CONTROLLER_BUTTON_A);
				case SDL_CONTROLLER_BUTTON_B:				return Localize(STR_CONTROLLER_BUTTON_B);
				case SDL_CONTROLLER_BUTTON_X:				return Localize(STR_CONTROLLER_BUTTON_X);
				case SDL_CONTROLLER_BUTTON_Y:				return Localize(STR_CONTROLLER_BUTTON_Y);
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:	return Localize(STR_CONTROLLER_BUTTON_LEFTSHOULDER);
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:	return Localize(STR_CONTROLLER_BUTTON_RIGHTSHOULDER);
				case SDL_CONTROLLER_BUTTON_LEFTSTICK:		return Localize(STR_CONTROLLER_BUTTON_LEFTSTICK);
				case SDL_CONTROLLER_BUTTON_RIGHTSTICK:		return Localize(STR_CONTROLLER_BUTTON_RIGHTSTICK);
				case SDL_CONTROLLER_BUTTON_DPAD_UP:			return Localize(STR_CONTROLLER_BUTTON_DPAD_UP);
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:		return Localize(STR_CONTROLLER_BUTTON_DPAD_DOWN);
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:		return Localize(STR_CONTROLLER_BUTTON_DPAD_LEFT);
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:		return Localize(STR_CONTROLLER_BUTTON_DPAD_RIGHT);
				default:
					return SDL_GameControllerGetStringForButton(kb->gamepad[col].id);
			}
			break;

		case kInputTypeAxisPlus:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_AXIS_INVALID:			return Localize(STR_UNBOUND_PLACEHOLDER);
				case SDL_CONTROLLER_AXIS_LEFTX:				return Localize(STR_CONTROLLER_AXIS_LEFTSTICK_RIGHT);
				case SDL_CONTROLLER_AXIS_LEFTY:				return Localize(STR_CONTROLLER_AXIS_LEFTSTICK_DOWN);
				case SDL_CONTROLLER_AXIS_RIGHTX:			return Localize(STR_CONTROLLER_AXIS_RIGHTSTICK_RIGHT);
				case SDL_CONTROLLER_AXIS_RIGHTY:			return Localize(STR_CONTROLLER_AXIS_RIGHTSTICK_DOWN);
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return Localize(STR_CONTROLLER_AXIS_LEFTTRIGGER);
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return Localize(STR_CONTROLLER_AXIS_RIGHTTRIGGER);
				default:
					return SDL_GameControllerGetStringForAxis(kb->gamepad[col].id);
			}
			break;

		case kInputTypeAxisMinus:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_AXIS_INVALID:			return Localize(STR_UNBOUND_PLACEHOLDER);
				case SDL_CONTROLLER_AXIS_LEFTX:				return Localize(STR_CONTROLLER_AXIS_LEFTSTICK_LEFT);
				case SDL_CONTROLLER_AXIS_LEFTY:				return Localize(STR_CONTROLLER_AXIS_LEFTSTICK_UP);
				case SDL_CONTROLLER_AXIS_RIGHTX:			return Localize(STR_CONTROLLER_AXIS_RIGHTSTICK_LEFT);
				case SDL_CONTROLLER_AXIS_RIGHTY:			return Localize(STR_CONTROLLER_AXIS_RIGHTSTICK_UP);
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return Localize(STR_CONTROLLER_AXIS_LEFTTRIGGER);
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return Localize(STR_CONTROLLER_AXIS_RIGHTTRIGGER);
				default:
					return SDL_GameControllerGetStringForAxis(kb->gamepad[col].id);
			}
			break;

		default:
			return "???";
	}
}

static const char* GetMouseBindingName(int row)
{
	DECLARE_STATIC_WORKBUF(buf, bufSize);

	KeyBinding* kb = GetBindingAtRow(row);

	switch (kb->mouseButton)
	{
		case 0:							return Localize(STR_UNBOUND_PLACEHOLDER);
		case SDL_BUTTON_LEFT:			return Localize(STR_MOUSE_BUTTON_LEFT);
		case SDL_BUTTON_MIDDLE:			return Localize(STR_MOUSE_BUTTON_MIDDLE);
		case SDL_BUTTON_RIGHT:			return Localize(STR_MOUSE_BUTTON_RIGHT);
		case SDL_BUTTON_WHEELUP:		return Localize(STR_MOUSE_WHEEL_UP);
		case SDL_BUTTON_WHEELDOWN:		return Localize(STR_MOUSE_WHEEL_DOWN);
		default:
			snprintf(buf, bufSize, "%s %d", Localize(STR_BUTTON), kb->mouseButton);
			return buf;
	}
}

static bool IsMenuItemSelectable(const MenuItem* mi)
{
	switch (mi->type)
	{
		case kMISpacer:
		case kMILabel:
		case kMITitle:
		case kMISubtitle:
			return false;
		
		default:
			if (mi->displayIf != NULL && !(mi->displayIf(mi)))
			{
				return false;
			}
			else if (mi->enableIf != NULL)
			{
				return mi->enableIf(mi);
			}
			else
			{
				return true;
			}
	}

}

static void ReplaceMenuText(LocStrID originalTextInMenuDefinition, LocStrID newText)
{
	for (int i = 0; i < MAX_MENU_ROWS && gNav->menu[i].type != kMISENTINEL; i++)
	{
		if (gNav->menu[i].text == originalTextInMenuDefinition)
		{
			MakeText(Localize(newText), i, 0, 0);
		}
	}
}

static void SaveSelectedRowInHistory(void)
{
	gNav->history[gNav->historyPos].row = gNav->menuRow;
}

/****************************/
/*    MENU MOVE CALLS       */
/****************************/
#pragma mark - Move calls

static void MoveDarkenPane(ObjNode* node)
{
	node->ColorFilter.a = gNav->menuFadeAlpha * gNav->style.darkenPaneOpacity;
}

static void MoveGenericMenuItem(ObjNode* node, float baseAlpha)
{
	float sweepAlpha = 1;

	if (node->SpecialSweepTimer < 1.0f)
	{
		node->SpecialSweepTimer += gFramesPerSecondFrac * gNav->style.sweepInSpeed;

		if (node->SpecialSweepTimer < 0)
		{
			sweepAlpha = 0;
		}
		else if (node->SpecialSweepTimer < 1)
		{
			sweepAlpha = node->SpecialSweepTimer;

			float xBackup = node->Coord.x;

			float p = (1.0f - node->SpecialSweepTimer);
			node->Coord.x -= p*p * 50.0f;
			UpdateObjectTransforms(node);

			node->Coord.x = xBackup;
		}
		else
		{
			sweepAlpha = 1;
			UpdateObjectTransforms(node);
		}
	}

	if (node->SpecialMuted)
	{
		baseAlpha *= .5f;
	}

	node->ColorFilter.a = baseAlpha * sweepAlpha;

	// Don't mix gNav->menuFadeAlpha -- it's only useful when fading OUT,
	// in which case we're using a different move call for all menu items
}

static void MoveLabel(ObjNode* node)
{
	MoveGenericMenuItem(node, 1);
}

static void MoveAction(ObjNode* node)
{
	if (node->SpecialRow == gNav->menuRow)
		node->ColorFilter = gNav->style.highlightColor; //TwinkleColor();
	else
		node->ColorFilter = gNav->style.inactiveColor;

	MoveGenericMenuItem(node, 1);
}

static void MoveKeyBinding(ObjNode* node)
{
	if (node->SpecialRow == gNav->menuRow && node->SpecialCol == (gNav->keyColumn+1))
	{
		if (gNav->menuState == kMenuStateAwaitingKeyPress)
		{
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		}
		else
			node->ColorFilter = gNav->style.highlightColor; //TwinkleColor();
	}
	else
	{
		node->ColorFilter = gNav->style.inactiveColor;
	}

	MoveGenericMenuItem(node, node->ColorFilter.a);
}

static void MovePadBinding(ObjNode* node)
{
	if (node->SpecialRow == gNav->menuRow && node->SpecialCol == (gNav->padColumn+1))
	{
		if (gNav->menuState == kMenuStateAwaitingPadPress)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = gNav->style.highlightColor; //TwinkleColor();
	}
	else
	{
		node->ColorFilter = gNav->style.inactiveColor;
	}

	MoveGenericMenuItem(node, node->ColorFilter.a);
}

static void MoveMouseBinding(ObjNode* node)
{
	if (node->SpecialRow == gNav->menuRow)
	{
		if (gNav->menuState == kMenuStateAwaitingMouseClick)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = gNav->style.highlightColor; //TwinkleColor();
	}
	else
	{
		node->ColorFilter = gNav->style.inactiveColor;
	}

	MoveGenericMenuItem(node, node->ColorFilter.a);
}

static void MoveAsyncFadeOutAndDelete(ObjNode *theNode)
{
	theNode->ColorFilter.a -= gFramesPerSecondFrac * theNode->SpecialAsyncFadeOutSpeed;
	if (theNode->ColorFilter.a < 0.0f)
	{
		DeleteObject(theNode);
	}
}

static void InstallAsyncFadeOut(ObjNode* theNode)
{
	theNode->MoveCall = MoveAsyncFadeOutAndDelete;
	theNode->SpecialAsyncFadeOutSpeed = gNav->style.fadeOutSpeed;

}

/****************************/
/*    MENU NAVIGATION       */
/****************************/
#pragma mark - Menu navigation

static void GoBackInHistory(void)
{
	MyFlushEvents();

	if (gNav->historyPos != 0)
	{
		PlayEffect(kSfxBack);
		gNav->historyPos--;
		LayOutMenu(gNav->history[gNav->historyPos].menuID);
	}
	else if (gNav->style.canBackOutOfRootMenu)
	{
		PlayEffect(kSfxBack);
		gNav->menuState = kMenuStateFadeOut;
	}
	else
	{
		PlayEffect(kSfxError);
	}
}

static void RepositionArrows(void)
{
	bool showArrows = true;
	switch (gNav->menu[gNav->menuRow].type)
	{
		case kMICycler1:
		case kMICycler2:
		case kMIFloatRange:
		case kMIKeyBinding:
		case kMIPadBinding:
		case kMIMouseBinding:
			showArrows = true;
			break;
		default:
			showArrows = false;
			break;
	}

	if (showArrows)
	{
		ObjNode* snapTo = gNav->menuObjects[gNav->menuRow][0];
		OGLRect extents = TextMesh_GetExtents(snapTo);

		float spacing = 50 * snapTo->Scale.x;

		for (int i = 0; i < 2; i++)
		{
			ObjNode* arrowObj = gNav->arrowObjects[i];
			SetObjectVisible(arrowObj, true);

			arrowObj->Coord.x = i==0? extents.left - spacing: extents.right + spacing;
			arrowObj->Coord.y = snapTo->Coord.y;
			arrowObj->Scale = snapTo->Scale;
			UpdateObjectTransforms(arrowObj);

		}
	}
	else
	{

		for (int i = 0; i < 2; i++)
		{
			SetObjectVisible(gNav->arrowObjects[i], false);
		}
	}


}

static void NavigateSettingEntriesVertically(int delta)
{
	bool makeSound = gNav->menuRow >= 0;
	int browsed = 0;
	bool skipEntry = false;

	do
	{
		gNav->menuRow += delta;
		gNav->menuRow = PositiveModulo(gNav->menuRow, (unsigned int)gNav->numMenuEntries);

		skipEntry = !IsMenuItemSelectable(&gNav->menu[gNav->menuRow]);

		if (browsed++ > gNav->numMenuEntries)
		{
			// no entries are selectable
			return;
		}
	} while (skipEntry);

	gNav->idleTime = 0;
	gNav->mouseHoverValid = false;

	if (makeSound)
	{
		PlayEffect(kSfxNavigate);
	}

	// Reposition arrows
	RepositionArrows();
}

static void NavigateSettingEntriesMouseHover(void)
{
	if (!gMouseMotionNow)
	{
		return;
	}

	int mxRaw, myRaw;
	SDL_GetMouseState(&mxRaw, &myRaw);

	float mx = (mxRaw - gGameWindowWidth/2.0f) * g2DLogicalWidth / gGameWindowWidth;
	float my = (myRaw - gGameWindowHeight/2.0f) * g2DLogicalHeight / gGameWindowHeight;

	gNav->mouseHoverValid = false;
	gNav->mouseHoverColumn = -1;

	for (int row = 0; row < gNav->numMenuEntries; row++)
	{
		if (!IsMenuItemSelectable(&gNav->menu[row]))
		{
			continue;
		}

		OGLRect fullExtents;
		fullExtents.top		= fullExtents.left	= 100000;
		fullExtents.bottom	= fullExtents.right	= -100000;

		for (int col = 0; col < MAX_MENU_COLS; col++)
		{
			ObjNode* textNode = gNav->menuObjects[row][col];
			if (!textNode)
				continue;

			OGLRect extents = TextMesh_GetExtents(textNode);
			if (extents.top		< fullExtents.top	) fullExtents.top		= extents.top;
			if (extents.left	< fullExtents.left	) fullExtents.left		= extents.left;
			if (extents.bottom	> fullExtents.bottom) fullExtents.bottom	= extents.bottom;
			if (extents.right	> fullExtents.right	) fullExtents.right		= extents.right;

			if (my >= extents.top
				&& my <= extents.bottom
				&& mx >= extents.left - 10
				&& mx <= extents.right + 10)
			{
				gNav->mouseHoverColumn = col;
			}
		}

		if (my >= fullExtents.top &&
			my <= fullExtents.bottom &&
			mx >= fullExtents.left - 10 &&
			mx <= fullExtents.right + 10)
		{
			gNav->mouseHoverValid = true;

			SetHandMouseCursor();				// set hand cursor

			if (gNav->menuRow != row)
			{
				gNav->menuRow = row;
				PlayEffect(kSfxNavigate);
			}

			return;
		}
	}

	GAME_ASSERT(!gNav->mouseHoverValid);		// if we got here, we're not hovering over anything

	SetStandardMouseCursor();					// restore standard cursor
}

static void NavigatePick(const MenuItem* entry)
{
	if (GetNewNeedStateAnyP(kNeed_UIConfirm) || (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT)))
	{
		PlayConfirmEffect();

		gNav->idleTime = 0;
		gNav->menuPick = entry->id;

		if (entry->callback)
		{
			entry->callback(entry);
		}

		switch (entry->next)
		{
			case 0:
			case 'NOOP':
				break;

			case 'EXIT':
				gNav->menuState = kMenuStateFadeOut;
				break;

			case 'BACK':
				GoBackInHistory();
				break;

			default:
				SaveSelectedRowInHistory();  // remember which row we were on

				// advance history
				gNav->historyPos++;
				gNav->history[gNav->historyPos].menuID = entry->next;
				gNav->history[gNav->historyPos].row = 0;  // don't reuse stale row value

				LayOutMenu(entry->next);
		}
	}
}

static int GetCyclerNumChoices(const MenuItem* entry)
{
	for (int i = 0; i < MAX_MENU_CYCLER_CHOICES; i++)
	{
		if (entry->cycler.choices[i].text == STR_NULL)
			return i;
	}

	return MAX_MENU_CYCLER_CHOICES;
}

static int GetValueIndexInCycler(const MenuItem* entry, uint8_t value)
{
	for (int i = 0; i < MAX_MENU_CYCLER_CHOICES; i++)
	{
		if (entry->cycler.choices[i].text == STR_NULL)
			break;

		if (entry->cycler.choices[i].value == value)
			return i;
	}

	return -1;
}

static void NavigateCycler(const MenuItem* entry)
{
	int delta = 0;

	if (GetNewNeedStateAnyP(kNeed_UILeft)
		|| GetNewNeedStateAnyP(kNeed_UIPrev)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_RIGHT))
//		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_WHEELDOWN))
		)
	{
		delta = -1;
	}
	else if (GetNewNeedStateAnyP(kNeed_UIRight)
		|| GetNewNeedStateAnyP(kNeed_UINext)
		|| GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT))
//		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_WHEELUP))
		)
	{
		delta = 1;
	}

	if (delta != 0)
	{
		gNav->idleTime = 0;
		PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3 + (RandomFloat2() * 0x3000));

		if (entry->cycler.valuePtr && !entry->cycler.callbackSetsValue)
		{
			int index = GetValueIndexInCycler(entry, *entry->cycler.valuePtr);
			if (index >= 0)
				index = PositiveModulo(index + delta, GetCyclerNumChoices(entry));
			else
				index = 0;
			*entry->cycler.valuePtr = entry->cycler.choices[index].value;
		}

		if (entry->callback)
			entry->callback(entry);

		if (entry->type == kMICycler1)
			LayOutCMRCycler(gNav->menuRow, 0);
		else
			LayOutCyclerValueText(gNav->menuRow);
	}
}

static void NavigateFloatRange(const MenuItem* entry)
{
	int delta = 0;

	if (GetNewNeedStateAnyP(kNeed_UILeft)
		|| GetNewNeedStateAnyP(kNeed_UIPrev)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_RIGHT)))
	{
		delta = -1;
	}
	else if (GetNewNeedStateAnyP(kNeed_UIRight)
		|| GetNewNeedStateAnyP(kNeed_UINext)
		|| GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT)))
	{
		delta = 1;
	}

	if (delta != 0)
	{
		gNav->idleTime = 0;
		PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2 / 3 + (RandomFloat2() * 0x3000));

		*entry->floatRange.valuePtr = *entry->floatRange.valuePtr + delta;

		if (entry->callback)
			entry->callback(entry);

		LayOutFloatRangeValueText(gNav->menuRow);
	}
}

static void NavigateKeyBinding(const MenuItem* entry)
{
	if (gNav->mouseHoverValid && (gNav->mouseHoverColumn == 1 || gNav->mouseHoverColumn == 2))
		gNav->keyColumn = gNav->mouseHoverColumn - 1;

	if (GetNewNeedStateAnyP(kNeed_UILeft) || GetNewNeedStateAnyP(kNeed_UIPrev))
	{
		gNav->idleTime = 0;
		gNav->keyColumn = PositiveModulo(gNav->keyColumn - 1, KEYBINDING_MAX_KEYS);
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValid = false;
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIRight) || GetNewNeedStateAnyP(kNeed_UINext))
	{
		gNav->idleTime = 0;
		gNav->keyColumn = PositiveModulo(gNav->keyColumn + 1, KEYBINDING_MAX_KEYS);
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValid = false;
		return;
	}

	// Past this point we must have a valid column
	if (gNav->keyColumn < 0 || gNav->keyColumn >= KEYBINDING_MAX_KEYS)
	{
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| GetNewKeyState(SDL_SCANCODE_SPACE)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_MIDDLE)))
	{
		gNav->idleTime = 0;
		gGamePrefs.keys[entry->inputNeed].key[gNav->keyColumn] = 0;
		PlayEffect(kSfxDelete);
		MakeText(Localize(STR_UNBOUND_PLACEHOLDER), gNav->menuRow, gNav->keyColumn+1, kTextMeshAllCaps | kTextMeshAlignLeft);
		return;
	}

	if (GetNewKeyState(SDL_SCANCODE_RETURN)
		|| GetNewKeyState(SDL_SCANCODE_KP_ENTER)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT)))
	{
		gNav->idleTime = 0;
		gNav->menuState = kMenuStateAwaitingKeyPress;
		MakeText(Localize(STR_PRESS), gNav->menuRow, gNav->keyColumn+1, kTextMeshAllCaps | kTextMeshAlignLeft);

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP_CANCEL);

		return;
	}
}

static void NavigatePadBinding(const MenuItem* entry)
{
	if (gNav->mouseHoverValid && (gNav->mouseHoverColumn == 1 || gNav->mouseHoverColumn == 2))
		gNav->padColumn = gNav->mouseHoverColumn - 1;

	if (GetNewNeedStateAnyP(kNeed_UILeft) || GetNewNeedStateAnyP(kNeed_UIPrev))
	{
		gNav->idleTime = 0;
		gNav->padColumn = PositiveModulo(gNav->padColumn - 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValid = false;
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIRight) || GetNewNeedStateAnyP(kNeed_UINext))
	{
		gNav->idleTime = 0;
		gNav->padColumn = PositiveModulo(gNav->padColumn + 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValid = false;
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| GetNewKeyState(SDL_SCANCODE_SPACE)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_MIDDLE)))
	{
		gNav->idleTime = 0;
		gGamePrefs.keys[entry->inputNeed].gamepad[gNav->padColumn].type = kInputTypeUnbound;
		PlayEffect(kSfxDelete);
		MakeText(Localize(STR_UNBOUND_PLACEHOLDER), gNav->menuRow, gNav->padColumn+1, kTextMeshAllCaps | kTextMeshAlignLeft);
		return;
	}

	if ( (GetNewNeedStateAnyP(kNeed_UIConfirm) && !GetNewKeyState(SDL_SCANCODE_SPACE))
		|| GetNewKeyState(SDL_SCANCODE_KP_ENTER)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT)))
	{
		gNav->idleTime = 0;
		InvalidateAllInputs();

		gNav->menuState = kMenuStateAwaitingPadPress;
		MakeText(Localize(STR_PRESS), gNav->menuRow, gNav->padColumn+1, kTextMeshAllCaps | kTextMeshAlignLeft);

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP_CANCEL);

		return;
	}
}

static void NavigateMouseBinding(const MenuItem* entry)
{
	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_MIDDLE)))
	{
		gNav->idleTime = 0;
		gGamePrefs.keys[entry->inputNeed].mouseButton = 0;
		PlayEffect(kSfxDelete);
		MakeText(Localize(STR_UNBOUND_PLACEHOLDER), gNav->menuRow, 1, 0);
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT)))
	{
		gNav->idleTime = 0;
		InvalidateAllInputs();

		gNav->menuState = kMenuStateAwaitingMouseClick;
		MakeText(Localize(STR_CLICK), gNav->menuRow, 1, 0);
		return;
	}
}

static void NavigateMenu(void)
{
	GAME_ASSERT(gNav->style.isInteractive);

	if (GetNewNeedStateAnyP(kNeed_UIBack)
		|| GetNewClickState(SDL_BUTTON_X1))
	{
		GoBackInHistory();
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIUp))
	{
		NavigateSettingEntriesVertically(-1);
		SaveSelectedRowInHistory();
	}
	else if (GetNewNeedStateAnyP(kNeed_UIDown))
	{
		NavigateSettingEntriesVertically(1);
		SaveSelectedRowInHistory();
	}
	else
	{
//		NavigateSettingEntriesMouseHover();
	}

	const MenuItem* entry = &gNav->menu[gNav->menuRow];
	const MenuItemClass* cls = &kMenuItemClasses[entry->type];

	if (cls->navigateCallback)
	{
		cls->navigateCallback(entry);
	}
}

/****************************/
/* INPUT BINDING CALLBACKS  */
/****************************/
#pragma mark - Input binding callbacks

static void UnbindScancodeFromAllRemappableInputNeeds(int16_t sdlScancode)
{
	for (int row = 0; row < gNav->numMenuEntries; row++)
	{
		if (gNav->menu[row].type != kMIKeyBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
		{
			if (binding->key[j] == sdlScancode)
			{
				binding->key[j] = 0;
				MakeText(Localize(STR_UNBOUND_PLACEHOLDER), row, j+1, kTextMeshAllCaps | kTextMeshAlignLeft);
			}
		}
	}
}

static void UnbindPadButtonFromAllRemappableInputNeeds(int8_t type, int8_t id)
{
	for (int row = 0; row < gNav->numMenuEntries; row++)
	{
		if (gNav->menu[row].type != kMIPadBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
		{
			if (binding->gamepad[j].type == type && binding->gamepad[j].id == id)
			{
				binding->gamepad[j].type = kInputTypeUnbound;
				binding->gamepad[j].id = 0;
				MakeText(Localize(STR_UNBOUND_PLACEHOLDER), row, j+1, kTextMeshAllCaps | kTextMeshAlignLeft);
			}
		}
	}
}

static void UnbindMouseButtonFromAllRemappableInputNeeds(int8_t id)
{
	for (int row = 0; row < gNav->numMenuEntries; row++)
	{
		if (gNav->menu[row].type != kMIMouseBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		if (binding->mouseButton == id)
		{
			binding->mouseButton = 0;
			MakeText(Localize(STR_UNBOUND_PLACEHOLDER), row, 1, kTextMeshAllCaps | kTextMeshAlignLeft);
		}
	}
}

static void AwaitKeyPress(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		MakeText(GetKeyBindingName(gNav->menuRow, gNav->keyColumn), gNav->menuRow, 1 + gNav->keyColumn, kTextMeshAllCaps | kTextMeshAlignLeft);
		gNav->menuState = kMenuStateReady;
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gNav->menuRow);

	for (int16_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
	{
		if (GetNewKeyState(scancode))
		{
			UnbindScancodeFromAllRemappableInputNeeds(scancode);
			kb->key[gNav->keyColumn] = scancode;
			MakeText(GetKeyBindingName(gNav->menuRow, gNav->keyColumn), gNav->menuRow, gNav->keyColumn+1, kTextMeshAllCaps | kTextMeshAlignLeft);
			gNav->menuState = kMenuStateReady;
			gNav->idleTime = 0;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
			return;
		}
	}


}

static bool AwaitGamepadPress(SDL_GameController* controller)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE)
		|| SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))
	{
		MakeText(GetPadBindingName(gNav->menuRow, gNav->padColumn), gNav->menuRow, 1 + gNav->padColumn, kTextMeshAllCaps | kTextMeshAlignLeft);
		gNav->menuState = kMenuStateReady;
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP);
		return true;
	}

	KeyBinding* kb = GetBindingAtRow(gNav->menuRow);

	for (int8_t button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
	{
#if 0
		switch (button)
		{
			case SDL_CONTROLLER_BUTTON_DPAD_UP:			// prevent binding those
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				continue;
		}
#endif

		if (SDL_GameControllerGetButton(controller, button))
		{
			UnbindPadButtonFromAllRemappableInputNeeds(kInputTypeButton, button);
			kb->gamepad[gNav->padColumn].type = kInputTypeButton;
			kb->gamepad[gNav->padColumn].id = button;
			MakeText(GetPadBindingName(gNav->menuRow, gNav->padColumn), gNav->menuRow, gNav->padColumn+1, kTextMeshAllCaps | kTextMeshAlignLeft);
			gNav->menuState = kMenuStateReady;
			gNav->idleTime = 0;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP);
			return true;
		}
	}

	for (int8_t axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
	{
		switch (axis)
		{
			case SDL_CONTROLLER_AXIS_LEFTX:				// prevent binding those
			case SDL_CONTROLLER_AXIS_LEFTY:
#if 0
			case SDL_CONTROLLER_AXIS_RIGHTX:
			case SDL_CONTROLLER_AXIS_RIGHTY:
#endif
				continue;
		}

		int16_t axisValue = SDL_GameControllerGetAxis(controller, axis);
		if (abs(axisValue) > kJoystickDeadZone_BindingThreshold)
		{
			int axisType = axisValue < 0 ? kInputTypeAxisMinus : kInputTypeAxisPlus;
			UnbindPadButtonFromAllRemappableInputNeeds(axisType, axis);
			kb->gamepad[gNav->padColumn].type = axisType;
			kb->gamepad[gNav->padColumn].id = axis;
			MakeText(GetPadBindingName(gNav->menuRow, gNav->padColumn), gNav->menuRow, gNav->padColumn+1, kTextMeshAllCaps | kTextMeshAlignLeft);
			gNav->menuState = kMenuStateReady;
			gNav->idleTime = 0;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP_CANCEL, STR_CONFIGURE_GAMEPAD_HELP);
			return true;
		}
	}

	return false;
}

static void AwaitMetaGamepadPress(void)
{
	bool anyGamepadFound = false;

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		SDL_GameController* controller = GetController(i);
		if (controller)
		{
			anyGamepadFound = true;
			if (AwaitGamepadPress(controller))
			{
				return;
			}
		}
	}

	if (!anyGamepadFound)
	{
		MakeText(GetPadBindingName(gNav->menuRow, gNav->padColumn), gNav->menuRow, gNav->padColumn+1, kTextMeshAllCaps | kTextMeshAlignLeft);
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_NO_GAMEPAD_DETECTED);
		PlayEffect(kSfxError);
		gNav->menuState = kMenuStateReady;
		gNav->idleTime = 0;
	}
}

static void AwaitMouseClick(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		MakeText(GetMouseBindingName(gNav->menuRow), gNav->menuRow, 1, 0);
		gNav->menuState = kMenuStateReady;
		gNav->idleTime = 0;
		PlayEffect(kSfxError);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gNav->menuRow);

	for (int8_t mouseButton = 0; mouseButton < NUM_SUPPORTED_MOUSE_BUTTONS; mouseButton++)
	{
		if (GetNewClickState(mouseButton))
		{
			UnbindMouseButtonFromAllRemappableInputNeeds(mouseButton);
			kb->mouseButton = mouseButton;
			MakeText(GetMouseBindingName(gNav->menuRow), gNav->menuRow, 1, 0);
			gNav->menuState = kMenuStateReady;
			gNav->idleTime = 0;
			PlayEffect(kSfxCycle);
			return;
		}
	}
}

/****************************/
/*    PAGE LAYOUT           */
/****************************/
#pragma mark - Page Layout

#if 0
static ObjNode* MakeDarkenPane(void)
{
	ObjNode* pane;

	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.flags = STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOFOG
				| STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_MOVEINPAUSE,
		.slot = SLOT_OF_DUMB + 100 - 1,
		.scale = 1,
	};

	pane = MakeNewObject(&def);
	pane->CustomDrawFunction = DrawDarkenPane;
	pane->ColorFilter = (OGLColorRGBA) {0, 0, 0, 0};
	pane->Scale.y = gNav->style.darkenPaneScaleY;
	pane->MoveCall = MoveDarkenPane;

	return pane;
}
#endif

static void DeleteAllText(void)
{
	for (int row = 0; row < MAX_MENU_ROWS; row++)
	{
		for (int col = 0; col < MAX_MENU_COLS; col++)
		{
			if (gNav->menuObjects[row][col])
			{
				DeleteObject(gNav->menuObjects[row][col]);
				gNav->menuObjects[row][col] = nil;
			}
		}
	}
}

static float GetMenuItemHeight(int row)
{
	const MenuItem* menuItem = &gNav->menu[row];
	
	if (menuItem->displayIf && !(menuItem->displayIf(menuItem)))
		return false;
	else if (menuItem->customHeight > 0)
		return menuItem->customHeight;
	else
		return kMenuItemClasses[menuItem->type].height;
}

static ObjNode* MakeText(const char* text, int row, int col, int textMeshFlags)
{
	ObjNode* node = gNav->menuObjects[row][col];

	if (node)
	{
		// Recycling existing text lets us keep the move call, color and specials
		TextMesh_Update(text, textMeshFlags, node);
	}
	else
	{
		NewObjectDefinitionType def =
		{
			.coord = (OGLPoint3D) { 0, gNav->menuRowYs[row], 0 },
			.scale = GetMenuItemHeight(row) * gNav->style.standardScale,
			.slot = SLOT_OF_DUMB + 100,
			.flags = STATUS_BIT_MOVEINPAUSE | STATUS_BIT_OVERLAYPANE,
		};

		node = TextMesh_New(text, textMeshFlags, &def);
		node->SpecialRow = row;
		node->SpecialCol = col;
		gNav->menuObjects[row][col] = node;
	}

	/*
	if (centered)
	{
		int paddedRightOff = ((gNav->menuColXs[col+1]-170) - node->Coord.x) / node->Scale.x;
		if (paddedRightOff > node->RightOff)
			node->RightOff = paddedRightOff;
	}
	*/

	if (gNav->style.uniformXExtent)
	{
		if (-gNav->style.uniformXExtent < node->LeftOff)
			node->LeftOff = -gNav->style.uniformXExtent;
		if (gNav->style.uniformXExtent > node->RightOff)
			node->RightOff = gNav->style.uniformXExtent;
	}

	return node;
}

static const char* GetMenuItemLabel(const MenuItem* entry)
{
	if (entry->rawText)
		return entry->rawText;
	else if (entry->generateText)
		return entry->generateText();
	else
		return Localize(entry->text);
}

static const char* GetCyclerValueText(int row)
{
	const MenuItem* entry = &gNav->menu[row];
	int index = GetValueIndexInCycler(entry, *entry->cycler.valuePtr);
	if (index >= 0)
		return Localize(entry->cycler.choices[index].text);
	return "VALUE NOT FOUND???";
}

static ObjNode* LayOutTitle(int row, float sweepFactor)
{
	const MenuItem* entry = &gNav->menu[row];

	ObjNode* label = MakeText(GetMenuItemLabel(entry), row, 0, 0);
	label->ColorFilter = gNav->style.titleColor;
	label->MoveCall = MoveLabel;
	label->SpecialSweepTimer = .5f;		// Title appears sooner than the rest

	return label;
}

static ObjNode* LayOutSubtitle(int row, float sweepFactor)
{
	const MenuItem* entry = &gNav->menu[row];
	
	ObjNode* label = MakeText(GetMenuItemLabel(entry), row, 0, 0);
	label->ColorFilter = (OGLColorRGBA){ 0.7, .4, .2, 1 };
	label->MoveCall = MoveLabel;
	label->SpecialSweepTimer = .5f;		// Title appears sooner than the rest

	return label;
}

static ObjNode* LayOutLabel(int row, float sweepFactor)
{
	const MenuItem* entry = &gNav->menu[row];

	ObjNode* label = MakeText(GetMenuItemLabel(entry), row, 0, 0);
	label->ColorFilter = (OGLColorRGBA){ 0.7, .4, .2, 1 };//gNav->style.inactiveColor;
	label->MoveCall = MoveLabel;
	label->SpecialSweepTimer = sweepFactor;

	return label;
}

static ObjNode* LayOutPick(int row, float sweepFactor)
{
	const MenuItem* entry = &gNav->menu[row];

	ObjNode* node = MakeText(GetMenuItemLabel(entry), row, 0, 0);
	node->MoveCall = MoveAction;
	node->SpecialSweepTimer = sweepFactor;
	node->SpecialMuted = !IsMenuItemSelectable(entry);

	return node;
}

static ObjNode* LayOutCyclerValueText(int row)
{
	ObjNode* node2 = MakeText(GetCyclerValueText(row), row, 1, 0);
	node2->MoveCall = MoveAction;
	return node2;
}

static ObjNode* LayOutCycler(int row, float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);

	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", GetMenuItemLabel(entry));

	ObjNode* node1 = MakeText(buf, row, 0, 0);
	node1->MoveCall = MoveAction;
	node1->SpecialSweepTimer = sweepFactor;

	ObjNode* node2 = LayOutCyclerValueText(row);
	node2->SpecialSweepTimer = sweepFactor;

	return node1;	// TODO: chain node2?
}

static ObjNode* LayOutCMRCycler(int row, float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);

	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s: %s", GetMenuItemLabel(entry), GetCyclerValueText(row));

	ObjNode* node = MakeText(buf, row, 0, 0);
	node->MoveCall = MoveAction;
	node->SpecialSweepTimer = sweepFactor;

	return node;
}

static ObjNode* LayOutFloatRangeValueText(int row)
{
	const MenuItem* entry = &gNav->menu[row];
	DECLARE_WORKBUF(buf, bufSize);

	snprintf(buf, bufSize, "%.4f", *entry->floatRange.valuePtr);
	ObjNode* node2 = MakeText(buf, row, 3, kTextMeshAlignRight);
	return node2;
}

static ObjNode* LayOutFloatRange(int row, float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", GetMenuItemLabel(entry));
	ObjNode* node1 = MakeText(buf, row, 0, kTextMeshAlignLeft);
	node1->MoveCall = MoveAction;
	node1->SpecialSweepTimer = sweepFactor;

	ObjNode* node2 = LayOutFloatRangeValueText(row);
	node2->SpecialSweepTimer = sweepFactor;

	return node1;	// TODO: chain node2?
}

static ObjNode* LayOutKeyBinding(int row, float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->inputNeed));

	ObjNode* label = MakeText(buf, row, 0, kTextMeshAlignLeft);
	label->Coord.x -= 256;
	label->ColorFilter = gNav->style.inactiveColor2;
	label->MoveCall = MoveLabel;
	label->SpecialSweepTimer = sweepFactor;

	for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
	{
		ObjNode* keyNode = MakeText(GetKeyBindingName(row, j), row, j + 1, kTextMeshAllCaps | kTextMeshAlignLeft);
		keyNode->Coord.x = -50 + j * 170 ;
		keyNode->MoveCall = MoveKeyBinding;
//		keyNode->Scale.x *= 0.75f;
		keyNode->SpecialSweepTimer = sweepFactor;
	}

	return label;	// TODO: chain other nodes?
}

static ObjNode* LayOutPadBinding(int row, float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->inputNeed));

	ObjNode* label = MakeText(buf, row, 0, kTextMeshAlignLeft);
	label->Coord.x -= 256;
	label->ColorFilter = gNav->style.inactiveColor2;
	label->MoveCall = MoveLabel;
	label->SpecialSweepTimer = sweepFactor;

	for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
	{
		ObjNode* keyNode = MakeText(GetPadBindingName(row, j), row, j+1, kTextMeshAllCaps | kTextMeshAlignLeft);
		keyNode->Coord.x = -50 + j * 170;
		keyNode->MoveCall = MovePadBinding;
		keyNode->SpecialSweepTimer = sweepFactor;
	}

	return label;		// TODO: chain other nodes?
}

static ObjNode* LayOutMouseBinding(int row, float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->inputNeed));

	ObjNode* label = MakeText(buf, row, 0, 0);
	label->ColorFilter = gNav->style.inactiveColor2;
	label->MoveCall = MoveAction;
	label->SpecialSweepTimer = sweepFactor;

	ObjNode* keyNode = MakeText(GetMouseBindingName(row), row, 1, 0);
	keyNode->MoveCall = MoveMouseBinding;
	keyNode->SpecialSweepTimer = sweepFactor;
	
	return label;		// TODO: chain other nodes?
}

static void LayOutMenu(int menuID)
{
	// Remember we've been to this menu
	gNav->history[gNav->historyPos].menuID = menuID;

	const MenuItem* menu = LookUpMenu(menuID);

	if (!menu)
	{
		DoFatalAlert("Menu not registered: '%s'", FourccToString(menuID));
	}

	gNav->menu				= menu;
	gNav->menuID			= menuID;
	gNav->numMenuEntries	= 0;
	gNav->menuPick			= -1;
	gNav->idleTime			= 0;

	// Restore old row
	gNav->menuRow = gNav->history[gNav->historyPos].row;

	DeleteAllText();

	float totalHeight = 0;
	for (int row = 0; menu[row].type != kMISENTINEL; row++)
	{
		totalHeight += GetMenuItemHeight(row) * gNav->style.rowHeight;
	}

	float y = -totalHeight*.5f + gNav->style.yOffset;

	float sweepFactor = 0.0f;

	for (int row = 0; menu[row].type != kMISENTINEL; row++)
	{
		gNav->menuRowYs[row] = y;

		const MenuItem* entry = &menu[row];
		const MenuItemClass* cls = &kMenuItemClasses[entry->type];

		if (entry->displayIf && !(entry->displayIf(entry)))
		{
			gNav->numMenuEntries++;
			continue;
		}

		if (cls->layOutCallback)
		{
			cls->layOutCallback(row, sweepFactor);
		}

		y += GetMenuItemHeight(row) * gNav->style.rowHeight;

		if (entry->type != kMISpacer)
		{
			sweepFactor -= .2f;
		}

		gNav->numMenuEntries++;
		GAME_ASSERT(gNav->numMenuEntries < MAX_MENU_ROWS);
	}

	if (gNav->menuRow <= 0
		|| !IsMenuItemSelectable(&gNav->menu[gNav->menuRow]))	// we had selected this item when we last were in this menu, but it's been disabled since then
	{
		// Scroll down to first pickable entry
		gNav->menuRow = -1;
		NavigateSettingEntriesVertically(1);
	}

	RepositionArrows();
}

void LayoutCurrentMenuAgain(void)
{
	GAME_ASSERT(gNav->menu);
	LayOutMenu(gNav->menuID);
}

#define MAX_REGISTERED_MENUS 32

int gNumMenusRegistered = 0;
const MenuItem* gMenuRegistry[MAX_REGISTERED_MENUS];

void RegisterMenu(const MenuItem* menuTree)
{
	for (const MenuItem* menuItem = menuTree; menuItem->type || menuItem->id; menuItem++)
	{
		if (menuItem->type == 0)
		{
			if (menuItem->id == 0)			// end sentinel
			{
				break;
			}

			if (LookUpMenu(menuItem->id))	// already registered
			{
				continue;
			}

			GAME_ASSERT(gNumMenusRegistered < MAX_REGISTERED_MENUS);
			gMenuRegistry[gNumMenusRegistered] = menuItem;
			gNumMenusRegistered++;

//			printf("Registered menu '%s'\n", FourccToString(menuItem->id));
		}
	}
}

static const MenuItem* LookUpMenu(int menuID)
{
	for (int i = 0; i < gNumMenusRegistered; i++)
	{
		if (gMenuRegistry[i]->id == menuID)
			return gMenuRegistry[i] + 1;
	}

	return NULL;
}

int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*update)(void),
		void (*draw)(OGLSetupOutputType *))
{
	int cursorStateBeforeMenu = SDL_ShowCursor(-1);
	SDL_ShowCursor(1);

		/* INITIALIZE MENU STATE */

	gNumMenusRegistered = 0;
	RegisterMenu(menu);

	InitMenuNavigation();
	if (style)
		memcpy(&gNav->style, style, sizeof(*style));
	gNav->menuState			= kMenuStateFadeIn;
	gNav->menuFadeAlpha		= 0;
	gNav->menuRow			= -1;

		/* LAY OUT MENU COMPONENTS */

	ObjNode* pane = nil;

#if 0
	if (gNav->style.darkenPane)
		pane = MakeDarkenPane();
#endif

	LayOutMenu(menu->id);

		/* SHOW IN ANIMATED LOOP */

	CalcFramesPerSecond();
	ReadKeyboard();

	while (gNav->menuState != kMenuStateOff)
	{
		DoSDLMaintenance();

		gNav->idleTime += gFramesPerSecondFrac;

		if (gNav->style.startButtonExits && gNav->style.canBackOutOfRootMenu && GetNewNeedStateAnyP(kNeed_UIStart))
			gNav->menuState = kMenuStateFadeOut;

		switch (gNav->menuState)
		{
			case kMenuStateFadeIn:
				gNav->menuFadeAlpha += gFramesPerSecondFrac * gNav->style.fadeInSpeed;
				if (gNav->menuFadeAlpha >= 1.0f)
				{
					gNav->menuFadeAlpha = 1.0f;
					gNav->menuState = kMenuStateReady;
				}
				break;

			case kMenuStateFadeOut:
				if (gNav->style.asyncFadeOut)
				{
					gNav->menuState = kMenuStateOff;		// exit loop
				}
				else
				{
					gNav->menuFadeAlpha -= gFramesPerSecondFrac * gNav->style.fadeOutSpeed;
					if (gNav->menuFadeAlpha <= 0.0f)
					{
						gNav->menuFadeAlpha = 0.0f;
						gNav->menuState = kMenuStateOff;
					}
				}
				break;

			case kMenuStateReady:
				if (gNav->style.isInteractive)
				{
					NavigateMenu();
				}
				else if (UserWantsOut())
				{
					GoBackInHistory();
				}
				break;

			case kMenuStateAwaitingKeyPress:
				AwaitKeyPress();
				break;

			case kMenuStateAwaitingPadPress:
			{
				AwaitMetaGamepadPress();
				break;
			}

			case kMenuStateAwaitingMouseClick:
				AwaitMouseClick();
				break;

			default:
				break;
		}

			/* DRAW STUFF */

		CalcFramesPerSecond();

		if (update)
			update();
		else
			MoveObjects();
			
		OGL_DrawScene(gGameViewInfoPtr, draw);
	}


		/* FADE OUT */

	if (gNav->style.fadeOutSceneOnExit)
	{
		OGL_FadeOutScene(gGameViewInfoPtr, draw, NULL);
	}



		/* CLEANUP */

	if (gNav->style.asyncFadeOut)
	{
		if (pane)
		{
			InstallAsyncFadeOut(pane);
		}

		for (int row = 0; row < MAX_MENU_ROWS; row++)
		{
			for (int col = 0; col < MAX_MENU_COLS; col++)
			{
				if (gNav->menuObjects[row][col])
				{
					InstallAsyncFadeOut(gNav->menuObjects[row][col]);
				}
			}
		}

		memset(gNav->menuObjects, 0, sizeof(gNav->menuObjects));
	}
	else
	{
		DeleteAllText();

		if (pane)
			DeleteObject(pane);
	}

	DoSDLMaintenance();
	MyFlushEvents();

	SetStandardMouseCursor();
	SDL_ShowCursor(cursorStateBeforeMenu);

	int finalPick = gNav->menuPick;
	DisposeMenuNavigation();

	return finalPick;
}
