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
#include <string.h>
#include <time.h>

#include "physical.h"

double g_lossRate;
double g_corruptionRate;

void corrupt_pkt(Frame_t* frame) {
	memset((void *)frame + FRAME_HEADER, 0xffff, 2);
}

/* 0 for no corrupt, 1 for corrupted */
int detect_corruption(Frame_t* frame) {
	// Discard corrupted frame
	frame->checksum = ntohl (frame->checksum);
	if (frame->checksum != calcChecksum(&(frame->pkt))) {
//		printf("currFrame.checksum=%u, calc checksum=%u\n", frame.checksum, calcChecksum(&(frame.pkt)));
//		printf("currFrame(type=%d, seq_no=%d, data={%d}, length=%d)\n",
//				frame.pkt.type, frame.pkt.seq_no, frame.pkt.data[0], frame.pkt.length);
		return 1;
	}
	return 0;
}

void physical_init(double lossRate, double corruptionRate) {
	g_lossRate = lossRate;
	g_corruptionRate = corruptionRate;

	srand48(time(NULL));/* seed the pseudorandom number generator */
}

int physical_send(int sockfd, Frame_t* frame, size_t data_length) {
	if(g_lossRate > drand48())
		return 0; /* drop packet - for testing/debug purposes */
	if(g_corruptionRate > drand48()) {
		corrupt_pkt(frame);
	}

	size_t numbytes = send(sockfd, frame, data_length, 0);
	g_gbnStat.frameSent++;
	if (numbytes == -1) {
		perror("physical_send() sent a different number of bytes than expected");
		return -1;
	}
	return numbytes;
}

size_t physical_recv(int sockfd, Frame_t* frame, size_t data_length) {

	size_t numbytes = recv(sockfd, frame, data_length, 0);
	if (numbytes == -1) {
		perror("physical_recv() receive a different number of bytes than expected");
		return -1;
	}
	if (detect_corruption(frame)) {
		printf("corruption detected seq_no %d\n", ntohl(frame->pkt.seq_no));
		return 0;
	}
	return numbytes;
}
