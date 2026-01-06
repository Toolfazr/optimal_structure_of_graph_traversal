#include "TraversalAlgo.hpp"

#include <queue>
#include <stack>
#include <unordered_set>

#include "Constants.hpp"

TraversalTrace TraversalAlgo::bfsTrace(Graph& graph) {
    TraversalTrace t;
    const int n = static_cast<int>(graph.getNodeCount());
    if (n <= 0) return t;

    t.parent.assign(n, -1);

    std::queue<Index> qu;
    std::unordered_set<Index> visited;

    qu.push(ROOT);
    visited.insert(ROOT);
    t.parent[static_cast<std::size_t>(ROOT)] = -1;

    while (!qu.empty()) {
        Index cur = qu.front();
        qu.pop();

        t.order.push_back(cur);

        const std::vector<Index> neighbors = graph.getNeighbors(cur);
        for (Index adj : neighbors) {
            if (visited.insert(adj).second) { // first time discovered
                t.parent[static_cast<std::size_t>(adj)] = cur;
                qu.push(adj);
            }
        }
    }
    return t;
}

TraversalTrace TraversalAlgo::dfsTrace(Graph& graph) {
    TraversalTrace t;
    const int n = static_cast<int>(graph.getNodeCount());
    if (n <= 0) return t;

    t.parent.assign(n, -1);

    std::stack<Index> st;
    std::unordered_set<Index> visited;

    st.push(ROOT);
    visited.insert(ROOT);
    t.parent[static_cast<std::size_t>(ROOT)] = -1;

    while (!st.empty()) {
        Index cur = st.top();
        st.pop();

        t.order.push_back(cur);

        const std::vector<Index> neighbors = graph.getNeighbors(cur);
        for (Index adj : neighbors) {
            if (visited.insert(adj).second) { // first time discovered
                t.parent[static_cast<std::size_t>(adj)] = cur;
                st.push(adj);
            }
        }
    }
    return t;
}

void TraversalAlgo::bfs(Graph& graph) {
    (void)TraversalAlgo::bfsTrace(graph);
}

void TraversalAlgo::dfs(Graph& graph) {
    (void)TraversalAlgo::dfsTrace(graph);
}
