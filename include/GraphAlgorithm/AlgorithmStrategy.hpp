#pragma once

#include <vector>
#include <string>
#include "Graph.hpp"
#include <memory>
#include "Node.hpp"
#include "Aggregate.hpp"

class AlgorithmStrategy {
public:
    // 接收一张图和给定的访问秩，同构出一张用traversal方法遍历时访问顺序与给定访问秩相同的图
    virtual std::unique_ptr<Graph> construction(const Graph& graph, const std::vector<std::string> accessRank) = 0;
    virtual size_t traversal(const Graph& graph, Index root, std::vector<std::string>& accessRank) = 0;
    virtual std::unique_ptr<Aggregate> reGraph(const Graph& graph) = 0;
    virtual std::unique_ptr<Aggregate> rankSeeking(Graph& graph) = 0;
};