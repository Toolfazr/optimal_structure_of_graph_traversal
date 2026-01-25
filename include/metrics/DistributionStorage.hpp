#pragma once
#include <map>
#include <vector>
#include <string>
#include "Graph.hpp"
#include "Node.hpp"

class DistributionStorage {
public:
    DistributionStorage() = default;
    void insert(std::vector<std::string> accessRank, size_t maxSize);
    const std::map<size_t, std::vector<std::vector<std::string>>>& getDistribution();
    void clear();
    void toCsv(const std::string& path) const;
    unsigned long long size();
private:
    std::map<size_t, std::vector<std::vector<std::string>>> distribution;
    unsigned long long accessRankNum = 0;
};