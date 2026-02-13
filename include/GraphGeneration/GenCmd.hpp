#pragma once

#include <vector>
#include <string>

class GenManager;

class GenCmd {
public:
    GenCmd(std::string cmd);
    bool execute(GenManager& manager);
    void appendCmdToList(GenCmd cmd);
    std::string getCmd();
private:
    std::vector<GenCmd> toDoList;
    std::string cmd;
};