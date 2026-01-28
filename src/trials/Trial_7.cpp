// 在邻接链表&DFS这一case下，观察最优访问秩与图、节点之间的关系

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
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <n> <p> <number>" << std::endl;
        return 1;
    }

    size_t n = static_cast<size_t>(std::stoul(argv[1]));
    double p = std::stod(argv[2]);
    string number = string(argv[3]);
    auto listG = GraphGen::makeGraph<AdjListGraph>(n, p);

    listG.setLabel(to_string(n) + "_" + to_string(p) + "_" +number);
    DistributionStorage dfsGeneralDistributionOfListG;
    doDFSSpaceMeasure(listG, dfsGeneralDistributionOfListG);
    string graphInfoPath = "graph_info_" + listG.getLabel() + ".csv";
    string distributionPath = "general_distribution_" + listG.getLabel() + ".csv";
    
    listG.toCsv("./TrialRes/Trial_7/" + graphInfoPath);
    dfsGeneralDistributionOfListG.toCsv("./TrialRes/Trial_7/" + distributionPath);
    return 0;
}