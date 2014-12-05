/*
 * gobackn.h
 *
 * Created on: 2014/11/21
 * Author: Yang Lu
 */

#ifndef GOBACKN_H_
#define GOBACKN_H_

void gbn_init(int windowSize, double lossRate, double corruptionRate);
size_t gbn_send(int sockfd, char* buf, size_t length);
size_t gbn_recv(int sockfd, char* buf);

#endif /* GOBACKN_H_ */
