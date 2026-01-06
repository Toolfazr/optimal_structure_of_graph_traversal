#include "GraphGen.hpp"
#include "Metrics.hpp"
#include "ReGraph.hpp"
#include "MetricsResStorage.hpp"
#include "BestSpaceConstruction.hpp"
#include "Constants.hpp"
#include <iostream>

int main() {
    using namespace std;
    AdjListGraph list_binary_tree = GraphGen::makeBinaryTreeAdjList(9);
    list_binary_tree.setLabel("List: Binary Tree");

    AdjListGraph list_star = GraphGen::makeStarAdjList(9);
    list_star.setLabel("List: Star");

    AdjListGraph list_grid = GraphGen::makeGridAdjList(3, 3);
    list_grid.setLabel("List: Grid");

    AdjListGraph list_clique = GraphGen::makeCliqueTailAdjList(5, 4);
    list_clique.setLabel("List: Clique");

    AdjMatrixGraph matrix_clique = GraphGen::makeCliqueTailAdjMatrix(5, 4);
    matrix_clique.setLabel("Matrix: Clique");

    AdjMatrixGraph matrix_grid = GraphGen::makeGridAdjMatrix(3, 3);
    matrix_grid.setLabel("Matrix: Grid");

    AdjMatrixGraph matrix_star = GraphGen::makeStarAdjMatrix(9);
    matrix_star.setLabel("Matrix: Star");

    AdjMatrixGraph matrix_binary_tree = GraphGen::makeBinaryTreeAdjMatrix(9);
    matrix_binary_tree.setLabel("Matrix: Binary Tree");

    vector<AdjListGraph> listGraphs;
    vector<AdjMatrixGraph> matrixGraphs;

    listGraphs.push_back(list_binary_tree);
    listGraphs.push_back(list_star);
    listGraphs.push_back(list_grid);
    listGraphs.push_back(list_clique);
    matrixGraphs.push_back(matrix_clique);
    matrixGraphs.push_back(matrix_grid);
    matrixGraphs.push_back(matrix_star);
    matrixGraphs.push_back(matrix_binary_tree);

    // ===== AdjListGraph: BFS / DFS 分开判定 =====
    for (AdjListGraph& listGraph : listGraphs) {
        AdjListGraph bestGraph = BestSpaceConstruction::getBestSpaceConstruction(listGraph);

        RootOptResult bestBfs = Metrics::measureBFSMaxQueue(bestGraph);
        RootOptResult bestDfs = Metrics::measureDFSMaxStack(bestGraph);

        ReGraph::Enumerator<AdjListGraph> reGrapher(listGraph);
        AdjListGraph newGraph;

        bool passBfs = true;
        bool passDfs = true;

        while (reGrapher.next(newGraph)) {
            RootOptResult curBfs = Metrics::measureBFSMaxQueue(newGraph);
            if (bestBfs.bestPeak > curBfs.bestPeak) {
                passBfs = false;
            }

            RootOptResult curDfs = Metrics::measureDFSMaxStack(newGraph);
            if (bestDfs.bestPeak > curDfs.bestPeak) {
                passDfs = false;
            }

            if (!passBfs && !passDfs) break;
        }

        std::cout << listGraph.getLabel() << " | "
                << "BFS: " << (passBfs ? "PASS" : "FAIL") << " (Queue Peak: " << bestBfs.bestPeak << ") "
                << "DFS: " << (passDfs ? "PASS" : "FAIL") << " (Stack Peak: " << bestDfs.bestPeak << ")"
                << std::endl;
    }

    // ===== AdjMatrixGraph: BFS / DFS 分开判定 =====
    for (AdjMatrixGraph& matrixGraph : matrixGraphs) {
        AdjMatrixGraph bestGraph = BestSpaceConstruction::getBestSpaceConstruction(matrixGraph);

        RootOptResult bestBfs = Metrics::measureBFSMaxQueue(bestGraph);
        RootOptResult bestDfs = Metrics::measureDFSMaxStack(bestGraph);

        ReGraph::Enumerator<AdjMatrixGraph> reGrapher(matrixGraph);
        AdjMatrixGraph newGraph;

        bool passBfs = true;
        bool passDfs = true;

        while (reGrapher.next(newGraph)) {
            RootOptResult curBfs = Metrics::measureBFSMaxQueue(newGraph);
            if (bestBfs.bestPeak > curBfs.bestPeak) {
                passBfs = false;
            }

            RootOptResult curDfs = Metrics::measureDFSMaxStack(newGraph);
            if (bestDfs.bestPeak > curDfs.bestPeak) {
                passDfs = false;
            }

            if (!passBfs && !passDfs) break;
        }

        std::cout << matrixGraph.getLabel() << " | "
                << "BFS: " << (passBfs ? "PASS" : "FAIL") << " (Queue Peak: " << bestBfs.bestPeak << ") "
                << "DFS: " << (passDfs ? "PASS" : "FAIL") << " (Stack Peak: " << bestDfs.bestPeak << ")"
                << std::endl;
    }


    return 0;
}