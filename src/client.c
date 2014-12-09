/*
 * client.c - Client program that lets the user connect to the Text Chat Roullette
 * server and chat with other members
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <libgen.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#include "common.h"
#include "control_msg.h"
#include "datalink.h"

/* global variables for the client */
static int g_sockfd;

///* request help messages from server */
//void request_help() {
//	if (datalink_send(g_sockfd, MSG_HELP, sizeof(MSG_HELP)) < 0) {
//		perror("send help request fails");
//	}
//	char * buf = malloc(BUF_MAX);
//	if (datalink_recv(g_sockfd, buf, BUF_MAX) < 0) {
//		perror("recv help fails");
//	}
//	printf("\n%s\n", buf);
//}

/* parses commands entered by the client */
static void parse_control_command(char * cmd) {
	char *params[2] = {0};
	char *token;
	char delim[2] = " ";
	int count = 0;
	pthread_t sender, receiver;

	token = strsep(&cmd, delim);
	params[0] = token;
	params[1] = cmd;

	if (strcmp(params[0], CONNECT) == 0) {
		if (params[1] == NULL) {
			printf("Usage: %s [hostname]\n", CONNECT);
			return;
		}
		g_sockfd = create_connection(params[1], PORT_2);
		if (g_sockfd == -1) {
			return;
		}
		//pthread_create(&receiver, NULL, &receiver_thread, (void *)&g_sockfd);
	} else if (strcmp(params[0], CHAT) == 0) {
		if (params[1] == NULL) {
			printf("Usage: %s [message]\n", CHAT);
			return;
		}
		chat(g_sockfd, params[1]);
	} else if (strcmp(params[0], TRANSFER) == 0) {
		if (params[1] == NULL) {
			printf("Usage: %s [filename]\n", TRANSFER);
			return;
		}
		transfer(g_sockfd, params[1]);
	} else if (strcmp(params[0], EXIT) == 0) {
		grace_exit(g_sockfd);
	} else if (strcmp(params[0], HELP) == 0) {
		help();
	} else if (strcmp(params[0], STATS) == 0) {
		write_sender_stats("log/client_sender.txt");
		write_receiver_stats("log/client_receiver.txt");
		printf("write stats successfully\n");
	} else {
		printf("%s: Command not found. Type '%s' for more information.\n", params[0], HELP);
	}

}


/* main loop to be executed, handles the state transition */
void * server_loop(void * arg) {
    int listener_fd;
	int fdmax;
	fd_set master;   // master file descriptor list
	fd_set read_fds; // tmp file descriptor list for select
	int i, j;
	pthread_t connector, receiver;

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);

    // create socket and listen on it
	listener_fd = listen_connection(PORT_1);

	//g_state = SERVER_RUNNING;

    // add the listener to the g_master set
    FD_SET(listener_fd, &master);

	// keep track of the biggest file descriptor
	fdmax = listener_fd;

//	struct sigaction sa;
//	/* Install timer_handler as the signal handler for SIGVTALRM. */
//	memset (&sa, 0, sizeof (sa));
//	sa.sa_handler = &exit_server;
//	sigaction (SIGINT, &sa, NULL);

	while(1) {
	    read_fds = master; // copy it
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1 ) {
            perror("select() fails");
			exit(4);
		}
		// run through the existing connections looking for data to read
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener_fd) {
					// getting new incoming connection
					accept_connection(listener_fd, &fdmax, &master);
				} else {
					char* buffer = malloc(sizeof(Message_t));	/* allocate a buffer for message */
					memset (buffer, 0, sizeof(Message_t));		/* zero out the buffer */
					// handling data from client
					int bytesRead = datalink_recv(i, buffer);
					Message_t* msg = (Message_t*)buffer;
					// read the application header
					switch (msg->type) {
					case REMOTE_SHUTDOWN_MSG:
						close(i);
						FD_CLR(i, &master);
						break;
					case CHAT_MSG:
						printf("%s\n", msg->data); // print chat text
						break;
					case TRANSFER_MSG:
						store(msg, bytesRead);
						break;
					}
				}
			}
		}
	}
	return NULL;
}

/* main loop for the client program */
int main(int argc, char *argv[])
{
	char user_input[BUF_MAX];
	int i;
	int connected = 0; /* 0 means unconnected, 1 means connected*/
    pthread_t g_connector;

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

    pthread_create(&g_connector, NULL, &server_loop, NULL);

    while (1) {
		printf("> "); // prompt
		memset(user_input, 0, BUF_MAX);
		char* ret = fgets(user_input, BUF_MAX, stdin);
		if (ret == NULL) {
			continue;
		}
		user_input[strlen(user_input) - 1] = '\0';
		if (user_input[0] == '/') {
			parse_control_command(user_input);
			continue;
		} else {
			if (strcmp(strip(user_input), "") == 0) {
				continue;
			}
            printf("%s: Command not found. Type '%s' for more information.\n", user_input, HELP);
			continue;
		}
	}

    return 0;
}
