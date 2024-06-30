#include "Canvas.h"

/**
 * @brief Adds a draw command to the canvas.
 *
 * This function adds a draw command to the canvas. The draw command is stored in the `commands` map
 * with a unique ID. The ID is incremented after each command is added.
 *
 * @param cmd The draw command to be added.
 */
void Canvas::addCommand(const DrawCommand& cmd) {
    lock_guard<mutex> lock(mtx);
    commands[next_id++] = cmd;
}

/**
 * @brief Removes a command from the canvas.
 * 
 * This function removes a command with the specified ID from the canvas.
 * The command is erased from the `commands` container.
 * 
 * @param id The ID of the command to be removed.
 */
void Canvas::removeCommand(int id) {
    lock_guard<mutex> lock(mtx);
    commands.erase(id);
}

/**
 * Modifies a draw command in the canvas.
 * 
 * This function modifies a draw command in the canvas by replacing it with a new command.
 * The original type and ID of the command are preserved in the updated command.
 * 
 * @param id The ID of the command to be modified.
 * @param newCmd The new draw command to replace the existing command.
 */
void Canvas::modifyCommand(int id, const DrawCommand& newCmd) {
    lock_guard<mutex> lock(mtx);
    auto it = commands.find(id);
    if (it != commands.end()) {
        // Preserve the original type and ID
        DrawCommand updatedCmd = newCmd;
        updatedCmd.type = it->second.type;
        updatedCmd.id = id;
        
        // Update the command
        it->second = updatedCmd;
        
        cout << "Command with ID " << id << " has been modified." << endl;
    } else {
        cout << "Command with ID " << id << " not found." << endl;
    }
}

/**
 * Retrieves the list of draw commands stored in the canvas.
 *
 * This function returns a vector of DrawCommand objects, representing the draw commands
 * stored in the canvas. The draw commands are retrieved in a thread-safe manner using a
 * lock_guard to ensure exclusive access to the commands container.
 *
 * @return A vector of DrawCommand objects representing the draw commands in the canvas.
 */
vector<DrawCommand> Canvas::getCommands() const {
    lock_guard<std::mutex> lock(mtx);
    vector<DrawCommand> cmdList;
    for (const auto& [id, cmd] : commands) {
        cmdList.push_back(cmd);
    }
    return cmdList;
}

void Canvas::printCommands() const {
    cout << "Number of commands: " << commands.size() << "\n";
    for (const auto& [id, cmd] : commands) {
        cout << "ID: " << id << ", Type: " << cmd.type << ", Coordinates: (" << cmd.x1 << ", " << cmd.y1 << ") to (" << cmd.x2 << ", " << cmd.y2 << "), Text: " << cmd.text << ", Color: (" << cmd.r << ", " << cmd.g << ", " << cmd.b << ")\n";
    }
}

/**
 * Sends the current commands to the specified file descriptor.
 * 
 * This function iterates over the commands stored in the Canvas object and sends them to the specified file descriptor.
 * Each command is formatted as a string and sent using the send() function.
 * The format of the command string depends on the type of command.
 * If the command type is "text", the string includes the command type, ID, coordinates, text, and color information.
 * If the command type is not "text", the string includes the command type, ID, coordinates, and color information.
 * The command string is terminated with the "END" delimiter.
 * 
 * @param fd The file descriptor to send the commands to.
 */
void Canvas::sendCurrentCommands(int fd) const {
    lock_guard<std::mutex> lock(mtx);
    for (const auto& [id, cmd] : commands) {
        string response = "draw ";
        if (cmd.type == "text") {
            response += cmd.type + " " + 
                        to_string(cmd.id) + " " + 
                        to_string(cmd.x1) + " " + 
                        to_string(cmd.y1) + " '" + 
                        cmd.text + "' " + 
                        to_string(cmd.r) + " " + 
                        to_string(cmd.g) + " " + 
                        to_string(cmd.b) + "\n";
        } else {
            response += cmd.type + " " + 
                        to_string(cmd.id) + " " + 
                        to_string(cmd.x1) + " " + 
                        to_string(cmd.y1) + " " + 
                        to_string(cmd.x2) + " " + 
                        to_string(cmd.y2) + " " + 
                        to_string(cmd.r) + " " + 
                        to_string(cmd.g) + " " + 
                        to_string(cmd.b) + "\n";
        }
        response += "END\n";  // Add delimiter
        send(fd, response.c_str(), response.size(), 0);
    }
}

/**
 * Sends filtered commands to a specified file descriptor.
 *
 * This function sends filtered commands from the `commands` container to the specified file descriptor `fd`.
 * The commands are filtered based on the tool and user filters provided as arguments.
 * If the tool filter is set to "all", all commands will be considered.
 * If the user filter is set to "all", all users' commands will be considered.
 * If the user filter is set to "mine", only the commands from the user associated with the specified file descriptor `fd` will be considered.
 *
 * @param fd The file descriptor to send the filtered commands to.
 * @param toolFilter The tool filter to apply. Set to "all" to consider all tools.
 * @param userFilter The user filter to apply. Set to "all" to consider all users' commands, or "mine" to consider only the commands from the user associated with the specified file descriptor `fd`.
 */
void Canvas::sendFilteredCommands(int fd, const string& toolFilter, const string& userFilter) const {
    lock_guard<std::mutex> lock(mtx);

    int matchCount = 0;
    for (const auto& [id, cmd] : commands) {
        
        bool toolMatch = (toolFilter == "all" || cmd.type == toolFilter);
        bool userMatch = (userFilter == "all" || (userFilter == "mine" && cmd.fd == fd));

        if (toolMatch && userMatch) {
            matchCount++;
            ostringstream oss;
            oss << "list " << cmd.id << " => [" << cmd.type << "] [" << cmd.r << " " << cmd.g << " " << cmd.b << "] ";
            if (cmd.type == "text") {
                oss << "[" << cmd.x1 << " " << cmd.y1 << "] *\"" << cmd.text << "\"*\n";
            } else {
                oss << "[" << cmd.x1 << " " << cmd.y1 << " " << cmd.x2 << " " << cmd.y2 << "]\n";
            }
            string response = oss.str();
            ssize_t sent = send(fd, response.c_str(), response.size(), 0);
        }
    }

    // Send END marker
    ssize_t sent = send(fd, "END\n", 4, 0);
}

/**
 * @brief Clears all the commands from the canvas and resets the next_id to 1.
 */
void Canvas::clearAll() {
    lock_guard<mutex> lock(mtx);
    commands.clear();
    next_id = 1;  // Reset the next_id to 1
    cout << "All commands cleared from the canvas" << endl;
}

/**
 * @brief Clears the client commands associated with a specific file descriptor.
 * 
 * This function removes all client commands from the canvas that are associated with the given file descriptor.
 * 
 * @param fd The file descriptor of the client.
 */
void Canvas::clearClientCommands(int fd) {
    lock_guard<mutex> lock(mtx);
    for (auto it = commands.begin(); it != commands.end();) {
        if (it->second.fd == fd) {
            it = commands.erase(it);
        } else {
            ++it;
        }
    }
    cout << "Client commands cleared from the canvas" << endl;
}
