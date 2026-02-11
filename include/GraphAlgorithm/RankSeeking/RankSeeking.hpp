#pragma once

#include <string>
#include <vector>
#include "Graph.hpp"
#include <cstdint>

class RankSeeking {
public:
    static std::vector<std::vector<std::string>> getBestRanksForDFS(const Graph& graph);
    static std::vector<std::vector<std::string>> getBestRanksForBFS(const Graph& graph);
    
private:
    static std::vector<std::vector<std::string>> findOptimalOrdersPendingDFS(
        Graph& graph,
        std::size_t maxSolutions = 20,
        std::uint64_t timeLimitMs = 5000
    );

    static std::vector<std::vector<std::string>> findOptimalOrdersPendingBFS(
        Graph& graph,
        std::size_t maxSolutions = 20,
        std::uint64_t timeLimitMs = 5000
    );

    static std::size_t simulateGreedyUpperBoundPendingDFS(
        const std::vector<std::vector<int>>& adj,
        const std::vector<int>& deg,
        int root,
        std::vector<int>& outOrder
    );

    static std::size_t simulateGreedyUpperBoundPendingBFS(
        const std::vector<std::vector<int>>& adj,
        const std::vector<int>& deg,
        int root,
        std::vector<int>& outOrder
    );
};