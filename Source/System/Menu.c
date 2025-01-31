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

static ObjNode* LayOutCycler2ValueText(int row);
static ObjNode* LayOutFloatRangeValueText(int row);

static ObjNode* LayOutCycler1(int row);
static ObjNode* LayOutCycler2(int row);
static ObjNode* LayOutPick(int row);
static ObjNode* LayOutLabel(int row);
static ObjNode* LayOutKeyBinding(int row);
static ObjNode* LayOutPadBinding(int row);
static ObjNode* LayOutMouseBinding(int row);
static ObjNode* LayOutFloatRange(int row);

static void NavigateCycler(const MenuItem* entry);
static void NavigatePick(const MenuItem* entry);
static void NavigateKeyBinding(const MenuItem* entry);
static void NavigatePadBinding(const MenuItem* entry);
static void NavigateMouseBinding(const MenuItem* entry);
static void NavigateFloatRange(const MenuItem* entry);

static float GetMenuItemHeight(int row);
static int GetCyclerNumChoices(const MenuItem* entry);
static int GetValueIndexInCycler(const MenuItem* entry, uint8_t value);

typedef struct
{
	Boolean		muted;
	int			row;
	int			col;
	float		pulsateTimer;
	float		incrementCooldown;
	int			nIncrements;
} MenuNodeData;
CheckSpecialDataStruct(MenuNodeData);

#define GetMenuNodeData(node) GetSpecialData(node, MenuNodeData)

/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_MENU_ROWS	25
#define MAX_STACK_LENGTH 16
#define CLEAR_BINDING_SCANCODE SDL_SCANCODE_X

#define kSfxNavigate	EFFECT_SELECTCLICK
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
	.darkenPaneOpacity	= 0,
	.fadeInSpeed		= (1.0f / 0.3f),
	.fadeOutSpeed		= (1.0f / 0.2f),
	.asyncFadeOut		= true,
	.fadeOutSceneOnExit	= true,
	.standardScale		= .5f,
	.rowHeight			= 40,
	.uniformXExtent		= 0,
	.startButtonExits	= false,
	.isInteractive		= true,
	.canBackOutOfRootMenu	= false,
	.textSlot			= MENU_SLOT,
	.yOffset			= 0,

	.highlightColor		= {0.3f, 0.5f, 0.2f, 1.0f},
	.arrowColor			= {0.7f, 0.7f, 0.7f, 1.0f},
	.idleColor			= {1.0f, 1.0f, 1.0f, 1.0f},
	.labelColor			= {0.35f, 0.35f, 0.35f, 1.0f},
};

typedef struct
{
	float height;
	ObjNode* (*layOutCallback)(int row);
	void (*navigateCallback)(const MenuItem* menuItem);
} MenuItemClass;

static const MenuItemClass kMenuItemClasses[kMI_COUNT] =
{
	[kMISENTINEL]		= {0.0f, NULL, NULL },
	[kMILabel]			= {0.5f, LayOutLabel, NULL },
	[kMISpacer]			= {0.5f, NULL, NULL },
	[kMICycler1]		= {1.0f, LayOutCycler1, NavigateCycler },
	[kMICycler2]		= {1.0f, LayOutCycler2, NavigateCycler },
	[kMIFloatRange]		= {0.6f, LayOutFloatRange, NavigateFloatRange },
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
	int					menuCol;

	float				menuRowYs[MAX_MENU_ROWS];
	float				menuFadeAlpha;
	int					menuState;
	int					menuPick;
	ObjNode*			menuObjects[MAX_MENU_ROWS];

	struct
	{
		int menuID;
		int row;
	} history[MAX_STACK_LENGTH];
	int					historyPos;

	bool				mouseHoverValid;
//	int					mouseHoverColumn;
	SDL_Cursor*			handCursor;
	SDL_Cursor*			standardCursor;

	float				idleTime;

	ObjNode*			arrowObjects[2];
	ObjNode*			darkenPane;
} MenuNavigation;

static MenuNavigation* gNav = NULL;

static float gTempInitialSweepFactor = 0;
static bool gTempForceSwipeRTL = false;

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
//	nav->mouseHoverColumn = -1;

	nav->standardCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	nav->handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

	NewObjectDefinitionType arrowDef =
	{
		.scale = 1,
		.slot = nav->style.textSlot,
		.flags = STATUS_BIT_OVERLAYPANE | STATUS_BIT_MOVEINPAUSE,
	};
	nav->arrowObjects[0] = TextMesh_New("<", 0, &arrowDef);
	nav->arrowObjects[1] = TextMesh_New(">", 0, &arrowDef);

	nav->arrowObjects[0]->ColorFilter = nav->style.arrowColor;
	nav->arrowObjects[1]->ColorFilter = nav->style.arrowColor;
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

#if 0
static void SetHandMouseCursor(void)
{
	if (gNav->handCursor != NULL &&
		gNav->handCursor != SDL_GetCursor())
	{
		SDL_SetCursor(gNav->handCursor);
	}
}
#endif

ObjNode* GetCurrentMenuItemObject(void)
{
	if (gNav->menuRow < 0)
		return NULL;

	return gNav->menuObjects[gNav->menuRow];
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

static int GetLayoutFlags(const MenuItem* mi)
{
	if (mi->getLayoutFlags)
		return mi->getLayoutFlags(mi);
	else
		return 0;
}

static OGLColorRGBA PulsateColor(float* time)
{
	*time += gFramesPerSecondFrac;
	float intensity = (1.0f + sinf(*time * 10.0f)) * 0.5f;
	return (OGLColorRGBA) {1,1,1,intensity};
}

static InputBinding* GetBindingAtRow(int row)
{
	return &gGamePrefs.bindings[gNav->menu[row].inputNeed];
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
	const PadBinding* pb = &GetBindingAtRow(row)->pad[col];

	switch (pb->type)
	{
		case kInputTypeUnbound:
			return Localize(STR_UNBOUND_PLACEHOLDER);

		case kInputTypeButton:
			switch (pb->id)
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
					return SDL_GameControllerGetStringForButton(pb->id);
			}
			break;

		case kInputTypeAxisPlus:
			switch (pb->id)
			{
				case SDL_CONTROLLER_AXIS_INVALID:			return Localize(STR_UNBOUND_PLACEHOLDER);
				case SDL_CONTROLLER_AXIS_LEFTX:				return Localize(STR_CONTROLLER_AXIS_LEFTSTICK_RIGHT);
				case SDL_CONTROLLER_AXIS_LEFTY:				return Localize(STR_CONTROLLER_AXIS_LEFTSTICK_DOWN);
				case SDL_CONTROLLER_AXIS_RIGHTX:			return Localize(STR_CONTROLLER_AXIS_RIGHTSTICK_RIGHT);
				case SDL_CONTROLLER_AXIS_RIGHTY:			return Localize(STR_CONTROLLER_AXIS_RIGHTSTICK_DOWN);
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return Localize(STR_CONTROLLER_AXIS_LEFTTRIGGER);
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return Localize(STR_CONTROLLER_AXIS_RIGHTTRIGGER);
				default:
					return SDL_GameControllerGetStringForAxis(pb->id);
			}
			break;

		case kInputTypeAxisMinus:
			switch (pb->id)
			{
				case SDL_CONTROLLER_AXIS_INVALID:			return Localize(STR_UNBOUND_PLACEHOLDER);
				case SDL_CONTROLLER_AXIS_LEFTX:				return Localize(STR_CONTROLLER_AXIS_LEFTSTICK_LEFT);
				case SDL_CONTROLLER_AXIS_LEFTY:				return Localize(STR_CONTROLLER_AXIS_LEFTSTICK_UP);
				case SDL_CONTROLLER_AXIS_RIGHTX:			return Localize(STR_CONTROLLER_AXIS_RIGHTSTICK_LEFT);
				case SDL_CONTROLLER_AXIS_RIGHTY:			return Localize(STR_CONTROLLER_AXIS_RIGHTSTICK_UP);
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return Localize(STR_CONTROLLER_AXIS_LEFTTRIGGER);
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return Localize(STR_CONTROLLER_AXIS_RIGHTTRIGGER);
				default:
					return SDL_GameControllerGetStringForAxis(pb->id);
			}
			break;

		default:
			return "???";
	}
}

static const char* GetMouseBindingName(int row)
{
	DECLARE_STATIC_WORKBUF(buf, bufSize);

	InputBinding* binding = GetBindingAtRow(row);

	switch (binding->mouseButton)
	{
		case 0:							return Localize(STR_UNBOUND_PLACEHOLDER);
		case SDL_BUTTON_LEFT:			return Localize(STR_MOUSE_BUTTON_LEFT);
		case SDL_BUTTON_MIDDLE:			return Localize(STR_MOUSE_BUTTON_MIDDLE);
		case SDL_BUTTON_RIGHT:			return Localize(STR_MOUSE_BUTTON_RIGHT);
		case SDL_BUTTON_WHEELUP:		return Localize(STR_MOUSE_WHEEL_UP);
		case SDL_BUTTON_WHEELDOWN:		return Localize(STR_MOUSE_WHEEL_DOWN);
		default:
			snprintf(buf, bufSize, "%s %d", Localize(STR_BUTTON), binding->mouseButton);
			return buf;
	}
}

static ObjNode* MakeKbText(int row, int keyNo)
{
	const char* kbName;

	if (gNav->menuState == kMenuStateAwaitingKeyPress)
	{
		kbName = Localize(STR_PRESS);
	}
	else
	{
		kbName = GetKeyBindingName(row, keyNo);
	}

	ObjNode* node = MakeText(kbName, row, 1+keyNo, kTextMeshAllCaps | kTextMeshAlignLeft);
	GetMenuNodeData(node)->col = keyNo;
	return node;
}

static ObjNode* MakePbText(int row, int btnNo)
{
	const char* pbName;

	if (gNav->menuState == kMenuStateAwaitingPadPress)
	{
		pbName = Localize(STR_PRESS);
	}
	else
	{
		pbName = GetPadBindingName(row, btnNo);
	}

	ObjNode* node = MakeText(pbName, row, 1+btnNo, kTextMeshAllCaps | kTextMeshAlignLeft);
	GetMenuNodeData(node)->col = btnNo;
	return node;
}

static bool IsMenuItemSelectable(const MenuItem* mi)
{
	switch (mi->type)
	{
		case kMISpacer:
		case kMILabel:
			return false;

		default:
			return !(GetLayoutFlags(mi) & (kMILayoutFlagHidden | kMILayoutFlagDisabled));
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

static void TwitchSelectionInOrOut(bool scaleIn)
{
	ObjNode* obj = GetCurrentMenuItemObject();

	switch (gNav->menu[gNav->menuRow].type)
	{
		case kMIKeyBinding:
		case kMIPadBinding:
		case kMIMouseBinding:
			obj = GetNthChainedNode(obj, 1+gNav->menuCol, NULL);
			break;

		case kMIFloatRange:
			obj = GetNthChainedNode(obj, 1, NULL);
			break;

		default:
			break;
	}

	if (!obj)
		return;


	float s = GetMenuItemHeight(gNav->menuRow) * gNav->style.standardScale;
	obj->Scale.x = s * (scaleIn? 1.15: 1);
	obj->Scale.y = s * (scaleIn? 1.15: 1);

	MakeTwitch(obj, (scaleIn ? kTwitchPreset_MenuSelect : kTwitchPreset_MenuDeselect) | kTwitchFlags_Chain);
}

static void TwitchSelection(void)
{
	TwitchSelectionInOrOut(true);
}

static void TwitchOutSelection(void)
{
	TwitchSelectionInOrOut(false);
}

/****************************/
/*    DARKEN PANE           */
/****************************/
#pragma mark - Move calls

static void MoveDarkenPane(ObjNode* node)
{
	node->Scale.x = gGameView->panes[GetOverlayPaneNumber()].logicalWidth * (1.0f / 4.0f);
	UpdateObjectTransforms(node);
}

static void RescaleDarkenPane(ObjNode* node, float totalHeight)
{
	float prevScale = node->Scale.y;
	float newScale = 1.3f * totalHeight / GetSpriteInfo(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_OverlayBackground)->yadv;

	node->Coord.y = gNav->style.yOffset;
	node->Scale.y = newScale;
	UpdateObjectTransforms(gNav->darkenPane);

	Twitch* scaleEffect = MakeTwitch(gNav->darkenPane, kTwitchPreset_MenuDarkenPaneResize);
	if (scaleEffect)
	{
		scaleEffect->amplitude = prevScale/newScale;
	}
}

static ObjNode* MakeDarkenPane(void)
{
	ObjNode* pane;

	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.flags = STATUS_BITS_FOR_2D | STATUS_BIT_MOVEINPAUSE | STATUS_BIT_OVERLAYPANE,
		.slot = gNav->style.textSlot-1,
		.scale = EPS,
		.group = SPRITE_GROUP_INFOBAR,
		.type = INFOBAR_SObjType_OverlayBackground,
		.coord = {0, gNav->style.yOffset, 0},
		.moveCall = MoveDarkenPane,
	};

	pane = MakeSpriteObject(&def);
	pane->ColorFilter = (OGLColorRGBA) {0, 0, 0, gNav->style.darkenPaneOpacity};

	return pane;
}

/****************************/
/*    MENU MOVE CALLS       */
/****************************/
#pragma mark - Move calls

static void MoveGenericMenuItem(ObjNode* node, float baseAlpha)
{
	MenuNodeData* data = GetMenuNodeData(node);

	if (data->muted)
	{
		baseAlpha *= .5f;
	}

	node->ColorFilter.a = baseAlpha;

	// Don't mix gNav->menuFadeAlpha -- it's only useful when fading OUT,
	// in which case we're using a different move call for all menu items
}

static void MoveLabel(ObjNode* node)
{
	MoveGenericMenuItem(node, 1);
}

static void MoveAction(ObjNode* node)
{
	if (GetMenuNodeData(node)->row == gNav->menuRow)
		node->ColorFilter = gNav->style.highlightColor; //TwinkleColor();
	else
		node->ColorFilter = gNav->style.idleColor;

	MoveGenericMenuItem(node, 1);
}

static void MoveControlBinding(ObjNode* node)
{
	MenuNodeData* data = GetMenuNodeData(node);

	int pulsatingState = 0;
	switch (gNav->menu[data->row].type)
	{
		case kMIKeyBinding:
			pulsatingState = kMenuStateAwaitingKeyPress;
			break;

		case kMIPadBinding:
			pulsatingState = kMenuStateAwaitingPadPress;
			break;

		case kMIMouseBinding:
			pulsatingState = kMenuStateAwaitingMouseClick;
			break;

		default:
			DoFatalAlert("MoveControlBinding: unknown MI type");
	}

	if (data->row == gNav->menuRow && data->col == gNav->menuCol)
	{
		if (gNav->menuState == pulsatingState)
		{
			node->ColorFilter = PulsateColor(&data->pulsateTimer);
		}
		else
		{
			node->ColorFilter = gNav->style.highlightColor; //TwinkleColor();
		}
	}
	else
	{
		node->ColorFilter = gNav->style.idleColor;
	}

	MoveGenericMenuItem(node, node->ColorFilter.a);
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

		gTempForceSwipeRTL = true;
		LayOutMenu(gNav->history[gNav->historyPos].menuID);
		gTempForceSwipeRTL = false;
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
	ObjNode* snapTo = NULL;
	ObjNode* chainRoot = GetCurrentMenuItemObject();

	bool visible[2] = {false, false};

	const MenuItem* entry = &gNav->menu[gNav->menuRow];
	switch (entry->type)
	{
		case kMICycler1:
		{
			snapTo = chainRoot;

			int index = GetValueIndexInCycler(entry, *entry->cycler.valuePtr);

			visible[0] = (index != 0);
			visible[1] = (index != GetCyclerNumChoices(entry)-1);
			break;
		}

		case kMICycler2:
		case kMIFloatRange:
			visible[0] = true;
			visible[1] = true;
			snapTo = GetNthChainedNode(chainRoot, 1, NULL);
			break;

		case kMIKeyBinding:
		case kMIPadBinding:
		case kMIMouseBinding:
			visible[0] = true;
			visible[1] = true;
			switch (gNav->menuState)
			{
				case kMenuStateAwaitingKeyPress:
				case kMenuStateAwaitingPadPress:
				case kMenuStateAwaitingMouseClick:
					break;
				default:
					snapTo = GetNthChainedNode(chainRoot, 1 + gNav->menuCol, NULL);
			}
			break;

		default:
			snapTo = NULL;
			break;
	}

	if (snapTo)
	{
		OGLRect extents = TextMesh_GetExtents(snapTo);

		float spacing = 45 * snapTo->Scale.x;

		for (int i = 0; i < 2; i++)
		{
			ObjNode* arrowObj = gNav->arrowObjects[i];
			SetObjectVisible(arrowObj, true);

			arrowObj->Coord.x = i==0? extents.left - spacing: extents.right + spacing;
			arrowObj->Coord.y = snapTo->Coord.y;
			arrowObj->Scale = snapTo->Scale;
			UpdateObjectTransforms(arrowObj);

			arrowObj->ColorFilter.a = visible[i] ? 1 : 0;//0.25f;
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

	TwitchOutSelection();

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
//	gNav->mouseHoverValid = false;

	if (makeSound)
	{
		PlayEffect(kSfxNavigate);
		TwitchSelection();
	}

	// Reposition arrows
	RepositionArrows();
}

#if 0
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
#endif

static void NavigatePick(const MenuItem* entry)
{
	if (GetNewNeedStateAnyP(kNeed_UIConfirm)
//			|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT))
			)
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
				TwitchSelection();
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
	if (entry->cycler.isDynamicallyGenerated)
	{
		return entry->cycler.generator.generateNumChoices();
	}

	for (int i = 0; i < MAX_MENU_CYCLER_CHOICES; i++)
	{
		if (entry->cycler.choices[i].text == STR_NULL)
			return i;
	}

	return MAX_MENU_CYCLER_CHOICES;
}

static int GetValueIndexInCycler(const MenuItem* entry, uint8_t value)
{
	if (entry->cycler.isDynamicallyGenerated)
	{
		return value;
	}

	for (int i = 0; i < MAX_MENU_CYCLER_CHOICES; i++)
	{
		if (entry->cycler.choices[i].text == STR_NULL)
			break;

		if (entry->cycler.choices[i].value == value)
			return i;
	}

	return -1;
}

static Byte GetCyclerValueForIndex(const MenuItem* entry, int index)
{
	if (entry->cycler.isDynamicallyGenerated)
	{
		return index;
	}
	else
	{
		return entry->cycler.choices[index].value;
	}
}

static void NavigateCycler(const MenuItem* entry)
{
	int delta = 0;

	if (GetNewNeedStateAnyP(kNeed_UILeft)
		|| GetNewNeedStateAnyP(kNeed_UIPrev)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_RIGHT))
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_WHEELDOWN))
		)
	{
		delta = -1;
	}
	else if (GetNewNeedStateAnyP(kNeed_UIRight)
		|| GetNewNeedStateAnyP(kNeed_UINext)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT))
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_WHEELUP))
		)
	{
		delta = 1;
	}
	else if (GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		MakeTwitch(GetCurrentMenuItemObject(), kTwitchPreset_MenuSelect);
		MakeTwitch(gNav->arrowObjects[0], kTwitchPreset_PadlockWiggle);
		MakeTwitch(gNav->arrowObjects[1], kTwitchPreset_PadlockWiggle);
		PlayEffect(EFFECT_BADSELECT);
		return;
	}

	if (delta != 0)
	{
		gNav->idleTime = 0;

		if (entry->cycler.valuePtr)
		{
			int index = GetValueIndexInCycler(entry, *entry->cycler.valuePtr);

			if ((index == 0 && delta < 0) ||
				(index == GetCyclerNumChoices(entry)-1 && delta > 0))
			{
				PlayEffect(EFFECT_BADSELECT);
				MakeTwitch(GetCurrentMenuItemObject(), kTwitchPreset_MenuWiggle);
				MakeTwitch(gNav->arrowObjects[0], kTwitchPreset_PadlockWiggle);
				MakeTwitch(gNav->arrowObjects[1], kTwitchPreset_PadlockWiggle);
				return;
			}

			if (index >= 0)
				index = PositiveModulo(index + delta, GetCyclerNumChoices(entry));
			else
				index = 0;

			*entry->cycler.valuePtr = GetCyclerValueForIndex(entry, index);
			PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE /2 + 4096 * index);
		}
		else
		{
			PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3 + (RandomFloat2() * 0x3000));
		}

		if (entry->callback)
		{
			entry->callback(entry);
		}

		gTempForceSwipeRTL = (delta == -1);

		if (entry->type == kMICycler1)
			LayOutCycler1(gNav->menuRow);
		else
			LayOutCycler2ValueText(gNav->menuRow);

		RepositionArrows();

		if (delta < 0)
			MakeTwitch(gNav->arrowObjects[0], kTwitchPreset_DisplaceLTR);
		else
			MakeTwitch(gNav->arrowObjects[1], kTwitchPreset_DisplaceRTL);
	}
}

static void NavigateFloatRange(const MenuItem* entry)
{
enum
{
	kSaneIncrement = 0,
	kInsaneIncrement = 20,
	kSuperInsaneIncrement = 59,
};

	int delta = 0;

	if (GetNeedStateAnyP(kNeed_UILeft)
		|| GetNeedStateAnyP(kNeed_UIPrev)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_RIGHT)))
	{
		delta = -1;
	}
	else if (GetNeedStateAnyP(kNeed_UIRight)
		|| GetNeedStateAnyP(kNeed_UINext)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT)))
	{
		delta = 1;
	}
	else if (GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		MakeTwitch(GetCurrentMenuItemObject(), kTwitchPreset_MenuSelect);
		MakeTwitch(gNav->arrowObjects[0], kTwitchPreset_PadlockWiggle);
		MakeTwitch(gNav->arrowObjects[1], kTwitchPreset_PadlockWiggle);
		PlayEffect(EFFECT_BADSELECT);
		return;
	}

	MenuNodeData* data = GetMenuNodeData(GetCurrentMenuItemObject());

	if (delta == 0)
	{
		data->incrementCooldown = 0;
		data->nIncrements = 0;
	}
	else
	{
		gNav->idleTime = 0;

		data->incrementCooldown -= gFramesPerSecondFrac;

		if (data->incrementCooldown > 0)
		{
			// Wait for cooldown to go negative to increment/decrement value
			return;
		}

		float deltaFrac = delta * entry->floatRange.incrementFrac;

		if (data->nIncrements > kSuperInsaneIncrement)
			deltaFrac *= 20;

		float valueFrac = *entry->floatRange.valuePtr / *entry->floatRange.equilibriumPtr;
		valueFrac += deltaFrac;

		// If possible, round fraction to 0% or 100%, to avoid floating-point drift
		// when the user sets values back to 0% or 100% manually.
		if (fabsf(valueFrac - 1.0f) < 0.001f)		// Round to 100%
			valueFrac = 1.0f;
		else if (fabsf(valueFrac) < 0.001f)			// Round to 0%
			valueFrac = 0.0f;

		float newValue = valueFrac * *entry->floatRange.equilibriumPtr;

		// Set value
		*entry->floatRange.valuePtr = newValue;

		// Invoke callback
		if (entry->callback)
			entry->callback(entry);


		// Play sound
		float pitch = 0.5f;
		pitch += GAME_MIN(6, data->nIncrements) * (1.0f/6.0f) * 0.5f;
		pitch += RandomFloat() * 0.2f;
		PlayEffect_Parms(EFFECT_MINE, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * pitch);

		// Prepare to lay out new value text
		gTempInitialSweepFactor = .5f;
		gTempForceSwipeRTL = delta > 0;

		// Lay out new value text
		LayOutFloatRangeValueText(gNav->menuRow);

		// Restore sweep params
		gTempInitialSweepFactor = 0;
		gTempForceSwipeRTL = false;

		// Adjust arrows
		RepositionArrows();
		if (delta < 0)
			MakeTwitch(gNav->arrowObjects[0], kTwitchPreset_DisplaceLTR);
		else
			MakeTwitch(gNav->arrowObjects[1], kTwitchPreset_DisplaceRTL);

		// Update cooldown timer
		if (valueFrac == 1.0f || valueFrac == 0.0f)		// Force speed bump at 0% and 100%
		{
			data->incrementCooldown = .3f;
			data->nIncrements = 0;
		}
		else
		{
			const float kCooldownProgression[] = {.4f, .3f, .25f, .2f, .175f, .15f, .125f, .1f};
			const int nCooldowns = sizeof(kCooldownProgression) / sizeof(kCooldownProgression[0]);

			data->incrementCooldown = kCooldownProgression[ GAME_MIN(data->nIncrements, nCooldowns-1) ];

			if (data->nIncrements > kSuperInsaneIncrement)
				data->incrementCooldown = 1/30.0f;
			else if (data->nIncrements > kInsaneIncrement)
				data->incrementCooldown = 1/15.0f;

			data->nIncrements++;
		}
	}
}

static void NavigateKeyBinding(const MenuItem* entry)
{
	gNav->menuCol = PositiveModulo(gNav->menuCol, MAX_USER_BINDINGS_PER_NEED);
	int keyNo = gNav->menuCol;
	int row = gNav->menuRow;

#if 0 // TODO: review this if we ever bring back mouse support in menus
	if (gNav->mouseHoverValid && (gNav->mouseHoverColumn == 1 || gNav->mouseHoverColumn == 2))
		gNav->keyColumn = gNav->mouseHoverColumn - 1;
#endif

	if (GetNewNeedStateAnyP(kNeed_UILeft) || GetNewNeedStateAnyP(kNeed_UIPrev))
	{
		TwitchOutSelection();
		keyNo = PositiveModulo(keyNo - 1, MAX_USER_BINDINGS_PER_NEED);
		gNav->idleTime = 0;
		gNav->menuCol = keyNo;
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValid = false;
		TwitchSelection();
		RepositionArrows();
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIRight) || GetNewNeedStateAnyP(kNeed_UINext))
	{
		TwitchOutSelection();
		keyNo = PositiveModulo(keyNo + 1, MAX_USER_BINDINGS_PER_NEED);
		gNav->idleTime = 0;
		gNav->menuCol = keyNo;
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValid = false;
		TwitchSelection();
		RepositionArrows();
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| GetNewKeyState(CLEAR_BINDING_SCANCODE)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_MIDDLE)))
	{
		TwitchOutSelection();
		gNav->idleTime = 0;
		gGamePrefs.bindings[entry->inputNeed].key[keyNo] = 0;
		PlayEffect(kSfxDelete);
		MakeKbText(row, keyNo);
		TwitchSelection();
		RepositionArrows();
		return;
	}

	if (GetNewKeyState(SDL_SCANCODE_RETURN)
		|| GetNewKeyState(SDL_SCANCODE_KP_ENTER)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT)))
	{
		PlayEffect(EFFECT_MINE);
		InvalidateAllInputs();
		gNav->idleTime = 0;
		gNav->menuState = kMenuStateAwaitingKeyPress;

		MakeKbText(row, keyNo);		// It'll show "PRESS!" because we're in kMenuStateAwaitingKeyPress
		TwitchSelection();
		RepositionArrows();

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP_CANCEL);
		return;
	}
}

static void NavigatePadBinding(const MenuItem* entry)
{
	gNav->menuCol = PositiveModulo(gNav->menuCol, MAX_USER_BINDINGS_PER_NEED);
	int btnNo = gNav->menuCol;
	int row = gNav->menuRow;

#if 0 // TODO: review this if we ever bring back mouse support in menus
	if (gNav->mouseHoverValid && (gNav->mouseHoverColumn == 1 || gNav->mouseHoverColumn == 2))
		gNav->padColumn = gNav->mouseHoverColumn - 1;
#endif

	if (GetNewNeedStateAnyP(kNeed_UILeft) || GetNewNeedStateAnyP(kNeed_UIPrev))
	{
		TwitchOutSelection();
		btnNo = PositiveModulo(btnNo - 1, MAX_USER_BINDINGS_PER_NEED);
		gNav->menuCol = btnNo;
		gNav->idleTime = 0;
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValid = false;
		TwitchSelection();
		RepositionArrows();
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIRight) || GetNewNeedStateAnyP(kNeed_UINext))
	{
		TwitchOutSelection();
		btnNo = PositiveModulo(btnNo + 1, MAX_USER_BINDINGS_PER_NEED);
		gNav->menuCol = btnNo;
		gNav->idleTime = 0;
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValid = false;
		TwitchSelection();
		RepositionArrows();
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| GetNewKeyState(CLEAR_BINDING_SCANCODE)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_MIDDLE)))
	{
		TwitchOutSelection();
		gNav->idleTime = 0;
		gGamePrefs.bindings[entry->inputNeed].pad[btnNo].type = kInputTypeUnbound;
		PlayEffect(kSfxDelete);
		MakePbText(row, btnNo);
		TwitchSelection();
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| GetNewKeyState(SDL_SCANCODE_KP_ENTER)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_LEFT)))
	{
		// Unlike keyboard bindings, don't call InvalidateAllInputs() here,
		// because we don't keep a shadow array of all gamepad button states,
		// so we have to query SDL for a new button directly.
		// AwaitMetaGamepadPress will wait for the user to let go of kNeed_UIConfirm.

		PlayEffect(EFFECT_MINE);

		gNav->idleTime = 0;
		gNav->menuState = kMenuStateAwaitingPadPress;

		MakePbText(row, btnNo);			// It'll show "PRESS!" because we're in MenuStateAwaitingPadPress
		RepositionArrows();
		TwitchSelection();

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP_CANCEL);

		return;
	}
}

static void NavigateMouseBinding(const MenuItem* entry)
{
#if 0 // TODO: review this if we bring back mouse input support
	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| (gNav->mouseHoverValid && GetNewClickState(SDL_BUTTON_MIDDLE)))
	{
		gNav->idleTime = 0;
		gGamePrefs.bindings[entry->inputNeed].mouseButton = 0;
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
#endif
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

		InputBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < MAX_USER_BINDINGS_PER_NEED; j++)
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

		InputBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < MAX_USER_BINDINGS_PER_NEED; j++)
		{
			if (binding->pad[j].type == type && binding->pad[j].id == id)
			{
				binding->pad[j].type = kInputTypeUnbound;
				binding->pad[j].id = 0;
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

		InputBinding* binding = GetBindingAtRow(row);

		if (binding->mouseButton == id)
		{
			binding->mouseButton = 0;
			MakeText(Localize(STR_UNBOUND_PLACEHOLDER), row, 1, kTextMeshAllCaps | kTextMeshAlignLeft);
		}
	}
}

static void AwaitKeyPress(void)
{
	int row = gNav->menuRow;
	int keyNo = gNav->menuCol;
	GAME_ASSERT(keyNo >= 0);
	GAME_ASSERT(keyNo < MAX_USER_BINDINGS_PER_NEED);

	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		PlayEffect(kSfxError);
		goto updateText;
	}

	for (int16_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
	{
		if (GetNewKeyState(scancode))
		{
			UnbindScancodeFromAllRemappableInputNeeds(scancode);
			GetBindingAtRow(row)->key[keyNo] = scancode;

			PlayEffect(kSfxCycle);
			goto updateText;
		}
	}
	return;

updateText:
	gNav->menuState = kMenuStateReady;
	gNav->idleTime = 0;
	MakeKbText(row, keyNo);		// update text after state changed back to Ready
	RepositionArrows();
	ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
}

static bool AwaitGamepadPress(SDL_GameController* controller)
{
	int row = gNav->menuRow;
	int btnNo = gNav->menuCol;
	GAME_ASSERT(btnNo >= 0);
	GAME_ASSERT(btnNo < MAX_USER_BINDINGS_PER_NEED);

	if (GetNewKeyState(SDL_SCANCODE_ESCAPE)
		|| SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))
	{
		PlayEffect(kSfxError);
		goto updateText;
	}

	InputBinding* binding = GetBindingAtRow(gNav->menuRow);

	for (int8_t button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
	{
		if (SDL_GameControllerGetButton(controller, button))
		{
			PlayEffect(kSfxCycle);
			UnbindPadButtonFromAllRemappableInputNeeds(kInputTypeButton, button);
			binding->pad[btnNo].type = kInputTypeButton;
			binding->pad[btnNo].id = button;
			goto updateText;
		}
	}

	for (int8_t axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
	{
		int16_t axisValue = SDL_GameControllerGetAxis(controller, axis);
		if (abs(axisValue) > kJoystickDeadZone_BindingThreshold)
		{
			PlayEffect(kSfxCycle);
			int axisType = axisValue < 0 ? kInputTypeAxisMinus : kInputTypeAxisPlus;
			UnbindPadButtonFromAllRemappableInputNeeds(axisType, axis);
			binding->pad[btnNo].type = axisType;
			binding->pad[btnNo].id = axis;
			goto updateText;
		}
	}

	return false;

updateText:
	gNav->menuState = kMenuStateReady;
	gNav->idleTime = 0;
	MakePbText(row, btnNo);		// update text after state changed back to Ready
	RepositionArrows();
	ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP_CANCEL, STR_CONFIGURE_GAMEPAD_HELP);
	return true;
}

static void AwaitMetaGamepadPress(void)
{
	// Wait for user to let go of confirm button before accepting a new button binding.
	if (GetNeedStateAnyP(kNeed_UIConfirm) && !GetNewNeedStateAnyP(kNeed_UIConfirm))
	{
		return;
	}

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
		gNav->menuState = kMenuStateReady;
		gNav->idleTime = 0;
		MakePbText(gNav->menuRow, gNav->menuCol);	// update text after state changed back to Ready
		RepositionArrows();
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_NO_GAMEPAD_DETECTED);
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

	InputBinding* binding = GetBindingAtRow(gNav->menuRow);

	for (int8_t mouseButton = 0; mouseButton < NUM_SUPPORTED_MOUSE_BUTTONS; mouseButton++)
	{
		if (GetNewClickState(mouseButton))
		{
			UnbindMouseButtonFromAllRemappableInputNeeds(mouseButton);
			binding->mouseButton = mouseButton;
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

static void DeleteAllText(void)
{
	for (int row = 0; row < MAX_MENU_ROWS; row++)
	{
		if (gNav->menuObjects[row])
		{
			DeleteObject(gNav->menuObjects[row]);
			gNav->menuObjects[row] = nil;
		}
	}
}

static float GetMenuItemHeight(int row)
{
	const MenuItem* menuItem = &gNav->menu[row];

	if (GetLayoutFlags(menuItem) & kMILayoutFlagHidden)
		return 0;
	else if (menuItem->customHeight > 0)
		return menuItem->customHeight;
	else
		return kMenuItemClasses[menuItem->type].height;
}

static ObjNode* MakeText(const char* text, int row, int desiredCol, int textMeshFlags)
{
	ObjNode* chainHead = gNav->menuObjects[row];
	ObjNode* pNode = NULL;
	GAME_ASSERT(GetNodeChainLength(chainHead) >= desiredCol);

	ObjNode* node = GetNthChainedNode(chainHead, desiredCol, &pNode);

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
			.slot = gNav->style.textSlot + desiredCol,  // chained node must be after their parent!
			.flags = STATUS_BIT_MOVEINPAUSE | STATUS_BIT_OVERLAYPANE,
		};

		node = TextMesh_New(text, textMeshFlags, &def);

		if (pNode)
		{
			pNode->ChainHead = chainHead;
			pNode->ChainNode = node;
		}
		else
		{
			gNav->menuObjects[row] = node;
		}
	}

	if (gNav->style.uniformXExtent)
	{
		if (-gNav->style.uniformXExtent < node->LeftOff)
			node->LeftOff = -gNav->style.uniformXExtent;
		if (gNav->style.uniformXExtent > node->RightOff)
			node->RightOff = gNav->style.uniformXExtent;
	}

	MenuNodeData* data = GetMenuNodeData(node);
	data->row = row;
	data->col = desiredCol;

	Twitch* fadeEffect = MakeTwitch(node, kTwitchPreset_MenuFadeIn);
	if (fadeEffect)
	{
		fadeEffect->delay = -gTempInitialSweepFactor * fadeEffect->duration;

		Twitch* displaceEffect = MakeTwitch(node, kTwitchPreset_MenuSweep | kTwitchFlags_Chain);
		if (displaceEffect)
		{
			displaceEffect->delay = fadeEffect->delay;
			displaceEffect->amplitude *= (gTempForceSwipeRTL ? 1 : -1);
			displaceEffect->amplitude *= (1.0 + displaceEffect->delay);
		}
	}

	return node;
}

static const char* GetMenuItemLabel(const MenuItem* entry)
{
	return Localize(entry->text);
}

static const char* GetCyclerValueText(int row)
{
	const MenuItem* entry = &gNav->menu[row];

	if (entry->cycler.isDynamicallyGenerated)
	{
		return entry->cycler.generator.generateChoiceString(*entry->cycler.valuePtr);
	}

	int index = GetValueIndexInCycler(entry, *entry->cycler.valuePtr);
	if (index >= 0)
		return Localize(entry->cycler.choices[index].text);
	return "VALUE NOT FOUND???";
}

static ObjNode* LayOutLabel(int row)
{
	const MenuItem* entry = &gNav->menu[row];

	ObjNode* label = MakeText(GetMenuItemLabel(entry), row, 0, 0);
	label->ColorFilter = gNav->style.labelColor;
	label->MoveCall = MoveLabel;

	return label;
}

static ObjNode* LayOutPick(int row)
{
	const MenuItem* entry = &gNav->menu[row];

	ObjNode* obj = MakeText(GetMenuItemLabel(entry), row, 0, 0);
	obj->MoveCall = MoveAction;

	MenuNodeData* data = GetSpecialData(obj, MenuNodeData);
	data->muted = !IsMenuItemSelectable(entry);

	return obj;
}

static ObjNode* LayOutCycler2ValueText(int row)
{
	ObjNode* node2 = MakeText(GetCyclerValueText(row), row, 1, 0);
	node2->MoveCall = MoveAction;
	return node2;
}

static ObjNode* LayOutCycler2(int row)
{
	DECLARE_WORKBUF(buf, bufSize);

	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", GetMenuItemLabel(entry));

	ObjNode* node1 = MakeText(buf, row, 0, kTextMeshAlignLeft);
	node1->Coord.x -= 120;
	node1->MoveCall = MoveAction;

	ObjNode* node2 = LayOutCycler2ValueText(row);
	node2->Coord.x += 100;

	return node1;
}

static ObjNode* LayOutCycler1(int row)
{
	DECLARE_WORKBUF(buf, bufSize);

	const MenuItem* entry = &gNav->menu[row];

	if (entry->text == STR_NULL)
		snprintf(buf, bufSize, "%s", GetCyclerValueText(row));
	else
		snprintf(buf, bufSize, "%s: %s", GetMenuItemLabel(entry), GetCyclerValueText(row));

	ObjNode* node = MakeText(buf, row, 0, 0);
	node->MoveCall = MoveAction;

	return node;
}

static ObjNode* LayOutFloatRangeValueText(int row)
{
	const MenuItem* entry = &gNav->menu[row];
	DECLARE_WORKBUF(buf, bufSize);

	float percent = *entry->floatRange.valuePtr / *entry->floatRange.equilibriumPtr;
	percent *= 100.0f;

	snprintf(buf, bufSize, "%d%%", (int)roundf(percent));
	ObjNode* node2 = MakeText(buf, row, 1, kTextMeshAlignRight);
	node2->MoveCall = MoveAction;
	return node2;
}

static ObjNode* LayOutFloatRange(int row)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", GetMenuItemLabel(entry));
	ObjNode* node1 = MakeText(buf, row, 0, kTextMeshAlignLeft);
	node1->MoveCall = MoveAction;
	node1->Coord.x -= entry->floatRange.xSpread;

	ObjNode* node2 = LayOutFloatRangeValueText(row);
	node2->Coord.x += entry->floatRange.xSpread;
	UpdateObjectTransforms(node2);

	return node1;
}

static ObjNode* LayOutKeyBinding(int row)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->inputNeed));

	ObjNode* label = MakeText(buf, row, 0, kTextMeshAlignLeft);
	label->Coord.x -= 256;
	label->ColorFilter = gNav->style.labelColor;
	label->MoveCall = MoveLabel;

	for (int j = 0; j < MAX_USER_BINDINGS_PER_NEED; j++)
	{
		ObjNode* keyNode = MakeKbText(row, j);
		keyNode->Coord.x = -50 + j * 170 ;
		keyNode->MoveCall = MoveControlBinding;
		GetMenuNodeData(keyNode)->col = j;
	}

	return label;
}

static ObjNode* LayOutPadBinding(int row)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->inputNeed));

	ObjNode* label = MakeText(buf, row, 0, kTextMeshAlignLeft);
	label->Coord.x -= 256;
	label->ColorFilter = gNav->style.labelColor;
	label->MoveCall = MoveLabel;

	for (int j = 0; j < MAX_USER_BINDINGS_PER_NEED; j++)
	{
		ObjNode* keyNode = MakePbText(row, j);
		keyNode->Coord.x = -50 + j * 170;
		keyNode->MoveCall = MoveControlBinding;
		GetMenuNodeData(keyNode)->col = j;
	}

	return label;
}

static ObjNode* LayOutMouseBinding(int row)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->inputNeed));

	ObjNode* label = MakeText(buf, row, 0, 0);
	label->ColorFilter = gNav->style.labelColor;
	label->MoveCall = MoveAction;

	ObjNode* keyNode = MakeText(GetMouseBindingName(row), row, 1, 0);
	keyNode->MoveCall = MoveControlBinding;

	return label;
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
	float firstHeight = 0;
	for (int row = 0; menu[row].type != kMISENTINEL; row++)
	{
		float height = GetMenuItemHeight(row);
		if (firstHeight == 0)
			firstHeight = height;
		totalHeight += height * gNav->style.rowHeight;
	}

	float y = -totalHeight*.5f + gNav->style.yOffset;
	y += firstHeight * gNav->style.rowHeight / 2.0f;

	// Fit darken pane to new menu height
	if (gNav->darkenPane)
	{
		RescaleDarkenPane(gNav->darkenPane, totalHeight);
	}

	gTempInitialSweepFactor = 0.0f;

	for (int row = 0; menu[row].type != kMISENTINEL; row++)
	{
		gNav->menuRowYs[row] = y;

		const MenuItem* entry = &menu[row];
		const MenuItemClass* cls = &kMenuItemClasses[entry->type];

		if (GetLayoutFlags(entry) & kMILayoutFlagHidden)
		{
			gNav->numMenuEntries++;
			continue;
		}

		if (cls->layOutCallback)
		{
			cls->layOutCallback(row);
		}

		y += GetMenuItemHeight(row) * gNav->style.rowHeight;

		if (entry->type != kMISpacer)
		{
			gTempInitialSweepFactor -= .2f;
		}

		gNav->numMenuEntries++;
		GAME_ASSERT(gNav->numMenuEntries < MAX_MENU_ROWS);
	}

	gTempInitialSweepFactor = 0.0f;

	if (gNav->menuRow <= 0
		|| !IsMenuItemSelectable(&gNav->menu[gNav->menuRow]))	// we had selected this item when we last were in this menu, but it's been disabled since then
	{
		// Scroll down to first pickable entry
		gNav->menuRow = -1;
		NavigateSettingEntriesVertically(1);
	}

	TwitchSelection();
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

// TODO: harmonize order of update/draw params with rest of game
int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*update)(void),
		void (*draw)(void))
{
//	int cursorStateBeforeMenu = SDL_ShowCursor(-1);
//	SDL_ShowCursor(1);

		/* INITIALIZE MENU STATE */

	gNumMenusRegistered = 0;
	RegisterMenu(menu);

	InitMenuNavigation();
	if (style)
		memcpy(&gNav->style, style, sizeof(*style));
	gNav->menuState			= kMenuStateFadeIn;
	gNav->menuFadeAlpha		= 0;
	gNav->menuRow			= -1;

	gTempInitialSweepFactor	= 0;
	gTempForceSwipeRTL		= false;

		/* LAY OUT MENU COMPONENTS */

	gNav->darkenPane = nil;
	if (gNav->style.darkenPaneOpacity > 0.0f)
	{
		gNav->darkenPane = MakeDarkenPane();
	}

	LayOutMenu(menu->id);

		/* SHOW IN ANIMATED LOOP */

	CalcFramesPerSecond();
	ReadKeyboard();

	while (gNav->menuState != kMenuStateOff)
	{
		DoSDLMaintenance();

		gNav->idleTime += gFramesPerSecondFrac;

		if (GetNewNeedStateAnyP(kNeed_UIStart)
			&& gNav->style.startButtonExits
			&& gNav->style.canBackOutOfRootMenu
			&& gNav->menuState != kMenuStateAwaitingPadPress
			&& gNav->menuState != kMenuStateAwaitingKeyPress
			&& gNav->menuState != kMenuStateAwaitingMouseClick
			)
		{
			gNav->menuState = kMenuStateFadeOut;
		}

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

		OGL_DrawScene(draw);
	}


		/* FADE OUT */

	if (gNav->style.fadeOutSceneOnExit)
	{
		OGL_FadeOutScene(draw, NULL);
	}



		/* CLEANUP */

	if (gNav->style.asyncFadeOut)
	{
		if (gNav->darkenPane)
		{
			gNav->darkenPane->MoveCall = nil;
			MakeTwitch(gNav->darkenPane, kTwitchPreset_MenuFadeOut | kTwitchFlags_KillPuppet);
			MakeTwitch(gNav->darkenPane, kTwitchPreset_MenuDarkenPaneVanish | kTwitchFlags_Chain);
		}

		for (int row = 0; row < MAX_MENU_ROWS; row++)
		{
			if (gNav->menuObjects[row])
			{
				gNav->menuObjects[row]->MoveCall = nil;
				MakeTwitch(gNav->menuObjects[row], kTwitchPreset_MenuFadeOut | kTwitchFlags_KillPuppet);
			}
		}

		memset(gNav->menuObjects, 0, sizeof(gNav->menuObjects));
	}
	else
	{
		DeleteAllText();

		if (gNav->darkenPane)
		{
			DeleteObject(gNav->darkenPane);
			gNav->darkenPane = nil;
		}
	}

	DoSDLMaintenance();
	MyFlushEvents();

	SetStandardMouseCursor();
//	SDL_ShowCursor(cursorStateBeforeMenu);

	int finalPick = gNav->menuPick;
	DisposeMenuNavigation();

	return finalPick;
}
