/****************************/
/* LOW-LEVEL NETWORKING     */
/* (c)2022 Iliyas Jorio     */
/****************************/

#if _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int ssize_t;
typedef int socklen_t;
#define MSG_NOSIGNAL 0
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "game.h"
#include "network.h"

#define LOBBY_PORT 49959
#define LOBBY_PORT_STR "49959"
#define LOBBY_BROADCAST_INTERVAL 1.0f

#define PENDING_CONNECTIONS_QUEUE 10

#define MAX_LOBBIES 5

#define NSPGAME_COOKIE32 'NSpG'

#if _WIN32
#define kSocketError_WouldBlock			WSAEWOULDBLOCK
#define kSocketError_AddressInUse		WSAEADDRINUSE
#else
#define kSocketError_WouldBlock			EAGAIN
#define kSocketError_AddressInUse		EADDRINUSE
#endif

typedef enum
{
	kNSpPlayerState_Offline,
	kNSpPlayerState_Me,
	kNSpPlayerState_ConnectedPeer,
	kNSpPlayerState_AwaitingHandshake,
} NSpPlayerState;

typedef struct NSpPlayer
{
	NSpPlayerID					id;
	NSpPlayerState				state;
	sockfd_t					sockfd;
	char						name[kNSpPlayerNameLength];
} NSpPlayer;

typedef struct NSpGame
{
	sockfd_t					hostAdvertiseSocket;
	sockfd_t					hostListenSocket;
	sockfd_t					clientToHostSocket;

	bool						isHosting;
	bool						isAdvertising;
	NSpPlayerID					myID;

	NSpPlayer					players[MAX_CLIENTS];

	float						timeToReadvertise;

	uint32_t					cookie;
} NSpGame;

typedef struct
{
	struct sockaddr_in			hostAddr;
} LobbyInfo;

typedef struct NSpSearch
{
	sockfd_t					listenSocket;
	int							numGamesFound;
	LobbyInfo					gamesFound[MAX_LOBBIES];
} NSpSearch;

static uint32_t gOutboundMessageCounter = 1;

static int gLastQueriedSocketError = 0;

static NSpGameReference JoinLobby(const LobbyInfo* lobby);
static sockfd_t CreateTCPSocket(bool bindIt);
static NSpGame* NSpGame_Unbox(NSpGameReference gameRef);
static NSpGameReference NSpGame_Alloc(void);
static void NSpPlayer_Clear(NSpPlayer* player);
static int SendOnSocket(sockfd_t sockfd, NSpMessageHeader* header);

static NSpPlayerID NSpGame_ClientSlotToID(NSpGameReference gameRef, int slot);
static int NSpGame_ClientIDToSlot(NSpGameReference gameRef, NSpPlayerID id);
static NSpPlayer* NSpGame_GetPlayerFromID(NSpGameReference gameRef, NSpPlayerID id);

#pragma mark - Cross-platform compat

int GetSocketError(void)
{
#if _WIN32
	gLastQueriedSocketError = WSAGetLastError();
#else
	gLastQueriedSocketError = errno;
#endif
	return gLastQueriedSocketError;
}

int GetLastSocketError(void)
{
	return gLastQueriedSocketError;
}

bool IsSocketValid(sockfd_t sockfd)
{
	return sockfd != INVALID_SOCKET;
}

bool MakeSocketNonBlocking(sockfd_t sockfd)
{
#if _WIN32
	u_long flags = 1;  // 0=blocking, 1=non-blocking
	return NO_ERROR == ioctlsocket(sockfd, FIONBIO, &flags);
#else
	int flags = fcntl(sockfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	return -1 != fcntl(sockfd, F_SETFL, flags);
#endif
}

bool CloseSocket(sockfd_t* sockfdPtr)
{
	if (!sockfdPtr)
	{
		return false;
	}

	if (!IsSocketValid(*sockfdPtr))
	{
		return false;
	}

#if _WIN32
	closesocket(*sockfdPtr);
#else
	close(*sockfdPtr);
#endif

	printf("Closed socket %d.\n", *sockfdPtr);

	*sockfdPtr = INVALID_SOCKET;

	return true;
}

static const char* FormatAddress(struct sockaddr_in hostAddr)
{
	static char hostname[128];

	snprintf(hostname, sizeof(hostname), "[EMPTY]");
	inet_ntop(hostAddr.sin_family, &hostAddr.sin_addr, hostname, sizeof(hostname));

	snprintfcat(hostname, sizeof(hostname), ":%d", hostAddr.sin_port);

	return hostname;
}

const char* NSp4CCString(uint32_t fourcc)
{
	static char cstr[5];
	cstr[0] = (fourcc >> 24) & 0xFF;
	cstr[1] = (fourcc >> 16) & 0xFF;
	cstr[2] = (fourcc >> 8) & 0xFF;
	cstr[3] = (fourcc) & 0xFF;
	return cstr;
}

static void CopyPlayerName(char* dest, const char* src)
{
	memset(dest, 0, kNSpPlayerNameLength);
	strncpy(dest, src, kNSpPlayerNameLength);
	dest[kNSpPlayerNameLength-1] = '\0';
}

#pragma mark - Lobby broadcast

static sockfd_t CreateUDPBroadcastSocket(void)
{
	sockfd_t sockfd = INVALID_SOCKET;

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!IsSocketValid(sockfd))
	{
		printf("%s: socket failed: %d\n", __func__, GetSocketError());
		goto fail;
	}

	int broadcast = 1;
	int sockoptRC = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void*) &broadcast, sizeof(broadcast));
	if (-1 == sockoptRC)
	{
		goto fail;
	}

	if (!MakeSocketNonBlocking(sockfd))
	{
		goto fail;
	}

	printf("Created UDP socket %d.\n", (int) sockfd);
	return sockfd;

fail:
	CloseSocket(&sockfd);
	return sockfd;
}



#pragma mark - Join lobby

static NSpGameReference JoinLobby(const LobbyInfo* lobby)
{
	printf("%s: %s\n", __func__, FormatAddress(lobby->hostAddr));

	int sockfd = INVALID_SOCKET;
	
	sockfd = CreateTCPSocket(false);
	if (!IsSocketValid(sockfd))
	{
		printf("%s: socket failed: %d\n", __func__, GetSocketError());
		goto fail;
	}

	struct sockaddr_in bindAddr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(LOBBY_PORT),//lobby->hostAddr.sin_port,
		.sin_addr.s_addr = lobby->hostAddr.sin_addr.s_addr,
	};

	int connectRC = connect(sockfd, (const struct sockaddr*) &bindAddr, sizeof(bindAddr));

	if (connectRC == -1)
	{
		printf("%s: connect failed: %d\n", __func__, GetSocketError());
		goto fail;
	}

	// make it blocking AFTER connecting
	if (!MakeSocketNonBlocking(sockfd))
	{
		printf("%s: non-blocking failed: %d\n", __func__, GetSocketError());
		goto fail;
	}

	NSpJoinRequestMessage message;
	NSpClearMessageHeader(&message.header);
	message.header.messageLen	= sizeof(NSpJoinRequestMessage);
	message.header.to			= kNSpHostID;
	message.header.what			= kNSpJoinRequest;
	snprintf(message.name, sizeof(message.name), "CLIENT");

	int rc = SendOnSocket(sockfd, &message.header);

	if (rc != kNSpRC_OK)
	{
		goto fail;
	}

	NSpGameReference gameRef = NSpGame_Alloc();
	NSpGame* game = NSpGame_Unbox(gameRef);
	game->isHosting = false;
	game->clientToHostSocket = sockfd;
	return gameRef;

fail:
	CloseSocket(&sockfd);
	return NULL;
}

#pragma mark - Host lobby

NSpPlayerID NSpGame_AcceptNewClient(NSpGameReference gameRef)
{
	int newClientSlot = -1;
	NSpPlayerID newPlayerID = kNSpUnspecifiedEndpoint;
	NSpGame* game = NSpGame_Unbox(gameRef);

	GAME_ASSERT(game->isHosting);

	sockfd_t newSocket = INVALID_SOCKET;

	newSocket = accept(game->hostListenSocket, NULL, NULL);

	if (!IsSocketValid(newSocket))	// nobody's trying to connect right now
	{
		goto fail;
	}

	// make the socket non-blocking
	if (!MakeSocketNonBlocking(newSocket))
	{
		goto fail;
	}

	// Find vacant player slot
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (game->players[i].state == kNSpPlayerState_Offline)
		{
			newClientSlot = i;
			newPlayerID = NSpGame_ClientSlotToID(gameRef, newClientSlot);
			break;
		}
	}

	if (newClientSlot >= 0)
	{
		// Create new player
		NSpPlayer* newPlayer = &game->players[newClientSlot];

		newPlayer->id			= newPlayerID;
		newPlayer->state		= kNSpPlayerState_AwaitingHandshake;
		newPlayer->sockfd		= newSocket;
		snprintf(newPlayer->name, sizeof(newPlayer->name), "PLAYER %d", newPlayer->id);
	}
	else
	{
		// All slots used up
		printf("%s: A new client wants to connect, but the game is full!\n", __func__);

		NSpJoinDeniedMessage deniedMessage = {0};
		NSpClearMessageHeader(&deniedMessage.header);
		deniedMessage.header.messageLen = sizeof(NSpJoinDeniedMessage);
		deniedMessage.header.from = kNSpHostID;
		deniedMessage.header.what = kNSpJoinDenied;
		snprintf(deniedMessage.reason, sizeof(deniedMessage.reason), "THE GAME IS FULL.");

		SendOnSocket(newSocket, &deniedMessage.header);

		goto fail;
	}

	printf("%s: Accepted client #%d on new socket %d.\n", __func__, newPlayerID, (int) newSocket);

	return newPlayerID;

fail:
	CloseSocket(&newSocket);
	return -1;
}

#pragma mark - Message socket

static sockfd_t CreateTCPSocket(bool bindIt)
{
	sockfd_t sockfd = INVALID_SOCKET;
	struct addrinfo* res = NULL;

	struct addrinfo hints =
	{
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_PASSIVE,
		.ai_protocol = IPPROTO_TCP,
	};

	if (0 != getaddrinfo(NULL, LOBBY_PORT_STR, &hints, &res))
	{
		goto fail;
	}

	struct addrinfo* goodRes = res;  // TODO: actually walk through res

	sockfd = socket(goodRes->ai_family, goodRes->ai_socktype, goodRes->ai_protocol);
	if (!IsSocketValid(sockfd))
	{
		goto fail;
	}

	if (bindIt && !MakeSocketNonBlocking(sockfd))
	{
		goto fail;
	}

//	int set = 1;
//	if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int)))
//	{
//		goto fail;
//	}

	if (bindIt)
	{
		int bindRC = bind(sockfd, goodRes->ai_addr, goodRes->ai_addrlen);
		if (0 != bindRC)
		{
			printf("%s: bind failed: %d\n", __func__, GetSocketError());
			goto fail;
		}
	}

	freeaddrinfo(res);
	res = NULL;

	printf("Created TCP socket %d.\n", (int) sockfd);

	return sockfd;

fail:
	if (res != NULL)
	{
		freeaddrinfo(res);
		res = NULL;
	}
	CloseSocket(&sockfd);
	return INVALID_SOCKET;
}

#pragma mark - Basic message

void NSpClearMessageHeader(NSpMessageHeader* h)
{
	memset(h, 0, sizeof(NSpMessageHeader));
	h->version		= kNSpCMRProtocol4CC;
	h->when			= 0;
	h->id			= gOutboundMessageCounter;
	h->from			= kNSpUnspecifiedEndpoint;
	h->to			= kNSpUnspecifiedEndpoint;
	h->messageLen	= 0xBADBABEE;
	h->what			= kNSpUndefinedMessage;

	gOutboundMessageCounter++;
}

static NSpMessageHeader* PollSocket(sockfd_t sockfd, bool* outBrokenPipe)
{
	NSpMessageHeader* outMessage = NULL;
	bool brokenPipe = false;

	char messageBuf[kNSpMaxMessageLength];

	// Read header
	ssize_t recvRC = recv(
		sockfd,
		messageBuf,
		sizeof(NSpMessageHeader),
		MSG_NOSIGNAL
	);

	// If received 0 bytes, our peer is probably gone (in theory we never send 0-byte messages)
	if (recvRC == 0)
	{
		brokenPipe = true;
		goto bye;
	}

	// if -1, probably EAGAIN since our sockets are non-blocking
	if (recvRC == -1)
	{
		goto bye;
	}

	if ((size_t) recvRC < sizeof(NSpMessageHeader))
	{
		printf("%s: not enough bytes: %ld\n", __func__, recvRC);
		goto bye;
	}

	NSpMessageHeader* header = (NSpMessageHeader*) messageBuf;

	if (header->version != kNSpCMRProtocol4CC)
	{
		printf("%s: bad protocol %08x\n", __func__, header->version);
		goto bye;
	}

	if (header->messageLen > kNSpMaxPayloadLength
		|| header->messageLen < sizeof(NSpMessageHeader))
	{
		printf("%s: invalid message length %u\n", __func__, header->messageLen);
		goto bye;
	}

	ssize_t payloadLen = header->messageLen - sizeof(NSpMessageHeader);

	// Read payload if there's more to the message than just the header
	if (payloadLen > 0)
	{
		// Read rest of payload
		recvRC = recv(
			sockfd,
			messageBuf + sizeof(NSpMessageHeader),
			payloadLen,
			MSG_NOSIGNAL
		);

		// If received 0 bytes, our peer is probably gone (in theory we never send 0-byte messages)
		if (recvRC == 0)
		{
			brokenPipe = true;
			goto bye;
		}

		// if -1, probably EAGAIN since our sockets are non-blocking
		if (recvRC == -1)
		{
			printf("%s: couldn't read payload for message '%s'\n", __func__, NSp4CCString(header->what));
			goto bye;
		}
	}

	char* returnBuf = AllocPtr(header->messageLen);
	memcpy(returnBuf, messageBuf, header->messageLen);
	outMessage = (NSpMessageHeader*) returnBuf;
	printf("Got message '%s' (%d bytes), from player %d, on socket %d\n",
		NSp4CCString(outMessage->what), outMessage->messageLen, outMessage->from, (int) sockfd);

bye:
	if (brokenPipe)
	{
		printf("%s: broken pipe on socket %d\n", __func__, (int) sockfd);
		//NSpMessageHeader* brokenPipeHeader = (NSpMessageHeader*) AllocPtr(sizeof)
	}
	if (outBrokenPipe)
	{
		*outBrokenPipe = brokenPipe;
	}
	return outMessage;
}

NSpMessageHeader* NSpMessage_Get(NSpGameReference gameRef)
{
	NSpGame* game = NSpGame_Unbox(gameRef);
	NSpMessageHeader* message = NULL;
	bool brokenPipe = false;

	if (!game)
	{
		return NULL;
	}

	if (!game->isHosting)
	{
		message = PollSocket(game->clientToHostSocket, &brokenPipe);

		if (brokenPipe)
		{
			GAME_ASSERT(!message);

			// This socket is dead now
			CloseSocket(&game->clientToHostSocket);

			// Pass a fake NSpGameTerminatedMessage to application code so it knows to shut down the net game
			NSpGameTerminatedMessage* gameTerminated = (NSpGameTerminatedMessage*) AllocPtrClear(sizeof(NSpGameTerminatedMessage));
			gameTerminated->header.messageLen	= sizeof(NSpGameTerminatedMessage);
			gameTerminated->header.what			= kNSpGameTerminated;
			gameTerminated->reason				= kNSpGameTerminated_NetworkError;
			message = &gameTerminated->header;
		}
	}
	else
	{
		for (int i = 0; message == NULL && i < MAX_CLIENTS; i++)
		{
			NSpPlayer* player = &game->players[i];

			if (!IsSocketValid(player->sockfd))
			{
				continue;
			}

			message = PollSocket(player->sockfd, &brokenPipe);

			if (brokenPipe)
			{
				GAME_ASSERT(!message);

				// This socket is dead now
				CloseSocket(&player->sockfd);

				// Kick the client and tell others that this guy left
				NSpGame_KickClient(gameRef, player->id);
			}
			else if (message)
			{
				// Force client ID. The client may not know their ID yet,
				// and we don't want them to forge a bogus ID anyway.
				message->from = player->id;
				GAME_ASSERT(NSpGame_IsValidPlayerID(gameRef, message->from));

				// Forward broadcast messages
				if (message->to == kNSpAllPlayers)
				{
					// TODO: if we ever do UDP, we'll need to decide what to do with the flags below
					NSpMessage_Send(gameRef, message, kNSpSendFlag_Registered);
				}
			}
		}
	}

	// If we did get a message, handle internal message types
	if (message)
	{
		switch (message->what)
		{
			case kNSpJoinApproved:
			{
				GAME_ASSERT(!game->isHosting);
				game->myID = message->to;		// that's our ID
				break;
			}

			case kNSpPlayerJoined:
			{
				GAME_ASSERT(!game->isHosting);

				NSpPlayerJoinedMessage* joinedMessage = (NSpPlayerJoinedMessage*) message;

				NSpPlayer* newPlayer = NSpGame_GetPlayerFromID(gameRef, joinedMessage->playerInfo.id);
				GAME_ASSERT(newPlayer);

				newPlayer->id			= joinedMessage->playerInfo.id;
				newPlayer->state		= kNSpPlayerState_ConnectedPeer;
				newPlayer->sockfd		= INVALID_SOCKET;		// client has no p2p connection to other players
				CopyPlayerName(newPlayer->name, joinedMessage->playerInfo.name);

				break;
			}

			case kNSpPlayerLeft:
			{
				GAME_ASSERT(!game->isHosting);

				NSpPlayerLeftMessage* leftMessage = (NSpPlayerLeftMessage*) message;

				NSpPlayer* deadPlayer = NSpGame_GetPlayerFromID(gameRef, leftMessage->playerID);
				if (deadPlayer)
				{
					NSpPlayer_Clear(deadPlayer);
				}

				break;
			}
		}
	}

	return message;
}

int NSpGame_KickClient(NSpGameReference gameRef, NSpPlayerID kickedPlayerID)
{
	char playerNameBackup[kNSpPlayerNameLength];

	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	GAME_ASSERT(game->isHosting);
	GAME_ASSERT(kickedPlayerID != kNSpHostID);		// can't kick myself!

	NSpPlayer* kickedPlayer = NSpGame_GetPlayerFromID(gameRef, kickedPlayerID);
	if (kickedPlayer == NULL)
	{
		return kNSpRC_InvalidPlayer;
	}

	// If we did the initial handshake, we've already let the others know about this peer.
	// So we'll need to tell them that this one left.
	bool tellOthers = kickedPlayer->state == kNSpPlayerState_ConnectedPeer;

	// Send this peer an NSpGameTerminatedMessage if their socket is still live
	if (IsSocketValid(kickedPlayer->sockfd))
	{
		NSpGameTerminatedMessage byeMessage = {0};
		NSpClearMessageHeader(&byeMessage.header);
		byeMessage.header.messageLen	= sizeof(NSpGameTerminatedMessage);
		byeMessage.header.from			= kNSpHostID;
		byeMessage.header.to			= kickedPlayer->id;
		byeMessage.header.what			= kNSpGameTerminated;
		byeMessage.reason				= kNSpGameTerminated_YouGotKicked;
		NSpMessage_Send(gameRef, &byeMessage.header, kNSpSendFlag_Registered);
	}

	CopyPlayerName(playerNameBackup, kickedPlayer->name);

	CloseSocket(&kickedPlayer->sockfd);
	NSpPlayer_Clear(kickedPlayer);
	kickedPlayer = NULL;

	// Tell other players that this guy left
	if (tellOthers)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (!IsSocketValid(game->players[i].sockfd))
			{
				continue;
			}

			NSpPlayerLeftMessage leftMessage;
			NSpClearMessageHeader(&leftMessage.header);
			leftMessage.header.messageLen = sizeof(NSpPlayerLeftMessage);
			leftMessage.header.what = kNSpPlayerLeft;
			leftMessage.header.from = kNSpHostID;
			leftMessage.header.to = NSpGame_ClientSlotToID(gameRef, i);
			leftMessage.playerCount = NSpGame_GetNumActivePlayers(gameRef);
			leftMessage.playerID = kickedPlayerID;
			CopyPlayerName(leftMessage.playerName, playerNameBackup);

			NSpMessage_Send(gameRef, &leftMessage.header, kNSpSendFlag_Registered);
		}
	}

	return kNSpRC_OK;
}

int NSpGame_AckJoinRequest(NSpGameReference gameRef, NSpMessageHeader* message)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	GAME_ASSERT(game->isHosting);
	GAME_ASSERT(message->what == kNSpJoinRequest);

	NSpPlayerID newPlayerID = message->from;
	NSpPlayer* newPlayer = NSpGame_GetPlayerFromID(gameRef, newPlayerID);

	if (newPlayer == NULL)
	{
		return kNSpRC_InvalidPlayer;
	}

	if (newPlayer->state != kNSpPlayerState_AwaitingHandshake)
	{
		return kNSpRC_BadState;
	}

	NSpJoinRequestMessage* joinRequestMessage = (NSpJoinRequestMessage*) message;

	// Save their name
	CopyPlayerName(newPlayer->name, joinRequestMessage->name);

	// Tell them they're in
	NSpJoinApprovedMessage approvedMessage;
	NSpClearMessageHeader(&approvedMessage.header);
	approvedMessage.header.messageLen	= sizeof(NSpJoinApprovedMessage);
	approvedMessage.header.what			= kNSpJoinApproved;
	approvedMessage.header.from			= kNSpHostID;
	approvedMessage.header.to			= newPlayerID;

	int rc = NSpMessage_Send(game, &approvedMessage.header, kNSpSendFlag_Registered);
	if (rc != kNSpRC_OK)
	{
		return rc;
	}

	// Send a flurry of PlayerJoined messages to the new client so it knows who joined
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		NSpPlayer* player = &game->players[i];

		if (player->state == kNSpPlayerState_Offline		// skip over dead clients
			|| player->id == newPlayerID)					// skip over itself
		{
			continue;
		}

		NSpPlayerJoinedMessage joinedMessage;
		NSpClearMessageHeader(&joinedMessage.header);
		joinedMessage.header.messageLen	= sizeof(NSpPlayerJoinedMessage);
		joinedMessage.header.what		= kNSpPlayerJoined;
		joinedMessage.header.from		= kNSpHostID;
		joinedMessage.header.to			= newPlayerID;
		joinedMessage.playerCount		= 1 + NSpGame_GetNumActivePlayers(gameRef);
		joinedMessage.playerInfo.id		= NSpGame_ClientSlotToID(gameRef, i);
		CopyPlayerName(joinedMessage.playerInfo.name, game->players[i].name);

		rc = NSpMessage_Send(gameRef, &joinedMessage.header, kNSpSendFlag_Registered);
		if (rc != kNSpRC_OK)
		{
			return rc;
		}
	}

	// We've done the initial handshake
	newPlayer->state = kNSpPlayerState_ConnectedPeer;

	// Tell peers that someone joined
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		NSpPlayer* peerPlayer = &game->players[i];

		if (peerPlayer->state != kNSpPlayerState_ConnectedPeer
			|| peerPlayer->id == newPlayer->id)						// don't send joinedMessage to the new client
		{
			continue;
		}

		NSpPlayerJoinedMessage joinedMessage;
		NSpClearMessageHeader(&joinedMessage.header);
		joinedMessage.header.messageLen	= sizeof(NSpPlayerJoinedMessage);
		joinedMessage.header.what		= kNSpPlayerJoined;
		joinedMessage.header.from		= kNSpHostID;
		joinedMessage.header.to			= peerPlayer->id;
		joinedMessage.playerCount		= NSpGame_GetNumActivePlayers(gameRef);
		joinedMessage.playerInfo.id		= newPlayerID;
		CopyPlayerName(joinedMessage.playerInfo.name, newPlayer->name);

		NSpMessage_Send(gameRef, &joinedMessage.header, kNSpSendFlag_Registered);
	}

	return kNSpRC_OK;
}

void NSpMessage_Release(NSpGameReference gameRef, NSpMessageHeader* message)
{
	(void) gameRef;
	SafeDisposePtr((Ptr) message);
}

#pragma mark - Create and kill lobby

NSpGameReference NSpGame_Host(void)
{
	NSpGameReference gameRef = NULL;
	NSpGame *game = NULL;

	gameRef = NSpGame_Alloc();
	game = NSpGame_Unbox(gameRef);
	game->isHosting				= true;
	game->myID					= kNSpHostID;

	game->hostListenSocket		= CreateTCPSocket(true);

	if (!IsSocketValid(game->hostListenSocket))
	{
		goto fail;
	}

	int listenRC = listen(game->hostListenSocket, PENDING_CONNECTIONS_QUEUE);

	if (listenRC != 0)
	{
		goto fail;
	}

	// Create a player for myself
	NSpPlayer* me = NSpGame_GetPlayerFromID(game, kNSpHostID);
	if (me == NULL)
	{
		goto fail;
	}
	me->id			= kNSpHostID;
	me->state		= kNSpPlayerState_Me;
	me->sockfd		= INVALID_SOCKET;
	snprintf(me->name, sizeof(me->name), "HOST");

	return gameRef;

fail:
	NSpGame_Dispose(gameRef, 0);
	return NULL;
}

#pragma mark - Search for lobbies

static NSpSearch* NSpSearch_Unbox(NSpSearchReference searchRef)
{
	if (searchRef == NULL)
	{
		return NULL;
	}

	NSpSearch* searchPtr = (NSpSearch*) searchRef;

//	GAME_ASSERT(NSPGAME_COOKIE32 == gamePtr->cookie);

	return searchPtr;
}

int NSpSearch_Dispose(NSpSearchReference searchRef)
{
	NSpSearch* search = NSpSearch_Unbox(searchRef);

	if (!search)
	{
		return kNSpRC_OK;
	}

	CloseSocket(&search->listenSocket);

	SafeDisposePtr((Ptr) search);

	return kNSpRC_OK;
}

NSpSearchReference NSpSearch_StartSearchingForGameHosts(void)
{
	NSpSearch* search = (NSpSearch*) AllocPtrClear(sizeof(NSpSearch));

	search->listenSocket = CreateUDPBroadcastSocket();

	if (!IsSocketValid(search->listenSocket))
	{
		printf("%s: couldn't create socket\n", __func__);
		goto fail;
	}

	struct sockaddr_in bindAddr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(LOBBY_PORT),
		.sin_addr.s_addr = INADDR_ANY,
	};

	int bindRC = bind(
		search->listenSocket,
		(struct sockaddr*) &bindAddr,
		sizeof(bindAddr));

	if (0 != bindRC)
	{
		printf("%s: bind failed: %d\n", __func__, GetSocketError());

		if (GetSocketError() == kSocketError_AddressInUse)
		{
			printf("(addr in use)\n");
		}

		goto fail;
	}

	printf("Created lobby search\n");
	return search;

fail:


	SafeDisposePtr((Ptr) search);
	return NULL;
}

static bool NSpSearch_IsHostKnown(NSpSearchReference searchRef, const struct sockaddr_in* remoteAddr)
{
	NSpSearch* search = NSpSearch_Unbox(searchRef);

	if (!search)
	{
		return false;
	}

	for (int i = 0; i < search->numGamesFound; i++)
	{
		if (search->gamesFound[i].hostAddr.sin_addr.s_addr == remoteAddr->sin_addr.s_addr
			&& search->gamesFound[i].hostAddr.sin_port == remoteAddr->sin_port)
		{
			return true;
		}
	}

	return false;
}

int NSpSearch_Tick(NSpSearchReference searchRef)
{
	NSpSearch* search = NSpSearch_Unbox(searchRef);

	if (!search)
	{
		return kNSpRC_NoSearch;
	}

	if (!IsSocketValid(search->listenSocket))
	{
		return kNSpRC_InvalidSocket;
	}

	char message[kNSpMaxMessageLength];
	struct sockaddr_in remoteAddr;
	socklen_t remoteAddrLen = sizeof(remoteAddr);

	ssize_t receivedBytes = recvfrom(
		search->listenSocket,
		message,
		sizeof(message),
		0,
		(struct sockaddr*) &remoteAddr,
		&remoteAddrLen
	);

	if (receivedBytes == -1)
	{
		if (GetSocketError() == kSocketError_WouldBlock)
		{
			return kNSpRC_OK;
		}
		else
		{
			printf("%s: error %d\n", __func__, GetSocketError());
			return kNSpRC_RecvFailed;
		}
	}
	else
	{
		// TODO: Check payload!!

		if (search->numGamesFound < MAX_LOBBIES &&
			!NSpSearch_IsHostKnown(search, &remoteAddr))
		{
			char hostname[128];
			snprintf(hostname, sizeof(hostname), "[EMPTY]");
			inet_ntop(remoteAddr.sin_family, &remoteAddr.sin_addr, hostname, sizeof(hostname));
			printf("%s: Found a game! %s:%d\n", __func__, hostname, remoteAddr.sin_port);

			search->gamesFound[search->numGamesFound].hostAddr = remoteAddr;

			search->numGamesFound++;

			GAME_ASSERT(search->numGamesFound <= MAX_LOBBIES);
		}

		return kNSpRC_OK;
	}
}

int NSpSearch_GetNumGamesFound(NSpSearchReference searchRef)
{
	NSpSearch* search = NSpSearch_Unbox(searchRef);

	if (!search)
	{
		return 0;
	}

	return search->numGamesFound;
}

NSpGameReference NSpSearch_JoinGame(NSpSearchReference searchRef, int lobbyNum)
{
	NSpSearch* search = NSpSearch_Unbox(searchRef);

	if (!search)
	{
		return NULL;
	}

	GAME_ASSERT(lobbyNum >= 0);
	GAME_ASSERT(lobbyNum < search->numGamesFound);
	return JoinLobby(&search->gamesFound[lobbyNum]);
}

const char* NSpSearch_GetHostAddress(NSpSearchReference searchRef, int lobbyNum)
{
	NSpSearch* search = NSpSearch_Unbox(searchRef);

	if (!search)
	{
		return NULL;
	}

	GAME_ASSERT(lobbyNum >= 0);
	GAME_ASSERT(lobbyNum < search->numGamesFound);
	return FormatAddress(search->gamesFound[lobbyNum].hostAddr);
}

#pragma mark - NSpGame

static NSpGameReference NSpGame_Alloc(void)
{
	NSpGame* game = AllocPtrClear(sizeof(NSpGame));
	game = (NSpGameReference) game;

	game->hostListenSocket		= INVALID_SOCKET;
	game->hostAdvertiseSocket	= INVALID_SOCKET;
	game->clientToHostSocket	= INVALID_SOCKET;
	game->isHosting				= false;
	game->timeToReadvertise		= 0;
	game->myID					= kNSpUnspecifiedEndpoint;
	game->cookie				= NSPGAME_COOKIE32;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		NSpPlayer_Clear(&game->players[i]);
	}

	return (NSpGameReference) game;
}

static NSpGame* NSpGame_Unbox(NSpGameReference gameRef)
{
	if (gameRef == NULL)
	{
		return NULL;
	}

	NSpGame* gamePtr = (NSpGame*) gameRef;

	GAME_ASSERT(NSPGAME_COOKIE32 == gamePtr->cookie);

	return gamePtr;
}

int NSpGame_Dispose(NSpGameReference inGame, int disposeFlags)
{
	NSpGame* game = NSpGame_Unbox(inGame);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	// If we're terminating the game, tell people about it.
	if (disposeFlags & kNSpGameFlag_ForceTerminateGame)
	{
		NSpGameTerminatedMessage byeMessage = {0};
		NSpClearMessageHeader(&byeMessage.header);
		byeMessage.header.messageLen	= sizeof(NSpGameTerminatedMessage);
		byeMessage.header.what			= kNSpGameTerminated;
		byeMessage.header.to			= kNSpAllPlayers;
		byeMessage.header.from			= kNSpHostID;
		byeMessage.reason				= kNSpGameTerminated_HostBailed;
		NSpMessage_Send(inGame, &byeMessage.header, kNSpSendFlag_Registered);
	}

	CloseSocket(&game->clientToHostSocket);
	CloseSocket(&game->hostListenSocket);
	CloseSocket(&game->hostAdvertiseSocket);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		NSpPlayer* client = &game->players[i];
		CloseSocket(&client->sockfd);
		NSpPlayer_Clear(client);
	}

	game->cookie = 'DEAD';
	SafeDisposePtr((Ptr) game);

	return kNSpRC_OK;
}

int NSpGame_GetNumActivePlayers(NSpGameReference gameRef)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return 0;
	}

	int count = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		switch (game->players[i].state)
		{
			case kNSpPlayerState_Me:
			case kNSpPlayerState_ConnectedPeer:
				count++;
				break;

			default:
				break;
		}
	}

	return count;
}

bool NSpGame_IsValidPlayerID(NSpGameReference gameRef, NSpPlayerID id)
{
	if (id < kNSpClientID0 || id >= kNSpClientID0 + MAX_CLIENTS)
	{
		return false;
	}

	// TODO: Check that it's live?
	return true;
}

static NSpPlayerID NSpGame_ClientSlotToID(NSpGameReference gameRef, int slot)
{
	if (slot < 0 || slot >= MAX_CLIENTS)
	{
		return kNSpUnspecifiedEndpoint;
	}
	else
	{
		return slot + kNSpHostID;
	}
}

static int NSpGame_ClientIDToSlot(NSpGameReference gameRef, NSpPlayerID id)
{
	if (!NSpGame_IsValidPlayerID(gameRef, id))
	{
		return -1;
	}
	else
	{
		return id - kNSpHostID;
	}
}

static NSpPlayer* NSpGame_GetPlayerFromID(NSpGameReference gameRef, NSpPlayerID id)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return NULL;
	}

	int slot = NSpGame_ClientIDToSlot(gameRef, id);

	if (slot >= 0)
	{
		return &game->players[id];
	}
	else
	{
		return NULL;
	}
}

NSpPlayerID NSpGame_GetNthActivePlayerID(NSpGameReference gameRef, int n)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return kNSpUnspecifiedEndpoint;
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		NSpPlayer* player = &game->players[i];

		if (player->state == kNSpPlayerState_Offline
			|| player->state == kNSpPlayerState_AwaitingHandshake)
		{
			continue;
		}

		if (n == 0)
		{
			return player->id;
		}
		else
		{
			n--;
		}
	}

	return kNSpUnspecifiedEndpoint;
}

bool NSpGame_IsAdvertising(NSpGameReference gameRef)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	return game && game->isHosting && game->isAdvertising;
}

int NSpGame_StartAdvertising(NSpGameReference gameRef)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	if (!game->isHosting)
	{
		return kNSpRC_BadState;
	}

	if (game->isAdvertising)
	{
		return kNSpRC_OK;
	}

	game->hostAdvertiseSocket = CreateUDPBroadcastSocket();

	if (!IsSocketValid(game->hostAdvertiseSocket))
	{
		return kNSpRC_InvalidSocket;
	}

	game->isAdvertising = true;
	game->timeToReadvertise = 0;
	return kNSpRC_OK;
}

int NSpGame_StopAdvertising(NSpGameReference gameRef)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	if (!game->isHosting)
	{
		return kNSpRC_BadState;
	}

	if (!game->isAdvertising)
	{
		return kNSpRC_OK;
	}

	CloseSocket(&game->hostAdvertiseSocket);
	game->isAdvertising = false;
	game->timeToReadvertise = 0;

	return kNSpRC_OK;
}

int NSpGame_AdvertiseTick(NSpGameReference gameRef, float dt)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	if (!game->isHosting ||
		!game->isAdvertising)
	{
		return kNSpRC_BadState;
	}

	if (!IsSocketValid(game->hostAdvertiseSocket))
	{
		return kNSpRC_InvalidSocket;
	}

	game->timeToReadvertise -= dt;

	if (game->timeToReadvertise > 0)
	{
		return kNSpRC_OK;
	}

	// Fire the message

	game->timeToReadvertise = LOBBY_BROADCAST_INTERVAL;

	printf("%s: broadcasting message\n", __func__);

	const char* message = "JOIN MY CMR GAME";
	size_t messageLength = strlen(message);

	struct sockaddr_in broadcastAddr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(LOBBY_PORT),
		.sin_addr.s_addr = INADDR_BROADCAST,
	};

	ssize_t rc = sendto(
		game->hostAdvertiseSocket,
		message,
		messageLength,
		MSG_NOSIGNAL,
		(struct sockaddr*) &broadcastAddr,
		sizeof(broadcastAddr)
	);

	if (rc == -1)
	{
		printf("%s: sendto(%d) : error % d\n",
			__func__, (int) game->hostAdvertiseSocket, GetSocketError());
		return kNSpRC_SendFailed;
	}

	return kNSpRC_OK;
}

#pragma mark - NSpMessage

static int SendOnSocket(sockfd_t sockfd, NSpMessageHeader* header)
{
	GAME_ASSERT_MESSAGE(header->what != kNSpUndefinedMessage, "Did you forget to set header->what?");
	GAME_ASSERT_MESSAGE(header->messageLen != 0xBADBABEE, "Did you forget to set header->messageLen?");
	GAME_ASSERT(header->messageLen >= sizeof(NSpMessageHeader));
	GAME_ASSERT(header->messageLen <= kNSpMaxMessageLength);
	GAME_ASSERT(header->version == kNSpCMRProtocol4CC);

	if (!IsSocketValid(sockfd))
	{
		printf("%s: invalid socket %d\n", __func__, (int) sockfd);
		return kNSpRC_InvalidSocket;
	}

	ssize_t sendRC = send(
		sockfd,
		(char*) header,
		header->messageLen,
		MSG_NOSIGNAL
	);

	if (sendRC == -1)
	{
		printf("%s: error sending message on socket %d\n", __func__, (int) sockfd);
		return kNSpRC_SendFailed;
	}
	else
	{
		printf("%s: sent message '%s' (%d bytes) on socket %d\n",
			__func__, NSp4CCString(header->what), header->messageLen, (int) sockfd);
		return kNSpRC_OK;
	}
}

// Attempts to send a message.
// If we're the host and sending fails, automatically kicks the client.
// Kicking the client will in turn broadcast a PlayerLeftMessage to remaining clients,
// if the other clients already know the kicked client.
int NSpMessage_Send(NSpGameReference gameRef, NSpMessageHeader* header, int flags)
{
	NSpGame* game = NSpGame_Unbox(gameRef);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	if (header->from == kNSpUnspecifiedEndpoint)
	{
		header->from = game->myID;
	}

	GAME_ASSERT_MESSAGE(flags == kNSpSendFlag_Registered, "only reliable messages are supported");

	if (game->isHosting)
	{
		switch (header->to)
		{
			case kNSpAllPlayers:
			{
				int anyError = 0;
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					NSpPlayer* peer = &game->players[i];

					if (peer->state == kNSpPlayerState_ConnectedPeer	// send to peers with complete handshake
						&& peer->id != header->from)					// don't return to sender!
					{
						int rc = SendOnSocket(game->players[i].sockfd, header);
						if (rc != kNSpRC_OK)
						{
							anyError = rc;
							NSpPlayerID kickedPlayerID = NSpGame_ClientSlotToID(gameRef, i);
							NSpGame_KickClient(gameRef, kickedPlayerID);
						}
					}
				}
				return anyError;
			}

			case kNSpHostID:
				GAME_ASSERT_MESSAGE(false, "Host cannot send itself a message");
				break;

			default:
			{
				NSpPlayerID playerID = header->to;
				NSpPlayer* peer = NSpGame_GetPlayerFromID(gameRef, playerID);

				if (peer
					&& (peer->state == kNSpPlayerState_ConnectedPeer || peer->state == kNSpPlayerState_AwaitingHandshake))
				{
					int rc = SendOnSocket(peer->sockfd, header);
					if (rc != kNSpRC_OK)
					{
						NSpGame_KickClient(gameRef, playerID);
					}
					return rc;
				}
				else
				{
					return kNSpRC_InvalidPlayer;
				}
			}
		}
	}
	else
	{
		// If we're a client, forward ALL messages to the host.
		// The host will dispatch the message for us.

		int rc = SendOnSocket(game->clientToHostSocket, header);
		if (rc != kNSpRC_OK)
		{
			// TODO: Client couldn't send a message to the host! Kill the game?
			puts("Client couldn't send a message to the host! Kill the game?");
		}

		return rc;
	}
}

#pragma mark - NSpCMRClient

static void NSpPlayer_Clear(NSpPlayer* player)
{
	memset(player, 0, sizeof(NSpPlayer));
	player->id			= kNSpUnspecifiedEndpoint;
	player->sockfd		= INVALID_SOCKET;
	player->state		= kNSpPlayerState_Offline;
}
