//在邻接链表&DFS这一case下，测试Decomposition给出的访问秩是否具有较好的空间占用
#include "GraphGen.hpp"
#include "ReGraph.hpp"
#include "DistributionStorage.hpp"
#include "Metrics.hpp"
#include "Constants.hpp"
#include "Construction.hpp"
#include "Decomposition.hpp"
#include "Metrics.hpp"
#include <iostream>

using namespace std;

namespace {
    template <class G>
    void doBFSSpaceMeasure(const G& graph, DistributionStorage& occupiedSpaceDistribution) {
        occupiedSpaceDistribution.clear();
        int nodeCount = graph.getNodeCount();
        if(nodeCount <= 0) return;

        ReGraph::Enumerator<G> reGrapher(graph);
        G res;
        if(nodeCount <= SMALL_SCALE) {
            // 全排列遍历
            while(reGrapher.next(res)) {
                
                for(int root = 0; root < nodeCount; root++) {
                    vector<string> accessRank;
                    size_t occupiedSpace = Metrics::measureBFSMaxQueueFromRoot(res, accessRank, root);
                    occupiedSpaceDistribution.insert(accessRank, occupiedSpace);
                }
            }
        } else {
            // 随机获取MAX_PERM_NUM个排列
            while(reGrapher.nextRandom(res)) {
                for(int root = 0; root < nodeCount; root++) {
                    vector<string> accessRank;
                    size_t occupiedSpace = Metrics::measureBFSMaxQueueFromRoot(res, accessRank, root);
                    occupiedSpaceDistribution.insert(accessRank, occupiedSpace);
                }
            }
        }
    }

    template <class G>
    void doDFSSpaceMeasure(const G& graph, DistributionStorage& occupiedSpaceDistribution) {
        occupiedSpaceDistribution.clear();
        int nodeCount = graph.getNodeCount();
        if(nodeCount <= 0) return;
        ReGraph::Enumerator<G> reGrapher(graph);
        G res;
        if(nodeCount <= SMALL_SCALE) {
            // 全排列遍历
            
            while(reGrapher.next(res)) {
                for(int root = 0; root < nodeCount; root++) {
                    vector<string> accessRank;
                    size_t occupiedSpace = Metrics::measureDFSMaxStackFromRoot(res, accessRank, root);
                    occupiedSpaceDistribution.insert(accessRank, occupiedSpace);
                }
            }
        } else {
            // 随机获取MAX_PERM_NUM个排列
            while(reGrapher.nextRandom(res)) {
                for(int root = 0; root < nodeCount; root++) {
                    vector<string> accessRank;
                    size_t occupiedSpace = Metrics::measureDFSMaxStackFromRoot(res, accessRank, root);
                    occupiedSpaceDistribution.insert(accessRank, occupiedSpace);
                }
            }
        }
    }

}

int main() {
    cout << "Start" << endl;
    // 统计原存储结构整体空间占用
    auto listG = GraphGen::makeGraph<AdjListGraph>(12, 0.33);
    DistributionStorage dfsGeneralDistributionOfListG; // 邻接链表DFS的整体空间分布
    doBFSSpaceMeasure(listG, dfsGeneralDistributionOfListG);
    cout << "Step 1 done, total entries num: " << dfsGeneralDistributionOfListG.size() << endl;

    // 统计新存储结构空间占用
    DistributionStorage dfsOptimalDistributionOfListG; // 最优邻接链表DFS的空间分布。
    auto optimalRanksForListG = Decomposition::getRanks(listG);
    for(auto rank : optimalRanksForListG) {
        AdjListGraph dfsOptimalListG;
        // 当访问秩合法时
        if(Construction::reorderListForDFS(listG, rank, dfsOptimalListG)) {
            Node firstNode = dfsOptimalListG.getNode(rank[0]);
            vector<string> order;
            size_t maxSize = Metrics::measureDFSMaxStackFromRoot(dfsOptimalListG, order, firstNode.index);
            dfsOptimalDistributionOfListG.insert(order, maxSize);
        }
    }
    cout << "Step 2 done, total entries num: " << dfsOptimalDistributionOfListG.size() << endl;

    cout << "Writing to csv" << endl;
    dfsGeneralDistributionOfListG.toCsv("./TrialRes/Trial_6/general_distribution_list_dfs.csv");
    dfsOptimalDistributionOfListG.toCsv("./TrialRes/Trial_6/optimal_distribution_list_dfs.csv");
    cout << "Done" << endl;
    return 0;
}