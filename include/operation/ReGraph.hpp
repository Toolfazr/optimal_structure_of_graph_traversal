#pragma once

#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <cstdint>
#include <limits>

#include "Node.hpp"
#include "Constants.hpp"
#include "Utility.hpp"

class ReGraph {
public:
    template <class G>
    class Enumerator {
    public:
        explicit Enumerator(const G& graph);

        // 确定性枚举（仅支持小 n）
        bool next(G& out);

        // 随机生成不重复的重排图
        bool nextRandom(G& out);

        // 当前排列：perm[oldId] = newId
        const std::vector<Index>& currentPerm() const { return perm_; }

        bool done() const { return done_; }

    private:
        int n_ = 0;
        bool started_ = false;
        bool done_ = true;

        bool randomInitialized_ = false;
        size_t randomGenerated_ = 0;
        size_t randomTarget_ = 0;
        size_t attemptsLimit_ = 0;
        bool useRankSampling_ = false;
        uint64_t totalPerms_ = 0;

        std::vector<std::vector<Index>> originalAdj_;
        std::vector<Node> originalNodes_;

        // perm_[old] = new
        std::vector<Index> perm_;

        std::mt19937 rng_{std::random_device{}()};
        std::vector<uint64_t> factorials_;
        std::unordered_set<uint64_t> seenRanks_;
        std::unordered_set<uint64_t> seenHashes_;

        bool buildGraphFromPerm(const std::vector<Index>& perm, G& out);
        uint64_t computeTotalPermutations() const;
        std::vector<uint64_t> computeFactorials() const;
        std::vector<Index> permFromRank(uint64_t rank) const;
        uint64_t hashPermutation(const std::vector<Index>& perm) const;
    };

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
    if (n_ >= 10) return false;
    if (done_) return false;

    if (!started_) {
        started_ = true;
    } else {
        if (!std::next_permutation(perm_.begin(), perm_.end())) {
            done_ = true;
            return false;
        }
    }

    return buildGraphFromPerm(perm_, out);
}

template <class G>
bool ReGraph::Enumerator<G>::nextRandom(G& out) {
    if (n_ <= 0) return false;

    if (!randomInitialized_) {
        totalPerms_ = computeTotalPermutations();

        randomTarget_ = static_cast<size_t>(MAX_PERM_NUM);
        randomGenerated_ = 0;

        // attemptsLimit_ = max(randomTarget_ * 50, 100)  (防溢出 + 上限夹逼)
        {
            const uint64_t rtU64 = static_cast<uint64_t>(randomTarget_);
            uint64_t attemptsU64;
            if (rtU64 > std::numeric_limits<uint64_t>::max() / 50ull) {
                attemptsU64 = std::numeric_limits<uint64_t>::max();
            } else {
                attemptsU64 = rtU64 * 50ull;
            }
            if (attemptsU64 < 100ull) attemptsU64 = 100ull;

            const uint64_t sizeMaxU64 = static_cast<uint64_t>(std::numeric_limits<size_t>::max());
            attemptsLimit_ = static_cast<size_t>(std::min<uint64_t>(attemptsU64, sizeMaxU64));
        }

        useRankSampling_ = (n_ <= 20);

        if (useRankSampling_) {
            factorials_ = computeFactorials();
            seenRanks_.clear();

            // reserve(randomTarget_ * 2) 防溢出
            size_t reserveN = randomTarget_;
            if (reserveN > std::numeric_limits<size_t>::max() / 2) {
                reserveN = std::numeric_limits<size_t>::max();
            } else {
                reserveN *= 2;
            }
            seenRanks_.reserve(reserveN);
        } else {
            seenHashes_.clear();

            // reserve(randomTarget_ * 2) 防溢出
            size_t reserveN = randomTarget_;
            if (reserveN > std::numeric_limits<size_t>::max() / 2) {
                reserveN = std::numeric_limits<size_t>::max();
            } else {
                reserveN *= 2;
            }
            seenHashes_.reserve(reserveN);
        }

        randomInitialized_ = true;
    }

    if (randomGenerated_ >= randomTarget_) return false;

    size_t attempts = 0;
    while (attempts < attemptsLimit_) {
        if (useRankSampling_) {
            std::uniform_int_distribution<uint64_t> dist(0, totalPerms_ - 1);
            uint64_t rank = dist(rng_);
            if (seenRanks_.insert(rank).second) {
                std::vector<Index> perm = permFromRank(rank);
                ++randomGenerated_;
                return buildGraphFromPerm(perm, out);
            }
        } else {
            std::vector<Index> perm(n_);
            std::iota(perm.begin(), perm.end(), static_cast<Index>(0));
            std::shuffle(perm.begin(), perm.end(), rng_);
            uint64_t h = hashPermutation(perm);
            if (seenHashes_.insert(h).second) {
                ++randomGenerated_;
                return buildGraphFromPerm(perm, out);
            }
        }
        ++attempts;
    }
    return false;
}


template <class G>
uint64_t ReGraph::Enumerator<G>::computeTotalPermutations() const {
    uint64_t total = 1;
    for (int i = 2; i <= n_; ++i) {
        if (total > std::numeric_limits<uint64_t>::max() / static_cast<uint64_t>(i)) {
            return std::numeric_limits<uint64_t>::max();
        }
        total *= static_cast<uint64_t>(i);
    }
    return total;
}

template <class G>
std::vector<uint64_t> ReGraph::Enumerator<G>::computeFactorials() const {
    std::vector<uint64_t> f(static_cast<size_t>(n_) + 1, 1);
    for (int i = 2; i <= n_; ++i) {
        f[static_cast<size_t>(i)] =
            f[static_cast<size_t>(i - 1)] * static_cast<uint64_t>(i);
    }
    return f;
}

template <class G>
std::vector<Index> ReGraph::Enumerator<G>::permFromRank(uint64_t rank) const {
    std::vector<Index> elements(n_);
    std::iota(elements.begin(), elements.end(), static_cast<Index>(0));

    std::vector<Index> perm;
    perm.reserve(n_);

    for (int i = n_; i >= 1; --i) {
        uint64_t fact = factorials_[static_cast<size_t>(i - 1)];
        uint64_t idx = rank / fact;
        rank %= fact;
        perm.push_back(elements[static_cast<size_t>(idx)]);
        elements.erase(elements.begin() + static_cast<std::ptrdiff_t>(idx));
    }
    return perm;
}

template <class G>
uint64_t ReGraph::Enumerator<G>::hashPermutation(const std::vector<Index>& perm) const {
    uint64_t h = 1469598103934665603ull;
    for (Index v : perm) {
        h ^= static_cast<uint64_t>(v) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        h *= 1099511628211ull;
    }
    return h;
}

template <class G>
bool ReGraph::Enumerator<G>::buildGraphFromPerm(const std::vector<Index>& perm, G& out) {
    std::vector<Index> invrs(n_);
    for (int oldId = 0; oldId < n_; ++oldId) {
        Index newId = perm[oldId];
        invrs[static_cast<int>(newId)] = static_cast<Index>(oldId);
    }

    G newGraph;

    for (int newId = 0; newId < n_; ++newId) {
        Index oldId = invrs[newId];
        Node newNode(static_cast<Index>(newId),
                     originalNodes_[static_cast<int>(oldId)].label);
        newGraph.addNode(newNode);
    }

    for (int newId = 0; newId < n_; ++newId) {
        Index oldId = invrs[newId];
        for (Index adjOld : originalAdj_[static_cast<int>(oldId)]) {
            Index adjNew = perm[static_cast<int>(adjOld)];
            newGraph.addEdge(static_cast<Index>(newId), adjNew);
            newGraph.addEdge(adjNew, static_cast<Index>(newId));
        }
    }

    out = std::move(newGraph);
    return true;
}
