#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <atomic>

#define PORT 6001
#define MAX_CLIENTS 100
#define INACTIVITY_TIMEOUT 1800 
#define RECONNECT_TIMEOUT 60 

using namespace std;

shared_mutex clients_mutex;
vector<thread> client_threads;
atomic<int> num_clients(0);   

struct Client {
    int fd;
    struct sockaddr_in client_addr;
    char nickname[20];
    socklen_t client_addr_len;
    time_t last_activity;
    vector<string> draw_commands; // to do: Need to implement a way to store draw commands

    Client(int socket, struct sockaddr_in addr, socklen_t len)
        : fd(socket), client_addr(addr), client_addr_len(len), last_activity(time(nullptr)) {
        strcpy(nickname, "default");
    }
};

struct DisconnectedClient {
    vector<string> draw_commands;
    std::time_t last_activity;

    DisconnectedClient() : last_activity(0) {}
    DisconnectedClient(const vector<string>& commands, time_t last_act)
        : draw_commands(commands), last_activity(last_act) {}
};

vector<Client> clients;
map<string, DisconnectedClient> disconnected_draw_commands;

void handle_client(Client& client);
void broadcast_update(const Client& sender, const char* buffer, size_t buffer_length);
bool process_command(Client& client, const char* buffer, ssize_t bytes_received);
void remove_client(Client& client);
void check_inactivity();
void adopt_draw_commands(const string& nickname);
void shutdown_server(int server_fd);

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        cerr << "Failed to create socket: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        cerr << "Failed to set socket options: " << strerror(errno) << endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
        cerr << "Failed to bind to port " << PORT << ": " << std::strerror(errno) << endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) != 0) { // '3' is the backlog, number of pending connections allowed
        cerr << "Failed to listen on socket: " << strerror(errno) << endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    cout << "Server is listening on port " << PORT << endl;

    thread inactivity_thread(check_inactivity); // Thread to check for inactive clients
    inactivity_thread.detach();

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            cerr << "Error accepting connection: " << strerror(errno) << endl;
            continue;
        }

        int keep_alive = 1;
        if (setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive)) < 0) {
            cerr << "Failed to set SO_KEEPALIVE: " << strerror(errno) << endl;
        }
        
        struct timeval timeout;
        timeout.tv_sec = 10; 
        timeout.tv_usec = 0;
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            cerr << "Failed to set receive timeout: " << strerror(errno) << std::endl;
        }
        if (setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
            std::cerr << "Failed to set send timeout: " << strerror(errno) << std::endl;
        }

        unique_lock<shared_mutex> lock(clients_mutex);
        if (clients.size() >= MAX_CLIENTS) {
            cout << "Maximum number of clients reached. Connection rejected.\n";
            const char* msg = "Server full: Maximum connection limit reached.";
            send(client_socket, msg, strlen(msg), 0);
            close(client_socket);
        } else {
            clients.emplace_back(client_socket, client_addr, client_addr_len);
            client_threads.emplace_back([&]() { handle_client(clients.back()); });
            client_threads.back().detach();
        }
    }

    shutdown_server(server_fd);
    return 0;
}

void handle_client(Client& client) {
    char buffer[1024];
    while (true) {
        ssize_t bytes_received = recv(client.fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                cout << "Client " << client.nickname << " disconnected gracefully.\n";
            } else {
                cerr << "Error receiving data from " << client.nickname << ": " << strerror(errno) << "\n";
            }
            break;
        }
        if (!process_command(client, buffer, bytes_received)) {
            cout << "Invalid command received from " << client.nickname << ". Disconnecting client.\n";
            break;
        }
        broadcast_update(client, buffer, bytes_received);
    }

    close(client.fd);
    remove_client(client);
}

void broadcast_update(const Client& sender, const char* buffer, size_t buffer_length) {
    lock_guard<shared_mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if (&client != &sender) {
            ssize_t num_bytes = send(client.fd, buffer, buffer_length, 0);
            if (num_bytes < 0) {
                perror("send");
                remove_client(client);
            }
        }
    }
}

bool process_command(Client& client, const char* buffer, ssize_t bytes_received) {
    // To do: implement command processing
    return true;
}

void remove_client(Client& client) {
    lock_guard<shared_mutex> lock(clients_mutex);
    disconnected_draw_commands[client.nickname] = DisconnectedClient(client.draw_commands, client.last_activity);
    close(client.fd); // Close the socket
    clients.erase(remove_if(clients.begin(), clients.end(), [&](const Client& c) { return c.fd == client.fd; }), clients.end());
}

void check_inactivity() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(60));
        unique_lock<shared_mutex> lock(clients_mutex);
        time_t now = time(nullptr);
        for (auto it = clients.begin(); it != clients.end(); ) {
            if (difftime(now, it->last_activity) > INACTIVITY_TIMEOUT) {
                cout << "Client " << it->nickname << " timed out due to inactivity.\n";
                close(it->fd);
                disconnected_draw_commands[it->nickname] = DisconnectedClient(it->draw_commands, it->last_activity);
                it = clients.erase(it);
            } else {
                ++it;
            }
        }

        for (auto it = disconnected_draw_commands.begin(); it != disconnected_draw_commands.end(); ) {
            // If the client has been disconnected for more than RECONNECT_TIMEOUT seconds, adopt their commands
            if (difftime(now, it->second.last_activity) > RECONNECT_TIMEOUT) {
                adopt_draw_commands(it->first);
                it = disconnected_draw_commands.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void adopt_draw_commands(const string& nickname) {
    auto it = disconnected_draw_commands.find(nickname);
    if (it != disconnected_draw_commands.end()) {
        // Iterate through stored commands and apply them to the canvas
        for (const auto& command : it->second.draw_commands) {
            //apply_draw_command(command);
        }
        cout << "Adopted draw commands from " << nickname << "\n";
    }
}