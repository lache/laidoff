#include "lwudp.h"
#include "lwlog.h"
#include "puckgamepacket.h"
#include "lwcontext.h"
#include "puckgameupdate.h"
#include "lwtimepoint.h"
#include "puckgame.h"

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
	struct hostent* he = gethostbyname(LW_UDP_SERVER);
	struct in_addr** addr_list = (struct in_addr **) he->h_addr_list;
	udp->si_other.sin_addr.s_addr = addr_list[0]->s_addr;
	udp->tv.tv_sec = 0;
	udp->tv.tv_usec = 0;
	make_socket_nonblocking(udp->s);
	udp->ready = 1;
	udp->master = 1;
	ringbuffer_init(&udp->state_ring_buffer, udp->state_buffer, sizeof(LWPSTATE), LW_STATE_RING_BUFFER_CAPACITY);
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
		//LOGE("sendto() failed with error...");
#endif
		//exit(EXIT_FAILURE);
	}
}

void queue_state(LWUDP* udp, const LWPSTATE* p) {
	ringbuffer_queue(&udp->state_ring_buffer, p);
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
				// Go back to single play mode
				udp->master = 1;
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
		//case LPGP_LWSTATE:
		//{
		//	LWPSTATE* p = (LWPSTATE*)udp->buf;
		//	/*LOGI("Network player pos %.2f, %.2f, %.2f",
		//		p->player[0], p->player[1], p->player[2]);*/
		//	memcpy(&pLwc->puck_game_state, p, sizeof(LWPSTATE));
		//	break;
		//}
		case LPGP_LWPTOKEN:
		{
			if (udp->recv_len != sizeof(LWPTOKEN)) {
				LOGE("LWPTOKEN: Size error %d (%d expected)", udp->recv_len, sizeof(LWPTOKEN));
			}
			LWPTOKEN* p = (LWPTOKEN*)udp->buf;
			LOGI("LWPTOKEN: Change token from 0x%08x to 0x%08x", udp->token, p->token);
			udp->token = p->token;
			udp->state = LUS_QUEUE;
			break;
		}
		case LPGP_LWPMATCHED:
		{
			if (udp->recv_len != sizeof(LWPMATCHED)) {
				LOGE("LWPMATCHED: Size error %d (%d expected)", udp->recv_len, sizeof(LWPMATCHED));
			}
			LWPMATCHED* p = (LWPMATCHED*)udp->buf;
			LOGI("LWPMATCHED: Matched! I'm %s", p->master ? "master." : "slave.");
			udp->state = LUS_MATCHED;
			udp->master = p->master;
			break;
		}
		case LPGP_LWPPLAYERDAMAGED:
		{
			puck_game_player_decrease_hp_test(pLwc->puck_game);
			break;
		}
		// Battle packets
		case LPGP_LWPMOVE:
		{
			if (udp->master) {
				LWPMOVE* p = (LWPMOVE*)udp->buf;
				if (udp->recv_len != sizeof(LWPMOVE)) {
					LOGE("LWPMOVE: Size error %d (%d expected)", udp->recv_len, sizeof(LWPMOVE));
				}
				puck_game_target_move(pLwc->puck_game, p->dx, p->dy);
				//LOGI("MOVE dx=%.2f dy=%.2f", p->dx, p->dy);
				//server->dir_pad_dragging = 1;
				//server->dx = p->dx;
				//server->dy = p->dy;
			}
			break;
		}
		case LPGP_LWPSTOP:
		{
			if (udp->master) {
				LWPSTOP* p = (LWPSTOP*)udp->buf;
				if (udp->recv_len != sizeof(LWPSTOP)) {
					LOGE("LWPSTOP: Size error %d (%d expected)", udp->recv_len, sizeof(LWPSTOP));
				}
				puck_game_target_stop(pLwc->puck_game);
				//LOGI("STOP");
				/*server->dir_pad_dragging = 0;*/
			}
			
			break;
		}
		case LPGP_LWPDASH:
		{
			if (udp->master) {
				LWPDASH* p = (LWPDASH*)udp->buf;
				if (udp->recv_len != sizeof(LWPDASH)) {
					LOGE("LWPDASH: Size error %d (%d expected)", udp->recv_len, sizeof(LWPDASH));
				}
				puck_game_target_dash(pLwc->puck_game);
				//LOGI("DASH");
				/*const dReal* player_vel = dBodyGetLinearVel(puck_game->go[LPGO_PLAYER].body);
				puck_game_commit_dash(puck_game,
					(float)player_vel[0], (float)player_vel[1]);*/
			}
			break;
		}
		case LPGP_LWPSTATE:
		{
			if (!udp->master) {
				LWPSTATE* p = (LWPSTATE*)udp->buf;
				if (udp->recv_len != sizeof(LWPSTATE)) {
					LOGE("LWPSTATE: Size error %d (%d expected)", udp->recv_len, sizeof(LWPSTATE));
				}
				//int tick_diff = p->update_tick - pLwc->puck_game_state.update_tick;
				/*if (tick_diff > 0)*/ {
					/*if (tick_diff != 1) {
						LOGI("Packet jitter");
					}*/
					//memcpy(&pLwc->puck_game_state, p, sizeof(LWPSTATE));
					double last_received = lwtimepoint_now_seconds();
					double state_packet_interval = last_received - pLwc->puck_game_state_last_received;
					pLwc->puck_game_state_last_received = last_received;
					pLwc->puck_game_state_last_received_interval = state_packet_interval * 1000;

					queue_state(pLwc->udp, p);
					int rb_size = ringbuffer_size(&pLwc->udp->state_ring_buffer);
					if (pLwc->udp->state_count == 0) {
						pLwc->udp->state_start_timepoint = lwtimepoint_now_seconds();
					}
					pLwc->udp->state_count++;
					double elapsed_from_start = lwtimepoint_now_seconds() - pLwc->udp->state_start_timepoint;
					//LOGI("State packet interval: %.3f ms (rb size=%d) (%.2f pps)", pLwc->puck_game_state_last_received_interval, rb_size, (float)pLwc->udp->state_count / elapsed_from_start);

				}
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
