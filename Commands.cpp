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

bool Commands::process(Client& client, const char* buffer, ssize_t bytes_received) {
    Commands command = parse_command(buffer);
    cout << "Command type: " << command.type << endl;

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
    // Implement list command logic here
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
    // Implement clear command logic here
}

void Commands::show_commands(Client& client, const std::vector<std::string>& params, Canvas& canvas) {
    // Implement show command logic here
}

void Commands::apply_draw_command(const std::string& command) {
    std::istringstream iss(command);
    std::string cmdType;
    iss >> cmdType;

    DrawCommand drawCmd;

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
            getline(iss, remaining);

            // Find the last quote
            size_t last_quote = remaining.rfind('\'');

            // Extract the color and the text
            std::string colorStr = remaining.substr(last_quote + 2); // Skip the quote and the space
            std::istringstream colorIss(colorStr);
            colorIss >> drawCmd.r >> drawCmd.g >> drawCmd.b;

            drawCmd.text = remaining.substr(2, last_quote - 2); // Skip the initial quote

            std::cout << "Text: " << drawCmd.text << ", Color: R" << drawCmd.r << " G" << drawCmd.g << " B" << drawCmd.b << "\n";
        } else {
            iss >> drawCmd.x1 >> drawCmd.y1 >> drawCmd.x2 >> drawCmd.y2 >> drawCmd.r >> drawCmd.g >> drawCmd.b ;
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
              << ", Coordinates: (" << drawCmd.x1 << ", " << drawCmd.y1 << ") to (" << drawCmd.x2 << ", " << drawCmd.y2 
              << "), Color: R" << drawCmd.r << " G" << drawCmd.g << " B" << drawCmd.b << "\n";

    std::cout << "Applying command: " << command << "\n";
}
