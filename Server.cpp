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
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        {
            shared_lock<shared_mutex> lock(clients_mutex);
            for (const auto& client : clients) {
                if (client.fd != -1) {
                    FD_SET(client.fd, &read_fds);
                    max_fd = std::max(max_fd, client.fd);
                }
            }
        }

        struct timeval timeout;
        timeout.tv_sec = 1;  // Set a 1-second timeout
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (activity < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted system call, just continue
            }
            cerr << "select error: " << strerror(errno) << endl;
            break;
        }

        if (activity == 0) {
            // Timeout occurred, use this opportunity to clean up disconnected clients
            check_inactivity();
            continue;
        }

        // Check for new connections
        if (FD_ISSET(server_fd, &read_fds)) {
            handle_new_connection();
        }

        // Check all clients for data
        vector<Client*> clients_to_remove;
        {
            shared_lock<shared_mutex> lock(clients_mutex);
            for (auto& client : clients) {
                if (client.fd != -1 && FD_ISSET(client.fd, &read_fds)) {
                    if (!handle_client(client)) {
                        clients_to_remove.push_back(&client);
                    }
                }
            }
        }

        // Remove disconnected clients
        /*for (auto client : clients_to_remove) {
            remove_client(*client);
        }*/
    }

    shutdown_server();
}

bool Server::handle_client(Client& client) {
    char buffer[1024];
    ssize_t bytes_received = recv(client.fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            std::cout << "Client " << client.nickname << " disconnected gracefully." << std::endl;
        } else {
            std::cerr << "Error receiving data from " << client.nickname << ": " << strerror(errno) << std::endl;
        }
        remove_client(client);
    } else {
        bool success = process_command(client, buffer, bytes_received);
        std::string response_message = success ? "Command processed successfully." : "Invalid command.";
        send(client.fd, response_message.c_str(), response_message.size(), 0);
        broadcast_update(client, buffer, bytes_received);
    }
}

void Server::broadcast_update(const Client& sender, const char* buffer, size_t buffer_length) {
    printf("Broadcasting update to %lu clients\n", clients.size());
    //shared_lock<shared_mutex> lock(clients_mutex);
    for (auto& client : clients) {
        printf("Client %s\n", client.nickname);
        if (client.fd != sender.fd) {
            printf("Sending to client %s\n", client.nickname);
            if (fcntl(client.fd, F_GETFD) != -1) {
                //const char* buffer = "Server broadcast"; // Change the assignment to a character array
                ssize_t num_bytes = send(client.fd, buffer, buffer_length, 0);
                if (num_bytes < 0) {
                    log("Error sending data to client " + std::string(client.nickname) + ": " + std::string(strerror(errno)));
                    client.fd = -1; // Mark client as removed
                }
            } else {
                log("Invalid file descriptor for client " + std::string(client.nickname) + ". Removing client.");
                client.fd = -1; // Mark client as removed
            }
        }
    }
}

bool Server::process_command(Client& client, const char* buffer, ssize_t bytes_received) {
    Commands processor;
    return processor.process(client, buffer, bytes_received);
}

void Server::remove_client(Client& client) {
    printf("Removing client %s\n", client.nickname);
    
    {
        unique_lock<shared_mutex> lock(clients_mutex);
        printf("Statement 1: %s\n", client.nickname);
        disconnected_draw_commands[client.nickname] = DisconnectedClient(client.draw_commands, client.last_activity);
        printf("Statement 2: %s\n", client.nickname);
    }
    
    int fd_to_close = client.fd;
    client.fd = -1; // Mark client as removed
    
    if (fd_to_close != -1) {
        printf("Statement 3: %s\n", client.nickname);
        close(fd_to_close); // Close the socket
    }
    
    {
        unique_lock<shared_mutex> lock(clients_mutex);
        clients.erase(remove_if(clients.begin(), clients.end(), 
            [](const Client& c) { return c.fd == -1; }),
            clients.end());
    }
}

void Server::check_inactivity() {
    vector<Client*> clients_to_remove;
    
    {
        shared_lock<shared_mutex> lock(clients_mutex);
        time_t now = time(nullptr);
        for (auto& client : clients) {
            if (client.fd != -1 && difftime(now, client.last_activity) > INACTIVITY_TIMEOUT) {
                clients_to_remove.push_back(&client);
            }
        }
    }
    
    for (auto client_ptr : clients_to_remove) {
        cout << "Client " << client_ptr->nickname << " timed out due to inactivity.\n";
        remove_client(*client_ptr);
        printf("Client removed %s\n", client_ptr->nickname);
    }

    {
        unique_lock<shared_mutex> lock(clients_mutex);
        time_t now = time(nullptr);
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
    DrawCommand cmd;
    istringstream iss(command);
    iss >> cmd.type >> cmd.id >> cmd.x1 >> cmd.y1 >> cmd.x2 >> cmd.y2 >> cmd.color;
    canvas.addCommand(cmd);
}

void Server::shutdown_server() {
    close(server_fd);
}

string Server::serialize_draw_command(const DrawCommand& cmd) {
    std::ostringstream oss;
    oss << cmd.type << " " << cmd.id << " " << cmd.x1 << " " << cmd.y1 << " " << cmd.x2 << " " << cmd.y2 << " " << cmd.color;
    return oss.str();
}

void Server::handle_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    
    if (new_socket < 0) {
        cerr << "Error accepting connection: " << strerror(errno) << endl;
        return;
    }

    // Set the new socket to non-blocking mode
    int flags = fcntl(new_socket, F_GETFL, 0);
    fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

    // Add new client to the client list
    {
        unique_lock<shared_mutex> lock(clients_mutex);
        clients.emplace_back(new_socket, client_addr, client_addr_len, "client_" + to_string(new_socket));
        cout << "New connection from client " << clients.back().nickname << endl;
    }

    // Send current canvas state to the new client
    canvas.sendCurrentCommands(new_socket);
}