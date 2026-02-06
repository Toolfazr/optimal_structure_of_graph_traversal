#include "AdjListGraph.hpp"
#include <fstream>
#include <sstream>

using namespace std;

void AdjListGraph::addNode(const Node& node) {
    if(node.index < 0) return;

    nodes.insert(node);
    labelToIndex[node.label] = node.index;
    if(adjList.count(node.index) == 0) {
        adjList.insert({node.index, std::vector<Index>()});
    }
}

size_t AdjListGraph::getNodeCount() const {
    return nodes.size();
}

Node AdjListGraph::getNode(Index nodeId) const {
    Node res(-1, "none");
    auto it = nodes.find(Node(nodeId));
    if(it != nodes.end()) res = *it;
    return res;
}

Node AdjListGraph::getNode(std::string label) const {
    return getNode(labelToIndex.at(label));
}

void AdjListGraph::addEdge(Index from, Index to) {
    // 防负数id
    if (from < 0 || to < 0) return;

    // 检查图中是否有这两个node
    if(nodes.find(Node(from)) == nodes.end()) return;
    if(nodes.find(Node(to)) == nodes.end()) return;

    // 检查是否加了重复边
    bool edgeExisted = false;
    for(Index index : adjList[from]) {
        if(index == to) edgeExisted = true;
    }

    if(!edgeExisted) adjList[from].push_back(to);
}

void AdjListGraph::removeEdge(Index from, Index to) {
    if(from < 0 || to < 0) return;
    
    // 检查图中是否有from对应的node
    auto it = nodes.find(Node(from));
    if(it != nodes.end()) {
        auto& neighbors = adjList[it->index];
        for (auto iter = neighbors.begin(); iter != neighbors.end(); ++iter) {
            if (*iter == to) {
                neighbors.erase(iter);
                break;
            }
        }
    }
}

bool AdjListGraph::hasEdge(Index from, Index to) const {
    if(from < 0 || to < 0) return false;
    
    // 检查图中是否有from对应的node
    auto it = nodes.find(Node(from));
    if(it != nodes.end()) {
        auto& neighbors = adjList.at(it->index);
        for (auto iter = neighbors.begin(); iter != neighbors.end(); ++iter) {
            if (*iter == to) {
                return true;
            }
        }
    }

    return false;
}

std::vector<Index> AdjListGraph::getNeighbors(Index nodeId) const {
    std::vector<Index> res;

    // 检查图中有没有该node
    auto it = nodes.find(Node(nodeId));
    if(it == nodes.end()) return res;
    
    res = adjList.at(it->index);
    return res;
}

void AdjListGraph::setLabel(std::string label) {
    this->label = label;
}

std::string AdjListGraph::getLabel() const {
    return label;
}

void AdjListGraph::toCsv(const std::string& path) const {
    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        throw std::runtime_error("Failed to open csv file: " + path);
    }

    // header
    ofs << "node,degree,adjNodes\n";
    for(const auto &[index, adjNodes] : adjList) {
        Node node = getNode(index);
        ofs << node.label << "," << adjNodes.size() << ",";
        int adjNodeNum = 0;
        for(Index adjIndex : adjNodes) {
            adjNodeNum++;
            Node adjNode = getNode(adjIndex);
            ofs << adjNode.label;
            if(adjNodeNum != adjNodes.size()) ofs << ";";
        } 
        ofs << "\n";
    }

    ofs.close();
}