/*
 * gobackn.c
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#include <stdio.h>			/* for printf() and fprintf() */
#include <stdlib.h>			/* for atoi() and exit() */
#include <string.h>			/* for memset() */
#include <unistd.h>			/* for close() and alarm() */
#include <errno.h>			/* for errno and EINTR */
#include <signal.h>			/* for sigaction() */

#include "gbn.h"
#include "datalink.h"
#include "packet.h"
#include "common.h"

static int g_window_size;
static double g_loss_rate;
static int g_base = 0;   	/* next expected sequence number */
//static int g_tries = 0;			/* Count of times sent - GLOBAL for signal-handler access */
//static int g_sendflag = 1;
//
//void CatchAlarm (int ignored)	/* Handler for SIGALRM */
//{
//  g_tries += 1;
//  g_sendflag = 1;
//}

void gbn(int window_size, double loss_rate) {
	g_window_size = window_size;
	g_loss_rate = loss_rate;
}

int max (int a, int b) {
	if (b > a)
		return b;
	return a;
}

int min(int a, int b) {
	  if (b > a)
		  return a;
	  return b;
}



size_t gbn_send(int sockfd, char* buffer, size_t length) {
	int packet_received = -1;	/* highest ack received */
	int packet_sent = -1;		/* highest packet sent */
	int nPackets = 0;		/* number of packets to send */
	int sendBytes;

	/* if it doesn't divide cleanly, need one more odd-sized packet */
	nPackets = length / CHUNKSIZE;
	if (length % CHUNKSIZE) {
		nPackets++;
	}

	while ((packet_received < nPackets - 1)/* && (g_tries < MAXTRIES) */) {
		int counter; /*window size counter */
		for (counter = 0; counter < g_window_size; counter++) {
			packet_sent = min (max (g_base + counter, packet_sent), nPackets - 1); /* calc highest packet sent */
			Packet_t currpacket; /* current packet we're working with */
			if ((g_base + counter) < nPackets) {
				memset(&currpacket, 0, sizeof(currpacket));
				printf ("sending packet %d packet_sent %d packet_received %d\n",
						g_base+counter, packet_sent, packet_received);

				currpacket.type = htonl (PACKET); /*convert to network endianness */
				currpacket.seq_no = htonl (g_base + counter);

				/* calculate packet length, chunksize except last packet */
				int currlength;
				if ((length - ((g_base + counter) * CHUNKSIZE)) >= CHUNKSIZE)	{
					currlength = CHUNKSIZE;
				} else {
					currlength = length % CHUNKSIZE;
				}
				currpacket.length = htonl (currlength);

				/*copy buffer data into packet */
				memcpy (currpacket.data, buffer + ((g_base + counter) * CHUNKSIZE), currlength);

				sendBytes += physical_send(sockfd, &currpacket, currlength);
			}
		}

		// Got response
		//alarm (TIMEOUT);	/* Set the timeout */
		Packet_t currAck;
		int recvBytes = physical_recv(sockfd, &currAck, 0); // expect to receive a ACK message
		if (recvBytes < 0) {
			perror("gbn_send wait ack failed");
		} else {
			/* recvfrom() got something -- cancel the timeout */
			int acktype = ntohl (currAck.type); /* convert to host byte order */
			int ackno = ntohl (currAck.seq_no);
			if (ackno > packet_received && acktype == ACK_MSG)
			{
				printf ("received ack %d\n", ackno); /* receive/handle ack */
				packet_received++;
				g_base = packet_received; /* handle new ack */
				if (packet_received == packet_sent)	{
					alarm(0); /* all sent packets acked, clear alarm */
				} else {
					alarm(TIMEOUT); /* not all sent packets acked, reset alarm */
				}
			}
		}
	}
	return sendBytes;
}
//
//int gbn_send_2(int sockfd, char* buffer, size_t length) {
//	int packet_received = -1;	/* highest ack received */
//	int packet_sent = -1;		/* highest packet sent */
//	int nPackets = 0;		/* number of packets to send */
//	int recvBytes;			/* Size of received packet */
//	struct sigaction myAction;	/* For setting signal handler */
//	size_t bytesSent = 0;
//
//	nPackets = length / CHUNKSIZE;
//	if (length % CHUNKSIZE) {
//		nPackets++;			/* if it doesn't divide cleanly, need one more odd-sized packet */
//	}
//
//	/* Set signal handler for alarm signal */
//	myAction.sa_handler = CatchAlarm;
//	if (sigfillset (&myAction.sa_mask) < 0)	{
//		/* block everything in handler */
//		perror ("sigfillset() failed");
//		return -1;
//	}
//	myAction.sa_flags = 0;
//
//	if (sigaction (SIGALRM, &myAction, 0) < 0) {
//		perror ("sigaction() failed for SIGALRM");
//		return -1;
//	}
//
//	/* Send the string to the server */
////	while ((packet_received < nPackets - 1) && (g_tries < MAXTRIES)) {
//	{
//		 printf ("in the send loop base %d packet_sent %d packet_received %d\n",
//				 g_base, packet_sent, packet_received);
//		if (g_sendflag > 0) {
//			g_sendflag = 0;
//			int counter; /*window size counter */
//			for (counter = 0; counter < g_window_size; counter++) {
//				packet_sent = min (max (g_base + counter, packet_sent), nPackets - 1); /* calc highest packet sent */
//				struct packet currpacket; /* current packet we're working with */
//				if ((g_base + counter) < nPackets) {
//					memset(&currpacket, 0, sizeof(currpacket));
//					printf ("sending packet %d packet_sent %d packet_received %d\n",
//							g_base+counter, packet_sent, packet_received);
//
//					currpacket.type = htonl (1); /*convert to network endianness */
//					currpacket.seq_no = htonl (g_base + counter);
//
//					/* calculate packet length, chunksize except last packet */
//					int currlength;
//					if ((length - ((g_base + counter) * CHUNKSIZE)) >= CHUNKSIZE)	{
//						currlength = CHUNKSIZE;
//					} else {
//						currlength = length % CHUNKSIZE;
//					}
//					currpacket.length = htonl (currlength);
//
//					/*copy buffer data into packet */
//					memcpy (currpacket.data, buffer + ((g_base + counter) * CHUNKSIZE), currlength);
//
//					bytesSent += physical_send(sockfd, currpacket, currlength);
//				}
//			}
////			/* Get a response */
////			alarm (TIMEOUT_SECS);	/* Set the timeout */
////			struct packet currAck;
////			recvBytes = physical_recv(sockfd, &currAck);
////			if (recvBytes < 0) {
////				if (errno == EINTR)	{
////					/* Alarm went off  */
////					if (g_tries < MAXTRIES)	{
////						printf ("timed out, %d more tries...\n", MAXTRIES - g_tries);
////						break;
////					} else {
////						perror ("No Response");
////						return -1;
////					}
////				} else {
////					perror ("recvfrom() failed");
////					return -1;
////				}
////			} else {
////				/* recvfrom() got something -- cancel the timeout */
////				int acktype = ntohl (currAck.type); /* convert to host byte order */
////				int ackno = ntohl (currAck.seq_no);
////				if (ackno > packet_received && acktype == 2)
////				{
////					printf ("received ack\n"); /* receive/handle ack */
////					packet_received++;
////					g_base = packet_received; /* handle new ack */
////					if (packet_received == packet_sent) /* all sent packets acked */
////					{
////						alarm(0); /* clear alarm */
////						g_tries = 0;
////						g_sendflag = 1;
////					}
////					else /* not all sent packets acked */
////					{
////						g_tries = 0; /* reset retry counter */
////						g_sendflag = 0;
////						alarm(TIMEOUT_SECS); /* reset alarm */
////
////					}
////				}
////			}
//		}
//
//
////		int ctr;
////		for (ctr = 0; ctr < 10; ctr++) /* send 10 teardown packets - don't have to necessarily send 10 but spec said "up to 10" */
////		{
////			struct packet teardown;
////			teardown.type = htonl (4);
////			teardown.seq_no = htonl (0);
////			teardown.length = htonl (0);
////			sendto (sock, &teardown, (sizeof (int) * 3), 0,
////					(struct sockaddr *) &gbnServAddr, sizeof (gbnServAddr));
////		}
//	}
//	return bytesSent;
//}

size_t gbn_recv(int sockfd, char* buffer, size_t length) {
	int packet_rcvd = -1;		/* highest packet successfully received */

	Packet_t currPacket; /* current packet that we're working with */
	memset(&currPacket, 0, sizeof(currPacket));
	/* Block until receive message from a client */
	int recvBytes = physical_recv(sockfd, &currPacket, CHUNKSIZE);
	currPacket.type = ntohl (currPacket.type);
	currPacket.length = ntohl (currPacket.length); /* convert from network to host byte order */
	currPacket.seq_no = ntohl (currPacket.seq_no);
	printf("currPacket.type=%d length=%d seq_no=%d data=%s\n",
			currPacket.type, currPacket.length, currPacket.seq_no, currPacket.data);

	/* Send ack and store in buffer */
	if (currPacket.seq_no == packet_rcvd + 1) {
		packet_rcvd++;
		int buff_offset = CHUNKSIZE * currPacket.seq_no;
		/* copy packet data to buffer */
		memcpy (&buffer[buff_offset], currPacket.data, currPacket.length);
		printf ("---- SEND ACK %d\n", packet_rcvd);
		Packet_t currAck; /* ack packet */
		currAck.type = htonl (ACK_MSG); /*convert to network byte order */
		currAck.seq_no = htonl (packet_rcvd);
		currAck.length = htonl(0);
		if (physical_send(sockfd, &currAck, 0) != HEADER_SIZE) {
			perror("datalink_recv ack sent a different number of bytes than expected");
			return -1;
		}
	}
	return recvBytes;
}

//
//size_t gbn_recv_2(int sockfd, char* buffer, size_t length) {
//	int recvBytes;		/* Size of received message */
//	int packet_rcvd = -1;		/* highest packet successfully received */
//
//	struct sigaction myAction;
//	myAction.sa_handler = CatchAlarm;
//	if (sigfillset (&myAction.sa_mask) < 0) {
//		/*setup everything for the timer - only used for teardown */
//	    perror ("sigfillset failed");
//	    return -1;
//	}
//	myAction.sa_flags = 0;
//	if (sigaction (SIGALRM, &myAction, 0) < 0) {
//		perror ("sigaction failed for SIGALRM");
//		return -1;
//	}
//	for (;;) {
//		/* set the size of the in-out parameter */
//		struct packet currPacket; /* current packet that we're working with */
//		memset(&currPacket, 0, sizeof(currPacket));
//		/* Block until receive message from a client */
//		recvBytes = physical_recv(sockfd, &currPacket);
//		currPacket.type = ntohl (currPacket.type);
//		currPacket.length = ntohl (currPacket.length); /* convert from network to host byte order */
//		currPacket.seq_no = ntohl (currPacket.seq_no);
//		if (currPacket.type == 4) {
////			/* tear-down message */
////			printf ("%s\n", buffer);
////			struct gbnpacket ackmsg;
////			ackmsg.type = htonl(8);
////			ackmsg.seq_no = htonl(0);/*convert to network endianness */
////			ackmsg.length = htonl(0);
////			if (physical_send(sockfd, &ackmsg) != sizeof (ackmsg)) {
////				perror ("Error sending tear-down ack"); /* not a big deal-data already rcvd */
////				return -1;
////			}
////			alarm (7);
////			while (1) {
////				while ((physical_recv(sockfd, &currPacket)) < 0) {
////					if (errno == EINTR)	/* Alarm went off  */
////					{
////						/* never reached */
////						exit(0);
////					}
////				}
////				if (ntohl(currPacket.type) == 4) /* respond to more teardown messages */ {
////					ackmsg.type = htonl(8);
////					ackmsg.seq_no = htonl(0);/* convert to network endianness */
////					ackmsg.length = htonl(0);
//////					if (sendto
//////							(sock, &ackmsg, sizeof (ackmsg), 0, /* send teardown ack */
//////									(struct sockaddr *) &gbnClntAddr,
//////									recvBytes) != sizeof (ackmsg))
//////					{
//////						DieWithError ("Error sending tear-down ack");
//////					}
////				}
////			}
////			DieWithError ("recvfrom() failed");
//		} else {
////			if(g_loss_rate > drand48())
////				continue; /* drop packet - for testing/debug purposes */
//			printf ("---- RECEIVE PACKET %d length %d\n", currPacket.seq_no, currPacket.length);
//
//			/* Send ack and store in buffer */
//			if (currPacket.seq_no == packet_rcvd + 1) {
//				packet_rcvd++;
//				int buff_offset = CHUNKSIZE * currPacket.seq_no;
//				memcpy (&buffer[buff_offset], currPacket.data, /* copy packet data to buffer */
//						currPacket.length);
//			}
//			printf ("---- SEND ACK %d\n", packet_rcvd);
//			struct packet currAck; /* ack packet */
//			currAck.type = htonl (2); /*convert to network byte order */
//			currAck.seq_no = htonl (packet_rcvd);
//			currAck.length = htonl(0);
//			if (physical_send(sockfd, &currAck) != sizeof (currAck)) {
//				perror("sendto() sent a different number of bytes than expected");
//				return -1;
//			}
//		}
//	}
//	/* NOT REACHED */
//	return 0;
//}

