#include "server.h"
#include <unistd.h>

/**
 * Creates a server socket.
 *
 * @return The file descriptor of the server socket.
 */
int create_server_socket(void) {
    int opt = 1; 
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); 
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); 
    return server_fd;
}

/**
 * Configures the server address.
 *
 * @return The configured server address.
 */
struct sockaddr_in configure_server_address(void) {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    return address;
}

/**
 * Binds the server socket to the specified address and starts listening for incoming connections.
 *
 * @param server_fd The file descriptor of the server socket.
 * @param address The server address to bind to.
 */
void bind_and_listen(int server_fd, struct sockaddr_in address) {
    bind(server_fd, (struct sockaddr *)&address, sizeof(address)); 
    listen(server_fd, 5); 
}

/**
 * Accepts a client connection on the server socket.
 *
 * @param server_fd The file descriptor of the server socket.
 * @param address The server address.
 * @return The file descriptor of the client socket.
 */
int accept_client_connection(int server_fd, struct sockaddr_in address) {
    int addrlen = sizeof(address);
    int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    return client_fd;
}