#pragma once

#include "Graph.hpp"
#include <vector>

// - order : 节点的访问顺序
// - parent: 遍历树的父节点（root 为 -1；未访问也为 -1）
struct TraversalTrace {
    std::vector<Index> order;
    std::vector<Index> parent;
};

class TraversalAlgo {
public:
    // 兼容原接口：只遍历，不返回轨迹（内部调用 Trace 版本丢弃结果）
    static void bfs(Graph& graph);
    static void dfs(Graph& graph);

    // 新接口：返回遍历轨迹（用于 Metrics 的结构指标测量）
    static TraversalTrace bfsTrace(Graph& graph);
    static TraversalTrace dfsTrace(Graph& graph);
};
