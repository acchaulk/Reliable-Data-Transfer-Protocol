/*
 * gobackn.c
 *
 * Created on: 2014/11/21
 * Author: Yang Lu
 */

#include <stdio.h>			/* for printf() and fprintf() */
#include <stdlib.h>			/* for atoi() and exit() */
#include <string.h>			/* for memset() */
#include <unistd.h>			/* for close() and alarm() */
#include <errno.h>			/* for errno and EINTR */
#include <signal.h>			/* for sigaction() */
#include <semaphore.h>      /* for semaphore */
#include <pthread.h>		/* for pthread_create */

#include "gbn.h"
#include "packet.h"
#include "common.h"
#include "physical.h"

static int g_windowSize;

int tries = 0;			/* Count of times sent - GLOBAL for signal-handler access */
int sendflag = 1;

void
CatchAlarm (int ignored)	/* Handler for SIGALRM */
{
	tries += 1;
	sendflag = 1;
}

void
DieWithError (char *errorMessage)
{
	perror (errorMessage);
	exit (1);
}

int
max (int a, int b)
{
	if (b > a)
		return b;
	return a;
}

int
min(int a, int b)
{
	if(b>a)
		return a;
	return b;
}

// checksum function
int calcChecksum(Packet_t * pkt) {
	return 0;
}

void gbn_init(int windowSize, double lossRate, double corruptionRate) {
	g_windowSize = windowSize;
	physical_init(lossRate, corruptionRate);
}

size_t gbn_send(int sockfd, char* buffer, size_t length) {
	struct sigaction timerAction;	/* For setting signal handler */
	int respLen;			/* Size of received datagram */
	int pkt_recv = -1;	/* highest ack received */
	int pkt_send = -1;		/* highest packet sent */
	int nPackets = 0;		/* number of packets to send */
	int base = 0;
	int bytesSent = 0;

	nPackets = length / CHUNKSIZE;
	if (length % CHUNKSIZE) {
		nPackets++;			/* if it doesn't divide cleanly, need one more odd-sized packet */
	}

	/* Set signal handler for alarm signal */
	timerAction.sa_handler = CatchAlarm;
	/* block everything in handler */
	if (sigfillset (&timerAction.sa_mask) < 0) {
		DieWithError ("sigfillset() failed");
	}
	timerAction.sa_flags = 0;
	if (sigaction (SIGALRM, &timerAction, 0) < 0) {
		DieWithError ("sigaction() failed for SIGALRM");
	}

	/* Send the string to the server */
	while ((pkt_recv < nPackets-1) && (tries < MAXTRIES))
	{
		// printf ("in the send loop base %d packet_sent %d packet_received %d\n",
		//      base, packet_sent, packet_received);
		if (sendflag > 0)	{
			sendflag = 0;
			int ctr; /*window size counter */
			for (ctr = 0; ctr < g_windowSize; ctr++)
			{
				pkt_send = min(max (base + ctr, pkt_send),nPackets-1); /* calc highest packet sent */
				Packet_t currpacket; /* current packet we're working with */
				if ((base + ctr) < nPackets)
				{
					memset(&currpacket,0,sizeof(currpacket));
					printf ("sending packet %d packet_sent %d packet_received %d\n",
							base+ctr, pkt_send, pkt_recv);

					currpacket.type = htonl (PACKET); /*convert to network endianness */
					currpacket.seq_no = htonl (base + ctr);
					int currlength;
					if ((length - ((base + ctr) * CHUNKSIZE)) >= CHUNKSIZE) /* length chunksize except last packet */
						currlength = CHUNKSIZE;
					else
						currlength = length % CHUNKSIZE;
					bytesSent += currlength;
					currpacket.length = htonl (currlength);
					memcpy (currpacket.data, /*copy buffer data into packet */
							buffer + ((base + ctr) * CHUNKSIZE), currlength);
					currpacket.checksum = htonl (calcChecksum(&currpacket));
					if (physical_send(sockfd, &currpacket, HEADER_SIZE + currlength) < 0) {
						DieWithError
						("physical_send() sent a different number of bytes than expected");
					}
				}
			}
		}
		/* Get a response */

		alarm (TIMEOUT);	/* Set the timeout */
		Packet_t currAck;
		respLen = physical_recv (sockfd, &currAck, sizeof(currAck));
		while (respLen < 0) {
			if (errno == EINTR)	/* Alarm went off  */
			{
				if (tries < MAXTRIES)	/* incremented by signal handler */
				{
					printf ("timed out, %d more tries...\n", MAXTRIES - tries);
					break;
				}
				else
					DieWithError ("No Response");
			}
			else
				DieWithError ("physical_recv failed");
		}

		/* recvfrom() got something --  cancel the timeout */
		if (respLen) {
			int acktype = ntohl (currAck.type); /* convert to host byte order */
			int ackno = ntohl (currAck.seq_no);
			int checksum = ntohl (currAck.checksum);

			// TODO: if checksum is not match the data, discard the ACK

			if (ackno > pkt_recv && acktype == ACK_MSG) {
				printf ("received ack %d\n", ackno); /* receive/handle ack */
				pkt_recv = ackno;
				base = pkt_recv + 1; /* handle new ack */
				if (pkt_recv == pkt_send) { /* all sent packets acked */
					alarm (0); /* clear alarm */
					tries = 0;
					sendflag = 1;
				}
				else { /* not all sent packets acked */
					tries = 0; /* reset retry counter */
					sendflag = 0;
					alarm(TIMEOUT); /* reset alarm */
				}
			}
		}
	}
	int ctr;
	for (ctr = 0; ctr < 1; ctr++) /* send 1 teardown packets - don't have to necessarily send 10 but spec said "up to 10" */
	{
		Packet_t teardown;
		teardown.type = htonl (TEARDOWN);
		teardown.seq_no = htonl (0);
		teardown.length = htonl (0);
		teardown.checksum = htonl (calcChecksum(&teardown));
		while (1) {
			int bytesSent = physical_send (sockfd, &teardown, sizeof(teardown));
			printf("send out teardown msg\n");
			if (bytesSent == sizeof(teardown)) {
				printf("teardown msg reach the other side\n");
				break;
			}
		}
	}
	return bytesSent;
}


size_t gbn_recv(int sockfd, char* buffer) {
	int recvMsgSize = 0;		/* Size of received message */
	int packet_rcvd = -1;		/* highest packet successfully received */
	struct sigaction myAction;

	while (1) {
		Packet_t currPacket; /* current packet that we're working with */
		memset(&currPacket, 0, sizeof(currPacket));
		/* Block until receive message from a client */
		//while ((recvMsgSize = physical_recv (sockfd, &currPacket, sizeof (currPacket))) > 0) {

		physical_recv (sockfd, &currPacket, sizeof (currPacket));
		currPacket.type = ntohl (currPacket.type);
		currPacket.length = ntohl (currPacket.length); /* convert from network to host byte order */
		currPacket.seq_no = ntohl (currPacket.seq_no);
		currPacket.checksum = ntohl (currPacket.checksum);
		if (currPacket.type == TEARDOWN) /* tear-down message */
		{
			// assume tear-down message is always received
			printf("Receive a teardown msg\n");
			break;
//			printf ("%s\n", buffer);
//			Packet_t ackmsg;
//			ackmsg.type = htonl(8); // ack of tear-down
//			ackmsg.seq_no = htonl(0);/*convert to network endianness */
//			ackmsg.length = htonl(0);
//			if (physical_send (sockfd, &ackmsg, sizeof (ackmsg)) != sizeof (ackmsg))
//			{
//				DieWithError ("Error sending tear-down ack"); /* not a big deal-data already rcvd */
//			}
//			while (1)
//			{
//				physical_send (sockfd, &currPacket, sizeof (int)*3 + CHUNKSIZE);
//				if (ntohl(currPacket.type) == 4) /* respond to more teardown messages */
//				{
//					ackmsg.type = htonl(8);
//					ackmsg.seq_no = htonl(0);/* convert to network endianness */
//					ackmsg.length = htonl(0);
//					if (physical_send (sockfd, &ackmsg, sizeof (ackmsg)) != sizeof (ackmsg))
//					{
//						DieWithError ("Error sending tear-down ack");
//					}
//				}
//			}
//			DieWithError ("physical_send() failed");
		} else if (currPacket.type == PACKET) {
			printf ("---- RECEIVE PACKET %d length %d\n", currPacket.seq_no, currPacket.length);

			/* Send ack and store in buffer */
			if (currPacket.seq_no == packet_rcvd + 1) {
				packet_rcvd++;
				int buff_offset = CHUNKSIZE * currPacket.seq_no;
				memcpy (&buffer[buff_offset], currPacket.data, /* copy packet data to buffer */
						currPacket.length);
				recvMsgSize += currPacket.length;
			}
			printf ("---- SEND ACK %d\n", packet_rcvd);
			Packet_t currAck; /* ack packet */
			currAck.type = htonl (ACK_MSG); /*convert to network byte order */
			currAck.seq_no = htonl (packet_rcvd);
			currAck.length = htonl(0);
			currAck.checksum = htonl(calcChecksum(&currAck));
			if (physical_send (sockfd, &currAck, sizeof(currAck)) != sizeof(currAck)) {
				printf("ack msg %d loss\n", packet_rcvd);
			}
		}
	}

	return recvMsgSize;
}


