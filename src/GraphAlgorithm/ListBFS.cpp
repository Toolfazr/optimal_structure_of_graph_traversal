#include "ListBFS.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "AdjListGraph.hpp"
#include <algorithm>
#include <limits>
#include "RankSeeking.hpp"

using namespace std;

std::unique_ptr<Graph> ListBFS::construction(const Graph& graph, const std::vector<std::string> accessRank) {
    // 合法检查
    if(graph.getNodeCount() == 0) return nullptr;
    if(accessRank.size() != graph.getNodeCount()) return nullptr;
    
    unordered_set<string> labelsMapped; // 通过labelsMapped判定是否访问重复节点
    vector<Index> rankToIndex; //获取访问秩对应的index访问顺序
    for(auto& label : accessRank) {
        if(!labelsMapped.insert(label).second) {
            return nullptr;
        }
        Index index = graph.getNode(label).index;
        rankToIndex.push_back(index);
    }

    // 调整Node的邻接节点Index顺序, 使其符合访问秩
    unordered_map<Index, bool> visited;
    vector<vector<Index>> adjNodes; // 调整完毕的、用Node.index表示的节点邻接关系
    adjNodes.resize(graph.getNodeCount());
    for(Index index : rankToIndex) {
        visited.insert({index, false});
    }
    queue<Index> qu;
    Index root = rankToIndex[0];
    qu.push(root);
    visited[root] = true;
    size_t pos = 1;
    while(!qu.empty()) {
        Index u = qu.front();
        qu.pop();
        // 调整邻接未访问节点的Index顺序,邻接已访问的节点肯定处理其它节点时已调整过了, 且不影响当前节点的访问
        std::vector<Index> unvisited;
        for(Index v : graph.getNeighbors(u)) {
            if(!visited[v]) {
                unvisited.push_back(v);
            }
        }

        const size_t need = unvisited.size();
        if(pos + need > accessRank.size()) return nullptr;

        // 获取rankToIndex[cur]节点调整后的邻接节点Index顺序
        vector<Index> expected;
        expected.reserve(static_cast<size_t>(need));
        for(int i = 0; i < need; i++) {
            expected.push_back(rankToIndex[pos + i]);
        }
        
        // 进行判断: 如果排序后未访问节点序列与访问秩片段不是identical的，那么访问秩非法
        vector<Index> unvisitedSorted = unvisited;
        vector<Index> expectedSorted = expected;
        sort(unvisitedSorted.begin(), unvisitedSorted.end());
        sort(expectedSorted.begin(), expectedSorted.end());
        if (unvisitedSorted != expectedSorted) {
            return nullptr;
        }

        // 将调整好的邻接节点Index存入adjNodes
        vector<Index> reordered;
        Node curNode = graph.getNode(u);
        reordered.reserve(graph.getNeighbors(curNode.index).size());
        for(Index v : expected) {
            reordered.push_back(v);
        }
        for(Index v : graph.getNeighbors(curNode.index)) {
            if(visited[v]){
                reordered.push_back(v);
            }
        }
        adjNodes[u] = move(reordered);

        for(Index v : expected) {
            visited[v] = true;
            qu.push(v);
        }

        pos += need;
    }

    if(pos != accessRank.size()) return nullptr;

    // 开始建图
    unique_ptr<AdjListGraph> newGraph = make_unique<AdjListGraph>();
    newGraph->setLabel(graph.getLabel());

    // 向图中加节点
    for(int i = 0; i < accessRank.size(); i++) {
        Node newNode(graph.getNode(accessRank[i]).index, accessRank[i]);
        newGraph->addNode(newNode);
    }

    // 向图中加边
    for(int i = 0; i < accessRank.size(); i++) {
        Index u = graph.getNode(accessRank[i]).index;
        for(Index v : adjNodes[u]) {
            newGraph->addEdge(u, v);
            newGraph->addEdge(v, u);
        }
    }

    return newGraph;
}

std::unique_ptr<Aggregate> ListBFS::rankSeeking(Graph& graph) {
    class ListBFSRankIterator : public Iterator {
    public:
        explicit ListBFSRankIterator(std::vector<std::vector<std::string>> ranks)
            : ranks_(std::move(ranks)), cursor_(0) {}

        bool hasNext() override { return cursor_ < ranks_.size(); }

        void* next() override {
            if (!hasNext()) return nullptr;
            return static_cast<void*>(&ranks_[cursor_++]);
        }

    private:
        std::vector<std::vector<std::string>> ranks_;
        std::size_t cursor_;
    };

    class ListBFSAggregate : public Aggregate {
    public:
        explicit ListBFSAggregate(std::vector<std::vector<std::string>> ranks)
            : iteratorImpl_(std::move(ranks)) {}

        Iterator& iterator() override { return iteratorImpl_; }

    private:
        ListBFSRankIterator iteratorImpl_;
    };

    auto ranks = RankSeeking::getBestRanksForBFS(graph);
    return std::make_unique<ListBFSAggregate>(std::move(ranks));
}