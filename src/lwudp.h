#pragma once
#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
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
#define LW_UDP_SERVER "192.168.0.28"
#define LW_UDP_BUFLEN 512
#define LW_UDP_PORT 10288
#include "puckgamepacket.h"

typedef enum _LW_UDP_STATE {
	// Init
	LUS_INIT,
	// Before token received
	LUS_GETTOKEN,
	// Wait match
	LUS_QUEUE,
	// Battle started
	LUS_MATCHED,
} LW_UDP_STATE;

#define LW_STATE_BUFFER_SIZE (16)

typedef struct _LWUDP {
#if LW_PLATFORM_WIN32
	WSADATA wsa;
#endif
	struct sockaddr_in si_other;
	int s, slen;
	char buf[LW_UDP_BUFLEN];
	char message[LW_UDP_BUFLEN];
	fd_set readfds;
	struct timeval tv;
	int recv_len;
	int ready;
	// State
	LW_UDP_STATE state;
	// Network session token
	int token;
	// 1 if master, 0 if slave
	int master;
	// State buffer (even, odd)
	LWPUCKGAMEPACKETSTATE puck_game_state_buffer[2][LW_STATE_BUFFER_SIZE];
	// State buffer (even, odd) index (should be 0 or 1)
	int puck_game_state_buffer_index;
} LWUDP;

typedef struct _LWCONTEXT LWCONTEXT;

LWUDP* new_udp();
void destroy_udp(LWUDP** udp);
void udp_send(LWUDP* udp, const char* data, int size);
void udp_update(LWCONTEXT* pLwc, LWUDP* udp);
