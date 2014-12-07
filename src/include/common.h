/*
 * common.h - contains constants, functions, and structs used by the client and server
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>

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

#define CHUNKSIZE              100
#define PACKET_HEADER	       sizeof(int) * 3
#define FRAME_HEADER           sizeof(unsigned int) + PACKET_HEADER
#define TIMEOUT                1       // default timeout value for gbn timer, second
#define MAXTRIES			   10
#define DL_BUFFER_SIZE         8000

typedef enum PacketType {
	PACKET = 1,
	ACK_MSG = 2,
	TEARDOWN = 4,
} PacketType_t;

typedef struct Packet
{
	int type;
	int seq_no;
	int length;
	char data[CHUNKSIZE];
} Packet_t;

typedef struct Frame {
	unsigned int checksum;
	Packet_t pkt;
} Frame_t;

typedef struct Statistics {
	unsigned int actualFrames; /* sender */
	unsigned int frameSent;   /* sender */
	unsigned int frameRetrans;  /* sender */
	unsigned int ackRecv;     /* sender */
	unsigned int bytesSent;		/*sender*/
	unsigned int filesize;     /* sender */
	unsigned int timeTaken;		/*sender*/
	unsigned int ackSent;     /* receiver */
	unsigned int dupFrameRecv;   /*receiver*/
} Stats_t;

typedef void (*timerCallback_t)(union sigval si);

void print_ascii_art();

/* strip leading and trailing whitespace */
char* strip(char *s);

int listen_connection(char * port);
int create_connection(char *hostname, char *port);
int accept_connection(int sockfd, int *fdmax, fd_set *master);
FILE* open_file(const char * input_file);
int receive_file(FILE* fp, char * filebuf, int length);
int read_file(const char * input_file, char** buffer);
unsigned short calcChecksum(Packet_t *pkt);
void write_sender_stats(const char* path);
void write_receiver_stats(const char* path);
void make_timer(timer_t * timers_head, int index, timerCallback_t callback, int timeout);
void delete_timer(timer_t *head, int index);
void reset_timer(timer_t *head, int index, int timeout, int interval);

Stats_t g_gbnStat;


#endif /* __COMMON_H__ */
