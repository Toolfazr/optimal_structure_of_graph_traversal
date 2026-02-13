#include "GenCmd.hpp"

GenCmd::GenCmd(std::string cmd) : cmd(cmd) {};

bool GenCmd::execute(GenManager& manager) {
    if (!manager.doCmd(*this))
    {
        return false;
    }

    for (auto& pendingCmd : toDoList)
    {
        if (!manager.doCmd(pendingCmd))
        {
            return false;
        }
    }

    return true;
}

void GenCmd::appendCmdToList(GenCmd cmd) {
    toDoList.push_back(cmd);
}

std::string GenCmd::getCmd() {
    return cmd;
}