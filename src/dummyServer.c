/*
 * dummyServer.c
 *
 *  Created on: 2014年12月1日
 *      Author: ylu
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

int main(int argc, char* argv[]) {
	if (argc != 5) {
		fprintf (stderr, "Usage: %s [gbn|sr] [window_size] [loss_rate] [corruption_rate]\n", argv[0]);
		exit(1);
	}

	char * protocol = argv[1];
	int windowSize = atoi (argv[2]);
	double lossRate = atof(argv[3]);	/* loss rate as percentage i.e. 50% = .50 */
	if (lossRate > 1.0f && lossRate < 0) {
		fprintf(stderr, "lossRate must between 0 and 1\n");
		exit(1);
	}

	double corruptionRate = atof(argv[4]);	/* corruption rate as percentage i.e. 50% = .50 */
	if (corruptionRate > 1.0f && corruptionRate < 0) {
		fprintf(stderr, "corruptionRate must between 0 and 1\n");
		exit(1);
	}

	char *buffer = malloc(100*1024*1024);		/* 100M buffer */
	memset (buffer, 0, 100*1024*1024);		/* zero out the buffer */

	datalink_init(protocol, windowSize, lossRate, corruptionRate);
	int listener_fd = listen_connection(SEND_PORT);
	int fdmax = listener_fd;


	fd_set master;   // master file descriptor list
	fd_set read_fds; // tmp file descriptor list for select
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	FD_SET(listener_fd, &master); // add the listener to the g_master set

	while(1) {
		read_fds = master; // copy it
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1 ) {
			perror("select() fails");
			exit(4);
		}
		// run through the existing connections looking for data to read
		int i;
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener_fd) {
					// getting new incoming connection
					accept_connection(i, &fdmax, &master);
				} else {
					// handling data from client
					int bytesRead = datalink_recv(i, buffer);
					/*parse the file name */
					char * filename = "test.txt";
					receive_file (open_file(filename), buffer, bytesRead);
					printf("receive msg successfully\n");
					write_receiver_stats("log/server.txt");
				}
			}
		}
	}

}
