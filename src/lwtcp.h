#pragma once
#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#else
#include <czmq_prelude.h>
#include <fcntl.h>
#include <sys/socket.h>
#if !LW_PLATFORM_OSX && !LW_PLATFORM_IOS
//#include <czmq_prelude.h>
#include <linux/in.h>
#include <endian.h>
#endif
#include <stdlib.h>
//#include <czmq_prelude.h>
#endif
#include "lwringbuffer.h"
#include "lwuniqueid.h"
#include "lwhostaddr.h"

#define LW_TCP_BUFLEN 1024

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTCP LWTCP;

typedef void(*LWTCP_ON_CONNECT)(LWTCP*, const char*);
typedef int(*LWTCP_ON_RECV_PACKETS)(LWTCP* tcp);

#define NEW_TCP_PACKET(vartype, varname) \
vartype varname; \
varname.size = sizeof(vartype); \
varname.type = LPGP_##vartype

#define NEW_TCP_PACKET_CAPITAL(vartype, varname) \
vartype varname; \
varname.Size = sizeof(vartype); \
varname.Type = LPGP_##vartype

#define CHECK_PACKET(packet_type, packet_size, type) \
packet_type == LPGP_##type && packet_size == sizeof(type)

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
	int recvbufnotparsed;
	LWUNIQUEID user_id;
    LWCONTEXT* pLwc;
    LWTCP_ON_CONNECT on_connect;
    LWTCP_ON_RECV_PACKETS on_recv_packets;
    LWHOSTADDR host_addr;
} LWTCP;

LWTCP* new_tcp(LWCONTEXT* pLwc,
               const char* path_prefix,
               const LWHOSTADDR* host_addr,
               LWTCP_ON_CONNECT on_connect,
               LWTCP_ON_RECV_PACKETS on_recv_packets);
void destroy_tcp(LWTCP** tcp);
void tcp_update(LWTCP* tcp);
int tcp_connect(LWTCP* tcp);
#if !LW_PLATFORM_WIN32
int WSAGetLastError();
#endif
