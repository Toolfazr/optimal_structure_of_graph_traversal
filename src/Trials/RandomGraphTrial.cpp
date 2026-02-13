#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"
#include "RandomGen.hpp"
#include "GenCmd.hpp"
#include "RankSeeking.hpp"
#include "DistributionStorage.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

    template <class G>
    G buildGraphFromPermutation(const G &graph, const vector<Index> &perm)
    {
        const int n = static_cast<int>(graph.getNodeCount());
        vector<Index> inverse(static_cast<size_t>(n), -1);
        for (int oldId = 0; oldId < n; ++oldId)
        {
            inverse[static_cast<size_t>(perm[static_cast<size_t>(oldId)])] = static_cast<Index>(oldId);
        }

        G out;
        for (int newId = 0; newId < n; ++newId)
        {
            const Index oldId = inverse[static_cast<size_t>(newId)];
            out.addNode(Node(static_cast<Index>(newId), graph.getNode(oldId).label));
        }

        for (int newId = 0; newId < n; ++newId)
        {
            const Index oldId = inverse[static_cast<size_t>(newId)];
            for (Index oldAdj : graph.getNeighbors(oldId))
            {
                const Index newAdj = perm[static_cast<size_t>(oldAdj)];
                out.addEdge(static_cast<Index>(newId), newAdj);
                out.addEdge(newAdj, static_cast<Index>(newId));
            }
        }

        return out;
    }

    template <class G>
    size_t measureDFSMaxStackFromRoot(const G &graph, vector<string> &order, Index root)
    {
        order.clear();
        const int n = static_cast<int>(graph.getNodeCount());
        if (n <= 0 || root < 0 || root >= n)
            return 0;

        vector<bool> visited(static_cast<size_t>(n), false);
        vector<Index> st;
        st.push_back(root);
        size_t peak = st.size();

        while (!st.empty())
        {
            Index u = st.back();
            st.pop_back();
            if (visited[static_cast<size_t>(u)])
                continue;

            visited[static_cast<size_t>(u)] = true;
            order.push_back(graph.getNode(u).label);

            auto ns = graph.getNeighbors(u);
            for (auto it = ns.rbegin(); it != ns.rend(); ++it)
            {
                if (!visited[static_cast<size_t>(*it)])
                    st.push_back(*it);
            }
            peak = max(peak, st.size());
        }

        return peak;
    }

    template <class G>
    size_t measureBFSMaxQueueFromRoot(const G &graph, vector<string> &order, Index root)
    {
        order.clear();
        const int n = static_cast<int>(graph.getNodeCount());
        if (n <= 0 || root < 0 || root >= n)
            return 0;

        vector<bool> visited(static_cast<size_t>(n), false);
        deque<Index> q;
        q.push_back(root);
        visited[static_cast<size_t>(root)] = true;
        size_t peak = q.size();

        while (!q.empty())
        {
            Index u = q.front();
            q.pop_front();
            order.push_back(graph.getNode(u).label);

            for (Index v : graph.getNeighbors(u))
            {
                if (!visited[static_cast<size_t>(v)])
                {
                    visited[static_cast<size_t>(v)] = true;
                    q.push_back(v);
                }
            }
            peak = max(peak, q.size());
        }

        return peak;
    }

    template <class G>
    void doSpaceMeasure(const G &graph, DistributionStorage &dist, bool isDFS)
    {
        dist.clear();
        const int n = static_cast<int>(graph.getNodeCount());
        if (n <= 0)
            return;

        if (n <= static_cast<int>(SMALL_SCALE))
        {
            vector<Index> perm(static_cast<size_t>(n));
            iota(perm.begin(), perm.end(), static_cast<Index>(0));
            do
            {
                G remapped = buildGraphFromPermutation(graph, perm);
                for (int root = 0; root < n; ++root)
                {
                    vector<string> accessRank;
                    const size_t space = isDFS
                                             ? measureDFSMaxStackFromRoot(remapped, accessRank, root)
                                             : measureBFSMaxQueueFromRoot(remapped, accessRank, root);
                    dist.insert(accessRank, space);
                }
            } while (next_permutation(perm.begin(), perm.end()));
            return;
        }

        mt19937 rng{random_device{}()};
        unordered_set<string> seen;
        const size_t target = static_cast<size_t>(MAX_PERM_NUM);
        const size_t attemptLimit = max<size_t>(100, target * 50);

        size_t generated = 0;
        size_t attempts = 0;
        while (generated < target && attempts < attemptLimit)
        {
            ++attempts;
            vector<Index> perm(static_cast<size_t>(n));
            iota(perm.begin(), perm.end(), static_cast<Index>(0));
            shuffle(perm.begin(), perm.end(), rng);

            string sig;
            sig.reserve(static_cast<size_t>(n) * 4);
            for (Index x : perm)
            {
                sig.append(to_string(x));
                sig.push_back(',');
            }
            if (!seen.insert(sig).second)
                continue;

            ++generated;
            G remapped = buildGraphFromPermutation(graph, perm);
            for (int root = 0; root < n; ++root)
            {
                vector<string> accessRank;
                const size_t space = isDFS
                                         ? measureDFSMaxStackFromRoot(remapped, accessRank, root)
                                         : measureBFSMaxQueueFromRoot(remapped, accessRank, root);
                dist.insert(accessRank, space);
            }
        }
    }

    template <class G>
    bool reorderByRank(const G &in, const vector<string> &rank, G &out)
    {
        const int n = static_cast<int>(in.getNodeCount());
        if (static_cast<int>(rank.size()) != n || n <= 0)
            return false;

        unordered_map<string, Index> labelToNew;
        labelToNew.reserve(static_cast<size_t>(n));
        vector<Index> newToOld(static_cast<size_t>(n), -1);

        for (int i = 0; i < n; ++i)
        {
            if (labelToNew.find(rank[static_cast<size_t>(i)]) != labelToNew.end())
                return false;
            Node oldNode = in.getNode(rank[static_cast<size_t>(i)]);
            if (oldNode.index < 0)
                return false;
            labelToNew[rank[static_cast<size_t>(i)]] = static_cast<Index>(i);
            newToOld[static_cast<size_t>(i)] = oldNode.index;
        }

        G g;
        g.setLabel(in.getLabel());
        for (int i = 0; i < n; ++i)
        {
            g.addNode(Node(static_cast<Index>(i), rank[static_cast<size_t>(i)]));
        }

        for (int oldU = 0; oldU < n; ++oldU)
        {
            const string ul = in.getNode(static_cast<Index>(oldU)).label;
            const Index newU = labelToNew[ul];

            vector<pair<Index, Index>> ns;
            for (Index oldV : in.getNeighbors(static_cast<Index>(oldU)))
            {
                const string vl = in.getNode(oldV).label;
                const Index newV = labelToNew[vl];
                ns.push_back({newV, oldV});
            }
            sort(ns.begin(), ns.end(), [](const auto &a, const auto &b)
                 { return a.first < b.first; });

            for (const auto &it : ns)
            {
                g.addEdge(newU, it.first);
            }
        }

        out = std::move(g);
        return true;
    }

    template <class G>
    void runOneCase(
        const size_t n,
        const double p,
        const std::string &number,
        const std::string &structureType,
        const std::string &caseTag,
        const std::string &fileTag,
        bool isDFS)
    {
        using Clock = std::chrono::steady_clock;

        const std::string tag =
            "[RandomGraphTrial]" + caseTag +
            "[n=" + std::to_string(n) +
            "][p=" + std::to_string(p) +
            "][no=" + number + "] ";

        auto g = makeRandomConnectedGraph<G>(n, p, structureType);
        g.setLabel(std::to_string(n) + "_" + std::to_string(p) + "_" + number);

        DistributionStorage generalDist;
        {
            std::cout << tag << "Collecting general distribution..." << std::endl;
            auto t0 = Clock::now();

            doSpaceMeasure(g, generalDist, isDFS);

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
                const bool ok = reorderByRank(g, rank, reorderedGraph);

                if (!ok)
                {
                    ++invalidRankCnt;
                    std::cout << tag << "ERROR: invalid access rank (reorder failed). rank_idx=" << i
                              << std::endl;
                    continue;
                }

                std::vector<std::string> order;
                const size_t space = isDFS
                                         ? measureDFSMaxStackFromRoot(reorderedGraph, order, reorderedGraph.getNode(rank[0]).index)
                                         : measureBFSMaxQueueFromRoot(reorderedGraph, order, reorderedGraph.getNode(rank[0]).index);

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

    runOneCase<AdjListGraph>(n, p, number, "AdjList", "[AdjList][DFS]", "AdjList_DFS", true);
    runOneCase<AdjListGraph>(n, p, number, "AdjList", "[AdjList][BFS]", "AdjList_BFS", false);

    runOneCase<AdjMatrixGraph>(n, p, number, "AdjMatrix", "[AdjMatrix][DFS]", "AdjMatrix_DFS", true);
    runOneCase<AdjMatrixGraph>(n, p, number, "AdjMatrix", "[AdjMatrix][BFS]", "AdjMatrix_BFS", false);

    return 0;
}
