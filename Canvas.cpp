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
    if (commands.find(id) != commands.end()) {
        commands[id] = newCmd;
        commands[id].id = id;  // Ensure the ID remains the same
    } else {
        cout << "Command with ID " << id << " not found." << endl;
    }
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
        cout << "ID: " << id << ", Type: " << cmd.type << ", Coordinates: (" << cmd.x1 << ", " << cmd.y1 << ") to (" << cmd.x2 << ", " << cmd.y2 << "), Text: " << cmd.text << ", Color: (" << cmd.r << ", " << cmd.g << ", " << cmd.b << ")\n";
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
        cout << "Response: " << response;
    }
}
