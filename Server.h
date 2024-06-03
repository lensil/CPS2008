#ifndef SERVER_H
#define SERVER_H

#include "Client.h"
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <map>
#include <atomic>
#include <netinet/in.h>
#include "DisconnectedClient.h"

#define PORT 6001
#define MAX_CLIENTS 100
#define INACTIVITY_TIMEOUT 1800 
#define RECONNECT_TIMEOUT 60 

class Server {
public:
    Server(int port);
    void run();

private:
    int server_fd;
    int port;
    std::vector<Client> clients;
    std::vector<std::thread> client_threads;
    std::shared_mutex clients_mutex;
    std::map<std::string, DisconnectedClient> disconnected_draw_commands;
    std::atomic<int> num_clients;

    void handle_client(Client& client);
    void broadcast_update(const Client& sender, const char* buffer, size_t buffer_length);
    bool process_command(Client& client, const char* buffer, ssize_t bytes_received);
    void remove_client(Client& client);
    void check_inactivity();
    void adopt_draw_commands(const std::string& nickname);
    void apply_draw_command(const std::string& command);
    void shutdown_server();
};

#endif // SERVER_H
