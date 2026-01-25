#pragma once

#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"
#include <vector>

class Construction {
public:
    static bool reorderListForBFS(const AdjListGraph& g,
        const std::vector<std::string>& rank,
        AdjListGraph& out);
    static bool reorderListForDFS(const AdjListGraph& g,
            const std::vector<std::string>& rank,
            AdjListGraph& out);
    static bool reorderMatrixForBFS(const AdjMatrixGraph& g,
            const std::vector<std::string>& rank,
            AdjMatrixGraph& out);
    static bool reorderMatrixForDFS(const AdjMatrixGraph& g,
            const std::vector<std::string>& rank,
            AdjMatrixGraph& out);
};
