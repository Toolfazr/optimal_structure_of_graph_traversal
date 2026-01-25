#pragma once

#include "Graph.hpp"
#include "TraversalAlgo.hpp"
#include <vector>
#include <cstddef>
#include <chrono>

struct RootOptResult
{
    std::size_t bestPeak = 0;     // 最小的“最大占用”
    std::vector<Index> bestRoots; // 所有达到 bestPeak 的 ROOT
};

class Metrics
{
public:
    // 测量遍历时间（ns / 次）
    template <typename Func>
    static double measureAveTraverlsalTime(Graph &g, Func algo, int repeat)
    {
        using namespace std::chrono;
        if (repeat <= 0)
            return 0.0;

        volatile int fuse = 0;

        auto start = steady_clock::now();
        for (int i = 0; i < repeat; i++)
        {
            algo(g);
            fuse ^= 1;
        }
        auto end = steady_clock::now();

        double totalNs = static_cast<double>(
            duration_cast<nanoseconds>(end - start).count());
        return totalNs / repeat;
    }

    // 测量遍历过程中从所有节点开始的最大空间占用，时间复杂度为 n! × n
    static RootOptResult measureDFSMaxStack(Graph &graph);
    static RootOptResult measureBFSMaxQueue(Graph &graph);

    // 测量一张图从ROOT == 0开始的最大空间占用, 并返回遍历序列
    static size_t measureDFSMaxStackFromRoot(Graph &graph, std::vector<std::string> &order);
    static size_t measureBFSMaxQueueFromRoot(Graph &graph, std::vector<std::string> &order);

    // 测量一张图从给定ROOT节点开始的最大空间占用，并返回遍历序列
    static size_t measureDFSMaxStackFromRoot(Graph &graph, std::vector<std::string> &order, Index root);
    static size_t measureBFSMaxQueueFromRoot(Graph &graph, std::vector<std::string> &order, Index root);

    // 比较两个遍历序列的相似程度，范围为[0.0, 1.0]。
    // 越接近1.0表示两个序列越接近，1.0表示两个序列完全相同
    static double getLcsSimilarity(const std::vector<std::string> &orderA,
                                   const std::vector<std::string> &orderB);
    static double getKendallSimilarity(const std::vector<std::string> &orderA,
                                       const std::vector<std::string> &orderB);
    /******************************  Deprecated Code Begin ******************************/

    // 测量遍历过程中的大度节点间距：依赖访问序（visit order）
    // algoTrace: Graph& -> TraversalTrace，例如 TraversalAlgo::bfsTrace / dfsTrace
    template <typename FuncTrace>
    static double measureHighDegreeSpacing(Graph &graph, FuncTrace algoTrace)
    {
        TraversalTrace t = algoTrace(graph);
        return highDegreeSpacingImpl(graph, t);
    }

    // 测量遍历过程中的分支悬挂度：依赖 parent 树与访问序
    template <typename FuncTrace>
    static double measureBranchSuspension(Graph &graph, FuncTrace algoTrace)
    {
        TraversalTrace t = algoTrace(graph);
        return branchSuspensionImpl(t);
    }

    // Pearson 相关系数
    static double pearsonCorr(const std::vector<double> &x,
                              const std::vector<double> &y);
    /******************************  Deprecated Code End ******************************/
private:
    static double highDegreeSpacingImpl(const Graph &graph, const TraversalTrace &t);
    static double branchSuspensionImpl(const TraversalTrace &t);
    static bool hasHighDegreeNode(Graph &graph);
    static std::size_t dfsMaxStackFromRoot(Graph &graph, Index root);
    static std::size_t bfsMaxQueueFromRoot(Graph &graph, Index root);
};
