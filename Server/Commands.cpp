#include "Commands.h"
#include <iostream>

extern Canvas canvas; // Use the global canvas object

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
            // Handle client exit
            return false;
        default:
            std::cerr << "Invalid command received: " << buffer << std::endl;
            return false;
    }

    return true;
}

void Commands::list_commands(Client& client, const std::vector<std::string>& params, Canvas& canvas) {
    string toolFilter = params[0];
    string userFilter = params[1];

    canvas.sendFilteredCommands(client.fd, toolFilter, userFilter);
}

void Commands::select_command(Client& client, const std::vector<std::string>& params) {
    // Implement select command logic here
}

void Commands::delete_command(Client& client, const std::vector<std::string>& params, Canvas& canvas) {
    int id = std::stoi(params[0]);
    canvas.removeCommand(id);
}

void Commands::undo_command(Client& client) {
    // Implement undo command logic here
}

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
    } else if (cmdType == "delete") {
        int id;
        iss >> id;
        canvas.removeCommand(id);
    } else if (cmdType == "modify") {
        iss >> drawCmd.id >> drawCmd.type >> drawCmd.x1 >> drawCmd.y1 >> drawCmd.x2 >> drawCmd.y2 >> drawCmd.r >> drawCmd.g >> drawCmd.b;
        canvas.modifyCommand(drawCmd.id, drawCmd);
    }
    std::cout << "Type: " << drawCmd.type << ", ID: " << drawCmd.id 
              << ", Coordinates: (" << drawCmd.x1 << ", " << drawCmd.y1 << ") to (" 
              << drawCmd.x2 << ", " << drawCmd.y2 << "), Color: (" 
              << drawCmd.r << ", " << drawCmd.g << ", " << drawCmd.b << ")\n";

    std::cout << "Applying command: " << command << "\n";
}

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
        drawCmd.type = "line";  // Assume it's a line for now
    } else if (subCommand == "draw") {
        // Keep the existing color (you may need to fetch it from the canvas)
        iss >> drawCmd.x1 >> drawCmd.y1 >> drawCmd.x2 >> drawCmd.y2;
        drawCmd.type = "line";  // Assume it's a line for now
    }

    canvas.modifyCommand(id, drawCmd);

    std::cout << "Modifying command: " << command << "\n";
}