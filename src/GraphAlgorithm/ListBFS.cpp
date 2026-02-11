#include "ListBFS.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "AdjListGraph.hpp"
#include <algorithm>
#include <limits>
#include "RankSeeking.hpp"
#include <numeric>
#include <Constants.hpp>
#include <random>

using namespace std;

namespace
{
    class ListBFSReGraphIterator final : public Iterator
    {
    public:
        explicit ListBFSReGraphIterator(const Graph &graph)
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
                const size_t target = static_cast<size_t>(MAX_PERM_NUM);
                randomTarget_ = target;
                const uint64_t rt = static_cast<uint64_t>(target);
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

            unique_ptr<AdjListGraph> graph = make_unique<AdjListGraph>();
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

    class ListBFSReGraphAggregate final : public Aggregate
    {
    public:
        explicit ListBFSReGraphAggregate(const Graph &graph) : iterator_(graph) {}
        Iterator &iterator() override { return iterator_; }

    private:
        ListBFSReGraphIterator iterator_;
    };
} // namespace
std::unique_ptr<Graph> ListBFS::construction(const Graph &graph, const std::vector<std::string> accessRank)
{
    // 合法检查
    if (graph.getNodeCount() == 0)
        return nullptr;
    if (accessRank.size() != graph.getNodeCount())
        return nullptr;

    unordered_set<string> labelsMapped; // 通过labelsMapped判定是否访问重复节点
    vector<Index> rankToIndex;          // 获取访问秩对应的index访问顺序
    for (auto &label : accessRank)
    {
        if (!labelsMapped.insert(label).second)
        {
            return nullptr;
        }
        Index index = graph.getNode(label).index;
        rankToIndex.push_back(index);
    }

    // 调整Node的邻接节点Index顺序, 使其符合访问秩
    unordered_map<Index, bool> visited;
    vector<vector<Index>> adjNodes; // 调整完毕的、用Node.index表示的节点邻接关系
    adjNodes.resize(graph.getNodeCount());
    for (Index index : rankToIndex)
    {
        visited.insert({index, false});
    }
    queue<Index> qu;
    Index root = rankToIndex[0];
    qu.push(root);
    visited[root] = true;
    size_t pos = 1;
    while (!qu.empty())
    {
        Index u = qu.front();
        qu.pop();
        // 调整邻接未访问节点的Index顺序,邻接已访问的节点肯定处理其它节点时已调整过了, 且不影响当前节点的访问
        std::vector<Index> unvisited;
        for (Index v : graph.getNeighbors(u))
        {
            if (!visited[v])
            {
                unvisited.push_back(v);
            }
        }

        const size_t need = unvisited.size();
        if (pos + need > accessRank.size())
            return nullptr;

        // 获取rankToIndex[cur]节点调整后的邻接节点Index顺序
        vector<Index> expected;
        expected.reserve(static_cast<size_t>(need));
        for (int i = 0; i < need; i++)
        {
            expected.push_back(rankToIndex[pos + i]);
        }

        // 进行判断: 如果排序后未访问节点序列与访问秩片段不是identical的，那么访问秩非法
        vector<Index> unvisitedSorted = unvisited;
        vector<Index> expectedSorted = expected;
        sort(unvisitedSorted.begin(), unvisitedSorted.end());
        sort(expectedSorted.begin(), expectedSorted.end());
        if (unvisitedSorted != expectedSorted)
        {
            return nullptr;
        }

        // 将调整好的邻接节点Index存入adjNodes
        vector<Index> reordered;
        Node curNode = graph.getNode(u);
        reordered.reserve(graph.getNeighbors(curNode.index).size());
        for (Index v : expected)
        {
            reordered.push_back(v);
        }
        for (Index v : graph.getNeighbors(curNode.index))
        {
            if (visited[v])
            {
                reordered.push_back(v);
            }
        }
        adjNodes[u] = move(reordered);

        for (Index v : expected)
        {
            visited[v] = true;
            qu.push(v);
        }

        pos += need;
    }

    if (pos != accessRank.size())
        return nullptr;

    // 开始建图
    unique_ptr<AdjListGraph> newGraph = make_unique<AdjListGraph>();
    newGraph->setLabel(graph.getLabel());

    // 向图中加节点
    for (int i = 0; i < accessRank.size(); i++)
    {
        Node newNode(graph.getNode(accessRank[i]).index, accessRank[i]);
        newGraph->addNode(newNode);
    }

    // 向图中加边
    for (int i = 0; i < accessRank.size(); i++)
    {
        Index u = graph.getNode(accessRank[i]).index;
        for (Index v : adjNodes[u])
        {
            newGraph->addEdge(u, v);
            newGraph->addEdge(v, u);
        }
    }

    return newGraph;
}

std::unique_ptr<Aggregate> ListBFS::rankSeeking(Graph &graph)
{
    class ListBFSRankIterator : public Iterator
    {
    public:
        explicit ListBFSRankIterator(std::vector<std::vector<std::string>> ranks)
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

    class ListBFSAggregate : public Aggregate
    {
    public:
        explicit ListBFSAggregate(std::vector<std::vector<std::string>> ranks)
            : iteratorImpl_(std::move(ranks)) {}

        Iterator &iterator() override { return iteratorImpl_; }

    private:
        ListBFSRankIterator iteratorImpl_;
    };

    auto ranks = RankSeeking::getBestRanksForBFS(graph);
    return std::make_unique<ListBFSAggregate>(std::move(ranks));
}

size_t ListBFS::traversal(const Graph &graph, Index root, std::vector<std::string> &accessRank)
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

std::unique_ptr<Aggregate> ListBFS::reGraph(const Graph& graph) {
    return make_unique<ListBFSReGraphAggregate>(graph);
}