#pragma once
#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#else
#include <fcntl.h>
#include <sys/socket.h>
#if !LW_PLATFORM_OSX && !LW_PLATFORM_IOS
#include <linux/in.h>
#include <endian.h>
#endif
#include <stdlib.h>
#include <czmq_prelude.h>
#endif
#define LW_TCP_SERVER "192.168.0.28"
#define LW_TCP_BUFLEN 512
#define LW_TCP_PORT 19856

typedef struct _LWTCP {
#if LW_PLATFORM_WIN32
	WSADATA wsaData;
#endif
	SOCKET ConnectSocket;
	struct addrinfo* result;
	struct addrinfo* ptr;
	struct addrinfo hints;
	char sendbuf[LW_TCP_BUFLEN];
	char recvbuf[LW_TCP_BUFLEN];
	int iResult;
	int recvbuflen;
} LWTCP;

typedef struct _LWCONTEXT LWCONTEXT;

LWTCP* new_tcp();
void destroy_tcp(LWTCP** tcp);
void tcp_send(LWTCP* tcp, const char* data, int size);
void tcp_update(LWCONTEXT* pLwc, LWTCP* tcp);
