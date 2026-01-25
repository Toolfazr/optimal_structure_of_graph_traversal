// 测试Decomposition & Construction
#include "Construction.hpp"
#include "Decomposition.hpp"
#include "GraphGen.hpp"
#include "Metrics.hpp"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> toLabelOrder(const std::vector<Index>& rank) {
    std::vector<std::string> labels;
    labels.reserve(rank.size());
    for (Index id : rank) {
        labels.push_back(std::to_string(id));
    }
    return labels;
}

bool verifyBfsList(const std::vector<Index>& rank) {
    AdjListGraph graph = Construction::getListForBFS(rank);
    if (graph.getNodeCount() != rank.size()) {
        return false;
    }

    std::vector<std::string> order;
    Metrics::measureBFSMaxQueueFromRoot(graph, order);
    return order == toLabelOrder(rank);
}

bool verifyDfsList(const std::vector<Index>& rank) {
    AdjListGraph graph = Construction::getListForDFS(rank);
    if (graph.getNodeCount() != rank.size()) {
        return false;
    }

    std::vector<std::string> order;
    Metrics::measureDFSMaxStackFromRoot(graph, order);
    return order == toLabelOrder(rank);
}

bool verifyBfsMatrix(const std::vector<Index>& rank) {
    AdjMatrixGraph graph = Construction::getMatrixForBFS(rank);
    if (graph.getNodeCount() != rank.size()) {
        return false;
    }

    std::vector<std::string> order;
    Metrics::measureBFSMaxQueueFromRoot(graph, order);
    return order == toLabelOrder(rank);
}

bool verifyDfsMatrix(const std::vector<Index>& rank) {
    AdjMatrixGraph graph = Construction::getMatrixForDFS(rank);
    if (graph.getNodeCount() != rank.size()) {
        return false;
    }

    std::vector<std::string> order;
    Metrics::measureDFSMaxStackFromRoot(graph, order);
    return order == toLabelOrder(rank);
}

template <typename GraphT>
void runTrial(const std::string& label,
              GraphT& graph,
              const std::function<bool(const std::vector<Index>&)>& bfsChecker,
              const std::function<bool(const std::vector<Index>&)>& dfsChecker) {
    Decomposition decomposition;
    const std::vector<std::vector<Index>> ranks = decomposition.getRanks(graph);

    bool passDecomposition = !ranks.empty();
    bool passBfs = true;
    bool passDfs = true;

    const size_t expectedSize = graph.getNodeCount();
    for (const auto& rank : ranks) {
        if (rank.size() != expectedSize) {
            passDecomposition = false;
            passBfs = false;
            passDfs = false;
            break;
        }
        if (!bfsChecker(rank)) {
            passBfs = false;
        }
        if (!dfsChecker(rank)) {
            passDfs = false;
        }
        if (!passBfs && !passDfs) {
            break;
        }
    }

    std::cout << label << " | "
              << "Ranks: " << ranks.size() << " | "
              << "Decomposition: " << (passDecomposition ? "PASS" : "FAIL") << " | "
              << "BFS: " << (passBfs ? "PASS" : "FAIL") << " | "
              << "DFS: " << (passDfs ? "PASS" : "FAIL")
              << std::endl;
}
} // namespace

int main() {
    using namespace std;

    AdjListGraph list_star = GraphGen::makeStarAdjList(5);
    list_star.setLabel("List: Star");

    AdjListGraph list_grid = GraphGen::makeGridAdjList(2, 2);
    list_grid.setLabel("List: Grid");

    AdjMatrixGraph matrix_binary_tree = GraphGen::makeBinaryTreeAdjMatrix(5);
    matrix_binary_tree.setLabel("Matrix: Binary Tree");

    AdjMatrixGraph matrix_clique_tail = GraphGen::makeCliqueTailAdjMatrix(3, 2);
    matrix_clique_tail.setLabel("Matrix: Clique Tail");

    runTrial("Decomposition & Construction | " + list_star.getLabel(),
             list_star,
             verifyBfsList,
             verifyDfsList);

    runTrial("Decomposition & Construction | " + list_grid.getLabel(),
             list_grid,
             verifyBfsList,
             verifyDfsList);

    runTrial("Decomposition & Construction | " + matrix_binary_tree.getLabel(),
             matrix_binary_tree,
             verifyBfsMatrix,
             verifyDfsMatrix);

    runTrial("Decomposition & Construction | " + matrix_clique_tail.getLabel(),
             matrix_clique_tail,
             verifyBfsMatrix,
             verifyDfsMatrix);

    return 0;
}
