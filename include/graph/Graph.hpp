#pragma once

#include <vector>
#include "Node.hpp"
#include <string>

// 有向图
class Graph {
public:
    virtual ~Graph() = default;

    // 节点操作
    virtual void addNode(const Node& node) = 0;
    virtual size_t getNodeCount() const = 0;
    virtual Node getNode(Index nodeId) const = 0;
    virtual Node getNode (std::string label) const = 0;
    // 边操作
    virtual void addEdge(Index from, Index to) = 0;
    virtual void removeEdge(Index from, Index to) = 0;
    virtual bool hasEdge(Index from, Index to) const = 0;

    // 遍历接口
    virtual std::vector<Index> getNeighbors(Index nodeId) const = 0;

    // 设置图标签
    virtual void setLabel(std::string label) = 0;
    virtual std::string getLabel() const = 0;
};
