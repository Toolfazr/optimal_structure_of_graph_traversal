#pragma once

#include "Generator.hpp"
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <functional>
#include <queue>
#include <type_traits>

class RandomGen : public Generator
{
public:
    RandomGen();
    virtual ~RandomGen() = default;
    virtual bool parseCmd(GenCmd cmd) override;
    virtual std::unique_ptr<Graph> genGraph() override;

private:
    size_t n;
    double p;
    std::string structureType;
    using GraphFactory = std::function<std::unique_ptr<Graph>()>;
    std::unordered_map<std::string, GraphFactory> factory;

    template <class G>
    std::unique_ptr<Graph> makeGraph(size_t n, double p)
    {
        static_assert(std::is_base_of_v<Graph, G>,
                      "G must derive from Graph");

        std::random_device rd;
        std::mt19937 gen(rd());
        std::bernoulli_distribution dist(p);

        auto buildGraph = [&]() -> std::unique_ptr<G>
        {
            auto g = std::make_unique<G>();

            for (size_t i = 0; i < n; ++i)
            {
                g->addNode(Node(static_cast<Index>(i), std::to_string(i)));
            }

            for (size_t i = 0; i < n; ++i)
            {
                for (size_t j = i + 1; j < n; ++j)
                {
                    if (dist(gen))
                    {
                        g->addEdge(static_cast<Index>(i), static_cast<Index>(j));
                        g->addEdge(static_cast<Index>(j), static_cast<Index>(i));
                    }
                }
            }
            return g;
        };

        auto isConnected = [&](const Graph &g)
        {
            if (n == 1)
                return true;

            std::vector<bool> visited(n, false);
            std::queue<Index> q;
            q.push(0);
            visited[0] = true;
            size_t visitedCount = 1;

            while (!q.empty())
            {
                Index u = q.front();
                q.pop();
                for (Index v : g.getNeighbors(u))
                {
                    if (!visited[v])
                    {
                        visited[v] = true;
                        ++visitedCount;
                        q.push(v);
                    }
                }
            }
            return visitedCount == n;
        };

        std::unique_ptr<G> g;
        do
        {
            g = buildGraph();
        } while (!isConnected(*g));

        return g; // implicit upcast to unique_ptr<Graph>
    }
};