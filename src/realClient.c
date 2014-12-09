/*
 * realClient.c
 *
 *  Created on: 2014年12月8日
 *      Author: ylu
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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

	pid_t pid = fork();
	if (pid == 0) { // child
		char *args[] = {"./client", argv[1], argv[2], argv[3], argv[4], (char *) 0 };
		execv("./client", args);
	} else if (pid < 0) { // failed to fork
		perror("failed to fork");
		exit(1);
	} else { // parent

	}

}
