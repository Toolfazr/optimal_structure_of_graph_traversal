#include "Decomposition.hpp"

#include <algorithm>
#include <queue>
#include <utility>

namespace
{
    std::vector<Index> getNeighborsFiltered(const Graph &g, Index u, const std::vector<char> &allowed)
    {
        std::vector<Index> neighbors = g.getNeighbors(u);
        std::vector<Index> filtered;
        filtered.reserve(neighbors.size());
        for (Index v : neighbors)
        {
            if (v >= 0 && static_cast<size_t>(v) < allowed.size() && allowed[v])
            {
                filtered.push_back(v);
            }
        }
        std::sort(filtered.begin(), filtered.end());
        return filtered;
    }

    struct BfsResult
    {
        Index farthest = -1;
        std::vector<int> dist;
        std::vector<Index> parent;
    };

    BfsResult bfsFarthest(const Graph &g, Index start, const std::vector<char> &allowed)
    {
        BfsResult result;
        result.dist.assign(allowed.size(), -1);
        result.parent.assign(allowed.size(), -1);

        std::queue<Index> q;
        result.dist[start] = 0;
        q.push(start);
        result.farthest = start;

        while (!q.empty())
        {
            Index u = q.front();
            q.pop();
            std::vector<Index> neighbors = getNeighborsFiltered(g, u, allowed);
            for (Index v : neighbors)
            {
                if (result.dist[v] != -1)
                {
                    continue;
                }
                result.dist[v] = result.dist[u] + 1;
                result.parent[v] = u;
                q.push(v);
                if (result.dist[v] > result.dist[result.farthest] || (result.dist[v] == result.dist[result.farthest] && v < result.farthest))
                {
                    result.farthest = v;
                }
            }
        }
        return result;
    }

    std::vector<Index> buildMainChain(const Graph &g, const std::vector<Index> &nodes, const std::vector<char> &allowed)
    {
        Index start = nodes.front();
        BfsResult first = bfsFarthest(g, start, allowed);
        Index a = first.farthest;
        BfsResult second = bfsFarthest(g, a, allowed);
        Index b = second.farthest;

        std::vector<Index> path;
        for (Index cur = b; cur != -1; cur = second.parent[cur])
        {
            path.push_back(cur);
            if (cur == a)
            {
                break;
            }
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    std::vector<std::vector<Index>> getComponents(const Graph &g, const std::vector<Index> &nodes)
    {
        if (nodes.empty())
        {
            return {};
        }
        size_t total = static_cast<size_t>(*std::max_element(nodes.begin(), nodes.end())) + 1;
        std::vector<char> allowed(total, false);
        for (Index u : nodes)
        {
            if (u >= 0 && static_cast<size_t>(u) < allowed.size())
            {
                allowed[u] = true;
            }
        }

        std::vector<char> visited(allowed.size(), false);
        std::vector<std::pair<Index, std::vector<Index>>> components;

        for (Index start : nodes)
        {
            if (!allowed[start] || visited[start])
            {
                continue;
            }
            std::vector<Index> component;
            std::queue<Index> q;
            q.push(start);
            visited[start] = true;

            while (!q.empty())
            {
                Index u = q.front();
                q.pop();
                component.push_back(u);
                std::vector<Index> neighbors = getNeighborsFiltered(g, u, allowed);
                for (Index v : neighbors)
                {
                    if (visited[v])
                    {
                        continue;
                    }
                    visited[v] = true;
                    q.push(v);
                }
            }
            std::sort(component.begin(), component.end());
            components.emplace_back(component.front(), component);
        }

        std::sort(components.begin(), components.end(), [](const auto &a, const auto &b)
                  { return a.first < b.first; });

        std::vector<std::vector<Index>> result;
        result.reserve(components.size());
        for (auto &entry : components)
        {
            result.push_back(std::move(entry.second));
        }
        return result;
    }

    std::vector<std::vector<Index>> decompose(const Graph &g, const std::vector<Index> &nodes, const std::vector<char> &allowed)
    {
        if (nodes.empty())
        {
            return {{}};
        }
        if (nodes.size() == 1)
        {
            return {{nodes.front()}};
        }

        size_t n = allowed.size();
        std::vector<int> degree(n, 0);
        for (Index u : nodes)
        {
            std::vector<Index> neighbors = getNeighborsFiltered(g, u, allowed);
            degree[u] = static_cast<int>(neighbors.size());
        }

        std::vector<Index> mainChain = buildMainChain(g, nodes, allowed);
        std::vector<char> inMainChain(n, false);
        for (Index v : mainChain)
        {
            inMainChain[v] = true;
        }

        std::vector<std::vector<Index>> sideSets;
        sideSets.reserve(mainChain.size());
        size_t bmax = 0;
        for (Index v : mainChain)
        {
            std::vector<Index> neighbors = getNeighborsFiltered(g, v, allowed);
            std::vector<Index> side;
            for (Index u : neighbors)
            {
                if (!inMainChain[u])
                {
                    side.push_back(u);
                }
            }
            std::sort(side.begin(), side.end(), [&](Index a, Index b)
                      {
            if (degree[a] != degree[b]) {
                return degree[a] < degree[b];
            }
            return a < b; });
            bmax = std::max(bmax, side.size());
            sideSets.push_back(std::move(side));
        }

        std::vector<std::vector<Index>> allOrders;
        for (size_t k = 0; k <= bmax; ++k)
        {
            std::vector<char> inVCG = inMainChain;
            std::vector<std::vector<Index>> leavesPerNode;
            leavesPerNode.reserve(sideSets.size());

            for (const auto &side : sideSets)
            {
                std::vector<Index> leaves;
                leaves.reserve(std::min(k, side.size()));

                for (Index u : side)
                {
                    if (leaves.size() >= k)
                    {
                        break;
                    }
                    // u may appear in multiple sideSets (common in cyclic / dense graphs).
                    // Ensure each node is selected at most once into the VCG.
                    if (inVCG[u])
                    {
                        continue;
                    }
                    leaves.push_back(u);
                    inVCG[u] = true;
                }

                leavesPerNode.push_back(std::move(leaves));
            }

            std::vector<Index> currentOrder;
            currentOrder.reserve(nodes.size());
            for (size_t i = 0; i < mainChain.size(); ++i)
            {
                currentOrder.push_back(mainChain[i]);
                for (Index leaf : leavesPerNode[i])
                {
                    currentOrder.push_back(leaf);
                }
            }

            std::vector<Index> remaining;
            remaining.reserve(nodes.size());
            for (Index u : nodes)
            {
                if (!inVCG[u])
                {
                    remaining.push_back(u);
                }
            }

            std::vector<std::vector<Index>> components = getComponents(g, remaining);
            std::vector<std::vector<std::vector<Index>>> componentOrders;
            componentOrders.reserve(components.size());

            for (const auto &component : components)
            {
                std::vector<char> componentAllowed(n, false);
                for (Index u : component)
                {
                    componentAllowed[u] = true;
                }
                componentOrders.push_back(decompose(g, component, componentAllowed));
            }

            std::vector<std::vector<Index>> combined;
            combined.push_back(currentOrder);
            for (const auto &subOrders : componentOrders)
            {
                std::vector<std::vector<Index>> next;
                for (const auto &prefix : combined)
                {
                    for (const auto &sub : subOrders)
                    {
                        std::vector<Index> merged = prefix;
                        merged.insert(merged.end(), sub.begin(), sub.end());
                        next.push_back(std::move(merged));
                    }
                }
                combined = std::move(next);
            }

            for (auto &order : combined)
            {
                allOrders.push_back(std::move(order));
            }
        }

        return allOrders;
    }
} // namespace

std::vector<std::vector<std::string>> Decomposition::getRanks(const Graph &g)
{
    size_t count = g.getNodeCount();
    std::vector<Index> nodes;
    nodes.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        nodes.push_back(static_cast<Index>(i));
    }
    std::vector<char> allowed(count, true);
    auto res = decompose(g, nodes, allowed);
    std::vector<std::vector<std::string>> ranks;
    ranks.reserve(res.size());

    for (const auto &r : res)
    {
        std::vector<std::string> sr;
        sr.reserve(r.size());
        for (Index x : r)
        {
            sr.emplace_back(std::to_string(x));
        }
        ranks.emplace_back(std::move(sr));
    }

    return ranks;
}
