#include "Generator.hpp"
#include "AdjListGraph.hpp"

bool Generator::parseCmd(GenCmd cmd) {
    return false;
}

std::unique_ptr<Graph> Generator::genGraph() {
    return std::make_unique<AdjListGraph>(new AdjListGraph());
}