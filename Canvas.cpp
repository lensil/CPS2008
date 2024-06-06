#include "Canvas.h"

void Canvas::addCommand(const DrawCommand& cmd) {
    lock_guard<mutex> lock(mtx);
    commands[cmd.id] = cmd;
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