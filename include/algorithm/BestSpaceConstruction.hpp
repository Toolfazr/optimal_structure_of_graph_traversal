#pragma once

#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"

class BestSpaceConstruction {
public:
    static AdjListGraph getBestSpaceConstruction(AdjListGraph& graph);
    static AdjMatrixGraph getBestSpaceConstruction(AdjMatrixGraph& graph);
};