// Trial_8.cpp
// Test whether the access ranks given by RankSeeking always achieve optimal traversal space usage
// across storage (AdjList/AdjMatrix) and traversal (DFS/BFS).

#include "GraphGen.hpp"
#include "ReGraph.hpp"
#include "DistributionStorage.hpp"
#include "Metrics.hpp"
#include "Constants.hpp"
#include "Construction.hpp"
#include "RankSeeking.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace
{
    static inline long long msSince(const std::chrono::steady_clock::time_point &t0)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - t0)
            .count();
    }

    template <class G>
    void doDFSSpaceMeasure(const G &graph, DistributionStorage &dist)
    {
        dist.clear();
        const int nodeCount = graph.getNodeCount();
        if (nodeCount <= 0)
            return;

        ReGraph::Enumerator<G> reGrapher(graph);
        G res;

        if (nodeCount <= SMALL_SCALE)
        {
            while (reGrapher.next(res))
            {
                for (int root = 0; root < nodeCount; ++root)
                {
                    vector<string> accessRank;
                    const size_t space = Metrics::measureDFSMaxStackFromRoot(res, accessRank, root);
                    dist.insert(accessRank, space);
                }
            }
        }
        else
        {
            while (reGrapher.nextRandom(res))
            {
                for (int root = 0; root < nodeCount; ++root)
                {
                    vector<string> accessRank;
                    const size_t space = Metrics::measureDFSMaxStackFromRoot(res, accessRank, root);
                    dist.insert(accessRank, space);
                }
            }
        }
    }

    template <class G>
    void doBFSSpaceMeasure(const G &graph, DistributionStorage &dist)
    {
        dist.clear();
        const int nodeCount = graph.getNodeCount();
        if (nodeCount <= 0)
            return;

        ReGraph::Enumerator<G> reGrapher(graph);
        G res;

        if (nodeCount <= SMALL_SCALE)
        {
            while (reGrapher.next(res))
            {
                for (int root = 0; root < nodeCount; ++root)
                {
                    vector<string> accessRank;
                    const size_t space = Metrics::measureBFSMaxQueueFromRoot(res, accessRank, root);
                    dist.insert(accessRank, space);
                }
            }
        }
        else
        {
            while (reGrapher.nextRandom(res))
            {
                for (int root = 0; root < nodeCount; ++root)
                {
                    vector<string> accessRank;
                    const size_t space = Metrics::measureBFSMaxQueueFromRoot(res, accessRank, root);
                    dist.insert(accessRank, space);
                }
            }
        }
    }

    // NOTE:
    // If your Construction API naming differs, only adjust these 4 wrappers.
    static inline bool reorderForDFS(const AdjListGraph &in, const vector<string> &rank, AdjListGraph &out)
    {
        return Construction::reorderListForDFS(in, rank, out);
    }
    static inline bool reorderForBFS(const AdjListGraph &in, const vector<string> &rank, AdjListGraph &out)
    {
        return Construction::reorderListForBFS(in, rank, out);
    }
    static inline bool reorderForDFS(const AdjMatrixGraph &in, const vector<string> &rank, AdjMatrixGraph &out)
    {
        return Construction::reorderMatrixForDFS(in, rank, out);
    }
    static inline bool reorderForBFS(const AdjMatrixGraph &in, const vector<string> &rank, AdjMatrixGraph &out)
    {
        return Construction::reorderMatrixForBFS(in, rank, out);
    }

    template <class G>
    size_t measureRankDFS(G &reorderedGraph,
                          const std::vector<std::string> &rank,
                          std::vector<std::string> &outOrder)
    {
        Node root = reorderedGraph.getNode(rank[0]);
        return Metrics::measureDFSMaxStackFromRoot(reorderedGraph, outOrder, root.index);
    }

    template <class G>
    size_t measureRankBFS(G &reorderedGraph,
                          const std::vector<std::string> &rank,
                          std::vector<std::string> &outOrder)
    {
        Node root = reorderedGraph.getNode(rank[0]);
        return Metrics::measureBFSMaxQueueFromRoot(reorderedGraph, outOrder, root.index);
    }

    template <class G>
    void runOneCase(
        const size_t n,
        const double p,
        const std::string &number,
        const std::string &caseTag,
        const std::string &fileTag,
        bool isDFS)
    {
        using Clock = std::chrono::steady_clock;

        const std::string tag =
            "[Trial_8]" + caseTag +
            "[n=" + std::to_string(n) +
            "][p=" + std::to_string(p) +
            "][no=" + number + "] ";

        auto g = GraphGen::makeGraph<G>(n, p);
        g.setLabel(std::to_string(n) + "_" + std::to_string(p) + "_" + number);

        // 1) General distribution
        DistributionStorage generalDist;
        {
            std::cout << tag << "Collecting general distribution..." << std::endl;
            auto t0 = Clock::now();

            if (isDFS)
                doDFSSpaceMeasure(g, generalDist);
            else
                doBFSSpaceMeasure(g, generalDist);

            std::cout << tag << "General distribution collected. elapsed=" << msSince(t0) << "ms"
                      << std::endl;

            std::cout << tag << "Writing general CSVs..." << std::endl;
            const std::string graphInfoPath =
                "graph_info_" + fileTag + "_" + g.getLabel() + ".csv";
            const std::string distributionPath =
                "general_distribution_" + fileTag + "_" + g.getLabel() + ".csv";

            g.toCsv("./TrialRes/Trial_8/" + graphInfoPath);
            generalDist.toCsv("./TrialRes/Trial_8/" + distributionPath);

            std::cout << tag << "General CSVs written: " << graphInfoPath
                      << ", " << distributionPath << std::endl;
        }

        // 2) RankSeeking ranks
        std::vector<std::vector<std::string>> ranksSought;
        {
            std::cout << tag
                      << (isDFS ? "RankSeeking::getBestRanksForDFS..." : "RankSeeking::getBestRanksForBFS...")
                      << std::endl;
            auto t0 = Clock::now();

            ranksSought = isDFS ? RankSeeking::getBestRanksForDFS(g)
                                : RankSeeking::getBestRanksForBFS(g);

            std::cout << tag << "RankSeeking done. size=" << ranksSought.size()
                      << " elapsed=" << msSince(t0) << "ms" << std::endl;
        }

        // 3) Measure ranksSought and write optimal distribution
        {
            std::cout << tag << "Measuring ranksSought traversal space..." << std::endl;
            auto t0 = Clock::now();

            DistributionStorage optimalDist;

            std::size_t invalidRankCnt = 0;
            std::size_t mismatchCnt = 0;

            for (std::size_t i = 0; i < ranksSought.size(); ++i)
            {
                const auto &rank = ranksSought[i];

                G reorderedGraph;
                bool ok = false;
                if (isDFS)
                    ok = reorderForDFS(g, rank, reorderedGraph);
                else
                    ok = reorderForBFS(g, rank, reorderedGraph);

                if (!ok)
                {
                    ++invalidRankCnt;
                    std::cout << tag << "ERROR: invalid access rank (reorder failed). rank_idx=" << i
                              << std::endl;
                    continue;
                }

                std::vector<std::string> order;
                size_t space = 0;
                if (isDFS)
                    space = measureRankDFS(reorderedGraph, rank, order);
                else
                    space = measureRankBFS(reorderedGraph, rank, order);

                if (order != rank)
                {
                    ++mismatchCnt;
                    std::cout << tag << "ERROR: rank != traversal order. rank_idx=" << i
                              << " space=" << space << std::endl;

                    std::cout << tag << "rank : ";
                    for (auto &s : rank)
                        std::cout << s << " ";
                    std::cout << std::endl;

                    std::cout << tag << "order: ";
                    for (auto &s : order)
                        std::cout << s << " ";
                    std::cout << std::endl;
                }

                optimalDist.insert(order, space);
            }

            std::cout << tag << "Measured ranksSought. elapsed=" << msSince(t0) << "ms"
                      << " invalid=" << invalidRankCnt
                      << " mismatched=" << mismatchCnt
                      << std::endl;

            std::cout << tag << "Writing optimal distribution CSV..." << std::endl;
            const std::string optimalPath =
                "optimal_distribution_" + fileTag + "_" + g.getLabel() + ".csv";
            optimalDist.toCsv("./TrialRes/Trial_8/" + optimalPath);
            std::cout << tag << "Optimal CSV written: " << optimalPath << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <n> <p> <number>" << std::endl;
        return 1;
    }

    const size_t n = static_cast<size_t>(std::stoul(argv[1]));
    const double p = std::stod(argv[2]);
    const std::string number = std::string(argv[3]);

    // AdjListGraph
    runOneCase<AdjListGraph>(n, p, number, "[AdjList][DFS]", "AdjList_DFS", /*isDFS*/ true);
    runOneCase<AdjListGraph>(n, p, number, "[AdjList][BFS]", "AdjList_BFS", /*isDFS*/ false);

    // AdjMatrixGraph
    runOneCase<AdjMatrixGraph>(n, p, number, "[AdjMatrix][DFS]", "AdjMatrix_DFS", /*isDFS*/ true);
    runOneCase<AdjMatrixGraph>(n, p, number, "[AdjMatrix][BFS]", "AdjMatrix_BFS", /*isDFS*/ false);

    return 0;
}