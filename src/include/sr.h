/*
 * sr.h
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#ifndef SR_H_
#define SR_H_

#include <stdlib.h>

size_t sr_send(int sockfd, char* buf, size_t length);
size_t sr_recv(int sockfd, char* buf, size_t length);
void sr(int window_size, double loss_rate);

#endif /* SR_H_ */
