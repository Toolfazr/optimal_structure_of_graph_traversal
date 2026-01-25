#pragma once
#include "Node.hpp"

// 图的根节点
inline constexpr Index ROOT = 0;

// 大度节点判定阈值：d(v) >= mean + k * std
// k 越大，判定越严格
constexpr double HIGH_DEGREE_K = 1.5;

// 当MetricsResStorage的size达到 2500时，写入一次
constexpr size_t FLUSH_CONTROL = 2500;

// 图节点数量小于等于该值时认为是小规模图
constexpr size_t SMALL_SCALE = 9;

// 小规模图的最大排列数
constexpr int MAX_PERM_NUM = 9*8*7*6*5*4*3*2;