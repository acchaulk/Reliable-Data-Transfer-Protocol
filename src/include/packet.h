/*
 * packet.h
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <pthread.h>
#include "common.h"

typedef enum PacketType {
	PACKET = 1,
	ACK_MSG = 2,
	TEARDOWN = 4,
} PacketType_t;

typedef struct Packet
{
	int type;
	int seq_no;
	int length;
	char data[CHUNKSIZE];
	int checksum;
} Packet_t;

typedef unsigned char SeqNum; // sequence number

//typedef struct {
//	int pos;
//	int capacity;
//	char * buffer;
//	sem_t mutex;
//} Datalink_buffer;
//
//typedef struct {
//	SeqNum latest_send;		/* highest sequence number be sent */
//	SeqNum latest_ack;	    /* highest sequence number be ACKed */
//	SeqNum next_seqnum;     /* next expected sequence number */
//	pthread_mutex_t mutex;
//	Datalink_buffer* sendQ;
//	Datalink_buffer* recvQ;
//} GBN_controller;

#endif /* PACKET_H_ */
