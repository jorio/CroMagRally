//
// player.h
//

#pragma once

#include "checkpoints.h"

#define	MAX_SUBMARINE_SPEED		4000.0f

#define	NUM_VEHICLE_PARAMETERS		4

#define	CapturedFlag	SpecialPtr[3]			// objNode of attached team flag
#define	InvincibleTimer	SpecialF[4]				// timer for invicibility after being hurt

#define	WhoThrew		SpecialPtr[0]			// objnode of player who threw the weapon

#define	HEAD_THROW_READY_FLAG	Flag[0]			// set when throw anim is ready to throw

#define	POSITION_TIMER			1.0f				// how often to check if CPU player got stuck


#define	ROMAN_CANDLE_RANGE		(TERRAIN_SUPERTILE_UNIT_SIZE*3)
#define	BOTTLE_ROCKET_RANGE		(TERRAIN_SUPERTILE_UNIT_SIZE*2)
#define	TORPEDO_RANGE			(TERRAIN_SUPERTILE_UNIT_SIZE*4)

#define	PLAYER_COLLISION_CTYPE	(CTYPE_PLAYER|CTYPE_MISC|CTYPE_TRIGGER|CTYPE_LIQUID|CTYPE_AUTOTARGET|CTYPE_VISCOUS)


enum
{
	CAR_TYPE_MAMMOTH,
	CAR_TYPE_BONEBUGGY,
	CAR_TYPE_GEODE,
	CAR_TYPE_LOG,
	CAR_TYPE_TURTLE,
	CAR_TYPE_ROCK,

	CAR_TYPE_TROJANHORSE,
	CAR_TYPE_OBELISK,
	CAR_TYPE_CATAPULT,
	CAR_TYPE_CHARIOT,

	NUM_LAND_CAR_TYPES,

	CAR_TYPE_SUB = NUM_LAND_CAR_TYPES,

	NUM_CAR_TYPES_TOTAL
};

enum
{
	PLAYER_ANIM_SIT,
	PLAYER_ANIM_FREAKOUT,
	PLAYER_ANIM_RIGHTTURN,
	PLAYER_ANIM_LEFTTURN,
	PLAYER_ANIM_HANGTIME,
	PLAYER_ANIM_ACCEL,
	PLAYER_ANIM_DECEL,
	PLAYER_ANIM_THROWFORWARD,
	PLAYER_ANIM_THROWBACKWARD
};




		/* CAR STATS */

typedef struct
{
	float	maxSpeed;
	float	airFriction;
	float	tireTraction;
	float	minPlaningAngle;
	float	minPlaningSpeed;
	float	acceleration;
	float	suspension;

}CarStatsType;


		/***************/
		/* PLAYER INFO */
		/***************/

typedef struct
{
	int					startX,startZ;
	float				startRotY;

	OGLPoint3D			coord;
	ObjNode				*objNode;

	ObjNode				*wheelObj[4],*headObj;

	short				sex;					// 0 = male, 1 = female
	short				vehicleType;
	short 				skin;

	Boolean				onThisMachine;
	float				distToFloor;
	float				mostRecentFloorY;

	Boolean				onWater;
	float				waterY;

	OGLRect				itemDeleteWindow;

	OGLCameraPlacement	camera;
	short				splitPaneNum;
	float				cameraRingRot;
	float				cameraUserRot;
	Byte				cameraMode;
	bool				quickRearView;

	short				snowParticleGroup;
	float				snowTimer;
	float				frozenTimer;				// when hit with freeze POW

	float				bumpSoundTimer;


			/* PHYSICS STUFF */

	CarStatsType		carStats,carStatsCopy;
	float				steering;	// -1 .. +1
	float				skidDot;						// perp @ 0.0
	float				currentRPM;


			/* DRAG DEBRIS */

	Boolean				tiresAreDragging;
	Boolean				alwaysDoDrag;
	float				dragDebrisTimer;
	int					dragDebrisParticleGroup;
	int					dragDebrisMagicNum;
	short				dragDebrisTexture;

			/* ACCELERATION INFO */

	float				currentThrust;
	Boolean				gasPedalDown;
	Boolean				accelBackwards;
	Boolean				movingBackwards;
	Boolean				braking;
	Boolean				wrongWay;


		/* TILE ATTRIBUTE PHYSICS TWEAKS */


	float				groundTraction;
	float				groundFriction;
	float				groundSteering;
	float				groundAcceleration;
	Boolean				noSkids;

			/* LAPPING/CHECKPOINTS */

	short				lapNum;
	short				checkpointNum;
	Boolean				checkpointTagged[MAX_CHECKPOINTS];
	short				place;
	float				distToNextCheckpoint;
	Boolean				raceComplete;
	Boolean				cheated;


			/* BATTLE MODES */

	float				tagTimer;									// timer for tag game mode
	float				tagOccilation;								// for occilating the color when tagged
	Boolean				isIt;										// player is it in tag mode
	Boolean				isEliminated;								// true if player eliminated from game
	Byte				team;										// team # for capture the flag mode
	float				health;
	float				impactResetTimer;							// so can't get multiple impacts back-to-back

			/* SKID STUFF */

	Boolean				isPlaning;										// true if we're planing which means that we're skidding uncontrollably
	float				greasedTiresTimer;
	OGLPoint2D			lastSkidSegCoord;								// where last skid was
	OGLVector2D			lastSkidVector;									// dir of last skid
	int					skidSmokeParticleGroup;
	int					skidSmokeMagicNum;
	float				skidSmokeTimer;
	Boolean				makingSkid;
	short				skidChannel;
	float				skidSoundTimer;
	OGLColorRGBA		skidColor;
	float				submarineImmobilized;

			/* POW TIMERS */

	float				nitroTimer;
	float				stickyTiresTimer;
	float				superSuspensionTimer;
	float				invisibilityTimer;
	float				flamingTimer;


			/* CONTROL INFO */

	uint32_t			controlBits;
	uint32_t			controlBits_New;
	OGLVector2D			analogSteering;

			/* INVENTORY INFO */

	short				powType;									// powerup type in inventory
	short				powTypeBeingThrown;							// used for delaying bullet creation until anim flag (bone/oil/bird/freeze)
	short				powQuantity;
	short				numTokens;

			/* NETWORK STUFF */
			//
			// note:  	do not init this stuff in InitPlayerInfo_Game() since
			// 			that would wipe out stuff already set.
			//

	struct
	{
		int32_t			nspPlayerID;			// this player's net sprocket id#
		uint8_t			pauseState;
	} net;



			/* ARTIFICIAL INTELLIGENCE */

	Boolean             isComputer;									// true if computer is playing this player
	float				oldPositionTimer;							// used for seeing if need to check player stuck/stopped
	OGLPoint3D			oldPosition;								// where it was last time we checked
	float				reverseTimer;
	float				attackTimer;
	short				targetedPlayer;								// player currently targeted
	float				targetingTimer;								// how long player has been targeted
	OGLVector2D			pathVec;

			/* AUDIO */

	short				engineChannel;

			/* TIME ATTACK */

	float				lapTimes[LAPS_PER_RACE];
}PlayerInfoType;


typedef union
{
	struct
	{
		int8_t	speed;
		int8_t	acceleration;
		int8_t	traction;
		int8_t	suspension;
	};

	int8_t params[NUM_VEHICLE_PARAMETERS];
} UserCarStats;

typedef struct
{
	float		steeringResponsiveness;
	float		carMaxTightTurn;			// bigger == more responsive steering at high speeds
	float		carTurningRadius;			// bigger == tighter turns, smaller = wider turns
	float		tireTraction;				// amount of grip the tires have when turning
	float		tireFriction;				// when travelling perpendicular to motion
	float		carGravity;
	float		slopeRatioAdjuster;			// bigger == able to climb steep hills easier, smaller == bounce off walls more.
	float		terrainHeight;
	UserCarStats	carStats[NUM_CAR_TYPES_TOTAL];
} UserPhysics;


//=======================================================

void InitPlayerInfo_Game(void);
void InitPlayersAtStartOfLevel(void);
void SetPlayerParmsFromTileAttributes(short playerNum, uint16_t flags);
void ChooseTaggedPlayer(void);
void UpdateTagMarker(void);
short FindClosestPlayer(ObjNode *thePlayer, float x, float z, float range, Boolean allowCPUCars, float *dist);
short FindClosestPlayerInBack(ObjNode *theNode, float range, Boolean allowCPUCars, float *dist, float angle);
short FindClosestPlayerInFront(ObjNode *theNode, float range, Boolean allowCPUCars, float *dist, float angle);
void PlayerLoseHealth(short p, float damage);
void SetStickyTires(short playerNum);
void SetSuspensionPOW(short playerNum);
void SetCarStatusBits(short	playerNum, uint32_t bits);
void ClearCarStatusBits(short	playerNum, uint32_t bits);


		/* CAR */

ObjNode *InitPlayer_Car(int playerNum, OGLPoint3D *where, float rotY);
void SetPhysicsForVehicleType(short playerNum);
void CreateCarWheelsAndHead(ObjNode *theCar, short playerNum);
void AlignWheelsAndHeadOnCar(ObjNode *theCar);
void BlastCars(short whoThrew, float x, float y, float z, float radius);
void SetTractionPhysics(CarStatsType *carStats, float p);
void SetSuspensionPhysics(CarStatsType *carStats, float n);
void RotateCarXZ(ObjNode *theNode);
short DoCPU_AvoidObjects(ObjNode *theCar);
void DoCPUPowerupLogic(ObjNode *carObj, short playerNum);
void UpdateNitro(short p);
void SetDefaultPhysics(void);


		/* SUB */

ObjNode *InitPlayer_Submarine(int playerNum, OGLPoint3D *where, float rotY);


		/* WEAPON */

void CheckPOWControls(ObjNode *theNode);
void ThrowBone(short playerNum, Boolean throwForward);
void ThrowOil(short playerNum, Boolean throwForward);
void ActivateNitroPOW(short playerNum);
void ThrowBirdBomb(short playerNum, Boolean throwForward);
void ThrowFreeze(short playerNum, Boolean throwForward);
Boolean DoTrig_LandMine(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);


		/* SKID MARKS */

void InitSkidMarks(void);
void DoSkidMark(ObjNode *owner, short subID, float x, float z, float dx, float dz);
void DrawSkidMarks(void);
void UpdateSkidMarks(void);
void StretchSkidMark(ObjNode *owner, short subID, float x, float z, float dx, float dz);
void DetachOwnerFromSkid(ObjNode *owner);
void MakeSkidSmoke(short p, OGLPoint3D *coord);

