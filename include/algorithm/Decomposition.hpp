#pragma once

#include "Graph.hpp"
#include <vector>

class Decomposition {
public:
    Decomposition() = default;

    std::vector<std::vector<Index>> getRanks(const Graph& g);
};
