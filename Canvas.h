#ifndef CANVAS_H
#define CANVAS_H

#include "Server.h"
#include "Commands.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <chrono>
#include <map>
#include <vector>
#include <mutex>

using namespace std;

struct DrawCommand {
    int id; // Unique identifier for the command
    string type; // Type of command (e.g., "line", "text")
    int x1, y1, x2, y2; // Coordinates for the command
    string text; // Text for the command (if applicable)
    int r, g, b; // RGB color values
};

class Canvas {
public:
    void addCommand(const DrawCommand& cmd);
    void removeCommand(int id);
    void modifyCommand(int id, const DrawCommand& newCmd);
    vector<DrawCommand> getCommands() const;

private:
    map<int, DrawCommand> commands;
    mutable mutex mtx;
};

extern Canvas canvas; // Global canvas object

#endif // CANVAS_H