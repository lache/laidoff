#include "lwtcp.h"
#include <string.h>
#include "lwlog.h"


#if !LW_PLATFORM_WIN32
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

int tcp_connect(LWTCP* tcp) {
    if (tcp->ConnectSocket) {
        closesocket(tcp->ConnectSocket);
    }
    tcp->ConnectSocket = INVALID_SOCKET;
    tcp->recvbuflen = LW_TCP_BUFLEN;

    
    // Attempt to connect to an address until one succeeds
    for (tcp->ptr = tcp->result; tcp->ptr != NULL; tcp->ptr = tcp->ptr->ai_next) {
        // Create a socket for connecting to server
        tcp->ConnectSocket = socket(tcp->ptr->ai_family, tcp->ptr->ai_socktype,
                                    tcp->ptr->ai_protocol);
        if (tcp->ConnectSocket == INVALID_SOCKET) {
            LOGE("socket failed with error: %ld", (long)WSAGetLastError());
            return -2;
        }
        
        make_socket_nonblocking(tcp->ConnectSocket);

        // Connect to server
        int connect_ret = connect(tcp->ConnectSocket, tcp->ptr->ai_addr, (int)tcp->ptr->ai_addrlen);
        if (connect_ret == -1) {
            int tcp_connect_errno = errno;
            if (tcp_connect_errno == 0) {
                LOGI("[INFO] TCP connect() returned value indicates it finished synchronously (connecting to localhost?)");
            } else {
                if (tcp_connect_errno != EINPROGRESS && tcp_connect_errno != 0) {
                    LOGE("TCP connect failed! (refused?)");
                    closesocket(tcp->ConnectSocket);
                    tcp->ConnectSocket = INVALID_SOCKET;
                    continue;
                }
                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(tcp->ConnectSocket, &fdset);
                struct timeval connect_timeout;
                connect_timeout.tv_sec = 3;
                connect_timeout.tv_usec = 0;
                int select_ret = select(tcp->ConnectSocket + 1, NULL, &fdset, NULL, &connect_timeout);
                if (select_ret == 0) {
                    LOGE("TCP connect timeout");
                    closesocket(tcp->ConnectSocket);
                    tcp->ConnectSocket = INVALID_SOCKET;
                    continue;
                }
                if (select_ret != 1) {
                    LOGE("TCP connect select failed");
                    closesocket(tcp->ConnectSocket);
                    tcp->ConnectSocket = INVALID_SOCKET;
                    continue;
                }
                if (!FD_ISSET(tcp->ConnectSocket, &fdset)) {
                    LOGE("TCP connect failed!");
                    closesocket(tcp->ConnectSocket);
                    tcp->ConnectSocket = INVALID_SOCKET;
                    continue;
                }
            }
            int optval;
            socklen_t optlen;
            optval = -1;
            optlen = sizeof(optval);
            if (getsockopt(tcp->ConnectSocket, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) == -1) {
                LOGE("getsockopt error");
                continue;
            }
            if (optval == 0) {
                // Connection ok.
            } else {
                LOGE("Connection error, optval=%d (%s)", optval, strerror(optval));
                closesocket(tcp->ConnectSocket);
                tcp->ConnectSocket = INVALID_SOCKET;
                continue;
            }
        }
        break;
    }
    //freeaddrinfo(tcp->result);
    if (tcp->ConnectSocket == INVALID_SOCKET) {
        LOGE("Unable to connect to server!");
        return -3;
    }
#if LW_PLATFORM_IOS
    int set = 1;
    setsockopt (tcp->ConnectSocket, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof (int));
#endif
    return 0;
}

LWTCP* new_tcp(LWCONTEXT* pLwc,
               const char* path_prefix,
               const LWHOSTADDR* host_addr,
               LWTCP_ON_CONNECT on_connect,
               LWTCP_ON_RECV_PACKETS on_recv_packets) {
	LWTCP* tcp = (LWTCP*)malloc(sizeof(LWTCP));
	memset(tcp, 0, sizeof(LWTCP));
    tcp->hints.ai_family = AF_UNSPEC;
    tcp->hints.ai_socktype = SOCK_STREAM;
    tcp->hints.ai_protocol = IPPROTO_TCP;
    tcp->on_connect = on_connect;
    tcp->on_recv_packets = on_recv_packets;
    memcpy(&tcp->host_addr, host_addr, sizeof(LWHOSTADDR));
    tcp->pLwc = pLwc;
#if !LW_PLATFORM_WIN32
    // Handle SIGPIPE in our side (prevent crashing iOS app)
    signal(SIGPIPE, SIG_IGN);
#endif
    
    tcp->iResult = getaddrinfo(host_addr->host, host_addr->port_str, &tcp->hints, &tcp->result);
    if (tcp->iResult != 0) {
        LOGE("getaddrinfo failed with error: %d", tcp->iResult);
        free(tcp);
        return 0;
    }
    
    if (tcp_connect(tcp) < 0) {
        free(tcp);
        return 0;
    }

    if (tcp->on_connect) {
        tcp->on_connect(tcp, path_prefix);
    }
    
	return tcp;
}

void destroy_tcp(LWTCP** tcp) {
	if (*tcp) {
        freeaddrinfo((*tcp)->result);
		free(*tcp);
		*tcp = 0;
	}
}

void tcp_update(LWTCP* tcp) {
	if (!tcp) {
		return;
	}
	if (LW_TCP_BUFLEN - tcp->recvbufnotparsed <= 0) {
		LOGE("TCP receive buffer overrun!!!");
	}
	int n = (int)recv(tcp->ConnectSocket, tcp->recvbuf + tcp->recvbufnotparsed, LW_TCP_BUFLEN - tcp->recvbufnotparsed, 0);
	if (n > 0) {
        if (tcp->on_recv_packets) {
            LOGI("TCP received: %d bytes", n);
            tcp->recvbufnotparsed += n;
            int parsed_bytes = tcp->on_recv_packets(tcp);
            if (parsed_bytes > 0) {
                for (int i = 0; i < LW_TCP_BUFLEN - parsed_bytes; i++) {
                    tcp->recvbuf[i] = tcp->recvbuf[i + parsed_bytes];
                }
                tcp->recvbufnotparsed -= parsed_bytes;
            }
        }
	}
}
