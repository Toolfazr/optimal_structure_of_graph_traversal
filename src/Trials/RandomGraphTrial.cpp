#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"
#include "RandomGen.hpp"
#include "GenCmd.hpp"
#include "DistributionStorage.hpp"
#include "ListDFS.hpp"
#include "ListBFS.hpp"
#include "MatrixDFS.hpp"
#include "MatrixBFS.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    static inline long long msSince(const std::chrono::steady_clock::time_point &t0)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - t0)
            .count();
    }

    template <class G>
    G makeRandomConnectedGraph(size_t n, double p, const std::string &structureType)
    {
        RandomGen generator;
        GenCmd cmd("RandomGraph " + structureType + " " + std::to_string(n) + " " + std::to_string(p));

        if (!generator.parseCmd(cmd))
        {
            throw std::runtime_error("Failed to parse random graph generation command.");
        }

        std::unique_ptr<Graph> generated = generator.genGraph();
        if (!generated)
        {
            throw std::runtime_error("Random graph generation returned null graph.");
        }

        auto *typed = dynamic_cast<G *>(generated.get());
        if (typed == nullptr)
        {
            throw std::runtime_error("Generated graph type mismatch.");
        }

        return *typed;
    }

    template <class G, class S>
    void runOneCase(
        const size_t n,
        const double p,
        const std::string &number,
        const std::string &structureType,
        const std::string &caseTag,
        const std::string &fileTag)
    {
        using Clock = std::chrono::steady_clock;

        const std::string tag =
            "[RandomGraphTrial]" + caseTag +
            "[n=" + std::to_string(n) +
            "][p=" + std::to_string(p) +
            "][no=" + number + "] ";

        S strategy;
        G g = makeRandomConnectedGraph<G>(n, p, structureType);
        g.setLabel(std::to_string(n) + "_" + std::to_string(p) + "_" + number);

        DistributionStorage generalDist;
        {
            std::cout << tag << "Collecting general distribution..." << std::endl;
            auto t0 = Clock::now();

            std::unique_ptr<Aggregate> allGraphs = strategy.reGraph(g);
            Iterator &it = allGraphs->iterator();
            while (it.hasNext())
            {
                auto *remapped = static_cast<Graph *>(it.next());
                if (remapped == nullptr)
                {
                    continue;
                }

                const int nodeCount = static_cast<int>(remapped->getNodeCount());
                for (int root = 0; root < nodeCount; ++root)
                {
                    std::vector<std::string> accessRank;
                    const size_t space = strategy.traversal(*remapped, static_cast<Index>(root), accessRank);
                    generalDist.insert(accessRank, space);
                }
            }

            std::cout << tag << "General distribution collected. elapsed=" << msSince(t0) << "ms"
                      << std::endl;

            std::cout << tag << "Writing general CSVs..." << std::endl;
            const std::string graphInfoPath =
                "graph_info_" + fileTag + "_" + g.getLabel() + ".csv";
            const std::string distributionPath =
                "general_distribution_" + fileTag + "_" + g.getLabel() + ".csv";

            g.toCsv("./TrialRes/RandomGraphTrial/" + graphInfoPath);
            generalDist.toCsv("./TrialRes/RandomGraphTrial/" + distributionPath);

            std::cout << tag << "General CSVs written: " << graphInfoPath
                      << ", " << distributionPath << std::endl;
        }

        {
            std::cout << tag << "Measuring rankSeeking traversal space..." << std::endl;
            auto t0 = Clock::now();

            DistributionStorage optimalDist;
            std::size_t invalidRankCnt = 0;
            std::size_t mismatchCnt = 0;
            std::size_t rankCount = 0;

            std::unique_ptr<Aggregate> ranks = strategy.rankSeeking(g);
            Iterator &it = ranks->iterator();
            while (it.hasNext())
            {
                auto *rank = static_cast<std::vector<std::string> *>(it.next());
                if (rank == nullptr || rank->empty())
                {
                    ++invalidRankCnt;
                    continue;
                }
                ++rankCount;

                std::unique_ptr<Graph> reordered = strategy.construction(g, *rank);
                if (!reordered)
                {
                    ++invalidRankCnt;
                    std::cout << tag << "ERROR: invalid access rank (construction failed)." << std::endl;
                    continue;
                }

                const Index root = reordered->getNode((*rank)[0]).index;
                std::vector<std::string> order;
                const size_t space = strategy.traversal(*reordered, root, order);

                if (order != *rank)
                {
                    ++mismatchCnt;
                    std::cout << tag << "ERROR: rank != traversal order. space=" << space << std::endl;
                    
                    // Debug code begin
                    std::cout << "The rank is: ";
                    for(auto& label : *rank) {
                        std::cout << label << " ";
                    }
                    std::cout << std::endl;

                    std::cout << "While the order is: ";
                    for(auto& label : order) {
                        std::cout << label << " ";
                    }
                    std::cout << std::endl;
                    // Debug code end
                }
                
                
                optimalDist.insert(order, space);
            }

            std::cout << tag << "Measured rankSeeking. size=" << rankCount
                      << " elapsed=" << msSince(t0) << "ms"
                      << " invalid=" << invalidRankCnt
                      << " mismatched=" << mismatchCnt
                      << std::endl;

            std::cout << tag << "Writing optimal distribution CSV..." << std::endl;
            const std::string optimalPath =
                "optimal_distribution_" + fileTag + "_" + g.getLabel() + ".csv";
            optimalDist.toCsv("./TrialRes/RandomGraphTrial/" + optimalPath);
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

    runOneCase<AdjListGraph, ListDFS>(n, p, number, "AdjList", "[AdjList][DFS]", "AdjList_DFS");
    runOneCase<AdjListGraph, ListBFS>(n, p, number, "AdjList", "[AdjList][BFS]", "AdjList_BFS");

    runOneCase<AdjMatrixGraph, MatrixDFS>(n, p, number, "AdjMatrix", "[AdjMatrix][DFS]", "AdjMatrix_DFS");
    runOneCase<AdjMatrixGraph, MatrixBFS>(n, p, number, "AdjMatrix", "[AdjMatrix][BFS]", "AdjMatrix_BFS");

    return 0;
}
