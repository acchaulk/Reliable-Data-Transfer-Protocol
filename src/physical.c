/*
 * physical.c
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#include <stdio.h>
#include <sys/socket.h>		/* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>		/* for sockaddr_in and inet_addr() */
#include <pthread.h>
#include "physical.h"

double g_lossRate;
double g_corruptionRate;

void corrupt_pkt(Packet_t* pkt) {
	//memset(pkt, 0, 2);
}

/* 0 for no corruption */
int detect_corruption(Packet_t* pkt) {
	return 0;
}

void physical_init(double lossRate, double corruptionRate) {
	g_lossRate = lossRate;
	g_corruptionRate = corruptionRate;

	srand48(123456789);/* seed the pseudorandom number generator */
}

int physical_send(int sockfd, Packet_t* pkt, size_t data_length) {
	if(g_lossRate > drand48())
		return 0; /* drop packet - for testing/debug purposes */
	if(g_corruptionRate > drand48()) {
		corrupt_pkt(pkt);
	}

	size_t numbytes = send(sockfd, pkt, data_length, 0);
	if (numbytes == -1) {
		perror("physical_send() sent a different number of bytes than expected");
		return -1;
	}
	return numbytes;
}

size_t physical_recv(int sockfd, Packet_t* pkt, size_t data_length) {

	size_t numbytes = recv(sockfd, pkt, data_length, 0);
	if (numbytes == -1) {
		perror("physical_recv() receive a different number of bytes than expected");
		return -1;
	}
	if (detect_corruption(pkt)) {
		perror("corruption detected");
		return 0;
	}
	return numbytes;
}
