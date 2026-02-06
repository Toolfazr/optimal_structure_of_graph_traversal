#pragma once

#include "Generator.hpp"
#include "GenCmd.hpp"
#include <memory>

class GenManager {
public:
    static GenManager& getGenMngr();
    bool doCmd(GenCmd cmd);
    void appendGenerator(std::unique_ptr<Generator> gen);
    void clearResults();
private:
    GenManager();
    ~GenManager() = default;

    GenManager(const GenManager&) = delete;
    GenManager& operator=(const GenManager&) = delete;

    std::vector<std::unique_ptr<Generator>> generators;
    std::vector<std::unique_ptr<Graph>> results;
};