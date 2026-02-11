// MatrixBFS.cpp
#include "MatrixBFS.hpp"
#include <string>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>
#include <cstdint>
#include "AdjMatrixGraph.hpp"
#include <memory>
#include "RankSeeking.hpp"
#include <Constants.hpp>
#include <numeric>
#include <random>

using namespace std;

namespace
{
    class MatrixBFSReGraphIterator final : public Iterator
    {
    public:
        explicit MatrixBFSReGraphIterator(const Graph &graph)
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

    class MatrixBFSReGraphAggregate final : public Aggregate
    {
    public:
        explicit MatrixBFSReGraphAggregate(const Graph &graph) : iterator_(graph) {}
        Iterator &iterator() override { return iterator_; }

    private:
        MatrixBFSReGraphIterator iterator_;
    };
} // namespace

std::unique_ptr<Graph> MatrixBFS::construction(const Graph &graph,
                                               const std::vector<std::string> accessRank)
{
    // 合法检查
    if (graph.getNodeCount() == 0)
        return nullptr;
    if (accessRank.size() != graph.getNodeCount())
        return nullptr;

    unordered_set<string> labelsMapped;
    vector<Index> rankToIndex; // rankToIndex[newPos] = oldIndex
    rankToIndex.reserve(accessRank.size());

    for (auto &label : accessRank)
    {
        if (!labelsMapped.insert(label).second)
            return nullptr;
        Index index = graph.getNode(label).index;
        rankToIndex.push_back(index);
    }

    const int n = static_cast<int>(graph.getNodeCount());

    // oldToNew[oldIndex] = newIndex(=rank position)
    vector<Index> oldToNew(static_cast<size_t>(n), -1);
    for (int i = 0; i < n; ++i)
    {
        Index oldIdx = rankToIndex[static_cast<size_t>(i)];
        if (oldIdx < 0 || oldIdx >= n)
            return nullptr;
        if (oldToNew[static_cast<size_t>(oldIdx)] != -1)
            return nullptr;
        oldToNew[static_cast<size_t>(oldIdx)] = static_cast<Index>(i);
    }

    // BFS 合法性检查（基于“存在某种邻接顺序可实现”）：复刻 reorderBfs 的集合检查
    vector<uint8_t> visited(static_cast<size_t>(n), 0);
    queue<Index> qu;

    Index root = rankToIndex[0];
    visited[static_cast<size_t>(root)] = 1;
    qu.push(root);
    int pos = 1;

    while (!qu.empty())
    {
        Index u = qu.front();
        qu.pop();

        vector<Index> unvisited;
        for (Index v : graph.getNeighbors(u))
        {
            if (!visited[static_cast<size_t>(v)])
                unvisited.push_back(v);
        }

        const int need = static_cast<int>(unvisited.size());
        if (pos + need > n)
            return nullptr;

        vector<Index> expected;
        expected.reserve(static_cast<size_t>(need));
        for (int i = 0; i < need; ++i)
        {
            expected.push_back(rankToIndex[static_cast<size_t>(pos + i)]);
        }

        auto unvisitedSorted = unvisited;
        auto expectedSorted = expected;
        sort(unvisitedSorted.begin(), unvisitedSorted.end());
        sort(expectedSorted.begin(), expectedSorted.end());
        if (unvisitedSorted != expectedSorted)
            return nullptr;

        for (Index v : expected)
        {
            visited[static_cast<size_t>(v)] = 1;
            qu.push(v);
        }
        pos += need;
    }

    if (pos != n)
        return nullptr;

    // 开始建图（矩阵无法“按节点重排邻接顺序”，因此采用重编号：index=rank位置）
    unique_ptr<AdjMatrixGraph> newGraph = make_unique<AdjMatrixGraph>();
    newGraph->setLabel(graph.getLabel());

    for (int i = 0; i < n; ++i)
    {
        newGraph->addNode(Node(static_cast<Index>(i), accessRank[static_cast<size_t>(i)]));
    }

    // 加边：把旧端点翻译为新端点
    for (int oldU = 0; oldU < n; ++oldU)
    {
        Index newU = oldToNew[static_cast<size_t>(oldU)];
        for (Index oldV : graph.getNeighbors(static_cast<Index>(oldU)))
        {
            Index newV = oldToNew[static_cast<size_t>(oldV)];
            newGraph->addEdge(newU, newV);
            newGraph->addEdge(newV, newU); // 无向
        }
    }

    return newGraph;
}

std::unique_ptr<Aggregate> MatrixBFS::rankSeeking(Graph &graph)
{
    class MatrixBFSRankIterator : public Iterator
    {
    public:
        explicit MatrixBFSRankIterator(std::vector<std::vector<std::string>> ranks)
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

    class MatrixBFSAggregate : public Aggregate
    {
    public:
        explicit MatrixBFSAggregate(std::vector<std::vector<std::string>> ranks)
            : iteratorImpl_(std::move(ranks)) {}

        Iterator &iterator() override { return iteratorImpl_; }

    private:
        MatrixBFSRankIterator iteratorImpl_;
    };

    auto ranks = RankSeeking::getBestRanksForBFS(graph);
    return std::make_unique<MatrixBFSAggregate>(std::move(ranks));
}

size_t MatrixBFS::traversal(const Graph &graph, Index root, std::vector<std::string> &accessRank)
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

    queue<Index> qu;
    vector<uint8_t> visited(static_cast<size_t>(n), 0);

    size_t maxQueueSize = 0;

    qu.push(root);
    visited[static_cast<size_t>(root)] = 1;
    accessRank.push_back(graph.getNode(root).label);
    maxQueueSize = max(maxQueueSize, qu.size());

    while (!qu.empty())
    {
        Index cur = qu.front();
        qu.pop();

        for (Index adj : graph.getNeighbors(cur))
        {
            if (adj < 0 || adj >= n)
            {
                continue;
            }
            if (!visited[static_cast<size_t>(adj)])
            {
                visited[static_cast<size_t>(adj)] = 1;
                qu.push(adj);
                accessRank.push_back(graph.getNode(adj).label);
                maxQueueSize = max(maxQueueSize, qu.size());
            }
        }
    }

    return maxQueueSize;
}

std::unique_ptr<Aggregate> MatrixBFS::reGraph(const Graph& graph) {
    return make_unique<MatrixBFSReGraphAggregate>(graph);
}