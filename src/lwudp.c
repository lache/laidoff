#include "lwudp.h"
#include "lwlog.h"
#include "puckgamepacket.h"
#include "lwcontext.h"

static int make_socket_nonblocking(int sock) {
#if defined(WIN32) || defined(_WIN32) || defined(IMN_PIM)
	unsigned long arg = 1;
	return ioctlsocket(sock, FIONBIO, &arg) == 0;
#elif defined(VXWORKS)
	int arg = 1;
	return ioctl(sock, FIONBIO, (int)&arg) == 0;
#else
	int curFlags = fcntl(sock, F_GETFL, 0);
	return fcntl(sock, F_SETFL, curFlags | O_NONBLOCK) >= 0;
#endif
}

LWUDP* new_udp() {
	LWUDP* udp = (LWUDP*)malloc(sizeof(LWUDP));
	memset(udp, 0, sizeof(LWUDP));
	udp->slen = sizeof(udp->si_other);
#if LW_PLATFORM_WIN32
	LOGI("Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &udp->wsa) != 0)
	{
		LOGE("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	LOGI("Initialised.\n");
#endif

	//create socket
	if ((udp->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
#if LW_PLATFORM_WIN32
		LOGE("socket() failed with error code : %d", WSAGetLastError());
#else
		LOGE("socket() failed...");
#endif
		exit(EXIT_FAILURE);
	}

	//setup address structure
	memset((char *)&udp->si_other, 0, sizeof(udp->si_other));
	udp->si_other.sin_family = AF_INET;
	udp->si_other.sin_port = htons(LW_UDP_PORT);
#if LW_PLATFORM_WIN32
	udp->si_other.sin_addr.S_un.S_addr = inet_addr(LW_UDP_SERVER);
#else
	udp->si_other.sin_addr.s_addr = inet_addr(LW_UDP_SERVER);
#endif
	
	udp->tv.tv_sec = 0;
	udp->tv.tv_usec = 0;
	make_socket_nonblocking(udp->s);
	udp->ready = 1;
	return udp;
}

void destroy_udp(LWUDP** udp) {
	free(*udp);
	*udp = 0;
}

void udp_send(LWUDP* udp, const char* data, int size) {
	//send the message
	if (sendto(udp->s, data, size, 0, (struct sockaddr *) &udp->si_other, udp->slen) == SOCKET_ERROR)
	{
#if LW_PLATFORM_WIN32
		LOGE("sendto() failed with error code : %d", WSAGetLastError());
#else
		LOGE("sendto() failed with error...");
#endif
		exit(EXIT_FAILURE);
	}
}

void udp_update(LWCONTEXT* pLwc, LWUDP* udp) {
	if (udp->ready == 0) {
		return;
	}
	FD_ZERO(&udp->readfds);
	FD_SET(udp->s, &udp->readfds);
	int rv = 0;
	while ((rv = select(udp->s + 1, &udp->readfds, NULL, NULL, &udp->tv)) == 1)
	{
		if ((udp->recv_len = recvfrom(udp->s, udp->buf, LW_UDP_BUFLEN, 0, (struct sockaddr *) &udp->si_other, &udp->slen)) == SOCKET_ERROR) {
#if LW_PLATFORM_WIN32
			int wsa_error_code = WSAGetLastError();
			if (wsa_error_code == WSAECONNRESET) {
				// UDP server not ready?
				return;
			}
			else {
				LOGI("recvfrom() failed with error code : %d", wsa_error_code);
				exit(EXIT_FAILURE);
			}
#else
#endif
		}
		const int packet_type = *(int*)udp->buf;
		switch (packet_type) {
		case LPGPT_STATE:
		{
			LWPUCKGAMEPACKETSTATE* p = (LWPUCKGAMEPACKETSTATE*)udp->buf;
			/*LOGI("Network player pos %.2f, %.2f, %.2f",
				p->player[0], p->player[1], p->player[2]);*/
			memcpy(&pLwc->puck_game_state, p, sizeof(LWPUCKGAMEPACKETSTATE));
			break;
		}
		default:
		{
			LOGE("Unknown UDP datagram received.");
			break;
		}
		}
	}
}
