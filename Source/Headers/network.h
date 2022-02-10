//
// network.h
//

//#include <NetSprocket.h>
#include "main.h"

enum
{
	kStandardMessageSize	= 256,	//0,
	kBufferSize				= 200000, 	//0,
	kQElements				= 200,
	kTimeout				= 0
};


enum
{
	kNetConfigureMessage = 1,
	kNetSyncMessage,
	kNetHostControlInfoMessage,
	kNetClientControlInfoMessage,
	kNetPlayerCharTypeMessage,
	kNetNullPacket
};


		/***************************/
		/* MESSAGE DATA STRUCTURES */
		/***************************/

		/* GAME CONFIGURATION MESSAGE */

typedef struct
{
//	NSpMessageHeader	h;
	int					gameMode;							// game mode (tag, race, etc.)
	int					age;								// which age to play for race mode
	int					trackNum;							// which track to play for battle modes
	long				playerNum;							// this player's index
	long				numPlayers;							// # players in net game
	short				numAgesCompleted;					// pass saved game value to clients so we're all the same here
	short				difficulty;							// pass host's difficulty setting so we're in sync
	short				tagDuration;						// # minutes in tag game
}NetConfigMessageType;

		/* SYNC MESSAGE */

typedef struct
{
//	NSpMessageHeader	h;
	long				playerNum;							// this player's index
}NetSyncMessageType;


		/* HOST CONTROL INFO MESSAGE */

typedef struct
{
//	NSpMessageHeader	h;
	float				fps, fpsFrac;
	u_long				randomSeed;					// simply used for error checking (all machines should have same seed!)
	u_long				controlBits[MAX_PLAYERS];
	u_long				controlBitsNew[MAX_PLAYERS];
	float				analogSteering[MAX_PLAYERS];
	u_long				frameCounter;
}NetHostControlInfoMessageType;


		/* CLIENT CONTROL INFO MESSAGE */

typedef struct
{
//	NSpMessageHeader	h;
	short				playerNum;
	u_long				controlBits;
	u_long				controlBitsNew;
	u_long				frameCounter;
	float				analogSteering;
}NetClientControlInfoMessageType;


		/* PLAYER CHAR TYPE MESSAGE */

typedef struct
{
//	NSpMessageHeader	h;
	short				playerNum;
	short				vehicleType;
	short				sex;				// 0 = male, 1 = female
}NetPlayerCharTypeMessage;




//===============================================================================


void InitNetworkManager(void);
Boolean SetupNetworkHosting(void);
Boolean SetupNetworkJoin(void);
void ClientTellHostLevelIsPrepared(void);
void HostWaitForPlayersToPrepareLevel(void);

void HostSend_ControlInfoToClients(void);
void ClientSend_ControlInfoToHost(void);
void ClientReceive_ControlInfoFromHost(void);
void HostReceive_ControlInfoFromClients(void);

void PlayerBroadcastVehicleType(void);
void GetVehicleSelectionFromNetPlayers(void);


void EndNetworkGame(void);
void PlayerBroadcastNullPacket(void);
