// RankSeeking.cpp
#include "RankSeeking.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

// n<=63 use uint64 bitmask
static inline std::uint64_t bit(int i) { return 1ull << static_cast<unsigned>(i); }

struct VecHash {
    std::size_t operator()(const std::vector<int>& v) const noexcept {
        std::size_t h = 1469598103934665603ull;
        for (int x : v) {
            h ^= static_cast<std::size_t>(x) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        }
        return h;
    }
};

struct OrderHash {
    std::size_t operator()(const std::vector<std::string>& v) const noexcept {
        std::size_t h = 0;
        for (auto const& s : v) {
            std::size_t hs = std::hash<std::string>{}(s);
            h ^= hs + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        }
        return h;
    }
};

} // namespace

// ======================== Public APIs ========================

std::vector<std::vector<std::string>> RankSeeking::getBestRanksForDFS(const Graph& graph)
{
    // 内部算法只读 graph，但接口是 Graph&；这里做 const_cast
    auto& g = const_cast<Graph&>(graph);
    return findOptimalOrdersPendingDFS(g, /*maxSolutions*/ 50, /*timeLimitMs*/ 10000);
}

std::vector<std::vector<std::string>> RankSeeking::getBestRanksForBFS(const Graph& graph)
{
    auto& g = const_cast<Graph&>(graph);
    return findOptimalOrdersPendingBFS(g, /*maxSolutions*/ 50, /*timeLimitMs*/ 10000);
}

// ======================== Pending-DFS (stack) ========================

// ======================== Pending-BFS (queue) ========================

std::vector<std::vector<std::string>> RankSeeking::findOptimalOrdersPendingBFS(
    Graph& graph,
    std::size_t maxSolutions,
    std::uint64_t timeLimitMs)
{
    const int n = graph.getNodeCount();
    std::vector<std::vector<std::string>> result;
    if (n <= 0) return result;
    if (n > 63) return result;

    std::vector<std::vector<int>> adj(n);
    std::vector<int> deg(n, 0);

    for (int i = 0; i < n; ++i) {
        auto ns = graph.getNeighbors(static_cast<Index>(i));
        adj[i].reserve(ns.size());
        for (auto x : ns) {
            int v = static_cast<int>(x);
            if (0 <= v && v < n) adj[i].push_back(v);
        }
        deg[i] = static_cast<int>(adj[i].size());
    }

    const std::uint64_t fullMask = (1ull << n) - 1ull;

    const auto t0 = std::chrono::steady_clock::now();
    auto timeout = [&]() -> bool {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
        return static_cast<std::uint64_t>(ms) > timeLimitMs;
    };

    std::size_t globalBest = std::numeric_limits<std::size_t>::max();
    std::unordered_set<std::vector<std::string>, OrderHash> uniq;

    struct StateKey {
        std::uint64_t visited;
        std::vector<int> qu; // front..back
        bool operator==(const StateKey& o) const noexcept { return visited == o.visited && qu == o.qu; }
    };
    struct StateHash {
        std::size_t operator()(StateKey const& k) const noexcept {
            std::size_t h = std::hash<std::uint64_t>{}(k.visited);
            std::size_t hq = VecHash{}(k.qu);
            h ^= hq + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            return h;
        }
    };

    for (int root = 0; root < n; ++root) {
        if (timeout()) break;

        std::vector<int> greedyOrder;
        std::size_t bestPeak = simulateGreedyUpperBoundPendingBFS(adj, deg, root, greedyOrder);

        if (bestPeak > globalBest) continue;

        std::vector<std::vector<int>> localSols;
        localSols.reserve(maxSolutions);

        std::unordered_map<StateKey, std::size_t, StateHash> memo;
        memo.reserve(1 << 14);

        std::vector<int> curOrder;
        curOrder.reserve(n);

        std::function<void(std::uint64_t, std::vector<int>&, std::size_t)> dfs =
            [&](std::uint64_t visited, std::vector<int>& qu, std::size_t peakSoFar) {
                if (timeout()) return;
                // 允许探索 peakSoFar == bestPeak 的分支，以枚举“并列最优”解
                if (peakSoFar > bestPeak) return;

                if (qu.empty()) {
                    if (visited == fullMask) {
                        if (peakSoFar < bestPeak) {
                            bestPeak = peakSoFar;
                            localSols.clear();
                            localSols.push_back(curOrder);
                        } else if (peakSoFar == bestPeak && localSols.size() < maxSolutions) {
                            localSols.push_back(curOrder);
                        }
                    }
                    return;
                }

                StateKey key{visited, qu};
                auto it = memo.find(key);
                // 做“最优值”搜索时可用 >= 剪掉重复状态；但枚举解会丢掉不同前缀。
                // 因此这里只剪掉“更差”的到达方式：peakSoFar > bestSeen。
                if (it != memo.end() && peakSoFar > it->second) return;
                if (it == memo.end()) {
                    memo.emplace(std::move(key), peakSoFar);
                } else {
                    // 保留该状态的最好 peak，用于后续剪枝
                    it->second = std::min(it->second, peakSoFar);
                }

                int cur = qu.front();
                qu.erase(qu.begin());
                curOrder.push_back(cur);

                std::vector<int> U;
                U.reserve(adj[cur].size());
                for (int nb : adj[cur]) if ((visited & bit(nb)) == 0) U.push_back(nb);

                if (U.empty()) {
                    dfs(visited, qu, peakSoFar);
                    curOrder.pop_back();
                    qu.insert(qu.begin(), cur);
                    return;
                }

                // 强下界：pop 后必 push |U| => 队列至少变为 qu.size()+|U|
                {
                    std::size_t lb = std::max<std::size_t>(peakSoFar, qu.size() + U.size());
                    // lb == bestPeak 仍可能产生并列最优解
                    if (lb > bestPeak) {
                        curOrder.pop_back();
                        qu.insert(qu.begin(), cur);
                        return;
                    }
                }

                const std::uint64_t U_mask = (~visited) & fullMask;

                // signature 去对称
                struct Sig { int d; std::uint64_t m; int v; };
                std::vector<Sig> sigs;
                sigs.reserve(U.size());
                for (int v : U) {
                    std::uint64_t m = 0;
                    for (int w : adj[v]) if (U_mask & bit(w)) m |= bit(w);
                    sigs.push_back({deg[v], m, v});
                }
                std::sort(sigs.begin(), sigs.end(), [](const Sig& a, const Sig& b) {
                    if (a.d != b.d) return a.d < b.d;
                    if (a.m != b.m) return a.m < b.m;
                    return a.v < b.v;
                });

                std::vector<int> base;
                base.reserve(U.size());
                for (auto& s : sigs) base.push_back(s.v);

                auto expToUnvisited = [&](int v) -> int {
                    int c = 0;
                    for (int w : adj[v]) if (U_mask & bit(w)) ++c;
                    return c;
                };

                std::vector<std::vector<int>> perms;
                if (base.size() <= 7) {
                    std::sort(base.begin(), base.end());
                    std::unordered_set<std::string> seenSig;
                    seenSig.reserve(512);

                    do {
                        std::string sigKey;
                        sigKey.reserve(base.size() * 24);
                        for (int v : base) {
                            std::uint64_t m = 0;
                            for (int w : adj[v]) if (U_mask & bit(w)) m |= bit(w);
                            sigKey.append(std::to_string(deg[v]));
                            sigKey.push_back('#');
                            sigKey.append(std::to_string(m));
                            sigKey.push_back('|');
                        }
                        if (seenSig.insert(sigKey).second) perms.push_back(base);
                    } while (std::next_permutation(base.begin(), base.end()));
                } else {
                    std::vector<int> p1 = base;
                    std::sort(p1.begin(), p1.end(), [&](int a, int b) {
                        int ea = expToUnvisited(a), eb = expToUnvisited(b);
                        if (ea != eb) return ea < eb; // BFS：小扩张先入队
                        if (deg[a] != deg[b]) return deg[a] < deg[b];
                        return a < b;
                    });
                    perms.push_back(std::move(p1));
                }

                // 优先探索“扩张力小先入队”
                std::sort(perms.begin(), perms.end(), [&](const std::vector<int>& A, const std::vector<int>& B) {
                    for (std::size_t i = 0; i < std::min(A.size(), B.size()); ++i) {
                        int ea = expToUnvisited(A[i]);
                        int eb = expToUnvisited(B[i]);
                        if (ea != eb) return ea < eb;
                    }
                    return A.size() < B.size();
                });

                const std::size_t baseSize = qu.size();
                for (auto const& perm : perms) {
                    std::uint64_t vmask = visited;
                    std::size_t peak = peakSoFar;

                    qu.resize(baseSize);
                    for (int v : perm) {
                        vmask |= bit(v);
                        qu.push_back(v);
                        peak = std::max<std::size_t>(peak, qu.size());
                        if (peak > bestPeak) break;
                    }
                    if (peak <= bestPeak) dfs(vmask, qu, peak);

                    if (timeout()) break;
                }

                qu.resize(baseSize);
                curOrder.pop_back();
                qu.insert(qu.begin(), cur);
            };

        std::vector<int> qu;
        qu.push_back(root);
        dfs(bit(root), qu, 1);

        if (localSols.empty()) continue;

        if (bestPeak < globalBest) {
            globalBest = bestPeak;
            result.clear();
            uniq.clear();
        }
        if (bestPeak == globalBest) {
            for (auto const& sol : localSols) {
                std::vector<std::string> labels;
                labels.reserve(sol.size());
                for (int id : sol) labels.push_back(graph.getNode(static_cast<Index>(id)).label);
                if (uniq.insert(labels).second) {
                    result.push_back(std::move(labels));
                    if (result.size() >= maxSolutions) break;
                }
            }
        }
        if (result.size() >= maxSolutions) break;
    }

    return result;
}

// ===== DFS(pending-stack) greedy upper bound =====
// LIFO：把“扩张力大”的先 push（更靠栈底，后处理），通常更利于降低堆积峰值

// ===== BFS(pending-queue) greedy upper bound =====
// FIFO：把“扩张力小”的先入队（先消化），把“扩张力大”的后入队（延后消化）
std::size_t RankSeeking::simulateGreedyUpperBoundPendingBFS(
    const std::vector<std::vector<int>>& adj,
    const std::vector<int>& deg,
    int root,
    std::vector<int>& outOrder)
{
    const int n = static_cast<int>(adj.size());
    const std::uint64_t fullMask = (1ull << n) - 1ull;

    std::uint64_t visited = bit(root);
    std::deque<int> qu;
    qu.push_back(root);

    std::size_t peak = 1;
    outOrder.clear();
    outOrder.reserve(n);

    while (!qu.empty()) {
        int cur = qu.front();
        qu.pop_front();
        outOrder.push_back(cur);

        const std::uint64_t U_mask = (~visited) & fullMask;

        std::vector<int> U;
        U.reserve(adj[cur].size());
        for (int nb : adj[cur]) {
            if ((visited & bit(nb)) == 0) U.push_back(nb);
        }
        if (U.empty()) continue;

        auto expToUnvisited = [&](int v) -> int {
            int c = 0;
            for (int w : adj[v]) if (U_mask & bit(w)) ++c;
            return c;
        };

        std::sort(U.begin(), U.end(), [&](int a, int b) {
            int ea = expToUnvisited(a), eb = expToUnvisited(b);
            if (ea != eb) return ea < eb;
            if (deg[a] != deg[b]) return deg[a] < deg[b];
            return a < b;
        });

        for (int v : U) {
            visited |= bit(v);
            qu.push_back(v);
            peak = std::max<std::size_t>(peak, qu.size());
        }
    }
    return peak;
}

// RankSeeking.cpp

// ======================== Path-DFS (stack) ========================

std::vector<std::vector<std::string>> RankSeeking::findOptimalOrdersPendingDFS(
    Graph& graph,
    std::size_t maxSolutions,
    std::uint64_t timeLimitMs)
{
    const int n = graph.getNodeCount();
    std::vector<std::vector<std::string>> result;
    if (n <= 0) return result;
    if (n > 63) return result;

    std::vector<std::vector<int>> adj(n);
    std::vector<int> deg(n, 0);

    for (int i = 0; i < n; ++i) {
        auto ns = graph.getNeighbors(static_cast<Index>(i));
        adj[i].reserve(ns.size());
        for (auto x : ns) {
            int v = static_cast<int>(x);
            if (0 <= v && v < n) adj[i].push_back(v);
        }
        deg[i] = static_cast<int>(adj[i].size());
    }

    const std::uint64_t fullMask = (1ull << n) - 1ull;

    const auto t0 = std::chrono::steady_clock::now();
    auto timeout = [&]() -> bool {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
        return static_cast<std::uint64_t>(ms) > timeLimitMs;
    };

    std::size_t globalBest = std::numeric_limits<std::size_t>::max();
    std::unordered_set<std::vector<std::string>, OrderHash> uniq;

    struct StateKey {
        std::uint64_t visited;
        std::vector<int> st; // bottom..top (path stack)
        bool operator==(const StateKey& o) const noexcept { return visited == o.visited && st == o.st; }
    };
    struct StateHash {
        std::size_t operator()(StateKey const& k) const noexcept {
            std::size_t h = std::hash<std::uint64_t>{}(k.visited);
            std::size_t hs = VecHash{}(k.st);
            h ^= hs + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            return h;
        }
    };

    for (int root = 0; root < n; ++root) {
        if (timeout()) break;

        std::vector<int> greedyOrder;
        std::size_t bestPeak = simulateGreedyUpperBoundPendingDFS(adj, deg, root, greedyOrder);

        if (bestPeak > globalBest) continue;

        std::vector<std::vector<int>> localSols;
        localSols.reserve(maxSolutions);

        std::unordered_map<StateKey, std::size_t, StateHash> memo;
        memo.reserve(1 << 14);

        std::vector<int> curOrder;
        curOrder.reserve(n);
        curOrder.push_back(root); // Path-DFS：order 为“发现顺序(preorder)”

        std::function<void(std::uint64_t, std::vector<int>&, std::size_t)> dfs =
            [&](std::uint64_t visited, std::vector<int>& st, std::size_t peakSoFar) {
                if (timeout()) return;
                // 允许探索 peakSoFar == bestPeak 的分支，以枚举“并列最优”解
                if (peakSoFar > bestPeak) return;

                if (st.empty()) {
                    if (visited == fullMask) {
                        if (peakSoFar < bestPeak) {
                            bestPeak = peakSoFar;
                            localSols.clear();
                            localSols.push_back(curOrder);
                        } else if (peakSoFar == bestPeak && localSols.size() < maxSolutions) {
                            localSols.push_back(curOrder);
                        }
                    }
                    return;
                }

                // Path-DFS：峰值下界至少为当前深度（==bestPeak 仍可能是最优解）
                if (st.size() > bestPeak) return;

                StateKey key{visited, st};
                auto it = memo.find(key);
                // 枚举解：不同前缀可能抵达同一(visited,st)。这里只剪掉“更差”的到达方式。
                if (it != memo.end() && peakSoFar > it->second) return;
                if (it == memo.end()) {
                    memo.emplace(std::move(key), peakSoFar);
                } else {
                    it->second = std::min(it->second, peakSoFar);
                }

                int cur = st.back();

                // U = unvisited neighbors of cur
                std::vector<int> U;
                U.reserve(adj[cur].size());
                for (int nb : adj[cur]) if ((visited & bit(nb)) == 0) U.push_back(nb);

                if (U.empty()) {
                    // 回溯
                    st.pop_back();
                    dfs(visited, st, peakSoFar);
                    st.push_back(cur);
                    return;
                }

                const std::uint64_t U_mask = (~visited) & fullMask;

                // signature 去对称：sig(v)=(deg[v], neighborMaskWithinUnvisited)
                struct Sig { int d; std::uint64_t m; int v; };
                std::vector<Sig> sigs;
                sigs.reserve(U.size());
                for (int v : U) {
                    std::uint64_t m = 0;
                    for (int w : adj[v]) if (U_mask & bit(w)) m |= bit(w);
                    sigs.push_back({deg[v], m, v});
                }
                std::sort(sigs.begin(), sigs.end(), [](const Sig& a, const Sig& b) {
                    if (a.d != b.d) return a.d < b.d;
                    if (a.m != b.m) return a.m < b.m;
                    return a.v < b.v;
                });

                // 只对不同 signature 的代表点分支（降低对称）
                std::vector<int> cand;
                cand.reserve(sigs.size());
                for (std::size_t i = 0; i < sigs.size(); ++i) {
                    if (i == 0 || sigs[i].d != sigs[i-1].d || sigs[i].m != sigs[i-1].m) {
                        cand.push_back(sigs[i].v);
                    }
                }

                // 启发式：优先选“扩张力小”的，倾向于早回溯从而控制深度
                auto expToUnvisited = [&](int v) -> int {
                    int c = 0;
                    for (int w : adj[v]) if (U_mask & bit(w)) ++c;
                    return c;
                };
                std::sort(cand.begin(), cand.end(), [&](int a, int b) {
                    int ea = expToUnvisited(a), eb = expToUnvisited(b);
                    if (ea != eb) return ea < eb;
                    if (deg[a] != deg[b]) return deg[a] < deg[b];
                    return a < b;
                });

                for (int v : cand) {
                    st.push_back(v);
                    curOrder.push_back(v);
                    const std::size_t peak2 = std::max<std::size_t>(peakSoFar, st.size());
                    dfs(visited | bit(v), st, peak2);
                    curOrder.pop_back();
                    st.pop_back();

                    if (timeout()) break;
                }
            };

        std::vector<int> st;
        st.push_back(root);
        dfs(bit(root), st, 1);

        if (localSols.empty()) continue;

        if (bestPeak < globalBest) {
            globalBest = bestPeak;
            result.clear();
            uniq.clear();
        }
        if (bestPeak == globalBest) {
            for (auto const& sol : localSols) {
                std::vector<std::string> labels;
                labels.reserve(sol.size());
                for (int id : sol) labels.push_back(graph.getNode(static_cast<Index>(id)).label);
                if (uniq.insert(labels).second) {
                    result.push_back(std::move(labels));
                    if (result.size() >= maxSolutions) break;
                }
            }
        }
        if (result.size() >= maxSolutions) break;
    }

    return result;
}

// ===== DFS(Path-DFS) greedy upper bound =====
// Path-DFS：每步只选择一个未访问邻居深入；栈表示“当前路径”。
std::size_t RankSeeking::simulateGreedyUpperBoundPendingDFS(
    const std::vector<std::vector<int>>& adj,
    const std::vector<int>& deg,
    int root,
    std::vector<int>& outOrder)
{
    const int n = static_cast<int>(adj.size());
    const std::uint64_t fullMask = (1ull << n) - 1ull;

    std::uint64_t visited = bit(root);
    std::vector<int> st;
    st.push_back(root);

    std::size_t peak = 1;
    outOrder.clear();
    outOrder.reserve(n);
    outOrder.push_back(root);

    while (!st.empty()) {
        int cur = st.back();

        // 从 cur 的未访问邻居中选一个继续深入；没有则回溯
        const std::uint64_t U_mask = (~visited) & fullMask;

        std::vector<int> U;
        U.reserve(adj[cur].size());
        for (int nb : adj[cur]) {
            if ((visited & bit(nb)) == 0) U.push_back(nb);
        }
        if (U.empty()) {
            st.pop_back();
            continue;
        }

        auto expToUnvisited = [&](int v) -> int {
            int c = 0;
            for (int w : adj[v]) if (U_mask & bit(w)) ++c;
            return c;
        };

        // 启发式：优先选“扩张力小”的，倾向于更快回溯、降低最大深度
        std::sort(U.begin(), U.end(), [&](int a, int b) {
            int ea = expToUnvisited(a), eb = expToUnvisited(b);
            if (ea != eb) return ea < eb;
            if (deg[a] != deg[b]) return deg[a] < deg[b];
            return a < b;
        });

        int v = U.front();
        visited |= bit(v);
        st.push_back(v);
        outOrder.push_back(v);
        peak = std::max<std::size_t>(peak, st.size());
    }
    return peak;
}