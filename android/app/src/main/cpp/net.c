/*
    Simple udp client
*/
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include <lwlog.h>
#include "net.h"

#define SERVER "222.110.4.119"
#define BUFLEN 512  //Max length of buffer
#define PORT 10001   //The port on which to send data

void die(char *s)
{
	perror(s);
	exit(1);
}

void init_net() {
	struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);
	char buf[BUFLEN];
	const char* message = "android ^__^";

	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);

	if (inet_aton(SERVER , &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	//send the message
	if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
	{
		die("sendto()");
	}

	//receive a reply and print it
	//clear the buffer by filling null, it might have previously received data
	memset(buf,'\0', BUFLEN);
	//try to receive some data, this is a blocking call
	if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
	{
		die("recvfrom()");
	}

	LOGV("NET: %s", buf);

	close(s);
}
