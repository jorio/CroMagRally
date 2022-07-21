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
	// Host: TCP socket to listen on
	// Client: TCP socket to talk to host
	sockfd_t						sockfd;

	bool							isHosting;
	bool							isAdvertising;

	int								numClientsConnected;
	int								hostToClientSockets[MAX_CLIENTS];

	uint32_t						cookie;
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

static NSpGameReference gHostingGame = NULL;

static uint32_t gOutboundMessageCounter = 1;

static bool IsKnownLobbyAddress(const struct sockaddr_in* remoteAddr);
static NSpGameReference JoinLobby(const LobbyInfo* lobby);
static sockfd_t CreateGameSocket(bool bindIt);
static NSpCMRGame* UnboxGameReference(NSpGameReference gameRef);
static NSpGameReference NSpGame_Alloc(void);

#pragma mark - Cross-platform compat

int GetSocketError(void)
{
#if _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
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

	NSpJoinRequestMessage message;
	NSpClearMessageHeader(&message.header);
	message.header.to = kNSpHostOnly;
	message.header.what = kNSpJoinRequest;
	message.header.messageLen = sizeof(NSpJoinRequestMessage);
	snprintf(message.name, sizeof(message.name), "CLIENT");

	ssize_t rc = send( //sendto(
			sockfd,
			(const char*) &message,
			sizeof(message),
			MSG_NOSIGNAL
//			(struct sockaddr*) &bindAddr,
//			sizeof(bindAddr)
			);

	if (rc == -1)
	{
		printf("%s: Error sending message\n", __func__);
		goto fail;
	}

	NSpGameReference gameRef = NSpGame_Alloc();
	NSpCMRGame* game = UnboxGameReference(gameRef);
	game->isHosting = false;
	game->sockfd = sockfd;
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

static int AcceptNewClient(NSpGameReference gameRef)
{
	NSpCMRGame* game = UnboxGameReference(gameRef);

	sockfd_t newSocket = INVALID_SOCKET;

	newSocket = accept(game->sockfd, NULL, NULL);

	if (!IsSocketValid(newSocket))	// nobody's trying to connect right now
	{
		goto fail;
	}

	printf("A new player's knocking at the door...\n");

	// make the socket non-blocking
	if (!MakeSocketNonBlocking(newSocket))
	{
		goto fail;
	}

	if (game->numClientsConnected < MAX_PLAYERS)
	{
		game->hostToClientSockets[game->numClientsConnected] = newSocket;
		game->numClientsConnected++;
	}
	else
	{
		// All slots used up
		goto fail;
	}

	return game->numClientsConnected - 1;

fail:
	if (IsSocketValid(newSocket))
	{
		closesocket(newSocket);
		newSocket = INVALID_SOCKET;
	}
	return -1;
}

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

bool CheckIncomingMessage(const void* message, ssize_t length)
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

void NSpClearMessageHeader(NSpMessageHeader* h)
{
	memset(h, 0, sizeof(NSpMessageHeader));
	h->version = kNSpCMRProtocol4CC;
	h->when = 0;
	h->id = gOutboundMessageCounter;

	gOutboundMessageCounter++;
}

NSpMessageHeader* NSpMessage_Get(NSpGameReference gameRef)
{
	char msg[256];

	NSpCMRGame* game = UnboxGameReference(gameRef);

	if (game->isHosting)
	{
		for (int i = 0; i < game->numClientsConnected; i++)
		{
			ssize_t recvRC = recv(
					game->hostToClientSockets[i],
					msg,
					sizeof(msg),
					MSG_NOSIGNAL
					);

			// TODO: if received 0 bytes, the client is probably gone
			if (recvRC != -1 && recvRC != 0)
			{
				printf("Got %d bytes from client %d - %d\n", recvRC, i, GetSocketError());

				if (CheckIncomingMessage(msg, recvRC))
				{
					char* buf = AllocPtr(recvRC);
					memcpy(buf, msg, recvRC);
					return (NSpMessageHeader*) buf;
				}

//				OnIncomingMessage(msg, recvRC);
			}
		}
	}

	return NULL;
}

#pragma mark - Create and kill lobby

NSpGameReference Net_CreateLobby(void)
{
	GAME_ASSERT_MESSAGE(gHostingGame == NULL, "Already hosting a game");

	gHostingGame = NSpGame_Alloc();

	if (!CreateLobbyBroadcastSocket())
	{
		goto fail;
	}

	NSpCMRGame* game = UnboxGameReference(gHostingGame);
	game->isHosting = true;
	game->sockfd = CreateGameSocket(true);
	printf("Host TCP socket: %d\n", (int) game->sockfd);

	if (!IsSocketValid(game->sockfd))
	{
		goto fail;
	}

	int listenRC = listen(game->sockfd, PENDING_CONNECTIONS_QUEUE);
	printf("ListenRC: %d\n", listenRC);

	if (listenRC != 0)
	{
		goto fail;
	}

	return gHostingGame;

fail:
	DisposeLobbyBroadcastSocket();

	NSpGame_Dispose(gHostingGame, 0);
	gHostingGame = NULL;

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
	gHostingGame = (NSpGameReference) game;

	game->cookie = NSPGAME_COOKIE32;
	game->sockfd = INVALID_SOCKET;
	game->numClientsConnected = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		game->hostToClientSockets[i] = INVALID_SOCKET;
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
		return noErr;
	}

	if (IsSocketValid(game->sockfd))
	{
		closesocket(game->sockfd);
		game->sockfd = INVALID_SOCKET;
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (IsSocketValid(game->hostToClientSockets[i]))
		{
			closesocket(game->hostToClientSockets[i]);
			game->hostToClientSockets[i] = INVALID_SOCKET;
		}
	}
	game->numClientsConnected = 0;

	game->cookie = 'DEAD';
	SafeDisposePtr((Ptr) game);

	return noErr;
}

int NSpGame_GetNumPlayersConnectedToHost(NSpGameReference inGame)
{
	NSpCMRGame* game = UnboxGameReference(inGame);

	if (!game)
	{
		return 0;
	}

	return game->numClientsConnected;
}

#pragma mark - NSpMessage

int NSpMessage_Send(NSpGameReference gameRef, NSpMessageHeader* header, int flags)
{
	NSpCMRGame* game = UnboxGameReference(gameRef);

	if (!game)
	{
		return 1;
	}

	GAME_ASSERT(header->messageLen >= sizeof(NSpMessageHeader));
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
				int clientSlot = header->to - kNSpClientID0;

				GAME_ASSERT(clientSlot >= 0);
				GAME_ASSERT(clientSlot < MAX_CLIENTS);

				if (!IsSocketValid(game->hostToClientSockets[clientSlot]))
				{
					printf("%s: invalid socket for client %d\n", __func__, clientSlot);
					return 2;
				}

				ssize_t sendRC = send(
					game->hostToClientSockets[clientSlot],
					(char*) header,
					header->messageLen,
					MSG_NOSIGNAL
				);

				if (sendRC == -1)
				{
					printf("%s: error sending message to client %d\n", __func__, clientSlot);
				}
				else
				{
					printf("%s: sent message %d (%d bytes) to client %d\n", __func__, header->what, header->messageLen, clientSlot);
				}

				break;
			}
		}
	}
	else
	{
		GAME_ASSERT_MESSAGE(header->to == kNSpHostOnly || header->to == kNSpHostID,
			"Clients may only communicate with the host");

		IMPLEMENT_ME_SOFT();
	}

	return 0;
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

	if (gHostingGame)
	{
		int newClient = AcceptNewClient(gHostingGame);
		if (newClient >= 0)
		{
			printf("Accepted client #%d\n", newClient);
		}

		NSpMessageHeader* hello = NSpMessage_Get(gHostingGame);
	}
}
