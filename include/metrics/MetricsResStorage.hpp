#pragma once

#include <vector>
#include <string>
#include "Metrics.hpp"

class MetricsResStorage {
public:
    MetricsResStorage() = default;

    void clear();
    void merge(MetricsResStorage& another);
    // 参数顺序：bfsMaxQueue, dfsMaxStack, bfsHDS, dfsHDS, bfsBS, dfsBS, label
    bool append(
        RootOptResult _bfsMaxQueue,
        RootOptResult _dfsMaxStack,
        double _bfsHDS,
        double _dfsHDS,
        double _bfsBS,
        double _dfsBS,
        std::string _label
    );
    const std::vector<RootOptResult>& getDfsMaxStack() const;
    const std::vector<RootOptResult>& getBfsMaxQueue() const;
    const std::vector<double>& getBfsHDS() const;
    const std::vector<double>& getDfsHDS() const;
    const std::vector<double>& getBfsBS() const;
    const std::vector<double>& getDfsBS() const;
    const std::vector<std::string>& getLabel() const;
    size_t getResGroupSize() const;
private:
    size_t resGroupSize = 0;
    std::vector<RootOptResult> dfsMaxStack;
    std::vector<RootOptResult> bfsMaxQueue;
    std::vector<double> bfsHDS;
    std::vector<double> dfsHDS;
    std::vector<double> bfsBS;
    std::vector<double> dfsBS;
    std::vector<std::string> label;
};