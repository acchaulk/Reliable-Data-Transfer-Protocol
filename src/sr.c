/*
 * sr.c
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#include "sr.h"
#include "common.h"

static int g_windowSize;
static pthread_t g_masterThread;
static timer_t *g_timerList;

// Only purpose here is changing the default behavior of SIGALRM
static void dummy_handler(int ingored) {}

static void handler(union sigval si) {
	int i;
	for (i = 0; i < g_windowSize; i++) {
		if (&(g_timerList[i]) == si.sival_ptr) {
			// trigger retransmission, implement the code here
			// ...
			pthread_kill(g_masterThread, SIGALRM);
			return;
		}
	}
}

void sr_init(int windowSize, double lossRate, double corruptionRate) {
	int i;

	physical_init(lossRate, corruptionRate);
	g_windowSize = windowSize;
	g_masterThread = pthread_self();
	// create timer list, add timers into the list
	g_timerList = malloc(sizeof(timer_t) * g_windowSize);
	for (i = 0; i < g_windowSize; i++) {
		make_timer(g_timerList, i, handler, 0);
	}
}

/**
 * Window size: 5
 * each slot within the window should has a associated timer
 * both the sender and receiver have sliding window
 */

size_t sr_send(int sockfd, char* buf, size_t length) {
	int nPackets = 0;		/* number of packets to send */
	struct sigaction timerAction;	/* For setting signal handler */
	int pktRecv = -1;	/* number of ack received */


	/* Set signal handler for alarm signal */
	timerAction.sa_handler = dummy_handler;
	/* block everything in handler */
	if (sigfillset (&timerAction.sa_mask) < 0) {
		die ("sigfillset() failed");
	}
	timerAction.sa_flags = 0;
	if (sigaction (SIGALRM, &timerAction, 0) < 0) {
		die ("sigaction() failed for SIGALRM");
	}

	nPackets = length / CHUNKSIZE;
	if (length % CHUNKSIZE) {
		nPackets++;			/* if it doesn't divide cleanly, need one more odd-sized packet */
	}

	while ((pktRecv < nPackets-1)) {
		int ctr; /*window size counter */
		for (ctr = 0; ctr < g_windowSize; ctr++) {

		}
	}
	return 0;
}

size_t sr_recv(int sockfd, char* buf) {
	return 0;
}
