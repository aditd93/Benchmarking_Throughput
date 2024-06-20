/*******************************************************************
| Course            : Accelerated networks Lab - 371.1.2444
| Authors           : Adi Tzurdecker
| Host              : Gil Bloch
| Description       : EX1. Benchmarking throughput over TCP.               
| Github repository : https://github.com/aditd93/Benchmarking_Throughput
| Date              : 6/6/2024
/******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <time.h>
#include "header.h"

int main(int argc, char **argv) {
    int server_mode = 1; int client_mode = 0; // server mode running by default
    char *server_ip = NULL; int opt;
    int port = DEFAULT_PORT; int interval = INTERVAL;
    while((opt = getopt(argc,argv,"sc:p:i:h:")) != -1) { // get user inputs
        switch(opt) {
            case 's':
                server_mode = 1; // execute server role
                break;
            case 'c':
                client_mode = 1; // execute client role
                server_mode = 0; // 
                server_ip = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                if(port < 1 || port > 65535){
                    fprintf(stderr, "Invailid port number: %d. Valid Range [1, 65535]\n", port);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                if(server_mode) {
                    fprintf(stderr,"Interval time [-i] can be defined only by client\n");
                    exit(EXIT_FAILURE);
                }
                interval = atoi(optarg); // interval must be equal for both client and server
                break;
            case 'h':
            default:
                fprintf(stderr, "Usage: %s [-s] [-c server_ip] [-p port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    if(server_mode && client_mode) {
        fprintf(stderr, "Can not specified for both client [-c ] and server [-s ] modes, exit program\n");
        exit(EXIT_FAILURE);
    }
    
    client_mode == 1 ? run_client(server_ip,port, interval) : run_server(port, interval); // running chosen role
    
    exit(EXIT_SUCCESS);
}