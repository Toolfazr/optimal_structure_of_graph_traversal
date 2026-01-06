#pragma once

#include <string>

typedef int Index;

class Node {
public:
    Node() = delete;
    Node(Index _index, std::string _label = "") : index(_index), label(_label) {}
    Index index; // 访问Node的句柄
    std::string label; // 标识Node的标签
};

inline bool operator==(const Node& a, const Node& b) noexcept {
    return a.index == b.index;
}

// 基于node的index哈希
struct NodeHash {
    size_t operator()(const Node& n) const noexcept {
        return std::hash<Index>()(n.index);
    }
};