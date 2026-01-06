// BestSpaceConstruction.cpp
#include "BestSpaceConstruction.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <queue>
#include <stack>
#include <utility>
#include <vector>

// ======================================================
// New construction algorithm (root-free optimization):
// 1) Choose a "best" start vertex r* (since perm may change which old node maps to ROOT=0)
//    - Candidate roots: R lowest-degree nodes
//    - Score(c) = max( BFS_peak(start=c), DFS_peak(start=c) )
// 2) Build BFS-tree order from r* using:
//    parent-layer: degree-zigzag
//    child order : low-degree first
//    layer output: round-robin across parents
// 3) Map visitOrder[i] -> newId = i, thus newId[r*]=0
// 4) Relabel graph by newId[old]=new
// ======================================================

namespace {

// -------- degree helper --------
static inline int degOf(const Graph& g, Index u) {
    return static_cast<int>(g.getNeighbors(u).size());
}

// -------- peak measurement from arbitrary start --------
static std::size_t bfsPeakFrom(const Graph& g, Index start) {
    const int n = static_cast<int>(g.getNodeCount());
    if (n <= 0) return 0;
    if (start < 0 || start >= n) return 0;

    std::vector<char> vis(n, false);
    std::queue<Index> q;
    q.push(start);
    vis[start] = true;

    std::size_t peak = q.size();

    while (!q.empty()) {
        peak = std::max<std::size_t>(peak, q.size());
        Index u = q.front();
        q.pop();

        for (Index v : g.getNeighbors(u)) {
            if (v < 0 || v >= n) continue;
            if (!vis[v]) {
                vis[v] = true;
                q.push(v);
            }
        }
    }
    peak = std::max<std::size_t>(peak, q.size());
    return peak;
}

static std::size_t dfsPeakFrom(const Graph& g, Index start) {
    const int n = static_cast<int>(g.getNodeCount());
    if (n <= 0) return 0;
    if (start < 0 || start >= n) return 0;

    // IMPORTANT: keep this consistent with TraversalAlgo::dfs / Metrics::dfsMaxStackFromRoot
    // (visited-on-push), otherwise chooseBestRoot() may be misled by an inflated peak.
    std::vector<char> vis(n, false);
    std::stack<Index> st;

    st.push(start);
    vis[start] = true;
    std::size_t peak = st.size();

    while (!st.empty()) {
        peak = std::max<std::size_t>(peak, st.size());
        Index u = st.top();
        st.pop();
        if (u < 0 || u >= n) continue;

        // Follow graph.getNeighbors(u) order (AdjMatrix is naturally increasing;
        // AdjList is later normalized to increasing in relabelAdjList()).
        for (Index v : g.getNeighbors(u)) {
            if (v < 0 || v >= n) continue;
            if (!vis[v]) {
                st.push(v);
                vis[v] = true;
            }
        }
    }
    return peak;
}

// -------- choose best root (root-free) --------
static Index chooseBestRoot(const Graph& g) {
    const int n = static_cast<int>(g.getNodeCount());
    if (n <= 0) return ROOT;

    // R: number of candidate roots (small, stable)
    // Use min(16, n) by default; can be tuned if needed.
    int R = std::min(4, n);

    std::vector<Index> nodes(n);
    for (int i = 0; i < n; ++i) nodes[i] = static_cast<Index>(i);

    // pick R lowest-degree nodes
    std::nth_element(nodes.begin(), nodes.begin() + (R - 1), nodes.end(),
                     [&](Index a, Index b) {
                         int da = degOf(g, a), db = degOf(g, b);
                         if (da != db) return da < db;
                         return a < b;
                     });
    nodes.resize(R);

    // evaluate candidates by score = max(BFS_peak, DFS_peak)
    Index best = nodes[0];
    std::size_t bestScore = std::numeric_limits<std::size_t>::max();
    std::size_t bestB = 0, bestD = 0;

    for (Index c : nodes) {
        std::size_t b = bfsPeakFrom(g, c);
        std::size_t d = dfsPeakFrom(g, c);
        std::size_t sc = std::max(b, d);

        // tie-break: smaller BFS peak, then smaller DFS peak, then smaller id
        if (sc < bestScore ||
            (sc == bestScore && (b < bestB ||
                                 (b == bestB && (d < bestD ||
                                                 (d == bestD && c < best)))))) {
            best = c;
            bestScore = sc;
            bestB = b;
            bestD = d;
        }
    }
    return best;
}

// -------- degree-desc + zigzag --------
static std::vector<Index> zigzagByDegree(std::vector<Index> nodes, const Graph& g) {
    std::sort(nodes.begin(), nodes.end(),
              [&](Index a, Index b) {
                  int da = degOf(g, a), db = degOf(g, b);
                  if (da != db) return da > db;
                  return a < b;
              });

    std::vector<Index> out;
    out.reserve(nodes.size());
    int i = 0, j = static_cast<int>(nodes.size()) - 1;
    while (i <= j) {
        out.push_back(nodes[i++]);
        if (i <= j) out.push_back(nodes[j--]);
    }
    return out;
}

static std::vector<Index> buildVisitOrder(const Graph& g, Index root) {
    const int n = static_cast<int>(g.getNodeCount());
    std::vector<Index> order;
    order.reserve(n);
    if (n <= 0) return order;
    if (root < 0 || root >= n) return order;

    std::vector<int> level(n, -1);
    std::vector<Index> parent(n, -1);
    std::vector<std::vector<Index>> layers;

    std::queue<Index> q;
    q.push(root);
    level[root] = 0;

    while (!q.empty()) {
        Index u = q.front();
        q.pop();

        int L = level[u];
        if (static_cast<int>(layers.size()) <= L) layers.resize(L + 1);
        layers[L].push_back(u);

        for (Index v : g.getNeighbors(u)) {
            if (v < 0 || v >= n) continue;
            if (level[v] == -1) {
                level[v] = L + 1;
                parent[v] = u;
                q.push(v);
            }
        }
    }

    std::vector<char> used(n, false);
    used[root] = true;
    order.push_back(root);

    for (int L = 0; L + 1 < static_cast<int>(layers.size()); ++L) {
        // parents: degree-desc + zigzag
        auto parents = zigzagByDegree(layers[L], g);

        // collect children for each parent (BFS-tree edges only), then sort children by low degree first
        std::vector<std::vector<Index>> children(parents.size());

        for (int i = 0; i < static_cast<int>(parents.size()); ++i) {
            Index p = parents[i];
            for (Index v : g.getNeighbors(p)) {
                if (v < 0 || v >= n) continue;
                if (level[v] == L + 1 && parent[v] == p) {
                    children[i].push_back(v);
                }
            }
            std::sort(children[i].begin(), children[i].end(),
                      [&](Index a, Index b) {
                          int da = degOf(g, a), db = degOf(g, b);
                          if (da != db) return da < db;
                          return a < b;
                      });
        }

        // parent-by-parent contiguous emission (align with BS(span))
        for (int i = 0; i < static_cast<int>(parents.size()); ++i) {
            for (Index v : children[i]) {
                if (!used[v]) {
                    used[v] = true;
                    order.push_back(v);
                }
            }
        }
    }

    // disconnected fallback
    for (Index v = 0; v < n; ++v) {
        if (!used[v]) order.push_back(v);
    }
    return order;
}

// -------- relabel helpers (newId[old]=new) --------
template <typename GraphT>
static void extractGraphInfoPublic(
    const GraphT& graph,
    std::vector<std::vector<Index>>& originalAdj,
    std::vector<Node>& originalNodes
) {
    originalAdj.clear();
    originalNodes.clear();

    int size = static_cast<int>(graph.getNodeCount());
    originalAdj.resize(size);
    originalNodes.resize(size, Node(-1, "none"));

    for (int id = 0; id < size; ++id) {
        originalAdj[static_cast<Index>(id)] = graph.getNeighbors(id);
        originalNodes[id] = graph.getNode(id);
    }
}

static AdjListGraph relabelAdjList(const AdjListGraph& graph, const std::vector<Index>& newId) {
    std::vector<std::vector<Index>> originalAdj;
    std::vector<Node> originalNodes;
    extractGraphInfoPublic(graph, originalAdj, originalNodes);

    const int n = static_cast<int>(graph.getNodeCount());

    std::vector<Index> inv(n, -1); // inv是访问秩的逆
    for (int old = 0; old < n; ++old) {
        Index nu = newId[old];
        if (nu >= 0 && nu < n) inv[nu] = static_cast<Index>(old);
    }

    AdjListGraph out;
    for (int newIdx = 0; newIdx < n; ++newIdx) {
        Index old = inv[newIdx];
        if (old < 0 || old >= n) old = static_cast<Index>(newIdx);
        out.addNode(Node(static_cast<Index>(newIdx), originalNodes[old].label));
    }

    std::vector<std::vector<Index>> newAdj(n);
    for (int oldU = 0; oldU < n; ++oldU) {
        Index newU = newId[oldU];
        if (newU < 0 || newU >= n) continue;
        for (Index oldV : originalAdj[oldU]) {
            if (oldV < 0 || oldV >= n) continue;
            Index newV = newId[oldV];
            if (newV < 0 || newV >= n) continue;
            newAdj[newU].push_back(newV);
        }
    }
    for (int u = 0; u < n; ++u) {
        auto& vec = newAdj[u];
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
        for (Index v : vec) {
            out.addEdge(static_cast<Index>(u), v);
        }
    }
    return out;
}

static AdjMatrixGraph relabelAdjMatrix(const AdjMatrixGraph& graph, const std::vector<Index>& newId) {
    std::vector<std::vector<Index>> originalAdj;
    std::vector<Node> originalNodes;
    extractGraphInfoPublic(graph, originalAdj, originalNodes);

    const int n = static_cast<int>(graph.getNodeCount());

    std::vector<Index> inv(n, -1);
    for (int old = 0; old < n; ++old) {
        Index nu = newId[old];
        if (nu >= 0 && nu < n) inv[nu] = static_cast<Index>(old);
    }

    AdjMatrixGraph out;
    for (int newIdx = 0; newIdx < n; ++newIdx) {
        Index old = inv[newIdx];
        if (old < 0 || old >= n) old = static_cast<Index>(newIdx);
        out.addNode(Node(static_cast<Index>(newIdx), originalNodes[old].label));
    }

    std::vector<std::vector<Index>> newAdj(n);
    for (int oldU = 0; oldU < n; ++oldU) {
        Index newU = newId[oldU];
        if (newU < 0 || newU >= n) continue;
        for (Index oldV : originalAdj[oldU]) {
            if (oldV < 0 || oldV >= n) continue;
            Index newV = newId[oldV];
            if (newV < 0 || newV >= n) continue;
            newAdj[newU].push_back(newV);
        }
    }
    for (int u = 0; u < n; ++u) {
        auto& vec = newAdj[u];
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
        for (Index v : vec) {
            out.addEdge(static_cast<Index>(u), v);
        }
    }
    return out;
}

} // namespace

// ======================================================
// Public API
// ======================================================

AdjListGraph BestSpaceConstruction::getBestSpaceConstruction(AdjListGraph& graph) {
    Index r = chooseBestRoot(graph);

    auto order = buildVisitOrder(graph, r); // order是最优存储结构对应的访问秩的逆

    const int n = static_cast<int>(graph.getNodeCount());
    std::vector<Index> newId(n, -1); // newId 是访问秩
    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
        newId[order[i]] = static_cast<Index>(i);
    }
    Index next = static_cast<Index>(order.size()); // 防御性，但目前研究的一直是联通图
    for (int old = 0; old < n; ++old) {
        if (newId[old] == -1) newId[old] = next++;
    }

    return relabelAdjList(graph, newId);
}

AdjMatrixGraph BestSpaceConstruction::getBestSpaceConstruction(AdjMatrixGraph& graph) {
    Index r = chooseBestRoot(graph);

    auto order = buildVisitOrder(graph, r);

    const int n = static_cast<int>(graph.getNodeCount());
    std::vector<Index> newId(n, -1);
    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
        newId[order[i]] = static_cast<Index>(i);
    }
    Index next = static_cast<Index>(order.size());
    for (int old = 0; old < n; ++old) {
        if (newId[old] == -1) newId[old] = next++;
    }

    return relabelAdjMatrix(graph, newId);
}
