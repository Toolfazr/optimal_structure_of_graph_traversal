#pragma once

#include "AlgorithmStrategy.hpp"

class ListDFS : public AlgorithmStrategy {
public:
    std::unique_ptr<Graph> construction(const Graph& graph, const std::vector<std::string> accessRank) override;
    size_t traversal(const Graph& graph, Index root, std::vector<std::string>& accessRank) override;
    std::unique_ptr<Aggregate> reGraph(const Graph& graph) override;
    std::unique_ptr<Aggregate> rankSeeking(Graph& graph) override;
};