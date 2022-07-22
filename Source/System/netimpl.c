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
#define SOCKFMT "%llu"
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define closesocket(x) close(x)
#define SOCKFMT "%d"
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

typedef struct
{
	sockfd_t sockfd;
	bool boundForListening;
	float timeUntilNextBroadcast;
} LobbyBroadcast;

typedef struct
{
	sockfd_t					sockfd;
	char						name[32];
	bool						didJoinRequestHandshake;
} NSpCMRClient;

typedef struct NSpCMRGame
{
	sockfd_t					hostListenSocket;
	sockfd_t					clientToHostSocket;

	bool						isHosting;
	bool						isAdvertising;

	int							numClients;
	NSpCMRClient				clients[MAX_CLIENTS];

	uint32_t					cookie;
} NSpCMRGame;

typedef struct
{
	struct sockaddr_in hostAddr;
} LobbyInfo;

static LobbyBroadcast gLobbyBroadcast =
{
	.sockfd = INVALID_SOCKET,
	.timeUntilNextBroadcast = 0,
};

static LobbyInfo gLobbiesFound[MAX_LOBBIES];
static int gNumLobbiesFound = 0;

static uint32_t gOutboundMessageCounter = 1;

static int gLastQueriedSocketError = 0;

static bool IsKnownLobbyAddress(const struct sockaddr_in* remoteAddr);
static NSpGameReference JoinLobby(const LobbyInfo* lobby);
static sockfd_t CreateGameSocket(bool bindIt);
static NSpCMRGame* UnboxGameReference(NSpGameReference gameRef);
static NSpGameReference NSpGame_Alloc(void);
static void NSpCMRClient_Clear(NSpCMRClient* client);
static int SendOnSocket(sockfd_t sockfd, NSpMessageHeader* header);

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

static const char* FormatAddress(struct sockaddr_in hostAddr)
{
	static char hostname[128];

	snprintf(hostname, sizeof(hostname), "[EMPTY]");
	inet_ntop(hostAddr.sin_family, &hostAddr.sin_addr, hostname, sizeof(hostname));

	snprintfcat(hostname, sizeof(hostname), ":%d", hostAddr.sin_port);

	return hostname;
}

static const char* FourCCToString(uint32_t fourcc)
{
	static char cstr[5];
	cstr[0] = (fourcc >> 24) & 0xFF;
	cstr[1] = (fourcc >> 16) & 0xFF;
	cstr[2] = (fourcc >> 8) & 0xFF;
	cstr[3] = (fourcc) & 0xFF;
	return cstr;
}

#pragma mark - Lobby broadcast

bool Net_IsLobbyBroadcastOpen(void)
{
	return IsSocketValid(gLobbyBroadcast.sockfd);
}

static bool CreateLobbyBroadcastSocket(void)
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

	gLobbyBroadcast.sockfd = sockfd;
	gLobbyBroadcast.boundForListening = false;
	gLobbyBroadcast.timeUntilNextBroadcast = 0;

	printf("%s: socket " SOCKFMT "\n", __func__, gLobbyBroadcast.sockfd);
	return true;

fail:
	if (IsSocketValid(sockfd))
	{
		closesocket(sockfd);
		sockfd = INVALID_SOCKET;
	}
	return false;
}

static void DisposeLobbyBroadcastSocket(void)
{
	if (IsSocketValid(gLobbyBroadcast.sockfd))
	{
		closesocket(gLobbyBroadcast.sockfd);
	}

	gLobbyBroadcast.sockfd = INVALID_SOCKET;
	gLobbyBroadcast.timeUntilNextBroadcast = 0;
}

static void SendLobbyBroadcastMessage(void)
{
	gLobbyBroadcast.timeUntilNextBroadcast -= gFramesPerSecondFrac;

	if (gLobbyBroadcast.timeUntilNextBroadcast > 0.0f)
	{
		return;
	}

	gLobbyBroadcast.timeUntilNextBroadcast = LOBBY_BROADCAST_INTERVAL;

	printf("%s...\n", __func__);

	const char* message = "JOIN MY CMR GAME";
	size_t messageLength = strlen(message);

	struct sockaddr_in broadcastAddr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(LOBBY_PORT),
		.sin_addr.s_addr = INADDR_BROADCAST,
	};

	ssize_t rc = sendto(
		gLobbyBroadcast.sockfd,
		message,
		messageLength,
		MSG_NOSIGNAL,
		(struct sockaddr*) &broadcastAddr,
		sizeof(broadcastAddr)
	);

	if (rc == -1)
	{
		printf("%s: sendto(" SOCKFMT ") : error % d\n", __func__, gLobbyBroadcast.sockfd, GetSocketError());
	}
}

static void ReceiveLobbyBroadcastMessage(void)
{
	char buf[256];
	struct sockaddr_in remoteAddr;
	socklen_t remoteAddrLen = sizeof(remoteAddr);

	ssize_t receivedBytes = recvfrom(
		gLobbyBroadcast.sockfd,
		buf,
		sizeof(buf),
		0,
		(struct sockaddr*) &remoteAddr,
		&remoteAddrLen
	);

	if (receivedBytes == -1)
	{
		if (GetSocketError() == kSocketError_WouldBlock)
		{
			return;
		}
		else
		{
			printf("%s: error %d\n", __func__, GetSocketError());
		}
	}
	else
	{
		if (gNumLobbiesFound < MAX_LOBBIES &&
			!IsKnownLobbyAddress(&remoteAddr))
		{
			char hostname[128];
			snprintf(hostname, sizeof(hostname), "[EMPTY]");
			inet_ntop(remoteAddr.sin_family, &remoteAddr.sin_addr, hostname, sizeof(hostname));
			printf("%s: Found a game! %s:%d\n", __func__, hostname, remoteAddr.sin_port);

			gLobbiesFound[gNumLobbiesFound].hostAddr = remoteAddr;

			gNumLobbiesFound++;
			GAME_ASSERT(gNumLobbiesFound <= MAX_LOBBIES);

#if 0
			if (gNumLobbiesFound == 1)
			{
				// Try joining that one
				JoinLobby(&gLobbiesFound[0]);
			}
#endif
		}
	}
}

#pragma mark - Join lobby

static bool IsKnownLobbyAddress(const struct sockaddr_in* remoteAddr)
{
	for (int i = 0; i < gNumLobbiesFound; i++)
	{
		if (gLobbiesFound[i].hostAddr.sin_addr.s_addr == remoteAddr->sin_addr.s_addr
			&& gLobbiesFound[i].hostAddr.sin_port == remoteAddr->sin_port)
		{
			return true;
		}
	}

	return false;
}

static NSpGameReference JoinLobby(const LobbyInfo* lobby)
{
	printf("%s: %s\n", __func__, FormatAddress(lobby->hostAddr));

	int sockfd = INVALID_SOCKET;
	
	sockfd = CreateGameSocket(false);
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
	message.header.to = kNSpHostOnly;
	message.header.what = kNSpJoinRequest;
	message.header.messageLen = sizeof(NSpJoinRequestMessage);
	snprintf(message.name, sizeof(message.name), "CLIENT");

	int rc = SendOnSocket(sockfd, &message.header);

	if (rc != kNSpRC_OK)
	{
		goto fail;
	}

	NSpGameReference gameRef = NSpGame_Alloc();
	NSpCMRGame* game = UnboxGameReference(gameRef);
	game->isHosting = false;
	game->clientToHostSocket = sockfd;
	return gameRef;

fail:
	if (IsSocketValid(sockfd))
	{
		closesocket(sockfd);
		sockfd = INVALID_SOCKET;
	}
	return NULL;
}

#pragma mark - Host lobby

int NSpGame_AcceptNewClient(NSpGameReference gameRef)
{
	int newClient = -1;
	NSpCMRGame* game = UnboxGameReference(gameRef);

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

	if (game->numClients < MAX_PLAYERS)
	{
		newClient = game->numClients;

		game->clients[newClient].sockfd = newSocket;
		game->clients[newClient].didJoinRequestHandshake = false;
		snprintf(game->clients[newClient].name, sizeof(game->clients[newClient].name), "PLAYER %d", newClient);

		game->numClients++;
	}
	else
	{
		// All slots used up
		printf("%s: A new client wants to connect, but the game is full!\n", __func__);

		NSpJoinDeniedMessage deniedMessage = {0};
		NSpClearMessageHeader(&deniedMessage.header);

		deniedMessage.header.from = kNSpHostID;
		deniedMessage.header.what = kNSpJoinDenied;
		deniedMessage.header.messageLen = sizeof(NSpMessageHeader);
		snprintf(deniedMessage.reason, sizeof(deniedMessage.reason), "THE GAME IS FULL.");

		SendOnSocket(newSocket, &deniedMessage.header);

		goto fail;
	}

	printf("%s: Accepted client #%d\n", __func__, newClient);

	return game->numClients - 1;

fail:
	if (IsSocketValid(newSocket))
	{
		closesocket(newSocket);
		newSocket = INVALID_SOCKET;
	}
	return -1;
}

//void NSpGame_

#pragma mark - Message socket

static sockfd_t CreateGameSocket(bool bindIt)
{
	sockfd_t sockfd = INVALID_SOCKET;
	struct addrinfo* res = NULL;

	struct addrinfo hints =
	{
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_PASSIVE
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

	return sockfd;

fail:
	if (res != NULL)
	{
		freeaddrinfo(res);
		res = NULL;
	}
	if (IsSocketValid(sockfd))
	{
		closesocket(sockfd);
		sockfd = INVALID_SOCKET;
	}
	return INVALID_SOCKET;
}

#pragma mark - Basic message

void NSpClearMessageHeader(NSpMessageHeader* h)
{
	memset(h, 0, sizeof(NSpMessageHeader));
	h->version = kNSpCMRProtocol4CC;
	h->when = 0;
	h->id = gOutboundMessageCounter;
	h->from = kNSpUnspecifiedEndpoint;
	h->to = kNSpUnspecifiedEndpoint;
	h->messageLen = sizeof(NSpMessageHeader);

	gOutboundMessageCounter++;
}

static bool CheckIncomingMessage(const void* message, ssize_t length)
{
	const NSpMessageHeader* header = (const NSpMessageHeader*) message;

	if (length < (ssize_t) sizeof(NSpMessageHeader)			// buffer should be big enough for header before we read from it
		|| header->version != kNSpCMRProtocol4CC			// bad protocol magic
		|| (ssize_t) header->messageLen < length			// buffer should be big enough for full message
		)
	{
		return false;
	}

	return true;
}

static NSpMessageHeader* PollSocket(sockfd_t sockfd)
{
	char payload[256];

	ssize_t recvRC = recv(
		sockfd,
		payload,
		sizeof(payload),
		MSG_NOSIGNAL
	);

	// TODO: if received 0 bytes, the client is probably gone
	if (recvRC != -1 && recvRC != 0)
	{
		printf("Got %d bytes from socket %d - %d\n", (int)recvRC, sockfd, GetSocketError());

		if (CheckIncomingMessage(payload, recvRC))
		{
			char* buf = AllocPtr(recvRC);
			memcpy(buf, payload, recvRC);
			printf("Got message '%s'\n", FourCCToString(((NSpMessageHeader*)buf)->what));
			return (NSpMessageHeader*) buf;
		}
	}

	return NULL;
}

NSpMessageHeader* NSpMessage_Get(NSpGameReference gameRef)
{
	NSpCMRGame* game = UnboxGameReference(gameRef);

	if (game->isHosting)
	{
		for (int i = 0; i < game->numClients; i++)
		{
			NSpMessageHeader* message = PollSocket(game->clients[i].sockfd);

			if (message)
			{
				// Force client ID. The client may not know their ID yet,
				// and we don't want them to forge a bogus ID anyway.
				message->from = NSpGame_ClientSlotToID(gameRef, i);
				GAME_ASSERT(NSpGame_IsValidClientID(gameRef, message->from));

				return message;
			}
		}

		return NULL;
	}
	else
	{
		return PollSocket(game->clientToHostSocket);
	}
}

int NSpGame_AckJoinRequest(NSpGameReference gameRef, NSpMessageHeader* message)
{
	NSpCMRGame* game = UnboxGameReference(gameRef);

	GAME_ASSERT(game->isHosting);
	GAME_ASSERT(message->what == kNSpJoinRequest);

	int clientSlot = NSpGame_ClientIDToSlot(gameRef, message->from);
	if (clientSlot < 0)
	{
		return kNSpRC_InvalidClient;
	}

	NSpJoinRequestMessage* joinRequestMessage = (NSpJoinRequestMessage*) message;

	// Save their name
	snprintf(game->clients[clientSlot].name, kNSpPlayerNameLength, "%s", joinRequestMessage->name);

	// We've done the initial handshake
	game->clients[clientSlot].didJoinRequestHandshake = true;

	// Tell them they're in
	NSpJoinApprovedMessage approvedMessage;
	NSpClearMessageHeader(&approvedMessage.header);

	approvedMessage.header.what = kNSpJoinApproved;
	approvedMessage.header.from = kNSpHostOnly;
	approvedMessage.header.to = message->from;
	approvedMessage.header.messageLen = sizeof(NSpJoinApprovedMessage);

	return NSpMessage_Send(game, &approvedMessage.header, kNSpSendFlag_Registered);
}

void NSpMessage_Release(NSpGameReference gameRef, NSpMessageHeader* message)
{
	(void) gameRef;
	SafeDisposePtr((Ptr) message);
}

#pragma mark - Create and kill lobby

NSpGameReference Net_CreateHostedGame(void)
{
//	GAME_ASSERT_MESSAGE(gHostingGame == NULL, "Already hosting a game");

	NSpGameReference gameRef = NULL;
	NSpCMRGame *game = NULL;

	if (!CreateLobbyBroadcastSocket())
	{
		goto fail;
	}

	gameRef = NSpGame_Alloc();
	game = UnboxGameReference(gameRef);
	game->isHosting = true;
	game->hostListenSocket = CreateGameSocket(true);

	if (!IsSocketValid(game->hostListenSocket))
	{
		goto fail;
	}

	int listenRC = listen(game->hostListenSocket, PENDING_CONNECTIONS_QUEUE);

	if (listenRC != 0)
	{
		goto fail;
	}

	return gameRef;

fail:
	DisposeLobbyBroadcastSocket();

	NSpGame_Dispose(gameRef, 0);
	gameRef = NULL;
	game = NULL;

	return NULL;
}

void Net_CloseLobby(void)
{
	DisposeLobbyBroadcastSocket();
}

#pragma mark - Search for lobbies

bool Net_CreateLobbySearch(void)
{
	gNumLobbiesFound = 0;
	memset(gLobbiesFound, 0, sizeof(gLobbiesFound));

	if (!CreateLobbyBroadcastSocket())
	{
		printf("%s: couldn't create socket\n", __func__);
		return false;
	}

	gLobbyBroadcast.boundForListening = true;

	struct sockaddr_in bindAddr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(LOBBY_PORT),
		.sin_addr.s_addr = INADDR_ANY,
	};
	int bindRC = bind(gLobbyBroadcast.sockfd, (struct sockaddr*) &bindAddr, sizeof(bindAddr));
	if (0 != bindRC)
	{
		printf("%s: bind failed: %d\n", __func__, GetSocketError());

		if (GetSocketError() == kSocketError_AddressInUse)
		{
			printf("(addr in use)\n");
		}

		DisposeLobbyBroadcastSocket();
		return false;
	}

	printf("Created lobby search\n");
	return true;
}

void Net_CloseLobbySearch(void)
{
	DisposeLobbyBroadcastSocket();
}

int Net_GetNumLobbiesFound(void)
{
	return gNumLobbiesFound;
}

NSpGameReference Net_JoinLobby(int lobbyNum)
{
	GAME_ASSERT(lobbyNum >= 0);
	GAME_ASSERT(lobbyNum < gNumLobbiesFound);
	return JoinLobby(&gLobbiesFound[lobbyNum]);
}

const char* Net_GetLobbyAddress(int lobbyNum)
{
	GAME_ASSERT(lobbyNum >= 0);
	GAME_ASSERT(lobbyNum < gNumLobbiesFound);
	return FormatAddress(gLobbiesFound[lobbyNum].hostAddr);
}

#pragma mark - NSpGame

static NSpGameReference NSpGame_Alloc(void)
{
	NSpCMRGame* game = AllocPtrClear(sizeof(NSpCMRGame));
	game = (NSpGameReference) game;

	game->cookie = NSPGAME_COOKIE32;
	game->hostListenSocket = INVALID_SOCKET;
	game->clientToHostSocket = INVALID_SOCKET;
	game->numClients = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		NSpCMRClient_Clear(&game->clients[i]);
	}

	return (NSpGameReference) game;
}

static NSpCMRGame* UnboxGameReference(NSpGameReference gameRef)
{
	if (gameRef == NULL)
	{
		return NULL;
	}

	NSpCMRGame* gamePtr = (NSpCMRGame*) gameRef;

	GAME_ASSERT(NSPGAME_COOKIE32 == gamePtr->cookie);

	return gamePtr;
}

int NSpGame_Dispose(NSpGameReference inGame, int disposeFlags)
{
	NSpCMRGame* game = UnboxGameReference(inGame);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	if (IsSocketValid(game->clientToHostSocket))
	{
		closesocket(game->clientToHostSocket);
		game->clientToHostSocket = INVALID_SOCKET;
	}

	if (IsSocketValid(game->hostListenSocket))
	{
		closesocket(game->hostListenSocket);
		game->hostListenSocket = INVALID_SOCKET;
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		NSpCMRClient* client = &game->clients[i];
		if (IsSocketValid(client->sockfd))
		{
			closesocket(client->sockfd);
			NSpCMRClient_Clear(client);
		}
	}
	game->numClients = 0;

	game->cookie = 'DEAD';
	SafeDisposePtr((Ptr) game);

	return kNSpRC_OK;
}

int NSpGame_GetNumClients(NSpGameReference inGame)
{
	NSpCMRGame* game = UnboxGameReference(inGame);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	// TODO: only return # of clients with a complete handshake?
	return game->numClients;
}

bool NSpGame_IsValidClientID(NSpGameReference gameRef, NSpPlayerID id)
{
	if (id < kNSpClientID0 || id >= kNSpClientID0 + MAX_CLIENTS)
	{
		return false;
	}

	// TODO: Check that it's live?
	return true;
}

NSpPlayerID NSpGame_ClientSlotToID(NSpGameReference gameRef, int slot)
{
	if (slot < 0 || slot >= MAX_CLIENTS)
	{
		return kNSpRC_InvalidClient;
	}
	else
	{
		return slot + kNSpClientID0;
	}
}

int NSpGame_ClientIDToSlot(NSpGameReference gameRef, NSpPlayerID id)
{
	if (!NSpGame_IsValidClientID(gameRef, id))
	{
		return kNSpUnspecifiedEndpoint;
	}
	else
	{
		return id - kNSpClientID0;
	}
}

#pragma mark - NSpMessage

static int SendOnSocket(sockfd_t sockfd, NSpMessageHeader* header)
{
	GAME_ASSERT(header->messageLen >= sizeof(NSpMessageHeader));

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
			__func__, FourCCToString(header->what), header->messageLen, (int) sockfd);
		return kNSpRC_OK;
	}
}

int NSpMessage_Send(NSpGameReference gameRef, NSpMessageHeader* header, int flags)
{
	NSpCMRGame* game = UnboxGameReference(gameRef);

	if (!game)
	{
		return kNSpRC_NoGame;
	}

	GAME_ASSERT_MESSAGE(flags == kNSpSendFlag_Registered, "only reliable messages are supported");

	if (game->isHosting)
	{
		switch (header->to)
		{
			case kNSpAllPlayers:
				GAME_ASSERT_MESSAGE(false, "implement me: send to all players");
				break;

			case kNSpHostID:
			case kNSpHostOnly:
				GAME_ASSERT_MESSAGE(false, "Host cannot send itself a message");
				break;

			default:
			{
				int clientSlot = NSpGame_ClientIDToSlot(gameRef, header->to);

				GAME_ASSERT(clientSlot >= 0);
				GAME_ASSERT(clientSlot < MAX_CLIENTS);

				return SendOnSocket(game->clients[clientSlot].sockfd, header);
			}
		}
	}
	else
	{
//		GAME_ASSERT_MESSAGE(header->to == kNSpHostOnly || header->to == kNSpHostID,
//			"Clients may only communicate with the host");

		return SendOnSocket(game->clientToHostSocket, header);
	}
}

#pragma mark - NSpCMRClient

static void NSpCMRClient_Clear(NSpCMRClient* client)
{
	memset(client, 0, sizeof(NSpCMRClient));
	client->sockfd = INVALID_SOCKET;
}

#pragma mark - Network system tick

void Net_Tick(void)
{
	if (Net_IsLobbyBroadcastOpen())
	{
		if (!gLobbyBroadcast.boundForListening)
		{
			SendLobbyBroadcastMessage();
		}
		else
		{
			ReceiveLobbyBroadcastMessage();
		}
	}
}
