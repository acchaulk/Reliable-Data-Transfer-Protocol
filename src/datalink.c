/*
 * datalink.c
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#include <string.h>
#include "datalink.h"
#include "gbn.h"
#include "sr.h"

static Object g_datalink;

void datalink(char * protocol, int window_size, double loss_rate) {
    if (strcmp(protocol, "gbn") == 0) {
    	gbn(window_size, loss_rate);
    	g_datalink.send = &gbn_send;
    	g_datalink.recv = &gbn_recv;
    } else if (strcmp(protocol, "sr") == 0) {
    	sr(window_size, loss_rate);
    	g_datalink.send = &sr_send;
    	g_datalink.recv = &sr_recv;
    } else {
    	printf("Abort: Unrecognized transfer protocol. \n");
    	exit(1);
    }
}

size_t datalink_send(int sockfd, char* buffer, size_t length) {
	return (g_datalink.send)(sockfd, buffer, length);
}

size_t datalink_recv(int sockfd, char* buffer, size_t length) {
	return (g_datalink.recv)(sockfd, buffer, length);
}
