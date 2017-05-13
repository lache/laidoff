#include "zmq.h"
#include "zhelpers.h"
#include <stdio.h>      /* for printf(), fprintf() */
#include <winsock.h>    /* for socket(),... */
#include <stdlib.h>     /* for exit() */
#include "net.h"
#include "lwlog.h"

#define ECHOMAX 255     /* Longest string to echo */

UINT_PTR sock;                        /* Socket descriptor */
struct sockaddr_in echoServAddr; /* Echo server address */
struct sockaddr_in fromAddr;     /* Source address of echo */
unsigned short echoServPort;     /* Echo server port */
int fromSize;           /* In-out of address size for recvfrom() */
char *servIP;                    /* IP address of server */
char *echoString;                /* String to send to echo server */
char echoBuffer[ECHOMAX];        /* Buffer for echo string */
int echoStringLen;               /* Length of string to echo */
int respStringLen;               /* Length of response string */
WSADATA wsaData;                 /* Structure for WinSock setup communication */

void DieWithError(char *errorMessage) {
	LOGE("NET ERROR: %s", errorMessage);
}

void init_zmq() {
	//  Socket to talk to server
	printf("Collecting updates from weather server¡¦\n");
	void *context = zmq_ctx_new();
	void *subscriber = zmq_socket(context, ZMQ_SUB);
	int rc = zmq_connect(subscriber, "tcp://localhost:5556");
	assert(rc == 0);

	//  Subscribe to zipcode, default is NYC, 10001
	char *filter = "10001 ";
	rc = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE,
		filter, strlen(filter));
	assert(rc == 0);

	//  Process 100 updates
	int update_nbr;
	long total_temp = 0;
	for (update_nbr = 0; update_nbr < 100; update_nbr++) {
		char *string = s_recv(subscriber);

		int zipcode, temperature, relhumidity;
		sscanf(string, "%d %d %d",
			&zipcode, &temperature, &relhumidity);
		total_temp += temperature;
		free(string);
	}
	printf("Average temperature for zipcode '%s' was %dF\n",
		filter, (int)(total_temp / update_nbr));

	zmq_close(subscriber);
	zmq_ctx_destroy(context);
}

void init_net(struct _LWCONTEXT* pLwc) {
	servIP = "222.110.4.119";
	echoString = "desktop!!";
	echoServPort = 10001;
	echoStringLen = (int)strlen(echoString);

	/* Load Winsock 2.0 DLL */
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup() failed");
		exit(1);
	}

	/* Create a best-effort datagram socket using UDP */
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == 0) {
		DieWithError("socket() failed");
	}
		
	/* Construct the server address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
	echoServAddr.sin_family = AF_INET;                 /* Internet address family */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
	echoServAddr.sin_port = htons(echoServPort);     /* Server port */

	init_zmq();
}

void deinit_net(struct _LWCONTEXT* pLwc) {
	closesocket(sock);
	WSACleanup();  /* Cleanup Winsock */
}

void net_rtt_test(struct _LWCONTEXT* pLwc) {
	if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != echoStringLen)
		DieWithError("sendto() sent a different number of bytes than expected");

	/* Recv a response */

	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
		LOGE("NET Error: setsockopt");
	}

	fromSize = sizeof(fromAddr);
	if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize)) < 0) {
		DieWithError("recvfrom() failed");
	} else {
		if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
			LOGE("Error: received a packet from unknown source.");
		} else {
			echoBuffer[respStringLen] = '\0';
			LOGI("Received: %s", echoBuffer);    /* Print the echoed arg */
		}
	}
}
