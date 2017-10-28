#include "lwtcp.h"
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

LWTCP* new_tcp() {
#if 0
	LWTCP* tcp = (LWTCP*)malloc(sizeof(LWTCP));
	memset(tcp, 0, sizeof(LWTCP));
	tcp->ConnectSocket = INVALID_SOCKET;
	strcpy(tcp->sendbuf, "Send buf content goes here...");
	tcp->recvbuflen = LW_TCP_BUFLEN;

	tcp->hints.ai_family = AF_UNSPEC;
	tcp->hints.ai_socktype = SOCK_STREAM;
	tcp->hints.ai_protocol = IPPROTO_TCP;
	
	tcp->iResult = getaddrinfo(LW_TCP_SERVER, LW_TCP_PORT, &tcp->hints, &tcp->result);
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
		// Connect to server
		tcp->iResult = connect(tcp->ConnectSocket, tcp->ptr->ai_addr, (int)tcp->ptr->ai_addrlen);
		if (tcp->iResult == SOCKET_ERROR) {
			closesocket(tcp->ConnectSocket);
			tcp->ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(tcp->result);
	if (tcp->ConnectSocket == INVALID_SOCKET) {
		LOGE("Unable to connect to server!");
		free(tcp);
		return 0;
	}
	tcp->iResult = send(tcp->ConnectSocket, tcp->sendbuf, (int)strlen(tcp->sendbuf), 0);
	if (tcp->iResult == SOCKET_ERROR) {
		LOGE("send failed with error: %d", WSAGetLastError());
		closesocket(tcp->ConnectSocket);
		free(tcp);
		return 0;
	}
	LOGI("Bytes sent: %ld", tcp->iResult);
	tcp->iResult = shutdown(tcp->ConnectSocket, SD_SEND);
	if (tcp->iResult == SOCKET_ERROR) {
		LOGI("shutdown failed with error: %d", WSAGetLastError());
		closesocket(tcp->ConnectSocket);
		free(tcp);
		return 0;
	}

	return tcp;
#else
    return 0;
#endif
}

void destroy_tcp(LWTCP** tcp) {
	if (*tcp) {
		free(*tcp);
		*tcp = 0;
	}
}

void tcp_send(LWTCP* tcp, const char* data, int size) {
}

void tcp_update(LWCONTEXT* pLwc, LWTCP* tcp) {
}
