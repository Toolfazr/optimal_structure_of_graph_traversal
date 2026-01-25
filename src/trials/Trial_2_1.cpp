// 测量8/9节点完全二叉树的BFS/DFS遍历空间占用分布
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <limits>
#include <cstddef>
#include "Metrics.hpp"
#include "GraphGen.hpp"

struct SpaceStats {
    std::size_t count = 0;
    std::size_t minElems = std::numeric_limits<std::size_t>::max();
    std::size_t maxElems = 0;
    double sumElems = 0.0;
    std::size_t minIndex = 0;
    std::size_t maxIndex = 0;

    // hist[elems] = how many permutations produced this peak
    std::vector<std::size_t> hist;

    void add(std::size_t elems, std::size_t permIndex) {
        ++count;
        sumElems += static_cast<double>(elems);
        if (elems < minElems) { minElems = elems; minIndex = permIndex; }
        if (elems > maxElems) { maxElems = elems; maxIndex = permIndex; }

        if (hist.size() <= elems) hist.resize(elems + 1, 0);
        hist[elems] += 1;
    }

    double avgElems() const {
        return (count == 0) ? 0.0 : (sumElems / static_cast<double>(count));
    }
};

template <typename G>
static void extractGraphInfo(const G& graph,
                             std::vector<std::vector<Index>>& originalAdj,
                             std::vector<Node>& originalNodes) {
    originalAdj.clear();
    originalNodes.clear();

    int n = graph.getNodeCount();
    originalAdj.resize(n);
    originalNodes.resize(n, Node(-1, "none"));

    for (int id = 0; id < n; ++id) {
        originalAdj[static_cast<Index>(id)] = graph.getNeighbors(id);
        originalNodes[id] = graph.getNode(id);
    }
}

template <typename G>
static void runAllPermutationsSpace(const G& baseGraph,
                                    SpaceStats& bfsStats,
                                    SpaceStats& dfsStats) {
    const int n = baseGraph.getNodeCount();
    if (n <= 0) return;

    std::vector<std::vector<Index>> originalAdj;
    std::vector<Node> originalNodes;
    extractGraphInfo(baseGraph, originalAdj, originalNodes);

    std::vector<Index> perm(n);
    std::iota(perm.begin(), perm.end(), 0);

    std::size_t permIndex = 0;
    do {
        // invrs[newId] = oldId
        std::vector<Index> invrs(n);
        for (int oldId = 0; oldId < n; ++oldId) {
            int newId = perm[oldId];
            invrs[newId] = static_cast<Index>(oldId);
        }

        // rebuild permuted graph
        G g;
        for (int newId = 0; newId < n; ++newId) {
            const Index oldId = invrs[newId];
            g.addNode(Node(static_cast<Index>(newId), originalNodes[oldId].label));
        }

        // Copy directed adjacency as-is.
        for (int newU = 0; newU < n; ++newU) {
            const Index oldU = invrs[newU];
            for (Index oldV : originalAdj[oldU]) {
                const Index newV = perm[oldV];
                g.addEdge(static_cast<Index>(newU), newV);
            }
        }

        // measure peaks
        const std::size_t bfsPeak = Metrics::measureBFSMaxQueue(g).bestPeak;
        const std::size_t dfsPeak = Metrics::measureDFSMaxStack(g).bestPeak;

        bfsStats.add(bfsPeak, permIndex);
        dfsStats.add(dfsPeak, permIndex);

        ++permIndex;
    } while (std::next_permutation(perm.begin(), perm.end()));
}

static void printHist(const SpaceStats& s) {
    // Print only bins that appear.
    std::cout << "  distribution (peak_elems : count, ratio)\n";
    for (std::size_t elems = 0; elems < s.hist.size(); ++elems) {
        std::size_t c = s.hist[elems];
        if (c == 0) continue;
        double ratio = (s.count == 0) ? 0.0 : (static_cast<double>(c) / static_cast<double>(s.count));
        std::cout << "    " << elems << " : " << c << ", " << ratio << "\n";
    }
}

static void printStats(const char* title, const SpaceStats& s) {
    const std::size_t bytesPerElem = sizeof(Index);

    std::cout << title << "\n";
    std::cout << "  count = " << s.count << "\n";
    std::cout << "  min   = " << s.minElems << " elems (perm index = " << s.minIndex
              << "), bytes ~= " << (s.minElems * bytesPerElem) << "\n";
    std::cout << "  max   = " << s.maxElems << " elems (perm index = " << s.maxIndex
              << "), bytes ~= " << (s.maxElems * bytesPerElem) << "\n";
    std::cout << "  avg   = " << s.avgElems() << " elems, bytes ~= " << (s.avgElems() * bytesPerElem) << "\n";
    printHist(s);
    std::cout << "\n";
}

template <typename G>
static void runOneCase(const char* caseName, const G& baseGraph) {
    SpaceStats bfs, dfs;
    runAllPermutationsSpace(baseGraph, bfs, dfs);

    std::cout << caseName << "\n";
    printStats("BFS (max queue size):", bfs);
    printStats("DFS (max stack size):", dfs);
}

int main() {
    std::cout << "===== BinaryTree-only: n=8 and n=9 (full permutations) =====\n\n";

    // --- n = 8 ---
    {
        const int n = 8;
        std::cout << "----- n = 8 (8! = 40320) -----\n\n";

        std::cout << "[AdjList]\n\n";
        runOneCase("BinaryTree(8) [AdjList]", GraphGen::makeBinaryTreeAdjList(n));

        std::cout << "[AdjMatrix]\n\n";
        runOneCase("BinaryTree(8) [AdjMatrix]", GraphGen::makeBinaryTreeAdjMatrix(n));
    }

    // --- n = 9 ---
    {
        const int n = 9;
        std::cout << "----- n = 9 (9! = 362880) -----\n\n";

        std::cout << "[AdjList]\n\n";
        runOneCase("BinaryTree(9) [AdjList]", GraphGen::makeBinaryTreeAdjList(n));

        std::cout << "[AdjMatrix]\n\n";
        runOneCase("BinaryTree(9) [AdjMatrix]", GraphGen::makeBinaryTreeAdjMatrix(n));
    }

    return 0;
}
