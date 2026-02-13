// MatrixDFS.cpp
#include "MatrixDFS.hpp"
#include <string>
#include <unordered_set>
#include <vector>
#include <stack>
#include <algorithm>
#include <cstdint>
#include "AdjMatrixGraph.hpp"
#include <memory>
#include "RankSeeking.hpp"
#include <numeric>
#include <Constants.hpp>
#include <random>

using namespace std;

namespace
{
    class MatrixDFSReGraphIterator final : public Iterator
    {
    public:
        explicit MatrixDFSReGraphIterator(const Graph &graph)
        {
            n_ = static_cast<int>(graph.getNodeCount());
            if (n_ <= 0)
            {
                finished_ = true;
                return;
            }

            originalAdj_.clear();
            originalNodes_.clear();
            originalAdj_.resize(static_cast<size_t>(n_));
            originalNodes_.resize(static_cast<size_t>(n_), Node(-1, "none"));
            for (int id = 0; id < n_; ++id)
            {
                originalAdj_[static_cast<size_t>(id)] = graph.getNeighbors(static_cast<Index>(id));
                originalNodes_[static_cast<size_t>(id)] = graph.getNode(static_cast<Index>(id));
            }
            perm_.resize(static_cast<size_t>(n_));
            iota(perm_.begin(), perm_.end(), static_cast<Index>(0));

            if (n_ >= 10)
            {
                randomTarget_ = static_cast<size_t>(MAX_PERM_NUM);
                const uint64_t rt = static_cast<uint64_t>(randomTarget_);
                uint64_t attempts = (rt > numeric_limits<uint64_t>::max() / 50ull)
                                        ? numeric_limits<uint64_t>::max()
                                        : rt * 50ull;
                if (attempts < 100ull)
                    attempts = 100ull;
                attemptsLimit_ = static_cast<size_t>(
                    min<uint64_t>(attempts, static_cast<uint64_t>(numeric_limits<size_t>::max())));
            }
        }

        bool hasNext() override
        {
            if (finished_)
                return false;
            return n_ < 10 ? true : (generated_ < randomTarget_);
        }

        void *next() override
        {
            if (!hasNext())
                return nullptr;

            if (n_ < 10)
            {
                if (started_)
                {
                    if (!next_permutation(perm_.begin(), perm_.end()))
                    {
                        finished_ = true;
                        return nullptr;
                    }
                }
                else
                {
                    started_ = true;
                }
                currentGraph_ = buildFromPermutation(perm_);
                return currentGraph_.get();
            }

            size_t attempts = 0;
            while (attempts < attemptsLimit_)
            {
                vector<Index> candidate(static_cast<size_t>(n_));
                iota(candidate.begin(), candidate.end(), static_cast<Index>(0));
                shuffle(candidate.begin(), candidate.end(), rng_);

                uint64_t h = 1469598103934665603ull;
                for (Index v : candidate)
                {
                    h ^= static_cast<uint64_t>(v) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
                    h *= 1099511628211ull;
                }

                if (seenHashes_.insert(h).second)
                {
                    ++generated_;
                    currentGraph_ = buildFromPermutation(candidate);
                    return currentGraph_.get();
                }
                ++attempts;
            }

            finished_ = true;
            return nullptr;
        }

    private:
        unique_ptr<Graph> buildFromPermutation(const vector<Index> &perm) const
        {
            vector<Index> inverse(static_cast<size_t>(n_), -1);
            for (int oldId = 0; oldId < n_; ++oldId)
            {
                inverse[static_cast<size_t>(perm[static_cast<size_t>(oldId)])] = static_cast<Index>(oldId);
            }

            unique_ptr<AdjMatrixGraph> graph = make_unique<AdjMatrixGraph>();
            for (int newId = 0; newId < n_; ++newId)
            {
                Index oldId = inverse[static_cast<size_t>(newId)];
                graph->addNode(Node(static_cast<Index>(newId), originalNodes_[static_cast<size_t>(oldId)].label));
            }

            for (int newId = 0; newId < n_; ++newId)
            {
                Index oldId = inverse[static_cast<size_t>(newId)];
                for (Index adjOld : originalAdj_[static_cast<size_t>(oldId)])
                {
                    Index adjNew = perm[static_cast<size_t>(adjOld)];
                    graph->addEdge(static_cast<Index>(newId), adjNew);
                    graph->addEdge(adjNew, static_cast<Index>(newId));
                }
            }

            return graph;
        }

        int n_ = 0;
        bool started_ = false;
        bool finished_ = false;
        size_t generated_ = 0;
        size_t randomTarget_ = 0;
        size_t attemptsLimit_ = 0;

        vector<vector<Index>> originalAdj_;
        vector<Node> originalNodes_;
        vector<Index> perm_;
        unordered_set<uint64_t> seenHashes_;
        mt19937 rng_{random_device{}()};
        unique_ptr<Graph> currentGraph_;
    };

    class MatrixDFSReGraphAggregate final : public Aggregate
    {
    public:
        explicit MatrixDFSReGraphAggregate(const Graph &graph) : iterator_(graph) {}
        Iterator &iterator() override { return iterator_; }

    private:
        MatrixDFSReGraphIterator iterator_;
    };
} // namespace
std::unique_ptr<Graph> MatrixDFS::construction(const Graph &graph,
                                               const std::vector<std::string> accessRank)
{
    // 合法检查
    const int n = static_cast<int>(graph.getNodeCount());
    if (n == 0)
        return nullptr;
    if (static_cast<int>(accessRank.size()) != n)
        return nullptr;

    // accessRank -> rankToOldIndex（并检查 label 唯一）
    std::unordered_set<std::string> labelsMapped;
    std::vector<Index> rankToOld;
    rankToOld.reserve(static_cast<size_t>(n));

    for (const auto &label : accessRank)
    {
        if (!labelsMapped.insert(label).second)
            return nullptr;
        Index oldIdx = graph.getNode(label).index;
        rankToOld.push_back(oldIdx);
    }

    // oldToNew[oldIndex] = newIndex(=rank position)
    std::vector<Index> oldToNew(static_cast<size_t>(n), -1);
    for (int newIdx = 0; newIdx < n; ++newIdx)
    {
        Index oldIdx = rankToOld[static_cast<size_t>(newIdx)];
        if (oldIdx < 0 || oldIdx >= n)
            return nullptr;
        if (oldToNew[static_cast<size_t>(oldIdx)] != -1)
            return nullptr;
        oldToNew[static_cast<size_t>(oldIdx)] = static_cast<Index>(newIdx);
    }

    // 在“重编号后的图 + 邻居按 newIndex 升序（矩阵典型行为）”下做合法性检查，
    // 并用与 MatrixDFS::traversal 完全一致的 nextIdx 语义模拟 preorder。
    std::vector<std::vector<Index>> newNeighbors(static_cast<size_t>(n));
    for (int oldU = 0; oldU < n; ++oldU)
    {
        Index newU = oldToNew[static_cast<size_t>(oldU)];
        if (newU < 0 || newU >= n)
            return nullptr;

        for (Index oldV : graph.getNeighbors(static_cast<Index>(oldU)))
        {
            if (oldV < 0 || oldV >= n)
                continue;
            Index newV = oldToNew[static_cast<size_t>(oldV)];
            if (newV < 0 || newV >= n)
                return nullptr;
            newNeighbors[static_cast<size_t>(newU)].push_back(newV);
        }

        std::sort(newNeighbors[static_cast<size_t>(newU)].begin(),
                  newNeighbors[static_cast<size_t>(newU)].end());
        newNeighbors[static_cast<size_t>(newU)].erase(
            std::unique(newNeighbors[static_cast<size_t>(newU)].begin(),
                        newNeighbors[static_cast<size_t>(newU)].end()),
            newNeighbors[static_cast<size_t>(newU)].end());
    }

    std::vector<uint8_t> visited(static_cast<size_t>(n), 0);
    std::vector<size_t> nextIdx(static_cast<size_t>(n), 0);
    std::stack<Index> st;

    // 新编号下：root 就是 rank=0 => newIndex=0
    Index root = 0;
    visited[static_cast<size_t>(root)] = 1;
    st.push(root);

    // 目标 preorder 在新编号下必须是 0,1,2,...,n-1
    for (int cur = 1; cur < n; ++cur)
    {
        Index target = static_cast<Index>(cur);
        bool matched = false;

        while (!st.empty())
        {
            Index curU = st.top();
            const auto &nbrs = newNeighbors[static_cast<size_t>(curU)];
            size_t &i = nextIdx[static_cast<size_t>(curU)];

            bool pushed = false;
            while (i < nbrs.size())
            {
                Index adj = nbrs[i++];
                if (adj < 0 || adj >= n)
                    continue;
                if (!visited[static_cast<size_t>(adj)])
                {
                    // traversal 的下一次 push 必然是这个 adj
                    if (adj != target)
                        return nullptr;

                    visited[static_cast<size_t>(adj)] = 1;
                    st.push(adj);
                    pushed = true;
                    matched = true;
                    break;
                }
            }

            if (pushed)
                break;

            // 当前点扫描完毕，回溯
            st.pop();
        }

        if (!matched)
            return nullptr;
    }

    for (int i = 0; i < n; ++i)
        if (!visited[static_cast<size_t>(i)])
            return nullptr;

    // 建图（矩阵采用重编号：index=rank位置）
    // 邻接矩阵的遍历优先级唯一由 Index 确定，将 index 置为 rank 即可
    auto newGraph = std::make_unique<AdjMatrixGraph>();
    newGraph->setLabel(graph.getLabel());

    for (int i = 0; i < n; ++i)
    {
        newGraph->addNode(Node(static_cast<Index>(i), accessRank[static_cast<size_t>(i)]));
    }

    // 加边：旧端点 -> 新端点
    for (int oldU = 0; oldU < n; ++oldU)
    {
        Index newU = oldToNew[static_cast<size_t>(oldU)];
        for (Index oldV : graph.getNeighbors(static_cast<Index>(oldU)))
        {
            if (oldV < 0 || oldV >= n)
                continue;
            Index newV = oldToNew[static_cast<size_t>(oldV)];
            if (newV < 0 || newV >= n)
                continue;

            newGraph->addEdge(newU, newV);
            newGraph->addEdge(newV, newU);
        }
    }

    return newGraph;
}

std::unique_ptr<Aggregate> MatrixDFS::rankSeeking(Graph &graph)
{
    class MatrixDFSRankIterator : public Iterator
    {
    public:
        explicit MatrixDFSRankIterator(std::vector<std::vector<std::string>> ranks)
            : ranks_(std::move(ranks)), cursor_(0) {}

        bool hasNext() override { return cursor_ < ranks_.size(); }

        void *next() override
        {
            if (!hasNext())
                return nullptr;
            return static_cast<void *>(&ranks_[cursor_++]);
        }

    private:
        std::vector<std::vector<std::string>> ranks_;
        std::size_t cursor_;
    };

    class MatrixDFSAggregate : public Aggregate
    {
    public:
        explicit MatrixDFSAggregate(std::vector<std::vector<std::string>> ranks)
            : iteratorImpl_(std::move(ranks)) {}

        Iterator &iterator() override { return iteratorImpl_; }

    private:
        MatrixDFSRankIterator iteratorImpl_;
    };

    auto ranks = RankSeeking::getBestRanksForDFS(graph);
    return std::make_unique<MatrixDFSAggregate>(std::move(ranks));
}

size_t MatrixDFS::traversal(const Graph &graph, Index root, std::vector<std::string> &accessRank)
{
    accessRank.clear();

    const int n = static_cast<int>(graph.getNodeCount());
    if (n == 0)
    {
        return 0;
    }
    if (root < 0 || root >= n)
    {
        return 0;
    }

    stack<Index> st;
    vector<uint8_t> visited(static_cast<size_t>(n), 0);
    vector<size_t> nextIdx(static_cast<size_t>(n), 0);

    size_t maxStackSize = 0;

    st.push(root);
    visited[static_cast<size_t>(root)] = 1;
    accessRank.push_back(graph.getNode(root).label);
    maxStackSize = max(maxStackSize, st.size());

    while (!st.empty())
    {
        Index cur = st.top();
        const vector<Index> neighbors = graph.getNeighbors(cur);

        bool pushed = false;
        size_t &i = nextIdx[static_cast<size_t>(cur)];
        while (i < neighbors.size())
        {
            Index adj = neighbors[i++];
            if (adj < 0 || adj >= n)
            {
                continue;
            }
            if (!visited[static_cast<size_t>(adj)])
            {
                visited[static_cast<size_t>(adj)] = 1;
                st.push(adj);
                accessRank.push_back(graph.getNode(adj).label);
                maxStackSize = max(maxStackSize, st.size());
                pushed = true;
                break;
            }
        }

        if (!pushed)
        {
            st.pop();
        }
    }

    return maxStackSize;
}

std::unique_ptr<Aggregate> MatrixDFS::reGraph(const Graph &graph)
{
    return make_unique<MatrixDFSReGraphAggregate>(graph);
}