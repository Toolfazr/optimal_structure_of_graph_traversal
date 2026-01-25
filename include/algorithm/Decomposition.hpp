#pragma once

#include "Graph.hpp"
#include <vector>
#include <string>

class Decomposition {
public:
    Decomposition() = default;
    static std::vector<std::vector<std::string>> getRanks(const Graph& g);
};
