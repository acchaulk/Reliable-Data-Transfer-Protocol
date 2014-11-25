/*
 * packet.h
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "common.h"

typedef enum PacketType {
	PACKET,
	ACK_MSG,
	TEARDOWN,
} PacketType_t;

typedef struct Packet
{
	int type;
	int seq_no;
	int length;
	char data[CHUNKSIZE];
} Packet_t;

#endif /* PACKET_H_ */
