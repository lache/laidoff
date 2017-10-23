#pragma once
#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
#endif
#define LW_UDP_SERVER "127.0.0.1"
#define LW_UDP_BUFLEN 512
#define LW_UDP_PORT 19856

typedef struct _LWUDP {
#if LW_PLATFORM_WIN32
	struct sockaddr_in si_other;
	int s, slen;
	char buf[LW_UDP_BUFLEN];
	char message[LW_UDP_BUFLEN];
	WSADATA wsa;
	fd_set readfds;
	struct timeval tv;
	int recv_len;
#endif
} LWUDP;

typedef struct _LWCONTEXT LWCONTEXT;

LWUDP* new_udp();
void destroy_udp(LWUDP** udp);
void udp_send(LWUDP* udp, const char* data, int size);
void udp_update(LWCONTEXT* pLwc, LWUDP* udp);
