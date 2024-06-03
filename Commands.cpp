#include "Commands.h"
#include <iostream>

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
    if (command_str == "exit") return EXIT;
    return INVALID;
}

Commands Commands::parse_command(const std::string& input) {
    std::istringstream iss(input);
    std::string command_str;
    iss >> command_str;

    CommandType type = get_command_type(command_str);
    std::vector<std::string> parameters;
    std::string param;
    while (iss >> param) {
        parameters.push_back(param);
    }

    return Commands(type, parameters);
}

bool Commands::process(Client& client, const char* buffer, ssize_t bytes_received) {
    Commands command = parse_command(buffer);

    switch (command.type) {
        case TOOL:
            // Implement tool selection logic
            break;
        case COLOUR:
            // Implement color setting logic
            break;
        case DRAW:
            apply_draw_command(buffer);
            break;
        case LIST:
            list_commands(client, command.parameters);
            break;
        case SELECT:
            select_command(client, command.parameters);
            break;
        case DELETE:
            delete_command(client, command.parameters);
            break;
        case UNDO:
            undo_command(client);
            break;
        case CLEAR:
            clear_commands(client, command.parameters);
            break;
        case SHOW:
            show_commands(client, command.parameters);
            break;
        case EXIT:
            // Handle client exit
            return false;
        default:
            std::cerr << "Invalid command received: " << buffer << std::endl;
            return false;
    }

    return true;
}

void Commands::list_commands(Client& client, const std::vector<std::string>& params) {
    // Implement list command logic here
}

void Commands::select_command(Client& client, const std::vector<std::string>& params) {
    // Implement select command logic here
}

void Commands::delete_command(Client& client, const std::vector<std::string>& params) {
    // Implement delete command logic here
}

void Commands::undo_command(Client& client) {
    // Implement undo command logic here
}

void Commands::clear_commands(Client& client, const std::vector<std::string>& params) {
    // Implement clear command logic here
}

void Commands::show_commands(Client& client, const std::vector<std::string>& params) {
    // Implement show command logic here
}

void Commands::apply_draw_command(const std::string& command) {
    // Parse the command and apply it to the shared canvas
    // This is a placeholder function and needs to be implemented based on your drawing logic
    std::cout << "Applying command: " << command << "\n";
}
