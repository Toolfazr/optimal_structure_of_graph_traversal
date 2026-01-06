#pragma once

#include <vector>
#include <numeric>
#include <algorithm>
#include "Node.hpp"
#include "Utility.hpp" 

class ReGraph {
public:
    template <class G>
    class Enumerator {
    public:
        explicit Enumerator(const G& graph);

        // 生成一个新的重标号图；成功返回 true；若所有排列已枚举完返回 false
        bool next(G& out);

        // 当前排列：perm[oldId] = newId
        const std::vector<Index>& currentPerm() const { return perm_; }

        bool done() const { return done_; }

    private:
        int n_ = 0;
        bool started_ = false;
        bool done_ = true;

        std::vector<std::vector<Index>> originalAdj_;
        std::vector<Node> originalNodes_;

        // perm_[old] = new
        std::vector<Index> perm_;
    };

    // 小规模场景：一次性收集所有重标号图
    template <class G>
    static std::vector<G> reGraphAll(const G& graph) {
        std::vector<G> res;
        Enumerator<G> it(graph);
        G tmp;
        while (it.next(tmp)) res.push_back(tmp);
        return res;
    }
};

// -------------------- Template implementation --------------------

template <class G>
ReGraph::Enumerator<G>::Enumerator(const G& graph) {
    n_ = graph.getNodeCount();
    if (n_ <= 0) {
        done_ = true;
        return;
    }

    Utility::extractGraphInfo(graph, originalAdj_, originalNodes_);

    perm_.resize(n_);
    std::iota(perm_.begin(), perm_.end(), static_cast<Index>(0));

    started_ = false;
    done_ = false;
}

template <class G>
bool ReGraph::Enumerator<G>::next(G& out) {
    if (done_) return false;

    if (!started_) {
        started_ = true; // 第一次用 identity perm
    } else {
        if (!std::next_permutation(perm_.begin(), perm_.end())) {
            done_ = true;
            return false;
        }
    }

    // invrs[newId] = oldId
    std::vector<Index> invrs(n_);
    for (int oldId = 0; oldId < n_; ++oldId) {
        Index newId = perm_[oldId];
        invrs[static_cast<int>(newId)] = static_cast<Index>(oldId);
    }

    // 构造新图
    G newGraph;

    // 加节点
    for (int newId = 0; newId < n_; ++newId) {
        Index oldId = invrs[newId];
        Node newNode(static_cast<Index>(newId), originalNodes_[static_cast<int>(oldId)].label);
        newGraph.addNode(newNode);
    }

    // 加边
    for (int newId = 0; newId < n_; ++newId) {
        Index oldId = invrs[newId];
        for (Index adjOld : originalAdj_[static_cast<int>(oldId)]) {
            Index adjNew = perm_[static_cast<int>(adjOld)];
            newGraph.addEdge(static_cast<Index>(newId), adjNew);
            newGraph.addEdge(adjNew, static_cast<Index>(newId));
        }
    }

    out = std::move(newGraph);
    return true;
}
