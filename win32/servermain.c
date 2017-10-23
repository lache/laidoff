#include <winsock2.h>
#include "platform_detection.h"
#include "lwlog.h"
#include "puckgame.h"
#include <inttypes.h>
#include <fcntl.h>
#include "puckgamepacket.h"
#if LW_PLATFORM_WIN32
#define LwChangeDirectory(x) SetCurrentDirectory(x)
#else
#define LwChangeDirectory(x) chdir(x)
#endif

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

void update_puck_game(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, double delta_time) {
	puck_game->player.puck_contacted = 0;
	dSpaceCollide(puck_game->space, puck_game, puck_game_near_callback);
//dWorldStep(puck_game->world, 0.005f);
dWorldQuickStep(puck_game->world, 0.02f);
dJointGroupEmpty(puck_game->contact_joint_group);
if (puck_game->player.puck_contacted == 0) {
	puck_game->player.last_contact_puck_body = 0;
}
}

#define BUFLEN 512  //Max length of buffer
#define PORT 19856   //The port on which to listen for incoming data

typedef struct _LWSERVER {
	SOCKET s;
	struct sockaddr_in server, si_other;
	int slen, recv_len;
	char buf[BUFLEN];
	WSADATA wsa;
} LWSERVER;

LWSERVER* new_server() {
	LWSERVER* server = malloc(sizeof(LWSERVER));
	memset(server, 0, sizeof(LWSERVER));
	server->slen = sizeof(server->si_other);
	//Initialise winsock
	LOGI("Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &server->wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	LOGI("Initialised.\n");

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

int main(int argc, char* argv[]) {
	LOGI("LAIDOFF-SERVER: Greetings.");
	while (!directory_exists("assets") && LwChangeDirectory(".."))
	{
	}
	LWSERVER* server = new_server();
	LWPUCKGAME* puck_game = new_puck_game();
	__int64 update_tick = 0;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 8000;
	fd_set readfds;
	make_socket_nonblocking(server->s);
	while (1) {
		update_puck_game(0, puck_game, 0);
		update_tick++;
		//LOGI("Update tick %"PRId64, update_tick);

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
				exit(EXIT_FAILURE);
			}

			dJointID pcj = puck_game->player_control_joint;
			float player_speed = 0.5f;

			//print details of the client/peer and the data received
			printf("Received packet from %s:%d (size:%d)\n",
				inet_ntoa(server->si_other.sin_addr),
				ntohs(server->si_other.sin_port),
				server->recv_len);
			const int packet_type = *(int*)server->buf;
			switch (packet_type) {
			case LPGPT_MOVE:
			{
				LWPUCKGAMEPACKETMOVE* p = (LWPUCKGAMEPACKETMOVE*)server->buf;
				LOGI("MOVE dx=%.2f dy=%.2f", p->dx, p->dy);
				dJointSetLMotorParam(pcj, dParamVel1, player_speed * p->dx);
				dJointSetLMotorParam(pcj, dParamVel2, player_speed * p->dy);
				break;
			}
			case LPGPT_STOP:
			{
				LWPUCKGAMEPACKETSTOP* p = (LWPUCKGAMEPACKETSTOP*)server->buf;
				LOGI("STOP");
				dJointSetLMotorParam(pcj, dParamVel1, 0);
				dJointSetLMotorParam(pcj, dParamVel2, 0);
				break;
			}
			case LPGPT_DASH:
			{
				LWPUCKGAMEPACKETDASH* p = (LWPUCKGAMEPACKETDASH*)server->buf;
				LOGI("DASH");
				break;
			}
			default:
			{
				break;
			}
			}

			LWPUCKGAMEPACKETSTATE packet_state;
			packet_state.type = LPGPT_STATE;
			packet_state.puck[0] = puck_game->go[LPGO_PUCK].pos[0];
			packet_state.puck[1] = puck_game->go[LPGO_PUCK].pos[1];
			packet_state.puck[2] = puck_game->go[LPGO_PUCK].pos[2];
			packet_state.player[0] = puck_game->go[LPGO_PLAYER].pos[0];
			packet_state.player[1] = puck_game->go[LPGO_PLAYER].pos[1];
			packet_state.player[2] = puck_game->go[LPGO_PLAYER].pos[2];
			packet_state.target[0] = puck_game->go[LPGO_TARGET].pos[0];
			packet_state.target[1] = puck_game->go[LPGO_TARGET].pos[1];
			packet_state.target[2] = puck_game->go[LPGO_TARGET].pos[2];
			quat_from_mat4x4(packet_state.puck_rot, puck_game->go[LPGO_PUCK].rot);
			quat_from_mat4x4(packet_state.player_rot, puck_game->go[LPGO_PLAYER].rot);
			quat_from_mat4x4(packet_state.target_rot, puck_game->go[LPGO_TARGET].rot);
			//now reply the client with the same data
			if (sendto(server->s, (const char*)&packet_state, sizeof(packet_state), 0, (struct sockaddr*) &server->si_other, server->slen) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d", WSAGetLastError());
				exit(EXIT_FAILURE);
			}
		}
		else {
			//LOGI("EMPTY");
		}
	}
	delete_puck_game(&puck_game);
	return 0;
}
