#pragma once

#include <memory>
#include "Graph.hpp"

class GenCmd;
class Generator;

class GenManager {
public:
    static GenManager& getGenMngr();
    bool doCmd(GenCmd cmd);
    void appendGenerator(std::unique_ptr<Generator> gen);
    void clearResults();
private:
    GenManager();
    virtual ~GenManager();

    GenManager(const GenManager&) = delete;
    GenManager& operator=(const GenManager&) = delete;

    std::vector<std::unique_ptr<Generator>> generators;
    std::vector<std::unique_ptr<Graph>> results;
};