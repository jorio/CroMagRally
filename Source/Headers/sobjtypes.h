//
// sobjtypes.h
//


enum
{
	SPRITE_GROUP_FONT				=	0,
	SPRITE_GROUP_MAINMENU			=	1,
	SPRITE_GROUP_INFOBAR			=	2,
	SPRITE_GROUP_TRACKSELECTSCREEN	=	2,
	SPRITE_GROUP_PARTICLES			=	3,
	SPRITE_GROUP_SHADOWS			=	4,
	SPRITE_GROUP_FENCES				= 	5,
	SPRITE_GROUP_GLOBAL				= 	6,
	SPRITE_GROUP_LENSFLARES			=	7,

	MAX_SPRITE_GROUPS
};

enum
{
	MENUS_SObjType_NULL = 0,
	MENUS_SObjType_Padlock,
	MENUS_SObjType_LeftArrow,
	MENUS_SObjType_RightArrow,
	MENUS_SObjType_UpArrow,
};

enum
{
	SHADOW_SObjType_NULL = 0,
	SHADOW_SObjType_Circular,
	SHADOW_SObjType_Car_Mammoth,
	SHADOW_SObjType_Car_Bone,
	SHADOW_SObjType_Car_Geode,
	SHADOW_SObjType_Car_Log,
	SHADOW_SObjType_Car_Turtle,
	SHADOW_SObjType_Car_Rock,
	SHADOW_SObjType_Car_TrojanHorse,
	SHADOW_SObjType_Car_Obelisk,
	SHADOW_SObjType_Car_Catapult,
	SHADOW_SObjType_Car_Chariot,
	SHADOW_SObjType_Car_Sub,
};

enum
{
	GLOBAL_SObjType_Shadow_Brog_Brown,
	GLOBAL_SObjType_Shadow_Brog_Green,
	GLOBAL_SObjType_Shadow_Brog_Blue,
	GLOBAL_SObjType_Shadow_Brog_Grey,
	GLOBAL_SObjType_Shadow_Brog_Red,
	GLOBAL_SObjType_Shadow_Brog_Yellow
};

enum
{
	PARTICLE_SObjType_NULL = 0,
	PARTICLE_SObjType_WhiteSpark,
	PARTICLE_SObjType_Dirt,
	PARTICLE_SObjType_GreySmoke,
	PARTICLE_SObjType_GreenFire,
	PARTICLE_SObjType_Splash,
	PARTICLE_SObjType_SnowFlakes,
	PARTICLE_SObjType_SnowDust,
	PARTICLE_SObjType_Fire,
	PARTICLE_SObjType_BlackSmoke,
	PARTICLE_SObjType_Bubbles,
	PARTICLE_SObjType_RedSpark,
	PARTICLE_SObjType_BlueSpark
};

enum
{
	INFOBAR_SObjType_NULL = 0,

	INFOBAR_SObjType_Ready,
	INFOBAR_SObjType_Set,
	INFOBAR_SObjType_Go,

	INFOBAR_SObjType_Place1,
	INFOBAR_SObjType_Place2,
	INFOBAR_SObjType_Place3,
	INFOBAR_SObjType_Place4,
	INFOBAR_SObjType_Place5,
	INFOBAR_SObjType_Place6,

	INFOBAR_SObjType_Weapon_Bone,
	INFOBAR_SObjType_Weapon_Oil,
	INFOBAR_SObjType_Weapon_Nitro,
	INFOBAR_SObjType_Weapon_BirdBomb,
	INFOBAR_SObjType_Weapon_RomanCandle,
	INFOBAR_SObjType_Weapon_BottleRocket,
	INFOBAR_SObjType_Weapon_Torpedo,
	INFOBAR_SObjType_Weapon_Freeze,
	INFOBAR_SObjType_Weapon_LandMine,

	INFOBAR_SObjType_WeaponX,

	INFOBAR_SObjType_WrongWay,

	INFOBAR_SObjType_Lap1of3,
	INFOBAR_SObjType_Lap2of3,
	INFOBAR_SObjType_Lap3of3,

	INFOBAR_SObjType_Token_Arrowhead,
	INFOBAR_SObjType_Token_ArrowheadDim,

	INFOBAR_SObjType_StickyTires,
	INFOBAR_SObjType_Suspension,
	INFOBAR_SObjType_Invisibility,


		/* TAG ICONS */

	INFOBAR_SObjType_TimeBar,
	INFOBAR_SObjType_Marker,

	INFOBAR_SObjType_RedTorch,
	INFOBAR_SObjType_GreenTorch
};

