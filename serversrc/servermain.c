#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <winsock2.h>
#else
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#if !LW_PLATFORM_OSX
#include <endian.h>
#endif
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h> 
#include<arpa/inet.h> //inet_addr
#include <unistd.h> // chdir
#include <inttypes.h>
#endif
#include "lwlog.h"
#include "puckgame.h"
#include <inttypes.h>
#include <fcntl.h>
#include "puckgamepacket.h"
#include "spherebattlepacket.h"
#if LW_PLATFORM_WIN32
#define LwChangeDirectory(x) SetCurrentDirectory(x)
#else
#define LwChangeDirectory(x) chdir(x)
#endif

#include "lwtimepoint.h"

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (~0)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#define BUFLEN 512  //Max length of buffer
#define PORT 10288   //The port on which to listen for incoming data

typedef struct _LWSERVER {
	SOCKET s;
	struct sockaddr_in server, si_other;
	int slen, recv_len;
	char buf[BUFLEN];
#if LW_PLATFORM_WIN32
	WSADATA wsa;
#endif
	// Player command
	int dir_pad_dragging;
	float dx;
	float dy;
} LWSERVER;

static BOOL directory_exists(const char* szPath)
{
#if LW_PLATFORM_WIN32
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
	DIR* dir = opendir(szPath);
	if (dir)
	{
		return 1;
	}
	else
	{
		return 0;
	}
#endif
}

static int lw_get_normalized_dir_pad_input(const LWSERVER* server, float *dx, float *dy, float *dlen) {
	if (server->dir_pad_dragging == 0) {
		return 0;
	}
	*dx = server->dx;
	*dy = server->dy;
	return 1;
}

static void update_puck_game(LWSERVER* server, LWPUCKGAME* puck_game, double delta_time) {
	puck_game->player.puck_contacted = 0;
	dSpaceCollide(puck_game->space, puck_game, puck_game_near_callback);
	//dWorldStep(puck_game->world, 0.005f);
	dWorldQuickStep(puck_game->world, delta_time);
	dJointGroupEmpty(puck_game->contact_joint_group);
	if (puck_game->player.puck_contacted == 0) {
		puck_game->player.last_contact_puck_body = 0;
	}
	
	const dReal* p = dBodyGetPosition(puck_game->go[LPGO_PUCK].body);

	//LOGI("pos %.2f %.2f %.2f", p[0], p[1], p[2]);

	dJointID pcj = puck_game->player_control_joint;
	float player_speed = 2.5f;
	//dJointSetLMotorParam(pcj, dParamVel1, player_speed * (pLwc->player_move_right - pLwc->player_move_left));
	//dJointSetLMotorParam(pcj, dParamVel2, player_speed * (pLwc->player_move_up - pLwc->player_move_down));

	//pLwc->player_pos_last_moved_dx
	float dx, dy, dlen;
	if (lw_get_normalized_dir_pad_input(server, &dx, &dy, &dlen)) {
		dJointSetLMotorParam(pcj, dParamVel1, player_speed * dx);
		dJointSetLMotorParam(pcj, dParamVel2, player_speed * dy);
	}
	else {
		dJointSetLMotorParam(pcj, dParamVel1, 0);
		dJointSetLMotorParam(pcj, dParamVel2, 0);
	}

	// Move direction fixed while dashing
	if (puck_game->dash.remain_time > 0) {
		player_speed *= puck_game->dash_speed_ratio;
		dx = puck_game->dash.dir_x;
		dy = puck_game->dash.dir_y;
		dJointSetLMotorParam(pcj, dParamVel1, player_speed * dx);
		dJointSetLMotorParam(pcj, dParamVel2, player_speed * dy);
		puck_game->dash.remain_time = LWMAX(0, puck_game->dash.remain_time - (float)delta_time);
	}
}

#if LW_PLATFORM_WIN32
#else
int WSAGetLastError() {
	return -1;
}
#endif

LWSERVER* new_server() {
	LWSERVER* server = malloc(sizeof(LWSERVER));
	memset(server, 0, sizeof(LWSERVER));
	server->slen = sizeof(server->si_other);
#if LW_PLATFORM_WIN32
	//Initialise winsock
	LOGI("Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &server->wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	LOGI("Initialised.\n");
#endif
	//Create a socket
	if ((server->s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		LOGE("Could not create socket : %d", WSAGetLastError());
	}
	LOGI("Socket created.\n");

	//Prepare the sockaddr_in structure
	server->server.sin_family = AF_INET;
	server->server.sin_addr.s_addr = INADDR_ANY;
	server->server.sin_port = htons(PORT);

	//Bind
	if (bind(server->s, (struct sockaddr *)&server->server, sizeof(server->server)) == SOCKET_ERROR)
	{
		LOGE("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	LOGI("Bind done");
	return server;
}

int make_socket_nonblocking(int sock) {
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

void normalize_quaternion(float* q) {
	const float l = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
	q[0] /= l;
	q[1] /= l;
	q[2] /= l;
	q[3] /= l;
}

void server_send(LWSERVER* server, const char* p, int s) {
	sendto(server->s, p, s, 0, (struct sockaddr*) &server->si_other, server->slen);
}

#define SERVER_SEND(server, packet) server_send(server, (const char*)&packet, sizeof(packet))

typedef struct _LWCONN {
	unsigned long long ipport;
	struct sockaddr_in si;
	double last_ingress_timepoint;
} LWCONN;

#define LW_CONN_CAPACITY (32)

unsigned long long to_ipport(unsigned int ip, unsigned short port) {
	return ((unsigned long long)port << 32) | ip;
}

void add_conn(LWCONN* conn, int conn_capacity, struct sockaddr_in* si) {
	unsigned long long ipport = to_ipport(si->sin_addr.S_un.S_addr, si->sin_port);
	// Update last ingress for existing element
	for (int i = 0; i < conn_capacity; i++) {
		if (conn[i].ipport == ipport) {
			conn[i].last_ingress_timepoint = lwtimepoint_now_seconds();
			return;
		}
	}
	// New element
	for (int i = 0; i < conn_capacity; i++) {
		if (conn[i].ipport == 0) {
			conn[i].ipport = ipport;
			conn[i].last_ingress_timepoint = lwtimepoint_now_seconds();
			memcpy(&conn[i].si, si, sizeof(struct sockaddr_in));
			return;
		}
	}
	LOGE("add_conn: maximum capacity exceeded.");
}

void invalidate_dead_conn(LWCONN* conn, int conn_capacity, double current_timepoint, double life) {
	for (int i = 0; i < conn_capacity; i++) {
		if (conn[i].ipport) {
			if (current_timepoint - conn[i].last_ingress_timepoint > life) {
				conn[i].ipport = 0;
			}
		}
	}
}

void broadcast_packet(LWSERVER* server, const LWCONN* conn, int conn_capacity, const char* p, int s) {
	for (int i = 0; i < conn_capacity; i++) {
		if (conn[i].ipport) {
			sendto(server->s, p, s, 0, (struct sockaddr*)&conn[i].si, server->slen);
		}
	}	
}

int main(int argc, char* argv[]) {
	LOGI("LAIDOFF-SERVER: Greetings.");
	while (!directory_exists("assets") && LwChangeDirectory(".."))
	{
	}
	LWSERVER* server = new_server();
	LWPUCKGAME* puck_game = new_puck_game();
	int update_tick = 0;
	const double sim_timestep = 1.0f / 60; // sec
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;// sim_timestep * 1000 * 1000;
	fd_set readfds;
	make_socket_nonblocking(server->s);
	int token_counter = 0;
	LWCONN conn[LW_CONN_CAPACITY];
	double elapsed_ms = 0;
	while (1) {
		const double loop_start = lwtimepoint_now_seconds();
		if (elapsed_ms > 0) {
			int iter = (int)(elapsed_ms / (sim_timestep * 1000));
			for (int i = 0; i < iter; i++) {
				update_puck_game(server, puck_game, sim_timestep);
				update_tick++;
			}
			elapsed_ms = fmod(elapsed_ms, (sim_timestep * 1000));
			if (iter) {
				// Broadcast state to clients
				LWPUCKGAMEPACKETSTATE packet_state;
				packet_state.type = LPGPT_STATE;
				packet_state.token = 0;
				packet_state.update_tick = update_tick;
				packet_state.puck[0] = puck_game->go[LPGO_PUCK].pos[0];
				packet_state.puck[1] = puck_game->go[LPGO_PUCK].pos[1];
				packet_state.puck[2] = puck_game->go[LPGO_PUCK].pos[2];
				packet_state.player[0] = puck_game->go[LPGO_PLAYER].pos[0];
				packet_state.player[1] = puck_game->go[LPGO_PLAYER].pos[1];
				packet_state.player[2] = puck_game->go[LPGO_PLAYER].pos[2];
				packet_state.target[0] = puck_game->go[LPGO_TARGET].pos[0];
				packet_state.target[1] = puck_game->go[LPGO_TARGET].pos[1];
				packet_state.target[2] = puck_game->go[LPGO_TARGET].pos[2];
				memcpy(packet_state.puck_rot, puck_game->go[LPGO_PUCK].rot, sizeof(mat4x4));
				memcpy(packet_state.player_rot, puck_game->go[LPGO_PLAYER].rot, sizeof(mat4x4));
				memcpy(packet_state.target_rot, puck_game->go[LPGO_TARGET].rot, sizeof(mat4x4));
				broadcast_packet(server, conn, LW_CONN_CAPACITY, (const char*)&packet_state, sizeof(packet_state));
			}
		}
		
		//LOGI("Update tick %"PRId64, update_tick);

		invalidate_dead_conn(conn, LW_CONN_CAPACITY, lwtimepoint_now_seconds(), 2.0);

		//LOGI("Waiting for data...");
		fflush(stdout);

		//clear the buffer by filling null, it might have previously received data
		memset(server->buf, '\0', BUFLEN);

		FD_ZERO(&readfds);
		FD_SET(server->s, &readfds);

		int rv = select(server->s + 1, &readfds, NULL, NULL, &tv);

		//try to receive some data, this is a blocking call
		if (rv == 1) {
			if ((server->recv_len = recvfrom(server->s, server->buf, BUFLEN, 0, (struct sockaddr *) &server->si_other, &server->slen)) == SOCKET_ERROR) {
				printf("recvfrom() failed with error code : %d", WSAGetLastError());
				//exit(EXIT_FAILURE);
			}
			else {
				add_conn(conn, LW_CONN_CAPACITY, &server->si_other);

				//print details of the client/peer and the data received
				printf("Received packet from %s:%d (size:%d)\n",
					inet_ntoa(server->si_other.sin_addr),
					ntohs(server->si_other.sin_port),
					server->recv_len);

				const int packet_type = *(int*)server->buf;
				switch (packet_type) {
				case LSBPT_GETTOKEN:
				{
					LWSPHEREBATTLEPACKETTOKEN p;
					p.type = LSBPT_TOKEN;
					++token_counter;
					p.token = token_counter;
					SERVER_SEND(server, p);
					break;
				}
				case LSBPT_QUEUE:
				{
					LWSPHEREBATTLEPACKETMATCHED p;
					p.type = LSBPT_MATCHED;
					p.master = 0;
					SERVER_SEND(server, p);
					break;
				}
				case LPGPT_MOVE:
				{
					LWPUCKGAMEPACKETMOVE* p = (LWPUCKGAMEPACKETMOVE*)server->buf;
					LOGI("MOVE dx=%.2f dy=%.2f", p->dx, p->dy);
					server->dir_pad_dragging = 1;
					server->dx = p->dx;
					server->dy = p->dy;
					break;
				}
				case LPGPT_STOP:
				{
					LWPUCKGAMEPACKETSTOP* p = (LWPUCKGAMEPACKETSTOP*)server->buf;
					LOGI("STOP");
					server->dir_pad_dragging = 0;
					break;
				}
				case LPGPT_DASH:
				{
					LWPUCKGAMEPACKETDASH* p = (LWPUCKGAMEPACKETDASH*)server->buf;
					LOGI("DASH");
					const dReal* player_vel = dBodyGetLinearVel(puck_game->go[LPGO_PLAYER].body);
					puck_game_commit_dash(puck_game, &puck_game->dash,
						(float)player_vel[0], (float)player_vel[1]);
					break;
				}
				default:
				{
					break;
				}
				}
			}

			
		}
		else {
			//LOGI("EMPTY");
		}
		elapsed_ms += (lwtimepoint_now_seconds() - loop_start) * 1000;
	}
	delete_puck_game(&puck_game);
	return 0;
}
