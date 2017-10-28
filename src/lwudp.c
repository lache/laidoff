#include "lwudp.h"
#include "lwlog.h"
#include "puckgamepacket.h"
#include "spherebattlepacket.h"
#include "lwcontext.h"
#include "puckgameupdate.h"

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
		//case LPGPT_STATE:
		//{
		//	LWPUCKGAMEPACKETSTATE* p = (LWPUCKGAMEPACKETSTATE*)udp->buf;
		//	/*LOGI("Network player pos %.2f, %.2f, %.2f",
		//		p->player[0], p->player[1], p->player[2]);*/
		//	memcpy(&pLwc->puck_game_state, p, sizeof(LWPUCKGAMEPACKETSTATE));
		//	break;
		//}
		case LSBPT_TOKEN:
		{
			if (udp->recv_len != sizeof(LWSPHEREBATTLEPACKETTOKEN)) {
				LOGE("LWSPHEREBATTLEPACKETTOKEN: Size error %d (%d expected)", udp->recv_len, sizeof(LWSPHEREBATTLEPACKETTOKEN));
			}
			LWSPHEREBATTLEPACKETTOKEN* p = (LWSPHEREBATTLEPACKETTOKEN*)udp->buf;
			LOGI("LWSPHEREBATTLEPACKETTOKEN: Change token from 0x%08x to 0x%08x", udp->token, p->token);
			udp->token = p->token;
			udp->state = LUS_QUEUE;
			break;
		}
		case LSBPT_MATCHED:
		{
			if (udp->recv_len != sizeof(LWSPHEREBATTLEPACKETMATCHED)) {
				LOGE("LWSPHEREBATTLEPACKETMATCHED: Size error %d (%d expected)", udp->recv_len, sizeof(LWSPHEREBATTLEPACKETMATCHED));
			}
			LWSPHEREBATTLEPACKETMATCHED* p = (LWSPHEREBATTLEPACKETMATCHED*)udp->buf;
			LOGI("LWSPHEREBATTLEPACKETMATCHED: Matched! I'm %s", p->master ? "master." : "slave.");
			udp->state = LUS_MATCHED;
			udp->master = p->master;
			break;
		}
		// Battle packets
		case LPGPT_MOVE:
		{
			if (udp->master) {
				LWPUCKGAMEPACKETMOVE* p = (LWPUCKGAMEPACKETMOVE*)udp->buf;
				if (udp->recv_len != sizeof(LWPUCKGAMEPACKETMOVE)) {
					LOGE("LWPUCKGAMEPACKETMOVE: Size error %d (%d expected)", udp->recv_len, sizeof(LWPUCKGAMEPACKETMOVE));
				}
				puck_game_target_move(pLwc->puck_game, p->dx, p->dy);
				//LOGI("MOVE dx=%.2f dy=%.2f", p->dx, p->dy);
				//server->dir_pad_dragging = 1;
				//server->dx = p->dx;
				//server->dy = p->dy;
			}
			break;
		}
		case LPGPT_STOP:
		{
			if (udp->master) {
				LWPUCKGAMEPACKETSTOP* p = (LWPUCKGAMEPACKETSTOP*)udp->buf;
				if (udp->recv_len != sizeof(LWPUCKGAMEPACKETSTOP)) {
					LOGE("LWPUCKGAMEPACKETSTOP: Size error %d (%d expected)", udp->recv_len, sizeof(LWPUCKGAMEPACKETSTOP));
				}
				puck_game_target_stop(pLwc->puck_game);
				//LOGI("STOP");
				/*server->dir_pad_dragging = 0;*/
			}
			
			break;
		}
		case LPGPT_DASH:
		{
			if (udp->master) {
				LWPUCKGAMEPACKETDASH* p = (LWPUCKGAMEPACKETDASH*)udp->buf;
				if (udp->recv_len != sizeof(LWPUCKGAMEPACKETDASH)) {
					LOGE("LWPUCKGAMEPACKETDASH: Size error %d (%d expected)", udp->recv_len, sizeof(LWPUCKGAMEPACKETDASH));
				}
				puck_game_target_dash(pLwc->puck_game);
				//LOGI("DASH");
				/*const dReal* player_vel = dBodyGetLinearVel(puck_game->go[LPGO_PLAYER].body);
				puck_game_commit_dash(puck_game,
					(float)player_vel[0], (float)player_vel[1]);*/
			}
			break;
		}
		case LPGPT_STATE:
		{
			if (!udp->master) {
				LWPUCKGAMEPACKETSTATE* p = (LWPUCKGAMEPACKETSTATE*)udp->buf;
				if (udp->recv_len != sizeof(LWPUCKGAMEPACKETSTATE)) {
					LOGE("LWPUCKGAMEPACKETSTATE: Size error %d (%d expected)", udp->recv_len, sizeof(LWPUCKGAMEPACKETSTATE));
				}
				memcpy(&pLwc->puck_game_state, p, sizeof(LWPUCKGAMEPACKETSTATE));
			}
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
