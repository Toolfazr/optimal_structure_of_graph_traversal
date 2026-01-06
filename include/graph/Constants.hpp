#pragma once
#include "Node.hpp"

// 默认所有图的遍历从Index = 0的节点开始
inline constexpr Index ROOT = 0;

// 大度节点判定阈值：d(v) >= mean + k * std
// k 越大，判定越严格
constexpr double HIGH_DEGREE_K = 1.5;

// 当MetricsResStorage的size达到 2500时，写入一次
constexpr size_t FLUSH_CONTROL = 2500;
