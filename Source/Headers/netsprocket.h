// Cro-Mag Rally low-level networking layer
//
// This is loosely inspired from the NetSprocket API,
// but it's NOT an accurate implementation of NetSprocket.
//
// If you're looking for a drop-in replacement for NetSocket,
// check out OpenPlay: https://github.com/mistysoftware/openplay

#pragma once

#if _WIN32
#include <winsock2.h>
typedef SOCKET sockfd_t;
#else
typedef int sockfd_t;
#define INVALID_SOCKET (-1)
#endif

#define MAX_CLIENTS MAX_LOCAL_PLAYERS
#define kNSpPlayerNameLength 32
#define kNSpMaxPayloadLength 256
#define kNSpMaxMessageLength (kNSpMaxPayloadLength + sizeof(NSpMessageHeader))

#if _DEBUG							// in debug builds, messages may have extra debug info
#define kNSpCMRProtocol4CC 'CMRd'
#else
#define kNSpCMRProtocol4CC 'CMR6'
#endif

typedef enum
{
	kNSpUnspecifiedEndpoint		= -2,

	// Send message to all connected players
	kNSpAllPlayers				= -1,

	// Used to send a message to the NSp game's host
	kNSpHostID					= 0,

	kNSpClientID0				= kNSpHostID,
}NSpPlayerSpecials;

// All-caps 4CCs are reserved for internal use.
enum
{
	kNSpError					= 'ERR!',
	kNSpUndefinedMessage		= 'UNDF',
	kNSpJoinRequest				= 'JREQ',
	kNSpJoinApproved			= 'JACK',
	kNSpJoinDenied				= 'JDNY',
	kNSpPlayerJoined			= 'PJND',
	kNSpPlayerLeft				= 'PLFT',
	kNSpHostChanged				= 'HCHG',
	kNSpGameTerminated			= 'FINI',
	kNSpGroupCreated			= 'GNEW',
	kNSpGroupDeleted			= 'GDEL',
	kNSpPlayerAddedToGroup		= 'P+GR',
	kNSpPlayerRemovedFromGroup	= 'P-GR',
	kNSpPlayerTypeChanged		= 'PTCH',
};

enum
{
	kNSpSendFlag_Junk			= 0x00100000,		// will be sent (try once) when there is nothing else pending
	kNSpSendFlag_Normal			= 0x00200000,		// will be sent immediately (try once)
	kNSpSendFlag_Registered		= 0x00400000,		// will be sent immediately (guaranteed, in order)
	kNSpSendFlag_DontKickOnFail	= 0x00800000,
};

enum
{
	kNSpGameFlag_DontAdvertise = 0x00000001,
	kNSpGameFlag_ForceTerminateGame = 0x00000002
};

enum
{
	kNSpRC_Failed				= -127,
	kNSpRC_SendFailed,
	kNSpRC_RecvFailed,
	kNSpRC_InvalidPlayer,
	kNSpRC_InvalidSocket,
	kNSpRC_NoGame,
	kNSpRC_NoSearch,
	kNSpRC_BadState,
	kNSpRC_OK					= 0,
};

enum
{
	kNSpGameTerminated_HostBailed		= 0,
	kNSpGameTerminated_NetworkError		= 1,
	kNSpGameTerminated_YouGotKicked		= 2,
};

typedef int32_t						NSpPlayerID;

typedef struct sockaddr_in*			NSpAddressReference;

typedef struct NSpGame*				NSpGameReference;
typedef struct NSpSearch*			NSpSearchReference;

typedef struct
{
	uint32_t						version;		// Protocol version cookie. It's an int, so we can only talk to peers with matching endianness
	int32_t							what;			// The kind of message (e.g. player joined)
	NSpPlayerID						from;			// ID of the sender
	NSpPlayerID						to;				// (player or group) id of the intended recipient
	uint32_t						id;				// Unique ID for this message & (from) player
	uint32_t						when;			// Timestamp for the message
	uint32_t						messageLen;		// Bytes of data in the entire message (including the header)
} NSpMessageHeader;

typedef struct
{
	NSpMessageHeader				header;
	char							name[kNSpPlayerNameLength];
} NSpJoinRequestMessage;

typedef struct
{
	NSpMessageHeader 				header;
	char							reason[256];
} NSpJoinDeniedMessage;

typedef struct
{
	NSpMessageHeader 				header;
} NSpJoinApprovedMessage;

typedef struct
{
	NSpMessageHeader 				header;
	uint32_t						reason;
} NSpGameTerminatedMessage;

typedef struct
{
	NSpMessageHeader 				header;
	uint32_t 						playerCount;
	struct
	{
		NSpPlayerID					id;
		char						name[kNSpPlayerNameLength];
	} playerInfo;
} NSpPlayerJoinedMessage;

typedef struct
{
	NSpMessageHeader 				header;
	uint32_t						playerCount;
	NSpPlayerID 					playerID;
	char							playerName[kNSpPlayerNameLength];
} NSpPlayerLeftMessage;

void NSpClearMessageHeader(NSpMessageHeader* h);
NSpMessageHeader* NSpMessage_Get(NSpGameReference gameRef);
int NSpMessage_Send(NSpGameReference inGame, NSpMessageHeader* inMessage, int inFlags);
void NSpMessage_Release(NSpGameReference gameRef, NSpMessageHeader* message);

int GetSocketError(void);
int GetLastSocketError(void);
const char* NSp4CCString(uint32_t fourcc);

bool NSpGame_IsValidPlayerID(NSpGameReference gameRef, NSpPlayerID id);
NSpGameReference NSpGame_Host(void);
NSpPlayerID NSpGame_AcceptNewClient(NSpGameReference gameRef);
int NSpGame_StopAcceptingNewClients(NSpGameReference gameRef);
int NSpGame_AckJoinRequest(NSpGameReference gameRef, NSpMessageHeader* inMessage);
int NSpGame_GetNumActivePlayers(NSpGameReference gameRef);
uint32_t NSpGame_GetActivePlayersIDMask(NSpGameReference gameRef);
NSpPlayerID NSpGame_GetNthActivePlayerID(NSpGameReference gameRef, int n);
int NSpGame_StartAdvertising(NSpGameReference gameRef);
int NSpGame_StopAdvertising(NSpGameReference gameRef);
int NSpGame_AdvertiseTick(NSpGameReference gameRef, float dt);
bool NSpGame_IsAdvertising(NSpGameReference gameRef);
int NSpGame_Dispose(NSpGameReference inGame, int disposeFlags);

NSpSearchReference NSpSearch_StartSearchingForGameHosts(void);
int NSpSearch_Dispose(NSpSearchReference searchRef);
int NSpSearch_GetNumGamesFound(NSpSearchReference searchRef);
int NSpSearch_Tick(NSpSearchReference searchRef);
NSpGameReference NSpSearch_JoinGame(NSpSearchReference searchRef, int gameNum);
const char* NSpSearch_GetHostAddress(NSpSearchReference searchRef, int gameNum);

int NSpPlayer_Kick(NSpGameReference gameRef, NSpPlayerID kickedPlayerID);
const char* NSpPlayer_GetName(NSpGameReference gameRef, NSpPlayerID playerID);
NSpPlayerID NSpPlayer_GetMyID(NSpGameReference gameRef);
