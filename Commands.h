#ifndef COMMANDS_H
#define COMMANDS_H

#include "Client.h"
#include "Canvas.h"
#include <string>
#include <vector>
#include <sstream>

class Canvas; 

enum CommandType {
    TOOL,
    COLOUR,
    DRAW,
    LIST,
    SELECT,
    DELETE,
    UNDO,
    CLEAR,
    SHOW,
    EXIT,
    INVALID
};


class Commands {
public:
    //Default constructor
    Commands() : type(INVALID) {}
    Commands(CommandType t, const std::vector<std::string>& params) : type(t), parameters(params) {}
    bool process(Client& client, const char* buffer, ssize_t bytes_received);
    CommandType get_command_type(const std::string& command_str);
    Commands parse_command(const std::string& input);
private:
    CommandType type;
    std::vector<std::string> parameters;
    void apply_draw_command(const std::string& command);
    void list_commands(Client& client, const std::vector<std::string>& params, Canvas& canvas);
    void select_command(Client& client, const std::vector<std::string>& params);
    void delete_command(Client& client, const std::vector<std::string>& params, Canvas& canvas);
    void undo_command(Client& client);
    void clear_commands(Client& client, const std::vector<std::string>& params, Canvas& canvas);
    void show_commands(Client& client, const std::vector<std::string>& params, Canvas& canvas);
};

#endif // COMMANDS_H
