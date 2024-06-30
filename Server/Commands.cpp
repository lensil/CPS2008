#include "Commands.h"
#include <iostream>

extern Canvas canvas; // Use the global canvas object

/**
 * Returns the corresponding CommandType based on the given command string.
 *
 * @param command_str The command string to be checked.
 * @return The corresponding CommandType if the command string is valid, otherwise returns INVALID.
 */
CommandType Commands::get_command_type(const std::string& command_str) {
    if (command_str == "tool") return TOOL;
    if (command_str == "colour") return COLOUR;
    if (command_str == "draw") return DRAW;
    if (command_str == "list") return LIST;
    if (command_str == "select") return SELECT;
    if (command_str == "delete") return DELETE;
    if (command_str == "undo") return UNDO;
    if (command_str == "clear") return CLEAR;
    if (command_str == "show") return SHOW;
    if (command_str == "modify") return MODIFY;
    if (command_str == "exit") return EXIT;
    return INVALID;
}

/**
 * Parses the input string to extract the command type and parameters.
 *
 * @param input The input string containing the command.
 * @return A Commands object representing the parsed command.
 */
Commands Commands::parse_command(const std::string& input) {
    istringstream iss(input);
    string command_str;
    iss >> command_str;
    cout << "Command_str: " << command_str << endl;


    CommandType type = get_command_type(command_str);

    vector<string> parameters;
    string param;
    while (iss >> param) {
        cout << "Param: " << param << endl;
        parameters.push_back(param);
    }

    return Commands(type, parameters);
}

/**
 * @brief Processes the command received from the client.
 * 
 * This function takes a Client object, a buffer containing the command, the number of bytes received,
 * and the client file descriptor as parameters. It parses the command, determines its type, and performs
 * the corresponding action based on the command type. The function returns true if the command was processed
 * successfully, and false otherwise.
 * 
 * @param client The client object representing the connected client.
 * @param buffer The buffer containing the command received from the client.
 * @param bytes_received The number of bytes received in the buffer.
 * @param client_fd The file descriptor of the client.
 * @return true if the command was processed successfully, false otherwise.
 */
bool Commands::process(Client& client, const char* buffer, ssize_t bytes_received, int client_fd) {
    Commands command = parse_command(buffer);
    cout << "Commands type: " << command.type << endl;

    switch (command.type) {
        case DRAW:
            apply_draw_command(buffer, client_fd);
            break;
        case LIST:
            list_commands(client, command.parameters, canvas);
            break;
        case SELECT:
            select_command(client, command.parameters);
            break;
        case DELETE:
            delete_command(client, command.parameters, canvas);
            break;
        case UNDO:
            undo_command(client);
            break;
        case CLEAR:
            clear_commands(client, command.parameters, canvas);
            break;
        case SHOW:
            show_commands(client, command.parameters, canvas);
            break;
        case MODIFY:
            apply_modify_command(buffer, client_fd);
            break;
        case EXIT:
            return false;
        default:
            std::cerr << "Invalid command received: " << buffer << std::endl;
            return false;
    }

    return true;
}

/**
 * Sends filtered commands to the client.
 * 
 * @param client The client to send the commands to.
 * @param params The parameters for filtering the commands.
 *               The first parameter is the tool filter and the second parameter is the user filter.
 * @param canvas The canvas containing the commands.
 */
void Commands::list_commands(Client& client, const std::vector<std::string>& params, Canvas& canvas) {
    string toolFilter = params[0];
    string userFilter = params[1];

    canvas.sendFilteredCommands(client.fd, toolFilter, userFilter);
}

void Commands::select_command(Client& client, const std::vector<std::string>& params) {
    // Implement select command logic here
}

/**
 * Deletes a command from the canvas.
 *
 * @param client The client who issued the command.
 * @param params The parameters passed to the command.
 *               The first parameter is the ID of the command to be deleted.
 * @param canvas The canvas object.
 */
void Commands::delete_command(Client& client, const std::vector<std::string>& params, Canvas& canvas) {
    int id = std::stoi(params[0]);
    canvas.removeCommand(id);
}

void Commands::undo_command(Client& client) {
    // Implement undo command logic here
}

/**
 * Clears commands on the canvas based on the specified parameters.
 * If the parameter is "all", it clears all commands on the canvas.
 * If the parameter is "mine", it clears only the commands associated with the client.
 *
 * @param client The client object.
 * @param params The vector of string parameters.
 * @param canvas The canvas object.
 */
void Commands::clear_commands(Client& client, const std::vector<std::string>& params, Canvas& canvas) {
    if (!params.empty() && params[0] == "all") {
        canvas.clearAll();
    }
    else if (!params.empty() && params[0] == "mine") {
        canvas.clearClientCommands(client.fd);
    } 
    
}

void Commands::show_commands(Client& client, const std::vector<std::string>& params, Canvas& canvas) {
    // Implement show command logic here
}

/**
 * Applies a draw command to the canvas.
 *
 * This function parses the given command string and applies the corresponding draw command to the canvas.
 * The command string should be in the format: "<command_type> <command_arguments>".
 * Supported command types are "draw", "delete", and "modify".
 *
 * @param command The command string to apply.
 * @param client_fd The file descriptor of the client.
 */
void Commands::apply_draw_command(const std::string& command, int client_fd) {
    std::istringstream iss(command);
    std::string cmdType;
    iss >> cmdType;

    DrawCommand drawCmd;

    drawCmd.fd = client_fd;

    if (cmdType == "draw") {
        std::cout << "Drawing command\n";
        iss >> drawCmd.type;
        iss >> drawCmd.id;
        printf("ID: %d\n", drawCmd.id);
        if (drawCmd.type == "text") {
            std::cout << "Text command\n";
            iss >> drawCmd.x1 >> drawCmd.y1;

            // Get the remaining part of the string
            std::string remaining;
            std::getline(iss, remaining);

            // Find the last quote
            size_t last_quote = remaining.rfind('\'');

            // Extract the color and the text
            std::string color = remaining.substr(last_quote + 2); // Skip the quote and the space
            drawCmd.text = remaining.substr(2, last_quote - 2); // Skip the initial quote

            // Parse RGB values
            std::istringstream color_iss(color);
            color_iss >> drawCmd.r >> drawCmd.g >> drawCmd.b;

            std::cout << "Text: " << drawCmd.text << ", Color: (" << drawCmd.r << ", " << drawCmd.g << ", " << drawCmd.b << ")\n";
        } else {
            iss >> drawCmd.x1 >> drawCmd.y1 >> drawCmd.x2 >> drawCmd.y2 >> drawCmd.r >> drawCmd.g >> drawCmd.b;
        }
        canvas.addCommand(drawCmd);
    } else if (cmdType == "delete") { // Delete command
        int id;
        iss >> id;
        canvas.removeCommand(id); 
    } else if (cmdType == "modify") { // Modify command
        iss >> drawCmd.id >> drawCmd.type >> drawCmd.x1 >> drawCmd.y1 >> drawCmd.x2 >> drawCmd.y2 >> drawCmd.r >> drawCmd.g >> drawCmd.b;
        canvas.modifyCommand(drawCmd.id, drawCmd); 
    }
}

/**
 * Modifies a command based on the given command string and applies the changes to the canvas.
 *
 * @param command The command string to modify.
 * @param client_fd The file descriptor of the client.
 */
void Commands::apply_modify_command(const std::string& command, int client_fd) {
    std::istringstream iss(command);
    std::string cmdType;
    iss >> cmdType;

    DrawCommand drawCmd;
    drawCmd.fd = client_fd;

    int id;
    iss >> id;
    drawCmd.id = id;

    std::string subCommand;
    iss >> subCommand;

    if (subCommand == "colour") {
        iss >> drawCmd.r >> drawCmd.g >> drawCmd.b;
        iss >> subCommand;  // Read "draw"
        iss >> drawCmd.x1 >> drawCmd.y1 >> drawCmd.x2 >> drawCmd.y2;
        drawCmd.type =  "line";  // Assume it's a line for now
    } else if (subCommand == "draw") {
        // Keep the existing color
        iss >> drawCmd.x1 >> drawCmd.y1 >> drawCmd.x2 >> drawCmd.y2;
        drawCmd.type = "line";  // Assume it's a line for now
    }

    canvas.modifyCommand(id, drawCmd);

    std::cout << "Modifying command: " << command << "\n";
}