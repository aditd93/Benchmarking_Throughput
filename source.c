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

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

void run_server(int port, int interval) {

    // Initialize sockets and file descriptors
    int ctrl_socket, ctrl_fd, data_socket, data_fd; 
    struct sockaddr_in address; socklen_t addrlen = sizeof(address); 

    ctrl_fd = configure_server(port);
    data_fd = configure_server(port+1);

    // Accept client connection and create a control socket 
    if((ctrl_socket = accept(ctrl_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("Accept new connection failed, exit program\n");
        exit(EXIT_FAILURE);
    }

    // Accept client connection and create a control socket 
    if((data_socket = accept(data_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("Accept new connection failed, exit program\n");
        exit(EXIT_FAILURE);
    }

    // time interval exchange
    if(recv(ctrl_socket,&interval,4,0) < 0) {
        perror("Failed to update server with time interval duration, exit program\n");
        exit(EXIT_FAILURE);
    }

    // running measures...
    throughput('s', ctrl_socket, data_socket, interval);

    // Safely closing file descriptors and sockets
    close(data_socket);
    close(ctrl_socket);
    close(ctrl_fd);
    close(data_fd);
}

void run_client(char *server_ip,int port, int interval) {
    
    // Initialize sockets and file descriptors
    int client_ctrl, client_data; 

    // Create client sockets
    client_ctrl = configure_client(port,server_ip);
    client_data = configure_client(port+1,server_ip);

    // time interval exchange
    if(send(client_ctrl,&interval,4,0) < 0) {
        perror("Failed to update server with time interval duration, exit program\n");
        exit(EXIT_FAILURE);
    }

    // running measures...
    throughput('c', client_ctrl, client_data, interval);

    // Safely closing file descriptors and sockets
    close(client_data);
    close(client_ctrl);
}

int configure_server(int port) {
    
    // Initialize variables
    int fd; struct sockaddr_in address; 

    // Create server's socket file descriptor
    if((fd = socket(AF_INET, SOCK_STREAM, 0) ) < 0) {
        perror("Create socket failed\n"); // In case of a fail
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    int reuse_sock = 1; // free port when finishes program
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char*)&reuse_sock,sizeof(reuse_sock));
    if(bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed, exit program\n");
        exit(EXIT_FAILURE);
    }

    // Listen to new connection request
    if(listen(fd, 3) < 0) {
        perror("listen to incoming connections requests failed, exit program\n");
        exit(EXIT_FAILURE);
    }

    return fd;
}

int configure_client(int port, char *server_ip) {

     // Initialize sockets and file descriptors
    int new_socket = 0; struct sockaddr_in addr;

    // Create file descriptor
    if ((new_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed create file descriptor, exit program\n");
        exit(EXIT_FAILURE);
    }

    // Set the server adderss
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("Invalid address, exit program\n");
        close(new_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(new_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connection failed, exit program\n");
        close(new_socket);
        exit(EXIT_FAILURE);
    }

    return new_socket;
}

void throughput(char mode, int control_socket, int data_socket, int interval) {
    
    switch(mode) {
        case 's':
                ssize_t bytes, bytes_total = 0;
                fd_set readfds; int max_sd, action; int done = 1;
                ssize_t window = 1; char data_buffer[DATA_BUFFER_SIZE];
                while(done) {
                    FD_ZERO(&readfds);
                    FD_SET(control_socket,&readfds); FD_SET(data_socket,&readfds);
                    max_sd = control_socket > data_socket ? control_socket : data_socket;
                    action = select(max_sd + 1, &readfds, NULL, NULL, NULL);
                    if((action < 0) && (errno != EINTR)) {
                        perror("select error\n");
                        break;
                    }
                    if(FD_ISSET(control_socket, &readfds)) {
                        int state = syncing('s',control_socket,0);
                        switch(state) {
                            case START:
                                break;
                                
                            case END_ROUND:
                                // pretty_print(bytes_total,window,interval); // DEBUG check throughput synced between client and server
                                bytes_total = 0;
                                window *= 2;
                                break;
                                
                            case DONE:
                                done = 0;
                                break;
                        }
                    }
                    if(FD_ISSET(data_socket, &readfds)) {
                        bytes = recv(data_socket,&data_buffer,MIN(window,DATA_BUFFER_SIZE),0);
                        if(bytes < 0) {
                            perror("recv error\n");
                            break;
                        }
                        bytes_total += bytes;
                    }
                }
            break;

        case 'c':
            for(ssize_t window = 1; window <= MB; window *= 2) {
                ssize_t bytes_total = 0; 
                char *data_buffer = (char*)malloc(sizeof(char)*window);
                memset(data_buffer,'$',window);
                if(syncing('c',control_socket,START) == START) {
                    time_t t = time(NULL);
                    while(time(NULL) - t < interval) {
                    ssize_t bytes = send(data_socket,data_buffer,window,0);
                    bytes_total += bytes;
                    }
                    pretty_print(bytes_total,window,interval);
                    if(syncing('c',control_socket,END_ROUND) != END_ROUND) { 
                        free(data_buffer);
                        break; 
                    }
                }
                free(data_buffer);
            }
            if(syncing('c',control_socket,DONE) == DONE) {
                printf("Done\n");
                break;
            }
            break;
    }
}

int syncing(char mode, int control_socket, int stage) {
    uint8_t control_buffer = 0; int res = 0;
    switch(mode) {
        case 's':
            recv(control_socket,&stage,1,0); // receive state from client
            switch(stage) {
                case START:
                    if(recv(control_socket,&control_buffer,1,0) != 1) {
                        perror("Failed to receive Rrquest Start from client\n");
                        return 0;
                    }
                    if(control_buffer == REQ_START) {
                        control_buffer = APP_START;
                        res = (send(control_socket,&control_buffer,1,0) > 0) ? START : 0;
                    }
                    break;

                case START_ROUND:
                    recv(control_socket,&control_buffer,1,0);
                    if(control_buffer == REQ_START_INTERVAL) {
                        control_buffer = APP_START_INTERVAL;
                        res = (send(control_socket,&control_buffer,1,0) > 0) ? START_ROUND : 0;
                    }
                    break;

                case END_ROUND:
                    recv(control_socket,&control_buffer,1,0);
                    if(control_buffer == REQ_END_INTERVAL) {
                        control_buffer = APP_END_INTERVAL;
                        res = (send(control_socket,&control_buffer,1,0) > 0) ? END_ROUND : 0;
                    }
                    break;
                
                case DONE:
                    recv(control_socket,&control_buffer,1,0);
                    if(control_buffer == REQ_DONE) {
                        control_buffer = APP_DONE;
                        res = (send(control_socket,&control_buffer,1,0) > 0) ? DONE : 0;
                    }
                    break;
                
                default:
                    break;
            }
            break;

        case 'c':
            send(control_socket,&stage,1,0);
            switch (stage) {
                case START:
                    control_buffer = REQ_START;
                    if(send(control_socket,&control_buffer,1,0) != 1) {
                        perror("Failed to send Request Start to server\n");
                        return 0;
                    }
                    recv(control_socket,&control_buffer,1,0);
                    res = (control_buffer == APP_START) ? START : 0;
                    break;

                case START_ROUND:
                    control_buffer = REQ_START_INTERVAL;
                    if(send(control_socket,&control_buffer,1,0) != 1) {
                        perror("Failed to send Request Interval to server\n");
                        return 0;
                    }
                    recv(control_socket,&control_buffer,1,0);
                    res = (control_buffer == APP_START_INTERVAL) ? START_ROUND : 0;
                    break;

                case END_ROUND:
                    control_buffer = REQ_END_INTERVAL;
                    if(send(control_socket,&control_buffer,1,0) != 1) {
                        perror("Failed to send Request Interval to server\n");
                        return 0;
                    }
                    recv(control_socket,&control_buffer,1,0);
                    res = (control_buffer == APP_END_INTERVAL) ? END_ROUND : 0;
                    break;

                case DONE:
                    control_buffer = REQ_DONE;
                    if(send(control_socket,&control_buffer,1,0) != 1) {
                        perror("Failed to send Request Interval to server\n");
                        return 0;
                    }
                    recv(control_socket,&control_buffer,1,0);
                    res = (control_buffer == APP_DONE) ? DONE : 0;
                    break;

                default:
                    break;
            }
            break;
    }
    return res;
}

void pretty_print(ssize_t bytes, ssize_t window,int interval) {
    ssize_t bps = bytes*(8/((double)interval));
    if(bps < 1e6) {
        printf("%ld\t%.2lf\tKbits/sec\n",window,bps/1e3);
    }
    else if(bps >= 1e6 && bps < 1e9) {
        printf("%ld\t%.2lf\tMbits/sec\n",window,bps/1e6);
    }
    else if(bps >= 1e9) {
        printf("%ld\t%.2lf\tGbits/sec\n",window,bps/1e9);
    }
}