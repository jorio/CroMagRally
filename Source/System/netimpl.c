/****************************/
/* LOW-LEVEL NETWORKING     */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>

#include "game.h"
#include "network.h"

#define LOBBY_PORT 21812
#define LOBBY_PORT_STR "21812"
#define LOBBY_BROADCAST_INTERVAL 1.0f

#define PENDING_CONNECTIONS_QUEUE 10

#define MAX_LOBBIES 5

#define NSPGAME_COOKIE32 'NSpG'

typedef struct
{
	int sockfd;
	bool boundForListening;
	float timeUntilNextBroadcast;
} LobbyBroadcast;

typedef struct
{
	struct sockaddr_in				hostAddr;

	// Host: TCP socket to listen on
	int								sockfd;

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
	.sockfd = -1,
	.timeUntilNextBroadcast = 0,
};

static LobbyInfo gLobbiesFound[MAX_LOBBIES];
int gNumLobbiesFound = 0;

static NSpGameReference gHostingGame = NULL;

static uint32_t gOutboundMessageCounter = 1;

static bool IsKnownLobbyAddress(const struct sockaddr_in* remoteAddr);
static void JoinLobby(const LobbyInfo* lobby);
static int CreateGameSocket(bool bindIt);
static NSpCMRGame* UnboxGameReference(NSpGameReference gameRef);
static NSpGameReference NSpGame_Alloc(void);

#pragma mark - Lobby broadcast

bool Net_IsLobbyBroadcastOpen(void)
{
	return gLobbyBroadcast.sockfd >= 0;
}

static bool CreateLobbyBroadcastSocket(void)
{
	int sockfd = -1;

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (-1 == sockfd)
	{
		goto fail;
	}

	int broadcast = 1;
	int sockoptRC = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	if (-1 == sockoptRC)
	{
		goto fail;
	}

	if (-1 == fcntl(sockfd, F_SETFL, O_NONBLOCK))
	{
		goto fail;
	}

	gLobbyBroadcast.sockfd = sockfd;
	gLobbyBroadcast.boundForListening = false;
	gLobbyBroadcast.timeUntilNextBroadcast = 0;
	return true;

fail:
	if (sockfd >= 0)
	{
		close(sockfd);
		sockfd = -1;
	}
	return false;
}

static void DisposeLobbyBroadcastSocket(void)
{
	if (gLobbyBroadcast.sockfd >= 0)
	{
		close(gLobbyBroadcast.sockfd);
	}

	gLobbyBroadcast.sockfd = -1;
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
		.sin_addr = {INADDR_BROADCAST},
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
		printf("%s: sendto: error %d\n", __func__, errno);
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
		if (errno == EAGAIN)
		{
			return;
		}
		else
		{
			printf("%s: error %d\n", __func__, errno);
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
		.sin_addr = {lobby->hostAddr.sin_addr.s_addr},
	};

	int connectRC = connect(sock, (const struct sockaddr*) &bindAddr, sizeof(bindAddr));

	if (connectRC == -1)
	{
		printf("%s: connect failed: %d\n", __func__, errno);
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
			&message,
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

	int newSocket = -1;

	newSocket = accept(game->sockfd, NULL, NULL);

	if (newSocket == -1)	// nobody's trying to connect right now
	{
		goto fail;
	}

	printf("A new player's knocking at the door...\n");

	// make the socket non-blocking
	if (-1 == fcntl(newSocket, F_SETFL, O_NONBLOCK))
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
	if (newSocket >= 0)
	{
		close(newSocket);
	}
	return false;
}

#pragma mark - Message socket

static int CreateGameSocket(bool bindIt)
{
	int sockfd = -1;
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
	if (-1 == sockfd)
	{
		goto fail;
	}

	if (bindIt)
	{
		if (-1 == fcntl(sockfd, F_SETFL, O_NONBLOCK))
		{
			goto fail;
		}
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
			printf("%s: bind failed: %d\n", __func__, errno);
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
	if (sockfd >= 0)
	{
		close(sockfd);
		sockfd = -1;
	}
	return -1;
}

#pragma mark - Basic message

bool CheckIncomingMessage(const void* message, ssize_t length)
{
	const NSpMessageHeader* header = (const NSpMessageHeader*) message;

	if (length < (ssize_t) sizeof(NSpMessageHeader)			// buffer should be big enough for header before we read from it
		|| header->version != kNSpCMRProtocol4CC			// bad protocol magic
		|| header->messageLen < length						// buffer should be big enough for full message
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
				printf("Got %zu bytes from client %d - %d\n", recvRC, i, errno);

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
	printf("Host TCP socket: %d\n", game->sockfd);

	if (game->sockfd == -1)
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
		printf("%s: bind failed: %d\n", __func__, errno);
		if (errno == EADDRINUSE)
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
	game->sockfd = -1;
	game->numClientsConnected = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		game->hostToClientSockets[i] = -1;
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

	if (game->sockfd != -1)
	{
		close(game->sockfd);
		game->sockfd = -1;
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (game->hostToClientSockets[i] != -1)
		{
			close(game->hostToClientSockets[i]);
			game->hostToClientSockets[i] = -1;
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
