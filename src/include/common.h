/*
 * common.h - contains constants, functions, and structs used by the client and server
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

typedef enum { INIT, CONNECTING, CHATTING, TRANSFERING } client_state_t;
typedef enum { SERVER_INIT, SERVER_RUNNING,  GRACE_PERIOD } server_state_t;


#define SEND_PORT              "3490"   // the port client will be connecting to
#define RECV_PORT               "3491"   // the port server will be connecting to
#define SERV_HOST         	   "localhost"
#define BUF_MAX                256    // max size for client data
#define CLIENT_MAX             64     // how many pending connections queue will hold
#define PARAMS_MAX             10     // maximum number of parameter
#define NAME_LENGTH            10     // maximum characters for client name
#define GRACE_PERIOD_SECONDS   10     // grace period seconds for stopping the server

#define STAT_FILEPATH       "log/stat.txt"

#define CHUNKSIZE              100
#define HEADER_SIZE			   sizeof(int) * 4
#define TIMEOUT                3       // default timeout value for gbn timer, second
#define MAXTRIES			   5
#define DL_BUFFER_SIZE         8000


void print_ascii_art();

/* strip leading and trailing whitespace */
char* strip(char *s);

int listen_connection(char * port);
int create_connection(char *hostname, char *port);
int accept_connection(int sockfd, int *fdmax, fd_set *master);
FILE* open_file(const char * input_file);
int receive_file(FILE* fp, char * filebuf, int length);
int read_file(const char * input_file, char** buffer);

#endif /* __COMMON_H__ */
