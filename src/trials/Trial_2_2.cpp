// 测量8/9节点完全二叉树的BFS/DFS遍历时间占用分布，分析同一Case下遍历时间的差异
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <limits>
#include <chrono>
#include <cmath>
#include <cstdlib>

#include "TraversalAlgo.hpp"
#include "GraphGen.hpp"

// Trial_2_2.cpp (Time measurement, BinaryTree, full permutations)
// Added: Trial layer (repeat full-permutation measurement T times, then summarize across trials)

struct TimeStats {
    std::size_t count = 0;
    double minNs = std::numeric_limits<double>::max();
    double maxNs = 0.0;
    double sumNs = 0.0;
    std::size_t minIndex = 0;
    std::size_t maxIndex = 0;

    void add(double ns, std::size_t idx) {
        ++count;
        sumNs += ns;
        if (ns < minNs) { minNs = ns; minIndex = idx; }
        if (ns > maxNs) { maxNs = ns; maxIndex = idx; }
    }

    double avgNs() const {
        return (count == 0) ? 0.0 : sumNs / static_cast<double>(count);
    }
};

template <typename G>
static void extractGraphInfo(const G& graph,
                             std::vector<std::vector<Index>>& adj,
                             std::vector<Node>& nodes) {
    adj.clear();
    nodes.clear();

    int n = graph.getNodeCount();
    adj.resize(n);
    nodes.reserve(n);

    for (int i = 0; i < n; ++i) {
        adj[i] = graph.getNeighbors(i);
        const Node& nd = graph.getNode(i);
        nodes.emplace_back(nd.index, nd.label);
    }
}

template <typename G, typename AlgoFunc>
static void runAllPermutationsTime(const G& baseGraph,
                                   AlgoFunc algo,
                                   TimeStats& stats) {
    using clock = std::chrono::steady_clock;

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
            invrs[perm[oldId]] = static_cast<Index>(oldId);
        }

        // rebuild permuted graph
        G g;
        for (int newId = 0; newId < n; ++newId) {
            Index oldId = invrs[newId];
            g.addNode(Node(static_cast<Index>(newId), originalNodes[oldId].label));
        }

        // Copy directed adjacency as-is.
        for (int newU = 0; newU < n; ++newU) {
            Index oldU = invrs[newU];
            for (Index oldV : originalAdj[oldU]) {
                g.addEdge(static_cast<Index>(newU), perm[oldV]);
            }
        }

        auto t1 = clock::now();
        algo(g);
        auto t2 = clock::now();

        double ns = static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()
        );
        stats.add(ns, permIndex);

        ++permIndex;
    } while (std::next_permutation(perm.begin(), perm.end()));
}

static double meanOf(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    double s = std::accumulate(v.begin(), v.end(), 0.0);
    return s / static_cast<double>(v.size());
}

// sample std (N-1). If N<2, return 0.
static double stddevOf(const std::vector<double>& v) {
    const std::size_t n = v.size();
    if (n < 2) return 0.0;
    double m = meanOf(v);
    double acc = 0.0;
    for (double x : v) {
        double d = x - m;
        acc += d * d;
    }
    return std::sqrt(acc / static_cast<double>(n - 1));
}

static void printAcrossTrialsStats(const char* title,
                                   const std::vector<TimeStats>& trials) {
    std::vector<double> avgs;
    avgs.reserve(trials.size());

    double globalMin = std::numeric_limits<double>::max();
    double globalMax = 0.0;

    // Track which trial produced extreme permutation times (optional)
    std::size_t minTrial = 0, maxTrial = 0;
    std::size_t minPermIndex = 0, maxPermIndex = 0;

    for (std::size_t t = 0; t < trials.size(); ++t) {
        avgs.push_back(trials[t].avgNs());

        if (trials[t].minNs < globalMin) {
            globalMin = trials[t].minNs;
            minTrial = t;
            minPermIndex = trials[t].minIndex;
        }
        if (trials[t].maxNs > globalMax) {
            globalMax = trials[t].maxNs;
            maxTrial = t;
            maxPermIndex = trials[t].maxIndex;
        }
    }

    double m = meanOf(avgs);
    double sd = stddevOf(avgs);

    std::cout << title << "\n";
    std::cout << "  trials = " << trials.size() << "\n";
    std::cout << "  avg(mean) = " << m << " ns\n";
    std::cout << "  avg(std)  = " << sd << " ns\n";
    std::cout << "  min(overall) = " << globalMin
              << " ns (trial=" << minTrial << ", perm index=" << minPermIndex << ")\n";
    std::cout << "  max(overall) = " << globalMax
              << " ns (trial=" << maxTrial << ", perm index=" << maxPermIndex << ")\n\n";
}

template <typename G>
static void runOneCaseTrials(const char* caseName,
                             const G& baseGraph,
                             int trialsCount) {
    std::vector<TimeStats> bfsTrials;
    std::vector<TimeStats> dfsTrials;
    bfsTrials.reserve(trialsCount);
    dfsTrials.reserve(trialsCount);

    for (int t = 0; t < trialsCount; ++t) {
        TimeStats bfsStats, dfsStats;

        runAllPermutationsTime(
            baseGraph,
            [](Graph& g) { TraversalAlgo::bfs(g); },
            bfsStats
        );

        runAllPermutationsTime(
            baseGraph,
            [](Graph& g) { TraversalAlgo::dfs(g); },
            dfsStats
        );

        bfsTrials.push_back(bfsStats);
        dfsTrials.push_back(dfsStats);

        // Progress line (optional; comment out if you want cleaner output)
        std::cout << "  [trial " << t << "] "
                  << "BFS avg=" << bfsStats.avgNs() << " ns, "
                  << "DFS avg=" << dfsStats.avgNs() << " ns\n";
    }
    std::cout << "\n";

    std::cout << caseName << "\n";
    printAcrossTrialsStats("BFS time (across trials):", bfsTrials);
    printAcrossTrialsStats("DFS time (across trials):", dfsTrials);
}

int main(int argc, char** argv) {
    int trialsCount = 10; // default
    if (argc >= 2) {
        int x = std::atoi(argv[1]);
        if (x > 0) trialsCount = x;
    }

    std::cout << "===== Trial_2.2: Time measurement (BinaryTree, full permutations) =====\n";
    std::cout << "Trials per case = " << trialsCount << "\n\n";

    // n = 8
    {
        const int n = 8;
        std::cout << "----- n = 8 (8! = 40320) -----\n\n";

        std::cout << "[AdjList]\n\n";
        runOneCaseTrials("BinaryTree(8) [AdjList]", GraphGen::makeBinaryTreeAdjList(n), trialsCount);

        std::cout << "[AdjMatrix]\n\n";
        runOneCaseTrials("BinaryTree(8) [AdjMatrix]", GraphGen::makeBinaryTreeAdjMatrix(n), trialsCount);
    }

    // n = 9
    {
        const int n = 9;
        std::cout << "----- n = 9 (9! = 362880) -----\n\n";

        std::cout << "[AdjList]\n\n";
        runOneCaseTrials("BinaryTree(9) [AdjList]", GraphGen::makeBinaryTreeAdjList(n), trialsCount);

        std::cout << "[AdjMatrix]\n\n";
        runOneCaseTrials("BinaryTree(9) [AdjMatrix]", GraphGen::makeBinaryTreeAdjMatrix(n), trialsCount);
    }

    return 0;
}
