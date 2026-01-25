#pragma once

#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"
#include <random>
#include <string>
#include <stdexcept>
#include <queue>
#include <stack>
// Graph generator utilities for experiments (undirected graphs are created by adding edges in both directions).
// Node ids are 0..n-1 and labels are stringified ids.

class GraphGen
{
public:
    // Random graph G(n, p) conditioned on connectivity (simple, undirected).
    template <class G>
    static G makeGraph(size_t n, double p)
    {
        if (n == 0)
        {
            throw std::invalid_argument("makeGraph: n must be > 0");
        }
        if (p < 0.0 || p > 1.0)
        {
            throw std::invalid_argument("makeGraph: p must be between 0 and 1");
        }
        if (n > 1 && p == 0.0)
        {
            throw std::invalid_argument("makeGraph: p must be > 0 when n > 1");
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::bernoulli_distribution dist(p);

        auto buildGraph = [&]()
        {
            G g;
            for (size_t i = 0; i < n; ++i)
            {
                g.addNode(Node(static_cast<Index>(i), std::to_string(i)));
            }

            for (size_t i = 0; i < n; ++i)
            {
                for (size_t j = i + 1; j < n; ++j)
                {
                    if (dist(gen))
                    {
                        g.addEdge(static_cast<Index>(i), static_cast<Index>(j));
                        g.addEdge(static_cast<Index>(j), static_cast<Index>(i));
                    }
                }
            }
            return g;
        };

        auto isConnected = [&](const G &g)
        {
            if (n == 1)
            {
                return true;
            }
            std::vector<bool> visited(n, false);
            std::queue<Index> queue;
            queue.push(static_cast<Index>(0));
            visited[0] = true;
            size_t visitedCount = 1;

            while (!queue.empty())
            {
                Index u = queue.front();
                queue.pop();
                for (Index v : g.getNeighbors(u))
                {
                    if (!visited[v])
                    {
                        visited[v] = true;
                        ++visitedCount;
                        queue.push(v);
                    }
                }
            }

            return visitedCount == n;
        };

        G g;
        do
        {
            g = buildGraph();
        } while (!isConnected(g));

        return g;
    }

    // Star graph: center 0 connected to all 1..n-1
    static AdjListGraph makeStarAdjList(int n);
    static AdjMatrixGraph makeStarAdjMatrix(int n);

    // Grid graph: w x h, 4-neighbor (up/down/left/right). id = r*w + c
    static AdjListGraph makeGridAdjList(int w, int h);
    static AdjMatrixGraph makeGridAdjMatrix(int w, int h);

    // Clique + Tail (lollipop): cliqueSize nodes fully connected (0..cliqueSize-1),
    // then a path of length tailLen attached to (cliqueSize-1).
    static AdjListGraph makeCliqueTailAdjList(int cliqueSize, int tailLen);
    static AdjMatrixGraph makeCliqueTailAdjMatrix(int cliqueSize, int tailLen);

    // Binary tree in array form: for node i, children are 2i+1 and 2i+2 if < n.
    static AdjListGraph makeBinaryTreeAdjList(int n);
    static AdjMatrixGraph makeBinaryTreeAdjMatrix(int n);
};
