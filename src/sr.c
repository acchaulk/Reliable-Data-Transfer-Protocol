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

size_t sr_send(int sockfd, char* buf, size_t length) {


	return 0;
}

size_t sr_recv(int sockfd, char* buf) {
	return 0;
}
