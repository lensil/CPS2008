#include "Server.h"
#include "Commands.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <chrono>
#include <fcntl.h>
#include <sstream>

using namespace std;

void log(const string& message) {
    cerr << "[SERVER LOG] " << message << endl;
}

Server::Server(int port) : port(port), num_clients(0), max_fd(-1) {
    FD_ZERO(&master_set);
    FD_ZERO(&read_fds);
}

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

    // Set the server socket to non-blocking mode
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    // Add the server socket to the master set
    FD_SET(server_fd, &master_set);
    max_fd = server_fd;

    thread inactivity_thread(&Server::check_inactivity, this); // Thread to check for inactive clients
    inactivity_thread.detach();

    while (true) {
        read_fds = master_set;

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0 && errno != EINTR) {
            cerr << "select error: " << strerror(errno) << endl;
            break;
        }

        // Check for new connections
        if (FD_ISSET(server_fd, &read_fds)) {
            int new_socket;
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
                cerr << "Error accepting connection: " << strerror(errno) << endl;
                continue;
            }

            // Set the new socket to non-blocking mode
            fcntl(new_socket, F_SETFL, O_NONBLOCK);

            // Add new client to the client list
            {
                unique_lock<shared_mutex> lock(clients_mutex);
                clients.emplace_back(new_socket, client_addr, client_addr_len, "client_" + std::to_string(new_socket));
                FD_SET(new_socket, &master_set);
                if (new_socket > max_fd) {
                    max_fd = new_socket;
                }
                cout << "New connection from client " << clients.back().nickname << endl;
            }
        }

        // Check all clients for data
        {
            unique_lock<shared_mutex> lock(clients_mutex);
            for (auto it = clients.begin(); it != clients.end();) {
                int client_fd = it->fd;
                if (FD_ISSET(client_fd, &read_fds)) {
                    handle_client(*it);
                    if (it->fd == -1) {
                        FD_CLR(client_fd, &master_set);
                        it = clients.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    ++it;
                }
            }
        }
    }

    shutdown_server();
}

void Server::handle_client(Client& client) {
    char buffer[1024];
    ssize_t bytes_received = recv(client.fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            std::cout << "Client " << client.nickname << " disconnected gracefully." << std::endl;
        } else {
            std::cerr << "Error receiving data from " << client.nickname << ": " << strerror(errno) << std::endl;
        }
        close(client.fd);
        client.fd = -1; // Mark client as removed
    } else {
        bool success = process_command(client, buffer, bytes_received);
        std::string response_message = success ? "Command processed successfully." : "Invalid command.";
        send(client.fd, response_message.c_str(), response_message.size(), 0);
        
        broadcast_update(client, response_message.c_str(), response_message.size());
    }
}

void Server::broadcast_update(const Client& sender, const char* buffer, size_t buffer_length) {
    std::unique_lock<std::shared_mutex> lock(clients_mutex); // Use unique_lock for writing

    for (auto it = clients.begin(); it != clients.end();) {
        if (it->fd != sender.fd) {
            if (fcntl(it->fd, F_GETFD) != -1) {
                ssize_t num_bytes = send(it->fd, buffer, buffer_length, 0);
                if (num_bytes < 0) {
                    log("Error sending data to client " + std::string(it->nickname) + ": " + std::string(strerror(errno)));
                    close(it->fd); // Close the socket
                    it = clients.erase(it); // Remove client from the list
                } else {
                    ++it;
                }
            } else {
                log("Invalid file descriptor for client " + std::string(it->nickname) + ". Removing client.");
                close(it->fd); // Close the socket
                it = clients.erase(it); // Remove client from the list
            }
        } else {
            ++it;
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
    client.fd = -1; // Mark client as removed
}

void Server::check_inactivity() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(60));
        unique_lock<shared_mutex> lock(clients_mutex);
        time_t now = time(nullptr);
        for (auto it = clients.begin(); it != clients.end();) {
            if (difftime(now, it->last_activity) > INACTIVITY_TIMEOUT) {
                cout << "Client " << it->nickname << " timed out due to inactivity.\n";
                close(it->fd);
                disconnected_draw_commands[it->nickname] = DisconnectedClient(it->draw_commands, it->last_activity);
                it = clients.erase(it);
            } else {
                ++it;
            }
        }

        for (auto it = disconnected_draw_commands.begin(); it != disconnected_draw_commands.end();) {
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
        for (const auto& command : it->second.draw_commands) {
            apply_draw_command(command);
        }
        cout << "Adopted draw commands from " << nickname << "\n";
    }
}

void Server::apply_draw_command(const std::string& command) {
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
