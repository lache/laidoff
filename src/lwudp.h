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
#include "puckgamepacket.h"
#include "lwringbuffer.h"

#define LW_UDP_BUFLEN 512

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

#define LW_STATE_RING_BUFFER_CAPACITY (16)

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
	// State ring buffer
	LWPSTATE state_buffer[LW_STATE_RING_BUFFER_CAPACITY];
	LWRINGBUFFER state_ring_buffer;
	double puck_state_sync_server_timepoint;
	double puck_state_sync_client_timepoint;
	int state_count;
	double state_start_timepoint;
} LWUDP;

typedef struct _LWCONTEXT LWCONTEXT;

LWUDP* new_udp();
void udp_update_addr(LWUDP* udp, unsigned long ip, unsigned short port);
void destroy_udp(LWUDP** udp);
void udp_send(LWUDP* udp, const char* data, int size);
void udp_update(LWCONTEXT* pLwc, LWUDP* udp);
