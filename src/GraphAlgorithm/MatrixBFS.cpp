// MatrixBFS.cpp
#include "MatrixBFS.hpp"
#include <string>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>
#include <cstdint>
#include "AdjMatrixGraph.hpp"
#include <memory>

using namespace std;

std::unique_ptr<Graph> MatrixBFS::construction(const Graph& graph,
                                              const std::vector<std::string> accessRank)
{
    // 合法检查
    if (graph.getNodeCount() == 0) return nullptr;
    if (accessRank.size() != graph.getNodeCount()) return nullptr;

    unordered_set<string> labelsMapped;
    vector<Index> rankToIndex;  // rankToIndex[newPos] = oldIndex
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

    // BFS 合法性检查（基于“存在某种邻接顺序可实现”）：复刻 reorderBfs 的集合检查
    vector<uint8_t> visited(static_cast<size_t>(n), 0);
    queue<Index> qu;

    Index root = rankToIndex[0];
    visited[static_cast<size_t>(root)] = 1;
    qu.push(root);
    int pos = 1;

    while (!qu.empty()) {
        Index u = qu.front();
        qu.pop();

        vector<Index> unvisited;
        for (Index v : graph.getNeighbors(u)) {
            if (!visited[static_cast<size_t>(v)]) unvisited.push_back(v);
        }

        const int need = static_cast<int>(unvisited.size());
        if (pos + need > n) return nullptr;

        vector<Index> expected;
        expected.reserve(static_cast<size_t>(need));
        for (int i = 0; i < need; ++i) {
            expected.push_back(rankToIndex[static_cast<size_t>(pos + i)]);
        }

        auto unvisitedSorted = unvisited;
        auto expectedSorted = expected;
        sort(unvisitedSorted.begin(), unvisitedSorted.end());
        sort(expectedSorted.begin(), expectedSorted.end());
        if (unvisitedSorted != expectedSorted) return nullptr;

        for (Index v : expected) {
            visited[static_cast<size_t>(v)] = 1;
            qu.push(v);
        }
        pos += need;
    }

    if (pos != n) return nullptr;

    // 开始建图（矩阵无法“按节点重排邻接顺序”，因此采用重编号：index=rank位置）
    unique_ptr<AdjMatrixGraph> newGraph = make_unique<AdjMatrixGraph>();
    newGraph->setLabel(graph.getLabel());

    for (int i = 0; i < n; ++i) {
        newGraph->addNode(Node(static_cast<Index>(i), accessRank[static_cast<size_t>(i)]));
    }

    // 加边：把旧端点翻译为新端点
    for (int oldU = 0; oldU < n; ++oldU) {
        Index newU = oldToNew[static_cast<size_t>(oldU)];
        for (Index oldV : graph.getNeighbors(static_cast<Index>(oldU))) {
            Index newV = oldToNew[static_cast<size_t>(oldV)];
            newGraph->addEdge(newU, newV);
            newGraph->addEdge(newV, newU); // 无向
        }
    }

    return newGraph;
}
