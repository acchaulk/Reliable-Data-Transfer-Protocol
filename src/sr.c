/*
 * sr.c
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "sr.h"
#include "common.h"

static int g_windowSize;
static pthread_t g_masterThread;
static timer_t *g_timerList;
static int* g_pktAck;  /*1 for ACK, 0 for non-ACK, vector for keeping track of each ack within slide window */
static int* g_sendflag;
static char* g_sharedBuffer;
static ackTimer_t* ackTimer;

// Only purpose here is changing the default behavior of SIGALRM
static void dummy_handler(int ingored) {}

static void handler(union sigval si) {
	int i;
	for (i = 0; i < g_windowSize; i++) {
		if (ackTimer[i].timer_id == si.sival_ptr) {
			// trigger retransmission, implement the code here
			g_sendflag[ackTimer[i].seq_no % g_windowSize] = 1;
			printf("timer %d timeout\n", ackTimer[i].seq_no);
			pthread_kill(g_masterThread, SIGALRM);
			return;
		}
	}
}

void dequeue(int* sendflag) {
	int i;
	for (i = 0; i < g_windowSize - 1; i++) {
		sendflag[i] = sendflag[i + 1];
	}
	sendflag[i] = 1;
}

void sr_init(int windowSize, double lossRate, double corruptionRate) {
	int i;

	physical_init(lossRate, corruptionRate);
	g_windowSize = windowSize;
	g_masterThread = pthread_self();

	ackTimer = malloc (sizeof(ackTimer_t) * g_windowSize);
	// create timer list, add timers into the list
	g_timerList = malloc(sizeof(timer_t) * g_windowSize);
	for (i = 0; i < g_windowSize; i++) {
		make_timer(g_timerList, i, handler, 0);
	}

	g_sendflag = malloc (sizeof(int) * g_windowSize);
	for (i = 0; i < g_windowSize; i++) {
		g_sendflag[i] = 1;
	}
}

/**
 * Window size: 3
 * each slot within the window should has a associated timer
 * both the sender and receiver have sliding window
 */

size_t sr_send(int sockfd, char* buffer, size_t length) {
	int nPackets = 0;				/* number of packets to send */
	struct sigaction timerAction;	/* For setting signal handler */
	int ackRecv = -1;				/* number of ack received */
	int pktSent = -1;				/* number of packet sent */
	int lowestSent = -1;            /* lowest sent packet*/
	int base = 0;                   /* beginning index of slide window */
	int respLen;					/* Size of received frame */
	int bytesSent = 0;

	// make the message visible across all threads
	g_sharedBuffer = malloc(length);
	memcpy(g_sharedBuffer, buffer, length);


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

	g_pktAck = malloc (sizeof (int) * nPackets);
	memset(g_pktAck, 0, sizeof(int) * nPackets);

	while ((ackRecv < nPackets-1)) {
		int ctr; /*window size counter */
		/* Open sender-side slide window and move forward */
		for (ctr = 0; ctr < g_windowSize; ctr++) {
			if (g_sendflag[ctr] > 0)	{
				g_sendflag[ctr] = 0;

				/* current packet we're working with */
				Frame_t currFrame;
				if ((base + ctr) < nPackets && g_pktAck[base + ctr] == 0) {
					memset(&currFrame, 0, sizeof(currFrame));

					/*convert to network endianness */
					currFrame.pkt.type = htonl (PACKET);
					currFrame.pkt.seq_no = htonl (base + ctr);

					/*length of last packet is special */
					int currlength;
					if ((length - ((base + ctr) * CHUNKSIZE)) >= CHUNKSIZE) {
						currlength = CHUNKSIZE;
					} else {
						currlength = length % CHUNKSIZE;
					}
					bytesSent += currlength;
					currFrame.pkt.length = htonl (currlength);

					/*copy buffer data into packet */
					memcpy (currFrame.pkt.data,
							buffer + ((base + ctr) * CHUNKSIZE), currlength);
					currFrame.checksum = htonl (calcChecksum(&(currFrame.pkt)));

					/*send via physical layer */
					int bytes = physical_send(sockfd, &currFrame, FRAME_HEADER + currlength);
					if ( bytes < 0) {
						die ("physical_send() sent a different number of bytes than expected");
					} else if (bytes == 0) {
						printf("drop packet %d\n", ntohl(currFrame.pkt.seq_no));
					} else {
						printf ("---- SEND PACKET %d\n", base + ctr);
					}

					/* start a timer to track the packet */
					reset_timer(g_timerList, ctr, TIMEOUT, 0);
					ackTimer[ctr].timer_id = g_timerList + ctr;
					ackTimer[ctr].seq_no = base + ctr;
				} /* if ((base + ctr) < nPackets) */
			} /* if (g_sendflag > 0) */
		} /* for loop */

		/* Get a response */
		Frame_t currAck;
		respLen = physical_recv (sockfd, &currAck, sizeof(currAck));
		if (respLen < 0) {
			if (errno == EINTR) { /* Alarm went off  */
				//if (g_tries < MAXTRIES)	{ /* incremented by signal handler */
				printf ("timed out, more tries...\n");
				//				} else {
				//					die ("No Response");
				//				}
			} else {
				die ("physical_recv failed");
			}
		}
		/* recv() got something --  cancel the timeout */
		if (respLen) {
			int acktype = ntohl (currAck.pkt.type); /* convert to host byte order */
			int ackno = ntohl (currAck.pkt.seq_no);
			if (acktype == ACK_MSG) {
				if (ackno == base) {
					base++;
					dequeue(g_sendflag);
					reset_timer(g_timerList, ackno % g_windowSize, 0, 0); //cancel timer of this packet
				}
				if (ackno > base) {
					reset_timer(g_timerList, ackno % g_windowSize, 0, 0); //cancel timer of this packet
					g_sendflag[ackno % g_windowSize] = 1;
				}
				printf ("---- RECEIVE ACK %d\n", ackno); /* receive/handle ack */
				ackRecv++;
				/* Set the corresponding Ack vector to 1 */
				g_pktAck[ackno] = 1;
			}
		}
	} /* while ((ackRecv < nPackets-1)) */

	int ctr;
	for (ctr = 0; ctr < 1; ctr++) /* send 1 teardown packets - don't have to necessarily send 10 but spec said "up to 10" */
	{
		Frame_t teardown;
		teardown.pkt.type = htonl (TEARDOWN);
		teardown.pkt.seq_no = htonl (0);
		teardown.pkt.length = htonl (0);
		teardown.checksum = htonl (calcChecksum(&(teardown.pkt)));
		while (1) {
			int bytesSent = physical_send (sockfd, &teardown, sizeof(teardown));
			printf("---- SEND TEARDOWN\n");
			if (bytesSent == sizeof(teardown)) {
				printf("teardown msg reach the other side\n");
				break;
			}
		}
	}
	return 0;
}

size_t sr_recv(int sockfd, char* buffer) {
	int recvMsgSize = 0;		/* Size of received message */
	int packet_rcvd = -1;		/* highest packet successfully received */
//	int base = 0;               /* receiver slide window */
	struct sigaction myAction;

	while (1) {
		Frame_t currFrame;
		memset(&currFrame, 0, sizeof(currFrame));
		/* Block until receive message from a client */
		if (physical_recv (sockfd, &currFrame, sizeof (currFrame)) == 0) {
			continue; // corruption detected
		}

		currFrame.pkt.type = ntohl (currFrame.pkt.type);
		currFrame.pkt.length = ntohl (currFrame.pkt.length); /* convert from network to host byte order */
		currFrame.pkt.seq_no = ntohl (currFrame.pkt.seq_no);
		if (currFrame.pkt.type == TEARDOWN) /* tear-down message */
		{
			// assume tear-down message is always received
			printf("---- RECEIVE TEARDOWN\n");
			return recvMsgSize;
		} else if (currFrame.pkt.type == PACKET) {
			printf ("---- RECEIVE PACKET %d length %d\n", currFrame.pkt.seq_no, currFrame.pkt.length);

			/* Send ack and store in buffer */
//			if (currFrame.pkt.seq_no == base) {
//				base++;
//			}
			int buff_offset = CHUNKSIZE * currFrame.pkt.seq_no;
			memcpy (&buffer[buff_offset], currFrame.pkt.data, /* copy packet data to buffer */
					currFrame.pkt.length);
			recvMsgSize += currFrame.pkt.length;

			Frame_t currAck;
			currAck.pkt.type = htonl (ACK_MSG); /*convert to network byte order */
			currAck.pkt.seq_no = htonl (currFrame.pkt.seq_no);
			currAck.pkt.length = htonl(0);
			currAck.checksum = htonl(calcChecksum(&(currAck.pkt)));
			int bytes = physical_send (sockfd, &currAck, sizeof(currAck));
			if ( bytes < 0) {
				die ("physical_send() sent a different number of bytes than expected");
			} else if (bytes == 0) {
				printf("ack msg %d loss\n", currFrame.pkt.seq_no);
			} else {
				printf ("---- SEND ACK %d\n", currFrame.pkt.seq_no);
			}
		}
	}

	return recvMsgSize;
}
