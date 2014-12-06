/*
 * physical.h
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#ifndef PHYSICAL_H_
#define PHYSICAL_H_

#include <stdlib.h>
#include "common.h"

void physical_init(double lossRate, double corruptionRate);

int physical_send(int sockfd, Frame_t* pkt, size_t data_length);

size_t physical_recv(int sockfd, Frame_t* pkt, size_t data_length);

#endif /* PHYSICAL_H_ */
