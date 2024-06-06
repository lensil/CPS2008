#ifndef SERVER_H
#define SERVER_H

#include "Client.h"
#include "Canvas.h"
#include "Commands.h"
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

using namespace std;

class DrawCommand;
class Canvas;

class Server {
public:
    Server(int port);
    void run();

private:
    int server_fd;
    int port;
    vector<Client> clients;
    vector<thread> client_threads;
    shared_mutex clients_mutex;
    map<string, DisconnectedClient> disconnected_draw_commands;
    atomic<int> num_clients;

    void handle_client(Client& client);
    void broadcast_update(const Client& sender, const char* buffer, size_t buffer_length);
    bool process_command(Client& client, const char* buffer, ssize_t bytes_received);
    void remove_client(Client& client);
    void check_inactivity();
    void adopt_draw_commands(const string& nickname);
    void apply_draw_command(const string& command);
    void shutdown_server();
    string serialize_draw_command(const DrawCommand& cmd);
};

#endif // SERVER_H
