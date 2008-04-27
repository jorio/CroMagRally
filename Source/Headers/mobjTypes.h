//
// mobjtypes.h
//

enum
{
	MODEL_GROUP_GLOBAL		=	0,
	MODEL_GROUP_CARPARTS	=	1,
	MODEL_GROUP_WEAPONS		=	2,
	MODEL_GROUP_LEVELSPECIFIC =	3,
	MODEL_GROUP_WINNERS		=	4,
	MODEL_GROUP_CARSELECT	=	5,

	MODEL_GROUP_SKELETONBASE				// skeleton files' models are attached here
};




/******************* GLOBAL *************************/

enum
{
	GLOBAL_ObjType_BonePOW,
	GLOBAL_ObjType_OilPOW,
	GLOBAL_ObjType_NitroPOW,
	GLOBAL_ObjType_BirdBombPOW,
	GLOBAL_ObjType_RomanCandlePOW,
	GLOBAL_ObjType_BottleRocketPOW,
	GLOBAL_ObjType_TorpedoPOW,
	GLOBAL_ObjType_FreezePOW,
	GLOBAL_ObjType_MinePOW,

	GLOBAL_ObjType_InvisibilityPOW,
	GLOBAL_ObjType_StickyTiresPOW,
	GLOBAL_ObjType_SuspensionPOW,

	GLOBAL_ObjType_Token_ArrowHead,


	GLOBAL_ObjType_WaterPatch,

	GLOBAL_ObjType_TeamTorch,
	GLOBAL_ObjType_TeamBaseRed,
	GLOBAL_ObjType_TeamBaseGreen,

	GLOBAL_ObjType_Sign_Fire,
	GLOBAL_ObjType_Sign_Twister,
	GLOBAL_ObjType_Sign_Slippery,
	GLOBAL_ObjType_Sign_Curves,
	GLOBAL_ObjType_Sign_Ramp,
	GLOBAL_ObjType_Sign_Snow,

	GLOBAL_ObjType_GreyRock,
	GLOBAL_ObjType_RedRock,
	GLOBAL_ObjType_IceRock

};

/******************* WEAPONS *************************/

enum
{
	WEAPONS_ObjType_BoneBullet,
	WEAPONS_ObjType_RomanCandleBullet,
	WEAPONS_ObjType_BottleRocketBullet,
	WEAPONS_ObjType_OilBullet,
	WEAPONS_ObjType_Torpedo,
	WEAPONS_ObjType_FreezeBullet,
	WEAPONS_ObjType_LandMine,

	WEAPONS_ObjType_OilPatch,
	WEAPONS_ObjType_Shockwave,
	WEAPONS_ObjType_ConeBlast,
	WEAPONS_ObjType_SnowShockwave
};



/******************* DESERT *************************/

enum
{
	DESERT_ObjType_Cyclorama,
	DESERT_ObjType_StartingLine,

	DESERT_ObjType_Cactus,
	DESERT_ObjType_ShortCactus,

	DESERT_ObjType_DustDevilTop,
	DESERT_ObjType_DustDevil4,
	DESERT_ObjType_DustDevil3,
	DESERT_ObjType_DustDevil2,
	DESERT_ObjType_DustDevilBottom,

	DESERT_ObjType_RockOverhang,
	DESERT_ObjType_RockOverhang2,
	DESERT_ObjType_RockColumn1,
	DESERT_ObjType_RockColumn2,
	DESERT_ObjType_RockColumn3
};


/******************* JUNGLE *************************/

enum
{
	JUNGLE_ObjType_Cyclorama,
	JUNGLE_ObjType_StartingLine,
	JUNGLE_ObjType_EasterHead,
	JUNGLE_ObjType_Tree1,
	JUNGLE_ObjType_Tree2,
	JUNGLE_ObjType_Tree3,
	JUNGLE_ObjType_Vine,
	JUNGLE_ObjType_Hut1,
	JUNGLE_ObjType_Hut2,
	JUNGLE_ObjType_Volcano,
	JUNGLE_ObjType_TotemPole,
	JUNGLE_ObjType_Dart

};


/******************* ICE *************************/

enum
{
	ICE_ObjType_Cyclorama,
	ICE_ObjType_StartingLine,

	ICE_ObjType_SnoMan,
	ICE_ObjType_CampFire,
	ICE_ObjType_IceBridge,
	ICE_ObjType_Igloo,
	ICE_ObjType_Tree
};

/******************* CHINA *************************/

enum
{
	CHINA_ObjType_Cyclorama,
	CHINA_ObjType_StartingLine,

	CHINA_ObjType_Rickshaw,
	CHINA_ObjType_GongFrame,
	CHINA_ObjType_Gong,
	CHINA_ObjType_House
};


/******************* SCANDINAVIA *************************/

enum
{
	SCANDINAVIA_ObjType_Cyclorama,
	SCANDINAVIA_ObjType_StartingLine,

	SCANDINAVIA_ObjType_Stump1,
	SCANDINAVIA_ObjType_Stump2,
	SCANDINAVIA_ObjType_Stump3,
	SCANDINAVIA_ObjType_Stump4,
	SCANDINAVIA_ObjType_Stump5,

	SCANDINAVIA_ObjType_Baracade1,
	SCANDINAVIA_ObjType_Baracade2,

	SCANDINAVIA_ObjType_Cabin1,
	SCANDINAVIA_ObjType_Cabin2,
	SCANDINAVIA_ObjType_Cabin3,

	SCANDINAVIA_ObjType_VikingFlag,
	SCANDINAVIA_ObjType_LookoutTower,
	SCANDINAVIA_ObjType_TorchPot,
	SCANDINAVIA_ObjType_VikingShip,
	SCANDINAVIA_ObjType_WeaponsRack,
	SCANDINAVIA_ObjType_Campfire,
	SCANDINAVIA_ObjType_TallPine,
	SCANDINAVIA_ObjType_WidePine

};




/******************* EGYPT *************************/

enum
{
	EGYPT_ObjType_Cyclorama,
	EGYPT_ObjType_StartingLine,

	EGYPT_ObjType_Obelisk,
	EGYPT_ObjType_Pillar,
	EGYPT_ObjType_Pylon,
	EGYPT_ObjType_Boat,
	EGYPT_ObjType_Statue,
	EGYPT_ObjType_Sphinx,
	EGYPT_ObjType_Vase,
	EGYPT_ObjType_CatStatue
};


/******************* CRETE *************************/

enum
{
	CRETE_ObjType_Cyclorama,
	CRETE_ObjType_StartingLine,

	CRETE_ObjType_Column1,
	CRETE_ObjType_Column2,
	CRETE_ObjType_Column3,
	CRETE_ObjType_Boat,
	CRETE_ObjType_Clock,
	CRETE_ObjType_House1,
	CRETE_ObjType_House2,
	CRETE_ObjType_Palace,
	CRETE_ObjType_House3,
	CRETE_ObjType_Goddess,
	CRETE_ObjType_BullStatue,
	CRETE_ObjType_TallTree,
	CRETE_ObjType_WideTree

};


/******************* EURPOE *************************/

enum
{
	EUROPE_ObjType_Cyclorama,
	EUROPE_ObjType_StartingLine,

	EUROPE_ObjType_CastleTower,
	EUROPE_ObjType_SiegeTower,
	EUROPE_ObjType_Cauldron,
	EUROPE_ObjType_Cottage,
	EUROPE_ObjType_Lodge,
	EUROPE_ObjType_TownHouse,
	EUROPE_ObjType_Well,
	EUROPE_ObjType_Cannon,
	EUROPE_ObjType_CannonBall,
	EUROPE_ObjType_TallPine,
	EUROPE_ObjType_WidePine
};


/******************* ATLANTIS *************************/

enum
{
	ATLANTIS_ObjType_Cyclorama,
	ATLANTIS_ObjType_StartingLine,

	ATLANTIS_ObjType_Column1,
	ATLANTIS_ObjType_Column2,
	ATLANTIS_ObjType_Clam,

	ATLANTIS_ObjType_BugDome,
	ATLANTIS_ObjType_SaucerDome,
	ATLANTIS_ObjType_TwinkieDome,
	ATLANTIS_ObjType_Tower,
	ATLANTIS_ObjType_Capsule,
	ATLANTIS_ObjType_SeaMine,
	ATLANTIS_ObjType_Shipwreck
};


/******************* STONE HENGE *************************/

enum
{
	STONEHENGE_ObjType_Cyclorama,
	STONEHENGE_ObjType_Post,
	STONEHENGE_ObjType_InnerHenge,
	STONEHENGE_ObjType_OuterHenge
};


/******************* AZTEC *************************/

enum
{
	AZTEC_ObjType_Cyclorama,
	AZTEC_ObjType_StoneHead,
	AZTEC_ObjType_Tree
};


/******************* COLISEUM *************************/

enum
{
	COLISEUM_ObjType_Cyclorama,
	COLISEUM_ObjType_Wall,
	COLISEUM_ObjType_Column
};

/******************* TAR *************************/

enum
{
	TAR_ObjType_Cyclorama,
	TAR_ObjType_TarPatch
};




/******************* CAR PARTS *************************/

enum
{

	CARPARTS_ObjType_Body_Mammoth,
	CARPARTS_ObjType_Body_Bone,
	CARPARTS_ObjType_Body_Geode,
	CARPARTS_ObjType_Body_Log,
	CARPARTS_ObjType_Body_Turtle,
	CARPARTS_ObjType_Body_Rock,
	CARPARTS_ObjType_Body_TrojanHorse,
	CARPARTS_ObjType_Body_Obelisk,
	CARPARTS_ObjType_Body_Catapult,
	CARPARTS_ObjType_Body_xxxxxx,

	CARPARTS_ObjType_Wheel_MammothFL,
	CARPARTS_ObjType_Wheel_MammothFR,
	CARPARTS_ObjType_Wheel_MammothBR,
	CARPARTS_ObjType_Wheel_MammothBL,

	CARPARTS_ObjType_Wheel_BoneFL,
	CARPARTS_ObjType_Wheel_BoneFR,
	CARPARTS_ObjType_Wheel_BoneBR,
	CARPARTS_ObjType_Wheel_BoneBL,

	CARPARTS_ObjType_Wheel_GeodeFL,
	CARPARTS_ObjType_Wheel_GeodeFR,
	CARPARTS_ObjType_Wheel_GeodeBR,
	CARPARTS_ObjType_Wheel_GeodeBL,

	CARPARTS_ObjType_Wheel_LogFL,
	CARPARTS_ObjType_Wheel_LogFR,
	CARPARTS_ObjType_Wheel_LogBR,
	CARPARTS_ObjType_Wheel_LogBL,

	CARPARTS_ObjType_Wheel_TurtleFL,
	CARPARTS_ObjType_Wheel_TurtleFR,
	CARPARTS_ObjType_Wheel_TurtleBR,
	CARPARTS_ObjType_Wheel_TurtleBL,

	CARPARTS_ObjType_Wheel_RockFL,
	CARPARTS_ObjType_Wheel_RockFR,
	CARPARTS_ObjType_Wheel_RockBR,
	CARPARTS_ObjType_Wheel_RockBL,

	CARPARTS_ObjType_Wheel_TrojanFL,
	CARPARTS_ObjType_Wheel_TrojanFR,
	CARPARTS_ObjType_Wheel_TrojanBR,
	CARPARTS_ObjType_Wheel_TrojanBL,

	CARPARTS_ObjType_Wheel_ObeliskFL,
	CARPARTS_ObjType_Wheel_ObeliskFR,
	CARPARTS_ObjType_Wheel_ObeliskBR,
	CARPARTS_ObjType_Wheel_ObeliskBL,

	CARPARTS_ObjType_Wheel_CatapultFL,
	CARPARTS_ObjType_Wheel_CatapultFR,
	CARPARTS_ObjType_Wheel_CatapultBR,
	CARPARTS_ObjType_Wheel_CatapultBL,

	CARPARTS_ObjType_Wheel_xxxxFL,
	CARPARTS_ObjType_Wheel_xxxxFR,
	CARPARTS_ObjType_Wheel_xxxxBR,
	CARPARTS_ObjType_Wheel_xxxxBL,

	CARPARTS_ObjType_Submarine,
	CARPARTS_ObjType_Propeller
};



