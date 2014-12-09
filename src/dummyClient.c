/*
 * dummyClient.c
 *
 *  Created on: 2014年12月1日
 *      Author: ylu
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "datalink.h"
#include "common.h"

static int g_sockfd;

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

//	struct sigaction sa;
//	/* Install timer_handler as the signal handler for SIGINT. */
//	memset (&sa, 0, sizeof (sa));
//	sa.sa_handler = &grace_exit;
//	sigaction (SIGINT, &sa, NULL);

	datalink_init(protocol, windowSize, lossRate, corruptionRate);

	g_sockfd = create_connection(SERV_HOST, PORT_1);

	char * buffer;
	int length = read_file("Project_1.pdf", &buffer);
	if (length == -1) {
		printf("file is not found\n");
		exit(1);
	}
	datalink_send(g_sockfd, buffer, length);
	write_sender_stats("log/client.txt");
	close(g_sockfd);

	return 0;
}
