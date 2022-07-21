//
// network.h
//

#include "main.h"
#include "netsprocket.h"

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
	kNetNullPacket,
};

		/***************************/
		/* MESSAGE DATA STRUCTURES */
		/***************************/

		/* GAME CONFIGURATION MESSAGE */

typedef struct
{
	NSpMessageHeader	h;
	int32_t				gameMode;							// game mode (tag, race, etc.)
	int32_t				age;								// which age to play for race mode
	int32_t				trackNum;							// which track to play for battle modes
	int32_t				playerNum;							// this player's index
	int32_t				numPlayers;							// # players in net game
	int16_t				numTracksCompleted;					// pass saved game value to clients so we're all the same here
	int16_t				difficulty;							// pass host's difficulty setting so we're in sync
	int16_t				tagDuration;						// # minutes in tag game
}NetConfigMessageType;

		/* SYNC MESSAGE */

typedef struct
{
	NSpMessageHeader	h;
	int32_t				playerNum;							// this player's index
}NetSyncMessageType;


		/* HOST CONTROL INFO MESSAGE */

typedef struct
{
	NSpMessageHeader	h;
	float				fps, fpsFrac;
	uint32_t			randomSeed;					// simply used for error checking (all machines should have same seed!)
	uint32_t			controlBits[MAX_PLAYERS];
	uint32_t			controlBitsNew[MAX_PLAYERS];
	float				analogSteering[MAX_PLAYERS];
	uint32_t			frameCounter;
}NetHostControlInfoMessageType;


		/* CLIENT CONTROL INFO MESSAGE */

typedef struct
{
	NSpMessageHeader	h;
	int16_t				playerNum;
	uint32_t			controlBits;
	uint32_t			controlBitsNew;
	uint32_t			frameCounter;
	float				analogSteering;
}NetClientControlInfoMessageType;


		/* PLAYER CHAR TYPE MESSAGE */

typedef struct
{
	NSpMessageHeader	h;
	int16_t				playerNum;
	int16_t				vehicleType;
	int16_t				sex;				// 0 = male, 1 = female
}NetPlayerCharTypeMessage;




//===============================================================================


void InitNetworkManager(void);
void ShutdownNetworkManager(void);
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

//===============================================================================

enum
{
	kNetSequence_Offline,
	kNetSequence_Error,
	
	kNetSequence_HostOffline,
	kNetSequence_HostCreatedLobby,
	kNetSequence_HostWaitingForAnyPlayersToJoinLobby,
	kNetSequence_HostWaitingForMorePlayersToJoinLobby,
	kNetSequence_HostReadyToStartGame,
	kNetSequence_HostStartingGame,
	
	kNetSequence_ClientOffline,
	kNetSequence_ClientSearchingForGames,
	kNetSequence_ClientFoundGames,
	kNetSequence_ClientJoiningGame,
	kNetSequence_ClientWaitingForGameConfigInfo,

	kNetSequence_WaitingForPlayerVehicles1,
	kNetSequence_WaitingForPlayerVehicles2,
	kNetSequence_WaitingForPlayerVehicles3,
	kNetSequence_WaitingForPlayerVehicles4,
	kNetSequence_WaitingForPlayerVehicles5,
	kNetSequence_WaitingForPlayerVehicles6,
	kNetSequence_WaitingForPlayerVehicles7,
	kNetSequence_WaitingForPlayerVehicles8,
	kNetSequence_WaitingForPlayerVehicles9,
	kNetSequence_GotAllPlayerVehicles,
};


//===============================================================================
// Low-level networking routines

void Net_Tick(void);
bool Net_IsLobbyBroadcastOpen(void);
NSpGameReference Net_CreateLobby(void);
void Net_CloseLobby(void);
bool Net_CreateLobbySearch(void);
void Net_CloseLobbySearch(void);
int Net_GetNumLobbiesFound(void);
NSpGameReference Net_JoinLobby(int lobbyNum);
const char* Net_GetLobbyAddress(int lobbyNum);
