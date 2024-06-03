#include "Client.h"
#include <cstring>
#include <ctime>

Client::Client(int socket, struct sockaddr_in addr, socklen_t len)
    : fd(socket), client_addr(addr), client_addr_len(len), last_activity(time(nullptr)) {
    strcpy(nickname, "default");
}
