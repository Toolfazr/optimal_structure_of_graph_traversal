#include "AdjMatrixGraph.hpp"
#include <fstream>

using namespace std;

void AdjMatrixGraph::addNode(const Node& node) {
    // 防负数id
    if(node.index < 0) return;

    nodes.insert(node);
    labelToIndex[node.label] = node.index;
    // 扩充邻接矩阵
    Index newSize = node.index + 1;
    if(adjMatrix.empty()) {
        adjMatrix.assign(newSize, std::vector<Index>(newSize, 0));
    } else {
        Index oldSize = adjMatrix.size();
        if(oldSize < newSize) {
            adjMatrix.resize(newSize);
            for(auto& row : adjMatrix) {
                row.resize(newSize, 0);
            }
        }
    }
}

size_t AdjMatrixGraph::getNodeCount() const {
    return nodes.size();
}

Node AdjMatrixGraph::getNode(Index nodeId) const {
    Node res(-1, "none");
    auto it = nodes.find(Node(nodeId));
    if(it != nodes.end()) res = *it;
    return res;
}

Node AdjMatrixGraph::getNode(std::string label) const {
    return getNode(labelToIndex.at(label));
}

void AdjMatrixGraph::addEdge(Index from, Index to) {
    // 防负数id
    if (from < 0 || to < 0) return;

    // 检查图中是否有这两个node
    if(nodes.find(Node(from)) == nodes.end()) return;
    if(nodes.find(Node(to)) == nodes.end()) return;
    
    // 扩充邻接矩阵
    Index newSize = std::max(from, to) + 1;
    if(adjMatrix.empty()) {
        adjMatrix.assign(newSize, std::vector<Index>(newSize, 0));
    } else {
        Index oldSize = adjMatrix.size();
        if(oldSize < newSize) {
            adjMatrix.resize(newSize);
            for(auto& row : adjMatrix) {
                row.resize(newSize, 0);
            }
        }
    }

    adjMatrix[from][to] = 1;
}

void AdjMatrixGraph::removeEdge(Index from, Index to) {
    if(from < 0 || to < 0) return;

    Index newSize = std::max(from, to) + 1;
    if(newSize > adjMatrix.size()) return;

    adjMatrix[from][to] = 0;
}

bool AdjMatrixGraph::hasEdge(Index from, Index to) const {
    bool res = false;

    if(from < 0 || to < 0) return res;
    
    Index newSize = std::max(from, to) + 1;
    if(newSize > adjMatrix.size()) {
        res = false;
    } else {
        res = (adjMatrix[from][to] == 1) ? (true) : (false);
    }

    return res;
}

std::vector<Index> AdjMatrixGraph::getNeighbors(Index nodeId) const {
    std::vector<Index> res;
    
    // 检查图中有没有该node
    if(nodes.find(Node(nodeId)) == nodes.end()) return res;

    Index requiredSize = nodeId + 1;
    if(nodeId >= 0 && requiredSize <= adjMatrix.size()) {
        for(Index index = 0; index < adjMatrix.size(); index++) {
            // 允许自环
            if(adjMatrix[nodeId][index] == 1) {
                res.push_back(index);
            }
        }
    }

    return res;
}

void AdjMatrixGraph::setLabel(std::string label) {
    this->label = label;
}

std::string AdjMatrixGraph::getLabel() const {
    return label;
}

void AdjMatrixGraph::toCsv(const std::string& path) const {
    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        throw std::runtime_error("Failed to open csv file: " + path);
    }

    // header
    ofs << "node,degree,adjNodes\n";

    for(Index index = 0; index < getNodeCount(); index++) {
        Node node = getNode(index);
        vector<string> adjNodes;
        for(int i = 0; i < getNodeCount(); i++) {
            if(adjMatrix[index][i] == 1) {
                Node adjNode = getNode(i);
                adjNodes.push_back(adjNode.label);
            }
        }

        ofs << node.label << "," << adjNodes.size() << ",";
        int adjNodeNum = 0;
        for(string adjNodeLabel : adjNodes) {
            adjNodeNum++;
            ofs << adjNodeLabel;
            if(adjNodeNum != adjNodes.size()) ofs << ";";
        } 
        ofs << "\n";
    }
    ofs.close();
}