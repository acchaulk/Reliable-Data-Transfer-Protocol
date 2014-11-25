/*
 * physical.c
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#include <stdio.h>
#include <sys/socket.h>		/* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>		/* for sockaddr_in and inet_addr() */
#include "physical.h"

int physical_send(int sockfd, Packet_t* pkt, size_t data_length) {

	size_t numbytes = send(sockfd, pkt, HEADER_SIZE + data_length, 0);
	if (numbytes == -1) {
		perror("physical_send() sent a different number of bytes than expected");
		return -1;
	}
	return numbytes;
}


size_t physical_recv(int sockfd, Packet_t* pkt, size_t data_length) {

	size_t numbytes = recv(sockfd, pkt, HEADER_SIZE + data_length, 0);
	if (numbytes == -1) {
		perror("physical_recv() receive a different number of bytes than expected");
		return -1;
	}
	return numbytes;
}
