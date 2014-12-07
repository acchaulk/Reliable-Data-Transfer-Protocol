/*
 * sr.c
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#include "sr.h"
#include "common.h"

static int g_windowSize;

void sr_init(int windowSize, double lossRate, double corruptionRate) {
	physical_init(lossRate, corruptionRate);
	g_windowSize = windowSize;
}

/**
 * Window size: 5
 * each slot within the window should has a associated timer
 * both the sender and receiver have sliding window
 */

size_t sr_send(int sockfd, char* buf, size_t length) {
	int nPackets = 0;		/* number of packets to send */

	nPackets = length / CHUNKSIZE;
	if (length % CHUNKSIZE) {
		nPackets++;			/* if it doesn't divide cleanly, need one more odd-sized packet */
	}
	return 0;
}

size_t sr_recv(int sockfd, char* buf) {
	return 0;
}
