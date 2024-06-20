#ifndef HEADER_H_
#define HEADER_H_

// Variables and Macros

#define MB 1024*1024
#define WARM_UP 256 * 1024
#define DEFAULT_PORT 5555
#define INTERVAL 10 // seconds
#define DEFULAT_SERVER '127.0.0.1'
#define DATA_BUFFER_SIZE 131072

// Syncing states
#define START 1
#define START_ROUND 2
#define END_ROUND 3
#define DONE 4

#define REQ_START 0x01
#define APP_START 0x02
#define REQ_START_INTERVAL 0x03
#define APP_START_INTERVAL 0x04
#define REQ_END_INTERVAL 0x05
#define APP_END_INTERVAL 0x06
#define REQ_DONE_INTERVAL 0x07
#define APP_DONE_INTERVAL 0x08
#define REQ_DONE 0x09
#define APP_DONE 0x0A

/*  
    Notes:
        1. DATA_BUFFER_SIZE set to 128 KB: same as iperf default read/write buffer size.
        2. WARM_UP cycles set to 256 kB: quarter of the maximum up ahead challenge is a good warm up, gave best results.
*/

// functions

void run_server(int port, int interval);
void run_client(char *server_ip,int port, int interval);
int configure_server(int port);
int configure_client(int port, char *server_ip);
void throughput(char mode, int control_socket, int data_socket, int interval);
int syncing(char mode, int control_socket, int stage);
void pretty_print(ssize_t bytes,ssize_t window,int interval);
void warm_up(char mode, int control_socket, int data_socket);

#endif /* SERVER_HEADER_H_ */
