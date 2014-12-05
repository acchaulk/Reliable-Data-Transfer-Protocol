/*
 * sr.c
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#include "sr.h"
#include "common.h"

static int g_windowSize;
static double g_lossRate;

void sr_init(int windowSize, double lossRate, double corruptionRate) {

}

size_t sr_send(int sockfd, char* buf, size_t length) {
	return 0;
}

size_t sr_recv(int sockfd, char* buf, size_t length) {
	return 0;
}
