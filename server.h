#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 5000 

int create_server_socket(void);
struct sockaddr_in configure_server_address(void);
void bind_and_listen(int server_fd, struct sockaddr_in address);
int accept_client_connection(int server_fd, struct sockaddr_in address);

#endif