/*
 * server.c - Chat server for the Text ChatRoullette program
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

#include "common.h"
#include "control_msg.h"
#include "datalink.h"

/* global variables for the server */
static server_state_t g_state =  SERVER_INIT;
struct client_info* g_clients[CLIENT_MAX]; // chat queue
fd_set g_bitmap;  // bitmap for chat channel
fd_set g_master;  // global socket map
long g_useid = 0;  // global user id
pthread_t g_connector;
int g_sockfd;   // connect to client

/* used by cleanup() to handle children */
void sigchld_handler(int s) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


/* cleans up current processes */
void cleanup() {
	struct sigaction sa;
	
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
}

//static void grace_exit(int signum) {
//	int i;
//	for (i = 0; i <= g_fdmax; i++) {
//		if (FD_ISSET(i, &g_master)) {
//			if (datalink_send(i, MSG_REMOTE_SHUTDOWN, strlen(MSG_REMOTE_SHUTDOWN)) == -1) {
//				perror("notify client fails");
//			}
//		}
//	}
//	sleep(1);
//	exit(1);
//}

///* handler for transfering files */
//void handle_transfer(const char * file_name, struct client_info *client, struct client_info *partner) {
//
//	char buf[BUF_MAX];
//	sprintf(buf, "%s:%s", MSG_RECEIVING_FILE, file_name);
//	if (send(partner->sockfd, buf, sizeof(buf), 0) == -1) {
//		perror("send receiving file fails");
//		return;
//	}
//
//	if (send(client->sockfd, MSG_TRANSFER_ACK, sizeof(MSG_TRANSFER_ACK), 0) == -1) {
//		perror("send reponse ack fails");
//		return;
//	}
//	client->state = TRANSFERING;
//	partner->state = TRANSFERING;
//}


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
	FD_ZERO(&g_bitmap);

    // create socket and listen on it
	listener_fd = listen_connection(PORT_2);

	g_state = SERVER_RUNNING;

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
	return 0;
}

/* parses control commands entered by the admin */
static void parse_control_command(char * cmd) {
	char *params[PARAMS_MAX];
	char *token;
	char delim[2] = " ";
	int count = 0;

	while ((token = strsep(&cmd, delim)) != NULL) {
		params[count] = strdup(token);
		count++;
	}

	if (strcmp(params[0], START) == 0) {
		// create socket and listen on it
		g_sockfd = create_connection("localhost", PORT_1);
		if (g_sockfd == -1) {
			return;
		}
		pthread_create(&g_connector, NULL, &server_loop, NULL);
	} else if (strcmp(params[0], CHAT) == 0) {
		if (count == 1) {
			printf("Usage: %s [message]\n", CHAT);
			return;
		}
		chat(g_sockfd, params[1]);
	} else if (strcmp(params[0], TRANSFER) == 0) {
		if (count == 1) {
			printf("Usage: %s [filename]\n", TRANSFER);
			return;
		}
		transfer(g_sockfd, params[1]);
	} else if (strcmp(params[0], EXIT) == 0) {
		exit(1);
	} else if (strcmp(params[0], HELP) == 0) {
		help();
	} else if (strcmp(params[0], STATS) == 0) {
		write_sender_stats("log/server_sender.txt");
		write_receiver_stats("log/server_receiver.txt");
		printf("write stats successfully\n");
	}  else {
		printf("%s: Command not found. Type '%s' for more information.\n", params[0], HELP);
	}
}

/* main function */
int main(int argc, char* argv[]) {
	char user_input[BUF_MAX];
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
		} else {
			if (strcmp(strip(user_input), "") == 0) {
				continue;
			}
			printf("%s: Command not found. Type '%s' for more information.\n", user_input, HELP);
		}
	}

	return 0;
}
