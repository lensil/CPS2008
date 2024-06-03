#ifndef DISCONNECTED_CLIENT_H
#define DISCONNECTED_CLIENT_H

#include <vector>
#include <string>
#include <ctime>

struct DisconnectedClient {
    std::vector<std::string> draw_commands;
    std::time_t last_activity;

    DisconnectedClient() : last_activity(0) {}
    DisconnectedClient(const std::vector<std::string>& commands, time_t last_act)
        : draw_commands(commands), last_activity(last_act) {}
};

#endif // DISCONNECTED_CLIENT_H
