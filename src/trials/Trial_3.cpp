#include "GraphGen.hpp"
#include "Metrics.hpp"
#include "ReGraph.hpp"
#include "MetricsResStorage.hpp"
#include "Constants.hpp"

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

    MetricsResStorage res;
    for(AdjListGraph& listGraph : listGraphs) {
        ReGraph::Enumerator<AdjListGraph> reGrapher(listGraph);
        AdjListGraph newGraph;
        while(reGrapher.next(newGraph)) {
            newGraph.setLabel(listGraph.getLabel());
            MetricsResStorage entry = Utility::doSpaceMeasure(newGraph);
            res.merge(entry);
            if(res.getResGroupSize() >= FLUSH_CONTROL) {
                Utility::saveRes("./TrialRes/SpaceTrial/res.csv", res);
                res.clear();
            }
        }
    }

    for(AdjMatrixGraph& matrixGraph : matrixGraphs) {
        ReGraph::Enumerator<AdjMatrixGraph> reGrapher(matrixGraph);
        AdjMatrixGraph newGraph;
        while(reGrapher.next(newGraph)) {
            newGraph.setLabel(matrixGraph.getLabel());
            MetricsResStorage entry = Utility::doSpaceMeasure(newGraph);
            res.merge(entry);
            if(res.getResGroupSize() >= FLUSH_CONTROL) {
                Utility::saveRes("./TrialRes/SpaceTrial/res.csv", res);
                res.clear();
            }
        }
    }

    if (res.getResGroupSize() > 0) {
        Utility::saveRes("./TrialRes/SpaceTrial/res.csv", res);
        res.clear(); // 或 clear()
    }
    return 0;
}