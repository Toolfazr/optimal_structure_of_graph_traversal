#include "GraphGen.hpp"

#include <string>
#include <stdexcept>

namespace 
{
    template <typename G>
    static inline void addUndirectedEdge(G& g, Index u, Index v) {
        g.addEdge(u, v);
        g.addEdge(v, u);
    }

    template <typename G>
    static inline void addNodes0toNMinus1(G& g, int n) {
        for (int i = 0; i < n; ++i) {
            g.addNode(Node(static_cast<Index>(i), std::to_string(i)));
        }
    }

    template <typename G>
    static G makeStar(int n) {
        if (n <= 0) throw std::invalid_argument("makeStar: n must be > 0");
        G g;
        addNodes0toNMinus1(g, n);
        for (int i = 1; i < n; ++i) {
            addUndirectedEdge(g, static_cast<Index>(0), static_cast<Index>(i));
        }
        return g;
    }

    template <typename G>
    static G makeGrid(int w, int h) {
        if (w <= 0 || h <= 0) throw std::invalid_argument("makeGrid: w and h must be > 0");
        int n = w * h;

        G g;
        addNodes0toNMinus1(g, n);

        auto id = [w](int r, int c) { return r * w + c; };

        for (int r = 0; r < h; ++r) {
            for (int c = 0; c < w; ++c) {
                int u = id(r, c);
                if (c + 1 < w) addUndirectedEdge(g, static_cast<Index>(u), static_cast<Index>(id(r, c + 1)));
                if (r + 1 < h) addUndirectedEdge(g, static_cast<Index>(u), static_cast<Index>(id(r + 1, c)));
            }
        }
        return g;
    }

    template <typename G>
    static G makeCliqueTail(int cliqueSize, int tailLen) {
        if (cliqueSize <= 0) throw std::invalid_argument("makeCliqueTail: cliqueSize must be > 0");
        if (tailLen < 0) throw std::invalid_argument("makeCliqueTail: tailLen must be >= 0");

        int n = cliqueSize + tailLen;

        G g;
        addNodes0toNMinus1(g, n);

        // clique: 0..cliqueSize-1 fully connected
        for (int i = 0; i < cliqueSize; ++i) {
            for (int j = i + 1; j < cliqueSize; ++j) {
                addUndirectedEdge(g, static_cast<Index>(i), static_cast<Index>(j));
            }
        }

        // tail: (cliqueSize-1) - cliqueSize - ... - (n-1)
        if (tailLen > 0) {
            int prev = cliqueSize - 1;
            for (int v = cliqueSize; v < n; ++v) {
                addUndirectedEdge(g, static_cast<Index>(prev), static_cast<Index>(v));
                prev = v;
            }
        }

        return g;
    }

    template <typename G>
    static G makeBinaryTree(int n) {
        if (n <= 0) throw std::invalid_argument("makeBinaryTree: n must be > 0");

        G g;
        addNodes0toNMinus1(g, n);

        for (int i = 0; i < n; ++i) {
            int l = 2 * i + 1;
            int r = 2 * i + 2;
            if (l < n) addUndirectedEdge(g, static_cast<Index>(i), static_cast<Index>(l));
            if (r < n) addUndirectedEdge(g, static_cast<Index>(i), static_cast<Index>(r));
        }
        return g;
    }

}

AdjListGraph GraphGen::makeStarAdjList(int n) { 
    return makeStar<AdjListGraph>(n); 
}

AdjMatrixGraph GraphGen::makeStarAdjMatrix(int n) { 
    return makeStar<AdjMatrixGraph>(n); 
}

AdjListGraph GraphGen::makeGridAdjList(int w, int h) { 
    return makeGrid<AdjListGraph>(w, h); 
}

AdjMatrixGraph GraphGen::makeGridAdjMatrix(int w, int h) { 
    return makeGrid<AdjMatrixGraph>(w, h); 
}

AdjListGraph GraphGen::makeCliqueTailAdjList(int cliqueSize, int tailLen) {
    return makeCliqueTail<AdjListGraph>(cliqueSize, tailLen);
}

AdjMatrixGraph GraphGen::makeCliqueTailAdjMatrix(int cliqueSize, int tailLen) {
    return makeCliqueTail<AdjMatrixGraph>(cliqueSize, tailLen);
}

AdjListGraph GraphGen::makeBinaryTreeAdjList(int n) { 
    return makeBinaryTree<AdjListGraph>(n); 
}

AdjMatrixGraph GraphGen::makeBinaryTreeAdjMatrix(int n) { 
    return makeBinaryTree<AdjMatrixGraph>(n); 
}
