#pragma once

#include "Graph.hpp"
#include "Node.hpp"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>

class AdjMatrixGraph: public Graph {
public:
    AdjMatrixGraph() = default;

    // 节点操作
    void addNode(const Node& node) override;
    size_t getNodeCount() const override;
    Node getNode(Index nodeId) const override;
    Node getNode(std::string label) const override;
    // 边操作
    void addEdge(Index from, Index to) override;
    void removeEdge(Index from, Index to) override;
    bool hasEdge(Index from, Index to) const override;

    // 遍历接口
    std::vector<Index> getNeighbors(Index nodeId) const override;

    //设置图标签
    void setLabel(std::string label) override;
    std::string getLabel() const override;
    void toCsv(const std::string& path) const override;
private:
    std::vector<std::vector<Index>> adjMatrix;
    std::unordered_set<Node, NodeHash> nodes;    
    std::string label;
    std::unordered_map<std::string, Index> labelToIndex;
};