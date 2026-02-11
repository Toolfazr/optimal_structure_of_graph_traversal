// ListDFS.cpp
#include "ListDFS.hpp"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stack>
#include <algorithm>
#include <cstdint>
#include "AdjListGraph.hpp"
#include <memory>
#include "RankSeeking.hpp"

using namespace std;

std::unique_ptr<Graph> ListDFS::construction(const Graph& graph,
                                            const std::vector<std::string> accessRank)
{
    // 合法检查
    if (graph.getNodeCount() == 0) return nullptr;
    if (accessRank.size() != graph.getNodeCount()) return nullptr;

    unordered_set<string> labelsMapped; // 通过labelsMapped判定是否访问重复节点
    vector<Index> rankToIndex; //获取访问秩对应的index访问顺序
    rankToIndex.reserve(accessRank.size()); // 暂未调整的、用Node.index表示的节点邻接关系

    for (auto& label : accessRank) {
        if (!labelsMapped.insert(label).second) return nullptr;
        Index index = graph.getNode(label).index;
        rankToIndex.push_back(index);
    }

    const int n = static_cast<int>(graph.getNodeCount());

    vector<vector<Index>> adjNodes;
    adjNodes.resize(static_cast<size_t>(n));
    for (int u = 0; u < n; ++u) {
        adjNodes[static_cast<size_t>(u)] = graph.getNeighbors(static_cast<Index>(u));
    }

    // DFS 合法性检查 + 记录 DFS-tree 孩子顺序（按发现顺序）
    vector<vector<Index>> childOrder;
    childOrder.resize(static_cast<size_t>(n));

    vector<uint8_t> visited(static_cast<size_t>(n), 0);
    stack<Index> st;

    auto hasUnvisitedNeighbor = [&](Index u) -> bool {
        for (Index v : adjNodes[static_cast<size_t>(u)]) {
            if (!visited[static_cast<size_t>(v)]) return true;
        }
        return false;
    };

    auto isUnvisitedNeighbor = [&](Index u, Index target) -> bool {
        if (visited[static_cast<size_t>(target)]) return false;
        for (Index v : adjNodes[static_cast<size_t>(u)]) {
            if (v == target) return true;
        }
        return false;
    };

    Index root = rankToIndex[0];
    if (root < 0 || root >= n) return nullptr;
    visited[static_cast<size_t>(root)] = 1;
    st.push(root);

    // 找Path-DFS的preorder
    for (int cur = 1; cur < n; ++cur) {
        Index target = rankToIndex[static_cast<size_t>(cur)];
        if (target < 0 || target >= n) return nullptr;

        // target不是parent(st.top())未访问邻居就回溯
        // 回溯过程中，如果当前st.top()还有未访问邻居，就说明这条DFS路径还没走完，访问秩非法
        // 倘若一直回溯到真正的parent,即跳出while循环,如果parent此时还有未访问邻居,就说明这条DFS路径仍没走完
        // 则访问秩非法
        // 除此之外都是合法访问秩
        // ————————————————————————————————————————————————————————————————
        // 一个非法访问秩的例子：
        // 0: {1, 2}
        // 1: {0, 3}
        // 3: {1}
        // 2: {0}
        // 访问秩：0, 1, 2, 3对于这个AdjList + DFS就是非法的 
        // ————————————————————————————————————————————————————————————————
        while (!st.empty() && !isUnvisitedNeighbor(st.top(), target)) {
            Index u = st.top();
            if (hasUnvisitedNeighbor(u)) {
                // 还有未访问邻居时 DFS 不会回溯，rank 非法
                return nullptr;
            }
            st.pop();
        }
        if (st.empty()) return nullptr;

        Index parent = st.top();
        if (!isUnvisitedNeighbor(parent, target)) return nullptr;

        childOrder[static_cast<size_t>(parent)].push_back(target);
        visited[static_cast<size_t>(target)] = 1;
        st.push(target);
    }

    // 必须覆盖全图（连通图前提下应成立）
    for (int i = 0; i < n; ++i) {
        if (!visited[static_cast<size_t>(i)]) return nullptr;
    }

    // 构造静态邻接表顺序：孩子在前，其它邻居在后
    for (int u = 0; u < n; ++u) {
        const auto& kids = childOrder[static_cast<size_t>(u)];
        if (kids.empty()) continue;

        vector<uint8_t> isKid(static_cast<size_t>(n), 0);
        for (Index v : kids) isKid[static_cast<size_t>(v)] = 1;

        vector<Index> reordered;
        reordered.reserve(adjNodes[static_cast<size_t>(u)].size());

        for (Index v : kids) reordered.push_back(v);
        for (Index v : adjNodes[static_cast<size_t>(u)]) {
            if (!isKid[static_cast<size_t>(v)]) reordered.push_back(v);
        }

        adjNodes[static_cast<size_t>(u)] = std::move(reordered);
    }

    // 开始建图
    unique_ptr<AdjListGraph> newGraph = make_unique<AdjListGraph>();
    newGraph->setLabel(graph.getLabel());

    // 加节点
    for (int i = 0; i < static_cast<int>(accessRank.size()); ++i) {
        Index idx = graph.getNode(accessRank[static_cast<size_t>(i)]).index;
        Node newNode(idx, accessRank[static_cast<size_t>(i)]);
        newGraph->addNode(newNode);
    }

    // 加边
    for (int u = 0; u < n; ++u) {
        for (Index v : adjNodes[static_cast<size_t>(u)]) {
            newGraph->addEdge(static_cast<Index>(u), v);
            newGraph->addEdge(v, static_cast<Index>(u));
        }
    }

    return newGraph;
}

std::unique_ptr<Aggregate> ListDFS::rankSeeking(Graph& graph) {
    class ListDFSRankIterator : public Iterator {
    public:
        explicit ListDFSRankIterator(std::vector<std::vector<std::string>> ranks)
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

    class ListDFSAggregate : public Aggregate {
    public:
        explicit ListDFSAggregate(std::vector<std::vector<std::string>> ranks)
            : iteratorImpl_(std::move(ranks)) {}

        Iterator& iterator() override { return iteratorImpl_; }

    private:
        ListDFSRankIterator iteratorImpl_;
    };

    auto ranks = RankSeeking::getBestRanksForDFS(graph);
    return std::make_unique<ListDFSAggregate>(std::move(ranks));
}