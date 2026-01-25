//对于一张十分简单的图，测量其平均遍历时间
#include <iostream>
#include <vector>
#include <limits>
#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"
#include "TraversalAlgo.hpp"
#include "ReGraph.hpp"
#include "Metrics.hpp"

int main() {
    using namespace std;

    // 构造邻接表图（无向图：双向加边）
    AdjListGraph listGraph;
    listGraph.addNode(Node(0, "0"));
    listGraph.addNode(Node(1, "1"));
    listGraph.addNode(Node(2, "2"));
    listGraph.addNode(Node(3, "3"));
    listGraph.addEdge(0, 1);
    listGraph.addEdge(1, 0);
    listGraph.addEdge(1, 2);
    listGraph.addEdge(2, 1);
    listGraph.addEdge(1, 3);
    listGraph.addEdge(3, 1);
    listGraph.addEdge(2, 3);
    listGraph.addEdge(3, 2);
    vector<AdjListGraph> listGraphs = ReGraph::reGraphAll(listGraph);

    // 构造邻接矩阵图（同样拓扑）
    AdjMatrixGraph matrixGraph;
    matrixGraph.addNode(Node(0, "0"));
    matrixGraph.addNode(Node(1, "1"));
    matrixGraph.addNode(Node(2, "2"));
    matrixGraph.addNode(Node(3, "3"));
    matrixGraph.addEdge(0, 1);
    matrixGraph.addEdge(1, 0);
    matrixGraph.addEdge(1, 2);
    matrixGraph.addEdge(2, 1);
    matrixGraph.addEdge(1, 3);
    matrixGraph.addEdge(3, 1);
    matrixGraph.addEdge(2, 3);
    matrixGraph.addEdge(3, 2);
    vector<AdjMatrixGraph> matrixGraphs = ReGraph::reGraphAll(matrixGraph);

    // 打印一下重排数量（应该是 4! = 24）
    cout << "AdjList permutations:   " << listGraphs.size()   << "\n";
    cout << "AdjMatrix permutations: " << matrixGraphs.size() << "\n\n";

    int repeatTimes = 100000;  // 重复次数，越大越稳定

    // 工具函数：打印 min / max / avg
    auto printStats = [](const string& title, const vector<double>& v) {
        if (v.empty()) {
            cout << title << ": empty\n\n";
            return;
        }
        double mn = numeric_limits<double>::max();
        double mx = numeric_limits<double>::lowest();
        double sum = 0.0;
        size_t idx_min = 0, idx_max = 0;

        for (size_t i = 0; i < v.size(); ++i) {
            double x = v[i];
            sum += x;
            if (x < mn) { mn = x; idx_min = i; }
            if (x > mx) { mx = x; idx_max = i; }
        }
        cout << title << ":\n";
        cout << "  count = " << v.size() << "\n";
        cout << "  min   = " << mn << " ns (perm index = " << idx_min << ")\n";
        cout << "  max   = " << mx << " ns (perm index = " << idx_max << ")\n";
        cout << "  avg   = " << (sum / v.size()) << " ns\n\n";
    };

    // 邻接表 BFS
    vector<double> listBfsRes;
    listBfsRes.reserve(listGraphs.size());
    for (auto& graph : listGraphs) {
        listBfsRes.push_back(Metrics::measureAveTraverlsalTime(
            graph,
            [](Graph& g) { TraversalAlgo::bfs(g); },
            repeatTimes
        ));
    }

    // 邻接表 DFS
    vector<double> listDfsRes;
    listDfsRes.reserve(listGraphs.size());
    for (auto& graph : listGraphs) {
        listDfsRes.push_back(Metrics::measureAveTraverlsalTime(
            graph,
            [](Graph& g) { TraversalAlgo::dfs(g); },
            repeatTimes
        ));
    }

    // 邻接矩阵 DFS
    vector<double> matrixDfsRes;
    matrixDfsRes.reserve(matrixGraphs.size());
    for (auto& graph : matrixGraphs) {
        matrixDfsRes.push_back(Metrics::measureAveTraverlsalTime(
            graph,
            [](Graph& g) { TraversalAlgo::dfs(g); },
            repeatTimes
        ));
    }

    // 邻接矩阵 BFS
    vector<double> matrixBfsRes;
    matrixBfsRes.reserve(matrixGraphs.size());
    for (auto& graph : matrixGraphs) {
        matrixBfsRes.push_back(Metrics::measureAveTraverlsalTime(
            graph,
            [](Graph& g) { TraversalAlgo::bfs(g); },
            repeatTimes
        ));
    }

    // 输出统计结果
    printStats("AdjList BFS",   listBfsRes);
    printStats("AdjList DFS",   listDfsRes);
    printStats("AdjMatrix BFS", matrixBfsRes);
    printStats("AdjMatrix DFS", matrixDfsRes);

    return 0;
}
