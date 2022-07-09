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

#include "game.h"
#include "network.h"

#define LOBBY_PORT 21809
#define LOBBY_BROADCAST_INTERVAL 1.0f

#define MAX_LOBBIES 5

typedef struct
{
	int sockfd;
	bool boundForListening;
	float timeUntilNextBroadcast;
} LobbyBroadcast;

typedef struct
{
	struct sockaddr_in remoteAddr;
} LobbyInfo;

static LobbyBroadcast gLobbyBroadcast =
{
	.sockfd = -1,
	.timeUntilNextBroadcast = 0,
};

static LobbyInfo gLobbiesFound[MAX_LOBBIES];
int gNumLobbiesFound = 0;

static void DisposeLobbyBroadcastSocket(void)
{
	if (gLobbyBroadcast.sockfd >= 0)
	{
		close(gLobbyBroadcast.sockfd);
	}

	gLobbyBroadcast.sockfd = -1;
	gLobbyBroadcast.timeUntilNextBroadcast = 0;
}

static bool CreateLobbyBroadcastSocket(void)
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
	}
	return false;
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
		0,
		(struct sockaddr*) &broadcastAddr,
		sizeof(broadcastAddr)
	);

	if (rc == -1)
	{
		printf("%s: sendto: error %d\n", __func__, errno);
	}
}

static bool IsKnownLobbyAddress(const struct sockaddr_in* remoteAddr)
{
	for (int i = 0; i < gNumLobbiesFound; i++)
	{
		if (gLobbiesFound[i].remoteAddr.sin_addr.s_addr == remoteAddr->sin_addr.s_addr
			&& gLobbiesFound[i].remoteAddr.sin_port == remoteAddr->sin_port)
		{
			return true;
		}
	}

	return false;
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

			gLobbiesFound[gNumLobbiesFound].remoteAddr = remoteAddr;

			gNumLobbiesFound++;
			GAME_ASSERT(gNumLobbiesFound <= MAX_LOBBIES);
		}
	}
}

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

bool Net_IsLobbyBroadcastOpen(void)
{
	return gLobbyBroadcast.sockfd >= 0;
}

void Net_CreateLobby(void)
{
	CreateLobbyBroadcastSocket();
}

void Net_CloseLobby(void)
{
	DisposeLobbyBroadcastSocket();
}

void Net_CreateLobbySearch(void)
{
	gNumLobbiesFound = 0;
	memset(gLobbiesFound, 0, sizeof(gLobbiesFound));

	if (!CreateLobbyBroadcastSocket())
	{
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
		DisposeLobbyBroadcastSocket();
		return;
	}

	printf("Created lobby search\n");
}

void Net_CloseLobbySearch(void)
{
	DisposeLobbyBroadcastSocket();
}
