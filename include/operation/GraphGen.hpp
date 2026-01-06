#pragma once

#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"

// Graph generator utilities for experiments (undirected graphs are created by adding edges in both directions).
// Node ids are 0..n-1 and labels are stringified ids.

class GraphGen {
public:
    // Star graph: center 0 connected to all 1..n-1
    static AdjListGraph makeStarAdjList(int n);
    static AdjMatrixGraph makeStarAdjMatrix(int n);

    // Grid graph: w x h, 4-neighbor (up/down/left/right). id = r*w + c
    static AdjListGraph makeGridAdjList(int w, int h);
    static AdjMatrixGraph makeGridAdjMatrix(int w, int h);

    // Clique + Tail (lollipop): cliqueSize nodes fully connected (0..cliqueSize-1),
    // then a path of length tailLen attached to (cliqueSize-1).
    static AdjListGraph makeCliqueTailAdjList(int cliqueSize, int tailLen);
    static AdjMatrixGraph makeCliqueTailAdjMatrix(int cliqueSize, int tailLen);

    // Binary tree in array form: for node i, children are 2i+1 and 2i+2 if < n.
    static AdjListGraph makeBinaryTreeAdjList(int n);
    static AdjMatrixGraph makeBinaryTreeAdjMatrix(int n); 
}; 
