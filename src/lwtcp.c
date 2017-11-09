#include "lwtcp.h"
#include "lwlog.h"
#include "puckgamepacket.h"
#include "lwcontext.h"
#include "sysmsg.h"
#include "puckgame.h"
#include "lwudp.h"
#include "file.h"

#if LW_AUTO_BUILD || LW_PLATFORM_IOS
#define LW_TCP_SERVER "puck-highend.popsongremix.com"
#else
//#define LW_TCP_SERVER "192.168.0.28"
//#define LW_TCP_SERVER "puck-highend.popsongremix.com"
//#define LW_TCP_SERVER "puck.popsongremix.com"
#define LW_TCP_SERVER "221.147.71.76"
#endif
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define LW_TCP_PORT 19856
#define LW_TCP_PORT_STR STR(LW_TCP_PORT)

#if LW_PLATFORM_WIN32
#else
int WSAGetLastError() {
	return -1;
}
#endif

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

#define NEW_TCP_PACKET(vartype, varname) \
vartype varname; \
varname.size = sizeof(vartype); \
varname.type = LPGP_##vartype

#define NEW_TCP_PACKET_CAPITAL(vartype, varname) \
vartype varname; \
varname.Size = sizeof(vartype); \
varname.Type = LPGP_##vartype


LWTCP* new_tcp(const char* path_prefix) {
	LWTCP* tcp = (LWTCP*)malloc(sizeof(LWTCP));
	memset(tcp, 0, sizeof(LWTCP));
	tcp->ConnectSocket = INVALID_SOCKET;
	tcp->recvbuflen = LW_TCP_BUFLEN;
	tcp->hints.ai_family = AF_UNSPEC;
	tcp->hints.ai_socktype = SOCK_STREAM;
	tcp->hints.ai_protocol = IPPROTO_TCP;

	tcp->iResult = getaddrinfo(LW_TCP_SERVER, LW_TCP_PORT_STR, &tcp->hints, &tcp->result);
	if (tcp->iResult != 0) {
		LOGE("getaddrinfo failed with error: %d", tcp->iResult);
		free(tcp);
		return 0;
	}

	// Attempt to connect to an address until one succeeds
	for (tcp->ptr = tcp->result; tcp->ptr != NULL; tcp->ptr = tcp->ptr->ai_next) {
		// Create a socket for connecting to server
		tcp->ConnectSocket = socket(tcp->ptr->ai_family, tcp->ptr->ai_socktype,
			tcp->ptr->ai_protocol);
		if (tcp->ConnectSocket == INVALID_SOCKET) {
			LOGE("socket failed with error: %ld", WSAGetLastError());
			free(tcp);
			return 0;
		}

		make_socket_nonblocking(tcp->ConnectSocket);

        fd_set fdset;
        // Connect to server
		connect(tcp->ConnectSocket, tcp->ptr->ai_addr, (int)tcp->ptr->ai_addrlen);
        FD_ZERO(&fdset);
        FD_SET(tcp->ConnectSocket, &fdset);
        struct timeval connect_timeout;
        connect_timeout.tv_sec = 3;
        connect_timeout.tv_usec = 0;
        if (select(tcp->ConnectSocket+1, NULL, &fdset, NULL, &connect_timeout) != 1) {
            LOGE("TCP connect timeout");
            closesocket(tcp->ConnectSocket);
            tcp->ConnectSocket = INVALID_SOCKET;
            continue;
        }
		break;
	}
	freeaddrinfo(tcp->result);
	if (tcp->ConnectSocket == INVALID_SOCKET) {
		LOGE("Unable to connect to server!");
		//free(tcp);
		return tcp;
	}

	if (get_cached_user_id(path_prefix, &tcp->user_id) == 0) {
		LOGI("Cached user id: %08x-%08x-%08x-%08x",
			tcp->user_id.v[0], tcp->user_id.v[1], tcp->user_id.v[2], tcp->user_id.v[3]);
		tcp_send_querynick(tcp, &tcp->user_id);
	} else {
		// Request a new user to be created
		tcp_send_newuser(tcp);
	}


	return tcp;
}

void destroy_tcp(LWTCP** tcp) {
	if (*tcp) {
		free(*tcp);
		*tcp = 0;
	}
}

void tcp_send(LWTCP* tcp, const char* data, int size) {
}

#define CHECK_PACKET(packet_type, packet_size, type) \
packet_type == LPGP_##type && packet_size == sizeof(type)

int parse_recv_packets(LWCONTEXT* pLwc, LWTCP* tcp) {
	// too small for parsing
	if (tcp->recvbufnotparsed < 2) {
		return -1;
	}
	int parsed_bytes = 0;
	char* cursor = tcp->recvbuf;
	while (1) {
		unsigned short packet_size = *(unsigned short*)(cursor + 0);
		// still incomplete packet
		if (packet_size == 0 || packet_size > tcp->recvbufnotparsed - parsed_bytes) {
			return parsed_bytes;
		}
		unsigned short packet_type = *(unsigned short*)(cursor + 2);
		if (CHECK_PACKET(packet_type, packet_size, LWPMATCHED2)) {
			LWPMATCHED2* p = (LWPMATCHED2*)cursor;
			LOGI("LWPMATCHED2 - bid:%d", p->battle_id);
			pLwc->puck_game->battle_id = p->battle_id;
			pLwc->puck_game->token = p->token;
			pLwc->puck_game->player_no = p->player_no;
			memcpy(pLwc->puck_game->target_nickname, p->target_nickname, sizeof(p->target_nickname));
			udp_update_addr(pLwc->udp, *(unsigned long*)p->ipaddr, p->port);
			//show_sys_msg(pLwc->def_sys_msg, "LWPMATCHED2 received");
		} else if (CHECK_PACKET(packet_type, packet_size, LWPQUEUEOK)) {
			LOGI("LWPQUEUEOK received");
			//show_sys_msg(pLwc->def_sys_msg, "LWPQUEUEOK received");
		} else if (CHECK_PACKET(packet_type, packet_size, LWPRETRYQUEUE)) {
			LOGI("LWPRETRYQUEUE received");
			//show_sys_msg(pLwc->def_sys_msg, "LWPRETRYQUEUE received");
			// Resend QUEUE2
			tcp_send_queue2(tcp, &pLwc->tcp->user_id);
		} else if (CHECK_PACKET(packet_type, packet_size, LWPMAYBEMATCHED)) {
			LOGI("LWPMAYBEMATCHED received");
			//show_sys_msg(pLwc->def_sys_msg, "LWPMAYBEMATCHED received");
		} else if (CHECK_PACKET(packet_type, packet_size, LWPNEWUSERDATA)) {
			LOGI("LWPNEWUSERDATA received");
			LWPNEWUSERDATA* p = (LWPNEWUSERDATA*)cursor;
			save_cached_user_id(pLwc->internal_data_path, (LWUNIQUEID*)p->id);
			get_cached_user_id(pLwc->internal_data_path, &pLwc->tcp->user_id);
			LOGI("[NEW] Cached user nick: %s, id: %08x-%08x-%08x-%08x",
				p->nickname,
				pLwc->tcp->user_id.v[0],
				pLwc->tcp->user_id.v[1],
				pLwc->tcp->user_id.v[2],
				pLwc->tcp->user_id.v[3]);
			memcpy(pLwc->puck_game->nickname, p->nickname, sizeof(char) * LW_NICKNAME_MAX_LEN);
			tcp_send_queue2(tcp, &pLwc->tcp->user_id);
		} else if (CHECK_PACKET(packet_type, packet_size, LWPNICK)) {
			LOGI("LWPNICK received");
			LWPNICK* p = (LWPNICK*)cursor;
			LOGI("Cached user nick: %s", p->nickname);
			memcpy(pLwc->puck_game->nickname, p->nickname, sizeof(char) * LW_NICKNAME_MAX_LEN);
			tcp_send_queue2(tcp, &pLwc->tcp->user_id);
		} else {
			LOGE("Unknown TCP packet");
		}
		parsed_bytes += packet_size;
		cursor += packet_size;
	}
	return parsed_bytes;
}

void tcp_update(LWCONTEXT* pLwc, LWTCP* tcp) {
	if (!pLwc || !tcp) {
		return;
	}
	if (LW_TCP_BUFLEN - tcp->recvbufnotparsed <= 0) {
		LOGE("TCP receive buffer overrun!!!");
	}
	int n = recv(tcp->ConnectSocket, tcp->recvbuf + tcp->recvbufnotparsed, LW_TCP_BUFLEN - tcp->recvbufnotparsed, 0);
	if (n > 0) {
		LOGI("TCP received: %d bytes", n);
		tcp->recvbufnotparsed += n;
		int parsed_bytes = parse_recv_packets(pLwc, tcp);
		if (parsed_bytes > 0) {
			for (int i = 0; i < LW_TCP_BUFLEN - parsed_bytes; i++) {
				tcp->recvbuf[i] = tcp->recvbuf[i + parsed_bytes];
			}
			tcp->recvbufnotparsed -= parsed_bytes;
		}
	}
}

int tcp_send_newuser(LWTCP* tcp) {
	NEW_TCP_PACKET(LWPNEWUSER, p);
	memcpy(tcp->sendbuf, &p, sizeof(p));
	return send(tcp->ConnectSocket, tcp->sendbuf, (int)sizeof(p), 0);
}

int tcp_send_querynick(LWTCP* tcp, const LWUNIQUEID* id) {
	NEW_TCP_PACKET_CAPITAL(LWPQUERYNICK, p);
	memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
	memcpy(tcp->sendbuf, &p, sizeof(p));
	return send(tcp->ConnectSocket, tcp->sendbuf, (int)sizeof(p), 0);
}

int tcp_send_queue2(LWTCP* tcp, const LWUNIQUEID* id) {
	NEW_TCP_PACKET_CAPITAL(LWPQUEUE2, p);
	memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
	memcpy(tcp->sendbuf, &p, sizeof(p));
	return send(tcp->ConnectSocket, tcp->sendbuf, (int)sizeof(p), 0);
}

int tcp_send_suddendeath(LWTCP* tcp, int battle_id, unsigned int token) {
	NEW_TCP_PACKET_CAPITAL(LWPSUDDENDEATH, p);
	p.Battle_id = battle_id;
	p.Token = token;
	memcpy(tcp->sendbuf, &p, sizeof(p));
	return send(tcp->ConnectSocket, tcp->sendbuf, (int)sizeof(p), 0);
}

const char* tcp_addr() {
    return LW_TCP_SERVER;
}

int tcp_port() {
    return LW_TCP_PORT;
}
