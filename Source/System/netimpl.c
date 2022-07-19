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
	struct sockaddr_in				hostAddr;

	// Host: TCP socket to listen on
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
int gNumLobbiesFound = 0;

static NSpGameReference gHostingGame = NULL;

static uint32_t gOutboundMessageCounter = 1;

static bool IsKnownLobbyAddress(const struct sockaddr_in* remoteAddr);
static void JoinLobby(const LobbyInfo* lobby);
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
			printf("%s: Found a game! %s:%d\n", __func__,
				   inet_ntoa(remoteAddr.sin_addr),
				   remoteAddr.sin_port);

			gLobbiesFound[gNumLobbiesFound].hostAddr = remoteAddr;

			gNumLobbiesFound++;
			GAME_ASSERT(gNumLobbiesFound <= MAX_LOBBIES);

			if (gNumLobbiesFound == 1)
			{
				// Try joining that one
				JoinLobby(&gLobbiesFound[0]);
			}
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

static void JoinLobby(const LobbyInfo* lobby)
{
	int sock = CreateGameSocket(false);

	struct sockaddr_in bindAddr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(LOBBY_PORT),//lobby->hostAddr.sin_port,
		.sin_addr.s_addr = lobby->hostAddr.sin_addr.s_addr,
	};

	int connectRC = connect(sock, (const struct sockaddr*) &bindAddr, sizeof(bindAddr));

	if (connectRC == -1)
	{
		printf("%s: connect failed: %d\n", __func__, GetSocketError());
		return;
	}

	NSpJoinRequestMessage message;
	NSpClearMessageHeader(&message.header);
	message.header.to = kNSpHostOnly;
	message.header.what = kNSpJoinRequest;
	message.header.messageLen = sizeof(NSpJoinRequestMessage);
	snprintf(message.name, sizeof(message.name), "CLIENT");

	ssize_t rc = send( //sendto(
			sock,
			(const char*) &message,
			sizeof(message),
			MSG_NOSIGNAL
//			(struct sockaddr*) &bindAddr,
//			sizeof(bindAddr)
			);

	if (rc == -1)
	{
		puts("Error sending");
	}
}

#pragma mark - Host lobby

static bool AcceptNewClient(NSpGameReference gameRef)
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

	return true;

fail:
	if (IsSocketValid(newSocket))
	{
		closesocket(newSocket);
		newSocket = INVALID_SOCKET;
	}
	return false;
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

void Net_CreateLobbySearch(void)
{
	gNumLobbiesFound = 0;
	memset(gLobbiesFound, 0, sizeof(gLobbiesFound));

	if (!CreateLobbyBroadcastSocket())
	{
		printf("%s: couldn't create socket\n", __func__);
		return;
	}

	gLobbyBroadcast.boundForListening = true;

	struct sockaddr_in bindAddr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(LOBBY_PORT),
		.sin_addr = {INADDR_ANY},
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
		return;
	}

	printf("Created lobby search\n");
}

void Net_CloseLobbySearch(void)
{
	DisposeLobbyBroadcastSocket();
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
		if (AcceptNewClient(gHostingGame))
		{
			printf("Just accepted a new client!\n");
		}

		NSpMessageHeader* hello = NSpMessage_Get(gHostingGame);
	}
}
