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

#define MAX_CLIENTS 6
#define kNSpCMRProtocol4CC 'CMR6'
#define kNSpPlayerNameLength 32
#define kNSpMaxPayloadLength 256
#define kNSpMaxMessageLength (kNSpMaxPayloadLength + sizeof(NSpMessageHeader))

typedef enum
{
	// Send message to all connected players
	kNSpAllPlayers				= 0,

	// Used to send a message to the NSp game's host
	kNSpHostID					= 1,

	kNSpClientID0				= 2,

	// For use in a headless server setup
	kNSpHostOnly				= -1,

	kNSpUnspecifiedEndpoint		= -2,
}NSpPlayerSpecials;

// All-caps 4CCs are reserved for internal use.
enum
{
	kNSpError					= 'ERR!',
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
	kNSpSendFlag_Registered		= 0x00400000		// will be sent immediately (guaranteed, in order)
};

enum
{
	kNSpGameFlag_DontAdvertise = 0x00000001,
	kNSpGameFlag_ForceTerminateGame = 0x00000002
};

enum
{
	kNSpRC_OK					= 0,
	kNSpRC_Failed				= -1,
	kNSpRC_SendFailed			= -2,
	kNSpRC_InvalidClient		= -3,
	kNSpRC_InvalidSocket		= -4,
	kNSpRC_NoGame				= -5,
};

typedef int32_t						NSpPlayerID;

typedef struct sockaddr_in*			NSpAddressReference;

typedef struct NSpCMRGame*			NSpGameReference;

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
} NSpGameTerminatedMessage;

typedef struct
{
	NSpMessageHeader 				header;
	uint32_t						playerCount;
	NSpPlayerID 					playerID;
	char							playerName[kNSpPlayerNameLength];
} NSpPlayerLeftMessage;

void NSpClearMessageHeader(NSpMessageHeader* h);

int NSpMessage_Send(NSpGameReference inGame, NSpMessageHeader* inMessage, int inFlags);

int NSpGame_Dispose(NSpGameReference inGame, int disposeFlags);

// The following functions aren't true NetSprocket calls.
const char* NSp4CCString(uint32_t fourcc);
bool NSpGame_IsValidClientID(NSpGameReference gameRef, NSpPlayerID id);
NSpPlayerID NSpGame_ClientSlotToID(NSpGameReference gameRef, int slot);
int NSpGame_ClientIDToSlot(NSpGameReference gameRef, NSpPlayerID id);
int NSpGame_AcceptNewClient(NSpGameReference gameRef);
int NSpGame_AckJoinRequest(NSpGameReference gameRef, NSpMessageHeader* inMessage);
int NSpGame_GetNumClients(NSpGameReference inGame);
