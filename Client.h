#ifndef CLIENT_H
#define CLIENT_H

#include <vector>
#include <ctime>
#include <netinet/in.h>
#include <string>

class Client {
public:
    int fd;
    struct sockaddr_in client_addr;
    char nickname[20];
    socklen_t client_addr_len;
    time_t last_activity;
    std::vector<std::string> draw_commands;

    Client(int socket, struct sockaddr_in addr, socklen_t len, const std::string& name);
};

#endif // CLIENT_H
