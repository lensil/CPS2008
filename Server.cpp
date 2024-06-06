#include "Server.h"
#include "Commands.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <chrono>
#include <fcntl.h>

using namespace std;

Server::Server(int port) : port(port), num_clients(0) {}

void Server::run() {
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
    address.sin_port = htons(port);

    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
        cerr << "Failed to bind to port " << port << ": " << strerror(errno) << endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) != 0) {
        cerr << "Failed to listen on socket: " << strerror(errno) << endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    cout << "Server is listening on port " << port << endl;

    thread inactivity_thread(&Server::check_inactivity, this); // Thread to check for inactive clients
    inactivity_thread.detach();

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            cerr << "Error accepting connection: " << strerror(errno) << endl;
            continue;
        }

        // Ensure the client socket is in blocking mode
        int flags = fcntl(client_socket, F_GETFL, 0);
        if (flags == -1) {
            std::cerr << "fcntl failed to get flags for socket: " << strerror(errno) << std::endl;
            close(client_socket);
            continue;
        }
        flags &= ~O_NONBLOCK;
        if (fcntl(client_socket, F_SETFL, flags) == -1) {
            std::cerr << "fcntl failed to set flags for socket: " << strerror(errno) << std::endl;
            close(client_socket);
            continue;
        }

        int keep_alive = 1;
        if (setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive)) < 0) {
            cerr << "Failed to set SO_KEEPALIVE: " << strerror(errno) << endl;
        }

        unique_lock<shared_mutex> lock(clients_mutex);
        if (clients.size() >= MAX_CLIENTS) {
            cout << "Maximum number of clients reached. Connection rejected.\n";
            const char* msg = "Server full: Maximum connection limit reached.";
            send(client_socket, msg, strlen(msg), 0);
            close(client_socket);
        } else {
            clients.emplace_back(client_socket, client_addr, client_addr_len);
            client_threads.emplace_back(&Server::handle_client, this, ref(clients.back()));
            client_threads.back().detach();
        }
    }

    shutdown_server();
}

void Server::handle_client(Client& client) {
    char buffer[1024];
    while (true) {
        ssize_t bytes_received = recv(client.fd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            std::cerr << "Error receiving data from " << client.nickname << " fd: " << client.fd << ": " << strerror(errno) << "\n";
            break;
        } else if (bytes_received == 0) {
            // Client disconnected
            std::cout << "Client " << client.nickname << " disconnected gracefully.\n";
            break;
        }

        if (!process_command(client, buffer, bytes_received)) {
            std::cout << "Invalid command received from " << client.nickname << ". Disconnecting client.\n";
            break;
        }
        broadcast_update(client, buffer, bytes_received);
    }

    close(client.fd);
    remove_client(client);
}

void Server::broadcast_update(const Client& sender, const char* buffer, size_t buffer_length) {
    shared_lock<shared_mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if (&client != &sender) {
            ssize_t num_bytes = send(client.fd, buffer, buffer_length, 0);
            if (num_bytes < 0) {
                std::cerr << "Error sending data to client " << client.nickname << " fd: " << client.fd << ": " << strerror(errno) << "\n";
                remove_client(client);
            }
        }
    }
}

bool Server::process_command(Client& client, const char* buffer, ssize_t bytes_received) {
    Commands processor;
    return processor.process(client, buffer, bytes_received);
}

void Server::remove_client(Client& client) {
    lock_guard<shared_mutex> lock(clients_mutex);
    disconnected_draw_commands[client.nickname] = DisconnectedClient(client.draw_commands, client.last_activity);
    close(client.fd); // Close the socket
    clients.erase(remove_if(clients.begin(), clients.end(), [&](const Client& c) { return c.fd == client.fd; }), clients.end());
}

void Server::check_inactivity() {
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
            if (difftime(now, it->second.last_activity) > RECONNECT_TIMEOUT) {
                adopt_draw_commands(it->first);
                it = disconnected_draw_commands.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void Server::adopt_draw_commands(const std::string& nickname) {
    auto it = disconnected_draw_commands.find(nickname);
    if (it != disconnected_draw_commands.end()) {
        // Iterate through stored commands and apply them to the canvas
        for (const auto& command : it->second.draw_commands) {
            apply_draw_command(command);
        }
        cout << "Adopted draw commands from " << nickname << "\n";
    }
}

void Server::apply_draw_command(const std::string& command) {
    // Parse the command and apply it to the shared canvas
    // This is a placeholder function and needs to be implemented based on your drawing logic
    cout << "Applying command: " << command << "\n";
}

void Server::shutdown_server() {
    close(server_fd);
}

string Server::serialize_draw_command(const DrawCommand& cmd) {
    std::ostringstream oss;
    oss << cmd.type << " " << cmd.id << " " << cmd.x1 << " " << cmd.y1 << " " << cmd.x2 << " " << cmd.y2 << " " << cmd.r << " " << cmd.g << " " << cmd.b;
    return oss.str();
}
