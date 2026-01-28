// 在邻接链表&DFS这一case下，测试Decomposition给出的访问秩是否具有较好的空间占用
#include "GraphGen.hpp"
#include "ReGraph.hpp"
#include "DistributionStorage.hpp"
#include "Metrics.hpp"
#include "Constants.hpp"
#include "Construction.hpp"
#include "Decomposition.hpp"
#include "Metrics.hpp"
#include <iostream>
#include <chrono>

using namespace std;

namespace
{
    template <class G>
    void doBFSSpaceMeasure(const G &graph, DistributionStorage &occupiedSpaceDistribution)
    {
        occupiedSpaceDistribution.clear();
        int nodeCount = graph.getNodeCount();
        if (nodeCount <= 0)
            return;

        ReGraph::Enumerator<G> reGrapher(graph);
        G res;
        if (nodeCount <= SMALL_SCALE)
        {
            // 全排列遍历
            while (reGrapher.next(res))
            {

                for (int root = 0; root < nodeCount; root++)
                {
                    vector<string> accessRank;
                    size_t occupiedSpace = Metrics::measureBFSMaxQueueFromRoot(res, accessRank, root);
                    occupiedSpaceDistribution.insert(accessRank, occupiedSpace);
                }
            }
        }
        else
        {
            // 随机获取MAX_PERM_NUM个排列
            while (reGrapher.nextRandom(res))
            {
                for (int root = 0; root < nodeCount; root++)
                {
                    vector<string> accessRank;
                    size_t occupiedSpace = Metrics::measureBFSMaxQueueFromRoot(res, accessRank, root);
                    occupiedSpaceDistribution.insert(accessRank, occupiedSpace);
                }
            }
        }
    }

    template <class G>
    void doDFSSpaceMeasure(const G &graph, DistributionStorage &occupiedSpaceDistribution)
    {
        occupiedSpaceDistribution.clear();
        int nodeCount = graph.getNodeCount();
        if (nodeCount <= 0)
            return;
        ReGraph::Enumerator<G> reGrapher(graph);
        G res;
        if (nodeCount <= SMALL_SCALE)
        {
            // 全排列遍历
            while (reGrapher.next(res))
            {
                for (int root = 0; root < nodeCount; root++)
                {
                    vector<string> accessRank;
                    size_t occupiedSpace = Metrics::measureDFSMaxStackFromRoot(res, accessRank, root);
                    occupiedSpaceDistribution.insert(accessRank, occupiedSpace);
                }
            }
        }
        else
        {
            // 随机获取MAX_PERM_NUM个排列
            while (reGrapher.nextRandom(res))
            {
                for (int root = 0; root < nodeCount; root++)
                {
                    vector<string> accessRank;
                    size_t occupiedSpace = Metrics::measureDFSMaxStackFromRoot(res, accessRank, root);
                    occupiedSpaceDistribution.insert(accessRank, occupiedSpace);
                }
            }
        }
    }

}

int main(int argc, char **argv) {
    using Clock = std::chrono::steady_clock;

    // 参数解析
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <n> <p>" << std::endl;
        return 1;
    }

    size_t n = static_cast<size_t>(std::stoul(argv[1]));
    double p = std::stod(argv[2]);

    cout << "Start" << endl;
    auto t0 = Clock::now();

    // Step 1：原存储结构整体空间占用
    auto t1_begin = Clock::now();
    auto listG = GraphGen::makeGraph<AdjListGraph>(n, p);
    DistributionStorage dfsGeneralDistributionOfListG;
    doDFSSpaceMeasure(listG, dfsGeneralDistributionOfListG);
    auto t1_end = Clock::now();

    cout << "Step 1 done, total entries num: "
         << dfsGeneralDistributionOfListG.size()
         << ", time = "
         << std::chrono::duration_cast<std::chrono::milliseconds>(t1_end - t1_begin).count()
         << " ms" << endl;

    // Step 2：最优存储结构空间占用
    auto t2_begin = Clock::now();
    DistributionStorage dfsOptimalDistributionOfListG;
    auto optimalRanksForListG = Decomposition::getRanks(listG);
    cout << "optimalRanks num : " << optimalRanksForListG.size() << endl;
    for (auto rank : optimalRanksForListG)
    {
        AdjListGraph dfsOptimalListG;
        if (Construction::reorderListForDFS(listG, rank, dfsOptimalListG))
        {
            Node firstNode = dfsOptimalListG.getNode(rank[0]);
            vector<string> order;
            size_t maxSize =
                Metrics::measureDFSMaxStackFromRoot(dfsOptimalListG, order, firstNode.index);
            dfsOptimalDistributionOfListG.insert(order, maxSize);
        }
    }
    auto t2_end = Clock::now();

    cout << "Step 2 done, total entries num: "
         << dfsOptimalDistributionOfListG.size()
         << ", time = "
         << std::chrono::duration_cast<std::chrono::milliseconds>(t2_end - t2_begin).count()
         << " ms" << endl;

    // Step 3：写文件
    auto t3_begin = Clock::now();
    cout << "Writing to csv" << endl;
    dfsGeneralDistributionOfListG.toCsv("./TrialRes/Trial_6/general_distribution_list_dfs.csv");
    dfsOptimalDistributionOfListG.toCsv("./TrialRes/Trial_6/optimal_distribution_list_dfs.csv");
    auto t3_end = Clock::now();

    cout << "CSV writing time = "
         << std::chrono::duration_cast<std::chrono::milliseconds>(t3_end - t3_begin).count()
         << " ms" << endl;

    auto t_end = Clock::now();
    cout << "Done, total time = "
         << std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t0).count()
         << " ms" << endl;

    return 0;
}