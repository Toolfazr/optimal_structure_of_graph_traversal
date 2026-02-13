#include "GenManager.hpp"

#include "GenCmd.hpp"
#include "RandomGen.hpp"

GenManager::GenManager() {
    appendGenerator(std::make_unique<RandomGen>());
}

GenManager& GenManager::getGenMngr() {
    static GenManager manager;
    return manager;
}

bool GenManager::doCmd(GenCmd cmd) {
    for(auto& gen : generators) {
        if(gen->parseCmd(cmd)) {
            results.push_back(gen->genGraph());
            return true;
        }
    }

    return false;
}

void GenManager::appendGenerator(std::unique_ptr<Generator> gen) {
    generators.push_back(std::move(gen));
}

void GenManager::clearResults() {
    results.clear();
}