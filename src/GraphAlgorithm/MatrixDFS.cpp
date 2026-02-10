// MatrixDFS.cpp
#include "MatrixDFS.hpp"
#include <string>
#include <unordered_set>
#include <vector>
#include <stack>
#include <algorithm>
#include <cstdint>
#include "AdjMatrixGraph.hpp"
#include <memory>

using namespace std;

std::unique_ptr<Graph> MatrixDFS::construction(const Graph& graph,
                                              const std::vector<std::string> accessRank)
{
    // 合法检查
    if (graph.getNodeCount() == 0) return nullptr;
    if (accessRank.size() != graph.getNodeCount()) return nullptr;

    unordered_set<string> labelsMapped;
    vector<Index> rankToIndex; // rankToIndex[newPos] = oldIndex
    rankToIndex.reserve(accessRank.size());

    for (auto& label : accessRank) {
        if (!labelsMapped.insert(label).second) return nullptr;
        Index index = graph.getNode(label).index;
        rankToIndex.push_back(index);
    }

    const int n = static_cast<int>(graph.getNodeCount());

    // oldToNew[oldIndex] = newIndex(=rank position)
    vector<Index> oldToNew(static_cast<size_t>(n), -1);
    for (int i = 0; i < n; ++i) {
        Index oldIdx = rankToIndex[static_cast<size_t>(i)];
        if (oldIdx < 0 || oldIdx >= n) return nullptr;
        if (oldToNew[static_cast<size_t>(oldIdx)] != -1) return nullptr;
        oldToNew[static_cast<size_t>(oldIdx)] = static_cast<Index>(i);
    }

    // DFS 合法性检查（标准 Path-DFS preorder 可实现性）
    vector<uint8_t> visited(static_cast<size_t>(n), 0);
    stack<Index> st;

    auto hasUnvisitedNeighbor = [&](Index u) -> bool {
        for (Index v : graph.getNeighbors(u)) {
            if (!visited[static_cast<size_t>(v)]) return true;
        }
        return false;
    };

    auto isUnvisitedNeighbor = [&](Index u, Index target) -> bool {
        if (visited[static_cast<size_t>(target)]) return false;
        for (Index v : graph.getNeighbors(u)) {
            if (v == target) return true;
        }
        return false;
    };

    Index root = rankToIndex[0];
    visited[static_cast<size_t>(root)] = 1;
    st.push(root);

    for (int cur = 1; cur < n; ++cur) {
        Index target = rankToIndex[static_cast<size_t>(cur)];

        while (!st.empty() && !isUnvisitedNeighbor(st.top(), target)) {
            Index u = st.top();
            if (hasUnvisitedNeighbor(u)) return nullptr;
            st.pop();
        }
        if (st.empty()) return nullptr;

        Index parent = st.top();
        if (!isUnvisitedNeighbor(parent, target)) return nullptr;

        visited[static_cast<size_t>(target)] = 1;
        st.push(target);
    }

    for (int i = 0; i < n; ++i) {
        if (!visited[static_cast<size_t>(i)]) return nullptr;
    }

    // 开始建图（矩阵采用重编号：index=rank位置）
    // 邻接矩阵的遍历优先级唯一由Index确定,将index置为rank即可
    unique_ptr<AdjMatrixGraph> newGraph = make_unique<AdjMatrixGraph>();
    newGraph->setLabel(graph.getLabel());

    for (int i = 0; i < n; ++i) {
        newGraph->addNode(Node(static_cast<Index>(i), accessRank[static_cast<size_t>(i)]));
    }

    // 加边：旧端点 -> 新端点
    for (int oldU = 0; oldU < n; ++oldU) {
        Index newU = oldToNew[static_cast<size_t>(oldU)];
        for (Index oldV : graph.getNeighbors(static_cast<Index>(oldU))) {
            Index newV = oldToNew[static_cast<size_t>(oldV)];
            newGraph->addEdge(newU, newV);
            newGraph->addEdge(newV, newU);
        }
    }

    return newGraph;
}
