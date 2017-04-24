#include <stdio.h>      /* for printf(), fprintf() */
#include <winsock.h>    /* for socket(),... */
#include <stdlib.h>     /* for exit() */
#include "net.h"

#define ECHOMAX 255     /* Longest string to echo */

void DieWithError(char *errorMessage) {

}

void init_net(struct _LWCONTEXT* pLwc) {
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

	servIP = "222.110.4.119";
	echoString = "desktop!!";
	echoServPort = 10001;
	echoStringLen = (int)strlen(echoString);
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) /* Load Winsock 2.0 DLL */
	{
		fprintf(stderr, "WSAStartup() failed");
		exit(1);
	}
	/* Create a best-effort datagram socket using UDP */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");
	/* Construct the server address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
	echoServAddr.sin_family = AF_INET;                 /* Internet address family */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
	echoServAddr.sin_port = htons(echoServPort);     /* Server port */
													 /* Send the string, including the null terminator, to the server */
	if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != echoStringLen)
		DieWithError("sendto() sent a different number of bytes than expected");

	/* Recv a response */

	fromSize = sizeof(fromAddr);
	if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize)) != echoStringLen)
		DieWithError("recvfrom() failed");
	if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
		fprintf(stderr, "Error: received a packet from unknown source.\n");
		exit(1);
	}
	echoBuffer[respStringLen] = '\0';
	printf("Received: %s\n", echoBuffer);    /* Print the echoed arg */
	closesocket(sock);
	WSACleanup();  /* Cleanup Winsock */
}

