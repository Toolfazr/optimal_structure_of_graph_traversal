#include "Metrics.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <stack>
#include <unordered_set>
#include <unordered_map>

namespace
{
    // 存储图中节点度的相关信息, 用于判断是否存在大度节点
    struct DegreeStats
    {
        double mean = 0.0;
        double std = 0.0;
    };

    // 统计图中节点度的相关信息
    static DegreeStats degreeStats(const Graph &g)
    {
        DegreeStats s;
        const int n = static_cast<int>(g.getNodeCount());
        if (n <= 0)
            return s;

        std::vector<double> deg(n, 0.0);
        double sum = 0.0;
        for (int v = 0; v < n; ++v)
        {
            double d = static_cast<double>(g.getNeighbors(v).size());
            deg[v] = d;
            sum += d;
        }
        s.mean = sum / n;

        double var = 0.0;
        for (int v = 0; v < n; ++v)
        {
            double x = deg[v] - s.mean;
            var += x * x;
        }
        var /= n; // 总体方差
        s.std = std::sqrt(var);
        return s;
    }

    // 将节点的访问顺序变为访问秩
    // before: order[1] = 7, 第1个被访问的节点为节点7
    // after: pos[7] = 1, 节点7的访问秩为1(采用 0-base 访问秩)
    static std::vector<int> buildPos(const std::vector<Index> &order, std::size_t n)
    {
        std::vector<int> pos(n, -1);
        for (int i = 0; i < static_cast<int>(order.size()); ++i)
        {
            Index v = order[static_cast<std::size_t>(i)];
            if (v >= 0 && static_cast<std::size_t>(v) < n)
            {
                pos[static_cast<std::size_t>(v)] = i;
            }
        }
        return pos;
    }

    // 返回图中节点按度降序的排列
    static std::vector<std::pair<int, int>> degreesWithId(const Graph &g)
    {
        const int n = static_cast<int>(g.getNodeCount());
        std::vector<std::pair<int, int>> dv;
        dv.reserve(n);
        for (int v = 0; v < n; ++v)
        {
            dv.push_back({static_cast<int>(g.getNeighbors(v).size()), v});
        }
        std::sort(dv.begin(), dv.end(), [](const auto &a, const auto &b)
                  {
            if (a.first != b.first) return a.first > b.first; // degree desc
            return a.second < b.second; });
        return dv;
    }

    // 根据阈值选出大度节点 hub：d(v) >= mean + k*std
    static std::vector<Index> pickHubsMeanStd(const Graph &g)
    {
        std::vector<Index> hubs;
        const int n = static_cast<int>(g.getNodeCount());
        if (n <= 0)
            return hubs;

        DegreeStats st = degreeStats(g);
        const double th = st.mean + HIGH_DEGREE_K * st.std;

        for (int v = 0; v < n; ++v)
        {
            double d = static_cast<double>(g.getNeighbors(v).size());
            if (d >= th)
                hubs.push_back(static_cast<Index>(v));
        }
        return hubs;
    }

    // 线性归一化到大度节点的度数到[0,1]：若全相等则全置 1
    // hubs: 大度节点集合（Index 列表）
    // return: weight[v] = normalized degree in [0,1]
    static std::unordered_map<Index, double>
    normalizeDegreesOnSetMap(const Graph &g, const std::vector<Index> &hubs)
    {
        std::unordered_map<Index, double> w;
        if (hubs.empty())
            return w;

        int dmin = std::numeric_limits<int>::max();
        int dmax = std::numeric_limits<int>::min();

        for (Index v : hubs)
        {
            int d = static_cast<int>(g.getNeighbors(v).size());
            dmin = std::min(dmin, d);
            dmax = std::max(dmax, d);
        }

        if (dmax == dmin)
        {
            for (Index v : hubs)
                w[v] = 1.0;
            return w;
        }

        const double denom = static_cast<double>(dmax - dmin);
        for (Index v : hubs)
        {
            int d = static_cast<int>(g.getNeighbors(v).size());
            w[v] = (static_cast<double>(d) - dmin) / denom; // in [0,1]
        }
        return w;
    }

} // anonymous namespace

std::size_t Metrics::dfsMaxStackFromRoot(Graph &graph, Index root)
{
    const int n = graph.getNodeCount();
    if (n == 0)
        return 0;

    std::stack<Index> st;
    std::vector<uint8_t> visited(n, 0);

    std::size_t maxSize = 0;

    st.push(root);
    visited[root] = 1;
    maxSize = std::max(maxSize, st.size());

    while (!st.empty())
    {
        Index cur = st.top();
        st.pop();

        for (Index adj : graph.getNeighbors(cur))
        {
            if (adj < 0 || adj >= n)
                continue; // 若你的 Index 保证范围可删
            if (!visited[adj])
            {
                visited[adj] = 1;
                st.push(adj);
                maxSize = std::max(maxSize, st.size());
            }
        }
    }
    return maxSize;
}

// 单次：给定 root，测 BFS 最大队列
std::size_t Metrics::bfsMaxQueueFromRoot(Graph &graph, Index root)
{
    const int n = graph.getNodeCount();
    if (n == 0)
        return 0;

    std::queue<Index> qu;
    std::vector<uint8_t> visited(n, 0);

    std::size_t maxSize = 0;

    qu.push(root);
    visited[root] = 1;
    maxSize = std::max(maxSize, qu.size());

    while (!qu.empty())
    {
        Index cur = qu.front();
        qu.pop();

        for (Index adj : graph.getNeighbors(cur))
        {
            if (adj < 0 || adj >= n)
                continue;
            if (!visited[adj])
            {
                visited[adj] = 1;
                qu.push(adj);
                maxSize = std::max(maxSize, qu.size());
            }
        }
    }
    return maxSize;
}

RootOptResult Metrics::measureDFSMaxStack(Graph &graph)
{
    const int n = graph.getNodeCount();
    RootOptResult res;
    if (n == 0)
        return res;

    res.bestPeak = static_cast<std::size_t>(-1);
    res.bestRoots.clear();

    for (Index r = 0; r < n; ++r)
    {
        const std::size_t peak = dfsMaxStackFromRoot(graph, r);

        if (peak < res.bestPeak)
        {
            res.bestPeak = peak;
            res.bestRoots.assign(1, r);
        }
        else if (peak == res.bestPeak)
        {
            res.bestRoots.push_back(r);
        }
    }
    return res;
}

RootOptResult Metrics::measureBFSMaxQueue(Graph &graph)
{
    const int n = graph.getNodeCount();
    RootOptResult res;
    if (n == 0)
        return res;

    res.bestPeak = static_cast<std::size_t>(-1);
    res.bestRoots.clear();

    for (Index r = 0; r < n; ++r)
    {
        const std::size_t peak = bfsMaxQueueFromRoot(graph, r);

        if (peak < res.bestPeak)
        {
            res.bestPeak = peak;
            res.bestRoots.assign(1, r);
        }
        else if (peak == res.bestPeak)
        {
            res.bestRoots.push_back(r);
        }
    }
    return res;
}

// 计算大度节点间距
double Metrics::highDegreeSpacingImpl(const Graph &graph, const TraversalTrace &t)
{
    const int n = static_cast<int>(graph.getNodeCount());
    if (n <= 0 || t.order.size() < 2)
        return 0.0;

    // 访问秩
    std::vector<int> pos = buildPos(t.order, static_cast<std::size_t>(n));

    // Top-K：默认 K = ceil(sqrt(n))，至少2，至多n
    int K = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(n))));
    K = std::max(2, K);
    K = std::min(K, n);

    // 取度数 Top-K 节点
    auto dv = degreesWithId(graph); // (deg, id) 按 deg 降序
    std::vector<int> p;
    p.reserve(static_cast<std::size_t>(K));
    for (int i = 0; i < K; ++i)
    {
        int v = dv[static_cast<std::size_t>(i)].second;
        int pv = pos[static_cast<std::size_t>(v)];
        if (pv >= 0)
            p.push_back(pv); // 非连通时可能 -1，跳过
    }

    if (p.size() < 2)
        return -1.0; // 无法计算间距

    std::sort(p.begin(), p.end());

    const int K2 = static_cast<int>(p.size());
    if (K2 < 2)
        return -1.0;

    const double deltaStar = static_cast<double>(n - 1) / static_cast<double>(K2 - 1);
    if (deltaStar <= 0.0)
        return 1.0; // 退化情况，视为“最好”

    double sumAbs = 0.0;
    for (int i = 0; i + 1 < K2; ++i)
    {
        const int delta = p[i + 1] - p[i];
        // delta 理论上 > 0（因为 p 已排序且若有相等说明重复访问秩，通常不会发生）
        sumAbs += std::fabs(static_cast<double>(delta) - deltaStar);
    }

    const double hdsNorm = sumAbs / (static_cast<double>(K2 - 1) * deltaStar);
    return hdsNorm;
}

// 分支悬挂度：对每个 parent，收集其 children 的访问位置，
// span = max(childPos) - min(childPos)，对所有拥有 >=2 个 child 的 parent 取平均
double Metrics::branchSuspensionImpl(const TraversalTrace &t)
{
    const int n = static_cast<int>(t.parent.size());
    if (n <= 0)
        return 0.0;
    if (t.order.empty())
        return 0.0;

    auto pos = buildPos(t.order, static_cast<std::size_t>(n));

    std::vector<std::vector<int>> childPos(static_cast<std::size_t>(n));
    for (int v = 0; v < n; ++v)
    {
        Index p = t.parent[static_cast<std::size_t>(v)];
        if (p >= 0 && p < n)
        {
            int pv = pos[static_cast<std::size_t>(v)];
            if (pv >= 0)
                childPos[static_cast<std::size_t>(p)].push_back(pv);
        }
    }

    double sumSpan = 0.0;
    int cnt = 0;
    for (int u = 0; u < n; ++u)
    {
        auto &vec = childPos[static_cast<std::size_t>(u)];
        if (vec.size() < 2)
            continue;
        auto mm = std::minmax_element(vec.begin(), vec.end());
        sumSpan += static_cast<double>(*mm.second - *mm.first);
        cnt += 1;
    }
    if (cnt == 0)
        return 0.0;
    return sumSpan / static_cast<double>(cnt);
}

// 计算 Pearson 相关系数
double Metrics::pearsonCorr(const std::vector<double> &x,
                            const std::vector<double> &y)
{
    if (x.size() != y.size() || x.size() < 2)
        return 0.0;

    const std::size_t n = x.size();
    double mx = 0.0, my = 0.0;
    for (std::size_t i = 0; i < n; ++i)
    {
        mx += x[i];
        my += y[i];
    }
    mx /= static_cast<double>(n);
    my /= static_cast<double>(n);

    double num = 0.0, vx = 0.0, vy = 0.0;
    for (std::size_t i = 0; i < n; ++i)
    {
        double dx = x[i] - mx;
        double dy = y[i] - my;
        num += dx * dy;
        vx += dx * dx;
        vy += dy * dy;
    }
    if (vx <= 0.0 || vy <= 0.0)
        return 0.0;
    return num / std::sqrt(vx * vy);
}

bool Metrics::hasHighDegreeNode(Graph &graph)
{
    const int n = static_cast<int>(graph.getNodeCount());
    if (n <= 1)
        return false;

    DegreeStats st = degreeStats(graph);

    const double threshold = st.mean + HIGH_DEGREE_K * st.std;

    int cnt = 0;
    for (int v = 0; v < n; ++v)
    {
        double d = static_cast<double>(graph.getNeighbors(v).size());
        if (d >= threshold)
        {
            cnt++;
            if (cnt >= 2)
                return true; // |H| >= 2
        }
    }
    return false;
}

std::size_t Metrics::measureDFSMaxStackFromRoot(Graph &graph, std::vector<std::string> &order)
{
    order.clear();
    const int n = graph.getNodeCount();
    if (n == 0)
        return 0;

    std::stack<Index> st;
    std::vector<uint8_t> visited(n, 0);

    std::size_t maxSize = 0;

    st.push(ROOT);
    visited[ROOT] = 1;
    maxSize = std::max(maxSize, st.size());

    while (!st.empty())
    {
        Index cur = st.top();
        st.pop();

        order.push_back(graph.getNode(cur).label);

        for (Index adj : graph.getNeighbors(cur))
        {
            if (adj < 0 || adj >= n)
                continue; // 若你的 Index 保证范围可删
            if (!visited[adj])
            {
                visited[adj] = 1;
                st.push(adj);
                maxSize = std::max(maxSize, st.size());
            }
        }
    }
    return maxSize;
}

std::size_t Metrics::measureBFSMaxQueueFromRoot(Graph &graph, std::vector<std::string> &order)
{
    order.clear();
    const int n = graph.getNodeCount();
    if (n == 0)
        return 0;

    std::queue<Index> qu;
    std::vector<uint8_t> visited(n, 0);

    std::size_t maxSize = 0;

    qu.push(ROOT);
    visited[ROOT] = 1;
    maxSize = std::max(maxSize, qu.size());

    while (!qu.empty())
    {
        Index cur = qu.front();
        qu.pop();

        order.push_back(graph.getNode(cur).label);

        for (Index adj : graph.getNeighbors(cur))
        {
            if (adj < 0 || adj >= n)
                continue;
            if (!visited[adj])
            {
                visited[adj] = 1;
                qu.push(adj);
                maxSize = std::max(maxSize, qu.size());
            }
        }
    }
    return maxSize;
}


size_t Metrics::measureDFSMaxStackFromRoot(Graph &graph, std::vector<std::string> &order, Index root) {
    order.clear();
    const int n = graph.getNodeCount();
    if (n == 0)
        return 0;

    std::stack<Index> st;
    std::vector<uint8_t> visited(n, 0);

    std::size_t maxSize = 0;

    st.push(root);
    visited[root] = 1;
    maxSize = std::max(maxSize, st.size());

    while (!st.empty())
    {
        Index cur = st.top();
        st.pop();

        order.push_back(graph.getNode(cur).label);

        for (Index adj : graph.getNeighbors(cur))
        {
            if (adj < 0 || adj >= n)
                continue; // 若你的 Index 保证范围可删
            if (!visited[adj])
            {
                visited[adj] = 1;
                st.push(adj);
                maxSize = std::max(maxSize, st.size());
            }
        }
    }
    return maxSize;
}
size_t Metrics::measureBFSMaxQueueFromRoot(Graph &graph, std::vector<std::string> &order, Index root) {
    order.clear();
    const int n = graph.getNodeCount();
    if (n == 0)
        return 0;

    std::queue<Index> qu;
    std::vector<uint8_t> visited(n, 0);

    std::size_t maxSize = 0;

    qu.push(root);
    visited[root] = 1;
    maxSize = std::max(maxSize, qu.size());

    while (!qu.empty())
    {
        Index cur = qu.front();
        qu.pop();

        order.push_back(graph.getNode(cur).label);

        for (Index adj : graph.getNeighbors(cur))
        {
            if (adj < 0 || adj >= n)
                continue;
            if (!visited[adj])
            {
                visited[adj] = 1;
                qu.push(adj);
                maxSize = std::max(maxSize, qu.size());
            }
        }
    }
    return maxSize;
}

double Metrics::getLcsSimilarity(const std::vector<std::string> &orderA,
                                 const std::vector<std::string> &orderB)
{
    const std::size_t sizeA = orderA.size();
    const std::size_t sizeB = orderB.size();
    const std::size_t maxSize = std::max(sizeA, sizeB);

    if (maxSize == 0)
    {
        return 1.0;
    }
    if (sizeA == 0 || sizeB == 0)
    {
        return 0.0;
    }

    std::vector<int> prev(sizeB + 1, 0);
    std::vector<int> cur(sizeB + 1, 0);

    for (std::size_t i = 1; i <= sizeA; ++i)
    {
        for (std::size_t j = 1; j <= sizeB; ++j)
        {
            if (orderA[i - 1] == orderB[j - 1])
            {
                cur[j] = prev[j - 1] + 1;
            }
            else
            {
                cur[j] = std::max(prev[j], cur[j - 1]);
            }
        }
        std::swap(prev, cur);
        std::fill(cur.begin(), cur.end(), 0);
    }

    const double lcs = static_cast<double>(prev[sizeB]);
    return lcs / static_cast<double>(maxSize);
}

double Metrics::getKendallSimilarity(const std::vector<std::string> &orderA,
                                     const std::vector<std::string> &orderB)
{
    if (orderA.size() != orderB.size())
    {
        return 0.0;
    }

    const std::size_t n = orderA.size();
    if (n < 2)
    {
        return 1.0;
    }

    std::unordered_map<std::string, std::size_t> pos;
    pos.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        pos[orderB[i]] = i;
    }

    std::vector<std::size_t> sequence;
    sequence.reserve(n);
    for (const auto &item : orderA)
    {
        auto it = pos.find(item);
        if (it == pos.end())
        {
            return 0.0;
        }
        sequence.push_back(it->second);
    }

    std::vector<std::size_t> bit(n + 1, 0);
    auto bitAdd = [&bit](std::size_t idx)
    {
        for (++idx; idx < bit.size(); idx += idx & (~idx + 1))
        {
            bit[idx] += 1;
        }
    };
    auto bitSum = [&bit](std::size_t idx)
    {
        std::size_t sum = 0;
        for (++idx; idx > 0; idx -= idx & (~idx + 1))
        {
            sum += bit[idx];
        }
        return sum;
    };

    std::size_t inversions = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        const std::size_t value = sequence[i];
        const std::size_t seen = i;
        const std::size_t notGreater = bitSum(value);
        inversions += seen - notGreater;
        bitAdd(value);
    }

    const double totalPairs = static_cast<double>(n) * (n - 1) / 2.0;
    return 1.0 - 2.0 * static_cast<double>(inversions) / totalPairs;
}
