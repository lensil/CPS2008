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

/**
 * @brief Runs the server and handles incoming connections and client data.
 * 
 * This function creates a socket, binds it to a port, and listens for incoming connections.
 * It sets the server socket to non-blocking mode and adds it to the master set.
 * It also creates a separate thread to check for inactive clients.
 * 
 * The function enters a loop where it uses the `select` function to monitor the server socket and client sockets for activity.
 * If there is activity on the server socket, a new connection is handled.
 * If there is activity on a client socket, the client data is handled.
 * 
 * The function also checks for a timeout and cleans up disconnected clients.
 * 
 * @note This function runs indefinitely until an error occurs or the server is shut down.
 */
void Server::run() {
    struct sockaddr_in address;
    int opt = 1;

    // Create the server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        cerr << "Failed to create socket: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        cerr << "Failed to set socket options: " << strerror(errno) << endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
        cerr << "Failed to bind to port " << port << ": " << strerror(errno) << endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
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

    // Start a separate thread to check for inactive clients
    thread inactivity_thread(&Server::check_inactivity, this);
    inactivity_thread.detach();

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        {
            shared_lock<shared_mutex> lock(clients_mutex);
            // Add all active client sockets to the read_fds set
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

        // Use select to monitor the sockets for activity
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
    }

    // Shutdown the server
    shutdown_server();
}

/**
 * Handles a client connection.
 *
 * This function receives data from the client, processes the command,
 * sends a response message back to the client, and broadcasts the
 * command to all connected clients.
 *
 * @param client The client object representing the connected client.
 * @return True if the command was processed successfully, false otherwise.
 */
bool Server::handle_client(Client& client) {
    // Receive data from the client
    char buffer[1024];
    ssize_t bytes_received = recv(client.fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            std::cout << "Client " << client.nickname << " disconnected gracefully." << std::endl;
        } else {
            std::cerr << "Error receiving data from " << client.nickname << ": " << strerror(errno) << std::endl;
        }
        // Remove the client from the server
        remove_client(client);
        return false;
    } else {
        // Process the received command
        bool success = process_command(client, buffer, bytes_received, client.fd);
        std::string response_message = success ? "Command processed successfully." : "Invalid command.";
        // Send the response message back to the client
        send(client.fd, response_message.c_str(), response_message.size(), 0);
        // Broadcast the command to all connected clients
        broadcast_update(client, buffer, bytes_received);
    }
    return true;
}

/**
 * Broadcasts an update to all connected clients, except the sender.
 *
 * @param sender The client who sent the update.
 * @param buffer A pointer to the buffer containing the update data.
 * @param buffer_length The length of the update data in bytes.
 */
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

/**
 * Processes a command received from a client.
 *
 * This function uses an instance of the `Commands` class to process the command received from the client.
 * It passes the necessary parameters to the `Commands::process` function and returns the result.
 *
 * @param client The client object associated with the command.
 * @param buffer The buffer containing the command data.
 * @param bytes_received The number of bytes received in the buffer.
 * @param client_fd The file descriptor of the client connection.
 * @return `true` if the command was processed successfully, `false` otherwise.
 */
bool Server::process_command(Client& client, const char* buffer, ssize_t bytes_received, int client_fd) {
    Commands processor;
    return processor.process(client, buffer, bytes_received, client_fd);
}


/**
 * @brief Removes a client from the server.
 * 
 * This function removes the specified client from the server. It performs the following steps:
 * 1. Stores the client's draw commands and last activity in the disconnected_draw_commands map.
 * 2. Marks the client as removed by setting its file descriptor (fd) to -1.
 * 3. Closes the socket associated with the client's file descriptor, if it is valid.
 * 4. Removes the client from the clients vector.
 * 
 * @param client The client to be removed.
 */
void Server::remove_client(Client& client) {
    

    disconnected_draw_commands[client.nickname] = DisconnectedClient(client.draw_commands, client.last_activity);
    
    int fd_to_close = client.fd;
    client.fd = -1; // Mark client as removed
    
    if (fd_to_close != -1) {
        close(fd_to_close); // Close the socket
    }
    
    {
        clients.erase(remove_if(clients.begin(), clients.end(), 
            [](const Client& c) { return c.fd == -1; }),
            clients.end());
    }
}

/**
 * Checks for inactivity of clients and removes them if they have timed out.
 * Also checks for disconnected clients that need to be reconnected.
 */
void Server::check_inactivity() {
    // Create a vector to store the clients that need to be removed due to inactivity
    vector<Client*> clients_to_remove;
    
    {
        // Acquire a shared lock on the clients_mutex to safely access the clients vector
        shared_lock<shared_mutex> lock(clients_mutex); 
        
        // Get the current time
        time_t now = time(nullptr);
        
        // Iterate over all clients
        for (auto& client : clients) {
            // Check if the client is active and has exceeded the inactivity timeout
            if (client.fd != -1 && difftime(now, client.last_activity) > INACTIVITY_TIMEOUT) {
                // Add the client to the clients_to_remove vector
                clients_to_remove.push_back(&client);
            }
        }
    }
    
    // Remove the inactive clients
    for (auto client_ptr : clients_to_remove) {
        // Print a message indicating that the client has timed out due to inactivity
        cout << "Client " << client_ptr->nickname << " timed out due to inactivity.\n";
        
        // Remove the client from the server
        remove_client(*client_ptr);
        
        // Print a message indicating that the client has been removed
        printf("Client removed %s\n", client_ptr->nickname);
    }

    {
        unique_lock<shared_mutex> lock(clients_mutex); // Acquire a unique lock to modify the clients vector
        time_t now = time(nullptr); // Get the current time

        // Check for disconnected clients that need to be reconnected
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

/**
 * @brief Adopts draw commands from a disconnected client.
 * 
 * This function retrieves the draw commands associated with the specified nickname from the disconnected_draw_commands map.
 * It then applies each draw command by calling the apply_draw_command function.
 * Finally, it prints a message indicating that the draw commands have been adopted.
 * 
 * @param nickname The nickname of the disconnected client.
 */
void Server::adopt_draw_commands(const std::string& nickname) {
    auto it = disconnected_draw_commands.find(nickname);
    if (it != disconnected_draw_commands.end()) {
        for (const auto& command : it->second.draw_commands) {
            apply_draw_command(command);
        }
        cout << "Adopted draw commands from " << nickname << "\n";
    }
}

/**
 * Applies a draw command to the server's canvas.
 *
 * This function takes a command string and parses it to extract the relevant information
 * for a draw command. It then creates a DrawCommand object and adds it to the server's canvas.
 *
 * @param command The draw command string to apply.
 */
void Server::apply_draw_command(const std::string& command) {
    cout << "Applying command: " << command << "\n";
    DrawCommand cmd;
    istringstream iss(command);
    iss >> cmd.type >> cmd.id >> cmd.x1 >> cmd.y1 >> cmd.x2 >> cmd.y2 >> cmd.r >> cmd.g >> cmd.b;
    canvas.addCommand(cmd);
}

/**
 * @brief Shuts down the server by closing the server file descriptor.
 */
void Server::shutdown_server() {
    close(server_fd);
}

/**
 * Serializes a DrawCommand object into a string representation.
 *
 * @param cmd The DrawCommand object to be serialized.
 * @return The serialized string representation of the DrawCommand object.
 */
string Server::serialize_draw_command(const DrawCommand& cmd) {
    std::ostringstream oss;
    oss << cmd.type << " " << cmd.id << " " << cmd.x1 << " " << cmd.y1 << " " << cmd.x2 << " " << cmd.y2 << " " << cmd.r << " " << cmd.g << " " << cmd.b;
    return oss.str();
}

/**
 * Handles a new incoming connection from a client.
 * 
 * This function accepts a new connection from a client, sets the socket to non-blocking mode,
 * adds the client to the client list, and sends the current canvas state to the new client.
 * 
 * @return void
 */
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