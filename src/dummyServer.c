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

static fd_set g_master;   // master file descriptor list
static int g_fdmax;

static void grace_exit(int signum) {
	int i;
	for (i = 0; i <= g_fdmax; i++) {
		if (FD_ISSET(i, &g_master)) {
			if (datalink_send(i, MSG_REMOTE_SHUTDOWN, strlen(MSG_REMOTE_SHUTDOWN)) == -1) {
				perror("notify client fails");
			}
		}
	}
	sleep(1);
	exit(1);
}

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

	struct sigaction sa;
	/* Install timer_handler as the signal handler for SIGINT. */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &grace_exit;
	sigaction (SIGINT, &sa, NULL);

	char *buffer = malloc(100*1024*1024);		/* 100M buffer */

	datalink_init(protocol, windowSize, lossRate, corruptionRate);
	int listener_fd = listen_connection(PORT_1);
	g_fdmax = listener_fd;


	fd_set read_fds; // tmp file descriptor list for select
	FD_ZERO(&g_master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	FD_SET(listener_fd, &g_master); // add the listener to the g_master set

	while(1) {
		read_fds = g_master; // copy it
		if (select(g_fdmax + 1, &read_fds, NULL, NULL, NULL) == -1 ) {
			perror("select() fails");
			exit(4);
		}
		// run through the existing connections looking for data to read
		int i;
		for (i = 0; i <= g_fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener_fd) {
					// getting new incoming connection
					accept_connection(i, &g_fdmax, &g_master);
				} else {
					memset (buffer, 0, 100*1024*1024);		/* zero out the buffer */
					// handling data from client
					int bytesRead = datalink_recv(i, buffer);

					if (strcmp(buffer, MSG_REMOTE_SHUTDOWN) == 0) {
						printf("receive remote shutdown\n");
						close(i);
						FD_CLR(i, &g_master);
					} else {
						/*parse the file name */
						char * filename = "Project_1.pdf";
						receive_file (open_file(filename), buffer, bytesRead);
						printf("receive msg successfully\n");
						write_receiver_stats("log/server.txt");
					}
				}
			}
		}
	}

}
