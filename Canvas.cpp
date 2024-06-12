#include "Canvas.h"

void Canvas::addCommand(const DrawCommand& cmd) {
    lock_guard<mutex> lock(mtx);
    commands[next_id++] = cmd;
}

void Canvas::removeCommand(int id) {
    lock_guard<mutex> lock(mtx);
    commands.erase(id);
}

void Canvas::modifyCommand(int id, const DrawCommand& newCmd) {
    lock_guard<mutex> lock(mtx);
    commands[id] = newCmd;
}

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
        cout << "ID: " << id << ", Type: " << cmd.type << ", Coordinates: (" << cmd.x1 << ", " << cmd.y1 << ") to (" << cmd.x2 << ", " << cmd.y2 << "), Text: " << cmd.text << ", Color: (" << cmd.color << ")\n";
    }
}

/*void Canvas::sendCurrentCommands(int fd) const {
    lock_guard<std::mutex> lock(mtx);
    string response = "draw ";
    for (const auto& [id, cmd] : commands) {
        response += cmd.type + " " + to_string(cmd.x1) + " " + to_string(cmd.y1) + " " + to_string(cmd.x2) + " " + to_string(cmd.y2) + " " + cmd.color + "\n";
        response += "\n";
        send(fd, response.c_str(), response.size(), 0);
        // Print response
        cout << "Response: " << response;
        response = "draw ";
    }
}*/

void Canvas::sendCurrentCommands(int fd) const {
    lock_guard<std::mutex> lock(mtx);
    for (const auto& [id, cmd] : commands) {
        string response = "draw ";
        response += cmd.type + " " + to_string(cmd.x1) + " " + to_string(cmd.y1) + " " + to_string(cmd.x2) + " " + to_string(cmd.y2) + " " + cmd.color + "\n";
        response += "END\n";  // Add delimiter
        send(fd, response.c_str(), response.size(), 0);
        // Print response
        cout << "Response: " << response;
    }
}
