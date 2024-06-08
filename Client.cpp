#include "Client.h"
#include <cstring>
#include <ctime>

Client::Client(int socket, struct sockaddr_in addr, socklen_t len, const std::string& name)
    : fd(socket), client_addr(addr), client_addr_len(len), last_activity(time(nullptr)) {
    strncpy(nickname, name.c_str(), sizeof(nickname));
    nickname[sizeof(nickname) - 1] = '\0';
}

