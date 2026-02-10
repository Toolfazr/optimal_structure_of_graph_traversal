#include "GenCmd.hpp"

GenCmd::GenCmd(std::string cmd) : cmd(cmd) {};

bool GenCmd::execute(GenManager& manager) {
    manager.doCmd(*this);
    for(auto& cmd : toDoList) {
        manager.doCmd(*this);
    }
}

void GenCmd::appendCmdToList(GenCmd cmd) {
    toDoList.push_back(cmd);
}

std::string GenCmd::getCmd() {
    return cmd;
}