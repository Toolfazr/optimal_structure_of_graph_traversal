// Construction.cpp
#include "Construction.hpp"

#include <algorithm>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

namespace {

struct LabelMapping {
    std::unordered_map<std::string, Index> labelToIndex;
    std::vector<Node> nodes;
    bool valid = true;
};

template <typename GraphT>
LabelMapping buildLabelMapping(const GraphT& g) {
    LabelMapping mapping;
    const int n = static_cast<int>(g.getNodeCount());
    mapping.nodes.resize(static_cast<std::size_t>(n));

    std::unordered_set<std::string> seenLabels;
    for (int i = 0; i < n; ++i) {
        Node node = g.getNode(static_cast<Index>(i));
        if (node.index < 0) {
            mapping.valid = false;
            return mapping;
        }
        if (!seenLabels.insert(node.label).second) {
            mapping.valid = false;
            return mapping;
        }
        mapping.nodes[static_cast<std::size_t>(i)] = node;
        mapping.labelToIndex.emplace(node.label, node.index);
    }
    return mapping;
}

bool buildRankIndex(const std::vector<std::string>& rank,
                    const std::unordered_map<std::string, Index>& labelToIndex,
                    std::vector<Index>& rankIdx) {
    rankIdx.clear();
    rankIdx.reserve(rank.size());

    std::unordered_set<std::string> seenRankLabels;
    for (const auto& label : rank) {
        if (!seenRankLabels.insert(label).second) {
            return false;
        }
        auto it = labelToIndex.find(label);
        if (it == labelToIndex.end()) {
            return false;
        }
        rankIdx.push_back(it->second);
    }
    return true;
}

template <typename GraphT>
void copyGraphNodes(const GraphT& g,
                    const std::vector<Node>& nodes,
                    GraphT& out) {
    out = GraphT();
    out.setLabel(g.getLabel());
    for (const auto& node : nodes) {
        out.addNode(node);
    }
}

template <typename GraphT>
std::vector<std::vector<Index>> buildNeighborOrder(const GraphT& g) {
    const int n = static_cast<int>(g.getNodeCount());
    std::vector<std::vector<Index>> neighbors(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        neighbors[static_cast<std::size_t>(i)] = g.getNeighbors(static_cast<Index>(i));
    }
    return neighbors;
}

template <typename GraphT>
void buildOutEdges(const std::vector<std::vector<Index>>& neighbors,
                   GraphT& out) {
    const int n = static_cast<int>(neighbors.size());
    for (int u = 0; u < n; ++u) {
        for (Index v : neighbors[static_cast<std::size_t>(u)]) {
            out.addEdge(static_cast<Index>(u), v);
        }
    }
}

std::vector<Index> buildOldToNewMapping(const std::vector<Index>& rankIdx) {
    const int n = static_cast<int>(rankIdx.size());
    std::vector<Index> oldToNew(static_cast<std::size_t>(n), -1);
    for (int i = 0; i < n; ++i) {
        oldToNew[static_cast<std::size_t>(rankIdx[static_cast<std::size_t>(i)])] = static_cast<Index>(i);
    }
    return oldToNew;
}

void buildMatrixNodesFromRank(const AdjMatrixGraph& g,
                              const std::vector<std::string>& rank,
                              AdjMatrixGraph& out) {
    out = AdjMatrixGraph();
    out.setLabel(g.getLabel());
    for (int i = 0; i < static_cast<int>(rank.size()); ++i) {
        out.addNode(Node(static_cast<Index>(i), rank[static_cast<std::size_t>(i)]));
    }
}

void buildMatrixEdgesFromNeighbors(const std::vector<std::vector<Index>>& neighbors,
                                   const std::vector<Index>& oldToNew,
                                   AdjMatrixGraph& out) {
    const int n = static_cast<int>(neighbors.size());
    for (int u = 0; u < n; ++u) {
        Index newU = oldToNew[static_cast<std::size_t>(u)];
        for (Index v : neighbors[static_cast<std::size_t>(u)]) {
            Index newV = oldToNew[static_cast<std::size_t>(v)];
            out.addEdge(newU, newV);
        }
    }
}

bool reorderBfs(const std::vector<Index>& rankIdx,
                std::vector<std::vector<Index>>& neighbors) {
    const int n = static_cast<int>(neighbors.size());
    if (n == 0) return true;

    std::vector<uint8_t> visited(static_cast<std::size_t>(n), 0);
    std::queue<Index> qu;

    Index root = rankIdx[0];
    visited[static_cast<std::size_t>(root)] = 1;
    qu.push(root);
    int cur = 1;

    while (!qu.empty()) {
        Index u = qu.front();
        qu.pop();

        std::vector<Index> unvisited;
        for (Index v : neighbors[static_cast<std::size_t>(u)]) {
            if (!visited[static_cast<std::size_t>(v)]) {
                unvisited.push_back(v);
            }
        }

        const int need = static_cast<int>(unvisited.size());
        if (cur + need > n) {
            return false;
        }

        std::vector<Index> expected;
        expected.reserve(static_cast<std::size_t>(need));
        for (int i = 0; i < need; ++i) {
            expected.push_back(rankIdx[static_cast<std::size_t>(cur + i)]);
        }

        std::vector<Index> unvisitedSorted = unvisited;
        std::vector<Index> expectedSorted = expected;
        std::sort(unvisitedSorted.begin(), unvisitedSorted.end());
        std::sort(expectedSorted.begin(), expectedSorted.end());
        if (unvisitedSorted != expectedSorted) {
            return false;
        }

        std::vector<Index> reordered;
        reordered.reserve(neighbors[static_cast<std::size_t>(u)].size());
        for (Index v : expected) {
            reordered.push_back(v);
        }
        for (Index v : neighbors[static_cast<std::size_t>(u)]) {
            if (visited[static_cast<std::size_t>(v)]) {
                reordered.push_back(v);
            }
        }
        neighbors[static_cast<std::size_t>(u)] = std::move(reordered);

        for (Index v : expected) {
            visited[static_cast<std::size_t>(v)] = 1;
            qu.push(v);
        }
        cur += need;
    }

    return cur == n;
}

bool reorderDfs(const std::vector<Index>& rankIdx,
                std::vector<std::vector<Index>>& neighbors) {
    const int n = static_cast<int>(neighbors.size());
    if (n == 0) return true;

    std::vector<int> pos(static_cast<std::size_t>(n), -1);
    for (int i = 0; i < n; ++i) {
        pos[static_cast<std::size_t>(rankIdx[static_cast<std::size_t>(i)])] = i;
    }

    std::vector<uint8_t> visited(static_cast<std::size_t>(n), 0);
    std::stack<Index> st;

    Index root = rankIdx[0];
    visited[static_cast<std::size_t>(root)] = 1;
    st.push(root);
    int cur = 1;

    while (!st.empty()) {
        Index u = st.top();

        std::vector<Index> unvisited;
        for (Index v : neighbors[static_cast<std::size_t>(u)]) {
            if (!visited[static_cast<std::size_t>(v)]) {
                unvisited.push_back(v);
            }
        }

        if (unvisited.empty()) {
            st.pop();
            continue;
        }

        Index target = rankIdx[static_cast<std::size_t>(cur)];
        bool found = false;
        for (Index v : unvisited) {
            if (v == target) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }

        std::vector<Index> remaining;
        remaining.reserve(unvisited.size());
        for (Index v : unvisited) {
            if (v != target) {
                remaining.push_back(v);
            }
        }
        std::sort(remaining.begin(), remaining.end(),
                  [&](Index a, Index b) { return pos[static_cast<std::size_t>(a)] < pos[static_cast<std::size_t>(b)]; });

        std::vector<Index> reordered;
        reordered.reserve(neighbors[static_cast<std::size_t>(u)].size());
        reordered.push_back(target);
        for (Index v : remaining) {
            reordered.push_back(v);
        }
        for (Index v : neighbors[static_cast<std::size_t>(u)]) {
            if (visited[static_cast<std::size_t>(v)]) {
                reordered.push_back(v);
            }
        }
        neighbors[static_cast<std::size_t>(u)] = std::move(reordered);

        Index next = -1;
        for (Index v : neighbors[static_cast<std::size_t>(u)]) {
            if (!visited[static_cast<std::size_t>(v)]) {
                next = v;
                break;
            }
        }
        if (next != target) {
            return false;
        }

        visited[static_cast<std::size_t>(next)] = 1;
        st.push(next);
        ++cur;
    }

    return cur == n;
}

} // namespace

bool Construction::reorderListForBFS(const AdjListGraph& g,
                                     const std::vector<std::string>& rank,
                                     AdjListGraph& out) {
    const int n = static_cast<int>(g.getNodeCount());
    if (static_cast<int>(rank.size()) != n) {
        return false;
    }

    LabelMapping mapping = buildLabelMapping(g);
    if (!mapping.valid) {
        return false;
    }

    std::vector<Index> rankIdx;
    if (!buildRankIndex(rank, mapping.labelToIndex, rankIdx)) {
        return false;
    }

    std::vector<std::vector<Index>> neighbors = buildNeighborOrder(g);
    if (!reorderBfs(rankIdx, neighbors)) {
        return false;
    }

    copyGraphNodes(g, mapping.nodes, out);
    buildOutEdges(neighbors, out);
    return true;
}

bool Construction::reorderListForDFS(const AdjListGraph& g,
                                     const std::vector<std::string>& rank,
                                     AdjListGraph& out) {
    const int n = static_cast<int>(g.getNodeCount());
    if (static_cast<int>(rank.size()) != n) {
        return false;
    }

    LabelMapping mapping = buildLabelMapping(g);
    if (!mapping.valid) {
        return false;
    }

    std::vector<Index> rankIdx;
    if (!buildRankIndex(rank, mapping.labelToIndex, rankIdx)) {
        return false;
    }

    std::vector<std::vector<Index>> neighbors = buildNeighborOrder(g);
    if (!reorderDfs(rankIdx, neighbors)) {
        return false;
    }

    copyGraphNodes(g, mapping.nodes, out);
    buildOutEdges(neighbors, out);
    return true;
}

bool Construction::reorderMatrixForBFS(const AdjMatrixGraph& g,
                                       const std::vector<std::string>& rank,
                                       AdjMatrixGraph& out) {
    const int n = static_cast<int>(g.getNodeCount());
    if (static_cast<int>(rank.size()) != n) {
        return false;
    }

    LabelMapping mapping = buildLabelMapping(g);
    if (!mapping.valid) {
        return false;
    }

    std::vector<Index> rankIdx;
    if (!buildRankIndex(rank, mapping.labelToIndex, rankIdx)) {
        return false;
    }

    std::vector<std::vector<Index>> neighbors = buildNeighborOrder(g);
    if (!reorderBfs(rankIdx, neighbors)) {
        return false;
    }

    std::vector<Index> oldToNew = buildOldToNewMapping(rankIdx);
    buildMatrixNodesFromRank(g, rank, out);
    buildMatrixEdgesFromNeighbors(neighbors, oldToNew, out);
    return true;
}

bool Construction::reorderMatrixForDFS(const AdjMatrixGraph& g,
                                       const std::vector<std::string>& rank,
                                       AdjMatrixGraph& out) {
    const int n = static_cast<int>(g.getNodeCount());
    if (static_cast<int>(rank.size()) != n) {
        return false;
    }

    LabelMapping mapping = buildLabelMapping(g);
    if (!mapping.valid) {
        return false;
    }

    std::vector<Index> rankIdx;
    if (!buildRankIndex(rank, mapping.labelToIndex, rankIdx)) {
        return false;
    }

    std::vector<std::vector<Index>> neighbors = buildNeighborOrder(g);
    if (!reorderDfs(rankIdx, neighbors)) {
        return false;
    }

    std::vector<Index> oldToNew = buildOldToNewMapping(rankIdx);
    buildMatrixNodesFromRank(g, rank, out);
    buildMatrixEdgesFromNeighbors(neighbors, oldToNew, out);
    return true;
}