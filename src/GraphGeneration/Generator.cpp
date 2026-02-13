#include "Generator.hpp"
#include "GenCmd.hpp"
#include "AdjListGraph.hpp"

bool Generator::parseCmd(GenCmd cmd) {
    return false;
}

std::unique_ptr<Graph> Generator::genGraph() {
    return std::make_unique<AdjListGraph>();
}
