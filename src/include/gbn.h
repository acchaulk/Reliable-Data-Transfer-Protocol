/*
 * gobackn.h
 *
 *  Created on: 2014年11月21日
 *      Author: ylu
 */

#ifndef GOBACKN_H_
#define GOBACKN_H_

size_t gbn_send(int sockfd, char* buf, size_t length);
size_t gbn_recv(int sockfd, char* buf, size_t length);
void gbn(int window_size, double loss_rate);

#endif /* GOBACKN_H_ */
