#pragma once

#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"
#include <vector>

class Construction {
public:
    static AdjListGraph getListForBFS(const std::vector<Index>& rank);
    static AdjMatrixGraph getMatrixForBFS(const std::vector<Index>& rank);
    static AdjListGraph getListForDFS(const std::vector<Index>& rank);
    static AdjMatrixGraph getMatrixForDFS(const std::vector<Index>& rank);
};
