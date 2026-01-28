// Construction.cpp
#include "Construction.hpp"

#include <algorithm>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

namespace
{

    struct LabelMapping
    {
        std::unordered_map<std::string, Index> labelToIndex;
        std::vector<Node> nodes;
        bool valid = true;
    };

    template <typename GraphT>
    LabelMapping buildLabelMapping(const GraphT &g)
    {
        LabelMapping mapping;
        const int n = static_cast<int>(g.getNodeCount());
        mapping.nodes.resize(static_cast<std::size_t>(n));

        std::unordered_set<std::string> seenLabels;
        for (int i = 0; i < n; ++i)
        {
            Node node = g.getNode(static_cast<Index>(i));
            if (node.index < 0)
            {
                mapping.valid = false;
                return mapping;
            }
            if (!seenLabels.insert(node.label).second)
            {
                mapping.valid = false;
                return mapping;
            }
            mapping.nodes[static_cast<std::size_t>(i)] = node;
            mapping.labelToIndex.emplace(node.label, node.index);
        }
        return mapping;
    }

    void buildListNodesFromRank(const AdjListGraph &g,
                                const std::vector<std::string> &rank,
                                AdjListGraph &out)
    {
        out = AdjListGraph();
        out.setLabel(g.getLabel());
        for (int i = 0; i < static_cast<int>(rank.size()); ++i)
        {
            out.addNode(Node(static_cast<Index>(i), rank[static_cast<std::size_t>(i)]));
        }
    }

    void buildListEdgesFromNeighbors(const std::vector<std::vector<Index>> &neighbors,
                                     const std::vector<Index> &oldToNew,
                                     AdjListGraph &out)
    {
        const int n = static_cast<int>(neighbors.size());
        for (int u = 0; u < n; ++u)
        {
            Index newU = oldToNew[static_cast<std::size_t>(u)];
            for (Index v : neighbors[static_cast<std::size_t>(u)])
            {
                Index newV = oldToNew[static_cast<std::size_t>(v)];
                out.addEdge(newU, newV);
            }
        }
    }
    bool buildRankIndex(const std::vector<std::string> &rank,
                        const std::unordered_map<std::string, Index> &labelToIndex,
                        std::vector<Index> &rankIdx)
    {
        rankIdx.clear();
        rankIdx.reserve(rank.size());

        std::unordered_set<std::string> seenRankLabels;
        for (const auto &label : rank)
        {
            if (!seenRankLabels.insert(label).second)
            {
                return false;
            }
            auto it = labelToIndex.find(label);
            if (it == labelToIndex.end())
            {
                return false;
            }
            rankIdx.push_back(it->second);
        }
        return true;
    }

    template <typename GraphT>
    void copyGraphNodes(const GraphT &g,
                        const std::vector<Node> &nodes,
                        GraphT &out)
    {
        out = GraphT();
        out.setLabel(g.getLabel());
        for (const auto &node : nodes)
        {
            out.addNode(node);
        }
    }

    template <typename GraphT>
    std::vector<std::vector<Index>> buildNeighborOrder(const GraphT &g)
    {
        const int n = static_cast<int>(g.getNodeCount());
        std::vector<std::vector<Index>> neighbors(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i)
        {
            neighbors[static_cast<std::size_t>(i)] = g.getNeighbors(static_cast<Index>(i));
        }
        return neighbors;
    }

    template <typename GraphT>
    void buildOutEdges(const std::vector<std::vector<Index>> &neighbors,
                       GraphT &out)
    {
        const int n = static_cast<int>(neighbors.size());
        for (int u = 0; u < n; ++u)
        {
            for (Index v : neighbors[static_cast<std::size_t>(u)])
            {
                out.addEdge(static_cast<Index>(u), v);
            }
        }
    }

    std::vector<Index> buildOldToNewMapping(const std::vector<Index> &rankIdx)
    {
        const int n = static_cast<int>(rankIdx.size());
        std::vector<Index> oldToNew(static_cast<std::size_t>(n), -1);
        for (int i = 0; i < n; ++i)
        {
            oldToNew[static_cast<std::size_t>(rankIdx[static_cast<std::size_t>(i)])] = static_cast<Index>(i);
        }
        return oldToNew;
    }

    void buildMatrixNodesFromRank(const AdjMatrixGraph &g,
                                  const std::vector<std::string> &rank,
                                  AdjMatrixGraph &out)
    {
        out = AdjMatrixGraph();
        out.setLabel(g.getLabel());
        for (int i = 0; i < static_cast<int>(rank.size()); ++i)
        {
            out.addNode(Node(static_cast<Index>(i), rank[static_cast<std::size_t>(i)]));
        }
    }

    void buildMatrixEdgesFromNeighbors(const std::vector<std::vector<Index>> &neighbors,
                                       const std::vector<Index> &oldToNew,
                                       AdjMatrixGraph &out)
    {
        const int n = static_cast<int>(neighbors.size());
        for (int u = 0; u < n; ++u)
        {
            Index newU = oldToNew[static_cast<std::size_t>(u)];
            for (Index v : neighbors[static_cast<std::size_t>(u)])
            {
                Index newV = oldToNew[static_cast<std::size_t>(v)];
                out.addEdge(newU, newV);
            }
        }
    }

    bool reorderBfs(const std::vector<Index> &rankIdx,
                    std::vector<std::vector<Index>> &neighbors)
    {
        const int n = static_cast<int>(neighbors.size());
        if (n == 0)
            return true;

        std::vector<uint8_t> visited(static_cast<std::size_t>(n), 0);
        std::queue<Index> qu;

        Index root = rankIdx[0];
        visited[static_cast<std::size_t>(root)] = 1;
        qu.push(root);
        int cur = 1;

        while (!qu.empty())
        {
            Index u = qu.front();
            qu.pop();

            std::vector<Index> unvisited;
            for (Index v : neighbors[static_cast<std::size_t>(u)])
            {
                if (!visited[static_cast<std::size_t>(v)])
                {
                    unvisited.push_back(v);
                }
            }

            const int need = static_cast<int>(unvisited.size());
            if (cur + need > n)
            {
                return false;
            }

            std::vector<Index> expected;
            expected.reserve(static_cast<std::size_t>(need));
            for (int i = 0; i < need; ++i)
            {
                expected.push_back(rankIdx[static_cast<std::size_t>(cur + i)]);
            }

            std::vector<Index> unvisitedSorted = unvisited;
            std::vector<Index> expectedSorted = expected;
            std::sort(unvisitedSorted.begin(), unvisitedSorted.end());
            std::sort(expectedSorted.begin(), expectedSorted.end());
            if (unvisitedSorted != expectedSorted)
            {
                return false;
            }

            std::vector<Index> reordered;
            reordered.reserve(neighbors[static_cast<std::size_t>(u)].size());
            for (Index v : expected)
            {
                reordered.push_back(v);
            }
            for (Index v : neighbors[static_cast<std::size_t>(u)])
            {
                if (visited[static_cast<std::size_t>(v)])
                {
                    reordered.push_back(v);
                }
            }
            neighbors[static_cast<std::size_t>(u)] = std::move(reordered);

            for (Index v : expected)
            {
                visited[static_cast<std::size_t>(v)] = 1;
                qu.push(v);
            }
            cur += need;
        }

        return cur == n;
    }

    bool reorderDfs(const std::vector<Index> &rankIdx,
                    std::vector<std::vector<Index>> &neighbors)
    {
        const int n = static_cast<int>(neighbors.size());
        if (n == 0)
            return true;

        // pos 用于可选的稳定排序（不是必须）
        std::vector<int> pos(static_cast<std::size_t>(n), -1);
        for (int i = 0; i < n; ++i)
        {
            pos[static_cast<std::size_t>(rankIdx[static_cast<std::size_t>(i)])] = i;
        }

        auto hasUnvisitedNeighbor = [&](Index u, const std::vector<uint8_t> &visited) -> bool
        {
            for (Index v : neighbors[static_cast<std::size_t>(u)])
            {
                if (!visited[static_cast<std::size_t>(v)])
                    return true;
            }
            return false;
        };

        auto isUnvisitedNeighbor = [&](Index u, Index target, const std::vector<uint8_t> &visited) -> bool
        {
            if (visited[static_cast<std::size_t>(target)])
                return false;
            for (Index v : neighbors[static_cast<std::size_t>(u)])
            {
                if (v == target)
                    return true;
            }
            return false;
        };

        // 记录 DFS 树孩子顺序：childOrder[u] = u 的孩子按发现顺序排列
        std::vector<std::vector<Index>> childOrder(static_cast<std::size_t>(n));
        std::vector<uint8_t> visited(static_cast<std::size_t>(n), 0);
        std::stack<Index> st;

        Index root = rankIdx[0];
        visited[static_cast<std::size_t>(root)] = 1;
        st.push(root);

        // 按 rank 强制重放“标准 Path-DFS 的发现序(preorder)”
        for (int cur = 1; cur < n; ++cur)
        {
            Index target = rankIdx[static_cast<std::size_t>(cur)];

            // 回溯：只有当栈顶已无未访问邻居时才能 pop
            while (!st.empty() && !isUnvisitedNeighbor(st.top(), target, visited))
            {
                Index u = st.top();
                if (hasUnvisitedNeighbor(u, visited))
                {
                    // DFS 不会在还有未访问邻居时回溯，因此该 rank 非法
                    return false;
                }
                st.pop();
            }
            if (st.empty())
                return false;

            Index parent = st.top();
            // target 必须是 parent 的未访问邻居
            if (!isUnvisitedNeighbor(parent, target, visited))
                return false;

            childOrder[static_cast<std::size_t>(parent)].push_back(target);
            visited[static_cast<std::size_t>(target)] = 1;
            st.push(target);
        }

        // 所有节点都应已访问
        for (int i = 0; i < n; ++i)
        {
            if (!visited[static_cast<std::size_t>(i)])
                return false;
        }

        // 一次性构造静态邻接表顺序：孩子在前，其它邻居在后
        for (int u = 0; u < n; ++u)
        {
            const auto &kids = childOrder[static_cast<std::size_t>(u)];
            if (kids.empty())
                continue;

            std::vector<uint8_t> isKid(static_cast<std::size_t>(n), 0);
            for (Index v : kids)
                isKid[static_cast<std::size_t>(v)] = 1;

            std::vector<Index> reordered;
            reordered.reserve(neighbors[static_cast<std::size_t>(u)].size());

            // 1) DFS-tree 孩子按发现顺序
            for (Index v : kids)
                reordered.push_back(v);

            // 2) 其它邻居放后面（顺序保持原有插入序；也可以按 pos 排序）
            for (Index v : neighbors[static_cast<std::size_t>(u)])
            {
                if (!isKid[static_cast<std::size_t>(v)])
                    reordered.push_back(v);
            }

            neighbors[static_cast<std::size_t>(u)] = std::move(reordered);
        }

        return true;
    }

} // namespace

bool Construction::reorderListForBFS(const AdjListGraph &g,
                                     const std::vector<std::string> &rank,
                                     AdjListGraph &out)
{
    const int n = static_cast<int>(g.getNodeCount());
    if (static_cast<int>(rank.size()) != n)
    {
        return false;
    }

    LabelMapping mapping = buildLabelMapping(g);
    if (!mapping.valid)
    {
        return false;
    }

    std::vector<Index> rankIdx;
    if (!buildRankIndex(rank, mapping.labelToIndex, rankIdx))
    {
        return false;
    }

    std::vector<std::vector<Index>> neighbors = buildNeighborOrder(g);
    if (!reorderBfs(rankIdx, neighbors))
    {
        return false;
    }

    std::vector<Index> oldToNew = buildOldToNewMapping(rankIdx);
    buildListNodesFromRank(g, rank, out);
    buildListEdgesFromNeighbors(neighbors, oldToNew, out);
    return true;
}

bool Construction::reorderListForDFS(const AdjListGraph &g,
                                     const std::vector<std::string> &rank,
                                     AdjListGraph &out)
{
    const int n = static_cast<int>(g.getNodeCount());
    if (static_cast<int>(rank.size()) != n)
    {
        return false;
    }

    LabelMapping mapping = buildLabelMapping(g);
    if (!mapping.valid)
    {
        return false;
    }

    std::vector<Index> rankIdx;
    if (!buildRankIndex(rank, mapping.labelToIndex, rankIdx))
    {
        return false;
    }

    std::vector<std::vector<Index>> neighbors = buildNeighborOrder(g);
    if (!reorderDfs(rankIdx, neighbors))
    {
        return false;
    }

    std::vector<Index> oldToNew = buildOldToNewMapping(rankIdx);
    buildListNodesFromRank(g, rank, out);
    buildListEdgesFromNeighbors(neighbors, oldToNew, out);
    return true;
}

bool Construction::reorderMatrixForBFS(const AdjMatrixGraph &g,
                                       const std::vector<std::string> &rank,
                                       AdjMatrixGraph &out)
{
    const int n = static_cast<int>(g.getNodeCount());
    if (static_cast<int>(rank.size()) != n)
    {
        return false;
    }

    LabelMapping mapping = buildLabelMapping(g);
    if (!mapping.valid)
    {
        return false;
    }

    std::vector<Index> rankIdx;
    if (!buildRankIndex(rank, mapping.labelToIndex, rankIdx))
    {
        return false;
    }

    std::vector<std::vector<Index>> neighbors = buildNeighborOrder(g);
    if (!reorderBfs(rankIdx, neighbors))
    {
        return false;
    }

    std::vector<Index> oldToNew = buildOldToNewMapping(rankIdx);
    buildMatrixNodesFromRank(g, rank, out);
    buildMatrixEdgesFromNeighbors(neighbors, oldToNew, out);
    return true;
}

bool Construction::reorderMatrixForDFS(const AdjMatrixGraph &g,
                                       const std::vector<std::string> &rank,
                                       AdjMatrixGraph &out)
{
    const int n = static_cast<int>(g.getNodeCount());
    if (static_cast<int>(rank.size()) != n)
    {
        return false;
    }

    LabelMapping mapping = buildLabelMapping(g);
    if (!mapping.valid)
    {
        return false;
    }

    std::vector<Index> rankIdx;
    if (!buildRankIndex(rank, mapping.labelToIndex, rankIdx))
    {
        return false;
    }

    std::vector<std::vector<Index>> neighbors = buildNeighborOrder(g);
    if (!reorderDfs(rankIdx, neighbors))
    {
        return false;
    }

    std::vector<Index> oldToNew = buildOldToNewMapping(rankIdx);
    buildMatrixNodesFromRank(g, rank, out);
    buildMatrixEdgesFromNeighbors(neighbors, oldToNew, out);
    return true;
}