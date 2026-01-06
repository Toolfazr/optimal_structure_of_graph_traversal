#pragma once

#include "Graph.hpp"
#include "MetricsResStorage.hpp"
#include <string>

class Utility {
public:
    static void extractGraphInfo(
        const Graph& graph,
        std::vector<std::vector<Index>>& originalAdj,
        std::vector<Node>& originalNodes
    );
    
    static MetricsResStorage doSpaceMeasure(Graph& graph);
    static void saveRes(const std::string& path, const MetricsResStorage& res);
};