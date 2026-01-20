// Construction.cpp
#include "Construction.hpp"

#include <queue>
#include <stack>
#include <string>
#include <vector>

namespace {

struct IndexMapping {
    std::vector<Index> labelToInternal;
    std::vector<Index> internalToLabel;
    bool valid = true;
};

static IndexMapping buildMapping(const std::vector<Index>& rank) {
    IndexMapping mapping;
    const int n = static_cast<int>(rank.size());
    mapping.labelToInternal.assign(n, -1);
    mapping.internalToLabel = rank;

    for (int i = 0; i < n; ++i) {
        Index labelIndex = rank[static_cast<std::size_t>(i)];
        if (labelIndex < 0 || labelIndex >= n) {
            mapping.valid = false;
            return mapping;
        }
        if (mapping.labelToInternal[labelIndex] != -1) {
            mapping.valid = false;
            return mapping;
        }
        mapping.labelToInternal[labelIndex] = static_cast<Index>(i);
    }
    return mapping;
}

template <typename GraphT>
static void addNodesFromMapping(GraphT& graph, const IndexMapping& mapping) {
    const int n = static_cast<int>(mapping.internalToLabel.size());
    for (int internalIndex = 0; internalIndex < n; ++internalIndex) {
        Index labelIndex = mapping.internalToLabel[static_cast<std::size_t>(internalIndex)];
        graph.addNode(Node(static_cast<Index>(internalIndex), std::to_string(labelIndex)));
    }
}

template <typename GraphT>
static void addUndirectedEdge(GraphT& graph, Index u, Index v) {
    graph.addEdge(u, v);
    graph.addEdge(v, u);
}

template <typename GraphT>
static bool buildBfsEdges(GraphT& graph, const std::vector<Index>& rank, const IndexMapping& mapping) {
    const int n = static_cast<int>(rank.size());
    if (n == 0) return true;

    Index source = mapping.labelToInternal[rank[0]];
    if (source < 0 || source >= n) return false;

    std::queue<Index> qu;
    qu.push(source);

    int cur = 1;
    while (!qu.empty()) {
        Index u = qu.front();
        qu.pop();

        if (cur >= n) continue;

        Index labelChild = rank[static_cast<std::size_t>(cur)];
        Index child = mapping.labelToInternal[labelChild];
        if (child < 0 || child >= n) return false;

        addUndirectedEdge(graph, u, child);
        qu.push(child);
        ++cur;
    }
    return cur == n;
}

template <typename GraphT>
static bool buildDfsEdges(GraphT& graph, const std::vector<Index>& rank, const IndexMapping& mapping) {
    const int n = static_cast<int>(rank.size());
    if (n == 0) return true;

    Index source = mapping.labelToInternal[rank[0]];
    if (source < 0 || source >= n) return false;

    std::stack<Index> st;
    std::vector<char> visited(n, false);
    st.push(source);
    visited[source] = true;

    int cur = 1;
    while (cur < n) {
        Index labelChild = rank[static_cast<std::size_t>(cur)];
        Index child = mapping.labelToInternal[labelChild];
        if (child < 0 || child >= n) return false;
        if (visited[child]) return false;

        Index parent = st.top();
        addUndirectedEdge(graph, parent, child);
        st.push(child);
        visited[child] = true;
        ++cur;
    }
    return true;
}

template <typename GraphT>
static GraphT buildBfsGraph(const std::vector<Index>& rank) {
    GraphT graph;
    IndexMapping mapping = buildMapping(rank);
    if (!mapping.valid) return graph;

    addNodesFromMapping(graph, mapping);
    if (!buildBfsEdges(graph, rank, mapping)) {
        return GraphT();
    }
    return graph;
}

template <typename GraphT>
static GraphT buildDfsGraph(const std::vector<Index>& rank) {
    GraphT graph;
    IndexMapping mapping = buildMapping(rank);
    if (!mapping.valid) return graph;

    addNodesFromMapping(graph, mapping);
    if (!buildDfsEdges(graph, rank, mapping)) {
        return GraphT();
    }
    return graph;
}

} // namespace

AdjListGraph Construction::getListForBFS(const std::vector<Index>& rank) {
    return buildBfsGraph<AdjListGraph>(rank);
}

AdjMatrixGraph Construction::getMatrixForBFS(const std::vector<Index>& rank) {
    return buildBfsGraph<AdjMatrixGraph>(rank);
}

AdjListGraph Construction::getListForDFS(const std::vector<Index>& rank) {
    return buildDfsGraph<AdjListGraph>(rank);
}

AdjMatrixGraph Construction::getMatrixForDFS(const std::vector<Index>& rank) {
    return buildDfsGraph<AdjMatrixGraph>(rank);
}
