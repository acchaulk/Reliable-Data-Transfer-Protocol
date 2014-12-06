/*
 * sr.h
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#ifndef SR_H_
#define SR_H_

#include <stdlib.h>

void sr_init(int windowSize, double lossRate, double corruptionRate);
size_t sr_send(int sockfd, char* buf, size_t length);
size_t sr_recv(int sockfd, char* buf);

#endif /* SR_H_ */
